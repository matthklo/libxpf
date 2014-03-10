/*******************************************************************************
 * Copyright (c) 2013 matt@moregeek.com.tw
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 *
 * 3. This notice may not be removed or altered from any source
 *    distribution.
 ********************************************************************************/

#include <xpf/netiomux.h>

#ifdef _XPF_NETIOMUX_IMPL_INCLUDED_
#error Multiple NetIoMux implementation files included
#else
#define _XPF_NETIOMUX_IMPL_INCLUDED_
#endif

#include "netiomux_syncfifo.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

namespace xpf
{

	struct EpollOverlapped
	{
		NetIoMux::EIoType iotype;
		NetEndpoint *sep;
		NetEndpoint *tep;
		c8	*buffer;
		u32	length;
		NetEndpoint::Peer *peer;
		NetIoMuxCallback *cb;
	};

	class NetIoMuxImpl
	{
	public:
		NetIoMuxImpl()
			: mEnable(true)
		{
			xpfSAssert(sizeof(socklen_t) == sizeof(s32));

			mEpollfd = epoll_create1(0);
			xpfAssert(mEpollfd != -1);
			if (mEpollfd == -1)
				mEnable = false;
		}

		~NetIoMuxImpl()
		{
			enable(false);

			if (mEpollfd != -1)
				close(mEpollfd);
			mEpollfd = -1;
		}

		void enable(bool val)
		{
            mEnable = val;
		}

		void run()
		{
            while (mEnable)
            {
                if (NetIoMux::ERS_DISABLED == runOnce(10))
                    break;
            }
		}

		NetIoMux::ERunningStaus runOnce(u32 timeoutMs)
		{
			return NetIoMux::ERS_NORMAL;
		}

		void asyncRecv(NetEndpoint *ep, c8 *buf, u32 buflen, NetIoMuxCallback *cb)
		{
			const NetIoMux::EIoType iotype = NetIoMux::EIT_RECV;

			const NetEndpoint::EStatus stat = ep->getStatus();
			xpfAssert(("Invalid socket status.", NetEndpoint::ESTAT_CONNECTED == stat));
			if (NetEndpoint::ESTAT_CONNECTED != stat)
			{
				cb->onIoCompleted(iotype, NetEndpoint::EE_INVALID_OP, ep, 0, buf, 0);
				return;
			}

			ssize_t bytes = recv(ep->getSocket(), buf, (size_t)buflen, MSG_DONTWAIT);
			if ((bytes != -1) || (errno == EAGAIN || errno == EWOULDBLOCK))
			{
				EpollOverlapped *o = new EpollOverlapped;
				o->iotype = iotype;
				o->sep = ep;
				o->tep = 0;
				o->buffer = buf;
				o->cb = cb;
				o->peer = 0;
				if (bytes != -1) // data is available and has been read
				{
					o->length = (u32)bytes;
					mReadyList.push_back((void*)o);
				}
				else // data is not available, therefore we should ask epoll to wait for any.
				{
					struct epoll_event ev;
					ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
					ev.data.ptr = (void*)o;

					o->length = buflen;
					int ec = epoll_ctl(mEpollfd, EPOLL_CTL_MOD, ep->getSocket(), &ev);
					xpfAssert(ec == 0);
					if (ec != 0)
					{
						cb->onIoCompleted(iotype, NetEndpoint::EE_RECV, ep, 0, buf, 0);
					}
				}
			}
			else
			{
				// i/o error
				cb->onIoCompleted(iotype, NetEndpoint::EE_RECV, ep, 0, buf, 0);
			}
		}

		void asyncRecvFrom(NetEndpoint *ep, c8 *buf, u32 buflen, NetIoMuxCallback *cb)
		{
			const NetIoMux::EIoType iotype = NetIoMux::EIT_RECVFROM;

			const NetEndpoint::EStatus stat = ep->getStatus();
			xpfAssert(("Invalid socket status.", NetEndpoint::ESTAT_CONNECTED == stat));
			if (NetEndpoint::ESTAT_CONNECTED != stat)
			{
				cb->onIoCompleted(iotype, NetEndpoint::EE_INVALID_OP, ep, 0, buf, 0);
				return;
			}

			NetEndpoint::Peer *peer = new NetEndpoint::Peer;

			ssize_t bytes = recvfrom(ep->getSocket(), buf, (size_t)buflen, MSG_DONTWAIT, (struct sockaddr*)peer->Data, (socklen_t*)&peer->Length);
			if ((bytes != -1) || (errno == EAGAIN || errno == EWOULDBLOCK))
			{
				EpollOverlapped *o = new EpollOverlapped;
				o->iotype = iotype;
				o->sep = ep;
				o->tep = 0;
				o->buffer = buf;
				o->cb = cb;
				o->peer = peer;
				if (bytes != -1) // data is available and has been read
				{
					o->length = (u32)bytes;
					mReadyList.push_back((void*)o);
				}
				else // data is not available, therefore we should ask epoll to wait for any.
				{
					struct epoll_event ev;
					ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
					ev.data.ptr = (void*)o;

					o->length = buflen;
					int ec = epoll_ctl(mEpollfd, EPOLL_CTL_MOD, ep->getSocket(), &ev);
					xpfAssert(ec == 0);
					if (ec != 0)
					{
						delete peer;
						cb->onIoCompleted(iotype, NetEndpoint::EE_RECV, ep, 0, buf, 0);
					}
				}
			}
			else
			{
				// i/o error
				delete peer;
				cb->onIoCompleted(iotype, NetEndpoint::EE_RECV, ep, 0, buf, 0);
			}
		}

		void asyncSend(NetEndpoint *ep, const c8 *buf, u32 buflen, NetIoMuxCallback *cb)
		{
			const NetIoMux::EIoType iotype = NetIoMux::EIT_SEND;

			const NetEndpoint::EStatus stat = ep->getStatus();
			xpfAssert(("Invalid socket status.", NetEndpoint::ESTAT_CONNECTED == stat));
			if (NetEndpoint::ESTAT_CONNECTED != stat)
			{
				cb->onIoCompleted(iotype, NetEndpoint::EE_INVALID_OP, ep, 0, buf, 0);
				return;
			}

			ssize_t bytes = send(ep->getSocket(), buf, (size_t)buflen, MSG_DONTWAIT);
			if ((bytes != -1) || (errno == EAGAIN || errno == EWOULDBLOCK))
			{
				EpollOverlapped *o = new EpollOverlapped;
				o->iotype = iotype;
				o->sep = ep;
				o->tep = 0;
				o->buffer = (c8*)buf;
				o->cb = cb;
				o->peer = 0;
				if (bytes != -1) // data has been sent
				{
					o->length = (u32)bytes;
					mReadyList.push_back((void*)o);
				}
				else // yet available for data sending, ask epoll to wait.
				{
					struct epoll_event ev;
					ev.events = EPOLLOUT | EPOLLET | EPOLLONESHOT;
					ev.data.ptr = (void*)o;

					o->length = buflen;
					int ec = epoll_ctl(mEpollfd, EPOLL_CTL_MOD, ep->getSocket(), &ev);
					xpfAssert(ec == 0);
					if (ec != 0)
					{
						cb->onIoCompleted(iotype, NetEndpoint::EE_SEND, ep, 0, buf, 0);
					}
				}
			}
			else
			{
				// i/o error
				cb->onIoCompleted(iotype, NetEndpoint::EE_SEND, ep, 0, buf, 0);
			}
		}

		void asyncSendTo(NetEndpoint *ep, const NetEndpoint::Peer *peer, const c8 *buf, u32 buflen, NetIoMuxCallback *cb)
		{
			const NetIoMux::EIoType iotype = NetIoMux::EIT_SENDTO;

			const NetEndpoint::EStatus stat = ep->getStatus();
			xpfAssert(("Invalid socket status.", NetEndpoint::ESTAT_CONNECTED == stat));
			if (NetEndpoint::ESTAT_CONNECTED != stat)
			{
				cb->onIoCompleted(iotype, NetEndpoint::EE_INVALID_OP, ep, (vptr)peer, buf, 0);
				return;
			}

			NetEndpoint::Peer *p = new NetEndpoint::Peer(*peer); // clone

			ssize_t bytes = sendto(ep->getSocket(), buf, (size_t)buflen, MSG_DONTWAIT,
					(const struct sockaddr*)p->Data, (socklen_t)p->Length);
			if ((bytes != -1) || (errno == EAGAIN || errno == EWOULDBLOCK))
			{
				EpollOverlapped *o = new EpollOverlapped;
				o->iotype = iotype;
				o->sep = ep;
				o->tep = 0;
				o->buffer = (c8*)buf;
				o->cb = cb;
				o->peer = p;
				if (bytes != -1) // data has been sent
				{
					o->length = (u32)bytes;
					mReadyList.push_back((void*)o);
				}
				else // yet available for data sending, ask epoll to wait.
				{
					struct epoll_event ev;
					ev.events = EPOLLOUT | EPOLLET | EPOLLONESHOT;
					ev.data.ptr = (void*)o;

					o->length = buflen;
					int ec = epoll_ctl(mEpollfd, EPOLL_CTL_MOD, ep->getSocket(), &ev);
					xpfAssert(ec == 0);
					if (ec != 0)
					{
						delete p;
						cb->onIoCompleted(iotype, NetEndpoint::EE_SEND, ep, (vptr)peer, buf, 0);
					}
				}
			}
			else
			{
				// i/o error
				delete p;
				cb->onIoCompleted(iotype, NetEndpoint::EE_SEND, ep, (vptr)peer, buf, 0);
			}
		}

		void asyncAccept(NetEndpoint *ep, NetIoMuxCallback *cb)
		{
			const NetIoMux::EIoType iotype = NetIoMux::EIT_ACCEPT;

			const NetEndpoint::EStatus stat = ep->getStatus();
			xpfAssert(("Invalid socket status.", NetEndpoint::ESTAT_LISTENING == stat));
			if (NetEndpoint::ESTAT_LISTENING != stat)
			{
				cb->onIoCompleted(iotype, NetEndpoint::EE_INVALID_OP, ep, 0, 0, 0);
				return;
			}

			NetEndpoint::Peer *peer = new NetEndpoint::Peer;
			int peersock = accept(ep->getSocket(), (struct sockaddr*)peer->Data, (socklen_t*)&peer->Length);
			if ((peersock != -1) || (errno == EAGAIN || errno == EWOULDBLOCK))
			{
				EpollOverlapped *o = new EpollOverlapped;
				o->iotype = iotype;
				o->sep = ep;
				o->buffer = 0;
				o->length = 0;
				o->cb = cb;
				o->peer = peer;
				if (peersock != -1) // new connection accepted
				{
					o->tep = new NetEndpoint(ep->getProtocol(), peersock, NetEndpoint::ESTAT_CONNECTED);
					join(o->tep);
					mReadyList.push_back((void*)o);
				}
				else // not incoming connection available, ask epoll to wait
				{
					struct epoll_event ev;
					ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
					ev.data.ptr = (void*)o;

					o->tep = 0;
					int ec = epoll_ctl(mEpollfd, EPOLL_CTL_MOD, ep->getSocket(), &ev);
					xpfAssert(ec == 0);
					if (ec != 0)
					{
						delete peer;
						cb->onIoCompleted(iotype, NetEndpoint::EE_ACCEPT, ep, 0, 0, 0);
					}
				}
			}
			else
			{
				// i/o error
				delete peer;
				cb->onIoCompleted(iotype, NetEndpoint::EE_ACCEPT, ep, 0, 0, 0);
			}
		}

		void asyncConnect(NetEndpoint *ep, const c8 *host, u32 port, NetIoMuxCallback *cb)
		{
			const NetIoMux::EIoType iotype = NetIoMux::EIT_CONNECT;

			const NetEndpoint::EStatus stat = ep->getStatus();
			xpfAssert(("Invalid socket status.", NetEndpoint::ESTAT_INIT == stat));
			if (NetEndpoint::ESTAT_INIT != stat)
			{
				cb->onIoCompleted(iotype, NetEndpoint::EE_INVALID_OP, ep, 0, 0, 0);
				return;
			}

			NetEndpoint::Peer *peer = new NetEndpoint::Peer;
			if (!NetEndpoint::resolvePeer(ep->getProtocol(), *peer, host, 0, port))
			{
				cb->onIoCompleted(iotype, NetEndpoint::EE_RESOLVE, ep, 0, 0, 0);
				return;
			}

			int ec = connect(ep->getSocket(), (const struct sockaddr*)peer->Data, (socklen_t)peer->Length);
			if ((ec == 0) || (errno == EINPROGRESS))
			{
				EpollOverlapped *o = new EpollOverlapped;
				o->iotype = iotype;
				o->sep = ep;
				o->tep = 0;
				o->buffer = 0;
				o->length = 0;
				o->cb = cb;
				o->peer = peer;

				if (ec == 0)
				{
					mReadyList.push_back((void*)o);
				}
				else
				{
					struct epoll_event ev;
					ev.events = EPOLLOUT | EPOLLET | EPOLLONESHOT;
					ev.data.ptr = (void*)o;

					int ec = epoll_ctl(mEpollfd, EPOLL_CTL_MOD, ep->getSocket(), &ev);
					xpfAssert(ec == 0);
					if (ec != 0)
					{
						delete peer;
						cb->onIoCompleted(iotype, NetEndpoint::EE_CONNECT, ep, 0, 0, 0);
					}
				}
			}
			else
			{
				// i/o error
				delete peer;
				cb->onIoCompleted(iotype, NetEndpoint::EE_CONNECT, ep, 0, 0, 0);
			}
		}

		bool join(NetEndpoint *ep)
		{
            s32 sock = ep->getSocket();
            int flags = fcntl(sock, F_GETFL);
            xpfAssert(flags != 0xffffffff);
            fcntl(sock, F_SETFL, flags | O_NONBLOCK);

			// add given endpoint to epoll group without reacting to any event.
            struct epoll_event ev;
            ev.events = 0;
            ev.data.ptr = 0;
            int ec = epoll_ctl(mEpollfd, EPOLL_CTL_ADD, sock, &ev);
            xpfAssert(ec == 0);

			return (ec == 0);
		}

		bool depart(NetEndpoint *ep)
		{
			s32 sock = ep->getSocket();

			// remove given endpoint from epoll group
			struct epoll_event dummy;
			int ec = epoll_ctl(mEpollfd, EPOLL_CTL_DEL, sock, &dummy); // dummy is not used but cannot be NULL
			xpfAssert(ec == 0);

            int flags = fcntl(sock, F_GETFL);
            xpfAssert(flags != 0xffffffff);
            fcntl(sock, F_SETFL, flags & (0 ^ O_NONBLOCK));
			return (ec == 0);
		}

		static const char * getMultiplexerType(NetIoMux::EPlatformMultiplexer &epm)
		{
			epm = NetIoMux::EPM_EPOLL;
			return "epoll";
		}

	private:
		NetIoMuxSyncFifo mReadyList;
		bool mEnable;
		int mEpollfd;
	}; // end of class NetIoMuxImpl (epoll)

} // end of namespace xpf

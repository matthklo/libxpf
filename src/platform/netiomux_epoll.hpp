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

#define MAX_EPOLL_EVENTS_AT_ONCE (128)
#define MAX_EPOLL_READY_LIST_LEN (10240)

#define ASYNC_OP_READ  (0)
#define ASYNC_OP_WRITE (1)

namespace xpf
{

	// data record per operation
	struct EpollOverlapped
	{
		EpollOverlapped(NetEndpoint *ep, NetIoMux::EIoType iocode)
			: iotype(iocode), sep(ep), tep(0), buffer(0), length(0), peer(0), cb(0), errorcode(0) {}

		NetIoMux::EIoType iotype;
		NetEndpoint *sep;
		NetEndpoint *tep;
		c8	*buffer;
		u32	length;
		NetEndpoint::Peer *peer;
		NetIoMuxCallback *cb;
		int errorcode;
	};

	// data record per socket
	struct EpollAsyncContext
	{
		std::deque<EpollOverlapped*>  rdqueue; // queued read operations
		std::deque<EpollOverlapped*>  wrqueue; // queued write operations
		bool                          ready;
		ThreadLock                    lock;
		NetEndpoint                  *ep;
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
			// Process the completion queue. Emit the completion event.
			u32 remaining = 0;
			EpollOverlapped *co = (EpollOverlapped*) mCompletionList.pop_front(remaining);
			if (co)
			{
				switch (co->iotype)
				{
				case NetIoMux::EIT_RECV:
					if (co->errorcode == 0)
						co->cb->onIoCompleted(co->iotype, NetEndpoint::EE_SUCCESS, co->sep, 0, co->buffer, co->length);
					else
						co->cb->onIoCompleted(co->iotype, NetEndpoint::EE_RECV, co->sep, 0, co->buffer, 0);
					break;
				case NetIoMux::EIT_RECVFROM:
					if (co->errorcode == 0)
						co->cb->onIoCompleted(co->iotype, NetEndpoint::EE_SUCCESS, co->sep, (vptr)co->peer, co->buffer, co->length);
					else
						co->cb->onIoCompleted(co->iotype, NetEndpoint::EE_RECV, co->sep, 0, co->buffer, 0);
					delete co->peer;
					break;
				case NetIoMux::EIT_SEND:
					if (co->errorcode == 0)
						co->cb->onIoCompleted(co->iotype, NetEndpoint::EE_SUCCESS, co->sep, 0, co->buffer, co->length);
					else
						co->cb->onIoCompleted(co->iotype, NetEndpoint::EE_SEND, co->sep, 0, co->buffer, 0);
					break;
				case NetIoMux::EIT_SENDTO:
					if (co->errorcode == 0)
						co->cb->onIoCompleted(co->iotype, NetEndpoint::EE_SUCCESS, co->sep, (vptr)co->peer, co->buffer, co->length);
					else
						co->cb->onIoCompleted(co->iotype, NetEndpoint::EE_SEND, co->sep, (vptr)co->peer, co->buffer, 0);
					delete co->peer;
					break;
				case NetIoMux::EIT_ACCEPT:
					if (co->errorcode == 0)
						co->cb->onIoCompleted(co->iotype, NetEndpoint::EE_SUCCESS, co->sep, (vptr)co->tep, 0, 0);
					else
						co->cb->onIoCompleted(co->iotype, NetEndpoint::EE_ACCEPT, co->sep, 0, 0, 0);
					break;
				case NetIoMux::EIT_CONNECT:
					if (co->errorcode == 0)
						co->cb->onIoCompleted(co->iotype, NetEndpoint::EE_SUCCESS, co->sep, (vptr)co->peer, 0, 0);
					else
						co->cb->onIoCompleted(co->iotype, NetEndpoint::EE_CONNECT, co->sep, 0, 0, 0);
					delete co->peer;
					break;
				case NetIoMux::EIT_INVALID:
				default:
					xpfAssert(("Unrecognized iotype. Maybe a corrupted EpollOverlapped.", false));
					break;
				}
				delete co;
			}


			// Consume ready list:
			// Pop the front socket out of list, and
			// process all r/w operations until EWOULDBLOCK.
			// Re-arm the socket and push back to the tail
			// of list if there are more operations remaining.
			bool consumeSome = false;
			NetEndpoint *ep = (NetEndpoint*) mReadyList.pop_front(remaining);
			do
			{
				if (!ep) break;
				EpollAsyncContext *ctx = (EpollAsyncContext*) ep->getAsyncContext();
				if (!ctx) break;

				ScopedThreadLock ml(ctx->lock);
				xpfAssert(("Expecting ready flag on for all ", ctx->ready));

				while (true) // process rqueue.
				{
					EpollOverlapped *o =
							(!ctx->rdqueue.empty()) ? ctx->rdqueue.front() : 0;

					if (!o) break;
					consumeSome = true;
					
					if (performIoLocked(ep, o))
						ctx->rdqueue.pop_front();
				} // end of while (true)

				while (true) // process wrqueue
				{
					EpollOverlapped *o =
							(!ctx->wrqueue.empty()) ? ctx->wrqueue.front() : 0;

					if (!o) break;
					consumeSome = true;

					if (performIoLocked(ep, o))
						ctx->wrqueue.pop_front();
				} // end of while (true)

				bool rearm = false;
				epoll_event evt;
				evt.events = EPOLLET | EPOLLONESHOT;
				evt.data.ptr = (void*) ep;
				if (!ctx->rdqueue.empty()) { evt.events |= EPOLLIN;  rearm = true; }
				if (!ctx->wrqueue.empty()) { evt.events |= EPOLLOUT; rearm = true; }

				if (rearm)
				{
					ctx->ready = false;
					int ec = epoll_ctl(mEpollfd, EPOLL_CTL_MOD, ep->getSocket(), &evt);
					if ((ec == -1) && (errno == ENOENT))
					{
						ec = epoll_ctl(mEpollfd, EPOLL_CTL_ADD, ep->getSocket(), &evt);
					}
					xpfAssert(ec == 0);
				}

			} while (0);

			// epoll_wait for more ready events.
			// Will skip if the length of list is too large.
			if (remaining < MAX_EPOLL_READY_LIST_LEN)
			{
				epoll_event evts[MAX_EPOLL_EVENTS_AT_ONCE];
				int nevts = epoll_wait(mEpollfd, evts, MAX_EPOLL_EVENTS_AT_ONCE, (consumeSome)? 0 : timeoutMs);
				xpfAssert(("Failed on calling epoll_wait", nevts != -1));
				if (0 == nevts)
				{
					return NetIoMux::ERS_TIMEOUT;
				}
				else if (nevts > 0)
				{
					for (int i=0; i<nevts; ++i)
					{
						uint32_t events = evts[i].events;
						NetEndpoint *ep = (NetEndpoint*) evts[i].data.ptr;
						EpollAsyncContext *ctx = (EpollAsyncContext*) ep->getAsyncContext();
						
						ScopedThreadLock ml(ctx->lock);
						xpfAssert(("Expecting non-ready ep in epoll_wait.", ctx->ready == false));
						ctx->ready = false;
						
						if ((events & (EPOLLERR | EPOLLHUP)))
						{
							for (std::deque<EpollOverlapped*>::iterator it = ctx->rdqueue.begin();
									it != ctx->rdqueue.end(); ++it)
							{
								EpollOverlapped *o = (*it);
								o->errorcode = ECONNABORTED;
								mCompletionList.push_back((void*)o);
							}
							for (std::deque<EpollOverlapped*>::iterator it = ctx->wrqueue.begin();
									it != ctx->wrqueue.end(); ++it)
							{
								EpollOverlapped *o = (*it);
								o->errorcode = ECONNABORTED;
								mCompletionList.push_back((void*)o);
							}

							ctx->rdqueue.clear();
							ctx->wrqueue.clear();

							continue;
						}

						if (events & (EPOLLIN | EPOLLOUT))
						{
							xpfAssert(("Expecting non-ready ep in epoll_wait.", ctx->ready == false));
							ctx->ready = true;
							mReadyList.push_back((void*)ep);
						}

					} // end of for (int i=0; i<nevts; ++i)
				}
				else
				{
					return NetIoMux::ERS_DISABLED;
				}
			}

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

			EpollOverlapped *o = new EpollOverlapped(ep, iotype);
			o->buffer = buf;
			o->length = buflen;
			o->cb = cb;
			
			EpollAsyncContext *ctx = (EpollAsyncContext*) ep->getAsyncContext();
			ScopedThreadLock ml(ctx->lock);
			if (!performIoLocked(ep, o))
				appendAsyncOpLocked(ep, o, ASYNC_OP_READ);
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

			EpollOverlapped *o = new EpollOverlapped(ep, iotype);
			o->buffer = buf;
			o->length = buflen;
			o->cb = cb;
			o->peer = peer;

			EpollAsyncContext *ctx = (EpollAsyncContext*) ep->getAsyncContext();
			ScopedThreadLock ml(ctx->lock);
			if (!performIoLocked(ep, o))
				appendAsyncOpLocked(ep, o, ASYNC_OP_READ);
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

			EpollOverlapped *o = new EpollOverlapped(ep, iotype);
			o->buffer = (c8*)buf;
			o->length = buflen;
			o->cb = cb;

			EpollAsyncContext *ctx = (EpollAsyncContext*) ep->getAsyncContext();
			ScopedThreadLock ml(ctx->lock);
			if (!performIoLocked(ep, o))
				appendAsyncOpLocked(ep, o, ASYNC_OP_WRITE);
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

			EpollOverlapped *o = new EpollOverlapped(ep, iotype);
			o->buffer = (c8*)buf;
			o->length = buflen;
			o->cb = cb;
			o->peer = p;

			EpollAsyncContext *ctx = (EpollAsyncContext*) ep->getAsyncContext();
			ScopedThreadLock ml(ctx->lock);
			if (!performIoLocked(ep, o))
				appendAsyncOpLocked(ep, o, ASYNC_OP_WRITE);
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

			EpollOverlapped *o = new EpollOverlapped(ep, iotype);
			o->cb = cb;

			EpollAsyncContext *ctx = (EpollAsyncContext*) ep->getAsyncContext();
			ScopedThreadLock ml(ctx->lock);
			if (!performIoLocked(ep, o))
				appendAsyncOpLocked(ep, o, ASYNC_OP_READ);
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

			// Note: the resolving is blockable.
			NetEndpoint::Peer *peer = new NetEndpoint::Peer;
			if (!NetEndpoint::resolvePeer(ep->getProtocol(), *peer, host, 0, port))
			{
				cb->onIoCompleted(iotype, NetEndpoint::EE_RESOLVE, ep, 0, 0, 0);
				return;
			}

			EpollOverlapped *o = new EpollOverlapped(ep, iotype);
			o->cb = cb;
			o->peer = peer;

			EpollAsyncContext *ctx = (EpollAsyncContext*) ep->getAsyncContext();
			ScopedThreadLock ml(ctx->lock);
			if (!performIoLocked(ep, o))
				appendAsyncOpLocked(ep, o, ASYNC_OP_WRITE);
		}

		bool join(NetEndpoint *ep)
		{
			s32 sock = ep->getSocket();

			// bundle with an async context
			EpollAsyncContext *ctx = new EpollAsyncContext;
			ctx->ready = false;
			ctx->ep = ep;
			ep->setAsyncContext((vptr)ctx);

			// request the socket to be non-blocking
            int flags = fcntl(sock, F_GETFL);
            xpfAssert(flags != 0xffffffff);
            fcntl(sock, F_SETFL, flags | O_NONBLOCK);

			return true;
		}

		bool depart(NetEndpoint *ep)
		{
			s32 sock = ep->getSocket();

			// remove given endpoint from epoll group
			struct epoll_event dummy;
			int ec = epoll_ctl(mEpollfd, EPOLL_CTL_DEL, sock, &dummy); // dummy is not used but cannot be NULL
			xpfAssert((ec == 0 || errno == ENOENT));
			if ((ec != 0) && (errno != ENOENT))
			{
				return false;
			}

			// delete the async context
			EpollAsyncContext *ctx = (EpollAsyncContext*) ep->getAsyncContext();
			xpfAssert(ctx != 0);
			if (!ctx)
			{
				return false;
			}
			else
			{
				ctx->lock.lock();
				if (ctx->ready)
				{
					mReadyList.erase((void*)ep); // Note: Acquire mReadyList's lock while holding ctx's lock.
				}
				delete ctx;
				ep->setAsyncContext(0);
				// since the whole context object has been deleted, there's no bother to call unlock.
			}

			// reset the socket to be blocking
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

		bool performIoLocked(NetEndpoint *ep, EpollOverlapped *o) // require ep->ctx locked.
		{
			EpollAsyncContext *ctx = (EpollAsyncContext*) ep->getAsyncContext();
			xpfAssert(ctx != 0);
			if (ctx == 0)
				return false;

			bool complete = false;
			switch (o->iotype)
			{
			case NetIoMux::EIT_RECV:
				{
					ssize_t bytes = ::recv(ep->getSocket(), o->buffer, (size_t)o->length, MSG_DONTWAIT);
					if (bytes >= 0)
					{
						o->length = bytes;
						o->errorcode = 0;
						complete = true;
					}
					else if (errno != EWOULDBLOCK && errno != EAGAIN)
					{
						o->length = 0;
						o->errorcode = errno;
						complete = true;
					}
				}
				break;
			case NetIoMux::EIT_RECVFROM:
				{
					ssize_t bytes = ::recvfrom(ep->getSocket(), o->buffer, (size_t)o->length, MSG_DONTWAIT, (struct sockaddr*)o->peer->Data, (socklen_t*)&o->peer->Length);
					if (bytes >= 0)
					{
						o->length = bytes;
						o->errorcode = 0;
						complete = true;
					}
					else if (errno != EWOULDBLOCK && errno != EAGAIN)
					{
						o->length = 0;
						o->errorcode = errno;
						complete = true;
					}
				}
				break;
			case NetIoMux::EIT_ACCEPT:
				{
					int peersock = ::accept(ep->getSocket(), (struct sockaddr*)o->peer->Data, (socklen_t*)&o->peer->Length);
					if (peersock != -1)
					{
						o->errorcode = 0;
						o->length = 0;
						o->tep = new NetEndpoint(ep->getProtocol(), peersock, NetEndpoint::ESTAT_CONNECTED);
						join(o->tep);
						complete = true;
						ep->setStatus(NetEndpoint::ESTAT_LISTENING);
					}
					else if (errno != EWOULDBLOCK && errno != EAGAIN)
					{
						o->tep = 0;
						o->length = 0;
						o->errorcode = errno;
						complete = true;
						ep->setStatus(NetEndpoint::ESTAT_LISTENING);
					}
					else
					{
						ep->setStatus(NetEndpoint::ESTAT_ACCEPTING);
					}
				}
				break;
			case NetIoMux::EIT_SEND:
				{
					ssize_t bytes = ::send(ep->getSocket(), o->buffer, (size_t)o->length, MSG_DONTWAIT);
					if (bytes >=0 )
					{
						o->length = bytes;
						o->errorcode = 0;
						complete = true;
					}
					else if (errno != EWOULDBLOCK && errno != EAGAIN)
					{
						o->length = 0;
						o->errorcode = errno;
						complete = true;
					}
				}
				break;
			case NetIoMux::EIT_SENDTO:
				{
					ssize_t bytes = ::sendto(ep->getSocket(), o->buffer, (size_t)o->length, MSG_DONTWAIT,
							(const struct sockaddr*)o->peer->Data, (socklen_t)o->peer->Length);
					if (bytes >=0 )
					{
						o->length = bytes;
						o->errorcode = 0;
						complete = true;
					}
					else if (errno != EWOULDBLOCK && errno != EAGAIN)
					{
						o->length = 0;
						o->errorcode = errno;
						complete = true;
					}
				}
				break;
			case NetIoMux::EIT_CONNECT:
				if (o->length == 0) // not yet called connect
				{
					int ec = ::connect(ep->getSocket(), (const struct sockaddr*)o->peer->Data, (socklen_t)o->peer->Length);
					if (ec == 0)
					{
						o->length = 0;
						o->errorcode = 0;
						complete = true;
						ep->setStatus(NetEndpoint::ESTAT_CONNECTED);
					}
					else if (errno != EINPROGRESS)
					{
						o->length = 0;
						o->errorcode = errno;
						complete = true;
						ep->setStatus(NetEndpoint::ESTAT_INIT);
					}
					else
					{
						xpfAssert(o->length == 0);
						ep->setStatus(NetEndpoint::ESTAT_CONNECTING);
						o->length = 1; // mark a connect() call in progress.
					}
				}
				else // a connect() was called and waiting for result.
				{
					int val = 0;
					socklen_t valsize = sizeof(int);
					int ec = ::getsockopt(ep->getSocket(), SOL_SOCKET, SO_ERROR, &val, &valsize);
					xpfAssert(ec == 0);
					if (ec == 0 && val == 0)
					{
						o->errorcode = 0;
						o->length = 0;
						ep->setStatus(NetEndpoint::ESTAT_CONNECTED);
					}
					else
					{
						o->errorcode = errno;
						delete o->peer;
						o->peer = 0;
						ep->setStatus(NetEndpoint::ESTAT_INIT);
					}
					complete = true;
				}
				break;

			default:
				xpfAssert(("Unexpected iotype.", false));
				break;
			} // end of switch (o->iotype)
			
			if (complete)
				mCompletionList.push_back(o);
			
			return complete;
		}
	
		void appendAsyncOpLocked(NetEndpoint *ep, EpollOverlapped *o, u8 mode) // require ep->ctx locked.
		{
			EpollAsyncContext *ctx = (ep == 0)? 0 : (EpollAsyncContext*)ep->getAsyncContext();
			if (ctx == 0 || o == 0)
			{
				xpfAssert(false);
				return;
			}

			switch (mode)
			{
			case ASYNC_OP_READ:
				ctx->rdqueue.push_back(o);
				break;
			case ASYNC_OP_WRITE:
				ctx->wrqueue.push_back(o);
				break;
			default:
				xpfAssert(("Unexpected async op mode.",false));
				break;
			}

			if (!ctx->ready)
			{
				ctx->ready = true;
				mReadyList.push_back((void*)ep);
			}
		}

		NetIoMuxSyncFifo mCompletionList; // fifo of EpollOverlapped.
		NetIoMuxSyncFifo mReadyList;      // fifo of NetEndpoints.
		bool mEnable;
		int mEpollfd;
	}; // end of class NetIoMuxImpl (epoll)

} // end of namespace xpf

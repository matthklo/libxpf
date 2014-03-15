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
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#define MAX_EVENTS_AT_ONCE (128)
#define MAX_READY_LIST_LEN (10240)

#define ASYNC_OP_READ  (0)
#define ASYNC_OP_WRITE (1)

namespace xpf
{

	// data record per operation
	struct Overlapped
	{
		Overlapped(NetEndpoint *ep, NetIoMux::EIoType iocode)
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
	struct AsyncContext
	{
		std::deque<Overlapped*>  rdqueue; // queued read operations
		std::deque<Overlapped*>  wrqueue; // queued write operations
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

			mKqueue = kqueue();
			xpfAssert(mKqueue != -1);
			if (mKqueue == -1)
				mEnable = false;
		}

		~NetIoMuxImpl()
		{
			enable(false);

			if (mKqueue != -1)
				close(mKqueue);
			mKqueue = -1;
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
			bool consumeSome = false;
			u32 pendingCnt = 0;

			// Process the completion queue. Emit the completion event.
			Overlapped *co = (Overlapped*) mCompletionList.pop_front(pendingCnt);
			if (co)
			{
				consumeSome = true;
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
					xpfAssert(("Unrecognized iotype. Maybe a corrupted Overlapped.", false));
					break;
				}
				delete co;
			}


			// Consume ready list:
			// Pop the front socket out of list, and
			// process all r/w operations until EWOULDBLOCK.
			// Re-arm the socket if there are more pending
			// operations.
			NetEndpoint *ep = (NetEndpoint*) mReadyList.pop_front(pendingCnt);
			do
			{
				if (!ep) break;
				AsyncContext *ctx = (AsyncContext*) ep->getAsyncContext();
				if (!ctx) break;

				ScopedThreadLock ml(ctx->lock);
				xpfAssert(("Expecting ready flag on for all ", ctx->ready));
				ctx->ready = false;
				while (!ctx->rdqueue.empty()) // process rqueue.
				{
					Overlapped *o = ctx->rdqueue.front();
					if (!o) break;
					consumeSome = true;
					
					if (performIoLocked(ep, o))
						ctx->rdqueue.pop_front();
					else
						break;
				} // end of while (true)

				while (!ctx->wrqueue.empty()) // process wrqueue
				{
					Overlapped *o = ctx->wrqueue.front();
					if (!o) break;
					consumeSome = true;

					if (performIoLocked(ep, o))
						ctx->wrqueue.pop_front();
					else
						break;
				} // end of while (true)

				struct kevent changes[2] = {0};
				int changeCnt = 0;

				if (!ctx->rdqueue.empty())
				{
					changes[changeCnt].filter = EVFILT_READ;
					changes[changeCnt].flags = EV_ADD | EV_ONESHOT | EV_ENABLE;
					changes[changeCnt].ident = ep->getSocket();
					changes[changeCnt].udata = (void*) ep;
					changeCnt++;
				}

				if (!ctx->wrqueue.empty())
				{
					changes[changeCnt].filter = EVFILT_WRITE;
					changes[changeCnt].flags = EV_ADD | EV_ONESHOT | EV_ENABLE;
					changes[changeCnt].ident = ep->getSocket();
					changes[changeCnt].udata = (void*) ep;
					changeCnt++;
				}

				if (changeCnt > 0)
				{
					int ec = kevent(mKqueue, changes, changeCnt, 0, 0, 0);
					xpfAssert(ec != -1);
				}
			} while (0);

			// Wait for more ready events.
			// Will skip if the length of list is too large.
			if (pendingCnt < MAX_READY_LIST_LEN)
			{
				timespec ts;
				ts.tv_sec = 0;
				ts.tv_nsec = (consumeSome) ? 0 : timeoutMs * 1000000;

				struct kevent evts[MAX_EVENTS_AT_ONCE];
				int nevts = kevent(mKqueue, 0, 0, evts, MAX_EVENTS_AT_ONCE, &ts);
				xpfAssert(("Failed on calling kevent", nevts != -1));
				if (0 == nevts)
				{
					return NetIoMux::ERS_TIMEOUT;
				}
				else if (nevts > 0)
				{
					for (int i=0; i<nevts; ++i)
					{
						short   filter = evts[i].filter;
						u_short flags = evts[i].flags;
						NetEndpoint *ep = (NetEndpoint*) evts[i].udata;
						AsyncContext *ctx = (AsyncContext*) ep->getAsyncContext();
						
						ScopedThreadLock ml(ctx->lock);
						xpfAssert(("Expecting non-ready ep in kqueue.", ctx->ready == false));
						ctx->ready = false;
						switch (filter)
						{
						case EVFILT_READ:
							if (flags & EV_EOF)
							{
								for (std::deque<Overlapped*>::iterator it = ctx->rdqueue.begin();
										it != ctx->rdqueue.end(); ++it)
								{
									Overlapped *o = (*it);
									o->errorcode = ECONNABORTED;
									mCompletionList.push_back((void*)o);
								}
								ctx->rdqueue.clear();
							}
							else
							{
								ctx->ready = true;
								mReadyList.push_back((void*)ep);
							}
							break;
						case EVFILT_WRITE:
							if (flags & EV_EOF)
							{
								for (std::deque<Overlapped*>::iterator it = ctx->wrqueue.begin();
										it != ctx->wrqueue.end(); ++it)
								{
									Overlapped *o = (*it);
									o->errorcode = ECONNABORTED;
									mCompletionList.push_back((void*)o);
								}
								ctx->wrqueue.clear();
							}
							else
							{
								ctx->ready = true;
								mReadyList.push_back((void*)ep);
							}
							break;
						default:
							xpfAssert(("Unrecognizable kqueue evt filter.", false));
							break;
						} // end of switch(filter)
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

			Overlapped *o = new Overlapped(ep, iotype);
			o->buffer = buf;
			o->length = buflen;
			o->cb = cb;
			
			AsyncContext *ctx = (AsyncContext*) ep->getAsyncContext();
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

			Overlapped *o = new Overlapped(ep, iotype);
			o->buffer = buf;
			o->length = buflen;
			o->cb = cb;
			o->peer = peer;

			AsyncContext *ctx = (AsyncContext*) ep->getAsyncContext();
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

			Overlapped *o = new Overlapped(ep, iotype);
			o->buffer = (c8*)buf;
			o->length = buflen;
			o->cb = cb;

			AsyncContext *ctx = (AsyncContext*) ep->getAsyncContext();
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

			Overlapped *o = new Overlapped(ep, iotype);
			o->buffer = (c8*)buf;
			o->length = buflen;
			o->cb = cb;
			o->peer = p;

			AsyncContext *ctx = (AsyncContext*) ep->getAsyncContext();
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

			Overlapped *o = new Overlapped(ep, iotype);
			o->cb = cb;

			AsyncContext *ctx = (AsyncContext*) ep->getAsyncContext();
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

			// Note: resolving can be blockable.
			NetEndpoint::Peer *peer = new NetEndpoint::Peer;
			if (!NetEndpoint::resolvePeer(ep->getProtocol(), *peer, host, 0, port))
			{
				cb->onIoCompleted(iotype, NetEndpoint::EE_RESOLVE, ep, 0, 0, 0);
				return;
			}

			Overlapped *o = new Overlapped(ep, iotype);
			o->cb = cb;
			o->peer = peer;

			AsyncContext *ctx = (AsyncContext*) ep->getAsyncContext();
			ScopedThreadLock ml(ctx->lock);
			if (!performIoLocked(ep, o))
				appendAsyncOpLocked(ep, o, ASYNC_OP_WRITE);
		}

		bool join(NetEndpoint *ep)
		{
			s32 sock = ep->getSocket();

			// bundle with an async context
			AsyncContext *ctx = new AsyncContext;
			ctx->ready = false;
			ctx->ep = ep;
			ep->setAsyncContext((vptr)ctx);

			// request the socket to be non-blocking
			int flags = fcntl(sock, F_GETFL);
			xpfAssert(flags != -1);
			fcntl(sock, F_SETFL, flags | O_NONBLOCK);

			return true;
		}

		bool depart(NetEndpoint *ep)
		{
			s32 sock = ep->getSocket();

			// ensure the under lying fd removed from kqueue
			struct kevent changes[2] = {0};
			changes[0].ident = changes[1].ident = sock;
			changes[0].flags = changes[1].flags = EV_DELETE;
			changes[0].filter = EVFILT_READ;
			changes[1].filter = EVFILT_WRITE;
			int ec = kevent(mKqueue, changes, 2, 0, 0, 0);
			xpfAssert((ec == 0 || errno == ENOENT));
			if ((ec != 0) && (errno != ENOENT))
			{
				return false;
			}

			// delete the async context
			AsyncContext *ctx = (AsyncContext*) ep->getAsyncContext();
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
			xpfAssert(flags != -1);
			fcntl(sock, F_SETFL, flags & (0 ^ O_NONBLOCK));

			return (ec == 0);
		}

		static const char * getMultiplexerType(NetIoMux::EPlatformMultiplexer &epm)
		{
			epm = NetIoMux::EPM_KQUEUE;
			return "kqueue";
		}

	private:

		bool performIoLocked(NetEndpoint *ep, Overlapped *o) // require ep->ctx locked.
		{
			AsyncContext *ctx = (AsyncContext*) ep->getAsyncContext();
			xpfAssert(ctx != 0);
			if (ctx == 0)
				return false;

			bool complete = true;
			switch (o->iotype)
			{
			case NetIoMux::EIT_RECV:
				{
					ssize_t bytes = ::recv(ep->getSocket(), o->buffer, (size_t)o->length, MSG_DONTWAIT);
					if (bytes >= 0)
					{
						o->length = bytes;
						o->errorcode = 0;
					}
					else if (errno != EWOULDBLOCK && errno != EAGAIN)
					{
						o->length = 0;
						o->errorcode = errno;
					}
					else
					{
						complete = false;
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
					}
					else if (errno != EWOULDBLOCK && errno != EAGAIN)
					{
						o->length = 0;
						o->errorcode = errno;
					}
					else
					{
						complete = false;
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
						ep->setStatus(NetEndpoint::ESTAT_LISTENING);
					}
					else if (errno != EWOULDBLOCK && errno != EAGAIN)
					{
						o->tep = 0;
						o->length = 0;
						o->errorcode = errno;
						ep->setStatus(NetEndpoint::ESTAT_LISTENING);
					}
					else
					{
						complete = false;
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
					}
					else if (errno != EWOULDBLOCK && errno != EAGAIN)
					{
						o->length = 0;
						o->errorcode = errno;
					}
					else
					{
						complete = false;
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
					}
					else if (errno != EWOULDBLOCK && errno != EAGAIN)
					{
						o->length = 0;
						o->errorcode = errno;
					}
					else
					{
						complete = false;
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
						ep->setStatus(NetEndpoint::ESTAT_CONNECTED);
					}
					else if (errno != EINPROGRESS)
					{
						o->length = 0;
						o->errorcode = errno;
						ep->setStatus(NetEndpoint::ESTAT_INIT);
					}
					else
					{
						xpfAssert(o->length == 0);
						ep->setStatus(NetEndpoint::ESTAT_CONNECTING);
						o->length = 1; // mark a connect() call in progress.
						complete = false;
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
	
		void appendAsyncOpLocked(NetEndpoint *ep, Overlapped *o, u8 mode) // require ep->ctx locked.
		{
			AsyncContext *ctx = (ep == 0)? 0 : (AsyncContext*)ep->getAsyncContext();
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

		NetIoMuxSyncFifo mCompletionList; // fifo of Overlapped.
		NetIoMuxSyncFifo mReadyList;      // fifo of NetEndpoints.
		bool mEnable;
		int  mKqueue;
	}; // end of class NetIoMuxImpl (kqueue)

} // end of namespace xpf

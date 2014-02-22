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

namespace xpf
{

	struct EpollAsyncContext
	{

	};

	class NetIoMuxImpl
	{
	public:
		NetIoMuxImpl()
		{

		}

		~NetIoMuxImpl()
		{

		}

		void enable(bool val)
		{

		}

		void run()
		{

		}

		NetIoMux::RunningStaus runOnce(u32 timeoutMs)
		{
			return NetIoMux::RS_NORMAL;
		}

		void asyncRecv(const NetEndpoint *ep, c8 *buf, u32 buflen, NetIoMux::RecvCallback cb)
		{

		}

		void asyncRecvFrom(const NetEndpoint *ep, NetEndpoint::Peer *peer, c8 *buf, u32 buflen, NetIoMux::RecvFromCallback cb)
		{

		}

		void asyncSend(const NetEndpoint *ep, const c8 *buf, u32 buflen, NetIoMux::SendCallback cb)
		{

		}

		void asyncSendTo(const NetEndpoint *ep, const NetEndpoint::Peer *peer, const c8 *buf, u32 buflen, NetIoMux::SendToCallback cb)
		{

		}

		void asyncAccept(const NetEndpoint *ep, NetIoMux::AcceptCallback cb)
		{

		}

		void asyncConnect(const c8 *host, u32 port, NetIoMux::ConnectCallback cb)
		{

		}

		static bool provision(NetEndpoint *ep)
		{
			if (ep->getAsyncContext() != 0)
				return false;

			ep->setAsyncContext((vptr) new EpollAsyncContext);
			return true;
		}

		static bool unprovision(NetEndpoint *ep)
		{
			EpollAsyncContext *ctx = (EpollAsyncContext*)((ep) ? ep->getAsyncContext() : 0);
			if (ctx)
			{
				delete ctx;
				return true;
			}
			return false;
		}

		static const char * getMultiplexerType(NetIoMux::EPlatformMultiplexer &epm)
		{
			epm = NetIoMux::EPM_EPOLL;
			return "epoll";
		}

	private:

	}; // end of class NetIoMuxImpl (epoll)

} // end of namespace xpf

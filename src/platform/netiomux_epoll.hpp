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

#include "netiomux_readylist.hpp"
#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>

namespace xpf
{

	//struct EpollAsyncContext
	//{

	//};

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

		NetIoMux::ERunningStaus runOnce(u32 timeoutMs)
		{
			return NetIoMux::ERS_NORMAL;
		}

		void asyncRecv(NetEndpoint *ep, c8 *buf, u32 buflen, NetIoMuxCallback *cb)
		{

		}

		void asyncRecvFrom(NetEndpoint *ep, c8 *buf, u32 buflen, NetIoMuxCallback *cb)
		{

		}

		void asyncSend(NetEndpoint *ep, const c8 *buf, u32 buflen, NetIoMuxCallback *cb)
		{

		}

		void asyncSendTo(NetEndpoint *ep, const NetEndpoint::Peer *peer, const c8 *buf, u32 buflen, NetIoMuxCallback *cb)
		{

		}

		void asyncAccept(NetEndpoint *ep, NetIoMuxCallback *cb)
		{

		}

		void asyncConnect(NetEndpoint *ep, const c8 *host, u32 port, NetIoMuxCallback *cb)
		{

		}

		bool join(NetEndpoint *ep)
		{
            s32 sock = ep->getSocket();
            int flags = fcntl(sock, F_GETFL);
            xpfAssert(flags != 0xffffffff);
            fcntl(sock, F_SETFL, flags | O_NONBLOCK);
			return true;
		}

		bool depart(NetEndpoint *ep)
		{
            s32 sock = ep->getSocket();
            int flags = fcntl(sock, F_GETFL);
            xpfAssert(flags != 0xffffffff);
            fcntl(sock, F_SETFL, flags & (0 ^ O_NONBLOCK));
			return false;
		}

		static const char * getMultiplexerType(NetIoMux::EPlatformMultiplexer &epm)
		{
			epm = NetIoMux::EPM_EPOLL;
			return "epoll";
		}

	private:
        NetIoMuxReadyList mReadyList;
	}; // end of class NetIoMuxImpl (epoll)

} // end of namespace xpf

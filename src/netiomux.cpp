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

#if defined(XPF_PLATFORM_WINDOWS)
#  include "platform/netiomux_iocp.hpp"
#elif defined(XPF_PLATFORM_LINUX)
// Use epoll on Linux, Android hosts.
#  include "platform/netiomux_epoll.hpp"
#elif defined(XPF_PLATFORM_BSD)
// Use kqueue on BSD-family hosts (both MacOSX and ios are included)
#  include "platform/netiomux_kqueue.hpp"
#else
#  error NetIoMux is not supported on current platform.
#endif

namespace xpf
{

NetIoMux::NetIoMux()
{
	pImpl = new NetIoMuxImpl();
}

NetIoMux::~NetIoMux()
{
	if (pImpl)
	{
		delete pImpl;
		pImpl = 0;
	}
}

void NetIoMux::enable(bool val)
{
	pImpl->enable(val);
}

void NetIoMux::run()
{
	pImpl->run();
}

NetIoMux::ERunningStaus NetIoMux::runOnce(u32 timeoutMs)
{
	return pImpl->runOnce(timeoutMs);
}

void NetIoMux::asyncRecv(NetEndpoint *ep, c8 *buf, u32 buflen, NetIoMuxCallback *cb)
{
	pImpl->asyncRecv(ep, buf, buflen, cb);
}

void NetIoMux::asyncRecvFrom(NetEndpoint *ep, c8 *buf, u32 buflen, NetIoMuxCallback *cb)
{
	pImpl->asyncRecvFrom(ep, buf, buflen, cb);
}

void NetIoMux::asyncSend(NetEndpoint *ep, const c8 *buf, u32 buflen, NetIoMuxCallback *cb)
{
	pImpl->asyncSend(ep, buf, buflen, cb);
}

void NetIoMux::asyncSendTo(NetEndpoint *ep, const NetEndpoint::Peer *peer, const c8 *buf, u32 buflen, NetIoMuxCallback *cb)
{
	pImpl->asyncSendTo(ep, peer, buf, buflen, cb);
}

void NetIoMux::asyncAccept(NetEndpoint *ep, NetIoMuxCallback *cb)
{
	pImpl->asyncAccept(ep, cb);
}

void NetIoMux::asyncConnect(NetEndpoint *ep, const c8 *host, u32 port, NetIoMuxCallback *cb)
{
	pImpl->asyncConnect(ep, host, port, cb);
}

bool NetIoMux::join(NetEndpoint *ep)
{
	return pImpl->join(ep);
}

bool NetIoMux::depart(NetEndpoint *ep)
{
	return pImpl->depart(ep);
}

const char * NetIoMux::getMultiplexerType(EPlatformMultiplexer &epm)
{
	return NetIoMuxImpl::getMultiplexerType(epm);
}

} // end of namespace xpf

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

#ifndef _XPF_NETIOMUX_HEADER_
#define _XPF_NETIOMUX_HEADER_

#include "platform.h"
#include "netendpoint.h"
#include "delegate.h"

namespace xpf
{

class NetIoMux
{
public:
	enum NetIoMuxRunningStaus
	{
		NIMRS_NORMAL = 0,
		NIMRS_TIMEOUT,
		NIMRS_SHUTDOWN,
	};

	typedef Delegate<void(u32, const NetEndpoint*, c8*, u32)> RecvCallback; // ec, ep, buf, bytes
	typedef Delegate<void(u32, const NetEndpoint*, NetEndpoint::Peer*, c8*, u32)> RecvFromCallback; // ec, ep, peer, buf, bytes
	typedef Delegate<void(u32, const NetEndpoint*, const c8*, u32)> SendCallback; // ec, ep, buf, bytes
	typedef Delegate<void(u32, const NetEndpoint*, const NetEndpoint::Peer*, const c8*, u32)> SendToCallback; // ec, ep, peer, buf, bytes
	typedef Delegate<void(u32, const NetEndpoint*, NetEndpoint*)> AcceptCallback; // ec, ep, accepted ep
	typedef Delegate<void(u32, NetEndpoint*)> ConnectCallback; // ec, connected ep

	NetIoMux();
	virtual ~NetIoMux();

	// life cycle
	void shutdown();

	// For worker threads.
	void                 run();
	NetIoMuxRunningStaus runOnce(u32 timeoutMs = 0xffffffff);

	// For I/O control
	void asyncRecv(const NetEndpoint *ep, c8 *buf, u32 buflen, RecvCallback cb);
	void asyncRecvFrom(const NetEndpoint *ep, NetEndpoint::Peer *peer, c8 *buf, u32 buflen, RecvFromCallback cb);
	void asyncSend(const NetEndpoint *ep, const c8 *buf, u32 buflen, SendCallback cb);
	void asyncSendTo(const NetEndpoint *ep, const NetEndpoint::Peer *peer, const c8 *buf, u32 buflen, SendToCallback cb);
	void asyncAccept(const NetEndpoint *ep, AcceptCallback cb);
	void asyncConnect(const c8 *host, u32 port, ConnectCallback cb);

	// For endpoints control
	NetEndpoint* createEndpoint(NetEndpoint::EndpointTypeEnum type);

	// TODO: should we provide attach/detach functions for a single endpoint?

private:
	// Non-copyable
	NetIoMux(const NetIoMux& that) {}
	NetIoMux& operator = (const NetIoMux& that) {}

	vptr pImpl;
};

}; // end of namespace xpf

#endif // _XPF_NETIOMUX_HEADER_

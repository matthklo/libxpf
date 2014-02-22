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

class NetIoMuxImpl;

class NetIoMux
{
public:
	enum EPlatformMultiplexer
	{
		EPM_IOCP = 0,
		EPM_EPOLL,
		EPM_KQUEUE,

		EPM_MAX,
		EPM_UNKNOWN,
	};

	enum RunningStaus
	{
		RS_NORMAL = 0,
		RS_TIMEOUT,
		RS_DISABLED,
	};

	typedef Delegate<void(u32, NetEndpoint*, c8*, u32)> RecvCallback; // ec, ep, buf, bytes
	typedef Delegate<void(u32, NetEndpoint*, NetEndpoint::Peer*, c8*, u32)> RecvFromCallback; // ec, ep, peer, buf, bytes
	typedef Delegate<void(u32, NetEndpoint*, const c8*, u32)> SendCallback; // ec, ep, buf, bytes
	typedef Delegate<void(u32, NetEndpoint*, const NetEndpoint::Peer*, const c8*, u32)> SendToCallback; // ec, ep, peer, buf, bytes
	typedef Delegate<void(u32, NetEndpoint*, NetEndpoint*)> AcceptCallback; // ec, ep, accepted ep
	typedef Delegate<void(u32, NetEndpoint*)> ConnectCallback; // ec, connected ep

	NetIoMux();
	virtual ~NetIoMux();

	// life cycle
	void enable(bool val = true);
	inline void disable() { enable(false); }

	// For worker threads.
	void         run();
	RunningStaus runOnce(u32 timeoutMs = 0xffffffff);

	// For I/O control
	void asyncRecv(NetEndpoint *ep, c8 *buf, u32 buflen, RecvCallback cb);
	void asyncRecvFrom(NetEndpoint *ep, NetEndpoint::Peer *peer, c8 *buf, u32 buflen, RecvFromCallback cb);
	void asyncSend(NetEndpoint *ep, const c8 *buf, u32 buflen, SendCallback cb);
	void asyncSendTo(NetEndpoint *ep, const NetEndpoint::Peer *peer, const c8 *buf, u32 buflen, SendToCallback cb);
	void asyncAccept(NetEndpoint *ep, AcceptCallback cb);
	void asyncConnect(NetEndpoint *ep, const c8 *host, u32 port, ConnectCallback cb);

	// Join/depart the endpoint to/from netiomux.
	bool join(NetEndpoint *ep);
	bool depart(NetEndpoint *ep);

	static const char * getMultiplexerType(EPlatformMultiplexer &epm);

private:
	// Non-copyable
	NetIoMux(const NetIoMux& that) {}
	NetIoMux& operator = (const NetIoMux& that) {}

	NetIoMuxImpl *pImpl;
};

}; // end of namespace xpf

#endif // _XPF_NETIOMUX_HEADER_

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

namespace xpf
{

class NetIoMux
{
public:
	NetIoMux();
	virtual ~NetIoMux();

	// life cycle
	void shutdown();

	// For worker threads.
	void run();
	bool runOnce();

	// For I/O control
	void asyncRecv(const NetEndpoint *ep, c8 *buf, u32 buflen /*, callback */);
	void asyncRecvFrom(const NetEndpoint *ep, NetEndpoint::Peer *peer, c8 *buf, u32 buflen /*, callback */);
	void asyncSend(const NetEndpoint *ep, const c8 *buf, u32 buflen /*, callback */);
	void asyncSendTo(const NetEndpoint *ep, const NetEndpoint::Peer *peer, const c8 *buf, u32 buflen /*, callback */);
	void asyncAccept(const NetEndpoint *ep /*, callback */);

	// For endpoints control
	NetEndpoint* createEndpoint(NetEndpoint::EndpointTypeEnum type);

	// TODO: should we provide attach/detach functions for a single endpoint?

private:
	// Non-copyable
	NetIoMux(const NetIoMux& that);
	NetIoMux& operator = (const NetIoMux& that);

	vptr pImpl;
};

}; // end of namespace xpf

#endif // _XPF_NETIOMUX_HEADER_

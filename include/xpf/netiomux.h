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

class NetIoMuxImpl;
class NetIoMuxCallback;

class XPF_API NetIoMux
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

	enum ERunningStaus
	{
		ERS_NORMAL = 0,
		ERS_TIMEOUT,
		ERS_DISABLED,
	};

	enum EIoType
	{
		EIT_INVALID = 0,
		EIT_RECV,
		EIT_RECVFROM,
		EIT_SEND,
		EIT_SENDTO,
		EIT_ACCEPT,
		EIT_CONNECT,
	};

	NetIoMux();
	virtual ~NetIoMux();

	// life cycle
	void enable(bool val = true);
	inline void disable() { enable(false); }

	// For worker threads.
	void          run();
	ERunningStaus runOnce(u32 timeoutMs = 0xffffffff);

	// For I/O control
	void asyncRecv(NetEndpoint *ep, c8 *buf, u32 buflen, NetIoMuxCallback *cb = 0);
	void asyncRecvFrom(NetEndpoint *ep, c8 *buf, u32 buflen, NetIoMuxCallback *cb = 0);
	void asyncSend(NetEndpoint *ep, const c8 *buf, u32 buflen, NetIoMuxCallback *cb = 0);
	void asyncSendTo(NetEndpoint *ep, const NetEndpoint::Peer *peer, const c8 *buf, u32 buflen, NetIoMuxCallback *cb = 0);
	void asyncAccept(NetEndpoint *ep, NetIoMuxCallback *cb = 0);
	void asyncConnect(NetEndpoint *ep, const c8 *host, const c8 *serviceOrPort, NetIoMuxCallback *cb = 0);
	void asyncConnect(NetEndpoint *ep, const c8 *host, u32 port, NetIoMuxCallback *cb = 0); // A varient asyncConnect() which takes a numeric port number. 

	// Join/depart the endpoint to/from netiomux.
	bool join(NetEndpoint *ep);
	bool depart(NetEndpoint *ep);

	// Default callback setter/getter
	inline void setDefaultCallback(NetIoMuxCallback *cb) { pDefaultMuxCallback = cb; }
	inline NetIoMuxCallback* getDefaultCallback() const { return pDefaultMuxCallback; }

	static const char * getMultiplexerType(EPlatformMultiplexer &epm);

private:
	// Non-copyable
	NetIoMux(const NetIoMux& that) {}
	NetIoMux& operator = (const NetIoMux& that) { return *this; }

	NetIoMuxCallback *pDefaultMuxCallback;
	NetIoMuxImpl *pImpl;
};


class NetIoMuxCallback
{
public:
	virtual void onIoCompleted(NetIoMux::EIoType type, NetEndpoint::EError ec, NetEndpoint *sep, vptr tepOrPeer, const c8 *buf, u32 len) = 0;
};

}; // end of namespace xpf

#endif // _XPF_NETIOMUX_HEADER_

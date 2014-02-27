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

#ifndef _XPF_NETENDPOINT_HEADER_
#define _XPF_NETENDPOINT_HEADER_

#include "platform.h"

#define XPF_NETENDPOINT_MAXADDRLEN (128)

namespace xpf
{

class NetEndpointImpl;

class XPF_API NetEndpoint
{
public:
	enum EStatus
	{
		ESTAT_INIT = 0,
		ESTAT_LISTENING,
		ESTAT_ACCEPTING,
		ESTAT_CONNECTING,
		ESTAT_CONNECTED,
		ESTAT_CLOSING,
		ESTAT_INVALID,

		ESTAT_MAX,
		ESTAT_UNKNOWN,
	};

	enum EError
	{
		EE_SUCCESS = 0,
		EE_INVALID_OP,
		EE_RECV,
		EE_SEND,
		EE_CONNECT,
		EE_BIND,
		EE_LISTEN,
		EE_RESOLVE,
		EE_ACCEPT,
		EE_SHUTDOWN,
		EE_WOULDBLOCK,

		EE_MAX,
		EE_UNKNOWN,
	};

	enum EShutdownDir
	{
		ESD_READ = 0,
		ESD_WRITE,
		ESD_BOTH,
	};

	struct Peer
	{
		c8  Data[XPF_NETENDPOINT_MAXADDRLEN];
		s32 Length;
	};

	static const u32 ProtocolTCP  = 0x1;
	static const u32 ProtocolUDP  = 0x2;
	static const u32 ProtocolIPv4 = 0x100;
	static const u32 ProtocolIPv6 = 0x200;

	static NetEndpoint* create(u32 protocol);
	static NetEndpoint* create(u32 protocol, const c8 *addr, u32 port, u32 *errorcode = 0, u32 backlog = 10);
	static void         release(NetEndpoint* ep);
	static bool         resolvePeer(u32 protocol, Peer &peer, const c8 * host, const c8 * serv, u32 port = 0);

	explicit NetEndpoint(u32 protocol);
	virtual ~NetEndpoint();

	// Outgoing endpoint only
	bool         connect(const c8 *addr, u32 port, u32 *errorcode = 0);

	// Incoming endpoint only
	bool         listen (const c8 *addr, u32 port, u32 *errorcode = 0, u32 backlog = 10);
	NetEndpoint* accept (u32 *errorcode = 0);

	s32          recv     ( c8 *buf, s32 len, u32 *errorcode = 0);
	s32          recvFrom ( Peer *peer, c8 *buf, s32 len, u32 *errorcode = 0);
	s32          send     ( const c8 *buf, s32 len, u32 *errorcode = 0);
	s32          sendTo   ( const Peer *peer, const c8 *buf, s32 len, u32 *errorcode = 0);
	void         shutdown ( EShutdownDir dir, u32 *errorcode = 0);
	void         close    ( );

	EStatus      getStatus() const;
	const c8*    getAddress() const;
	u32          getPort() const;
	u32          getProtocol() const;
	s32          getSocket() const;
	vptr         getUserData() const;
	vptr         setUserData(vptr ud);

	// TODO: getsockopt setsockopt

private:
	static bool       platformInit();

	NetEndpoint();
	NetEndpoint(u32 protocol, int socket, EStatus status);

	// Non-copyable
	NetEndpoint(const NetEndpoint& that) {}
	NetEndpoint& operator = (const NetEndpoint& that) { return *this; }

	inline void setStatus(EStatus status);
	inline vptr       getAsyncContext() const { return pAsyncContext; }
	inline vptr       setAsyncContext(vptr newdata) { vptr olddata = pAsyncContext; pAsyncContext = newdata; return olddata; }

	NetEndpointImpl *pImpl;
	vptr             pAsyncContext;
	vptr             pUserData;

	friend class NetIoMuxImpl;
};

}; // end of namespace xpf

#endif // _XPF_NETENDPOINT_HEADER_

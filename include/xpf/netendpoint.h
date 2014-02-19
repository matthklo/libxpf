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

class XPF_API NetEndpoint
{
public:
	enum EndpointTypeEnum
	{
		ETE_INCOMING = 0,
		ETE_OUTGOING,

		ETE_MAX,
		ETE_UNKNOWN,
	};

	enum EndpointStatusEnum
	{
		ESE_INIT = 0,
		ESE_LISTENING,
		ESE_CONNECTED,
		ESE_CLOSING,
		ESE_INVALID,

		ESE_MAX,
		ESE_UNKNOWN,
	};

	enum EndpointErrorEnum
	{
		EEE_SUCCESS = 0,
		EEE_INVALID_OP,
		EEE_RECV,
		EEE_SEND,
		EEE_CONNECT,
		EEE_BIND,
		EEE_LISTEN,
		EEE_RESOLVE,
		EEE_ACCEPT,
		EEE_SHUTDOWN,
		EEE_TIMEOUT,

		EEE_MAX,
		EEE_UNKNOWN,
	};

	enum EndpointShutdownDir
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

	static const u32 EndpointProtocolTCP  = 0x1;
	static const u32 EndpointProtocolUDP  = 0x2;
	static const u32 EndpointProtocolIPv4 = 0x100;
	static const u32 EndpointProtocolIPv6 = 0x200;

	static NetEndpoint* createEndpoint(EndpointTypeEnum type, u32 protocol, const c8 *addr, u32 port, u32 *errorcode = 0, u32 backlog = 10);

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
	void         shutdown ( EndpointShutdownDir dir, u32 *errorcode = 0);
	void         close    ( );

	inline EndpointStatusEnum getStatus() const;
	inline const c8*          getAddress() const;
	inline u32                getPort() const;
	inline u32                getProtocol() const;
	inline s32                getSocket() const;

	// TODO: getsockopt setsockopt

private:
	NetEndpoint();
	// Non-copyable
	NetEndpoint(const NetEndpoint& that);
	NetEndpoint& operator = (const NetEndpoint& that);

	vptr pImpl;
};

}; // end of namespace xpf

#endif // _XPF_NETENDPOINT_HEADER_
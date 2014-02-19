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

#include <xpf/netendpoint.h>
#include <xpf/string.h>
#include <xpf/lexicalcast.h>

#if defined(XPF_PLATFORM_WINDOWS)
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netdb.h>
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif

namespace xpf
{

struct NetEndpointDetail
{
	explicit NetEndpointDetail(u32 protocol)
		: Protocol(protocol)
	{
		platformInit();
		reset();
		Status = NetEndpoint::ESE_INIT;

		// Decide detail socket parameters.
		int family = AF_UNSPEC;
		int type = 0;
		int proto = 0;

		if (protocol & NetEndpoint::EndpointProtocolIPv4)
			family = AF_INET;
		else if (protocol & NetEndpoint::EndpointProtocolIPv6)
			family = AF_INET6;

		xpfAssert( ("Invalid procotol specified.", AF_UNSPEC != family) );
		if ( AF_UNSPEC == family )
		{
			Status = NetEndpoint::ESE_INVALID;
			return;
		}

		if (protocol & NetEndpoint::EndpointProtocolTCP)
		{
			type = SOCK_STREAM;
			proto = IPPROTO_TCP;
		}
		else if (protocol & NetEndpoint::EndpointProtocolUDP)
		{
			type = SOCK_DGRAM;
			proto = IPPROTO_UDP;
		}

		xpfAssert( ("Invalid procotol specified.", ((type != 0) && (proto != 0))) );
		if (type == 0 || proto == 0)
		{
			Status = NetEndpoint::ESE_INVALID;
			return;
		}

		Socket = ::socket(family, type, proto);
		if (INVALID_SOCKET == Socket)
		{
			Status = NetEndpoint::ESE_INVALID;
		}
	}

	NetEndpointDetail(u32 protocol, int socket, NetEndpoint::EndpointStatusEnum status = NetEndpoint::ESE_CONNECTED)
		: Protocol(protocol)
	{
		reset();
		Status = status;
		Socket = socket;

		int addrlen = XPF_NETENDPOINT_MAXADDRLEN;
		int ec = ::getsockname(socket, (struct sockaddr*)&SockAddr, &addrlen);
		xpfAssert( ("Valid socket provisioning.", (ec == 0) && (addrlen < XPF_NETENDPOINT_MAXADDRLEN)) );

		if (ec == 0)
		{
			c8 servbuf[128];
			ec = ::getnameinfo((struct sockaddr*)&SockAddr, addrlen,
					Address, XPF_NETENDPOINT_MAXADDRLEN, servbuf, 128, 
					NI_NUMERICHOST | NI_NUMERICSERV);
			if (ec == 0)
			{
				Port = lexical_cast<u32>(servbuf);
				return;
			}
		}

		// abnormal case.
		close();
	}

	~NetEndpointDetail()
	{
		close();
	}

	bool connect(const c8 *addr, u32 port, u32 *errorcode)
	{
		bool ret = false;

		if (0 == addr)
			addr = "localhost";

		xpfAssert( ("Stale endpoint.", NetEndpoint::ESE_INIT == Status) );
		if ( (0 == port) || (NetEndpoint::ESE_INIT != Status) )
		{
			if (errorcode)
				*errorcode = (u32)NetEndpoint::EEE_INVALID_OP;
			return false;
		}

		string portStr = lexical_cast<c8>(port);

		// perform getaddrinfo() to prepare proper sockaddr data.
		struct addrinfo hint = {0};
		struct addrinfo *results;
		hint.ai_family = (Protocol & NetEndpoint::EndpointProtocolIPv6)? AF_INET6 : AF_INET;
		hint.ai_socktype = (Protocol & NetEndpoint::EndpointProtocolTCP)? SOCK_STREAM : SOCK_DGRAM;
		hint.ai_protocol = (Protocol & NetEndpoint::EndpointProtocolTCP)? IPPROTO_TCP : IPPROTO_UDP;
		hint.ai_flags = (AF_INET6 == hint.ai_family) ? AI_V4MAPPED : 0;

		int ec = ::getaddrinfo(addr, portStr.c_str(), &hint, &results);
		if (0 != ec)
		{
			if (errorcode)
				*errorcode = (u32)NetEndpoint::EEE_RESOLVE;
			return false;
		}

		xpfAssert( ("Expecting valid results from getaddrinfo().", 
			(results != 0) && (results->ai_addrlen > 0) && (results->ai_addrlen <= XPF_NETENDPOINT_MAXADDRLEN)) );

		do
		{
			// Always use the 1st record in results.
			ec = ::connect(Socket, results->ai_addr, results->ai_addrlen);
			if (0 != ec)
				break;

			c8 *sockaddrdata = (c8*)results->ai_addr;
			for (u32 i=0; i<results->ai_addrlen; ++i)
			{
				SockAddr[i] = sockaddrdata[i];
			}

			ret = true;
			Status = NetEndpoint::ESE_CONNECTED;
			Port = port;

			ec = ::getnameinfo(results->ai_addr, results->ai_addrlen, Address, XPF_NETENDPOINT_MAXADDRLEN,
					0, 0, NI_NUMERICHOST);
			if (0 != ec)
			{
				xpfAssert( ("Expecting getnameinfo succeed after connect.", false) );
				for (int i=0; i<XPF_NETENDPOINT_MAXADDRLEN; ++i)
				{
					Address[i] = addr[i];
					if (addr[i] == '\0')
						break;
				}
			}

		} while (0);

		if (errorcode)
			*errorcode = (ret)? (u32)NetEndpoint::EEE_SUCCESS : (u32)NetEndpoint::EEE_CONNECT;
		freeaddrinfo(results);
		return ret;
	}

	// TCP: do bind() and listen(). UDP: do bind().
	bool listen (const c8 *addr, u32 port, u32 *errorcode, u32 backlog)
	{
		bool ret = false;

		xpfAssert( ("Stale endpoint.", NetEndpoint::ESE_INIT == Status) );
		if ( (NetEndpoint::ESE_INIT != Status) || (0 == port) )
		{
			if (errorcode)
				*errorcode = (u32)NetEndpoint::EEE_INVALID_OP;
			return false;
		}

		string portStr = lexical_cast<c8>(port);

		// perform getaddrinfo() to prepare proper sockaddr data.
		struct addrinfo hint = {0};
		struct addrinfo *results;
		hint.ai_family = (Protocol & NetEndpoint::EndpointProtocolIPv6)? AF_INET6 : AF_INET;
		hint.ai_socktype = (Protocol & NetEndpoint::EndpointProtocolTCP)? SOCK_STREAM : SOCK_DGRAM;
		hint.ai_protocol = (Protocol & NetEndpoint::EndpointProtocolTCP)? IPPROTO_TCP : IPPROTO_UDP;
		hint.ai_flags = 0;

		if (0 == addr)
			hint.ai_flags |= AI_PASSIVE;
		if (AF_INET6 == hint.ai_family)
			hint.ai_flags |= AI_V4MAPPED;

		int ec = ::getaddrinfo(addr, portStr.c_str(), &hint, &results);
		if (0 != ec)
		{
			if (errorcode)
				*errorcode = (u32)NetEndpoint::EEE_RESOLVE;
			return false;
		}

		xpfAssert( ("Expecting valid results from getaddrinfo().", 
			(results != 0) && (results->ai_addrlen > 0) && (results->ai_addrlen <= XPF_NETENDPOINT_MAXADDRLEN)) );

		do
		{
			ec = ::bind(Socket, results->ai_addr, results->ai_addrlen);
			if (0 != ec)
			{
				if (errorcode)
					*errorcode = (u32)NetEndpoint::EEE_BIND;
				break;
			}

			// for TCP endpoint, need to do listen().
			if ( (Protocol & NetEndpoint::EndpointProtocolTCP) != 0 )
			{
				ec = ::listen(Socket, backlog);
				if (0 != ec)
				{
					if (errorcode)
						*errorcode = (u32)NetEndpoint::EEE_LISTEN;
					Status = NetEndpoint::ESE_INVALID;
					break;
				}
			}

			Status = NetEndpoint::ESE_LISTENING;
			if (errorcode)
				*errorcode = (u32)NetEndpoint::EEE_SUCCESS;
			ret = true;

		} while (0);

		freeaddrinfo(results);
		return ret;
	}

	// TCP only.
	NetEndpointDetail* accept (u32 *errorcode)
	{
		bool isTcp = ( (Protocol & NetEndpoint::EndpointProtocolTCP) != 0 );
		xpfAssert( ("Expecting TCP endpoint.", isTcp) );
		xpfAssert( ("Not a listening endpoint.", Status == NetEndpoint::ESE_LISTENING) );
		if (!isTcp || (Status != NetEndpoint::ESE_LISTENING))
		{
			if (errorcode)
				*errorcode = (u32) NetEndpoint::EEE_INVALID_OP;
			return 0;
		}

		int acceptedSocket = ::accept(Socket, 0, 0);
		xpfAssert( ("Expecting a valid accepted socket.", acceptedSocket != INVALID_SOCKET) );
		if (acceptedSocket == INVALID_SOCKET)
		{
			if (errorcode)
				*errorcode = (u32) NetEndpoint::EEE_ACCEPT;
			return 0;
		}

		NetEndpointDetail *peer = new NetEndpointDetail(Protocol, acceptedSocket);
		if (errorcode)
			*errorcode = (u32) NetEndpoint::EEE_SUCCESS;
		return peer;
	}

	// TCP and UDP
	s32 recv (c8 *buf, s32 len, u32 *errorcode)
	{
		bool isTcp = ((Protocol & NetEndpoint::EndpointProtocolTCP) != 0);
		if (isTcp)
		{
			xpfAssert( ("Not a connected TCP endpoint.", Status == NetEndpoint::ESE_CONNECTED) );
		}
		else
		{
			xpfAssert( ("Not a listening UDP endpoint.", Status == NetEndpoint::ESE_LISTENING) );
		}

		if ((isTcp && (Status != NetEndpoint::ESE_CONNECTED)) ||
			(!isTcp && (Status != NetEndpoint::ESE_LISTENING)))
		{
			if (errorcode)
				*errorcode = (u32) NetEndpoint::EEE_INVALID_OP;
			return 0;
		}

		s32 cnt = ::recv(Socket, buf, len, 0);
		if (cnt < 0)
		{
			if (errorcode)
				*errorcode = (u32) NetEndpoint::EEE_RECV;
			return 0;
		}

		if (errorcode)
			*errorcode = (u32) NetEndpoint::EEE_SUCCESS;
		return cnt;
	}

	// UDP only
	s32 recvFrom (NetEndpoint::Peer *peer, c8 *buf, s32 len, u32 *errorcode)
	{
		bool isUdp = ((Protocol & NetEndpoint::EndpointProtocolUDP) != 0);
		xpfAssert( ("Expecting a valid peer.", peer != 0) );
		xpfAssert( ("Expecting an UDP endpoint.", isUdp) );
		xpfAssert( ("Neither a connected nor a listening endpoint.", (Status == NetEndpoint::ESE_CONNECTED || Status == NetEndpoint::ESE_LISTENING)) );
		if (!isUdp || !peer || ((Status != NetEndpoint::ESE_CONNECTED) && (Status != NetEndpoint::ESE_LISTENING)))
		{
			if (errorcode)
				*errorcode = (u32) NetEndpoint::EEE_INVALID_OP;
			return 0;
		}

		peer->Length = XPF_NETENDPOINT_MAXADDRLEN;
		s32 cnt = ::recvfrom(Socket, buf, len, 0, (struct sockaddr*)&peer->Data[0], &peer->Length);
		if (cnt < 0)
		{
			if (errorcode)
				*errorcode = (u32) NetEndpoint::EEE_RECV;
			return 0;
		}

		if (errorcode)
			*errorcode = (u32) NetEndpoint::EEE_SUCCESS;
		return cnt;
	}

	// TCP and UDP
	s32 send (const c8 *buf, s32 len, u32 *errorcode)
	{
		xpfAssert( ("Not a connected endpoint.", Status == NetEndpoint::ESE_CONNECTED) );
		if (Status != NetEndpoint::ESE_CONNECTED)
		{
			if (errorcode)
				*errorcode = (u32) NetEndpoint::EEE_INVALID_OP;
			return 0;
		}

		s32 cnt = ::send(Socket, buf, len, 0);
		if (cnt < 0)
		{
			if (errorcode)
				*errorcode = (u32) NetEndpoint::EEE_SEND;
			return 0;
		}

		if (errorcode)
			*errorcode = (u32) NetEndpoint::EEE_SUCCESS;
		return cnt;
	}

	// UDP only
	s32 sendTo (const NetEndpoint::Peer *peer, const c8 *buf, s32 len, u32 *errorcode)
	{
		const bool isUdp = ((Protocol & NetEndpoint::EndpointProtocolUDP) != 0);
		xpfAssert( ("Expecting a valid peer.", peer != 0) );
		xpfAssert( ("Expecting an UDP endpoint.", isUdp) );
		xpfAssert( ("Not a connected endpoint.", Status == NetEndpoint::ESE_CONNECTED) );
		if (!isUdp || !peer || (Status != NetEndpoint::ESE_CONNECTED))
		{
			if (errorcode)
				*errorcode = (u32) NetEndpoint::EEE_INVALID_OP;
			return 0;
		}

		s32 cnt = ::sendto(Socket, buf, len, 0, (const struct sockaddr*)&peer->Data[0], peer->Length);
		if (cnt < 0)
		{
			if (errorcode)
				*errorcode = (u32) NetEndpoint::EEE_SEND;
			return 0;
		}

		if (errorcode)
			*errorcode = (u32) NetEndpoint::EEE_SUCCESS;
		return cnt;
	}

	void shutdown(NetEndpoint::EndpointShutdownDir dir, u32 *errorcode)
	{
		s32 shutdownFlag = 0;
		switch (dir)
		{
#ifdef XPF_PLATFORM_WINDOWS
		case NetEndpoint::ESD_READ:
			shutdownFlag = SD_RECEIVE;
			break;
		case SD_SEND:
			shutdownFlag = SD_SEND;
			break;
		case SD_BOTH:
			shutdownFlag = SD_BOTH;
			break;
#else
		case NetEndpoint::ESD_READ:
			shutdownFlag = SHUT_RD;
			break;
		case SD_SEND:
			shutdownFlag = SHUT_WR;
			break;
		case SD_BOTH:
			shutdownFlag = SHUT_RDWR;
			break;
#endif
		default:
			xpfAssert( ("Invalid shutdown flag.", false) );
			if (errorcode)
				*errorcode = (u32)NetEndpoint::EEE_INVALID_OP;
			return;
		}

		s32 ec = ::shutdown(Socket, shutdownFlag);
		if (errorcode)
			*errorcode = (0 == ec) ? (u32)NetEndpoint::EEE_SUCCESS : (u32) NetEndpoint::EEE_SHUTDOWN;
	}

	void close ()
	{
		Status = NetEndpoint::ESE_CLOSING;
		if (Socket != INVALID_SOCKET)
		{
			shutdown(NetEndpoint::ESD_BOTH, 0);
#ifdef XPF_PLATFORM_WINDOWS
			::closesocket(Socket);
#else
			::close(Socket);
#endif
		}
		reset();
	}

	void reset()
	{
		Port = 0;
		Status = NetEndpoint::ESE_INVALID;
		Socket = INVALID_SOCKET;

		for (int i=0; i<XPF_NETENDPOINT_MAXADDRLEN; ++i)
		{
			Address[i] = 0;
			SockAddr[i] = 0;
		}
	}

#if defined(XPF_PLATFORM_WINDOWS)
	bool platformInit()
	{
		static bool initialized = false;

		// Do WSAStartup if never did before.
		if (!initialized)
		{
			WORD wVersionRequested;
			WSADATA wsaData;

			wVersionRequested = MAKEWORD(2, 2);

			if (0 == ::WSAStartup(wVersionRequested, &wsaData))
			{
				initialized = true;
			}
			else
			{
				xpfAssert( ("WSAStartup failed.", false) );
				Status = NetEndpoint::ESE_INVALID;
			}
		} // end of if (!initialized)
		return initialized;
	}
#else
	inline bool platformInit() { return true; }
#endif

	c8                               Address[XPF_NETENDPOINT_MAXADDRLEN]; // Always in numeric form.
	c8                               SockAddr[XPF_NETENDPOINT_MAXADDRLEN];
	u32                              Port;
	const u32                        Protocol;
	NetEndpoint::EndpointStatusEnum  Status;
	s32                              Socket;
}; // end of struct NetEndpointDetail


//============------------- NetEndpoint Impl. ------------======================//

NetEndpoint::NetEndpoint()
	: pImpl(0)
{
}

NetEndpoint::NetEndpoint(u32 protocol)
{
	pImpl = (vptr) new NetEndpointDetail(protocol);
}

NetEndpoint::~NetEndpoint()
{
	if (pImpl != 0)
	{
		delete (NetEndpointDetail*) pImpl;
		pImpl = 0;
	}
}

NetEndpoint* NetEndpoint::createEndpoint(EndpointTypeEnum type, u32 protocol, const c8 *addr, u32 port, u32 *errorcode, u32 backlog)
{
	NetEndpoint *ret = 0;

	switch (type)
	{
	case ETE_OUTGOING:
		ret = new NetEndpoint(protocol);
		ret->connect(addr, port, errorcode);
		break;
	case ETE_INCOMING:
		ret = new NetEndpoint(protocol);
		ret->listen(addr, port, errorcode, backlog);
		break;
	default:
		xpfAssert( ("Unknown endpoint type.", false) );
		break;
	}

	return ret;
}

bool NetEndpoint::connect(const c8 *addr, u32 port, u32 *errorcode)
{
	return ((NetEndpointDetail*)pImpl)->connect(addr, port, errorcode);
}

bool NetEndpoint::listen (const c8 *addr, u32 port, u32 *errorcode, u32 backlog)
{
	return ((NetEndpointDetail*)pImpl)->listen(addr, port, errorcode, backlog);
}

NetEndpoint* NetEndpoint::accept (u32 *errorcode)
{
	NetEndpointDetail *peerDetail = ((NetEndpointDetail*)pImpl)->accept(errorcode);
	if (peerDetail == 0)
	{
		return 0;
	}

	NetEndpoint *peer = new NetEndpoint;
	peer->pImpl = (vptr) peerDetail;
	return peer;
}

s32 NetEndpoint::recv ( c8 *buf, s32 len, u32 *errorcode )
{
	return ((NetEndpointDetail*)pImpl)->recv(buf, len, errorcode);
}

s32 NetEndpoint::recvFrom ( Peer *peer, c8 *buf, s32 len, u32 *errorcode )
{
	return ((NetEndpointDetail*)pImpl)->recvFrom(peer, buf, len, errorcode);
}

s32 NetEndpoint::send ( const c8 *buf, s32 len, u32 *errorcode )
{
	return ((NetEndpointDetail*)pImpl)->send(buf, len, errorcode);
}

s32 NetEndpoint::sendTo ( const Peer *peer, const c8 *buf, s32 len, u32 *errorcode )
{
	return ((NetEndpointDetail*)pImpl)->sendTo(peer, buf, len, errorcode);
}

void NetEndpoint::shutdown(NetEndpoint::EndpointShutdownDir dir, u32 *errorcode)
{
	return ((NetEndpointDetail*)pImpl)->shutdown(dir, errorcode);
}

void NetEndpoint::close ()
{
	((NetEndpointDetail*)pImpl)->close();
}

NetEndpoint::EndpointStatusEnum NetEndpoint::getStatus() const
{
	return ((NetEndpointDetail*)pImpl)->Status;
}

const c8* NetEndpoint::getAddress() const
{
	return ((NetEndpointDetail*)pImpl)->Address;
}

u32 NetEndpoint::getPort() const
{
	return ((NetEndpointDetail*)pImpl)->Port;
}

u32 NetEndpoint::getProtocol() const
{
	return ((NetEndpointDetail*)pImpl)->Protocol;
}

int NetEndpoint::getSocket() const
{
	return ((NetEndpointDetail*)pImpl)->Socket;
}

}; // end of namespace xpf


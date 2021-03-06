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

#include <cstring>

#if defined(XPF_PLATFORM_WINDOWS)
#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <netdb.h>
#include <unistd.h>
#include <errno.h>
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET (-1)
#endif

namespace xpf
{

class NetEndpointImpl
{
public:

	explicit NetEndpointImpl(u32 protocol)
		: Protocol(protocol)
	{
		reset();

		if (!platformInit())
		{
#ifdef XPF_PLATFORM_WINDOWS
			Errno = WSANOTINITIALISED;
#endif
			return;
		}

		Status = NetEndpoint::ESTAT_INIT;

		// Decide detail socket parameters.
		int family = AF_UNSPEC;
		int type = 0;
		int proto = 0;

		if (protocol & NetEndpoint::ProtocolIPv4)
			family = AF_INET;
		else if (protocol & NetEndpoint::ProtocolIPv6)
			family = AF_INET6;

		xpfAssert( ("Invalid procotol specified.", AF_UNSPEC != family) );
		if ( AF_UNSPEC == family )
		{
			Status = NetEndpoint::ESTAT_INVALID;
			return;
		}

		if (protocol & NetEndpoint::ProtocolTCP)
		{
			type = SOCK_STREAM;
			proto = IPPROTO_TCP;
		}
		else if (protocol & NetEndpoint::ProtocolUDP)
		{
			type = SOCK_DGRAM;
			proto = IPPROTO_UDP;
		}

		xpfAssert( ("Invalid procotol specified.", ((type != 0) && (proto != 0))) );
		if (type == 0 || proto == 0)
		{
			Status = NetEndpoint::ESTAT_INVALID;
			return;
		}

		Socket = ::socket(family, type, proto);
		if (INVALID_SOCKET == Socket)
		{
			saveLastError();
			Status = NetEndpoint::ESTAT_INVALID;
		}
	}

	NetEndpointImpl(u32 protocol, int socket, NetEndpoint::EStatus status = NetEndpoint::ESTAT_CONNECTED)
		: Protocol(protocol)
	{
		reset();
		Status = status;
		Socket = socket;

		SockInfo.Length = XPF_NETENDPOINT_MAXADDRLEN;
		int ec = ::getsockname(socket, (struct sockaddr*)SockInfo.Data, (socklen_t*)&SockInfo.Length);
		xpfAssert( ("Valid socket provisioning.", (ec == 0) && (SockInfo.Length < XPF_NETENDPOINT_MAXADDRLEN)) );

		if (ec == 0)
		{
			c8 servbuf[128];
			ec = ::getnameinfo((struct sockaddr*)SockInfo.Data, (socklen_t) SockInfo.Length,
					Address, XPF_NETENDPOINT_MAXADDRLEN, servbuf, 128,
					NI_NUMERICHOST | NI_NUMERICSERV);
			if (ec == 0)
			{
				Port = lexical_cast<u32, c8>(&servbuf[0]);
				return;
			}
		}

		// abnormal case.
		saveLastError();
		close();
	}

	~NetEndpointImpl()
	{
		close();
	}

	bool connect(const c8 *addr, const c8 *serviceOrPort, u32 *errorcode)
	{
		bool ret = false;

		if (0 == addr)
			addr = "localhost";

		xpfAssert(("Stale endpoint.", NetEndpoint::ESTAT_INIT == Status));
		if ((0 == serviceOrPort) || (NetEndpoint::ESTAT_INIT != Status))
		{
			if (errorcode)
				*errorcode = (u32)NetEndpoint::EE_INVALID_OP;
			return false;
		}

		NetEndpoint::Peer peer;
		if (!NetEndpoint::resolvePeer(Protocol, peer, addr, serviceOrPort))
		{
			if (errorcode)
				*errorcode = (u32)NetEndpoint::EE_RESOLVE;
			return false;
		}

		xpfAssert( ("Expecting valid results from getaddrinfo().",
			(peer.Length > 0) && (peer.Length <= XPF_NETENDPOINT_MAXADDRLEN)));

		do
		{
			// Always use the 1st record in results.
			int ec = ::connect(Socket, (const sockaddr*)peer.Data, peer.Length);
			if (0 != ec)
			{
				saveLastError();
				break;
			}

			std::memcpy(SockInfo.Data, peer.Data, peer.Length);
			SockInfo.Length = peer.Length;

			ret = true;
			Status = NetEndpoint::ESTAT_CONNECTED;

			c8 servbuf[128];
			ec = ::getnameinfo((const sockaddr*)peer.Data, peer.Length, Address, XPF_NETENDPOINT_MAXADDRLEN,
					servbuf, 128, NI_NUMERICHOST | NI_NUMERICSERV);
			if (0 != ec)
			{
				saveLastError();
				xpfAssert( ("Expecting getnameinfo succeed after connect.", false) );
				for (int i=0; i<XPF_NETENDPOINT_MAXADDRLEN; ++i)
				{
					Address[i] = addr[i];
					if (addr[i] == '\0')
						break;
				}
				Port = 0;
			}
			else
			{
				Port = lexical_cast<u32, c8>(&servbuf[0]);
			}

		} while (0);

		if (errorcode)
			*errorcode = (ret)? (u32)NetEndpoint::EE_SUCCESS : (u32)NetEndpoint::EE_CONNECT;
		return ret;
	}

	// TCP: do bind() and listen(). UDP: do bind().
	bool listen (const c8 *addr, const c8 *serviceOrPort, u32 *errorcode, u32 backlog)
	{
		bool ret = false;

		xpfAssert(("Stale endpoint.", NetEndpoint::ESTAT_INIT == Status));
		if ((NetEndpoint::ESTAT_INIT != Status) || (0 == serviceOrPort))
		{
			if (errorcode)
				*errorcode = (u32)NetEndpoint::EE_INVALID_OP;
			return false;
		}

		// perform getaddrinfo() to prepare proper sockaddr data.
		struct addrinfo hint = {0};
		struct addrinfo *results;
		hint.ai_family = (Protocol & NetEndpoint::ProtocolIPv6)? AF_INET6 : AF_INET;
		hint.ai_socktype = (Protocol & NetEndpoint::ProtocolTCP)? SOCK_STREAM : SOCK_DGRAM;
		hint.ai_protocol = (Protocol & NetEndpoint::ProtocolTCP)? IPPROTO_TCP : IPPROTO_UDP;
		hint.ai_flags = 0;

		if (0 == addr)
			hint.ai_flags |= AI_PASSIVE;
		if (AF_INET6 == hint.ai_family)
			hint.ai_flags |= AI_V4MAPPED;

		int ec = ::getaddrinfo(addr, serviceOrPort, &hint, &results);
		if (0 != ec)
		{
			if (errorcode)
				*errorcode = (u32)NetEndpoint::EE_RESOLVE;
			saveLastError();
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
					*errorcode = (u32)NetEndpoint::EE_BIND;
				saveLastError();
				break;
			}

			// for TCP endpoint, need to do listen().
			if ( (Protocol & NetEndpoint::ProtocolTCP) != 0 )
			{
				ec = ::listen(Socket, backlog);
				if (0 != ec)
				{
					if (errorcode)
						*errorcode = (u32)NetEndpoint::EE_LISTEN;
					Status = NetEndpoint::ESTAT_INVALID;
					saveLastError();
					break;
				}
			}

			std::memcpy(SockInfo.Data, results->ai_addr, results->ai_addrlen);
			SockInfo.Length = (s32) results->ai_addrlen;

			c8 servbuf[128];
			ec = ::getnameinfo((const sockaddr*)results->ai_addr, results->ai_addrlen, Address, XPF_NETENDPOINT_MAXADDRLEN,
					servbuf, 128, NI_NUMERICHOST | NI_NUMERICSERV);
			if (0 != ec)
			{
				saveLastError();
				xpfAssert( ("Expecting getnameinfo succeed after listen.", false) );
				for (int i=0; i<XPF_NETENDPOINT_MAXADDRLEN; ++i)
				{
					Address[i] = addr[i];
					if (addr[i] == '\0')
						break;
				}
				Port = 0;
			}
			else
			{
				Port = lexical_cast<u32, c8>(&servbuf[0]);
			}

			Status = NetEndpoint::ESTAT_LISTENING;
			if (errorcode)
				*errorcode = (u32)NetEndpoint::EE_SUCCESS;
			ret = true;

		} while (0);

		freeaddrinfo(results);
		return ret;
	}

	// TCP only.
	NetEndpointImpl* accept (u32 *errorcode)
	{
		bool isTcp = ( (Protocol & NetEndpoint::ProtocolTCP) != 0 );
		xpfAssert( ("Expecting TCP endpoint.", isTcp) );
		xpfAssert(("Not a listening endpoint.", Status == NetEndpoint::ESTAT_LISTENING));
		if (!isTcp || (Status != NetEndpoint::ESTAT_LISTENING))
		{
			if (errorcode)
				*errorcode = (u32) NetEndpoint::EE_INVALID_OP;
			return 0;
		}

		int acceptedSocket = ::accept(Socket, 0, 0);
		if (acceptedSocket == INVALID_SOCKET)
		{
			if (errorcode)
				*errorcode = (u32) NetEndpoint::EE_ACCEPT;
			saveLastError();
			return 0;
		}

		NetEndpointImpl *peer = new NetEndpointImpl(Protocol, acceptedSocket);
		if (errorcode)
			*errorcode = (u32) NetEndpoint::EE_SUCCESS;
		return peer;
	}

	// TCP and UDP
	s32 recv (c8 *buf, s32 len, u32 *errorcode)
	{
		bool isTcp = ((Protocol & NetEndpoint::ProtocolTCP) != 0);
		if (isTcp)
		{
			xpfAssert(("Not a connected TCP endpoint.", Status == NetEndpoint::ESTAT_CONNECTED));
		}
		else
		{
			xpfAssert(("Not a listening UDP endpoint.", Status == NetEndpoint::ESTAT_LISTENING));
		}

		if ((isTcp && (Status != NetEndpoint::ESTAT_CONNECTED)) ||
			(!isTcp && (Status != NetEndpoint::ESTAT_LISTENING)))
		{
			if (errorcode)
				*errorcode = (u32) NetEndpoint::EE_INVALID_OP;
			return 0;
		}

		s32 cnt = ::recv(Socket, buf, len, 0);
		if (cnt < 0)
		{
			if (errorcode)
				*errorcode = (u32) NetEndpoint::EE_RECV;
			saveLastError();
			return 0;
		}

		if (errorcode)
			*errorcode = (u32) NetEndpoint::EE_SUCCESS;
		return cnt;
	}

	// UDP only
	s32 recvFrom (NetEndpoint::Peer *peer, c8 *buf, s32 len, u32 *errorcode)
	{
		bool isUdp = ((Protocol & NetEndpoint::ProtocolUDP) != 0);
		xpfAssert( ("Expecting a valid peer.", peer != 0) );
		xpfAssert( ("Expecting an UDP endpoint.", isUdp) );
		xpfAssert(("Neither a connected nor a listening endpoint.", (Status == NetEndpoint::ESTAT_CONNECTED || Status == NetEndpoint::ESTAT_LISTENING)));
		if (!isUdp || !peer || ((Status != NetEndpoint::ESTAT_CONNECTED) && (Status != NetEndpoint::ESTAT_LISTENING)))
		{
			if (errorcode)
				*errorcode = (u32) NetEndpoint::EE_INVALID_OP;
			return 0;
		}

		peer->Length = XPF_NETENDPOINT_MAXADDRLEN;
		s32 cnt = ::recvfrom(Socket, buf, len, 0, (struct sockaddr*)&peer->Data[0], (socklen_t*)&peer->Length);
		if (cnt < 0)
		{
			if (errorcode)
				*errorcode = (u32) NetEndpoint::EE_RECV;
			saveLastError();
			return 0;
		}

		if (errorcode)
			*errorcode = (u32) NetEndpoint::EE_SUCCESS;
		return cnt;
	}

	// TCP and UDP
	s32 send (const c8 *buf, s32 len, u32 *errorcode)
	{
		xpfAssert(("Not a connected endpoint.", Status == NetEndpoint::ESTAT_CONNECTED));
		if (Status != NetEndpoint::ESTAT_CONNECTED)
		{
			if (errorcode)
				*errorcode = (u32) NetEndpoint::EE_INVALID_OP;
			return 0;
		}

		s32 cnt = ::send(Socket, buf, len, 0);
		if (cnt < 0)
		{
			if (errorcode)
				*errorcode = (u32) NetEndpoint::EE_SEND;
			saveLastError();
			return 0;
		}

		if (errorcode)
			*errorcode = (u32) NetEndpoint::EE_SUCCESS;
		return cnt;
	}

	// UDP only
	s32 sendTo (const NetEndpoint::Peer *peer, const c8 *buf, s32 len, u32 *errorcode)
	{
		const bool isUdp = ((Protocol & NetEndpoint::ProtocolUDP) != 0);
		xpfAssert( ("Expecting a valid peer.", peer != 0) );
		xpfAssert( ("Expecting an UDP endpoint.", isUdp) );
		xpfAssert( ("Not a connected endpoint.", Status == NetEndpoint::ESTAT_CONNECTED) );
		if (!isUdp || !peer || (Status != NetEndpoint::ESTAT_CONNECTED))
		{
			if (errorcode)
				*errorcode = (u32) NetEndpoint::EE_INVALID_OP;
			return 0;
		}

		s32 cnt = ::sendto(Socket, buf, len, 0, (const struct sockaddr*)&peer->Data[0], peer->Length);
		if (cnt < 0)
		{
			if (errorcode)
				*errorcode = (u32) NetEndpoint::EE_SEND;
			saveLastError();
			return 0;
		}

		if (errorcode)
			*errorcode = (u32) NetEndpoint::EE_SUCCESS;
		return cnt;
	}

	void shutdown(NetEndpoint::EShutdownDir dir, u32 *errorcode)
	{
		s32 shutdownFlag = 0;
		switch (dir)
		{
#ifdef XPF_PLATFORM_WINDOWS
		case NetEndpoint::ESD_READ:
			shutdownFlag = SD_RECEIVE;
			break;
		case NetEndpoint::ESD_WRITE:
			shutdownFlag = SD_SEND;
			break;
		case NetEndpoint::ESD_BOTH:
			shutdownFlag = SD_BOTH;
			break;
#else
		case NetEndpoint::ESD_READ:
			shutdownFlag = SHUT_RD;
			break;
		case NetEndpoint::ESD_WRITE:
			shutdownFlag = SHUT_WR;
			break;
		case NetEndpoint::ESD_BOTH:
			shutdownFlag = SHUT_RDWR;
			break;
#endif
		default:
			xpfAssert( ("Invalid shutdown flag.", false) );
			if (errorcode)
				*errorcode = (u32)NetEndpoint::EE_INVALID_OP;
			return;
		}

		s32 ec = ::shutdown(Socket, shutdownFlag);
		if (ec != 0)
			saveLastError();
		if (errorcode)
			*errorcode = (0 == ec) ? (u32)NetEndpoint::EE_SUCCESS : (u32) NetEndpoint::EE_SHUTDOWN;
	}

	void close ()
	{
		Status = NetEndpoint::ESTAT_CLOSING;
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
		Status = NetEndpoint::ESTAT_INVALID;
		Socket = INVALID_SOCKET;
		Errno = 0;

		SockInfo.Length = 0;
		for (int i=0; i<XPF_NETENDPOINT_MAXADDRLEN; ++i)
		{
			Address[i] = 0;
			SockInfo.Data[i] = 0;
		}
	}

	void saveLastError()
	{
#ifdef XPF_PLATFORM_WINDOWS
		Errno = WSAGetLastError();
#else
		Errno = errno;
#endif
	}

	static bool platformInit()
	{
#if defined(XPF_PLATFORM_WINDOWS)
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
			}
		} // end of if (!initialized)
		return initialized;
#else
		return true;
#endif
	}

	c8                    Address[XPF_NETENDPOINT_MAXADDRLEN]; // Always in numeric form.
	NetEndpoint::Peer     SockInfo;
	u32                   Port;
	const u32             Protocol;
	NetEndpoint::EStatus  Status;
	s32                   Socket;
	s32                   Errno;
}; // end of struct NetEndpointImpl


//============------------- NetEndpoint Impl. ------------======================//

NetEndpoint::NetEndpoint()
	: pImpl(0)
	, pAsyncContext(0)
	, pUserData(0)
{
}

NetEndpoint::NetEndpoint(u32 protocol)
	: pAsyncContext(0)
	, pUserData(0)
{
	pImpl = new NetEndpointImpl(protocol);
}

NetEndpoint::NetEndpoint(u32 protocol, int socket, NetEndpoint::EStatus status)
	: pAsyncContext(0)
	, pUserData(0)
{
	pImpl = new NetEndpointImpl(protocol, socket, status);
}

NetEndpoint::~NetEndpoint()
{
	if (pImpl != 0)
	{
		delete pImpl;
		pImpl = 0;
	}
}

NetEndpoint* NetEndpoint::create(u32 protocol)
{
	return new NetEndpoint(protocol);
}

NetEndpoint* NetEndpoint::create(u32 protocol, const c8 *addr, const c8 *serviceOrPort, u32 *errorcode, u32 backlog)
{
	NetEndpoint *ret = new NetEndpoint(protocol);
	if (!ret || !ret->listen(addr, serviceOrPort, errorcode, backlog))
	{
		delete ret;
		return 0;
	}
	return ret;
}

void NetEndpoint::release(NetEndpoint *ep)
{
	delete ep;
}

bool NetEndpoint::resolvePeer(u32 protocol, NetEndpoint::Peer &peer, const c8 * host, const c8 * serviceOrPort)
{
	struct addrinfo hint = { 0 };
	struct addrinfo *results;
	hint.ai_family = (protocol & NetEndpoint::ProtocolIPv6) ? AF_INET6 : AF_INET;
	hint.ai_socktype = (protocol & NetEndpoint::ProtocolTCP) ? SOCK_STREAM : SOCK_DGRAM;
	hint.ai_protocol = (protocol & NetEndpoint::ProtocolTCP) ? IPPROTO_TCP : IPPROTO_UDP;
	hint.ai_flags = (AF_INET6 == hint.ai_family) ? AI_V4MAPPED : 0;
	int ec = ::getaddrinfo(host, serviceOrPort, &hint, &results);
	if (0 != ec)
	{
		return false;
	}

	std::memcpy(peer.Data, results->ai_addr, results->ai_addrlen);
	peer.Length = (s32)results->ai_addrlen;
	::freeaddrinfo(results);
	return true;
}

bool NetEndpoint::platformInit()
{
	return NetEndpointImpl::platformInit();
}

bool NetEndpoint::connect(const c8 *addr, const c8 *serviceOrPort, u32 *errorcode)
{
	return pImpl->connect(addr, serviceOrPort, errorcode);
}

bool NetEndpoint::listen (const c8 *addr, const c8 *serviceOrPort, u32 *errorcode, u32 backlog)
{
	return pImpl->listen(addr, serviceOrPort, errorcode, backlog);
}

NetEndpoint* NetEndpoint::accept (u32 *errorcode)
{
	NetEndpointImpl *peerDetail = pImpl->accept(errorcode);
	if (peerDetail == 0)
	{
		return 0;
	}

	NetEndpoint *peer = new NetEndpoint;
	peer->pImpl = peerDetail;
	return peer;
}

s32 NetEndpoint::recv ( c8 *buf, s32 len, u32 *errorcode )
{
	return pImpl->recv(buf, len, errorcode);
}

s32 NetEndpoint::recvFrom ( Peer *peer, c8 *buf, s32 len, u32 *errorcode )
{
	return pImpl->recvFrom(peer, buf, len, errorcode);
}

s32 NetEndpoint::send ( const c8 *buf, s32 len, u32 *errorcode )
{
	return pImpl->send(buf, len, errorcode);
}

s32 NetEndpoint::sendTo ( const Peer *peer, const c8 *buf, s32 len, u32 *errorcode )
{
	return pImpl->sendTo(peer, buf, len, errorcode);
}

void NetEndpoint::shutdown(NetEndpoint::EShutdownDir dir, u32 *errorcode)
{
	return pImpl->shutdown(dir, errorcode);
}

void NetEndpoint::close ()
{
	pImpl->close();
}

NetEndpoint::EStatus NetEndpoint::getStatus() const
{
	return pImpl->Status;
}

void NetEndpoint::setStatus(NetEndpoint::EStatus status)
{
	pImpl->Status = status;
}

const c8* NetEndpoint::getAddress() const
{
	return pImpl->Address;
}

u32 NetEndpoint::getPort() const
{
	return pImpl->Port;
}

u32 NetEndpoint::getProtocol() const
{
	return pImpl->Protocol;
}

int NetEndpoint::getSocket() const
{
	return pImpl->Socket;
}

vptr NetEndpoint::getUserData() const
{
	return pUserData;
}

vptr NetEndpoint::setUserData(vptr ud)
{
	vptr olddata = pUserData;
	pUserData = ud;
	return olddata;
}

s32 NetEndpoint::getLastPlatformErrno() const
{
	return pImpl->Errno;
}

void NetEndpoint::setLastPlatformErrno(s32 ec)
{
	pImpl->Errno = ec;
}

}; // end of namespace xpf


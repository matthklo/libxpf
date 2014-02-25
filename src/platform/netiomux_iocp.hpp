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
#include <xpf/string.h>
#include <xpf/lexicalcast.h>

#ifdef _XPF_NETIOMUX_IMPL_INCLUDED_
#error Multiple NetIoMux implementation files included
#else
#define _XPF_NETIOMUX_IMPL_INCLUDED_
#endif

#include <WS2tcpip.h>
#include <WinSock2.h>
#include <Windows.h>
#include <Mswsock.h>
#include <string.h>

namespace xpf
{

struct NetIoMuxOverlapped : public OVERLAPPED
{
	WSABUF            Buffer;
	u32               IoType;
	NetIoMuxCallback *Callback;
	SOCKET            AcceptingSocket;
	NetEndpoint::Peer PeerData;
};

struct IocpAsyncContext
{
	IocpAsyncContext()
		: pAcceptEx(0), pConnectEx(0) {}
	LPFN_ACCEPTEX  pAcceptEx;
	LPFN_CONNECTEX pConnectEx;
};

class NetIoMuxImpl
{
public:
	NetIoMuxImpl()
		: mhIocp(INVALID_HANDLE_VALUE)
		, bEnable(true)
	{
		if (!NetEndpoint::platformInit())
			return;

		mhIocp = ::CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
		xpfAssert( ("Cannot create an empty IO completeion port.", NULL != mhIocp) );
	}

	~NetIoMuxImpl()
	{
		enable(false);

		if (mhIocp != INVALID_HANDLE_VALUE)
		{
			CloseHandle(mhIocp);
			mhIocp = INVALID_HANDLE_VALUE;
		}
	}

	void enable(bool val)
	{
		bEnable = val;
	}

	void run()
	{
		while (bEnable)
		{
			if (NetIoMux::ERS_DISABLED == runOnce(10))
				break;
		}
	}

	NetIoMux::ERunningStaus runOnce(u32 timeoutMs)
	{
		DWORD bytes = 0;
		ULONG_PTR key = 0;
		NetIoMuxOverlapped *odata = 0;
		BOOL ret = ::GetQueuedCompletionStatus(mhIocp, &bytes, &key, (LPOVERLAPPED*)&odata, timeoutMs);
		if (FALSE == ret)
		{
			if ((odata == 0) && (GetLastError() != ERROR_ABANDONED_WAIT_0))
				return NetIoMux::ERS_TIMEOUT;
			return NetIoMux::ERS_DISABLED;
		}

		NetEndpoint *ep = (NetEndpoint*)key;
		ret = ::GetOverlappedResult((HANDLE)ep->getSocket(), odata, &bytes, FALSE);

		NetEndpoint::EStatus st = ep->getStatus();
		NetIoMuxCallback *cb = odata->Callback;
		NetIoMux::EIoType iotype = (NetIoMux::EIoType)odata->IoType;
		switch (iotype)
		{
		case NetIoMux::EIT_ACCEPT:
			xpfAssert(NetEndpoint::ESTAT_ACCEPTING == st);
			ep->setStatus(NetEndpoint::ESTAT_LISTENING);
			if (ret == FALSE)
			{
				cb->onIoCompleted(iotype, NetEndpoint::EE_ACCEPT, ep, 0, 0, 0);
			}
			else
			{
				int listeningSocket = ep->getSocket();
				xpfAssert(("Expecting valid peer socket.", odata->AcceptingSocket != INVALID_SOCKET));

				int ec = setsockopt(odata->AcceptingSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
					(char *)&listeningSocket, sizeof(listeningSocket));
				xpfAssert(("Failed to update accepted context on newly connected socket.", ec == 0));

				NetEndpoint *tep = new NetEndpoint(ep->getProtocol(), odata->AcceptingSocket, NetEndpoint::ESTAT_CONNECTED);
				join(tep);
				cb->onIoCompleted(iotype, NetEndpoint::EE_SUCCESS, ep, (vptr)tep, 0, 0);
			}
			break;

		case NetIoMux::EIT_CONNECT:
			xpfAssert(NetEndpoint::ESTAT_CONNECTING == st);
			if (ret == FALSE)
			{
				ep->setStatus(NetEndpoint::ESTAT_INIT);
				cb->onIoCompleted(iotype, NetEndpoint::EE_CONNECT, ep, 0, 0, 0);
			}
			else
			{
				ep->setStatus(NetEndpoint::ESTAT_CONNECTED);
				int ec = setsockopt(ep->getSocket(), SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
				xpfAssert(("Failed to update connected context on outgoing socket.", ec == 0));
				cb->onIoCompleted(iotype, NetEndpoint::EE_SUCCESS, ep, 0, 0, 0);
			}
			break;

		case NetIoMux::EIT_RECV:
			cb->onIoCompleted(iotype, 
				(ret)?NetEndpoint::EE_SUCCESS:NetEndpoint::EE_RECV, ep, 0, odata->Buffer.buf, bytes);
			break;

		case NetIoMux::EIT_RECVFROM:
			cb->onIoCompleted(iotype, 
				(ret)?NetEndpoint::EE_SUCCESS:NetEndpoint::EE_RECV, ep, (vptr)&odata->PeerData, odata->Buffer.buf, bytes);
			break;

		case NetIoMux::EIT_SEND:
			cb->onIoCompleted(iotype,
				(ret) ? NetEndpoint::EE_SUCCESS : NetEndpoint::EE_SEND, ep, 0, odata->Buffer.buf, bytes);
			break;

		case NetIoMux::EIT_SENDTO:
			cb->onIoCompleted(iotype,
				(ret) ? NetEndpoint::EE_SUCCESS : NetEndpoint::EE_SEND, ep, (vptr)&odata->PeerData, odata->Buffer.buf, bytes);
			break;

		default:
			xpfAssert(("Unrecognized EIoType.", false));
			break;
		}

		// TODO: recycle / reuse NetIoMuxOverlapped.
		delete odata;

		return NetIoMux::ERS_NORMAL;
	}

	void asyncRecv(NetEndpoint *ep, c8 *buf, u32 buflen, NetIoMuxCallback *cb)
	{
		const NetIoMux::EIoType iotype = NetIoMux::EIT_RECV;

		const NetEndpoint::EStatus stat = ep->getStatus();
		xpfAssert(("Invalid socket status.", NetEndpoint::ESTAT_CONNECTED == stat));
		if (NetEndpoint::ESTAT_CONNECTED != stat)
		{
			cb->onIoCompleted(iotype, NetEndpoint::EE_INVALID_OP, ep, 0, buf, 0);
			return;
		}

		NetIoMuxOverlapped *odata = new NetIoMuxOverlapped;
		::memset(odata, 0, sizeof(NetIoMuxOverlapped));
		odata->Buffer.buf = buf;
		odata->Buffer.len = buflen;
		odata->IoType = iotype;
		odata->Callback = cb;

		DWORD flags = 0;
		int ec = WSARecv(ep->getSocket(), &odata->Buffer, 1, 0, &flags, (LPWSAOVERLAPPED)odata, 0);
		if ((ec != 0) && (ERROR_IO_PENDING != WSAGetLastError()))
		{
			cb->onIoCompleted(iotype, NetEndpoint::EE_RECV, ep, 0, buf, 0);
		}
	}

	void asyncRecvFrom(NetEndpoint *ep, c8 *buf, u32 buflen, NetIoMuxCallback *cb)
	{
		const NetIoMux::EIoType iotype = NetIoMux::EIT_RECVFROM;

		const NetEndpoint::EStatus stat = ep->getStatus();
		xpfAssert(("Invalid socket status.", NetEndpoint::ESTAT_CONNECTED == stat));
		if (NetEndpoint::ESTAT_CONNECTED != stat)
		{
			cb->onIoCompleted(iotype, NetEndpoint::EE_INVALID_OP, ep, 0, buf, 0);
			return;
		}

		NetIoMuxOverlapped *odata = new NetIoMuxOverlapped;
		::memset(odata, 0, sizeof(NetIoMuxOverlapped));
		odata->Buffer.buf = buf;
		odata->Buffer.len = buflen;
		odata->IoType = iotype;
		odata->Callback = cb;
		odata->PeerData.Length = XPF_NETENDPOINT_MAXADDRLEN;

		DWORD flags = 0;
		int ec = WSARecvFrom(ep->getSocket(), &odata->Buffer, 1, 0, &flags, (sockaddr*)odata->PeerData.Data,
			&odata->PeerData.Length, (LPWSAOVERLAPPED)odata, 0);
		if ((ec != 0) && (ERROR_IO_PENDING != WSAGetLastError()))
		{
			cb->onIoCompleted(iotype, NetEndpoint::EE_RECV, ep, 0, buf, 0);
		}
	}

	void asyncSend(NetEndpoint *ep, const c8 *buf, u32 buflen, NetIoMuxCallback *cb)
	{
		const NetIoMux::EIoType iotype = NetIoMux::EIT_SEND;

		const NetEndpoint::EStatus stat = ep->getStatus();
		xpfAssert(("Invalid socket status.", NetEndpoint::ESTAT_CONNECTED == stat));
		if (NetEndpoint::ESTAT_CONNECTED != stat)
		{
			cb->onIoCompleted(iotype, NetEndpoint::EE_INVALID_OP, ep, 0, buf, 0);
			return;
		}

		NetIoMuxOverlapped *odata = new NetIoMuxOverlapped;
		::memset(odata, 0, sizeof(NetIoMuxOverlapped));
		odata->Buffer.buf = (c8*)buf;
		odata->Buffer.len = buflen;
		odata->IoType = iotype;
		odata->Callback = cb;

		int ec = WSASend(ep->getSocket(), &odata->Buffer, 1, 0, 0, (LPWSAOVERLAPPED)odata, 0);
		if ((ec != 0) && (ERROR_IO_PENDING != WSAGetLastError()))
		{
			cb->onIoCompleted(iotype, NetEndpoint::EE_SEND, ep, 0, buf, 0);
		}
	}

	void asyncSendTo(NetEndpoint *ep, const NetEndpoint::Peer *peer, const c8 *buf, u32 buflen, NetIoMuxCallback *cb)
	{
		const NetIoMux::EIoType iotype = NetIoMux::EIT_SENDTO;

		const NetEndpoint::EStatus stat = ep->getStatus();
		xpfAssert(("Invalid socket status.", NetEndpoint::ESTAT_CONNECTED == stat));
		if (NetEndpoint::ESTAT_CONNECTED != stat)
		{
			cb->onIoCompleted(iotype, NetEndpoint::EE_INVALID_OP, ep, (vptr)peer, buf, 0);
			return;
		}

		NetIoMuxOverlapped *odata = new NetIoMuxOverlapped;
		::memset(odata, 0, sizeof(NetIoMuxOverlapped));
		odata->Buffer.buf = (c8*)buf;
		odata->Buffer.len = buflen;
		odata->IoType = iotype;
		odata->Callback = cb;
		odata->PeerData.Length = peer->Length;
		::memcpy(odata->PeerData.Data, peer->Data, peer->Length);

		const u32 proto = ep->getProtocol();
		int ec = WSASendTo(ep->getSocket(), &odata->Buffer, 1, 0, 0, 
			(const sockaddr*)odata->PeerData.Data, odata->PeerData.Length, (LPWSAOVERLAPPED)odata, 0);
		if ((ec != 0) && (ERROR_IO_PENDING != WSAGetLastError()))
		{
			cb->onIoCompleted(iotype, NetEndpoint::EE_SEND, ep, (vptr)peer, buf, 0);
		}
	}

	void asyncAccept(NetEndpoint *ep, NetIoMuxCallback *cb)
	{
		const NetIoMux::EIoType iotype = NetIoMux::EIT_ACCEPT;

		IocpAsyncContext *ctx = (IocpAsyncContext*)ep->getAsyncContext();
		xpfAssert( ("Unprovisioned netendpoint.", ctx != 0) );

		const NetEndpoint::EStatus stat = ep->getStatus();
		xpfAssert(("Invalid socket status.", NetEndpoint::ESTAT_LISTENING == stat));
		if (NetEndpoint::ESTAT_LISTENING != stat)
		{
			cb->onIoCompleted(iotype, NetEndpoint::EE_INVALID_OP, ep, 0, 0, 0);
			return;
		}

		if (xpfUnlikely(ctx->pAcceptEx == 0))
		{
			GUID GuidAcceptEx = WSAID_ACCEPTEX;
			DWORD dwBytes = 0;
			int ec = WSAIoctl(ep->getSocket(), SIO_GET_EXTENSION_FUNCTION_POINTER,
				&GuidAcceptEx, sizeof (GuidAcceptEx),
				&ctx->pAcceptEx, sizeof (ctx->pAcceptEx),
				&dwBytes, NULL, NULL);
			xpfAssert(("Unable to retrieve AcceptEx func ptr.", ec != SOCKET_ERROR));
			if (ec == SOCKET_ERROR)
			{
				cb->onIoCompleted(iotype, NetEndpoint::EE_ACCEPT, ep, 0, 0, 0);
				return;
			}
		}

		NetIoMuxOverlapped *odata = new NetIoMuxOverlapped;
		::memset(odata, 0, sizeof(NetIoMuxOverlapped));
		odata->IoType = iotype;
		odata->Callback = cb;

		u32 proto = ep->getProtocol();
		odata->AcceptingSocket = socket(
			(proto & NetEndpoint::ProtocolIPv4) ? AF_INET : AF_INET6,
			(proto & NetEndpoint::ProtocolTCP) ? SOCK_STREAM : SOCK_DGRAM,
			(proto & NetEndpoint::ProtocolTCP) ? IPPROTO_TCP : IPPROTO_UDP
			);
		xpfAssert(("Unable to create socket for accepting peer", odata->AcceptingSocket != INVALID_SOCKET));
		if (odata->AcceptingSocket == INVALID_SOCKET)
		{
			cb->onIoCompleted(iotype, NetEndpoint::EE_ACCEPT, ep, 0, 0, 0);
			return;
		}

		ep->setStatus(NetEndpoint::ESTAT_ACCEPTING);

		DWORD bytes = 0;
		const int addrlen = (proto & NetEndpoint::ProtocolIPv4) ? sizeof(sockaddr_in)+16 : sizeof(sockaddr_in6)+16;
		BOOL ret = ctx->pAcceptEx(ep->getSocket(), odata->AcceptingSocket, odata->PeerData.Data,
			0, addrlen, addrlen, &bytes, (LPWSAOVERLAPPED)odata);

		if ((ret == FALSE) && (ERROR_IO_PENDING != WSAGetLastError()))
		{
			ep->setStatus(NetEndpoint::ESTAT_LISTENING);
			cb->onIoCompleted(iotype, NetEndpoint::EE_ACCEPT, ep, 0, 0, 0);
		}
	}

	void asyncConnect(NetEndpoint *ep, const c8 *host, u32 port, NetIoMuxCallback *cb)
	{
		const NetIoMux::EIoType iotype = NetIoMux::EIT_CONNECT;

		IocpAsyncContext *ctx = (IocpAsyncContext*)ep->getAsyncContext();
		xpfAssert(("Unprovisioned netendpoint.", ctx != 0));

		const NetEndpoint::EStatus stat = ep->getStatus();
		xpfAssert(("Invalid socket status.", NetEndpoint::ESTAT_INIT == stat));
		if (NetEndpoint::ESTAT_INIT != stat)
		{
			cb->onIoCompleted(iotype, NetEndpoint::EE_INVALID_OP, ep, 0, 0, 0);
			return;
		}

		// TODO: Addr resolving is done in sync (blocking). 
		//       Should we just take NetEndpoint::Peer as input and require
		//       the outsider to do the addr-resolving?
		NetEndpoint::Peer peer;
		if (!NetEndpoint::resolvePeer(ep->getProtocol(), peer, host, 0, port))
		{
			cb->onIoCompleted(iotype, NetEndpoint::EE_RESOLVE, ep, 0, 0, 0);
			return;
		}

		if (xpfUnlikely(ctx->pConnectEx == 0))
		{
			GUID GuidConnectEx = WSAID_CONNECTEX;
			DWORD dwBytes = 0;
			int ec = WSAIoctl(ep->getSocket(), SIO_GET_EXTENSION_FUNCTION_POINTER,
				&GuidConnectEx, sizeof (GuidConnectEx),
				&ctx->pConnectEx, sizeof (ctx->pConnectEx),
				&dwBytes, NULL, NULL);
			xpfAssert(("Unable to retrieve ConnectEx func ptr.", ec != SOCKET_ERROR));
			if (ec == SOCKET_ERROR)
			{
				cb->onIoCompleted(iotype, NetEndpoint::EE_CONNECT, ep, 0, 0, 0);
				return;
			}
		}

		// Caveat: The socket provided to ConnectEx() must be bound.
		int ec = 0;
		if (ep->getProtocol() & NetEndpoint::ProtocolIPv4)
		{
			sockaddr_in sa = {0};
			sa.sin_family = AF_INET;
			sa.sin_addr.s_addr = INADDR_ANY;
			ec = ::bind(ep->getSocket(), (const sockaddr*)&sa, sizeof(sockaddr_in));
		}
		else
		{
			sockaddr_in6 sa = {0};
			sa.sin6_family = AF_INET6;
			sa.sin6_addr = in6addr_any;
			ec = ::bind(ep->getSocket(), (const sockaddr*)&sa, sizeof(sockaddr_in6));
		}
		if (0 != ec)
		{
			cb->onIoCompleted(iotype, NetEndpoint::EE_BIND, ep, 0, 0, 0);
			return;
		}

		NetIoMuxOverlapped *odata = new NetIoMuxOverlapped;
		::memset(odata, 0, sizeof(NetIoMuxOverlapped));
		odata->IoType = iotype;
		odata->Callback = cb;

		ep->setStatus(NetEndpoint::ESTAT_CONNECTING);

		// After socket has been bound, we can finally calls ConnectEx.
		BOOL ret = ctx->pConnectEx(ep->getSocket(), (const sockaddr*)peer.Data, peer.Length,
			0, 0, 0, (LPWSAOVERLAPPED)odata);

		if ((FALSE == ret) && (ERROR_IO_PENDING != WSAGetLastError()))
		{
			ep->setStatus(NetEndpoint::ESTAT_INIT);
			cb->onIoCompleted(iotype, NetEndpoint::EE_CONNECT, ep, 0, 0, 0);
		}
	}

	bool join(NetEndpoint *ep)
	{
		bool joined = false;

		if (ep->getAsyncContext() == 0)
		{
			HANDLE h = CreateIoCompletionPort((HANDLE)ep->getSocket(), mhIocp, (ULONG_PTR)ep, 0);
			xpfAssert(("Unable to join a netendpoint to mux.", h != NULL));
			if (h == NULL)
				return false;

			ep->setAsyncContext((vptr) new IocpAsyncContext);
			joined = true;
		}
		
		return joined;
	}

	bool depart(NetEndpoint *ep)
	{
		IocpAsyncContext *ctx = (IocpAsyncContext*)((ep) ? ep->getAsyncContext() : 0);
		if (ctx)
		{
			delete ctx;
			ep->close();
			return true;
		}
		return false;
	}

	static const char * getMultiplexerType(NetIoMux::EPlatformMultiplexer &epm)
	{
		epm = NetIoMux::EPM_IOCP;
		return "iocp";
	}

private:
	HANDLE			mhIocp;
	bool            bEnable;
}; // end of class NetIoMuxImpl (IOCP)

} // end of namespace xpf
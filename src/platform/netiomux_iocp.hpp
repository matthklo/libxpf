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

struct IocpAsyncContext
{
	IocpAsyncContext()
		: pAcceptEx(0), pConnectEx(0)
	{
		resetReadOverlapped();
		resetWriteOverlapped();
		AcceptingPeerSocket = INVALID_SOCKET;

		RecvBuf.buf = 0;
		RecvBuf.len = 0;
		SendBuf.buf = 0;
		SendBuf.len = 0;
	}

	void resetReadOverlapped() { ::memset(&ReadOverlapped, 0, sizeof(OVERLAPPED)); }
	void resetWriteOverlapped() { ::memset(&WriteOverlapped, 0, sizeof(OVERLAPPED)); }

	OVERLAPPED ReadOverlapped;
	OVERLAPPED WriteOverlapped;
	LPFN_ACCEPTEX  pAcceptEx;
	LPFN_CONNECTEX pConnectEx;
	WSABUF    RecvBuf;
	WSABUF	  SendBuf;

	NetIoMux::RecvCallback RecvCb;
	NetIoMux::RecvFromCallback RecvFromCb;
	NetIoMux::SendCallback SendCb;
	NetIoMux::SendToCallback SendToCb;
	NetIoMux::AcceptCallback AcceptCb;
	NetIoMux::ConnectCallback ConnectCb;

	NetEndpoint::Peer PeerData;
	SOCKET AcceptingPeerSocket;
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

	}

	NetIoMux::RunningStaus runOnce(u32 timeoutMs)
	{
		return NetIoMux::RS_NORMAL;
	}

	void asyncRecv(NetEndpoint *ep, c8 *buf, u32 buflen, NetIoMux::RecvCallback cb)
	{
		IocpAsyncContext *ctx = (IocpAsyncContext*)ep->getAsyncContext();
		xpfAssert(("Unprovisioned netendpoint.", ctx != 0));

		const NetEndpoint::EStatus stat = ep->getStatus();
		xpfAssert(("Invalid socket status.", NetEndpoint::ESTAT_CONNECTED == stat));
		if (NetEndpoint::ESTAT_CONNECTED != stat)
		{
			cb(NetEndpoint::EE_INVALID_OP, ep, buf, 0);
			return;
		}

		ctx->RecvCb = cb;
		ctx->RecvBuf.buf = buf;
		ctx->RecvBuf.len = buflen;

		int ec = WSARecv(ep->getSocket(), &ctx->RecvBuf, 1, 0, 0, &ctx->ReadOverlapped, 0);
		if ((ec != 0) && (ERROR_IO_PENDING != WSAGetLastError()))
		{
			ctx->RecvCb.Clear();
			cb(NetEndpoint::EE_RECV, ep, buf, 0);
		}
	}

	void asyncRecvFrom(NetEndpoint *ep, NetEndpoint::Peer *peer, c8 *buf, u32 buflen, NetIoMux::RecvFromCallback cb)
	{
		IocpAsyncContext *ctx = (IocpAsyncContext*)ep->getAsyncContext();
		xpfAssert(("Unprovisioned netendpoint.", ctx != 0));

		const NetEndpoint::EStatus stat = ep->getStatus();
		xpfAssert(("Invalid socket status.", NetEndpoint::ESTAT_CONNECTED == stat));
		if (NetEndpoint::ESTAT_CONNECTED != stat)
		{
			cb(NetEndpoint::EE_INVALID_OP, ep, peer, buf, 0);
			return;
		}

		ctx->RecvFromCb = cb;
		ctx->RecvBuf.buf = buf;
		ctx->RecvBuf.len = buflen;
		peer->Length = XPF_NETENDPOINT_MAXADDRLEN;

		int ec = WSARecvFrom(ep->getSocket(), &ctx->RecvBuf, 1, 0, 0, (sockaddr*)peer->Data,
			&peer->Length, &ctx->ReadOverlapped, 0);
		if ((ec != 0) && (ERROR_IO_PENDING != WSAGetLastError()))
		{
			ctx->RecvFromCb.Clear();
			cb(NetEndpoint::EE_RECV, ep, peer, buf, 0);
		}
	}

	void asyncSend(NetEndpoint *ep, const c8 *buf, u32 buflen, NetIoMux::SendCallback cb)
	{
		IocpAsyncContext *ctx = (IocpAsyncContext*)ep->getAsyncContext();
		xpfAssert(("Unprovisioned netendpoint.", ctx != 0));

		const NetEndpoint::EStatus stat = ep->getStatus();
		xpfAssert(("Invalid socket status.", NetEndpoint::ESTAT_CONNECTED == stat));
		if (NetEndpoint::ESTAT_CONNECTED != stat)
		{
			cb(NetEndpoint::EE_INVALID_OP, ep, buf, 0);
			return;
		}

		ctx->SendCb = cb;
		ctx->SendBuf.buf = (c8*)buf;
		ctx->SendBuf.len = buflen;

		int ec = WSASend(ep->getSocket(), &ctx->SendBuf, 1, 0, 0, &ctx->WriteOverlapped, 0);
		if ((ec != 0) && (ERROR_IO_PENDING != WSAGetLastError()))
		{
			ctx->SendCb.Clear();
			cb(NetEndpoint::EE_SEND, ep, buf, 0);
		}
	}

	void asyncSendTo(NetEndpoint *ep, const NetEndpoint::Peer *peer, const c8 *buf, u32 buflen, NetIoMux::SendToCallback cb)
	{
		IocpAsyncContext *ctx = (IocpAsyncContext*)ep->getAsyncContext();
		xpfAssert(("Unprovisioned netendpoint.", ctx != 0));

		const NetEndpoint::EStatus stat = ep->getStatus();
		xpfAssert(("Invalid socket status.", NetEndpoint::ESTAT_CONNECTED == stat));
		if (NetEndpoint::ESTAT_CONNECTED != stat)
		{
			cb(NetEndpoint::EE_INVALID_OP, ep, peer, buf, 0);
			return;
		}

		ctx->SendToCb = cb;
		ctx->SendBuf.buf = (c8*)buf;
		ctx->SendBuf.len = buflen;

		const u32 proto = ep->getProtocol();
		const s32 addrlen = (proto & NetEndpoint::ProtocolIPv4) ? sizeof(sockaddr_in) : sizeof(sockaddr_in6);
		int ec = WSASendTo(ep->getSocket(), &ctx->SendBuf, 1, 0, 0, 
			(const sockaddr*)peer->Data, addrlen, &ctx->WriteOverlapped, 0);
		if ((ec != 0) && (ERROR_IO_PENDING != WSAGetLastError()))
		{
			ctx->SendToCb.Clear();
			cb(NetEndpoint::EE_SEND, ep, peer, buf, 0);
		}
	}

	void asyncAccept(NetEndpoint *ep, NetIoMux::AcceptCallback cb)
	{
		IocpAsyncContext *ctx = (IocpAsyncContext*)ep->getAsyncContext();
		xpfAssert( ("Unprovisioned netendpoint.", ctx != 0) );

		const NetEndpoint::EStatus stat = ep->getStatus();
		xpfAssert(("Invalid socket status.", NetEndpoint::ESTAT_LISTENING == stat));
		if (NetEndpoint::ESTAT_LISTENING != stat)
		{
			cb(NetEndpoint::EE_INVALID_OP, ep, 0);
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
				cb(NetEndpoint::EE_ACCEPT, ep, 0);
				return;
			}
		}

		u32 proto = ep->getProtocol();
		ctx->AcceptingPeerSocket = socket(
			(proto & NetEndpoint::ProtocolIPv4) ? AF_INET : AF_INET6,
			(proto & NetEndpoint::ProtocolTCP) ? SOCK_STREAM : SOCK_DGRAM,
			(proto & NetEndpoint::ProtocolTCP) ? IPPROTO_TCP : IPPROTO_UDP
			);
		xpfAssert(("Unable to create socket for accepting peer", ctx->AcceptingPeerSocket != INVALID_SOCKET));
		if (ctx->AcceptingPeerSocket == INVALID_SOCKET)
		{
			cb(NetEndpoint::EE_ACCEPT, ep, 0);
			return;
		}

		ep->setStatus(NetEndpoint::ESTAT_ACCEPTING);
		ctx->AcceptCb = cb;

		DWORD bytes = 0;
		const int addrlen = (proto & NetEndpoint::ProtocolIPv4) ? sizeof(sockaddr_in)+16 : sizeof(sockaddr_in6)+16;
		BOOL ret = ctx->pAcceptEx(ep->getSocket(), ctx->AcceptingPeerSocket, ctx->PeerData.Data,
			0, addrlen, addrlen, &bytes, &ctx->ReadOverlapped);

		if (TRUE == ret)
		{
			ep->setStatus(NetEndpoint::ESTAT_LISTENING);
			ctx->AcceptCb.Clear();

			int listeningSocket = ep->getSocket();
			int ec = setsockopt(ctx->AcceptingPeerSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
				(char *)&listeningSocket, sizeof(listeningSocket));
			xpfAssert(("Failed to update accepted context on newly connected socket.", ec == 0));

			NetEndpoint *aep = new NetEndpoint(ep->getProtocol(), ctx->AcceptingPeerSocket, NetEndpoint::ESTAT_CONNECTED);
			join(aep);
			cb(NetEndpoint::EE_SUCCESS, ep, aep);
		}
		else if (ERROR_IO_PENDING != WSAGetLastError())
		{
			cb(NetEndpoint::EE_ACCEPT, ep, 0);
		}
	}

	void asyncConnect(NetEndpoint *ep, const c8 *host, u32 port, NetIoMux::ConnectCallback cb)
	{
		IocpAsyncContext *ctx = (IocpAsyncContext*)ep->getAsyncContext();
		xpfAssert(("Unprovisioned netendpoint.", ctx != 0));

		const NetEndpoint::EStatus stat = ep->getStatus();
		xpfAssert(("Invalid socket status.", NetEndpoint::ESTAT_INIT == stat));
		if (NetEndpoint::ESTAT_INIT != stat)
		{
			cb(NetEndpoint::EE_INVALID_OP, ep);
			return;
		}

		NetEndpoint::Peer peer;
		if (!NetEndpoint::resolvePeer(ep->getProtocol(), peer, host, 0, port))
		{
			cb(NetEndpoint::EE_RESOLVE, ep);
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
				cb(NetEndpoint::EE_CONNECT, ep);
				return;
			}
		}

		ep->setStatus(NetEndpoint::ESTAT_CONNECTING);
		ctx->ConnectCb = cb;

		BOOL ret = ctx->pConnectEx(ep->getSocket(), (const sockaddr*)peer.Data, peer.Length,
			0, 0, 0, &ctx->WriteOverlapped);

		if (TRUE == ret)
		{
			ep->setStatus(NetEndpoint::ESTAT_CONNECTED);
			ctx->ConnectCb.Clear();

			int ec = setsockopt(ep->getSocket(), SOL_SOCKET, SO_UPDATE_CONNECT_CONTEXT, NULL, 0);
			xpfAssert(("Failed to update connected context on outgoing socket.", ec == 0));
			cb(NetEndpoint::EE_SUCCESS, ep);
		}
		else if (ERROR_IO_PENDING != WSAGetLastError())
		{
			cb(NetEndpoint::EE_CONNECT, ep);
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
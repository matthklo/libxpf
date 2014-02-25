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

#include "async_client.h"
#include "async_server.h"

#ifdef XPF_PLATFORM_WINDOWS
// http://msdn.microsoft.com/en-us/library/vstudio/x98tx3cf.aspx
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <stdio.h>

using namespace xpf;

TestAsyncClient::TestAsyncClient(u32 threadNum)
{
	mMux = new NetIoMux();
	xpfAssert(threadNum > 0);
	for (u32 i = 0; i < threadNum; ++i)
	{
		mThreads.push_back(new WorkerThread(mMux));
	}

	for (u32 i = 0; i < 16; ++i)
	{
		Client *c = new Client;
		c->Checksum = 0;
		c->Count = 0;
		NetEndpoint *ep = NetEndpoint::create(NetEndpoint::ProtocolIPv4 | NetEndpoint::ProtocolTCP);
		mMux->join(ep);
		ep->setUserData((vptr)c);
		mClients[i] = ep;
	}
}

TestAsyncClient::~TestAsyncClient()
{
	stop();

	for (u32 i = 0; i < 16; ++i)
	{
		Client *c = (Client*)mClients[i]->getUserData();
		delete mClients[i];
		delete c;
		mClients[i] = 0;
	}

	delete mMux;
	mMux = 0;
}

void TestAsyncClient::start()
{
	printf("[Client] Starting worker threads ...\n");
	for (u32 i = 0; i < mThreads.size(); ++i)
		mThreads[i]->start();

	for (u32 i = 0; i < 16; ++i)
	{
		mMux->asyncConnect(mClients[i], "localhost", 50123, this);
	}
}

void TestAsyncClient::stop()
{
	if (mThreads.empty())
		return;

	while (true)
	{
		u32 i = 0;
		u32 errcnt = 0;
		u32 donecnt = 0;
		for (; i < 16; ++i)
		{
			Client *c = (Client*)mClients[i]->getUserData();
			if (c->Count >= 9999)
				errcnt++;
			else if (c->Count <= 1000)
				donecnt += c->Count;
		}
		if (donecnt == 16 * 1000)
		{
			printf("[Client] All jobs done.\n");
			for (u32 j=0; j<16; ++j)
				printf("[Client] job count: [%u] %u.\n", j, ((Client*)mClients[j]->getUserData())->Count);
			break;
		}
		else
		{
			printf("[Client] waiting (%u / 16000)...\n", donecnt);
			Thread::sleep(1000);
		}
	}

	mMux->disable();
	printf("[Client] Joining async client worker threads ...\n");
	while (!mThreads.empty())
	{
		for (u32 i = 0; i < mThreads.size(); ++i)
		{
			bool joined = mThreads[i]->join();
			if (joined)
			{
				delete mThreads[i];
				mThreads[i] = mThreads.back();
				mThreads.pop_back();
			}
		}
	}
	printf("[Client] All worker threads of async client have been joined.\n");
}

void TestAsyncClient::onIoCompleted(
	NetIoMux::EIoType type, 
	NetEndpoint::EError ec, 
	NetEndpoint *sep, 
	vptr tepOrPeer, 
	const c8 *buf, 
	u32 len)
{
	switch (type)
	{
	case NetIoMux::EIT_CONNECT:
		ConnectCb(ec, sep);
		break;
	case NetIoMux::EIT_RECV:
		RecvCb(ec, sep, buf, len);
		break;
	case NetIoMux::EIT_SEND:
		SendCb(ec, sep, buf, len);
		break;
	default:
		xpfAssert(("Unexpected NetIoMux::EIoType.", false));
		break;
	}
}

void TestAsyncClient::RecvCb(u32 ec, NetEndpoint* ep, const c8* buf, u32 bytes)
{
	Client *c = (Client*)ep->getUserData();
	if (ec == NetEndpoint::EE_SUCCESS)
	{
		if (bytes == 0)
		{
			printf("[Client] end of data.\n");
			ep->close();
			c->Count = 9999;
			return;
		}

		xpfAssert(bytes == 2);

		xpfAssert((*(u16*)c->RData) == c->Checksum);
		c->Count++;
		if (c->Count >= 1000)
		{
			printf("[Client] Client job completed.\n");
			ep->close();
		}
		else
		{
			sendTestData(ep);
		}
	}
	else
	{
		printf("[Client] Failed on recving.\n");
		ep->close();
		c->Count = 9999;
	}
}

void TestAsyncClient::SendCb(u32 ec, NetEndpoint* ep, const c8* buf, u32 bytes)
{
	Client *c = (Client*)ep->getUserData();
	if (ec == NetEndpoint::EE_SUCCESS)
	{
		if (bytes == 0)
		{
			printf("[Client] truncated sending.\n");
			ep->close();
			c->Count = 9999;
			return;
		}

		mMux->asyncRecv(ep, c->RData, 2, this);
	}
	else
	{
		printf("[Client] Failed on sending.\n");
		ep->close();
		c->Count = 9999;
	}
}

void TestAsyncClient::ConnectCb(u32 ec, NetEndpoint* ep)
{
	if (ec == NetEndpoint::EE_SUCCESS)
	{
		sendTestData(ep);
	}
	else
	{
		printf("[Client] Failed on connecting.\n");
		ep->close();
		Client *c = (Client*)ep->getUserData();
		c->Count = 9999;
	}
}

void TestAsyncClient::sendTestData(NetEndpoint* ep)
{
	Client *c = (Client*)ep->getUserData();

	u16 datalen = (rand() % 1022) + 1; // 1 ~ 1022
	u16 sum = 0;
	for (u16 i = 1; i <= datalen; ++i)
	{
		const u16 data = (u16)(rand() % 65536);
		*(u16*)(&c->WData[i * sizeof(u16)]) = data;
		sum += data;
	}
	*(u16*)(&c->WData[(datalen + 1) * sizeof(u16)]) = sum;
	*(u16*)(c->WData) = (datalen + 1) * sizeof(u16);

	const s32 tlen = (s32)((datalen + 2) * sizeof(u16));
	xpfAssert(tlen <= 2048);
	//s32 bytes = mEndpoint->send(mBuf, tlen, &ec);
	c->Checksum = sum;
	mMux->asyncSend(ep, c->WData, tlen, this);
}

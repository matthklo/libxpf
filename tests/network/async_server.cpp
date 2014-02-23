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

#include "async_server.h"

#ifdef XPF_PLATFORM_WINDOWS
// http://msdn.microsoft.com/en-us/library/vstudio/x98tx3cf.aspx
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <stdio.h>

using namespace xpf;

WorkerThread::WorkerThread(NetIoMux *mux)
	: mMux(mux)
{

}

WorkerThread::~WorkerThread()
{

}

u32 WorkerThread::run(u64 udata)
{
	mMux->run();
	return 0;
}

//=================------------------=====================//

TestAsyncServer::TestAsyncServer(u32 threadNum)
{
	mMux = new NetIoMux();
	xpfAssert(threadNum > 0);
	for (u32 i = 0; i < threadNum; ++i)
	{
		mThreads.push_back(new WorkerThread(mMux));
	}

	mListeningEp = NetEndpoint::create(NetEndpoint::ProtocolIPv4 | NetEndpoint::ProtocolTCP,
		"localhost", 50123);
	xpfAssert(mListeningEp != 0);
}

TestAsyncServer::~TestAsyncServer()
{
	mMux->disable();
	printf("Joining async server worker threads ...\n");
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
	printf("All worker threads of async server have been joined.\n");

	for (std::map<vptr, vptr>::iterator it = mBufferMap.begin();
		it != mBufferMap.end(); ++it)
	{
		Buffer *b = (Buffer*)(*it).second;
		delete b;
	}
	mBufferMap.clear();

	delete mMux;
	mMux = 0;
}

void TestAsyncServer::start()
{
	if (mListeningEp)
		mMux->asyncAccept(mListeningEp, XCALLBACK(TestAsyncServer::AcceptCb));
}

void TestAsyncServer::stop()
{

}

void TestAsyncServer::AcceptCb(u32 ec, NetEndpoint* listeningEp, NetEndpoint* acceptedEp)
{
	xpfAssert(listeningEp == mListeningEp);
	if (ec != (u32) NetEndpoint::EE_SUCCESS)
	{
		printf("Disable NetIoMux - accept failed. \n");
		mMux->disable();
	}
	else
	{
		Buffer *b = new Buffer;
		b->Used = 0;
		mBufferMap.insert(std::make_pair((vptr)acceptedEp, (vptr)b));

		printf("A new connection accepted.\n");
		mMux->asyncRecv(acceptedEp, b->RData, 2048, XCALLBACK(TestAsyncServer::RecvCb));
	}
}

void TestAsyncServer::RecvCb(u32 ec, NetEndpoint* ep, c8* buf, u32 bytes)
{
	if ((ec != (u32)NetEndpoint::EE_SUCCESS) || (bytes == 0))
	{
		printf("Recv error.");
	}
	else
	{
		std::map<vptr, vptr>::iterator it = mBufferMap.find((vptr)ep);
		xpfAssert(it != mBufferMap.end());
		Buffer *b = (Buffer*)(*it).second;

		const u16 tbytes = b->Used + bytes;
		u16 idx = 0;
		while (true)
		{
			u16 sum = 0;
			u16 packetlen = *(u16*)(&b->RData[idx]) + sizeof(u16);
			xpfAssert(packetlen <= 2048);

			if (idx + packetlen > tbytes)
			{
				printf("Truncated packet data.\n");
				b->Used = tbytes - idx;
				if (b->Used > 0)
					memmove(b->RData, &b->RData[idx], b->Used);
				break;
			}

			bool verified = false;
			const u16 cnt = *(u16*)(&b->RData[idx]) / sizeof(u16);
			for (u16 i = 1; i <= cnt; ++i)
			{
				if (i != cnt)
					sum += *(u16*)(&b->RData[idx + (i*sizeof(u16))]);
				else
					verified = (sum == *(u16*)(&b->RData[idx + (i*sizeof(u16))]));
			}
			xpfAssert(verified);
			*(u16*)b->WData = sum;
			mMux->asyncSend(ep, b->WData, sizeof(u16), XCALLBACK(TestAsyncServer::SendCb));

			idx += packetlen;
			if (idx >= tbytes)
			{
				b->Used = 0;
				break;
			}
		} // end of while (true)

		mMux->asyncRecv(ep, &b->RData[b->Used], (u32)(2048 - b->Used), XCALLBACK(TestAsyncServer::RecvCb));
	}
}

void TestAsyncServer::SendCb(xpf::u32 ec, xpf::NetEndpoint* ep, const xpf::c8* buf, xpf::u32 bytes)
{
	xpfAssert(ec == (u32) NetEndpoint::EE_SUCCESS);
}

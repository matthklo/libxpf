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

#ifndef _XPF_TEST_ASYNC_CLIENT_HDR_
#define _XPF_TEST_ASYNC_CLIENT_HDR_

#include <xpf/platform.h>
#include <xpf/netiomux.h>
#include <xpf/thread.h>

#include <vector>

class WorkerThread;

class TestAsyncClient
{
public:
	struct Client
	{
		xpf::u16 Checksum;
		xpf::c8  RData[2];
		xpf::c8  WData[2048];
		xpf::u16 Count;
	};

	explicit TestAsyncClient(xpf::u32 threadNum);
	virtual ~TestAsyncClient();

	void start();
	void stop();

	// async callbacks (** multi-thread accessing)
	void RecvCb(xpf::u32 ec, xpf::NetEndpoint* ep, xpf::c8* buf, xpf::u32 bytes);
	void SendCb(xpf::u32 ec, xpf::NetEndpoint* ep, const xpf::c8* buf, xpf::u32 bytes);
	void ConnectCb(xpf::u32 ec, xpf::NetEndpoint* ep);

private:
	void sendTestData(xpf::NetEndpoint* connectedEp);

	std::vector<WorkerThread*>   mThreads;
	xpf::NetEndpoint*            mClients[16];
	xpf::NetIoMux               *mMux;
};

#endif // _XPF_TEST_ASYNC_CLIENT_HDR_
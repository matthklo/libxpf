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

#include "sync_client.h"
#include <stdlib.h>
#include <stdio.h>

using namespace xpf;

TestSyncClient::TestSyncClient(u16 times, u32 latencyMs)
	: mEndpoint(0)
	, mPingPongTimes(times)
	, mPingPongLatencyMs(latencyMs)
{
	mBuf = new c8[2048];
}

TestSyncClient::~TestSyncClient()
{
	if (mEndpoint)
	{
		mEndpoint->close();
		join();
		NetEndpoint::freeEndpoint(mEndpoint);
		mEndpoint = 0;
	}

	delete[] mBuf;
}

u32 TestSyncClient::run(u64 udata)
{
	mEndpoint = NetEndpoint::createEndpoint(NetEndpoint::ProtocolTCP | NetEndpoint::ProtocolIPv4);
	xpfAssert(mEndpoint != 0);

	bool ret = false;
	u32 ec = 0;
	ret = mEndpoint->connect("localhost", 50123, &ec);
	xpfAssert(("TestSyncClient - Failed on connecting.",ret == true));

	if (!ret)
		return 1;

	for (u16 cnt = 0; cnt < mPingPongTimes; cnt++)
	{
		u16 datalen = (rand() % 1022) + 1; // 1 ~ 1022
		u16 sum = 0;
		for (u16 i = 1; i <= datalen; ++i)
		{
			const u16 data = (u16)(rand() % 65536);
			*(u16*)(&mBuf[i * sizeof(u16)]) = data;
			sum += data;
		}
		*(u16*)(&mBuf[(datalen + 1) * sizeof(u16)]) = sum;
		*(u16*)(mBuf) = (datalen + 1) * sizeof(u16);

		s32 tlen = (s32)((datalen + 2) * sizeof(u16));
		s32 bytes = mEndpoint->send(mBuf, tlen, &ec);
		xpfAssert(bytes == tlen);
		//printf("TestSyncClient send out %u bytes of data.\n", bytes);

		// waiting for response
		bytes = mEndpoint->recv(mBuf, 2, &ec);
		xpfAssert(bytes == 2);
		xpfAssert(*(u16*)mBuf == sum);

		if (mPingPongLatencyMs > 0)
			sleep(mPingPongLatencyMs);
	}

	return 0;
}


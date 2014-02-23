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

using namespace xpf;

TestSyncClient::TestSyncClient()
	: mEndpoint(0)
{
	mBuf = new u16[2048];
}

TestSyncClient::~TestSyncClient()
{
	if (mEndpoint)
	{
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

	for (u16 cnt = 0; cnt < 3000; cnt++)
	{
		u16 len = (rand() % 1023) + 1; // 1 ~ 1023
		u16 sum = 0;
		for (u16 i = 1; i < len; ++i)
		{
			mBuf[i] = rand() % 65536;
			sum += mBuf[i];
		}
		mBuf[len] = sum;
		mBuf[0] = len * sizeof(u16);

		s32 tlen = (s32)(mBuf[0] + sizeof(u16));
		s32 bytes = mEndpoint->send((const c8*)mBuf, tlen, &ec);
		xpfAssert(bytes == tlen);

		// waiting for response
		bytes = mEndpoint->recv((c8*)mBuf, 2048, &ec);
		xpfAssert(bytes == 2);
		xpfAssert(mBuf[0] == sum);
	}

	return 0;
}


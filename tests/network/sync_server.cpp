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

#include "sync_server.h"
#include <set>

using namespace xpf;

std::set<TestSyncServer*> _g_server_conns;

TestSyncServer::TestSyncServer()
	: mEndpoint(0)
	, mBuf(0), mOffset(0)
	, mAccepting(true)
{
	u32 ec = 0;
	mEndpoint = NetEndpoint::createEndpoint(NetEndpoint::ProtocolIPv4 | NetEndpoint::ProtocolTCP, "localhost", 50123, &ec);
	xpfAssert(("Failed on creating server endpoint.", mEndpoint != 0));
}

TestSyncServer::TestSyncServer(NetEndpoint *ep)
	: mEndpoint(ep), mOffset(0)
	, mAccepting(false)
{
	mBuf = new u16[2048];
}

TestSyncServer::~TestSyncServer()
{
	if (mEndpoint)
	{
		NetEndpoint::freeEndpoint(mEndpoint);
		mEndpoint = 0;
	}
	delete[] mBuf;
}

u32 TestSyncServer::run(u64 udata)
{
	u32 ec = 0;
	if (mAccepting)
	{
		while (true)
		{
			NetEndpoint *ep = mEndpoint->accept(&ec);
			if (!ep)
				break;
			TestSyncServer *s = new TestSyncServer(ep);
			_g_server_conns.insert(s);
			s->start();
		}
		return 0;
	}

	while (true)
	{
		//s32 bytes = mEndpoint->recv(((c8*)mBuf) + mOffset, (s32)(2048 - mOffset), &ec);
		//if (bytes == 0)
		//	break;

		//if (bytes > (s32)mBuf[0])
		//	mOffset = 
		//else
		//	mOffset = 0;

		u16 sum = 0;
		bool verified = false;
		for (u16 i = 0; i < mBuf[0]; ++i)
		{
			if (i != mBuf[0] - 1)
				sum += mBuf[i];
			else
				verified = (sum == mBuf[i]);
		}
		xpfAssert(verified);

	}
	return 0;
}
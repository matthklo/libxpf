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

#ifdef XPF_PLATFORM_WINDOWS
// http://msdn.microsoft.com/en-us/library/vstudio/x98tx3cf.aspx
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <set>
#include <stdio.h>
#include <string.h>

using namespace xpf;

std::set<TestSyncServer*> _g_server_conns;

TestSyncServer::TestSyncServer()
	: mEndpoint(0)
	, mBuf(0), mBufUsed(0)
	, mAccepting(true)
{
	u32 ec = 0;
	mEndpoint = NetEndpoint::create(NetEndpoint::ProtocolIPv4 | NetEndpoint::ProtocolTCP, "localhost", 50123, &ec);
	xpfAssert(("Failed on creating server endpoint.", mEndpoint != 0));
}

TestSyncServer::TestSyncServer(NetEndpoint *ep)
	: mEndpoint(ep), mBufUsed(0)
	, mAccepting(false)
{
	mBuf = new c8[2048];
}

TestSyncServer::~TestSyncServer()
{
	if (mAccepting)
	{
		printf("Free up all connected conns (%lu)...\n", _g_server_conns.size());
		for (std::set<TestSyncServer*>::iterator it = _g_server_conns.begin();
			it != _g_server_conns.end(); ++it)
		{
			delete (*it);
		}
		printf("All connected conns freed.\n");
		_g_server_conns.clear();
	}

	if (mEndpoint)
	{
		mEndpoint->close();
		join();
		NetEndpoint::release(mEndpoint);
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
				return 1;
			TestSyncServer *s = new TestSyncServer(ep);
			_g_server_conns.insert(s);
			s->start();
		}
		return 0;
	}

	while (true)
	{
		// fill buf
		const s32 available = (s32)(2048 - mBufUsed);
		s32 bytes = mEndpoint->recv(mBuf + mBufUsed, available, &ec);
		if (bytes == 0)
		{
			mBufUsed = 0;
			break;
		}
		const u16 tbytes = bytes + mBufUsed;

		// consume packet
		u16 idx = 0;
		while (true)
		{
			u16 sum = 0;
			u16 packetlen = *(u16*)(&mBuf[idx]) + sizeof(u16);
			xpfAssert(packetlen <= 2048);

			if (idx + packetlen > tbytes)
			{
				printf("Truncated packet data.\n");
				mBufUsed = tbytes - idx;
				if (mBufUsed > 0)
					memmove(mBuf, &mBuf[idx], mBufUsed);
				break;
			}

			bool verified = false;
			const u16 cnt = *(u16*)(&mBuf[idx]) / sizeof(u16);
			for (u16 i = 1; i <= cnt; ++i)
			{
				if (i != cnt)
					sum += *(u16*)(&mBuf[idx+(i*sizeof(u16))]);
				else
					verified = (sum == *(u16*)(&mBuf[idx + (i*sizeof(u16))]));
			}
			xpfAssert(verified);
			if (!verified)
			{
				return 1;
			}
			else
			{
				bytes = mEndpoint->send((const c8*)&sum, sizeof(u16), &ec);
				xpfAssert(bytes == sizeof(u16));
			}

			idx += packetlen;
			if (idx >= tbytes)
			{
				mBufUsed = 0;
				break;
			}
		}
	}
	return 0;
}

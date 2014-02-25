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

#include <xpf/platform.h>
#include <xpf/string.h>
#include "async_client.h"
#include "async_server.h"
#include "sync_client.h"
#include "sync_server.h"

#ifdef XPF_PLATFORM_WINDOWS
// http://msdn.microsoft.com/en-us/library/vstudio/x98tx3cf.aspx
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <vector>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

int test_sync()
{
	TestSyncServer *syncServ = new TestSyncServer;
	printf("Sync Server started .\n");
	syncServ->start();
	xpf::Thread::sleep(100);

	std::vector<TestSyncClient*> clients;
	for (xpf::u32 i = 0; i < 20; ++i)
	{
		TestSyncClient *syncClient = new TestSyncClient(1000, 0);
		clients.push_back(syncClient);
	}

	printf("Starting %u sync clients ...\n", clients.size());
	for (xpf::u32 i = 0; i < clients.size(); ++i)
	{
		clients[i]->start();
	}

	xpf::u32 errorcnt = 0;
	while (!clients.empty())
	{
		for (xpf::u32 i = 0; i < clients.size(); ++i)
		{
			if (clients[i]->join(10))
			{
				xpf::u32 exitcode = clients[i]->getExitCode();
				if (exitcode != 0)
					errorcnt++;
				delete clients[i];
				clients[i] = clients.back();
				clients.pop_back();
			}
		}

		xpf::u32 remains = clients.size();
		if ((remains != 0) && ((remains % 10) == 0))
		{
			printf("%u remaining sync clients ...\n", remains);
		}
	}
	printf("All sync Clients have stopped. %u clients end up with error.\n", errorcnt);

	printf("Sync Server stopping ...\n");
	delete syncServ;
	printf("Sync Server stopped .\n");
	return 0;
}

int test_async()
{
	TestAsyncServer *asyncServ = new TestAsyncServer(5);
	TestAsyncClient *asyncClient = new TestAsyncClient(5);
	asyncServ->start();

	xpf::Thread::sleep(100);
	asyncClient->start();
	asyncClient->stop();
	delete asyncClient;

	asyncServ->stop();
	delete asyncServ;
	return 0;
}

int main(int argc, char *argv[])
{
	srand((unsigned int)time(0));

	if ((argc >= 2) && (xpf::string(argv[1]) == "async"))
	{
		printf("==== Running async test ====\n");
		test_async();
	}
	else
	{
		printf("==== Running sync test ====\n");
		test_sync();
	}

	return 0;
}

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
#include "async_client.h"
#include "async_server.h"
#include "sync_client.h"
#include "sync_server.h"
#include <stdlib.h>
#include <time.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
	srand((unsigned int)time(0));

	TestSyncServer *syncServ = new TestSyncServer;
	printf("Sync Server started .\n");
	syncServ->start();
	xpf::Thread::sleep(500);

	TestSyncClient *syncClient = new TestSyncClient(100, 0);
	printf("Sync Client started.\n");
	syncClient->start();
	syncClient->join();
	printf("Sync Client stopped.\n");
	delete syncClient;

	printf("Sync Server stopping ...\n");
	delete syncServ;
	printf("Sync Server stopped .\n");
	return 0;
}

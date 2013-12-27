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

#include <xpf/thread.h>
#include <xpf/atomic.h>
#include <stdio.h>

#ifdef XPF_PLATFORM_WINDOWS
// http://msdn.microsoft.com/en-us/library/vstudio/x98tx3cf.aspx
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

using namespace xpf;

static int _g_hot = 0;

class MyThread : public Thread
{
public:
	MyThread(int count, int step)
		: mCount(count), mStep(step)
	{
	}

	u32 run(u64 userdata)
	{
		while (mCount--)
		{
			// not thread-safe:
			//_g_hot += mStep;

			// thread-safe:
			xpfAtomicAdd(&_g_hot, mStep);
		}
		return 0;
	}

private:
	int       mCount;
	const int mStep;
};

int main()
{
	MyThread *threadVector[8] = {
		new MyThread(10000, 1),
		new MyThread(3334, 1),
		new MyThread(10000, 2),
		new MyThread(3333, 2),
		new MyThread(5000, -2),
		new MyThread(10000, -1),
		new MyThread(10000, -1),
		new MyThread(10000, -1),
	};

	for (u32 i=0; i<8; i++)
		threadVector[i]->start();

	printf("Test started.\n");

	while(true)
	{
		Thread::sleep(1000);
		bool quit = true;
		for (u32 i=0; i<8; i++)
		{
			if (threadVector[i]->getStatus() != Thread::TRS_FINISHED)
			{
				quit = false;
				break;
			}
		}

		if (quit)
			break;
	}

	printf("Test finished. Joining all threads ...\n");
	for (u32 i=0; i<8; i++)
		threadVector[i]->join();

	printf("All threads joined ...\n");
	for (u32 i=0; i<8; i++)
		delete threadVector[i];

	xpfAssert(_g_hot == 0);

	printf("Test pass.\n");
	return 0;
}

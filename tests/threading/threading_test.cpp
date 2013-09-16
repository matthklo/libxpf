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

#ifdef XPF_PLATFORM_WINDOWS
// http://msdn.microsoft.com/en-us/library/vstudio/x98tx3cf.aspx
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include <time.h>

using namespace xpf;

class MyThread : public Thread
{
public:
	MyThread() : tid(Thread::INVALID_THREAD_ID) {}
	~MyThread() { printf("Thread [%llu] terminated.\n", tid); }

	u32 run(u64 userdata)
	{
		if (Thread::INVALID_THREAD_ID == tid)
		{
			tid = Thread::getThreadID();
		}

		u64 sec = userdata;
		while (sec--)
		{
			printf("Thread [%llu]: Remaining %llu seconds. \n", tid, sec);
			Thread::sleep(1000);
		}

		return (u32)tid;
	}

private:
	ThreadID tid;
};

int main()
{
#ifdef XPF_PLATFORM_WINDOWS
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	::srand(::time(NULL));

	std::vector<Thread*> ta;

	for (int i=0; i<30; ++i)
	{
		ta.push_back(new MyThread);
		ta.back()->setData((u64)(rand()%30 + 1));
	}

	for (unsigned int i=0; i<ta.size(); ++i)
	{
		xpfAssert(ta[i]->getStatus() == Thread::TRS_READY);
		ta[i]->start();
	}

	while(ta.size() > 0)
	{
		u32 i;
		bool joinedAny = false;
		for (i=0; i<ta.size(); ++i)
		{
			if (ta[i]->join(0))
			{
				xpfAssert(ta[i]->getStatus() == Thread::TRS_FINISHED);
				xpfAssert((u32)ta[i]->getID() == ta[i]->getExitCode());
				delete ta[i];
				ta[i] = ta[ta.size()-1];
				ta.pop_back();
				i--;
				joinedAny = true;
				break;
			}
		}

		if (!joinedAny)
		Thread::sleep(1000);
	}

	return 0;
}

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

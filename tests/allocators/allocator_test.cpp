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

#include <xpf/allocators.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <deque>

#ifdef XPF_PLATFORM_WINDOWS
#include <Windows.h>
#else
#include <sys/time.h>
#endif

using namespace xpf;

class StopWatch
{
public:
	StopWatch()
	{
#ifdef XPF_PLATFORM_WINDOWS
		QueryPerformanceFrequency(&Freq);
		QueryPerformanceCounter(&Counter);
#else
		gettimeofday(&Time, NULL);
#endif
	}

	u32 click() // return in ms
	{
#ifdef XPF_PLATFORM_WINDOWS
		LARGE_INTEGER c;
		QueryPerformanceCounter(&c);
		double ret = (double)(c.QuadPart - Counter.QuadPart) / (double)Freq.QuadPart;
		Counter = c;
		return (u32)(ret * 1000);
#else
		struct timeval t, s;
		gettimeofday(&t, NULL);
		timersub(&t, &Time, &s);
		Time = t;
		return (u32)((s.tv_sec * 1000) + (s.tv_usec / 1000));
#endif
	}

private:
#ifdef XPF_PLATFORM_WINDOWS
	LARGE_INTEGER Freq;
	LARGE_INTEGER Counter;
#else
	struct timeval Time;
#endif
};

struct MemObj
{
	MemObj(void * p, u32 s)
		: pattern(0), ptr(p), size(s) {}

	void provision(u32 pt)
	{
		pattern = pt;

		u32 cnt = size/sizeof(u32) - 2;
		u32 *ppp = (u32*)ptr;
		xpfAssert((*ppp != 0x52995299) || (*(ppp+1) != 0x99259925));
		*ppp = 0x52995299; ppp++;
		*ppp = 0x99259925; ppp++;
		while(cnt--)
		{
			*ppp = pattern;
			ppp++;
		}
	}

	bool validate()
	{
		u32 *ppp = (u32*)ptr;
		if (*ppp != 0x52995299)
			return false;
		*ppp = 0;
		ppp++;
		if (*ppp != 0x99259925)
			return false;
		*ppp = 0;
		ppp++;
		u32 cnt = size/sizeof(u32) - 2;
		while(cnt--)
		{
			if ((*ppp) != pattern)
				return false;
		}
		return true;
	}

	u32   size;
	u32   pattern;
	void* ptr;
};

struct Bank
{
	Bank(u32 s, u32 n)
		: size(s)
		, number(n)
	{}
	
	u32 size;
	u32 number;
	std::deque<MemObj> objs;
};

bool _g_log = true;
bool _g_memop = true;

//128mb
#define POOLSIZE (1 << 27)

bool test(bool sysalloc = false)
{
	BuddyAllocator *pool = (sysalloc) ? 0 : BuddyAllocator::instance();

	Bank profile[] =
	{
		Bank((1<<24), 2),    // 16mb * 2 = 32mb
		Bank((1<<23), 4),    //  8mb * 4 = 32mb
		Bank((1<<21), 16),   //  2mb *16 = 32mb
		Bank((1<<12), 2048), //  4kb *2048 = 8mb
		Bank((1<<7), (1<<16)),  // 128b * 2^16 = 8mb

		Bank(0,0),
	};

	int probs[] = { 2, 6, 22, 2070, 67606, 0 };
	
	int randomCnt = 0;
	int opCnt = 1000;
	bool op = true; // true: alloc, false: dealloc
	int totalCnt = 100000;
	int falseOp = 0;
	int falseAlloc = 0;

	u32 liveBytes = 0;
	u32 liveObjs = 0;

	while (totalCnt--)
	{
		bool alloc = false;
		if (opCnt > 0)
		{
			opCnt--;
			alloc = op;
		}
		else
		{
			alloc = ((rand() & 0x1) == 0x1);
			randomCnt++;
			if (randomCnt >= 1000)
			{
				if (_g_log) printf("Enqueue 500 allocs after 1000 random ops.\n");
				op = true;
				opCnt = 500;
				randomCnt = 0;
			}
		}

		int p = (rand() % 67606);
		int i = 0;
		for (; probs[i] !=0; ++i)
		{
			if (p<probs[i])
				break;
		}
		Bank& b = profile[i];

		if (alloc)
		{
			if (b.number == 0)
			{
				if (_g_log) printf("Alloc: MemObj of size %u run out of quota.\n", b.size);
				falseOp++;
			}
			else
			{
				void * ptr = (sysalloc)? malloc(b.size): pool->alloc(b.size);
				if (NULL == ptr)
				{
					if (_g_log) printf("Alloc: MemoryPool failed to allocate MemObj of size %u. (liveBytes = %u, liveObjs = %u).\n", b.size, liveBytes, liveObjs);
					op = false;
					opCnt = 2000;
					randomCnt = 0;
					falseAlloc++;
				}
				else
				{
					b.number--;
					b.objs.push_back(MemObj(ptr, b.size));
					u32 pt = (u32)rand();
					if (_g_memop)
						b.objs.back().provision(pt);
					liveObjs++;
					liveBytes += b.size;
				}
			}
		}
		else // dealloc
		{
			if (b.objs.empty())
			{
				if (_g_log) printf("Dealloc: No more live MemObj of size %u.\n", b.size);
				falseOp++;
			}
			else
			{
				MemObj &obj = b.objs.front();

				if (_g_memop)
				{
					if (!obj.validate())
						return false;
				}

				bool useDealloc = (rand()%2 > 0);
				if (sysalloc)
				{
					free(obj.ptr);
				}
				else
				{
					if (useDealloc)
						pool->dealloc(obj.ptr, obj.size);
					else
						pool->free(obj.ptr);
				}

				b.objs.pop_front();
				b.number++;

				liveObjs--;
				liveBytes -= b.size;
			}
		}

		if (_g_log && (totalCnt%1000 == 0))
		{
			printf("Test: liveBytes = %u, liveObjs = %u, falseOp = %u, falseAlloc = %u\n", liveBytes, liveObjs, falseOp, falseAlloc);
		}

	} // while (totalCnt--)

	printf("Test: liveBytes = %u, liveObjs = %u, falseOp = %u, falseAlloc = %u\n", liveBytes, liveObjs, falseOp, falseAlloc);

	// free all objs and make sure the top-most block can be allocated
	if (_g_log)
	{
		for(int i=0; ; ++i)
		{
			Bank& b = profile[i];
			if ((b.size == 0) && (b.number == 0))
				break;

			while(!b.objs.empty())
			{
				liveObjs--;
				liveBytes -= b.size;
				pool->free(b.objs.front().ptr);
				//pool->dealloc(b.objs.front().ptr, b.size);
				b.objs.pop_front();
			}
		}

		xpfAssert((liveObjs == 0) && (liveBytes == 0));
#ifdef XPF_ALLOCATORS_ENABLE_TRACE
		u32 usedBytes, highPeak;
		pool->trace(usedBytes, highPeak, true);
		xpfAssert( ("UsedBytes shall be 0", usedBytes == 0) );
		printf("HighPeakBytes = %u bytes\n", highPeak);
#endif

		void * p = pool->alloc(POOLSIZE);
		if (!p)
			return false;

		pool->free(p);
	}

	return true;
}

int main()
{
	unsigned int seed = (unsigned int)time(NULL);
	srand(seed);

	printf("\n==== SanityTest ====\n");

	u32 size = BuddyAllocator::create(POOLSIZE);
	xpfAssert(0 != size);


	bool ret = test();
	xpfAssert(ret);

	// std::allocator compatibility
	{
		std::deque<int, Allocator<int, AllocInstOf<BuddyAllocator> > > verifyingDeque;
		int i=0;
		for (; i<20000; i++)
		{
			verifyingDeque.push_back(i);
		}
		i=0;
		while (!verifyingDeque.empty())
		{
			xpfAssert( ( "std::allocator compatibility", i == verifyingDeque.front() ) );
			i++;
			verifyingDeque.pop_front();
		}
	}
	BuddyAllocator::destory();

	LinearAllocator::create(1024);
	LinearAllocator *stack = LinearAllocator::instance();
	xpfAssert((stack != 0) && (stack->capacity() == 1024) && (stack->available() == 1016));
	void *ptrs[5];
	for (u32 i=0; i<5; i++)
	{
		ptrs[i] = stack->alloc(32);
		xpfAssert(ptrs[i] != 0);
		for (u32 j=0; j<8; ++j)
		{
			*((int*)ptrs[i] + j) = 0xababbadc;
		}
	}
	xpfAssert((stack->used() == 200) && (stack->available() == 816));
	for (u32 i=0; i<5; i++)
	{
		for (u32 j=0; j<8; ++j)
		{
			xpfAssert(*((int*)ptrs[i] + j) == 0xababbadc);
		}
		stack->free(ptrs[i]);
		if (i == 4)
		{
			u32 u = stack->used();
			xpfAssert(stack->used() == 0);
		}
		else
		{
			xpfAssert(stack->used() == 200);
		}
	}
	for (u32 i=0; i<5; i++)
	{
		ptrs[i] = stack->alloc(32);
		xpfAssert(ptrs[i] != 0);
		for (u32 j=0; j<8; ++j)
		{
			*((int*)ptrs[i] + j) = 0xababbadc;
		}
	}
	xpfAssert((stack->used() == 200) && (stack->available() == 816));
	stack->free(ptrs[3]);
	xpfAssert(stack->used() == 200);
	stack->free(ptrs[4]);
	xpfAssert(stack->used() == 120);
	ptrs[3] = stack->alloc(192);
	xpfAssert(stack->used() == 320);
	ptrs[4] = stack->alloc(2000);
	xpfAssert(stack->used() == 320);
	xpfAssert(ptrs[4] == 0);
	stack->free(ptrs[1]);
	stack->free(ptrs[2]);
	stack->free(ptrs[3]);
	xpfAssert(stack->used() == 40);
	xpfAssert(stack->hwm() == 320);
	stack->reset();
	xpfAssert((stack->used() == 0) && (stack->hwm() == 0));
	LinearAllocator::destory();

	// done sanity test.

	_g_log = false;
	while (true)
	{
		printf("\n==== Benchmark (%s memop) ====\n", (_g_memop)? "with": "without");

		size = BuddyAllocator::create(POOLSIZE);
		xpfAssert(0 != size);
		srand(seed);

		StopWatch sw;
		test();
		u32 timeCost1 = sw.click();
		printf("Time cost of buddy = %u ms\n", timeCost1);
		BuddyAllocator::destory();

		srand(seed);
		sw.click();
		test(true);
		u32 timeCost2 = sw.click();
		printf("Time cost of sysalloc = %u ms\n", timeCost2);

		if (timeCost2 > timeCost1)
		{
			printf("Improved: %f%%\n", (double)(timeCost2 - timeCost1)*100.00/(double)timeCost2);
		}
		else
		{
			printf("Worse than sysalloc.\n");
		}

		if (_g_memop)
			_g_memop = false;
		else
			break;
	}

	
	return 0;
}

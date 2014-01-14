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
#endif

using namespace xpf;

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

bool _g_sanity = true;

bool test(bool sysalloc = false)
{
	MemoryPool *pool = (sysalloc)? 0: MemoryPool::instance();

	Bank profile[] =
	{
		Bank((1<<24), 2),    // 16mb * 2 = 32mb
		Bank((1<<23), 4),    //  8mb * 4 = 32mb
		Bank((1<<21), 16),   //  2mb *16 = 32mb
		Bank((1<<12), 2048), //  4kb *2048 = 8mb
		Bank((1<<7), (1<<16)),  // 128b * 2^16 = 8mb
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
				if (_g_sanity) printf("Enqueue 500 allocs after 1000 random ops.\n");
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
				if (_g_sanity) printf("Alloc: MemObj of size %u run out of quota.\n", b.size);
				falseOp++;
			}
			else
			{
				void * ptr = (sysalloc)? malloc(b.size): pool->alloc(b.size);
				if (NULL == ptr)
				{
					if (_g_sanity) printf("Alloc: MemoryPool failed to allocate MemObj of size %u. (liveBytes = %u, liveObjs = %u).\n", b.size, liveBytes, liveObjs);
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
					if (_g_sanity)
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
				if (_g_sanity) printf("Dealloc: No more live MemObj of size %u.\n", b.size);
				falseOp++;
			}
			else
			{
				MemObj &obj = b.objs.front();

				if (_g_sanity)
				{
					if (!obj.validate())
						return false;
				}

				bool useDealloc = rand()%2;
				if (sysalloc)
				{
					free(obj.ptr);
				}
				else
				{
					//if (useDealloc)
						pool->dealloc(obj.ptr, obj.size);
					//else
						//pool->free(obj.ptr);
				}

				b.objs.pop_front();
				b.number++;

				liveObjs--;
				liveBytes -= b.size;
			}
		}

		if (_g_sanity && (totalCnt%1000 == 0))
		{
			printf("Test: liveBytes = %u, liveObjs = %u, falseOp = %u, falseAlloc = %u\n", liveBytes, liveObjs, falseOp, falseAlloc);
		}

	} // while (totalCnt--)

	printf("Test: liveBytes = %u, liveObjs = %u, falseOp = %u, falseAlloc = %u\n", liveBytes, liveObjs, falseOp, falseAlloc);

	return true;
}

int main()
{
	unsigned int seed = (unsigned int)time(NULL);
	srand(seed);

	u32 poolSize = (1 << 27); //128mb
	u32 size = MemoryPool::create(poolSize);
	xpfAssert(0 != size);


	bool ret = test();
	xpfAssert(ret);
	MemoryPool::destory();

	printf("\n====Benchmark====\n");

	size = MemoryPool::create(poolSize);
	xpfAssert(0 != size);
	_g_sanity = false;
#ifdef XPF_PLATFORM_WINDOWS
	srand(seed);

	LARGE_INTEGER freq, cnt1, cnt2;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&cnt1);
	test();
	QueryPerformanceCounter(&cnt2);
	double timeCost1 = (double)(cnt2.QuadPart - cnt1.QuadPart) / (double)freq.QuadPart;
	printf("Time cost of buddy = %f secs\n", timeCost1);
	MemoryPool::destory();

	srand(seed);
	QueryPerformanceCounter(&cnt1);
	test(true);
	QueryPerformanceCounter(&cnt2);
	double timeCost2 = (double)(cnt2.QuadPart - cnt1.QuadPart) / (double)freq.QuadPart;
	printf("Time cost of sysalloc = %f secs\n", timeCost2);

	printf("Improved: %f%%\n", (timeCost2 - timeCost1)*100.00/timeCost2);
#endif

	
	return 0;
}

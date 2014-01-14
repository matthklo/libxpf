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

using namespace xpf;

struct MemObj
{
	MemObj(void * p)
		: pattern(0), ptr(p), size(0) {}

	void provision(u32 pt, u32 s)
	{
		pattern = pt;
		size = s;

		u32 cnt = size/sizeof(u32);
		u32 *ppp = (u32*)ptr;
		while(cnt--)
		{
			*ppp = pattern;
			ppp++;
		}
	}

	bool validate()
	{
		u32 cnt = size/sizeof(u32);
		u32 *ppp = (u32*)ptr;
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

bool sanity()
{
	MemoryPool *pool = MemoryPool::instance();

	Bank profile[] =
	{
		Bank((1<<24), 2),    // 16mb * 2 = 32mb
		Bank((1<<23), 4),    //  8mb * 4 = 32mb
		Bank((1<<21), 16),   //  2mb *16 = 32mb
		Bank((1<<12), 1024), //  4kb *1024 = 4mb
		Bank((1<<7), (1<<15)),  // 128b * 2^15 = 4mb
	};

	int probs[] = { 2, 6, 22, 1046, 33814, 0 };
	
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
				printf("Enqueue 500 allocs after 1000 random ops.\n");
				op = true;
				opCnt = 500;
				randomCnt = 0;
			}
		}

		int p = (rand() % 33814);
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
				printf("Alloc: MemObj of size %u run out of quota.\n", b.size);
				falseOp++;
			}
			else
			{
				void * ptr = pool->alloc(b.size);
				if (NULL == ptr)
				{
					printf("Alloc: MemoryPool failed to allocate MemObj of size %u. (liveBytes = %u, liveObjs = %u).\n", b.size, liveBytes, liveObjs);
					op = false;
					opCnt = 2000;
					randomCnt = 0;
					falseAlloc++;
				}
				else
				{
					b.number--;
					b.objs.push_back(MemObj(ptr));
					b.objs.back().provision((u32)rand(), b.size);
					liveObjs++;
					liveBytes += b.size;
				}
			}
		}
		else // dealloc
		{
			if (b.objs.empty())
			{
				printf("Dealloc: No more live MemObj of size %u.\n", b.size);
				falseOp++;
			}
			else
			{
				MemObj &obj = b.objs.front();
				if (!obj.validate())
					return false;

				//if (rand()%2)
					pool->dealloc(obj.ptr, obj.size);
				//else
					//pool->free(obj.ptr);

				b.objs.pop_front();
				b.number++;

				liveObjs--;
				liveBytes -= b.size;
			}
		}

		if (totalCnt%1000 == 0)
		{
			printf("Sanity: liveBytes = %u, liveObjs = %u, falseOp = %u, falseAlloc = %u\n", liveBytes, liveObjs, falseOp, falseAlloc);
		}

	} // while (totalCnt--)

	return true;
}

int main()
{
	srand((unsigned int)time(NULL));

	u32 poolSize = (1 << 27); //128mb
	u32 size = MemoryPool::create(poolSize);
	xpfAssert(0 != size);


	bool ret = sanity();
	xpfAssert(ret);

	MemoryPool::destory();
	return 0;
}

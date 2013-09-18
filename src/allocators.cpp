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
#include <memory>
#include <set>

#define DEBUG_MEMORY_POOL
#define SMALLEST_CELL_POW (4)

namespace xpf {


static u32 _NearsetTier ( const u32 v )
{
	u32 i = (0x1 << SMALLEST_CELL_POW);
	u32 t = 0;

	if ( xpfUnlikely (v > 0x80000000) )
		return -1L;

	while ( xpfLikely (i < v) )
	{
		i <<= 1;
		t++;
	}
	return t;
}

static u32 _GetBuddy ( const u32 v )
{
	if ( 1 == (v & 0x1) )
	{
		return v-1;
	}
	else
	{
		return v+1;
	}
}

struct MemoryPoolTier;

struct MemoryPoolDetails
{
	u32		         Capacity;
	char*	         Chunk;

	u32              MaxPowOf2;
	u32              TierNum;
	MemoryPoolTier  *Tiers;

	static MemoryPool* Instance;
};

struct MemoryPoolTier
{
	typedef std::set<u32> CellSet;

	MemoryPoolTier() { }
	~MemoryPoolTier() { }

	void provision ( u32 index, u32 powof2, MemoryPoolDetails *details )
	{
		Details = details;
		Index = index;
		PowOf2 = powof2;
		CellSize = ( 0x1 << PowOf2 );

		// If this is the toppest tier, the cell #0, the only cell in this tier,
		// should be marked as free.
		if ( xpfUnlikely (PowOf2 == Details->MaxPowOf2) )
		{
			FreeCells.insert(0);
		}
	}

	// Allocate a cell of this tier, return the starting address of the cell.
	// Return NULL is out of space.
	void* alloc()
	{
		u32 cell = obtain();
		if ( xpfLikely (-1L != cell) )
		{
			return (void*)(Details->Chunk + (cell * CellSize));
		}
		throw std::bad_alloc();
		return NULL;
	}

	void dealloc( void *p )
	{
		u32 cell = ((char*)p - Details->Chunk)/CellSize;
		recycle(cell);
	}

	u32 obtain ()
	{
		// If free cells of current tier runs out, request more from upper tier.
		if ( xpfUnlikely (FreeCells.empty()) )
		{
			// Can not do anything if current tier is the toppest tier.
			if ( xpfUnlikely ( PowOf2 >= Details->MaxPowOf2 ) )
			{
				return -1L;
			}
			
			u32 upperCell = Details->Tiers[Index+1].obtain();
			if ( xpfUnlikely ( -1L == upperCell ) )
			{
				// no more free cells from upper tier.
				return -1L;
			}

			u32 obtainedCell = (upperCell << 1);
			UsedCells.insert(obtainedCell);
			FreeCells.insert(obtainedCell + 1);
			return obtainedCell;
		}

		u32 cell = *(FreeCells.begin());
		FreeCells.erase(FreeCells.begin());
		UsedCells.insert(cell);
		return cell;
	}

	void recycle ( u32 cell )
	{
		u32 cnt = UsedCells.erase(cell);
		if ( xpfUnlikely( 0 == cnt ))
		{
			xpfAssert(false);
			return;
		}

		// For all tiers which is not the toppest, apply buddy check
		if ( xpfLikely (PowOf2 < Details->MaxPowOf2) )
		{
			u32 buddyCell = _GetBuddy(cell);
			cnt = FreeCells.erase(buddyCell);
			if ( xpfUnlikely (cnt > 0))
			{
				// merge them and give it back to upper tier.
				Details->Tiers[Index+1].recycle((cell >> 1));
				return;
			}
		}
		
		FreeCells.insert(cell);
	}

	u32     Index;
	u32     PowOf2;
	u32     CellSize; // cell size of this tier = 2^PowOf2. 
	CellSet FreeCells;
	CellSet UsedCells;

	MemoryPoolDetails *Details;
};

MemoryPool* MemoryPoolDetails::Instance = 0;

MemoryPool::MemoryPool(u32 poolSize)
	: mDetails(new MemoryPoolDetails)
{
	u32 s = 1024;
	u32 t = 10;
	while ( xpfLikely (s < poolSize) )
	{
		s <<= 1;
		t++;
		if ( xpfUnlikely (0x80000000 == s) )
			break;
	}
	mDetails->Capacity = s;

	std::allocator<char> alloc;
	mDetails->Chunk = alloc.allocate(mDetails->Capacity);

	// Prepare tiers
	mDetails->MaxPowOf2 = t;
	mDetails->TierNum = t - SMALLEST_CELL_POW + 1;
	mDetails->Tiers = new MemoryPoolTier[mDetails->TierNum];
	for (u32 pow=SMALLEST_CELL_POW; pow<=t; ++pow)
	{
		const u32 idx = pow - SMALLEST_CELL_POW;
		MemoryPoolTier &tier = mDetails->Tiers[idx];
		tier.provision(idx, pow, mDetails);
	}
}

MemoryPool::~MemoryPool()
{
	std::allocator<char> alloc;
	alloc.deallocate(mDetails->Chunk, mDetails->Capacity);

	delete[] mDetails->Tiers;

	delete mDetails;
	mDetails = (MemoryPoolDetails*) 0xfefefefe;
}

u32 MemoryPool::capacity() const
{
	return mDetails->Capacity;
}

void* MemoryPool::alloc ( u32 bytes )
{
	// determine which tier we're dealing with
	u32 tier = _NearsetTier( bytes );
	if ( xpfUnlikely (-1L == tier) )
	{
		throw std::bad_alloc();
		return NULL;
	}

	return mDetails->Tiers[tier].alloc();
}

void MemoryPool::dealloc ( void *p, u32 bytes )
{
	// determine which tier we're dealing with
	u32 tier = _NearsetTier( bytes );
	if ( xpfUnlikely (-1L == tier) )
		return;

	mDetails->Tiers[tier].dealloc(p);
}

u32 MemoryPool::create( u32 poolSize )
{
	if ( xpfLikely (MemoryPoolDetails::Instance) )
	{
		delete MemoryPoolDetails::Instance;
	}
	MemoryPoolDetails::Instance = new MemoryPool(poolSize);
	return MemoryPoolDetails::Instance->capacity();
}

MemoryPool* MemoryPool::instance()
{
	return MemoryPoolDetails::Instance;
}

}; // end of namespace xpf


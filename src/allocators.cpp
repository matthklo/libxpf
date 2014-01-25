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
#include <string.h>

#define MINSIZE_POWOF2 (4)
#define MAXSIZE_POWOF2 (31)
#define INVALID_VALUE  (0xFFFFFFFF)
#define MAXPOOL_SLOT (256)

namespace xpf {

static MemoryPool* _global_pool_instances[MAXPOOL_SLOT] = {0};

/****************************************************************************
 * A memory pool implementation based on Buddy memory allocation algorithm
 * http://en.wikipedia.org/wiki/Buddy_memory_allocation
 ****************************************************************************/

struct MemoryPoolDetails
{
private:
	struct FreeBlockRecord;

	// The element type used for the linked-list of free blocks.
	// Since we store this data structure inside each of free block, 
	// it is crucial to make sure that sizeof(FreeBlockRecord) is 
	// always smaller then the smallest-possible block size 
	// (1 << MINSIZE_POWOF2 bytes).
	struct FreeBlockRecord
	{
		FreeBlockRecord *Prev;
		FreeBlockRecord *Next;
	};

public:
	explicit MemoryPoolDetails( u32 size )
		: TierNum(1)
		, Capacity(1 << MINSIZE_POWOF2)
	{
		// This is a static assertion which makes sure the size 
		// of FreeBlockRecord is always less or equal to the smallest 
		// possible block size.
		// If compiler nag anything about this line, the assumption
		// probably doesn't hold. Which is fatal to our implementation.
		xpfSAssert((sizeof(FreeBlockRecord) <= (1<<MINSIZE_POWOF2)));

		const u32 maxSize = (1 << MAXSIZE_POWOF2);

		while ( xpfLikely (Capacity < size) )
		{
			Capacity <<= 1;
			++TierNum;
			if ( xpfUnlikely (maxSize == Capacity) )
				break;
		}
		FlagsLen = (1 << ((TierNum <= 3) ? 0 : (TierNum - 3))); // about 1/64 size of Capacity.

		Chunk         = (char*)             ::malloc(sizeof(char) * Capacity);
		Flags         = (char*)             ::malloc(sizeof(char) * FlagsLen);
		FreeChainHead = (FreeBlockRecord**) ::malloc(sizeof(FreeBlockRecord*) * TierNum);

		::memset(Flags, 0, sizeof(char) * FlagsLen);
		::memset(FreeChainHead, 0, sizeof(FreeBlockRecord*) * TierNum);

		// make sure the top-most tier places its only block in its free chain.
		setBlockInUse(0, 0, true);
		recycle(0, 0);
	}

	~MemoryPoolDetails()
	{
		::free(Chunk);
		::free(Flags);
		::free(FreeChainHead);
	}

	void* alloc ( const u32 size )
	{
		const u32 tier = tierOf(size);
		if ( xpfUnlikely(INVALID_VALUE == tier) )
			return NULL;

		const u32 blockId = obtain(tier);
		if ( xpfUnlikely (INVALID_VALUE == blockId) )
			return NULL;

		return (void*)(Chunk + (blockId * blockSizeOf(tier)));
	}

	// Return the allocated block with size hint.
	void dealloc ( void *p, const u32 size )
	{
		const u32 tier = tierOf(size);
		xpfAssert( ( "Expecting a valid tier index." , tier != INVALID_VALUE ) );
		if ( xpfUnlikely(INVALID_VALUE == tier) )
			return;

		const u32 blockId = blockIdOf(tier, p);
		xpfAssert( ( "Expecting a valid blockId." , blockId != INVALID_VALUE ) );
		if ( xpfLikely ( blockId != INVALID_VALUE ) )
			recycle(tier, blockId);
	}

	// dealloc() without size hint.
	void free ( void *p )
	{
		u32 tier, blockId;
		bool located = locateBlockInUse(p, tier, blockId);
		xpfAssert( ( "Expecting a managed pointer.", located ) );
		if (located)
		{
			recycle(tier, blockId);
		}
	}

	// Change the size of allocated block pointed by p.
	// May move the memory block to a new location (whose addr will be returned).
	// The content of the memory block is preserved up to the lesser of the new and old sizes.
	// Return NULL on error, in which case the memory content will not be touched.
	void* realloc ( void *p, const u32 size )
	{
		u32 tier, blockId;
		if (locateBlockInUse(p, tier, blockId))
		{
			const u32 blockSize = blockSizeOf(tier);

			// boundary cases
			if ((0 == tier) && (size > blockSize))
				return 0;
			if (((TierNum-1) == tier) && (size <= blockSize))
				return p;

			// The new size is large than the current block size, 
			// promote to a bigger block.
			if (size > blockSize)
			{
				void * b = alloc(size);
				if (NULL != b)
				{
					::memcpy(b, p, blockSize);
					dealloc(p, blockSize);
				}
				return b;
			}

			// The new size is less-or-equal to half of the current 
			// block size, demote to a smaller block.
			if (size <= (blockSize >> 1))
			{
				void * b = alloc(size);
				if (NULL != b)
				{
					::memcpy(b, p, size);
					dealloc(p, blockSize);
				}
				return b;
			}

			// Current block is still suitable for the new size, so nothing changes.
			return p;
		}
		return 0;
	}

	inline u32 capacity() const { return Capacity; }

private:

	u32 obtain( const u32 tier )
	{
		// 1. Check if there is any free block available in given tier, 
		//    return one of them if does.
		FreeBlockRecord *fbr = FreeChainHead[tier];
		if (NULL != fbr)
		{
			const u32 blockSize = blockSizeOf(tier);
			const u32 blockId = ((char*)fbr - Chunk)/blockSize;

			FreeChainHead[tier] = fbr->Next;
			if (fbr->Next)
			{
				fbr->Next->Prev = NULL;
			}

			setBlockInUse(tier, blockId, true);
			return blockId;
		}

		// 2. Ask the upper tier to split one of its free block to two smaller 
		//    blocks to join given tier. Return one of them for use and push
		//    another in the free block chain of current tier.
		const u32 upperBlockId = (tier > 0)? obtain(tier-1) : INVALID_VALUE;
		if (INVALID_VALUE != upperBlockId)
		{
			const u32 blockId1 = (upperBlockId << 1);
			const u32 blockId2 = blockId1 + 1;
			setBlockInUse(tier, blockId1, true);
			setBlockInUse(tier, blockId2, true);
			recycle(tier, blockId2);
			return blockId1;
		}

		// 3. No available free block, returns INVALID_VALUE.
		return INVALID_VALUE;
	}

	void recycle( const u32 tier, const u32 blockId)
	{
		// 1. Validate the in-use bit of given block.
		xpfAssert( ( "Expecting a in-using blockId.", isBlockInUse(tier, blockId) == true ) );
		setBlockInUse(tier, blockId, false);

		// 2. Check the status of its buddy block. 
		const u32 buddyBlockId = buddyIdOf(blockId);
		const u32 blockSize = blockSizeOf(tier);
		if ((0 == tier) || (isBlockInUse(tier, buddyBlockId)))
		{
			// 2a. The buddy block is currently in-use, so we just
			//     clear the in-use bit of the recycling block and
			//     push it to the free block chain of current tier.

			// Setup FreeBlockRecord and push to free chain
			FreeBlockRecord * fbr = (FreeBlockRecord*)(Chunk + (blockSize * (blockId + 1)) - sizeof(FreeBlockRecord));
			if (FreeChainHead[tier])
			{
				FreeChainHead[tier]->Prev = fbr;
			}
			fbr->Next = FreeChainHead[tier];
			fbr->Prev = NULL;
			FreeChainHead[tier] = fbr;
		}
		else
		{
			// 2b. The buddy block is also free, remove buddy block 
			//     from the free block chain and return these 2 blocks
			//     back to upper tier.

			FreeBlockRecord * fbr = (FreeBlockRecord*)(Chunk + (blockSize * (buddyBlockId + 1)) - sizeof(FreeBlockRecord));
			if (fbr->Prev)
			{
				fbr->Prev->Next = fbr->Next;
			}
			if (fbr->Next)
			{
				fbr->Next->Prev = fbr->Prev;
			}
			if (fbr == FreeChainHead[tier])
			{
				FreeChainHead[tier] = (fbr->Prev)? fbr->Prev: fbr->Next;
			}

			recycle(tier-1, (blockId >> 1));
		}
	}

	inline bool isValidPtr ( void *p ) const
	{
		xpfAssert( ( "Expecting a managed pointer (>= bulk start).", (char*)p >= Chunk ) );
		if ( xpfUnlikely ((char*)p < Chunk) )
			return false;

		const u32 offset = (u32)((char*)p - Chunk);
		xpfAssert( ( "Expecting a managed pointer ( < bulk length).", offset < Capacity ) );
		if ( xpfUnlikely (offset >= Capacity) )
			return false;

		// Check if it aligns to minimal block size.
		const bool aligned = (offset == (offset & (0xFFFFFFFF << MINSIZE_POWOF2)));
		xpfAssert( ( "Expecting an aligned pointer.", aligned ) );
		return aligned;
	}

	// Input: A pointer to an allocated block and its tier index.
	// Return: A pointer to its buddy block on the same tier. Or NULL on error.
	inline void* buddyAddrOf ( const u32 tier, void *p )
	{
		if ( xpfUnlikely (isValidPtr(p)) )
			return NULL;

		const u32 offset = (u32)((char*)p - Chunk);
		return (void*)(Chunk + (offset ^ blockSizeOf(tier)));
	}

	// Input: Block id of any tier.
	// Return: The block id of its buddy on the same tier.
	inline u32 buddyIdOf ( const u32 blockId ) const
	{
		return (blockId ^ 0x1);
	}

	// Input: A pointer to an allocated block and its tier index.
	// Return: The block id of the input block on that tier. Or INVALID_VALUE on error.
	inline u32 blockIdOf ( const u32 tier, void *p ) const
	{
		const bool valid = isValidPtr(p);
		xpfAssert( ( "Expecting a valid pointer.", valid ) );
		if ( xpfUnlikely (!valid) )
			return INVALID_VALUE;

		// Check if p aligns to the block size of given tier.
		const u32 offset = (u32)((char*)p - Chunk);
		const u32 mask = (0xFFFFFFFF << (MINSIZE_POWOF2 + TierNum - tier - 1));
		const bool aligned = (offset == (offset & mask));
		xpfAssert( ( "Expecting an aligned pointer.", aligned ) );
		if ( xpfUnlikely (!aligned) )
			return INVALID_VALUE;

		return (offset / blockSizeOf(tier));
	}

	// Return the block size of given tier.
	inline u32 blockSizeOf ( const u32 tier ) const
	{
		xpfAssert( ( "Expecting a valid tier index.", tier < TierNum ) );
		return (1 << (MINSIZE_POWOF2 + TierNum - tier - 1));
	}

	// Input: Block size.
	// Return: The index of tier which deals with such block size.
	inline u32 tierOf ( const u32 size ) const
	{
		u32 s = (1 << MINSIZE_POWOF2);
		s32 t = TierNum - 1;
		while (t >= 0)
		{
			if (size <= s)
				return (u32)t;
			s <<= 1;
			t--;
		}

		return INVALID_VALUE;
	}

	inline bool isBlockInUse (const u32 tier, const u32 blockId) const
	{
		xpfAssert( ( "Expecting a valid tier index.", tier < TierNum ) );
		const u32 idx = (1 << tier) + blockId;
		const u32 offset = (idx >> 3);
		const char mask = (1 << (idx & 0x7));
		return (0 != (Flags[offset] & mask));
	}

	inline void setBlockInUse (const u32 tier, const u32 blockId, bool val)
	{
		xpfAssert( ( "Expecting a valid tier index.", tier < TierNum ) );
		const u32 idx = (1 << tier) + blockId;
		const u32 offset = (idx >> 3);
		const char mask = (1 << (idx & 0x7));
		if (val)
		{
			Flags[offset] |= mask;
		}
		else
		{
			Flags[offset] &= (mask ^ 0xFF);
		}
	}

	// Try to find out the blockId and tier index of a in-use block
	// based on given pointer. Return true if found one with its tier
	// index and block id stored in outTier and outBlockId. Return false
	// on error.
	bool locateBlockInUse ( void *p, u32& outTier, u32& outBlockId ) const
	{
		bool ret = false;
		u32 t = TierNum;
		do
		{
			--t;
			const u32 blockId = blockIdOf(t, p);
			xpfAssert( ( "Expecting a valid blockId.", blockId != INVALID_VALUE ) );
			if (INVALID_VALUE == blockId)
				break;

			if (isBlockInUse(t, blockId))
			{
				outTier = t;
				outBlockId = blockId;
				ret = true;
				break;
			}
		} while (0 != t);
		return ret;
	}

	u32		          Capacity;
	char*	          Chunk;
	u32               FlagsLen;
	char*             Flags;
	u32               TierNum;
	FreeBlockRecord** FreeChainHead;
};

// *****************************************************************************

MemoryPool::MemoryPool(u32 size)
	: mDetails(new MemoryPoolDetails(size))
{
}

MemoryPool::~MemoryPool()
{
	if (mDetails)
	{
		delete mDetails;
		mDetails = (MemoryPoolDetails*) 0xfefefefe;
	}
}

u32 MemoryPool::capacity() const
{
	xpfAssert( ( "Null mDetails.", mDetails != 0 ) );
	return mDetails->capacity();
}

void* MemoryPool::alloc ( u32 size )
{
	xpfAssert( ( "Null mDetails.", mDetails != 0 ) );
	return mDetails->alloc(size);
}

void MemoryPool::dealloc ( void *p, u32 size )
{
	xpfAssert( ( "Null mDetails.", mDetails != 0 ) );
	mDetails->dealloc(p, size);
}

void MemoryPool::free ( void *p )
{
	xpfAssert( ( "Null mDetails.", mDetails != 0 ) );
	mDetails->free(p);
}

void* MemoryPool::realloc ( void *p, u32 size )
{
	xpfAssert( ( "Null mDetails.", mDetails != 0 ) );
	return mDetails->realloc(p, size);
}

u32 MemoryPool::create( u32 size, u16 slotId )
{
	if ( xpfUnlikely(slotId >= MAXPOOL_SLOT) )
		return 0;

	destory(slotId);
	_global_pool_instances[slotId] = new MemoryPool(size);
	xpfAssert( ( "Unable to create memory bulk.", _global_pool_instances[slotId] != 0 ) );
	return (_global_pool_instances[slotId])? _global_pool_instances[slotId]->capacity() : 0;
}

void MemoryPool::destory(u16 slotId)
{
	if ( xpfUnlikely(slotId >= MAXPOOL_SLOT) )
		return;

	if ( xpfLikely (_global_pool_instances[slotId] != 0) )
	{
		delete _global_pool_instances[slotId];
		_global_pool_instances[slotId] = 0;
	}
}

MemoryPool* MemoryPool::instance(u16 slotId)
{
	return (xpfUnlikely(slotId >= MAXPOOL_SLOT))? NULL: _global_pool_instances[slotId];
}

}; // end of namespace xpf


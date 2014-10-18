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
#define MAXSLOT (256)

namespace xpf {

//============---------- BuddyAllocator -----------================//

static BuddyAllocator* _global_pool_instances[MAXSLOT] = { 0 };

/****************************************************************************
 * An implementation of Buddy memory allocation algorithm
 * http://en.wikipedia.org/wiki/Buddy_memory_allocation
 ****************************************************************************/

struct BuddyAllocatorDetails
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
	explicit BuddyAllocatorDetails(u32 size)
		: TierNum(1)
		, Capacity(1 << MINSIZE_POWOF2)
		, UsedBytes(0)
		, HWMBytes(0)
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

	~BuddyAllocatorDetails()
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

		const u32 bsize = blockSizeOf(tier);

		UsedBytes += bsize;
		if (UsedBytes > HWMBytes)
			HWMBytes = UsedBytes;

		return (void*)(Chunk + (blockId * bsize));
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
		{
			recycle(tier, blockId);
			UsedBytes -= blockSizeOf(tier);
		}
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
			UsedBytes -= blockSizeOf(tier);
		}
	}

	// Change the size of allocated block pointed by p.
	// May move the memory block to a new location (whose addr will be returned).
	// The content of the memory block is preserved up to the lesser of the new and old sizes.
	// Return NULL on error, in which case the memory content will not be touched.
	void* realloc ( void *p, const u32 size )
	{
		// forwarding cases
		if ( xpfUnlikely ( NULL == p ) )
		{
			return alloc(size);
		}
		if ( xpfUnlikely ( 0 == size ) )
		{
			free(p);
			return NULL;
		}

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
	inline u32 usedBytes() const { return UsedBytes; }
	inline u32 hwmBytes(bool reset = false) { u32 ret = HWMBytes; if (reset) HWMBytes = 0; return ret; }
	u32 available() const
	{
		for (u32 t=0; t<TierNum; ++t)
		{
			if (NULL != FreeChainHead[t])
			{
				return blockSizeOf(t);
			}
		}
		return 0;
	}

private:

	u32 obtain( const u32 tier )
	{
		// 1. Check if there is any free block available in given tier, 
		//    return one of them if does.
		FreeBlockRecord *fbr = FreeChainHead[tier];
		if (NULL != fbr)
		{
			const u32 blockSize = blockSizeOf(tier);
			const u32 blockId = ((u32)((char*)fbr - Chunk))/blockSize;

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

	u32               UsedBytes;
	u32               HWMBytes;
};

// *****************************************************************************

BuddyAllocator::BuddyAllocator(u32 size)
	: mDetails(new BuddyAllocatorDetails(size))
{
}

BuddyAllocator::~BuddyAllocator()
{
	if (mDetails)
	{
		delete mDetails;
		mDetails = (BuddyAllocatorDetails*)0xfefefefe;
	}
}

u32 BuddyAllocator::capacity() const
{
	xpfAssert( ( "Null mDetails.", mDetails != 0 ) );
	return mDetails->capacity();
}

u32 BuddyAllocator::available() const
{
	xpfAssert( ( "Null mDetails.", mDetails != 0 ) );
	return mDetails->available();
}

u32 BuddyAllocator::used() const
{
	xpfAssert( ( "Null mDetails.", mDetails != 0 ) );
	return mDetails->usedBytes();
}

u32 BuddyAllocator::hwm() const
{
	xpfAssert( ( "Null mDetails.", mDetails != 0 ) );
	return mDetails->hwmBytes();
}

u32 BuddyAllocator::reset()
{
	xpfAssert( ( "Null mDetails.", mDetails != 0 ) );
	return mDetails->hwmBytes(true);
}

void* BuddyAllocator::alloc(u32 size)
{
	xpfAssert( ( "Null mDetails.", mDetails != 0 ) );
	return mDetails->alloc(size);
}

void BuddyAllocator::dealloc(void *p, u32 size)
{
	xpfAssert( ( "Null mDetails.", mDetails != 0 ) );
	mDetails->dealloc(p, size);
}

void BuddyAllocator::free(void *p)
{
	xpfAssert( ( "Null mDetails.", mDetails != 0 ) );
	mDetails->free(p);
}

void* BuddyAllocator::realloc(void *p, u32 size)
{
	xpfAssert( ( "Null mDetails.", mDetails != 0 ) );
	return mDetails->realloc(p, size);
}

void* BuddyAllocator::calloc(u32 num, u32 size)
{
	xpfAssert( ( "Null mDetails.", mDetails != 0 ) );
	const u32 length = num * size;
	void *ptr = mDetails->alloc(length);
	if ( xpfLikely(NULL != ptr) )
	{
		::memset(ptr, 0, length);
	}
	return ptr;
}



//===========----- BuddyAllocator static members ------==============//

u32 BuddyAllocator::create(u32 size, u16 slotId)
{
	if ( xpfUnlikely(slotId >= MAXSLOT) )
		return 0;

	destory(slotId);
	_global_pool_instances[slotId] = new BuddyAllocator(size);
	xpfAssert( ( "Unable to create memory bulk.", _global_pool_instances[slotId] != 0 ) );
	return (_global_pool_instances[slotId])? _global_pool_instances[slotId]->capacity() : 0;
}

void BuddyAllocator::destory(u16 slotId)
{
	if ( xpfUnlikely(slotId >= MAXSLOT) )
		return;

	if ( xpfLikely (_global_pool_instances[slotId] != 0) )
	{
		delete _global_pool_instances[slotId];
		_global_pool_instances[slotId] = 0;
	}
}

BuddyAllocator* BuddyAllocator::instance(u16 slotId)
{
	return (xpfUnlikely(slotId >= MAXSLOT))? NULL: _global_pool_instances[slotId];
}

//============---------- BuddyAllocator -----------================//
















//============---------- LinearAllocator -----------================//

static LinearAllocator* _global_stack_instances[MAXSLOT] = { 0 };

/****************************************************************************
 * An allocator implementation which shares similiar idea from obstack project.
 * https://github.com/cleeus/obstack
 ****************************************************************************/

struct LinearAllocatorDetails
{
	struct FreeCellRecord
	{
		u32 PrevOffset;
		u32 CRC;
	};

	char *Chunk;
	u32   Current;
	u32   Previous;

	u32   Capacity;
	u32   HWMBytes;
};

LinearAllocator::LinearAllocator(u32 size)
{
	// This assumption shall always true for current implementation.
	xpfSAssert( MAXSIZE_POWOF2 <= 31);

	if ( size < (1 << MINSIZE_POWOF2) )
	{
		size = (1 << MINSIZE_POWOF2);
	}
	else if ( size > (1 << MAXSIZE_POWOF2) )
	{
		size = (1 << MAXSIZE_POWOF2);
	}
	else if ( xpfUnlikely(0 != (size & 0x7)) )
	{
		// Upgrade size if it is not a multiply of 8.
		size = (((size >> 3) + 1) << 3);
	}

	mDetails = new LinearAllocatorDetails;
	mDetails->Chunk = (char*) ::malloc(size);
	mDetails->Current = 0;
	mDetails->Previous = 0;
	mDetails->Capacity = size;
	mDetails->HWMBytes = 0;
}

LinearAllocator::~LinearAllocator()
{
	if (mDetails)
	{
		::free(mDetails->Chunk);
		delete mDetails;
		mDetails = (LinearAllocatorDetails*)0xfefefefe;
	}
}

u32 LinearAllocator::capacity() const
{
	xpfAssert( ( "Null mDetails.", mDetails != 0 ) );
	return mDetails->Capacity;
}

u32 LinearAllocator::available() const
{
	xpfAssert( ( "Null mDetails.", mDetails != 0 ) );
	const u32 size = mDetails->Capacity - mDetails->Current;
	if (size < sizeof(LinearAllocatorDetails::FreeCellRecord))
		return 0;
	return size - sizeof(LinearAllocatorDetails::FreeCellRecord);
}

u32 LinearAllocator::used() const
{
	xpfAssert( ( "Null mDetails.", mDetails != 0 ) );
	return mDetails->Current;
}

u32 LinearAllocator::hwm() const
{
	xpfAssert( ( "Null mDetails.", mDetails != 0 ) );
	return mDetails->HWMBytes;
}

u32 LinearAllocator::reset()
{
	xpfAssert( ( "Null mDetails.", mDetails != 0 ) );

	u32 ret = mDetails->HWMBytes;
	mDetails->Current = mDetails->Previous = mDetails->HWMBytes = 0;
	return ret;
}

void* LinearAllocator::alloc(u32 size)
{
	xpfAssert( ( "Null mDetails.", mDetails != 0 ) );

	// Every allocation has an internal space overhead of
	// sizeof(MemoryStackDetails::FreeCellRecord) (8 bytes).
	size += sizeof(LinearAllocatorDetails::FreeCellRecord);

	// Upgrade the size if it is not a multiply of 8.
	// This can make sure every pointer we return is 8-bytes aligned.
	if ( 0 != (size & 0x7) )
	{
		size = (((size >> 3) + 1) << 3);
	}

	if (size > available())
		return NULL;

	xpfAssert( ("Expecting Current < Capacity", mDetails->Current < mDetails->Capacity) );
	if ( xpfLikely(mDetails->Current < mDetails->Capacity) )
	{
		LinearAllocatorDetails::FreeCellRecord *rec =
			(LinearAllocatorDetails::FreeCellRecord*)(mDetails->Chunk + mDetails->Current);
		rec->PrevOffset = mDetails->Previous;
		rec->CRC        = rec->PrevOffset ^ 0x5FEE1299; // as checksum of prev.
		mDetails->Previous = mDetails->Current;

		// This implies the first bit of rec->PrevOffset is not used and 
		// can be used as a flag indicates the chunk has been freed or not.
		xpfAssert( ("Expecting PrevOffset < (1<<MAXSIZE_POWOF2)", rec->PrevOffset < (1<<MAXSIZE_POWOF2)) );

		char *ret = mDetails->Chunk + mDetails->Current + sizeof(LinearAllocatorDetails::FreeCellRecord);
		mDetails->Current += size;

		if ( mDetails->Current > mDetails->HWMBytes )
			mDetails->HWMBytes = mDetails->Current;

		return (void*)ret;
	}

	return NULL;
}

void LinearAllocator::dealloc(void *p, u32 size)
{
	xpfAssert( ( "Null mDetails.", mDetails != 0 ) );
	XPF_NOTUSED(size);
	free(p);
}

void LinearAllocator::free(void *p)
{
	xpfAssert( ( "Null mDetails.", mDetails != 0 ) );

	char *cell = (char*)p - sizeof(LinearAllocatorDetails::FreeCellRecord);

	xpfAssert( ( "Expecting managed pointer.", ( (cell >= mDetails->Chunk) && (cell < mDetails->Chunk + mDetails->Capacity) ) ) );

	if ( xpfLikely( (cell >= mDetails->Chunk) && (cell < mDetails->Chunk + mDetails->Capacity) ) )
	{
		// Check if data corrupted.
		LinearAllocatorDetails::FreeCellRecord *rec = (LinearAllocatorDetails::FreeCellRecord*)cell;
		xpfAssert( ("Checksum matched (data corrupted)", ((rec->PrevOffset & 0x7FFFFFFF) ^ 0x5FEE1299) == rec->CRC ) );
		xpfAssert( ("In-using cell", (0 == (rec->PrevOffset & 0x80000000))) );

		// Mark this cell as freed
		rec->PrevOffset |= 0x80000000;

		// For cells who are not at the top of the stack,
		// just return here.
		if ( mDetails->Previous != (cell - mDetails->Chunk) )
			return;

		// For the top-most cell, we need to apply a rollback sequence
		// to adjust both mDetails->Current and mDetails->Previous to fit 
		// the next top-most cell which is still alive.
		while (true)
		{
			mDetails->Current = (u32)((char*)rec - mDetails->Chunk);

			// Quit the loop if hit the bottom of stack (all cells have been freed).
			if ( xpfUnlikely ( 0 == mDetails->Current ) )
			{
				mDetails->Previous = 0;
				break;
			}

			// Locate and verify the FreeCellRecord of previous cell.
			rec = (LinearAllocatorDetails::FreeCellRecord*)(mDetails->Chunk + mDetails->Previous);
			xpfAssert( ("Checksum matched (data corrupted)", ((rec->PrevOffset & 0x7FFFFFFF) ^ 0x5FEE1299) == rec->CRC ) );
			mDetails->Previous = (rec->PrevOffset & 0x7FFFFFFF);

			// Quit the loop whenever we meet an alive cell.
			if ( 0 == (rec->PrevOffset & 0x80000000) )
				break;
		} // end of while (true)
	}
}

void* LinearAllocator::realloc(void *p, u32 size)
{
	// NOTE: Using of realloc() of MemoryStack is highly
	//       discouraged.
	//       It always allocates a new cell with given
	//       'size' and copy the content pointed by 'p' to 
	//       newly allocated cell.
	//       This means it could be very space-expensive.

	xpfAssert( ( "Null mDetails.", mDetails != 0 ) );

	if ( 0 == size )
		return NULL;

	void *ptr = alloc(size);
	if ( xpfLikely ( NULL != ptr) )
	{
		if ( NULL != p )
		{
			::memcpy(ptr, p, size);
			free(p);
		}
	}
	return ptr;
}

void* LinearAllocator::calloc(u32 num, u32 size)
{
	xpfAssert( ( "Null mDetails.", mDetails != 0 ) );
	const u32 length = num * size;
	void *ptr = alloc(length);
	if ( xpfLikely(NULL != ptr) )
	{
		::memset(ptr, 0, length);
	}
	return ptr;
}


//===========----- LinearAllocator static members ------==============//

u32  LinearAllocator::create(u32 size, u16 slotId)
{
	if ( xpfUnlikely(slotId >= MAXSLOT) )
		return 0;

	destory(slotId);
	_global_stack_instances[slotId] = new LinearAllocator(size);
	xpfAssert( ( "Unable to create memory bulk.", _global_stack_instances[slotId] != 0 ) );
	return (_global_stack_instances[slotId])? _global_stack_instances[slotId]->capacity() : 0;
}

void LinearAllocator::destory(u16 slotId)
{
	if ( xpfUnlikely(slotId >= MAXSLOT) )
		return;

	if ( xpfLikely (_global_stack_instances[slotId] != 0) )
	{
		delete _global_stack_instances[slotId];
		_global_stack_instances[slotId] = 0;
	}
}

LinearAllocator* LinearAllocator::instance(u16 slotId)
{
	return (xpfUnlikely(slotId >= MAXSLOT))? NULL: _global_stack_instances[slotId];
}


//============---------- LinearAllocator -----------================//


}; // end of namespace xpf


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

#ifndef _XPF_ALLOCATORS_HEADER_
#define _XPF_ALLOCATORS_HEADER_

#include "platform.h"
#include <memory>

namespace xpf {

//============---------- MemoryPool -----------================//

struct MemoryPoolDetails;

class XPF_API MemoryPool
{
public:
	/*****
	 *  Initialize a global memory pool in slot of 'slotId' which pre-
	 *  allocates a memory bulk of length of 'size'. The 'size' shall be 
	 *  power of 2 or it will be promoted to be one (nearest but not 
	 *  less than). The promoted size value should neither less than 2^4 
	 *  (16 bytes) nor larger than 2^31 (2 Gb). Later calls of 
	 *  MemoryPool::instance() with the same 'slotId' returns the pointer
	 *  to this global memory pool instance. One should call MemoryPool::
	 *  destroy() with 'slotId' after done using to release the memory
	 *  bulk.
	 *
	 *  'slotId' should be a unsigned short integer ranged from 0 to 255.
	 *
	 *  Returns the promoted memory pool size. Returns 0 on error.
	 */
	static u32  create( u32 size, u16 slotId = 0 );

	/*****
	 *  Delete the global memory pool instance in slot of 'slotId' 
	 *  if ever created.
	 */
	static void destory(u16 slotId = 0);

	/*****
	 *  Return the pointer to the global accessable memory pool instance
	 *  in given slot. Make sure the MemoryPool::create() has been called 
	 *  on the same slot otherwise it returns 0 (NULL).
	 */
	static MemoryPool* instance(u16 slotId = 0);

public:
	explicit MemoryPool ( u32 size );
	~MemoryPool();

	// Returns the pool size in bytes.
	u32   capacity () const;

	// Return the current maximum allocatable space in bytes.
	u32   available () const;

	// Return the used (allocated) space in bytes.
	u32   used() const;

	// Return the high water mark in bytes since last reset
	u32   hwm() const;

	// Return the high water mark in bytes and reset its value.
	u32   reset();

	// Allocate a memory chunk which is at least 'bytes' long.
	// Our implementation guarantees the returned pointer is 
	// aligned on 16-bytes boundary.
	void* alloc ( u32 size );

	// Free up a memory chunk which is previously allocated by this pool.
	void  dealloc ( void *p, u32 size );

	// dealloc() without size hint
	void  free ( void *p );

	// Extend or shrink the allocated block size.
	// May move the memory block to a new location (whose addr will be returned).
	// The content of the memory block is preserved up to the lesser of the new and old sizes.
	void* realloc ( void *p, u32 size );

	// Allocate for an array of 'num' elements, each of length 'size' bytes.
	// All allocated memory bytes are initialized to 0.
	void* calloc ( u32 num, u32 size );

private:
	// non-copyable
	MemoryPool ( const MemoryPool& other ) {}
	MemoryPool& operator = ( const MemoryPool& other ) { return *this; }

	MemoryPoolDetails *mDetails;
}; // end of class MemoryPool

//============---------- MemoryPool -----------================//







//============---------- MemoryStack -----------================//



struct MemoryStackDetails;

class XPF_API MemoryStack
{
	// NOTE: Expose the same interface as MemoryPool.
public:
	static u32  create( u32 size, u16 slotId = 0 );
	static void destory(u16 slotId = 0);
	static MemoryStack* instance(u16 slotId = 0);

	explicit MemoryStack ( u32 size );
	~MemoryStack();

	u32   capacity () const;
	u32   available () const;
	u32   used() const;
	u32   hwm() const;
	// Return current hwm and reset both hwm and stack pointer.
	u32   reset();

	void* alloc ( u32 size );
	void  dealloc ( void *p, u32 size );
	void  free ( void *p );
	void* realloc ( void *p, u32 size );
	void* calloc ( u32 num, u32 size );

private:
	// non-copyable
	MemoryStack ( const MemoryStack& other ) {}
	MemoryStack& operator = ( const MemoryStack& other ) { return *this; }

	MemoryStackDetails *mDetails;
}; // end of class MemoryStack



//============---------- MemoryStack -----------================//








//============---------- STL allocator compatible -----------================//

template < typename T , typename ALLOCATOR, xpf::u16 SLOT = 0 >
class Allocator
{
public:
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T value_type;
	typedef u32 size_type;
	typedef ptrdiff_t difference_type;

	template < typename U >
	struct rebind { typedef Allocator<U, ALLOCATOR, SLOT> other; };

public:
	Allocator ()
	{
		mPool = ALLOCATOR::instance(SLOT);
	}

	Allocator ( const Allocator& other )
	{
		mPool = other.mPool;
	}

	template < typename U >
	Allocator ( const Allocator<U, ALLOCATOR, SLOT>& other )
	{
		mPool = other.mPool;
	}

	~Allocator ()
	{
		mPool = 0;
	}

	Allocator& operator = ( const Allocator& other )
	{
		// DO NOT exchange mPool
	}

	template < typename U >
	Allocator& operator = ( const Allocator<U, ALLOCATOR, SLOT>& other )
	{
		// DO NOT exchange mPool
	}

	pointer address ( reference x ) const
	{
		return &x;
	}

	const_pointer address ( const_reference x ) const
	{
		return &x;
	}

	pointer allocate ( size_type n, void* hint = 0 )
	{
		xpfAssert( ( "Null mPool.", mPool != 0 ) );
		pointer ptr = (pointer) mPool->alloc(n * sizeof(value_type));
		if (NULL == ptr)
			throw std::bad_alloc();
		return ptr;
	}

	void deallocate ( pointer p, size_type n )
	{
		xpfAssert( ( "Null mPool.", mPool != 0 ) );
		mPool->dealloc( (void*) p, n * sizeof(value_type) );
	}

	size_type max_size() const
	{
		xpfAssert( ( "Null mPool.", mPool != 0 ) );
		return (mPool->capacity() / sizeof(value_type));
	}

	// Call the c'tor to construct object using the storage space passed in.
	void construct ( pointer p, const_reference val )
	{
		new ((void*)p) value_type (val);
	}

	// Call the d'tor but not release the storage.
	void destroy (pointer p)
	{
		p->~value_type();
	}

public:
	ALLOCATOR* mPool; // weak reference to a exists memory pool instance.

}; // end of class Allocator

}; // end of namespace xpf

#endif // _XPF_ALLOCATORS_HEADER_

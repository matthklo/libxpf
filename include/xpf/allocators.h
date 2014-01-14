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

struct MemoryPoolDetails;

class XPF_API MemoryPool
{
public:
	/*****
	 *  Initialize a global memory pool which pre-allocates a memory bulk 
	 *  of length of 'size'. The 'size' must be power of 2 or it will be
	 *  promoted to be one (nearest but not less than). The promoted size
	 *  value should neither less than 2^4 (16 bytes) nor larger than 2^31
	 *  (2 Gb). Later calls of MemoryPool::instance() returns the pointer
	 *  to this global memory pool instance. One should call MemoryPool::
	 *  destroy() after done using.
	 *
	 *  Returns the promoted memory pool size. Returns 0 on error.
	 */
	static u32  create( u32 size );

	/*****
	 *  Delete the global memory pool instance if ever created.
	 */
	static void destory();

	/*****
	 *  Return the pointer to the global accessable memory pool instance.
	 *  Make sure the MemoryPool::create() has been called before or
	 *  this returns 0 (NULL).
	 */
	static MemoryPool* instance();

public:
	explicit MemoryPool ( u32 size );
	~MemoryPool();

	// Returns the pool size in bytes.
	u32   capacity () const;

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

private:
	// non-copyable
	MemoryPool ( const MemoryPool& other ) {}
	MemoryPool& operator = ( const MemoryPool& other ) { return *this; }

	MemoryPoolDetails *mDetails;
};

template < typename T, MemoryPool* INST = 0 >
class MemoryPoolAllocator
{
public:
	typedef T* pointer;
	typedef const T* const_pointer;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T value_type;
	typedef u32 size_type;

	template < typename U >
	struct rebind { typedef MemoryPoolAllocator<U> other; };

public:
	MemoryPoolAllocator ()
	{
		mPool = (INST == 0)? MemoryPool::instance(): INST;
	}

	MemoryPoolAllocator ( const MemoryPoolAllocator& other )
	{
		mPool = other.mPool;
	}

	template < typename U >
	MemoryPoolAllocator ( const MemoryPoolAllocator<U>& other )
	{
		mPool = other.mPool;
	}

	~MemoryPoolAllocator ()
	{
		mPool = 0;
	}

	MemoryPoolAllocator& operator = ( const MemoryPoolAllocator& other )
	{
		// DO NOT exchange mPool
	}

	template < typename U >
	MemoryPoolAllocator& operator = ( const MemoryPoolAllocator<U>& other )
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
		xpfAssert(mPool != 0);
		return (pointer) mPool->alloc(n * sizeof(value_type));
	}

	void deallocate ( pointer p, size_type n )
	{
		xpfAssert(mPool != 0);
		mPool->dealloc( (void*) p, n * sizeof(value_type) );
	}

	size_type max_size() const
	{
		xpfAssert(mPool != 0);
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

private:
	MemoryPool* mPool; // weak reference to a exists memory pool instance.

}; // end of class MemoryPoolAllocator

}; // end of namespace xpf

#endif // _XPF_ALLOCATORS_HEADER_
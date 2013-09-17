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
#include "refcnt.h"
#include <memory>

namespace xpf {

class MemoryPool;

template < typename T , MemoryPool * _Pool >
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
	struct rebind { typedef MemoryPoolAllocator<U, _Pool> other; };

public:
	explicit MemoryPoolAllocator ()
	{
		mPool = _Pool;
	}

	MemoryPoolAllocator ( const MemoryPoolAllocator& other )
	{
	}

	template < typename U >
	MemoryPoolAllocator ( const MemoryPoolAllocator<U, _Pool>& other )
	{
	}

	~MemoryPoolAllocator ()
	{
	}

	MemoryPoolAllocator& operator = ( const MemoryPoolAllocator& other )
	{
	}

	template < typename U >
	MemoryPoolAllocator& operator = ( const MemoryPoolAllocator<U, _Pool>& other )
	{
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
		return mPool->alloc(n * sizeof(value_type));
	}

	void deallocate ( pointer p, size_type n )
	{
		mPool->dealloc( (void*) p, n * sizeof(value_type) );
	}

	size_type max_size() const
	{
		return (mPool->capacity() / sizeof(value_type));
	}

	void construct ( pointer p, const_reference val )
	{
		new ((void*)p) value_type (val);
	}

	void destroy (pointer p)
	{
		p->~value_type();
	}

private:
	MemoryPool *mPool;
};

class MemoryPool
{
public:
	explicit MemoryPool ( u32 poolSize )  // in bytes. 1Kb ~ 2Gb
		: mCapacity ( poolSize )
	{
		u32 i = 1024;
		while ( i < poolSize )
		{
			i <<= 1;
			if (0x80000000 == i)
				break;
		}
		mCapacity = i;
		mBuffer = new char[mCapacity];
	}

	~MemoryPool()
	{
		delete[] mBuffer;
		mBuffer = (char*) 0xfefefefe;
	}

	u32 capacity() const
	{
		return mCapacity;
	}

	void* alloc ( u32 bytes )
	{
		return 0;
	}

	void dealloc ( void *p, u32 bytes )
	{
	}

private:
	MemoryPool ( const MemoryPool& other ) {}
	MemoryPool& operator = ( const MemoryPool& other ) {}

	u32    mCapacity;
	char  *mBuffer;
};

}; // end of namespace xpf

#endif // _XPF_ALLOCATORS_HEADER_
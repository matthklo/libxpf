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

struct MemoryPoolDetails;

class XPF_API MemoryPool
{
public:
	static u32         create( u32 poolSize ); // in bytes. 1Kb ~ 2Gb
	static void        destory();
	static MemoryPool* instance();

public:
	~MemoryPool();

	u32   capacity () const;
	void* alloc ( u32 bytes );
	void  dealloc ( void *p, u32 bytes );

protected:
	explicit MemoryPool ( u32 poolSize ); 

private:
	MemoryPool ( const MemoryPool& other ) {}
	MemoryPool& operator = ( const MemoryPool& other ) { return *this; }

	MemoryPoolDetails *mDetails;
};

template < typename T >
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
	}

	MemoryPoolAllocator ( const MemoryPoolAllocator& other )
	{
	}

	template < typename U >
	MemoryPoolAllocator ( const MemoryPoolAllocator<U>& other )
	{
	}

	~MemoryPoolAllocator ()
	{
	}

	MemoryPoolAllocator& operator = ( const MemoryPoolAllocator& other )
	{
	}

	template < typename U >
	MemoryPoolAllocator& operator = ( const MemoryPoolAllocator<U>& other )
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
		MemoryPool *mp = MemoryPool::instance();
		return (pointer) mp->alloc(n * sizeof(value_type));
	}

	void deallocate ( pointer p, size_type n )
	{
		MemoryPool *mp = MemoryPool::instance();
		mp->dealloc( (void*) p, n * sizeof(value_type) );
	}

	size_type max_size() const
	{
		MemoryPool *mp = MemoryPool::instance();
		return (mp->capacity() / sizeof(value_type));
	}

	void construct ( pointer p, const_reference val )
	{
		new ((void*)p) value_type (val);
	}

	void destroy (pointer p)
	{
		p->~value_type();
	}
};

}; // end of namespace xpf

#endif // _XPF_ALLOCATORS_HEADER_
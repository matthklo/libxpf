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

#ifndef _XPF_TLS_HEADER_
#define _XPF_TLS_HEADER_

#include "platform.h"
#include "threadlock.h"
#include <map>

namespace xpf {


template <	typename ValueType, 
			typename Compare = std::less<ValueType>, 
			typename Alloc = std::allocator< std::pair< xpf::ThreadID, ValueType> > >
class ThreadLocalStorage
{
public:
	typedef std::map< xpf::ThreadID, ValueType, Compare, Alloc > StorageMapType;

public:
	ThreadLocalStorage() {}
	ThreadLocalStorage(const ThreadLocalStorage& other) { *this = other; }
	~ThreadLocalStorage() {}

	ThreadLocalStorage& operator=(const ThreadLocalStorage& other)
	{
		ScopedThreadLock sl(mLock);
		other.dump(mMap);
	}

	void dump(StorageMapType &outMap)
	{
		ScopedThreadLock sl(mLock);
		outMap = mMap;
	}

	bool put(const ValueType& data, bool replace = false, ThreadID tid = 0)
	{
		ScopedThreadLock sl(mLock);
		if (0 == tid)
		{
			tid = Thread::getThreadID();
		}

		if (replace)
		{
			mMap.erase(tid);
		}
		return mMap.insert( std::make_pair(tid, data) ).second;
	}

	bool get(ValueType& outData, ThreadID tid = 0)
	{
		ScopedThreadLock sl(mLock);
		if (0 == tid)
		{
			tid = Thread::getThreadID();
		}

		typename StorageMapType::iterator it = mMap.find(tid);
		if (it != mMap.end())
		{
			outData = it->second;
			return true;
		}
		return false;
	}

	bool clear(ThreadID tid = 0)
	{
		ScopedThreadLock sl(mLock);
		if (0 == tid)
		{
			tid = Thread::getThreadID();
		}

		return (mMap.erase(tid) > 0);
	}

private:
	ThreadLock     mLock;
	StorageMapType mMap;
};

}; // end of namespace xpf

#endif // _XPF_TLS_HEADER_


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

// Compiler-specific TLS keyword
// http://en.wikipedia.org/wiki/Thread-local_storage
#ifdef XPF_COMPILER_SPECIFIED
#  if defined(XPF_COMPILER_MSVC)
#    define XPF_TLS __declspec(thread)
#  else
#    define XPF_TLS __thread
#  endif
#endif

namespace xpf {

/*
 * An abstraction of TLS support from native platforms.
 * On Windows, it wraps TlsAlloc()/TlsFree()/TlsGetValue()/TlsSetValue().
 * On POSIX platforms, it wraps pthread_key_create()/pthread_key_delete()/pthread_getspecific()/pthread_setspecific().
 */

// Returns an opaque value called index. Except 0 indicates an error occurred, do not make any 
// assumption on the content of it.
// Use this index in subsequent calls to TlsDelete()/TlsGet()/TlsSet().
vptr XPF_API TlsCreate();
// Delete an index created by TlsCreate().
void XPF_API TlsDelete(vptr index);
// Retrieve the data associated with the index and the calling thread.
// After an index has been created, it is by default associated with 0
// for all threads.
vptr XPF_API TlsGet(vptr index);
// Associate the index with a custom data.
// This association is space-independent among threads.
void XPF_API TlsSet(vptr index, vptr data);


/*
 * A standard STL map accompany with a thread lock.
 * The key of the map is a thread ID, so that get()/put() calls made 
 * from each individual thread are guaranteed to be targeting on
 * seperated storage space.
 *
 * One should generally use TlsCreate()/TlsDelete()/TlsGet()/TlsSet() 
 * instead of this TlsMap to implement TLS for better performance.
 * TlsMap should be considered as a fallback mechanism on platforms
 * which TlsXXX functions are not working properly. Or for special
 * purpose.
 */
template <	typename ValueType, 
			typename Compare = std::less<ValueType>, 
			typename Alloc = std::allocator< std::pair< xpf::ThreadID, ValueType> > >
class TlsMap
{
public:
	typedef std::map< xpf::ThreadID, ValueType, Compare, Alloc > StorageMapType;

public:
	TlsMap() {}
	TlsMap(const TlsMap& other) { *this = other; }
	~TlsMap() {}

	TlsMap& operator=(const TlsMap& other)
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


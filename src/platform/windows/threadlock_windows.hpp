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

#include <xpf/threadlock.h>

#ifndef XPF_PLATFORM_WINDOWS
#  error threadlock_windows.hpp shall not build on platforms other than Windows.
#endif

#include <Windows.h>

namespace xpf { namespace details {

class XpfThreadLock
{
public:
	XpfThreadLock()
		: mOwner(Thread::INVALID_THREAD_ID)
	{
		::InitializeCriticalSection(&mMutex);
	}

	~XpfThreadLock()
	{
		::DeleteCriticalSection(&mMutex);
	}

	inline void lock()
	{
		::EnterCriticalSection(&mMutex);
		mOwner = Thread::getThreadID();
	}

	inline bool tryLock()
	{
		BOOL ret = ::TryEnterCriticalSection(&mMutex);
		if (TRUE == ret)
			mOwner = Thread::getThreadID();
		return (TRUE == ret)? true: false;
	}

	inline bool isLocked() const
	{
		return (mOwner != Thread::INVALID_THREAD_ID);
	}

	inline void unlock()
	{
		::LeaveCriticalSection(&mMutex);
		mOwner = Thread::INVALID_THREAD_ID;
	}

	inline ThreadID getOwner() const
	{
		return mOwner;
	}

private:
	XpfThreadLock(const XpfThreadLock& that)
	{
		xpfAssert(false);
	}

	XpfThreadLock& operator = (const XpfThreadLock& that)
	{
		xpfAssert(false);
		return *this;
	}

	ThreadID           mOwner;
	CRITICAL_SECTION   mMutex;
};

};};
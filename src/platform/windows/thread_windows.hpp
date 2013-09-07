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

#include <xpf/thread.h>
#include <xpf/threadevent.h>

#ifndef XPF_PLATFORM_WINDOWS
#  error thread_windows.hpp shall not build on platforms other than Windows.
#endif

#include <Windows.h>

namespace xpf { namespace details {

class XpfThread
{
public:
	explicit XpfThread(Thread * host)
		: mExitCode(0)
		, mUserData(0)
		, mID(Thread::INVALID_THREAD_ID)
		, mEvent(new ThreadEvent(false))
		, mStatus(Thread::TRS_READY)
		, mAborting(false)
		, mHost(host)
	{
		DWORD tid;
		mThreadHandle = ::CreateThread(NULL, 0, XpfThread::exec, this, 0, &tid);
		mID = tid;
	}

	virtual ~XpfThread()
	{
		switch (mStatus)
		{
		case Thread::TRS_READY:
		case Thread::TRS_RUNNING:
			mAborting = true;
			// fall through
		case Thread::TRS_FINISHED:
			join(0xFFFFFFFF);
			break;
		default:
			break;
		}
		delete mEvent;
		mEvent = (ThreadEvent*) 0xfefefefe;
	}

	inline void start()
	{
		mStatus = Thread::TRS_RUNNING;
		mEvent->set();
	}

	inline bool join(u32 timeoutMs /*= -1L*/)
	{
		if (mStatus == Thread::TRS_FINISHED)
			return true;

		if ((mStatus == Thread::TRS_RUNNING) &&
			(WAIT_OBJECT_0 == ::WaitForSingleObject(mThreadHandle, timeoutMs)))
		{
			::CloseHandle(mThreadHandle);
			mStatus = Thread::TRS_FINISHED;
			return true;
		}
		return false;
	}

	inline Thread::RunningStatus getStatus() const
	{
		return mStatus;
	}

	inline void setExitCode(u32 code)
	{
		mExitCode = code;
	}

	inline u32 getExitCode() const
	{
		return mExitCode;
	}

	inline u64 getData() const
	{
		return mUserData;
	}

	inline void setData(u64 data)
	{
		mUserData = data;
	}

	inline ThreadID getID() const
	{
		return mID;
	}

	inline void requestAbort()
	{
		mAborting = true;
	}

	inline bool isAborting() const
	{
		return mAborting;
	}

	static void sleep(u32 durationMs)
	{
		::Sleep(durationMs);
	}

	static void yield()
	{
		::Sleep(1);
	}

	static ThreadID getThreadID()
	{
		return (ThreadID) ::GetCurrentThreadId();
	}

	// ====
	static DWORD WINAPI exec (LPVOID pParam)
	{
		XpfThread *thread = (XpfThread*)pParam;
		thread->mEvent->wait(); // wait for start()
		if (!thread->mAborting)
		{
			thread->mExitCode = thread->mHost->run(thread->getData());
		}
		return (DWORD) thread->mExitCode;
	}

private:
	XpfThread(const XpfThread& that)
	{
		xpfAssert(false);
	}

	XpfThread& operator = (const XpfThread& that)
	{
		xpfAssert(false);
		return *this;
	}

	HANDLE                          mThreadHandle;
	ThreadID                        mID;
	u32                             mExitCode;
	u64                             mUserData;
	ThreadEvent                    *mEvent;
	Thread                         *mHost;
	Thread::RunningStatus           mStatus;
	bool                            mAborting;
};

};};
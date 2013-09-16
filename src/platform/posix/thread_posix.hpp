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

#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

namespace xpf { namespace details {

class XpfThread
{
public:
	explicit XpfThread(Thread * host)
		: mExitCode(0)
		, mUserData(0)
		, mID(Thread::INVALID_THREAD_ID)
		, mStartEvent(new ThreadEvent(false))
		, mFinishEvent(new ThreadEvent(false))
		, mStatus(Thread::TRS_READY)
		, mAborting(false)
		, mHost(host)
	{
		pthread_create(&mThreadHandle, NULL, XpfThread::exec, this);
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
			join(-1L);
			break;
		default:
			break;
		}
		delete mStartEvent;
		mStartEvent = (ThreadEvent*) 0xfefefefe;
		delete mFinishEvent;
		mFinishEvent = (ThreadEvent*) 0xfefefefe;
	}

	inline void start()
	{
		mStatus = Thread::TRS_RUNNING;
		mStartEvent->set();
	}

	inline bool join(u32 timeoutMs /*= -1L*/)
	{
		if (mStatus == Thread::TRS_FINISHED)
			return true;

		if ((mStatus == Thread::TRS_RUNNING) &&
			((-1L == timeoutMs) || (mFinishEvent->wait(timeoutMs))))
		{
			pthread_join(mThreadHandle, NULL);
			pthread_detach(mThreadHandle);
			mThreadHandle = 0;
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
		usleep(durationMs * 1000);
	}

	static void yield()
	{
		usleep(1000);
	}

	static ThreadID getThreadID()
	{
		return (ThreadID) pthread_self();
	}

	// ====
	static void* exec (void* param)
	{
		XpfThread *thread = (XpfThread*) param;

		thread->mID = XpfThread::getThreadID();
		thread->mStartEvent->wait();
		if (!thread->mAborting)
		{
			thread->mExitCode = thread->mHost->run(thread->getData());
			thread->mFinishEvent->set();
		}
		return (void*) (vptr) thread->mExitCode;
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

	pthread_t                       mThreadHandle;
	u32                             mExitCode;
	u64                             mUserData;
	Thread::RunningStatus           mStatus;
	ThreadEvent                    *mStartEvent;
	ThreadEvent                    *mFinishEvent;
	Thread                         *mHost;
	ThreadID                        mID;
	bool                            mAborting;
};

};};

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

#ifndef _XPF_THREAD_HEADER_
#define _XPF_THREAD_HEADER_

#include "platform.h"

namespace xpf {

typedef u64 ThreadID;

class XPF_API Thread
{
public:
	static const ThreadID INVALID_THREAD_ID = -1LL;

	enum RunningStatus
	{
		TRS_READY = 0,  // Created but not yet started.
		TRS_RUNNING,    // Started but not yet finished.
		TRS_FINISHED,   // Finished.
	};

public:
	Thread();
	virtual ~Thread();

	/* Causes the caller thread to sleep for the given interval of time (given in milliseconds). */
	static void     sleep(u32 durationMs);
	/* Causes the calling Thread to yield execution time. */
	static void     yield();
	/* Return the caller thread id */
	static ThreadID getThreadID();

	/* Every thread is created as suspended until its start() is called from other thread. */
	void          start();

	/* 
	 * Block the caller thread and wait for the given interval of time trying to join this thread.
	 * If timeoutMs is -1L (or 0xFFFFFFFF) then it blocks forever until this thread has finished.
	 * Return true if this thread has finished and joined, return false on timeout (and this thread is still running).
	 */
	bool          join(u32 timeoutMs = -1L);

	/*
	 * This is the thread body which will be called after this thread 
	 * has started. User must provide their customized body via overriding.
	 * userdata can be set via setData().
	 * The return value will be set to be exit code.
	 *
	 * NOTE: One could hint thread to abort current job and leave asap by
	 *       
	 */
	virtual u32   run(u64 userdata) = 0;

	/* Return true if this thread has been started. */
	RunningStatus getStatus() const;

	/* getter / setter of exit code */
	void          setExitCode(u32 code);
	u32           getExitCode() const;

	/* getter / setter of user data */
	u64           getData() const;
	void          setData(u64 data);

	/* Get the identifier of this thread */
	ThreadID      getID() const;

	/* 
	 * Hint the thread body to abort and return asap.
	 * The detail reaction depends on the derived impl of run().
	 */
	void          requestAbort();

protected:
	/* 
	 * Whether an abort request has been issued.
	 * All derived run() implementation should check
	 * it as frequent as possible and perform 
	 * aborting if requested.
	 */
	bool          isAborting() const;

private:
	// Non-copyable
	Thread(const Thread& that);
	Thread& operator = (const Thread& that);

	vptr pImpl;
};

} // end of namespace xpf

#endif // _XPF_THREAD_HEADER_
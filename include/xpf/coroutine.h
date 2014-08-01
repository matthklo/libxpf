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

#ifndef _XPF_COROUTINE_HEADER_
#define _XPF_COROUTINE_HEADER_

#include "platform.h"

namespace xpf
{

/*****
 * API to create and manipulating coroutines (or fibers).
 * Coroutine are like thread: both have individual execution
 * context and stack. However, unlike threads, context-switching 
 * among coroutines should be performed manually by themselves.
 *
 * One who ever uses fibers on Windows should find this API model
 * seems to be familiar. They shares the same idea of usage.
 * On POSIX platforms, these function are implemented by getcontext(),
 * makecontext(), and swapcontext().
 *
 * General usage:
 *   1. A thread shall calls InitThreadForCoroutines() to install
 *      a coroutine manager in TLS before making other coroutine
 *      API calls. Caller of InitThreadForCoroutines() becomes
 *      the main coroutine.
 *   2. The thread is then able to use CreateCoroutine() to 
 *      create one or more subcoroutines. These subcoroutines
 *      will not run until SwitchToCoroutine() has been called.
 *   3. Whenever a coroutine returns from its coroutine body
 *      or calls DeleteCoroutine() to delete itself, it 
 *      terminates the current thread.
 *   4. When a subcoroutine has finished its job, it shall call
 *      SwitchToCoroutine() to switch back to either: one of other
 *      unfinished subcoroutines, or the main coroutine.
 *   5. The main coroutine shall implement a mechanism to call
 *      DeleteCoroutine() properly on those subcoroutines who have
 *      done their jobs.
 */

/* 
 * Function prototype of the coroutine body.
 * The input argument is used as a userdata dedicated
 * for this coroutine. The coroutine can call 
 * GetCoroutineData() to retrieve this userdata anywhere
 * while it is running.
 */
#ifdef XPF_PLATFORM_WINDOWS
typedef void( __stdcall *CoroutineFunc)(vptr);
#else
typedef void(*CoroutineFunc)(vptr);
#endif

/*
 * Must call before making any other coroutine API.
 * Install a coroutine manager in cuurent thread's
 * local storage (TLS). The input argument is used
 * as userdata for the main coroutine.
 */
vptr XPF_API InitThreadForCoroutines(vptr data);

/*
 * Return the current running coroutine.
 * Can be either main or one of created subcoroutines.
 */
vptr XPF_API GetCurrentCoroutine();

/*
 * Return the userdata associated for
 * the current running coroutine.
 */
vptr XPF_API GetCoroutineData();

/*
 * Create a subcoroutine. Given a 'stackSize' as 0
 * to use the platform default value.
 */
vptr XPF_API CreateCoroutine(u32 stackSize, CoroutineFunc body, vptr data);

/*
 * Manually perform a context switching to the target
 * coroutine. After done, calling coroutine stops
 * running and the target coroutine resumes from
 * either: 1) start of its coroutine body, or 2)
 * the last point it calls SwitchToCoroutine().
 *
 * Self-switching can casue undefined behaviour
 * and hence should be avoided.
 */
void XPF_API SwitchToCoroutine(vptr coroutine);

/*
 * Stop and delete a coroutine.
 * Self-deleting or deleting the main coroutine
 * can terminate current thread.
 */
void XPF_API DeleteCoroutine(vptr coroutine);

}

#endif
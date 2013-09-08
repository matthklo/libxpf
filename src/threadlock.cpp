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

// Currently only 2 threading models are supported: windows, posix.
#ifdef XPF_PLATFORM_WINDOWS
#include "platform/windows/threadlock_windows.hpp"
#else
#include "platform/posix/threadlock_posix.hpp"
#endif

namespace xpf {

ThreadLock::ThreadLock()
{
	pImpl = (vptr) new details::XpfThreadLock;
}

ThreadLock::ThreadLock(const ThreadLock& that)
{
	xpfAssert(false);
}

ThreadLock::~ThreadLock()
{
	details::XpfThreadLock *impl = (details::XpfThreadLock*) pImpl;
	delete impl;
	pImpl = 0xfefefefe;
}

ThreadLock& ThreadLock::operator = (const ThreadLock& that)
{
	xpfAssert(false);
	return *this;
}

void ThreadLock::lock()
{
	details::XpfThreadLock *impl = (details::XpfThreadLock*) pImpl;
	impl->lock();
}

bool ThreadLock::tryLock()
{
	details::XpfThreadLock *impl = (details::XpfThreadLock*) pImpl;
	return impl->tryLock();
}

bool ThreadLock::isLocked() const
{
	details::XpfThreadLock *impl = (details::XpfThreadLock*) pImpl;
	return impl->isLocked();
}

void ThreadLock::unlock()
{
	details::XpfThreadLock *impl = (details::XpfThreadLock*) pImpl;
	impl->unlock();
}

ThreadID ThreadLock::getOwner() const
{
	details::XpfThreadLock *impl = (details::XpfThreadLock*) pImpl;
	return impl->getOwner();
}

};

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

#include <xpf/platform.h>

// Currently only 2 threading models are supported: windows, posix.
#ifdef XPF_PLATFORM_WINDOWS
#include "platform/windows/thread_windows.hpp"
#else
#include "platform/posix/thread_posix.hpp"
#endif

namespace xpf {

void Thread::sleep(u32 durationMs)
{
	details::XpfThread::sleep(durationMs);
}

void Thread::yield()
{
	details::XpfThread::yield();
}

ThreadID Thread::getThreadID()
{
	return details::XpfThread::getThreadID();
}


//=================================================

Thread::Thread()
{
	pImpl = (vptr) new details::XpfThread(this);
}

Thread::Thread(const Thread& that)
{
	xpfAssert( ( "non-copyable", false ) );
}

Thread::~Thread()
{
	details::XpfThread * impl = (details::XpfThread*) pImpl;
	delete impl;
	pImpl = 0xfefefefe;
}

Thread& Thread::operator = (const Thread& that)
{
	xpfAssert( ( "non-copyable", false ) );
	return *this;
}

void Thread::start()
{
	details::XpfThread * impl = (details::XpfThread*) pImpl;
	impl->start();
}

bool Thread::join(u32 timeoutMs)
{
	details::XpfThread * impl = (details::XpfThread*) pImpl;
	return impl->join(timeoutMs);
}

Thread::RunningStatus Thread::getStatus() const
{
	details::XpfThread * impl = (details::XpfThread*) pImpl;
	return impl->getStatus();
}

void Thread::setExitCode(u32 code)
{
	details::XpfThread * impl = (details::XpfThread*) pImpl;
	impl->setExitCode(code);
}

u32 Thread::getExitCode() const
{
	details::XpfThread * impl = (details::XpfThread*) pImpl;
	return impl->getExitCode();
}

u64 Thread::getData() const
{
	details::XpfThread * impl = (details::XpfThread*) pImpl;
	return impl->getData();
}

void Thread::setData(u64 data)
{
	details::XpfThread * impl = (details::XpfThread*) pImpl;
	impl->setData(data);
}

ThreadID Thread::getID() const
{
	details::XpfThread * impl = (details::XpfThread*) pImpl;
	return impl->getID();
}

void Thread::requestAbort()
{
	details::XpfThread * impl = (details::XpfThread*) pImpl;
	impl->requestAbort();
}

bool Thread::isAborting() const
{
	details::XpfThread * impl = (details::XpfThread*) pImpl;
	return impl->isAborting();
}

};


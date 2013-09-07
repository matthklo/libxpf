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

#include <xpf/threadevent.h>

// Currently only 2 threading model are supported: windows, posix.
#ifdef XPF_PLATFORM_WINDOWS
#include "platform/windows/threadevent_windows.hpp"
#else
#include "platform/posix/threadevent_event.hpp"
#endif

namespace xpf {

ThreadEvent::ThreadEvent(bool set)
{
	pImpl = (vptr) new details::XpfThreadEvent(set);
}

ThreadEvent::ThreadEvent(const ThreadEvent& that)
{
	xpfAssert(false); // ThreadEvent is non-copyable
}

ThreadEvent::~ThreadEvent()
{
	details::XpfThreadEvent *impl = (details::XpfThreadEvent*) pImpl;
	delete impl;
	pImpl = 0xfefefefe;
}

ThreadEvent& ThreadEvent::operator = (const ThreadEvent &that)
{
	xpfAssert(false); // ThreadEvent is non-copyable
	return *this;
}

void ThreadEvent::set()
{
	details::XpfThreadEvent *impl = (details::XpfThreadEvent*) pImpl;
	impl->set();
}

void ThreadEvent::reset()
{
	details::XpfThreadEvent *impl = (details::XpfThreadEvent*) pImpl;
	impl->reset();
}

bool ThreadEvent::wait(u32 timeoutMs)
{
	details::XpfThreadEvent *impl = (details::XpfThreadEvent*) pImpl;
	return impl->wait(timeoutMs);
}

bool ThreadEvent::isSet()
{
	details::XpfThreadEvent *impl = (details::XpfThreadEvent*) pImpl;
	return impl->isSet();
}

};

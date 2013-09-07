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

#ifndef XPF_PLATFORM_WINDOWS
#  error threadevent_windows.hpp shall not build on platforms other than Windows.
#endif

#include <Windows.h>

namespace xpf { namespace details {

class XpfThreadEvent
{
public:
	explicit XpfThreadEvent(bool set /* = false */)
	{
		mEvent = ::CreateEvent(NULL, TRUE, (set)? TRUE: FALSE, NULL);
	}

	~XpfThreadEvent()
	{
		::CloseHandle(mEvent);
	}

	inline void XpfThreadEvent::set()
	{
		::SetEvent(mEvent);
	}

	inline void XpfThreadEvent::reset()
	{
		::ResetEvent(mEvent);
	}

	inline bool XpfThreadEvent::wait(u32 timeoutMs /* = -1L */)
	{
		// -1L (0xFFFFFFFF) happens to be the INFINITE on windows.
		return (WAIT_OBJECT_0 == ::WaitForSingleObject(mEvent, timeoutMs));
	}

	inline bool XpfThreadEvent::isSet()
	{
		return wait(0);
	}

private:
	XpfThreadEvent(const XpfThreadEvent& that)
	{
		xpfAssert(false);
	}

	XpfThreadEvent& operator = (const XpfThreadEvent& that)
	{
		xpfAssert(false);
		return *this;
	}

	::HANDLE mEvent;
};

};};
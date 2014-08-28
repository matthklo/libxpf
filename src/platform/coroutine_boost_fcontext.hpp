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

#ifdef _XPF_COROUTINE_IMPL_INCLUDED_
#  error Multiple coroutine implementation files included
#else
#  define _XPF_COROUTINE_IMPL_INCLUDED_
#endif

#include <xpf/fcontext.h>
#include <xpf/coroutine.h>

#if defined(XPF_PLATFORM_CYGWIN)
#  error Coroutine on CYGWIN platform has not yet verified.
#elif defined (XPF_PLATFORM_WINDOWS)
#  if defined (XPF_CPU_X86)
#    if defined (XPF_MODEL_64)
#      include "fcontext_x86_64_win.hpp"
#    else
#      include "fcontext_i386_win.hpp"
#    endif
#  elif defined (XPF_CPU_ARM)
#    include "fcontext_arm_win.hpp"
#  else
#    error Unsupported CPU-Arch on Windows platform.
#  endif
#elif defined (XPF_PLATFORM_APPLE)
#  if defined (XPF_PLATFORM_IOSSIM)
#    include "fcontext_i386.hpp"
#  elif defined (XPF_PLATFORM_IOS)
#    include "fcontext_arm_mac.hpp"
#  elif defined (XPF_MODEL_64)
#    include "fcontext_x86_64.hpp"
#  else
#    include "fcontext_i386.hpp"
#  endif
#else
#  if defined (XPF_CPU_X86)
#    if defined (XPF_MODEL_64)
#      include "fcontext_x86_64.hpp"
#    else
#      include "fcontext_i386.hpp"
#    endif
#  elif defined (XPF_CPU_ARM)
#    include "fcontext_arm.hpp"
#  else
#    error Unsupported CPU-Arch on UNIX platform.
#  endif
#endif

namespace xpf
{

vptr XPF_API InitThreadForCoroutines(vptr data)
{
	return 0;
}

vptr XPF_API GetCurrentCoroutine()
{
	return 0;
}

vptr XPF_API GetCoroutineData()
{
	return 0;
}

vptr XPF_API CreateCoroutine(u32 stackSize, CoroutineFunc body, vptr data)
{
	return 0;
}

void XPF_API SwitchToCoroutine(vptr coroutine)
{
	// dummy invoke
	jump_fcontext(0,0,0);
	make_fcontext(0,0,0);
}

void XPF_API DeleteCoroutine(vptr coroutine)
{
}

};
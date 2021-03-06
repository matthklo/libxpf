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
#error Multiple coroutine implementation files included
#else
#define _XPF_COROUTINE_IMPL_INCLUDED_
#endif

#ifndef XPF_PLATFORM_WINDOWS
#  error coroutine_windows.hpp shall not build on platforms other than Windows.
#endif

#include <Windows.h>
#include <xpf/coroutine.h>

namespace xpf
{

vptr XPF_API InitThreadForCoroutines(vptr data)
{
	return (vptr)::ConvertThreadToFiber((LPVOID)data);
}

vptr XPF_API GetCurrentCoroutine()
{
	return (vptr)::GetCurrentFiber();
}

vptr XPF_API GetCoroutineData()
{
	return (vptr)::GetFiberData();
}

vptr XPF_API CreateCoroutine(u32 stackSize, CoroutineFunc body, vptr data)
{
	return (vptr)::CreateFiber((SIZE_T)stackSize, (void( __stdcall *)(void*))body, (LPVOID)data);
}

void XPF_API SwitchToCoroutine(vptr coroutine)
{
	::SwitchToFiber((LPVOID)coroutine);
}

void XPF_API DeleteCoroutine(vptr coroutine)
{
	::DeleteFiber((LPVOID)coroutine);
}

};

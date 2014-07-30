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

#ifdef _XPF_TLS_IMPL_INCLUDED_
#error Multiple TLS implementation files included
#else
#define _XPF_TLS_IMPL_INCLUDED_
#endif

#ifndef XPF_PLATFORM_WINDOWS
#  error tls_windows.hpp shall not build on platforms other than Windows.
#endif

#include <Windows.h>

namespace xpf {

struct TlsIndex
{
	DWORD Index;
};

vptr XPF_API
TlsCreate()
{
	DWORD idx = TlsAlloc();
	if (idx == TLS_OUT_OF_INDEXES)
	{
		DWORD errorcode = GetLastError();
		xpfAssert(("Failed on TlsAlloc()", false));
		return 0;
	}

	TlsIndex *ret = new TlsIndex;
	ret->Index = idx;
	return (vptr)ret;
}

void XPF_API
TlsDelete(vptr index)
{
	xpfAssert(("Operate TlsDelete() with a null index.", (index != 0)));
	if (index == 0)
		return;
	
	TlsIndex *idx = (TlsIndex*)index;
	if (TlsFree(idx->Index) == 0)
	{
		DWORD errorcode = GetLastError();
		xpfAssert(("Failed on TlsFree()", false));
	}
	delete idx;
}

vptr XPF_API
TlsGet(vptr index)
{
	xpfAssert(("Operate TlsGet() with a null input.", (index != 0)));
	if (index == 0)
		return 0;

	TlsIndex *idx = (TlsIndex*)index;
	LPVOID data = TlsGetValue(idx->Index);
	if (data == 0)
	{
		DWORD errorcode = GetLastError();
		xpfAssert(("Failed on TlsGetValue()", (errorcode == ERROR_SUCCESS)));
	}
	return (vptr)data;
}

void XPF_API
TlsSet(vptr index, vptr data)
{
	xpfAssert(("Operate TlsSet() with a null input.", (index != 0)));
	if (index == 0)
		return;

	TlsIndex *idx = (TlsIndex*)index;
	if (TlsSetValue(idx->Index, (LPVOID)data) == 0)
	{
		DWORD errorcode = GetLastError();
		xpfAssert(("Failed on TlsSetValue()", false));
	}
}

};

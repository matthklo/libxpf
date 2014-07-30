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

#include <pthread.h>

namespace xpf {

struct TlsIndex
{
	pthread_key_t Index;
};

vptr
TlsCreate()
{
	pthread_key_t idx;
	if (0 != pthread_key_create(&idx, 0))
	{
		xpfAssert(("Failed on pthread_key_create().", false));
		return 0;
	}

	TlsIndex *index = new TlsIndex;
	index->Index = idx;
	return (vptr)index;
}

void
TlsDelete(vptr index)
{
	xpfAssert(("Operate TlsDelete() with a null index.", (index != 0)));
	if (index == 0)
		return;

	TlsIndex *idx = (TlsIndex*)index;
	if (0 != pthread_key_delete(idx->Index))
	{
		xpfAssert(("Failed on pthread_key_delete().", false));
		return;
	}
	delete idx;
}

vptr
TlsGet(vptr index)
{
	xpfAssert(("Operate TlsGet() with a null index.", (index != 0)));
	if (index == 0)
		return 0;

	TlsIndex *idx = (TlsIndex*)index;
	return (vptr)pthread_getspecific(idx->Index);
}

void
TlsSet(vptr index, vptr data)
{
	xpfAssert(("Operate TlsSet() with a null index.", (index != 0)));
	if (index == 0)
		return;

	TlsIndex *idx = (TlsIndex*)index;
	if (0 != pthread_setspecific(idx->Index, (void*)data))
	{
		xpfAssert(("Failed on pthread_setspecific().", false));
	}
}

};
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

#include <xpf/coroutine.h>
#include <iostream>

#ifdef XPF_PLATFORM_WINDOWS
// http://msdn.microsoft.com/en-us/library/vstudio/x98tx3cf.aspx
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

using namespace xpf;

vptr _g_mainroutine;

struct CustomData
{
	int id;
	bool done;
	vptr routine;
};

#ifdef XPF_PLATFORM_WINDOWS
void __stdcall coroutine_body(vptr data)
#else
void coroutine_body(vptr data)
#endif
{
	CustomData *d = (CustomData*)data;
	std::cout << "Routine: " << d->id << ", step  1" << std::endl;
	xpfAssert(data == GetCoroutineData());
	xpfAssert(GetCurrentCoroutine() == d->routine);
	SwitchToCoroutine(_g_mainroutine);
	std::cout << "Routine: " << d->id << ", step  2" << std::endl;
	xpfAssert(data == GetCoroutineData());
	xpfAssert(GetCurrentCoroutine() == d->routine);
	SwitchToCoroutine(_g_mainroutine);
	std::cout << "Routine: " << d->id << ", step  3" << std::endl;
	xpfAssert(data == GetCoroutineData());
	xpfAssert(GetCurrentCoroutine() == d->routine);
	SwitchToCoroutine(_g_mainroutine);
	std::cout << "Routine: " << d->id << ", step  4" << std::endl;
	xpfAssert(data == GetCoroutineData());
	xpfAssert(GetCurrentCoroutine() == d->routine);
	SwitchToCoroutine(_g_mainroutine);
	std::cout << "Routine: " << d->id << ", step  5" << std::endl;
	xpfAssert(data == GetCoroutineData());
	xpfAssert(GetCurrentCoroutine() == d->routine);
	//if (d->id == 5) DeleteCoroutine(d->routine);
	SwitchToCoroutine(_g_mainroutine);
	std::cout << "Routine: " << d->id << ", step  6" << std::endl;
	xpfAssert(data == GetCoroutineData());
	xpfAssert(GetCurrentCoroutine() == d->routine);
	SwitchToCoroutine(_g_mainroutine);
	std::cout << "Routine: " << d->id << ", step  7" << std::endl;
	xpfAssert(data == GetCoroutineData());
	xpfAssert(GetCurrentCoroutine() == d->routine);
	SwitchToCoroutine(_g_mainroutine);
	std::cout << "Routine: " << d->id << ", step  8" << std::endl;
	xpfAssert(data == GetCoroutineData());
	xpfAssert(GetCurrentCoroutine() == d->routine);
	SwitchToCoroutine(_g_mainroutine);
	std::cout << "Routine: " << d->id << ", step  9" << std::endl;
	xpfAssert(data == GetCoroutineData());
	xpfAssert(GetCurrentCoroutine() == d->routine);
	SwitchToCoroutine(_g_mainroutine);
	std::cout << "Routine: " << d->id << ", step 10" << std::endl;
	xpfAssert(data == GetCoroutineData());
	xpfAssert(GetCurrentCoroutine() == d->routine);
	d->done = true;
	SwitchToCoroutine(_g_mainroutine);

	//debug assert: should never reach here.
	xpfAssert(("SHOULD NOT REACH HERE.", false));
}

#define NUM_ROUTINES (10)
int main(int argc, char *argv[])
{
#ifdef XPF_PLATFORM_WINDOWS
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	CustomData d[NUM_ROUTINES];
	for (int i = 0; i<NUM_ROUTINES; ++i)
	{
		d[i].id = i + 1;
		d[i].done = false;
		d[i].routine = 0;
	}

	_g_mainroutine = InitThreadForCoroutines(0);
	xpfAssert(_g_mainroutine != 0);

	for (int i = 0; i<NUM_ROUTINES; ++i)
	{
		d[i].routine = CreateCoroutine(0, coroutine_body, (vptr)&d[i]);
		xpfAssert(d[i].routine != 0);
	}

	while (true)
	{
		bool alldone = true;
		for (int i = 0; i<NUM_ROUTINES; ++i)
		{
			if (d[i].done)
				continue;
			alldone = false;
			SwitchToCoroutine(d[i].routine);
		}
		if (alldone)
			break;
	}

	for (int i = 0; i<NUM_ROUTINES; ++i)
	{
		DeleteCoroutine(d[i].routine);
	}

	//if (_g_tls)
	//	pthread_key_delete(_g_tls);
	return 0;
}

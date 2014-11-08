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

/*
*  http://en.wikipedia.org/wiki/Setcontext
*  Context-manipulating functions were introduced in POSIX.1-2001 standard
*  but later marked as obsoleted and removed in POSIX.1-2004, POSIX.1-2008.
*  However, there are no other way to implement fibers/coroutines/cooperative
*  threads without them. So most UN*X platforms still support these functions.
*/

// MacOSX/iOS requires _XOPEN_SOURCE to be declared.
#ifdef __APPLE__
#define _XOPEN_SOURCE
#endif

#include <pthread.h>
#include <ucontext.h>
#include <map>
#include <utility>
#include <xpf/coroutine.h>

static bool _g_tls_inited = false;
static pthread_key_t _g_tls;

#define ENSURE_TLS_INIT \
	if (!_g_tls_inited) {\
		if (0 != pthread_key_create(&_g_tls, 0)) {\
			xpfAssert(("Failed on pthread_key_create()", false)); \
		}\
		_g_tls_inited = true; \
	}

#define FCSTKSZ (1048576)

namespace xpf
{

struct CoroutineSlot
{
	ucontext_t Context;
	void*      Data;
	void*      StackBuffer;
	size_t     StackSize;
};

struct CoroutineManager
{
	ucontext_t* MainContext;
	ucontext_t* CurrentContext;
	std::map<ucontext_t*, CoroutineSlot*> Slots;
};

vptr XPF_API InitThreadForCoroutines(vptr data)
{
	ENSURE_TLS_INIT;

	void* p = pthread_getspecific(_g_tls);
	if (p)
	{
		// has been inited.
		CoroutineManager *mgr = (CoroutineManager*)p;
		return (vptr)mgr->MainContext;
	}

	CoroutineSlot *slot = new CoroutineSlot;
	int ec = getcontext(&slot->Context);
	if (ec != 0)
	{
		xpfAssert(("Failed on getcontext().", false));
		delete slot;
		return 0;
	}
	slot->Data = (void*)data;
	slot->StackBuffer = 0;
	slot->StackSize = 0;

	CoroutineManager *mgr = new CoroutineManager;
	mgr->MainContext = mgr->CurrentContext = &slot->Context;
	mgr->Slots.insert(std::make_pair(&slot->Context, slot));

	if (0 != pthread_setspecific(_g_tls, mgr))
	{
		xpfAssert(("Failed on pthread_setspecific().", false));
		delete slot;
		delete mgr;
		return 0;
	}
	return (vptr)mgr->MainContext;
}

vptr XPF_API GetCurrentCoroutine()
{
	ENSURE_TLS_INIT;

	void* p = pthread_getspecific(_g_tls);
	xpfAssert(("Calling coroutine functions from a normal thread.", (p != 0)));

	if (p)
	{
		CoroutineManager *mgr = (CoroutineManager*)p;
		return (vptr)mgr->CurrentContext;
	}

	return 0;
}

vptr XPF_API GetCoroutineData()
{
	ENSURE_TLS_INIT;

	void* p = pthread_getspecific(_g_tls);
	xpfAssert(("Calling coroutine functions from a normal thread.", (p != 0)));

	if (p)
	{
		CoroutineManager *mgr = (CoroutineManager*)p;
		std::map<ucontext_t*, CoroutineSlot*>::iterator it = mgr->Slots.find(mgr->CurrentContext);
		xpfAssert(("No matching slot for current context.", it != mgr->Slots.end()));
		return (it != mgr->Slots.end()) ? (vptr)it->second->Data : 0;
	}

	return 0;
}

vptr XPF_API CreateCoroutine(u32 stackSize, CoroutineFunc body, vptr data)
{
	ENSURE_TLS_INIT;

	void* p = pthread_getspecific(_g_tls);
	xpfAssert(("Calling coroutine functions from a normal thread.", (p != 0)));

	if ((p == 0) || (body == 0))
		return 0;

	CoroutineManager *mgr = (CoroutineManager*)p;
	CoroutineSlot *slot = new CoroutineSlot;
	if (0 != getcontext(&slot->Context))
	{
		xpfAssert(("Failed on getcontext().", false));
		delete slot;
		return 0;
	}
	slot->Data = (void*)data;
	// If stackSize is 0, use the default size FCSTKSZ.
	slot->StackSize = (stackSize) ? stackSize : FCSTKSZ;
	slot->StackBuffer = new char[slot->StackSize];

	slot->Context.uc_stack.ss_sp = (char*) slot->StackBuffer;
	slot->Context.uc_stack.ss_size = slot->StackSize;
	slot->Context.uc_link = 0;

	mgr->Slots.insert(std::make_pair(&slot->Context, slot));
	// NOTE: Passing 64-bits pointer can causing porting issue.
	//       However, if the pointer is at the end of argument list, it should be fine.
	makecontext(&slot->Context, (void(*)())(body), 1, slot->Data);

	return (vptr)&slot->Context;
}

void XPF_API SwitchToCoroutine(vptr coroutine)
{
	ENSURE_TLS_INIT;

	void* p = pthread_getspecific(_g_tls);
	xpfAssert(("Calling coroutine functions from a normal thread.", (p != 0)));

	if ((p == 0) || (coroutine == 0) || (coroutine == GetCurrentCoroutine()))
		return;

	CoroutineManager *mgr = (CoroutineManager*)p;

	// Ensure the target coroutine has been registered.
	if (mgr->Slots.find((ucontext_t*)coroutine) == mgr->Slots.end())
	{
		xpfAssert(("Switching to an un-recognized coroutine.", false));
		return;
	}

	ucontext_t *curctx = mgr->CurrentContext;
	mgr->CurrentContext = (ucontext_t*)coroutine;
	swapcontext(curctx, (ucontext_t*)coroutine);

	// debug aserrt
	xpfAssert(("Current context check after swapcontext() returns.", (mgr->CurrentContext == curctx)));
}

void XPF_API DeleteCoroutine(vptr coroutine)
{
	ENSURE_TLS_INIT;

	void* p = pthread_getspecific(_g_tls);
	xpfAssert(("Calling coroutine functions from a normal thread.", (p != 0)));

	if ((coroutine == 0) || (p == 0))
		return;

	CoroutineManager *mgr = (CoroutineManager*)p;
	if ((coroutine != (vptr)mgr->CurrentContext) && (coroutine != (vptr)mgr->MainContext))
	{
		std::map<ucontext_t*, CoroutineSlot*>::iterator it =
			mgr->Slots.find((ucontext_t*)coroutine);
		if (it != mgr->Slots.end())
		{
			CoroutineSlot *slot = it->second;
			delete[](char*)slot->StackBuffer;
			delete slot;
			mgr->Slots.erase(it);
		}
	}
	else
	{
		// Full shutdown. Release everything and terminate current thread.
		CoroutineSlot *currentSlot = 0;
		for (std::map<ucontext_t*, CoroutineSlot*>::iterator it = mgr->Slots.begin();
			it != mgr->Slots.end(); ++it)
		{
			// Delay the destuction of current context to the last.
			if (it->first == mgr->CurrentContext)
			{
				currentSlot = it->second;
				continue;
			}
			delete[](char*)it->second->StackBuffer;
			delete it->second;
		}
		xpfAssert(("Unable to locate current context.", (currentSlot != 0)));
		mgr->Slots.clear();

		delete mgr; mgr = 0;
		pthread_setspecific(_g_tls, 0);

		// TODO: Does deleting current context causing any problem?
		char *sb = (char*)currentSlot->StackBuffer;
		delete currentSlot;
		delete[] sb;

		pthread_exit(0);
	}
}

};

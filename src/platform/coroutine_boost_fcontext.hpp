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

/*
 * The plain data structure of context object which 
 * the opaque fcontext_t pointer points to.
 */
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

#include <pthread.h>
#include <map>
#include <utility>

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
	fcontext_t Context;
	void*      Data;
	void*      StackBuffer;
	vptr       StackSize;
};

struct CoroutineManager
{
	void* MainContext;
	void* CurrentContext;
	std::map<void*, CoroutineSlot*> Slots; // stack head as key, map to slot.
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
	slot->Context = 0; // not yet known until first jump_fcontext.
	slot->Data = (void*)data;
	slot->StackBuffer = (void*)0x42; // magic number for the main coroutine.
	slot->StackSize = 0;

	CoroutineManager *mgr = new CoroutineManager;
	mgr->MainContext = mgr->CurrentContext = slot->StackBuffer;
	mgr->Slots.insert(std::make_pair(slot->StackBuffer, slot));

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
		std::map<void*, CoroutineSlot*>::iterator it = mgr->Slots.find(mgr->CurrentContext);
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
	slot->Data = (void*)data;
	// If stackSize is 0, use the default size FCSTKSZ.
	slot->StackSize = (stackSize) ? stackSize : FCSTKSZ;
	char *stkbuf = new char[slot->StackSize];
	slot->StackBuffer = (void*)stkbuf;
	slot->Context = make_fcontext(&stkbuf[slot->StackSize], slot->StackSize, body);

	mgr->Slots.insert(std::make_pair(slot->StackBuffer, slot));

	return (vptr)slot->StackBuffer;
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
	std::map<void*, CoroutineSlot*>::iterator tit = mgr->Slots.find((void*)coroutine);
	if (tit == mgr->Slots.end())
	{
		xpfAssert(("Switching to an un-recognized coroutine.", false));
		return;
	}

	// Locate the record of current coroutine
	std::map<void*, CoroutineSlot*>::iterator sit = mgr->Slots.find((void*)GetCurrentCoroutine());
	if (sit == mgr->Slots.end())
	{
		xpfAssert(("Switching from an un-recognized coroutine.", false));
		return;
	}

	mgr->CurrentContext = (void*)coroutine;
	jump_fcontext(&sit->second->Context, tit->second->Context, (vptr)tit->second->Data);

	// debug aserrt
	xpfAssert(("Current context check after swapcontext() returns.", (mgr->CurrentContext == sit->second->StackBuffer)));
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
	{	// Deleting a non-current and non-main coroutine.
		std::map<void*, CoroutineSlot*>::iterator it =
			mgr->Slots.find((void*)coroutine);
		if (it != mgr->Slots.end())
		{
			CoroutineSlot *slot = it->second;
			delete[](char*)slot->StackBuffer;
			delete slot;
			mgr->Slots.erase(it);
		}
	}
	else
	{	// Deleting on either the current or the main coroutine. Perform a
		// full shutdown: Release everything and terminate current thread.
		CoroutineSlot *currentSlot = 0;
		for (std::map<void*, CoroutineSlot*>::iterator it = mgr->Slots.begin();
			it != mgr->Slots.end(); ++it)
		{
			// Delay the destuction of current context to the last.
			if (it->first == mgr->CurrentContext)
			{
				currentSlot = it->second;
				continue;
			}
			char *sb = (char*)it->second->StackBuffer;
			delete it->second;
			if (0x42 != (vptr)sb)
				delete[] sb;
		}
		xpfAssert(("Unable to locate current context.", (currentSlot != 0)));
		mgr->Slots.clear();

		delete mgr; mgr = 0;
		pthread_setspecific(_g_tls, 0);

		// TODO: Does deleting current context causing any problem?
		char *sb = (char*)currentSlot->StackBuffer;
		delete currentSlot;
		if (0x42 != (vptr)sb)
			delete[] sb;

		pthread_exit(0);
	}
}

};

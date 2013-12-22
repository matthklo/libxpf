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

#include <xpf/atomic.h>

using namespace xpf;

#if defined(XPF_PLATFORM_WINDOWS) && defined(XPF_COMPILER_MSVC)
// http://msdn.microsoft.com/en-us/library/windows/desktop/ms686360(v=vs.85).aspx
// "Interlocked Functions"

#include <Windows.h>
#include <intrin.h>

// Atomic Add: Add the value pointed by 'val' with d and save the result
//             back to 'val'. Returns the original value in 'val'

s32 xpfAtomicAddS32(volatile s32 *val, s32 d)
{
	return (s32) InterlockedExchangeAdd((LONG volatile *)val, (LONG)d);
}

u32 xpfAtomicAddU32(volatile u32 *val, u32 d)
{
	return (u32) InterlockedExchangeAdd((unsigned int volatile *)val, (unsigned int)d);
}

s64 xpfAtomicAddS64(volatile s64 *val, s64 d)
{
	return (s64) InterlockedExchangeAdd64((LONGLONG volatile *)val, (LONGLONG)d);
}

u64 xpfAtomicAddU64(volatile u64 *val, u64 d)
{
	// No idea why the unsigned 64-bits version is named 'InterlockedExchangeAdd' 
	// instead of 'InterlockedExchangeAdd64'
	return (u64) InterlockedExchangeAdd((unsigned __int64 volatile *)val, (unsigned __int64)d);
}

// Atomic AND: Perform logical AND on the value pointed by 'val' with d and save the result back to 'val'.
//             Returns the value before operation.
c8  xpfAtomicAndS8 (volatile c8  *val, c8  d)
{
	return (c8) _InterlockedAnd8((char volatile*)val, (char)d);
}

s16 xpfAtomicAndS16(volatile s16 *val, s16 d)
{
	return (s16) _InterlockedAnd16((short volatile*)val, (short)d);
}

s32 xpfAtomicAndS32(volatile s32 *val, s32 d)
{
	// On Windows platform, long is always 32-bits even on a x64 host.
	return (s32) _InterlockedAnd((long volatile*)val, (long)d);
}


// Atomic OR: Perform logical OR on the value pointed by 'val' with d and save the result back to 'val'.
//             Returns the value before operation.
c8  xpfAtomicOrS8 (volatile c8  *val, c8  d)
{
	return (c8) _InterlockedOr8((char volatile*)val, (char)d);
}

s16 xpfAtomicOrS16(volatile s16 *val, s16 d)
{
	return (s16) _InterlockedOr16((short volatile*)val, (short)d);
}

s32 xpfAtomicOrS32(volatile s32 *val, s32 d)
{
	// On Windows platform, long is always 32-bits even on a x64 host.
	return (s32) _InterlockedOr((long volatile*)val, (long)d);
}


// Atomic XOR: Perform logical XOR on the value pointed by 'val' with d and save the result back to 'val'.
//             Returns the value before operation.
c8  xpfAtomicXorS8 (volatile c8  *val, c8  d)
{
	return (c8) _InterlockedXor8((char volatile*)val, (char)d);
}

s16 xpfAtomicXorS16(volatile s16 *val, s16 d)
{
	return (s16) _InterlockedXor16((short volatile*)val, (short)d);
}

s32 xpfAtomicXorS32(volatile s32 *val, s32 d)
{
	// On Windows platform, long is always 32-bits even on a x64 host.
	return (s32) _InterlockedXor((long volatile*)val, (long)d);
}

// Atomic Compare-and-swap operation: Compare the value pointed by 'val' to oldValue.
//                                    If equal, swap the value to be newVlaue.
//                                    Return the initial value store in 'val' BEFORE the operation.
s16 xpfAtomicCASS16(volatile s16 *val, s16 newValue, s16 oldValue)
{
	return (s16) InterlockedCompareExchange16((short volatile *)val, (short) newValue, (short) oldValue);
}

s32 xpfAtomicCASS32(volatile s32 *val, s32 newValue, s32 oldValue)
{
	return (s32) InterlockedCompareExchange((LONG volatile *)val, (LONG) newValue, (LONG) oldValue);
}

s64 xpfAtomicCASS64(volatile s64 *val, s64 newValue, s64 oldValue)
{
	return (s64) InterlockedCompareExchange64((LONGLONG volatile *)val, (LONGLONG) newValue, (LONGLONG) oldValue);
}


#elif defined(XPF_COMPILER_GNUC)

// http://gcc.gnu.org/onlinedocs/gcc-4.1.1/gcc/Atomic-Builtins.html
// "atomic memory access"

// Atomic Add: Add the value pointed by 'val' with d and save the result
//             back to 'val'. Returns the original value in 'val'
s32 xpfAtomicAddS32(volatile s32 *val, s32 d)
{
	return (s32) __sync_fetch_and_add(val, d);
}

u32 xpfAtomicAddU32(volatile u32 *val, u32 d)
{
	return (u32) __sync_fetch_and_add(val, d);
}

s64 xpfAtomicAddS64(volatile s64 *val, s64 d)
{
	return (s64) __sync_fetch_and_add(val, d);
}

u64 xpfAtomicAddU64(volatile u64 *val, u64 d)
{
	return (u64) __sync_fetch_and_add(val, d);
}

// Atomic AND: Perform logical AND on the value pointed by 'val' with d and save the result back to 'val'.
//             Returns the value before operation.
c8  xpfAtomicAndS8 (volatile c8  *val, c8  d)
{
	return (c8) __sync_fetch_and_and(val, d);
}

s16 xpfAtomicAndS16(volatile s16 *val, s16 d)
{
	return (s16) __sync_fetch_and_and(val, d);
}

s32 xpfAtomicAndS32(volatile s32 *val, s32 d)
{
	return (s32) __sync_fetch_and_and(val, d);
}


// Atomic OR: Perform logical OR on the value pointed by 'val' with d and save the result back to 'val'.
//             Returns the value before operation.
c8  xpfAtomicOrS8 (volatile c8  *val, c8  d)
{
	return (c8) __sync_fetch_and_or(val, d);
}

s16 xpfAtomicOrS16(volatile s16 *val, s16 d)
{
	return (s16) __sync_fetch_and_or(val, d);
}

s32 xpfAtomicOrS32(volatile s32 *val, s32 d)
{
	return (s32) __sync_fetch_and_or(val, d);
}


// Atomic XOR: Perform logical XOR on the value pointed by 'val' with d and save the result back to 'val'.
//             Returns the value before operation.
c8  xpfAtomicXorS8 (volatile c8  *val, c8  d)
{
	return (c8) __sync_fetch_and_xor(val, d);
}

s16 xpfAtomicXorS16(volatile s16 *val, s16 d)
{
	return (s16) __sync_fetch_and_xor(val, d);
}

s32 xpfAtomicXorS32(volatile s32 *val, s32 d)
{
	return (s32) __sync_fetch_and_xor(val, d);
}

// Atomic Compare-and-swap operation: Compare the value pointed by 'val' to oldValue.
//                                    If equal, swap the value to be newVlaue.
//                                    Return the initial value store in 'val' BEFORE the operation.
s16 xpfAtomicCASS16(volatile s16 *val, s16 newValue, s16 oldValue)
{
	return (s16) __sync_val_compare_and_swap(val, oldValue, newValue);
}

s32 xpfAtomicCASS32(volatile s32 *val, s32 newValue, s32 oldValue)
{
	return (s32) __sync_val_compare_and_swap(val, oldValue, newValue);
}

s64 xpfAtomicCASS64(volatile s64 *val, s64 newValue, s64 oldValue)
{
	return (s64) __sync_val_compare_and_swap(val, oldValue, newValue);
}


#else

#error "Need an atomic operations impl."

#endif


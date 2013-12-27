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

#ifndef _XPF_ATOMIC_HEADER_
#define _XPF_ATOMIC_HEADER_

#include "platform.h"

#ifdef XPF_COMPILER_MSVC
#  if _MSC_VER < 1400
#    error Atmoic intrinsics only work on MSVC 2005 or later version.
#  endif

#  define XPF_HAVE_ATOMIC_OPERATIONS
#  include <intrin.h>

// Atomic ADD: Returns the value before adding.
//  Ex:
//     xpf::s32 val, addend = 10;
//     xpf::s32 old = xpfAtomicAdd(&val, addend);
#define xpfAtomicAdd(_Val, _Add)   _InterlockedExchangeAdd((volatile long*)_Val, _Add)
#define xpfAtomicAdd64(_Val, _Add) _InterlockedExchangeAdd64((volatile __int64*)_Val, _Add)


// Atomic OR: Returns the value before OR.
//  Ex:
//     xpf::s32 val, mask = 0xf0f0f0f0;
//     xpf::s32 old = xpfAtomicOr(&val, mask);
#define xpfAtomicOr(_Val, _Add)   _InterlockedOr((volatile long*)_Val, _Add)
#define xpfAtomicOr8(_Val, _Add)  _InterlockedOr8((volatile char*)_Val, _Add)
#define xpfAtomicOr16(_Val, _Add) _InterlockedOr16((volatile short*)_Val, _Add)
#define xpfAtomicOr64(_Val, _Add) _InterlockedOr64((volatile __int64*)_Val, _Add)


// Atomic AND: Returns the value before AND.
//  Ex:
//     xpf::s32 val, mask = 0xf0f0f0f0;
//     xpf::s32 old = xpfAtomicAnd(&val, mask);
#define xpfAtomicAnd(_Val, _Add)   _InterlockedAnd((volatile long*)_Val, _Add)
#define xpfAtomicAnd8(_Val, _Add)  _InterlockedAnd8((volatile char*)_Val, _Add)
#define xpfAtomicAnd16(_Val, _Add) _InterlockedAnd16((volatile short*)_Val, _Add)
#define xpfAtomicAnd64(_Val, _Add) _InterlockedAnd64((volatile __int64*)_Val, _Add)


// Atomic XOR: Returns the value before XOR.
//  Ex:
//     xpf::s32 val, mask = 0xf0f0f0f0;
//     xpf::s32 old = xpfAtomicXor(&val, mask);
#define xpfAtomicXor(_Val, _Add)   _InterlockedXor((volatile long*)_Val, _Add)
#define xpfAtomicXor8(_Val, _Add)  _InterlockedXor8((volatile char*)_Val, _Add)
#define xpfAtomicXor16(_Val, _Add) _InterlockedXor16((volatile short*)_Val, _Add)
#define xpfAtomicXor64(_Val, _Add) _InterlockedXor64((volatile __int64*)_Val, _Add)


// Atomic Compare-and-swap operation (CAS): Compare the value pointed by '_dest' to '_comperand'.
//                                          If equal, swap the value to be '_exchange'.
//                                          Return the initial value store in '_dest' BEFORE the operation.
#define xpfAtomicCAS(_dest, _comperand, _exchange)   _InterlockedCompareExchange((volatile long*)_dest, _exchange, _comperand)
#define xpfAtomicCAS16(_dest, _comperand, _exchange) _InterlockedCompareExchange16((volatile short*)_dest, _exchange, _comperand)
#define xpfAtomicCAS64(_dest, _comperand, _exchange) _InterlockedCompareExchange64((volatile __int64*)_dest, _exchange, _comperand)


#elif defined(XPF_COMPILER_GNUC)

#  define XPF_HAVE_ATOMIC_OPERATIONS

// Atomic ADD: Returns the value before adding.
//  Ex:
//     xpf::s32 val, addend = 10;
//     xpf::s32 old = xpfAtomicAdd(&val, addend);
#define xpfAtomicAdd   __sync_fetch_and_add
#define xpfAtomicAdd64 __sync_fetch_and_add


// Atomic OR: Returns the value before OR.
//  Ex:
//     xpf::s32 val, mask = 0xf0f0f0f0;
//     xpf::s32 old = xpfAtomicOr(&val, mask);
#define xpfAtomicOr   __sync_fetch_and_or
#define xpfAtomicOr8  __sync_fetch_and_or
#define xpfAtomicOr16 __sync_fetch_and_or
#define xpfAtomicOr64 __sync_fetch_and_or

// Atomic AND: Returns the value before AND.
//  Ex:
//     xpf::s32 val, mask = 0xf0f0f0f0;
//     xpf::s32 old = xpfAtomicAnd(&val, mask);
#define xpfAtomicAnd   __sync_fetch_and_and
#define xpfAtomicAnd8  __sync_fetch_and_and
#define xpfAtomicAnd16 __sync_fetch_and_and
#define xpfAtomicAnd64 __sync_fetch_and_and


// Atomic XOR: Returns the value before XOR.
//  Ex:
//     xpf::s32 val, mask = 0xf0f0f0f0;
//     xpf::s32 old = xpfAtomicXor(&val, mask);
#define xpfAtomicXor   __sync_fetch_and_xor
#define xpfAtomicXor8  __sync_fetch_and_xor
#define xpfAtomicXor16 __sync_fetch_and_xor
#define xpfAtomicXor64 __sync_fetch_and_xor


// Atomic Compare-and-swap operation (CAS): Compare the value pointed by '_dest' to '_comperand'.
//                                          If equal, swap the value to be '_exchange'.
//                                          Return the initial value store in '_dest' BEFORE the operation.
#define xpfAtomicCAS(_dest, _comperand, _exchange)   __sync_val_compare_and_swap(_dest, _comperand, _exchange)
#define xpfAtomicCAS16(_dest, _comperand, _exchange) __sync_val_compare_and_swap(_dest, _comperand, _exchange)
#define xpfAtomicCAS64(_dest, _comperand, _exchange) __sync_val_compare_and_swap(_dest, _comperand, _exchange)


#else
# error Atomic operations have not yet implemented for your compiler.
#endif

#endif // _XPF_ATOMIC_HEADER_
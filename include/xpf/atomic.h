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
#define xpfAtomicAdd(_Ptr, _Add)   _InterlockedExchangeAdd((volatile long*)_Ptr, _Add)
#define xpfAtomicAdd64(_Ptr, _Add) _InterlockedExchangeAdd64((volatile __int64*)_Ptr, _Add)


// Atomic OR: Returns the value before OR.
//  Ex:
//     xpf::s32 val, mask = 0xf0f0f0f0;
//     xpf::s32 old = xpfAtomicOr(&val, mask);
#define xpfAtomicOr(_Ptr, _Mask)   _InterlockedOr((volatile long*)_Ptr, _Mask)
#define xpfAtomicOr8(_Ptr, _Mask)  _InterlockedOr8((volatile char*)_Ptr, _Mask)
#define xpfAtomicOr16(_Ptr, _Mask) _InterlockedOr16((volatile short*)_Ptr, _Mask)
#define xpfAtomicOr64(_Ptr, _Mask) _InterlockedOr64((volatile __int64*)_Ptr, _Mask)


// Atomic AND: Returns the value before AND.
//  Ex:
//     xpf::s32 val, mask = 0xf0f0f0f0;
//     xpf::s32 old = xpfAtomicAnd(&val, mask);
#define xpfAtomicAnd(_Ptr, _Mask)   _InterlockedAnd((volatile long*)_Ptr, _Mask)
#define xpfAtomicAnd8(_Ptr, _Mask)  _InterlockedAnd8((volatile char*)_Ptr, _Mask)
#define xpfAtomicAnd16(_Ptr, _Mask) _InterlockedAnd16((volatile short*)_Ptr, _Mask)
#define xpfAtomicAnd64(_Ptr, _Mask) _InterlockedAnd64((volatile __int64*)_Ptr, _Mask)


// Atomic XOR: Returns the value before XOR.
//  Ex:
//     xpf::s32 val, mask = 0xf0f0f0f0;
//     xpf::s32 old = xpfAtomicXor(&val, mask);
#define xpfAtomicXor(_Ptr, _Mask)   _InterlockedXor((volatile long*)_Ptr, _Mask)
#define xpfAtomicXor8(_Ptr, _Mask)  _InterlockedXor8((volatile char*)_Ptr, _Mask)
#define xpfAtomicXor16(_Ptr, _Mask) _InterlockedXor16((volatile short*)_Ptr, _Mask)
#define xpfAtomicXor64(_Ptr, _Mask) _InterlockedXor64((volatile __int64*)_Ptr, _Mask)


// Atomic Compare-and-swap operation (CAS): Compare the value pointed by '_Dest' to '_Comperand'.
//                                          If equal, swap the value to be '_Exchange'.
//                                          Return the initial value store in '_Dest' BEFORE the operation.
#define xpfAtomicCAS(_Dest, _Comperand, _Exchange)   _InterlockedCompareExchange((volatile long*)_Dest, _Exchange, _Comperand)
#define xpfAtomicCAS16(_Dest, _Comperand, _Exchange) _InterlockedCompareExchange16((volatile short*)_Dest, _Exchange, _Comperand)
#define xpfAtomicCAS64(_Dest, _Comperand, _Exchange) _InterlockedCompareExchange64((volatile __int64*)_Dest, _Exchange, _Comperand)


#elif defined(XPF_COMPILER_GNUC)

#  define XPF_HAVE_ATOMIC_OPERATIONS

// Atomic ADD: Returns the value before adding.
//  Ex:
//     xpf::s32 val, addend = 10;
//     xpf::s32 old = xpfAtomicAdd(&val, addend);
#define xpfAtomicAdd(_Ptr, _Add)   __sync_fetch_and_add(_Ptr, _Add)
#define xpfAtomicAdd64(_Ptr, _Add) __sync_fetch_and_add(_Ptr, _Add)


// Atomic OR: Returns the value before OR.
//  Ex:
//     xpf::s32 val, mask = 0xf0f0f0f0;
//     xpf::s32 old = xpfAtomicOr(&val, mask);
#define xpfAtomicOr(_Ptr, _Mask)   __sync_fetch_and_or(_Ptr, _Mask)
#define xpfAtomicOr8(_Ptr, _Mask)  __sync_fetch_and_or(_Ptr, _Mask)
#define xpfAtomicOr16(_Ptr, _Mask) __sync_fetch_and_or(_Ptr, _Mask)
#define xpfAtomicOr64(_Ptr, _Mask) __sync_fetch_and_or(_Ptr, _Mask)

// Atomic AND: Returns the value before AND.
//  Ex:
//     xpf::s32 val, mask = 0xf0f0f0f0;
//     xpf::s32 old = xpfAtomicAnd(&val, mask);
#define xpfAtomicAnd(_Ptr, _Mask)   __sync_fetch_and_and(_Ptr, _Mask)
#define xpfAtomicAnd8(_Ptr, _Mask)  __sync_fetch_and_and(_Ptr, _Mask)
#define xpfAtomicAnd16(_Ptr, _Mask) __sync_fetch_and_and(_Ptr, _Mask)
#define xpfAtomicAnd64(_Ptr, _Mask) __sync_fetch_and_and(_Ptr, _Mask)


// Atomic XOR: Returns the value before XOR.
//  Ex:
//     xpf::s32 val, mask = 0xf0f0f0f0;
//     xpf::s32 old = xpfAtomicXor(&val, mask);
#define xpfAtomicXor(_Ptr, _Mask)   __sync_fetch_and_xor(_Ptr, _Mask)
#define xpfAtomicXor8(_Ptr, _Mask)  __sync_fetch_and_xor(_Ptr, _Mask)
#define xpfAtomicXor16(_Ptr, _Mask) __sync_fetch_and_xor(_Ptr, _Mask)
#define xpfAtomicXor64(_Ptr, _Mask) __sync_fetch_and_xor(_Ptr, _Mask)


// Atomic Compare-and-swap operation (CAS): Compare the value pointed by '_Dest' to '_Comperand'.
//                                          If equal, swap the value to be '_Exchange'.
//                                          Return the initial value store in '_Dest' BEFORE the operation.
#define xpfAtomicCAS(_Dest, _Comperand, _Exchange)   __sync_val_compare_and_swap(_Dest, _Comperand, _Exchange)
#define xpfAtomicCAS16(_Dest, _Comperand, _Exchange) __sync_val_compare_and_swap(_Dest, _Comperand, _Exchange)
#define xpfAtomicCAS64(_Dest, _Comperand, _Exchange) __sync_val_compare_and_swap(_Dest, _Comperand, _Exchange)


#else
# error Atomic operations have not yet implemented for your compiler.
#endif

#endif // _XPF_ATOMIC_HEADER_
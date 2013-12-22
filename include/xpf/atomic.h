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

// Atomic add: Add the value pointed by 'val' with d and save the result back to 'val'. 
//             Returns the value before adding.
xpf::s32 XPF_API xpfAtomicAddS32(volatile xpf::s32 *val, xpf::s32 d);
xpf::u32 XPF_API xpfAtomicAddU32(volatile xpf::u32 *val, xpf::u32 d);
xpf::s64 XPF_API xpfAtomicAddS64(volatile xpf::s64 *val, xpf::s64 d);
xpf::u64 XPF_API xpfAtomicAddU64(volatile xpf::u64 *val, xpf::u64 d);

// Atomic AND: Perform logical AND on the value pointed by 'val' with d and save the result back to 'val'.
//             Returns the value before operation.
xpf::c8  XPF_API xpfAtomicAndS8 (volatile xpf::c8  *val, xpf::c8  d);
xpf::s16 XPF_API xpfAtomicAndS16(volatile xpf::s16 *val, xpf::s16 d);
xpf::s32 XPF_API xpfAtomicAndS32(volatile xpf::s32 *val, xpf::s32 d);

// Atomic OR: Perform logical OR on the value pointed by 'val' with d and save the result back to 'val'.
//             Returns the value before operation.
xpf::c8  XPF_API xpfAtomicOrS8 (volatile xpf::c8  *val, xpf::c8  d);
xpf::s16 XPF_API xpfAtomicOrS16(volatile xpf::s16 *val, xpf::s16 d);
xpf::s32 XPF_API xpfAtomicOrS32(volatile xpf::s32 *val, xpf::s32 d);

// Atomic XOR: Perform logical XOR on the value pointed by 'val' with d and save the result back to 'val'.
//             Returns the value before operation.
xpf::c8  XPF_API xpfAtomicXorS8 (volatile xpf::c8  *val, xpf::c8  d);
xpf::s16 XPF_API xpfAtomicXorS16(volatile xpf::s16 *val, xpf::s16 d);
xpf::s32 XPF_API xpfAtomicXorS32(volatile xpf::s32 *val, xpf::s32 d);

// Atomic Compare-and-swap operation: Compare the value pointed by 'val' to oldValue.
//                                    If equal, swap the value to be newVlaue.
//                                    Return the initial value store in 'val' BEFORE the operation.
xpf::s16 XPF_API xpfAtomicCASS16(volatile xpf::s16 *val, xpf::s16 newValue, xpf::s16 oldValue);
xpf::s32 XPF_API xpfAtomicCASS32(volatile xpf::s32 *val, xpf::s32 newValue, xpf::s32 oldValue);
xpf::s64 XPF_API xpfAtomicCASS64(volatile xpf::s64 *val, xpf::s64 newValue, xpf::s64 oldValue);

#endif // _XPF_ATOMIC_HEADER_
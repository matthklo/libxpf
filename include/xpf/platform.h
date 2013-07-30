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

#ifndef _XPF_PLATFORM_HEADER_
#define _XPF_PLATFORM_HEADER_

#include "assert.h"

//==========----------==========//

/* 
 * Detecting host platform via pre-defined compiler macros.
 * http://nadeausoftware.com/articles/2012/01/c_c_tip_how_use_compiler_predefined_macros_detect_operating_system 
 */

// Cygwin-POSIX
#if !defined(XPF_PLATFORM_SPECIFIED) && (defined(__CYGWIN__) || defined(__CYGWIN32__))
#  define XPF_PLATFORM_CYGWIN 1
#  define XPF_PLATFORM_SPECIFIED 1
#endif

// Windows-CE
#if !defined(XPF_PLATFORM_SPECIFIED) && defined(_WIN32_WCE)
#  define XPF_PLATFORM_WINDOWS 1
#  define XPF_PLATFORM_WINCE 1
#  define XPF_PLATFORM_SPECIFIED 1
#endif

// Windows-desktop
#if !defined(XPF_PLATFORM_SPECIFIED) && defined(_WIN32)
#  define XPF_PLATFORM_WINDOWS 1
#  if defined(__MINGW32__) || defined(__MINGW64__)
#    define XPF_PLATFORM_MINGW
#  endif
#  define XPF_PLATFORM_SPECIFIED 1
#endif

// x86-MaxOS and IOS
#if !defined(XPF_PLATFORM_SPECIFIED) && defined(__APPLE__) && defined(__MACH__)
#  include <TargetConditionals.h>
#  if TARGET_IPHONE_SIMULATOR == 1
#    define XPF_PLATFORM_IOSSIM 1
#  elif TARGET_OS_IPHONE == 1
#    define XPF_PLATFORM_IOS 1
#  elif TARGET_OS_MAC == 1
#    define XPF_PLATFORM_MAC 1
#  else
#    error This is an un-supported mac platform.
#  endif
#  define XPF_PLATFORM_BSD 1
#  define XPF_PLATFORM_SPECIFIED 1
#endif

// BSD-family
#if !defined(XPF_PLATFORM_SPECIFIED) && (defined(__DragonFly__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__))
#  define XPF_PLATFORM_BSD 1
#  if defined(__DragonFly__)
#    define XPF_PLATFORM_DRAGONFLY 1
#  elif defined(__FreeBSD__)
#    define XPF_PLATFORM_FREEBSD 1
#  elif defined(__NetBSD__)
#    define XPF_PLATFORM_NETBSD 1
#  elif defined(__OpenBSD__)
#    define XPF_PLATFORM_OPENBSD 1
#  endif
#  define XPF_PLATFORM_SPECIFIED 1
#endif

// Linux/Android
#if !defined(XPF_PLATFORM_SPECIFIED) && defined(__unix__) && defined(__linux__)
#  if defined(__ANDROID__)
#    define XPF_PLATFORM_ANDROID 1
#  endif
#  define XPF_PLATFORM_LINUX 1
#  define XPF_PLATFORM_SPECIFIED 1
#endif

// Solaris
#if !defined(XPF_PLATFORM_SPECIFIED) && defined(__sun) && defined(__SVR4)
#  define XPF_PLATFORM_SOLARIS 1
#  define XPF_PLATFORM_SPECIFIED 1
#endif



//==========----------==========//

/*
 * Determines address-model
 */

#ifndef XPF_MODEL_SPECIFIED

// Currently only x86/x86_64/arm are supported.
#if defined(XPF_PLATFORM_WINDOWS)
#  define XPF_CPU_X86 1
#  if defined(_WIN64)
#    define XPF_MODEL_64 1
#    define XPF_X86_64 1
#  else
#    define XPF_MODEL_32 1
#    define XPF_X86_32 1
#  endif
#else
#  if defined(__arm__)
#    define XPF_CPU_ARM 1
#    define XPF_MODEL_32 1
#    define XPF_ARM_32 1
#  elif defined(__x86_64) || defined(__amd64) || defined(__amd64__)
#    define XPF_CPU_X86 1
#    define XPF_MODEL_64 1
#    define XPF_X86_64 1
#  elif defined(__i386__) || defined(__i386) || defined(__i686__) || defined(__i686)
#    define XPF_CPU_X86 1
#    define XPF_MODEL_32 1
#    define XPF_X86_32 1
#  else
#    error Un-supported arch type.
#  endif
#endif

#endif // XPF_MODEL_SPECIFIED


//==========----------==========//

// Data types

namespace xpf {

typedef unsigned char      u8;
typedef char               c8;
typedef unsigned short     u16;
typedef short              s16;
typedef unsigned int       u32;
typedef int                s32;
typedef unsigned long long u64;
typedef long long          s64;
typedef float              f32;
typedef double             f64;
#if defined(XPF_MODEL_32)
typedef u32                vptr;
#elif defined(XPF_MODEL_64)
typedef u64                vptr;
#else
#  error Can not determine pointer data type due to unknown address-model.
#endif

}  // end of namespace xpf {

// Double confirm the width of all types at compile time.
xpfSAssert(1==(sizeof(xpf::u8)));
xpfSAssert(1==(sizeof(xpf::c8)));
xpfSAssert(2==(sizeof(xpf::u16)));
xpfSAssert(2==(sizeof(xpf::s16)));
xpfSAssert(4==(sizeof(xpf::u32)));
xpfSAssert(4==(sizeof(xpf::s32)));
xpfSAssert(4==(sizeof(xpf::f32)));
xpfSAssert(8==(sizeof(xpf::u64)));
xpfSAssert(8==(sizeof(xpf::s64)));
xpfSAssert(8==(sizeof(xpf::f64)));
#if defined(XPF_MODEL_32)
xpfSAssert(4==(sizeof(xpf::vptr)));
#elif defined(XPF_MODEL_64)
xpfSAssert(8==(sizeof(xpf::vptr)));
#else
#  error Can not determine pointer data type due to unknown address-model.
#endif

//==========----------==========//

#endif // _XPF_PLATFORM_HEADER_
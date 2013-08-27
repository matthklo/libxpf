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

#ifndef _XPF_COMPILER_HEADER_
#define _XPF_COMPILER_HEADER_

/*
 * Determines:
 *   1. Compiler vendors & versions.
 *   2. Debug/release configurations.
 * Impl:
 *   1. Library exporting macro.
 *   2. Handy macros for working with compiler.
 */

#if defined(__cplusplus)
#  define XPF_COMPILER_CXX 1
#else
#  define XPF_COMPILER_C 1
#endif

//==========----- Compiler vendor and version -----==========//

#if !defined(XPF_COMPILER_SPECIFIED) && defined(_MSC_VER)
#  if _MSC_VER >= 1700
     // Visual Studio 2012
#    define XPF_COMPILER_MSVC110 1
#  elif _MSC_VER >= 1600
     // Visual Studio 2010
#    define XPF_COMPILER_MSVC100 1
#  elif MSC_VER >= 1500
     // Visual Studio 2008
#    define XPF_COMPILER_MSVC90 1
#  elif MSC_VER >= 1400
     // Visual Studio 2005
#    define XPF_COMPILER_MSVC80 1
#  else
#    error Your Visual Studio version is too old.
#  endif
#  if defined(XPF_COMPILER_CXX) && defined(_CPPRTTI)
#    define XPF_COMPILER_CXX_RTTI 1
#  endif
#  define XPF_COMPILER_MSVC 1
#  define XPF_COMPILER_SPECIFIED 1
#endif

#if !defined(XPF_COMPILER_SPECIFIED) && defined(__GNUC__)
#  if __GNUC__ >= 4
#    define XPF_COMPILER_GNUC4 1
#  else
#    error Your GNUC (or LLVM with GNUC frontend) version is too old.
#  endif
#  define XPF_COMPILER_GNUC 1
#  if defined(__llvm__)
#    define XPF_COMPILER_LLVM 1
#  endif
#  if defined(XPF_COMPILER_CXX) && defined(__GXX_RTTI)
#    define XPF_COMPILER_CXX_RTTI 1
#  endif
#  define XPF_COMPILER_SPECIFIED 1
#endif

#if !defined(XPF_COMPILER_SPECIFIED) && (defined(__ICC) || defined(__INTEL_COMPILER))
#  define XPF_COMPILER_INTEL 1
#  if defined(XPF_COMPILER_CXX) && defined(__INTEL_RTTI__)
#    define XPF_COMPILER_CXX_RTTI 1
#  endif
#  define XPF_COMPILER_SPECIFIED 1
#endif


//==========----- Library exporting / function visibility macros -----==========//

#if defined(XPF_STATIC_LIBRARY) || defined(XPF_BUILD_STATIC_LIBRARY)
#  define XPF_API
#else
#  if defined(XPF_COMPILER_MSVC)
#    if defined(XPF_BUILD_LIBRARY)
#      define XPF_API __declspec(dllexport)
#    else
#      define XPF_API __declspec(dllimport)
#    endif
#  elif defined(XPF_COMPILER_GNUC)
#    if defined(XPF_BUILD_LIBRARY)
#      define XPF_API __attribute__ ((visibility("default")))
#    else
#      define XPF_API
#    endif
#  else
#    define XPF_API
#  endif
#endif

#if defined(XPF_COMPILER_CXX)
#  define XPF_EXTERNC extern "C"
#  define XPF_EXTERNC_BEGIN extern "C" {
#  define XPF_EXTERNC_END }
#else
#  define XPF_EXTERNC
#  define XPF_EXTERNC_BEGIN
#  define XPF_EXTERNC_END
#endif


//==========----- Build configurations -----==========//

#if defined(_WIN32)
#  if defined(_MT)
#    if defined(_DLL)
#      define XPF_WINCRT_DLL 1
#    else
#      define XPF_WINCRT_STATIC 1
#    endif
#  else
#    error Single-thread version of Windows crt is not supported.
#  endif
#  if defined(_DEBUG)
#    define XPF_BUILD_DEBUG 1
#  else
#    define XPF_BUILD_RELEASE 1
#  endif
#else
#  if !defined(NDEBUG)
#    define XPF_BUILD_DEBUG 1
#  else
#    define XPF_BUILD_RELEASE 1
#  endif
#endif


//==========----- Handy macros -----==========//

// Manual branch predictions (currently only work with GNUC compiler)
//
// Please note that manual branch prediction usually work worse than built-in
// CPU branch prediction when the branch code has temporal locality and the 
// distribution of branch occurances are in kind of pattern (i.e. not random).
//
// To know more about branch prediction, please refer to:
// http://stackoverflow.com/questions/11227809/why-is-processing-a-sorted-array-faster-than-an-unsorted-array

#if defined(XPF_COMPILER_GNUC)
#define xpfLikely(x) __builtin_expect((x),1)
#define xpfUnlikely(x) __builtin_expect((x),0) 
#else
#define xpfLikely(x) x
#define xpfUnlikely(x) x
#endif

// Supress unused local variable warning
#define XPF_RESERVED(x) x



//==========----------==========//

#endif // _XPF_COMPILER_HEADER_
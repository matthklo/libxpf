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

#ifndef _XPF_ASSERT_HEADER_
#define _XPF_ASSERT_HEADER_

/*
 * Here list those macros related to assertion (both runtime and static).
 * 
 * NOTE: Runtime assertions are enabled even in release mode by default. 
 *       Define 'NDEBUG' and add it to your overall compiler flags to remove runtime assertions.
 */

//==========----- Runtime assertion -----==========//

#ifdef NDEBUG
// From http://cnicholson.net/2009/02/stupid-c-tricks-adventures-in-assert/
// A way to disable assert() which shushes more compilers than just define an empty macro body (the way used by assert.h).
#	define xpfAssert(x) do { (void)sizeof(x); } while((void)(__LINE__==-1), 0)
#else
#	include <assert.h>
#	define xpfAssert assert
#endif

//==========----- Static assertion -----==========//
// From http://www.pixelbeat.org/programming/gcc/static_assert.html
// Old way to implement static assertion before C++0x
#define _XPF_ASSERT_CONCAT_(a, b) a##b
#define _XPF_ASSERT_CONCAT(a, b) _XPF_ASSERT_CONCAT_(a, b)
#define xpfSAssert(e) enum { _XPF_ASSERT_CONCAT(assert_line_, __LINE__) = 1/((e)?1:0) }

#endif // _XPF_ASSERT_HEADER_

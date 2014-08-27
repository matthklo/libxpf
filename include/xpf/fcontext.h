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

#ifndef _XPF_FCONTEXT_HEADER_
#define _XPF_FCONTEXT_HEADER_

#include "platform.h"

namespace xpf
{

typedef void*   fcontext_t;

XPF_EXTERNC
vptr
jump_fcontext( fcontext_t * ofc, fcontext_t nfc,
               vptr vp, bool preserve_fpu = true );
XPF_EXTERNC
fcontext_t
make_fcontext( void * sp, vptr size, void (* fn)( vptr ) );

};

#endif // _XPF_FCONTEXT_HEADER_


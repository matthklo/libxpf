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

#ifndef _XPF_UTFCONV_HEADER_
#define _XPF_UTFCONV_HEADER_

#include "platform.h"

namespace xpf
{
    // Functions defined in 'details' namespace should be only used internally by xpf.
    // For utf conversions, use xpf::utfCon() instead.
    namespace details
    {
        s32 XPF_API utfConvInner( void * sbuf, void * dbuf, u32 scw, u32 dcw, s32 scnt, s32 dcnt );
    };

    /***********************************************
     * Convert unicode strings between utf-8, ucs-2 (utf-16), and ucs-4 (utf-32).
     *
     * Template arguments:
     *    SrcType:  Character type of source string.
     *    DstType:  Character type of destination string.
     * Function arguments:
     *    srcBuf:   Pointer pointed to a string buffer of SrcType.
     *    dstBuf:   Pointer pointed to a string buffer of DstType.
     *    srcCount: Number of SrcType items in srcBuf to be converted. 
     *              If -1 is given, it assumes the source string is null-terminated.
     *    dstCount: Max number of DstType items in dstBuf can hold.
     *              If -1 is given, it assumes the dstBuf is large enough to hold all conversion result.
     * Return value:
     *    Return negative number if one of SrcType and DstType is not supported.
     *    Return >=0 indicates how many code point have been converted.
     *    utfConv() will try to append a null in the end of dstBuf if possible.
     *
     * Example:
     *    xpf::c8 utfString[] = {0xe6, 0xff, 0xff, 0xe6, 0xb8, 0xac, 0xff, 0xa9, 0xa6, 0x0};
     *    wchar_t wString[100];
     *
     *    int count = xpf::utfConv<xpf::c8, wchar_t>(utfString, wString);
     *
     *****/
    template< typename SrcType, typename DstType >
    s32 utfConv( SrcType * srcBuf, DstType * dstBuf, s32 srcCount = -1, s32 dstCount = -1 )
    {
        return details::utfConvInner( (void*)srcBuf, (void*)dstBuf, sizeof(SrcType), sizeof(DstType), srcCount, dstCount );
    }

}; // end of namespace xpf

#endif // _XPF_UTFCONV_HEADER_
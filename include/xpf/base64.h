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

#ifndef _XPF_BASE64_HEADER_
#define _XPF_BASE64_HEADER_

#include "platform.h"

namespace xpf
{

class XPF_API Base64
{
public:
	/**
	 *  Encode a bulk of memory buffer to a base64 encoded ASCII string.
	 *
	 *  Input Parameters:
	 *    input     - Pointer to the input buffer. Must NOT be NULL.
	 *    inputlen  - Length of input buffer. Must >= 0.
	 *    output    - Pointer to the output buffer to hold the encoded string.
	 *                If either output or outputlen is 0, the encode() simply
	 *                computes the output length required and returns.
	 *    outputlen - Length of output buffer. If its value is smaller than
	 *                required (except for 0, see above), encode() returns
	 *                -1 and fill the required length in *outputlen.
	 *  Return Value:
	 *    -1 to indicate an error. Otherwise it is the length of the encoded
	 *    string.
	 */
	static int encode(const char *input, int inputlen, char *output, int *outputlen);

	/**
	*  Decode a base64 encoded ASCII string to original data.
	*
	*  Input Parameters:
	*    input     - Pointer to the input buffer. Must NOT be NULL.
	*    inputlen  - Length of input buffer. Must >= 0.
	*    output    - Pointer to the output buffer to hold the decoded data.
	*                If either output or outputlen is 0, the decode() simply
	*                computes the output length required and returns.
	*    outputlen - Length of output buffer. If its value is smaller than
	*                required (except for 0, see above), decode() returns
	*                -1 and fill the required length in *outputlen.
	*  Return Value:
	*    -1 to indicate an error. Otherwise it is the length of the decoded
	*    data.
	*/
	static int decode(const char *input, int inputlen, char *output, int *outputlen);
};

}; // end of namespace xpf

#endif // _XPF_BASE64_HEADER_
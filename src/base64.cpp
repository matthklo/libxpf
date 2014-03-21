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

#include <xpf/base64.h>

static const char *chtab = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int idxofch(char ch)
{
	if ((ch >= 'A') && (ch <= 'Z'))
		return (ch - 'A');
	else if ((ch >= 'a') && (ch <= 'z'))
		return (ch - 'a') + 26;
	else if ((ch >= '0') && (ch <= '9'))
		return (ch - '0') + 52;
	else if (ch == '+')
		return 62;
	else if (ch == '/')
		return 63;
	else if (ch == '=')
		return 0;
	return -1;
}

namespace xpf
{

int Base64::encode(const char *input, int inputlen, char *output, int *outputlen)
{
	if (inputlen < 0) return -1;

	// Compute the encoded base64 string length
	int enclen = ((inputlen + 2) / 3) * 4;

	if (outputlen == 0 || output == 0)
	{
		if (outputlen) *outputlen = enclen;
		return enclen;
	}

	// Check if the output buffer is large enough
	int outlen = *outputlen;
	if (outlen < enclen)
	{
		*outputlen = enclen;
		return -1;
	}

	int outidx = 0;
	int inidx = 0;
	char temp = 0;
	while (outidx < enclen)
	{
		char ch = input[inidx];
		int mod = inidx % 3;
		switch (mod)
		{
		case 0:
			output[outidx++] = chtab[(ch >> 2) & 0x3f];
			temp = ((ch & 0x03) << 4);
			inidx++;
			break;
		case 1:
			output[outidx++] = chtab[((ch >> 4) & 0x0f) | temp];
			temp = ((ch & 0x0f) << 2);
			inidx++;
			break;
		case 2:
			output[outidx++] = chtab[((ch >> 6) & 0x03) | temp];
			output[outidx++] = chtab[(ch & 0x3f)];
			inidx++;
			break;
		}

		// EOF check. Apply padding if necessary.
		if ((inidx >= inputlen) && (mod != 2))
		{
			output[outidx++] = chtab[temp];
			output[outidx++] = '=';
			if (mod == 0)
				output[outidx++] = '=';
		}
	}

	return enclen;
}

int Base64::decode(const char *input, int inputlen, char *output, int *outputlen)
{
	// The length of a valid base64 string should always be a multiplier of 4.
	if ((inputlen < 0) || (inputlen % 4) != 0)
		return -1;

	int declen = (inputlen / 4) * 3;
	if (inputlen >= 4)
	{
		if (input[inputlen - 1] == '=')
			declen--;
		if (input[inputlen - 2] == '=')
			declen--;
	}

	if (outputlen == 0 || output == 0)
	{
		if (outputlen) *outputlen = declen;
		return declen;
	}

	// Check if the output buffer is large enough
	int outlen = *outputlen;
	if (outlen < declen)
	{
		*outputlen = declen;
		return -1;
	}

	int outidx = 0;
	for (int inidx = 0; inidx < inputlen; inidx += 4)
	{
		int ch1 = idxofch(input[inidx]);
		int ch2 = idxofch(input[inidx + 1]);
		int ch3 = idxofch(input[inidx + 2]);
		int ch4 = idxofch(input[inidx + 3]);

		if ((ch1 == -1) || (ch2 == -1) || (ch3 == -1) || (ch4 == -1) || (input[inidx] == '=') || (input[inidx + 1] == '='))
			return -1; // corrupted data

		output[outidx++] = (ch1 << 2) | ((ch2 >> 4) & (0x03));
		if (input[inidx + 2] == '=') break;
		output[outidx++] = ((ch2 & 0x0f) << 4) | ((ch3 >> 2) & 0x0f);
		if (input[inidx + 3] == '=') break;
		output[outidx++] = ((ch3 & 0x03) << 6) | ch4;
	}

	return outidx;
}



}; // end of namespace xpf


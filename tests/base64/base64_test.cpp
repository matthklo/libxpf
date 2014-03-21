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
#include <string.h>
#include <stdio.h>

// testcase from http://en.wikipedia.org/wiki/Base64

const char *plaintexts[] = {
	"Man is distinguished, not only by his reason, but by this singular passion from other animals, which is a lust of the mind, that by a perseverance of delight in the continued and indefatigable generation of knowledge, exceeds the short vehemence of any carnal pleasure.",
	"pleasure.",
	"leasure.",
	"easure.",
	"asure.",
	"sure.",
};

const char *codedtexts[] = {
	"TWFuIGlzIGRpc3Rpbmd1aXNoZWQsIG5vdCBvbmx5IGJ5IGhpcyByZWFzb24sIGJ1dCBieSB0aGlzIHNpbmd1bGFyIHBhc3Npb24gZnJvbSBvdGhlciBhbmltYWxzLCB3aGljaCBpcyBhIGx1c3Qgb2YgdGhlIG1pbmQsIHRoYXQgYnkgYSBwZXJzZXZlcmFuY2Ugb2YgZGVsaWdodCBpbiB0aGUgY29udGludWVkIGFuZCBpbmRlZmF0aWdhYmxlIGdlbmVyYXRpb24gb2Yga25vd2xlZGdlLCBleGNlZWRzIHRoZSBzaG9ydCB2ZWhlbWVuY2Ugb2YgYW55IGNhcm5hbCBwbGVhc3VyZS4=",
	"cGxlYXN1cmUu",
	"bGVhc3VyZS4=",
	"ZWFzdXJlLg==",
	"YXN1cmUu",
	"c3VyZS4=",
};

using namespace xpf;

int main()
{
	int testcases = sizeof(plaintexts) / sizeof(const char*);

	// encoding test
	for (int i = 0; i<testcases; ++i)
	{
		int olen = Base64::encode(plaintexts[i], 0, 0, 0);
		xpfAssert(olen == 0);

		int len = strlen(plaintexts[i]);
		olen = Base64::encode(plaintexts[i], len, 0, 0);
		xpfAssert(olen == strlen(codedtexts[i]));

		char *buf = new char[olen + 1];
		buf[olen] = '\0';

		// ensure the actual output len hint is work.
		int blen = 1;
		int tlen = Base64::encode(plaintexts[i], len, buf, &blen);
		xpfAssert((tlen == -1) && (blen == strlen(codedtexts[i])));

		// ensure encoding is work.
		blen = olen + 1;
		tlen = Base64::encode(plaintexts[i], len, buf, &blen);

		xpfAssert(blen == (tlen + 1)); // ensure blen is not touched when output buf is sufficient
		xpfAssert(tlen == olen);
		xpfAssert(strcmp(buf, codedtexts[i]) == 0);

		delete[] buf;
	}

	// decoding test
	for (int i = 0; i<testcases; ++i)
	{
		int olen = Base64::decode(codedtexts[i], 0, 0, 0);
		xpfAssert(olen == 0);

		int len = strlen(codedtexts[i]);
		olen = Base64::decode(codedtexts[i], len, 0, 0);
		xpfAssert(olen == strlen(plaintexts[i]));

		char *buf = new char[olen + 1];
		buf[olen] = '\0';

		// ensure the actual output len hint is work.
		int blen = 1;
		int tlen = Base64::decode(codedtexts[i], len, buf, &blen);
		xpfAssert((tlen == -1) && (blen == strlen(plaintexts[i])));

		// ensure decoding is work.
		blen = olen + 1;
		tlen = Base64::decode(codedtexts[i], len, buf, &blen);

		xpfAssert(blen == (tlen + 1)); // ensure blen is not touched when output buf is sufficient
		xpfAssert(tlen == olen);
		xpfAssert(strcmp(buf, plaintexts[i]) == 0);

		printf("testcase %d: %s\n", i + 1, buf);

		delete[] buf;
	}

	return 0;
}

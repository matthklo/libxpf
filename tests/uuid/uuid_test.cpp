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

#include <xpf/uuidv4.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string>

using namespace xpf;

int main(int argc, char *argv[])
{
	srand((unsigned int)time(0));

	Uuid id1 = Uuid::null();
	Uuid id2 = Uuid::null();

	xpfAssert(id1 == id2);
	xpfAssert(id2.asString() == "00000000-0000-0000-0000-000000000000");

	Uuid id3 = Uuid::fromString("00000000-0000-0000-0000-000000000000");
	xpfAssert(id1 == id3);
	
	id1 = Uuid::generate();
	std::string v = id1.asString();
	id2 = Uuid::fromString(v.c_str());
	id3 = Uuid::fromOctet(id1.getOctet());
	xpfAssert(id1 == id2);
	xpfAssert(id2 == id3);


	// Distribution test.
	// Generate 200 uuids and check if all 122 possible bits are covered.
	u8 coverage[16] = { 0, 0, 0, 0, 0, 0, 0xf0, 0, 0xc0, 0, 0, 0, 0, 0, 0, 0 };
	for (u32 i = 0; i < 200; ++i)
	{
		Uuid u = Uuid::generate();
		// check UUID v4 mark.
		xpfAssert((u.getOctet()[6] & 0xf0) == 0x40);
		xpfAssert((u.getOctet()[8] & 0xc0) == 0x80);

		// commit bit coverage.
		for (u32 j = 0; j < 16; ++j)
		{
			coverage[j] |= u.getOctet()[j];
		}
	}

	// check coverage
	for (u32 i = 0; i < 16; ++i)
	{
		xpfAssert(coverage[i] == 0xFF);
	}

	return 0;
}

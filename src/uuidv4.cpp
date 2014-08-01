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

#include <xpf/platform.h>
#include <xpf/uuidv4.h>
#include <string.h>
#include <stdlib.h>

namespace xpf
{

Uuid Uuid::generate()
{
	// A valid UUID v4 Layout:
	// xxxxxxxx-xxxx-4xxx-Nxxx-xxxxxxxxxxxx, where N 
	// could be 8,9,A,B.
	// Each rand() returns a 32-bits randome value
	// varies from 0 to RAND_MAX. Here we assume
	// RAND_MAX is 32767, which stands on most popular
	// platforms. By this assumption, we get only 15 
	// random bits at each rand() call.
	Uuid u;

	// Fill u.mOctet with 8 random value.
	for (u32 i = 0; i < 8; ++i)
	{
		u16 r = (u16)rand();
		u.mOctet[i * 2] = r >> 8;
		u.mOctet[i * 2 + 1] = r & 0xFF;
	}

	// By the assumption of RAND_MAX is 32767,
	// the most significant bit of those 8 random values
	// should all be 0. Here we get another random value
	// and use its bit to pad those 'holes'.
	u16 r = (u16)rand();
	u.mOctet[0] |= ((r & 0x1) << 7);
	u.mOctet[2] |= ((r & 0x2) << 6);
	u.mOctet[4] |= ((r & 0x4) << 5);
	u.mOctet[6] = ((u.mOctet[6] & 0x0F) | 0x40);
	u.mOctet[8] = ((u.mOctet[8] & 0x3F) | 0x80);
	u.mOctet[10] |= ((r & 0x8) << 4);
	u.mOctet[12] |= ((r & 0x10) << 3);
	u.mOctet[14] |= ((r & 0x20) << 2);

	return u;
}

Uuid Uuid::null()
{
	return Uuid();
}

Uuid Uuid::fromOctet(u8* octet)
{
	Uuid u;
	::memcpy(u.mOctet, octet, 16);
	return u;
}

Uuid Uuid::fromString(const c8* string)
{

	Uuid u;
	u8 cnt = 0;
	u8 w = 0xff;
	for (const c8 *p = string; (*p != '\0') && (cnt < 16); p++)
	{
		u8 v = (u8)*p;
		if ((v >= '0') && (v <= '9'))
			v -= '0';
		else if ((v >= 'a') && (v <= 'f'))
			v = 10 + (v - 'a');
		else if ((v >= 'A') && (v <= 'F'))
			v = 10 + (v - 'A');
		else
			continue;

		if (w == 0xff)
		{
			w = v;
			continue;
		}

		u.mOctet[cnt++] = (w << 4) | v;
		w = 0xff;
	}
	return u;
}

Uuid::Uuid()
{
	::memset(mOctet, 0, 16);
}

Uuid::Uuid(const Uuid& other)
{
	this->operator=(other);
}

Uuid::~Uuid()
{

}

u32 Uuid::getLength() const
{
	return 16;
}

u8* Uuid::getOctet()
{
	return mOctet;
}

std::string Uuid::asString()
{
	static const c8 *map = "0123456789abcdef";

	std::string str;
	for (u32 i = 0; i<16; ++i)
	{
		u8 c = mOctet[i];
		str.append(1, map[c >> 4]);
		str.append(1, map[c & 0xF]);
		if ((i == 3) || (i == 5) || (i == 7) || (i == 9))
			str.append(1, '-');
	}
	return str;
}

bool Uuid::operator< (const Uuid& other) const
{
	for (int i = 0; i<4; ++i)
	{
		if ((*(unsigned int*)&mOctet[i * 4]) <
			(*(unsigned int*)&other.mOctet[i * 4]))
			return true;
	}
	return false;
}

Uuid& Uuid::operator= (const Uuid& other)
{
	::memcpy(mOctet, other.mOctet, 16);
	return *this;
}

bool Uuid::operator== (const Uuid& other) const
{
	for (int i = 0; i<4; ++i)
	{
		if ((*(unsigned int*)&mOctet[i * 4]) !=
			(*(unsigned int*)&other.mOctet[i * 4]))
			return false;
	}
	return true;
}

};

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

#ifndef _XPF_UUIDV4_HEADER_
#define _XPF_UUIDV4_HEADER_

#include "platform.h"
#include <string>

namespace xpf
{

class XPF_API Uuid
{
public:
	/*
	 * Generate a version 4 (random) UUID.
	 * It utilizes the psuedo randomer of platform,
	 * so be sure to seed the randomer properly.
	 */
	static Uuid generate();
	/*
	 * Return a nil/null UUID.
	 */
	static Uuid null();
	/*
	 * Build an UUID object from raw octet (16 bytes).
	 */
	static Uuid fromOctet(u8* octet);
	/*
	 * Build an UUID object from an null-terminated UUID string.
	 */
	static Uuid fromString(const c8* string);

	Uuid(const Uuid& other);
	virtual ~Uuid();

	/*
	 * Return the length of octet, which should always be 16.
	 */
	u32 getLength() const;
	/*
	 * Retrieve the raw data of the octet.
	 */
	u8* getOctet();
	/*
	 * Get the readable UUID string of current octet.
	 */
	std::string asString();

	bool  operator<  (const Uuid& other) const;
	Uuid& operator=  (const Uuid& other);
	bool  operator== (const Uuid& other) const;

protected:
	/*
	 * Hide constructor. To construct an uuid, use Uuid::generate().
	 * Or use Uuid::null() to obtain a null uuid.
	 */
	Uuid();

private:
	u8 mOctet[16];
};

};

#endif
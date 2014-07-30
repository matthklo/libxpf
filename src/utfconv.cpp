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

#include <xpf/utfconv.h>

namespace xpf {
namespace details {

enum EStatus { END_OF_STREAM = 0xFFFFFFFF, BAD_FORMAT = 0xFFFFFFFE, SUCCESS = 0, };

class IUcReader
{
public:
	virtual ~IUcReader() {}
	virtual u32 read() = 0;
};

class IUcWriter
{
public:
	virtual ~IUcWriter() {}
	virtual u32 write(u32 cp) = 0;
	virtual u32 closure() = 0;
};

class Utf8Reader : public IUcReader
{
public:
	Utf8Reader(u8 * buf, s32 cnt) : Current(buf), Remaining(cnt) {};

	u32 read()
	{
		u32 codepoint = 0;
		s32 sbcnt = 0;

		if ((Remaining == 0) || (*Current == 0))
			return END_OF_STREAM;

		// Determine how many subsequent bytes for current code point.
		u8 c = *Current;
		if ((c & 0x80) == 0)
		{
			codepoint = c;
		} else if ((c & 0xE0) == 0xC0) {
			sbcnt = 1;
			codepoint = (c & 0x1F); 
		} else if ((c & 0xF0) == 0xE0) {
			sbcnt = 2;
			codepoint = (c & 0x0F);
		} else if ((c & 0xF8) == 0xF0) {
			sbcnt = 3;
			codepoint = (c & 0x07);
		} else if ((c & 0xFC) == 0xF8) {
			sbcnt = 4;
			codepoint = (c & 0x03);
		} else if ((c & 0xFE) == 0xFC) {
			sbcnt = 5;
			codepoint = (c & 0x01);
		} else {
			return BAD_FORMAT;
		}

		// Update remaining byte count.
		// If current reading code point is truncated by remaining count,
		// simply abort the reading and report EOS.
		if (Remaining > 0)
		{
			if (Remaining < (sbcnt + 1))
				return END_OF_STREAM;
			else
				Remaining -= (sbcnt + 1);
		}

		// utf-8 convertion
		while (sbcnt--)
		{
			Current++;
			if (((*Current) & 0xC0) == 0x80)
			{
				codepoint <<= 6;
				codepoint |= ((*Current) & 0x3F);
			} else {
				return BAD_FORMAT;
			}
		}

		Current++;
		return codepoint;
	}

private:
	s32 Remaining;
	u8* Current;
};

class Ucs2Reader : public IUcReader
{
public:
	Ucs2Reader(u16 * buf, s32 cnt) : Current(buf), Remaining(cnt) {}

	u32 read()
	{
		u32 codepoint = 0;

		if ((Remaining == 0) || (*Current == 0))
			return END_OF_STREAM;

		u16 c = *Current;
		if ((c >= 0xD800) && (c <= 0xDBFF))
		{
			if (Remaining > 0)
			{
				if (Remaining < 2)
					return END_OF_STREAM;
				else
					Remaining -= 2;
			}

			// A lead surrogate
			u16 t = *(++Current);
			if ((t >= 0xDC00) && (t <= 0xDFFF))
			{
				codepoint = (c - 0xD800);
				codepoint <<= 10;
				codepoint |= (t - 0xDC00);
				codepoint += 0x10000;
			} else {
				// A lead surrogate with no following trail surrogate.
				return BAD_FORMAT;
			}
		}
		else if ((c >= 0xDC00) && (c <= 0xDFFF))
		{
			// An orphan trail surrogate ...
			return BAD_FORMAT;
		}
		else
		{
			if (Remaining > 0)
				Remaining--;

			codepoint = c;
		}

		Current++;
		return codepoint;
	}

private:
	s32  Remaining;
	u16* Current;
};

class Ucs4Reader : public IUcReader
{
public:
	Ucs4Reader(u32 * buf, s32 cnt) : Current(buf), Remaining(cnt) {}

	u32 read()
	{
		if ((Remaining == 0) || (*Current == 0))
			return END_OF_STREAM;

		if (Remaining > 0)
			Remaining--;

		return *(Current++);
	}

private:
	s32  Remaining;
	u32* Current;
};

class Utf8Writer : public IUcWriter
{
public:
	Utf8Writer(u8 * buf, s32 cnt) : Current(buf), Remaining(cnt) {}

	u32 write(u32 cp)
	{
		if (Remaining == 0)
			return END_OF_STREAM;

		int sbcnt = 0;
		if ( cp >= 0x4000000 )
		{
			sbcnt = 5;
		} else if ( cp >= 0x200000 ) {
			sbcnt = 4;
		} else if ( cp >= 0x10000) {
			sbcnt = 3;
		} else if ( cp >= 0x800) {
			sbcnt = 2;
		} else if ( cp >= 0x80) {
			sbcnt = 1;
		}

		if (Remaining > 0)
		{
			if (Remaining < (sbcnt + 1))
				return END_OF_STREAM;
			else
				Remaining -= (sbcnt + 1);
		}

		for (int i=sbcnt; i>0; --i)
		{
			*(Current+i) = 0x80 | ((u8)(cp & 0x3F));
			cp >>= 6;
		}

		switch (sbcnt)
		{
		default:
		case 0:
			*Current = (u8)(cp & 0xFF);
			break;
		case 1:
			*Current = 0xC0 | (u8)(cp & 0x1F);
			break;
		case 2:
			*Current = 0xE0 | (u8)(cp & 0x0F);
			break;
		case 3:
			*Current = 0xF0 | (u8)(cp & 0x07);
			break;
		case 4:
			*Current = 0xF8 | (u8)(cp & 0x03);
			break;
		case 5:
			*Current = 0xFC | (u8)(cp & 0x01);
			break;
		}

		Current += (sbcnt + 1);
		return SUCCESS;
	}

	u32 closure()
	{
		if (Remaining != 0)
		{
			*Current = 0;
			return SUCCESS;
		}
		return END_OF_STREAM;
	}

private:
	s32 Remaining;
	u8* Current;
};

class Ucs2Writer : public IUcWriter
{
public:
	Ucs2Writer(u16 * buf, s32 cnt) : Current(buf), Remaining(cnt) {}

	u32 write(u32 cp)
	{
		if (Remaining == 0)
			return END_OF_STREAM;

		if (cp > 0x10FFFF)
		{
			// impossible code point value
		}
		else if (cp >= 0x10000)
		{
			if (Remaining > 0)
			{
				if (Remaining < 2)
					return END_OF_STREAM;
				else
					Remaining -= 2;
			}

			cp -= 0x10000;
			*(Current++)  = ((u16)((cp & 0xFFC00)>>10)) + 0xD800;
			*(Current++)  = ((u16)(cp & 0x3FF)) + 0xDC00;
		}
		else
		{
			if (Remaining > 0)
				Remaining--;

			*(Current++) = (u16) cp;
		}
		return SUCCESS;
	}

	u32 closure()
	{
		if (Remaining != 0)
		{
			*Current = 0;
			return SUCCESS;
		}
		return END_OF_STREAM;
	}

private:
	s32  Remaining;
	u16* Current;
};

class Ucs4Writer : public IUcWriter
{
public:
	Ucs4Writer(u32 * buf, s32 cnt) : Current(buf), Remaining(cnt) {}

	u32 write(u32 cp)
	{
		if (Remaining == 0)
			return END_OF_STREAM;

		if (Remaining > 0)
			Remaining--;

		*(Current++) = cp;
		return SUCCESS;
	}

	u32 closure()
	{
		if (Remaining != 0)
		{
			*Current = 0;
			return SUCCESS;
		}
		return END_OF_STREAM;
	}

private:
	s32  Remaining;
	u32* Current;
};

s32 utfConvInner( void * sbuf, void * dbuf, u32 scw, u32 dcw, s32 scnt, s32 dcnt )
{
	if (((scw != 4) && (scw != 2) && (scw != 1)) ||
		((dcw != 4) && (dcw != 2) && (dcw != 1)) )
		return -1;

	IUcReader *r = 0;
	IUcWriter *w = 0;

	// Select reader by the width of source char
	switch (scw)
	{
	case 4:
		r = new Ucs4Reader( (u32*) sbuf, scnt );
		break;
	case 2:
		r = new Ucs2Reader( (u16*) sbuf, scnt );
		break;
	case 1:
		r = new Utf8Reader( (u8*) sbuf, scnt );
		break;
	default:
		break;
	}

	// Select writer by the width of destination char
	switch (dcw)
	{
	case 4:
		w = new Ucs4Writer( (u32*) dbuf, dcnt );
		break;
	case 2:
		w = new Ucs2Writer( (u16*) dbuf, dcnt );
		break;
	case 1:
		w = new Utf8Writer( (u8*) dbuf, dcnt );
		break;
	default:
		break;
	}

	s32 cnt = 0;

	while (true)
	{
		u32 cp = r->read();
		if ((cp == END_OF_STREAM) || (cp == BAD_FORMAT))
		{
			w->closure();
			break;
		}

		if ( SUCCESS != w->write(cp) )
			break;

		cnt++;
	}

	delete r;
	delete w;
	return cnt;
}

};
};


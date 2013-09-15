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

#ifndef _XPF_STRING_HEADER_
#define _XPF_STRING_HEADER_

#include "platform.h"
#include <string>
#include <cstdio>
#include <stdarg.h>

#ifdef XPF_PLATFORM_WINDOWS
#pragma warning ( push )
#pragma warning ( disable : 4996 )
#endif

namespace xpf {

template < typename T, typename TAlloc = std::allocator<T> >
class xpfstring : public std::basic_string< T, std::char_traits<T>, TAlloc >
{
public:
	typedef std::basic_string< T, std::char_traits<T>, TAlloc > super_type;

	// Type definitions available from std::basic_string :
	//   value_type    : type of T 
	//   pointer       : type of T*
	//  const_pointer  : type of const T*
	//   reference     : type of T&
	//  const_reference: type of const T&
	//  allocator_type : type of TAlloc

public:
	
	explicit xpfstring (const TAlloc& alloc = TAlloc())
		: super_type(alloc)
	{
	}

	xpfstring (const super_type& str)
		: super_type(str)
	{
	}

	xpfstring (const xpfstring<T, TAlloc>& str)
		: super_type(str)
	{
	}

	xpfstring (const super_type& str, size_type pos, size_type len = npos, const TAlloc& alloc = TAlloc())
		: super_type(str, pos, len, alloc)
	{
	}

	xpfstring (const T* s, const TAlloc& alloc = TAlloc())
		: super_type(s, alloc)
	{
	}

	xpfstring (const T* s, size_type n, const TAlloc& alloc = TAlloc())
		: super_type(s, n, alloc)
	{
	}

	xpfstring (size_type n, T c, const TAlloc& alloc = TAlloc())
		: super_type(n, c, alloc)
	{
	}

	template <typename InputIterator>
	xpfstring  (InputIterator first, InputIterator last, const TAlloc& alloc = TAlloc())
		: super_type(first, last, alloc)
	{
	}

	explicit xpfstring (f64 val)
		: super_type()
	{
		c8 buf[128];
		std::sprintf(buf, "%0.6f", val);
		*this = buf;
	}

	explicit xpfstring (s32 val)
		: super_type()
	{
		bool negative = false;
		if (val < 0)
		{
			val = 0 - val;
			negative = true;
		}

		c8 tmpbuf[16]={0};
		u32 idx = 15;

		if (0 == val)
		{
			tmpbuf[14] = '0';
			*this = &tmpbuf[14];
			return;
		}

		while(val && idx)
		{
			--idx;
			tmpbuf[idx] = (c8)('0' + (val % 10));
			val /= 10;
		}

		if (negative)
		{
			--idx;
			tmpbuf[idx] = '-';
		}

		*this = &tmpbuf[idx];
	}

	explicit xpfstring (u32 val)
		: super_type()
	{
		c8 tmpbuf[16]={0};
		u32 idx = 15;

		if (0 == val)
		{
			tmpbuf[14] = '0';
			*this = &tmpbuf[14];
			return;
		}

		while(val && idx)
		{
			--idx;
			tmpbuf[idx] = (c8)('0' + (val % 10));
			val /= 10;
		}

		*this = &tmpbuf[idx];
	}

	explicit xpfstring (s64 val)
		: super_type()
	{
		bool negative = false;
		if (val < 0)
		{
			val = 0 - val;
			negative = true;
		}

		c8 tmpbuf[32]={0};
		u32 idx = 31;

		if (0 == val)
		{
			tmpbuf[30] = '0';
			*this = &tmpbuf[30];
			return;
		}

		while(val && idx)
		{
			--idx;
			tmpbuf[idx] = (c8)('0' + (val % 10));
			val /= 10;
		}

		if (negative)
		{
			--idx;
			tmpbuf[idx] = '-';
		}

		*this = &tmpbuf[idx];
	}

	explicit xpfstring (u64 val)
		: super_type()
	{
		c8 tmpbuf[32]={0};
		u32 idx = 31;

		if (0 == val)
		{
			tmpbuf[30] = '0';
			*this = &tmpbuf[30];
			return;
		}

		while(val && idx)
		{
			--idx;
			tmpbuf[idx] = (c8)('0' + (val % 10));
			val /= 10;
		}

		*this = &tmpbuf[idx];
	}

	template <typename B>
	xpfstring (const B* const c)
		: super_type()
	{
		*this = c;
	}

	xpfstring<T, TAlloc>& operator= (const super_type& str)
	{
		super_type::operator=(str);
		return *this;
	}

	xpfstring<T, TAlloc>& operator= (const xpfstring<T, TAlloc>& str)
	{
		super_type::operator=(str);
		return *this;
	}

	xpfstring<T, TAlloc>& operator= (const T* s)
	{
		super_type::operator=(s);
		return *this;
	}

	xpfstring<T, TAlloc>& operator= (T c)
	{
		super_type::operator=(c);
		return *this;
	}

	template <typename B>
	xpfstring<T, TAlloc>& operator=(const B* const c)
	{
		if (!c)
		{
			super_type::clear();
			return *this;
		}

		if ((void*)c == (void*)super_type::c_str())
			return *this;

		u32 len = 0;
		const B* p = c;
		do
		{
			++len;
		} while(*p++);

		for (u32 l = 0; l<len; ++l)
			super_type::append(1, (T)c[l]);

		return *this;
	}

	xpfstring<T,TAlloc> make_lower() const
	{
		xpfstring<T,TAlloc> result;
		for( super_type::const_iterator it = begin(); it != end(); ++it )
		{
			T ch = *it;
			c8 ch8 = (c8)ch;
			const c8 diff = 'a' - 'A';
			if ((ch8 >= 'A') && (ch8 <='Z'))
				result.append(1, (T)(ch8 + diff));
			else
				result.append(1, ch);
		}
		return result;
	}

	xpfstring<T,TAlloc> make_upper() const
	{
		xpfstring<T,TAlloc> result;
		for( super_type::const_iterator it = begin(); it != end(); ++it )
		{
			T ch = *it;
			c8 ch8 = (c8)ch;
			const c8 diff = 'a' - 'A';
			if ((ch8 >= 'a') && (ch8 <='z'))
				result.append(1, (T)(ch8 - diff));
			else
				result.append(1, ch);
		}
		return result;
	}

	// Note: Not efficient for large string.
	//       May throw exceptions for bad pos/len.
	bool equals_ignore_case( const super_type& other, s32 pos = 0, s32 len = -1) const
	{
		if (pos < 0)
			return false;

		size_type upos = (size_type)pos;
		size_type ulen = (len < 0)? other.size(): (size_type)len;

		xpfstring<T,TAlloc> str1 = this->make_lower();
		xpfstring<T,TAlloc> str2 = xpfstring<T,TAlloc>(other).make_lower();

		return (0 == str1.compare(upos, ulen, str2));
	}

	// return the trimmed string (remove the given characters at the begin and end of string)
	xpfstring<T,TAlloc> trim(const xpfstring<T,TAlloc>& whitespace = " \t\n\r") const
	{
		size_type begin = super_type::find_first_not_of(whitespace);
		if (begin == super_type::npos)
			return xpfstring<T,TAlloc>();

		size_type end = super_type::find_last_not_of(whitespace);
		if (end < begin)
			return xpfstring<T,TAlloc>();

		return super_type::substr(begin, (end - begin + 1));
	}

	// TODO: ignoreEmptyTokens is not working.
	template <typename Container>
	u32 split(Container& ret, const super_type& delimiter, bool ignoreEmptyTokens=true, bool keepSeparators=false) const
	{
		if (delimiter.empty())
			return 0;

		size_type oldSize = ret.size();
		u32 lastpos = 0;
		bool lastWasSeparator = false;
		for (u32 i=0; i<size(); ++i)
		{
			bool foundSeparator = false;
			for (u32 j=0; j<delimiter.size(); ++j)
			{
				if (at(i) == delimiter[j])
				{
					if ((!ignoreEmptyTokens || ((i - lastpos) != 0)) &&
							!lastWasSeparator)
						ret.push_back(xpfstring<T,TAlloc>(&c_str()[lastpos], i - lastpos));
					foundSeparator = true;
					lastpos = (keepSeparators ? i : i + 1);
					break;
				}
			}
			lastWasSeparator = foundSeparator;
		}
		if ((size() - 1) > lastpos)
			ret.push_back(xpfstring<T,TAlloc>(&c_str()[lastpos], (size() - 1) - lastpos));
		return ret.size() - oldSize;
	}

	u32 printf(const T* format, ...)
	{
		return 0;
	}

	// simple hash algorithm referenced from xerces-c project.
	inline u32 hash(const u32 modulus = 10993UL, const s32 len = -1, const s32 offset = 0) const
	{
		u32 ulen = (len < 0)? size(): (u32)len;
		u32 uoff = (offset < 0)? 0: (u32)offset;
		
		if (uoff + ulen > size())
			return 0;

		const T* curCh = c_str() + uoff;
		u32 hashVal = (u32)(*curCh++);

		for (u32 i=0; i<ulen; ++i)
			hashVal = (hashVal * 38) + (hashVal >> 24) + (u32)(*curCh++);

		return hashVal % modulus;
	}
};

typedef xpfstring< c8>     string;
typedef xpfstring<u16>     u16string;
typedef xpfstring<u32>     u32string;
typedef xpfstring<wchar_t> wstring;

}; // end of namespace xpf

#ifdef XPF_PLATFORM_WINDOWS
#pragma warning ( pop )
#endif

#endif // _XPF_STRING_HEADER_
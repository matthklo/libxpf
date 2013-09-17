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

// NOTE: Header-only impl.

namespace xpf {

template < typename T, typename TAlloc = std::allocator<T> >
class xpfstring : public std::basic_string< T, std::char_traits<T>, TAlloc >
{
public:
	typedef std::basic_string< T, std::char_traits<T>, TAlloc > base_type;

	// Type definitions available from std::basic_string :
	//   value_type    : type of T 
	//   pointer       : type of T*
	//  const_pointer  : type of const T*
	//   reference     : type of T&
	//  const_reference: type of const T&
	//  allocator_type : type of TAlloc

public:
	
	explicit xpfstring (const TAlloc& alloc = TAlloc())
		: base_type(alloc)
	{
	}

	xpfstring (const base_type& str)
		: base_type(str)
	{
	}

	xpfstring (const xpfstring<T, TAlloc>& str)
		: base_type(str)
	{
	}

	xpfstring (const base_type& str, typename base_type::size_type pos, typename base_type::size_type len = base_type::npos, const TAlloc& alloc = TAlloc())
		: base_type(str, pos, len, alloc)
	{
	}

	xpfstring (const T* s, const TAlloc& alloc = TAlloc())
		: base_type(s, alloc)
	{
	}

	xpfstring (const T* s, typename base_type::size_type n, const TAlloc& alloc = TAlloc())
		: base_type(s, n, alloc)
	{
	}

	xpfstring (typename base_type::size_type n, T c, const TAlloc& alloc = TAlloc())
		: base_type(n, c, alloc)
	{
	}

	template <typename InputIterator>
	xpfstring  (InputIterator first, InputIterator last, const TAlloc& alloc = TAlloc())
		: base_type(first, last, alloc)
	{
	}

	explicit xpfstring (f64 val)
		: base_type()
	{
		c8 buf[128];
		std::sprintf(buf, "%0.6f", val);
		*this = buf;
	}

	explicit xpfstring (s32 val)
		: base_type()
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
		: base_type()
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
		: base_type()
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
		: base_type()
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
		: base_type()
	{
		*this = c;
	}

	xpfstring<T, TAlloc>& operator= (const base_type& str)
	{
		base_type::operator=(str);
		return *this;
	}

	xpfstring<T, TAlloc>& operator= (const xpfstring<T, TAlloc>& str)
	{
		base_type::operator=(str);
		return *this;
	}

	xpfstring<T, TAlloc>& operator= (const T* s)
	{
		base_type::operator=(s);
		return *this;
	}

	xpfstring<T, TAlloc>& operator= (T c)
	{
		base_type::operator=(c);
		return *this;
	}

	template <typename B>
	xpfstring<T, TAlloc>& operator=(const B* const c)
	{
		if (!c)
		{
			base_type::clear();
			return *this;
		}

		if ((void*)c == (void*)base_type::c_str())
			return *this;

		u32 len = 0;
		const B* p = c;
		for (; *p != T(); ++p, ++len);

		for (u32 l = 0; l<len; ++l)
			base_type::append(1, (T)c[l]);

		return *this;
	}

	xpfstring<T,TAlloc> make_lower() const
	{
		xpfstring<T,TAlloc> result;
		for( typename base_type::const_iterator it = base_type::begin(); it != base_type::end(); ++it )
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
		for( typename base_type::const_iterator it = base_type::begin(); it != base_type::end(); ++it )
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
	bool equals_ignore_case( const base_type& other, s32 pos = 0, s32 len = -1) const
	{
		if (pos < 0)
			return false;

		typename base_type::size_type upos = (typename base_type::size_type)pos;
		typename base_type::size_type ulen = (len < 0)? other.size(): (typename base_type::size_type)len;

		xpfstring<T,TAlloc> str1 = this->make_lower();
		xpfstring<T,TAlloc> str2 = xpfstring<T,TAlloc>(other).make_lower();

		return (0 == str1.compare(upos, ulen, str2));
	}

	// return the trimmed string (remove the given characters at the begin and end of string)
	xpfstring<T,TAlloc> trim(const xpfstring<T,TAlloc>& whitespace = " \t\n\r") const
	{
		typename base_type::size_type begin = base_type::find_first_not_of(whitespace);
		if (begin == base_type::npos)
			return xpfstring<T,TAlloc>();

		typename base_type::size_type end = base_type::find_last_not_of(whitespace);
		if (end < begin)
			return xpfstring<T,TAlloc>();

		return base_type::substr(begin, (end - begin + 1));
	}

	// TODO: ignoreEmptyTokens is not working.
	template <typename Container>
	u32 split(Container& ret, const base_type& delimiter, bool ignoreEmptyTokens=true, bool keepSeparators=false) const
	{
		if (delimiter.empty())
			return 0;

		typename base_type::size_type oldSize = ret.size();
		u32 lastpos = 0;
		bool lastWasSeparator = false;
		for (u32 i=0; i<base_type::size(); ++i)
		{
			bool foundSeparator = false;
			for (u32 j=0; j<delimiter.size(); ++j)
			{
				if (base_type::at(i) == delimiter[j])
				{
					if ((!ignoreEmptyTokens || ((i - lastpos) != 0)) &&
							!lastWasSeparator)
						ret.push_back(xpfstring<T,TAlloc>(&base_type::c_str()[lastpos], i - lastpos));
					foundSeparator = true;
					lastpos = (keepSeparators ? i : i + 1);
					break;
				}
			}
			lastWasSeparator = foundSeparator;
		}
		if ((base_type::size() - 1) > lastpos)
			ret.push_back(xpfstring<T,TAlloc>(&base_type::c_str()[lastpos], (base_type::size() - 1) - lastpos));
		return ret.size() - oldSize;
	}


	//  * Simplified version, can only handle %d, %lld, %u, %llu, %x, %llx, %X, %llX, %f, %s, %%.
	//    CAN NOT handle percision nor width specification such as %20.20s.
	//  * Returns the final length of resulted string.
	u32 printf(const T* format, ...)
	{
		va_list ap;
		va_start(ap, format);

		base_type::clear();

		const char *hextable = "0123456789abcdef";
		int state = 0;

		for (const T* p = format; *p != T(); ++p)
		{
			char c = (char)(*p);

			switch (state)
			{
			case 0:
				if ('%' == c)
					state++;
				else
					base_type::append(1, *p);
				break;
			case 1:
				switch(c)
				{
				case 'c':
					{
						// care about self-promoting types
						int v = va_arg(ap, int);
						T vv = (T)v;
						base_type::append(1, vv);
					}
					state=0;
					break;
				case '%':
					base_type::append(1, (T)'%');
					state=0;
					break;
				case 'l':
					state++;
					break;
				case 'd':
					{
						int v = va_arg(ap, int);
						base_type::append(xpfstring<T,TAlloc>(v));
					}
					state=0;
					break;
				case 'u':
					{
						unsigned int v = va_arg(ap, unsigned int);
						base_type::append(xpfstring<T,TAlloc>(v));
					}
					state=0;
					break;
				case 'x':
				case 'X':
					{
						xpfstring<T,TAlloc> hex;
						unsigned int v = va_arg(ap, unsigned int);
						hex.append(1, (T)hextable[(v >> 28) & 0xF]);
						hex.append(1, (T)hextable[(v >> 24) & 0xF]);
						hex.append(1, (T)hextable[(v >> 20) & 0xF]);
						hex.append(1, (T)hextable[(v >> 16) & 0xF]);
						hex.append(1, (T)hextable[(v >> 12) & 0xF]);
						hex.append(1, (T)hextable[(v >>  8) & 0xF]);
						hex.append(1, (T)hextable[(v >>  4) & 0xF]);
						hex.append(1, (T)hextable[v & 0xF]);

						base_type::append(('X' == c)? hex.make_upper(): hex);
					}
					state=0;
					break;
				case 'f':
					{
						// va_args promote all float to double, so always use double to hold data.
						double v = va_arg(ap, double);
						base_type::append(xpfstring<T,TAlloc>(v));
					}
					state=0;
					break;
				case 's':
					{
						T* v = va_arg(ap, T*);
						while (*v != T())
						{
							base_type::append(1, *v);
							v++;
						}
					}
					state=0;
					break;
				default:
					state=0;
					break;
				}
				break;
			case 2:
				if ('l' == c)
					state++;
				else
					state=0;
				break;
			case 3:
				switch (c)
				{
				case 'd':
					{
						long long v = va_arg(ap, long long);
						base_type::append(xpfstring<T,TAlloc>(v));
					}
					break;
				case 'u':
					{
						unsigned long long v = va_arg(ap, unsigned long long);
						base_type::append(xpfstring<T,TAlloc>(v));
					}
					break;
				case 'x':
				case 'X':
					{
						xpfstring<T,TAlloc> hex;
						unsigned long long v = va_arg(ap, unsigned long long);
						hex.append(1, (T)hextable[(v >> 60) & 0xF]);
						hex.append(1, (T)hextable[(v >> 56) & 0xF]);
						hex.append(1, (T)hextable[(v >> 52) & 0xF]);
						hex.append(1, (T)hextable[(v >> 48) & 0xF]);
						hex.append(1, (T)hextable[(v >> 44) & 0xF]);
						hex.append(1, (T)hextable[(v >> 40) & 0xF]);
						hex.append(1, (T)hextable[(v >> 36) & 0xF]);
						hex.append(1, (T)hextable[(v >> 32) & 0xF]);
						hex.append(1, (T)hextable[(v >> 28) & 0xF]);
						hex.append(1, (T)hextable[(v >> 24) & 0xF]);
						hex.append(1, (T)hextable[(v >> 20) & 0xF]);
						hex.append(1, (T)hextable[(v >> 16) & 0xF]);
						hex.append(1, (T)hextable[(v >> 12) & 0xF]);
						hex.append(1, (T)hextable[(v >>  8) & 0xF]);
						hex.append(1, (T)hextable[(v >>  4) & 0xF]);
						hex.append(1, (T)hextable[v & 0xF]);

						base_type::append(('X' == c)? hex.make_upper(): hex);
					}
					break;
				default:
					break;
				}
				state=0;
				break;
			default:
				state=0;
				break;
			}
		}
		va_end(ap);
		return base_type::size();
	}

	// simple hash algorithm referenced from xerces-c project.
	inline u32 hash(const u32 modulus = 10993UL, const s32 len = -1, const s32 offset = 0) const
	{
		u32 ulen = (len < 0)? base_type::size(): (u32)len;
		u32 uoff = (offset < 0)? 0: (u32)offset;
		
		if (uoff + ulen > base_type::size())
			return 0;

		const T* curCh = base_type::c_str() + uoff;
		u32 hashVal = (u32)(*curCh++);

		for (u32 i=0; i<ulen; ++i)
			hashVal = (hashVal * 38) + (hashVal >> 24) + (u32)(*curCh++);

		return hashVal % modulus;
	}

	bool begin_with( const xpfstring<T,TAlloc>& str, bool ignoreCase = false ) const
	{
		if (base_type::size() < str.size())
			return false;
		xpfstring<T, TAlloc> snip = substr(0, str.size());
		if (ignoreCase)
		{
			return snip.equals_ignore_case(str);
		}
		return (snip == str);
	}

	bool end_with( const xpfstring<T,TAlloc>& str, bool ignoreCase = false ) const
	{
		if (base_type::size() < str.size())
			return false;
		xpfstring<T, TAlloc> snip = substr( base_type::size() - str.size(), str.size() );
		if (ignoreCase)
		{
			return snip.equals_ignore_case(str);
		}
		return (snip == str);
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


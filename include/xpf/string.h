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

#if defined(XPF_PLATFORM_WINDOWS) && defined(XPF_COMPILER_MSVC)
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


	/*  A nearly full implementation of standard printf() -
	 *  http://www.cplusplus.com/reference/cstdio/printf/
	 *  with few small tweaks:
	 *  Specifiers:
	 *      Supported: 'diuoxXfcsp%'
	 *      Fallback: 'FeEgGaA' fallback to be 'f'
	 *      Not supported: 'n'   (neither does libc on linux)
	 *  Length sub-specifiers:
	 *      Supported: 'hh', 'h', 'l', 'll'
	 *      Ignored: 'j', 'z', 't', 'L'  (just ignored, using them won't cause any error)
	 *      Tweak: Specifier 'c' and 's' will not take hint from length sub-specifier 
	 *             for the type of character. It just use the template parameter type T of
	 *             this class. Which means you can use %c/%s for both char and wchar_t
	 *             version (instead of %c/%s for char and %lc/%ls for wchar_t, which is
	 *             required by the printf() of libc, yuck!!).
	 *  Flags, Width, Percision:
	 *      Fully supported.
	 */
	u32 printf(const T* format, ...)
	{
		va_list ap;
		va_start(ap, format);

		base_type::clear();

		enum
		{
			ST_TEXT = 0,
			ST_FLAGS,
			ST_WIDTH,
			ST_PERCISION,
			ST_SUBSPEC,
			ST_SPEC,
		};

		enum
		{
			FLAG_LEFTJUSTIFY = 0x1,
			FLAG_FORCESIGN = 0x2,
			FLAG_BLANKSIGN = 0x4,
			FLAG_PREFIXSUFFIX = 0x8,
			FLAG_LEFTPADZERO = 0x10,

			FLAG_WIDTH_SPECIFIED = 0x100,
			FLAG_WIDTH_PARAM = 0x200,
			FLAG_PERCISION_SPECIFIED = 0x400,
			FLAG_PERCISION_PARAM = 0x800,

			FLAG_FMT_LONG     = 0x1000,
			FLAG_FMT_LONGLONG = 0x2000,
			FLAG_FMT_HALF     = 0x4000,
			FLAG_FMT_HALFHALF = 0x8000,
		};

		const char *hextable = "0123456789abcdef";
		int state = ST_TEXT;
		int width = 0;
		int percision = 0;
		unsigned short flag = 0;

		for (const T* p = format; *p != T(); ++p)
		{
			char c = (char)(*p);

			switch (state)
			{
            case ST_TEXT:
				if ('%' == c)
				{
					width = percision = 0;
					flag = 0;
					state = ST_FLAGS;
				}
				else
				{
					base_type::append(1, *p);
				}
                break;

			case ST_FLAGS:
				switch (c)
				{
				case '-':
					flag |= FLAG_LEFTJUSTIFY; break;
				case '+':
					flag |= FLAG_FORCESIGN; flag &= (~FLAG_BLANKSIGN); break;
				case ' ':
					flag |= FLAG_BLANKSIGN; flag &= (~FLAG_FORCESIGN); break;
				case '#':
					flag |= FLAG_PREFIXSUFFIX; break;
				case '0':
					flag |= FLAG_LEFTPADZERO; break;
				default:
					p--; state = ST_WIDTH; break;
				}
				break;

			case ST_WIDTH:
				if ((c >= '0') && (c <= '9'))
				{
					flag |= FLAG_WIDTH_SPECIFIED;
					if (width < 0) width = 0;
					width = width * 10 + (c - '0');
				}
				else if (c == '*')
				{
					flag |= FLAG_WIDTH_SPECIFIED;
					flag |= FLAG_WIDTH_PARAM;
				}
				else if (c == '.')
				{
					flag |= FLAG_PERCISION_SPECIFIED;
					state=ST_PERCISION;
				}
				else
				{
					p--; state = ST_SUBSPEC;
				}
				break;

			case ST_PERCISION:
				if ((c >= '0') && (c <= '9'))
				{
					if (percision < 0) percision = 0;
					percision = percision * 10 + (c - '0');
				}
				else if (c == '*')
				{
					flag |= FLAG_PERCISION_PARAM;
				}
				else
				{
					p--; state = ST_SUBSPEC;
				}
				break;

			case ST_SUBSPEC:
				// http://www.cplusplus.com/reference/cstdio/printf/
				// The sub-specifiers we're currently support is 'll', 'l', 'h', and 'hh', others are ignored.
				switch(c)
				{
				case 'l':
					if (*(p+1) == 'l')
					{
						flag |= FLAG_FMT_LONGLONG;
						p++;
					}
					else
					{
						flag |= FLAG_FMT_LONG;
					}
					break;

				case 'h':
					if (*(p+1) == 'h')
					{
						flag |= FLAG_FMT_HALFHALF;
						p++;
					}
					else
					{
						flag |= FLAG_FMT_HALF;
					}
					break;

				case 'j':
				case 'z':
				case 't':
				case 'L':
					// not yet implement
					break;

				default:
					p--;
					state = ST_SPEC;
					break;
				}
				break;

			case ST_SPEC:
                switch(c)
                {
// Handy macro to conditionally load width and percision arguments from va_args
#define _READ_PARAMS \
	if (flag & FLAG_WIDTH_PARAM)\
		width = va_arg(ap, int);\
	if (flag & FLAG_PERCISION_PARAM) \
		percision = va_arg(ap, int);

                case 'c':
                    {
						// Note: For specifier 'c', we ignore any sub-specifier (i.e., 'l', 'h', etc.).
						//       Instead using %lc for wchar_t (required for standard libc), usage of %c is also allowed.
						_READ_PARAMS;

						int prepad = 0;
						int postpad = 0;
						if (width > 1)
						{
							if (flag & FLAG_LEFTJUSTIFY)
								postpad = width - 1;
							else
								prepad = width - 1;
						}

						if (prepad > 0)
							base_type::append(prepad, (T)' ');

						// per C99 standard: char, short will be promoted to int when passing through '...'
						// http://www.gnu.org/software/libc/manual/html_node/Argument-Macros.html#Argument-Macros
						int v = va_arg(ap, int);
						if (v != 0) // never append internal null
							base_type::append(1, (T)v);

						if (postpad > 0)
							base_type::append(postpad, (T)' ');
                    }
                    state = ST_TEXT;
                    break;

                case '%':
					{
						_READ_PARAMS;
						base_type::append(1, (T)'%');
						state = ST_TEXT;
					}
                    break;

				case 'p':
					// Keep FLAG_WIDTH_PARAM and FLAG_PERCISION_PARAM but clear all other flag bits to 0.
					// Add FLAG_WIDTH_SPECIFIED, and FLAG_PREFIXSUFFIX.
					flag = (flag & (FLAG_WIDTH_SPECIFIED | FLAG_WIDTH_PARAM | FLAG_PERCISION_SPECIFIED | FLAG_PERCISION_PARAM)) | FLAG_PERCISION_SPECIFIED | FLAG_PREFIXSUFFIX;

					if (percision < 8)
						percision = 8;
					if (sizeof(void*) > 4) // FIXME: better way to detect whether the host use a 64-bit pointer type?
					{
						if (percision < 16)
							percision = 16;
						flag |= FLAG_FMT_LONGLONG;
					}

					// fall-through ...
					c = 'x';

				case 'F': // all these specifiers are modeled as 'f'
				case 'g':
				case 'G':
				case 'e':
				case 'E':
				case 'A':
				case 'a':
					if (c != 'x')
						c = 'f';
					// fall-through ...

				case 'f':
				case 'x':
				case 'X':
				case 'o':
                case 'd':
				case 'i':
                case 'u':
                    {
						_READ_PARAMS;

						const bool isSigned = ((c == 'd') || (c == 'i'));
						const bool isHex = ((c == 'x') || (c == 'X'));
						const bool isOct = (c == 'o');
						const bool isFloat = (c == 'f');

						// Read param from va_arg with the type hint by sub-specifier.
						// Apply necessary convertion. (i.e. to hex, to oct, or to integer)
						xpfstring<T,TAlloc> valstr;
						if (flag & FLAG_FMT_LONGLONG) // deal with 8-bytes value.
						{
							if (isSigned) // Signed 8-bytes integer
							{
								valstr = xpfstring<T,TAlloc>((long long)va_arg(ap, long long));
							}
							else if (isHex) // Unsigned 8-bytes hex.
							{
								unsigned long long v = va_arg(ap, unsigned long long);
								bool leadingZero = true;
								for (int i=0; i<16; ++i)
								{
									const T digit = (T)hextable[(v >> (64 - (4*(i+1)))) & 0xF];
									if (leadingZero && digit == (T)'0')
										continue;
									leadingZero = false;
									valstr.append(1, digit);
								}
								if (valstr.empty())
									valstr.append(1, (T)'0');

								if ('X' == c )
									valstr = valstr.make_upper();
							}
							else if (isOct) // Unsigned 8-bytes oct.
							{
								unsigned long long v = va_arg(ap, unsigned long long);
								bool leadingZero = true;
								for (int i=0; i<22; ++i)
								{
									const T digit = (T)hextable[(v >> (66 - (3*(i+1)))) & 0x7];
									if (leadingZero && digit == (T)'0')
										continue;
									leadingZero = false;
									valstr.append(1, digit);
								}
								if (valstr.empty())
									valstr.append(1, (T)'0');
							}
							else // Unsigned 8-bytes integer
							{
								valstr = xpfstring<T,TAlloc>((unsigned long long)va_arg(ap, unsigned long long));
							}
						}
						else // deal with 4-bytes value. (char, short, etc. will be prompted to be 4 bytes)
						{
							if (isFloat) // 4-bytes double/float
							{
								double v = va_arg(ap, double);
								valstr = xpfstring<T,TAlloc>(v);
							}
							else if (isSigned) // Signed 4-bytes integer
							{
								int v = (int)va_arg(ap, int);
								// Shrink to valid value width according to sub-specifiers
								if (flag & FLAG_FMT_HALF)
									v &= 0xFFFF;
								else if (flag & FLAG_FMT_HALFHALF)
									v &= 0xFF;
								valstr = xpfstring<T,TAlloc>(v);
							}
							else // Unsigned 4-bytes integer/hex/oct
							{
								unsigned int v = (unsigned int)va_arg(ap, unsigned int);
								// Shrink to valid value width according to sub-specifiers
								if (flag & FLAG_FMT_HALF)
									v &= 0xFFFF;
								else if (flag & FLAG_FMT_HALFHALF)
									v &= 0xFF;

								if (isHex) // Deal with 4-bytes hex
								{
									bool leadingZero = true;
									for (int i=0; i<8; ++i)
									{
										const T digit = (T)hextable[(v >> (32 - (4*(i+1)))) & 0xF];
										if (leadingZero && digit == (T)'0')
											continue;
										leadingZero = false;
										valstr.append(1, digit);
									}
									if (valstr.empty())
										valstr.append(1, (T)'0');

									if ('X' == c )
										valstr = valstr.make_upper();
								}
								else if (isOct) // Deal with 4-bytes oct
								{
									bool leadingZero = true;
									for (int i=0; i<11; ++i)
									{
										const T digit = (T)hextable[(v >> (33 - (3*(i+1)))) & 0x7];
										if (leadingZero && digit == (T)'0')
											continue;
										leadingZero = false;
										valstr.append(1, digit);
									}
									if (valstr.empty())
										valstr.append(1, (T)'0');
								}
								else // Unsigned 4-bytes integer
								{
									valstr = xpfstring<T,TAlloc>(v);
								}
							}
						}

						const bool neg = (valstr[0] == (T)'-');

						// Neither width specified nor percision, the simplest case.
						if (!(flag & (FLAG_WIDTH_SPECIFIED | FLAG_PERCISION_SPECIFIED)))
						{
							if (isSigned || isFloat)
							{
								if ((flag & FLAG_FORCESIGN) && !neg)
									append(1, (T)'+');
								if ((flag & FLAG_BLANKSIGN) && !neg)
									append(1, (T)' ');

								if (isFloat && (flag & FLAG_PREFIXSUFFIX))
								{
									if (valstr.find_first_of((T)'.') == base_type::npos)
										valstr.append(1, (T)'.');
								}
							}

							if ((isHex || isOct) && (flag & FLAG_PREFIXSUFFIX))
							{
								if (!((valstr[0]==(T)'0') && (valstr[1]==(T)0)))
								{
									append(1, (T)'0');
									if (c != 'o')
										append(1, (T)c); // append either 'x' or 'X'
								}
							}

							append(valstr);
						}
						else 
						{
							if (flag & (FLAG_PERCISION_SPECIFIED | FLAG_LEFTJUSTIFY))
							{
								// FLAG_LEFTPADZERO will be ignored when either FLAG_PERCISION_SPECIFIED or FLAG_LEFTJUSTIFY present
								flag &= (~FLAG_LEFTPADZERO);
							}

							xpfstring<T,TAlloc> prefix;
							if (isSigned || isFloat)
							{
								if (neg)
									prefix = "-";
								else if (flag & FLAG_FORCESIGN)
									prefix = "+";
								else if (flag & FLAG_BLANKSIGN)
									prefix = " ";
							}
							else if ((isHex || isOct) && (flag & FLAG_PREFIXSUFFIX))
							{
								prefix = "0";
								if (isHex)
									prefix.append(1, (T)c);
							}

							if (flag & FLAG_PERCISION_SPECIFIED)
							{
								do
								{
									// Per spec: If percision is 0, do not output any digit when val is also 0.
									if (!isFloat && (percision == 0) && (valstr[0]==(T)'0') && (valstr[1]==(T)0))
									{
										valstr = "";
										if (isSigned)
										{
											// Note: both msvc and gcc still output sign (either '+' or ' ') for such case.
											if (flag & FLAG_FORCESIGN)
												valstr = "+";
											else if (flag & FLAG_BLANKSIGN)
												valstr = " ";
										}
										break;
									}

									if (neg)
										valstr = valstr.substr(1);

									if (isFloat) // trim or pad for float values
									{
										size_t idx = valstr.find_first_of((T)'.');
										int padcnt = 0;
										if (idx == base_type::npos) // no '.' in content, only possible to pad
										{
											if ((percision > 0) || (flag & FLAG_PREFIXSUFFIX))
											{
												valstr.append(1, (T)'.');
												padcnt = percision;
											}
										}
										else // trim or pad
										{
											// trim
											int restcnt = (int)valstr.size() - (int)idx - 1;
											if (restcnt > percision)
											{
												// if percision is 0, only show tailing '.' if FLAG_PREFIXSUFFIX has been set. 
												if ((percision == 0) && !(flag & FLAG_PREFIXSUFFIX))
													valstr = valstr.substr(0, (int)idx);
												else
													valstr = valstr.substr(0, (int)idx + percision + 1);
											}
											else
												padcnt = percision - restcnt;
										}

										if (padcnt > 0)
											valstr.append(padcnt, (T)'0');

										valstr = prefix + valstr;
									}
									else // integer based values
									{
										// compute padding count
										const int cnt = (percision > (int)valstr.size())? (percision - (int)valstr.size()): 0;
										if (cnt > 0)
										{
											xpfstring<T,TAlloc> padding;
											padding.append(cnt, (T)'0');
											valstr = prefix + padding + valstr;
										}
										else
											valstr = prefix + valstr;
									}

								} while (0);

								// layout in given width
								const int cnt = (width > (int)valstr.size())? (width - (int)valstr.size()): 0;
								if (flag & FLAG_LEFTJUSTIFY)
								{
									append(valstr);
									if (cnt > 0)
										append(cnt, (T)' ');
								}
								else
								{
									if (cnt > 0)
										append(cnt, (T)' ');
									append(valstr);
								}
							}
							else // width specified but no percision
							{
								if (neg)
									valstr = valstr.substr(1);

								// Per spec: float values has a default percision of 6
								// However, the irrString constructor of the version 
								// which takes a double param already guarantee this.

								const int fulllen = (int)(prefix.size() + valstr.size());
								const int padlen = (width > fulllen)? (width - fulllen): 0;
								if (flag & FLAG_LEFTJUSTIFY)
								{
									append(prefix);
									append(valstr);
									if (padlen > 0)
										append(padlen, (T)' ');
								}
								else if (flag & FLAG_LEFTPADZERO)
								{
									append(prefix);
									if (padlen > 0)
										append(padlen, (T)'0');
									append(valstr);
								}
								else
								{
									if (padlen > 0)
										append(padlen, (T)' ');
									append(prefix);
									append(valstr);
								}
							}
						}
                    }
                    state = ST_TEXT;
                    break;

                case 's':
                    {
						_READ_PARAMS;
						T* v = va_arg(ap, T*);

						// Neither width specified nor percision, the simplest case.
						if (!(flag & (FLAG_WIDTH_SPECIFIED | FLAG_PERCISION_SPECIFIED)))
						{
							int cnt = 0;
							for (T* vp = v; *vp != T(); ++vp, ++cnt);

							if (cnt > 0)
								append(v, cnt);
						}
						else
						{
							// look for the string length (must not exceeds percision, if present)
							int cnt = 0;
							for (T* vp = v; *vp != T(); ++vp, ++cnt)
							{
								if ((flag & FLAG_PERCISION_SPECIFIED) && (cnt >= percision))
								{
									cnt = percision; break;
								}
							}

							int pad = ((flag & FLAG_WIDTH_SPECIFIED) && (width > cnt))? (width - cnt): 0;
							if (flag & FLAG_LEFTJUSTIFY)
							{
								append(v, cnt);
								if (pad > 0)
									append(pad, (T)' ');
							}
							else
							{
								if (pad > 0)
									append(pad, (T)' ');
								append(v, cnt);
							}
						}
                    }
                    state = ST_TEXT;
                    break;

                default:
                    state = ST_TEXT;
                    break;
                }
                break;

            default:
                state = ST_TEXT;
                break;
            }

#undef _READ_PARAMS

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

#if defined(XPF_PLATFORM_WINDOWS) && defined(XPF_COMPILER_MSVC)
#pragma warning ( pop )
#endif

#endif // _XPF_STRING_HEADER_


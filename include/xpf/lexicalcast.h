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

#ifndef _XPF_LEXICALCAST_HEADER_
#define _XPF_LEXICALCAST_HEADER_

#include "platform.h"
#include "string.h"
#include <cstdio>
#include <typeinfo>

#if defined(XPF_PLATFORM_WINDOWS) && defined(XPF_COMPILER_MSVC)
#pragma warning ( push )
#pragma warning ( disable : 4996 4800 )
#endif

namespace xpf {

	/****
	 *  Convert either value to string or string to value.
	 *  Ex:
	 *        wstring text = L"123.456";
	 *        double v = lexical_cast<double>(text);  // string to value
	 *
	 *        string text2 = lexical_cast<c8>(v); // value to string
	 *
	 *  Note:
	 *        - All 4 string types are supported: c8, u16, u32, wchar_t.
	 *        - Supported value types: bool, c8, u8, s16, u16, s32, u32, s64, u64, f32, f64.
	 *        - Use libc to do the actual conversion, so might have porting issues.
	 */

	template < typename TargetType, typename SChar, typename SAlloc >
	TargetType lexical_cast (const xpfstring< SChar, SAlloc >& src)
	{
		// ensure dealing with text in c8.
		xpfstring<c8> srcText;
		srcText = src.c_str();

		if ((typeid(bool) == typeid(TargetType)) ||
			(typeid(c8) == typeid(TargetType)) ||
			(typeid(s16) == typeid(TargetType)) ||
			(typeid(s32) == typeid(TargetType)) )
		{
			s32 buf;
			std::sscanf(srcText.c_str(), "%ld", &buf);
			return (TargetType) buf;
		}
		else if ((typeid(u8) == typeid(TargetType)) ||
				 (typeid(u16) == typeid(TargetType)) ||
				 (typeid(u32) == typeid(TargetType)))
		{
			u32 buf;
			std::sscanf(srcText.c_str(), "%lu", &buf);
			return (TargetType) buf;
		}
		else if (typeid(s64) == typeid(TargetType))
		{
			s64 buf;
			std::sscanf(srcText.c_str(), "%lld", &buf);
			return (TargetType) buf;
		}
		else if (typeid(u64) == typeid(TargetType))
		{
			u64 buf;
			std::sscanf(srcText.c_str(), "%llu", &buf);
			return (TargetType) buf;
		}
		else if (typeid(f32) == typeid(TargetType))
		{
			f32 buf;
			std::sscanf(srcText.c_str(), "%f", &buf);
			return (TargetType) buf;
		}
		else if (typeid(f64) == typeid(TargetType))
		{
			f64 buf;
			std::sscanf(srcText.c_str(), "%lf", &buf);
			return (TargetType) buf;
		}
		
		xpfAssert(false); // unsupported casting target
		return TargetType(); // shoud never reach here.
	}

	template < typename TargetType, typename SChar >
	TargetType lexical_cast (const SChar* src)
	{
		xpfstring<SChar> srcText = src;
		return lexical_cast<TargetType>(srcText);
	}

	template < typename TargetType, typename ValueType >
	xpfstring<TargetType> lexical_cast ( const ValueType& val )
	{
		xpfstring<TargetType> result;

		if ((typeid(c8) == typeid(ValueType)) ||
			(typeid(s16) == typeid(ValueType)) ||
			(typeid(s32) == typeid(ValueType)) )
		{
			xpfstring<TargetType> format("%ld");
			result.printf(format.c_str(), (s32)val);
		}
		else if ((typeid(u8) == typeid(ValueType)) ||
				 (typeid(u16) == typeid(ValueType)) ||
				 (typeid(u32) == typeid(ValueType)))
		{
			xpfstring<TargetType> format("%lu");
			result.printf(format.c_str(), (u32)val);
		}
		else if (typeid(s64) == typeid(ValueType))
		{
			xpfstring<TargetType> format("%lld");
			result.printf(format.c_str(), (s64)val);
		}
		else if (typeid(u64) == typeid(ValueType))
		{
			xpfstring<TargetType> format("%llu");
			result.printf(format.c_str(), (u64)val);
		}
		else if (typeid(f32) == typeid(ValueType) ||
				 typeid(f64) == typeid(ValueType))
		{
			xpfstring<TargetType> format("%f");
			result.printf(format.c_str(), val);
		}
		else if (typeid(bool) == typeid(ValueType))
		{
			result = (val)? "true" : "false";
		}
		else
		{
			xpfAssert(false);
		}

		return result;
	}

}; // end of namespace xpf

#if defined(XPF_PLATFORM_WINDOWS) && defined(XPF_COMPILER_MSVC)
#pragma warning ( pop )
#endif

#endif // _XPF_LEXICALCAST_HEADER_


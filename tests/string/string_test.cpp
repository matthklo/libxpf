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
#include <stdio.h>
#include <vector>

#ifdef XPF_PLATFORM_WINDOWS
// http://msdn.microsoft.com/en-us/library/vstudio/x98tx3cf.aspx
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif

using namespace xpf;


int main()
{
#ifdef XPF_PLATFORM_WINDOWS
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

	wstring str1 = L"The elements of the C language library are also included as a subset of the C++ Standard library. \x6b50\x7279\x514b\x651c\x624b\x5de8\x5320\x96fb\x8166\x9996\x5ea6\x63a8\x51fa\x004d\x006f\x006c\x0064\x0066\x006c\x006f\x0077\x8a8d\x8b49\x0020\x0020\x63d0\x5347\x81fa\x7063\x0049\x0043\x0054\x7522\x696d\x7af6\x722d\x529b";
	
	// apply a chain of conversions ...
	u32string str2 = to_ucs4(str1);
	string str3    = to_utf8(str2);
	u16string str4 = to_ucs2(str3);
	string str5    = to_utf8(str4);
	wstring str6   = to_wchar(str5);

	// check if the final output is identical to the original string
	xpfAssert((str3 == str5));
	xpfAssert((str6 == str1));
	xpfAssert((str3.hash() == str5.hash()));
	xpfAssert((str6.hash() == str1.hash()));
	xpfAssert((str3.hash() != str1.hash()));


	// test the printf function
	float fval = 12345.625f;
	double dval = 54321.5725;

	const char* format = "abc %f %s %%(%c) test %f @#$^& %%(%X) haha";
	const char* str = "%%%ddd%%%";
	const char ch = '%';
	const char* validOutput = "abc 12345.625000 %%%ddd%%% %(%) test 54321.572500 @#$^& %(FEE1DEAD) haha";

	{
		string utf8fmt = format;
		string utf8str = str;
		string utf8vo = validOutput;
		c8 utf8ch = (c8)ch;
		string buf;
		buf.printf(utf8fmt.c_str(), fval, utf8str.c_str(), utf8ch, dval, 0xfee1dead);
		xpfAssert((buf == utf8vo));
		xpfAssert((buf.hash() == utf8vo.hash()));
		xpfAssert((buf.hash() != utf8fmt.hash()));
	}

	{
		u16string u16fmt = format;
		u16string u16str = str;
		u16string u16vo = validOutput;
		u16 u16ch = (u16)ch;
		u16string buf;
		buf.printf(u16fmt.c_str(), fval, u16str.c_str(), u16ch, dval, 0xfee1dead);
		xpfAssert((buf == u16vo));
		xpfAssert((buf.hash() == u16vo.hash()));
		xpfAssert((buf.hash() != u16fmt.hash()));
	}

	{
		u32string u32fmt = format;
		u32string u32str = str;
		u32string u32vo = validOutput;
		u32 u32ch = (u32)ch;
		u32string buf;
		buf.printf(u32fmt.c_str(), fval, u32str.c_str(), u32ch, dval, 0xfee1dead);
		xpfAssert((buf == u32vo));
		xpfAssert((buf.hash() == u32vo.hash()));
		xpfAssert((buf.hash() != u32fmt.hash()));
	}

	{
		wstring wfmt = format;
		wstring wstr = str;
		wstring wvo = validOutput;
		wchar_t wch = (wchar_t)ch;
		wstring buf;
		buf.printf(wfmt.c_str(), fval, wstr.c_str(), wch, dval, 0xfee1dead);
		xpfAssert((buf == wvo));
		xpfAssert((buf.hash() == wvo.hash()));
		xpfAssert((buf.hash() != wfmt.hash()));
	}


	// test split
	{
		string buf = validOutput;
		std::vector< xpf::string > c;
		u32 cnt = buf.split(c, " %");
		xpfAssert((cnt == 10));
	}


	printf("string test pass.\n");
	return 0;
}

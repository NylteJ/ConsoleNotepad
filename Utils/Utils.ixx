// Utils.ixx
// 一些杂七杂八的函数
module;
#include <unicode/uchar.h>
export module Utils;

import std;

import String;

using namespace std;

export namespace NylteJ
{
	// 判断一个字符是否是两倍宽度的
	constexpr bool IsWideChar(Codepoint codepoint)
	{
		// ASCII 特判
        if (codepoint <= 0x7f)
            return false;

		const auto width = u_getIntPropertyValue(codepoint, UCHAR_EAST_ASIAN_WIDTH);
		return width == U_EA_FULLWIDTH || width == U_EA_WIDE;
	}

	// 获取一个单行且不含 Tab 的字符串的显示长度
	constexpr size_t GetDisplayLength(StringView str)
	{
		size_t ret = 0;

		for (auto&& codepoint : str)
			if (IsWideChar(codepoint))
				ret += 2;
			else
				ret++;

		return ret;
	}

	constexpr bool IsASCIIAlpha(integral auto&& chr)
	{
		return (chr >= 'A' && chr <= 'Z') || (chr >= 'a' && chr <= 'z');
	}

	template<typename ElementType, typename... Args>
	constexpr array<ElementType, sizeof...(Args)> MakeArray(Args&&... args)
	{
		return { ElementType{ forward<Args>(args) }... };
	}
	template<typename ElementType, typename... Args>
	constexpr vector<ElementType> MakeVector(Args&&... args)
	{
		return { ElementType{ forward<Args>(args) }... };
	}
}
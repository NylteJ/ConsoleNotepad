// Utils.ixx
// 一些杂七杂八的函数
export module Utils;

import std;

using namespace std;

export namespace NylteJ
{
	// 判断一个字符是否是两倍宽度的
	constexpr bool IsWideChar(wchar_t chr)
	{
		if (chr <= 127)
			return false;
		if (chr == L'“' || chr == L'”')
			return false;

		return true;
	}
}
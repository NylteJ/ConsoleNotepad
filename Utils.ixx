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

	// 获取一个单行且不含 Tab 的字符串的显示长度
	constexpr size_t GetDisplayLength(wstring_view str)
	{
		return str.size() + ranges::count_if(str, IsWideChar);
	}
}
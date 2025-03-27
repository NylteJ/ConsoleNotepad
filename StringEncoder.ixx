// StringEncoder.ixx
// 最喜欢 C++ 新标准的一期
// C++17 弃用了 std 里的字符串转换功能, 但替代方案又至少要等到 C++26 才有, 悲哀的是 MSVC 目前还没支持, 所以只能用平台相关的方案了
// 因为这个实现方式实在是不优雅, 所以我也就按不优雅地写了
module;
#ifdef _WIN32
#include <Windows.h>
#else
static_assert(false, "Not implemented yet");
#endif
export module StringEncoder;

import std;

using namespace std;

export namespace NylteJ
{
	wstring StrToWStr(string_view str)
	{
		wstring ret;

		size_t retSize = MultiByteToWideChar(CP_UTF8,
			NULL,
			reinterpret_cast<LPCCH>(str.data()),
			str.size(),
			nullptr,
			0);

		ret.resize(retSize);

		MultiByteToWideChar(CP_UTF8,
			NULL,
			reinterpret_cast<LPCCH>(str.data()),
			str.size(),
			ret.data(),
			retSize);

		return ret;
	}
}
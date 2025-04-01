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

import Exceptions;

using namespace std;

export namespace NylteJ
{
	enum class Encoding :size_t
	{
		UTF8,
		GB2312,

		FORCE = 0x114514,

		FirstEncoding = UTF8,
		LastEncoding = GB2312
	};

	Encoding GetEncoding(UINT encoding)
	{
		using enum Encoding;

		switch (encoding)
		{
		case CP_UTF8:
			return UTF8;
		case 10008:
			return GB2312;
		default:
			unreachable();
		}
	}
	UINT GetEncoding(Encoding encoding)
	{
		using enum Encoding;

		switch (encoding)
		{
		case UTF8:
			return CP_UTF8;
		case GB2312:
			return 10008;
		default:
			unreachable();
		}
	}

	wstring StrToWStr(string_view str, Encoding encoding = Encoding::UTF8, bool force = false)
	{
		wstring ret;

		auto encodingUINT = GetEncoding(encoding);

		auto flag = force ? NULL : MB_ERR_INVALID_CHARS;

		size_t retSize = MultiByteToWideChar(encodingUINT,
			flag,
			reinterpret_cast<LPCCH>(str.data()),
			str.size(),
			nullptr,
			0);

		ret.resize(retSize);

		bool success = MultiByteToWideChar(encodingUINT,
			flag,
			reinterpret_cast<LPCCH>(str.data()),
			str.size(),
			ret.data(),
			retSize);

		if (!success && GetLastError() == ERROR_NO_UNICODE_TRANSLATION)
			throw WrongEncodingException{ L"错误的文件编码!"s };

		return ret;
	}
	string WStrToStr(wstring_view str, Encoding encoding = Encoding::UTF8)
	{
		string ret;

		auto encodingUINT = GetEncoding(encoding);

		auto flag = WC_ERR_INVALID_CHARS;

		if (encoding == Encoding::GB2312)	// 不然会报 ERROR_INVALID_FLAGS
			flag = NULL;

		size_t retSize = WideCharToMultiByte(encodingUINT,
			flag,
			str.data(),
			str.size(),
			nullptr,
			0,
			nullptr,
			NULL);

		ret.resize(retSize);

		bool success = WideCharToMultiByte(encodingUINT,
			flag,
			str.data(),
			str.size(),
			reinterpret_cast<LPSTR>(ret.data()),
			ret.size(),
			nullptr,
			NULL);

		if (!success && GetLastError() == ERROR_NO_UNICODE_TRANSLATION)
			throw WrongEncodingException{ L"错误的文件编码!"s };

		return ret;
	}
}
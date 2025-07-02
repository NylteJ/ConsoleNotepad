// StringEncoder.ixx
// ��ϲ�� C++ �±�׼��һ��
// C++17 ������ std ����ַ���ת������, ���������������Ҫ�ȵ� C++26 ����, �������� MSVC Ŀǰ��û֧��, ����ֻ����ƽ̨��صķ�����
// ��Ϊ���ʵ�ַ�ʽʵ���ǲ�����, ������Ҳ�Ͱ������ŵ�д��
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
			throw WrongEncodingException{ L"������ļ�����!"s };

		return ret;
	}
	string WStrToStr(wstring_view str, Encoding encoding = Encoding::UTF8)
	{
		string ret;

		auto encodingUINT = GetEncoding(encoding);

		auto flag = WC_ERR_INVALID_CHARS;

		if (encoding == Encoding::GB2312)	// ��Ȼ�ᱨ ERROR_INVALID_FLAGS
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
			throw WrongEncodingException{ L"������ļ�����!"s };

		return ret;
	}
}
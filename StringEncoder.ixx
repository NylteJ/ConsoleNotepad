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
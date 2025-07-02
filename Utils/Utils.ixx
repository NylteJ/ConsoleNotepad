// Utils.ixx
// һЩ�����Ӱ˵ĺ���
export module Utils;

import std;

using namespace std;

export namespace NylteJ
{
	// �ж�һ���ַ��Ƿ���������ȵ�
	constexpr bool IsWideChar(wchar_t chr)
	{
		if (chr <= 127)
			return false;
		if (chr == L'��' || chr == L'��')
			return false;

		return true;
	}

	// ��ȡһ�������Ҳ��� Tab ���ַ�������ʾ����
	constexpr size_t GetDisplayLength(wstring_view str)
	{
		return str.size() + ranges::count_if(str, IsWideChar);
	}
}
export module Utils;

export namespace NylteJ
{
	namespace Utils
	{
		// ����� wide ָ�ַ����, ����˵ȫ��/���
		constexpr bool IsWideChar(wchar_t chr)
		{
			return chr >= 128;
		}
	}
}
export module Utils;

export namespace NylteJ
{
	namespace Utils
	{
		// 这里的 wide 指字符宽度, 或者说全角/半角
		constexpr bool IsWideChar(wchar_t chr)
		{
			return chr >= 128;
		}
	}
}
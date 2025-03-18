// ConsoleTypedef.ixx
// 用于定义一些与控制台相关的类型别名（和类）
export module ConsoleTypedef;

import std;

using namespace std;

export namespace NylteJ
{
	using ConsoleWidth = unsigned short;
	using ConsoleHeight = unsigned short;
	class ConsoleSize
	{
	public:
		ConsoleWidth width;
		ConsoleHeight height;
	};

	using ConsoleXPos = ConsoleWidth;
	using ConsoleYPos = ConsoleHeight;
	class ConsolePosition
	{
	public:
		ConsoleXPos x;
		ConsoleYPos y;
	public:
		ConsolePosition(auto&& x, auto&& y)
			:x(x), y(y)
		{
		}
	};

	class ConsoleRect
	{
	public:
		ConsolePosition leftTop, rightBottom;
	public:
		constexpr auto Width() const
		{
			return rightBottom.x - leftTop.x + 1;
		}
		constexpr auto Height() const
		{
			return rightBottom.y - leftTop.y + 1;
		}

		ConsoleRect(const ConsolePosition& leftTop, const ConsolePosition& rightBottom)
			:leftTop(leftTop), rightBottom(rightBottom)
		{
		}
	};

	class ConsoleColor
	{
	public:
		unsigned short R, G, B;
	public:
		constexpr bool operator==(const ConsoleColor& right) const = default;

		constexpr ConsoleColor(unsigned short R, unsigned short G, unsigned short B)
			:R(R), G(G), B(B)
		{
		}
	};
}
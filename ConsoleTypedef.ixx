// ConsoleTypedef.ixx
// ���ڶ���һЩ�����̨��ص����ͱ��������ࣩ
export module ConsoleTypedef;

import std;

using namespace std;

export namespace NylteJ
{
	using ConsoleWidth = int;
	using ConsoleHeight = int;
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
		ConsolePosition operator+(ConsolePosition right) const
		{
			right.x += x;
			right.y += y;

			return right;
		}
		ConsolePosition operator-(ConsolePosition right) const
		{
			right.x = x - right.x;
			right.y = y - right.y;

			return right;
		}

		bool operator==(const ConsolePosition& right) const = default;

		ConsolePosition(auto&& x, auto&& y)
			:x(static_cast<ConsoleXPos>(x)), y(static_cast<ConsoleYPos>(y))
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
		constexpr auto Contain(ConsolePosition pos) const
		{
			return (pos.x >= leftTop.x && pos.x <= rightBottom.x)
				&& (pos.y >= leftTop.y && pos.y <= rightBottom.y);
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

	enum class Direction
	{
		Left, Right, Up, Down, None
	};
}
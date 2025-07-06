// BasicColors.ixx
// 预先定义好的一系列颜色
export module BasicColors;

import ConsoleTypedef;

export namespace NylteJ
{
	namespace BasicColors
	{
		constexpr ConsoleColor black			{ 0, 0, 0 };

		constexpr ConsoleColor red				{ 190, 0, 0 };
		constexpr ConsoleColor brightRed		{ 255, 0, 0 };

		constexpr ConsoleColor green			{ 0, 190, 0 };
		constexpr ConsoleColor brightGreen		{ 0, 255, 0 };

		constexpr ConsoleColor blue				{ 0, 0, 190 };
		constexpr ConsoleColor brightBlue		{ 0, 0, 255 };

		constexpr ConsoleColor cyan				{ 0, 190, 190 };
		constexpr ConsoleColor brightCyan		{ 0, 255, 255 };

		constexpr ConsoleColor purple			{ 190, 0, 190 };
		constexpr ConsoleColor brightPurple		{ 255, 0, 255 };

		constexpr ConsoleColor yellow			{ 190, 190, 0 };
		constexpr ConsoleColor brightYellow		{ 255, 255, 0 };

		constexpr ConsoleColor white			{ 190, 190, 190 };
		constexpr ConsoleColor brightWhite		{ 255, 255, 255 };

		constexpr ConsoleColor stayOldColor		{ static_cast<unsigned short>(-1),
												  static_cast<unsigned short>(-1),
											      static_cast<unsigned short>(-1) };

		constexpr ConsoleColor inverseColor		{ static_cast<unsigned short>(-2),
												  static_cast<unsigned short>(-2),
												  static_cast<unsigned short>(-2) };
	}
}
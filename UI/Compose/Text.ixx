// Text.ixx
// 这个 Compose 和那个 Compose 不是一个东西(
// 其实一开始是想模仿一个 Compose 的, 但很多机制实现起来很麻烦而且没必要, 所以改成这样了
// 和一般的 Component 不同, 可以看作轻量级的组件, 以函数形式调用
export module Compose.Text;

import std;

import ConsoleHandler;
import String;
import ConsoleTypedef;
import Utils;
import BasicColors;

export import Compose.ComposeBase;

using namespace std;

namespace NylteJ::Compose
{
	void TextSingleLine(const ConsoleHandler& console,
						StringView text,
						ConsolePosition pos,
						ConsoleWidth maxWidth,
						Align align,
						const Colors& colors)
	{
		if (maxWidth <= 0)
			return;

		auto endIter = text.begin();
		ConsoleWidth nowWidth = 0;

		for (; nowWidth <= maxWidth && endIter != text.end(); ++endIter)
		{
			if (*endIter == u8'\n' || *endIter == u8'\r')
				TODO();
			else if (*endIter == u8'\t')
				TODO();
			else if (IsWideChar(*endIter))
				nowWidth += 2;
			else
				nowWidth++;
		}

		if (endIter == text.end() && nowWidth == maxWidth)
		{
			console.Print(text, pos, colors);
			return;
		}

		String output;

		if (endIter != text.end() || nowWidth > maxWidth)
		{
			while (endIter != text.begin() && nowWidth + 3 > maxWidth)
			{
				--endIter;

				if (IsWideChar(*endIter))
					nowWidth -= 2;
				else
					nowWidth--;
			}

			const auto pointCount = min(maxWidth - nowWidth, 3);

			output = String{ text.begin(), endIter } + String(u8'.', pointCount);

			nowWidth += pointCount;
		}
		else
			output = String{ text.begin(), endIter };

		switch (align)
		{
			using enum Align;

		case Left:
			output += views::repeat(u8' ', maxWidth - nowWidth);
			break;
		case Right:
			output = String(u8' ', maxWidth - nowWidth) + std::move(output);
			break;
		case Center:
			output = String(u8' ', (maxWidth - nowWidth) / 2)
			+ std::move(output)
			+ String(u8' ', (maxWidth - nowWidth) - ((maxWidth - nowWidth) / 2));
			break;
		}

		console.Print(output, pos, colors);
	}

	// 单行 Text, 会覆写空白处
	export void LineText(const ConsoleHandler& console,
					 StringView text,
					 ConsolePosition position, ConsoleXPos endX,
					 Align align = Align::Left,
					 const Colors& colors = { BasicColors::stayOldColor, BasicColors::stayOldColor })
	{
		TextSingleLine(console, text, position, endX - position.x, align, colors);
	}
}
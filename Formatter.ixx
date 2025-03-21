// Formatter.ixx
// 用于原始字符串的格式化与双向定位
// 简单来说, 就是负责将文件里读的字符串和屏幕上显示的位置一一对应的工具, 缩进的显示方式、自动换行等都是据此实现的
export module Formatter;

import std;

import ConsoleTypedef;

using namespace std;

export namespace NylteJ
{
	// 一个在这里非常有用的 Tip: FormattedString[y][x] 对应 (x, y) 字符, 并且 (x, y) 光标位置位于这个字符前面
	class FormattedString
	{
	public:
		class Line
		{
		public:
			size_t indexInRaw;
			wstring lineData;
		public:
			size_t DisplaySize() const
			{
				auto doubleLengthCount = ranges::count_if(lineData, [](auto&& chr) {return chr > 128; });

				return doubleLengthCount + lineData.size();
			}

			auto& operator[](size_t index)
			{
				return lineData[index];
			}
			const auto& operator[](size_t index) const
			{
				return lineData[index];
			}

			Line(size_t indexInRaw, const wstring& lineData)
				:indexInRaw(indexInRaw), lineData(lineData)
			{
			}
		};
	public:
		vector<Line> datas;
	public:
		Line& operator[](size_t index)
		{
			return datas[index];
		}
		const Line& operator[](size_t index) const
		{
			return datas[index];
		}

		auto& operator[](ConsolePosition index)
		{
			return datas[index.y][index.x];
		}
		const auto& operator[](ConsolePosition index) const
		{
			return datas[index.y][index.x];
		}
	};

	class FormatterBase
	{
	public:
		enum MotivationDirection
		{
			Left, Right, Up, Down, None
		};
	public:
		virtual FormattedString Format(wstring_view rawStr, size_t maxWidth, size_t maxHeight) = 0;

		virtual size_t GetRawIndex(const FormattedString& formattedStr, wstring_view rawStr, ConsolePosition pos) const = 0;

		virtual ConsolePosition RestrictPos(const FormattedString& formattedStr, wstring_view rawStr, ConsolePosition pos, MotivationDirection direction) const = 0;
	};

	class DefaultFormatter :public FormatterBase
	{
		// 没优化, 总之先跑起来再说
		FormattedString Format(wstring_view rawStrView, size_t maxWidth, size_t maxHeight)
		{
			FormattedString ret;

			auto lineBeginIter = rawStrView.begin();
			auto lineEndIter = find(rawStrView.begin(), rawStrView.end(), '\n');

			while (true)
			{
				ret.datas.emplace_back(lineBeginIter - rawStrView.begin(), wstring{ lineBeginIter,lineEndIter });

				if (lineEndIter != rawStrView.begin() && *(lineEndIter - 1) == '\r')
					ret.datas.back().lineData.pop_back();

				if (ret.datas.size() >= maxHeight)
					break;

				if (lineEndIter == rawStrView.end())
					break;

				lineBeginIter = lineEndIter + 1;
				lineEndIter = find(lineEndIter + 1, rawStrView.end(), '\n');
			}

			for (auto& [index, str] : ret.datas)
			{
				size_t tabIndex = str.find('\t');
				while (tabIndex != wstring::npos)
				{
					str.replace_with_range(	str.begin() + tabIndex,
											str.begin() + tabIndex + 1,
											ranges::views::repeat(' ', 4 - tabIndex % 4));

					tabIndex = str.find('\t', tabIndex);
				}

				str = str | ranges::views::take(maxWidth) | ranges::to<wstring>();
			}

			return ret;
		}

		size_t GetRawIndex(const FormattedString& formattedStr, wstring_view rawStr, ConsolePosition pos) const
		{
			size_t nowRawIndex = formattedStr.datas[pos.y].indexInRaw;
			size_t nowX = 0;

			while (nowX < pos.x)
			{
				if (rawStr[nowRawIndex] == '\t')
				{
					nowRawIndex++;
					nowX += 4 - nowX % 4;
				}
				else if (rawStr[nowRawIndex] > 128)
				{
					nowRawIndex++;
					nowX += 2;
				}
				else
				{
					nowRawIndex++;
					nowX++;
				}
			}

			return nowRawIndex;
		}

		ConsolePosition RestrictPos(const FormattedString& formattedStr, wstring_view rawStr, ConsolePosition pos, MotivationDirection direction) const
		{
			// 换行
			if (direction == Left && pos.x < 0 && pos.y > 0)
			{
				pos.y--;
				pos.x = formattedStr[pos.y].DisplaySize();
				direction = None;
			}
			if (direction == Right && pos.x > formattedStr[pos.y].DisplaySize() && pos.y + 1 < formattedStr.datas.size())
			{
				pos.y++;
				pos.x = 0;
				direction = None;
			}

			// 限制到有文字的区域
			if (pos.y < 0)
				pos.y = 0;
			if (pos.y >= formattedStr.datas.size())
				pos.y = formattedStr.datas.size() - 1;
			if (pos.x < 0)
				pos.x = 0;
			if (pos.x > formattedStr[pos.y].DisplaySize())
				pos.x = formattedStr[pos.y].DisplaySize();

			// 限制到与制表符对齐
			if (pos.x % 4 != 0 && rawStr[GetRawIndex(formattedStr, rawStr, pos) - 1] == '\t')
				if (direction == Left)
					pos.x = pos.x / 4 * 4;
				else if (direction == Right)
					pos.x = pos.x / 4 * 4 + 4;
				else
				{
					if (pos.x % 4 <= 1)
						pos.x = pos.x / 4 * 4;
					else
						pos.x = pos.x / 4 * 4 + 4;
				}

			// 限制到与双字节字符对齐
			if (pos.x > 0)
			{
				auto nowRawIndex = GetRawIndex(formattedStr, rawStr, pos);
				if (rawStr[nowRawIndex - 1] > 128 && nowRawIndex == GetRawIndex(formattedStr, rawStr, pos + ConsolePosition{ 1, 0 }))
					if (direction == Right)
						pos.x++;
					else
						pos.x--;
			}

			return pos;
		}
	};
}
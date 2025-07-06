// Formatter.ixx
// 用于原始字符串的格式化与双向定位
// 简单来说, 就是负责将文件里读的字符串和屏幕上显示的位置一一对应的工具, 缩进的显示方式、自动换行等都是据此实现的
// 我真的很努力地重构过了, 但是除了浪费了我好几天时间之外一无所获
// 所以还是愉快地在屎山上拉屎吧
export module Formatter;

import std;

import ConsoleTypedef;
import Utils;
import SettingMap;
import String;
import StringEncoder;

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
			String lineData;
			size_t lineIndex;
		public:
			size_t DisplaySize() const
			{
				size_t ret = 0;

				for (auto&& codepoint : lineData)
					if (IsWideChar(codepoint))
						ret += 2;
					else
						ret++;

				return ret;
			}

			//auto& operator[](size_t index)
			//{
			//	return lineData[index];
			//}
			//const auto& operator[](size_t index) const
			//{
			//	return lineData[index];
			//}

			operator StringView() const
			{
				return lineData;
			}
			operator String() const
			{
				return lineData;
			}

			Line(size_t indexInRaw, const String& lineData, size_t lineIndex)
				:indexInRaw(indexInRaw), lineData(lineData), lineIndex(lineIndex)
			{
			}
		};
	public:
		vector<Line> datas;
		StringView rawStr;	// 务必注意, 原字符串发生扩容/析构等情况时, 该缓存即刻失效
		size_t beginX = 0;
	public:
		Line& operator[](size_t index)
		{
			return datas[index];
		}
		const Line& operator[](size_t index) const
		{
			return datas[index];
		}

		//auto& operator[](ConsolePosition index)
		//{
		//	return datas[index.y][index.x];
		//}
		//const auto& operator[](ConsolePosition index) const
		//{
		//	return datas[index.y][index.x];
		//}
	};

	class FormatterBase
	{
	public:
		virtual size_t GetDisplaySize(StringView str) const = 0;
		virtual size_t GetRawDisplaySize(StringView str) const = 0;

		virtual FormattedString Format(StringView rawStrView, size_t maxWidth, size_t maxHeight, size_t beginX, size_t beginLineIndex) = 0;

		virtual size_t GetRawIndex(const FormattedString& formattedStr, ConsolePosition pos) const = 0;

		virtual size_t GetFormattedIndex(StringView formattedStr, ConsolePosition pos) const = 0;

		virtual ConsolePosition GetFormattedPos(const FormattedString& formattedStr, size_t index) const = 0;

		virtual ConsolePosition RestrictPos(const FormattedString& formattedStr, ConsolePosition pos, Direction direction, bool allowFlow = false) const = 0;

		virtual size_t SearchLineBeginIndex(StringView rawStr, size_t index) const = 0;
		virtual size_t SearchLineEndIndex(StringView rawStr, size_t index) const = 0;

		virtual size_t GetLineIndex(StringView rawStr, size_t index) const = 0;

		virtual ~FormatterBase() = default;
	};

	class DefaultFormatter :public FormatterBase
	{
	private:
		const SettingMap& settingMap;

		size_t GetTabWidth() const
		{
			if (settingMap.Get<SettingID::TabWidth>() > 0)
				return settingMap.Get<SettingID::TabWidth>();
			return 1;
		}
	public:
		size_t GetDisplaySize(StringView str) const
		{
			size_t ret = 0;

			for (auto&& codepoint : str)
				if (IsWideChar(codepoint))
					ret += 2;
				else
					ret++;

			return ret;
		}
		size_t GetRawDisplaySize(StringView str) const
		{
			size_t nowSize = 0;

			for (auto&& chr : str)
			{
				if (chr == '\t')
					nowSize = nowSize / GetTabWidth() * GetTabWidth() + GetTabWidth();
				else if (IsWideChar(chr))
					nowSize += 2;
				else if (chr != '\n' && chr != '\r')
					nowSize++;
			}

			return nowSize;
		}

		// 没优化 (指所有地方都现场重新算 formattedString, 完全不缓存), 总之先跑起来再说
		// 神奇的是, 测试了一下, 居然不会有什么性能瓶颈, 只在一行非常非常长 (KB 级) 的时候才会有明显卡顿
		// 也就是说可以继续不优化, COOOOOOOL!
		FormattedString Format(StringView rawStrView, size_t maxWidth, size_t maxHeight, size_t beginX, size_t beginLineIndex)
		{
			FormattedString ret;

			auto lineBeginIter = rawStrView.begin();
			auto lineEndIter = find(rawStrView.begin(), rawStrView.end(), u8'\n');

			while (true)
			{
				ret.datas.emplace_back(rawStrView.GetByteIndex(lineBeginIter), String{ lineBeginIter,lineEndIter }, beginLineIndex);
				beginLineIndex++;

				if (lineEndIter != rawStrView.begin() && *prev(lineEndIter) == '\r')
					ret.datas.back().lineData.pop_back();

				if (ret.datas.size() >= maxHeight)
					break;

				if (lineEndIter == rawStrView.end())
					break;

				lineBeginIter = next(lineEndIter);
				lineEndIter = find(next(lineEndIter), rawStrView.end(), u8'\n');
			}

			{
				const auto tabWidth = GetTabWidth();

				// 多少还是加了个并行, 也算优化吧
				for_each(execution::par, ret.datas.begin(), ret.datas.end(),
					[&tabWidth, &beginX, &maxWidth](FormattedString::Line& line)
					{
						auto&& str = line.lineData;

						size_t nowWidth = 0;
						bool nowInvisible = beginX > 0;	// 当前是否正在格式化屏幕左侧的不可见部分(这部分宽度总是 beginX)

						size_t nowByteIndex = 0;
						size_t visibleBeginIndex = 0;
						for (; nowByteIndex < str.ByteSize(); nowByteIndex = str.GetByteIndex(next(str.AtByteIndex(nowByteIndex))))
						{
							if (nowInvisible && nowWidth >= beginX)
							{
								// 中英文混排时可能出现截断(beginX 恰好位于汉字中间)
								// 此时要把这个截断的汉字换成空格(终端做不到显示半个汉字)
								if (nowWidth > beginX)
								{
									// 因为我们逐字符扫描, 跳过等于直接大于就意味着宽字符
									const auto prevIter = prev(str.AtByteIndex(nowByteIndex));
									const auto prevIndex = str.GetByteIndex(prevIter);

									str.replace_with_range(prevIter,
														   next(prevIter),
														   u8"  ");

									// 之后迭代器失效, 加上宽度截断问题, 需手动调整 (有点诡异)
									if (nowWidth - beginX != 1)
										unreachable();

									nowByteIndex = str.GetByteIndex(next(str.AtByteIndex(prevIndex), 2));
								}

								nowInvisible = false;
								nowWidth = 0;
								visibleBeginIndex = nowByteIndex;
							}
							if (!nowInvisible && nowWidth >= maxWidth)
								break;

							const auto currentIter = str.AtByteIndex(nowByteIndex);

							if (*currentIter == '\t')
							{
								const auto fullWidth = nowWidth + (nowInvisible ? 0 : beginX);
								const auto spaceWidth = (fullWidth / tabWidth + 1) * tabWidth - fullWidth;

								str.replace_with_range(currentIter,
													   next(currentIter),
													   String(u8' ', spaceWidth));

								nowWidth++;	// 这里加的实际上是原 '\t' 处空格的宽度, 后面多填充的空格留给后面几次循环慢慢加
							}
							else if (IsWideChar(*currentIter))
								nowWidth += 2;
							else
								nowWidth++;
						}

						// (见上) 处理末尾宽字符被截断的情况
						// 不写在里面, 否则宽字符在末尾时会出问题
						if (!nowInvisible && nowWidth > maxWidth)
						{
							const auto prevIter = prev(str.AtByteIndex(nowByteIndex));
							const auto prevIndex = str.GetByteIndex(prevIter);

							str.replace_with_range(prevIter,
												   next(prevIter),
												   u8"  ");

							if (nowWidth - maxWidth != 1)
								unreachable();
						}

						if (nowInvisible)
							str.clear();
						else
							str = String(str.AtByteIndex(visibleBeginIndex), str.AtByteIndex(nowByteIndex));
					});
			}

			ret.rawStr = { rawStrView.begin(),lineEndIter };
			ret.beginX = beginX;

			return ret;
		}

		ConsolePosition GetFormattedPos(const FormattedString& formattedStr, size_t index) const
		{
			if (formattedStr.datas.empty())
				return { 0,0 };

			// 屎山特有的 bug 特判解决法
			if (index == formattedStr.rawStr.ByteSize())
				return { formattedStr.datas.back().DisplaySize(),formattedStr.datas.size() - 1 };

			for (auto iter = formattedStr.datas.begin(); iter != formattedStr.datas.end(); ++iter)
				if (iter->indexInRaw <= index
					&& ((iter + 1 != formattedStr.datas.end() && (iter + 1)->indexInRaw > index)
						|| iter + 1 == formattedStr.datas.end()))
				{
					ConsolePosition pos{ 0,iter - formattedStr.datas.begin() };

					auto nowRawIter = formattedStr.rawStr.AtByteIndex(iter->indexInRaw);
					const auto indexIter = formattedStr.rawStr.AtByteIndex(index);

					while (nowRawIter != indexIter)
					{
						if (*nowRawIter == '\t')
							pos.x += GetTabWidth() - pos.x % GetTabWidth();
						else if (IsWideChar(*nowRawIter))
							pos.x += 2;
						else if (*nowRawIter != '\n' && *nowRawIter != '\r')	// 这俩在 formattedStr 里并不存储
							pos.x++;

						++nowRawIter;
					}

					pos.x -= formattedStr.beginX;

					return pos;
				}

			unreachable();
		}

		size_t GetRawIndex(const FormattedString& formattedStr, ConsolePosition pos) const
		{
			pos.x += formattedStr.beginX;

			size_t nowX = 0;

			auto nowRawIter = formattedStr.rawStr.AtByteIndex(formattedStr.datas[pos.y].indexInRaw);

			while (nowX < pos.x && nowRawIter != formattedStr.rawStr.end())
			{
				if (*nowRawIter == '\t')
					nowX += GetTabWidth() - nowX % GetTabWidth();
				else if (IsWideChar(*nowRawIter))
					nowX += 2;
				else if (*nowRawIter != '\n' && *nowRawIter != '\r')
					nowX++;
				else
					break;	// 跨行, 说明 beginX 需要调整, 当然不会在这里调, 但到这就说明已经可以返回了

				++nowRawIter;
			}

			return formattedStr.rawStr.GetByteIndex(nowRawIter);
		}

		size_t GetFormattedIndex(StringView formattedStr, ConsolePosition pos) const
		{
			size_t nowX = 0;

			auto nowFormattedIter = formattedStr.begin();

			while (nowX < pos.x)
			{
				if (IsWideChar(*nowFormattedIter))
					nowX += 2;
				else
					nowX++;

				++nowFormattedIter;
			}

			return formattedStr.GetByteIndex(nowFormattedIter);
		}

		// 这里的 allowFlow 实际上并不是完全不管溢出, 只是为了和以前的屎山接轨加了个参数
		ConsolePosition RestrictPos(const FormattedString& formattedStr, ConsolePosition pos, Direction direction, bool allowFlow = false) const
		{
			using enum Direction;

			// 限制到有文字的区域
			if (pos.y < 0)
				pos.y = 0;
			if (pos.y >= formattedStr.datas.size())
				pos.y = formattedStr.datas.size() - 1;
			if ((allowFlow && pos.x + static_cast<long long>(formattedStr.beginX) < 0)
				|| (!allowFlow && pos.x < 0))
				pos.x = 0;
			if (pos.x >= 0 && pos.x > formattedStr[pos.y].DisplaySize()
				&& !allowFlow)
				pos.x = formattedStr[pos.y].DisplaySize();

			// 限制到与制表符对齐
			if ((pos.x + formattedStr.beginX) % GetTabWidth() != 0
				&& GetRawIndex(formattedStr, pos) > 0 && *prev(formattedStr.rawStr.AtByteIndex(GetRawIndex(formattedStr, pos))) == '\t')
			{
				const auto nowRawIndex = GetRawIndex(formattedStr, pos);

				if ((direction == Left || (direction == None && pos.x % GetTabWidth() < GetTabWidth() / 2)) && nowRawIndex >= 2 && *prev(formattedStr.rawStr.AtByteIndex(nowRawIndex), 2) != '\t')
					pos = GetFormattedPos(formattedStr, nowRawIndex - 1);
				else
				{
					if (direction == Left)
						pos.x = (pos.x + formattedStr.beginX) / GetTabWidth() * GetTabWidth() - formattedStr.beginX;
					else if (direction == Right)
						pos.x = (pos.x + formattedStr.beginX) / GetTabWidth() * GetTabWidth() + GetTabWidth() - formattedStr.beginX;
					else
					{
						if (pos.x % GetTabWidth() < GetTabWidth() / 2)
							pos.x = (pos.x + formattedStr.beginX) / GetTabWidth() * GetTabWidth() - formattedStr.beginX;
						else
							pos.x = (pos.x + formattedStr.beginX) / GetTabWidth() * GetTabWidth() + GetTabWidth() - formattedStr.beginX;
					}
				}
			}

			// 限制到与双字节字符对齐
			if (pos.x + formattedStr.beginX > 0)
			{
				auto nowRawIndex = GetRawIndex(formattedStr, pos);
				if (nowRawIndex > 0 && IsWideChar(*prev(formattedStr.rawStr.AtByteIndex(nowRawIndex)))
					&& (allowFlow || pos.x < formattedStr[pos.y].DisplaySize())
					&& nowRawIndex == GetRawIndex(formattedStr, pos + ConsolePosition{ 1, 0 }))
					if (direction == Right)
						pos.x++;
					else
						pos.x--;
			}

			return pos;
		}

		size_t SearchLineBeginIndex(StringView rawStr, size_t index) const
		{
			if (index == 0 || rawStr.ByteSize() == 0)
				return 0;
			if (index >= rawStr.ByteSize())		// 临时救一下, 真不想改屎山了
			{
				if (rawStr.ByteSize() >= 1 && rawStr.ByteSize() == '\n')
					return rawStr.ByteSize();
				return SearchLineBeginIndex(rawStr, index - 1);
			}

			if (*rawStr.AtByteIndex(index) == '\n')
			{
				if (index >= 1 && *prev(rawStr.AtByteIndex(index)) == '\r')
				{
					if (index >= 2 && *prev(rawStr.AtByteIndex(index), 2) == '\n')
						return index - 1;
					return SearchLineBeginIndex(rawStr, index - 2);
				}
				if (index >= 1 && *prev(rawStr.AtByteIndex(index)) == '\n')
					return index;
				return SearchLineBeginIndex(rawStr, index - 1);
			}

			const auto lastNewLine = ranges::find_last(rawStr.begin(), next(rawStr.AtByteIndex(index)), '\n');

			if (!lastNewLine.empty())
				return rawStr.GetByteIndex(next(lastNewLine.begin()));
			return 0;
		}
		// 返回的是 End, 所以也符合左闭右开
		size_t SearchLineEndIndex(StringView rawStr, size_t index) const
		{
			auto retIter = find(rawStr.AtByteIndex(index), rawStr.end(), u8'\n');

			if (retIter == rawStr.end())
				return rawStr.ByteSize();

			if (retIter != rawStr.begin() && *prev(retIter) == '\r')
				--retIter;

			return rawStr.GetByteIndex(retIter);
		}

		size_t GetLineIndex(StringView rawStr, size_t index) const
		{
			return count(rawStr.begin(), rawStr.AtByteIndex(index), u8'\n');
		}

		DefaultFormatter(const SettingMap& settingMap)
			:settingMap(settingMap)
		{
		}
	};
}
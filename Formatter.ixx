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
			size_t lineIndex;
		public:
			size_t DisplaySize() const
			{
				auto doubleLengthCount = ranges::count_if(lineData, IsWideChar);

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

			operator wstring_view() const
			{
				return lineData;
			}
			operator wstring() const
			{
				return lineData;
			}

			Line(size_t indexInRaw, const wstring& lineData, size_t lineIndex)
				:indexInRaw(indexInRaw), lineData(lineData), lineIndex(lineIndex)
			{
			}
		};
	public:
		vector<Line> datas;
		wstring_view rawStr;	// 务必注意, 原字符串发生扩容/析构等情况时, 该缓存即刻失效
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
		virtual size_t GetDisplaySize(wstring_view str) const = 0;
		virtual size_t GetRawDisplaySize(wstring_view str) const = 0;

		virtual FormattedString Format(wstring_view rawStrView, size_t maxWidth, size_t maxHeight, size_t beginX, size_t beginLineIndex) = 0;

		virtual size_t GetRawIndex(const FormattedString& formattedStr, ConsolePosition pos) const = 0;

		virtual size_t GetFormattedIndex(wstring_view formattedStr, ConsolePosition pos) const = 0;

		virtual ConsolePosition GetFormattedPos(const FormattedString& formattedStr, size_t index) const = 0;

		virtual ConsolePosition RestrictPos(const FormattedString& formattedStr, ConsolePosition pos, Direction direction, bool allowFlow = false) const = 0;

		virtual size_t SearchLineBeginIndex(wstring_view rawStr, size_t index) const = 0;
		virtual size_t SearchLineEndIndex(wstring_view rawStr, size_t index) const = 0;

		virtual size_t GetLineIndex(wstring_view rawStr, size_t index) const = 0;

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
		size_t GetDisplaySize(wstring_view str) const
		{
			return str.size() + ranges::count_if(str, IsWideChar);
		}
		size_t GetRawDisplaySize(wstring_view str) const
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
		FormattedString Format(wstring_view rawStrView, size_t maxWidth, size_t maxHeight, size_t beginX, size_t beginLineIndex)
		{
			FormattedString ret;

			auto lineBeginIter = rawStrView.begin();
			auto lineEndIter = find(rawStrView.begin(), rawStrView.end(), '\n');

			while (true)
			{
				ret.datas.emplace_back(lineBeginIter - rawStrView.begin(), wstring{ lineBeginIter,lineEndIter }, beginLineIndex);
				beginLineIndex++;

				if (lineEndIter != rawStrView.begin() && *(lineEndIter - 1) == '\r')
					ret.datas.back().lineData.pop_back();

				if (ret.datas.size() >= maxHeight)
					break;

				if (lineEndIter == rawStrView.end())
					break;

				lineBeginIter = lineEndIter + 1;
				lineEndIter = find(lineEndIter + 1, rawStrView.end(), '\n');
			}

			for (auto& [index, str, lineIndex] : ret.datas)
			{
				size_t beginIndex = -1;

				size_t doubleLengthCharCount = 0, beforeBeginDoubleLenCharCount = 0;	// beforeBeginDoubleLenCharCount 仅用于对齐 Tab
				size_t nowIndex = 0;
				for (; nowIndex < str.size();)
				{
					if (beginIndex == -1 && nowIndex + doubleLengthCharCount >= beginX)
					{
						beginIndex = nowIndex;
						if (nowIndex + doubleLengthCharCount > beginX)	// 目前只可能是汉字被截断了一半
						{
							str.replace_with_range(str.begin() + nowIndex - 1, str.begin() + nowIndex, L"  "sv);
							doubleLengthCharCount--;
						}
						beforeBeginDoubleLenCharCount = doubleLengthCharCount;
						doubleLengthCharCount = 0;
					}

					if (beginIndex != -1 && nowIndex + doubleLengthCharCount >= maxWidth + beginIndex)
						break;

					if (str[nowIndex] == '\t')
					{
						str.replace_with_range(str.begin() + nowIndex, str.begin() + nowIndex + 1,
							ranges::views::repeat(L' ', GetTabWidth() - (nowIndex + doubleLengthCharCount + beforeBeginDoubleLenCharCount) % GetTabWidth()));

						nowIndex += GetTabWidth() - (nowIndex + doubleLengthCharCount + beforeBeginDoubleLenCharCount) % GetTabWidth();

						if (beginIndex == -1 && nowIndex + doubleLengthCharCount >= beginX)
						{
							beginIndex = beginX - doubleLengthCharCount;
							beforeBeginDoubleLenCharCount = doubleLengthCharCount;
							doubleLengthCharCount = 0;
						}
						if (beginIndex != -1 && nowIndex + doubleLengthCharCount >= maxWidth + beginIndex)
						{
							nowIndex = maxWidth + beginIndex - doubleLengthCharCount;
							break;
						}
					}
					else
						nowIndex++;

					if (IsWideChar(str[nowIndex - 1]))
						doubleLengthCharCount++;
				}

				if (nowIndex + doubleLengthCharCount > maxWidth + beginIndex)
					nowIndex--;

				if (beginIndex == -1)
				{
					str.clear();
					//index += rawSize;	// 行尾可能有影响
				}
				else
				{
					str = wstring{ str.begin() + beginIndex,str.begin() + nowIndex };
					//index += beginIndex;
				}
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
			if (index == formattedStr.rawStr.size())
				return { formattedStr.datas.back().DisplaySize(),formattedStr.datas.size() - 1 };

			for (auto iter = formattedStr.datas.begin(); iter != formattedStr.datas.end(); ++iter)
				if (iter->indexInRaw <= index
					&& ((iter + 1 != formattedStr.datas.end() && (iter + 1)->indexInRaw > index)
						|| iter + 1 == formattedStr.datas.end()))
				{
					ConsolePosition pos{ 0,iter - formattedStr.datas.begin() };

					size_t nowRawIndex = iter->indexInRaw;

					while (nowRawIndex < index)
					{
						if (formattedStr.rawStr[nowRawIndex] == '\t')
						{
							nowRawIndex++;
							pos.x += GetTabWidth() - pos.x % GetTabWidth();
						}
						else if (IsWideChar(formattedStr.rawStr[nowRawIndex]))
						{
							nowRawIndex++;
							pos.x += 2;
						}
						else if (formattedStr.rawStr[nowRawIndex] == '\n' || formattedStr.rawStr[nowRawIndex] == '\r')	// 这俩在 formattedStr 里并不存储
						{
							nowRawIndex++;
						}
						else
						{
							nowRawIndex++;
							pos.x++;
						}
					}

					pos.x -= formattedStr.beginX;

					return pos;
				}

			unreachable();
		}

		size_t GetRawIndex(const FormattedString& formattedStr, ConsolePosition pos) const
		{
			pos.x += formattedStr.beginX;

			size_t nowRawIndex = formattedStr.datas[pos.y].indexInRaw;
			size_t nowX = 0;

			while (nowX < pos.x && nowRawIndex < formattedStr.rawStr.size())
			{
				if (formattedStr.rawStr[nowRawIndex] == '\t')
				{
					nowRawIndex++;
					nowX += GetTabWidth() - nowX % GetTabWidth();
				}
				else if (IsWideChar(formattedStr.rawStr[nowRawIndex]))
				{
					nowRawIndex++;
					nowX += 2;
				}
				else if (formattedStr.rawStr[nowRawIndex] == '\n'
					|| formattedStr.rawStr[nowRawIndex] == '\r')	// 跨行, 说明 beginX 需要调整, 当然不会在这里调, 但到这就说明已经可以返回了
				{
					return nowRawIndex;
				}
				else
				{
					nowRawIndex++;
					nowX++;
				}
			}

			return nowRawIndex;
		}

		size_t GetFormattedIndex(wstring_view formattedStr, ConsolePosition pos) const
		{
			size_t nowX = 0;
			size_t nowFormattedIndex = 0;

			while (nowX < pos.x)
			{
				if (IsWideChar(formattedStr[nowFormattedIndex]))
				{
					nowFormattedIndex++;
					nowX += 2;
				}
				else
				{
					nowFormattedIndex++;
					nowX++;
				}
			}

			return nowFormattedIndex;
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
				&& GetRawIndex(formattedStr, pos) > 0 && formattedStr.rawStr[GetRawIndex(formattedStr, pos) - 1] == '\t')
			{
				const auto nowRawIndex = GetRawIndex(formattedStr, pos);

				if ((direction == Left || (direction == None && pos.x % GetTabWidth() < GetTabWidth() / 2)) && nowRawIndex >= 2 && formattedStr.rawStr[nowRawIndex - 2] != '\t')
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
				if (nowRawIndex > 0 && IsWideChar(formattedStr.rawStr[nowRawIndex - 1])
					&& (allowFlow || pos.x < formattedStr[pos.y].DisplaySize())
					&& nowRawIndex == GetRawIndex(formattedStr, pos + ConsolePosition{ 1, 0 }))
					if (direction == Right)
						pos.x++;
					else
						pos.x--;
			}

			return pos;
		}

		size_t SearchLineBeginIndex(wstring_view rawStr, size_t index) const
		{
			if (index == 0 || rawStr.size() == 0)
				return 0;
			if (index >= rawStr.size())		// 临时救一下, 真不想改屎山了
			{
				if (rawStr.size() >= 1 && rawStr.back() == '\n')
					return rawStr.size();
				return SearchLineBeginIndex(rawStr, index - 1);
			}

			if (rawStr[index] == '\n')
			{
				if (index >= 1 && rawStr[index - 1] == '\r')
				{
					if (index >= 2 && rawStr[index - 2] == '\n')
						return index - 1;
					return SearchLineBeginIndex(rawStr, index - 2);
				}
				if (index >= 1 && rawStr[index - 1] == '\n')
					return index;
				return SearchLineBeginIndex(rawStr, index - 1);
			}

			if (rawStr.rfind('\n', index) != string::npos)
				return rawStr.rfind('\n', index) + 1;	// 注意 rfind 的搜索范围是 [0,index], 并非左闭右开
			return 0;
		}
		// 返回的是 End, 所以也符合左闭右开
		size_t SearchLineEndIndex(wstring_view rawStr, size_t index) const
		{
			auto retIndex = rawStr.find('\n', index);

			if (retIndex == string::npos)
				return rawStr.size();

			if (retIndex >= 1 && rawStr[retIndex - 1] == '\r')
				retIndex--;

			return retIndex;
		}

		size_t GetLineIndex(wstring_view rawStr, size_t index) const
		{
			return count(rawStr.begin(), rawStr.begin() + index, '\n');
		}

		DefaultFormatter(const SettingMap& settingMap)
			:settingMap(settingMap)
		{
		}
	};
}
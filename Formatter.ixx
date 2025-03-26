// Formatter.ixx
// ����ԭʼ�ַ����ĸ�ʽ����˫��λ
// ����˵, ���Ǹ����ļ�������ַ�������Ļ����ʾ��λ��һһ��Ӧ�Ĺ���, ��������ʾ��ʽ���Զ����еȶ��Ǿݴ�ʵ�ֵ�
// ����ĺ�Ŭ�����ع�����, ���ǳ����˷����Һü���ʱ��֮��һ������
// ���Ի���������ʺɽ����ʺ��
export module Formatter;

import std;

import ConsoleTypedef;

using namespace std;

export namespace NylteJ
{
	// һ��������ǳ����õ� Tip: FormattedString[y][x] ��Ӧ (x, y) �ַ�, ���� (x, y) ���λ��λ������ַ�ǰ��
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

			operator wstring_view() const
			{
				return lineData;
			}
			operator wstring() const
			{
				return lineData;
			}

			Line(size_t indexInRaw, const wstring& lineData)
				:indexInRaw(indexInRaw), lineData(lineData)
			{
			}
		};
	public:
		vector<Line> datas;
		wstring_view rawStr;	// ���ע��, ԭ�ַ�����������/���������ʱ, �û��漴��ʧЧ
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

		virtual FormattedString Format(wstring_view rawStr, size_t maxWidth, size_t maxHeight, size_t beginX = 0) = 0;

		virtual size_t GetRawIndex(const FormattedString& formattedStr, ConsolePosition pos) const = 0;

		virtual size_t GetFormattedIndex(wstring_view formattedStr, ConsolePosition pos) const = 0;

		virtual ConsolePosition GetFormattedPos(const FormattedString& formattedStr, size_t index) const = 0;

		virtual ConsolePosition RestrictPos(const FormattedString& formattedStr, ConsolePosition pos, Direction direction, bool allowFlow = false) const = 0;

		virtual size_t SearchLineBeginIndex(wstring_view rawStr, size_t index) const = 0;
		virtual size_t SearchLineEndIndex(wstring_view rawStr, size_t index) const = 0;
	};

	class DefaultFormatter :public FormatterBase
	{
		size_t GetDisplaySize(wstring_view str) const
		{
			return str.size() + ranges::count_if(str, [](auto&& chr) {return chr > 128; });
		}
		size_t GetRawDisplaySize(wstring_view str) const
		{
			size_t nowSize = 0;

			for (auto&& chr : str)
			{
				if (chr == '\t')
					nowSize = nowSize / 4 * 4 + 4;
				else if (chr >= 128)
					nowSize += 2;
				else if (chr != '\n' && chr != '\r')
					nowSize++;
			}

			return nowSize;
		}

		// û�Ż� (ָ���еط����ֳ������� formattedString, ��ȫ������), ��֮����������˵
		// �������, ������һ��, ��Ȼ������ʲô����ƿ��, ֻ��һ�зǳ��ǳ��� (KB ��) ��ʱ��Ż������Կ���
		// Ҳ����˵���Լ������Ż�, COOOOOOOL!
		FormattedString Format(wstring_view rawStrView, size_t maxWidth, size_t maxHeight, size_t beginX = 0)
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
				size_t beginIndex = -1;

				size_t rawSize = str.size();
				size_t doubleLengthCharCount = 0, beforeBeginDoubleLenCharCount = 0;	// beforeBeginDoubleLenCharCount �����ڶ��� Tab
				size_t nowIndex = 0;
				for (; nowIndex < str.size();)
				{
					if (beginIndex == -1 && nowIndex + doubleLengthCharCount >= beginX)
					{
						beginIndex = nowIndex;
						if (nowIndex + doubleLengthCharCount > beginX)	// Ŀǰֻ�����Ǻ��ֱ��ض���һ��
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
							ranges::views::repeat(L' ', 4 - (nowIndex + doubleLengthCharCount + beforeBeginDoubleLenCharCount) % 4));

						nowIndex += 4 - (nowIndex + doubleLengthCharCount + beforeBeginDoubleLenCharCount) % 4;

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

					if (str[nowIndex - 1] >= 128)
						doubleLengthCharCount++;
				}

				if (nowIndex + doubleLengthCharCount > maxWidth + beginIndex)
					nowIndex--;

				if (beginIndex == -1)
				{
					str.clear();
					//index += rawSize;	// ��β������Ӱ��
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
							pos.x += 4 - pos.x % 4;
						}
						else if (formattedStr.rawStr[nowRawIndex] > 128)
						{
							nowRawIndex++;
							pos.x += 2;
						}
						else if (formattedStr.rawStr[nowRawIndex] == '\n' || formattedStr.rawStr[nowRawIndex] == '\r')	// ������ formattedStr �ﲢ���洢
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

			while (nowX < pos.x)
			{
				if (nowRawIndex >= formattedStr.rawStr.size())
					return formattedStr.rawStr.size() - 1;

				if (formattedStr.rawStr[nowRawIndex] == '\t')
				{
					nowRawIndex++;
					nowX += 4 - nowX % 4;
				}
				else if (formattedStr.rawStr[nowRawIndex] > 128)
				{
					nowRawIndex++;
					nowX += 2;
				}
				else if (formattedStr.rawStr[nowRawIndex] == '\n'
					|| formattedStr.rawStr[nowRawIndex] == '\r')	// ����, ˵�� beginX ��Ҫ����, ��Ȼ�����������, �������˵���Ѿ����Է�����
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
				if (formattedStr[nowFormattedIndex] > 128)
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

		// ����� allowFlow ʵ���ϲ�������ȫ�������, ֻ��Ϊ�˺���ǰ��ʺɽ�ӹ���˸�����
		ConsolePosition RestrictPos(const FormattedString& formattedStr, ConsolePosition pos, Direction direction, bool allowFlow = false) const
		{
			using enum Direction;

			// ���Ƶ������ֵ�����
			if (pos.y < 0)
				pos.y = 0;
			if (pos.y >= formattedStr.datas.size())
				pos.y = formattedStr.datas.size() - 1;
			if ((allowFlow && pos.x + formattedStr.beginX < 0)
				|| (!allowFlow && pos.x < 0))
				pos.x = 0;
			if (pos.x >= 0 && pos.x > formattedStr[pos.y].DisplaySize()
				&& !allowFlow)
				pos.x = formattedStr[pos.y].DisplaySize();

			// ���Ƶ����Ʊ������
			if ((pos.x + formattedStr.beginX) % 4 != 0 && formattedStr.rawStr[GetRawIndex(formattedStr, pos) - 1] == '\t')
			{
				const auto nowRawIndex = GetRawIndex(formattedStr, pos);

				if ((direction == Left || (direction == None && pos.x % 4 <= 1)) && nowRawIndex >= 2 && formattedStr.rawStr[nowRawIndex - 2] != '\t')
					pos = GetFormattedPos(formattedStr, nowRawIndex - 1);
				else
				{
					if (direction == Left)
						pos.x = (pos.x + formattedStr.beginX) / 4 * 4 - formattedStr.beginX;
					else if (direction == Right)
						pos.x = (pos.x + formattedStr.beginX) / 4 * 4 + 4 - formattedStr.beginX;
					else
					{
						if (pos.x % 4 <= 1)
							pos.x = (pos.x + formattedStr.beginX) / 4 * 4 - formattedStr.beginX;
						else
							pos.x = (pos.x + formattedStr.beginX) / 4 * 4 + 4 - formattedStr.beginX;
					}
				}
			}

			// ���Ƶ���˫�ֽ��ַ�����
			if (pos.x + formattedStr.beginX > 0)
			{
				auto nowRawIndex = GetRawIndex(formattedStr, pos);
				if (formattedStr.rawStr[nowRawIndex - 1] > 128
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
			if (index == 0)
				return 0;
			if (index >= rawStr.size())
				return SearchLineBeginIndex(rawStr, rawStr.size() - 1);

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
				return rawStr.rfind('\n', index) + 1;	// ע�� rfind ��������Χ�� [0,index], ��������ҿ�
			return 0;
		}
		// ���ص��� End, ����Ҳ��������ҿ�
		size_t SearchLineEndIndex(wstring_view rawStr, size_t index) const
		{
			auto retIndex = rawStr.find('\n', index);

			if (retIndex == string::npos)
				return rawStr.size();

			if (retIndex >= 1 && rawStr[retIndex - 1] == '\r')
				retIndex--;

			return retIndex;
		}
	};
}
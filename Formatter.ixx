// Formatter.ixx
// ����ԭʼ�ַ����ĸ�ʽ����˫��λ
// ����˵, ���Ǹ����ļ�������ַ�������Ļ����ʾ��λ��һһ��Ӧ�Ĺ���, ��������ʾ��ʽ���Զ����еȶ��Ǿݴ�ʵ�ֵ�
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
		wstring_view rawStr;
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
		virtual FormattedString Format(wstring_view rawStr, size_t maxWidth, size_t maxHeight) = 0;

		virtual size_t GetRawIndex(const FormattedString& formattedStr, ConsolePosition pos) const = 0;

		virtual size_t GetFormattedIndex(wstring_view formattedStr, ConsolePosition pos) const = 0;

		virtual ConsolePosition GetFormattedPos(const FormattedString& formattedStr, size_t index) const = 0;

		virtual ConsolePosition RestrictPos(const FormattedString& formattedStr, ConsolePosition pos, Direction direction) const = 0;

		virtual size_t SearchLineBeginIndex(wstring_view rawStr, size_t index) const = 0;
	};

	class DefaultFormatter :public FormatterBase
	{
		// û�Ż� (ָ���еط����ֳ������� formattedString, ��ȫ������), ��֮����������˵
		// �������, ������һ��, ��Ȼ������ʲô����ƿ��, ֻ��һ�зǳ��ǳ��� (KB ��) ��ʱ��Ż������Կ���
		// Ҳ����˵���Լ������Ż�, COOOOOOOL!
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

			ret.rawStr = { rawStrView.begin(),lineEndIter };

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

					return pos;
				}

			unreachable();
		}

		size_t GetRawIndex(const FormattedString& formattedStr, ConsolePosition pos) const
		{
			size_t nowRawIndex = formattedStr.datas[pos.y].indexInRaw;
			size_t nowX = 0;

			while (nowX < pos.x)
			{
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

		ConsolePosition RestrictPos(const FormattedString& formattedStr, ConsolePosition pos, Direction direction) const
		{
			using enum Direction;

			// ����
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

			// ���Ƶ������ֵ�����
			if (pos.y < 0)
				pos.y = 0;
			if (pos.y >= formattedStr.datas.size())
				pos.y = formattedStr.datas.size() - 1;
			if (pos.x < 0)
				pos.x = 0;
			if (pos.x > formattedStr[pos.y].DisplaySize())
				pos.x = formattedStr[pos.y].DisplaySize();

			// ���Ƶ����Ʊ������
			if (pos.x % 4 != 0 && formattedStr.rawStr[GetRawIndex(formattedStr, pos) - 1] == '\t')
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

			// ���Ƶ���˫�ֽ��ַ�����
			if (pos.x > 0)
			{
				auto nowRawIndex = GetRawIndex(formattedStr, pos);
				if (formattedStr.rawStr[nowRawIndex - 1] > 128 && nowRawIndex == GetRawIndex(formattedStr, pos + ConsolePosition{ 1, 0 }))
					if (direction == Right)
						pos.x++;
					else
						pos.x--;
			}

			return pos;
		}

		size_t SearchLineBeginIndex(wstring_view rawStr, size_t index) const
		{
			return rawStr.rfind('\n', index) + 1;
		}
	};
}
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
		// û�Ż�, ��֮����������˵
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

			// ���Ƶ���˫�ֽ��ַ�����
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
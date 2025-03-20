// Formatter.ixx
// ���ڸ�ʽ��ԭʼ�ַ���
// ʵ���ϻ���ֻ�ƹ��������Զ�����
// ������ַ���Ӧ�������� \t ��
// ͬʱҲ���𷴸�ʽ���ַ���λ�� (���ݸ�ʽ������ַ���λ��, ��ȡԭʼ�ַ����Ķ�Ӧλ��)
export module Formatter;

import std;

import ConsoleTypedef;

using namespace std;

export namespace NylteJ
{
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
	};

	class FormatterBase
	{
	public:
		virtual FormattedString Format(wstring_view rawStr, size_t maxWidth, size_t maxHeight) = 0;

		virtual size_t GetRawIndex(const FormattedString& formattedStr, wstring_view rawStr, size_t x, size_t y) = 0;
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

		size_t GetRawIndex(const FormattedString& formattedStr, wstring_view rawStr, size_t x, size_t y)
		{
			size_t nowRawIndex = formattedStr.datas[y].indexInRaw;
			size_t nowX = 0;

			while (nowX < x)
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
	};
}
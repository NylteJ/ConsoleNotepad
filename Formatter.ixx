// Formatter.ixx
// ���ڸ�ʽ��ԭʼ�ַ���
// ������ַ���Ӧ�������� \t ��
export module Formatter;

import std;

import ConsoleTypedef;

using namespace std;

export namespace NylteJ
{
	class FormatterBase
	{
	public:
		virtual string FormatTab(string rawStr) = 0;
	};

	class DefaultFormatter :public FormatterBase
	{
		// û�Ż�, ��֮����������˵
		string FormatTab(string rawStr)
		{
			size_t tabIndex = rawStr.find('\t');

			while (tabIndex < rawStr.size())
			{
				size_t lineBeginIndex = tabIndex;
				while (lineBeginIndex > 0 && rawStr[lineBeginIndex] != '\n')
					lineBeginIndex--;
				if (rawStr[lineBeginIndex] == '\n')
					lineBeginIndex++;

				rawStr.replace_with_range(	rawStr.begin() + tabIndex,
											rawStr.begin() + tabIndex + 1,
											ranges::views::repeat(' ', 4 - (tabIndex - lineBeginIndex) % 4));

				tabIndex = rawStr.find('\t', tabIndex + 1);
			}

			return rawStr;
		}
	};
}
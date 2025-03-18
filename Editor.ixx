// Editor.ixx
// 编辑器 UI 组件, 毫无疑问的核心部分
export module Editor;

import std;

import ConsoleHandler;
import ConsoleTypedef;
import Formatter;

using namespace std;

export namespace NylteJ
{
	class Editor
	{
	private:
		ConsoleHandler& console;

		string& data;

		ConsoleRect drawRange;

		shared_ptr<FormatterBase> formatter;
	public:
		void PrintData()
		{
			auto lineBeginIter = data.begin();
			auto lineEndIter = find(data.begin(), data.end(), '\n');

			auto nowCursorPos = drawRange.leftTop;

			while (true)
			{
				const auto validLength = min(lineEndIter - lineBeginIter, static_cast<ptrdiff_t>(drawRange.Width()));

				console.Print(formatter->FormatTab({ lineBeginIter,lineBeginIter + validLength }), nowCursorPos);
				nowCursorPos.y++;

				if (nowCursorPos.y > drawRange.rightBottom.y)
					break;

				if (lineEndIter == data.end())
					break;

				lineBeginIter = lineEndIter + 1;
				lineEndIter = find(lineEndIter + 1, data.end(), '\n');
			}
		}
		void RePrintData()
		{
			auto lineBeginIter = data.begin();
			auto lineEndIter = find(data.begin(), data.end(), '\n');

			auto nowCursorPos = drawRange.leftTop;

			while (true)
			{
				const string formattedStr = formatter->FormatTab({ lineBeginIter,lineEndIter });

				const auto validLength = min(formattedStr.size(), static_cast<size_t>(drawRange.Width()));

				console.Print(formattedStr.substr(0, validLength), nowCursorPos);
				console.Print(ranges::views::repeat(' ', drawRange.Width() - validLength) | ranges::to<string>());
				nowCursorPos.y++;

				if (nowCursorPos.y > drawRange.rightBottom.y)
					break;

				if (lineEndIter == data.end())
					break;

				lineBeginIter = lineEndIter + 1;
				lineEndIter = find(lineEndIter + 1, data.end(), '\n');
			}
		}

		void ChangeDrawRange(const ConsoleRect& drawRange)
		{
			this->drawRange = drawRange;
		}


		Editor(ConsoleHandler& console, string& data, const ConsoleRect& drawRange, shared_ptr<FormatterBase> formatter = make_shared<DefaultFormatter>())
			:console(console), data(data), drawRange(drawRange), formatter(formatter)
		{
			PrintData();
		}
	};
}
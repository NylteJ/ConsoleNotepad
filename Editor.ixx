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

		wstring fileData;

		ConsoleRect drawRange;

		shared_ptr<FormatterBase> formatter;

		ConsolePosition cursorPos = { 0,0 };	// 当前编辑器的光标位置, 是相对 drawRange 左上角而言的坐标
	public:
		// 输出内容的同时会覆盖背景
		void PrintData()
		{
			console.HideCursor();

			auto nowCursorPos = drawRange.leftTop;

			// TODO: 充分利用缓存, 不要每次都重新算
			FormattedString formattedStrs = formatter->Format(fileData, drawRange.Width(), drawRange.Height());

			for (auto& [index, str] : formattedStrs.datas)
			{
				console.Print(str, nowCursorPos);

				if (str.size() < drawRange.Width())
					console.Print(ranges::to<wstring>(ranges::views::repeat(' ', drawRange.Width() - str.size())));

				nowCursorPos.y++;
				if (nowCursorPos.y > drawRange.rightBottom.y)
					break;
			}

			while (nowCursorPos.y <= drawRange.rightBottom.y)	// 后面的空白也要覆写
			{
				console.Print(ranges::to<wstring>(ranges::views::repeat(' ', drawRange.Width())), nowCursorPos);

				nowCursorPos.y++;
			}
			// 貌似老式的 conhost 有蜜汁兼容问题, 有时会返回控制台有 9001 行......导致这里会出问题, 不过后面再调整好了 (反正这不是核心功能)

			console.ShowCursor();
		}

		const auto& GetDrawRange() const
		{
			return drawRange;
		}
		void ChangeDrawRange(const ConsoleRect& drawRange)
		{
			this->drawRange = drawRange;
		}

		wstring_view GetData() const
		{
			return fileData;
		}

		void FlushCursor() const
		{
			console.SetCursorTo(drawRange.leftTop + cursorPos);
		}

		void SetCursorPos(const ConsolePosition& pos)
		{
			if (drawRange.Contain(drawRange.leftTop + pos))
			{
				cursorPos = pos;
				FlushCursor();
			}
		}
		void SetCursorAbsPos(const ConsolePosition& absPos)
		{
			if (drawRange.Contain(absPos))
			{
				cursorPos = absPos - drawRange.leftTop;
				FlushCursor();
			}
		}

		// TODO: 用枚举, 好突出这个函数只能移动一格
		void MoveCursor(int deltaX, int deltaY)
		{
			FormattedString formattedStrs = formatter->Format(fileData, drawRange.Width(), drawRange.Height());

			// 这部分代码应该挪到 Formatter 里面, 但是脑力要耗尽了, TODO
			// 反正就是这部分代码需要重构
			if (fileData[formatter->GetRawIndex(formattedStrs, fileData, cursorPos.x, cursorPos.y)] == '\t' && deltaX == 1)
			{
				cursorPos.x += 4 - cursorPos.x % 4;
				SetCursorPos(cursorPos);
				return;
			}
			if (cursorPos.x > 0 && fileData[formatter->GetRawIndex(formattedStrs, fileData, cursorPos.x, cursorPos.y)-1] == '\t' && deltaX == -1)
			{
				if (cursorPos.x % 4 != 0)
					cursorPos.x -= cursorPos.x % 4;
				else
					cursorPos.x -= 4;

				SetCursorPos(cursorPos);
				return;
			}

			if (cursorPos.x == 0 && deltaX == -1)
			{
				if (cursorPos.y > 0)
					deltaY--;
				else
					deltaX = 0;
			}
			else if (cursorPos.x == formattedStrs[cursorPos.y].DisplaySize() && deltaX == 1)
			{
				deltaY++;
				cursorPos.x = 0;
				deltaX = 0;
			}

			if (cursorPos.y == 0 && deltaY == -1)
				deltaY = 0;

			if (fileData[formatter->GetRawIndex(formattedStrs, fileData, cursorPos.x, cursorPos.y)] > 128 && deltaX == 1)
				deltaX++;
			if (cursorPos.x > 1 && fileData[formatter->GetRawIndex(formattedStrs, fileData, cursorPos.x, cursorPos.y) - 1] > 128 && deltaX == -1)
				deltaX--;

			ConsoleXPos newX = cursorPos.x + deltaX;
			ConsoleYPos newY = cursorPos.y + deltaY;

			if (newY >= formattedStrs.datas.size())
				newY = formattedStrs.datas.size() - 1;

			if (newX > formattedStrs.datas[newY].DisplaySize())
				newX = formattedStrs.datas[newY].DisplaySize();

			if (newX % 4 != 0 && fileData[formatter->GetRawIndex(formattedStrs, fileData, newX, newY) - 1] == '\t')
			{
				if (newX % 4 <= 1)
					newX -= newX % 4;
				else
					newX += 4 - newX % 4;
			}

			SetCursorPos({ newX,newY });
		}

		const ConsolePosition& GetCursorPos() const
		{
			return cursorPos;
		}

		void Insert(wstring_view str, size_t x, size_t y)
		{
			const FormattedString formattedStrs = formatter->Format(fileData, drawRange.Width(), drawRange.Height());

			const size_t index = formatter->GetRawIndex(formattedStrs, fileData, x, y);

			fileData.insert_range(fileData.begin() + index, str);

			PrintData();

			if (str.size() > 1)
				SetCursorPos(cursorPos + ConsolePosition{ str.size() - 1, 0 });
			MoveCursor(1, 0);

			FlushCursor();
		}

		void Erase(size_t x, size_t y)
		{
			const FormattedString formattedStrs = formatter->Format(fileData, drawRange.Width(), drawRange.Height());

			size_t index = formatter->GetRawIndex(formattedStrs, fileData, x, y);

			if (index == 0)
				return;

			index--;

			wchar_t delChar = fileData[index];

			if (index > 0 && fileData[index - 1] == '\r')
				fileData.erase(index - 1, 2);
			else
				fileData.erase(fileData.begin() + index);	// 这里只传 index 就会把后面的全删掉

			PrintData();

			if (delChar == '\t')
				SetCursorPos(cursorPos - ConsolePosition{ 4,0 });	// TODO: 解耦这一块
			else
				MoveCursor(-1, 0);

			FlushCursor();
		}

		const wstring& GetFileData() const
		{
			return fileData;
		}

		Editor(ConsoleHandler& console, wstring& fileData, const ConsoleRect& drawRange, shared_ptr<FormatterBase> formatter = make_shared<DefaultFormatter>())
			:console(console), fileData(fileData), drawRange(drawRange), formatter(formatter)
		{
			PrintData();
		}
	};
}
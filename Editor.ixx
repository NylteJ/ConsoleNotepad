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

		size_t fileDataIndex = 0;	// 目前屏幕上显示的区域是从 fileData 的哪个下标开始的

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
			FormattedString formattedStrs = formatter->Format(	wstring_view{ fileData.begin() + fileDataIndex,fileData.end() },
																drawRange.Width(),
																drawRange.Height());

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
			using enum FormatterBase::MotivationDirection;

			FormatterBase::MotivationDirection direction = None;
			if (deltaX > 0)
				direction = Right;
			else if (deltaX < 0)
				direction = Left;
			else if (deltaY < 0)
				direction = Up;
			else
				direction = Down;

			if (cursorPos.y + 1 == drawRange.rightBottom.y && direction == Down)
			{
				fileDataIndex += formatter->Format(wstring_view{ fileData.begin() + fileDataIndex,fileData.end() },
					drawRange.Width(), drawRange.Height())[1].indexInRaw;

				PrintData();
			}
			if (cursorPos.y == 0 && direction == Up && fileDataIndex >= 2)
			{
				fileDataIndex = fileData.rfind('\n', fileDataIndex - 2) + 1;	// TODO: 优化

				PrintData();
			}

			SetCursorPos(formatter->RestrictPos(formatter->Format(wstring_view{ fileData.begin() + fileDataIndex,fileData.end() },
				drawRange.Width(), drawRange.Height()),
				wstring_view{ fileData.begin() + fileDataIndex,fileData.end() },
				ConsolePosition{ cursorPos.x + deltaX,cursorPos.y + deltaY },
				direction));
		}

		const ConsolePosition& GetCursorPos() const
		{
			return cursorPos;
		}

		void Insert(wstring_view str, size_t x, size_t y)
		{
			const FormattedString formattedStrs = formatter->Format(wstring_view{ fileData.begin() + fileDataIndex,fileData.end() }, drawRange.Width(), drawRange.Height());

			const size_t index = formatter->GetRawIndex(formattedStrs, wstring_view{ fileData.begin() + fileDataIndex,fileData.end() }, { x,y })+ fileDataIndex;

			fileData.insert_range(fileData.begin() + index, str);

			PrintData();

			if (str.size() > 1)
				SetCursorPos(cursorPos + ConsolePosition{ str.size() - 1, 0 });
			MoveCursor(1, 0);

			FlushCursor();
		}

		void Erase(size_t x, size_t y)
		{
			const FormattedString formattedStrs = formatter->Format(wstring_view{ fileData.begin() + fileDataIndex,fileData.end() }, drawRange.Width(), drawRange.Height());

			size_t index = formatter->GetRawIndex(formattedStrs, wstring_view{ fileData.begin() + fileDataIndex,fileData.end() }, { x, y })+ fileDataIndex;

			if (index == 0)
				return;

			index--;

			console.HideCursor();

			MoveCursor(-1, 0);

			if (index > 0 && fileData[index - 1] == '\r')
				fileData.erase(index - 1, 2);
			else
				fileData.erase(fileData.begin() + index);	// 这里只传 index 就会把后面的全删掉

			PrintData();

			FlushCursor();

			console.ShowCursor();
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
// Editor.ixx
// 编辑器 UI 组件, 毫无疑问的核心部分
export module Editor;

import std;

import ConsoleHandler;
import ConsoleTypedef;
import Formatter;
import BasicColors;

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

		size_t	selectBeginIndex = fileDataIndex,		// 这里的 "Begin" 与 "End" 都是逻辑上的 (从 Begin 选到 End), "End" 指当前光标所在/边界变化的位置
				selectEndIndex = selectBeginIndex;		// 所以 Begin 甚至可以大于 End (通过 Shift + 左方向键即可实现)
	private:
		constexpr size_t MinSelectIndex() const
		{
			return min(selectBeginIndex, selectEndIndex);
		}
		constexpr size_t MaxSelectIndex() const
		{
			return max(selectBeginIndex, selectEndIndex);
		}
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

			ConsolePosition selectBegin = { 0,0 }, selectEnd = { 0,0 };
			if(selectBeginIndex!=selectEndIndex)
			{
				selectBegin = formatter->GetFormattedPos(formattedStrs,
					MinSelectIndex() >= fileDataIndex ? (MinSelectIndex() - fileDataIndex) : 0);

				selectEnd = formatter->GetFormattedPos(formattedStrs,
					MaxSelectIndex() <= fileDataIndex + formattedStrs.rawStr.size() ? (MaxSelectIndex() - fileDataIndex) : formattedStrs.rawStr.size());
			}

			for (size_t y = 0; y < formattedStrs.datas.size(); y++)
			{
				if (selectBegin.y <= y && y <= selectEnd.y)
				{
					size_t secondBegin = 0, secondEnd = formattedStrs[y].lineData.size();

					if (selectEnd.y == y)
						secondEnd = formatter->GetFormattedIndex(formattedStrs[y], selectEnd);
					if (selectBegin.y == y)
						secondBegin = formatter->GetFormattedIndex(formattedStrs[y], selectBegin);

					console.Print(wstring_view{ formattedStrs[y].lineData.begin(),formattedStrs[y].lineData.begin() + secondBegin }, nowCursorPos);

					console.Print(wstring_view{ formattedStrs[y].lineData.begin() + secondBegin,formattedStrs[y].lineData.begin() + secondEnd }, BasicColors::inverseColor, BasicColors::inverseColor);

					console.Print(wstring_view{ formattedStrs[y].lineData.begin() + secondEnd,formattedStrs[y].lineData.end() });
				}
				else
					console.Print(formattedStrs[y], nowCursorPos);

				if (formattedStrs[y].lineData.size() < drawRange.Width())
					console.Print(ranges::to<wstring>(ranges::views::repeat(' ', drawRange.Width() - formattedStrs[y].lineData.size())));

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

			FlushCursor();		// 不加这个的话打字的时候光标有时会乱闪
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

		// 这个函数是从编辑器的角度移动光标, 所以从逻辑上来说相当于用户自己把光标移到了那个位置 (通过鼠标等)
		// 因此它会破坏当前选择区域
		// TODO: 是不是不该耦合得这么死?
		void SetCursorPos(const ConsolePosition& pos, bool selectMode = false)
		{
			if (drawRange.Contain(drawRange.leftTop + pos))
			{
				if(selectMode)
				{
					const auto formattedString = formatter->Format(wstring_view{ fileData.begin() + fileDataIndex,fileData.end() },drawRange.Width(), drawRange.Height());

					if (selectBeginIndex == selectEndIndex)
						selectBeginIndex = formatter->GetRawIndex(formattedString, cursorPos) + fileDataIndex;

					selectEndIndex = formatter->GetRawIndex(formattedString, pos) + fileDataIndex;

					PrintData();
				}
				else if (selectEndIndex != selectBeginIndex)
				{
					selectEndIndex = selectBeginIndex;
					PrintData();
				}

				cursorPos = pos;
				FlushCursor();
			}
		}
		void RestrictedAndSetCursorPos(const ConsolePosition& pos, bool selectMode = false)
		{
			SetCursorPos(formatter->RestrictPos(formatter->Format(wstring_view{ fileData.begin() + fileDataIndex,fileData.end() },
				drawRange.Width(), drawRange.Height()),
				pos,
				Direction::None),
				selectMode);
		}
		void SetCursorAbsPos(const ConsolePosition& absPos)
		{
			if (drawRange.Contain(absPos))
			{
				cursorPos = absPos - drawRange.leftTop;
				FlushCursor();
			}
		}

		void MoveCursor(Direction direction, bool selectMode = false)
		{
			using enum Direction;

			short deltaX = 0, deltaY = 0;

			switch (direction)
			{
			case Down:	deltaY = 1; break;
			case Up:	deltaY = -1; break;
			case Right:	deltaX = 1; break;
			case Left:	deltaX = -1; break;
			}

			if (cursorPos.y + 1 == drawRange.rightBottom.y && direction == Down)
			{
				fileDataIndex += formatter->Format(wstring_view{ fileData.begin() + fileDataIndex,fileData.end() },
					drawRange.Width(), drawRange.Height())[1].indexInRaw;

				PrintData();
			}
			if (cursorPos.y == 0 && direction == Up && fileDataIndex >= 2)
			{
				fileDataIndex = fileData.rfind('\n', fileDataIndex - 2) + 1;	// TODO: 优化

				PrintData();	// TODO: 适当地少 PrintData 几次 (比如现在如果既要滚动, 选择区域又发生了改变, 那就会多 PrintData 一次)
			}

			SetCursorPos(formatter->RestrictPos(formatter->Format(wstring_view{ fileData.begin() + fileDataIndex,fileData.end() },
				drawRange.Width(), drawRange.Height()),
				ConsolePosition{ cursorPos.x + deltaX,cursorPos.y + deltaY },
				direction),
				selectMode);
		}

		const ConsolePosition& GetCursorPos() const
		{
			return cursorPos;
		}

		// 基于当前光标/选区位置插入
		void Insert(wstring_view str)
		{
			const FormattedString formattedStrs = formatter->Format(wstring_view{ fileData.begin() + fileDataIndex,fileData.end() }, drawRange.Width(), drawRange.Height());

			if (selectBeginIndex == selectEndIndex)
			{
				const size_t index = formatter->GetRawIndex(formattedStrs, cursorPos) + fileDataIndex;

				fileData.insert_range(fileData.begin() + index, str);
			}
			else
			{
				if (MinSelectIndex() < fileDataIndex)	// 跨屏插入
					fileDataIndex = formatter->SearchLineBeginIndex(fileData, MinSelectIndex());

				fileData.replace_with_range(fileData.begin() + MinSelectIndex(), fileData.begin() + MaxSelectIndex(), str);

				SetCursorPos(formatter->GetFormattedPos(formatter->Format(wstring_view{ fileData.begin() + fileDataIndex,fileData.end() }, drawRange.Width(), drawRange.Height()),
					MinSelectIndex() >= fileDataIndex ? (MinSelectIndex() - fileDataIndex) : 0));
			}

			PrintData();

			if (str.size() > 1)
				SetCursorPos(cursorPos + ConsolePosition{ str.size() - 1, 0 });
			MoveCursor(Direction::Right);

			FlushCursor();
		}

		// 基于当前光标/选区位置删除 (等价于按一下 Backspace)
		void Erase()
		{
			const FormattedString formattedStrs = formatter->Format(wstring_view{ fileData.begin() + fileDataIndex,fileData.end() }, drawRange.Width(), drawRange.Height());

			if (selectBeginIndex == selectEndIndex)
			{
				size_t index = formatter->GetRawIndex(formattedStrs, cursorPos) + fileDataIndex;

				if (index == 0)
					return;

				index--;

				console.HideCursor();

				MoveCursor(Direction::Left);

				if (index > 0 && fileData[index - 1] == '\r')
					fileData.erase(index - 1, 2);
				else
					fileData.erase(fileData.begin() + index);	// 这里只传 index 就会把后面的全删掉

				PrintData();

				FlushCursor();

				console.ShowCursor();
			}
			else
			{
				if (MinSelectIndex() < fileDataIndex)	// 跨屏删除
					fileDataIndex = formatter->SearchLineBeginIndex(fileData, MinSelectIndex());

				fileData.erase(fileData.begin() + MinSelectIndex(), fileData.begin() + MaxSelectIndex());

				SetCursorPos(formatter->GetFormattedPos(formatter->Format(wstring_view{ fileData.begin() + fileDataIndex,fileData.end() }, drawRange.Width(), drawRange.Height()),
					MinSelectIndex() >= fileDataIndex ? (MinSelectIndex() - fileDataIndex) : 0));
			}
		}

		const wstring& GetFileData() const
		{
			return fileData;
		}

		void SelectAll()
		{
			selectBeginIndex = 0;
			selectEndIndex = fileData.size();

			PrintData();

			FlushCursor();	// 虽然 VS 和 VSCode 在全选时都会把光标挪到末尾, 但有一说一我觉得不动光标在这里更好用 (而且更好写)
		}

		wstring_view GetSelectedStr() const
		{
			return { fileData.begin() + MinSelectIndex(),fileData.begin() + MaxSelectIndex() };
		}

		Editor(ConsoleHandler& console, wstring& fileData, const ConsoleRect& drawRange, shared_ptr<FormatterBase> formatter = make_shared<DefaultFormatter>())
			:console(console), fileData(fileData), drawRange(drawRange), formatter(formatter)
		{
			PrintData();
		}
	};
}
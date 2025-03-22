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

		ConsolePosition cursorPos = { 0,0 };	// 当前编辑器的光标位置, 是相对 drawRange 左上角而言的坐标, 可以超出屏幕 (当然此时不显示)
		size_t cursorIndex = 0;		// 对应的 fileData 中的下标, 也能现场算但缓存一个会更简单

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

		constexpr size_t NewFileDataIndexForEdit(size_t index) const
		{
			if (index < fileDataIndex)
				return formatter->SearchLineBeginIndex(fileData, index);

			auto formattedStrs = formatter->Format(wstring_view{ fileData.begin() + fileDataIndex,fileData.end() }, drawRange.Width(), drawRange.Height());

			if (index > formatter->SearchLineEndIndex(fileData, formattedStrs.datas.back().indexInRaw + fileDataIndex))
			{
				auto nowIndex = formatter->SearchLineBeginIndex(fileData, index);

				for (int lineCount = 1; lineCount < drawRange.Height() && nowIndex >= 2; lineCount++)	// TODO: 这部分最好也能适配不同行尾
					nowIndex = formatter->SearchLineBeginIndex(fileData, nowIndex - 2);

				return nowIndex;
			}

			return fileDataIndex;
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
			if (selectBeginIndex != selectEndIndex && MaxSelectIndex() >= fileDataIndex)
			{
				auto beginIndex = MinSelectIndex();
				auto endIndex = MaxSelectIndex();
				
				const auto nowScreenEndIndex = formatter->SearchLineEndIndex(fileData, formattedStrs.datas.back().indexInRaw + fileDataIndex);

				if (beginIndex > nowScreenEndIndex
					|| endIndex < fileDataIndex)
					endIndex = beginIndex = 0;
				else
				{
					if (beginIndex < fileDataIndex)
						beginIndex = 0;
					else
						beginIndex -= fileDataIndex;

					if (endIndex > nowScreenEndIndex)
						endIndex = nowScreenEndIndex;

					endIndex -= fileDataIndex;
				}

				selectBegin = formatter->GetFormattedPos(formattedStrs, beginIndex);

				selectEnd = formatter->GetFormattedPos(formattedStrs, endIndex);
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

				if (formattedStrs[y].DisplaySize() < drawRange.Width())
					console.Print(ranges::to<wstring>(ranges::views::repeat(' ', drawRange.Width() - formattedStrs[y].DisplaySize())));

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

		// 激活 “这个编辑器” 的光标
		// 会在合适的时候显示和隐藏光标, 所以无需手动调用 ShowCursor 函数
		void FlushCursor() const
		{
			if (drawRange.Contain(drawRange.leftTop + cursorPos))
			{
				console.SetCursorTo(drawRange.leftTop + cursorPos);
				console.ShowCursor();
			}
			else
				console.HideCursor();
		}

		// 这个函数是从编辑器的角度移动光标, 所以从逻辑上来说相当于用户自己把光标移到了那个位置 (通过鼠标等)
		// 因此它会破坏当前选择区域
		// TODO: 是不是不该耦合得这么死?
		void SetCursorPos(const ConsolePosition& pos, bool selectMode = false)
		{
			if (drawRange.Contain(drawRange.leftTop + pos))
			{
				const auto formattedString = formatter->Format(wstring_view{ fileData.begin() + fileDataIndex,fileData.end() },drawRange.Width(), drawRange.Height());

				if(selectMode)
				{
					if (selectBeginIndex == selectEndIndex)
						selectBeginIndex = cursorIndex;

					selectEndIndex = formatter->GetRawIndex(formattedString, pos) + fileDataIndex;

					PrintData();
				}
				else if (selectEndIndex != selectBeginIndex)
				{
					selectEndIndex = selectBeginIndex;
					PrintData();
				}

				cursorPos = pos;
				cursorIndex = formatter->GetRawIndex(formattedString, cursorPos) + fileDataIndex;

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

		// 是的, 这一块的函数在几次迭代后已经变成屎山了
		// TODO: 重构屎山
		void MoveCursor(Direction direction, bool selectMode = false)
		{
			using enum Direction;

			short deltaX = 0, deltaY = 0;

			auto newPos = cursorPos;

			auto formattedStr = formatter->Format(wstring_view{ fileData.begin() + fileDataIndex,fileData.end() },
				drawRange.Width(), drawRange.Height());

			switch (direction)
			{
			case Down:	newPos.y++; break;
			case Up:	newPos.y--; break;
			case Right:	newPos.x++; break;
			case Left:	newPos.x--; break;
			}

			// 换行, 本来在 RestrictPos 里面的, 但是逻辑上放这里合适一点
			if (direction == Left && newPos.x < 0
				&& (newPos.y > 0 || fileDataIndex >= 2))
			{
				newPos.y--;
				if (newPos.y >= 0)
					newPos.x = formattedStr[newPos.y].DisplaySize();
				else
				{
					fileDataIndex = fileData.rfind('\n', fileDataIndex - 2) + 1;	// TODO: 优化

					formattedStr = formatter->Format(wstring_view{ fileData.begin() + fileDataIndex,fileData.end() },
						drawRange.Width(), drawRange.Height());

					newPos.x = formattedStr[0].DisplaySize();
					newPos.y++;

					PrintData();
				}
				direction = None;	// 向上换行直接在这里进行, 向下换行在后面进行. 至于为什么你别管, 能跑就行
			}
			if (direction == Right && newPos.x > formattedStr[newPos.y].DisplaySize())
			{
				newPos.y++;
				newPos.x = 0;
				direction = Down;
			}

			if (cursorPos.y + 1 == drawRange.rightBottom.y && direction == Down)
			{
				fileDataIndex += formattedStr[1].indexInRaw;

				formattedStr = formatter->Format(wstring_view{ fileData.begin() + fileDataIndex,fileData.end() },
					drawRange.Width(), drawRange.Height());

				PrintData();

				direction = None;
			}
			if (cursorPos.y == 0 && direction == Up && fileDataIndex >= 2)
			{
				fileDataIndex = fileData.rfind('\n', fileDataIndex - 2) + 1;	// TODO: 优化

				PrintData();	// TODO: 适当地少 PrintData 几次 (比如现在如果既要滚动, 选择区域又发生了改变, 那就会多 PrintData 一次)

				direction = None;
			}

			SetCursorPos(formatter->RestrictPos(formattedStr, newPos, direction), selectMode);
		}

		const ConsolePosition& GetCursorPos() const
		{
			return cursorPos;
		}

		// 基于当前光标/选区位置插入
		void Insert(wstring_view str)
		{
			if (selectBeginIndex == selectEndIndex)
			{
				fileDataIndex = NewFileDataIndexForEdit(cursorIndex);

				fileData.insert_range(fileData.begin() + cursorIndex, str);

				SetCursorPos(formatter->GetFormattedPos(formatter->Format(wstring_view{ fileData.begin() + fileDataIndex,fileData.end() }, drawRange.Width(), drawRange.Height()),
					cursorIndex - fileDataIndex));
			}
			else
			{
				fileDataIndex = NewFileDataIndexForEdit(MinSelectIndex());

				fileData.replace_with_range(fileData.begin() + MinSelectIndex(), fileData.begin() + MaxSelectIndex(), str);
				
				SetCursorPos(formatter->GetFormattedPos(formatter->Format(wstring_view{ fileData.begin() + fileDataIndex,fileData.end() }, drawRange.Width(), drawRange.Height()),
					MinSelectIndex() - fileDataIndex));
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
			if (selectBeginIndex == selectEndIndex)
			{
				fileDataIndex = NewFileDataIndexForEdit(cursorIndex);

				size_t index = cursorIndex;

				if (index == 0)
					return;

				index--;

				console.HideCursor();

				SetCursorPos(formatter->GetFormattedPos(formatter->Format(wstring_view{ fileData.begin() + fileDataIndex,fileData.end() }, drawRange.Width(), drawRange.Height()),
					cursorIndex - fileDataIndex));

				MoveCursor(Direction::Left);

				if (index > 0 && fileData[index - 1] == '\r')
					fileData.erase(index - 1, 2);
				else
					fileData.erase(fileData.begin() + index);	// 这里只传 index 就会把后面的全删掉

				PrintData();

				FlushCursor();
			}
			else
			{
				fileDataIndex = NewFileDataIndexForEdit(MinSelectIndex());

				fileData.erase(fileData.begin() + MinSelectIndex(), fileData.begin() + MaxSelectIndex());

				SetCursorPos(formatter->GetFormattedPos(formatter->Format(wstring_view{ fileData.begin() + fileDataIndex,fileData.end() }, drawRange.Width(), drawRange.Height()),
					MinSelectIndex() - fileDataIndex));
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

		void ScrollScreen(int line)
		{
			if (line < 0)
			{
				if (fileDataIndex < 2)
				{
					PrintData();
					return;
				}

				fileDataIndex = fileData.rfind('\n', fileDataIndex - 2) + 1;	// TODO: 优化

				cursorPos.y++;

				if (line == -1)
					PrintData();
				else
					ScrollScreen(line + 1);
			}
			else if (line > 0)
			{
				const auto formattedStr = formatter->Format(wstring_view{ fileData.begin() + fileDataIndex,fileData.end() },
					drawRange.Width(), drawRange.Height());

				if (formattedStr.datas.size() <= 1)
				{
					PrintData();
					return;
				}

				fileDataIndex += formattedStr[1].indexInRaw;

				cursorPos.y--;

				if (line == 1)
					PrintData();
				else
					ScrollScreen(line - 1);
			}
		}

		Editor(ConsoleHandler& console, wstring& fileData, const ConsoleRect& drawRange, shared_ptr<FormatterBase> formatter = make_shared<DefaultFormatter>())
			:console(console), fileData(fileData), drawRange(drawRange), formatter(formatter)
		{
			PrintData();
		}
	};
}
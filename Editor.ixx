// Editor.ixx
// 编辑器 UI 组件, 毫无疑问的核心部分
// 但需要注意的是不只有 UI 的核心模块用到了它
// 得益于模块化的代码, 任何需要输入文本的地方都能用它 (yysy, 虽然内部非常屎山, 但几乎不用怎么改代码就能直接拿来到处用, 只能说 OOP 赛高)
// 当然大部分地方用不到其全部的功能, 但 Editor 自身不对此做处理, 须由调用方自行阻拦换行、滚屏等操作 (原因无他, 实在不想改屎山了)
// 保存等与 “输入文本” 不直接相关的代码被移出去了
export module Editor;

import std;

import ConsoleHandler;
import ConsoleTypedef;
import Formatter;
import BasicColors;
import UIComponent;

using namespace std;

export namespace NylteJ
{
	class Editor :public UIComponent
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

		ConsoleXPos beginX = 0;		// 用于横向滚屏
	private:
		constexpr size_t MinSelectIndex() const
		{
			return min(selectBeginIndex, selectEndIndex);
		}
		constexpr size_t MaxSelectIndex() const
		{
			return max(selectBeginIndex, selectEndIndex);
		}

		constexpr wstring_view NowFileData() const
		{
			return { fileData.begin() + fileDataIndex,fileData.end() };
		}

		// 返回值: 是否进行了滚动
		constexpr bool ScrollToIndex(size_t index)
		{
			if (index < fileDataIndex)
			{
				fileDataIndex = formatter->SearchLineBeginIndex(fileData, index);
				return true;
			}

			auto formattedStrs = formatter->Format(NowFileData(), drawRange.Width(), drawRange.Height(), beginX);

			if (index > formatter->SearchLineEndIndex(fileData, formattedStrs.datas.back().indexInRaw + fileDataIndex))
			{
				auto nowIndex = formatter->SearchLineBeginIndex(fileData, index);

				for (int lineCount = 1; lineCount < drawRange.Height() && nowIndex >= 2; lineCount++)	// TODO: 这部分最好也能适配不同行尾
					nowIndex = formatter->SearchLineBeginIndex(fileData, nowIndex - 2);

				fileDataIndex = nowIndex;

				return true;
			}

			return false;
		}

		constexpr bool ChangeBeginX(size_t index)
		{
			auto distToBegin = formatter->GetRawDisplaySize({ fileData.begin() + formatter->SearchLineBeginIndex(fileData, index),fileData.begin() + index });
			
			if (distToBegin < beginX)
			{
				beginX = distToBegin;
				return true;
			}
			if (distToBegin + 1 > beginX + drawRange.Width())
			{
				beginX = distToBegin - drawRange.Width() + 1;
				return true;
			}
			return false;
		}
	public:
		// 输出内容的同时会覆盖背景
		// 没错, 它也变成屎山力 (悲)
		void PrintData()
		{
			console.HideCursor();

			auto nowCursorPos = drawRange.leftTop;

			// TODO: 充分利用缓存, 不要每次都重新算
			FormattedString formattedStrs = formatter->Format(NowFileData(), drawRange.Width(), drawRange.Height(), beginX);

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

					if (selectEnd.y == y && selectEnd.x >= 0 && selectEnd.x < formattedStrs[y].DisplaySize())
						secondEnd = formatter->GetFormattedIndex(formattedStrs[y], selectEnd);
					if (selectBegin.y == y && selectBegin.x >= 0 && selectBegin.x < formattedStrs[y].DisplaySize())
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
		void SetData(wstring_view newData)
		{
			fileData = newData;
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
				const auto formattedString = formatter->Format(NowFileData(), drawRange.Width(), drawRange.Height(), beginX);

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
			SetCursorPos(formatter->RestrictPos(formatter->Format(NowFileData(), drawRange.Width(), drawRange.Height(), beginX),
				pos,
				Direction::None),
				selectMode);
			MoveCursor(Direction::None, selectMode);
		}

		// 是的, 这一块的函数在几次迭代后已经变成屎山了
		// TODO: 重构屎山
		void MoveCursor(Direction direction, bool selectMode = false)
		{
			using enum Direction;

			bool needReprintData = ScrollToIndex(cursorIndex);
			needReprintData |= ChangeBeginX(cursorIndex);

			auto formattedStr = formatter->Format(NowFileData(), drawRange.Width(), drawRange.Height(), beginX);

			auto newPos = formatter->GetFormattedPos(formattedStr, cursorIndex - fileDataIndex);

			bool delayToChangeBeginX = false;

			switch (direction)
			{
			case Down:	newPos.y++; break;
			case Up:	newPos.y--; break;
			case Right:	newPos.x++; break;
			case Left:	newPos.x--; break;
			}

			// 换行, 本来在 RestrictPos 里面的, 但是逻辑上放这里合适一点
			if (direction == Left && newPos.x < 0
				&& (newPos.y > 0 || fileDataIndex >= 2)
				&& beginX == 0)
			{
				newPos.y--;
				if (newPos.y >= 0)
				{
					newPos.x = 0;
					auto rawIndex = formatter->GetRawIndex(formattedStr, newPos);
					newPos.x = formatter->GetRawDisplaySize({	fileData.begin() + formatter->SearchLineBeginIndex(fileData, rawIndex + fileDataIndex),
																fileData.begin() + formatter->SearchLineEndIndex(fileData, rawIndex + fileDataIndex) });
					auto nowBeginX = beginX;
					needReprintData |= ChangeBeginX(formatter->SearchLineEndIndex(fileData, rawIndex + fileDataIndex));
					newPos.x -= (beginX - nowBeginX);

					formattedStr = formatter->Format(NowFileData(), drawRange.Width(), drawRange.Height(), beginX);
				}
				else
				{
					fileDataIndex = formatter->SearchLineBeginIndex(fileData, fileDataIndex - 1);	// TODO: 优化

					formattedStr = formatter->Format(NowFileData(), drawRange.Width(), drawRange.Height(), beginX);

					newPos.x = formattedStr[0].DisplaySize();
					newPos.y++;

					needReprintData = true;
				}
				direction = None;	// 向上换行直接在这里进行, 向下换行在后面进行. 至于为什么你别管, 能跑就行
			}
			if (direction == Right && newPos.x > formattedStr[newPos.y].DisplaySize()
				&& newPos.x + beginX > formatter->GetRawDisplaySize({ fileData.begin() + formatter->SearchLineBeginIndex(fileData, formatter->GetRawIndex(formattedStr, newPos) + fileDataIndex),
															fileData.begin() + formatter->SearchLineEndIndex(fileData, formatter->GetRawIndex(formattedStr, newPos) + fileDataIndex) }))
				// 实际上最后一个条件隐含了倒数第二个, 但考虑到开销以及常用程度, 还是不删掉倒数第二个了
			{
				newPos.y++;
				newPos.x = 0;
				direction = Down;

				if (beginX > 0)
					delayToChangeBeginX = true;
			}

			if (newPos.y == drawRange.rightBottom.y && direction == Down)
			{
				fileDataIndex += formattedStr[1].indexInRaw;

				formattedStr = formatter->Format(NowFileData(), drawRange.Width(), drawRange.Height(), beginX);

				needReprintData = true;

				direction = None;

				newPos.y--;
			}
			if (newPos.y < 0 && direction == Up && fileDataIndex >= 2)
			{
				fileDataIndex = fileData.rfind('\n', fileDataIndex - 2) + 1;	// TODO: 优化

				needReprintData = true;

				direction = None;

				newPos.y++;
			}

			if(delayToChangeBeginX)
			{
				needReprintData |= ChangeBeginX(formatter->SearchLineBeginIndex(fileData, formatter->GetRawIndex(formattedStr, newPos)));

				formattedStr = formatter->Format(NowFileData(), drawRange.Width(), drawRange.Height(), beginX);
			}

			// 横向滚屏

			auto nowBeginX = beginX;

			needReprintData |= ChangeBeginX(fileDataIndex + formatter->GetRawIndex(formattedStr, formatter->RestrictPos(formattedStr, newPos, direction, true)));
			newPos.x -= (beginX - nowBeginX);
			formattedStr = formatter->Format(NowFileData(), drawRange.Width(), drawRange.Height(), beginX);

			if (needReprintData)
				PrintData();

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
				ScrollToIndex(cursorIndex);
				ChangeBeginX(cursorIndex);

				fileData.insert_range(fileData.begin() + cursorIndex, str);

				SetCursorPos(formatter->GetFormattedPos(formatter->Format(NowFileData(), drawRange.Width(), drawRange.Height(), beginX), cursorIndex - fileDataIndex));
			}
			else
			{
				ScrollToIndex(MinSelectIndex());
				ChangeBeginX(MinSelectIndex());

				fileData.replace_with_range(fileData.begin() + MinSelectIndex(), fileData.begin() + MaxSelectIndex(), str);
				
				SetCursorPos(formatter->GetFormattedPos(formatter->Format(NowFileData(), drawRange.Width(), drawRange.Height(), beginX), MinSelectIndex() - fileDataIndex));
			}

			PrintData();

			if (str.size() > 1)
				SetCursorPos(cursorPos + ConsolePosition{ formatter->GetDisplaySize(wstring_view{str.begin(),str.end() - 1}), 0 });
			MoveCursor(Direction::Right);

			FlushCursor();
		}

		// 基于当前光标/选区位置删除 (等价于按一下 Backspace)
		void Erase()
		{
			if (selectBeginIndex == selectEndIndex)
			{
				ScrollToIndex(cursorIndex);
				ChangeBeginX(cursorIndex);

				size_t index = cursorIndex;

				if (index == 0)
					return;

				index--;

				console.HideCursor();

				SetCursorPos(formatter->GetFormattedPos(formatter->Format(NowFileData(), drawRange.Width(), drawRange.Height(), beginX), cursorIndex - fileDataIndex));

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
				ScrollToIndex(MinSelectIndex());
				ChangeBeginX(MinSelectIndex());

				fileData.erase(fileData.begin() + MinSelectIndex(), fileData.begin() + MaxSelectIndex());

				SetCursorPos(formatter->GetFormattedPos(formatter->Format(NowFileData(), drawRange.Width(), drawRange.Height(), beginX), MinSelectIndex() - fileDataIndex));
			}
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
				const auto formattedStr = formatter->Format(NowFileData(), drawRange.Width(), drawRange.Height(), beginX);

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

		void HScrollScreen(int line)
		{
			if (beginX < -line)
			{
				cursorPos.x -= beginX;
				beginX = 0;
			}
			else
			{
				cursorPos.x -= line;
				beginX += line;
			}
			PrintData();
		}

		// 完全重置光标到初始位置
		void ResetCursor()
		{
			beginX = 0;
			cursorIndex = 0;
			cursorPos = { 0,0 };
			fileDataIndex = 0;
		}

		void ManageInput(const InputHandler::MessageWindowSizeChanged& message, UnionHandler& handlers) override {}		// 不在这里处理
		void ManageInput(const InputHandler::MessageKeyboard& message, UnionHandler& handlers) override
		{
			using enum InputHandler::MessageKeyboard::Key;

			auto IsNormalInput = [&]()
				{
					if (message.extraKeys.Ctrl() || message.extraKeys.Alt())
						return false;

					return message.inputChar != L'\0';
				};

			switch (message.key)
			{
			case Left:
				MoveCursor(Direction::Left, message.extraKeys.Shift());
				return;
			case Up:
				MoveCursor(Direction::Up, message.extraKeys.Shift());
				return;
			case Right:
				MoveCursor(Direction::Right, message.extraKeys.Shift());
				return;
			case Down:
				MoveCursor(Direction::Down, message.extraKeys.Shift());
				return;
			case Backspace:
				Erase();
				return;
			case Enter:
				Insert(L"\n");
				return;
			case Delete:	// 姑且也是曲线救国了
			{
				auto nowCursorPos = GetCursorPos();

				console.HideCursor();
				MoveCursor(Direction::Right);
				if (nowCursorPos != GetCursorPos())	// 用这种方式来并不优雅地判断是否到达末尾, 到达末尾时按 del 应该删不掉东西
					Erase();
				console.ShowCursor();
			}
			case Esc:
				return;
			default:
				break;
			}

			if (IsNormalInput())	// 正常输入
			{
				Insert(wstring{ message.inputChar });
				return;
			}

			if (message.extraKeys.Ctrl())
				switch (message.key)
				{
				case A:
					SelectAll();
					break;
				case C:
					handlers.clipboard.Write(GetSelectedStr());
					break;
				case X:
					handlers.clipboard.Write(GetSelectedStr());
					Erase();
					break;
				case V:		// TODO: 使用 Win11 的新终端时, 需要拦截掉终端自带的 Ctrl + V, 否则此处不生效
					Insert(handlers.clipboard.Read());
					break;
				case Special1:	// Ctrl + [{	(这里的 "[{" 指键盘上的这个键, 下同)
					if (message.extraKeys.Alt())
						HScrollScreen(-1);
					else
						HScrollScreen(-3);
					break;
				case Special2:	// Ctrl + ]}
					if (message.extraKeys.Alt())
						HScrollScreen(1);
					else
						HScrollScreen(3);
					break;
				}
		}
		void ManageInput(const InputHandler::MessageMouse& message, UnionHandler& handlers) override
		{
			using enum InputHandler::MessageMouse::Type;

			if (message.LeftClick())
				RestrictedAndSetCursorPos(message.position - GetDrawRange().leftTop);
			if (message.type == Moved && message.LeftHold())
				RestrictedAndSetCursorPos(message.position - GetDrawRange().leftTop, true);

			if (message.type == VWheeled)
				ScrollScreen(message.WheelMove());
			if (message.type == HWheeled)
				HScrollScreen(-message.WheelMove());
		}

		void WhenFocused() override
		{
			PrintData();
		}

		Editor(ConsoleHandler& console, const wstring& fileData, const ConsoleRect& drawRange, shared_ptr<FormatterBase> formatter = make_shared<DefaultFormatter>())
			:console(console), fileData(fileData), drawRange(drawRange), formatter(formatter)
		{
			PrintData();
		}
};
}
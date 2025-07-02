// Editor.ixx
// 编辑器 UI 组件, 毫无疑问的核心部分
// 但需要注意的是不只有 UI 的核心模块用到了它
// 得益于模块化的代码, 任何需要输入文本的地方都能用它 (yysy, 虽然内部非常屎山, 但几乎不用怎么改代码就能直接拿来到处用, 只能说 OOP 赛高)
// 当然大部分地方用不到其全部的功能, 但 Editor 自身不对此做处理, 须由调用方自行阻拦换行、滚屏等操作 (原因无他, 实在不想改屎山了)
// 保存等与 “输入文本” 不直接相关的代码被移出去了 (虽然后面代码越写越屎山导致还是有很多不应该写在里面的功能被写进去了)
export module Editor;

import std;

import ConsoleHandler;
import ConsoleTypedef;
import Formatter;
import BasicColors;
import UIComponent;
import SettingMap;
import InputHandler;

using namespace std;

export namespace NylteJ
{
	class Editor :public UIComponent
	{
	public:
		// 用于 Ctrl + Z / Y
		// 存储一 / 多步操作 (比如相邻的插入 / 删除会被算在一起, 不然挨字符 Ctrl + Z 实在是痛苦)
		class EditOperation
		{
		public:
			enum class Type
			{
				Insert, Erase	// 选择不算操作
			};
			using enum Type;
		public:
			Type type;

			size_t index;

			shared_ptr<wstring> data;	// 用于 Insert, 考虑到 Ctrl + Z / Y 的互换, Erase 也不得不保存, 不过二者共享一份就可以了
		public:
			void ReserveOperation()	// 逆过程, 用来交换 Ctrl + Z / Y 的
			{
				switch (type)
				{
				case Insert:
					type = Erase;
					return;
				case Erase:
					type = Insert;
					return;
				default:
					unreachable();
				}
			}
			EditOperation GetReverseOperation() const
			{
				EditOperation ret = *this;

				ret.ReserveOperation();

				return ret;
			}

			// 直接对字符串进行操作, 主要用在连续的 Undo / Redo 里
			void DoOperationDirect(wstring& str) const
			{
				switch (type)
				{
				case Insert:
					str.insert_range(str.begin() + index, *data);
					return;
				case Erase:
					str.erase(str.begin() + index, str.begin() + index + data->size());
					return;
				default:
					unreachable();
				}
			}
			void DoOperation(Editor& editor) const
			{
				// 这里无需调用 SetCursorPos 或 ScrollToIndex 等, 后面 Erase / Insert 会调用的

				switch (type)
				{
				case Insert:
					editor.cursorIndex = index;
					editor.selectBeginIndex = editor.selectEndIndex = index;	// Ctrl + Z 时如果还选中着东西就需要取消选中

					editor.Insert(*data, false);
					if (data->size() > 1)		// 只有一个字符时还选中会看起来很奇怪
					{
						editor.selectBeginIndex = index;
						editor.selectEndIndex = index + data->size();
						editor.PrintData();
					}
					return;
				case Erase:
					editor.selectBeginIndex = index;
					editor.selectEndIndex = index + data->size();
					editor.Erase(false);
					return;
				default:
					unreachable();
				}
			}
		};
	private:
		ConsoleHandler& console;

		wstring fileData;

		size_t fileDataIndex = 0;	// 目前屏幕上显示的区域是从 fileData 的哪个下标开始的
		size_t fileDataLineIndex = 0;	// 类似, 但存储的是行数, 从 0 开始

		shared_ptr<FormatterBase> formatter;

		ConsolePosition cursorPos = { 0,0 };	// 当前编辑器的光标位置, 是相对 drawRange 左上角而言的坐标, 可以超出屏幕 (当然此时不显示)
		size_t cursorIndex = 0;		// 对应的 fileData 中的下标, 也能现场算但缓存一个会更简单

		size_t	selectBeginIndex = fileDataIndex,		// 这里的 "Begin" 与 "End" 都是逻辑上的 (从 Begin 选到 End), "End" 指当前光标所在/边界变化的位置
				selectEndIndex = selectBeginIndex;		// 所以 Begin 甚至可以大于 End (通过 Shift + 左方向键即可实现)

		ConsoleXPos beginX = 0;		// 用于横向滚屏

		deque<EditOperation> undoDeque;	// Ctrl + Z
		deque<EditOperation> redoDeque;	// Ctrl + Y
		// 为什么要用 deque 呢? 因为 stack 不能 pop 栈底的元素, 无法控制 stack 大小

		const SettingMap& settingMap;

		function<void(void)> lineIndexPrinter = [] {};
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

		constexpr FormattedString GetFormattedString() const
		{
			return formatter->Format(NowFileData(), drawRange.Width(), drawRange.Height(), beginX, fileDataLineIndex);
		}

		void SetFileDataIndexs(size_t newFileDataIndex)
		{
			fileDataIndex = newFileDataIndex;
			fileDataLineIndex = formatter->GetLineIndex(fileData, fileDataIndex);
			lineIndexPrinter();
		}

		// 返回值: 是否进行了滚动
		constexpr bool ScrollToIndex(size_t index)
		{
			if (index < fileDataIndex)
			{
				SetFileDataIndexs(formatter->SearchLineBeginIndex(fileData, index));

				return true;
			}

			auto formattedStrs = GetFormattedString();

			if (index > formatter->SearchLineEndIndex(fileData, formattedStrs.datas.back().indexInRaw + fileDataIndex))
			{
				auto nowIndex = formatter->SearchLineBeginIndex(fileData, index);

				for (int lineCount = 1; lineCount < drawRange.Height() && nowIndex >= 1; lineCount++)
						nowIndex = formatter->SearchLineBeginIndex(fileData, nowIndex - 1);

				SetFileDataIndexs(nowIndex);

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
				beginX = distToBegin + 1 - drawRange.Width();
				return true;
			}
			return false;
		}

		// 只有正常进行的操作才应当调用这个函数来记录, 通过 Ctrl + Z 等进行的操作不应当通过这个记录, 直接放到 redoDeque 里即可
		// (这个函数会清除 redoDeque)
		void LogOperation(EditOperation::Type type, size_t index, wstring_view data)
		{
			redoDeque.clear();

			auto mergeInsert = [&]
				{
					*(undoDeque.front().data) += data;
				};

			auto mergeErase = [&]
				{
					undoDeque.front().index = index;
					undoDeque.front().data->insert_range(undoDeque.front().data->begin(), data);
				};

			auto addOperation = [&]
				{
					EditOperation operation;

					operation.type = type;
					operation.index = index;
					operation.data = make_shared<wstring>(data.begin(), data.end());

					operation.ReserveOperation();

					undoDeque.emplace_front(operation);

					if (undoDeque.size() > GetMaxUndoStep())
					{
						const auto eraseCount = undoDeque.size() - GetMaxUndoStep();
						undoDeque.erase(undoDeque.end() - eraseCount, undoDeque.end());
					}
				};

			if (!undoDeque.empty()
				&& undoDeque.front().type == EditOperation::Type::Erase && type == EditOperation::Type::Insert
				&& undoDeque.front().index + undoDeque.front().data->size() == index
				&& data.size() == 1
				&& undoDeque.front().data->size() + data.size() <= GetMaxMergeOperationStrLen())	// 都是插入时, 融合!
			{
				if (undoDeque.front().data->back() != '\n'
					|| settingMap.Get<SettingID::SplitUndoStrWhenEnter>() == 2)		// 总融合
					mergeInsert();
				else
					if (settingMap.Get<SettingID::SplitUndoStrWhenEnter>() == 1)	// 只融合连续换行
					{
						if (data[0] == '\n'
							&& all_of(undoDeque.front().data->begin(), undoDeque.front().data->end(), [](auto&& chr) {return chr == '\n'; }))
							mergeInsert();
						else
							addOperation();
					}
					else	// 总不融合
						addOperation();
			}
			else if (!undoDeque.empty()
				&& undoDeque.front().type == EditOperation::Type::Insert && type == EditOperation::Type::Erase
				&& data.size() == 1
				&& index + data.size() == undoDeque.front().index
				&& undoDeque.front().data->size() + data.size() <= GetMaxMergeOperationStrLen())	// 都是删除时, 融合!
				mergeErase();
			else
				addOperation();
		}

		auto GetMaxMergeOperationStrLen() const
		{
			return settingMap.Get<SettingID::MaxMergeCharUndoRedo>();
		}
		auto GetMaxUndoStep() const
		{
			return settingMap.Get<SettingID::MaxUndoStep>();
		}
		auto GetMaxRedoStep() const
		{
			return settingMap.Get<SettingID::MaxRedoStep>();
		}
	public:
		// 输出内容的同时会覆盖背景
		// 没错, 它也变成屎山力 (悲)
		void PrintData()
		{
			console.HideCursor();

			auto nowCursorPos = drawRange.leftTop;

			// TODO: 充分利用缓存, 不要每次都重新算
			FormattedString formattedStrs = GetFormattedString();

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

					if (selectEnd.y == y && selectEnd.x >= 0 && selectEnd.x <= formattedStrs[y].DisplaySize())
						secondEnd = formatter->GetFormattedIndex(formattedStrs[y], selectEnd);
					if (selectBegin.y == y && selectBegin.x >= 0 && selectBegin.x <= formattedStrs[y].DisplaySize())
						secondBegin = formatter->GetFormattedIndex(formattedStrs[y], selectBegin);

					console.Print(wstring_view{ formattedStrs[y].lineData.begin(),formattedStrs[y].lineData.begin() + secondBegin }, nowCursorPos);

					console.Print(wstring_view{ formattedStrs[y].lineData.begin() + secondBegin,formattedStrs[y].lineData.begin() + secondEnd }, BasicColors::inverseColor, BasicColors::inverseColor);

					console.Print(wstring_view{ formattedStrs[y].lineData.begin() + secondEnd,formattedStrs[y].lineData.end() });
				}
				else
					console.Print(formattedStrs[y], nowCursorPos);

				if (formattedStrs[y].DisplaySize() < drawRange.Width())
					console.Print(ranges::to<wstring>(views::repeat(' ', drawRange.Width() - formattedStrs[y].DisplaySize())));

				nowCursorPos.y++;
				if (nowCursorPos.y > drawRange.rightBottom.y)
					break;
			}

			while (nowCursorPos.y <= drawRange.rightBottom.y)	// 后面的空白也要覆写
			{
				console.Print(ranges::to<wstring>(views::repeat(' ', drawRange.Width())), nowCursorPos);

				nowCursorPos.y++;
			}

			FlushCursor();		// 不加这个的话打字的时候光标有时会乱闪
		}

		wstring_view GetData() const
		{
			return fileData;
		}
		void SetData(wstring_view newData)
		{
			// 显然这会彻底地破坏 Ctrl + Z 等
			// 因为逻辑上这并不是用户的操作, 而是其它代码的操作, 此时本来也不应该保留
			// 所以直接全删掉
			undoDeque.clear();
			redoDeque.clear();

			fileData = newData;

			lineIndexPrinter();
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
				const auto formattedString = GetFormattedString();

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
			SetCursorPos(formatter->RestrictPos(formatter->Format(NowFileData(), drawRange.Width(), drawRange.Height(), beginX, fileDataLineIndex),
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

			auto formattedStr = GetFormattedString();

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
				&& (newPos.y > 0 || fileDataLineIndex >= 1)
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

					formattedStr = GetFormattedString();
				}
				else
				{
					fileDataIndex = formatter->SearchLineBeginIndex(fileData, fileDataIndex - 1);	// TODO: 优化
					fileDataLineIndex--;	// 这个无法适配自动换行, 后面几个也是, 但现在的屎山代码本来也很难加自动换行, 姑且这么写着先
					lineIndexPrinter();

					formattedStr = GetFormattedString();

					newPos.x = formattedStr[0].DisplaySize();
					newPos.y++;

					needReprintData = true;
				}
				direction = None;	// 向上换行直接在这里进行, 向下换行在后面进行. 至于为什么你别管, 能跑就行
			}
			if (direction == Right && newPos.x > formattedStr[newPos.y].DisplaySize()
				&& formatter->GetRawIndex(formattedStr, newPos) + fileDataIndex < fileData.size()
				&& newPos.x + beginX > formatter->GetRawDisplaySize({ fileData.begin() + formatter->SearchLineBeginIndex(fileData, formatter->GetRawIndex(formattedStr, newPos) + fileDataIndex),
															fileData.begin() + formatter->SearchLineEndIndex(fileData, formatter->GetRawIndex(formattedStr, newPos) + fileDataIndex) }))
				// 实际上最后一个条件隐含了第二个, 但考虑到开销以及常用程度, 还是不删掉第二个了
			{
				newPos.y++;
				direction = Down;

				if (beginX > 0)		// 此时没法轻易判断是不是已经到文件尾了, 所以直接延后 (但是 y 可以且必须直接加, 下面的逻辑乱成依托时了所以我不是很想解释但基本就是这样)
					delayToChangeBeginX = true;
				else
					newPos.x = 0;
			}

			if (newPos.y == drawRange.rightBottom.y && direction == Down)
			{
				fileDataIndex += formattedStr[1].indexInRaw;
				fileDataLineIndex++;
				lineIndexPrinter();

				formattedStr = GetFormattedString();

				needReprintData = true;

				direction = None;

				newPos.y--;
			}
			if (newPos.y < 0 && direction == Up && fileDataLineIndex >= 1)
			{
				fileDataIndex = formatter->SearchLineBeginIndex(fileData, fileDataIndex - 1);
				fileDataLineIndex--;
				lineIndexPrinter();

				needReprintData = true;

				direction = None;

				newPos.y++;
			}

			if (delayToChangeBeginX && newPos.y < formattedStr.datas.size())
			{
				newPos.x = 0;

				needReprintData |= ChangeBeginX(formatter->SearchLineBeginIndex(fileData, formatter->GetRawIndex(formattedStr, newPos)));

				formattedStr = GetFormattedString();
			}

			// 横向滚屏

			auto nowBeginX = beginX;

			needReprintData |= ChangeBeginX(fileDataIndex + formatter->GetRawIndex(formattedStr, formatter->RestrictPos(formattedStr, newPos, direction, true)));
			newPos.x -= (beginX - nowBeginX);
			formattedStr = GetFormattedString();

			if (needReprintData)
				PrintData();

			SetCursorPos(formatter->RestrictPos(formattedStr, newPos, direction), selectMode);
		}

		const ConsolePosition& GetCursorPos() const
		{
			return cursorPos;
		}

		// 基于当前光标/选区位置插入
		void Insert(wstring_view str, bool log = true)
		{
			if (selectBeginIndex != selectEndIndex)
				Erase();	// 便于 Ctrl + Z, 把 “替换” 操作改成删除 + 插入

			ScrollToIndex(cursorIndex);
			ChangeBeginX(cursorIndex);

			if (log)
				LogOperation(EditOperation::Insert, cursorIndex, str);

			fileData.insert_range(fileData.begin() + cursorIndex, str);

			MoveCursorToIndex(cursorIndex + str.size(), cursorIndex + str.size());

			if (str.contains('\n'))
				lineIndexPrinter();
		}

		// 基于当前光标/选区位置删除 (等价于按一下 Backspace)
		void Erase(bool log = true)
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

				SetCursorPos(formatter->GetFormattedPos(formatter->Format(NowFileData(), drawRange.Width(), drawRange.Height(), beginX, fileDataLineIndex), cursorIndex - fileDataIndex));

				MoveCursor(Direction::Left);

				bool needReprintLineIndex = (fileData[index] == '\n');

				if (index > 0 && fileData[index - 1] == '\r')
				{
					if (log)
						LogOperation(EditOperation::Erase, index, wstring_view{ fileData.begin() + index - 1,fileData.begin() + index + 1 });

					fileData.erase(index - 1, 2);
				}
				else
				{
					if (log)
						LogOperation(EditOperation::Erase, index, wstring_view{ fileData.begin() + index,fileData.begin() + index + 1 });

					fileData.erase(fileData.begin() + index);	// 这里只传 index 就会把后面的全删掉
				}

				PrintData();
				if(needReprintLineIndex)
					lineIndexPrinter();
			}
			else
			{
				ScrollToIndex(MinSelectIndex());
				ChangeBeginX(MinSelectIndex());

				if (log)
					LogOperation(EditOperation::Erase, MinSelectIndex(), wstring_view{ fileData.begin() + MinSelectIndex(),fileData.begin() + MaxSelectIndex() });

				bool needReprintLineIndex = wstring_view{ fileData.begin() + MinSelectIndex(), fileData.begin() + MaxSelectIndex() }.contains('\n');

				fileData.erase(fileData.begin() + MinSelectIndex(), fileData.begin() + MaxSelectIndex());

				SetCursorPos(formatter->GetFormattedPos(formatter->Format(NowFileData(), drawRange.Width(), drawRange.Height(), beginX, fileDataLineIndex), MinSelectIndex() - fileDataIndex));

				if(needReprintLineIndex)
					lineIndexPrinter();
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
				if (fileDataLineIndex < 1)
				{
					PrintData();
					lineIndexPrinter();
					return;
				}

				fileDataIndex = formatter->SearchLineBeginIndex(fileData, fileDataIndex - 1);	// TODO: 优化
				fileDataLineIndex--;

				cursorPos.y++;

				if (line == -1)
				{
					PrintData();
					lineIndexPrinter();
				}
				else
					ScrollScreen(line + 1);
			}
			else if (line > 0)
			{
				const auto formattedStr = GetFormattedString();

				if (formattedStr.datas.size() <= 1)
				{
					PrintData();
					lineIndexPrinter();
					return;
				}

				fileDataIndex += formattedStr[1].indexInRaw;
				fileDataLineIndex++;

				cursorPos.y--;

				if (line == 1)
				{
					PrintData();
					lineIndexPrinter();
				}
				else
					ScrollScreen(line - 1);
			}
		}

		void HScrollScreen(int line)
		{
			if (beginX < -line)
			{
				cursorPos.x += beginX;
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
			// 这个比较特殊, 就不合并到下面了
			beginX = cursorIndex = fileDataIndex = selectBeginIndex = selectEndIndex = fileDataLineIndex = 0;
			cursorPos = { 0,0 };
			lineIndexPrinter();

			PrintData();
		}

		void MoveCursorToIndex(size_t from, size_t to)
		{
			cursorIndex = (from + to) / 2;	// 神人解决方案 TODO: 想一个不那么神人的

			ChangeBeginX(cursorIndex);
			ScrollToIndex(cursorIndex);

			auto formattedStr = GetFormattedString();

			SetCursorPos(formatter->RestrictPos(formattedStr, formatter->GetFormattedPos(formattedStr, cursorIndex - fileDataIndex), None));

			selectBeginIndex = from;
			selectEndIndex = cursorIndex = to;

			PrintData();
		}
		void MoveCursorToIndex(size_t pos)
		{
			MoveCursorToIndex(pos, pos);
		}

		void MoveCursorToEnd()
		{
			MoveCursorToIndex(fileData.size());
		}

		template<bool direct = false>
		void Undo()
		{
			if (undoDeque.empty())
				return;

			if constexpr (direct)
				undoDeque.front().DoOperationDirect(fileData);
			else
				undoDeque.front().DoOperation(*this);

			redoDeque.emplace_front(undoDeque.front().GetReverseOperation());

			undoDeque.pop_front();

			if (redoDeque.size() > GetMaxUndoStep())
			{
				const auto eraseCount = redoDeque.size() - GetMaxUndoStep();
				redoDeque.erase(redoDeque.end() - eraseCount, redoDeque.end());
			}
		}
		void Undo(size_t step)
		{
			if (step == 0)
				return;

			while (step > 1)
			{
				Undo<true>();

				--step;
			}

			Undo();
			// 前面几次 Undo 不检查行号变化, 所以如果前几次 Undo 了换行, 但最后一次又没有, 行号显示就会出问题
			// 补偿性地, 这里不加检查地重绘行号, 下 (Redo) 同
			lineIndexPrinter();
		}

		template<bool direct = false>
		void Redo()
		{
			if (redoDeque.empty())
				return;

			if constexpr (direct)
				redoDeque.front().DoOperationDirect(fileData);
			else
				redoDeque.front().DoOperation(*this);

			undoDeque.emplace_front(redoDeque.front().GetReverseOperation());

			redoDeque.pop_front();

			if (undoDeque.size() > GetMaxUndoStep())
			{
				const auto eraseCount = undoDeque.size() - GetMaxUndoStep();
				undoDeque.erase(undoDeque.end() - eraseCount, undoDeque.end());
			}
		}
		void Redo(size_t step)
		{
			if (step == 0)
				return;

			while (step > 1)
			{
				Redo<true>();

				--step;
			}

			Redo();
			lineIndexPrinter();
		}

		auto& GetUndoDeque() const
		{
			return undoDeque;
		}
		auto& GetRedoDeque() const
		{
			return redoDeque;
		}

		vector<size_t> GetLineIndexs() const
		{
			FormattedString formattedStrs = GetFormattedString();

			vector<size_t> ret;

			for (auto&& lineData : formattedStrs.datas)
				ret.emplace_back(lineData.lineIndex);

			return ret;
		}

		void SetLineIndexPrinter(function<void(void)> printer)
		{
			lineIndexPrinter = printer;
		}

		ConsolePosition GetAbsCursorPos() const
		{
			return cursorPos + ConsolePosition{ beginX,fileDataLineIndex };
		}

		size_t GetNowLineCharCount() const
		{
			return cursorIndex - formatter->SearchLineBeginIndex(fileData, cursorIndex);
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
				if (!message.extraKeys.Ctrl())
					MoveCursor(Direction::Up, message.extraKeys.Shift());
				else
					ScrollScreen(-1);
				return;
			case Right:
				MoveCursor(Direction::Right, message.extraKeys.Shift());
				return;
			case Down:
				if (!message.extraKeys.Ctrl())
					MoveCursor(Direction::Down, message.extraKeys.Shift());
				else
					ScrollScreen(1);
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
				return;
			case Esc:
				return;
			case Home:
				ResetCursor();
				return;
			case End:
				MoveCursorToEnd();
				return;
			case PageUp:
				ScrollScreen(-drawRange.Height() + 1);
				return;
			case PageDown:
				ScrollScreen(drawRange.Height() - 1);
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
					if (selectBeginIndex != selectEndIndex)
						handlers.clipboard.Write(GetSelectedStr());
					break;
				case X:
					if (selectBeginIndex != selectEndIndex)
					{
						handlers.clipboard.Write(GetSelectedStr());
						Erase();
					}
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
				case Z:
					Undo();
					break;
				case Y:
					Redo();
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

		void WhenRefocused() override
		{
			FlushCursor();
		}

		void WhenUnfocused() override
		{
			console.HideCursor();
		}

		Editor(ConsoleHandler& console,
			   const wstring& fileData,
			   const ConsoleRect& drawRange,
			   const SettingMap& settingMap,
			   shared_ptr<FormatterBase> formatter = nullptr)
			:UIComponent(drawRange),
			console(console),
			fileData(fileData),
			formatter(formatter),
			settingMap(settingMap)
		{
			if (formatter == nullptr)
				this->formatter = make_shared<DefaultFormatter>(settingMap);
		}
};
}
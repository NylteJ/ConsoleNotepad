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
import String;

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

			String data;	// 用于 Insert, 考虑到 Ctrl + Z / Y 的互换, Erase 也不得不保存 (不过可以消除复制, 使得二者共享同一份)
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

			// 直接对字符串进行操作, 主要用在连续的 Undo / Redo 里
			void DoOperationDirect(String& str) const
			{
				switch (type)
				{
				case Insert:
					str.insert_range(str.AtByteIndex(index), data);
					return;
				case Erase:
					str.erase(str.AtByteIndex(index), str.AtByteIndex(index + data.ByteSize()));
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

					editor.Insert(data, false);
					if (!data.empty() && next(data.begin()) != data.end())		// 字符数大于 1 时才选中
					{
						editor.selectBeginIndex = index;
						editor.selectEndIndex = index + data.ByteSize();
						editor.PrintData();
					}
					return;
				case Erase:
					editor.selectBeginIndex = index;
					editor.selectEndIndex = index + data.ByteSize();
					editor.Erase(false);
					return;
				default:
					unreachable();
				}
			}

			// 防止不必要的复制开销
			EditOperation& operator=(const EditOperation&) = delete;
			EditOperation& operator=(EditOperation&&) noexcept = default;

			EditOperation(const EditOperation&) = delete;
			EditOperation(EditOperation&&) noexcept = default;
			EditOperation() = default;
		};
	private:
		ConsoleHandler& console;

		String fileData;

		ConsolePosition leftTop = { 0,0 };	// 当前编辑器视口左上角在 Formatter 的虚拟坐标系中的坐标

		unique_ptr<ColorfulFormatter> formatter;

		ConsolePosition cursorPos = { 0,0 };	// 当前编辑器的光标位置, 是相对 drawRange 左上角而言的坐标, 可以超出屏幕 (当然此时不显示)
		size_t cursorIndex = 0;		// 对应的 fileData 中的下标, 也能现场算但缓存一个会更简单

		size_t	selectBeginIndex = 0,		// 这里的 "Begin" 与 "End" 都是逻辑上的 (从 Begin 选到 End), "End" 指当前光标所在/边界变化的位置
				selectEndIndex = selectBeginIndex;		// 所以 Begin 甚至可以大于 End (通过 Shift + 左方向键即可实现)

		deque<EditOperation> undoDeque;	// Ctrl + Z
		deque<EditOperation> redoDeque;	// Ctrl + Y
		// 为什么要用 deque 呢? 因为 stack 不能 pop 栈底的元素, 无法控制 stack 大小

		const SettingMap& settingMap;

		function<void()> lineIndexPrinter = [] {};
	private:
		constexpr size_t MinSelectIndex() const
		{
			return min(selectBeginIndex, selectEndIndex);
		}
		constexpr size_t MaxSelectIndex() const
		{
			return max(selectBeginIndex, selectEndIndex);
		}

		constexpr ColorfulFormattedString GetFormattedString() const
		{
			return formatter->GetColorfulString(drawRange - drawRange.leftTop + leftTop);
		}

		// 返回值: 是否进行了滚动
		constexpr bool ScrollToPosition(ConsolePosition pos)
		{
			const auto logicRange = drawRange - drawRange.leftTop + leftTop;

			if (logicRange.Contain(pos))
				return false;

			if (logicRange.leftTop.y > pos.y)
				leftTop.y = pos.y;	// 向上滚动
			else if (logicRange.rightBottom.y < pos.y)
				leftTop.y = pos.y - drawRange.Height() + 1;	// 向下滚动

			if (logicRange.leftTop.x > pos.x)
				leftTop.x = pos.x;	// 向左滚动
			else if (logicRange.rightBottom.x < pos.x)
				leftTop.x = pos.x - drawRange.Width() + 1;	// 向右滚动

			return true;
		}
		constexpr bool ScrollToIndex(size_t index)
		{
			return ScrollToPosition(formatter->IndexToConsolePosition(index));
		}

		// 只有正常进行的操作才应当调用这个函数来记录, 通过 Ctrl + Z 等进行的操作不应当通过这个记录, 直接放到 redoDeque 里即可
		// (这个函数会清除 redoDeque)
		void LogOperation(EditOperation::Type type, size_t index, StringView data)
		{
			redoDeque.clear();

			auto mergeInsert = [&]
				{
					undoDeque.front().data += data;
				};

			auto mergeErase = [&]
				{
					undoDeque.front().index = index;
					undoDeque.front().data.insert_range(undoDeque.front().data.begin(), data);
				};

			auto addOperation = [&]
				{
					EditOperation operation;

					operation.type = type;
					operation.index = index;
					operation.data = String(data.begin(), data.end());

					operation.ReserveOperation();

					undoDeque.emplace_front(std::move(operation));

					if (undoDeque.size() > GetMaxUndoStep())
					{
						const auto eraseCount = undoDeque.size() - GetMaxUndoStep();
						undoDeque.erase(undoDeque.end() - eraseCount, undoDeque.end());
					}
				};

			if (!undoDeque.empty()
				&& undoDeque.front().type == EditOperation::Type::Erase && type == EditOperation::Type::Insert
				&& undoDeque.front().index + undoDeque.front().data.ByteSize() == index
				&& !data.empty() && next(data.begin()) == data.end()		// data.Size() == 1
				&& undoDeque.front().data.Size() + data.Size() <= GetMaxMergeOperationStrLen())	// 都是插入时, 融合!	// TODO: 给 Size 加缓存
			{
				if (undoDeque.front().data.back() != '\n'
					|| settingMap.Get<SettingID::SplitUndoStrWhenEnter>() == 2)		// 总融合
					mergeInsert();
				else
					if (settingMap.Get<SettingID::SplitUndoStrWhenEnter>() == 1)	// 只融合连续换行
					{
						if (data.front() == '\n'
							&& ranges::all_of(undoDeque.front().data, [](auto&& chr) {return chr == '\n'; }))
							mergeInsert();
						else
							addOperation();
					}
					else	// 总不融合
						addOperation();
			}
			else if (!undoDeque.empty()
				&& undoDeque.front().type == EditOperation::Type::Insert && type == EditOperation::Type::Erase
				&& !data.empty() && next(data.begin()) == data.end()		// data.Size() == 1
				&& index + data.ByteSize() == undoDeque.front().index
				&& undoDeque.front().data.Size() + data.Size() <= GetMaxMergeOperationStrLen())	// 都是删除时, 融合!	// TODO: 给 Size 加缓存
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
		void PrintData() const
		{
			console.HideCursor();

			auto nowCursorPos = drawRange.leftTop;

			// 颜色数据只要在打印时填写即可
			if (selectBeginIndex != selectEndIndex)
				formatter->SetColorRanges({ {BasicColors::inverseColor, BasicColors::inverseColor, {MinSelectIndex(), MaxSelectIndex()}} });
			else
				formatter->SetColorRanges({});

			auto formattedString = GetFormattedString();

			for (auto&& [line, extraSpaceCount] : formattedString.data)
			{
				console.SetCursorTo(nowCursorPos);

				for (auto&& [str, textColor, backgroundColor] : line)
					console.Print(str, textColor, backgroundColor);

				if (extraSpaceCount > 0)
					console.Print(String(u8' ', extraSpaceCount));

				nowCursorPos.y++;
			}

			while (nowCursorPos.y <= drawRange.rightBottom.y)	// 后面的空白也要覆写
			{
				console.Print(String(u8' ', static_cast<size_t>(drawRange.Width())), nowCursorPos);

				nowCursorPos.y++;
			}

			FlushCursor();		// 不加这个的话打字的时候光标有时会乱闪
		}

		StringView GetData() const
		{
			return fileData;
		}
		void SetData(StringView newData)
		{
			// 显然这会彻底地破坏 Ctrl + Z 等
			// 因为逻辑上这并不是用户的操作, 而是其它代码的操作, 此时本来也不应该保留
			// 所以直接全删掉
			undoDeque.clear();
			redoDeque.clear();

			fileData = newData;
			formatter->OnStringUpdate(fileData);

			//lineIndexPrinter();	// TODO
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
				if(selectMode)
				{
					if (selectBeginIndex == selectEndIndex)
						selectBeginIndex = cursorIndex;

					selectEndIndex = formatter->ConsolePositionToIndex(pos + leftTop);

					PrintData();
				}
				else if (selectEndIndex != selectBeginIndex)
				{
					selectEndIndex = selectBeginIndex;
					PrintData();
				}

				cursorPos = pos;
				cursorIndex = formatter->ConsolePositionToIndex(cursorPos + leftTop);

				FlushCursor();
			}
		}
		void RestrictedAndSetCursorPos(const ConsolePosition& pos, bool selectMode = false)
		{
			const auto newPos = formatter->RestrictPosition(pos + leftTop, Direction::None);
			const bool needReprint = ScrollToPosition(newPos);

			SetCursorPos(newPos - leftTop, selectMode);

			if (needReprint)
				PrintData();
		}

		// 移动光标, 同时会约束光标位置, 把方向设置为 None 即可只约束位置
		void MoveCursor(Direction direction, bool selectMode = false)
		{
			using enum Direction;

			bool needReprintData = ScrollToIndex(cursorIndex);

			auto newPos = formatter->IndexToConsolePosition(cursorIndex) - leftTop;

			switch (direction)
			{
			case Down:	newPos.y++; break;
			case Up:	newPos.y--; break;
			case Right:	newPos.x++; break;
			case Left:	newPos.x--; break;
			default:break;
			}

			newPos = formatter->RestrictPosition(newPos + leftTop, direction) - leftTop;

			// 滚屏在此进行
			auto lastLeftTop = leftTop;
			if (ScrollToPosition(newPos + leftTop))
			{
				newPos = newPos + lastLeftTop - leftTop;
				needReprintData = true;
			}

			SetCursorPos(newPos, selectMode);

			if (needReprintData)
			{
				PrintData();
				lineIndexPrinter();	// TODO
			}
		}

		// 基于当前光标/选区位置插入
		void Insert(StringView str, bool log = true)
		{
			if (selectBeginIndex != selectEndIndex)
				Erase();	// 便于 Ctrl + Z, 把 “替换” 操作改成删除 + 插入

			ScrollToIndex(cursorIndex);

			if (log)
				LogOperation(EditOperation::Insert, cursorIndex, str);

			fileData.insert_range(fileData.AtByteIndex(cursorIndex), str);
			formatter->OnStringUpdate(fileData, cursorIndex);

			MoveCursorToIndex(cursorIndex + str.ByteSize(), cursorIndex + str.ByteSize());

			if (str.contains(u8'\n'))
				lineIndexPrinter();
		}

		// 基于当前光标/选区位置删除 (等价于按一下 Backspace)
		void Erase(bool log = true)
		{
			if (selectBeginIndex == selectEndIndex)
			{
				ScrollToIndex(cursorIndex);

				size_t index = cursorIndex;

				if (index == 0)
					return;

				index = fileData.GetByteIndex(prev(fileData.AtByteIndex(index)));

				console.HideCursor();

				SetCursorPos(formatter->IndexToConsolePosition(cursorIndex) - leftTop);

				MoveCursor(Direction::Left);

				bool needReprintLineIndex = (*fileData.AtByteIndex(index) == '\n');

				if (index > 0 && *prev(fileData.AtByteIndex(index)) == '\r')
				{
					const auto CRLFBeginIter = prev(fileData.AtByteIndex(index));
					const auto CRLFEndIter = next(fileData.AtByteIndex(index));
					const auto CRLFBeginIndex = fileData.GetByteIndex(CRLFBeginIter);

					if (log)
						LogOperation(EditOperation::Erase, CRLFBeginIndex, StringView{ CRLFBeginIter,CRLFEndIter });

					fileData.erase(CRLFBeginIter, CRLFEndIter);
					formatter->OnStringUpdate(fileData, CRLFBeginIndex);
				}
				else
				{
					if (log)
						LogOperation(EditOperation::Erase, index, StringView{ fileData.AtByteIndex(index),next(fileData.AtByteIndex(index)) });

					fileData.erase(fileData.AtByteIndex(index));
					formatter->OnStringUpdate(fileData, index);
				}

				PrintData();
				if(needReprintLineIndex)
					lineIndexPrinter();
			}
			else
			{
				ScrollToIndex(MinSelectIndex());

				if (log)
					LogOperation(EditOperation::Erase, MinSelectIndex(), StringView{ fileData.AtByteIndex(MinSelectIndex()),fileData.AtByteIndex(MaxSelectIndex()) });

				bool needReprintLineIndex = StringView{ fileData.AtByteIndex(MinSelectIndex()), fileData.AtByteIndex(MaxSelectIndex()) }.contains('\n');

				fileData.erase(fileData.AtByteIndex(MinSelectIndex()), fileData.AtByteIndex(MaxSelectIndex()));
				formatter->OnStringUpdate(fileData, MinSelectIndex());

				SetCursorPos(formatter->IndexToConsolePosition(MinSelectIndex()) - leftTop);

				if(needReprintLineIndex)
					lineIndexPrinter();
			}
		}

		void SelectAll()
		{
			selectBeginIndex = 0;
			selectEndIndex = fileData.ByteSize();

			PrintData();

			FlushCursor();	// 虽然 VS 和 VSCode 在全选时都会把光标挪到末尾, 但有一说一我觉得不动光标在这里更好用 (而且更好写)
		}

		StringView GetSelectedStr() const
		{
			return { fileData.AtByteIndex(MinSelectIndex()),fileData.AtByteIndex(MaxSelectIndex()) };
		}
		String::Iterator GetCursorIterator() const
		{
			return fileData.AtByteIndex(cursorIndex);
		}

		void ScrollScreen(int line)
		{
			if (line < 0)
			{
				const auto delta = max(line, -leftTop.y);
				leftTop.y += delta;
				cursorPos.y -= delta;
			}
			else if (line > 0)
			{
				const auto lastLeftTopY = leftTop.y;

				leftTop.y += line;
				leftTop.y = min(formatter->RestrictPosition(leftTop).y, leftTop.y);

				cursorPos.y -= leftTop.y - lastLeftTopY;
			}

			PrintData();
			lineIndexPrinter();
		}

		void HScrollScreen(int line)
		{
			if (leftTop.x < -line)
			{
				cursorPos.x += leftTop.x;
				leftTop.x = 0;
			}
			else
			{
				cursorPos.x -= line;
				leftTop.x += line;
			}

			PrintData();
		}

		// 完全重置光标到初始位置
		void ResetCursor()
		{
			// 这个比较特殊, 就不合并到下面了
			cursorIndex = selectBeginIndex = selectEndIndex = 0;
			cursorPos = leftTop = { 0,0 };
			lineIndexPrinter();

			PrintData();
		}

		void MoveCursorToIndex(size_t from, size_t to)
		{
			cursorIndex = (from + to) / 2;	// 神人解决方案 TODO: 想一个不那么神人的

			ScrollToIndex(cursorIndex);

			SetCursorPos(formatter->RestrictPosition(formatter->IndexToConsolePosition(cursorIndex)) - leftTop);

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
			MoveCursorToIndex(fileData.ByteSize());
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

			redoDeque.emplace_front(std::move(undoDeque.front()));
			redoDeque.front().ReserveOperation();

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

			// TODO: 优化代码(包括 Redo)
			size_t changedIndex = -1;

			while (step > 1)
			{
				if (!undoDeque.empty())
					changedIndex = min(changedIndex, undoDeque.front().index);

				Undo<true>();

				--step;
			}

			formatter->OnStringUpdate(fileData, changedIndex);	// 只更新一次即可
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

			undoDeque.emplace_front(std::move(redoDeque.front()));
			undoDeque.front().ReserveOperation();

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

			size_t changedIndex = -1;

			while (step > 1)
			{
				if (!redoDeque.empty())
					changedIndex = min(changedIndex, redoDeque.front().index);

				Redo<true>();

				--step;
			}

			formatter->OnStringUpdate(fileData, changedIndex);	// 只更新一次即可
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
			vector<size_t> ret;

			// TODO: 性能优化
			for (auto pos = leftTop; formatter->RestrictPosition(pos).y == pos.y; pos.y++)
				ret.emplace_back(formatter->LineIndexOf(pos));

			return ret;
		}

		void SetLineIndexPrinter(function<void()> printer)
		{
			lineIndexPrinter = printer;
		}

		ConsolePosition GetAbsCursorPos() const
		{
			return cursorPos + leftTop;
		}

		size_t GetNowLineCharCount() const
		{
			// TODO: 性能优化
			return StringView{ fileData.AtByteIndex(formatter->LineRangeOf(formatter->LineIndexOf(cursorIndex)).first),
							fileData.AtByteIndex(cursorIndex) }.Size();
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
				Insert(u8"\n"sv);
				return;
			case Delete:	// 姑且也是曲线救国了
			{
				const auto lastCursorPos = cursorPos + leftTop;

				console.HideCursor();
				MoveCursor(Direction::Right);
				if (lastCursorPos != cursorPos + leftTop)	// 用这种方式来并不优雅地判断是否到达末尾, 到达末尾时按 del 应该删不掉东西
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
				Insert(String{ message.inputChar });
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
			{
				RestrictedAndSetCursorPos(message.position - GetDrawRange().leftTop);

				if (message.type == DoubleClicked)
				{
					// TODO: 支持双击后拖选以大范围选择(干脆留到未来大重构吧)
					const auto [wordBegin, wordEnd] = GetWord(fileData, GetCursorIterator());

					selectBeginIndex = fileData.GetByteIndex(wordBegin);
					selectEndIndex = fileData.GetByteIndex(wordEnd);

					PrintData();
				}
			}
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
			   const String& fileData,
			   const ConsoleRect& drawRange,
			   const SettingMap& settingMap,
			   unique_ptr<ColorfulFormatter> formatter = nullptr)
			:UIComponent(drawRange),
			console(console),
			fileData(fileData),
			formatter(std::move(formatter)),
			settingMap(settingMap)
		{
			if (this->formatter == nullptr)
				this->formatter = make_unique<DefaultFormatter>(this->fileData, this->settingMap);
		}
};
}
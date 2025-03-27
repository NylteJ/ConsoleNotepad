// Editor.ixx
// �༭�� UI ���, �������ʵĺ��Ĳ���
// ����Ҫע����ǲ�ֻ�� UI �ĺ���ģ���õ�����
// ������ģ�黯�Ĵ���, �κ���Ҫ�����ı��ĵط��������� (yysy, ��Ȼ�ڲ��ǳ�ʺɽ, ������������ô�Ĵ������ֱ������������, ֻ��˵ OOP ����)
// ��Ȼ�󲿷ֵط��ò�����ȫ���Ĺ���, �� Editor �����Դ�������, ���ɵ��÷������������С������Ȳ��� (ԭ������, ʵ�ڲ����ʺɽ��)
// ������� �������ı��� ��ֱ����صĴ��뱻�Ƴ�ȥ��
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

		size_t fileDataIndex = 0;	// Ŀǰ��Ļ����ʾ�������Ǵ� fileData ���ĸ��±꿪ʼ��

		ConsoleRect drawRange;

		shared_ptr<FormatterBase> formatter;

		ConsolePosition cursorPos = { 0,0 };	// ��ǰ�༭���Ĺ��λ��, ����� drawRange ���ϽǶ��Ե�����, ���Գ�����Ļ (��Ȼ��ʱ����ʾ)
		size_t cursorIndex = 0;		// ��Ӧ�� fileData �е��±�, Ҳ���ֳ��㵫����һ�������

		size_t	selectBeginIndex = fileDataIndex,		// ����� "Begin" �� "End" �����߼��ϵ� (�� Begin ѡ�� End), "End" ָ��ǰ�������/�߽�仯��λ��
				selectEndIndex = selectBeginIndex;		// ���� Begin �������Դ��� End (ͨ�� Shift + ���������ʵ��)

		ConsoleXPos beginX = 0;		// ���ں������
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

		// ����ֵ: �Ƿ�����˹���
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

				for (int lineCount = 1; lineCount < drawRange.Height() && nowIndex >= 2; lineCount++)	// TODO: �ⲿ�����Ҳ�����䲻ͬ��β
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
		// ������ݵ�ͬʱ�Ḳ�Ǳ���
		// û��, ��Ҳ���ʺɽ�� (��)
		void PrintData()
		{
			console.HideCursor();

			auto nowCursorPos = drawRange.leftTop;

			// TODO: ������û���, ��Ҫÿ�ζ�������
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

			while (nowCursorPos.y <= drawRange.rightBottom.y)	// ����Ŀհ�ҲҪ��д
			{
				console.Print(ranges::to<wstring>(ranges::views::repeat(' ', drawRange.Width())), nowCursorPos);

				nowCursorPos.y++;
			}
			// ò����ʽ�� conhost ����֭��������, ��ʱ�᷵�ؿ���̨�� 9001 ��......��������������, ���������ٵ������� (�����ⲻ�Ǻ��Ĺ���)

			FlushCursor();		// ��������Ļ����ֵ�ʱ������ʱ������
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

		// ���� ������༭���� �Ĺ��
		// ���ں��ʵ�ʱ����ʾ�����ع��, ���������ֶ����� ShowCursor ����
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

		// ��������Ǵӱ༭���ĽǶ��ƶ����, ���Դ��߼�����˵�൱���û��Լ��ѹ���Ƶ����Ǹ�λ�� (ͨ������)
		// ��������ƻ���ǰѡ������
		// TODO: �ǲ��ǲ�����ϵ���ô��?
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

		// �ǵ�, ��һ��ĺ����ڼ��ε������Ѿ����ʺɽ��
		// TODO: �ع�ʺɽ
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

			// ����, ������ RestrictPos �����, �����߼��Ϸ��������һ��
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
					fileDataIndex = formatter->SearchLineBeginIndex(fileData, fileDataIndex - 1);	// TODO: �Ż�

					formattedStr = formatter->Format(NowFileData(), drawRange.Width(), drawRange.Height(), beginX);

					newPos.x = formattedStr[0].DisplaySize();
					newPos.y++;

					needReprintData = true;
				}
				direction = None;	// ���ϻ���ֱ�����������, ���»����ں������. ����Ϊʲô����, ���ܾ���
			}
			if (direction == Right && newPos.x > formattedStr[newPos.y].DisplaySize()
				&& newPos.x + beginX > formatter->GetRawDisplaySize({ fileData.begin() + formatter->SearchLineBeginIndex(fileData, formatter->GetRawIndex(formattedStr, newPos) + fileDataIndex),
															fileData.begin() + formatter->SearchLineEndIndex(fileData, formatter->GetRawIndex(formattedStr, newPos) + fileDataIndex) }))
				// ʵ�������һ�����������˵����ڶ���, �����ǵ������Լ����ó̶�, ���ǲ�ɾ�������ڶ�����
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
				fileDataIndex = fileData.rfind('\n', fileDataIndex - 2) + 1;	// TODO: �Ż�

				needReprintData = true;

				direction = None;

				newPos.y++;
			}

			if(delayToChangeBeginX)
			{
				needReprintData |= ChangeBeginX(formatter->SearchLineBeginIndex(fileData, formatter->GetRawIndex(formattedStr, newPos)));

				formattedStr = formatter->Format(NowFileData(), drawRange.Width(), drawRange.Height(), beginX);
			}

			// �������

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

		// ���ڵ�ǰ���/ѡ��λ�ò���
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

		// ���ڵ�ǰ���/ѡ��λ��ɾ�� (�ȼ��ڰ�һ�� Backspace)
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
					fileData.erase(fileData.begin() + index);	// ����ֻ�� index �ͻ�Ѻ����ȫɾ��

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

			FlushCursor();	// ��Ȼ VS �� VSCode ��ȫѡʱ����ѹ��Ų��ĩβ, ����һ˵һ�Ҿ��ò����������������� (���Ҹ���д)
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

				fileDataIndex = fileData.rfind('\n', fileDataIndex - 2) + 1;	// TODO: �Ż�

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

		// ��ȫ���ù�굽��ʼλ��
		void ResetCursor()
		{
			beginX = 0;
			cursorIndex = 0;
			cursorPos = { 0,0 };
			fileDataIndex = 0;
		}

		void ManageInput(const InputHandler::MessageWindowSizeChanged& message, UnionHandler& handlers) override {}		// �������ﴦ��
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
			case Delete:	// ����Ҳ�����߾ȹ���
			{
				auto nowCursorPos = GetCursorPos();

				console.HideCursor();
				MoveCursor(Direction::Right);
				if (nowCursorPos != GetCursorPos())	// �����ַ�ʽ���������ŵ��ж��Ƿ񵽴�ĩβ, ����ĩβʱ�� del Ӧ��ɾ��������
					Erase();
				console.ShowCursor();
			}
			case Esc:
				return;
			default:
				break;
			}

			if (IsNormalInput())	// ��������
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
				case V:		// TODO: ʹ�� Win11 �����ն�ʱ, ��Ҫ���ص��ն��Դ��� Ctrl + V, ����˴�����Ч
					Insert(handlers.clipboard.Read());
					break;
				case Special1:	// Ctrl + [{	(����� "[{" ָ�����ϵ������, ��ͬ)
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
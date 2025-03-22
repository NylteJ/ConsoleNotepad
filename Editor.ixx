// Editor.ixx
// �༭�� UI ���, �������ʵĺ��Ĳ���
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

		size_t fileDataIndex = 0;	// Ŀǰ��Ļ����ʾ�������Ǵ� fileData ���ĸ��±꿪ʼ��

		ConsoleRect drawRange;

		shared_ptr<FormatterBase> formatter;

		ConsolePosition cursorPos = { 0,0 };	// ��ǰ�༭���Ĺ��λ��, ����� drawRange ���ϽǶ��Ե�����, ���Գ�����Ļ (��Ȼ��ʱ����ʾ)
		size_t cursorIndex = 0;		// ��Ӧ�� fileData �е��±�, Ҳ���ֳ��㵫����һ�������

		size_t	selectBeginIndex = fileDataIndex,		// ����� "Begin" �� "End" �����߼��ϵ� (�� Begin ѡ�� End), "End" ָ��ǰ�������/�߽�仯��λ��
				selectEndIndex = selectBeginIndex;		// ���� Begin �������Դ��� End (ͨ�� Shift + ���������ʵ��)
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

				for (int lineCount = 1; lineCount < drawRange.Height() && nowIndex >= 2; lineCount++)	// TODO: �ⲿ�����Ҳ�����䲻ͬ��β
					nowIndex = formatter->SearchLineBeginIndex(fileData, nowIndex - 2);

				return nowIndex;
			}

			return fileDataIndex;
		}
	public:
		// ������ݵ�ͬʱ�Ḳ�Ǳ���
		void PrintData()
		{
			console.HideCursor();

			auto nowCursorPos = drawRange.leftTop;

			// TODO: ������û���, ��Ҫÿ�ζ�������
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

		// �ǵ�, ��һ��ĺ����ڼ��ε������Ѿ����ʺɽ��
		// TODO: �ع�ʺɽ
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

			// ����, ������ RestrictPos �����, �����߼��Ϸ��������һ��
			if (direction == Left && newPos.x < 0
				&& (newPos.y > 0 || fileDataIndex >= 2))
			{
				newPos.y--;
				if (newPos.y >= 0)
					newPos.x = formattedStr[newPos.y].DisplaySize();
				else
				{
					fileDataIndex = fileData.rfind('\n', fileDataIndex - 2) + 1;	// TODO: �Ż�

					formattedStr = formatter->Format(wstring_view{ fileData.begin() + fileDataIndex,fileData.end() },
						drawRange.Width(), drawRange.Height());

					newPos.x = formattedStr[0].DisplaySize();
					newPos.y++;

					PrintData();
				}
				direction = None;	// ���ϻ���ֱ�����������, ���»����ں������. ����Ϊʲô����, ���ܾ���
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
				fileDataIndex = fileData.rfind('\n', fileDataIndex - 2) + 1;	// TODO: �Ż�

				PrintData();	// TODO: �ʵ����� PrintData ���� (�������������Ҫ����, ѡ�������ַ����˸ı�, �Ǿͻ�� PrintData һ��)

				direction = None;
			}

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

		// ���ڵ�ǰ���/ѡ��λ��ɾ�� (�ȼ��ڰ�һ�� Backspace)
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
					fileData.erase(fileData.begin() + index);	// ����ֻ�� index �ͻ�Ѻ����ȫɾ��

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
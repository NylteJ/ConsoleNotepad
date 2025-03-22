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

		ConsolePosition cursorPos = { 0,0 };	// ��ǰ�༭���Ĺ��λ��, ����� drawRange ���ϽǶ��Ե�����

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

			while (nowCursorPos.y <= drawRange.rightBottom.y)	// ����Ŀհ�ҲҪ��д
			{
				console.Print(ranges::to<wstring>(ranges::views::repeat(' ', drawRange.Width())), nowCursorPos);

				nowCursorPos.y++;
			}
			// ò����ʽ�� conhost ����֭��������, ��ʱ�᷵�ؿ���̨�� 9001 ��......��������������, ���������ٵ������� (�����ⲻ�Ǻ��Ĺ���)

			FlushCursor();		// ��������Ļ����ֵ�ʱ������ʱ������
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

		// ��������Ǵӱ༭���ĽǶ��ƶ����, ���Դ��߼�����˵�൱���û��Լ��ѹ���Ƶ����Ǹ�λ�� (ͨ������)
		// ��������ƻ���ǰѡ������
		// TODO: �ǲ��ǲ�����ϵ���ô��?
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
				fileDataIndex = fileData.rfind('\n', fileDataIndex - 2) + 1;	// TODO: �Ż�

				PrintData();	// TODO: �ʵ����� PrintData ���� (�������������Ҫ����, ѡ�������ַ����˸ı�, �Ǿͻ�� PrintData һ��)
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

		// ���ڵ�ǰ���/ѡ��λ�ò���
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
				if (MinSelectIndex() < fileDataIndex)	// ��������
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

		// ���ڵ�ǰ���/ѡ��λ��ɾ�� (�ȼ��ڰ�һ�� Backspace)
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
					fileData.erase(fileData.begin() + index);	// ����ֻ�� index �ͻ�Ѻ����ȫɾ��

				PrintData();

				FlushCursor();

				console.ShowCursor();
			}
			else
			{
				if (MinSelectIndex() < fileDataIndex)	// ����ɾ��
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

			FlushCursor();	// ��Ȼ VS �� VSCode ��ȫѡʱ����ѹ��Ų��ĩβ, ����һ˵һ�Ҿ��ò����������������� (���Ҹ���д)
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
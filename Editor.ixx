// Editor.ixx
// �༭�� UI ���, �������ʵĺ��Ĳ���
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

		size_t fileDataIndex = 0;	// Ŀǰ��Ļ����ʾ�������Ǵ� fileData ���ĸ��±꿪ʼ��

		ConsoleRect drawRange;

		shared_ptr<FormatterBase> formatter;

		ConsolePosition cursorPos = { 0,0 };	// ��ǰ�༭���Ĺ��λ��, ����� drawRange ���ϽǶ��Ե�����
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

			for (auto& [index, str] : formattedStrs.datas)
			{
				console.Print(str, nowCursorPos);

				if (str.size() < drawRange.Width())
					console.Print(ranges::to<wstring>(ranges::views::repeat(' ', drawRange.Width() - str.size())));

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

		// TODO: ��ö��, ��ͻ���������ֻ���ƶ�һ��
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
				fileDataIndex = fileData.rfind('\n', fileDataIndex - 2) + 1;	// TODO: �Ż�

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
				fileData.erase(fileData.begin() + index);	// ����ֻ�� index �ͻ�Ѻ����ȫɾ��

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
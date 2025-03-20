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
			FormattedString formattedStrs = formatter->Format(fileData, drawRange.Width(), drawRange.Height());

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
			FormattedString formattedStrs = formatter->Format(fileData, drawRange.Width(), drawRange.Height());

			// �ⲿ�ִ���Ӧ��Ų�� Formatter ����, ��������Ҫ�ľ���, TODO
			// ���������ⲿ�ִ�����Ҫ�ع�
			if (fileData[formatter->GetRawIndex(formattedStrs, fileData, cursorPos.x, cursorPos.y)] == '\t' && deltaX == 1)
			{
				cursorPos.x += 4 - cursorPos.x % 4;
				SetCursorPos(cursorPos);
				return;
			}
			if (cursorPos.x > 0 && fileData[formatter->GetRawIndex(formattedStrs, fileData, cursorPos.x, cursorPos.y)-1] == '\t' && deltaX == -1)
			{
				if (cursorPos.x % 4 != 0)
					cursorPos.x -= cursorPos.x % 4;
				else
					cursorPos.x -= 4;

				SetCursorPos(cursorPos);
				return;
			}

			if (cursorPos.x == 0 && deltaX == -1)
			{
				if (cursorPos.y > 0)
					deltaY--;
				else
					deltaX = 0;
			}
			else if (cursorPos.x == formattedStrs[cursorPos.y].DisplaySize() && deltaX == 1)
			{
				deltaY++;
				cursorPos.x = 0;
				deltaX = 0;
			}

			if (cursorPos.y == 0 && deltaY == -1)
				deltaY = 0;

			if (fileData[formatter->GetRawIndex(formattedStrs, fileData, cursorPos.x, cursorPos.y)] > 128 && deltaX == 1)
				deltaX++;
			if (cursorPos.x > 1 && fileData[formatter->GetRawIndex(formattedStrs, fileData, cursorPos.x, cursorPos.y) - 1] > 128 && deltaX == -1)
				deltaX--;

			ConsoleXPos newX = cursorPos.x + deltaX;
			ConsoleYPos newY = cursorPos.y + deltaY;

			if (newY >= formattedStrs.datas.size())
				newY = formattedStrs.datas.size() - 1;

			if (newX > formattedStrs.datas[newY].DisplaySize())
				newX = formattedStrs.datas[newY].DisplaySize();

			if (newX % 4 != 0 && fileData[formatter->GetRawIndex(formattedStrs, fileData, newX, newY) - 1] == '\t')
			{
				if (newX % 4 <= 1)
					newX -= newX % 4;
				else
					newX += 4 - newX % 4;
			}

			SetCursorPos({ newX,newY });
		}

		const ConsolePosition& GetCursorPos() const
		{
			return cursorPos;
		}

		void Insert(wstring_view str, size_t x, size_t y)
		{
			const FormattedString formattedStrs = formatter->Format(fileData, drawRange.Width(), drawRange.Height());

			const size_t index = formatter->GetRawIndex(formattedStrs, fileData, x, y);

			fileData.insert_range(fileData.begin() + index, str);

			PrintData();

			if (str.size() > 1)
				SetCursorPos(cursorPos + ConsolePosition{ str.size() - 1, 0 });
			MoveCursor(1, 0);

			FlushCursor();
		}

		void Erase(size_t x, size_t y)
		{
			const FormattedString formattedStrs = formatter->Format(fileData, drawRange.Width(), drawRange.Height());

			size_t index = formatter->GetRawIndex(formattedStrs, fileData, x, y);

			if (index == 0)
				return;

			index--;

			wchar_t delChar = fileData[index];

			if (index > 0 && fileData[index - 1] == '\r')
				fileData.erase(index - 1, 2);
			else
				fileData.erase(fileData.begin() + index);	// ����ֻ�� index �ͻ�Ѻ����ȫɾ��

			PrintData();

			if (delChar == '\t')
				SetCursorPos(cursorPos - ConsolePosition{ 4,0 });	// TODO: ������һ��
			else
				MoveCursor(-1, 0);

			FlushCursor();
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
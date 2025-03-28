// Windows.ixx
// ���� Windows
// ʵ�����ǿ���̨����ģ��, �������Ѻõ���ʾ�õ�
// �� Editor ����, ֻ�ÿ����Լ��ڲ��Ļ���, �� UI ģ�鸺������ƴ������
export module Windows;

import std;

import ConsoleHandler;
import ConsoleTypedef;
import BasicColors;
import InputHandler;
import UIComponent;

import Editor;

using namespace std;

export namespace NylteJ
{
	// ֻ��һ�����
	class BasicWindow :public UIComponent
	{
	protected:
		ConsoleHandler& console;

		ConsoleRect drawRange;

		bool nowExit = false;	// �е��, ������������
	protected:
		void EraseThis(UnionHandler& handlers)
		{
			handlers.ui.EraseComponent(this);
			nowExit = true;
		}
	public:
		void PrintFrame()
		{
			if (drawRange.Height() < 2 || drawRange.Width() < 2)	// ��Ӧ���, ����ֱ�� throw ��
				throw invalid_argument{ "����̫С��! "s };

			console.HideCursor();

			console.Print(L'��' + (ranges::views::repeat(L'��', drawRange.Width() - 2) | ranges::to<wstring>()) + L'��', drawRange.leftTop);

			for (auto y = drawRange.leftTop.y + 1; y < drawRange.rightBottom.y; y++)
				console.Print(L'��' + (ranges::views::repeat(L' ', drawRange.Width() - 2) | ranges::to<wstring>()) + L'��', { drawRange.leftTop.x,y });

			console.Print(L'��' + (ranges::views::repeat(L'��', drawRange.Width() - 2) | ranges::to<wstring>()) + L'��', { drawRange.leftTop.x,drawRange.rightBottom.y });

			console.ShowCursor();
		}

		void ManageInput(const InputHandler::MessageWindowSizeChanged& message, UnionHandler& handlers) override
		{
			// ��û������������Ӧ�´��ڴ�С, ֻ�����´��ڴ�С��������������ʱ���Ի�
			// (���ڴ�Сһ�����Ի�ò��Ҳû��Ҫ......)
			if (drawRange.rightBottom.x >= message.newSize.width || drawRange.rightBottom.y >= message.newSize.height)
			{
				EraseThis(handlers);
				return;
			}
		}
		void ManageInput(const InputHandler::MessageKeyboard& message, UnionHandler& handlers) override
		{
			using enum InputHandler::MessageKeyboard::Key;

			if (message.key == Esc)
			{
				EraseThis(handlers);
				return;
			}
		}
		void ManageInput(const InputHandler::MessageMouse& message, UnionHandler& handlers) override {}

		void WhenFocused() override
		{
			PrintFrame();
		}

		BasicWindow(ConsoleHandler& console, const ConsoleRect& drawRange)
			:console(console),
			drawRange(drawRange)
		{
		}
	};

	// ѡ��� / ����·���Ĺ��в���
	// ��������ӻ�Ҳ������
	// �޷�ʵ����
	class FilePathWindow :public BasicWindow
	{
	protected:
		Editor editor;

		wstring_view tipText;
	public:
		// �����յ���·��
		virtual void ManagePath(wstring_view path, UnionHandler& handlers) = 0;

		void ManageInput(const InputHandler::MessageWindowSizeChanged& message, UnionHandler& handlers) override
		{
			BasicWindow::ManageInput(message, handlers);	// super.jpg
		}
		void ManageInput(const InputHandler::MessageKeyboard& message, UnionHandler& handlers) override
		{
			BasicWindow::ManageInput(message, handlers);

			if (nowExit)
				return;

			using enum InputHandler::MessageKeyboard::Key;

			if (message.key == Enter)
			{
				// ˳�������������, ��ֹ�д������·�������ź�˫��б��
				wstring filePath = editor.GetData()
					| ranges::views::filter([](auto&& chr) {return chr != L'\"'; })
					| ranges::to<wstring>();
				{
					size_t pos = 0;
					while ((pos = filePath.find(L"\\\\"sv, pos)) != string::npos)
					{
						filePath.replace(pos, L"\\\\"sv.size(), L"\\"sv);
						pos += L"\\"sv.size();
					}
				}

				ManagePath(filePath, handlers);

				EraseThis(handlers);
				return;
			}

			editor.ManageInput(message, handlers);
		}
		void ManageInput(const InputHandler::MessageMouse& message, UnionHandler& handlers) override
		{
			BasicWindow::ManageInput(message, handlers);

			if (nowExit)
				return;

			editor.ManageInput(message, handlers);
		}

		void WhenFocused() override
		{
			BasicWindow::WhenFocused();

			if (drawRange.Height() >= 4)
				console.Print(tipText, { drawRange.leftTop.x + 1,drawRange.leftTop.y + 1 });
			else
				console.Print(tipText, { drawRange.leftTop.x + 1,drawRange.leftTop.y });

			editor.WhenFocused();
		}

		FilePathWindow(ConsoleHandler& console, const ConsoleRect& drawRange, wstring_view tipText = L"����·��: "sv)
			:BasicWindow(console, drawRange),
			editor(console, L""s, { {drawRange.leftTop.x + 1,drawRange.leftTop.y + 1},
									{drawRange.rightBottom.x - 1,drawRange.rightBottom.y - 1} }),
			tipText(tipText)
		{
			if (drawRange.Width() >= 4)
				editor.ChangeDrawRange({	{drawRange.leftTop.x + 1,drawRange.leftTop.y + 2},
											{drawRange.rightBottom.x - 1,drawRange.rightBottom.y - 1} });
		}
	};

	class OpenFileWindow :public FilePathWindow
	{
	private:
		function<void()> callback;
	public:
		void ManagePath(wstring_view path, UnionHandler& handlers) override
		{
			handlers.file.OpenFile(path);

			handlers.ui.mainEditor.SetData(handlers.file.ReadAll());
			handlers.ui.mainEditor.ResetCursor();

			callback();
		}

		OpenFileWindow(ConsoleHandler& console, const ConsoleRect& drawRange, function<void()> callback = [] {})
			:FilePathWindow(console, drawRange, L"���ļ�: ������·�� (��� / ���Ծ���) �����»س�: "sv),
			callback(callback)
		{
		}
	};
	class SaveFileWindow :public FilePathWindow
	{
	private:
		function<void()> callback;
	public:
		void ManagePath(wstring_view path, UnionHandler& handlers) override
		{
			handlers.file.CreateFile(path);

			handlers.file.Write(handlers.ui.mainEditor.GetData());

			callback();
		}

		SaveFileWindow(ConsoleHandler& console, const ConsoleRect& drawRange, function<void()> callback = [] {})
			:FilePathWindow(console, drawRange, L"���浽: ������·�� (��� / ���Ծ���) �����»س�: "sv),
			callback(callback)
		{
		}
	};
}
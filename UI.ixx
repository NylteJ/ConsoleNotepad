// UI.ixx
// UI ������
export module UI;

import std;

import ConsoleHandler;
import BasicColors;
import ConsoleTypedef;
import FileHandler;

import Editor;

import InputHandler;

using namespace std;

export namespace NylteJ
{
	class UI
	{
	private:
		ConsoleHandler& console;
		InputHandler& inputHandler;
		FileHandler& fileHandler;
		Editor editor;
		
		wstring title;
	public:
		void PrintTitle()
		{
			console.Print(title + (ranges::views::repeat(' ', console.GetConsoleSize().width - title.size()) | ranges::to<wstring>()), { 0,0 }, BasicColors::black, BasicColors::yellow);
		}

		UI(ConsoleHandler& consoleHandler,
			wstring& editorData,
			InputHandler& inputHandler,
			FileHandler& fileHandler,
			const wstring& title = L"ConsoleNotepad ver. 0.1     made by NylteJ"s)
			:console(consoleHandler),
			inputHandler(inputHandler),
			fileHandler(fileHandler),
			editor(	consoleHandler,
					editorData,
					{	{ 0,1 },
						{	console.GetConsoleSize().width - 1,
							console.GetConsoleSize().height - 1 } }),
			// ����ı���ʵ��д����, �Թ���������ʽ��д����, ���ɶ��Ժ��񻹲������
			title(title)
		{
			PrintTitle();

			// ���ǹ�����Ϊ�û�����ͬʱ��������Ϣ, ��������ǿ�е����д���д (��Ȼ��д������:(  )

			inputHandler.SubscribeMessage([&](const InputHandler::MessageWindowSizeChanged& message)
				{
					editor.ChangeDrawRange({ {0,1},{message.newSize.width - 1,message.newSize.height - 1} });

					PrintTitle();

					editor.PrintData();

					editor.FlushCursor();
				});

			inputHandler.SubscribeMessage([&](const InputHandler::MessageKeyboard& message)
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
						editor.MoveCursor(-1, 0);
						return;
					case Up:
						editor.MoveCursor(0, -1);
						return;
					case Right:
						editor.MoveCursor(1, 0);
						return;
					case Down:
						editor.MoveCursor(0, 1);
						return;
					case Backspace:
						editor.Erase(editor.GetCursorPos().x, editor.GetCursorPos().y);
						return;
					case Enter:
						editor.Insert(L"\n", editor.GetCursorPos().x, editor.GetCursorPos().y);
						return;
					case Esc:

						return;
					case Delete:
						//editor.Erase(editor.GetCursorPos().x + 1, editor.GetCursorPos().y);
						return;
					default:
						break;
					}

					if (IsNormalInput())	// ��������
					{
						editor.Insert(wstring{ message.inputChar }, editor.GetCursorPos().x, editor.GetCursorPos().y);
						return;
					}

					if (message.extraKeys.Ctrl() && message.key == S)
					{
						wstring_view fileData = editor.GetFileData();

						fileHandler.Write(fileData);
					}
				});

			editor.FlushCursor();
		}
	};
}
// UI.ixx
// UI 主界面
export module UI;

import std;

import ConsoleHandler;
import BasicColors;
import ConsoleTypedef;

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
		Editor editor;
		
		wstring title;
	public:
		void PrintTitle()
		{
			console.Print(title + (ranges::views::repeat(' ', console.GetConsoleSize().width - title.size()) | ranges::to<wstring>()), { 0,0 }, BasicColors::black, BasicColors::yellow);
		}

		UI(ConsoleHandler& consoleHandler, wstring& editorData, InputHandler& inputHandler, const wstring& title = L"ConsoleNotepad ver. 0.1     made by NylteJ"s)
			:console(consoleHandler),
			inputHandler(inputHandler),
			editor(	consoleHandler,
					editorData,
					{	{ 0,1 },
						{	console.GetConsoleSize().width - 1,
							console.GetConsoleSize().height - 1 } }),
			// 这里的变量实在写不开, 试过把类型显式地写出来, 但可读性好像还不如这个
			title(title)
		{
			PrintTitle();

			// 我们姑且认为用户不会同时输入多个信息, 这样可以强行当串行代码写 (不然真写不完了:(  )

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

							char key = static_cast<char>(message.key);

							if (key >= static_cast<char>(A) && key <= static_cast<char>(Z))
								return true;

							if (key >= static_cast<char>(Num0) && key <= static_cast<char>(Num9))
								return true;

							if (message.extraKeys.NumLock()
								&& key >= static_cast<char>(Numpad0) && key <= static_cast<char>(Numpad9))
								return true;

							if (message.key == Space && !message.extraKeys.Ctrl())
								return true;

							return false;
						};

					if (IsNormalInput())	// 正常输入
					{
						char keyChr = static_cast<char>(message.key);

						if (keyChr >= static_cast<char>(A) && keyChr <= static_cast<char>(Z))
						{
							if (!(message.extraKeys.Shift() xor message.extraKeys.Caplock()))
								keyChr = keyChr - 'A' + 'a';
						}
						else if (keyChr >= static_cast<char>(Numpad0) && keyChr <= static_cast<char>(Numpad9))
							keyChr = keyChr - static_cast<char>(Numpad0) + '0';
						else if (keyChr == static_cast<char>(Space))
							keyChr = ' ';

						editor.Insert(wstring{ static_cast<wchar_t>(keyChr) }, editor.GetCursorPos().x, editor.GetCursorPos().y);
					}
					else switch (message.key)
					{
					case Left:
						editor.MoveCursor(-1, 0);
						break;
					case Up:
						editor.MoveCursor(0, -1);
						break;
					case Right:
						editor.MoveCursor(1, 0);
						break;
					case Down:
						editor.MoveCursor(0, 1);
						break;
					case Backspace:
						editor.Erase(editor.GetCursorPos().x, editor.GetCursorPos().y);
						break;
					case Enter:
						editor.Insert(L"\n", editor.GetCursorPos().x, editor.GetCursorPos().y);
						break;
					default:
						break;
					}
				});

			editor.FlushCursor();
		}
	};
}
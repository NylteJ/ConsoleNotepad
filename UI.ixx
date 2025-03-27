// UI.ixx
// UI 主界面
export module UI;

import std;

import ConsoleHandler;
import BasicColors;
import ConsoleTypedef;
import FileHandler;
import ClipboardHandler;

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
		ClipboardHandler& clipboardHandler;
		Editor editor;
		
		wstring title;
	public:
		void PrintTitle(wstring_view extraText = L""sv)
		{
			console.HideCursor();

			console.Print(ranges::views::repeat(' ', console.GetConsoleSize().width) | ranges::to<wstring>(), { 0,0 }, BasicColors::black, BasicColors::yellow);

			console.Print(title, { 0,0 }, BasicColors::black, BasicColors::yellow);

			size_t extraTextDisplayLength = 0;

			for (auto chr : extraText)
				if (chr > 128)
					extraTextDisplayLength += 2;
				else
					extraTextDisplayLength++;

			if (console.GetConsoleSize().width >= extraTextDisplayLength)
				console.Print(extraText, { console.GetConsoleSize().width - extraTextDisplayLength,0 }, BasicColors::black, BasicColors::yellow);

			editor.FlushCursor();
		}

		void PrintFooter(wstring_view extraText = L""sv)
		{
			console.HideCursor();

			console.Print(ranges::views::repeat(' ', console.GetConsoleSize().width) | ranges::to<wstring>(), { 0,console.GetConsoleSize().height - 1 }, BasicColors::black, BasicColors::yellow);

			console.Print(extraText, { 0,console.GetConsoleSize().height - 1 }, BasicColors::black, BasicColors::yellow);

			editor.FlushCursor();
		}

		UI(ConsoleHandler& consoleHandler,
			wstring& editorData,
			InputHandler& inputHandler,
			FileHandler& fileHandler,
			ClipboardHandler& clipboardHandler,
			const wstring& title = L"ConsoleNotepad ver. 0.1     made by NylteJ"s)
			:console(consoleHandler),
			inputHandler(inputHandler),
			fileHandler(fileHandler),
			clipboardHandler(clipboardHandler),
			editor(	consoleHandler,
					editorData,
					{	{ 0,1 },
						{	console.GetConsoleSize().width - 1,
							console.GetConsoleSize().height - 2 } }),
			// 这里的变量实在写不开, 试过把类型显式地写出来, 但可读性好像还不如这个
			title(title)
		{
			PrintTitle();
			PrintFooter();

			static bool lastIsEsc = false;	// 这个判定目前还有点简陋, 但至少不至于让人退不出来 (真随机字符串的获取方式.jpg)

			inputHandler.SubscribeMessage([&](const InputHandler::MessageWindowSizeChanged& message)
				{
					editor.ChangeDrawRange({ {0,1},{message.newSize.width - 1,message.newSize.height - 2} });

					lastIsEsc = false;

					PrintTitle();
					PrintFooter();

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

					PrintFooter();

					//static bool lastIsEsc = false;	// 把前面那个 lastIsEsc 挪到这里, 就能见识到 “‘CL.exe’已退出，代码为 -1073741819” 的奇观了

					if (message.key != Esc)
					{
						lastIsEsc = false;
						PrintTitle();
						goto wtf;
					}
					if (lastIsEsc)
					{
						console.ClearConsole();		// 如果是通过命令行运行的, 那退出时还是有必要清屏的
						console.~ConsoleHandler();	// exit 不会调用该析构函数, 但目前的程序架构下这个函数需要被调用 (虽然我感觉这种架构不太合理, 不过可以临时先顶着)
						exit(0);
					}
					lastIsEsc = true;
					PrintTitle(L"再按一次 Esc 以退出 (不会保存!!! 请使用 Ctrl + S 手动保存)"s);
					return;
				wtf:	// 我完全搞不清是为什么, 但是上面那个 PrintTitle 不能塞进下面的 switch, 甚至不能做成 else, 不然就会报 this 为 0xFFFFFFFFFFFFFFFF (甚至都还没进 PrintTitle 函数体, 在字面量构造阶段就爆了)
						// TODO: 搞清这到底是什么问题, 这是真 wtf (把字面量换成外部变量甚至全局变量都没用, 我感觉这也实在不能有 UB 啊, 不会是 MSVC 的神必 bug 吧)

					
					switch (message.key)
					{
					case Left:
						editor.MoveCursor(Direction::Left, message.extraKeys.Shift());
						return;
					case Up:
						editor.MoveCursor(Direction::Up, message.extraKeys.Shift());
						return;
					case Right:
						editor.MoveCursor(Direction::Right, message.extraKeys.Shift());
						return;
					case Down:
						editor.MoveCursor(Direction::Down, message.extraKeys.Shift());
						return;
					case Backspace:
						editor.Erase();
						return;
					case Enter:
						editor.Insert(L"\n");
						return;
					case Delete:	// 姑且也是曲线救国了
					{
						auto nowCursorPos = editor.GetCursorPos();

						console.HideCursor();
						editor.MoveCursor(Direction::Right);
						if (nowCursorPos != editor.GetCursorPos())	// 用这种方式来并不优雅地判断是否到达末尾, 到达末尾时按 del 应该删不掉东西
							editor.Erase();
						console.ShowCursor();
					}
						return;
					default:
						break;
					}
					
					if (IsNormalInput())	// 正常输入
					{
						editor.Insert(wstring{ message.inputChar });
						return;
					}

					if (message.extraKeys.Ctrl())
						switch (message.key)
						{
						case S:
							fileHandler.Write(editor.GetFileData());
							PrintFooter(L"已保存！"s);
							break;
						case A:
							editor.SelectAll();
							break;
						case C:
							clipboardHandler.Write(editor.GetSelectedStr());
							break;
						case X:
							clipboardHandler.Write(editor.GetSelectedStr());
							editor.Erase();
							break;
						case V:		// TODO: 使用 Win11 的新终端时, 需要拦截掉终端自带的 Ctrl + V, 否则此处不生效
							editor.Insert(clipboardHandler.Read());
							break;
						case Special1:	// Ctrl + [{	(这里的 "[{" 指键盘上的这个键, 下同)
							if (message.extraKeys.Alt())
								editor.HScrollScreen(-1);
							else
								editor.HScrollScreen(-3);
							break;
						case Special2:	// Ctrl + ]}
							if (message.extraKeys.Alt())
								editor.HScrollScreen(1);
							else
								editor.HScrollScreen(3);
							break;
						}
				});

			inputHandler.SubscribeMessage([&](const InputHandler::MessageMouse& message)
				{
					using enum InputHandler::MessageMouse::Type;


					if (message.buttonStatus.any() || message.WheelMove() != 0)
					{
						PrintFooter();

						if (lastIsEsc)
						{
							lastIsEsc = false;
							PrintTitle();
						}
					}

					if (message.LeftClick())
						editor.RestrictedAndSetCursorPos(message.position - editor.GetDrawRange().leftTop);
					if (message.type == Moved && message.LeftHold())
						editor.RestrictedAndSetCursorPos(message.position - editor.GetDrawRange().leftTop, true);

					if (message.type == VWheeled)
						editor.ScrollScreen(message.WheelMove());
				});

			editor.FlushCursor();
		}
	};
}
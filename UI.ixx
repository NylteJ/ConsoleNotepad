// UI.ixx
// UI ������
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
			// ����ı���ʵ��д����, �Թ���������ʽ��д����, ���ɶ��Ժ��񻹲������
			title(title)
		{
			PrintTitle();
			PrintFooter();

			static bool lastIsEsc = false;	// ����ж�Ŀǰ���е��ª, �����ٲ����������˲����� (������ַ����Ļ�ȡ��ʽ.jpg)

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

					//static bool lastIsEsc = false;	// ��ǰ���Ǹ� lastIsEsc Ų������, ���ܼ�ʶ�� ����CL.exe�����˳�������Ϊ -1073741819�� �������

					if (message.key != Esc)
					{
						lastIsEsc = false;
						PrintTitle();
						goto wtf;
					}
					if (lastIsEsc)
					{
						console.ClearConsole();		// �����ͨ�����������е�, ���˳�ʱ�����б�Ҫ������
						console.~ConsoleHandler();	// exit ������ø���������, ��Ŀǰ�ĳ���ܹ������������Ҫ������ (��Ȼ�Ҹо����ּܹ���̫����, ����������ʱ�ȶ���)
						exit(0);
					}
					lastIsEsc = true;
					PrintTitle(L"�ٰ�һ�� Esc ���˳� (���ᱣ��!!! ��ʹ�� Ctrl + S �ֶ�����)"s);
					return;
				wtf:	// ����ȫ�㲻����Ϊʲô, ���������Ǹ� PrintTitle ������������� switch, ������������ else, ��Ȼ�ͻᱨ this Ϊ 0xFFFFFFFFFFFFFFFF (��������û�� PrintTitle ������, ������������׶ξͱ���)
						// TODO: �����⵽����ʲô����, ������ wtf (�������������ⲿ��������ȫ�ֱ�����û��, �Ҹо���Ҳʵ�ڲ����� UB ��, ������ MSVC ����� bug ��)

					
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
					case Delete:	// ����Ҳ�����߾ȹ���
					{
						auto nowCursorPos = editor.GetCursorPos();

						console.HideCursor();
						editor.MoveCursor(Direction::Right);
						if (nowCursorPos != editor.GetCursorPos())	// �����ַ�ʽ���������ŵ��ж��Ƿ񵽴�ĩβ, ����ĩβʱ�� del Ӧ��ɾ��������
							editor.Erase();
						console.ShowCursor();
					}
						return;
					default:
						break;
					}
					
					if (IsNormalInput())	// ��������
					{
						editor.Insert(wstring{ message.inputChar });
						return;
					}

					if (message.extraKeys.Ctrl())
						switch (message.key)
						{
						case S:
							fileHandler.Write(editor.GetFileData());
							PrintFooter(L"�ѱ��棡"s);
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
						case V:		// TODO: ʹ�� Win11 �����ն�ʱ, ��Ҫ���ص��ն��Դ��� Ctrl + V, ����˴�����Ч
							editor.Insert(clipboardHandler.Read());
							break;
						case Special1:	// Ctrl + [{	(����� "[{" ָ�����ϵ������, ��ͬ)
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
// UI.ixx
// UI ������
// �������Ǹ���ˮ
export module UI;

import std;

import BasicColors;
import ConsoleTypedef;
import UnionHandler;
import UIHandler;
import StringEncoder;

import UIComponent;
import Editor;
import Windows;

using namespace std;

export namespace NylteJ
{
	class UI
	{
	private:
		UnionHandler handlers;

		shared_ptr<Editor> editor;	// editor ������ uiHandler ǰ��handlers �󱻳�ʼ�� (����������)

		UIHandler<shared_ptr<UIComponent>> uiHandler;

		wstring title;
	public:
		void PrintTitle(wstring_view extraText = L""sv)
		{
			handlers.console.HideCursor();

			handlers.console.Print(ranges::views::repeat(' ', handlers.console.GetConsoleSize().width) | ranges::to<wstring>(), { 0,0 }, BasicColors::black, BasicColors::yellow);

			handlers.console.Print(title, { 0,0 }, BasicColors::black, BasicColors::yellow);

			size_t extraTextDisplayLength = 0;

			for (auto chr : extraText)
				if (chr > 128)
					extraTextDisplayLength += 2;
				else
					extraTextDisplayLength++;

			if (handlers.console.GetConsoleSize().width >= extraTextDisplayLength)
				handlers.console.Print(extraText, { handlers.console.GetConsoleSize().width - extraTextDisplayLength,0 }, BasicColors::black, BasicColors::yellow);

			uiHandler.Refocus();
		}

		void PrintFooter(wstring_view extraText = L""sv)
		{
			handlers.console.HideCursor();

			handlers.console.Print(ranges::views::repeat(' ', handlers.console.GetConsoleSize().width) | ranges::to<wstring>(),
									{ 0,handlers.console.GetConsoleSize().height - 1 },
									BasicColors::black, BasicColors::yellow);

			handlers.console.Print(extraText, { 0,handlers.console.GetConsoleSize().height - 1 }, BasicColors::black, BasicColors::yellow);

			uiHandler.Refocus();
		}

		UI(ConsoleHandler& consoleHandler,
			wstring& editorData,
			InputHandler& inputHandler,
			FileHandler& fileHandler,
			ClipboardHandler& clipboardHandler,
			const wstring& title = L"ConsoleNotepad ver. 0.5     made by NylteJ"s)
			:handlers(consoleHandler, inputHandler, fileHandler, clipboardHandler, uiHandler),
			title(title),
			editor(make_shared<Editor>(consoleHandler, editorData, ConsoleRect{ { 0,1 },
																				{ handlers.console.GetConsoleSize().width - 1,
																				  handlers.console.GetConsoleSize().height - 2 } })),
			uiHandler(*editor)
		{
			uiHandler.GiveFocusTo(editor);

			uiHandler.components.emplace(uiHandler.mainEditorDepth, editor);

			PrintTitle();
			PrintFooter();

			static bool lastIsEsc = false;	// ����ж�Ŀǰ���е��ª, �����ٲ����������˲����� (������ַ����Ļ�ȡ��ʽ.jpg)

			inputHandler.SubscribeMessage([&](const InputHandler::MessageWindowSizeChanged& message)
				{
					editor->ChangeDrawRange({ {0,1},{message.newSize.width - 1,message.newSize.height - 2} });

					lastIsEsc = false;

					PrintTitle();
					PrintFooter();

					editor->PrintData();

					uiHandler.Refocus();

					uiHandler.nowFocus->ManageInput(message, handlers);
				});

			inputHandler.SubscribeMessage([&](const InputHandler::MessageKeyboard& message)
				{
					using enum InputHandler::MessageKeyboard::Key;

					try
					{
						// �Ȱ� Esc �����, ��һ����ȫ�ֵ�, ������������ζ������˲���ȥ��
						if (message.key != Esc)
						{
							lastIsEsc = false;
							PrintTitle();
						}
						else
						{
							if (lastIsEsc)
							{
								handlers.console.ClearConsole();		// �����ͨ�����������е�, ���˳�ʱ�����б�Ҫ������
								handlers.console.~ConsoleHandler();		// exit ������ø���������, ��Ŀǰ�ĳ���ܹ������������Ҫ������ (��Ȼ�Ҹо����ּܹ���̫����, ����������ʱ�ȶ���)
								exit(0);
							}
							lastIsEsc = true;
							PrintTitle(L"�ٰ�һ�� Esc ���˳� (���ᱣ��!!! ��ʹ�� Ctrl + S �ֶ�����)"s);
						}

						if (uiHandler.nowFocus == editor)
						{
							if (message.extraKeys.Ctrl())
							{
								if (message.key == S)
								{
									handlers.file.Write(editor->GetData());
									PrintFooter(L"�ѱ���! "sv);
								}
								else if (message.key == O)
								{
									PrintFooter(L"���ļ�......"sv);
									auto window = make_shared<WindowWithEditor>(handlers.console,
										ConsoleRect{ {handlers.console.GetConsoleSize().width * 0.25,handlers.console.GetConsoleSize().height * 0.33},
														{handlers.console.GetConsoleSize().width * 0.75,handlers.console.GetConsoleSize().height * 0.67} });
									uiHandler.GiveFocusTo(window);
									uiHandler.components.emplace(uiHandler.normalWindowDepth, window);
								}
							}
						}
						else
							PrintFooter();

						uiHandler.nowFocus->ManageInput(message, handlers);
					}
					catch (wstring e)
					{
						// TODO: ��һϵ���쳣��, ������ֱ�� throw wstring
						PrintFooter(L"�����쳣: "s + e);
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

					uiHandler.nowFocus->ManageInput(message, handlers);
				});

			uiHandler.Refocus();
		}
	};
}
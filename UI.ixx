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
import Exceptions;
import Utils;

import UIComponent;
import Editor;
import Windows;

using namespace std;

export namespace NylteJ
{
	class UI
	{
	private:
		constexpr static auto autoSaveDuration = 3min;
	private:
		UnionHandler handlers;

		FileHandler autoSaveFile = { true };

		shared_ptr<Editor> editor;	// editor ������ uiHandler ǰ��handlers �󱻳�ʼ�� (����������)

		UIHandler<shared_ptr<UIComponent>> uiHandler;

		wstring title;
		
		size_t lastSaveDataHash = hash<wstring_view>{}(L""sv);	// ֻ�� Hash

		chrono::time_point<chrono::steady_clock> lastSaveTime = chrono::steady_clock::now();
	private:
		void WhenFileSaved()
		{
			size_t nowDataHash = hash<wstring_view>{}(editor->GetData());

			if (nowDataHash != lastSaveDataHash)
			{
				lastSaveDataHash = nowDataHash;
				lastSaveTime = chrono::steady_clock::now();
				PrintFooter(L"�ѱ���! "sv);
			}
		}
		void WhenFileOpened()
		{
			lastSaveDataHash = hash<wstring_view>{}(editor->GetData());
			lastSaveTime = chrono::steady_clock::now();

			autoSaveFile.CreateFile(handlers.file.nowFilePath.concat(L".autosave"sv), true);

			PrintFooter(L"�Ѵ�! "sv);
		}

		bool IsFileSaved()
		{
			return hash<wstring_view>{}(editor->GetData()) == lastSaveDataHash;
		}

		void OpenFile(Encoding encoding = Encoding::UTF8)
		{
			PrintFooter(L"���ļ�......"sv);
			auto window = make_shared<OpenFileWindow>(handlers.console,
				ConsoleRect{	{handlers.console.GetConsoleSize().width * 0.25,handlers.console.GetConsoleSize().height * 0.33},
								{handlers.console.GetConsoleSize().width * 0.75,handlers.console.GetConsoleSize().height * 0.67} },
				encoding,
				bind(&UI::WhenFileOpened, this));
			uiHandler.components.emplace(uiHandler.normalWindowDepth, window);
			uiHandler.GiveFocusTo(window);
		}

		void NewFile()
		{
			editor->SetData(L""sv);
			editor->ResetCursor();

			handlers.file.CloseFile();
			autoSaveFile.CreateFile(handlers.file.nowFilePath.concat(L"__Unnamed_NewFile.autosave"sv), true);

			lastSaveDataHash = hash<wstring_view>{}(L""sv);
			lastSaveTime = chrono::steady_clock::now();

			PrintFooter(L"���½��ļ�!"sv);
		}

		void AskIfSave(function<void(size_t)> callback)
		{
			auto window = make_shared<SaveOrNotWindow>(handlers.console,
				ConsoleRect{	{handlers.console.GetConsoleSize().width * 0.25,handlers.console.GetConsoleSize().height * 0.33},
								{handlers.console.GetConsoleSize().width * 0.75,handlers.console.GetConsoleSize().height * 0.67} },
				callback);
			uiHandler.components.emplace(uiHandler.normalWindowDepth, window);
			uiHandler.GiveFocusTo(window);
		}

		void Exit(bool forced = false)
		{
			handlers.console.ClearConsole();		// �����ͨ�����������е�, ���˳�ʱ�����б�Ҫ������
			handlers.console.~ConsoleHandler();		// exit ������ø���������, ��Ŀǰ�ĳ���ܹ������������Ҫ������ (��Ȼ�Ҹо����ּܹ���̫����, ����������ʱ�ȶ���)

			if (!forced)
				autoSaveFile.~FileHandler();

			exit(0);
		}

		void AutoSave()
		{
			autoSaveFile.Write(editor->GetData());
			lastSaveTime = chrono::steady_clock::now();
		}
	public:
		void PrintTitle(wstring_view extraText = L""sv)
		{
			handlers.console.HideCursor();

			handlers.console.Print(ranges::views::repeat(' ', handlers.console.GetConsoleSize().width) | ranges::to<wstring>(), { 0,0 }, BasicColors::black, BasicColors::yellow);

			handlers.console.Print(title, { 0,0 }, BasicColors::black, BasicColors::yellow);

			size_t extraTextDisplayLength = 0;

			for (auto chr : extraText)
				if (IsWideChar(chr))
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

			wstring rightText;
			if (chrono::steady_clock::now() - lastSaveTime >= 1min)
				rightText = format(L"�ϴα���: {} ǰ. �Զ���������: {}"sv,
					chrono::duration_cast<chrono::minutes>(chrono::steady_clock::now() - lastSaveTime),
					autoSaveDuration);
			else
				rightText = format(L"�ϴα���: {} ǰ. �Զ���������: {}"sv,
					chrono::duration_cast<chrono::seconds>(chrono::steady_clock::now() - lastSaveTime),
					autoSaveDuration);

			size_t rightTextLen = 0;
			for (auto chr : rightText)
				if (IsWideChar(chr))
					rightTextLen += 2;
				else
					rightTextLen++;

			if (rightTextLen <= handlers.console.GetConsoleSize().width)
				handlers.console.Print(rightText,
					{ handlers.console.GetConsoleSize().width - rightTextLen,handlers.console.GetConsoleSize().height - 1 },
					BasicColors::black,
					BasicColors::yellow);

			uiHandler.Refocus();
		}

		UI(ConsoleHandler& consoleHandler,
			wstring& editorData,
			InputHandler& inputHandler,
			FileHandler& fileHandler,
			ClipboardHandler& clipboardHandler,
			const wstring& title = L"ConsoleNotepad ver. 0.8     made by NylteJ"s)
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

					editor->WhenFocused();

					uiHandler.GiveFocusTo(uiHandler.nowFocus);

					uiHandler.nowFocus->ManageInput(message, handlers);
				});

			inputHandler.SubscribeMessage([&](const InputHandler::MessageKeyboard& message)
				{
					using enum InputHandler::MessageKeyboard::Key;

					try
					{
						if (lastIsEsc)
							if (message.key != Esc)
							{
								lastIsEsc = false;
								PrintTitle();
							}
							else
								Exit(!IsFileSaved());

						if (uiHandler.nowFocus == editor)
						{

							if (message.key == Esc)
							{
								lastIsEsc = true;

								if (IsFileSaved())
									PrintTitle(L"�ٰ�һ�� Esc ���˳� (��ǰ�����ѱ���)"s);
								else
								{
									PrintTitle(L"�ٰ�һ�� Esc ��ǿ���˳� (��ǰ����δ����!!!)"s);

									AskIfSave([&](size_t) {Exit(); });

									return;		// ���ص��˴� Esc, ��Ȼ�����Ĵ��ڻ�ֱ������
								}
							}

							if (chrono::steady_clock::now() - lastSaveTime >= autoSaveDuration)
								AutoSave();

							if (message.extraKeys.Ctrl())
							{
								if (message.key == S)
								{
									if (message.extraKeys.Shift() || !handlers.file.Valid())	// Ctrl + Shift + S �������ļ�
									{
										PrintFooter(L"ѡ�񱣴�·��......"sv);
										auto window = make_shared<SaveFileWindow>(handlers.console,
											ConsoleRect{	{handlers.console.GetConsoleSize().width * 0.25,handlers.console.GetConsoleSize().height * 0.33},
															{handlers.console.GetConsoleSize().width * 0.75,handlers.console.GetConsoleSize().height * 0.67} },
											bind(&UI::WhenFileSaved, this));
										uiHandler.components.emplace(uiHandler.normalWindowDepth, window);
										uiHandler.GiveFocusTo(window);
									}
									else
									{
										handlers.file.Write(editor->GetData());
										WhenFileSaved();
									}
								}
								else if (message.key == O)
								{
									if (!IsFileSaved())
									{
										AskIfSave([&](size_t index){OpenFile();});
										return;
									}
									OpenFile();
								}
								else if (message.key == N)
								{
									if (!IsFileSaved())
									{
										AskIfSave([&](size_t index) {NewFile(); });
										return;
									}
									NewFile();
								}
								else if (message.key == F)	// ����Ҳ�ŵ� UI ����, ��Ϊ�����Ĳ��ҿ���Ҳ�� Editor, ���ҹ��ܷŵ� Editor ���ѭ������. ����Ҳֻ�������� Editor ��Ҫ���в���
								{
									auto window = make_shared<FindWindow>(handlers.console,
										ConsoleRect{	{handlers.console.GetConsoleSize().width * 0.65,1},
														{handlers.console.GetConsoleSize().width - 1,handlers.console.GetConsoleSize().height * 0.35} });
									uiHandler.components.emplace(uiHandler.normalWindowDepth, window);
									uiHandler.GiveFocusTo(window);
								}
								else if (message.key == H)
								{
									auto window = make_shared<ReplaceWindow>(handlers.console,
										ConsoleRect{	{handlers.console.GetConsoleSize().width * 0.65,1},
														{handlers.console.GetConsoleSize().width - 1,handlers.console.GetConsoleSize().height * 0.5} });
									uiHandler.components.emplace(uiHandler.normalWindowDepth, window);
									uiHandler.GiveFocusTo(window);
								}
								else
									PrintFooter();
							}
							else
								PrintFooter();
						}
						else
							/*PrintFooter()*/;

						uiHandler.nowFocus->ManageInput(message, handlers);
					}
					catch (Exception& e)
					{
						PrintFooter(L"�����쳣: "s + e.What());
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

			NewFile();

			uiHandler.GiveFocusTo(uiHandler.nowFocus);
		}
	};
}
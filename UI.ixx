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
import SettingMap;

import UIComponent;
import Editor;
import Windows;
import SettingWindow;

using namespace std;

export namespace NylteJ
{
	class UI
	{
	private:
		UnionHandler handlers;

		FileHandler autoSaveFile = { true };

		shared_ptr<Editor> editor;	// editor ������ uiHandler ǰ��handlers �󱻳�ʼ�� (����������)

		UIHandler<shared_ptr<UIComponent>> uiHandler;

		wstring title;
		
		size_t lastSaveDataHash = hash<wstring_view>{}(L""sv);	// ֻ�� Hash

		chrono::time_point<chrono::steady_clock> lastSaveTime = chrono::steady_clock::now();
	private:
		void WhenFileSaved(bool reSave = false)
		{
			size_t nowDataHash = hash<wstring_view>{}(editor->GetData());

			if (reSave || nowDataHash != lastSaveDataHash)
			{
				lastSaveDataHash = nowDataHash;
				lastSaveTime = chrono::steady_clock::now();

				wstring filename = handlers.file.nowFilePath.filename();

				PrintFooter(format(L"�ѱ��浽 {} !"sv, filename));
			}
		}
		void WhenFileOpened()
		{
			lastSaveDataHash = hash<wstring_view>{}(editor->GetData());
			lastSaveTime = chrono::steady_clock::now();

			wstring filename = handlers.file.nowFilePath.filename();

			auto tempPath = handlers.file.nowFilePath;

			autoSaveFile.CreateFile(tempPath.concat(L".autosave"sv), true);		// concat ��ı�ԭ path

			PrintFooter(format(L"�Ѵ� {} !"sv, filename));
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
				handlers.settings,
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
				handlers.settings,
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

			wstring rightText;

			auto leftTime = chrono::steady_clock::now() - lastSaveTime;
			auto saveDuration = handlers.settings.Get<SettingID::AutoSavingDuration>() * 1s;

			if (leftTime >= 1min)
				rightText = format(L"�ϴα���: {} ǰ. "sv, chrono::duration_cast<chrono::minutes>(leftTime));
			else
				rightText = format(L"�ϴα���: {} ǰ. "sv, chrono::duration_cast<chrono::seconds>(leftTime));

			if (saveDuration >= 1h)
				rightText += format(L"�Զ���������: {}"sv, chrono::duration_cast<chrono::hours>(saveDuration));
			else if (saveDuration >= 1min)
				rightText += format(L"�Զ���������: {}"sv, chrono::duration_cast<chrono::minutes>(saveDuration));
			else
				rightText += format(L"�Զ���������: {}"sv, chrono::duration_cast<chrono::seconds>(saveDuration));

			size_t rightTextLen = 0;
			for (auto chr : rightText)
				if (IsWideChar(chr))
					rightTextLen += 2;
				else
					rightTextLen++;

			handlers.console.Print(ranges::views::repeat(' ', handlers.console.GetConsoleSize().width - rightTextLen) | ranges::to<wstring>(),
									{ 0,handlers.console.GetConsoleSize().height - 1 },
									BasicColors::black, BasicColors::yellow);

			handlers.console.Print(extraText, { 0,handlers.console.GetConsoleSize().height - 1 }, BasicColors::black, BasicColors::yellow);

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
			SettingMap& settingMap,
			const wstring& title = L"ConsoleNotepad ver. 0.85    made by NylteJ"s)
			:handlers(consoleHandler, inputHandler, fileHandler, clipboardHandler, uiHandler, settingMap),
			editor(make_shared<Editor>(consoleHandler, editorData, ConsoleRect{ { 0,1 },
																				{ handlers.console.GetConsoleSize().width - 1,
																				  handlers.console.GetConsoleSize().height - 2 } },
				settingMap)),
			uiHandler(*editor),
			title(title)
		{
			uiHandler.GiveFocusTo(editor);

			uiHandler.components.emplace(uiHandler.mainEditorDepth, editor);

			PrintTitle();
			PrintFooter();

			static bool lastIsEsc = false;	// ����ж�Ŀǰ���е��ª, �����ٲ����������˲����� (������ַ����Ļ�ȡ��ʽ.jpg)

			inputHandler.SubscribeMessage([&](const InputHandler::MessageWindowSizeChanged& message)
				{
					// ò����ʽ�� conhost ����֭��������, ��ʱ�᷵�ؿ���̨�� 9001 ��......
					// ����ֱ�� handlers.console.GetConsoleSize() ���ص�����������.
					// ��������һ�� dirty ��Ľ��������
					// ˳��һ����ʽ�� conhost ��Ҫ�����ұ߿�Żᴥ������¼�, �����±߿��ǲ�������, ����
					InputHandler::MessageWindowSizeChanged newMessage = message;

					if (newMessage.newSize.height >= 1000)
						newMessage.newSize = handlers.console.GetConsoleSize();
					
					editor->SetDrawRange({ {0,1},{newMessage.newSize.width - 1,newMessage.newSize.height - 2} });

					lastIsEsc = false;

					PrintTitle();
					PrintFooter();

					editor->WhenFocused();

					uiHandler.GiveFocusTo(uiHandler.nowFocus);

					uiHandler.nowFocus->ManageInput(newMessage, handlers);
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

						if (chrono::steady_clock::now() - lastSaveTime >= handlers.settings.Get<SettingID::AutoSavingDuration>() * 1s)
							AutoSave();

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
											settingMap,
											bind(&UI::WhenFileSaved, this, true));
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
									ConsoleRect windowRange = { {handlers.console.GetConsoleSize().width * 0.65,1},
																{handlers.console.GetConsoleSize().width - 1,handlers.console.GetConsoleSize().height * 0.35} };
									if (windowRange.Height() < 8 && handlers.console.GetConsoleSize().height >= 10)
										windowRange.rightBottom.y = windowRange.leftTop.y + 7;

									auto window = make_shared<FindReplaceWindow>(handlers.console,
										windowRange,
										settingMap,
										editor->GetSelectedStr() | ranges::to<wstring>(),
										true);
									uiHandler.components.emplace(uiHandler.normalWindowDepth, window);
									uiHandler.GiveFocusTo(window);
								}
								else if (message.key == H)
								{
									ConsoleRect windowRange = { {handlers.console.GetConsoleSize().width * 0.65,1},
																{handlers.console.GetConsoleSize().width - 1,handlers.console.GetConsoleSize().height * 0.5} };
									if (windowRange.Height() < 10 && handlers.console.GetConsoleSize().height >= 12)
										windowRange.rightBottom.y = windowRange.leftTop.y + 9;

									auto window = make_shared<FindReplaceWindow>(handlers.console,
										windowRange,
										settingMap,
										editor->GetSelectedStr() | ranges::to<wstring>(),
										false);
									uiHandler.components.emplace(uiHandler.normalWindowDepth, window);
									uiHandler.GiveFocusTo(window);
								}
								else if (message.key == P)	// ����
								{
									auto window = make_shared<SettingWindow>(handlers.console,
										ConsoleRect{	{handlers.console.GetConsoleSize().width * 0.15,handlers.console.GetConsoleSize().height * 0.15},
														{handlers.console.GetConsoleSize().width * 0.85,handlers.console.GetConsoleSize().height * 0.85} },
										settingMap);
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
							PrintFooter();

						uiHandler.nowFocus->WhenRefocused();
						uiHandler.nowFocus->ManageInput(message, handlers);
					}
					catch (Exception& e)
					{
						PrintFooter(L"�����쳣: "s + e.What());
					}
					catch (exception& e)
					{
						PrintFooter(L"�����쳣: "s + StrToWStr(e.what(), Encoding::GB2312, true));	// ���ﲻ�������쳣��, ��Ȼ terminate ��
					}
				});

			inputHandler.SubscribeMessage([&](const InputHandler::MessageMouse& message)
				{
					using enum InputHandler::MessageMouse::Type;

					try
					{
						if (message.LeftClick() || message.RightClick() || message.WheelMove() != 0)
						{
							PrintFooter();

							if (lastIsEsc)
							{
								lastIsEsc = false;
								PrintTitle();
							}
						}

						uiHandler.nowFocus->ManageInput(message, handlers);
					}
					catch (Exception& e)
					{
						PrintFooter(L"�����쳣: "s + e.What());
					}
					catch (exception& e)
					{
						PrintFooter(L"�����쳣: "s + StrToWStr(e.what(), Encoding::GB2312, true));	// ���ﲻ�������쳣��, ��Ȼ terminate ��
					}
				});

			if (handlers.file.Valid())
			{
				try
				{
					editor->SetData(handlers.file.ReadAll());
					WhenFileOpened();
				}
				catch (WrongEncodingException&)
				{
					NewFile();
					PrintFooter(L"�������, �ļ�û�ܴ�, ���Զ��½��ļ�!"sv);
				}
			}
			else
				NewFile();

			uiHandler.GiveFocusTo(uiHandler.nowFocus);
		}
	};
}
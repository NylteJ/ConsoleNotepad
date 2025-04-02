// UI.ixx
// UI 主界面
// 基本就是个胶水
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

		shared_ptr<Editor> editor;	// editor 必须在 uiHandler 前、handlers 后被初始化 (否则会空引用)

		UIHandler<shared_ptr<UIComponent>> uiHandler;

		wstring title;
		
		size_t lastSaveDataHash = hash<wstring_view>{}(L""sv);	// 只存 Hash

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

				PrintFooter(format(L"已保存到 {} !"sv, filename));
			}
		}
		void WhenFileOpened()
		{
			lastSaveDataHash = hash<wstring_view>{}(editor->GetData());
			lastSaveTime = chrono::steady_clock::now();

			wstring filename = handlers.file.nowFilePath.filename();

			auto tempPath = handlers.file.nowFilePath;

			autoSaveFile.CreateFile(tempPath.concat(L".autosave"sv), true);		// concat 会改变原 path

			PrintFooter(format(L"已打开 {} !"sv, filename));
		}

		bool IsFileSaved()
		{
			return hash<wstring_view>{}(editor->GetData()) == lastSaveDataHash;
		}

		void OpenFile(Encoding encoding = Encoding::UTF8)
		{
			PrintFooter(L"打开文件......"sv);
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

			PrintFooter(L"已新建文件!"sv);
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
			handlers.console.ClearConsole();		// 如果是通过命令行运行的, 那退出时还是有必要清屏的
			handlers.console.~ConsoleHandler();		// exit 不会调用该析构函数, 但目前的程序架构下这个函数需要被调用 (虽然我感觉这种架构不太合理, 不过可以临时先顶着)

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
				rightText = format(L"上次保存: {} 前. "sv, chrono::duration_cast<chrono::minutes>(leftTime));
			else
				rightText = format(L"上次保存: {} 前. "sv, chrono::duration_cast<chrono::seconds>(leftTime));

			if (saveDuration >= 1h)
				rightText += format(L"自动保存周期: {}"sv, chrono::duration_cast<chrono::hours>(saveDuration));
			else if (saveDuration >= 1min)
				rightText += format(L"自动保存周期: {}"sv, chrono::duration_cast<chrono::minutes>(saveDuration));
			else
				rightText += format(L"自动保存周期: {}"sv, chrono::duration_cast<chrono::seconds>(saveDuration));

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

			static bool lastIsEsc = false;	// 这个判定目前还有点简陋, 但至少不至于让人退不出来 (真随机字符串的获取方式.jpg)

			inputHandler.SubscribeMessage([&](const InputHandler::MessageWindowSizeChanged& message)
				{
					// 貌似老式的 conhost 有蜜汁兼容问题, 有时会返回控制台有 9001 行......
					// 但是直接 handlers.console.GetConsoleSize() 返回的又是正常的.
					// 这里先用一个 dirty 点的解决方案吧
					// 顺便一提老式的 conhost 需要拉左右边框才会触发这个事件, 拉上下边框是不触发的, 很迷
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
									PrintTitle(L"再按一次 Esc 以退出 (当前内容已保存)"s);
								else
								{
									PrintTitle(L"再按一次 Esc 以强制退出 (当前内容未保存!!!)"s);

									AskIfSave([&](size_t) {Exit(); });

									return;		// 拦截掉此次 Esc, 不然弹出的窗口会直接趋势
								}
							}

							if (message.extraKeys.Ctrl())
							{
								if (message.key == S)
								{
									if (message.extraKeys.Shift() || !handlers.file.Valid())	// Ctrl + Shift + S 或是新文件
									{
										PrintFooter(L"选择保存路径......"sv);
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
								else if (message.key == F)	// 查找也放到 UI 里了, 因为弹出的查找框里也有 Editor, 查找功能放到 Editor 里会循环依赖. 而且也只有最外层的 Editor 需要进行查找
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
								else if (message.key == P)	// 设置
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
						PrintFooter(L"发生异常: "s + e.What());
					}
					catch (exception& e)
					{
						PrintFooter(L"发生异常: "s + StrToWStr(e.what(), Encoding::GB2312, true));	// 这里不能再抛异常了, 不然 terminate 了
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
						PrintFooter(L"发生异常: "s + e.What());
					}
					catch (exception& e)
					{
						PrintFooter(L"发生异常: "s + StrToWStr(e.what(), Encoding::GB2312, true));	// 这里不能再抛异常了, 不然 terminate 了
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
					PrintFooter(L"编码错误, 文件没能打开, 已自动新建文件!"sv);
				}
			}
			else
				NewFile();

			uiHandler.GiveFocusTo(uiHandler.nowFocus);
		}
	};
}
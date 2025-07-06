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
import FileHandler;
import ConsoleHandler;
import InputHandler;
import ClipboardHandler;
import String;

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

		String title;
		
		size_t lastSaveDataHash = hash<u8string_view>{}(u8""sv);	// 只存 Hash

		chrono::time_point<chrono::steady_clock> lastSaveTime = chrono::steady_clock::now();
	private:
		ConsoleRect WindowDrawRangeNormalSize() const
		{
			return ConsoleRect{ {handlers.console.GetConsoleSize().width * 0.25,handlers.console.GetConsoleSize().height * 0.33},
								{handlers.console.GetConsoleSize().width * 0.75,handlers.console.GetConsoleSize().height * 0.67} };
		}
		ConsoleRect WindowDrawRangeLargeSize() const
		{
			return ConsoleRect{ {handlers.console.GetConsoleSize().width * 0.15,handlers.console.GetConsoleSize().height * 0.15},
								{handlers.console.GetConsoleSize().width * 0.85,handlers.console.GetConsoleSize().height * 0.85} };
		}


		void WhenFileSaved(bool reSave = false)
		{
			size_t nowDataHash = hash<StringView>{}(editor->GetData());

			if (reSave || nowDataHash != lastSaveDataHash)
			{
				lastSaveDataHash = nowDataHash;
				lastSaveTime = chrono::steady_clock::now();

				String filename = handlers.file.nowFilePath.filename();

				PrintFooter(String::Format("已保存到 {} !"sv, filename));
			}
		}
		void WhenFileOpened()
		{
			lastSaveDataHash = hash<u8string_view>{}(editor->GetData().ToUTF8());
			lastSaveTime = chrono::steady_clock::now();

			String filename = handlers.file.nowFilePath.filename();

			ManageAutoSaveFile();

			PrintFooter(String::Format("已打开 {} !"sv, filename));
		}

		bool IsFileSaved()
		{
			return hash<u8string_view>{}(editor->GetData().ToUTF8()) == lastSaveDataHash;
		}

		void OpenFile(Encoding encoding = Encoding::UTF8)
		{
			PrintFooter(u8"打开文件......"s);
			auto window = make_shared<OpenFileWindow>(handlers.console,
				WindowDrawRangeNormalSize(),
				encoding,
				handlers.settings,
				bind(&UI::WhenFileOpened, this));
			uiHandler.components.emplace(uiHandler.normalWindowDepth, window);
			uiHandler.GiveFocusTo(window);
		}

		void NewFile()
		{
			editor->SetData(u8""sv);
			editor->ResetCursor();

			handlers.file.CloseFile();

			ManageAutoSaveFile(handlers.settings.Get<SettingID::NewFileAutoSaveName>());

			lastSaveDataHash = hash<u8string_view>{}(u8""sv);
			lastSaveTime = chrono::steady_clock::now();

			PrintFooter(String{ u8"已新建文件!"s });
		}

		// 神必 bug, 这个 callback 必须用引用传, 否则会爆
		// 死活查不出原因, 太酷炫力!
		void AskIfSave(auto&& callback)
		{
			auto window = make_shared<SaveOrNotWindow>(handlers.console,
				WindowDrawRangeNormalSize(),
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

		void ManageAutoSaveFile(filesystem::path mainFileName)
		{
			filesystem::path& autoSaveFileName = mainFileName;
			autoSaveFileName.concat(handlers.settings.Get<SettingID::AutoSaveFileExtension>());

			if (filesystem::exists(autoSaveFileName) && filesystem::is_regular_file(autoSaveFileName)
				&& (!filesystem::exists(autoSaveFile.nowFilePath)
					|| !filesystem::equivalent(autoSaveFileName, autoSaveFile.nowFilePath))
				&& !filesystem::is_empty(autoSaveFileName))
			{
				auto window = make_shared<LoadAutoSaveOrNotWindow>(handlers.console,
					WindowDrawRangeNormalSize(),
					[&, autoSaveFileName]
					(size_t choice)
					{
						if (choice == 0)
						{
							autoSaveFile.OpenFile(autoSaveFileName);
							editor->SetData(autoSaveFile.ReadAll());
						}
						else if (choice == 1)
							autoSaveFile.CreateFile(autoSaveFileName, true);
						else unreachable();
					});
				uiHandler.components.emplace(uiHandler.normalWindowDepth, window);
				uiHandler.GiveFocusTo(window);
			}
			else
				autoSaveFile.CreateFile(autoSaveFileName, true);
		}
		void ManageAutoSaveFile()
		{
			ManageAutoSaveFile(handlers.file.nowFilePath);
		}

		auto GetRealLineIndexWidth()
		{
			if (handlers.settings.Get<SettingID::LineIndexWidth>() > 0
				&& handlers.settings.Get<SettingID::LineIndexWidth>() < handlers.console.GetConsoleSize().width)
				return handlers.settings.Get<SettingID::LineIndexWidth>() + 1;
			return 0;
		}

		void PrintLineIndex()
		{
			if (GetRealLineIndexWidth() == 0)
				return;

			handlers.console.HideCursor();

			constexpr auto color = BasicColors::brightCyan;

			const auto lineIndexs = editor->GetLineIndexs();
			const auto drawHeight = min(static_cast<size_t>(handlers.console.GetConsoleSize().height - 2), lineIndexs.size());

			const auto stringToLong = (views::repeat(u8'.', handlers.settings.Get<SettingID::LineIndexWidth>()) | ranges::to<String>()) + u8"│"s;
			const auto stringNull = (views::repeat(u8' ', handlers.settings.Get<SettingID::LineIndexWidth>()) | ranges::to<String>()) + u8"│"s;

            bool alreadyTooLong = false;    // 行号有单调性, 第一个过长的行号后面的行号一定都是过长的, 缓存一下避免多次 O(N) 计算 Size()

			for (size_t i = 0; i < drawHeight; i++)
			{
				String outputStr = String::Format("{:>{}}│"sv, lineIndexs[i] + 1, handlers.settings.Get<SettingID::LineIndexWidth>());

				if (alreadyTooLong || outputStr.Size() > handlers.settings.Get<SettingID::LineIndexWidth>() + 1)
				{
				    outputStr = stringToLong;
                    alreadyTooLong = true;
				}

				handlers.console.Print(outputStr, { 0,i + 1 }, color);
			}
			for (size_t i = drawHeight; i < handlers.console.GetConsoleSize().height - 2; i++)
				handlers.console.Print(stringNull, { 0,i + 1 }, color);

			uiHandler.Refocus();
		}
	public:
		void PrintTitle(StringView extraText = u8""sv)
		{
			handlers.console.HideCursor();

			handlers.console.Print(views::repeat(u8' ', handlers.console.GetConsoleSize().width) | ranges::to<String>(), { 0,0 }, BasicColors::black, BasicColors::yellow);

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

		// 一样的某种神必 bug, 我是真的怀疑是 MSVC 的锅了
		void PrintFooter(String&& extraText = u8""s)
		{
			handlers.console.HideCursor();

			String rightText;

			auto leftTime = chrono::steady_clock::now() - lastSaveTime;
			auto saveDuration = handlers.settings.Get<SettingID::AutoSavingDuration>() * 1s;

			if (leftTime >= 1min)
				rightText = String::Format("上次保存: {} 前. "sv, chrono::duration_cast<chrono::minutes>(leftTime));
			else
				rightText = String::Format("上次保存: {} 前. "sv, chrono::duration_cast<chrono::seconds>(leftTime));

            if (saveDuration >= 1h)
                rightText += String::Format("自动保存周期: {}"sv, chrono::duration_cast<chrono::hours>(saveDuration));
			else if (saveDuration >= 1min)
				rightText += String::Format("自动保存周期: {}"sv, chrono::duration_cast<chrono::minutes>(saveDuration));
			else
				rightText += String::Format("自动保存周期: {}"sv, chrono::duration_cast<chrono::seconds>(saveDuration));

			// 这一块也改成屎山了 (悲)
			const size_t rightTextLen = GetDisplayLength(rightText);

			if (extraText.empty())	// 改为显示光标位置、文件名等
			{
				String cursorInfos = String::Format("   光标位置: ({},{})  行字符: {}"sv,
					editor->GetAbsCursorPos().x, editor->GetAbsCursorPos().y + 1,
					editor->GetNowLineCharCount());

				if (!handlers.file.nowFilePath.empty())
				{
					String filename = handlers.file.nowFilePath.filename();

					constexpr auto tipText = "当前文件: {}"sv;

					const auto othersWidth = rightTextLen + GetDisplayLength(String{ tipText }) + GetDisplayLength(cursorInfos) + 8;

					if (othersWidth < handlers.console.GetConsoleSize().width)
					{
						if (GetDisplayLength(filename) + othersWidth > handlers.console.GetConsoleSize().width)
						{
							size_t nowDisplayLength = 0;

							auto iter = filename.begin();
							while (true)
							{
								if (IsWideChar(*iter))
									nowDisplayLength += 2;
								else
									nowDisplayLength++;

								if (nowDisplayLength >= handlers.console.GetConsoleSize().width - othersWidth)
									break;

								++iter;
							}

							filename.erase(iter, filename.end());
							filename += u8"..."s;
						}

                        extraText = String::Format(tipText, filename);
					}
				}
				else
					extraText = u8"当前文件: (未命名新文件)"s;

				extraText += cursorInfos;
			}

			const size_t leftTextLen = GetDisplayLength(extraText);

			if (rightTextLen + leftTextLen < handlers.console.GetConsoleSize().width)
				handlers.console.Print(views::repeat(u8' ', handlers.console.GetConsoleSize().width - rightTextLen - leftTextLen) | ranges::to<String>(),
					{ leftTextLen,handlers.console.GetConsoleSize().height - 1 },
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
			String& editorData,
			InputHandler& inputHandler,
			FileHandler& fileHandler,
			ClipboardHandler& clipboardHandler,
			SettingMap& settingMap,
			const String& title = u8"ConsoleNotepad ver. 0.96b  made by NylteJ"s)
			:handlers(consoleHandler, inputHandler, fileHandler, clipboardHandler, uiHandler, settingMap),
			editor(make_shared<Editor>(consoleHandler, editorData, ConsoleRect{ { GetRealLineIndexWidth(),1 },
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

			PrintLineIndex();
			editor->SetLineIndexPrinter(bind(&UI::PrintLineIndex, this));

			static bool lastIsEsc = false;	// 这个判定目前还有点简陋, 但至少不至于让人退不出来 (真随机字符串的获取方式.jpg)

			inputHandler.SubscribeMessage([&](const InputHandler::MessageWindowSizeChanged& message, size_t)	// 与次数无关, 直接不用管第二个参数就好
				{
					// 貌似老式的 conhost 有蜜汁兼容问题, 有时会返回控制台有 9001 行......
					// 但是直接 handlers.console.GetConsoleSize() 返回的又是正常的.
					// 这里先用一个 dirty 点的解决方案吧
					// 顺便一提老式的 conhost 需要拉左右边框才会触发这个事件, 拉上下边框是不触发的, 很迷
					InputHandler::MessageWindowSizeChanged newMessage = message;

					if (newMessage.newSize.height >= 1000)
						newMessage.newSize = handlers.console.GetConsoleSize();
					
					editor->SetDrawRange({	{GetRealLineIndexWidth(),1},
											{newMessage.newSize.width - 1,newMessage.newSize.height - 2} });

					lastIsEsc = false;

					editor->WhenFocused();

					PrintLineIndex();

					uiHandler.GiveFocusTo(uiHandler.nowFocus);

					shared_ptr nowFocusPtr = uiHandler.nowFocus;

					nowFocusPtr->ManageInput(newMessage, handlers);

					PrintTitle();
					PrintFooter();
				});

			inputHandler.SubscribeMessage([&](const InputHandler::MessageKeyboard& message, size_t count)
				{
					using enum InputHandler::MessageKeyboard::Key;

					// 先朴实无华地循环 count 次, 后面需要优化时再改
					for (size_t i = 0; i < count; i++)
					{
						try
						{
							if (lastIsEsc)
								if (message.key != Esc)
								{
									lastIsEsc = false;
									PrintTitle();
								}
								else
									Exit(!handlers.settings.Get<SettingID::NormalExitWhenDoubleEsc>() && !IsFileSaved());

							if (chrono::steady_clock::now() - lastSaveTime >= handlers.settings.Get<SettingID::AutoSavingDuration>() * 1s)
								AutoSave();

							String footerText;

							if (uiHandler.nowFocus == editor)
							{

								if (message.key == Esc)
								{
									lastIsEsc = true;

									if (IsFileSaved())
										PrintTitle(u8"再按一次 Esc 以退出 (当前内容已保存)"sv);
									else
									{
										PrintTitle(u8"再按一次 Esc 以强制退出 (当前内容未保存!!!)"sv);

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
											footerText = u8"选择保存路径......"s;
											auto window = make_shared<SaveFileWindow>(handlers.console,
																					  WindowDrawRangeNormalSize(),
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
											AskIfSave([&](size_t index) {OpenFile(); });
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
										if (windowRange.Height() < 9 && handlers.console.GetConsoleSize().height >= 11)
											windowRange.rightBottom.y = windowRange.leftTop.y + 8;

										auto window = make_shared<FindReplaceWindow>(handlers.console,
																					 windowRange,
																					 settingMap,
																					 editor->GetSelectedStr() | ranges::to<String>(),
																					 true);
										uiHandler.components.emplace(uiHandler.normalWindowDepth, window);
										uiHandler.GiveFocusTo(window);
									}
									else if (message.key == H)
									{
										ConsoleRect windowRange = { {handlers.console.GetConsoleSize().width * 0.65,1},
																	{handlers.console.GetConsoleSize().width - 1,handlers.console.GetConsoleSize().height * 0.5} };
										if (windowRange.Height() < 11 && handlers.console.GetConsoleSize().height >= 13)
											windowRange.rightBottom.y = windowRange.leftTop.y + 10;

										auto window = make_shared<FindReplaceWindow>(handlers.console,
																					 windowRange,
																					 settingMap,
																					 editor->GetSelectedStr() | ranges::to<String>(),
																					 false);
										uiHandler.components.emplace(uiHandler.normalWindowDepth, window);
										uiHandler.GiveFocusTo(window);
									}
									else if (message.key == P)	// 设置
									{
										auto window = make_shared<SettingWindow>(handlers.console,
																				 WindowDrawRangeLargeSize(),
																				 settingMap);
										uiHandler.components.emplace(uiHandler.normalWindowDepth, window);
										uiHandler.GiveFocusTo(window);
									}
									else if (message.key == L)	// 历史记录
									{
										auto window = make_shared<HistoryWindow>(handlers.console,
																				 WindowDrawRangeNormalSize(),
																				 *editor);
										uiHandler.components.emplace(uiHandler.normalWindowDepth, window);
										uiHandler.GiveFocusTo(window);
									}
								}
							}

							shared_ptr nowFocusPtr = uiHandler.nowFocus;	// 防止提前析构, 非常重要

							nowFocusPtr->WhenRefocused();
							nowFocusPtr->ManageInput(message, handlers);

							PrintFooter(move(footerText));
						}
						catch (Exception& e)
						{
							PrintFooter(String::Format("发生异常: {}", e.What()));
						}
						catch (exception& e)
						{
							PrintFooter(String::Format("发生异常: {}", e.what()));
						}
					}
				});

			inputHandler.SubscribeMessage([&](const InputHandler::MessageMouse& message, size_t count)
				{
					using enum InputHandler::MessageMouse::Type;

					try
					{
						shared_ptr nowFocusPtr = uiHandler.nowFocus;

						// 暂时只处理鼠标滚轮的情况 (这个优化很有用, 在快速滚屏幕时能极其显著地提升性能, 同时快速滚屏幕又是非常常见的需求)
						if (message.type == VWheeled && count > 1)
						{
							auto newMessage = message;
							newMessage.wheelMove *= count;

							nowFocusPtr->ManageInput(newMessage, handlers);
						}
						else
							for (size_t i = 0; i < count; i++)
								nowFocusPtr->ManageInput(message, handlers);

						if (message.LeftClick() || message.RightClick() || message.WheelMove() != 0
							|| (message.buttonStatus.any() && message.type == Moved))
						{
							PrintFooter();

							if (lastIsEsc)
							{
								lastIsEsc = false;
								PrintTitle();
							}
						}
					}
					catch (Exception& e)
					{
						PrintFooter(String::Format("发生异常: {}", e.What()));
					}
					catch (exception& e)
					{
						PrintFooter(String::Format("发生异常: {}", e.what()));
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
					PrintFooter(u8"编码错误, 文件没能打开, 已自动新建文件!"s);
				}
			}
			else
				NewFile();

			uiHandler.GiveFocusTo(uiHandler.nowFocus);
		}
	};
}
// Windows.ixx
// 并非 Windows
// 实际上是控制台窗口模块, 用来更友好地显示用的
// 和 Editor 类似, 只用考虑自己内部的绘制, 由 UI 模块负责将他们拼接起来
// 现在貌似已经发展成各种窗口的集合体了
export module Windows;

import std;

import ConsoleHandler;
import ConsoleTypedef;
import BasicColors;
import InputHandler;
import UIComponent;

import StringEncoder;
import Exceptions;

import Editor;

using namespace std;

export namespace NylteJ
{
	// 只是一个外框
	class BasicWindow :public UIComponent
	{
	protected:
		ConsoleHandler& console;

		ConsoleRect drawRange;

		bool nowExit = false;	// 有点丑, 但可以先用着
	protected:
		void EraseThis(UnionHandler& handlers)
		{
			handlers.ui.EraseComponent(this);
			nowExit = true;
		}
	public:
		void PrintFrame()
		{
			if (drawRange.Height() < 2 || drawRange.Width() < 2)	// 不应如此, 所以直接 throw 吧
				throw invalid_argument{ "窗口太小了! "s };

			console.HideCursor();

			console.Print(L'┌' + (ranges::views::repeat(L'─', drawRange.Width() - 2) | ranges::to<wstring>()) + L'┐', drawRange.leftTop);

			for (auto y = drawRange.leftTop.y + 1; y < drawRange.rightBottom.y; y++)
				console.Print(L'│' + (ranges::views::repeat(L' ', drawRange.Width() - 2) | ranges::to<wstring>()) + L'│', { drawRange.leftTop.x,y });

			console.Print(L'└' + (ranges::views::repeat(L'─', drawRange.Width() - 2) | ranges::to<wstring>()) + L'┘', { drawRange.leftTop.x,drawRange.rightBottom.y });

			console.ShowCursor();
		}

		void ManageInput(const InputHandler::MessageWindowSizeChanged& message, UnionHandler& handlers) override
		{
			// 它没有能力重新适应新窗口大小, 只能在新窗口大小不足以容纳它的时候自毁
			// (窗口大小一动就自毁貌似也没必要......)
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
		void WhenRefocused() override
		{
			console.HideCursor();
		}

		BasicWindow(ConsoleHandler& console, const ConsoleRect& drawRange)
			:console(console),
			drawRange(drawRange)
		{
		}

		virtual ~BasicWindow() = default;
	};

	// 选择打开 / 保存路径的共有部分
	// 这样后面加花也更方便
	// 无法实例化
	class FilePathWindow :public BasicWindow
	{
	protected:
		Editor editor;

		wstring_view tipText;
	private:
		wstring GetNowPath() const
		{
			// 顺便做个防呆设计, 防止有大聪明给路径加引号和双反斜杠
			wstring ret=editor.GetData()
				| ranges::views::filter([](auto&& chr) {return chr != L'\"'; })
				| ranges::to<wstring>();

			size_t pos = 0;
			while ((pos = ret.find(L"\\\\"sv, pos)) != string::npos)
			{
				ret.replace(pos, L"\\\\"sv.size(), L"\\"sv);
				pos += L"\\"sv.size();
			}

			return ret;
		}
	public:
		// 处理收到的路径
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
				ManagePath(GetNowPath(), handlers);

				EraseThis(handlers);
				return;
			}
			else if (message.key == Tab)	// 自动补全
			{
				using namespace filesystem;

				path nowPath = GetNowPath();

				if (exists(nowPath))
				{
					if (is_directory(nowPath) && nowPath.wstring().back() == L'\\')
					{
						auto iter = directory_iterator{ nowPath,directory_options::skip_permission_denied };

						if (iter != end(iter))
						{
							editor.SetData((nowPath / iter->path().filename()).wstring());
							editor.PrintData();
							editor.MoveCursorToEnd();
						}
					}
					else if (is_directory(nowPath) || is_regular_file(nowPath))
					{
						auto prevPath = nowPath.parent_path();

						if (prevPath.empty())
							prevPath = L".";

						if (!equivalent(prevPath, nowPath))	// 比如 "C:\." 的 parent_path 是 "C:", 总之还是有可能二者完全相等的
						{
							auto iter = directory_iterator{ prevPath,directory_options::skip_permission_denied };

							while (!equivalent(iter->path(), nowPath))
								++iter;

							++iter;

							if (iter == end(iter))
								iter = directory_iterator{ prevPath,directory_options::skip_permission_denied };

							editor.SetData((prevPath / iter->path().filename()).wstring());
							editor.PrintData();
							editor.MoveCursorToEnd();
						}
					}
				}
				else
				{
					auto prevPath = nowPath.parent_path();

					if (prevPath.empty())
						prevPath = L".";

					if (exists(prevPath) && is_directory(prevPath))
					{
						auto iter = directory_iterator{ prevPath,directory_options::skip_permission_denied };

						while (iter != end(iter))
						{
							wstring nowFilename = iter->path().filename().wstring();
							wstring inputFilename = nowPath.filename().wstring();

							if (nowFilename.starts_with(inputFilename))
							{
								editor.SetData((prevPath / iter->path().filename()).wstring());
								editor.PrintData();
								editor.MoveCursorToEnd();
								break;
							}

							++iter;
						}
					}
				}

				return;		// 不发送到 Editor: 文件名正常应该也不会含有 Tab
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
		void WhenRefocused() override
		{
			editor.WhenRefocused();
		}

		FilePathWindow(ConsoleHandler& console, const ConsoleRect& drawRange, wstring_view tipText = L"输入路径 (按 Tab 自动补全, 按回车确认): "sv)
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
		Encoding encoding;

		function<void()> callback;
	public:
		void ManagePath(wstring_view path, UnionHandler& handlers) override;	// 要用到后面的 EncodingSelectWindow, 没办法

		OpenFileWindow(ConsoleHandler& console, const ConsoleRect& drawRange, Encoding encoding, function<void()> callback = [] {})
			:FilePathWindow(console, drawRange, L"打开文件: 输入路径 (按 Tab 自动补全, 按回车确认): "sv),
			encoding(encoding),
			callback(callback)
		{
		}
	};
	class SaveFileWindow :public FilePathWindow
	{
	private:
		function<void()> callback;
	public:
		void ManagePath(wstring_view path, UnionHandler& handlers) override;

		SaveFileWindow(ConsoleHandler& console, const ConsoleRect& drawRange, function<void()> callback = [] {})
			:FilePathWindow(console, drawRange, L"保存到: 输入路径 (按 Tab 自动补全, 按回车确认): "sv),
			callback(callback)
		{
		}
	};

	// 有两个或更多个选项的窗口
	// 其实还有挺多地方能用上的, 一些技术也能复用到查找 / 替换窗口里
	// 同样并不能实例化
	class SelectWindow :public BasicWindow
	{
	protected:
		vector<wstring> choices;

		size_t nowChoose = 0;
		size_t beginChoice = 0;		// 当窗口太小、选项太多时用来滚动

		wstring_view tipText;
	private:
		void PrintChoices()
		{
			if (drawRange.Height() < 4)
				throw invalid_argument{ "窗口太小了! "s };

			for (size_t i = beginChoice; i < choices.size() && (i - beginChoice) + 4 <= drawRange.Height(); i++)
			{
				if (i != nowChoose)
				{
					console.Print(choices[i], { drawRange.leftTop.x + 1 ,drawRange.leftTop.y + 2 + (i - beginChoice) });

					auto restLength = drawRange.rightBottom.x - console.GetCursorPos().x;
					if (restLength > 0)
						console.Print(ranges::views::repeat(L' ', restLength) | ranges::to<wstring>());
				}
				else
				{
					console.Print(choices[i], { drawRange.leftTop.x + 1 ,drawRange.leftTop.y + 2 + (i - beginChoice) }, BasicColors::inverseColor, BasicColors::inverseColor);

					auto restLength = drawRange.rightBottom.x - console.GetCursorPos().x;
					if (restLength > 0)
						console.Print(ranges::views::repeat(L' ', restLength) | ranges::to<wstring>(), BasicColors::inverseColor, BasicColors::inverseColor);
				}
			}
		}

		void ChangeChoose(size_t newChoose)
		{
			if (newChoose + 4 > beginChoice + drawRange.Height())
				beginChoice = newChoose + 4 - drawRange.Height();

			if (newChoose < beginChoice)
				beginChoice = newChoose;

			nowChoose = newChoose;
		}
	public:
		virtual void ManageChoice(size_t choiceIndex, UnionHandler& handlers) = 0;

		void ManageInput(const InputHandler::MessageWindowSizeChanged& message, UnionHandler& handlers) override
		{
			BasicWindow::ManageInput(message, handlers);
		}
		void ManageInput(const InputHandler::MessageKeyboard& message, UnionHandler& handlers) override
		{
			BasicWindow::ManageInput(message, handlers);

			if (nowExit)
				return;

			using enum InputHandler::MessageKeyboard::Key;

			switch (message.key)
			{
			case Up:
				if (nowChoose == 0)
					ChangeChoose(choices.size() - 1);
				else
					ChangeChoose(nowChoose - 1);

				PrintChoices();
				break;
			case Down:
				if (nowChoose + 1 == choices.size())
					ChangeChoose(0);
				else
					ChangeChoose(nowChoose + 1);

				PrintChoices();
				break;
			case Enter:
				ManageChoice(nowChoose, handlers);
				EraseThis(handlers);
				return;
			default:
				return;
			}
		}
		void ManageInput(const InputHandler::MessageMouse& message, UnionHandler& handlers) override
		{
			BasicWindow::ManageInput(message, handlers);

			if (nowExit)
				return;

			if (message.LeftClick() && drawRange.Contain(message.position))
			{
				const auto lineIndex = (message.position - drawRange.leftTop).y - 2;	// 前两行分别是边框和提示词

				if (lineIndex >= 0 && lineIndex + beginChoice < choices.size())
					if (nowChoose != lineIndex + beginChoice)	// 本来就选中了就直接选择了, 否则仅仅只是选中. 换言之鼠标要双击才有用
						nowChoose = lineIndex + beginChoice;
					else
					{
						ManageChoice(nowChoose, handlers);

						EraseThis(handlers);
						return;
					}
			}
		}

		void WhenFocused() override
		{
			BasicWindow::WhenFocused();

			console.HideCursor();

			console.Print(tipText, { drawRange.leftTop.x + 1,drawRange.leftTop.y + 1 });

			PrintChoices();
		}

		SelectWindow(ConsoleHandler& console, const ConsoleRect& drawRange, const vector<wstring>& choices = {}, wstring_view tipText = L"选择: "sv)
			:BasicWindow(console, drawRange),
			choices(choices),
			tipText(tipText)
		{
		}
	};

	class SaveOrNotWindow :public SelectWindow
	{
	private:
		function<void(size_t)> callback;
	public:
		void ManageChoice(size_t choiceIndex, UnionHandler& handlers) override
		{
			if (choiceIndex == 0)	// 保存
			{
				if (handlers.file.Valid())
				{
					handlers.file.Write(handlers.ui.mainEditor.GetData());
					callback(choiceIndex);
				}
				else
				{
					// TODO: 未来再考虑统一吧
					auto window = make_shared<SaveFileWindow>(handlers.console, drawRange,
						[callback = this->callback, choiceIndex] {callback(choiceIndex); });
					// 注意这里只能传值, 因为 callback 被调用时这个 Window 很可能已经被销毁了
					// 而且必须这样指定, 否则 lambda 只会捕获 this, callback 还是无效, 得这样才会让他复制一份

					handlers.ui.components.emplace(handlers.ui.normalWindowDepth, window);
					handlers.ui.GiveFocusTo(window);
				}
			}
			else if (choiceIndex == 1)	// 不保存
				callback(choiceIndex);
			else	// 取消, 此时不调用 callback, 效果和按 Esc 应该是一样的
				return;
		}

		SaveOrNotWindow(ConsoleHandler& console, const ConsoleRect& drawRange, function<void(size_t)> callback = [](size_t) {})
			:SelectWindow(console, drawRange, {L"保存"s, L"不保存"s, L"取消"s}, L"当前更改未保存. 是否先保存? "sv),
			callback(callback)
		{
		}
	};

	class EncodingSelectWindow :public SelectWindow
	{
	private:
		function<void(Encoding)> callback;
	public:
		void ManageChoice(size_t choiceIndex, UnionHandler& handlers) override
		{
			if (choiceIndex + 1 == choices.size())	// 取消
				return;
			if (choiceIndex + 2 == choices.size())	// 强制打开
			{
				callback(FORCE);
				return;
			}

			using enum Encoding;

			switch (choiceIndex)
			{
			case 0:		// UTF-8
				callback(UTF8);
				return;
			case 1:		// GB 2312
				callback(GB2312);
				return;
			default:
				unreachable();
			}
		}

		EncodingSelectWindow(ConsoleHandler& console, const ConsoleRect& drawRange, function<void(Encoding)> callback = [](Encoding) {}, wstring_view tip = L"手动选择文件编码: "sv)
			:SelectWindow(console, drawRange, { L"UTF-8 (默认)"s, L"GB 2312"s, L"强制打开 (极易出 bug)"s,L"取消"s}, tip),
			callback(callback)
		{
		}
	};

	class OverrideOrNotWindow :public SelectWindow
	{
	private:
		function<void(size_t)> doOverride;
	public:
		void ManageChoice(size_t choiceIndex, UnionHandler& handlers) override
		{
			if (choiceIndex == 0)	// 覆盖
				doOverride(choiceIndex);
			else	// 取消, 此时不调用 callback, 效果和按 Esc 应该是一样的
				return;
			// TODO: 理论上这里取消后应当回到原窗口, 但代码不好改
		}

		OverrideOrNotWindow(ConsoleHandler& console, const ConsoleRect& drawRange, function<void(size_t)> doOverride = [](size_t) {})
			:SelectWindow(console, drawRange, { L"覆盖"s, L"取消"s }, L"文件已存在. 是否覆盖? "sv),
			doOverride(doOverride)
		{
			nowChoose = 1;	// 默认不覆盖
		}
	};


	void OpenFileWindow::ManagePath(wstring_view path, UnionHandler& handlers)
	{
		try
		{
			handlers.file.OpenFile(path);

			handlers.ui.mainEditor.SetData(handlers.file.ReadAll(encoding));
			handlers.ui.mainEditor.ResetCursor();

			callback();
		}
		catch (WrongEncodingException&)
		{
			auto window = make_shared<EncodingSelectWindow>(handlers.console,
				ConsoleRect{ {handlers.console.GetConsoleSize().width * 0.25,handlers.console.GetConsoleSize().height * 0.33},
								{handlers.console.GetConsoleSize().width * 0.75,handlers.console.GetConsoleSize().height * 0.67} },
				[path = ranges::to<wstring>(path),
				callback = this->callback,
				&file = handlers.file,
				&mainEditor = handlers.ui.mainEditor]
				(Encoding x) mutable
				{
					file.OpenFile(path);

					mainEditor.SetData(file.ReadAll(x));
					mainEditor.ResetCursor();

					callback();
				},
				L"文件无法通过 UTF-8 编码打开, 请手动选择编码: "sv);
			handlers.ui.components.emplace(handlers.ui.normalWindowDepth, window);
			handlers.ui.GiveFocusTo(window);
		}
	}
	void SaveFileWindow::ManagePath(wstring_view path, UnionHandler& handlers)
	{
		{
			try
			{
				handlers.file.CreateFile(path);

				handlers.file.Write(handlers.ui.mainEditor.GetData());

				callback();
			}
			catch (FileOpenFailedException&)
			{
				auto window = make_shared<OverrideOrNotWindow>(handlers.console,
					ConsoleRect{	{handlers.console.GetConsoleSize().width * 0.25,handlers.console.GetConsoleSize().height * 0.33},
									{handlers.console.GetConsoleSize().width * 0.75,handlers.console.GetConsoleSize().height * 0.67} },
					[callback = this->callback,
					&file = handlers.file,
					path = path | ranges::to<wstring>(),
					&mainEditor = handlers.ui.mainEditor]
					(size_t)
					{
						file.CreateFile(path, true);

						file.Write(mainEditor.GetData());

						callback();
					});
				handlers.ui.components.emplace(handlers.ui.normalWindowDepth, window);
				handlers.ui.GiveFocusTo(window);
			}
		}
	}


	class FindWindow :public BasicWindow
	{
	private:
		constexpr static wstring_view tipText1 = L"查找: 按回车查找, 用方向键切换对象."sv;
		constexpr static wstring_view tipText2 = L"请使用 \\n, \\r 来代指回车和换行."sv;
		constexpr static wstring_view tipText3 = L"目前仅支持完全、非全字、非正则匹配."sv;
	private:
		Editor findEditor;

		vector<size_t> allFindedIndexs;

		size_t nowFindedIndexID = 0;

		wstring lastFindText = L""s;
	private:
		void PrintWindow()
		{
			console.Print(tipText1, { drawRange.leftTop.x + 1,drawRange.leftTop.y + 1 });
			if (drawRange.Height() >= 5)
				console.Print(tipText2, { drawRange.leftTop.x + 1,drawRange.leftTop.y + 2 });
			if (drawRange.Height() >= 6)
				console.Print(tipText3, { drawRange.leftTop.x + 1,drawRange.rightBottom.y - 1 });
		}

		void MoveMainEditorPos(UnionHandler& handlers)
		{
			if (nowFindedIndexID < allFindedIndexs.size())
			{
				handlers.ui.mainEditor.MoveCursorToIndex(allFindedIndexs[nowFindedIndexID], allFindedIndexs[nowFindedIndexID] + lastFindText.size());

				WhenFocused();
			}
			else if (allFindedIndexs.empty())
			{
				handlers.ui.mainEditor.ResetCursor();
				WhenFocused();
			}
		}

		wstring GetNowFindText()
		{
			wstring stringToFind = findEditor.GetData() | ranges::to<wstring>();

			for (auto&& [source, target] : vector{ pair	{L"\\n"sv,L"\n"sv},
														{L"\\r"sv,L"\r"sv} })
			{
				size_t pos = 0;
				while ((pos = stringToFind.find(source, pos)) != string::npos)
				{
					stringToFind.replace(pos, source.size(), target);
					pos += target.size();
				}
			}

			return stringToFind;
		}

		void ReFindAll(UnionHandler& handlers)
		{
			allFindedIndexs.clear();

			wstring stringToFind = GetNowFindText();
			wstring_view stringAll = handlers.ui.mainEditor.GetData();

			lastFindText = stringToFind;

			// BMH 应该在这里比 BM 更适合一点?
			boyer_moore_horspool_searcher searcher = { stringToFind.begin(),stringToFind.end() };

			auto iter = stringAll.begin();

			while (true)
			{
				iter = search(iter, stringAll.end(), searcher);

				if (iter == stringAll.end())
					break;
				
				allFindedIndexs.emplace_back(iter - stringAll.begin());

				++iter;
			}

			nowFindedIndexID = 0;

			MoveMainEditorPos(handlers);
		}

		void FindNext(UnionHandler& handlers)
		{
			if (allFindedIndexs.empty())
				return;

			nowFindedIndexID++;

			if (nowFindedIndexID == allFindedIndexs.size())
				nowFindedIndexID = 0;

			MoveMainEditorPos(handlers);
		}
		void FindPrev(UnionHandler& handlers)
		{
			if (allFindedIndexs.empty())
				return;

			nowFindedIndexID--;

			if (nowFindedIndexID == -1)
				nowFindedIndexID = allFindedIndexs.size() - 1;

			MoveMainEditorPos(handlers);
		}
	public:
		void ManageInput(const InputHandler::MessageWindowSizeChanged& message, UnionHandler& handlers) override
		{
			BasicWindow::ManageInput(message, handlers);
		}
		void ManageInput(const InputHandler::MessageKeyboard& message, UnionHandler& handlers) override
		{
			BasicWindow::ManageInput(message, handlers);

			if (nowExit)
				return;

			using enum InputHandler::MessageKeyboard::Key;

			if (message.key == Enter)
			{
				if (GetNowFindText() != lastFindText)
					ReFindAll(handlers);
				else
					FindNext(handlers);

				return;
			}
			else if (message.key == Up)
			{
				if (GetNowFindText() == lastFindText)
					FindPrev(handlers);

				return;
			}
			else if (message.key == Down)
			{
				if (GetNowFindText() == lastFindText)
					FindNext(handlers);

				return;
			}

			findEditor.ManageInput(message, handlers);
		}
		void ManageInput(const InputHandler::MessageMouse& message, UnionHandler& handlers) override
		{
			BasicWindow::ManageInput(message, handlers);

			if (nowExit)
				return;

			if ((message.LeftClick() || message.RightClick())
				&& !drawRange.Contain(message.position))
			{
				EraseThis(handlers);	// TODO: 改成可恢复的失焦, 而不是直接关掉

				handlers.ui.mainEditor.ManageInput(message, handlers);
				return;
			}

			findEditor.ManageInput(message, handlers);
		}

		void WhenFocused() override
		{
			BasicWindow::WhenFocused();

			PrintWindow();

			findEditor.WhenFocused();
		}
		void WhenRefocused() override
		{
			BasicWindow::WhenRefocused();

			findEditor.WhenRefocused();
		}

		FindWindow(ConsoleHandler& console, const ConsoleRect& drawRange)
			:BasicWindow(console, drawRange),
			findEditor(console, L""s, { {drawRange.leftTop.x + 1,drawRange.leftTop.y + 2},
										{drawRange.rightBottom.x - 1,drawRange.leftTop.y + 2} })
		{
			if (drawRange.Height() >= 5)
				findEditor.ChangeDrawRange({	{drawRange.leftTop.x + 1,drawRange.leftTop.y + 3},
												{drawRange.rightBottom.x - 1,drawRange.leftTop.y + 3} });
		}
	};

	// TODO: 其实这里应该要复用一部分上面的代码的, 甚至可以直接合并两个类
	// 但是先不管了, 复制粘贴真爽吧
	class ReplaceWindow :public BasicWindow
	{
	private:
		constexpr static wstring_view tipTextTitle = L"替换: 按回车替换, 用方向键切换对象."sv;
		constexpr static wstring_view tipText1 = L"Shift + 回车全部替换. 按 Tab 切换输入框."sv;
		constexpr static wstring_view tipTextFrom = L"将: "sv;
		constexpr static wstring_view tipTextTo = L"替换为: "sv;
		constexpr static wstring_view tipText2 = L"用 \\n, \\r, \\t 代指回车、换行、Tab."sv;
		constexpr static wstring_view tipText3 = L"目前仅支持完全、非全字、非正则匹配."sv;
	private:
		Editor findEditor, replaceEditor;
		Editor* nowEditor = &findEditor;

		vector<size_t> allFindedIndexs;

		size_t nowFindedIndexID = 0;

		wstring lastFindText = L""s;
	private:
		void PrintWindow()
		{
			// TODO: 做压行适配
			if (drawRange.Height() < 10)
				throw Exception{ L"窗口太小了!"sv };

			console.Print(tipTextTitle, { drawRange.leftTop.x + 1,drawRange.leftTop.y + 1 });
			console.Print(tipText1, { drawRange.leftTop.x + 1,drawRange.leftTop.y + 2 });
			console.Print(tipTextFrom, { drawRange.leftTop.x + 1,drawRange.leftTop.y + 3 });
			console.Print(tipTextTo, { drawRange.leftTop.x + 1,drawRange.leftTop.y + 5 });
			console.Print(tipText2, { drawRange.leftTop.x + 1,drawRange.rightBottom.y - 2 });
			console.Print(tipText3, { drawRange.leftTop.x + 1,drawRange.rightBottom.y - 1 });
		}

		void SwitchEditor()
		{
			if (nowEditor == &findEditor)
				nowEditor = &replaceEditor;
			else
				nowEditor = &findEditor;
		}

		void MoveMainEditorPos(UnionHandler& handlers)
		{
			if (nowFindedIndexID < allFindedIndexs.size())
			{
				handlers.ui.mainEditor.MoveCursorToIndex(allFindedIndexs[nowFindedIndexID], allFindedIndexs[nowFindedIndexID] + lastFindText.size());

				WhenFocused();
			}
			else if (allFindedIndexs.empty())
			{
				handlers.ui.mainEditor.ResetCursor();
				WhenFocused();
			}
		}

		wstring ConvertText(wstring raw) const
		{
			for (auto&& [source, target] : vector{ pair	{L"\\n"sv,L"\n"sv},
														{L"\\r"sv,L"\r"sv},
														{L"\\t"sv,L"\t"sv} })
			{
				size_t pos = 0;
				while ((pos = raw.find(source, pos)) != string::npos)
				{
					raw.replace(pos, source.size(), target);
					pos += target.size();
				}
			}

			return raw;
		}

		wstring GetNowFindText() const
		{
			return ConvertText(findEditor.GetData() | ranges::to<wstring>());
		}
		wstring GetNowReplaceText() const
		{
			return ConvertText(replaceEditor.GetData() | ranges::to<wstring>());
		}

		void ReFindAll(UnionHandler& handlers)
		{
			allFindedIndexs.clear();

			wstring stringToFind = GetNowFindText();
			wstring_view stringAll = handlers.ui.mainEditor.GetData();

			lastFindText = stringToFind;

			// BMH 应该在这里比 BM 更适合一点?
			boyer_moore_horspool_searcher searcher = { stringToFind.begin(),stringToFind.end() };

			auto iter = stringAll.begin();

			while (true)
			{
				iter = search(iter, stringAll.end(), searcher);

				if (iter == stringAll.end())
					break;

				allFindedIndexs.emplace_back(iter - stringAll.begin());

				++iter;
			}

			nowFindedIndexID = 0;

			MoveMainEditorPos(handlers);
		}

		void FindNext(UnionHandler& handlers)
		{
			if (allFindedIndexs.empty())
				return;

			nowFindedIndexID++;

			if (nowFindedIndexID == allFindedIndexs.size())
				nowFindedIndexID = 0;

			MoveMainEditorPos(handlers);
		}
		void FindPrev(UnionHandler& handlers)
		{
			if (allFindedIndexs.empty())
				return;

			nowFindedIndexID--;

			if (nowFindedIndexID == -1)
				nowFindedIndexID = allFindedIndexs.size() - 1;

			MoveMainEditorPos(handlers);
		}

		void ReplaceNext(UnionHandler& handlers)
		{
			if (allFindedIndexs.empty())
				return;

			MoveMainEditorPos(handlers);

			handlers.ui.mainEditor.Insert(GetNowReplaceText());

			const auto offset = static_cast<long long>(GetNowReplaceText().size()) - static_cast<long long>(GetNowFindText().size());

			nowFindedIndexID = allFindedIndexs.erase(allFindedIndexs.begin() + nowFindedIndexID) - allFindedIndexs.begin();

			for (size_t i = nowFindedIndexID; i < allFindedIndexs.size(); i++)	// 后面的都要偏移, 前面的都不用
				allFindedIndexs[i] += offset;

			if (nowFindedIndexID == allFindedIndexs.size() && !allFindedIndexs.empty())
				nowFindedIndexID = 0;

			MoveMainEditorPos(handlers);
		}
		void ReplaceAll(UnionHandler& handlers)
		{
			if (allFindedIndexs.empty())
				return;

			nowFindedIndexID = allFindedIndexs.size() - 1;

			// 这里选择反复调用 Editor::Insert 只是为了便于撤销 (虽然得挨个撤销)
			while (nowFindedIndexID > 0)
			{
				// 这里本窗体本身无需被绘制, 所以无需调用 MoveMainEditorPos
				handlers.ui.mainEditor.MoveCursorToIndex(allFindedIndexs[nowFindedIndexID],
					allFindedIndexs[nowFindedIndexID] + lastFindText.size());

				handlers.ui.mainEditor.Insert(GetNowReplaceText());

				nowFindedIndexID--;
			}
			// size_t 恒非负, 所以只能这样了
			handlers.ui.mainEditor.MoveCursorToIndex(allFindedIndexs[nowFindedIndexID],
				allFindedIndexs[nowFindedIndexID] + lastFindText.size());

			handlers.ui.mainEditor.Insert(GetNowReplaceText());

			allFindedIndexs.clear();

			nowFindedIndexID = 0;

			MoveMainEditorPos(handlers);
		}
	public:
		void ManageInput(const InputHandler::MessageWindowSizeChanged& message, UnionHandler& handlers) override
		{
			BasicWindow::ManageInput(message, handlers);
		}
		void ManageInput(const InputHandler::MessageKeyboard& message, UnionHandler& handlers) override
		{
			BasicWindow::ManageInput(message, handlers);

			if (nowExit)
				return;

			using enum InputHandler::MessageKeyboard::Key;

			if (message.key == Enter)
			{
				if(message.extraKeys.Shift())
				{
					if (GetNowFindText() != lastFindText)
						ReFindAll(handlers);
					ReplaceAll(handlers);
				}
				else
				{
					if (GetNowFindText() != lastFindText)
						ReFindAll(handlers);
					ReplaceNext(handlers);
				}

				return;
			}
			else if (message.key == Up)
			{
				if (GetNowFindText() != lastFindText)
					ReFindAll(handlers);
				FindPrev(handlers);

				return;
			}
			else if (message.key == Down)
			{
				if (GetNowFindText() != lastFindText)
					ReFindAll(handlers);
				FindNext(handlers);

				return;
			}
			else if (message.key == Tab)
			{
				SwitchEditor();

				nowEditor->WhenRefocused();

				return;
			}

			nowEditor->ManageInput(message, handlers);
		}
		void ManageInput(const InputHandler::MessageMouse& message, UnionHandler& handlers) override
		{
			BasicWindow::ManageInput(message, handlers);

			if (nowExit)
				return;

			if ((message.LeftClick() || message.RightClick())
				&& !drawRange.Contain(message.position))
			{
				EraseThis(handlers);	// TODO: 改成可恢复的失焦, 而不是直接关掉

				handlers.ui.mainEditor.ManageInput(message, handlers);
				return;
			}

			nowEditor->ManageInput(message, handlers);
		}

		void WhenFocused() override
		{
			BasicWindow::WhenFocused();

			PrintWindow();

			findEditor.WhenFocused();
			replaceEditor.WhenFocused();

			nowEditor->WhenRefocused();
		}
		void WhenRefocused() override
		{
			BasicWindow::WhenRefocused();

			nowEditor->WhenRefocused();
		}

		ReplaceWindow(ConsoleHandler& console, const ConsoleRect& drawRange)
			:BasicWindow(console, drawRange),
			findEditor(console, L""s, { {drawRange.leftTop.x + 1,drawRange.leftTop.y + 4},
										{drawRange.rightBottom.x - 1,drawRange.leftTop.y + 4} }),
			replaceEditor(console, L""s, {	{drawRange.leftTop.x + 1,drawRange.leftTop.y + 6},
											{drawRange.rightBottom.x - 1,drawRange.leftTop.y + 6} })
		{
		}
	};
}
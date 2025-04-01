// Windows.ixx
// 并非 Windows
// 实际上是控制台窗口模块, 用来更友好地显示用的
// 和 Editor 类似, 只用考虑自己内部的绘制, 由 UI 模块负责将他们拼接起来
// 曾经这里堆放了所有的窗口, 但是这样会让 IntelliSense 爆炸, 所以分了一部分出去 (六根说是)
// 这里剩下的两个没有分出去是因为彼此之间耦合比较严重, 要分就只能一个类一个模块, 而我懒 ()
export module Windows;

import std;

import ConsoleHandler;
import ConsoleTypedef;
import BasicColors;
import InputHandler;
import UIComponent;
import Selector;

import StringEncoder;
import Exceptions;

import Editor;

// 兼容旧代码
export import BasicWindow;
export import FindReplaceWindow;

using namespace std;

export namespace NylteJ
{
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
			wstring ret = editor.GetData()
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
					if (is_directory(nowPath) && nowPath.wstring().back() == L'\\')		// 补全子目录 / 文件
					{
						auto iter = directory_iterator{ nowPath,directory_options::skip_permission_denied };

						if (iter != end(iter))
						{
							editor.SetData((nowPath / iter->path().filename()).wstring());
							editor.PrintData();
							editor.MoveCursorToEnd();
						}
					}
					else if (is_directory(nowPath) || is_regular_file(nowPath))			// 补全到当前目录的下一个目录 / 文件
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
				else	// 补全当前文件名
				{
					auto prevPath = nowPath.parent_path();

					if (prevPath.empty())
						prevPath = L".";

					if (exists(prevPath) && is_directory(prevPath))
					{
						auto iter = directory_iterator{ prevPath,directory_options::skip_permission_denied };

						bool haveFind = false;

						while (iter != end(iter))	// 先区分大小写地找, 再不区分大小写地找
						{
							wstring nowFilename = iter->path().filename().wstring();
							wstring inputFilename = nowPath.filename().wstring();

							if (nowFilename.starts_with(inputFilename))
							{
								editor.SetData((prevPath / iter->path().filename()).wstring());
								editor.PrintData();
								editor.MoveCursorToEnd();
								haveFind = true;
								break;
							}

							++iter;
						}
						if (!haveFind)
						{
							iter = directory_iterator{ prevPath,directory_options::skip_permission_denied };
							while (iter != end(iter))
							{
								wstring nowFilename = iter->path().filename().wstring();
								wstring inputFilename = nowPath.filename().wstring();

								wstring nowFilenameLower = nowFilename
									| ranges::views::transform([](auto&& chr) {return towlower(chr); })
									| ranges::to<wstring>();
								wstring inputFilenameLower = inputFilename
									| ranges::views::transform([](auto&& chr) {return towlower(chr); })
									| ranges::to<wstring>();

								if (nowFilenameLower.starts_with(inputFilenameLower))
								{
									editor.SetData((prevPath / iter->path().filename()).wstring());
									editor.PrintData();
									editor.MoveCursorToEnd();
									haveFind = true;
									break;
								}

								++iter;
							}
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
				editor.SetDrawRange({ {drawRange.leftTop.x + 1,drawRange.leftTop.y + 2},
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
					{
						nowChoose = lineIndex + beginChoice;

						PrintChoices();
					}
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
			:SelectWindow(console, drawRange, { L"保存"s, L"不保存"s, L"取消"s }, L"当前更改未保存. 是否先保存? "sv),
			callback(callback)
		{
		}
	};

	class EncodingSelectWindow :public SelectWindow
	{
	private:
		constexpr static wstring_view lastChoiceText = L" (上次选择)"sv;
	private:
		function<void(Encoding)> callback;

		size_t lastChoice = 0;
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

			if (choiceIndex + 3 == choices.size())	//自动识别
			{
				for (size_t i = static_cast<size_t>(FirstEncoding); i <= static_cast<size_t>(LastEncoding); i++)
				{
					try
					{
						callback(static_cast<Encoding>(i));
						return;
					}
					catch (WrongEncodingException&) {}
				}
				throw WrongEncodingException{ L"目前尚不支持该编码! 请尝试强制打开."sv };
			}

			try
			{
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
			catch (WrongEncodingException&)
			{
				if (choices[lastChoice].ends_with(lastChoiceText))
					choices[lastChoice].erase(choices[lastChoice].size() - lastChoiceText.size(), lastChoiceText.size());

				lastChoice = choiceIndex;
				choices[lastChoice].append_range(lastChoiceText);

				Repaint();

				throw;
			}
		}

		EncodingSelectWindow(ConsoleHandler& console, const ConsoleRect& drawRange, function<void(Encoding)> callback = [](Encoding) {}, wstring_view tip = L"手动选择文件编码: "sv)
			:SelectWindow(console, drawRange, { L"UTF-8"s, L"GB 2312"s, L"自动识别 (实验性功能)"s, L"强制打开 (慎重!)"s,L"取消"s }, tip),
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
					ConsoleRect{ {handlers.console.GetConsoleSize().width * 0.25,handlers.console.GetConsoleSize().height * 0.33},
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
}
// Windows.ixx
// ���� Windows
// ʵ�����ǿ���̨����ģ��, �������Ѻõ���ʾ�õ�
// �� Editor ����, ֻ�ÿ����Լ��ڲ��Ļ���, �� UI ģ�鸺������ƴ������
// ��������ѷ������еĴ���, ������������ IntelliSense ��ը, ���Է���һ���ֳ�ȥ (����˵��)
// ����ʣ�µ�����û�зֳ�ȥ����Ϊ�˴�֮����ϱȽ�����, Ҫ�־�ֻ��һ����һ��ģ��, ������ ()
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

// ���ݾɴ���
export import BasicWindow;
export import FindReplaceWindow;

using namespace std;

export namespace NylteJ
{
	// ѡ��� / ����·���Ĺ��в���
	// ��������ӻ�Ҳ������
	// �޷�ʵ����
	class FilePathWindow :public BasicWindow
	{
	protected:
		Editor editor;

		wstring_view tipText;
	private:
		wstring GetNowPath() const
		{
			// ˳�������������, ��ֹ�д������·�������ź�˫��б��
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
		// �����յ���·��
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
			else if (message.key == Tab)	// �Զ���ȫ
			{
				using namespace filesystem;

				path nowPath = GetNowPath();

				if (exists(nowPath))
				{
					if (is_directory(nowPath) && nowPath.wstring().back() == L'\\')		// ��ȫ��Ŀ¼ / �ļ�
					{
						auto iter = directory_iterator{ nowPath,directory_options::skip_permission_denied };

						if (iter != end(iter))
						{
							editor.SetData((nowPath / iter->path().filename()).wstring());
							editor.PrintData();
							editor.MoveCursorToEnd();
						}
					}
					else if (is_directory(nowPath) || is_regular_file(nowPath))			// ��ȫ����ǰĿ¼����һ��Ŀ¼ / �ļ�
					{
						auto prevPath = nowPath.parent_path();

						if (prevPath.empty())
							prevPath = L".";

						if (!equivalent(prevPath, nowPath))	// ���� "C:\." �� parent_path �� "C:", ��֮�����п��ܶ�����ȫ��ȵ�
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
				else	// ��ȫ��ǰ�ļ���
				{
					auto prevPath = nowPath.parent_path();

					if (prevPath.empty())
						prevPath = L".";

					if (exists(prevPath) && is_directory(prevPath))
					{
						auto iter = directory_iterator{ prevPath,directory_options::skip_permission_denied };

						bool haveFind = false;

						while (iter != end(iter))	// �����ִ�Сд����, �ٲ����ִ�Сд����
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

				return;		// �����͵� Editor: �ļ�������Ӧ��Ҳ���Ậ�� Tab
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

		FilePathWindow(ConsoleHandler& console, const ConsoleRect& drawRange, wstring_view tipText = L"����·�� (�� Tab �Զ���ȫ, ���س�ȷ��): "sv)
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
		void ManagePath(wstring_view path, UnionHandler& handlers) override;	// Ҫ�õ������ EncodingSelectWindow, û�취

		OpenFileWindow(ConsoleHandler& console, const ConsoleRect& drawRange, Encoding encoding, function<void()> callback = [] {})
			:FilePathWindow(console, drawRange, L"���ļ�: ����·�� (�� Tab �Զ���ȫ, ���س�ȷ��): "sv),
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
			:FilePathWindow(console, drawRange, L"���浽: ����·�� (�� Tab �Զ���ȫ, ���س�ȷ��): "sv),
			callback(callback)
		{
		}
	};


	// ������������ѡ��Ĵ���
	// ��ʵ����ͦ��ط������ϵ�, һЩ����Ҳ�ܸ��õ����� / �滻������
	// ͬ��������ʵ����
	class SelectWindow :public BasicWindow
	{
	protected:
		vector<wstring> choices;

		size_t nowChoose = 0;
		size_t beginChoice = 0;		// ������̫С��ѡ��̫��ʱ��������

		wstring_view tipText;
	private:
		void PrintChoices()
		{
			if (drawRange.Height() < 4)
				throw invalid_argument{ "����̫С��! "s };

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
				const auto lineIndex = (message.position - drawRange.leftTop).y - 2;	// ǰ���зֱ��Ǳ߿����ʾ��

				if (lineIndex >= 0 && lineIndex + beginChoice < choices.size())
					if (nowChoose != lineIndex + beginChoice)	// ������ѡ���˾�ֱ��ѡ����, �������ֻ��ѡ��. ����֮���Ҫ˫��������
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

		SelectWindow(ConsoleHandler& console, const ConsoleRect& drawRange, const vector<wstring>& choices = {}, wstring_view tipText = L"ѡ��: "sv)
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
			if (choiceIndex == 0)	// ����
			{
				if (handlers.file.Valid())
				{
					handlers.file.Write(handlers.ui.mainEditor.GetData());
					callback(choiceIndex);
				}
				else
				{
					// TODO: δ���ٿ���ͳһ��
					auto window = make_shared<SaveFileWindow>(handlers.console, drawRange,
						[callback = this->callback, choiceIndex] {callback(choiceIndex); });
					// ע������ֻ�ܴ�ֵ, ��Ϊ callback ������ʱ��� Window �ܿ����Ѿ���������
					// ���ұ�������ָ��, ���� lambda ֻ�Ჶ�� this, callback ������Ч, �������Ż���������һ��

					handlers.ui.components.emplace(handlers.ui.normalWindowDepth, window);
					handlers.ui.GiveFocusTo(window);
				}
			}
			else if (choiceIndex == 1)	// ������
				callback(choiceIndex);
			else	// ȡ��, ��ʱ������ callback, Ч���Ͱ� Esc Ӧ����һ����
				return;
		}

		SaveOrNotWindow(ConsoleHandler& console, const ConsoleRect& drawRange, function<void(size_t)> callback = [](size_t) {})
			:SelectWindow(console, drawRange, { L"����"s, L"������"s, L"ȡ��"s }, L"��ǰ����δ����. �Ƿ��ȱ���? "sv),
			callback(callback)
		{
		}
	};

	class EncodingSelectWindow :public SelectWindow
	{
	private:
		constexpr static wstring_view lastChoiceText = L" (�ϴ�ѡ��)"sv;
	private:
		function<void(Encoding)> callback;

		size_t lastChoice = 0;
	public:
		void ManageChoice(size_t choiceIndex, UnionHandler& handlers) override
		{
			if (choiceIndex + 1 == choices.size())	// ȡ��
				return;
			if (choiceIndex + 2 == choices.size())	// ǿ�ƴ�
			{
				callback(FORCE);
				return;
			}

			using enum Encoding;

			if (choiceIndex + 3 == choices.size())	//�Զ�ʶ��
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
				throw WrongEncodingException{ L"Ŀǰ�в�֧�ָñ���! �볢��ǿ�ƴ�."sv };
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

		EncodingSelectWindow(ConsoleHandler& console, const ConsoleRect& drawRange, function<void(Encoding)> callback = [](Encoding) {}, wstring_view tip = L"�ֶ�ѡ���ļ�����: "sv)
			:SelectWindow(console, drawRange, { L"UTF-8"s, L"GB 2312"s, L"�Զ�ʶ�� (ʵ���Թ���)"s, L"ǿ�ƴ� (����!)"s,L"ȡ��"s }, tip),
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
			if (choiceIndex == 0)	// ����
				doOverride(choiceIndex);
			else	// ȡ��, ��ʱ������ callback, Ч���Ͱ� Esc Ӧ����һ����
				return;
			// TODO: ����������ȡ����Ӧ���ص�ԭ����, �����벻�ø�
		}

		OverrideOrNotWindow(ConsoleHandler& console, const ConsoleRect& drawRange, function<void(size_t)> doOverride = [](size_t) {})
			:SelectWindow(console, drawRange, { L"����"s, L"ȡ��"s }, L"�ļ��Ѵ���. �Ƿ񸲸�? "sv),
			doOverride(doOverride)
		{
			nowChoose = 1;	// Ĭ�ϲ�����
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
				L"�ļ��޷�ͨ�� UTF-8 �����, ���ֶ�ѡ�����: "sv);
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
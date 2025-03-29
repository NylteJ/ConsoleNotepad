// Windows.ixx
// ���� Windows
// ʵ�����ǿ���̨����ģ��, �������Ѻõ���ʾ�õ�
// �� Editor ����, ֻ�ÿ����Լ��ڲ��Ļ���, �� UI ģ�鸺������ƴ������
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
	// ֻ��һ�����
	class BasicWindow :public UIComponent
	{
	protected:
		ConsoleHandler& console;

		ConsoleRect drawRange;

		bool nowExit = false;	// �е��, ������������
	protected:
		void EraseThis(UnionHandler& handlers)
		{
			handlers.ui.EraseComponent(this);
			nowExit = true;
		}
	public:
		void PrintFrame()
		{
			if (drawRange.Height() < 2 || drawRange.Width() < 2)	// ��Ӧ���, ����ֱ�� throw ��
				throw invalid_argument{ "����̫С��! "s };

			console.HideCursor();

			console.Print(L'��' + (ranges::views::repeat(L'��', drawRange.Width() - 2) | ranges::to<wstring>()) + L'��', drawRange.leftTop);

			for (auto y = drawRange.leftTop.y + 1; y < drawRange.rightBottom.y; y++)
				console.Print(L'��' + (ranges::views::repeat(L' ', drawRange.Width() - 2) | ranges::to<wstring>()) + L'��', { drawRange.leftTop.x,y });

			console.Print(L'��' + (ranges::views::repeat(L'��', drawRange.Width() - 2) | ranges::to<wstring>()) + L'��', { drawRange.leftTop.x,drawRange.rightBottom.y });

			console.ShowCursor();
		}

		void ManageInput(const InputHandler::MessageWindowSizeChanged& message, UnionHandler& handlers) override
		{
			// ��û������������Ӧ�´��ڴ�С, ֻ�����´��ڴ�С��������������ʱ���Ի�
			// (���ڴ�Сһ�����Ի�ò��Ҳû��Ҫ......)
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
	};

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
		void ManagePath(wstring_view path, UnionHandler& handlers) override
		{
			handlers.file.CreateFile(path);

			handlers.file.Write(handlers.ui.mainEditor.GetData());

			callback();
		}

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
			:SelectWindow(console, drawRange, {L"����"s, L"������"s, L"ȡ��"s}, L"��ǰ����δ����. �Ƿ��ȱ���? "sv),
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
			if (choiceIndex + 1 == choices.size())	// ȡ��
				return;
			if (choiceIndex + 2 == choices.size())	// ǿ�ƴ�
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

		EncodingSelectWindow(ConsoleHandler& console, const ConsoleRect& drawRange, function<void(Encoding)> callback = [](Encoding) {}, wstring_view tip = L"�ֶ�ѡ���ļ�����: "sv)
			:SelectWindow(console, drawRange, { L"UTF-8 (Ĭ��)"s, L"GB 2312"s, L"ǿ�ƴ� (���׳� bug)"s,L"ȡ��"s}, tip),
			callback(callback)
		{
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


	class FindWindow :public BasicWindow
	{
	private:
		constexpr static wstring_view tipText1 = L"����: ���س�����, �÷�����л�����."sv;
		constexpr static wstring_view tipText2 = L"��ʹ�� \\n, \\r ����ָ�س��ͻ���."sv;
		constexpr static wstring_view tipText3 = L"Ŀǰ��֧����ȫ����ȫ�֡�������ƥ��."sv;
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

			size_t pos = 0;
			while ((pos = stringToFind.find(L"\\n"sv, pos)) != string::npos)
			{
				stringToFind.replace(pos, L"\\n"sv.size(), L"\n"sv);
				pos += L"\n"sv.size();
			}

			pos = 0;
			while ((pos = stringToFind.find(L"\\r"sv, pos)) != string::npos)
			{
				stringToFind.replace(pos, L"\\r"sv.size(), L"\r"sv);
				pos += L"\r"sv.size();
			}

			return stringToFind;
		}

		void ReFindAll(UnionHandler& handlers)
		{
			allFindedIndexs.clear();

			wstring stringToFind = GetNowFindText();
			wstring_view stringAll = handlers.ui.mainEditor.GetData();

			lastFindText = stringToFind;

			// BMH Ӧ��������� BM ���ʺ�һ��?
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
				EraseThis(handlers);	// TODO: �ĳɿɻָ���ʧ��, ������ֱ�ӹص�

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
}
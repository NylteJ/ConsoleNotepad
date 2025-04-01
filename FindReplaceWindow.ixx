// FindReplaceWindow.ixx
// �����滻�Ķ���һ����
// ���۾��Ǵ����е��ѿ�����
export module FindReplaceWindow;

import std;

import ConsoleHandler;
import InputHandler;
import UIComponent;
import Selector;
import BasicWindow;

import Editor;

using namespace std;

export namespace NylteJ
{
	class FindReplaceWindow :public BasicWindow
	{
	private:
		using FindOptions = bitset<4>;
	private:
		constexpr static wstring_view titleFind = L"����: ���س�����, �÷�����л�����."sv;
		constexpr static wstring_view titleReplace = L"�滻: ���س��滻, �÷�����л�����."sv;
		constexpr static wstring_view tipText1 = L"�� \\n, \\t ��ָ�س���Tab, \\\\n ��ָ \"\\n\"."sv;
		constexpr static wstring_view tipText2Find = L"�� Tab �л�ѡ��."sv;
		constexpr static wstring_view tipText2Replace = L"�� Tab �л�ѡ��, Shift + �س��滻ȫ��."sv;
		constexpr static wstring_view tipText3Replace = L"�滻Ϊ:"sv;
	private:
		Editor findEditor;
		Editor replaceEditor;

		vector<size_t> allFindedIndexs;

		size_t nowFindedIndexID = 0;

		wstring lastFindText = L""s;
		FindOptions lastFindOptions;

		Selector matchAllCase;	// ���ִ�Сд
		Selector matchFullWord;	// ȫ��ƥ��

		vector<UIComponent*> components = { &findEditor,&matchAllCase,&matchFullWord };

		decltype(components.begin()) nowFocused = components.begin();

		const bool findMode = true;	// ��ǰ����ģʽ
	private:
		void PrintWindow()
		{
			console.HideCursor();

			wstring_view title, tip1, tip2;

			if (findMode)
			{
				title = titleFind;
				tip2 = tipText2Find;
			}
			else
			{
				title = titleReplace;
				tip2 = tipText2Replace;

				console.Print(tipText3Replace, { drawRange.leftTop.x + 1,drawRange.leftTop.y + 4 });
			}

			tip1 = tipText1;

			console.Print(title, { drawRange.leftTop.x + 1,drawRange.leftTop.y + 1 });
			console.Print(tip1, { drawRange.leftTop.x + 1,drawRange.leftTop.y + 2 });
			console.Print(tip2, { drawRange.leftTop.x + 1,drawRange.rightBottom.y - 1 });
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

		template<bool inverse = false>
		wstring ConvertText(wstring str) const
		{
			auto table = vector{ pair	{L"\\n"sv,L"\n"sv},
										{L"\\\n"sv,L"\\n"sv},
										{L"\\t"sv,L"\t"sv},
										{L"\\\t"sv,L"\\t"sv},
										{L"\\r"sv,L"\r"sv},
										{L"\\\r"sv,L"\\r"sv} };

			auto getView = [&]() constexpr
				{
					if constexpr (inverse)
						return table
						| ranges::views::reverse
						| ranges::views::transform([](auto&& x) {return pair{ x.second,x.first }; });
					else
						return table
						| ranges::views::all;
				};

			for (auto&& [source, target] : getView())
			{
				size_t pos = 0;
				while ((pos = str.find(source, pos)) != string::npos)
				{
					str.replace(pos, source.size(), target);
					pos += target.size();
				}
			}

			return str;
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
			lastFindOptions = GetFindOptions();

			// BMH Ӧ��������� BM ���ʺ�һ��?
			boyer_moore_horspool_searcher searcher = { stringToFind.begin(),stringToFind.end(),hash<wchar_t>{},
				[&](auto&& x,auto&& y)
				{
					if (MatchAllCase() && iswalpha(x) && iswalpha(y))
						return towupper(x) == towupper(y);

					return x == y;
				} };

			auto iter = stringAll.begin();

			while (true)
			{
				iter = search(iter, stringAll.end(), searcher);

				if (iter == stringAll.end())
					break;
				if (!MatchFullWord()
					|| ((iter == stringAll.begin() || !iswalpha(*(iter - 1)))
						&& (iter + stringToFind.size() == stringAll.end() || !iswalpha(*(iter + stringToFind.size())))))
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

		bool MatchAllCase() const
		{
			return matchAllCase.GetNowChoose() == 1;
		}
		bool MatchFullWord() const
		{
			return matchFullWord.GetNowChoose() == 1;
		}

		FindOptions GetFindOptions() const
		{
			FindOptions ret;
			ret[0] = MatchAllCase();
			ret[1] = MatchFullWord();
			return ret;
		}

		void ReplaceNext(UnionHandler& handlers)
		{
			if (allFindedIndexs.empty())
				return;

			MoveMainEditorPos(handlers);

			if (!GetNowReplaceText().empty())
				handlers.ui.mainEditor.Insert(GetNowReplaceText());
			else
				handlers.ui.mainEditor.Erase();

			const auto offset = static_cast<long long>(GetNowReplaceText().size()) - static_cast<long long>(GetNowFindText().size());

			nowFindedIndexID = allFindedIndexs.erase(allFindedIndexs.begin() + nowFindedIndexID) - allFindedIndexs.begin();

			for (size_t i = nowFindedIndexID; i < allFindedIndexs.size(); i++)	// ����Ķ�Ҫƫ��, ǰ��Ķ�����
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

			// ����ѡ�񷴸����� Editor::Insert ֻ��Ϊ�˱��ڳ��� (��Ȼ�ð�������)
			while (nowFindedIndexID > 0)
			{
				// ���ﱾ���屾�����豻����, ����������� MoveMainEditorPos
				handlers.ui.mainEditor.MoveCursorToIndex(allFindedIndexs[nowFindedIndexID],
					allFindedIndexs[nowFindedIndexID] + lastFindText.size());

				if (!GetNowReplaceText().empty())
					handlers.ui.mainEditor.Insert(GetNowReplaceText());
				else
					handlers.ui.mainEditor.Erase();

				nowFindedIndexID--;
			}
			// size_t ��Ǹ�, ����ֻ��������
			handlers.ui.mainEditor.MoveCursorToIndex(allFindedIndexs[nowFindedIndexID],
				allFindedIndexs[nowFindedIndexID] + lastFindText.size());

			if (!GetNowReplaceText().empty())
				handlers.ui.mainEditor.Insert(GetNowReplaceText());
			else
				handlers.ui.mainEditor.Erase();

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
				if (GetNowFindText() != lastFindText || GetFindOptions() != lastFindOptions)
					ReFindAll(handlers);
				else if (findMode)
					FindNext(handlers);
				else if (message.extraKeys.Shift())
					ReplaceAll(handlers);
				else ReplaceNext(handlers);

				return;
			}
			else if (message.key == Up)
			{
				if (GetNowFindText() != lastFindText || GetFindOptions() != lastFindOptions)
					ReFindAll(handlers);

				FindPrev(handlers);	// ���ʱ��û����, ��ʱ��ֱ���������һ��, �������������������Ϊ��������һ��

				return;
			}
			else if (message.key == Down)
			{
				if (GetNowFindText() != lastFindText || GetFindOptions() != lastFindOptions)
					ReFindAll(handlers);
				else
					FindNext(handlers);

				return;
			}
			else if (message.key == Tab)
			{
				(*nowFocused)->WhenUnfocused();

				++nowFocused;
				if (nowFocused == components.end())
					nowFocused = components.begin();

				(*nowFocused)->WhenRefocused();

				return;
			}

			(*nowFocused)->ManageInput(message, handlers);
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

			(*nowFocused)->ManageInput(message, handlers);
		}

		void WhenFocused() override
		{
			BasicWindow::WhenFocused();

			PrintWindow();

			for (auto ptr : components)
				ptr->Repaint();

			(*nowFocused)->WhenRefocused();
		}
		void WhenRefocused() override
		{
			BasicWindow::WhenRefocused();

			(*nowFocused)->WhenRefocused();
		}

		FindReplaceWindow(ConsoleHandler& console, const ConsoleRect& drawRange, wstring strToFind = L""s, bool findMode = true)
			:BasicWindow(console, drawRange),
			findEditor(console, L""s, { {drawRange.leftTop.x + 1,drawRange.leftTop.y + 3},
												{drawRange.rightBottom.x - 1,drawRange.leftTop.y + 3} }),
			replaceEditor(console, L""s, { {drawRange.leftTop.x + 1,drawRange.leftTop.y + 5},
												{drawRange.rightBottom.x - 1,drawRange.leftTop.y + 5} }),
			matchAllCase(console,
				{ {drawRange.leftTop.x + 1,drawRange.leftTop.y + 4},
					{drawRange.rightBottom.x - 1,drawRange.leftTop.y + 4} },
				{ L"���ִ�Сд"s,L"�����ִ�Сд"s }),
			matchFullWord(console,
				{ {drawRange.leftTop.x + 1,drawRange.leftTop.y + 5},
					{drawRange.rightBottom.x - 1,drawRange.leftTop.y + 5} },
				{ L"ƥ������"s,L"ȫ��ƥ��"s }),
			findMode(findMode)
		{
			findEditor.SetData(ConvertText<true>(strToFind));

			if (!findMode)
			{
				components.insert(components.begin() + 1, &replaceEditor);
				nowFocused = components.begin();

				matchAllCase.SetDrawRange({ {drawRange.leftTop.x + 1,drawRange.leftTop.y + 6},
											{drawRange.rightBottom.x - 1,drawRange.leftTop.y + 6} });
				matchFullWord.SetDrawRange({ {drawRange.leftTop.x + 1,drawRange.leftTop.y + 7},
												{drawRange.rightBottom.x - 1,drawRange.leftTop.y + 7} });
			}

			findEditor.MoveCursorToEnd();
			findEditor.SelectAll();			// �����������༭����Ϊ
		}
	};
}
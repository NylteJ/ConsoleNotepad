// FindReplaceWindow.ixx
// 查找替换的二合一窗口
// 代价就是代码有点难看懂了
// 加上正则表达式后更乱了, 纯屎山来的
// TODO: 重构屎山 (这是我打的第几个这样的 TODO 了)
export module FindReplaceWindow;

import std;

import ConsoleHandler;
import InputHandler;
import UIComponent;
import Selector;
import BasicWindow;
import SettingMap;
import UnionHandler;

import Exceptions;

import Editor;

using namespace std;

export namespace NylteJ
{
	class FindReplaceWindow :public BasicWindow
	{
	private:
		using FindOptions = bitset<3>;
	private:
		constexpr static wstring_view titleFind = L"查找: 按回车查找, 用方向键切换对象."sv;
		constexpr static wstring_view titleReplace = L"替换: 按回车替换, 用方向键切换对象."sv;
		constexpr static wstring_view tipText1Normal = L"用 \\n, \\t 代指回车、Tab, \\\\n 代指 \"\\n\"."sv;
		constexpr static wstring_view tipText1Regex = L"基本遵循 ECMAScript 正则表达式文法."sv;
		constexpr static wstring_view tipText2Find = L"用 Tab 切换选框."sv;
		constexpr static wstring_view tipText2Replace = L"用 Tab 切换选框, Shift + 回车替换全部."sv;
		constexpr static wstring_view tipText3Replace = L"替换为:"sv;
	private:
		Editor findEditor;
		Editor replaceEditor;

		vector<size_t> allFindedIndexs;

		vector<size_t> allFindResultSizeForRegex;
		vector<wstring> allReplaceResultForRegex;	// 仅在正则替换时使用

		size_t nowFindedIndexID = 0;

		wstring lastFindText = L""s;
		wstring lastReplaceText = L""s;		// 仅在正则替换时使用, 其他时候当替换文本变更时无需重新搜索
		FindOptions lastFindOptions;

		Selector matchAllCase;	// 区分大小写
		Selector matchFullWord;	// 全字匹配
		Selector matchRegex;	// 正则表达式

		vector<UIComponent*> components = { &findEditor,&matchAllCase,&matchFullWord,&matchRegex };

		decltype(components.begin()) nowFocused = components.begin();

		const bool findMode = true;	// 当前窗口模式
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

			if (MatchRegex())
				tip1 = tipText1Regex;
			else
				tip1 = tipText1Normal;

			console.Print(title, { drawRange.leftTop.x + 1,drawRange.leftTop.y + 1 });
			console.Print(tip1, { drawRange.leftTop.x + 1,drawRange.leftTop.y + 2 });
			console.Print(tip2, { drawRange.leftTop.x + 1,drawRange.rightBottom.y - 1 });
		}

		void MoveMainEditorPos(UnionHandler& handlers)
		{
			if (nowFindedIndexID < allFindedIndexs.size())
			{
				if(MatchRegex())
					handlers.ui.mainEditor.MoveCursorToIndex(allFindedIndexs[nowFindedIndexID], allFindedIndexs[nowFindedIndexID] + allFindResultSizeForRegex[nowFindedIndexID]);
				else
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
						| views::reverse
						| views::transform([](auto&& x) {return pair{ x.second,x.first }; });
					else
						return table
						| views::all;
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
			auto ret = findEditor.GetData() | ranges::to<wstring>();

			if (MatchRegex())
				return ret;

			return ConvertText(ret);
		}
		wstring GetNowReplaceText() const
		{
			auto ret = replaceEditor.GetData() | ranges::to<wstring>();

			if (MatchRegex())
				return ret;

			return ConvertText(ret);
		}

		void ReFindAll(UnionHandler& handlers)
		{
			allFindedIndexs.clear();
			allFindResultSizeForRegex.clear();
			allReplaceResultForRegex.clear();

			wstring stringToFind = GetNowFindText();
			wstring_view stringAll = handlers.ui.mainEditor.GetData();

			lastFindText = stringToFind;
			lastFindOptions = GetFindOptions();

			if (!MatchRegex())
			{
				// BMH 应该在这里比 BM 更适合一点?
				boyer_moore_horspool_searcher searcher = { stringToFind.begin(),stringToFind.end(),hash<wchar_t>{},
					[&](auto&& x,auto&& y)
					{
						if (MatchAllCase())
							return towlower(x) == towlower(y);

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
			}
			else
			{
				if (!findMode)
					lastReplaceText = GetNowReplaceText();

				auto regexOption = regex_constants::ECMAScript | regex_constants::optimize;
				if (MatchAllCase())
					regexOption |= regex_constants::icase;
				if (MatchFullWord())
				{
					if (!stringToFind.starts_with(L"\\b"sv))
						stringToFind = L"\\b"s + stringToFind;
					if (!stringToFind.ends_with(L"\\b"sv))
						stringToFind += L"\\b"s;
				}

				wregex regexToFind{ stringToFind, regexOption };
				regex_iterator<decltype(stringAll.begin())> iter = {stringAll.begin(),stringAll.end(),regexToFind};
				decltype(iter) end{};
				while (iter != end)
				{
					allFindedIndexs.emplace_back(iter->position());
					allFindResultSizeForRegex.emplace_back(iter->length());

					if (!findMode)
						allReplaceResultForRegex.emplace_back(iter->format(GetNowReplaceText()));

					++iter;
				}
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
		bool MatchRegex() const
		{
			return matchRegex.GetNowChoose() == 1;
		}

		FindOptions GetFindOptions() const
		{
			FindOptions ret;
			ret[0] = MatchAllCase();
			ret[1] = MatchFullWord();
			ret[2] = MatchRegex();
			return ret;
		}

		void ReplaceNext(UnionHandler& handlers)
		{
			if (allFindedIndexs.empty())
				return;

			MoveMainEditorPos(handlers);

			auto nowReplaceText = GetNowReplaceText();

			if (MatchRegex())
				nowReplaceText = allReplaceResultForRegex[nowFindedIndexID];

			if (!nowReplaceText.empty())
				handlers.ui.mainEditor.Insert(nowReplaceText);
			else
				handlers.ui.mainEditor.Erase();

			long long offset;
			
			if (MatchRegex())
				offset = static_cast<long long>(nowReplaceText.size()) - static_cast<long long>(allFindResultSizeForRegex[nowFindedIndexID]);
			else
				offset = static_cast<long long>(nowReplaceText.size()) - static_cast<long long>(GetNowFindText().size());

			nowFindedIndexID = allFindedIndexs.erase(allFindedIndexs.begin() + nowFindedIndexID) - allFindedIndexs.begin();
			if (MatchRegex())
			{
				allFindResultSizeForRegex.erase(allFindResultSizeForRegex.begin() + nowFindedIndexID);
				allReplaceResultForRegex.erase(allReplaceResultForRegex.begin() + nowFindedIndexID);
			}

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

			long long nowIndexID = allFindedIndexs.size() - 1;

			// 这里选择反复调用 Editor::Insert 只是为了便于撤销 (虽然得挨个撤销)
			while (nowIndexID >= 0)
			{
				auto size = lastFindText.size();
				if (MatchRegex())
					size = allFindResultSizeForRegex[nowIndexID];

				// 这里本窗体本身无需被绘制, 所以无需调用 MoveMainEditorPos
				handlers.ui.mainEditor.MoveCursorToIndex(allFindedIndexs[nowIndexID],
					allFindedIndexs[nowIndexID] + size);

				auto nowReplaceText = GetNowReplaceText();

				if (MatchRegex())
					nowReplaceText = allReplaceResultForRegex[nowIndexID];

				if (!nowReplaceText.empty())
					handlers.ui.mainEditor.Insert(nowReplaceText);
				else
					handlers.ui.mainEditor.Erase();

				nowIndexID--;
			}
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

			using enum InputHandler::MessageKeyboard::Key;

			auto needRefind = [&]()
				{
					return GetNowFindText() != lastFindText
						|| GetFindOptions() != lastFindOptions
						|| (!findMode && MatchRegex() && GetNowReplaceText() != lastReplaceText);
				};

			if (message.key == Enter)
			{
				if (needRefind())
				{
					ReFindAll(handlers);

					if (message.extraKeys.Shift() && !findMode)
						ReplaceAll(handlers);
				}
				else if (findMode)
					FindNext(handlers);
				else if (message.extraKeys.Shift())
					ReplaceAll(handlers);
				else ReplaceNext(handlers);

				return;
			}
			else if (message.key == Up)
			{
				if (needRefind())
					ReFindAll(handlers);

				FindPrev(handlers);	// 这个时候没问题, 此时会直接跳到最后一个, 另外两种情况不行是因为会跳过第一个

				return;
			}
			else if (message.key == Down)
			{
				if (needRefind())
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

			bool lastRegexEnable = MatchRegex();

			(*nowFocused)->ManageInput(message, handlers);

			if (lastRegexEnable != MatchRegex())	// 为了让正则表达式的提示文本能即时生效
				WhenFocused();
		}
		void ManageInput(const InputHandler::MessageMouse& message, UnionHandler& handlers) override
		{
			BasicWindow::ManageInput(message, handlers);

			if ((message.LeftClick() || message.RightClick())
				&& !drawRange.Contain(message.position))
			{
				EraseThis(handlers);	// TODO: 改成可恢复的失焦, 而不是直接关掉

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

		FindReplaceWindow(ConsoleHandler& console, const ConsoleRect& drawRange, const SettingMap& settingMap, wstring strToFind = L""s, bool findMode = true)
			:BasicWindow(console, drawRange),
			findEditor(console, L""s, {			{drawRange.leftTop.x + 1,drawRange.leftTop.y + 3},
												{drawRange.rightBottom.x - 1,drawRange.leftTop.y + 3} },
				settingMap),
			replaceEditor(console, L""s, {		{drawRange.leftTop.x + 1,drawRange.leftTop.y + 5},
												{drawRange.rightBottom.x - 1,drawRange.leftTop.y + 5} },
				settingMap),
			matchAllCase(console,
				{	{drawRange.leftTop.x + 1,drawRange.leftTop.y + 4},
					{drawRange.rightBottom.x - 1,drawRange.leftTop.y + 4} },
				{ L"区分大小写"s,L"不区分大小写"s }),
			matchFullWord(console,
				{	{drawRange.leftTop.x + 1,drawRange.leftTop.y + 5},
					{drawRange.rightBottom.x - 1,drawRange.leftTop.y + 5} },
				{ L"匹配所有"s,L"全字匹配"s }),
			matchRegex(console,
				{	{drawRange.leftTop.x + 1,drawRange.leftTop.y + 6},
					{drawRange.rightBottom.x - 1,drawRange.leftTop.y + 6} },
				{ L"正常匹配"s,L"正则匹配"s }),
			findMode(findMode)
		{
			if (findMode && drawRange.Height() < 9)
				throw Exception{ L"窗口太小了!"sv };
			if (!findMode && drawRange.Height() < 11)
				throw Exception{ L"窗口太小了!"sv };

			findEditor.SetData(ConvertText<true>(strToFind));

			if (!findMode)
			{
				components.insert(components.begin() + 1, &replaceEditor);
				nowFocused = components.begin();

				matchAllCase.SetDrawRange({ {drawRange.leftTop.x + 1,drawRange.leftTop.y + 6},
											{drawRange.rightBottom.x - 1,drawRange.leftTop.y + 6} });
				matchFullWord.SetDrawRange({	{drawRange.leftTop.x + 1,drawRange.leftTop.y + 7},
												{drawRange.rightBottom.x - 1,drawRange.leftTop.y + 7} });
				matchRegex.SetDrawRange({	{drawRange.leftTop.x + 1,drawRange.leftTop.y + 8},
											{drawRange.rightBottom.x - 1,drawRange.leftTop.y + 8} });
			}

			findEditor.MoveCursorToEnd();
			findEditor.SelectAll();			// 更贴近主流编辑器行为
		}
	};
}
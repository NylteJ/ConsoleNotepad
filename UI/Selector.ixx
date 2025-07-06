// Selector.ixx
// 一个用左 / 右方向键进行选择的 UI 模块
// 只适配单行的情况
export module Selector;

import std;

import UIComponent;
import ConsoleTypedef;
import ConsoleHandler;
import Exceptions;
import BasicColors;
import Utils;
import InputHandler;
import String;

using namespace std;

export namespace NylteJ
{
	class Selector :public UIComponent
	{
	private:
		ConsoleHandler& console;

		vector<String> choices;
		size_t nowChoose = 0;
	private:
		void Print(bool highLight = true)
		{
			console.HideCursor();

			auto color = highLight ? BasicColors::inverseColor : BasicColors::stayOldColor;

			console.Print(u8"<"sv, drawRange.leftTop, color, color);
			console.Print(String(u8' ', drawRange.Width() - 2), color, color);
			console.Print(u8">"sv, color, color);

			size_t beginX = 0;
			if (GetDisplayLength(choices[nowChoose]) <= drawRange.Width())
				beginX = (drawRange.Width() - GetDisplayLength(choices[nowChoose])) / 2;

			console.Print(choices[nowChoose], { drawRange.leftTop.x + beginX,drawRange.leftTop.y }, color, color);
		}

		void ToLastChoice()
		{
			if (nowChoose == 0)
				nowChoose = choices.size() - 1;
			else
				nowChoose--;

			Print();
		}
		void ToNextChoice()
		{
			if (nowChoose == choices.size() - 1)
				nowChoose = 0;
			else
				nowChoose++;

			Print();
		}
	public:
		size_t GetNowChoose() const
		{
			return nowChoose;
		}
		void SetNowChoose(size_t chooseIndex)
		{
			nowChoose = chooseIndex;
		}

		void ManageInput(const InputHandler::MessageWindowSizeChanged& message, UnionHandler& handlers) override {}
		void ManageInput(const InputHandler::MessageKeyboard& message, UnionHandler& handlers) override
		{
			using enum InputHandler::MessageKeyboard::Key;

			switch (message.key)
			{
			case Left:
				ToLastChoice();
				return;
			case Right:
				ToNextChoice();
				return;
			}
		}
		void ManageInput(const InputHandler::MessageMouse& message, UnionHandler& handlers) override
		{
			if (message.LeftClick())
				if (message.position == drawRange.leftTop)
					ToLastChoice();
				else if (message.position == drawRange.rightBottom)
					ToNextChoice();
		}

		void WhenFocused() override
		{
			console.HideCursor();

			Print();
		}
		void WhenRefocused() override	// 它的 Refocus 也需要重绘, 因为 Refocus 就意味着曾经 Unfocus 过, 所以要重新变一次色
		{
			WhenFocused();
		}

		void WhenUnfocused() override
		{
			Print(false);
		}
		void Repaint() override
		{
			Print(false);
		}

		void SetDrawRange(const ConsoleRect& newRange)
		{
			if (drawRange.Width() <= 2 || drawRange.Height() <= 0)
				throw Exception{ u8"Selector 控件太小了"sv };
            drawRange = newRange;
		}

		Selector(ConsoleHandler& console, ConsoleRect drawRange, const vector<String>& choices, size_t nowChooseIndex = 0)
			:console(console), UIComponent(drawRange), choices(choices), nowChoose(nowChooseIndex)
		{
			if (choices.empty())
				throw Exception{ u8"选项不可为空"sv };
			if (drawRange.Width() <= 2 || drawRange.Height() <= 0)
				throw Exception{ u8"Selector 控件太小了"sv };
		}
	};
}
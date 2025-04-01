// Selector.ixx
// һ������ / �ҷ��������ѡ��� UI ģ��
// ֻ���䵥�е����
export module Selector;

import std;

import UIComponent;
import ConsoleTypedef;
import ConsoleHandler;
import Exceptions;
import BasicColors;
import Utils;

using namespace std;

export namespace NylteJ
{
	class Selector :public UIComponent
	{
	private:
		ConsoleHandler& console;
		ConsoleRect drawRange;

		vector<wstring> choices;
		size_t nowChoose = 0;
	private:
		void Print(bool highLight = true)
		{
			console.HideCursor();

			auto color = highLight ? BasicColors::inverseColor : BasicColors::stayOldColor;

			console.Print(L"<", drawRange.leftTop, color, color);
			console.Print(ranges::views::repeat(L' ', drawRange.Width() - 2) | ranges::to<wstring>(), color, color);
			console.Print(L">", color, color);

			size_t beginX = 0;
			if (GetDisplayLength(choices[nowChoose]) <= drawRange.Width())
				beginX = (drawRange.Width() - GetDisplayLength(choices[nowChoose])) / 2;

			console.Print(choices[nowChoose], { drawRange.leftTop.x + beginX,drawRange.leftTop.y }, color, color);
		}
	public:
		size_t GetNowChoose() const
		{
			return nowChoose;
		}

		void ManageInput(const InputHandler::MessageWindowSizeChanged& message, UnionHandlerInterface<shared_ptr<UIComponent>>& handlers) override {}
		void ManageInput(const InputHandler::MessageKeyboard& message, UnionHandlerInterface<shared_ptr<UIComponent>>& handlers) override
		{
			using enum InputHandler::MessageKeyboard::Key;

			switch (message.key)
			{
			case Left:
				if (nowChoose == 0)
					nowChoose = choices.size() - 1;
				else
					nowChoose--;

				Print();
				return;
			case Right:
				if (nowChoose == choices.size() - 1)
					nowChoose = 0;
				else
					nowChoose++;

				Print();
				return;
			}
		}
		void ManageInput(const InputHandler::MessageMouse& message, UnionHandlerInterface<shared_ptr<UIComponent>>& handlers) override {}

		void WhenFocused() override
		{
			console.HideCursor();

			Print();
		}
		void WhenRefocused() override	// ���� Refocus Ҳ��Ҫ�ػ�, ��Ϊ Refocus ����ζ������ Unfocus ��, ����Ҫ���±�һ��ɫ
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
				throw Exception{ L"Selector �ؼ�̫С��"sv };
			drawRange = newRange;
		}

		Selector(ConsoleHandler& console, ConsoleRect drawRange, const vector<wstring>& choices)
			:console(console), drawRange(drawRange), choices(choices)
		{
			if (choices.empty())
				throw Exception{ L"ѡ���Ϊ��"sv };
			if (drawRange.Width() <= 2 || drawRange.Height() <= 0)
				throw Exception{ L"Selector �ؼ�̫С��"sv };
		}
	};
}
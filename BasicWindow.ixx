// BasicWindow.ixx
// 只是一个外框
export module BasicWindow;

import std;

import ConsoleHandler;
import InputHandler;
import UIComponent;

using namespace std;

export namespace NylteJ
{
	class BasicWindow :public UIComponent
	{
	protected:
		ConsoleHandler& console;
	protected:
		void EraseThis(UnionHandler& handlers)
		{
			handlers.ui.EraseComponent(this);
		}
	public:
		void PrintFrame() const
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
			:UIComponent(drawRange),
			console(console)
		{
		}

		virtual ~BasicWindow() = default;
	};
}
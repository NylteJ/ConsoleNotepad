// BasicWindow.ixx
// ֻ��һ�����
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
			:UIComponent(drawRange),
			console(console)
		{
		}

		virtual ~BasicWindow() = default;
	};
}
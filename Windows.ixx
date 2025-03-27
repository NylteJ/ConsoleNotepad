// Windows.ixx
// 并非 Windows
// 实际上是控制台窗口模块, 用来更友好地显示用的
// 和 Editor 类似, 只用考虑自己内部的绘制, 由 UI 模块负责将他们拼接起来
export module Windows;

import std;

import ConsoleHandler;
import ConsoleTypedef;
import BasicColors;
import InputHandler;
import UIComponent;

import Editor;

using namespace std;

export namespace NylteJ
{
	class WindowWithEditor :public UIComponent
	{
	private:
		ConsoleHandler& console;

		ConsoleRect drawRange;

		Editor editor;

		constexpr static wstring_view tipText = L"打开文件: 请输入路径 (相对 / 绝对均可) 并按下回车: "sv;
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
				handlers.ui.EraseComponent(this);
		}
		void ManageInput(const InputHandler::MessageKeyboard& message, UnionHandler& handlers) override
		{
			using enum InputHandler::MessageKeyboard::Key;

			if (message.key == Esc)
			{
				handlers.ui.EraseComponent(this);
				return;
			}
			if (message.key == Enter)
			{
				// 顺便做个防呆设计, 防止有大聪明给路径加引号和双反斜杠
				wstring filePath = editor.GetData()
					| ranges::views::filter([](auto&& chr) {return chr != L'\"'; })
					| ranges::to<wstring>();
				{
					size_t pos = 0;
					while ((pos = filePath.find(L"\\\\"sv, pos)) != string::npos)
					{
						filePath.replace(pos, L"\\\\"sv.size(), L"\\"sv);
						pos += L"\\"sv.size();
					}
				}

				handlers.file.OpenFile(filePath);

				handlers.ui.mainEditor.SetData(handlers.file.ReadAll());
				handlers.ui.mainEditor.ResetCursor();

				handlers.ui.EraseComponent(this);
				return;
			}

			editor.ManageInput(message, handlers);
		}
		void ManageInput(const InputHandler::MessageMouse& message, UnionHandler& handlers) override
		{
			editor.ManageInput(message, handlers);
		}

		void WhenFocused() override
		{
			PrintFrame();
			if (drawRange.Height() >= 4)
				console.Print(tipText, { drawRange.leftTop.x + 1,drawRange.leftTop.y + 1 });
			else
				console.Print(tipText, { drawRange.leftTop.x + 1,drawRange.leftTop.y });

			editor.WhenFocused();
		}

		WindowWithEditor(ConsoleHandler& console, const ConsoleRect& drawRange)
			:console(console),
			drawRange(drawRange),
			editor(console, L""s, { {drawRange.leftTop.x + 1,drawRange.leftTop.y + 1},
									{drawRange.rightBottom.x - 1,drawRange.rightBottom.y - 1} })
		{
			if (drawRange.Width() >= 4)
				editor.ChangeDrawRange({	{drawRange.leftTop.x + 1,drawRange.leftTop.y + 2},
											{drawRange.rightBottom.x - 1,drawRange.rightBottom.y - 1} });
		}
	};
}
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

		constexpr static wstring_view tipText = L"���ļ�: ������·�� (��� / ���Ծ���) �����»س�: "sv;
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
				// ˳�������������, ��ֹ�д������·�������ź�˫��б��
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
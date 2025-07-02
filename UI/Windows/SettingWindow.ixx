// SettingWindow.ixx
// 设置页面
// 其实在实现了 Editor 和 Selector 后已经不难实现了
// 是 SettingsHandler 的前端
export module SettingWindow;

import std;

import BasicWindow;
import ConsoleTypedef;
import Editor;
import Selector;
import Exceptions;
import Utils;
import StringEncoder;
import FileHandler;
import SettingsHandler;
import SettingMap;
import InputHandler;
import UnionHandler;
import ConsoleHandler;

using namespace std;

export namespace NylteJ
{
	class SettingWindow :public BasicWindow
	{
	private:
		constexpr static wstring_view titleText = L"设置"sv;
		constexpr static wstring_view tipText = L"按回车键保存并退出, 按 Esc 键不保存退出. 用方向键切换设置项, 也支持鼠标操作."sv;
	private:
		size_t nowBeginSettingIndex = 0;

		size_t nowFocusedSettingIndex = 0;

		SettingsHandler settings;
	private:
		constexpr size_t MaxSettingInScreen() const
		{
			return (drawRange.Height() - 4) / 2;
		}

		constexpr auto NowVisibleSettingList() const
		{
			return settings.settingList
				| views::drop(nowBeginSettingIndex)
				| views::take(MaxSettingInScreen());
		}

		void PrintSelf() const
		{
			console.Print(titleText, { drawRange.leftTop.x + (drawRange.Width() - 2 - GetDisplayLength(titleText)) / 2 + 1,
				drawRange.leftTop.y + 1 });
			console.Print(tipText, { drawRange.leftTop.x + (drawRange.Width() - 2 - GetDisplayLength(tipText)) / 2 + 1,
				drawRange.rightBottom.y - 1 });

			ConsolePosition nowPos = { drawRange.leftTop.x + 1,drawRange.leftTop.y + 2 };

			for (auto&& settingItem : NowVisibleSettingList())
			{
				console.Print(views::repeat(L' ', drawRange.Width() - 2) | ranges::to<wstring>(), nowPos);
				console.Print(settingItem.tipText, nowPos);
				nowPos.y++;
				settingItem.component->SetDrawRange({ nowPos,{drawRange.rightBottom.x - 1,nowPos.y} });
				settingItem.component->Repaint();
				nowPos.y++;
			}
		}

		void ChangeFocusIndex(size_t newIndex)
		{
			if (newIndex >= nowBeginSettingIndex + MaxSettingInScreen())
			{
				nowBeginSettingIndex = newIndex - MaxSettingInScreen() + 1;
				nowFocusedSettingIndex = newIndex;
				PrintSelf();
			}
			else if (newIndex < nowBeginSettingIndex)
			{
				nowBeginSettingIndex = newIndex;
				nowFocusedSettingIndex = newIndex;
				PrintSelf();
			}
			else
				nowFocusedSettingIndex = newIndex;
		}
	public:
		void ManageInput(const InputHandler::MessageWindowSizeChanged& message, UnionHandler& handlers) override
		{
			BasicWindow::ManageInput(message, handlers);
		}
		void ManageInput(const InputHandler::MessageKeyboard& message, UnionHandler& handlers) override
		{
			using enum InputHandler::MessageKeyboard::Key;

			if (message.key == Esc)	// 得拦截掉
			{
				settings.ReloadAll();
				EraseThis(handlers);
				return;
			}

			BasicWindow::ManageInput(message, handlers);

			if (message.key == Enter)
			{
				settings.SaveAll();
				settings.ReloadAll();	// 实际上是一个规范化的过程, 比如 mins / minutes -> min

				// 这里要重载主 Editor 的 drawRange, 防止行号宽度改变带来的奇怪的显示问题
				// TODO: 写一个不那么丑陋的方法
				handlers.input.SendMessage(InputHandler::MessageWindowSizeChanged{ handlers.console.GetConsoleSize() });
				handlers.ui.mainEditor.MoveCursor(Direction::None);

				EraseThis(handlers);
				return;
			}
			else if (message.key == Down)
			{
				settings.settingList[nowFocusedSettingIndex].component->WhenUnfocused();

				if (nowFocusedSettingIndex + 1 == settings.settingList.size())
					ChangeFocusIndex(0);
				else
					ChangeFocusIndex(nowFocusedSettingIndex + 1);

				settings.settingList[nowFocusedSettingIndex].component->WhenRefocused();

				return;
			}
			else if (message.key == Up)
			{
				settings.settingList[nowFocusedSettingIndex].component->WhenUnfocused();

				if (nowFocusedSettingIndex == 0)
					ChangeFocusIndex(settings.settingList.size() - 1);
				else
					ChangeFocusIndex(nowFocusedSettingIndex - 1);

				settings.settingList[nowFocusedSettingIndex].component->WhenRefocused();

				return;
			}

			ChangeFocusIndex(nowFocusedSettingIndex);

			settings.settingList[nowFocusedSettingIndex].component->ManageInput(message, handlers);
		}
		void ManageInput(const InputHandler::MessageMouse& message, UnionHandler& handlers) override
		{
			BasicWindow::ManageInput(message, handlers);

			using enum InputHandler::MessageMouse::Type;

			if (message.LeftClick()
				&& drawRange.Contain(message.position)
				&& (message.position.y - drawRange.leftTop.y - 2) % 2 == 1)
			{
				auto newIndex = (message.position.y - drawRange.leftTop.y - 3) / 2 + nowBeginSettingIndex;
				if (newIndex != nowFocusedSettingIndex && newIndex < settings.settingList.size())
				{
					settings.settingList[nowFocusedSettingIndex].component->WhenUnfocused();
					nowFocusedSettingIndex = newIndex;
					settings.settingList[nowFocusedSettingIndex].component->WhenFocused();
				}
			}

			if (message.type == VWheeled)
			{
				auto move = message.WheelMove();
				if (move > 0)
					move = 1;
				else
					move = -1;

				if ((nowBeginSettingIndex > 0 || move > 0)
					&& (nowBeginSettingIndex + 1 < settings.settingList.size() || move < 0))
				{
					if (nowBeginSettingIndex + move + MaxSettingInScreen() >= settings.settingList.size())
					{
						if (settings.settingList.size() > MaxSettingInScreen())
							nowBeginSettingIndex = settings.settingList.size() - MaxSettingInScreen();
						else
							nowBeginSettingIndex = 0;
					}
					else if (move < 0 && nowBeginSettingIndex < -move)
						nowBeginSettingIndex = 0;
					else
						nowBeginSettingIndex += move;

					PrintSelf();
					if (nowFocusedSettingIndex >= nowBeginSettingIndex 
						&& nowFocusedSettingIndex < nowBeginSettingIndex + MaxSettingInScreen())
						settings.settingList[nowFocusedSettingIndex].component->WhenRefocused();
				}

				return;	// 拦截滚轮
			}

			settings.settingList[nowFocusedSettingIndex].component->ManageInput(message, handlers);
		}

		void WhenFocused() override
		{
			BasicWindow::WhenFocused();

			PrintSelf();

			settings.settingList[nowFocusedSettingIndex].component->WhenRefocused();
		}
		void WhenRefocused() override
		{
			BasicWindow::WhenRefocused();

			settings.settingList[nowFocusedSettingIndex].component->WhenRefocused();
		}

		SettingWindow(ConsoleHandler& console, const ConsoleRect& drawRange, SettingMap& settings)
			:BasicWindow(console, drawRange),
			settings(console, settings)
		{
			if (drawRange.Height() < 6)
				throw Exception{ L"窗口太小!"sv };
		}
	};
}
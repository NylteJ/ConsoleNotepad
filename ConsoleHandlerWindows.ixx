// ConsoleHandlerWindows.ixx
// Windows 下的具体控制台文本操作实现
// 一开始是想用虚拟终端序列实现的，但考虑到兼容性（Win10 以下的系统，以及传统 conhost 载体），最终还是使用了传统方案
module;
#include <Windows.h>
#include <csignal>
export module ConsoleHandlerWindows;

import std;

import ConsoleTypedef;
import BasicColors;
import InputHandler;

using namespace std;

export namespace NylteJ
{
	class ConsoleHandlerWindows
	{
	public:
		enum class ConsoleMode : DWORD
		{
			Insert = (ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT | ENABLE_EXTENDED_FLAGS)
			| ((0) << 16),
			Default = (ENABLE_ECHO_INPUT | ENABLE_INSERT_MODE | ENABLE_LINE_INPUT | ENABLE_MOUSE_INPUT | ENABLE_PROCESSED_INPUT | ENABLE_QUICK_EDIT_MODE)
			| ((ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT) << 16)		// TODO: 把这一项改为真正的 “默认” 选项 (比如改成在程序启动时的控制台模式)
		};
	private:
		HANDLE consoleOutputHandle;
		HANDLE consoleInputHandle;
	private:
		// 特殊约定：RGB 分量都为 -1 时代表保持不变
		WORD ColorConvert(ConsoleColor color, WORD originalAttributes, WORD red, WORD green, WORD blue, WORD intensity) const
		{
			if (color == BasicColors::stayOldColor)
				return originalAttributes;
			if (color == BasicColors::inverseColor)
				return originalAttributes xor (blue | green | red);

			WORD newAttributes = originalAttributes
				& (~(blue | green | red | intensity));

			const double maxRGB = max(max(color.R, color.G), color.B);

			if (maxRGB >= 0.1)
			{
				const double	R = color.R / maxRGB,
					G = color.G / maxRGB,
					B = color.B / maxRGB;
				if (R >= 0.5)
					newAttributes |= red;
				if (G >= 0.5)
					newAttributes |= green;
				if (B >= 0.5)
					newAttributes |= blue;

				if (maxRGB >= 192)
					newAttributes |= intensity;
			}

			return newAttributes;
		}
	public:
		ConsoleSize GetConsoleSize() const
		{
			CONSOLE_SCREEN_BUFFER_INFO consoleBufferInfo;

			GetConsoleScreenBufferInfo(consoleOutputHandle, &consoleBufferInfo);

			const ConsoleWidth width = consoleBufferInfo.srWindow.Right - consoleBufferInfo.srWindow.Left + 1;
			const ConsoleHeight height = consoleBufferInfo.srWindow.Bottom - consoleBufferInfo.srWindow.Top + 1;

			return { width,height };
		}

		ConsolePosition GetCursorPos() const
		{
			CONSOLE_SCREEN_BUFFER_INFO consoleBufferInfo;

			GetConsoleScreenBufferInfo(consoleOutputHandle, &consoleBufferInfo);

			return { consoleBufferInfo.dwCursorPosition.X, consoleBufferInfo.dwCursorPosition.Y };
		}

		void SetCursorTo(ConsolePosition pos) const
		{
			SetConsoleCursorPosition(consoleOutputHandle, { .X = static_cast<short>(pos.x),.Y = static_cast<short>(pos.y) });
		}

		// 复制粘贴领域大神
		void Print(wstring_view text) const
		{
			WriteConsoleW(consoleOutputHandle, text.data(), text.size(), nullptr, nullptr);
		}
		void Print(wstring_view text, ConsolePosition pos) const
		{
			SetCursorTo(pos);
			Print(text);
		}
		void Print(wstring_view text, ConsolePosition pos, ConsoleColor textColor) const
		{
			CONSOLE_SCREEN_BUFFER_INFO consoleBufferInfo;

			GetConsoleScreenBufferInfo(consoleOutputHandle, &consoleBufferInfo);

			auto originalAttributes = consoleBufferInfo.wAttributes;

			SetConsoleTextAttribute(consoleOutputHandle, ColorConvert(textColor, originalAttributes, FOREGROUND_RED, FOREGROUND_GREEN, FOREGROUND_BLUE, FOREGROUND_INTENSITY));
			Print(text, pos);

			SetConsoleTextAttribute(consoleOutputHandle, originalAttributes);
		}
		void Print(wstring_view text, ConsolePosition pos, ConsoleColor textColor, ConsoleColor backgrondColor) const
		{
			CONSOLE_SCREEN_BUFFER_INFO consoleBufferInfo;

			GetConsoleScreenBufferInfo(consoleOutputHandle, &consoleBufferInfo);

			auto originalAttributes = consoleBufferInfo.wAttributes;

			SetConsoleTextAttribute(consoleOutputHandle, ColorConvert(backgrondColor, originalAttributes, BACKGROUND_RED, BACKGROUND_GREEN, BACKGROUND_BLUE, BACKGROUND_INTENSITY));
			Print(text, pos, textColor);

			SetConsoleTextAttribute(consoleOutputHandle, originalAttributes);
		}
		void Print(wstring_view text, ConsoleColor textColor) const
		{
			CONSOLE_SCREEN_BUFFER_INFO consoleBufferInfo;

			GetConsoleScreenBufferInfo(consoleOutputHandle, &consoleBufferInfo);

			auto originalAttributes = consoleBufferInfo.wAttributes;

			SetConsoleTextAttribute(consoleOutputHandle, ColorConvert(textColor, originalAttributes, FOREGROUND_RED, FOREGROUND_GREEN, FOREGROUND_BLUE, FOREGROUND_INTENSITY));
			Print(text);

			SetConsoleTextAttribute(consoleOutputHandle, originalAttributes);
		}
		void Print(wstring_view text, ConsoleColor textColor, ConsoleColor backgrondColor) const
		{
			CONSOLE_SCREEN_BUFFER_INFO consoleBufferInfo;

			GetConsoleScreenBufferInfo(consoleOutputHandle, &consoleBufferInfo);

			auto originalAttributes = consoleBufferInfo.wAttributes;

			SetConsoleTextAttribute(consoleOutputHandle, ColorConvert(backgrondColor, originalAttributes, BACKGROUND_RED, BACKGROUND_GREEN, BACKGROUND_BLUE, BACKGROUND_INTENSITY));
			Print(text, textColor);

			SetConsoleTextAttribute(consoleOutputHandle, originalAttributes);
		}

		void ClearConsole() const
		{
			system("cls");
		}

		void HideCursor() const
		{
			CONSOLE_CURSOR_INFO cur;

			GetConsoleCursorInfo(consoleOutputHandle, &cur);
			cur.bVisible = false;
			SetConsoleCursorInfo(consoleOutputHandle, &cur);
		}
		void ShowCursor() const
		{
			CONSOLE_CURSOR_INFO cur;

			GetConsoleCursorInfo(consoleOutputHandle, &cur);
			cur.bVisible = true;
			SetConsoleCursorInfo(consoleOutputHandle, &cur);
		}

		void SetConsoleMode(ConsoleMode mode) const
		{
			::SetConsoleMode(consoleOutputHandle, static_cast<DWORD>(mode) >> 16);
			::SetConsoleMode(consoleInputHandle, static_cast<DWORD>(mode) & 0x0000FFFF);
		}

		[[noreturn]] void MonitorInput(InputHandler& inputHandler)
		{
#pragma push_macro("SendMessage")	// 唉, 宏定义
#undef SendMessage

			while (true)
			{
				INPUT_RECORD input;
				DWORD inputNum;

				bool sucess = ReadConsoleInput(consoleInputHandle, &input, 1, &inputNum);

				if (sucess && inputNum > 0)
				{
					switch (input.EventType)
					{
					case FOCUS_EVENT:
					case MENU_EVENT:break;
					case KEY_EVENT:
						if (input.Event.KeyEvent.bKeyDown)
						{
							InputHandler::MessageKeyboard message;

							message.key = static_cast<InputHandler::MessageKeyboard::Key>(input.Event.KeyEvent.wVirtualKeyCode);

							message.extraKeys.RawData() = input.Event.KeyEvent.dwControlKeyState;

							message.inputChar = input.Event.KeyEvent.uChar.UnicodeChar;

							inputHandler.SendMessage(message, input.Event.KeyEvent.wRepeatCount);

							input.Event.KeyEvent.wRepeatCount--;
						}
						break;
					case MOUSE_EVENT:
					{
						static bitset<2> lastButtonStatus;

						InputHandler::MessageMouse message{ .position = {	input.Event.MouseEvent.dwMousePosition.X,
																			input.Event.MouseEvent.dwMousePosition.Y} };

						message.buttonStatus = input.Event.MouseEvent.dwButtonState;

						switch (input.Event.MouseEvent.dwEventFlags)
						{
							using enum InputHandler::MessageMouse::Type;
						case 0:
							if (lastButtonStatus.count() < message.buttonStatus.count())
								message.type = Clicked;
							else
								message.type = Released;
							message.buttonStatus = lastButtonStatus xor message.buttonStatus;
							break;
						case MOUSE_MOVED:
							message.type = Moved;
							break;
						case DOUBLE_CLICK:
							message.type = DoubleClicked;
							break;
						case MOUSE_WHEELED:
							message.type = VWheeled;
							message.wheelMove = input.Event.MouseEvent.dwButtonState >> 8;
							break;
						case MOUSE_HWHEELED:
							message.type = HWheeled;
							message.wheelMove = input.Event.MouseEvent.dwButtonState >> 8;
							break;
						}

						lastButtonStatus = input.Event.MouseEvent.dwButtonState;

						inputHandler.SendMessage(message);
					}
						break;
					case WINDOW_BUFFER_SIZE_EVENT:
						inputHandler.SendMessage({ .newSize = { static_cast<unsigned short>(input.Event.WindowBufferSizeEvent.dwSize.X),
																static_cast<unsigned short>(input.Event.WindowBufferSizeEvent.dwSize.Y)} });
						break;
					default:break;
					}
				}

				//WaitForSingleObject(consoleInputHandle, INFINITE);	// 用不着等待, ReadConsoleInput 在读到足够的数据前不会返回

#pragma pop_macro("SendMessage")
			}
		}

		auto& GetRealHandler()
		{
			return consoleOutputHandle;
		}

		ConsoleHandlerWindows()
			:consoleOutputHandle(GetStdHandle(STD_OUTPUT_HANDLE)),
			consoleInputHandle(GetStdHandle(STD_INPUT_HANDLE))
		{
			if (consoleOutputHandle == INVALID_HANDLE_VALUE || consoleInputHandle == INVALID_HANDLE_VALUE)
				throw runtime_error(format("Failed to get console handle! Error code: {}.", GetLastError()));
		}
	};
}
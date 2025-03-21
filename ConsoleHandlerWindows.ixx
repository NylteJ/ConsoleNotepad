// ConsoleHandlerWindows.ixx
// Windows �µľ������̨�ı�����ʵ��
// һ��ʼ�����������ն�����ʵ�ֵģ������ǵ������ԣ�Win10 ���µ�ϵͳ���Լ���ͳ conhost ���壩�����ջ���ʹ���˴�ͳ����
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
			| ((0) << 16)
		};
	private:
		HANDLE consoleOutputHandle;
		HANDLE consoleInputHandle;
	private:
		// ����Լ����RGB ������Ϊ -1 ʱ�����ֲ���
		WORD ColorConvert(ConsoleColor color, WORD originalAttributes, WORD red, WORD green, WORD blue, WORD intensity) const
		{
			//if (color == BasicColors::stayOldColor)	// ���� IntelliSense �������, ֱ����ע�͵�����, ������ʱҲû����
				//return originalAttributes;

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

		void Print(wstring_view text) const
		{
			WriteConsoleW(consoleOutputHandle, text.data(), text.size(), nullptr, nullptr);
			//vprint_unicode(wcout,"{}", make_wformat_args(text));
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
#pragma push_macro("SendMessage")	// ��, �궨��
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
					case MOUSE_EVENT: break;
					case WINDOW_BUFFER_SIZE_EVENT:
						inputHandler.SendMessage({ .newSize = { static_cast<unsigned short>(input.Event.WindowBufferSizeEvent.dwSize.X),
																static_cast<unsigned short>(input.Event.WindowBufferSizeEvent.dwSize.Y)} });
						break;
					default:break;
					}
				}

				//WaitForSingleObject(consoleInputHandle, INFINITE);	// �ò��ŵȴ�, ReadConsoleInput �ڶ����㹻������ǰ���᷵��

#pragma pop_macro("SendMessage")
			}
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
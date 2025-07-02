// ConsoleHandlerWindows.ixx
// Windows �µľ������̨�ı�����ʵ��
// һ��ʼ�����������ն�����ʵ�ֵģ������ǵ������ԣ�Win10 ���µ�ϵͳ���Լ���ͳ conhost ���壩�����ջ���ʹ���˴�ͳ����
module;
#include <Windows.h>
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
			| ((ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT) << 16)		// TODO: ����һ���Ϊ������ ��Ĭ�ϡ� ѡ�� (����ĳ��ڳ�������ʱ�Ŀ���̨ģʽ)
		};
	private:
		HANDLE consoleOutputHandle;
		HANDLE consoleInputHandle;
	private:
		// ����Լ����RGB ������Ϊ -1 ʱ�����ֲ���, ��Ϊ -2 ʱ����ɫ, �����д�� BasicColors.ixx ����
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
			SetConsoleCursorPosition(consoleOutputHandle, { .X = static_cast<SHORT>(pos.x),.Y = static_cast<SHORT>(pos.y) });
		}

		// ����ճ���������
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
#pragma push_macro("SendMessage")	// ��, �궨��
#undef SendMessage

			constexpr size_t bufferSize = 32;

			array<INPUT_RECORD, bufferSize> inputBuffer;	// ����� buffer ������Ҫ�Ǵ�������õ�, ����ʱ�� (�����������̵�ʱ��) inputNum �������� 1
			DWORD inputNum;

			array<variant<	InputHandler::MessageKeyboard,
							InputHandler::MessageMouse,
							InputHandler::MessageWindowSizeChanged>, bufferSize * 5> messageBuffer;		// *5 ����Ϊ keyboardMessage �����ظ�, ���� 5 ��
			size_t messageCount = 0;

			while (true)
			{
				bool success = ReadConsoleInput(consoleInputHandle, inputBuffer.data(), inputBuffer.size(), &inputNum);

				if (!success)
					continue;

				for (auto&& input : inputBuffer | views::take(inputNum))
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

							while (input.Event.KeyEvent.wRepeatCount > 0)
							{
								messageBuffer[messageCount] = message;

								messageCount++;
								input.Event.KeyEvent.wRepeatCount--;
							}
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
							message.wheelMove = (input.Event.MouseEvent.dwButtonState & 0x80000000) ? 1 : -1;
							break;
						case MOUSE_HWHEELED:
							message.type = HWheeled;
							message.wheelMove = (input.Event.MouseEvent.dwButtonState & 0x80000000) ? 1 : -1;
							break;
						}

						lastButtonStatus = input.Event.MouseEvent.dwButtonState;

						if (message.wheelMove != 0 && !(input.Event.MouseEvent.dwControlKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)))
							message.wheelMove *= 3;

						messageBuffer[messageCount] = message;
						messageCount++;
					}
						break;
					case WINDOW_BUFFER_SIZE_EVENT:
						messageBuffer[messageCount] = InputHandler::MessageWindowSizeChanged
														{ .newSize = {	static_cast<unsigned short>(input.Event.WindowBufferSizeEvent.dwSize.X),
																		static_cast<unsigned short>(input.Event.WindowBufferSizeEvent.dwSize.Y)} };
						messageCount++;
						break;
					default:break;
					}
				}

				inputHandler.SendMessages(messageBuffer | views::take(messageCount));

				messageCount = 0;

				//WaitForSingleObject(consoleInputHandle, INFINITE);	// �ò��ŵȴ�, ReadConsoleInput �ڶ����㹻������ǰ���᷵��

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
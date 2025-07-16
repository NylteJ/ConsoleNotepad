// ConsoleHandlerWindows.ixx
// Windows 下的具体控制台文本操作实现
// 一开始是想用虚拟终端序列实现的，但考虑到兼容性（Win10 以下的系统，以及传统 conhost 载体），最终还是使用了传统方案
module;
#include <unicode/utf16.h>
#define NOMINMAX
#include <Windows.h>
export module ConsoleHandlerWindows;

import std;

import ConsoleTypedef;
import BasicColors;
import InputHandler;
import String;
import StringEncoder;

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
		// 特殊约定：RGB 分量都为 -1 时代表保持不变, 都为 -2 时代表反色, 具体的写在 BasicColors.ixx 里了
		constexpr WORD ColorConvert(ConsoleColor color, WORD originalAttributes, WORD red, WORD green, WORD blue, WORD intensity) const
		{
			if (color == BasicColors::stayOldColor)
				return originalAttributes;
			if (color == BasicColors::inverseColor)
				return originalAttributes xor (blue | green | red);

			WORD newAttributes = originalAttributes & (~(blue | green | red | intensity));

			const double maxRGB = max(max(color.R, color.G), color.B);

			if (maxRGB >= 0.1)
			{
				const double R = color.R / maxRGB,
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

		// 系统调用很慢(无论是打印还是改色), 足以成为性能瓶颈, 所以要尽可能减少相关调用
		constexpr void ChangeColorAnd(const ConsoleColor& textColor, auto&& oper) const
		{
			if (textColor == BasicColors::stayOldColor)
			{
				oper();
				return;
			}

			CONSOLE_SCREEN_BUFFER_INFO consoleBufferInfo;

			GetConsoleScreenBufferInfo(consoleOutputHandle, &consoleBufferInfo);

			const auto originalAttributes = consoleBufferInfo.wAttributes;

			const auto newAttributes = ColorConvert(textColor, originalAttributes, FOREGROUND_RED, FOREGROUND_GREEN, FOREGROUND_BLUE, FOREGROUND_INTENSITY);

			if (newAttributes == originalAttributes)
				oper();
			else
			{
				SetConsoleTextAttribute(consoleOutputHandle, newAttributes);
				oper();
				SetConsoleTextAttribute(consoleOutputHandle, originalAttributes);
			}
		}
		constexpr void ChangeColorAnd(const ConsoleColor& textColor, const ConsoleColor& backgroundColor, auto&& oper) const
		{
			if (textColor == BasicColors::stayOldColor && backgroundColor == BasicColors::stayOldColor)
			{
				oper();
				return;
			}

			CONSOLE_SCREEN_BUFFER_INFO consoleBufferInfo;

			GetConsoleScreenBufferInfo(consoleOutputHandle, &consoleBufferInfo);

			const auto originalAttributes = consoleBufferInfo.wAttributes;

			const auto newAttributes = ColorConvert(
				textColor,
				ColorConvert(backgroundColor, originalAttributes, BACKGROUND_RED, BACKGROUND_GREEN, BACKGROUND_BLUE, BACKGROUND_INTENSITY),
				FOREGROUND_RED, FOREGROUND_GREEN, FOREGROUND_BLUE, FOREGROUND_INTENSITY
			);

			if (newAttributes == originalAttributes)
				oper();
			else
			{
				SetConsoleTextAttribute(consoleOutputHandle, newAttributes);
				oper();
				SetConsoleTextAttribute(consoleOutputHandle, originalAttributes);
			}
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

		// 复制粘贴领域大神
		void Print(StringView text) const
		{
			const auto wText = U8StrToWStr(text.ToUTF8(), true);

			WriteConsoleW(consoleOutputHandle,
						  wText.data(),
						  static_cast<DWORD>(wText.size()),	// in_range 在 U8StrToWStr 中检查过了
						  nullptr,
						  nullptr);
		}
		void Print(StringView text, ConsolePosition pos) const
		{
			SetCursorTo(pos);
			Print(text);
		}
		void Print(StringView text, ConsolePosition pos, ConsoleColor textColor) const
		{
			ChangeColorAnd(textColor, [&] { Print(text, pos); });
		}
		void Print(StringView text, ConsolePosition pos, ConsoleColor textColor, ConsoleColor backgroundColor) const
		{
			ChangeColorAnd(textColor, backgroundColor, [&] { Print(text, pos); });
		}
		void Print(StringView text, ConsoleColor textColor) const
		{
			ChangeColorAnd(textColor, [&] { Print(text); });
		}
		void Print(StringView text, ConsoleColor textColor, ConsoleColor backgroundColor) const
		{
			ChangeColorAnd(textColor, backgroundColor, [&] { Print(text); });
		}

		void ClearConsole() const
		{
			system("cls");
		}

		void HideCursor() const
		{
			CONSOLE_CURSOR_INFO cur;

			GetConsoleCursorInfo(consoleOutputHandle, &cur);
			if (cur.bVisible)
			{
				cur.bVisible = false;
				SetConsoleCursorInfo(consoleOutputHandle, &cur);
			}
		}
		void ShowCursor() const
		{
			CONSOLE_CURSOR_INFO cur;

			GetConsoleCursorInfo(consoleOutputHandle, &cur);
			if (!cur.bVisible)
			{
				cur.bVisible = true;
				SetConsoleCursorInfo(consoleOutputHandle, &cur);
			}
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

			constexpr size_t bufferSize = 32;

			array<INPUT_RECORD, bufferSize> inputBuffer;	// 这里把 buffer 调大主要是处理滚轮用的, 其它时候 (包括脸滚键盘的时候) inputNum 几乎总是 1
			DWORD inputNum;

			// 另有一小缓冲区用于二次合并
			variant<InputHandler::MessageKeyboard, InputHandler::MessageMouse, InputHandler::MessageWindowSizeChanged> lastMessage;
			size_t lastMessageCount = 0;
			const auto sendMsgCached = [&]<typename Message>(Message&& message, size_t count = 1)
			{
				if (lastMessageCount != 0)
				{
					const auto ptr = get_if<remove_cvref_t<Message>>(&lastMessage);

					if (ptr != nullptr && *ptr == message)
					{
						lastMessageCount += count;
						return;
					}

					visit([&](auto&& msg) { inputHandler.SendMessage(msg, lastMessageCount); }, lastMessage);
				}

				lastMessage = std::forward<Message>(message);
				lastMessageCount = count;
			};

			wchar_t leadChar = 0;	// UTF-16 编码的字符可能会分两次输入, 此时其存储前半段

			while (true)
			{
				static_assert(in_range<DWORD>(inputBuffer.size()));

				bool success = ReadConsoleInput(consoleInputHandle,
												inputBuffer.data(),
												static_cast<DWORD>(inputBuffer.size()),	// 只是为了消除 Warning
												&inputNum);

				if (!success)
					continue;

				for (auto&& input : inputBuffer | views::take(inputNum))
				{
					switch (input.EventType)
					{
					case FOCUS_EVENT: [[fallthrough]];
					case MENU_EVENT:break;
					case KEY_EVENT:
						if (input.Event.KeyEvent.bKeyDown)
						{
							InputHandler::MessageKeyboard message;

							message.key = static_cast<InputHandler::MessageKeyboard::Key>(input.Event.KeyEvent.wVirtualKeyCode);

							message.extraKeys.RawData() = input.Event.KeyEvent.dwControlKeyState;

							if (UTF16_IS_SINGLE(input.Event.KeyEvent.uChar.UnicodeChar))
								message.inputChar = input.Event.KeyEvent.uChar.UnicodeChar;
							else
								if (leadChar == 0)
								{
									leadChar = input.Event.KeyEvent.uChar.UnicodeChar;
									break;	// 不发送该信息
								}
								else
								{
									message.inputChar = U16_GET_SUPPLEMENTARY(leadChar, input.Event.KeyEvent.uChar.UnicodeChar);
									leadChar = 0;
								}

							sendMsgCached(message, input.Event.KeyEvent.wRepeatCount);
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
						default: break;		// 尽管官方文档说“是以下值之一”(is one of the following values), 实际上这里的值可以是上述枚举的组合
											// 总之这么写有点问题, TODO: 后面改吧
						}

						lastButtonStatus = input.Event.MouseEvent.dwButtonState;

						if (message.wheelMove != 0 && !(input.Event.MouseEvent.dwControlKeyState & (RIGHT_CTRL_PRESSED | LEFT_CTRL_PRESSED)))
							message.wheelMove *= 3;

						sendMsgCached(message);
					}
						break;
					case WINDOW_BUFFER_SIZE_EVENT:
						inputHandler.SendMessage(InputHandler::MessageWindowSizeChanged
							{
								.newSize = {
									static_cast<ConsoleWidth>(input.Event.WindowBufferSizeEvent.dwSize.X),
									static_cast<ConsoleHeight>(input.Event.WindowBufferSizeEvent.dwSize.Y)
								}
							});		// 这个无需缓存, 直接发送即可
						break;
					default:break;
					}
				}

				if (lastMessageCount != 0)
				{
					visit([&](auto&& msg) {inputHandler.SendMessage(msg, lastMessageCount); }, lastMessage);
					lastMessageCount = 0;
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

			SetConsoleOutputCP(CP_UTF8);
			SetConsoleCP(CP_UTF8);
		}
	};
}
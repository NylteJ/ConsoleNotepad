// ConsoleHandler.ixx
// ���ڷ�װ����̨�ı������������������ڴ�ӡ����������д��
// ͬʱ��Կ���̨��һЩ��Ҫ�Ĵ���
// �ⲿ�ִ����ǿ�ƽ̨�ģ�ƽ̨��صĲ���������ģ��ʵ��
export module ConsoleHandler;

import std;

import ConsoleTypedef;
import InputHandler;

#ifdef _WIN32
import ConsoleHandlerWindows;
#else
static_assert(false, "Not implemented yet");
#endif

using namespace std;

export namespace NylteJ
{
	class ConsoleHandler
	{
	private:
#ifdef _WIN32
		ConsoleHandlerWindows handler;
#endif
	public:
		ConsoleSize GetConsoleSize() const
		{
			return handler.GetConsoleSize();
		}

		ConsolePosition GetCursorPos() const
		{
			return handler.GetCursorPos();
		}

		void SetCursorTo(ConsolePosition pos) const
		{
			handler.SetCursorTo(pos);
		}

		void Print(wstring_view text) const
		{
			handler.Print(text);
		}
		void Print(wstring_view text, ConsolePosition pos) const
		{
			handler.Print(text, pos);
		}
		void Print(wstring_view text, ConsolePosition pos, ConsoleColor textColor) const
		{
			handler.Print(text, pos, textColor);
		}
		void Print(wstring_view text, ConsolePosition pos, ConsoleColor textColor, ConsoleColor backgrondColor) const
		{
			handler.Print(text, pos, textColor, backgrondColor);
		}
		void Print(wstring_view text, ConsoleColor textColor) const
		{
			handler.Print(text, textColor);
		}
		void Print(wstring_view text, ConsoleColor textColor, ConsoleColor backgrondColor) const
		{
			handler.Print(text, textColor, backgrondColor);
		}

		void ClearConsole() const
		{
			handler.ClearConsole();
		}

		void HideCursor() const
		{
			handler.HideCursor();
		}
		void ShowCursor() const
		{
			handler.ShowCursor();
		}

		[[noreturn]] void MonitorInput(InputHandler& inputHandler)
		{
			handler.MonitorInput(inputHandler);
		}
		auto BeginMonitorInput(InputHandler& inputHandler)
		{
			thread{ bind(&ConsoleHandler::MonitorInput,this,ref(inputHandler)) } .detach();
		}

		auto& GetReadHandler()
		{
			return handler;
		}

		ConsoleHandler()
		{
			handler.SetConsoleMode(decltype(handler)::ConsoleMode::Insert);
		}

		~ConsoleHandler()
		{
			handler.SetConsoleMode(decltype(handler)::ConsoleMode::Default);
		}
	};
}
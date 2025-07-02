// ClipboardHandler.ixx
// ��ƽ̨�ļ��а�����ӿ�
export module ClipboardHandler;

import std;

#ifdef _WIN32
import ClipboardHandlerWindows;
#else
static_assert(false, "Not implemented yet");
#endif

import ConsoleHandler;

using namespace std;

export namespace NylteJ
{
	class ClipboardHandler
	{
	private:
#ifdef _WIN32
		ClipboardHandlerWindows handler;
#endif
	public:
		void Write(wstring_view source)
		{
			handler.Write(source);
		}

		wstring Read()
		{
			return handler.Read();
		}
	};
}
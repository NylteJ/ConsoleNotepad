// ClipboardHandler.ixx
// 跨平台的剪切板操作接口
export module ClipboardHandler;

import std;

import String;

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
		void Write(StringView source)
		{
            handler.Write(source);
		}

		String Read()
		{
			return handler.Read();
		}
	};
}
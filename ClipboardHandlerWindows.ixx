// ClipboardHandlerWindows.ixx
// 类似于 ConsoleHandlerWindows 和 FileHandlerWindows, 此处不再赘述
module;
#include <Windows.h>
export module ClipboardHandlerWindows;

import std;

import ConsoleHandlerWindows;

using namespace std;

export namespace NylteJ
{
	class ClipboardHandlerWindows
	{
	public:
		void Write(string_view source)
		{
			if (OpenClipboard(NULL))
			{
				EmptyClipboard();
				HGLOBAL clipbuffer;
				char* buffer;
				clipbuffer = GlobalAlloc(GMEM_DDESHARE, (source.size() + 1) * sizeof(char));
				buffer = reinterpret_cast<char*>(GlobalLock(clipbuffer));
				ranges::copy(source, buffer);
				buffer[source.size()] = '\0';
				GlobalUnlock(clipbuffer);
				SetClipboardData(CF_TEXT, clipbuffer);
				CloseClipboard();
			}
		}
		void Write(wstring_view source)
		{
			if (OpenClipboard(NULL))
			{
				EmptyClipboard();
				HGLOBAL clipbuffer;
				wchar_t* buffer;
				clipbuffer = GlobalAlloc(GMEM_DDESHARE, (source.size() + 1) * sizeof(wchar_t));
				buffer = reinterpret_cast<wchar_t*>(GlobalLock(clipbuffer));
				ranges::copy(source, buffer);
				buffer[source.size()] = L'\0';
				GlobalUnlock(clipbuffer);
				SetClipboardData(CF_UNICODETEXT, clipbuffer);
				CloseClipboard();
			}
		}
	};
}
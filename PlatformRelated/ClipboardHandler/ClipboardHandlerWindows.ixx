// ClipboardHandlerWindows.ixx
// 类似于 ConsoleHandlerWindows 和 FileHandlerWindows, 此处不再赘述
module;
#include <Windows.h>
export module ClipboardHandlerWindows;

import std;

import String;
import StringEncoder;

import ConsoleHandlerWindows;

using namespace std;

export namespace NylteJ
{
	class ClipboardHandlerWindows
	{
	public:
		void Write(wstring_view source)
		{
			if (OpenClipboard(NULL))
			{
				EmptyClipboard();
				HGLOBAL clipbuffer;
				wchar_t* buffer;
				clipbuffer = GlobalAlloc(GMEM_MOVEABLE, (source.size() + 1) * sizeof(wchar_t));
				buffer = reinterpret_cast<wchar_t*>(GlobalLock(clipbuffer));
				ranges::copy(source, buffer);
				buffer[source.size()] = L'\0';
				GlobalUnlock(clipbuffer);
				SetClipboardData(CF_UNICODETEXT, clipbuffer);
				CloseClipboard();
			}
		}
		void Write(StringView source)
		{
			Write(U8StrToWStr(source.ToUTF8()));
		}

		String Read()
		{
			wstring data;

			if (OpenClipboard(NULL))
			{
				auto handler = GetClipboardData(CF_UNICODETEXT);

				if (handler != NULL)
				{
					auto strPtr = reinterpret_cast<wchar_t*>(GlobalLock(handler));

					data = strPtr;

					GlobalUnlock(strPtr);
				}

				CloseClipboard();
			}

			return WStrToU8Str(data);
		}
	};
}
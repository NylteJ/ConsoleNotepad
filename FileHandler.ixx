// FileHandler.ixx
// 与 ConsoleHandler 类似, 跨平台的接口封装, 封装尽可能快速的文件 I/O
// 与 ConsoleHandler 不太一样的是会有更多的实际逻辑
export module FileHandler;

import std;
#ifdef _WIN32
import FileHandlerWindows;
#else
static_assert(false, "Not implemented yet");
#endif

using namespace std;

export namespace NylteJ
{
	class FileHandler
	{
	private:
#ifdef _WIN32
		FileHandlerWindows reader;
#endif
		Encoding nowEncoding;
	public:
		bool Valid() const
		{
			return reader.Valid();
		}

		void OpenFile(filesystem::path filePath)
		{
			reader.CloseFile();
			reader.OpenFile(filePath);
		}

		void CreateFile(filesystem::path filePath)
		{
			reader.CloseFile();
			reader.CreateFile(filePath);
		}

		wstring ReadAll(bool force = false) const
		{
			return reader.ReadAll(nowEncoding, force);
		}
		wstring ReadAll(Encoding encoding)
		{
			if (encoding != FORCE)
				nowEncoding = encoding;

			return ReadAll(encoding == FORCE);
		}

		void CloseFile()
		{
			reader.CloseFile();
		}

		void Write(wstring_view data)
		{
			reader.Write(data, nowEncoding);
		}
		void Write(wstring_view data, Encoding encoding)
		{
			if (encoding != FORCE)
				nowEncoding = encoding;

			Write(data);
		}

		FileHandler(filesystem::path filePath)
		{
			OpenFile(filePath);
		}
		FileHandler() {}

		~FileHandler()
		{
			CloseFile();
		}
	};
}
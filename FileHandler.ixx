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
	public:
		void OpenFile(filesystem::path filePath)
		{
			reader.CloseFile();
			reader.OpenFile(filePath);
		}

		wstring ReadAll() const
		{
			return reader.ReadAll();
		}

		void CloseFile()
		{
			reader.CloseFile();
		}

		void Write(wstring_view data)
		{
			reader.Write(data);
		}

		FileHandler(filesystem::path filePath)
		{
			OpenFile(filePath);
		}

		~FileHandler()
		{
			CloseFile();
		}
	};
}
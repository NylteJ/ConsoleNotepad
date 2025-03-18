// FileReader.ixx
// 与 ConsoleHandler 类似, 跨平台的接口封装, 封装尽可能快速的文件 I/O
export module FileReader;

import std;
#ifdef _WIN32
import FileReaderWindows;
#else
static_assert(false, "Not implemented yet");
#endif

using namespace std;

export namespace NylteJ
{
	class FileReader
	{
	private:
#ifdef _WIN32
		FileReaderWindows reader;
#endif
	public:
		void OpenFile(filesystem::path filePath)
		{
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

		FileReader(filesystem::path filePath)
		{
			OpenFile(filePath);
		}

		~FileReader()
		{
			CloseFile();
		}
	};
}
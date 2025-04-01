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
		filesystem::path nowFilePath;

		Encoding nowEncoding;

		bool isTempFile;
	public:
		bool Valid() const
		{
			return reader.Valid();
		}

		void OpenFile(filesystem::path filePath)
		{
			CloseFile();
			reader.OpenFile(filePath);
			nowFilePath = filePath;
		}

		void CreateFile(filesystem::path filePath, bool allowOverride = false)
		{
			CloseFile();
			reader.CreateFile(filePath, allowOverride);
			nowFilePath = filePath;
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

		string ReadAsBytes() const
		{
			return reader.ReadAsBytes();
		}

		void CloseFile()
		{
			reader.CloseFile();

			if (isTempFile)
				filesystem::remove(nowFilePath);

			nowFilePath.clear();
		}

		void Write(string_view bytes)
		{
			reader.Write(bytes);
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

		FileHandler(filesystem::path filePath, bool isTempFile = false)
			:isTempFile(isTempFile)
		{
			OpenFile(filePath);
		}
		FileHandler(bool isTempFile = false)
			:isTempFile(isTempFile)
		{
		}
		FileHandler(const FileHandler& right)
			:FileHandler(right.nowFilePath, right.isTempFile)
		{
		}

		~FileHandler()
		{
			CloseFile();
		}
	};
}
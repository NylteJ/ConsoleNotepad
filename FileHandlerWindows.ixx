// FileHandlerWindows.ixx
// Windows 平台的文件操作
module;
#include <Windows.h>
export module FileHandlerWindows;

import std;

import StringEncoder;
import Exceptions;

using namespace std;

export namespace NylteJ
{
	class FileHandlerWindows
	{
	private:
		HANDLE fileHandle = INVALID_HANDLE_VALUE;
	private:
		void OpenFileReal(filesystem::path filePath, DWORD mode)
		{
			SECURITY_ATTRIBUTES securityAtt;

			securityAtt.bInheritHandle = true;
			securityAtt.lpSecurityDescriptor = nullptr;
			securityAtt.nLength = sizeof(securityAtt);

			fileHandle = CreateFileW(filePath.c_str(),
				GENERIC_READ | GENERIC_WRITE,
				FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
				&securityAtt,
				mode,
				FILE_ATTRIBUTE_NORMAL,
				NULL);
		}
	public:
		bool Valid() const
		{
			return fileHandle != INVALID_HANDLE_VALUE;
		}

		void OpenFile(const filesystem::path& filePath)
		{
			OpenFileReal(filePath, OPEN_EXISTING);

			if (!Valid())		// 文件没能打开, 继续运行下去也没意义, 直接 throw
				throw FileOpenFailedException{ L"文件打开失败! 请检查文件名和访问权限!"s };
		}

#pragma push_macro("CreateFile")
#undef CreateFile
		// 实际上做的还是 OpenFile 的活, 只是会在文件不存在时创建、在文件存在时报错
		void CreateFile(filesystem::path filePath, bool allowOverride = false)
		{
			auto flag = allowOverride ? CREATE_ALWAYS : CREATE_NEW;

			OpenFileReal(filePath, flag);

			if (!Valid())
				throw FileOpenFailedException{ L"文件创建失败! 请检查文件是否已存在、路径是否合法以及有无访问权限!"s };
		}
#pragma pop_macro("CreateFile")

		wstring ReadAll(Encoding encoding, bool force = false) const
		{
			return StrToWStr(ReadAsBytes(), encoding, force);
		}

		string ReadAsBytes() const
		{
			if (!Valid())
				return ""s;

			SetFilePointer(fileHandle, 0, 0, FILE_BEGIN);

			LARGE_INTEGER fileSize;
			GetFileSizeEx(fileHandle, &fileSize);

			long long fileSizeByte = fileSize.QuadPart;

			string buffer;

			if (fileSizeByte >= 1024LL * 1024LL * 1024LL * 1024LL)	// 1TB
				throw "TODO: Large File Input";

			buffer.resize(fileSizeByte);

			DWORD temp;

			ReadFile(fileHandle, buffer.data(), buffer.size() * sizeof(buffer[0]), &temp, NULL);

			return buffer;
		}

		void CloseFile()
		{
			if (Valid())
			{
				CloseHandle(fileHandle);
				fileHandle = INVALID_HANDLE_VALUE;
			}
		}

		void Write(string_view bytes)
		{
			if (!Valid())
				return;

			SetFilePointer(fileHandle, 0, 0, FILE_BEGIN);

			DWORD temp;
			WriteFile(fileHandle, bytes.data(), bytes.size() * sizeof(bytes[0]), &temp, NULL);

			SetEndOfFile(fileHandle);	// 以上只是单纯的覆写, 如果新文件长度短于原文件长度, 后面的部分会保留, 所以要截断

			FlushFileBuffers(fileHandle);
		}
		void Write(wstring_view data, Encoding encoding)
		{
			Write(WStrToStr(data, encoding));
		}
	};
}
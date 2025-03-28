// FileHandlerWindows.ixx
// Windows 平台的文件操作
module;
#include <Windows.h>
export module FileHandlerWindows;

import std;

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

		void OpenFile(filesystem::path filePath)
		{
			OpenFileReal(filePath, OPEN_EXISTING);

			if (!Valid())		// 文件没能打开, 继续运行下去也没意义, 直接 throw
				throw L"文件打开失败! 请检查文件名和访问权限!"s;
		}

#pragma push_macro("CreateFile")
#undef CreateFile
		// 实际上做的还是 OpenFile 的活, 只是会在文件不存在时创建、在文件存在时报错
		void CreateFile(filesystem::path filePath)
		{
			OpenFileReal(filePath, CREATE_NEW);

			if (!Valid())
				throw L"文件创建失败! 请检查文件是否已存在、路径是否合法以及有无访问权限!"s;
		}
#pragma pop_macro("CreateFile")

		wstring ReadAll() const
		{
			if (!Valid())
				return L""s;

			LARGE_INTEGER fileSize;
			GetFileSizeEx(fileHandle, &fileSize);

			long long fileSizeByte = fileSize.QuadPart;

			vector<std::byte> buffer;

			if (fileSizeByte >= 1024LL * 1024LL * 1024LL)	// 1GB
				throw "TODO: Large File Input";

			buffer.resize(fileSizeByte);

			DWORD temp;

			ReadFile(fileHandle, buffer.data(), buffer.size(), &temp, NULL);

			wstring ret;

			size_t retSize = MultiByteToWideChar(CP_UTF8,
				NULL,
				reinterpret_cast<LPCCH>(buffer.data()),
				buffer.size(),
				nullptr,
				0);

			ret.resize(retSize);

			MultiByteToWideChar(CP_UTF8,
				NULL,
				reinterpret_cast<LPCCH>(buffer.data()),
				buffer.size(),
				ret.data(),
				retSize);

			return ret;
		}

		void CloseFile()
		{
			if (Valid())
			{
				CloseHandle(fileHandle);
				fileHandle = INVALID_HANDLE_VALUE;
			}
		}

		void Write(wstring_view data)
		{
			if (!Valid())
				return;

			size_t bufSize = WideCharToMultiByte(CP_UTF8,
				NULL,
				data.data(),
				data.size(),
				nullptr,
				0,
				nullptr,
				NULL);

			vector<std::byte> buffer;

			buffer.resize(bufSize);

			WideCharToMultiByte(CP_UTF8,
				NULL,
				data.data(),
				data.size(),
				reinterpret_cast<LPSTR>(buffer.data()),
				buffer.size(),
				nullptr,
				NULL);

			SetFilePointer(fileHandle, 0, 0, FILE_BEGIN);

			DWORD temp;
			WriteFile(fileHandle, buffer.data(), buffer.size(), &temp, NULL);

			SetEndOfFile(fileHandle);	// 以上只是单纯的覆写, 如果新文件长度短于原文件长度, 后面的部分会保留, 所以要截断

			FlushFileBuffers(fileHandle);
		}
	};
}
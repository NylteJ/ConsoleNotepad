// FileHandlerWindows.ixx
// Windows ƽ̨���ļ�����
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

			if (!Valid())		// �ļ�û�ܴ�, ����������ȥҲû����, ֱ�� throw
				throw L"�ļ���ʧ��! �����ļ����ͷ���Ȩ��!"s;
		}

#pragma push_macro("CreateFile")
#undef CreateFile
		// ʵ�������Ļ��� OpenFile �Ļ�, ֻ�ǻ����ļ�������ʱ���������ļ�����ʱ����
		void CreateFile(filesystem::path filePath)
		{
			OpenFileReal(filePath, CREATE_NEW);

			if (!Valid())
				throw L"�ļ�����ʧ��! �����ļ��Ƿ��Ѵ��ڡ�·���Ƿ�Ϸ��Լ����޷���Ȩ��!"s;
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

			SetEndOfFile(fileHandle);	// ����ֻ�ǵ����ĸ�д, ������ļ����ȶ���ԭ�ļ�����, ����Ĳ��ֻᱣ��, ����Ҫ�ض�

			FlushFileBuffers(fileHandle);
		}
	};
}
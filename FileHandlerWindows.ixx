// FileHandlerWindows.ixx
// Windows ƽ̨���ļ�����
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

		void OpenFile(filesystem::path filePath)
		{
			OpenFileReal(filePath, OPEN_EXISTING);

			if (!Valid())		// �ļ�û�ܴ�, ����������ȥҲû����, ֱ�� throw
				throw FileOpenFailedException{ L"�ļ���ʧ��! �����ļ����ͷ���Ȩ��!"s };
		}

#pragma push_macro("CreateFile")
#undef CreateFile
		// ʵ�������Ļ��� OpenFile �Ļ�, ֻ�ǻ����ļ�������ʱ���������ļ�����ʱ����
		void CreateFile(filesystem::path filePath)
		{
			OpenFileReal(filePath, CREATE_NEW);

			if (!Valid())
				throw FileOpenFailedException{ L"�ļ�����ʧ��! �����ļ��Ƿ��Ѵ��ڡ�·���Ƿ�Ϸ��Լ����޷���Ȩ��!"s };
		}
#pragma pop_macro("CreateFile")

		wstring ReadAll(Encoding encoding, bool force = false) const
		{
			if (!Valid())
				return L""s;

			LARGE_INTEGER fileSize;
			GetFileSizeEx(fileHandle, &fileSize);

			long long fileSizeByte = fileSize.QuadPart;

			string buffer;

			if (fileSizeByte >= 1024LL * 1024LL * 1024LL)	// 1GB
				throw "TODO: Large File Input";

			buffer.resize(fileSizeByte);

			DWORD temp;

			ReadFile(fileHandle, buffer.data(), buffer.size() * sizeof(buffer[0]), &temp, NULL);

			return StrToWStr(buffer, encoding, force);
		}

		void CloseFile()
		{
			if (Valid())
			{
				CloseHandle(fileHandle);
				fileHandle = INVALID_HANDLE_VALUE;
			}
		}

		void Write(wstring_view data, Encoding encoding)
		{
			if (!Valid())
				return;

			SetFilePointer(fileHandle, 0, 0, FILE_BEGIN);

			string buffer = WStrToStr(data, encoding);

			DWORD temp;
			WriteFile(fileHandle, buffer.data(), buffer.size()*sizeof(buffer[0]), &temp, NULL);

			SetEndOfFile(fileHandle);	// ����ֻ�ǵ����ĸ�д, ������ļ����ȶ���ԭ�ļ�����, ����Ĳ��ֻᱣ��, ����Ҫ�ض�

			FlushFileBuffers(fileHandle);
		}
	};
}
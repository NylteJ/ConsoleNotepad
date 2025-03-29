// Exceptions.ixx
// ��װ��ʹ�� wstring �ļ����쳣��
export module Exceptions;

import std;

using namespace std;

export namespace NylteJ
{
	class Exception
	{
	protected:
		wstring str;
	public:
		virtual wstring_view What() const
		{
			return str;
		}
		virtual wstring& What()
		{
			return str;
		}

		operator wstring_view() const
		{
			return What();
		}
		operator wstring&()
		{
			return What();
		}

		Exception(auto&& str)
			:str(str)
		{
		}
	};

#define ExceptionGener(name) class name :public Exception { public: name(auto&& str) :Exception(str) {} };

	ExceptionGener(FileOpenFailedException)

	ExceptionGener(WrongEncodingException)
}
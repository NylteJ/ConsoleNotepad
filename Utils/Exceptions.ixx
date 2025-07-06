// Exceptions.ixx
// 封装了使用 String 的几个异常类
export module Exceptions;

import std;

import String;

using namespace std;

export namespace NylteJ
{
	class Exception
	{
	protected:
		String str;
	public:
		virtual StringView What() const
		{
			return str;
		}

		operator StringView() const
		{
			return What();
		}

		template<convertible_to<String> Str>
		Exception(Str&& str)
			:str(std::forward<Str>(str))
		{
		}

		virtual ~Exception() = default;
	};

#define ExceptionGener(name) class name final :public Exception { public: template<convertible_to<String> Str>name(Str&& str) :Exception(std::forward<Str>(str)) {} };

	ExceptionGener(FileOpenFailedException)

	ExceptionGener(WrongEncodingException)

#undef ExceptionGener
}
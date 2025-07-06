// ConsoleArgumentsAnalyzer.ixx
// 用来解析命令行参数
export module ConsoleArgumentsAnalyzer;

import std;

import StringEncoder;

using namespace std;

export namespace NylteJ
{
	class ConsoleArguments
	{
	private:
		vector<string_view> arguments;
		string_view exeName;
	public:
		filesystem::path ExePath() const
		{
			return exeName;
		}
		optional<string_view> OpenFilePath() const
		{
			if (arguments.size() == 1)
				return arguments[0];
			if (arguments.size() >= 2)
			{
				auto iter = ranges::find(arguments, "-o"sv);
				if (arguments.end() - iter >= 2)
					return *(iter + 1);

				iter = ranges::find(arguments, "--open"sv);
				if (arguments.end() - iter >= 2)
					return *(iter + 1);
			}
			return {};
		}
		optional<Encoding> GetEncoding() const
		{
			static auto getEncodingByStr = [](string_view str)->optional<Encoding>
				{
					auto normalizedStr = str
						| views::transform([](auto&& chr)->char
							{
								if (chr >= 'a' && chr <= 'z')
									return chr - 'a' + 'A';
								return chr;
							})
						| views::filter([](auto&& chr) {return chr != '-' && chr != ' '; })
						| ranges::to<string>();

					using enum Encoding;

					if (normalizedStr == "UTF8"sv)
						return UTF8;
					if (normalizedStr == "GB2312"sv)
						return GB2312;
					return {};
				};

			if (arguments.size() >= 2)
			{
				auto iter = ranges::find(arguments, "-e"sv);
				if (arguments.end() - iter >= 2)
					return getEncodingByStr(*(iter + 1));

				iter = ranges::find(arguments, "--encoding"sv);
				if (arguments.end() - iter >= 2)
					return getEncodingByStr(*(iter + 1));
			}

			return {};
		}
		bool HelpMode() const
		{
			if (ranges::contains(arguments, "-h"sv)
				|| ranges::contains(arguments, "--help"sv)
				|| ranges::contains(arguments, "/?"sv))
				return true;
			return false;
		}

		ConsoleArguments(int argc, char** argv)
		{
			arguments.reserve(argc);

			if (argc > 0)
				exeName = argv[0];

			for (int i = 1; i < argc; i++)
				arguments.emplace_back(argv[i]);
			// 理论上 argv 在 main 函数结束前都会一直保持有效, 所以用 view 存也没什么问题, 大概
		}
	};
}
// String.ixx
// 封装字符串操作
// (为什么 C++ 项目都喜欢实现自己的 String 呢? 好难猜啊)
// 为了不和 Exception 产生循环依赖, String 类只抛出 STL 异常(目前的实现里主要是 runtime_error)
module;
#include <unicode/ucsdet.h>
#include <unicode/ucnv.h>
#include <unicode/utf8.h>
#include <unicode/utext.h>
#include <unicode/brkiter.h>
export module String;

import std;

using namespace std;
using namespace icu;

namespace NylteJ
{
	export using Codepoint = char32_t;

	export class StringView;

	class MatchAllASCIICaseFuncClass {};

	//TODO: 整合一部分 String 和 StringView 共用的代码

	// 字符串
	// 任何读取操作都不会使迭代器失效, 任何写入操作都可能使所有迭代器失效
	// 需要使用特殊的 ByteSize, ByteIndex 等来获取可用于随机访问的下标, 该下标和普通 string 的下标类似, 但只在其前面元素没有任何改变时才能保证有效
	// 返回值是 4 字节 Unicode 码点 (UCS-4)
	export class String
	{
		friend class StringView;

		// 内部使用 UTF-8 存储
		// (作为相当程度上的代码编辑器, 有必要为代码做特化, 加上 UTF-16 也是变长, 不如直接用 UTF-8)

	public:
		constexpr static MatchAllASCIICaseFuncClass matchAllASCIICase;	// 用于 search 算法特化的占位符, 能优化性能
	public:
		class Iterator
		{
			friend class String;
			friend class StringView;

			// 按 C++ STL 的设计哲学, 算法和数据结构应当分离
			// 所以这里选择新建非成员 search 函数, 通过 ADL 查找到此特化(但只能写成 Iterator 的 friend, 看起来有点奇怪...)
			// 实际上就用 STL 里的版本也是没问题的(按码点匹配), 但这种按字节匹配的算法效率高得多(可以做很多优化, 码点因为对应关系不明确基本只能老实匹配)

			// 对 String / StringView 的搜索特化
			// 不带谓词的版本效率可能更高
			friend constexpr Iterator search(Iterator haystackBegin,
											 Iterator hayStackEnd,
											 Iterator needleBegin,
											 Iterator needleEnd)
			{
				// 同样, 因 UTF-8 特性, 逐字节查找即可
				// TODO: benchmark 以找寻更好的分界点
				if (needleEnd.str - needleBegin.str <= 16)
					return Iterator{ std::search(haystackBegin.str,
												hayStackEnd.str,
												needleBegin.str,
												needleEnd.str) };
				else
				{
					const boyer_moore_horspool_searcher searcher{ needleBegin.str, needleEnd.str };

					return Iterator{ std::search(haystackBegin.str, hayStackEnd.str, searcher) };
				}
			}

			template<typename Func>
			friend constexpr Iterator search(Iterator haystackBegin,
											 Iterator hayStackEnd,
											 Iterator needleBegin,
											 Iterator needleEnd,
											 Func&& equalPred)
			{
				if constexpr (is_same_v<remove_cvref_t<Func>, MatchAllASCIICaseFuncClass>)
				{
					// ASCII 字符串的模糊大小写搜索有更快的解决方案(应该更快, 大概)
					constexpr static auto pred = [](auto left, auto right)
						{
							if (left >= 'a' && left <= 'z')
								left = left - 'a' + 'A';
							if (right >= 'a' && right <= 'z')
								right = right - 'a' + 'A';
							return left == right;
						};

					// TODO: 把这一段和上面那一段合并
					if (needleEnd.str - needleBegin.str <= 16)
						return Iterator{ std::search(haystackBegin.str,
													hayStackEnd.str,
													needleBegin.str,
													needleEnd.str,
													 pred) };
					else
					{
						const boyer_moore_horspool_searcher searcher{ needleBegin.str, needleEnd.str, hash<char8_t>{}, pred };

						return Iterator{ std::search(haystackBegin.str, hayStackEnd.str, searcher) };
					}
				}
				else
				{
					// BMH / BM 算法要求随机访问迭代器, 这里只能使用原始算法搜索
					return Iterator{ std::search(haystackBegin.str,
												 hayStackEnd.str,
												 needleBegin.str,
												 needleEnd.str,
												 forward<Func>(equalPred)) };
				}
			}

			friend constexpr Iterator find(Iterator strBegin,
										   Iterator strEnd,
										   char8_t chr)
			{
				return Iterator{ std::find(strBegin.str,
								 strEnd.str,
								 chr) };
			}

			friend constexpr auto count(Iterator strBegin,
										Iterator strEnd,
										char8_t chr)
			{
				return std::count(strBegin.str, strEnd.str, chr);
			}

		public:
			// STL 兼容
			using difference_type = std::ptrdiff_t;
			using value_type = Codepoint;
		private:
			const char8_t* str;
		public:
			constexpr auto&& operator++()
			{
				Codepoint temp = 0;

				U8_FWD_1_UNSAFE(str, temp);
				str += temp;

				return *this;
			}
			constexpr auto operator++(int)
			{
				auto ret = *this;
				++*this;
				return ret;
			}

			constexpr auto&& operator--()
			{
				// 负数下标是被允许的(E1[E2] 与表达式 *(E1 + E2) 在除值类别和求值顺序外的部分完全相同)
				int32_t temp = 0;

				U8_BACK_1_UNSAFE(str, temp);
				str += temp;

				return *this;
			}
			constexpr auto operator--(int)
			{
				auto ret = *this;
				--*this;
				return ret;
			}

			constexpr Codepoint operator*() const
			{
				Codepoint ret;

				U8_GET_UNSAFE(str, 0, ret);

				return ret;
			}

			constexpr auto operator<=>(const Iterator&) const = default;
			constexpr bool operator==(const Iterator&) const = default;

			constexpr explicit Iterator(const char8_t* str) noexcept
				: str(str)
			{}

			// STL 要求迭代器可默认构造
			constexpr explicit Iterator() noexcept
				: str(nullptr)
			{}
		};
	private:
		u8string data;
	public:
		constexpr Iterator begin() const { return Iterator{ data.data() }; }
		constexpr Iterator cbegin() const { return begin(); }
		constexpr Iterator end() const { return Iterator{ data.data() + data.size() }; }
		constexpr Iterator cend() const { return end(); }

		constexpr Codepoint front() const { return *begin(); }
		constexpr Codepoint back() const { return *prev(end()); }

		constexpr bool empty() const { return begin() == end(); }

		constexpr bool contains(char8_t chr) const
		{
			// 由于 UTF-8 的特性, 单字符时逐字节查找即可
			return data.contains(chr);
		}

		constexpr bool starts_with(u8string_view str) const { return data.starts_with(str); }
		constexpr bool starts_with(StringView str) const;
		constexpr bool ends_with(u8string_view str) const { return data.ends_with(str); }
		constexpr bool ends_with(StringView str) const;

		constexpr void clear() { data.clear(); }

		template<ranges::input_range Range>
		constexpr void replace_with_range(Iterator begin, Iterator end, Range&& range)
		{
			const auto beginIndex = GetByteIndex(begin);
			const auto endIndex = GetByteIndex(end);

			if constexpr (!is_same_v<remove_cvref_t<Range>, String> && !is_same_v<remove_cvref_t<Range>, StringView>)
				data.replace_with_range(data.begin() + beginIndex,
										data.begin() + endIndex,
										forward<Range>(range));
			else
				data.replace_with_range(data.begin() + beginIndex,
										data.begin() + endIndex,
										forward_like<Range>(range.data));
		}

		template<ranges::input_range Range>
		constexpr void insert_range(Iterator where, Range&& range)
		{
			if constexpr (!is_same_v<remove_cvref_t<Range>, String> && !is_same_v<remove_cvref_t<Range>, StringView>)
				data.insert_range(data.begin() + GetByteIndex(where), forward<Range>(range));
			else
				data.insert_range(data.begin() + GetByteIndex(where), forward_like<Range>(range.data));
		}

		template<ranges::input_range Range>
		constexpr void append_range(Range&& range)
		{
            if constexpr (!is_same_v<remove_cvref_t<Range>, String> && !is_same_v<remove_cvref_t<Range>, StringView>)
                data.append_range(forward<Range>(range));
            else
                data.append_range(forward_like<Range>(range.data));
		}

		void erase(Iterator begin, Iterator end)
		{
			const auto beginIndex = GetByteIndex(begin);
			const auto endIndex = GetByteIndex(end);

			data.erase(data.begin() + beginIndex,
					   data.begin() + endIndex);
		}
		constexpr void erase(Iterator where)
		{
			const auto length = U8_LENGTH(*where);

			data.erase(data.begin() + GetByteIndex(where), data.begin() + GetByteIndex(where) + length);
		}

		constexpr void pop_back()
		{
			data.resize(GetByteIndex(prev(end())));
		}

		// 逻辑上的 size(), 表示字符数
		// 时间复杂度为 O(N)
		constexpr size_t Size() const
		{
			return distance(begin(), end());
		}

		constexpr size_t ByteSize() const { return data.size(); }
		constexpr Iterator AtByteIndex(size_t index) const
		{
			return Iterator{ data.data() + index };
		}

		size_t GetByteIndex(Iterator iter) const
		{
			return static_cast<size_t>(iter.str - data.data());
		}

		constexpr auto ToUTF8() const { return data; }

		template<typename... Args>
        static constexpr String Format(format_string<Args...> fmt, Args&&... args)
		{
			// 因为 std::format 对自定义字符串类型的支持极烂(而标准甚至不支持 u8string), 这里我们只能用 string 替代了
			// 此函数中的 string 均为 UTF-8 编码, 和 u8string 等价
            // TODO: 对其它两家编译器的 string 编码调整 (MSVC 只要设置 /utf-8 就能强制以 UTF-8 编译, 字符串字面量都会按 UTF-8 编码)

			String ret;

			// TODO: 减少 Copy
			ret.data = reinterpret_cast<const char8_t*>(std::format(std::move(fmt), forward<Args>(args)...).data());	// TODO: 可能需要手动指定 locale?

			return ret;
		}

		template<ranges::input_range Range>
		constexpr auto&& operator+=(Range&& range)
		{
			append_range(forward<Range>(range));
			return *this;
		}

		template<ranges::input_range Range>
		constexpr String operator+(Range&& range)
		{
			return { data + forward<Range>(range) };
		}
		template<ranges::input_range Range>
			requires(!is_same_v<remove_cvref_t<Range>, String>)
		friend constexpr String operator+(Range&& left, const String& right)
		{
			return { forward<Range>(left) + right.data };
		}

		constexpr auto operator<=>(const String&) const = default;
		constexpr bool operator==(const String&) const = default;

		operator filesystem::path() const { return data; }

		String(char8_t chr, size_t count)
			: data(count, chr)
		{}

		template<ranges::input_range Range>
			requires is_same_v<ranges::range_value_t<Range>, Codepoint> && ranges::sized_range<Range>
		String(from_range_t, Range&& codepoints)
		{
			data.resize(codepoints.size() * U8_MAX_LENGTH);
			const auto dataPtr = data.data();	// 一定不会发生内存重分配, 可以缓存 data.data() (编译器缺少信息, 做不了这个优化)

			size_t index = 0;
			for (auto&& codepoint : codepoints)
				U8_APPEND_UNSAFE(dataPtr, index, codepoint);

			data.resize(index);
			data.shrink_to_fit();
		}
		// 源范围不可简单估计大小时退化到低效方案 (尽管内存意义上其实是高效方案)
		template<ranges::input_range Range>
			requires is_same_v<ranges::range_value_t<Range>, Codepoint> && !ranges::sized_range<Range>
		String(from_range_t, Range&& codepoints)
		{
			size_t index = 0;
			for (auto&& codepoint : codepoints)
			{
				const auto length = U8_LENGTH(codepoint);

				data.resize(data.size() + length);

				U8_APPEND_UNSAFE(data.data(), index, codepoint);
			}
		}

		explicit String(initializer_list<Codepoint> codepoints)
			:String(from_range_t{}, codepoints) {}

		constexpr String(const StringView& src);

		// 自动转换编码
		// (TODO: 纯 ASCII 码时有 bug, 会识别成 UTF-16 导致乱码)
		explicit String(string_view str)
		{
			static_assert(sizeof(char) == sizeof(char8_t)); // 正常平台应该都满足

			thread_local const LocalUConverterPointer utf8Converter = []
			{
				UErrorCode status = U_ZERO_ERROR;
				LocalUConverterPointer ret{ ucnv_open("UTF-8", &status) };

				if (!U_SUCCESS(status))
					throw runtime_error{ "Failed to open UTF-8 converter!" };

				return ret;
			}();

			UErrorCode status = U_ZERO_ERROR;

			const LocalUCharsetDetectorPointer detector{ ucsdet_open(&status) };

			if (U_FAILURE(status))
				throw runtime_error{ "Failed to open detector!" };

			auto srcData = str.data();

			ucsdet_setText(detector.getAlias(),
						   srcData,
						   static_cast<int32_t>(str.size()),
						   &status);

			const auto match = ucsdet_detect(detector.getAlias(), &status);

			const auto encodingName = ucsdet_getName(match, &status);

			if (U_FAILURE(status))
				throw runtime_error{ "Failed to detect encoding!" };

			if (strcmp(encodingName, "UTF-8") == 0)
			{
				data = u8string{ reinterpret_cast<const char8_t*>(str.data()), str.size() };
				return;
			}

			data.resize(str.size() * U8_MAX_LENGTH);

			const LocalUConverterPointer sourceConverter{ ucnv_open(encodingName, &status) };

			auto dstData = reinterpret_cast<char*>(data.data());

			ucnv_convertEx(
				utf8Converter.getAlias(), sourceConverter.getAlias(),
				&dstData, dstData + data.size(),
				&srcData, srcData + str.size(),
				nullptr, nullptr,
				nullptr, nullptr,
				true, true,
				&status
			);

			if (U_FAILURE(status))
				throw runtime_error{ "Convert failed!" };

			data.resize(reinterpret_cast<char8_t*>(dstData) - data.data());
			data.shrink_to_fit();
		}
		explicit String(const string& str)
			:String(string_view{ str }) {}

		template<ranges::input_range U8String>
			requires is_same_v<ranges::range_value_t<U8String>, char8_t>
		constexpr String(U8String&& str)
			: data(ranges::begin(str), ranges::end(str))
		{}

		constexpr String(const filesystem::path& src)
			: String(src.u8string())
		{}

		constexpr String(Iterator begin, Iterator end)
			: data(begin.str, static_cast<size_t>(end.str - begin.str))
		{}

		constexpr String() = default;
	};

	class StringView
	{
		friend class String;

	private:
		u8string_view data;
	public:
		constexpr String::Iterator begin() const { return String::Iterator{ data.data() }; }
		constexpr String::Iterator cbegin() const { return begin(); }
		constexpr String::Iterator end() const { return String::Iterator{ data.data() + data.size() }; }
		constexpr String::Iterator cend() const { return end(); }

		constexpr Codepoint front() const { return *begin(); }
		constexpr Codepoint back() const { return *prev(end()); }

		constexpr bool empty() const { return begin() == end(); }

		constexpr bool contains(char8_t chr) const
		{
			// 由于 UTF-8 的特性, 单字符时逐字节查找即可
			return data.contains(chr);
		}

		constexpr bool starts_with(u8string_view str) const { return data.starts_with(str); }
		constexpr bool starts_with(StringView str) const { return data.starts_with(str.data); }
		constexpr bool ends_with(u8string_view str) const { return data.ends_with(str); }
		constexpr bool ends_with(StringView str) const { return data.ends_with(str.data); }

		// 逻辑上的 size(), 表示字符数
		// 时间复杂度为 O(N)
		constexpr size_t Size() const
		{
			return distance(begin(), end());
		}

		constexpr size_t ByteSize() const { return data.size(); }
		constexpr String::Iterator AtByteIndex(size_t index) const
		{
			return String::Iterator{ data.data() + index };
		}

		// 根据迭代器获取可用于随机访问的下标
		// 首迭代器的该下标永远是 0
		size_t GetByteIndex(String::Iterator iter) const
		{
			return static_cast<size_t>(iter.str - data.data());
		}

		constexpr auto ToUTF8() const { return data; }

		constexpr auto operator<=>(const StringView&) const = default;
		constexpr bool operator==(const StringView&) const = default;

		operator filesystem::path() const { return data; }

		constexpr StringView(u8string_view str)
			: data(str.begin(), str.end())
		{}
        constexpr StringView(const u8string& str)
            : data(str.begin(), str.end())
        {}

		constexpr StringView(const String& str)
			: data(str.data)
		{}

		constexpr StringView(String::Iterator begin, String::Iterator end)
			: data(begin.str, static_cast<size_t>(end.str - begin.str))
		{}

		constexpr StringView() = default;
	};

	constexpr bool String::starts_with(StringView str) const { return data.starts_with(str.data); }
	constexpr bool String::ends_with(StringView str) const { return data.ends_with(str.data); }

	constexpr String::String(const StringView& src)
		: data(src.data)
	{}

	export auto GetWord(StringView str, String::Iterator where)
	{
		// 这两个变量都可以复用
		// 但都不是线程安全的, 所以声明为 thread_local 而非 static
		thread_local UText text = UTEXT_INITIALIZER;	// ICU 的 UText 相当智能, 不会复制底层字符串, 而是类似 ranges::view 地提供上层适配, 这样我们就能愉快地以 UTF-8 的底层表示无缝对接 ICU 的许多 API 了
		thread_local const unique_ptr<BreakIterator> iter = []
		{
			UErrorCode status = U_ZERO_ERROR;
			unique_ptr<BreakIterator> ret{ BreakIterator::createWordInstance(Locale::getDefault(), status) };

			if (!U_SUCCESS(status))
				throw runtime_error{ "Failed to create BreakIterator" };

			return ret;
		}();

		UErrorCode status = U_ZERO_ERROR;

		utext_openUTF8(&text,
					   reinterpret_cast<const char*>(str.ToUTF8().data()),
					   str.ByteSize(),
					   &status);

		if (!U_SUCCESS(status))
			throw runtime_error{ "Failed to open UText" };

		iter->setText(&text, status);

		if (!U_SUCCESS(status))
			throw runtime_error{ "Failed to set UText in BreakIterator" };

		auto byteIndex = str.GetByteIndex(where);
		if (byteIndex != str.ByteSize())
			byteIndex++;
		auto begin = iter->preceding(byteIndex);
		if (begin == BreakIterator::DONE)
			begin = 0;
		auto end = iter->next();
		if (end == BreakIterator::DONE)
			end = str.ByteSize();

		return ranges::subrange{ str.AtByteIndex(begin), str.AtByteIndex(end) };
	}
}

// STL 兼容
export template<>
constexpr bool std::ranges::enable_borrowed_range<NylteJ::StringView> = true;

// format 支持(应当和 String::Format 而不是 std::format 配合使用, 这里这么写是无奈之举)
export template<>
struct std::formatter<NylteJ::StringView, char> : std::formatter<std::string_view, char>
{
	template<class FmtContext>
	typename FmtContext::iterator format(const NylteJ::StringView& s, FmtContext& ctx) const
	{
		return std::formatter<std::string_view, char>::format(reinterpret_cast<const char*>(s.ToUTF8().data()), ctx);
	}
};
export template<>
struct std::formatter<NylteJ::String, char> : std::formatter<NylteJ::StringView, char>
{};


// hash 支持
export template<>
struct std::hash<NylteJ::StringView>
{
	static size_t operator()(const NylteJ::StringView& key) noexcept
	{
		return hash<u8string_view>{}(key.ToUTF8());
	}
};
export template<>
struct std::hash<NylteJ::String> : std::hash<NylteJ::StringView>
{};
// Utils.ixx
// 一些杂七杂八的函数
module;
#include <unicode/uchar.h>
export module Utils;

import std;

import String;
import Exceptions;

using namespace std;

export namespace NylteJ
{
	// 判断一个字符是否是两倍宽度的
	constexpr bool IsWideChar(Codepoint codepoint)
	{
		// ASCII 特判
        if (codepoint <= 0x7f)
            return false;

		const auto width = u_getIntPropertyValue(codepoint, UCHAR_EAST_ASIAN_WIDTH);
		return width == U_EA_FULLWIDTH || width == U_EA_WIDE;
	}

	// 获取一个单行且不含 Tab 的字符串的显示长度
	constexpr size_t GetDisplayLength(StringView str)
	{
		size_t ret = 0;

		for (auto&& codepoint : str)
			if (IsWideChar(codepoint))
				ret += 2;
			else
				ret++;

		return ret;
	}

	constexpr bool IsASCIIAlpha(integral auto&& chr)
	{
		return (chr >= 'A' && chr <= 'Z') || (chr >= 'a' && chr <= 'z');
	}

	template<typename ElementType, typename... Args>
	constexpr array<ElementType, sizeof...(Args)> MakeArray(Args&&... args)
	{
		return { ElementType{ forward<Args>(args) }... };
	}
	template<typename ElementType, typename... Args>
	constexpr vector<ElementType> MakeVector(Args&&... args)
	{
		return { ElementType{ forward<Args>(args) }... };
	}

	// 对枚举值较小的枚举类型使用, 替代 unordered_map, 一般能拥有更好的性能(甚至更低的空间占用)
	// 理论存取效率和裸数组相当, 为替代 unordered_map 会额外花费少许空间用于存储某一位置是否被写入
	// 使用方式基本完全等同于 unordered_map
	// TODO: 迭代器等
	template<typename EnumType, typename DataType, size_t maxEnum = static_cast<size_t>(EnumType::_MaxEnum)>
		requires is_enum_v<EnumType>
	class EnumMap
	{
	private:
		array<DataType, maxEnum + 1> data;
		bitset<maxEnum + 1> written;	// 没被记录过的等同于 unordered_map 的空位置
	public:
		template<typename Self>
			requires !is_const_v<remove_reference_t<Self>>
		constexpr auto&& operator[](this Self&& self, EnumType e)
		{
			forward_like<Self>(self.written[static_cast<size_t>(e)]) = true;
			return forward_like<Self>(self.data[static_cast<size_t>(e)]);
		}
		template<typename Self>
		constexpr auto&& at(this Self&& self, EnumType e)
		{
			if (static_cast<size_t>(e) > maxEnum)
				throw out_of_range{ "EnumMap: Enum value out of range!" };
			if constexpr (is_const_v<remove_reference_t<Self>>)
				if (!self.written[static_cast<size_t>(e)])
					throw out_of_range{ "EnumMap: Invalid!" };

			return forward_like<Self>(self.data[static_cast<size_t>(e)]);
		}

		constexpr bool contains(EnumType e) const
		{
			if (static_cast<size_t>(e) > maxEnum)
				return false;
			return written[static_cast<size_t>(e)];
		}

		constexpr bool empty() const { return written.none(); }

		constexpr void clear()
		{
			data.fill(DataType{});
			written.reset();
		}
	};
}

namespace NylteJ
{
	// Kotlin 用多了导致的
	class TODO_T
	{
	public:
		template<typename T>
		[[noreturn]]
		constexpr operator T() const
		{
			throw Exception{ String::Format("功能(获取一个 {} 类型的值)暂未实现!", typeid(T).name()) };
		}

		[[noreturn]]
		const TODO_T& operator()(const source_location & location = source_location::current()) const
		{
			throw Exception{ String::Format("功能(位于文件 {} 中的函数 {}, 第 {} 行)暂未实现!",
											location.file_name(),
											location.function_name(),
											location.line()) };
		}

		[[noreturn]]
		TODO_T(const TODO_T&)		// 这样 auto xxx = ... 的情况也能处理
		{
			throw Exception{ u8"功能(获取一个 auto 类型的值)暂未实现!" };		// 应该只可能是 auto 或者引用 / cv 变体, 因为这个类型本身不对外导出
		}
		[[noreturn]]
		TODO_T(TODO_T&&) noexcept(false)
		{
			throw Exception{ u8"功能暂未实现!" };		// 理论上只有可能是先 auto 复制了一份, 再移动那一份......好闲啊
		}
		constexpr TODO_T() = default;
	};

	// 类似 Kotlin 中的版本
	// 可以调用或是隐式转换成任何类型(或是调用然后将返回值转换成任何类型), 总是抛出异常表明该功能尚未实现
	export inline constexpr TODO_T TODO;
}

// Formatter.ixx
// 用于原始字符串的格式化与双向定位
// 简单来说, 就是负责将文件里读的字符串和屏幕上显示的位置一一对应的工具, 缩进的显示方式、自动换行等都是据此实现的
// 原则上, 只有 Formatter 知道这一对应关系, 其它组件应当无法获取(因此不能施加任何假设, 只能通过 Formatter 获取相关信息)

// 在 Debug 下容易莫名其妙报“间接呼叫临界检查检测到无效的控制传输”(非 Debug 模式下貌似不会), 很迷, 可能是 MSVC 的神必 bug (加一个默认实现的析构函数可以略微缓解, 但不能根治, 屋檐了)
export module Formatter;

import std;

import ConsoleTypedef;
import Utils;
import SettingMap;
import String;
import StringEncoder;
import BasicColors;

using namespace std;

export namespace NylteJ
{
	// 在原始字符串中定位的方式: 下标, 因为我们的光标位于两个字符之间, 所以光标的下标实际上是光标右侧字符的下标 (光标位于末尾时下标就是字符串长度)
	//						   在一行末尾时, 光标在换行符(无论是什么)的前面; 在另一行开头时, 则在上一行换行符(如果有)的后面
	// 在格式化字符串中定位的方式: (x, y) 坐标, 和下标类似, 在 (x, y) 的光标位于 (x, y) 字符的前面

	// 格式化后的字符串, Formatter 的返回值
	// 只保留必要的内容(包括行尾额外空格的数量)
	// 其值应当可以直接被逐行打印到指定位置, 无需做任何额外转换
	class FormattedString
	{
	public:
		class Line
		{
		public:
			String str;
			size_t extraSpaceCount;
		};
	public:
		vector<Line> data;

		auto&& operator[](size_t index) { return data[index]; }
	};

	class ColorfulFormattedString
	{
	public:
		class Data
		{
		public:
			String data;
			ConsoleColor textColor;
			ConsoleColor backgroundColor;
		};

		class Line
		{
		public:
			vector<Data> data;
			size_t extraSpaceCount;
		};
	public:
		vector<Line> data;
	};

	// 各类 Formatter 的基类
	// Formatter 可以有状态, 而且每个 Formatter 应当只对应一个字符串(以允许一些缓存优化, 尽管对应多个字符串也不会带来问题)
	// 表现得好像格式化并缓存了这个字符串的所有内容, 在调用时返回请求的部分, 并在适当地时候更新(需要手动标记字符串的某些位置发生了改变)
	class Formatter
	{
	public:
		// 将控制台上的位置转化为原始字符串的下标
		virtual size_t ConsolePositionToIndex(ConsolePosition pos) = 0;
		// 将原始字符串的下标转化为控制台上的位置
		virtual ConsolePosition IndexToConsolePosition(size_t index) = 0;

		// 规约指定坐标到正确的位置
		virtual ConsolePosition RestrictPosition(ConsolePosition pos, Direction direction = Direction::None) = 0;
		// 快速规约, 具体含义由实现定义
		// TODO: 想个优雅点的方案 (这个函数是为了解决行号打印的性能问题而诞生的)
		virtual ConsolePosition FastRestrictPosition(ConsolePosition pos, Direction direction = Direction::None) = 0;

		// 获取该行的起始和终止下标
		virtual pair<size_t, size_t> LineRangeOf(size_t lineIndex) = 0;

		// 获取指定位置的行下标
		// 注意, 换行符中间 (比如 \r\n 中间) 的下标属于前一行还是后一行是未指明的, 而且你不应该这么调用
		virtual size_t LineIndexOf(size_t index) = 0;
		virtual size_t LineIndexOf(ConsolePosition pos) = 0;

		// 获取指定范围上的格式化后的字符串
		virtual FormattedString GetString(ConsoleRect range) = 0;

		// 表明字符串发生了改变, 可以选择性地传入首个发生改变的下标以简化内部运算
		// 必须通过这个函数来通知 Formatter 字符串发生了改变
		virtual void OnStringUpdate(const String& newString) = 0;
		virtual void OnStringUpdate(const String& newString, size_t index) = 0;

		// 表示绘制范围发生了改变
		// 和 GetString 时传入的范围不同, 这个是“格式化并缓存了字符串所有内容”的范围
		// 可以理解为 GetString 传入的只是裁剪矩阵, 表示只观察了这部分格式化内容, 这个则是会影响格式化内容的矩阵
		// (当然, 视 Formatter 不同, 也可能不会影响)
		// 如果传入范围无法放下全部的格式化内容, 则具体行为由实现定义
		virtual void OnDrawRangeUpdate(ConsoleRect newDrawRange) = 0;

		// 表示绘图相关设置发生了改变, 如果内部缓存依赖于这些设置, 则需刷新内部缓存
		virtual void OnSettingUpdate() = 0;

		virtual ~Formatter() = default;
	};

	// 允许自由染色的 Formatter 的基类
	// 传入需要染色的下标范围和颜色, 即可在获取字符串时得到染色后的结果
	class ColorfulFormatter : public Formatter
	{
	public:
		class ColorRange
		{
		public:
			ConsoleColor textColor;
			ConsoleColor backgroundColor;
			pair<size_t, size_t> range;
		};
	public:
		virtual ColorfulFormattedString GetColorfulString(ConsoleRect range) = 0;

		virtual void SetColorRanges(const vector<ColorRange>& indices) = 0;
	};
}

namespace NylteJ
{
	// 为默认的两个 Formatter 提供一些通用工具的静态类
	class DefaultFormatterHelper
	{
	public:
		// 以下的 “CRLF” 均指 CR / LF / CRLF
		constexpr static auto ForwardCRLF(String::Iterator beg, String::Iterator end)
		{
			if (beg == end)
				return beg;

			const auto nextIter = next(beg);
			if (*beg == u8'\r' && nextIter != end && *nextIter == '\n')
				return next(nextIter);

			return nextIter;
		}
		constexpr static auto FindCRLF(String::Iterator beg, String::Iterator end)
		{
			static const u8string crlf = u8"\r\n";

			return find_first_of(beg, end, crlf.begin(), crlf.end());
		}

		constexpr static size_t WidthOf(auto&& str, size_t tabWidth)
		{
			size_t width = 0;

			for (auto&& chr : str)
			if (chr == '\t')
				width = (width / tabWidth + 1) * tabWidth;
			else if (IsWideChar(chr))
				width += 2;
			else
				width++;

			return width;
		}

		constexpr static ColorfulFormattedString::Line FormatLogicLine(StringView rawStr,
																	   ConsoleRect range,
																	   size_t tabWidth,
																	   ConsoleColor defaultTextColor,
																	   ConsoleColor defaultBackgroundColor,
																	   const auto& colorMap,
																	   size_t lineBeginIndex,
																	   size_t lineEndIndex)	// 相信编译器的优化能力吧...
		{
			ranges::subrange colors = colorMap;

			String stringBuffer;
			ColorfulFormattedString::Line line;

			const auto maxWidth = range.Width();

			size_t nowWidth = 0;
			bool nowInvisible = range.leftTop.x > 0;	// 当前是否正在格式化屏幕左侧的不可见部分(这部分宽度总是 range.leftTop.x)

			auto rawIter = rawStr.begin();
			size_t lastColorStrIndex = lineBeginIndex;
			for (; rawIter != rawStr.end(); ++rawIter)
			{
				const auto rawIndex = rawStr.GetByteIndex(rawIter) + lineBeginIndex;
				if (!colors.empty() && colors.front().first.first <= rawIndex)
				{
					if (lastColorStrIndex != colors.front().first.first)
					{
						if (!nowInvisible && !stringBuffer.empty())
							line.data.emplace_back(std::move(stringBuffer), defaultTextColor, defaultBackgroundColor);

						lastColorStrIndex = colors.front().first.first;
						stringBuffer.clear();
					}

					if (colors.front().first.second <= rawIndex)
					{
						if (!nowInvisible)	// 传入空的颜色区间不是我们需要修复的事, 维持语义即可
							line.data.emplace_back(std::move(stringBuffer), colors.front().second.first, colors.front().second.second);

						colors.advance(1);
						stringBuffer.clear();
					}
				}

				if (!nowInvisible && nowWidth >= maxWidth)
					break;

				if (*rawIter == '\t')
				{
					const auto fullWidth = nowWidth + (nowInvisible ? 0 : range.leftTop.x);
					const auto spaceWidth = (fullWidth / tabWidth + 1) * tabWidth - fullWidth;


					if (nowInvisible && range.leftTop.x - nowWidth <= spaceWidth)
					{
						stringBuffer = String(u8' ', spaceWidth - (range.leftTop.x - nowWidth));

						nowInvisible = false;
						nowWidth = spaceWidth - (range.leftTop.x - nowWidth);
					}
					else if (!nowInvisible && maxWidth - nowWidth <= spaceWidth)
					{
						stringBuffer.append_range(String(u8' ', maxWidth - nowWidth));

						break;
					}
					else
					{
						if (!nowInvisible)
							stringBuffer.append_range(String(u8' ', spaceWidth));
						nowWidth += spaceWidth;
					}
				}
				else if (IsWideChar(*rawIter))
				{
					// TODO: 减少重复代码
					if (nowInvisible && range.leftTop.x - nowWidth <= 2)
					{
						if (range.leftTop.x - nowWidth < 2)
							stringBuffer = String{ u8' ' };

						nowInvisible = false;
						nowWidth = 2 - (range.leftTop.x - nowWidth);
					}
					else if (!nowInvisible && maxWidth - nowWidth <= 2)
					{
						if (maxWidth - nowWidth < 2)
							stringBuffer.emplace_back(u8' ');
						else
							stringBuffer.emplace_back(*rawIter);

						break;
					}
					else
					{
						if (!nowInvisible)
							stringBuffer.emplace_back(*rawIter);
						nowWidth += 2;
					}
				}
				else
				{
					if (!nowInvisible)
						stringBuffer.emplace_back(*rawIter);
					nowWidth++;

					if (nowInvisible && range.leftTop.x == nowWidth)
					{
						nowInvisible = false;
						nowWidth = 0;
					}
					else if (!nowInvisible && maxWidth == nowWidth)
						break;
				}
			}

			if (!nowInvisible && !stringBuffer.empty())
				if (!colors.empty() && colors.front().first.first <= lineEndIndex && colors.front().first.second >= lineEndIndex)
				{
					if (colors.front().first.first < lineEndIndex)
						line.data.emplace_back(std::move(stringBuffer), colors.front().second.first, colors.front().second.second);
					else
					{
						line.data.emplace_back(std::move(stringBuffer), defaultTextColor, defaultBackgroundColor);
						// 此时染色从这一行的行尾开始, 我们会试图额外添加一个被选中的空格来体现这一点
						if (nowWidth < maxWidth)
						{
							line.data.emplace_back(String{ u8' ' }, colors.front().second.first, colors.front().second.second);
							nowWidth++;
						}
					}
				}
				else
					line.data.emplace_back(std::move(stringBuffer), defaultTextColor, defaultBackgroundColor);

			// 计算行尾额外空格的数量
			if (nowInvisible)
				line.extraSpaceCount = maxWidth;
			else if (nowWidth < maxWidth)
				line.extraSpaceCount = maxWidth - nowWidth;
			else
				line.extraSpaceCount = 0;

			return line;
		}

		DefaultFormatterHelper() = delete;
	};
}

export namespace NylteJ
{
	// 默认 Formatter
	// 使用引用语义传递字符串, 值语义传递颜色区间. 无自动换行
	// 绘制范围对其无作用, 始终假设拥有无限大的、左上角为 (0, 0) 的范围用于格式化
	class DefaultFormatter final : public ColorfulFormatter
	{
	private:
		using CacheLine = pair<size_t, size_t>;		// 左闭右开, 对应一整行 (不含换行)
	private:
		const String* str;

		const SettingMap& settingMap;

		ConsoleColor defaultTextColor;
		ConsoleColor defaultBackgroundColor;

		map<pair<size_t, size_t>, pair<ConsoleColor, ConsoleColor>> colorMap;

		vector<CacheLine> cacheLine;	// 暂且使用连续缓存, 下标即行号, 仅按行分割, 不做进一步处理
	private:
		// 即使对空字符串也会分割出一行 [0, 0)
		void DoSplitUntil(auto&& pred)
		{
			if (pred())
				return;

			auto lineBeginIter = str->begin();
			if (!cacheLine.empty())
			{
				if (cacheLine.back().second == str->ByteSize())
					return;

				lineBeginIter = DefaultFormatterHelper::ForwardCRLF(str->AtByteIndex(cacheLine.back().second), str->end());
			}
			auto lineEndIter = DefaultFormatterHelper::FindCRLF(lineBeginIter, str->end());

			while (!pred())
			{
				cacheLine.emplace_back(str->GetByteIndex(lineBeginIter), str->GetByteIndex(lineEndIter));

				if (lineEndIter == str->end())
					break;

				lineBeginIter = DefaultFormatterHelper::ForwardCRLF(lineEndIter, str->end());
				lineEndIter = DefaultFormatterHelper::FindCRLF(lineBeginIter, str->end());
			}
		}

		void SplitLineUntilLine(size_t lineIndex)
		{
			DoSplitUntil([&] {return cacheLine.size() > lineIndex; });
		}
		void SplitLineUntilIndex(size_t index)
		{
			DoSplitUntil([&] {return !cacheLine.empty() && cacheLine.back().second >= index; });
		}

		// 获取指定行的原始字符串, 可以传入 width 以尝试优化长行获取
		constexpr StringView GetLine(size_t y, size_t width) const
		{
			// 姑且认为不存在不可见字符, 据此做简单的长行优化
			// TODO: 优化这部分, 或干脆去掉(代价是超长行会带来额外内存和其它开销)
			const auto endIndex = min(cacheLine[y].second, cacheLine[y].first + 4 * width);		// 这实际用到了 String 内部使用 UTF-8 的特性
			return { str->AtByteIndex(cacheLine[y].first), str->AtByteIndex(endIndex) };
		}
	public:
		size_t ConsolePositionToIndex(ConsolePosition pos) override
		{
			SplitLineUntilLine(pos.y);

			const auto tabWidth = settingMap.Get<SettingID::TabWidth>();

			auto iter = str->AtByteIndex(cacheLine[pos.y].first);
			ConsoleWidth width = 0;

			while (width < pos.x)
			{
				if (*iter == '\t')
					width = (width / tabWidth + 1) * tabWidth;
				else if (IsWideChar(*iter))
					width += 2;
				else
					width++;

				++iter;
			}

			return str->GetByteIndex(iter);
		}
		ConsolePosition IndexToConsolePosition(size_t index) override
		{
			SplitLineUntilIndex(index);

			const auto tabWidth = settingMap.Get<SettingID::TabWidth>();

			const auto y = LineIndexOf(index);

			return ConsolePosition{
				DefaultFormatterHelper::WidthOf(str->View(cacheLine[y].first, index), tabWidth),
				y
			};
		}

		// 规约策略:
		// 1. pos.y 过大 / 过小时, 只规约 y 而不规约 x
		// 2. pos.x 过大时, 若方向向右则换行(除非已是最后一行), 否则归约到最近的 x
		// 3. pos.x 过小时类似, 方向向左则换行(除非是第一行), 否则归约到 0
		// 4. 位于字符中间(而非缝隙)时, 根据方向规约, 无方向则四舍五入
		ConsolePosition RestrictPosition(ConsolePosition pos, Direction direction) override
		{
			using enum Direction;

			pos = FastRestrictPosition(pos, direction);

			// ConsolePositionToIndex 在输入卡在字符中间时会返回下一个字符的下标, 比如 a\tb 中, (0,2) 对应 b 的下标
			const auto nextIndex = ConsolePositionToIndex(pos);
			const auto inversePosition = IndexToConsolePosition(nextIndex);
			if (inversePosition != pos)		// 此时只能是卡在两个字符中间了(不然就是出 bug 了)
			{
				if (str->AtByteIndex(nextIndex) == str->begin())
					unreachable();

				const auto currentIter = prev(str->AtByteIndex(nextIndex));
				const auto lastPosition = IndexToConsolePosition(str->GetByteIndex(currentIter));

				if (lastPosition.y != inversePosition.y || inversePosition.y != pos.y)
					unreachable();	// 应当不可能发生

				// 此时, lastPosition 对应当前字符开始位置, inversePosition 对应当前字符结束位置
				if (direction == Right)
					pos = inversePosition;
				else if (direction == Left)
					pos = lastPosition;
				else if (pos.x - lastPosition.x >= inversePosition.x - pos.x)
					pos = inversePosition;
				else
					pos = lastPosition;
			}

			return pos;
		}
		// 忽略位于字符中间的情况
		ConsolePosition FastRestrictPosition(ConsolePosition pos, Direction direction) override
		{
			using enum Direction;

			SplitLineUntilLine(pos.y);

			pos.y = clamp(pos.y, 0, static_cast<ConsoleYPos>(cacheLine.size() - 1));

			// 理论 pos.x == 0 时总是合法的, 为加速这里特判一下 (profile 表明(大概)值得)
			if (pos.x == 0)
				return pos;

			auto thisLineEndX = IndexToConsolePosition(cacheLine[pos.y].second).x;
			if (pos.x > thisLineEndX)
				if (direction != Right || pos.y == cacheLine.size() - 1)
					pos.x = thisLineEndX;
				else
				{
					pos.x = 0;
					pos.y++;
				}
			else if (pos.x < 0)
				if (direction != Left || pos.y == 0)
					pos.x = 0;
				else
				{
					pos.y--;
					pos.x = IndexToConsolePosition(cacheLine[pos.y].second).x;
				}

			return pos;
		}

		pair<size_t, size_t> LineRangeOf(size_t lineIndex) override
		{
			SplitLineUntilLine(lineIndex);
			return cacheLine[lineIndex];
		}

		size_t LineIndexOf(size_t index) override
		{
			SplitLineUntilIndex(index);

			// 下标位于这个字符的前面, 可以看作某种闭区间, 我们的缓存左闭右开, 所以 index 在 [first, second] 内即可
			const auto iter = ranges::lower_bound(cacheLine,
												  make_pair(index, index),
												  [](auto&& l, auto&& r) {return l.second < r.second; });

			// 实际上, 甚至不需要做其它验证, 直接返回即可
			return iter - cacheLine.begin();
		}
		size_t LineIndexOf(ConsolePosition pos) override
		{
			// 没有自动换行, 是一一对应关系
			return pos.y;
		}

		FormattedString GetString(ConsoleRect range) override
		{
			// ConsoleRect 是闭区间
			SplitLineUntilLine(range.rightBottom.y);

			FormattedString ret;

			if (cacheLine.size() <= range.leftTop.y)
				return ret;
			if (cacheLine.size() <= range.rightBottom.y)
				range.rightBottom.y = cacheLine.size() - 1;

			ret.data.reserve(range.Height());

			for (int y = range.leftTop.y; y <= range.rightBottom.y; y++)
			{
				String line = GetLine(y, range.rightBottom.x);		// [0, rightBottom.x] 的部分我们都需要

				const auto maxWidth = range.Width();
				const auto tabWidth = settingMap.Get<SettingID::TabWidth>();

				size_t nowWidth = 0;
				bool nowInvisible = range.leftTop.x > 0;	// 当前是否正在格式化屏幕左侧的不可见部分(这部分宽度总是 range.leftTop.x)

				size_t nowByteIndex = 0;
				size_t visibleBeginIndex = 0;
				for (; nowByteIndex < line.ByteSize(); nowByteIndex = line.GetByteIndex(next(line.AtByteIndex(nowByteIndex))))
				{
					if (nowInvisible && nowWidth >= range.leftTop.x)
					{
						// 中英文混排时可能出现截断(range.leftTop.x 恰好位于汉字中间)
						// 此时要把这个截断的汉字换成空格(终端做不到显示半个汉字)
						if (nowWidth > range.leftTop.x)
						{
							// 因为我们逐字符扫描, 跳过等于直接大于就意味着宽字符
							const auto prevIter = prev(line.AtByteIndex(nowByteIndex));
							const auto prevIndex = line.GetByteIndex(prevIter);

							line.replace_with_range(prevIter,
													next(prevIter),
													u8"  ");

							// 之后迭代器失效, 加上宽度截断问题, 需手动调整 (有点诡异)
							if (nowWidth - range.leftTop.x != 1)
								unreachable();

							nowByteIndex = line.GetByteIndex(next(line.AtByteIndex(prevIndex), 2));
						}

						nowInvisible = false;
						nowWidth = 0;
						visibleBeginIndex = nowByteIndex;
					}
					if (!nowInvisible && nowWidth >= maxWidth)
						break;

					const auto currentIter = line.AtByteIndex(nowByteIndex);

					if (*currentIter == '\t')
					{
						const auto fullWidth = nowWidth + (nowInvisible ? 0 : range.leftTop.x);
						const auto spaceWidth = (fullWidth / tabWidth + 1) * tabWidth - fullWidth;

						line.replace_with_range(currentIter,
												next(currentIter),
												String(u8' ', spaceWidth));

						nowWidth++;	// 这里加的实际上是原 '\t' 处空格的宽度, 后面多填充的空格留给后面几次循环慢慢加
					}
					else if (IsWideChar(*currentIter))
						nowWidth += 2;
					else
						nowWidth++;
				}

				// (见上) 处理末尾宽字符被截断的情况
				// 不写在里面, 否则宽字符在末尾时会出问题
				if (!nowInvisible && nowWidth > maxWidth)
				{
					const auto prevIter = prev(line.AtByteIndex(nowByteIndex));

					line.replace_with_range(prevIter,
											next(prevIter),
											u8"  ");

					if (nowWidth - maxWidth != 1)
						unreachable();
				}

				if (nowInvisible)
					line.clear();
				else
					line = String(line.AtByteIndex(visibleBeginIndex), line.AtByteIndex(nowByteIndex));

				// 计算行尾额外空格的数量
				size_t extraSpaceCount;
				if (nowInvisible)
					extraSpaceCount = maxWidth;
				else if (nowWidth < maxWidth)
					extraSpaceCount = maxWidth - nowWidth;
				else extraSpaceCount = 0;

				ret.data.emplace_back(std::move(line), extraSpaceCount);
			}

			return ret;
		}

		ColorfulFormattedString GetColorfulString(ConsoleRect range) override
		{
			// ConsoleRect 是闭区间
			SplitLineUntilLine(range.rightBottom.y);

			ColorfulFormattedString ret;

			if (cacheLine.size() <= range.leftTop.y)
				return ret;
			if (cacheLine.size() <= range.rightBottom.y)
				range.rightBottom.y = cacheLine.size() - 1;

			ret.data.reserve(range.Height());

			for (int y = range.leftTop.y; y <= range.rightBottom.y; y++)
				ret.data.emplace_back(DefaultFormatterHelper::FormatLogicLine(GetLine(y, range.rightBottom.x),	// [0, rightBottom.x] 的部分我们都需要
																			  range,
																			  settingMap.Get<SettingID::TabWidth>(),
																			  defaultTextColor,
																			  defaultBackgroundColor,
																			  colorMap,
																			  cacheLine[y].first,
																			  cacheLine[y].second));

			return ret;
		}

		void SetColorRanges(const vector<ColorRange>& indices) override
		{
			colorMap.clear();

			for (const auto& [textColor, backgroundColor, range] : indices)
				colorMap.emplace(range, make_pair(textColor, backgroundColor));
		}

		void OnStringUpdate(const String& newString) override
		{
			OnStringUpdate(newString, 0);	// 比较两字符串差异的开销可能很大, 暂时不考虑
		}
		void OnStringUpdate(const String& newString, size_t index) override
		{
			str = &newString;

			// 使缓存失效
			const auto it = ranges::lower_bound(cacheLine,
												CacheLine{ index, index },
												[](auto&& l, auto&& r) { return l.second < r.second; });
			cacheLine.erase(it, cacheLine.end());
		}

		void OnDrawRangeUpdate(ConsoleRect) override {}		// 我们不关心

		void OnSettingUpdate() override
		{
			// 先不考虑太多
			cacheLine.clear();
		}

		// 引用语义, 须保证 str 生命周期不短于 Formatter
		DefaultFormatter(const String& str, const SettingMap& settingMap, ConsoleColor defaultTextColor = BasicColors::white, ConsoleColor defaultBackgroundColor = BasicColors::black)
			:str(&str), settingMap(settingMap), defaultTextColor(defaultTextColor), defaultBackgroundColor(defaultBackgroundColor) {}
	};

	// 默认的自动换行 Formatter
	// 使用引用语义传递字符串, 值语义传递颜色区间. 贪心自动换行, 无语义分析(可能会在各种预想不到的地方换行, 单词中间、词组中间、标点前面等等)
	// 传入绘制范围宽度决定了自动换行位置, 长度被忽略, 始终假设左上角为 (0, 0), 无限长度
	// 如果单行宽度不足以放下单个字符则截断剩余部分 (比如一个究极宽的制表符), 其余时候永远不会截断字符
	// 在获取字符串时, 仍然可以修改传入的 range 以左右移动渲染范围, 尽管可能没什么用
	// 对于制表符, 始终依据逻辑行计算其宽度(比如, (制表符宽 4 字符), 一行被折成两行, 第一行宽 39 个字符, 第二行行首是制表符, 则这个制表符宽度为 4 而不是 1)
	// 也许不该这样, 但有点难改了所以就这样吧...
	class DefaultWarpFormatter final : public ColorfulFormatter
	{
	private:
		class CacheLine		// 物理行(CR / LF 等分割出的行)缓存
		{
		public:
			size_t logicLineIndex;			// 起始逻辑行号, 对应 ConsolePosition::y
			vector<size_t> headIndexes;		// 所有行首(包括物理行首和自动换行出的行首)下标, 对应该行起始字符
			size_t endIndex;				// 该行结束下标
			// 这一行打印出来应该类似于 [headIndexes[i], headIndexes[i+1]) + ... + [headIndexes.back(), endIndex)
			// 如果一行位于 headIndexes[i], 则它在换行后位于 logicLineIndex + i 行
			// 一个物理行占据 [logicLineIndex, logicLineIndex + headIndexes.size()) 逻辑行
			// (逻辑行编号从 0 开始)
			// 注意, 和非自动换行时的情况不同, 这里换行处下标对应两个坐标, 我们优先选择上一行末尾的坐标
		};
	private:
		const String* str;

		const SettingMap& settingMap;

		ConsoleRect drawRange;

		ConsoleColor defaultTextColor;
		ConsoleColor defaultBackgroundColor;

		map<pair<size_t, size_t>, pair<ConsoleColor, ConsoleColor>> colorMap;

		vector<CacheLine> cacheLine;	// 可以利用 logicLineIndex 二分搜索位置, 不使用 map, 因为我们用不着那么多特性
	private:
		// 直接在传入值上修改, 要求 headIndexes[0] 有值
		void SplitPhysicLine(CacheLine& line) const
		{
			const auto maxWidth = drawRange.Width();
			const auto tabWidth = settingMap.Get<SettingID::TabWidth>();

			line.headIndexes.resize(1);

			size_t nowWidth = 0;
			for (size_t nowByteIndex = line.headIndexes[0];
				 nowByteIndex < line.endIndex;
				 nowByteIndex = str->GetByteIndex(next(str->AtByteIndex(nowByteIndex))))
			{
				const auto currentChar = *str->AtByteIndex(nowByteIndex);

				size_t currentCharWidth;
				if (currentChar == '\t')
					currentCharWidth = (nowWidth / tabWidth + 1) * tabWidth - nowWidth;
				else if (IsWideChar(currentChar))
					currentCharWidth = 2;
				else
					currentCharWidth = 1;

				if (currentCharWidth + nowWidth > maxWidth)
				{
					if (nowByteIndex == line.headIndexes.back())
					{
						// 第一个字符就放不下, 特殊处理, 直接以下一个字符为首另起一行
						// 实际上只有第一行就放不下时会进到这里, 此后连续放不下时也会进到这里
						// 除此之外都会在下面被处理
						// 归根结底是因为初始内置了第一行的开头
						line.headIndexes.emplace_back(str->GetByteIndex(next(str->AtByteIndex(nowByteIndex))));

						nowWidth = 0;
						// 下一轮就如同在处理第一行一样
					}
					else
					{
						line.headIndexes.emplace_back(nowByteIndex);	// 以第一个放不下的字符为首另开一行, 然后我们实际上就不会再处理这个字符了

						if (currentChar != '\t')
							nowWidth = currentCharWidth;
						else
							nowWidth = tabWidth;
					}
				}
				else
					nowWidth += currentCharWidth;
			}
		}

		// 即使对空字符串也会分割出一行 [0, 0)
		void DoSplitUntil(auto&& pred)
		{
			if (pred())
				return;

			auto lineBeginIter = str->begin();
			if (!cacheLine.empty())
			{
				if (cacheLine.back().endIndex == str->ByteSize())
					return;

				lineBeginIter = DefaultFormatterHelper::ForwardCRLF(str->AtByteIndex(cacheLine.back().endIndex), str->end());
			}
			auto lineEndIter = DefaultFormatterHelper::FindCRLF(lineBeginIter, str->end());

			while (!pred())
			{
				{
					CacheLine line{
						.logicLineIndex = cacheLine.empty()
											  ? 0
											  : (cacheLine.back().logicLineIndex + cacheLine.back().headIndexes.size()),
						.headIndexes = { str->GetByteIndex(lineBeginIter) },
						.endIndex = str->GetByteIndex(lineEndIter)
					};

					SplitPhysicLine(line);

					cacheLine.emplace_back(std::move(line));
				}

				if (lineEndIter == str->end())
					break;

				lineBeginIter = DefaultFormatterHelper::ForwardCRLF(lineEndIter, str->end());
				lineEndIter = DefaultFormatterHelper::FindCRLF(lineBeginIter, str->end());
			}
		}

		void SplitLineUntilPhyLine(size_t lineIndex)
		{
			DoSplitUntil([&] {return cacheLine.size() > lineIndex; });
		}
		void SplitLineUntilLogicLine(size_t lineIndex)
		{
			DoSplitUntil([&] {return !cacheLine.empty() && cacheLine.back().logicLineIndex + cacheLine.back().headIndexes.size() > lineIndex; });
		}
		void SplitLineUntilIndex(size_t index)
		{
			DoSplitUntil([&] {return !cacheLine.empty() && cacheLine.back().endIndex >= index; });
		}

		// 不计算缓存, 调用时要确保缓存计算好了
		constexpr size_t LogicLineToPhyLine(size_t logicLineIndex)
		{
			// 这个就是左闭右开的了, 所以使用 upper_bound
			const auto iter = ranges::upper_bound(cacheLine,
												  CacheLine{ .logicLineIndex = logicLineIndex },
												  [](auto&& l, auto&& r)
												  {
													  return l.logicLineIndex + l.headIndexes.size() < r.logicLineIndex + r.headIndexes.size();
												  });

			return iter - cacheLine.begin();
		}

		constexpr pair<size_t, size_t> LogicLineRange(size_t logicLineIndex)
		{
			auto&& line = cacheLine[LogicLineToPhyLine(logicLineIndex)];

			const size_t beginIndex = line.headIndexes[logicLineIndex - line.logicLineIndex];

			const auto offset = logicLineIndex - line.logicLineIndex + 1;
			const size_t endIndex = (offset == line.headIndexes.size() ? line.endIndex : line.headIndexes[offset]);

			return { beginIndex, endIndex };
		}
		constexpr size_t LogicLineBegin(size_t logicLineIndex)
		{
			return LogicLineRange(logicLineIndex).first;
		}
		constexpr size_t LogicLineEnd(size_t logicLineIndex)
		{
			return LogicLineRange(logicLineIndex).second;
		}
	public:
		size_t ConsolePositionToIndex(ConsolePosition pos) override
		{
			SplitLineUntilLogicLine(pos.y);

			const auto tabWidth = settingMap.Get<SettingID::TabWidth>();

			auto iter = str->AtByteIndex(LogicLineBegin(pos.y));
			ConsoleWidth width = 0;

			while (width < pos.x)
			{
				if (*iter == '\t')
					width = (width / tabWidth + 1) * tabWidth;
				else if (IsWideChar(*iter))
					width += 2;
				else
					width++;

				++iter;
			}

			return str->GetByteIndex(iter);
		}
		ConsolePosition IndexToConsolePosition(size_t index) override
		{
			SplitLineUntilIndex(index);

			const auto tabWidth = settingMap.Get<SettingID::TabWidth>();

			auto&& lineCache = cacheLine[LineIndexOf(index)];
			const auto logicLineBoarder = distance(lineCache.headIndexes.begin(), ranges::lower_bound(lineCache.headIndexes, index));
			const auto logicLineOffset = logicLineBoarder == 0 ? 0 : (logicLineBoarder - 1);
			const auto y = lineCache.logicLineIndex + logicLineOffset;

			return ConsolePosition{
				DefaultFormatterHelper::WidthOf(str->View(lineCache.headIndexes[logicLineOffset], index), tabWidth),
				y
			};
		}

		// 规约策略:
		// 1. pos.y 过大 / 过小时, 只规约 y 而不规约 x
		// 2. pos.x 过大时, 若方向向右则换行(除非已是最后一行), 否则归约到最近的 x
		// 3. pos.x 过小时类似, 方向向左则换行(除非是第一行), 否则归约到 0
		// 4. 位于字符中间(而非缝隙)时, 根据方向规约, 无方向则四舍五入
		ConsolePosition RestrictPosition(ConsolePosition pos, Direction direction) override
		{
			using enum Direction;

			pos = FastRestrictPosition(pos, direction);

			// ConsolePositionToIndex 在输入卡在字符中间时会返回下一个字符的下标, 比如 a\tb 中, (0,2) 对应 b 的下标
			const auto nextIndex = ConsolePositionToIndex(pos);
			const auto inversePosition = IndexToConsolePosition(nextIndex);
			if (pos.x != 0 && inversePosition != pos)		// 此时只能是卡在两个字符中间了(不然就是出 bug 了)
			{
				if (str->AtByteIndex(nextIndex) == str->begin())
					unreachable();

				const auto currentIter = prev(str->AtByteIndex(nextIndex));
				auto lastPosition = IndexToConsolePosition(str->GetByteIndex(currentIter));

				if (lastPosition.y != inversePosition.y)
				{
					// 此时当前字符位于自动换行处, 我们手动调整 lastPosition
					lastPosition = ConsolePosition{ 0, inversePosition.y };

					if (inversePosition.y != pos.y)
						unreachable();
				}

				// 此时, lastPosition 对应当前字符开始位置, inversePosition 对应当前字符结束位置
				if (direction == Right)
					pos = inversePosition;
				else if (direction == Left)
					pos = lastPosition;
				else if (pos.x - lastPosition.x >= inversePosition.x - pos.x)
					pos = inversePosition;
				else
					pos = lastPosition;
			}

			return pos;
		}
		// 忽略位于字符中间的情况
		ConsolePosition FastRestrictPosition(ConsolePosition pos, Direction direction) override
		{
			using enum Direction;

			SplitLineUntilLogicLine(pos.y);

			const ConsoleHeight stringHeight = cacheLine.back().logicLineIndex + cacheLine.back().headIndexes.size();

			pos.y = clamp(pos.y, 0, stringHeight - 1);

			// 理论 pos.x == 0 时总是合法的, 为加速这里特判一下 (profile 表明(大概)值得)
			if (pos.x == 0)
				return pos;

			auto&& lineCache = cacheLine[LogicLineToPhyLine(pos.y)];
			const size_t logicLineEnd = LogicLineEnd(pos.y);
			const auto thisLineEndX = IndexToConsolePosition(logicLineEnd).x;
			if (pos.x > thisLineEndX)
				if (direction != Right || pos.y == stringHeight - 1)
					pos.x = thisLineEndX;
				else
				{
					if (pos.y == lineCache.logicLineIndex + lineCache.headIndexes.size() - 1)
						pos.x = 0;
					else
						pos.x = IndexToConsolePosition(str->GetByteIndex(next(str->AtByteIndex(logicLineEnd)))).x;		// 光标移到第一个字符后面

					pos.y++;
				}
			else if (pos.x < 0)
				if (direction != Left || pos.y == 0)
					pos.x = 0;
				else
				{
					if (pos.y == lineCache.logicLineIndex)
						pos.x = IndexToConsolePosition(LogicLineEnd(--pos.y)).x;
					else
						pos.x = IndexToConsolePosition(str->GetByteIndex(prev(str->AtByteIndex(LogicLineEnd(--pos.y))))).x;	// 光标移到最后一个字符前面
				}

			return pos;
		}

		pair<size_t, size_t> LineRangeOf(size_t lineIndex) override
		{
			SplitLineUntilPhyLine(lineIndex);
			return { cacheLine[lineIndex].headIndexes.front(), cacheLine[lineIndex].endIndex };
		}

		size_t LineIndexOf(size_t index) override
		{
			SplitLineUntilIndex(index);

			const auto iter = ranges::lower_bound(cacheLine,
												  CacheLine{ .endIndex = index },
												  [](auto&& l, auto&& r) {return l.endIndex < r.endIndex; });

			return iter - cacheLine.begin();
		}
		size_t LineIndexOf(ConsolePosition pos) override
		{
			SplitLineUntilLogicLine(pos.y);

			return LogicLineToPhyLine(pos.y);
		}

		FormattedString GetString(ConsoleRect range) override
		{
			return TODO;
		}

		ColorfulFormattedString GetColorfulString(ConsoleRect range) override
		{
			SplitLineUntilLogicLine(range.rightBottom.y);

			ColorfulFormattedString ret;

			{
				// 逻辑行区间 : [0, stringHeight)
				const auto stringHeight = cacheLine.back().logicLineIndex + cacheLine.back().headIndexes.size();
				if (stringHeight <= range.leftTop.y)
					return ret;
				if (stringHeight <= range.rightBottom.y)
					range.rightBottom.y = stringHeight - 1;
			}

			const auto tabWidth = settingMap.Get<SettingID::TabWidth>();

			const auto firstLineIndex = LogicLineToPhyLine(range.leftTop.y);
			const auto lastLineIndex = LogicLineToPhyLine(range.rightBottom.y);

			const auto skipFirstLogicLines = range.leftTop.y - cacheLine[firstLineIndex].logicLineIndex;
			const auto skipLastLogicLines = cacheLine[lastLineIndex].logicLineIndex + cacheLine[lastLineIndex].headIndexes.size() - range.rightBottom.y - 1;

			ret.data.reserve(range.Height()); 

			// 等 MSVC 适配 C++26 就可以用 views::concat 了, 现在先特殊处理最后一行, 凑合用着

			const auto formatSingleLine = [&](auto&& beginIndex, auto&& endIndex)
			{
				ret.data.emplace_back(DefaultFormatterHelper::FormatLogicLine(str->View(beginIndex, endIndex),
				                                                              range,
				                                                              tabWidth,
				                                                              defaultTextColor,
				                                                              defaultBackgroundColor,
				                                                              colorMap,
				                                                              beginIndex,
				                                                              endIndex));
			};
			const auto formatPhyLine = [&](auto&& headIndexes, auto&& endIndex)
				{
					for (auto&& [beginIndex, endIndex] : views::pairwise(headIndexes))
						formatSingleLine(beginIndex, endIndex);

					formatSingleLine(headIndexes.back(), endIndex);
				};

			const size_t endIndex = (skipLastLogicLines == 0)
				                        ? cacheLine[lastLineIndex].endIndex
				                        : (cacheLine[lastLineIndex].headIndexes[cacheLine[lastLineIndex].headIndexes.size() - skipLastLogicLines]);

			if (firstLineIndex != lastLineIndex)
			{
				formatPhyLine(cacheLine[firstLineIndex].headIndexes | views::drop(skipFirstLogicLines),
							  cacheLine[firstLineIndex].endIndex);

				for (auto&& physicLine : ranges::subrange{ cacheLine.begin() + firstLineIndex + 1, cacheLine.begin() + lastLineIndex })
					formatPhyLine(physicLine.headIndexes, physicLine.endIndex);

				formatPhyLine(cacheLine[lastLineIndex].headIndexes | views::take(cacheLine[lastLineIndex].headIndexes.size() - skipLastLogicLines),
							  endIndex);
			}
			else
				formatPhyLine(cacheLine[lastLineIndex].headIndexes
							 | views::take(cacheLine[lastLineIndex].headIndexes.size() - skipLastLogicLines)
							 | views::drop(skipFirstLogicLines),
							  endIndex);

			return ret;
		}

		void SetColorRanges(const vector<ColorRange>& indices) override
		{
			colorMap.clear();

			for (const auto& [textColor, backgroundColor, range] : indices)
				colorMap.emplace(range, make_pair(textColor, backgroundColor));
		}

		void OnStringUpdate(const String& newString) override
		{
			OnStringUpdate(newString, 0);
		}
		void OnStringUpdate(const String& newString, size_t index) override
		{
			str = &newString;

			// 使缓存失效
			const auto it = ranges::lower_bound(cacheLine,
												CacheLine{ .endIndex = index },
												[](auto&& l, auto&& r) { return l.endIndex < r.endIndex; });
			cacheLine.erase(it, cacheLine.end());
		}

		void OnDrawRangeUpdate(ConsoleRect newRange) override
		{
			const bool needReCalcCache = (drawRange.Width() != newRange.Width());

			drawRange = newRange;

			if (needReCalcCache)
			{
				size_t logicLineIndex = 0;
				for (auto&& cache : cacheLine)
				{
					cache.logicLineIndex = logicLineIndex;

					SplitPhysicLine(cache);

					logicLineIndex = cache.logicLineIndex + cache.headIndexes.size();
				}
			}
		}

		void OnSettingUpdate() override
		{
			// 先不考虑太多
			cacheLine.clear();
		}

		// 引用语义, 须保证 str 生命周期不短于 Formatter
		DefaultWarpFormatter(const String& str,
		                     const SettingMap& settingMap,
		                     ConsoleRect drawRange,
		                     ConsoleColor defaultTextColor       = BasicColors::stayOldColor,	// 用 stayOldColor 而不是固定色, 便于性能优化
		                     ConsoleColor defaultBackgroundColor = BasicColors::stayOldColor)	// (反正我们的打印函数总会复位颜色, 不用担心出现奇奇怪怪的颜色)
			: str(&str),
			  settingMap(settingMap),
			  drawRange(drawRange),
			  defaultTextColor(defaultTextColor),
			  defaultBackgroundColor(defaultBackgroundColor)
		{
		}
	};
}

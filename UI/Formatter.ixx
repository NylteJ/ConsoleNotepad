// Formatter.ixx
// 用于原始字符串的格式化与双向定位
// 简单来说, 就是负责将文件里读的字符串和屏幕上显示的位置一一对应的工具, 缩进的显示方式、自动换行等都是据此实现的
// 原则上, 只有 Formatter 知道这一对应关系, 其它组件应当无法获取(因此不能施加任何假设, 只能通过 Formatter 获取相关信息)
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

	// 默认 Formatter
	// 使用引用语义传递字符串, 值语义传递颜色区间. 无自动换行
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
			constexpr static auto forwardCRLF = [](auto&& iter, auto&& end)
			{
				if (iter == end)
					return iter;

				const auto nextIter = next(iter);
				if (*iter == u8'\r' && nextIter != end && *nextIter == '\n')
					return next(nextIter);

				return nextIter;
			};
			static const u8string crlf = u8"\r\n";

			if (pred())
				return;

			auto lineBeginIter = str->begin();
			if (!cacheLine.empty())
			{
				if (cacheLine.back().second == str->ByteSize())
					return;

				lineBeginIter = forwardCRLF(str->AtByteIndex(cacheLine.back().second), str->end());
			}
			auto lineEndIter = find_first_of(lineBeginIter, str->end(), crlf.begin(), crlf.end());

			while (!pred())
			{
				cacheLine.emplace_back(str->GetByteIndex(lineBeginIter), str->GetByteIndex(lineEndIter));

				if (lineEndIter == str->end())
					break;

				lineBeginIter = forwardCRLF(lineEndIter, str->end());
				lineEndIter = find_first_of(lineBeginIter, str->end(), crlf.begin(), crlf.end());
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
			size_t x = 0;

			for (auto&& chr : StringView{ str->AtByteIndex(cacheLine[y].first), str->AtByteIndex(index) })
				if (chr == '\t')
					x = (x / tabWidth + 1) * tabWidth;
				else if (IsWideChar(chr))
					x += 2;
				else
					x++;

			return ConsolePosition{ x,y };
		}

		// 规约策略:
		// 1. pos.y 过大 / 过小时, 只规约 y 而不规约 x
		// 2. pos.x 过大时, 若方向向右则换行(除非已是最后一行), 否则归约到最近的 x
		// 3. pos.x 过小时类似, 方向向左则换行(除非是第一行), 否则归约到 0
		// 4. 位于字符中间(而非缝隙)时, 根据方向规约, 无方向则四舍五入
		ConsolePosition RestrictPosition(ConsolePosition pos, Direction direction) override
		{
			using enum Direction;

			SplitLineUntilLine(pos.y);

			pos.y = clamp(pos.y, 0, static_cast<ConsoleYPos>(cacheLine.size() - 1));

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

			if (cacheLine.size() <= range.rightBottom.y)
				range.rightBottom.y = cacheLine.size() - 1;
			if (cacheLine.size() <= range.leftTop.y)
				return ret;

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

			if (cacheLine.size() <= range.rightBottom.y)
				range.rightBottom.y = cacheLine.size() - 1;
			if (cacheLine.size() <= range.leftTop.y)
				return ret;

			ret.data.reserve(range.Height());

			ranges::subrange colors = colorMap;
			for (int y = range.leftTop.y; y <= range.rightBottom.y; y++)
			{
				StringView rawStr = GetLine(y, range.rightBottom.x);		// [0, rightBottom.x] 的部分我们都需要
				String stringBuffer;
				ColorfulFormattedString::Line line;

				const auto maxWidth = range.Width();
				const auto tabWidth = settingMap.Get<SettingID::TabWidth>();

				size_t nowWidth = 0;
				bool nowInvisible = range.leftTop.x > 0;	// 当前是否正在格式化屏幕左侧的不可见部分(这部分宽度总是 range.leftTop.x)

				auto rawIter = rawStr.begin();
				size_t lastColorStrIndex = cacheLine[y].first;
				for (; rawIter != rawStr.end(); ++rawIter)
				{
					const auto rawIndex = rawStr.GetByteIndex(rawIter) + cacheLine[y].first;
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
					if (!colors.empty() && colors.front().first.first <= cacheLine[y].second && colors.front().first.second >= cacheLine[y].second)
					{
						if (colors.front().first.first < cacheLine[y].second)
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

				ret.data.emplace_back(std::move(line));
			}

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

		// 引用语义, 须保证 str 生命周期不短于 Formatter
		DefaultFormatter(const String& str, const SettingMap& settingMap, ConsoleColor defaultTextColor = BasicColors::white, ConsoleColor defaultBackgroundColor = BasicColors::black)
			:str(&str), settingMap(settingMap), defaultTextColor(defaultTextColor), defaultBackgroundColor(defaultBackgroundColor) {}

		~DefaultFormatter() override {}	// 不知道为什么, 不加这行在 Debug 下容易莫名其妙报“间接呼叫临界检查检测到无效的控制传输”(而且非 Debug 模式下貌似不会), 很迷, 可能是 MSVC 的神必 bug
	};
}
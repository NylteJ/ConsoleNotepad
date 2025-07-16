// SettingMap.ixx
// 是 SettingsHandler 的后端 (什么套娃)
// 只维护一个哈希表, 用于快速查找对应设置项
// 只需要把这个的引用到处传就可以了, 这样也不用担心循环引用
// (因为 SettingsHandler 里很难不 import Editor 等 UI 组件, 所以只能再套一层后端了)
// 使用了不少 (至少对我来说) 相当酷炫的模板技巧 (但我还是要说宏是模板它爷爷, 没有宏的话这里的各种打表会不可避免地吃屎)
// 原则上对设置上的查询都直接走这个表, 因为哈希表的查询本来也是 O(1) 的, 效率上和缓存一个变量是一样的, 还更加即时
// (这段是我之前写的, 有一说一槽点有点多......比如哈希表查表可能比普通 map 还慢, 以及这种表可以直接改成数组)
// (模板元编程的部分也可以优化, 虽然最好的方案还是静态反射......coming s∞n)
// (那部分就等静态反射出来直接一步到位吧, 模板元编程很难特别简单的实现)
export module SettingMap;

import std;

import FileHandler;
import Exceptions;
import String;
import Utils;

using namespace std;

export namespace NylteJ
{
	class SettingMap
	{
	public:
		// 用于在文件中区分不同设置项
		// 不应该改变现有项的值, 这样就算未来删除了某些项, 也不会影响到现有的设置文件
		// (之前的 UTF-8 重构已经破坏了一次兼容性, 所以顺路多改了几条)
		enum class ID :uint16_t	// 必须这么指定, 不然 x86 和 x64 的配置文件还不能通用
		{
			InternalVersionID                = 0,

			DefaultBehaviorWhenErrorEncoding = 1,
			AutoSavingDuration               = 2,
			MaxUndoStep                      = 3,
			MaxRedoStep                      = 4,
			MaxMergeCharUndoRedo             = 5,
			AutoSaveFileExtension            = 6,
			NewFileAutoSaveName              = 7,
			CloseHistoryWindowAfterEnter     = 8,
			SplitUndoStrWhenEnter            = 9,
			NormalExitWhenDoubleEsc          = 10,
			LineIndexWidth                   = 11,
			TabWidth                         = 12,
			Wrap                             = 13,
			DefaultNewLine                   = 14,

			_MaxEnum                         = 14,	// 这个值应当总是所有枚举值中最大的, 且应当尽可能地小
		};
		using enum ID;
	private:
#pragma region 现在是, 打表时间
		template<ID id>
		class DataTypeHelper { static_assert(false); };

		// 非常好宏定义, 使我免于复制粘贴
#define ID_TYPE(id_,type_) template<> class DataTypeHelper<id_> { public:using type = type_; };

		// 这里手动指定长度并不是为了省空间, 只是为了让配置文件向前向后都能兼容
		// 实际上无论这里指定成多少位, 后面的 variant 的长度都不会变, 顶多省点磁盘空间
		// 下面用 variant 的原因只是为了后续可能加入的字符串 / 浮点设置项等
		// 顺便用 Editor 时最低只能使用 16 位整数, 因为 8 位的是 char, 会出各种问题
		ID_TYPE(InternalVersionID, uint8_t)
		ID_TYPE(DefaultBehaviorWhenErrorEncoding, uint8_t)
		ID_TYPE(AutoSavingDuration, uint32_t)
		ID_TYPE(MaxUndoStep, uint32_t)
		ID_TYPE(MaxRedoStep, uint32_t)
		ID_TYPE(MaxMergeCharUndoRedo, uint32_t)
		ID_TYPE(AutoSaveFileExtension, u8string)
		ID_TYPE(NewFileAutoSaveName, u8string)
		ID_TYPE(CloseHistoryWindowAfterEnter, uint8_t)
		ID_TYPE(SplitUndoStrWhenEnter, uint8_t)
		ID_TYPE(NormalExitWhenDoubleEsc, uint8_t)
		ID_TYPE(LineIndexWidth, uint16_t)
		ID_TYPE(TabWidth, uint16_t)
		ID_TYPE(Wrap, uint8_t)
		ID_TYPE(DefaultNewLine, uint8_t)

#undef ID_TYPE

#pragma endregion
	public:
		using SizeDataType = uint16_t;	// 64KB 的单条数据大小上限应该无论如何都够用了
	public:
		template<ID id>
		using DataType = typename DataTypeHelper<id>::type;
		
		// 其实这个大一点也没关系, 因为每个设置项都只会带来一个新对象, 外界获取到的始终是实际的对应类型
		using StoreType = variant<uint8_t, uint16_t, uint32_t, uint64_t, float, double, u8string>;
	private:
		EnumMap<ID, StoreType> datas;

		FileHandler settingsFile;

		// 内部特殊版本号, 发生变动说明无法简单兼容, 应当直接重置
		// 不能太低, 以免和以前的 DefaultBehaviorWhenErrorEncoding 冲突
		static constexpr DataType<InternalVersionID> internalVersionID = 8;
	private:
		template<size_t depth>
		static consteval array<size_t, depth> GenerateVariantSizeArray()
		{
			if constexpr (depth == 1)
				return { sizeof(variant_alternative_t<0, StoreType>) };
			else
			{
				auto last = GenerateVariantSizeArray<depth - 1>();

				// 反正是 consteval, 效率拉稀一点无所谓
				array<size_t, depth> ret;
				ranges::move(last, ret.begin());

				ret.back() = sizeof(variant_alternative_t<depth - 1, StoreType>);

				return ret;
			}
		}

		// 后续删掉某些 ID 时再在这里加
		// 没有反射的痛
		static constexpr array allValidID{
			InternalVersionID,

			DefaultBehaviorWhenErrorEncoding,
			AutoSavingDuration,
			MaxUndoStep, MaxRedoStep, MaxMergeCharUndoRedo,
			AutoSaveFileExtension, NewFileAutoSaveName,
			CloseHistoryWindowAfterEnter,
			SplitUndoStrWhenEnter,
			NormalExitWhenDoubleEsc,
			LineIndexWidth,
			TabWidth,
			Wrap,
			DefaultNewLine,
		};

		static_assert(ranges::all_of(allValidID, [](auto&& id) {return id <= _MaxEnum; }));	// 防呆检测
	private:
		static constexpr bool IsValidID(ID id)
		{
			return ranges::contains(allValidID,id);
		}

		static constexpr StoreType GetDefaultValues(ID id)
		{
			switch (id)
			{
#define SET_DEFAULT_VALUE(id_, value_) case id_: return DataType<id_>{ value_ };

				SET_DEFAULT_VALUE(InternalVersionID, internalVersionID)

				SET_DEFAULT_VALUE(DefaultBehaviorWhenErrorEncoding, 1)
				SET_DEFAULT_VALUE(AutoSavingDuration, 180)
				SET_DEFAULT_VALUE(MaxUndoStep, 1024)
				SET_DEFAULT_VALUE(MaxRedoStep, 1024)
				SET_DEFAULT_VALUE(MaxMergeCharUndoRedo, 16)
				SET_DEFAULT_VALUE(AutoSaveFileExtension, u8".autosave"s)
				SET_DEFAULT_VALUE(NewFileAutoSaveName, u8"__Unnamed_NewFile"s)
				SET_DEFAULT_VALUE(CloseHistoryWindowAfterEnter, 0)
				SET_DEFAULT_VALUE(SplitUndoStrWhenEnter, 1)
				SET_DEFAULT_VALUE(NormalExitWhenDoubleEsc, 0)
				SET_DEFAULT_VALUE(LineIndexWidth, 3)
				SET_DEFAULT_VALUE(TabWidth, 4)
				SET_DEFAULT_VALUE(Wrap, false)
				SET_DEFAULT_VALUE(DefaultNewLine, 0)
					
#undef SET_DEFAULT_VALUE
			}
			unreachable();
		}
	public:
		template<typename DataType>
		constexpr DataType Get(ID id) const
		{
			return get<DataType>(datas.at(id));
		}
		template<ID id>
		constexpr DataType<id> Get() const
		{
			return Get<DataType<id>>(id);
		}

		template<typename DataType>
		void Set(ID id, DataType&& value)
		{
			datas[id] = value;
		}
		template<ID id>
		void Set(DataType<id> value)
		{
			Set<DataType<id>>(id, value);
		}

		void Clear()
		{
			datas.clear();
		}
		bool Empty() const
		{
			return datas.empty();
		}

		constexpr size_t VariantSize(ID id) const
		{
			constexpr size_t variantListSize = variant_size_v<StoreType>;
			
			constexpr array<size_t, variantListSize> sizeArray = GenerateVariantSizeArray<variantListSize>();

			return sizeArray[datas.at(id).index()];
		}

		void FromBytes(const string& bytesData)
		{
			istringstream input{ bytesData };

			while (!input.eof())
			{
				SizeDataType dataSize;
				ID id;
				input.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize));
				input.read(reinterpret_cast<char*>(&id), sizeof(id));

				if (input.eof())	// 必须在 “全部读完后继续读” 时才会触发 EOF, 所以只能在这里额外判断一下
					return;

				if(IsValidID(id))
				{
					datas[id] = GetDefaultValues(id);

					visit([&]<typename ValueType>(ValueType&& value)
						  {
							  if constexpr (is_same_v<remove_cvref_t<ValueType>, u8string>)
							  {
								  value.resize(dataSize);
								  input.read(reinterpret_cast<char*>(value.data()), dataSize);
							  }
							  else
								  input.read(reinterpret_cast<char*>(&value), dataSize);
						  }, datas[id]);
				}
				else
					input.seekg(dataSize, ios::cur);
			}
		}

		string ToBytes() const
		{
			ostringstream output;

			for (auto&& id : allValidID)
			{
				auto&& data = datas.at(id);

				SizeDataType dataSize = VariantSize(id);
				const char* dataPtr = reinterpret_cast<const char*>(&data);

				if (auto ptr = get_if<u8string>(&data); ptr != nullptr)
				{
					dataSize = static_cast<SizeDataType>(ptr->size() * sizeof(char8_t));
					dataPtr = reinterpret_cast<const char*>(ptr->data());
				}

				output.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize));
				output.write(reinterpret_cast<const char*>(&id), sizeof(id));
				output.write(dataPtr, dataSize);
			}

			return output.str();
		}

		void SaveToFile()
		{
			auto byteStr = ToBytes();
			auto bytes = span{ reinterpret_cast<const byte*>(byteStr.data()), byteStr.size() * sizeof(char) };

			settingsFile.Write(bytes);
		}
		void LoadFromFile()
		{
			FromBytes(settingsFile.ReadAsBytes());
		}

		SettingMap(const filesystem::path& saveFilePath = L".\\ConsoleNotepad.config"sv)
		{
			try
			{
				settingsFile.OpenFile(saveFilePath);

				LoadFromFile();

				for (auto&& id : allValidID)
					if (!datas.contains(id))
						datas[id] = GetDefaultValues(id);

				if (Get<InternalVersionID>() != internalVersionID)
				{
					// TODO: 弹出警告或者确认框

					for (auto&& id : allValidID)
						datas[id] = GetDefaultValues(id);

					SaveToFile();
				}
			}
			catch (const FileOpenFailedException&)
			{
				settingsFile.CreateFile(saveFilePath);

				for (auto&& id : allValidID)
					datas[id] = GetDefaultValues(id);

				SaveToFile();
			}
		}
	};

	using SettingID = SettingMap::ID;

	template<SettingID id>
	using SettingDataType = SettingMap::DataType<id>;
}
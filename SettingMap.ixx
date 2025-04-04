// SettingMap.ixx
// 是 SettingsHandler 的后端 (什么套娃)
// 只维护一个哈希表, 用于快速查找对应设置项
// 只需要把这个的引用到处传就可以了, 这样也不用担心循环引用
// (因为 SettingsHandler 里很难不 import Editor 等 UI 组件, 所以只能再套一层后端了)
// 使用了不少 (至少对我来说) 相当酷炫的模板技巧
// 原则上对设置上的查询都直接走这个表, 因为哈希表的查询本来也是 O(1) 的, 效率上和缓存一个变量是一样的, 还更加即时
export module SettingMap;

import std;

import FileHandler;
import Exceptions;

using namespace std;

export namespace NylteJ
{
	class SettingMap
	{
	public:
		// 用于在文件中区分不同设置项
		// 不应该改变现有项的值, 这样就算未来删除了某些项, 也不会影响到现有的设置文件
		// 也是考虑到这一点, 没有做成字符串的形式, 因为字符串总是会让人有改的欲望, 直接改成一个枚举数就不会了
		enum class ID :uint16_t	// 必须这么指定, 不然 x86 和 x64 的配置文件还不能通用
		{
			DefaultBehaviorWhenErrorEncoding = 0,
			AutoSavingDuration = 1,
			MaxUndoStep = 2,
			MaxRedoStep = 3,
			MaxMergeCharUndoRedo = 4,
			AutoSaveFileExtension = 5,
			NewFileAutoSaveName = 6,
			CloseHistoryWindowAfterEnter = 7,
			SplitUndoStrWhenEnter = 8
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
		ID_TYPE(DefaultBehaviorWhenErrorEncoding, uint8_t);
		ID_TYPE(AutoSavingDuration, uint32_t);
		ID_TYPE(MaxUndoStep, uint16_t);
		ID_TYPE(MaxRedoStep, uint16_t);
		ID_TYPE(MaxMergeCharUndoRedo, uint16_t);
		ID_TYPE(AutoSaveFileExtension, wstring);
		ID_TYPE(NewFileAutoSaveName, wstring);
		ID_TYPE(CloseHistoryWindowAfterEnter, uint8_t);
		ID_TYPE(SplitUndoStrWhenEnter, uint8_t);

#undef ID_TYPE

#pragma endregion
	public:
		using SizeDataType = uint16_t;	// 64KB 的单条数据大小上限应该无论如何都够用了
	private:
		class FromByteHelper
		{
		private:
			istringstream& input;
			SizeDataType dataSize;
		public:
			template<typename NormalType>
			void operator()(NormalType&& value) const
			{
				input.read(reinterpret_cast<char*>(&value), dataSize);
			}
			template<>
			void operator()(wstring& str) const
			{
				str.resize(dataSize / 2);
				input.read(reinterpret_cast<char*>(str.data()), dataSize);
			}

			FromByteHelper(istringstream& input, SizeDataType dataSize)
				:input(input), dataSize(dataSize)
			{
			}
		};
	public:
		template<ID id>
		using DataType = DataTypeHelper<id>::type;
		
		// 其实这个大一点也没关系, 因为每个设置项都只会带来一个新对象, 外界获取到的始终是实际的对应类型
		using StoreType = variant<uint8_t, uint16_t, uint32_t, uint64_t, float, double, wstring>;
	private:
		unordered_map<ID, StoreType> datas;

		FileHandler settingsFile;
	private:
		template<size_t depth>
		static consteval array<size_t, depth> GenerateVariantSizeArray()
		{
			auto last = GenerateVariantSizeArray<depth - 1>();

			array<size_t, depth> ret;
			for (size_t i = 0; i < last.size(); i++)
			{
				ret[i] = last[i];
			}
			ret.back() = sizeof(variant_alternative_t<depth - 1, StoreType>);

			return ret;
		}
		template<>
		static consteval array<size_t, 1> GenerateVariantSizeArray()
		{
			return { sizeof(variant_alternative_t<0, StoreType>) };
		}

		// 后续删掉某些 ID 时再在这里加
		// 没有反射的痛
		static constexpr array allValidID{ DefaultBehaviorWhenErrorEncoding,
			AutoSavingDuration,
			MaxUndoStep,MaxRedoStep,MaxMergeCharUndoRedo,
			AutoSaveFileExtension,NewFileAutoSaveName,
			CloseHistoryWindowAfterEnter,
			SplitUndoStrWhenEnter };
	private:
		static constexpr bool IsValidID(ID id)
		{
			return ranges::contains(allValidID,id);
		}

		static constexpr StoreType GetDefaultValues(ID id)
		{
			switch (id)
			{
			case DefaultBehaviorWhenErrorEncoding:
				return DataType<DefaultBehaviorWhenErrorEncoding>{ 1 };
			case AutoSavingDuration:
				return DataType<AutoSavingDuration>{ 180 };
			case MaxUndoStep:
				return DataType<MaxUndoStep>{ 1024 };
			case MaxRedoStep:
				return DataType<MaxRedoStep>{ 1024 };
			case MaxMergeCharUndoRedo:
				return DataType<MaxMergeCharUndoRedo>{ 16 };
			case AutoSaveFileExtension:
				return DataType<AutoSaveFileExtension>{ L".autosave"s };
			case NewFileAutoSaveName:
				return DataType<NewFileAutoSaveName>{ L"__Unnamed_NewFile"s };
			case CloseHistoryWindowAfterEnter:
				return DataType<CloseHistoryWindowAfterEnter>{ 0 };
			case SplitUndoStrWhenEnter:
				return DataType<SplitUndoStrWhenEnter>{ 1 };
			}
			unreachable();
		}
	public:
		template<typename DataType>
		DataType Get(ID id) const
		{
			return get<DataType>(datas.at(id));
		}
		template<ID id>
		DataType<id> Get() const
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

		void FromBytes(string bytesData)
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

					visit(FromByteHelper{ input,dataSize }, datas[id]);
				}
				else
					input.seekg(dataSize, ios::cur);
			}
		}

		string ToBytes() const
		{
			ostringstream output;

			for (auto&& [id, data] : datas)
			{
				SizeDataType dataSize = VariantSize(id);
				const char* dataPtr = reinterpret_cast<const char*>(&data);

				if (auto ptr = get_if<wstring>(&data);ptr != nullptr)
				{
					dataSize = static_cast<SizeDataType>(ptr->size() * sizeof(wstring::value_type));
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
			settingsFile.Write(ToBytes());
		}
		void LoadFromFile()
		{
			FromBytes(settingsFile.ReadAsBytes());
		}

		SettingMap(filesystem::path saveFilePath = L".\\ConsoleNotepad.config"sv)
		{
			try
			{
				settingsFile.OpenFile(saveFilePath);

				LoadFromFile();

				for (auto&& id : allValidID)
					if (!datas.contains(id))
						datas[id] = GetDefaultValues(id);
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
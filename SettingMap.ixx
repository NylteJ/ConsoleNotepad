// SettingMap.ixx
// �� SettingsHandler �ĺ�� (ʲô����)
// ֻά��һ����ϣ��, ���ڿ��ٲ��Ҷ�Ӧ������
// ֻ��Ҫ����������õ������Ϳ�����, ����Ҳ���õ���ѭ������
// (��Ϊ SettingsHandler ����Ѳ� import Editor �� UI ���, ����ֻ������һ������)
// ʹ���˲��� (���ٶ�����˵) �൱���ŵ�ģ�弼��
// ԭ���϶������ϵĲ�ѯ��ֱ���������, ��Ϊ��ϣ��Ĳ�ѯ����Ҳ�� O(1) ��, Ч���Ϻͻ���һ��������һ����, �����Ӽ�ʱ
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
		// �������ļ������ֲ�ͬ������
		// ��Ӧ�øı��������ֵ, ��������δ��ɾ����ĳЩ��, Ҳ����Ӱ�쵽���е������ļ�
		// Ҳ�ǿ��ǵ���һ��, û�������ַ�������ʽ, ��Ϊ�ַ������ǻ������иĵ�����, ֱ�Ӹĳ�һ��ö�����Ͳ�����
		enum class ID :uint16_t	// ������ôָ��, ��Ȼ x86 �� x64 �������ļ�������ͨ��
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
#pragma region ������, ���ʱ��
		template<ID id>
		class DataTypeHelper { static_assert(false); };

		// �ǳ��ú궨��, ʹ�����ڸ���ճ��
#define ID_TYPE(id_,type_) template<> class DataTypeHelper<id_> { public:using type = type_; };

		// �����ֶ�ָ�����Ȳ�����Ϊ��ʡ�ռ�, ֻ��Ϊ���������ļ���ǰ����ܼ���
		// ʵ������������ָ���ɶ���λ, ����� variant �ĳ��ȶ������, ����ʡ����̿ռ�
		// ������ variant ��ԭ��ֻ��Ϊ�˺������ܼ�����ַ��� / �����������
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
		using SizeDataType = uint16_t;	// 64KB �ĵ������ݴ�С����Ӧ��������ζ�������
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
		
		// ��ʵ�����һ��Ҳû��ϵ, ��Ϊÿ�������ֻ�����һ���¶���, ����ȡ����ʼ����ʵ�ʵĶ�Ӧ����
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

		// ����ɾ��ĳЩ ID ʱ���������
		// û�з����ʹ
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

				if (input.eof())	// ������ ��ȫ�������������� ʱ�Żᴥ�� EOF, ����ֻ������������ж�һ��
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
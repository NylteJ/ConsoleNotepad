// SettingsHandler.ixx
// �ƹܸ������õĴ洢�ͷ���
// �� SettingWindow �ĺ��
export module SettingsHandler;

import std;

import BasicWindow;
import ConsoleTypedef;
import Editor;
import Selector;
import Exceptions;
import Utils;
import StringEncoder;
import FileHandler;
import UIComponent;
import ConsoleHandler;

using namespace std;

export namespace NylteJ
{
	// ���ǵ�����չ��, �ڲ�ʹ��һ��Ч����������������ǰ������ܼ��ݵķ�ʽ
	// (��ǰ����ָ���Ƕ��°汾��������Ȼ�ܶ����󲿷ֹ�������, ���ұ�������ʱҲ���Ḳ���°汾���е�����)
	class SettingsHandler
	{
	public:
		// �趨��
		class SettingItem
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
			};
		public:
			const wstring tipText;
			const ID id;
			const shared_ptr<UIComponent> component;
		public:
			string BytesHead() const
			{
				ostringstream ret{ ios::binary };
				ret << "\x07\x21"sv;	// ��ʼ��־
				// ID
				ret.write(reinterpret_cast<const char*>(&id), sizeof(id));

				return ret.str();
			}

			string ToBytes() const
			{
				ostringstream ret{ ios::binary };
				ret << BytesHead();

				auto writeToRet = [&](auto&& x)
					{
						ret.write(reinterpret_cast<const char*>(&x), sizeof(x));
					};

				if (typeid(*component) == typeid(Selector))
					writeToRet(static_cast<Selector*>(component.get())->GetNowChoose());
				else if (typeid(*component) == typeid(Editor))
				{
					wstring_view str = static_cast<Editor*>(component.get())->GetData();

					istringstream strStream{ WStrToStr(str) };

					using enum ID;
					switch (id)
					{
					case AutoSavingDuration:
					{
						uint32_t duration;
						string unit;
						strStream >> duration >> unit;

						if (strStream.bad())
							throw Exception{ L"�����ʱ���ʽ! ��֧��С����!"sv };

						if (unit == ""sv || unit == "s"sv || unit == "sec"sv || unit == "secs"sv
							|| unit == "second"sv || unit == "seconds"sv)
							writeToRet(duration);
						else if (unit == "min"sv || unit == "mins"sv || unit == "minute" || unit == "minutes")
							writeToRet(duration * 60);
						else if (unit == "h"sv || unit == "hour"sv || unit == "hours"sv)
							writeToRet(duration * 3600);
						else
							throw Exception{ L"δ֪��ʱ�䵥λ! ֻ֧��ʱ / �� / ������Ӣ�ĵ�λ!"sv };
						break;
					}
					case MaxUndoStep:
					{
						uint32_t step;
						strStream >> step;
						if (strStream.bad())
							throw Exception{ L"��������ָ�ʽ!"sv };
						writeToRet(step);
						break;
					}
					case MaxRedoStep:
					{
						uint32_t step;
						strStream >> step;
						if (strStream.bad())
							throw Exception{ L"��������ָ�ʽ!"sv };
						writeToRet(step);
						break;
					}
					case MaxMergeCharUndoRedo:
					{
						uint32_t counts;
						strStream >> counts;
						if (strStream.bad())
							throw Exception{ L"��������ָ�ʽ!"sv };
						writeToRet(counts);
						break;
					}
					default:
						unreachable();
					}
				}
				else
					unreachable();

				return ret.str();
			}

			void FromBytes(string data)
			{
				string head = BytesHead();

				boyer_moore_horspool_searcher searcher = { head.begin(),head.end() };

				auto iter = search(data.begin(), data.end(), searcher);

				if (iter == data.end())
					return;

				istringstream dataStream{ string{ iter + head.size(),data.end() } };

				auto readFromData=[&](auto& x)
					{
						dataStream.read(reinterpret_cast<char*>(&x), sizeof(x));
					};

				if (typeid(*component) == typeid(Selector))
				{
					uint32_t choose;
					readFromData(choose);
					static_cast<Selector*>(component.get())->SetNowChoose(choose);
				}
				else if (typeid(*component) == typeid(Editor))
				{
					using enum ID;
					switch (this->id)
					{
					case AutoSavingDuration:
					{
						uint32_t duration;
						readFromData(duration);
						if (duration % 3600 == 0)
							static_cast<Editor*>(component.get())->SetData(format(L"{} h"sv, duration / 3600));
						else if (duration % 60 == 0)
							static_cast<Editor*>(component.get())->SetData(format(L"{} min"sv, duration / 60));
						else
							static_cast<Editor*>(component.get())->SetData(format(L"{} s"sv, duration));
						break;
					}
					case MaxUndoStep:
					{
						uint32_t step;
						readFromData(step);
						static_cast<Editor*>(component.get())->SetData(to_wstring(step));
						break;
					}
					case MaxRedoStep:
					{
						uint32_t step;
						readFromData(step);
						static_cast<Editor*>(component.get())->SetData(to_wstring(step));
						break;
					}
					case MaxMergeCharUndoRedo:
					{
						uint32_t counts;
						readFromData(counts);
						static_cast<Editor*>(component.get())->SetData(to_wstring(counts));
						break;
					}
					default:
						unreachable();
					}
					static_cast<Editor*>(component.get())->ResetCursor();
				}
				else
					unreachable();
			}

			SettingItem(wstring_view tipText, ID id, shared_ptr<UIComponent> component)
				:tipText(tipText), id(id), component(component)
			{
			}
		};
	public:
		vector<SettingItem> settingList;

		FileHandler saveFile;

		bool editing = false;	// ��ֹһ�߸�����һ�߶�����
	public:	// �������������Ļ�ȡ�ӿ�
		size_t BehaviorWhenErrorEncoding()
		{
			if (editing)
				throw Exception{L"���ȹر����ô���!"sv};

			for (auto&& settingItem : settingList)
				if (settingItem.id == SettingItem::ID::DefaultBehaviorWhenErrorEncoding)
					return static_cast<Selector*>(settingItem.component.get())->GetNowChoose();

			unreachable();
		}
		optional<chrono::seconds> AutoSavingDuration()
		{
			if(editing)
				return nullopt;

			for (auto&& settingItem : settingList)
				if (settingItem.id == SettingItem::ID::AutoSavingDuration)
				{
					istringstream dataStream{ WStrToStr(static_cast<Editor*>(settingItem.component.get())->GetData()) };

					uint32_t duration;
					string unit;

					dataStream >> duration >> unit;

					if (unit == "s")
						return duration * 1s;
					if (unit == "min")
						return duration * 1min;
					if (unit == "h")
						return duration * 1h;

					unreachable();
				}

			unreachable();
		}

		template<typename DataType>
		optional<DataType> UniversalGet(SettingItem::ID id)
		{
			if (editing)
				return nullopt;

			for (auto&& settingItem : settingList)
				if (settingItem.id == id)
				{
					if (typeid(*settingItem.component) == typeid(Editor))
					{
						istringstream dataStream{ WStrToStr(static_cast<Editor*>(settingItem.component.get())->GetData()) };
						DataType ret;

						dataStream >> ret;
						if (dataStream.bad())
							return nullopt;

						return ret;
					}
					else if (typeid(*settingItem.component) == typeid(Selector))
						return static_cast<Selector*>(settingItem.component.get())->GetNowChoose();
				}

			unreachable();
		}
	public:
		void SaveAll()
		{
			string data;

			for (auto&& settingItem : settingList)
				data += settingItem.ToBytes();

			saveFile.Write(data);
		}
		void ReloadAll(filesystem::path saveFilePath)
		{
			try
			{
				saveFile.OpenFile(saveFilePath);
				string binData = saveFile.ReadAsBytes();

				for (auto&& settingItem : settingList)
					settingItem.FromBytes(binData);
			}
			catch (FileOpenFailedException&)
			{
				saveFile.CreateFile(saveFilePath);
				SaveAll();
			}
		}
		void ReloadAll()
		{
			string binData = saveFile.ReadAsBytes();

			for (auto&& settingItem : settingList)
				settingItem.FromBytes(binData);
		}

		SettingsHandler(ConsoleHandler& console, filesystem::path saveFilePath = L".\\ConsoleNotepad.config"sv)
		{
			// ����� drawRange ����ռλ��, ʵ�ʵ� drawRange ����ÿ���ػ�ʱ�� PrintSelf �����
			ConsoleRect drawRange = { {0,0},{console.GetConsoleSize().width,console.GetConsoleSize().height} };

			using enum SettingItem::ID;

			settingList.emplace_back(L"Ĭ�ϱ����޷����ļ�ʱ����Ϊ:"s, DefaultBehaviorWhenErrorEncoding,
				make_shared<Selector>(console, drawRange,
					vector{ L"ֱ���ֶ�ѡ�����"s, L"�ȳ����Զ�ѡ��, ʧ�����ֶ�ѡ��"s, L"ֱ���Զ�ѡ��, ʧ����ǿ�ƴ�"s }));
			settingList.emplace_back(L"�Զ�������:"s, AutoSavingDuration,
				make_shared<Editor>(console, L"3 min"s, drawRange));
			settingList.emplace_back(L"���� (Ctrl + Z) �������� (������Ч, �������༭��):"s, MaxUndoStep,
				make_shared<Editor>(console, L"1024"s, drawRange));
			settingList.emplace_back(L"���� (Ctrl + Y) �������� (������Ч, �������༭��):"s, MaxRedoStep,
				make_shared<Editor>(console, L"1024"s, drawRange));
			settingList.emplace_back(L"���� / ����ʱ���ַ��ں����� (������Ч, �������༭��):"s, MaxMergeCharUndoRedo,
				make_shared<Editor>(console, L"16"s, drawRange));

			ReloadAll(saveFilePath);
		}
	};
}
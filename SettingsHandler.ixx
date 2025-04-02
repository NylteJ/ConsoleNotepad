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
import UIComponent;
import ConsoleHandler;
import SettingMap;

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
			using ID = SettingMap::ID;

			template<ID id>
			using DataType = SettingMap::DataType<id>;
		public:
			const wstring tipText;
			const ID id;
			const shared_ptr<UIComponent> component;
		public:
			SettingMap::StoreType GetValue() const
			{
				using enum ID;

				if (typeid(*component) == typeid(Selector))
					switch (id)
					{
#define NORMAL_CASE(id_) case id_: return static_cast<DataType<id_>>(static_pointer_cast<Selector>(component)->GetNowChoose());

						NORMAL_CASE(DefaultBehaviorWhenErrorEncoding)

#undef NORMAL_CASE
					}
				else if (typeid(*component) == typeid(Editor))
					switch (id)
					{
					case AutoSavingDuration:
					{
						wistringstream strStream{ static_pointer_cast<Editor>(component)->GetData() | ranges::to<wstring>() };
						DataType<AutoSavingDuration> duration;
						wstring unit;
						strStream >> duration >> unit;

						if (strStream.bad())
							throw Exception{ L"�����ʱ���ʽ! ��֧��С����!"sv };

						if (unit == L""sv || unit == L"s"sv || unit == L"sec"sv || unit == L"secs"sv
							|| unit == L"second"sv || unit == L"seconds"sv)
							return duration;
						else if (unit == L"min"sv || unit == L"mins"sv || unit == L"minute" || unit == L"minutes")
							return duration * 60;
						else if (unit == L"h"sv || unit == L"hour"sv || unit == L"hours"sv)
							return duration * 3600;

						throw Exception{ L"δ֪��ʱ�䵥λ! ֻ֧��ʱ / �� / ������Ӣ�ĵ�λ!"sv };
					}

#define NORMAL_CASE(id_) case id_:\
{\
	DataType<id_> ret;\
	wistringstream strStream{ static_pointer_cast<Editor>(component)->GetData() | ranges::to<wstring>() };\
	strStream>>ret;\
	if(strStream.bad())\
		throw Exception{ L"��������ݸ�ʽ!"sv };\
	return ret;\
}
						NORMAL_CASE(MaxUndoStep)
						NORMAL_CASE(MaxRedoStep)
						NORMAL_CASE(MaxMergeCharUndoRedo)
#undef NORMAL_CASE
					}
				unreachable();
			}

			void FromValue(const SettingMap& settingMap)
			{
				using enum ID;

				if (typeid(*component) == typeid(Selector))
					switch (id)
					{
#define NORMAL_CASE(id_) case id_: static_pointer_cast<Selector>(component)->SetNowChoose(settingMap.Get<id_>());return;

						NORMAL_CASE(DefaultBehaviorWhenErrorEncoding)

#undef NORMAL_CASE
					}
				else if (typeid(*component) == typeid(Editor))
					switch (id)
					{
					case AutoSavingDuration:
					{
						DataType<AutoSavingDuration> duration = settingMap.Get<AutoSavingDuration>();

						if (duration % 3600 == 0)
							static_pointer_cast<Editor>(component)->SetData(format(L"{} h"sv, duration / 3600));
						else if (duration % 60 == 0)
							static_pointer_cast<Editor>(component)->SetData(format(L"{} min"sv, duration / 60));
						else
							static_pointer_cast<Editor>(component)->SetData(format(L"{} s"sv, duration));
						
						return;
					}
#define NORMAL_CASE(id_) case id_: static_pointer_cast<Editor>(component)->SetData(to_wstring(settingMap.Get<id_>()));return;

						NORMAL_CASE(MaxUndoStep)
						NORMAL_CASE(MaxRedoStep)
						NORMAL_CASE(MaxMergeCharUndoRedo)

#undef NORMAL_CASE
					}

				unreachable();
			}

			SettingItem(wstring_view tipText, ID id, shared_ptr<UIComponent> component)
				:tipText(tipText), id(id), component(component)
			{
			}
		};
	private:
		SettingMap& settingMap;
	public:
		vector<SettingItem> settingList;
	public:
		void SaveAll()
		{
			for (auto&& settingItem : settingList)
				settingMap.Set(settingItem.id, settingItem.GetValue());

			settingMap.SaveToFile();
		}
		void ReloadAll()
		{
			for (auto&& settingItem : settingList)
				settingItem.FromValue(settingMap);
		}

		SettingsHandler(ConsoleHandler& console, SettingMap& settingMap)
			:settingMap(settingMap)
		{
			// ����� drawRange ����ռλ��, ʵ�ʵ� drawRange ����ÿ���ػ�ʱ�� PrintSelf �����
			ConsoleRect drawRange = { {0,0},{console.GetConsoleSize().width,console.GetConsoleSize().height} };

			using enum SettingItem::ID;

			settingList.emplace_back(L"Ĭ�ϱ����޷����ļ�ʱ����Ϊ:"s, DefaultBehaviorWhenErrorEncoding,
				make_shared<Selector>(console, drawRange,
					vector{ L"ֱ���ֶ�ѡ�����"s, L"�ȳ����Զ�ѡ��, ʧ�����ֶ�ѡ��"s, L"ֱ���Զ�ѡ��, ʧ����ǿ�ƴ�"s }));
			settingList.emplace_back(L"�Զ�������:"s, AutoSavingDuration,
				make_shared<Editor>(console, L""s, drawRange, settingMap));
			settingList.emplace_back(L"���� (Ctrl + Z) �������� (�����༭����������):"s, MaxUndoStep,
				make_shared<Editor>(console, L""s, drawRange, settingMap));
			settingList.emplace_back(L"���� (Ctrl + Y) �������� (�����༭����������):"s, MaxRedoStep,
				make_shared<Editor>(console, L""s, drawRange, settingMap));
			settingList.emplace_back(L"���� / ����ʱ���ַ��ں����� (�����༭����������):"s, MaxMergeCharUndoRedo,
				make_shared<Editor>(console, L""s, drawRange, settingMap));

			ReloadAll();
		}
	};
}
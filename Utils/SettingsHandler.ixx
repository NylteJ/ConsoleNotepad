// SettingsHandler.ixx
// �ƹܸ������õĴ洢�ͷ���
// �� SettingWindow �ĺ��
// (��˵�꺢����ģ�������������)
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
	// (��ǰ����ָ���Ƕ��°汾��������Ȼ�ܶ�����������)
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
						NORMAL_CASE(CloseHistoryWindowAfterEnter)
						NORMAL_CASE(SplitUndoStrWhenEnter)
						NORMAL_CASE(NormalExitWhenDoubleEsc)

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
#define WSTRING_CASE(id_) case id_: return static_pointer_cast<Editor>(component)->GetData() | ranges::to<wstring>();

						NORMAL_CASE(MaxUndoStep)
						NORMAL_CASE(MaxRedoStep)
						NORMAL_CASE(MaxMergeCharUndoRedo)
						WSTRING_CASE(AutoSaveFileExtension)
						WSTRING_CASE(NewFileAutoSaveName)
						NORMAL_CASE(LineIndexWidth)
						NORMAL_CASE(TabWidth)

#undef WSTRING_CASE
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
						NORMAL_CASE(CloseHistoryWindowAfterEnter)
						NORMAL_CASE(SplitUndoStrWhenEnter)
						NORMAL_CASE(NormalExitWhenDoubleEsc)

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
#define WSTRING_CASE(id_) case id_: static_pointer_cast<Editor>(component)->SetData(settingMap.Get<id_>());return;

						NORMAL_CASE(MaxUndoStep)
						NORMAL_CASE(MaxRedoStep)
						NORMAL_CASE(MaxMergeCharUndoRedo)
						WSTRING_CASE(AutoSaveFileExtension)
						WSTRING_CASE(NewFileAutoSaveName)
						NORMAL_CASE(LineIndexWidth)
						NORMAL_CASE(TabWidth)

#undef WSTRING_CASE
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

#define EDITOR_CASE(id_, tipText_) settingList.emplace_back(L##tipText_##s, id_, make_shared<Editor>(console, L""s, drawRange, settingMap))
#define SELECTOR_CASE(id_, tipText_, ...) settingList.emplace_back(L##tipText_##s, id_, make_shared<Selector>(console, drawRange, vector{ __VA_ARGS__ }))

			SELECTOR_CASE(DefaultBehaviorWhenErrorEncoding, "Ĭ�ϱ����޷����ļ�ʱ����Ϊ:", L"ֱ���ֶ�ѡ�����"s, L"�ȳ����Զ�ѡ��, ʧ�����ֶ�ѡ��"s, L"ֱ���Զ�ѡ��, ʧ����ǿ�ƴ�"s);
			EDITOR_CASE(AutoSavingDuration, "�Զ�������:");
			EDITOR_CASE(MaxUndoStep, "���� (Ctrl + Z) �������� (�����༭����������):");
			EDITOR_CASE(MaxRedoStep, "���� (Ctrl + Y) �������� (�����༭����������):");
			EDITOR_CASE(MaxMergeCharUndoRedo, "���� / ����ʱ�ĵ����ַ��ں�����:");
			EDITOR_CASE(AutoSaveFileExtension, "�Զ������ļ��ĺ�׺��:");
			EDITOR_CASE(NewFileAutoSaveName, "���ļ����Զ������ļ���:");
			SELECTOR_CASE(CloseHistoryWindowAfterEnter, "���»س����Ƿ�ر���ʷ��¼����:", L"���ر�"s, L"�ر�"s);
			SELECTOR_CASE(SplitUndoStrWhenEnter, "����ʱ�Ƿ��ںϳ��� / ��������:", L"�����ں�"s, L"������������ʱ�ں�"s, L"�ں�"s);
			SELECTOR_CASE(NormalExitWhenDoubleEsc, "δ���沢˫�� Esc ǿ���˳�ʱ:", L"�����쳣�˳� (�����Զ������ļ�)"s, L"���������˳� (ɾ���Զ������ļ�)"s);
			EDITOR_CASE(LineIndexWidth, "�кſ�� (��������, ����Ϊ 0 �Թر��к���ʾ):");
			EDITOR_CASE(TabWidth, "Tab ��� (����Ϊ 1):");

#undef SELECTOR_CASE
#undef EDITOR_CASE

			ReloadAll();
		}
	};
}
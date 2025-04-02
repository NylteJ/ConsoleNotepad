// SettingsHandler.ixx
// 掌管各种设置的存储和访问
// 是 SettingWindow 的后端
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
	// 考虑到可拓展性, 内部使用一套效率略慢但理论上向前、向后都能兼容的方式
	// (向前兼容指的是读新版本的设置仍然能读出大部分共有设置, 而且保存设置时也不会覆盖新版本特有的设置)
	class SettingsHandler
	{
	public:
		// 设定项
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
							throw Exception{ L"错误的时间格式! 不支持小数点!"sv };

						if (unit == L""sv || unit == L"s"sv || unit == L"sec"sv || unit == L"secs"sv
							|| unit == L"second"sv || unit == L"seconds"sv)
							return duration;
						else if (unit == L"min"sv || unit == L"mins"sv || unit == L"minute" || unit == L"minutes")
							return duration * 60;
						else if (unit == L"h"sv || unit == L"hour"sv || unit == L"hours"sv)
							return duration * 3600;

						throw Exception{ L"未知的时间单位! 只支持时 / 分 / 秒三种英文单位!"sv };
					}

#define NORMAL_CASE(id_) case id_:\
{\
	DataType<id_> ret;\
	wistringstream strStream{ static_pointer_cast<Editor>(component)->GetData() | ranges::to<wstring>() };\
	strStream>>ret;\
	if(strStream.bad())\
		throw Exception{ L"错误的数据格式!"sv };\
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
			// 下面的 drawRange 都是占位符, 实际的 drawRange 会在每次重绘时在 PrintSelf 里计算
			ConsoleRect drawRange = { {0,0},{console.GetConsoleSize().width,console.GetConsoleSize().height} };

			using enum SettingItem::ID;

			settingList.emplace_back(L"默认编码无法打开文件时的行为:"s, DefaultBehaviorWhenErrorEncoding,
				make_shared<Selector>(console, drawRange,
					vector{ L"直接手动选择编码"s, L"先尝试自动选择, 失败再手动选择"s, L"直接自动选择, 失败则强制打开"s }));
			settingList.emplace_back(L"自动保存间隔:"s, AutoSavingDuration,
				make_shared<Editor>(console, L""s, drawRange, settingMap));
			settingList.emplace_back(L"撤销 (Ctrl + Z) 步数上限 (各个编辑器单独计算):"s, MaxUndoStep,
				make_shared<Editor>(console, L""s, drawRange, settingMap));
			settingList.emplace_back(L"重做 (Ctrl + Y) 步数上限 (各个编辑器单独计算):"s, MaxRedoStep,
				make_shared<Editor>(console, L""s, drawRange, settingMap));
			settingList.emplace_back(L"撤销 / 重做时的字符融合上限 (各个编辑器单独计算):"s, MaxMergeCharUndoRedo,
				make_shared<Editor>(console, L""s, drawRange, settingMap));

			ReloadAll();
		}
	};
}
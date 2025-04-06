// SettingsHandler.ixx
// 掌管各种设置的存储和访问
// 是 SettingWindow 的后端
// (我说宏孩儿是模板祖宗你二龙吗)
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
	// (向前兼容指的是读新版本的设置仍然能读出共有设置)
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
			// 下面的 drawRange 都是占位符, 实际的 drawRange 会在每次重绘时在 PrintSelf 里计算
			ConsoleRect drawRange = { {0,0},{console.GetConsoleSize().width,console.GetConsoleSize().height} };

			using enum SettingItem::ID;

#define EDITOR_CASE(id_, tipText_) settingList.emplace_back(L##tipText_##s, id_, make_shared<Editor>(console, L""s, drawRange, settingMap))
#define SELECTOR_CASE(id_, tipText_, ...) settingList.emplace_back(L##tipText_##s, id_, make_shared<Selector>(console, drawRange, vector{ __VA_ARGS__ }))

			SELECTOR_CASE(DefaultBehaviorWhenErrorEncoding, "默认编码无法打开文件时的行为:", L"直接手动选择编码"s, L"先尝试自动选择, 失败再手动选择"s, L"直接自动选择, 失败则强制打开"s);
			EDITOR_CASE(AutoSavingDuration, "自动保存间隔:");
			EDITOR_CASE(MaxUndoStep, "撤销 (Ctrl + Z) 步数上限 (各个编辑器单独计算):");
			EDITOR_CASE(MaxRedoStep, "重做 (Ctrl + Y) 步数上限 (各个编辑器单独计算):");
			EDITOR_CASE(MaxMergeCharUndoRedo, "撤销 / 重做时的单步字符融合上限:");
			EDITOR_CASE(AutoSaveFileExtension, "自动保存文件的后缀名:");
			EDITOR_CASE(NewFileAutoSaveName, "新文件的自动保存文件名:");
			SELECTOR_CASE(CloseHistoryWindowAfterEnter, "按下回车后是否关闭历史记录窗口:", L"不关闭"s, L"关闭"s);
			SELECTOR_CASE(SplitUndoStrWhenEnter, "换行时是否融合撤销 / 重做操作:", L"永不融合"s, L"仅在连续换行时融合"s, L"融合"s);
			SELECTOR_CASE(NormalExitWhenDoubleEsc, "未保存并双击 Esc 强制退出时:", L"视作异常退出 (保留自动保存文件)"s, L"视作正常退出 (删除自动保存文件)"s);
			EDITOR_CASE(LineIndexWidth, "行号宽度 (不含竖线, 设置为 0 以关闭行号显示):");
			EDITOR_CASE(TabWidth, "Tab 宽度 (至少为 1):");

#undef SELECTOR_CASE
#undef EDITOR_CASE

			ReloadAll();
		}
	};
}
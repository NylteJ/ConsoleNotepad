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
import FileHandler;
import UIComponent;
import ConsoleHandler;

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
			// 用于在文件中区分不同设置项
			// 不应该改变现有项的值, 这样就算未来删除了某些项, 也不会影响到现有的设置文件
			// 也是考虑到这一点, 没有做成字符串的形式, 因为字符串总是会让人有改的欲望, 直接改成一个枚举数就不会了
			enum class ID :uint16_t	// 必须这么指定, 不然 x86 和 x64 的配置文件还不能通用
			{
				DefaultBehaviorWhenErrorEncoding = 0,
				AutoSavingDuration = 1,
			};
		public:
			const wstring tipText;
			const ID id;
			const shared_ptr<UIComponent> component;
		public:
			string BytesHead() const
			{
				ostringstream ret{ ios::binary };
				ret << "\x07\x21"sv;	// 开始标志
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
						size_t duration;
						string unit;
						strStream >> duration >> unit;

						if (strStream.bad())
							throw Exception{ L"错误的时间格式! 不支持小数点!"sv };

						if (unit == ""sv || unit == "s"sv || unit == "sec"sv || unit == "secs"sv
							|| unit == "second"sv || unit == "seconds"sv)
							writeToRet(duration);
						else if (unit == "min"sv || unit == "mins"sv || unit == "minute" || unit == "minutes")
							writeToRet(duration * 60);
						else if (unit == "h"sv || unit == "hour"sv || unit == "hours"sv)
							writeToRet(duration * 3600);
						else
							throw Exception{ L"未知的时间单位! 只支持时 / 分 / 秒三种英文单位!"sv };
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
					size_t choose;
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
						size_t duration;
						readFromData(duration);
						if (duration % 3600 == 0)
							static_cast<Editor*>(component.get())->SetData(format(L"{} h"sv, duration / 3600));
						else if (duration % 60 == 0)
							static_cast<Editor*>(component.get())->SetData(format(L"{} min"sv, duration / 60));
						else
							static_cast<Editor*>(component.get())->SetData(format(L"{} s"sv, duration));
						break;
					}
					default:
						unreachable();
					}
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

		bool editing = false;	// 防止一边改设置一边读设置
	public:	// 各个设置项对外的获取接口
		optional<chrono::seconds> AutoSavingDuration()
		{
			if(editing)
				return nullopt;

			for (auto&& settingItem : settingList)
				if (settingItem.id == SettingItem::ID::AutoSavingDuration)
				{
					istringstream dataStream{ WStrToStr(static_cast<Editor*>(settingItem.component.get())->GetData()) };

					size_t duration;
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
			return 0s;
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
			// 下面的 drawRange 都是占位符, 实际的 drawRange 会在每次重绘时在 PrintSelf 里计算
			ConsoleRect drawRange = { {0,0},{console.GetConsoleSize().width,console.GetConsoleSize().height} };

			using enum SettingItem::ID;

			settingList.emplace_back(L"默认编码无法打开文件时的行为 (W.I.P):"s, DefaultBehaviorWhenErrorEncoding,
				make_shared<Selector>(console, drawRange,
					vector{ L"直接手动选择编码"s, L"先尝试自动选择, 失败再手动选择"s, L"直接自动选择, 失败则强制打开"s }));
			settingList.emplace_back(L"自动保存间隔:"s, AutoSavingDuration,
				make_shared<Editor>(console, L"3 min"s, drawRange));

			ReloadAll(saveFilePath);
		}
	};
}
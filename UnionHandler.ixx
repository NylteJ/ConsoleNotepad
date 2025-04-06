// UnionHandler.ixx
// 简单地统一那一大堆 Handler
// 主要是为了可拓展地传给 UIComponent 之类的
export module UnionHandler;

import std;

import FileHandler;
import ClipboardHandler;
import ConsoleHandler;
import InputHandler;
import UIHandler;
import SettingMap;

using namespace std;

export namespace NylteJ
{
	class UIComponent;

	class UnionHandler
	{
	public:
		ConsoleHandler& console;
		InputHandler& input;
		FileHandler& file;
		ClipboardHandler& clipboard;
		UIHandler<shared_ptr<UIComponent>>& ui;		// shared_ptr 可以使用不完整类型, 所以这里直接用就可以了
		SettingMap& settings;
	public:
		UnionHandler(auto&& consoleHandler, auto&& inputHandler, auto&& fileHandler, auto&& clipboardHandler, auto&& uiHandler, auto&& settings)
			:console(consoleHandler), input(inputHandler), file(fileHandler), clipboard(clipboardHandler), ui(uiHandler), settings(settings)
		{
		}
	};
}
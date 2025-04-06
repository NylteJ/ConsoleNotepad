// UnionHandler.ixx
// �򵥵�ͳһ��һ��� Handler
// ��Ҫ��Ϊ�˿���չ�ش��� UIComponent ֮���
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
		UIHandler<shared_ptr<UIComponent>>& ui;		// shared_ptr ����ʹ�ò���������, ��������ֱ���þͿ�����
		SettingMap& settings;
	public:
		UnionHandler(auto&& consoleHandler, auto&& inputHandler, auto&& fileHandler, auto&& clipboardHandler, auto&& uiHandler, auto&& settings)
			:console(consoleHandler), input(inputHandler), file(fileHandler), clipboard(clipboardHandler), ui(uiHandler), settings(settings)
		{
		}
	};
}
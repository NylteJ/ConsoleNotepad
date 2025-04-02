// UnionHandler.ixx
// �򵥵�ͳһ��һ��� Handler
// ��Ҫ��Ϊ�˿���չ�ش��� UIComponent ֮���
export module UnionHandler;

import FileHandler;
import ClipboardHandler;
import ConsoleHandler;
import InputHandler;
import UIHandler;
import SettingMap;

export namespace NylteJ
{
	// ���ﲻ��ֱ��д���� (using UIComponentPtr = shared_ptr<UIComponent>), �����ѭ������
	// ����Ϊ���õ�ʱ�򷽱�, ���������� UIComponent.ixx ����, ���ﶨ�����һ�������
	template<typename UIComponentPtr>
	class UnionHandlerInterface
	{
	public:
		ConsoleHandler& console;
		InputHandler& input;
		FileHandler& file;
		ClipboardHandler& clipboard;
		UIHandler<UIComponentPtr>& ui;
		SettingMap& settings;
	public:
		UnionHandlerInterface(auto&& consoleHandler, auto&& inputHandler, auto&& fileHandler, auto&& clipboardHandler, auto&& uiHandler, auto&& settings)
			:console(consoleHandler), input(inputHandler), file(fileHandler), clipboard(clipboardHandler), ui(uiHandler), settings(settings)
		{
		}
	};
}
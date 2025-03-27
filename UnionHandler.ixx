// UnionHandler.ixx
// 简单地统一那一大堆 Handler
// 主要是为了可拓展地传给 UIComponent 之类的
export module UnionHandler;

import FileHandler;
import ClipboardHandler;
import ConsoleHandler;
import InputHandler;
import UIHandler;

export namespace NylteJ
{
	// 这里不能直接写出来 (using UIComponentPtr = shared_ptr<UIComponent>), 否则会循环依赖
	// 所以为了用的时候方便, 别名定义在 UIComponent.ixx 里了, 这里定义个长一点的名字
	template<typename UIComponentPtr>
	class UnionHandlerInterface
	{
	public:
		ConsoleHandler& console;
		InputHandler& input;
		FileHandler& file;
		ClipboardHandler& clipboard;
		UIHandler<UIComponentPtr>& ui;
	public:
		UnionHandlerInterface(auto&& consoleHandler, auto&& inputHandler, auto&& fileHandler, auto&& clipboardHandler, auto&& uiHandler)
			:console(consoleHandler), input(inputHandler), file(fileHandler), clipboard(clipboardHandler), ui(uiHandler)
		{
		}
	};
}
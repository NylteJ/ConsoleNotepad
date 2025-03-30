// UIComponent.ixx
// 纯虚接口类, 实际上包括了好几个范畴, 不过这里写一起了: 
// 1. 表示一个类可以接收来自 InputHandler 的输入
//    需要把三种都重载了才能实例化. 就算提供了默认实现, 只要子类重写一个函数, 父类的其它同名函数都会被掩盖过去, 故没有提供默认实现
// 2. 表示一个类可以获得焦点 (Focus) 和轻量化地重新获得焦点 (Refocus)
//    这个有默认实现 (什么都不做)						(执行 WhenFocused)
export module UIComponent;

import std;

import InputHandler;
import UnionHandler;

using namespace std;

export namespace NylteJ
{
	// 循环依赖, 堂堂解决!	(不懂的可以顺着 UnionHandler 往前看)
	class UIComponent
	{
	public:
		virtual void ManageInput(const InputHandler::MessageWindowSizeChanged& message, UnionHandlerInterface<shared_ptr<UIComponent>>& handlers) = 0;
		virtual void ManageInput(const InputHandler::MessageKeyboard& message, UnionHandlerInterface<shared_ptr<UIComponent>>& handlers) = 0;
		virtual void ManageInput(const InputHandler::MessageMouse& message, UnionHandlerInterface<shared_ptr<UIComponent>>& handlers) = 0;

		virtual void WhenFocused() {}
		virtual void WhenRefocused()
		{
			WhenFocused();
		}

		virtual ~UIComponent() = default;
	};

	using UnionHandler = UnionHandlerInterface<shared_ptr<UIComponent>>;
}
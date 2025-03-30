// UIComponent.ixx
// ����ӿ���, ʵ���ϰ����˺ü�������, ��������дһ����: 
// 1. ��ʾһ������Խ������� InputHandler ������
//    ��Ҫ�����ֶ������˲���ʵ����. �����ṩ��Ĭ��ʵ��, ֻҪ������дһ������, ���������ͬ���������ᱻ�ڸǹ�ȥ, ��û���ṩĬ��ʵ��
// 2. ��ʾһ������Ի�ý��� (Focus) �������������»�ý��� (Refocus)
//    �����Ĭ��ʵ�� (ʲô������)						(ִ�� WhenFocused)
export module UIComponent;

import std;

import InputHandler;
import UnionHandler;

using namespace std;

export namespace NylteJ
{
	// ѭ������, ���ý��!	(�����Ŀ���˳�� UnionHandler ��ǰ��)
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
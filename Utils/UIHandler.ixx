// UIHandler.ixx
// ������� UI ģ��Ĳ㼶��ϵ
// ͬʱ����һ��ָ���� Editor ������
// û��д�� UI �����ԭ���Ǳ���ѭ������ (����ʱ����Ҫ UI ģ���Լ������Ƿ�ɾ���Լ�, ���Եð���� Handler һ�𴫽�ȥ, д�� UI.ixx ���ѭ��������)
// Ҳ������ Handler ��ԭ�򴿴���ǿ��֢, ����ô������ UnionHandler �￴������
export module UIHandler;

import std;

using namespace std;

export namespace NylteJ
{
	class Editor;

	// ���ﲻ��ֱ��д���� (using UIComponentPtr = shared_ptr<UIComponent>), ����Ҳ��ѭ������ (���û�취, ��������û���ò���������)
	// ���� Editor Ҳ��......����ֻ���������Ի��ý��
	template<typename UIComponentPtr>
	class UIHandler
	{
	private:
		using Depth = int;
	public:
		// ��ʵ���ﱾ������д�����ƹ�դ��Ⱦ����Ч���� (ָ������Ȳ���, ʵʱ��Ⱦ����), ������д��д�ž���ȫû���Ƿ���д��, ���д������ȫ���Ի��Ƶ�Ч��
		// ���������������ô�������, ��Ȼ����һ������
		constexpr static Depth mainEditorDepth = 0;
		constexpr static Depth normalWindowDepth = 10;
	public:
		multimap<Depth, UIComponentPtr> components;

		UIComponentPtr nowFocus;

		Editor& mainEditor;
	public:
		void GiveFocusTo(UIComponentPtr componentPtr)
		{
			if (nowFocus != nullptr)
				nowFocus->WhenUnfocused();

			nowFocus = componentPtr;

			if (nowFocus != nullptr)
				nowFocus->WhenFocused();
		}

		void Refocus()
		{
			if (nowFocus != nullptr)
				nowFocus->WhenRefocused();
		}

		void EraseComponent(auto&& componentPtr)
		{
			for (auto iter = components.begin(); iter != components.end();)
				if (iter->second == componentPtr)
					iter = components.erase(iter);
				else
					++iter;

			if (nowFocus == componentPtr)
				if (components.empty())
					GiveFocusTo(nullptr);
				else
					GiveFocusTo(components.rbegin()->second);
		}

		UIHandler(Editor& editor)
			:mainEditor(editor)
		{
		}
	};
}
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

	// ���ﲻ��ֱ��д���� (using UIComponentPtr = shared_ptr<UIComponent>), ����Ҳ��ѭ������
	// ���� Editor Ҳ��......����ֻ���������Ի��ý��
	template<typename UIComponentPtr>
	class UIHandler
	{
	private:
		using Depth = int;
	public:
		constexpr static Depth mainEditorDepth = 0;
		constexpr static Depth normalWindowDepth = 10;
		constexpr static Depth hideDepth = numeric_limits<Depth>::lowest();		// ���⴦��, ֱ�Ӳ���ʾ
	public:
		multimap<Depth, UIComponentPtr> components;

		UIComponentPtr nowFocus;

		Editor& mainEditor;
	public:
		void GiveFocusTo(UIComponentPtr compontentPtr)
		{
			nowFocus = compontentPtr;

			if (nowFocus != nullptr)
				nowFocus->WhenFocused();
		}

		void Refocus()
		{
			if (nowFocus != nullptr)
				nowFocus->WhenFocused();
		}

		void EraseComponent(auto&& compontentPtr)
		{
			for (auto iter = components.begin(); iter != components.end();)
				if (iter->second.get() == compontentPtr)
					iter = components.erase(iter);
				else
					++iter;

			if (nowFocus.get() == compontentPtr)
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
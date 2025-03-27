// UIHandler.ixx
// 处理各个 UI 模块的层级关系
// 同时保存一个指向主 Editor 的引用
// 没有写进 UI 类里的原因是避免循环引用 (多数时候需要 UI 模块自己决定是否删除自己, 所以得把这个 Handler 一起传进去, 写进 UI.ixx 里就循环引用了)
// 也起名叫 Handler 的原因纯粹是强迫症, 不这么起塞到 UnionHandler 里看着难受
export module UIHandler;

import std;

using namespace std;

export namespace NylteJ
{
	class Editor;

	// 这里不能直接写出来 (using UIComponentPtr = shared_ptr<UIComponent>), 否则也会循环依赖
	// 还有 Editor 也是......好在只传引用所以还好解决
	template<typename UIComponentPtr>
	class UIHandler
	{
	private:
		using Depth = int;
	public:
		constexpr static Depth mainEditorDepth = 0;
		constexpr static Depth normalWindowDepth = 10;
		constexpr static Depth hideDepth = numeric_limits<Depth>::lowest();		// 特殊处理, 直接不显示
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
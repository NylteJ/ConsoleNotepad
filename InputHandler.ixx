// Inputhandler.ixx
// 用于处理输入的模块
// 是跨平台的
// 使用回调函数发送数据
export module InputHandler;

import std;

import ConsoleTypedef;
import String;

using namespace std;

export namespace NylteJ
{
	// TODO: profile 表明这里的加锁解锁非常、非常耗费性能
	// 试着改成无锁队列?
	class InputHandler
	{
	private:
		template<typename MessageType>
		class MessageDatas
		{
		public:
			vector<function<void(const MessageType&, size_t)>> callbacks;
			shared_mutex callbacksMutex;
		};
	public:
		class MessageWindowSizeChanged
		{
		public:
			ConsoleSize newSize;
		public:
            bool operator==(const MessageWindowSizeChanged&) const = default;
		};
		class MessageKeyboard
		{
		public:
			enum class Key
			{
				Num0 = '0',
				Num1, Num2, Num3, Num4, Num5, Num6, Num7, Num8, Num9,

				A = 'A',
				B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

				Numpad0 = 0x60,
				Numpad1, Numpad2, Numpad3, Numpad4, Numpad5, Numpad6, Numpad7, Numpad8, Numpad9,

				F1 = 0x70,
				F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,

				Tab = 0x09,
				Space = 0x20,
				Backspace = 0x08,
				Enter = 0x0D,
				Esc = 0x1B,
				Delete = 0x2E,

				Left = 0x25, Up, Right, Down,

				Home = 0x24,
				End = 0x23,
				PageUp = 0x21,
				PageDown = 0x22,

				Special1 = 0xDB, 	// [{
				Special2 = 0xDD		// ]}
			} key;
			class ExtraKeys
			{
			private:
				bitset<8> data;
			public:
				bool RightAlt() const
				{
					return data[0];
				}
				bool LeftAlt() const
				{
					return data[1];
				}
				bool Alt() const
				{
					return RightAlt() || LeftAlt();
				}

				bool RightCtrl() const
				{
					return data[2];
				}
				bool LeftCtrl() const
				{
					return data[3];
				}
				bool Ctrl() const
				{
					return LeftCtrl() || RightCtrl();
				}

				bool Shift() const
				{
					return data[4];
				}

				bool NumLock() const
				{
					return data[5];
				}

				bool ScrollLock() const
				{
					return data[6];
				}

				bool Caplock() const
				{
					return data[7];
				}

				auto& RawData()
				{
					return data;
				}

				bool operator==(const ExtraKeys&) const = default;
			} extraKeys;
			Codepoint inputChar = 0;
		public:
			bool operator==(const MessageKeyboard&) const = default;
		};
		class MessageMouse
		{
		public:
			enum class Type
			{
				VWheeled, HWheeled,
				Clicked, DoubleClicked,
				Moved,
				Released
			};
		public:
			Type type;
			bitset<2> buttonStatus;
			int wheelMove = 0;
			ConsolePosition position;
		public:
			bool LeftHold() const
			{
				return buttonStatus[0];
			}
			bool LeftClick() const
			{
				return buttonStatus[0] && (type == Type::Clicked || type == Type::DoubleClicked);
			}
			bool RightClick() const
			{
				return buttonStatus[1] && (type == Type::Clicked || type == Type::DoubleClicked);	// 第二次点击被标记为双击而非单击
			}

			int WheelMove() const
			{
				return wheelMove;
			}

			bool operator==(const MessageMouse&) const = default;
		};
	private:
		MessageDatas<MessageWindowSizeChanged> windowSizeChangedMessages;
		MessageDatas<MessageKeyboard> keyboardMessages;
		MessageDatas<MessageMouse> mouseMessages;
	public:
		// TODO: 把这一堆复制粘贴用更优雅的方式实现
		void SubscribeMessage(function<void(const MessageWindowSizeChanged&, size_t)> callback)
		{
			lock_guard _{ windowSizeChangedMessages.callbacksMutex };

			windowSizeChangedMessages.callbacks.emplace_back(std::move(callback));
		}
		void SubscribeMessage(function<void(const MessageKeyboard&, size_t)> callback)
		{
			lock_guard _{ keyboardMessages.callbacksMutex };

			keyboardMessages.callbacks.emplace_back(std::move(callback));
		}
		void SubscribeMessage(function<void(const MessageMouse&, size_t)> callback)
		{
			lock_guard _{ mouseMessages.callbacksMutex };

			mouseMessages.callbacks.emplace_back(std::move(callback));
		}

		void SendMessage(const MessageWindowSizeChanged& message, size_t count = 1)
		{
			shared_lock _{ windowSizeChangedMessages.callbacksMutex };

			for (auto&& callback : windowSizeChangedMessages.callbacks)
				callback(message, count);
		}
		void SendMessage(const MessageKeyboard& message, size_t count = 1)
		{
			shared_lock _{ keyboardMessages.callbacksMutex };

			for (auto&& callback : keyboardMessages.callbacks)
				callback(message, count);
		}
		void SendMessage(const MessageMouse& message, size_t count = 1)
		{
			shared_lock _{ mouseMessages.callbacksMutex };

			for (auto&& callback : mouseMessages.callbacks)
				callback(message, count);
		}

		InputHandler()
		{
		}
	};
}
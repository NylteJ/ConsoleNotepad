// Inputhandler.ixx
// 用于处理输入的模块
// 是跨平台的
// 使用回调函数发送数据
export module InputHandler;

import std;

import ConsoleTypedef;

using namespace std;

export namespace NylteJ
{
	class InputHandler
	{
	private:
		template<typename MessageType>
		class MessageDatas
		{
		public:
			queue<MessageType> messagesQueue;
			mutex messagesMutex;

			vector<function<void(const MessageType&)>> callbacks;
			mutex callbacksMutex;
		public:
		};
	public:
		class MessageWindowSizeChanged
		{
		public:
			ConsoleSize newSize;
		};
		class MessageKeyboard
		{
		public:
			enum class Key :char
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
			} extraKeys;
			wchar_t inputChar = L'\0';
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
				return buttonStatus[0] && type == Type::Clicked;
			}
			bool RightClick() const
			{
				return buttonStatus[1] && type == Type::Clicked;
			}

			int WheelMove() const
			{
				return wheelMove;
			}
		};
	private:
		MessageDatas<MessageWindowSizeChanged> windowSizeChangedMessages;
		MessageDatas<MessageKeyboard> keyboardMessages;
		MessageDatas<MessageMouse> mouseMessages;
	private:
		[[noreturn]] void DistributeMessages()
		{
			while (true)
			{
				{
					lock_guard messagesLock{ windowSizeChangedMessages.messagesMutex }, CallbacksLock{ windowSizeChangedMessages.callbacksMutex };

					while (!windowSizeChangedMessages.messagesQueue.empty())
					{
						for (auto& func : windowSizeChangedMessages.callbacks)
							func(windowSizeChangedMessages.messagesQueue.front());

						windowSizeChangedMessages.messagesQueue.pop();
					}
				}
				{
					lock_guard messagesLock{ keyboardMessages.messagesMutex }, CallbacksLock{ keyboardMessages.callbacksMutex };

					while (!keyboardMessages.messagesQueue.empty())
					{
						for (auto& func : keyboardMessages.callbacks)
							func(keyboardMessages.messagesQueue.front());

						keyboardMessages.messagesQueue.pop();
					}
				}
				{
					lock_guard messagesLock{ mouseMessages.messagesMutex }, CallbacksLock{ mouseMessages.callbacksMutex };

					while (!mouseMessages.messagesQueue.empty())
					{
						for (auto& func : mouseMessages.callbacks)
							func(mouseMessages.messagesQueue.front());

						mouseMessages.messagesQueue.pop();
					}
				}
			}
		}
	public:
		// TODO: 把这一堆复制粘贴用更优雅的方式实现
		void SubscribeMessage(function<void(const MessageWindowSizeChanged&)> callback)
		{
			lock_guard callbackLock{ windowSizeChangedMessages.callbacksMutex };
			windowSizeChangedMessages.callbacks.emplace_back(callback);
		}
		void SubscribeMessage(function<void(const MessageKeyboard&)> callback)
		{
			lock_guard callbackLock{ keyboardMessages.callbacksMutex };
			keyboardMessages.callbacks.emplace_back(callback);
		}
		void SubscribeMessage(function<void(const MessageMouse&)> callback)
		{
			lock_guard callbackLock{ mouseMessages.callbacksMutex };
			mouseMessages.callbacks.emplace_back(callback);
		}

		void SendMessage(const MessageWindowSizeChanged& message)
		{
			lock_guard messagesLock{ windowSizeChangedMessages.messagesMutex };
			windowSizeChangedMessages.messagesQueue.emplace(message);
		}
		void SendMessage(const MessageKeyboard& message, size_t count = 1)
		{
			lock_guard messagesLock{ keyboardMessages.messagesMutex };

			for (size_t i = 0; i < count; i++)
				keyboardMessages.messagesQueue.emplace(message);
		}
		void SendMessage(const MessageMouse& message)
		{
			lock_guard messagesLock{ mouseMessages.messagesMutex };
			mouseMessages.messagesQueue.emplace(message);
		}

		InputHandler()
		{
			thread{ bind(&InputHandler::DistributeMessages,this) }.detach();
		}
	};
}
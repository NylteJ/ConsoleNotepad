// Inputhandler.ixx
// ���ڴ��������ģ��
// �ǿ�ƽ̨��
// ʹ�ûص�������������
export module InputHandler;

import std;

import ConsoleTypedef;

using namespace std;

export namespace NylteJ
{
	class InputHandler
	{
	public:
		class MessageWindowSizeChanged
		{
		public:
			ConsoleSize newSize;
		};
	private:
		queue<MessageWindowSizeChanged> messagesWindowSizeChanged;	// ���Ҫ����
		mutex mutexMWSC;

		vector<function<void(const MessageWindowSizeChanged&)>> callbacksWindowSizeChanged;	// ֻ�� DistributeMessages �����, ������
		mutex mutexCWSC;
	private:
		void DistributeMessages()
		{
			while (true)
			{
				{
					lock_guard messagesLock{ mutexMWSC }, CallbacksLock{ mutexCWSC };

					while (!messagesWindowSizeChanged.empty())
					{
						for (auto& func : callbacksWindowSizeChanged)
							func(messagesWindowSizeChanged.front());

						messagesWindowSizeChanged.pop();
					}
				}
			}
		}
	public:
		void SubscribeMessage(function<void(const MessageWindowSizeChanged&)> callback)
		{
			lock_guard callbackLock{ mutexCWSC };
			callbacksWindowSizeChanged.emplace_back(callback);
		}

		void SendMessage(const MessageWindowSizeChanged& message)
		{
			lock_guard messagesLock{ mutexMWSC };
			messagesWindowSizeChanged.emplace(message);
		}

		InputHandler()
		{
			thread{ bind(&InputHandler::DistributeMessages,this) }.detach();
		}
	};
}
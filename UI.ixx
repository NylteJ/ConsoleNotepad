// UI.ixx
// UI Ö÷½çÃæ
export module UI;

import std;

import ConsoleHandler;
import BasicColors;

import Editor;

import InputHandler;

using namespace std;

export namespace NylteJ
{
	class UI
	{
	private:
		ConsoleHandler& console;
		InputHandler& inputHandler;
		Editor editor;
		
		string title;
	public:
		void PrintTitle()
		{
			console.Print(title + (ranges::views::repeat(' ', console.GetConsoleSize().width - title.size()) | ranges::to<string>()), { 0,0 }, BasicColors::black, BasicColors::yellow);
		}

		UI(ConsoleHandler& consoleHandler, string& editorData, InputHandler& inputHandler, const string& title = "ConsoleNotepad ver. 0.1     made by NylteJ"s)
			:console(consoleHandler),
			inputHandler(inputHandler),
			editor(	consoleHandler,
					editorData,
					{	{ 0,1 },
						{	console.GetConsoleSize().width - 1,
							console.GetConsoleSize().height - 1 } }),
			title(title)
		{
			PrintTitle();

			inputHandler.SubscribeMessage([&](const InputHandler::MessageWindowSizeChanged& message)
				{
					editor.ChangeDrawRange({ {0,1},{message.newSize.width - 1,message.newSize.height - 1} });
					
					PrintTitle();

					editor.RePrintData();
				});
		}
	};
}
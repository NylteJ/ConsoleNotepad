import std;

import ConsoleHandler;
import BasicColors;
import UI;
import InputHandler;
import FileHandler;
import ClipboardHandler;

using namespace std;

using namespace NylteJ;

int main()
{
	ConsoleHandler console;

	FileHandler file;

	wstring str;

	InputHandler inputHandler;

	ClipboardHandler clipboardHandler;

	UI ui{ console,str,inputHandler,file,clipboardHandler };

	console.BeginMonitorInput(inputHandler);

	this_thread::sleep_for(114514h);	// 睡美人说是
	
	return 0;
}
#include <locale>
import std;

import ConsoleHandler;
import BasicColors;
import UI;
import InputHandler;
import FileHandler;

using namespace std;

using namespace NylteJ;

int main()
{
	ConsoleHandler console;

	setlocale(LC_ALL, "chs");

	FileHandler file{ "testText.txt" };

	wstring str = file.ReadAll();
	//string str = to_string(console.GetConsoleSize().height);

	InputHandler inputHandler;

	UI ui{ console,str,inputHandler,file };

	console.BeginMonitorInput(inputHandler);

	this_thread::sleep_for(114514h);	// 睡美人说是

	return 0;
}
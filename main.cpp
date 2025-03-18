#include <locale>
import std;

import ConsoleHandler;
import BasicColors;
import UI;
import InputHandler;
import FileReader;

using namespace std;

using namespace NylteJ;

int main()
{
	ConsoleHandler console;

	setlocale(LC_ALL, "chs");

	FileReader file{ "testText.txt" };

	wstring str = file.ReadAll();
	//string str = to_string(console.GetConsoleSize().height);

	InputHandler inputHandler;

	UI ui{ console,str,inputHandler };

	console.BeginMonitorInput(inputHandler);

	this_thread::sleep_for(114514h);	// ˯����˵��

	return 0;
}
import std;

import ConsoleHandler;
import BasicColors;
import UI;
import InputHandler;

using namespace std;

using namespace NylteJ;

int main()
{
	ConsoleHandler console;

	ifstream file("BasicColors.ixx");

	stringstream ss;

	ss << file.rdbuf();

	string str = ss.str();

	InputHandler inputHandler;

	UI ui{ console,str,inputHandler };

	console.BeginMonitorInput(inputHandler);
	
	console.SetCursorTo({ 0,unsigned short(console.GetConsoleSize().height-1) });

	while (true);

	return 0;
}
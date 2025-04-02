import std;

import ConsoleHandler;
import BasicColors;
import UI;
import InputHandler;
import FileHandler;
import ClipboardHandler;
import SettingMap;

import ConsoleArgumentsAnalyzer;
import Exceptions;
import StringEncoder;

using namespace std;

using namespace NylteJ;

int main(int argc, char** argv)
{
	ConsoleArguments arguments{ argc, argv };

	if (arguments.HelpMode())
	{
		println("本程序可以通过命令行调用, 参数详解如下:"sv);
		println("1. \"-o\" / \"--open\" 表示打开, 后接要打开的文件路径"sv);
		println("2. \"-e\" / \"--encoding\" 表示编码, 后接编码名称"sv);
		println("	- 有防呆设计, 比如 \"utf-8\", \"UTF-8\", \"UTF 8\", \"utf8\" 都能定位到 UTF-8"sv);
		println("3. \"-h\" / \"--help\" / \"/?\" 可以输出帮助, 会覆盖其它一切参数"sv);
		println("4. 只需要指定打开文件路径时, 可以直接作为参数附加, 无需带 \"-o\" / \"--open\""sv);
		println("5. 示例   (程序名字取 ConsoleNotepad) : "sv);
		println("	1. ConsoleNotepad 1.txt"sv);
		println("	2. ConsoleNotepad -o 1.txt -e UTF-8"sv);
		println("	3. ConsoleNotepad --encoding \"gb 2312\" -o 1.txt"sv);
		println("	4. ConsoleNotepad /?"sv);
		return 0;
	}

	ConsoleHandler console;

	FileHandler file;

	wstring str;

	InputHandler inputHandler;

	ClipboardHandler clipboardHandler;

	SettingMap settings{};

	if (arguments.OpenFilePath().has_value())
	{
		if (arguments.GetEncoding().has_value())
			file.nowEncoding = arguments.GetEncoding().value();

		try
		{
			file.OpenFile(arguments.OpenFilePath().value());
		}
		catch (FileOpenFailedException&) {}
	}

	UI ui{ console,str,inputHandler,file,clipboardHandler,settings };

	console.BeginMonitorInput(inputHandler);

	this_thread::sleep_for(114514h);	// 睡美人说是
	
	return 0;
}
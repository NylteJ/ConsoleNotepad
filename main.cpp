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
		println("���������ͨ�������е���, �����������:"sv);
		println("1. \"-o\" / \"--open\" ��ʾ��, ���Ҫ�򿪵��ļ�·��"sv);
		println("2. \"-e\" / \"--encoding\" ��ʾ����, ��ӱ�������"sv);
		println("	- �з������, ���� \"utf-8\", \"UTF-8\", \"UTF 8\", \"utf8\" ���ܶ�λ�� UTF-8"sv);
		println("3. \"-h\" / \"--help\" / \"/?\" �����������, �Ḳ������һ�в���"sv);
		println("4. ֻ��Ҫָ�����ļ�·��ʱ, ����ֱ����Ϊ��������, ����� \"-o\" / \"--open\""sv);
		println("5. ʾ��   (��������ȡ ConsoleNotepad) : "sv);
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

	this_thread::sleep_for(114514h);	// ˯����˵��
	
	return 0;
}
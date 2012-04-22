#include "Precompiled.h"
#include "ToolInfo.h"
#include "ToolLink.h"
#include "ToolCallStack.h"
#include "ToolMemorySearch.h"
#include "ToolMemorySearchObject.h"
#include "Utility.h"

void showUsage();

int wmain(int argc, wchar_t* argv[])
{
	if (argc < 2)
	{
		showUsage();
		return 1;
	}

	wstring mode = argv[1];
	vector<wstring> args;
	for (int i=2; i<argc; i++)
		args.push_back(argv[i]);

	if		(mode == L"i" || mode == L"info")
	{
		ToolInfo::Param param;
		if (param.parseParam(args) == false ||
			ToolInfo::run(param) == false)
			return 1;
	}
	else if	(mode == L"l" || mode == L"link")
	{
		ToolLink::Param param;
		if (param.parseParam(args) == false ||
			ToolLink::run(param) == false)
			return 1;
	}
	else if	(mode == L"c" || mode == L"callstack")
	{
		ToolCallStack::Param param;
		if (param.parseParam(args) == false ||
			ToolCallStack::run(param) == false)
			return 1;
	}
	else if (mode == L"m" || mode == L"memeory")
	{
		ToolMemorySearch::Param param;
		if (param.parseParam(args) == false ||
			ToolMemorySearch::run(param) == false)
			return 1;
	}
	else if (mode == L"o" || mode == L"object")
	{
		ToolMemorySearchObject::Param param;
		if (param.parseParam(args) == false ||
			ToolMemorySearchObject::run(param) == false)
			return 1;
	}
	else
	{
		printf("%S: Invalid mode\n", mode.c_str());
		return 1;
	}

	return 0;
}

void showUsage()
{
	printf("Dump Analysis Tool by veblush\n");
	printf("Usage: Dat.exe [tool] [options] dmpfile\n");
	printf("  i : Show information of dump file\n");
	printf("  l : Link dmp to module\n");
	printf("      -o path : module name in dmp\n");
	printf("      -n path : new module path used for debugging\n");
	printf("      -p      : reset new module path in dmp\n");
	printf("  c : Recostruct a callstack from stack memory\n");
	printf("      -t tid  : thread id\n");
	printf("      -s      : stop constructing after finding common entry\n");
	printf("  m : Find address matched with filter from memory\n");
	printf("      -f flt  : [filter]\n");
	printf("  o : Find object from memory\n");
	printf("      -n name : type name\n");
	printf("      -f flt  : [filter]\n");
	printf("      -s size : size for searching range within object\n");
	printf("  [filter]\n");
	printf("      i#      : 4-bytes signed integer\n");
	printf("      u#      : 4-bytes unsigned integer\n");
	printf("      f#      : 4-bytes float\n");
	printf("      d#      : 8-bytes double\n");
	printf("      s<str>  : mbcs string\n");
	printf("      w<str>  : utf-16 unicode string\n");
}

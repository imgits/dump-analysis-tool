#pragma once

#include "DumpFile.h"

//-------------------------------------------------------------------------------- Symbols

namespace Symbols
{
	void initialize();
	void terminate();

	void loadModuleAllInDumpFile(DumpFile* dumpFile);
	void unloadModuleAll();

	struct FuncInfo
	{
		string name;
		string fileName;
		uint32 line;
		uint64 address;
	};

	bool getFuncInfo(uint32 addr, FuncInfo* funcInfo);
}

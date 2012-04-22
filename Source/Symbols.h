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

	public:
		string getString(uint32 codeAddress = 0) const;
		string getSourceString(uint32 codeAddress = 0) const;
	};

	bool getFuncInfo(uint32 addr, FuncInfo* funcInfo);
}

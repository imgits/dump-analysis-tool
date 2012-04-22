#include "Precompiled.h"
#include "Symbols.h"
#include "Utility.h"

//-------------------------------------------------------------------------------- Symbols

namespace Symbols
{
	struct LoadModule
	{
		string moduleName;
		DWORD64 baseAddr;
	};
	static vector<LoadModule> loadModules_;
}

void Symbols::initialize()
{
	SymSetOptions(SYMOPT_LOAD_LINES); // | SYMOPT_DEBUG);
	SymInitialize(GetCurrentProcess(), NULL, FALSE);
}

void Symbols::terminate()
{
	SymCleanup(GetCurrentProcess());
}

void Symbols::loadModuleAllInDumpFile(DumpFile* dumpFile)
{
	const vector<MINIDUMP_MODULE>& modules = dumpFile->getModules();
	for (int i=0; i<int(modules.size()); i++)
	{
		const MINIDUMP_MODULE& module = modules[i];

		string moduleName;
		{
			wstring moduleNameWide;
			dumpFile->getRvaString(module.ModuleNameRva, &moduleNameWide);
			moduleName = wstr2mbcs(moduleNameWide);
		}

		DWORD64 ret = SymLoadModuleEx(GetCurrentProcess(), NULL, 
			moduleName.c_str(), NULL, 
			module.BaseOfImage, 0, NULL, 0);
		if (ret != 0)
		{
			LoadModule lm;
			lm.moduleName = moduleName;
			lm.baseAddr = ret;
			loadModules_.push_back(lm);
		}
	}
}

void Symbols::unloadModuleAll()
{
	for (int i=0; i<int(loadModules_.size()); i++)
		SymUnloadModule64(GetCurrentProcess(), loadModules_[i].baseAddr);

	loadModules_.clear();
}

bool Symbols::getFuncInfo(uint32 addr, FuncInfo* funcInfo)
{
	funcInfo->line = 0;
	funcInfo->address = 0;

	// 함수 이름

	static char symbolInfoNote[4096];
	IMAGEHLP_SYMBOL64* sym = (IMAGEHLP_SYMBOL64*)symbolInfoNote;
	sym->SizeOfStruct = sizeof(IMAGEHLP_SYMBOL64);
	sym->MaxNameLength = 2048;
	uint64 disp64;
	if (SymGetSymFromAddr64(GetCurrentProcess(), addr, &disp64, sym))
		funcInfo->name = (const char*)sym->Name;
	else
		return false;

	// 라인 정보

	IMAGEHLP_LINE64 line;
	line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);
	uint32 disp32;
	if (SymGetLineFromAddr64(GetCurrentProcess(), addr, (PDWORD)&disp32, &line))
	{
		funcInfo->fileName = line.FileName;
		funcInfo->line = line.LineNumber;
		funcInfo->address = line.Address;
	}

	return true;
}

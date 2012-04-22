#include "Precompiled.h"
#include "ToolMemorySearch.h"
#include "DumpFile.h"
#include "Utility.h"

//----------------------------------------------------------------------- ToolMemorySearch

namespace ToolMemorySearch 
{

Param::Param()
{
}

bool Param::parseParam(const vector<wstring>& args)
{
	map<wchar_t, wstring> optMap;
	vector<wstring> leftArgs;
	if (parseOpt(args, L"f:", &optMap, &leftArgs) == false)
		return false;

	if (leftArgs.size() < 1)
	{
		printf("Path required!\n");
		return false;
	}

	if (isOpt(optMap, L'f'))
	{
		if (filter.setFromOptStr(optMap[L'f']) == false)
		{
			printf("%S: Invalid filter\n", optMap[L'f'].c_str());
			return false;
		}
	}
	else
	{
		printf("filter required!\n");
		return false;
	}

	dmpFilePath = leftArgs[0];

	return true;
}

bool run(const Param& param)
{
	DumpFilePtr dmp = DumpFile::open(param.dmpFilePath);
	if (dmp == nullptr)
	{
		printf("Cannot open %S\n", param.dmpFilePath.c_str());
		return false;
	}

	const SearchFilter& sf = param.filter;

	const byte* mappedMem = dmp->getMappedMem();
	const vector<MINIDUMP_MEMORY_DESCRIPTOR>& memories = dmp->getMemories();
	
	for (auto i = memories.begin(), i_end = memories.end(); i != i_end; ++i)
	{
		uint32 startAddr = ALIGN_UP(uint32(i->StartOfMemoryRange), sf.alignSize);
		uint32 endAddr = uint32(i->StartOfMemoryRange + i->Memory.DataSize - int32(sf.pattern.size()) + 1);
		for (uint32 a = startAddr; a < endAddr; a += sf.alignSize)
		{
			const byte* p = (mappedMem + (i->Memory.Rva + a - i->StartOfMemoryRange));
			if (memcmp(p, &sf.pattern[0], sf.pattern.size()) == 0)
				printf("0x%08x\n", a);
		}
	}

	return true;
}

} // namespace ToolMemorySearch 

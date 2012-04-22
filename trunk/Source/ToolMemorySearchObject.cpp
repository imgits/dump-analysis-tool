#include "Precompiled.h"
#include "ToolMemorySearchObject.h"
#include "DumpFile.h"
#include "Utility.h"
#include <unordered_map>

//---------------------------------------------------------------------------- RTTI Helper

// 32 비트 주소에서 올바른 주소를 나타내는 매크로
#define VALID_ADDR(addr) ((addr) >= 0x1000 && (addr) <= 0x7FFFFFFF && ((addr) % 4) == 0)

struct RttiCompleteObjectLocator
{
	DWORD signature;
	DWORD offset;
	DWORD cdOffset;
	DWORD typeDescriptorPtr;
	DWORD classHierarchyDescriptorPtr;
};

struct RttiTypeDescriptor
{
	DWORD vtbl;
	DWORD data;
	CHAR  name[1];
};

extern "C" void* __cdecl __unDNameHelper(
	char * outputString,
	const char * name,
	int maxStringLength,
	unsigned short disableFlags);

//----------------------------------------------------------------- ToolMemorySearchObject

namespace ToolMemorySearchObject
{

Param::Param()
{
}

bool Param::parseParam(const vector<wstring>& args)
{
	map<wchar_t, wstring> optMap;
	vector<wstring> leftArgs;
	if (parseOpt(args, L"n:f:s:", &optMap, &leftArgs) == false)
		return false;

	objectNamePattern = wstr2mbcs(getOptValue(optMap, L'n'));
	searchSizeForFilter = _wtoi(getOptValue(optMap, L's').c_str());

	if (isOpt(optMap, L'f') && filter.setFromOptStr(optMap[L'f']) == false)
	{
		printf("%S: Invalid filter\n", optMap[L'f'].c_str());
		return false;
	}

	if (filter.empty() == false && searchSizeForFilter == 0)
	{
		printf("Option s(SearchSizeForFilter) should be required\n");
		return false;
	}

	if (leftArgs.size() < 1)
	{
		printf("Path required!\n");
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

	const byte* mappedMem = dmp->getMappedMem();
	const vector<MINIDUMP_MEMORY_DESCRIPTOR>& memories = dmp->getMemories();

	struct RttiData
	{
		string name;
		string desc;
		string info;
		vector<pair<uint32, const byte*>> addrs;
	};

	typedef unordered_map<uint32, RttiData> RttiDataMap;
	RttiDataMap rdMap;

	// 메모리를 순회하면서 객체와 RTTI 정보를 수집

	for (auto i = memories.begin(), i_end = memories.end(); i != i_end; ++i)
	{
		uint32 startAddr = ALIGN_UP(uint32(i->StartOfMemoryRange), param.filter.alignSize);
		uint32 endAddr = uint32(i->StartOfMemoryRange + i->Memory.DataSize);
		for (uint32 a = startAddr; a < endAddr; a += 4)
		{
			const byte* ptr = mappedMem + (i->Memory.Rva + a - i->StartOfMemoryRange);
			uint32 vtbl_addr = *(uint32*)(ptr);
			if (!VALID_ADDR(vtbl_addr))
				continue;

			auto fi = rdMap.find(vtbl_addr);
			if (fi != rdMap.end())
			{
				fi->second.addrs.push_back(make_pair(a, ptr));
				continue;
			}

			// ptr 가 가리키는 객체의 RttiCompleteObjectLocator 를 찾기

			uint32 col_addr;
			if (dmp->readMemoryValue(vtbl_addr-4, &col_addr) == false || !VALID_ADDR(col_addr))
				continue;

			RttiCompleteObjectLocator col;
			if (dmp->readMemoryValue(col_addr, &col) == false ||
				col.signature != 0 || col.offset != 0 || !VALID_ADDR(col.typeDescriptorPtr))
				continue;

			// ptr 가 가리키는 객체 의 RttiTypeDescriptor 를 찾았다!

			const RttiTypeDescriptor* td = reinterpret_cast<const RttiTypeDescriptor*>(
				dmp->getMemoryPointer(col.typeDescriptorPtr, sizeof(*td)));
			if (td == nullptr || td->vtbl == 0 || td->name[0] != '.')
				continue;

			char desc[4096]; desc[0] = 0;
			__unDNameHelper(desc, td->name+1, sizeof(desc), 0);

			RttiData rttiData;
			rttiData.name = td->name+1;
			rttiData.desc = desc;
			rttiData.addrs.push_back(make_pair(a, ptr));
			rdMap.insert(RttiDataMap::value_type(vtbl_addr, rttiData));
		}
	}

	// 찾은 정보에 검색 필터를 적용합니다

	const SearchFilter& sf = param.filter;
	for (auto i = rdMap.begin(), i_end = rdMap.end(); i != i_end; ++i)
	{
		// 객체 타입 이름 검사

		if (!param.objectNamePattern.empty() &&
			i->second.desc.find(param.objectNamePattern) == string::npos)
			continue;

		printf("* %s (Total:%d)\n", i->second.desc.c_str(), i->second.addrs.size());

		// 객체 안에서 주소 검사

		for (auto j = i->second.addrs.begin(), j_end = i->second.addrs.end(); j != j_end; ++j)
		{
			uint32 addr = j->first;
			const byte* ptr = j->second;
			if (param.filter.empty())
			{
				printf("  0x%08x\n", addr);
			}
			else
			{
				if (IsBadReadPtr(ptr, param.searchSizeForFilter) == TRUE)
					continue;
				uint32 startAddr = ALIGN_UP(addr, sf.alignSize);
				uint32 endAddr = uint32(addr + param.searchSizeForFilter - int32(sf.pattern.size()) + 1);
				for (uint32 a = startAddr; a < endAddr; a += sf.alignSize)
				{
					const byte* p = ptr + (a - startAddr);
					if (memcmp(p, &sf.pattern[0], sf.pattern.size()) == 0)
					{
						printf("0x%08x (+ 0x%x)\n", addr, a - addr);
						break;
					}
				}
			}
		}
	}

	return true;
}

} // namespace ToolMemorySearchObject

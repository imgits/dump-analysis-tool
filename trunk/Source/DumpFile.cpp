#include "Precompiled.h"
#include "DumpFile.h"

//------------------------------------------------------------------------------- DumpFile

DumpFilePtr DumpFile::open(const wstring& path)
{
	// open the file as a memory mapped form

	HANDLE fileHandle = CreateFile(
		path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, 
		OPEN_EXISTING, NULL, NULL);
	if (fileHandle == INVALID_HANDLE_VALUE) 
		return nullptr;

	uint32 fileSize = GetFileSize(fileHandle, NULL);
	if (fileSize == 0)
	{
		CloseHandle(fileHandle);
		return nullptr;
	}

	HANDLE mappingHandle = CreateFileMapping(
		fileHandle, NULL, PAGE_READONLY, NULL, NULL, NULL);
	CloseHandle(fileHandle);
	if (mappingHandle == nullptr) 
		return nullptr;

	const byte* mappedMem = (const byte*)MapViewOfFile(
		mappingHandle, FILE_MAP_READ, NULL, NULL, NULL);
	CloseHandle(mappingHandle);
	if (mappedMem == nullptr) 
		return nullptr;

	// check header

	if (memcmp(mappedMem, "MDMP", 4) != 0)
	{
		UnmapViewOfFile(mappedMem);
		return nullptr;
	}

	// load stream

	DumpFilePtr dumpFile;
	dumpFile.reset(new DumpFile(path, fileSize, mappedMem));
	return dumpFile;
}

DumpFile::DumpFile(const wstring& path, uint32 fileSize, const byte* mappedMem)
	: path_(path)
	, fileSize_(fileSize)
	, mappedMem_(mappedMem)
{
	loadInfoFromDump();
}

void DumpFile::loadInfoFromDump()
{
	BOOL ret;
	MINIDUMP_DIRECTORY* mdDir;
	VOID* streamPointer;
	ULONG streamSize;

	// load ExceptionStream

	ZeroMemory(&exception_, sizeof(exception_));
	ret = MiniDumpReadDumpStream((PVOID)mappedMem_, ExceptionStream, &mdDir, &streamPointer, &streamSize);
	if (ret && IsBadReadPtr(streamPointer, sizeof(MINIDUMP_EXCEPTION_STREAM)) == FALSE) 
		exception_ = *(MINIDUMP_EXCEPTION_STREAM*)streamPointer;

	// load SystemInfoStream

	ZeroMemory(&sysInfo_, sizeof(sysInfo_));
	ret = MiniDumpReadDumpStream((PVOID)mappedMem_, SystemInfoStream, &mdDir, &streamPointer, &streamSize);
	if (ret && IsBadReadPtr(streamPointer, sizeof(MINIDUMP_SYSTEM_INFO)) == FALSE) 
		sysInfo_ = *(MINIDUMP_SYSTEM_INFO*)streamPointer;

	// load ThreadListStream

	ret = MiniDumpReadDumpStream((PVOID)mappedMem_, ThreadListStream, &mdDir, &streamPointer, &streamSize);
	if (ret && IsBadReadPtr(streamPointer, sizeof(MINIDUMP_THREAD_LIST)) == FALSE) 
	{
		MINIDUMP_THREAD_LIST* threadList = (MINIDUMP_THREAD_LIST*)streamPointer;
		threads_.reserve(threadList->NumberOfThreads);
		for (int i=0; i < int(threadList->NumberOfThreads); i++)
			threads_.push_back(threadList->Threads[i]);
	}

	// load ModuleListStream

	ret = MiniDumpReadDumpStream((PVOID)mappedMem_, ModuleListStream, &mdDir, &streamPointer, &streamSize);
	if (ret && IsBadReadPtr(streamPointer, sizeof(MINIDUMP_MODULE_LIST)) == FALSE) 
	{
		MINIDUMP_MODULE_LIST* moduleList = (MINIDUMP_MODULE_LIST*)streamPointer;
		modules_.reserve(moduleList->NumberOfModules);
		for (int i=0; i < int(moduleList->NumberOfModules); i++)
			modules_.push_back(moduleList->Modules[i]);
	}

	// load MemoryListStream

	ret = MiniDumpReadDumpStream((PVOID)mappedMem_, MemoryListStream, &mdDir, &streamPointer, &streamSize);
	if (ret && IsBadReadPtr(streamPointer, sizeof(MINIDUMP_MEMORY_LIST)) == FALSE) 
	{
		MINIDUMP_MEMORY_LIST* memoryList = (MINIDUMP_MEMORY_LIST*)streamPointer;
		memories_.reserve(memoryList->NumberOfMemoryRanges);
		for (int i=0; i < int(memoryList->NumberOfMemoryRanges); i++)
		{
			const byte* ptrInRange = mappedMem_ + memoryList->MemoryRanges[i].Memory.Rva;
			uint32 sizeInRange = memoryList->MemoryRanges[i].Memory.DataSize;
			if (IsBadReadPtr(ptrInRange, sizeInRange) == FALSE)
				memories_.push_back(memoryList->MemoryRanges[i]);
		}
	}

	ret = MiniDumpReadDumpStream((PVOID)mappedMem_, Memory64ListStream, &mdDir, &streamPointer, &streamSize);
	if (ret && IsBadReadPtr(streamPointer, sizeof(MINIDUMP_MEMORY64_LIST)) == FALSE) 
	{
		MINIDUMP_MEMORY64_LIST* memory64List = (MINIDUMP_MEMORY64_LIST*)streamPointer;
		memories_.reserve((int)memory64List->NumberOfMemoryRanges);
			
		uint32 rva = (uint32)memory64List->BaseRva;
		for (int i=0; i < int(memory64List->NumberOfMemoryRanges); i++)
		{
			MINIDUMP_MEMORY_DESCRIPTOR mem;
			{
				mem.StartOfMemoryRange = memory64List->MemoryRanges[i].StartOfMemoryRange;
				mem.Memory.DataSize = (uint32)memory64List->MemoryRanges[i].DataSize;
				mem.Memory.Rva = rva;
				rva += mem.Memory.DataSize;
			}
			memories_.push_back(mem);
		}
	}

	sort(memories_.begin(), memories_.end(), 
		[](const MINIDUMP_MEMORY_DESCRIPTOR& a, const MINIDUMP_MEMORY_DESCRIPTOR& b) -> bool 
		{ return a.StartOfMemoryRange < b.StartOfMemoryRange; });
}

DumpFile::~DumpFile()
{
	UnmapViewOfFile(mappedMem_);
	mappedMem_ = nullptr;
}

uint32 DumpFile::getFileSize() const
{
	return fileSize_;
}

const byte* DumpFile::getMappedMem() const
{
	return mappedMem_;
}

const byte* DumpFile::getStream(MINIDUMP_STREAM_TYPE type, uint32* size) const
{
	MINIDUMP_DIRECTORY* mdDir;
	VOID* streamPointer;
	ULONG streamSize;

	BOOL ret = MiniDumpReadDumpStream((PVOID)mappedMem_, type, &mdDir, &streamPointer, &streamSize);
	if (ret == FALSE)
		return nullptr;

	if (size != nullptr)
		*size = (uint32)streamSize;

	return (const byte*)streamPointer;
}

const MINIDUMP_EXCEPTION_STREAM& DumpFile::getException() const
{
	return exception_;
}

const MINIDUMP_SYSTEM_INFO& DumpFile::getSysInfo() const
{
	return sysInfo_;
}

const vector<MINIDUMP_THREAD>& DumpFile::getThreads() const
{
	return threads_;
}

const MINIDUMP_THREAD* DumpFile::getThread(uint32 threadId) const
{
	for (auto t = threads_.begin(), t_end = threads_.end(); t != t_end; t += 1)
	{
		if (t->ThreadId == threadId)
			return &*t;
	}
	return nullptr;
}

const vector<MINIDUMP_MODULE>& DumpFile::getModules() const
{
	return modules_;
}

const MINIDUMP_MODULE* DumpFile::getModuleHasAddr(uint32 addr) const
{
	for (auto m = modules_.begin(), m_end = modules_.end(); m != m_end; m += 1)
	{
		if (addr >= m->BaseOfImage && addr < m->BaseOfImage + m->SizeOfImage)
			return &*m;
	}
	return nullptr;
}

const vector<MINIDUMP_MEMORY_DESCRIPTOR>& DumpFile::getMemories() const
{
	return memories_;
}

const byte* DumpFile::getRvaMemory(RVA rva, int size) const
{
	const byte* buf = mappedMem_ + rva;
	if (IsBadReadPtr(buf, size) != FALSE)
		return nullptr;

	return buf;
}

bool DumpFile::getRvaString(RVA rva, wstring* str) const
{
	const MINIDUMP_STRING* mstr = (const MINIDUMP_STRING*)(mappedMem_ + rva);
	if (IsBadReadPtr(mstr->Buffer, mstr->Length) != FALSE)
		return false;

	str->resize(mstr->Length / 2);
	memcpy(&((*str)[0]), mstr->Buffer, mstr->Length);
	return true;
}

bool DumpFile::getRvaContext(RVA rva, CONTEXT* context) const
{
	const CONTEXT* ctx = (const CONTEXT*)(mappedMem_ + rva);
	if (IsBadReadPtr(ctx, sizeof(CONTEXT)) != FALSE)
		return false;

	*context = *ctx;
	return true;
}

const byte* DumpFile::getMemoryPointer(uint32 addr, int32 size) const
{
	// find a memory item which contains curAddr

	MINIDUMP_MEMORY_DESCRIPTOR value;
	value.StartOfMemoryRange = addr;
	auto i = lower_bound(memories_.begin(), memories_.end(), value,
		[](const MINIDUMP_MEMORY_DESCRIPTOR& a, const MINIDUMP_MEMORY_DESCRIPTOR& b) -> bool 
		{ return a.StartOfMemoryRange <= b.StartOfMemoryRange; });
	if (i == memories_.begin())
		return nullptr;
	i--;

	int32 posInRange = addr - uint32(i->StartOfMemoryRange);
	int32 availableInRange = i->Memory.DataSize - posInRange;
	if (availableInRange <= size)
		return nullptr;
	
	return mappedMem_ + (i->Memory.Rva + posInRange);
}

int32 DumpFile::readMemory(uint32 addr, int32 size, byte* buffer) const
{
	uint32 curAddr = addr;
	int32 leftSize = size;
	
	while (leftSize > 0)
	{
		// find a memory item which contains curAddr

		MINIDUMP_MEMORY_DESCRIPTOR value;
		value.StartOfMemoryRange = curAddr;
		auto i = lower_bound(memories_.begin(), memories_.end(), value,
			[](const MINIDUMP_MEMORY_DESCRIPTOR& a, const MINIDUMP_MEMORY_DESCRIPTOR& b) -> bool 
			{ return a.StartOfMemoryRange <= b.StartOfMemoryRange; });
		if (i == memories_.begin())
			break;
		i--;

		// copy memory from item found

		int32 posInRange = curAddr - uint32(i->StartOfMemoryRange);
		int32 availableInRange = i->Memory.DataSize - posInRange;
		if (availableInRange <= 0)
			break;

		if (leftSize <= availableInRange)
		{
			memcpy(
				buffer + (curAddr - addr), 
				mappedMem_ + (i->Memory.Rva + posInRange), 
				leftSize);
			curAddr += leftSize;
			leftSize = 0;
		}
		else
		{
			memcpy(
				buffer + (curAddr - addr), 
				mappedMem_ + (i->Memory.Rva + posInRange), 
				availableInRange);
			curAddr += availableInRange;
			leftSize -= availableInRange;
		}
	}

	return size - leftSize;
}

#pragma once

//------------------------------------------------------------------------------- DumpFile

class DumpFile;
typedef shared_ptr<DumpFile> DumpFilePtr;

class DumpFile
{
public:
	static DumpFilePtr open(const wstring& path);

private:
	DumpFile(const wstring& path, uint32 fileSize, const byte* mappedMem);
	void loadInfoFromDump();

public:
	virtual ~DumpFile();

public:
	uint32 getFileSize() const;

	const byte* getMappedMem() const;
	const byte* getStream(MINIDUMP_STREAM_TYPE type, uint32* size = nullptr) const;

	const MINIDUMP_EXCEPTION_STREAM& getException() const;	
	const MINIDUMP_SYSTEM_INFO& getSysInfo() const;

	const vector<MINIDUMP_THREAD>& getThreads() const;
	const MINIDUMP_THREAD* getThread(uint32 threadId) const;

	const vector<MINIDUMP_MODULE>& getModules() const;
	const MINIDUMP_MODULE* getModuleHasAddr(uint32 addr) const;

	const vector<MINIDUMP_MEMORY_DESCRIPTOR>& getMemories() const;

	const byte* getRvaMemory(RVA rva, int size) const;
	bool getRvaString(RVA rva, wstring* str) const;
	bool getRvaContext(RVA rva, CONTEXT* context) const;

	//! 지정한 주소에서 지정한 크기만큼 데이터를 읽을 수 있으면 읽을 수 있는
	//! 메모리 주소를 반환합니다. 실패하면 nullptr 을 반환합니다.
	const byte* getMemoryPointer(uint32 addr, int32 size) const;

	//! 지정한 주소에서 지정한 크기만큼 데이터를 읽어 buffer 에 복사합니다.
	//! 만약 데이터가 부족하다면 읽을 수 있는 만큼만 복사하며 복사한 크기를 반환합니다.
	int32 readMemory(uint32 addr, int32 size, byte* buffer) const;

	//! 지정한 주소에서 T 타입의 데이터를 읽어 반환합니다. 
	//! T 타입 크기만큼 온전히 읽지 못하면 false 를 반환합니다.
	template <typename T> bool readMemoryValue(uint32 addr, T* value) const
	{
		return readMemory(addr, sizeof(*value), (byte*)value) == sizeof(*value);
	}

private:
	wstring path_;
	uint32 fileSize_;
	const byte* mappedMem_;
	MINIDUMP_EXCEPTION_STREAM exception_;
	MINIDUMP_SYSTEM_INFO sysInfo_;
	vector<MINIDUMP_THREAD> threads_;
	vector<MINIDUMP_MODULE> modules_;
	vector<MINIDUMP_MEMORY_DESCRIPTOR> memories_;
};

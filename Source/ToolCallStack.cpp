#include "Precompiled.h"
#include "ToolCallStack.h"
#include "Symbols.h"
#include "Utility.h"

//-------------------------------------------------------------------------- ToolCallStack

namespace ToolCallStack
{

Param::Param()
	: threadId(0)
	, showSourceInfo(false)
	, stopOnCommonEntry(false)
{
}

bool Param::parseParam(const vector<wstring>& args)
{
	map<wchar_t, wstring> optMap;
	vector<wstring> leftArgs;
	if (parseOpt(args, L"t:se", &optMap, &leftArgs) == false)
		return false;

	if (leftArgs.size() < 1)
	{
		printf("Path required!\n");
		return false;
	}

	threadId = _wtoi(getOptValue(optMap, L't').c_str());
	showSourceInfo = isOpt(optMap, L's');
	stopOnCommonEntry = isOpt(optMap, L'e');
	dmpFilePath = leftArgs[0];

	return true;
}

bool isCommonEntryFunc(const string& funcName)
{
	return 
		funcName == "__tmainCRTStartup" ||
		funcName == "wmainCRTStartup";
}

bool run(const Param& param)
{
	Symbols::initialize();

	DumpFilePtr dmp = DumpFile::open(param.dmpFilePath);
	if (dmp == nullptr)
	{
		printf("Cannot open %S\n", param.dmpFilePath.c_str());
		return false;
	}

	Symbols::loadModuleAllInDumpFile(dmp.get());

	// 덤프 파일의 쓰레드 정보 읽기 (지정한 쓰레드 혹은 예외가 발생한 쓰레드)

	uint32 threadId = param.threadId;
	if (threadId == 0)
		threadId = dmp->getException().ThreadId;

	const MINIDUMP_THREAD* thread = dmp->getThread(threadId);
	if (thread == nullptr)
	{
		printf("Cannot find thread id=%d\n", threadId);
		return false;
	}	

	// 데이터를 읽을 때 주소 4 bytes 정렬이 되도록 조정

	uint32 stackAddr = uint32(thread->Stack.StartOfMemoryRange);
	uint32 stackSize = thread->Stack.Memory.DataSize;		
	if ((stackAddr % 4) != 0)
	{
		int32 a = 4 - (stackAddr % 4);
		stackAddr += a;
		stackSize -= a;
	}

	// 스택을 읽어가면서 확인

	uint32 lastFrameSavedAddr = 0;
	uint32 lastFrameSavedValue = 0;
	bool lastFrameLinked = false;

	CONTEXT threadContext;
	if (dmp->getRvaContext(thread->ThreadContext.Rva, &threadContext))
	{
		Symbols::FuncInfo funcInfo;
		if (Symbols::getFuncInfo(threadContext.Eip, &funcInfo))
		{
			printf("EIP             I(0x%08x) %s", 
				threadContext.Eip, funcInfo.name.c_str());
			if (param.showSourceInfo)
				printf(" %s\n", funcInfo.getSourceString(threadContext.Eip).c_str());
			else
				printf("\n");
		}

		lastFrameSavedValue = threadContext.Ebp;
	}	

	uint32 stackAddrEnd = stackAddr + stackSize;
	for (uint32 addr = stackAddr; addr < stackAddrEnd; addr += 4)
	{
		uint32 value;
		if (dmp->readMemoryValue(addr, &value) == false)
			break;

		if (value >= stackAddr && value < stackAddrEnd)
		{
			lastFrameLinked = (lastFrameSavedValue == addr);
			lastFrameSavedAddr = addr;
			lastFrameSavedValue = value;
		}

		const MINIDUMP_MODULE* moduleHit = dmp->getModuleHasAddr(value);
		if (moduleHit)
		{			
			Symbols::FuncInfo funcInfo;
			Symbols::getFuncInfo(value, &funcInfo);

			char tag = ' ';
			if (lastFrameSavedAddr == addr - 4)
				tag = lastFrameLinked ? '+' : '-';

			printf("S(0x%08x) %c I(0x%08x) %s", 
				addr, tag, value, funcInfo.name.c_str());

			if (param.showSourceInfo)
				printf(" %s\n", funcInfo.getSourceString(value).c_str());
			else
				printf("\n");

			if (param.stopOnCommonEntry && isCommonEntryFunc(funcInfo.name))
				break;
		}
	}
		
	Symbols::terminate();
	return true;
}

} // namespace ToolCallStack

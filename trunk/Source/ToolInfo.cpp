#include "Precompiled.h"
#include "ToolInfo.h"
#include "DumpFile.h"
#include "Utility.h"

//--------------------------------------------------------------------------- ToolInfo

namespace ToolInfo
{

Param::Param()
{
}

bool Param::parseParam(const vector<wstring>& args)
{
	map<wchar_t, wstring> optMap;
	vector<wstring> leftArgs;
	if (parseOpt(args, L"", &optMap, &leftArgs) == false)
		return false;

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

	printf("DMP: %S\n", param.dmpFilePath.c_str());
	printf("\n");

	printf("* Exception\n");
	{
		const MINIDUMP_EXCEPTION_STREAM& e = dmp->getException();
		const MINIDUMP_EXCEPTION& r = e.ExceptionRecord;

		printf("  TheadId: %d\n", e.ThreadId);
		printf("  Code:    0x%08x\n", r.ExceptionCode);
		printf("  Flags:   0x%08x\n", r.ExceptionFlags);
		printf("  Record:  0x%08x\n", uint32(r.ExceptionRecord));
		printf("  Address: 0x%08x\n", uint32(r.ExceptionAddress));
		printf("\n");
	}

	printf("* System\n");
	{
		const MINIDUMP_SYSTEM_INFO& s = dmp->getSysInfo();

		printf("  Processor\n");
		printf("    Architecture: %d\n", s.ProcessorArchitecture);
		printf("    Level:        %d\n", s.ProcessorLevel);
		printf("    Revision:     %d\n", s.ProcessorRevision);
		printf("  OS\n");
		printf("    MajorVersion: %d\n", s.MajorVersion);
		printf("    MinorVersion: %d\n", s.MinorVersion);
		printf("    BuildNumber:  %d\n", s.BuildNumber);
		printf("    PlatformId:   %d\n", s.PlatformId);

		wstring csd;
		dmp->getRvaString(s.CSDVersionRva, &csd);
		printf("    CSDVersion:   %S\n", csd.c_str());

		auto c = s.Cpu.X86CpuInfo;
		char vendorIdStr[13] = { 0, };
		memcpy(vendorIdStr, c.VendorId, 12);
		printf("  CPU\n");
		printf("    VendorId:     %s\n", vendorIdStr);
		printf("    VersionInfo:  0x%08x\n", c.VersionInformation);
		printf("    FeatureInfo:  0x%08x\n", c.FeatureInformation);
		printf("    AMDExtended:  0x%08x\n", c.AMDExtendedCpuFeatures);
		printf("\n");
	}

	printf("* Threads\n");
	{
		const vector<MINIDUMP_THREAD>& thread = dmp->getThreads();
		for (auto t = thread.begin(), t_end = thread.end(); t != t_end; ++t)
		{
			printf("  Thread(id:%d)\n", t->ThreadId);
			printf("    SuspendCount:  %d\n", t->SuspendCount);
			printf("    PriorityClass: %d\n", t->PriorityClass);
			printf("    Priority:      %d\n", t->Priority);
			printf("    Stack:         %d\n", t->Stack.StartOfMemoryRange);

			CONTEXT ctx;
			if (dmp->getRvaContext(t->ThreadContext.Rva, &ctx))
			{
				printf("    Context EIP:   0x%08x\n", ctx.Eip);
				printf("    Context EBP:   0x%08x\n", ctx.Ebp);
			}
		}
		printf("\n");
	}

	printf("* Modules\n");
	{
		const vector<MINIDUMP_MODULE>& modules = dmp->getModules();
		for (auto m = modules.begin(), m_end = modules.end(); m != m_end; ++m)
		{
			wstring name;
			dmp->getRvaString(m->ModuleNameRva, &name);

			printf("  Modules(%d)\n", m - modules.begin());
			printf("    Name:          %S\n", name.c_str());
			printf("    BaseOfImage:   0x%08x\n", uint32(m->BaseOfImage));
			printf("    SizeOfImage:   0x%08x\n", m->SizeOfImage);
			printf("    CheckSum:      0x%08x\n", m->CheckSum);
			printf("    TimeDateStamp: 0x%08x\n", m->TimeDateStamp);

			const byte* cvData = dmp->getRvaMemory(m->CvRecord.Rva, m->CvRecord.DataSize);
			if (cvData != nullptr && memcmp(cvData, "RSDS", 4) == 0)
			{
				const int* cdi = (const int*)cvData;
				printf("    CV_GUID:       %x-%x-%x-%x\n", cdi[1], cdi[2], cdi[3], cdi[4]);
				printf("    CV_AGE:        %d\n", cdi[5]);
				printf("    CV_Path:       %S\n", utf82wstr((char*)cvData+24).c_str());
			}
		}
		printf("\n");
	}

	printf("* Memories\n");
	{
		const vector<MINIDUMP_MEMORY_DESCRIPTOR>& memories = dmp->getMemories();
		for (auto m = memories.begin(), m_end = memories.end(); m != m_end; ++m)
		{
			printf("  addr: 0x%08x, size: 0x%08x\n", uint32(m->StartOfMemoryRange), m->Memory.DataSize);
		}
		printf("\n");
	}

	return true;
}

} // namespace ToolInfo

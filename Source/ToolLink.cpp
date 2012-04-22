#include "Precompiled.h"
#include "ToolLink.h"
#include "DumpFile.h"
#include "Utility.h"

//------------------------------------------------------------------------------- ToolLink

namespace ToolLink
{

Param::Param()
	: linkModulePath(false)
{
}

bool Param::parseParam(const vector<wstring>& args)
{
	map<wchar_t, wstring> optMap;
	vector<wstring> leftArgs;
	if (parseOpt(args, L"o:n:p", &optMap, &leftArgs) == false)
		return false;

	oldModuleName = getOptValue(optMap, L'o');

	newModulePath = getOptValue(optMap, L'n');
	if (newModulePath.empty())
	{
		printf("NewModulePath required!\n");
		return false;
	}

	linkModulePath = isOpt(optMap, L'p');

	if (leftArgs.size() < 1)
	{
		printf("Path required!\n");
		return false;
	}

	dmpFilePath = leftArgs[0];

	return true;
}

struct ModuleDebugInfo
{
	uint32  imageBase;
	uint32  timeDateStamp;
	uint32  sizeOfImage;
	uint32  checkSum;
	uint32  cvGuid[4];
	uint32  cvAge;
	wstring cvPdbPath;
};

bool getModuleDebugInfo(const wstring& path, ModuleDebugInfo* info)
{
	// MemoryMapped 형태로 파일을 열기

	HANDLE fileHandle = CreateFile(
		path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, 
		OPEN_EXISTING, NULL, NULL);
	if (fileHandle == INVALID_HANDLE_VALUE) 
		return nullptr;

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

	// 파일 PE 헤더 확인

	const byte* peStart = mappedMem + *(byte*)&mappedMem[0x3c];
	if (peStart[0] != 'P' || peStart[1] != 'E' || peStart[2] != 0 || peStart[3] != 0) 
	{
		UnmapViewOfFile(mappedMem);
		return false;
	}

	const byte* coffStart = peStart+4;
	const byte* optStart = coffStart + 20;

	uint32 sectionCount	= *(uint16*)&coffStart[2];
	uint32 optSize = *(uint16*)&coffStart[16];
	
	info->timeDateStamp = *(uint32*)&coffStart[4];
	info->imageBase	= *(uint32*)&optStart[28];
	info->sizeOfImage = *(uint32*)&optStart[56];
	info->checkSum  = *(uint32*)&optStart[64];

	uint32 debugRva	= *(uint32*)&optStart[144];
	uint32 debugSize = *(uint32*)&optStart[148];

	memset(info->cvGuid, 0, sizeof(info->cvGuid));
	info->cvAge = 0;

	// 섹션 정보를 돌면서 디버그 정보가 포함된 섹션을 찾아 데이터를 읽음

	const byte* sectionStart = coffStart + 20 + optSize;
	for (uint32 i=0; i<sectionCount; i++)
	{
		const byte* curSection = sectionStart + i*40;
		uint32 virtualSize = *(uint32*)&curSection[8];
		uint32 virtualOffset = *(uint32*)&curSection[12];
		uint32 physicalSize	= *(uint32*)&curSection[16];
		uint32 physicalOffset = *(uint32*)&curSection[20];

		if (virtualOffset <= debugRva && (debugRva + debugSize) <= (virtualOffset + virtualSize))
		{
			const byte* debugStart = mappedMem + physicalOffset + (debugRva - virtualOffset);
			const byte* dd = mappedMem + *(uint32*)&debugStart[24];
			if (memcmp(dd, "RSDS", 4) == 0)
			{
				const int* cdi = (int*)dd;
				info->cvGuid[0] = cdi[1];
				info->cvGuid[1] = cdi[2];
				info->cvGuid[2] = cdi[3];
				info->cvGuid[3] = cdi[4];
				info->cvAge = cdi[5];
				info->cvPdbPath = utf82wstr((char*)&dd[24]);
				break;
			}
		}
	}

	UnmapViewOfFile(mappedMem);
	return true;
}

bool setModuleDebugInfoToDmp(const wstring& dmpFilePath, 
	wstring oldModuleName, wstring newModulePath, bool linkModulePath,
	const ModuleDebugInfo& info)
{
	DumpFilePtr dmp = DumpFile::open(dmpFilePath);
	if (dmp == nullptr)
	{
		printf("Cannot open %S\n", dmpFilePath.c_str());
		return false;
	}

	// 미니덤프 안에서 대상 모듈 찾기

	int mmIdx = -1;
	MINIDUMP_MODULE mmInfo;
	wstring modulePath;

	const vector<MINIDUMP_MODULE>& modules = dmp->getModules();
	for (auto m = modules.begin(), m_end = modules.end(); m != m_end; ++m)
	{
		dmp->getRvaString(m->ModuleNameRva, &modulePath);
		
		wstring moduleName = getFileNameFromPath(modulePath);	
		if (_wcsicmp(moduleName.c_str(), oldModuleName.c_str()) == 0)
		{
			mmIdx = m - modules.begin();
			mmInfo = *m;
			break;
		}
	}

	if (mmIdx == -1)
	{
		printf("Cannot find module %S\n", oldModuleName.c_str());
		return false;
	}

	// 덤프 파일을 닫기 전에 주요 정보를 읽어둔다

	uint32 fileSize = dmp->getFileSize();
	uint32 msRva = dmp->getStream(ModuleListStream) - dmp->getMappedMem();

	dmp = nullptr;

	// 미니덤프를 저장 가능하도록 파일로 다시 읽기

	HANDLE fileHandle = CreateFile(dmpFilePath.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (fileHandle == INVALID_HANDLE_VALUE) 
	{
		printf("Cannot reopen %S\n", dmpFilePath.c_str());
		return false;
	}
	DWORD doneLen;

	// 모듈 경로 갱신
	
	if (linkModulePath)
	{
		if (newModulePath.size() > modulePath.size())
		{
			// 경로가 들어갈 자리가 부족하니 파일 맨 뒤에 추가
			mmInfo.ModuleNameRva = fileSize;
			fileSize += 4 + (int32(newModulePath.size()+1) * sizeof(wchar_t));
		}

		SetFilePointer(fileHandle, mmInfo.ModuleNameRva, NULL, FILE_BEGIN);
		int32 len = int32(newModulePath.size() * sizeof(wchar_t));
		WriteFile(fileHandle, &len, 4, &doneLen, NULL);
		WriteFile(fileHandle, newModulePath.c_str(), len + sizeof(wchar_t), &doneLen, NULL);
	}

	// CV 레코드가 없다면 CV 레코드 추가

	if (mmInfo.CvRecord.Rva == 0 && info.cvGuid[0] != 0)
	{
		string pdbPathUtf8 = wstr2utf8(info.cvPdbPath);

		mmInfo.CvRecord.Rva = fileSize;
		mmInfo.CvRecord.DataSize = 4 + 16 + 4 + (pdbPathUtf8.size()+1);

		// CV 레코드가 없었으니 파일 맨 뒤에 추가
		SetFilePointer(fileHandle, 0, NULL, FILE_END);
		fileSize += mmInfo.CvRecord.DataSize;

		WriteFile(fileHandle, "RSDS", 4, &doneLen, NULL);
		WriteFile(fileHandle, info.cvGuid, 16, &doneLen, NULL);
		WriteFile(fileHandle, &info.cvAge, 4, &doneLen, NULL);
		WriteFile(fileHandle, pdbPathUtf8.c_str(), pdbPathUtf8.size()+1, &doneLen, NULL);
	}

	// 모듈 정보 갱신

	modulePath.size();

	uint32 pos = msRva + 4 + mmIdx*sizeof(MINIDUMP_MODULE);
	SetFilePointer(fileHandle, pos, NULL, FILE_BEGIN);

	mmInfo.SizeOfImage = info.sizeOfImage;
	mmInfo.CheckSum = info.checkSum;
	mmInfo.TimeDateStamp = info.timeDateStamp;
	
	WriteFile(fileHandle, &mmInfo, sizeof(MINIDUMP_MODULE), &doneLen, NULL);

	// 모듈 CV 레코드 정보 갱신

	if (mmInfo.CvRecord.Rva != 0)
	{
		SetFilePointer(fileHandle, mmInfo.CvRecord.Rva, NULL, FILE_BEGIN);
		uint32 cvheader;
		ReadFile(fileHandle, &cvheader, 4, &doneLen, NULL); 
		if (memcmp(&cvheader, "RSDS", 4) == 0)
		{
			WriteFile(fileHandle, info.cvGuid, 16, &doneLen, NULL);
			WriteFile(fileHandle, &info.cvAge, 4, &doneLen, NULL);
		}
	}

	CloseHandle(fileHandle);
	return true;
}

bool run(const Param& param)
{
	// 새 모듈 파일로 부터 디버깅 정보를 가져옵니다.

	ModuleDebugInfo newDebugInfo;
	if (getModuleDebugInfo(param.newModulePath, &newDebugInfo) == false)
	{
		printf("Cannot get debug info from %S\n", param.newModulePath.c_str());
		return false;
	}

	// 가져온 디버깅 정보를 덤프 파일에 기록합니다.

	wstring oldModuleName = param.oldModuleName;
	if (oldModuleName.empty())
		oldModuleName = getFileNameFromPath(param.newModulePath);

	if (setModuleDebugInfoToDmp(param.dmpFilePath, oldModuleName,
		param.newModulePath, param.linkModulePath, newDebugInfo) == false)
	{
		return false;
	}

	printf("Link Succesfully : %S\n", param.dmpFilePath.c_str());
	return true;
}

} // namespace ToolLink

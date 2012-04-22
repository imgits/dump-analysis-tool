#pragma once

#include "SearchFilter.h"

//----------------------------------------------------------------- ToolMemorySearchObject

//! 메모리에서 오브젝트를 검색하는 툴
namespace ToolMemorySearchObject
{
	//! 검색 조건
	struct Param
	{
		wstring dmpFilePath;		//!< 대상 덤프 파일 경로
		string objectNamePattern;	//!< 객체를 찾을 때 이 패턴을 가진 타입의 객체만 포함
		SearchFilter filter;		//!< 찾을 바이트 필터
		int searchSizeForFilter;	//!< 객체 내부에서 필터 조건을 검색할 때 찾을 범위 (=sizeof(타입))
	
	public:
		Param();
		bool parseParam(const vector<wstring>& args);
	};

	bool run(const Param& param);
}

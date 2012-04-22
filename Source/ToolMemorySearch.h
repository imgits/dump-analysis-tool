#pragma once

#include "SearchFilter.h"

//----------------------------------------------------------------------- ToolMemorySearch

//! 메모리에서 검색하는 툴
namespace ToolMemorySearch
{
	//! 검색 조건
	struct Param
	{
		wstring dmpFilePath;	//!< 대상 덤프 파일 경로
		SearchFilter filter;	//!< 검색 필터
	
	public:
		Param();
		bool parseParam(const vector<wstring>& args);
	};

	bool run(const Param& param);
}

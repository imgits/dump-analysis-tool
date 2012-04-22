#pragma once

//------------------------------------------------------------------------------- ToolLink

namespace ToolLink
{
	struct Param
	{
		wstring dmpFilePath;		//!< 대상 덤프 파일 경로
		wstring oldModuleName;		//!< 대상 모듈 파일 이름 (없으면 newModulePath 에서 알아냄)
		wstring newModulePath;		//!< 변경될 모듈 경로
		bool	linkModulePath;		//!< 대상 덤프 파일의 연결된 모듈 경로를 새롭게 수정

	public:
		Param();
		bool parseParam(const vector<wstring>& args);
	};

	bool run(const Param& param);
}

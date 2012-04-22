#pragma once

//------------------------------------------------------------------------------- ToolInfo

namespace ToolInfo
{
	struct Param
	{
		wstring dmpFilePath;		//!< 대상 덤프 파일 경로

	public:
		Param();
		bool parseParam(const vector<wstring>& args);
	};

	bool run(const Param& param);
}

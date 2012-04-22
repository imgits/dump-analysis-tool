#pragma once

//-------------------------------------------------------------------------- ToolCallStack

namespace ToolCallStack
{
	struct Param
	{
		wstring dmpFilePath;		//!< 대상 덤프 파일 경로
		uint32 threadId;			//!< 대상 ThreadId (지정하지 않으면 예외가 난 쓰레드)
		bool stopOnCommonEntry;		//!< main 등 보통의 EntryPoint 가 나오면 중단

	public:
		Param();
		bool parseParam(const vector<wstring>& args);
	};

	bool run(const Param& param);
}

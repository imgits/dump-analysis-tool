#pragma once

//--------------------------------------------------------------------------- SearchFilter

struct SearchFilter
{
	vector<byte> pattern;		//!< 찾을 패턴의 바이트열
	int32 alignSize;			//!< 주소 정렬이 있다면 정렬 바이트 수 (없으면 1)

public:
	SearchFilter();
	SearchFilter(int64 value, int32 valueSize, int32 alignSize);
	SearchFilter(const char* str);
	SearchFilter(const wchar_t* str);
	
public:
	bool empty() const;
	bool setFromOptStr(const wstring& str);
};

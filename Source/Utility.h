#pragma once

//-------------------------------------------------------------------------------- Utility

//! 주어진 주소를 정렬 바이트에 맞추는 매크로 (정렬이 필요하면 주소를 증가시킴)
#define ALIGN_UP(addr, align) (((addr)+((align)-1)) & (~((align)-1)))

//! 유니코드 문자열을 MBCS 문자열로 변환
string wstr2mbcs(const wstring& str);

//! 유니코드 문자열을 UTF-8 로 인코딩해 반환
string wstr2utf8(const wstring& str);

//! 유니코드 문자열을 UTF-8 로 인코딩해 반환
wstring utf82wstr(const string& str);

//! 경로에서 파일 이름을 반환
wstring getFileNameFromPath(const wstring& str);

//! 주어진 args 를 파싱해 결과를 반환합니다.
bool parseOpt(
	const vector<wstring>& args, const wchar_t* optFormat, 
	map<wchar_t, wstring>* optMap, vector<wstring>* leftArgs);

//! OptMap 에서 opt 에 해당하는 값이 있는지 여부를 반환합니다.
bool isOpt(const map<wchar_t, wstring>& optMap, wchar_t opt);

//! OptMap 에서 opt 에 해당하는 값을 찾아 반환합니다. 없으면 defaultValue 를 반환합니다.
wstring getOptValue(
	const map<wchar_t, wstring>& optMap, wchar_t opt, 
	wstring defaultValue = wstring());

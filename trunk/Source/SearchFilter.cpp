#include "Precompiled.h"
#include "SearchFilter.h"
#include "Utility.h"
#include <unordered_map>

//--------------------------------------------------------------------------- SearchFilter

SearchFilter::SearchFilter()
	: alignSize(1)
{
}

SearchFilter::SearchFilter(int64 value, int32 valueSize, int32 alignSize)
	: alignSize(alignSize)
{
	assert(valueSize >= 1 && valueSize <= 8);
	pattern.resize(valueSize);
	memcpy(&pattern[0], &value, valueSize);
}

SearchFilter::SearchFilter(const char* str)
	: alignSize(1)
{
	int32 len = strlen(str);
	pattern.resize(len * sizeof(char));
	memcpy(&pattern[0], str, len * sizeof(char));
}

SearchFilter::SearchFilter(const wchar_t* str)
	: alignSize(1)
{
	int32 len = wcslen(str);
	pattern.resize(len * sizeof(wchar_t));
	memcpy(&pattern[0], str, len * sizeof(wchar_t));
}

bool SearchFilter::empty() const
{
	return pattern.empty();
}

bool SearchFilter::setFromOptStr(const wstring& str)
{
	switch (str[0])
	{
	case L'i': {
		int v = wcstol(&str[1], NULL, 0);
		pattern.resize(sizeof(v));
		memcpy(&pattern[0], &v, sizeof(v));
		alignSize = sizeof(v);
		return true;
		}

	case L'u': {
		unsigned v = (unsigned)_wcstoi64(&str[1], NULL, 0);
		pattern.resize(sizeof(v));
		memcpy(&pattern[0], &v, sizeof(v));
		alignSize = sizeof(v);
		return true;
		}

	case L'f': {
		float v = (float)_wtof(&str[1]);
		pattern.resize(sizeof(v));
		memcpy(&pattern[0], &v, sizeof(v));
		alignSize = sizeof(v);
		return true;
		}

	case L'd': {
		double v = _wtof(&str[1]);
		pattern.resize(sizeof(v));
		memcpy(&pattern[0], &v, sizeof(v));
		alignSize = sizeof(v);
		return true;
		}

	case L's': {
		string v = wstr2mbcs(&str[1]);
		pattern.resize(v.length() * sizeof(char));
		memcpy(&pattern[0], v.c_str(), pattern.size());
		alignSize = 1;
		return true;
		}

	case L'w': {
		wstring v = &str[1];
		pattern.resize(v.length() * sizeof(wchar_t));
		memcpy(&pattern[0], v.c_str(), pattern.size());
		alignSize = 1;
		return true;
		}

	default:
		return false;
	}
}

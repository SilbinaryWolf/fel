#ifndef WIN32_STRING_INCLUDE
#define WIN32_STRING_INCLUDE

#include <windows.h>
#include "string.h"

inline String WcharToUTF8String(AllocatorPool* pool, LPCWSTR string) {
	String result = {};
	u32 stringLength = wcslen(string);
	result.length = WideCharToMultiByte(CP_UTF8, 0, string, stringLength, NULL, 0, NULL, NULL);
	result.data = (char*)pushSize(result.length + 1, pool);
	WideCharToMultiByte(CP_UTF8, 0, string, stringLength, result.data, result.length, NULL, NULL);
	return result;
}

#endif
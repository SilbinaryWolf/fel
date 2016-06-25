#ifndef STRING_INCLUDE
#define STRING_INCLUDE

#include "memory.h"
#include <string.h>

struct String {
	s32 length; // -should- exclude null-termination byte
	char* data;
public:
	f32 toFloat();
	f64 toDouble();
	inline bool cmp(String& str) {
		if (str.length != length)
		{
			return false;
		}
		char* it_a = data;
		char* it_b = str.data;
		auto i = length;
		for (i = 0; i < length; ++i)
		{
			if (it_a[i] != it_b[i])
			{
				return false;
			}
		}
		return true;
	}
	inline bool cmp(char* str) {
		char* it_a = data;
		char* it_a_end = data + length;
		char* it_b = str;
		bool isEqual = true;
		for (;it_a < it_a_end;++it_a, ++it_b)
		{
			if (it_a[0] != it_b[0])
			{
				return false;
			}
		}
		// If at the end of string a and string b isn't yet finished (ie. null terminated)
		if (it_a == it_a_end && it_b[0] != 0) {
			return false;
		}
		return true;
	}
	inline static String create(const char* string)
	{
		String data = {};
		data.data = (char*)string;
		data.length = strlen(string);
		return data;
	}
	inline void toCString(char* dest, s32 destLength) {
		assert(destLength > 0);
		assert(destLength >= length);
		char* srcIt = data;
		char* srcEnd = data + length;
		char* destIt = dest;
		char* destEnd = dest + length;
		for (;destIt < destEnd; ++srcIt, ++destIt)
		{
			*destIt = *srcIt;
		}
		dest[length] = '\0';
	}
	inline String substring(s32 offset)
	{
		char* newStartOfString = data + offset;
		char* endOfString = data + length;
		s32 newLength = endOfString - newStartOfString;
		if (newLength <= 0)
		{
			// Return empty string if trimmed whole thing
			String result = {};
			result.data = data;
			result.length = 0;
			return result;
		}
		String result = {};
		result.length = newLength;
		result.data = newStartOfString;
		return result;
	}
	inline String goUpDirectory(s32 count = 1) {
		assert(count > 0);
		if (length > 0)
		{
			s32 directoriesCounted = 0;
			char* endOfString = data + length;
			char* it = endOfString - 1;
			// Skip trailing slash
			if (it[0] == '\\' || it[0] == '/')
			{
				--it;
			}
			for (; it > data; --it)
			{
				if (it[0] == '\\' || it[0] == '/')
				{
					++directoriesCounted;
					if (directoriesCounted == count)
					{
						++it;
						String result = {};
						result.data = data;
						result.length = it - data;
						return result;
					}
				}
			}
		}
		String nullResult = {};
		nullResult.data = data;
		nullResult.length = 0;
		return nullResult;
	}
	inline String basename() 
	{
		char* endOfString = data + length;
		for (char* it = endOfString - 1; it > data; --it)
		{
			if (it[0] == '\\' || it[0] == '/')
			{
				++it;
				String result = {};
				result.data = it;
				result.length = endOfString - it;
				return result;
			}
		}
		String nullResult = {};
		nullResult.data = data;
		nullResult.length = 0;
		return nullResult;
	}
	inline String fileExtension()
	{
		char* endOfString = data + length;
		for (char* it = endOfString - 1; it > data; --it)
		{
			if (it[0] == '.')
			{
				++it;
				String result = {};
				result.data = it;
				result.length = endOfString - it;
				return result;
			}
		}
		String nullResult = {};
		nullResult.data = data;
		nullResult.length = 0;
		return nullResult;
	}
	//inline static String create(u16* string, AllocatorPool* allocator);
};

struct StringLinkedListNode {
	String string;
	StringLinkedListNode* next;
};

struct StringLinkedList {
	u32 used;
	StringLinkedListNode* first;
	StringLinkedListNode* last;
public:
	inline void add(String string, AllocatorPool* pool) {
		StringLinkedListNode* node = pushStruct(StringLinkedListNode, pool);
		node->string = string;
		++used;

		if (first == NULL)
		{
			last = first = node;
		}
		else
		{
			last->next = node;
			last = last->next;
		}
	}
};

#endif
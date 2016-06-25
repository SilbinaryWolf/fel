#ifndef STRING_BUILDER_INCLUDE
#define STRING_BUILDER_INCLUDE

#include "string.h"

struct StringBuilder {
	s32 used;
	s32 size;
	s32 totalLength;
	String* strings;
	AllocatorPool* _pool;
public:
	StringBuilder(s32 arrSize, TemporaryPool pool) {
		init(arrSize, pool._allocator);
	}

	static StringBuilder* create(u32 size, AllocatorPool* pool) {
		assert(size > 0);
		StringBuilder* result = pushStruct(StringBuilder, pool);
		result->init(size, pool);
		return result;
	}

	inline static StringBuilder* create(u32 size, TemporaryPool pool) {
		return create(size, pool._allocator);
	}

	void add(String string) {
		assert(used < size);
		strings[used] = string;
		totalLength += string.length;
		++used;
	}

	void add(char* string) {
		add(String::create(string));
	}

	String toString(AllocatorPool* pool) {
		String result;
		zeroMemory(&result, sizeof(result));
		if (totalLength == 0)
		{
			return result;
		}

		result.data = (char*)pushSize(totalLength, pool);
		for (s32 s = 0; s < used; ++s)
		{
			String* string = &strings[s];
			for (s32 i = 0; i < string->length; ++i)
			{
				assert(result.length < totalLength);
				result.data[result.length] = string->data[i];
				++result.length;
			}
		}
		return result;
	}

	inline String toString(TemporaryPool pool) {
		return toString(pool._allocator);
	}
private:
	inline void init(s32 arrSize, AllocatorPool* pool) {
		assert(arrSize > 0);
		zeroMemory(this, sizeof(*this));
		size = arrSize;
		strings = pushArrayStruct(String, size, pool);
	}
};

#endif
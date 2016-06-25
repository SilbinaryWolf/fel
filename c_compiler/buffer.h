#ifndef BUFFER_INCLUDE
#define BUFFER_INCLUDE

#include <stdarg.h>
#include "print.h"

// http://codereview.stackexchange.com/questions/96354/sample-printf-implementation

struct Buffer {
	s32 used;
	s32 size;
	char* data;
	AllocatorPool* _pool;

	// String helper
	s32 indent;
public:
	Buffer(s32 size, TemporaryPool pool) {
		zeroMemory(this, sizeof(*this));
		init(size, pool._allocator);
	}
	inline static Buffer* create(s32 size, AllocatorPool* pool)
	{
		Buffer* result = pushStruct(Buffer, pool);
		result->init(size, pool);
		return result;
	}
	void clear() { used = 0; }
	void print()
	{
		printf("%*.*s", used, used, data);
	}
	void addNewline() 
	{
		add("\n");
		assert(indent >= 0);
		for (s32 i = 0; i < indent; ++i)
		{
			add("     ");
		}
	}
	inline void add(char character)
	{
		char* dataPlusOffset = data + used;
		char* dataEndPointer = data + size;
		dataPlusOffset[0] = character;
		++dataPlusOffset;
		assert(dataPlusOffset < dataEndPointer);
		++used;
	}
	void add(char* format, ...)
	{
		va_list valist;
		va_start(valist, format);
		char* dataPlusOffset = data + used;
		char* dataEndPointer = data + size;
		while(format[0] != '\0')
		{
			if(format[0] == '%')
			{
				char temp[256] = "";

				switch (format[1])
				{
					// Non-Decimal Number
					case 'd':
					{
						int	iTemp = va_arg(valist, int);
						_itoa(iTemp, temp, 10); 
						s32 tempLength = strlen(temp);

						for(int i = 0; i < tempLength; i++)
						{
							assert(dataPlusOffset < dataEndPointer);
							*dataPlusOffset = temp[i];
							++dataPlusOffset;
						}
					}
					break;

					// Decimal Number
					case 'f':
					{
						#define FLOAT_CHAR_SIZE 50
						double dTemp = va_arg(valist, double);
						char output[FLOAT_CHAR_SIZE];
						snprintf(output, FLOAT_CHAR_SIZE, "%f", dTemp);
						for(int i = 0; i < FLOAT_CHAR_SIZE; i++)
						{
							if (output[i] == '\0') {
								break;
							}
							assert(dataPlusOffset < dataEndPointer);
							*dataPlusOffset = output[i];
							++dataPlusOffset;
						}
						#undef FLOAT_CHAR_SIZE
					}
					break;

					// String class
					case 's':
					{
						String* strTemp = va_arg(valist, String*);
						assert(strTemp->length > 0);
						assert(strTemp->length < Kilobytes(1)); // NOTE(Jake): Catch unlikely case/Memory problem
						for (s32 i = 0; i < strTemp->length; ++i)
						{
							assert(dataPlusOffset < dataEndPointer);
							*dataPlusOffset = strTemp->data[i];
							++dataPlusOffset;
						}
					}
					break;

					default:
					{
						assert(false);
					}
					break;
				}

				format += 2;
			}
			else
			{
				assert(dataPlusOffset < dataEndPointer);
				*dataPlusOffset = format[0];
				++dataPlusOffset;
				++format;
			}
		}
		assert(dataPlusOffset < dataEndPointer);
		used = size - (dataEndPointer - dataPlusOffset);
		va_end(valist);
	}
private:
	inline void init(s32 allocSize, AllocatorPool* pool) {
		size = allocSize;
		assert(size > 0);
		_pool = pool;
		data = (char*)pushSize(size, pool);
		used = 0;
	}
};

#endif
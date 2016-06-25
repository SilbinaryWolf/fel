#ifndef PRINT_INCLUDE
#define PRINT_INCLUDE

// Used this as a starting point for custom print function
// http://codereview.stackexchange.com/questions/96354/sample-printf-implementation

#include "string.h"
#include "tokens.h"
#include <stdarg.h>
#include <math.h>

inline void print(char* format, ...)
{
	va_list valist;
	va_start(valist, format);
	while(format[0] != '\0')
	{
		if(format[0] == '%')
		{
			switch (format[1])
			{
				case 'd':
				{
					char temp[256] = "";
					int	iTemp = va_arg(valist, int);
					_itoa(iTemp, temp, 10); 
					printf("%s", temp);
				}
				break;

				// String class
				case 's':
				{
					String* strTemp = va_arg(valist, String*);
					assert(strTemp->length >= 0);
					assert(strTemp->length < Kilobytes(1)); // NOTE(Jake): Catch unlikely case.
					for (s32 i = 0; i < strTemp->length; ++i)
					{
						putchar(strTemp->data[i]);
					}
				}
				break;

				default:
				{
					// Not implemented
					assert(false);
				}
				break;
			}

			format += 2;
		}
		else
		{
			putchar(format[0]);
			++format;
		}
	}
}

// Catch accidental misuse of 'print' with strings
inline void print(char* format, String str) { assert(false); }
inline void print(char* format, String** str) { assert(false); }
inline void print(char* format, Token str) { assert(false); }
inline void print(char* format, Token** str) { assert(false); }

// snprintf support in VS2010
// http://stackoverflow.com/questions/2915672/snprintf-and-visual-studio-2010
#if defined(_MSC_VER) && _MSC_VER < 1900

#define snprintf c99_snprintf
#define vsnprintf c99_vsnprintf

__inline int c99_vsnprintf(char *outBuf, size_t size, const char *format, va_list ap)
{
    int count = -1;

    if (size != 0)
        count = _vsnprintf_s(outBuf, size, _TRUNCATE, format, ap);
    if (count == -1)
        count = _vscprintf(format, ap);

    return count;
}

__inline int c99_snprintf(char *outBuf, size_t size, const char *format, ...)
{
    int count;
    va_list ap;

    va_start(ap, format);
    count = c99_vsnprintf(outBuf, size, format, ap);
    va_end(ap);

    return count;
}

#endif

#endif
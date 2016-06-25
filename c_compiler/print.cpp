#include "print.h"

void print(char* format, ...)
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
					puts(temp);
				}
				break;

				// String class
				case 's':
				{
					String* strTemp = va_arg(valist, String*);
					assert(strTemp->length > 0);
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
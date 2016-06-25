#include "string.h"

f32 String::toFloat()
{
	// NOTE(Jake): Copy pasted straight from stackoverflow, cheers Ruslik and Karl
	// http://stackoverflow.com/questions/4392665/converting-string-to-float-without-atof-in-c
	char* s = this->data;
	f32 rez = 0, fact = 1;
	if (*s == '-')
	{
		s++;
		fact = -1;
	}
	s32 point_seen = 0;
	for (s32 i = 0; i < this->length; ++i, ++s)
	{
		if (*s == '.')
		{
			point_seen = 1; 
			continue;
		}
		s32 d = *s - '0';
		if (d >= 0 && d <= 9)
		{
			if (point_seen) fact /= 10.0f;
			rez = rez * 10.0f + (f32)d;
		}
	}
	return rez * fact;
}

f64 String::toDouble()
{
	// NOTE(Jake): Copy pasted straight from stackoverflow, cheers Ruslik and Karl
	// http://stackoverflow.com/questions/4392665/converting-string-to-float-without-atof-in-c
	char* s = this->data;
	f64 rez = 0, fact = 1;
	if (*s == '-')
	{
		s++;
		fact = -1;
	}
	s32 point_seen = 0;
	for (s32 i = 0; i < this->length; ++i, ++s)
	{
		if (*s == '.')
		{
			point_seen = 1; 
			continue;
		}
		s32 d = *s - '0';
		if (d >= 0 && d <= 9)
		{
			if (point_seen) fact /= 10.0f;
			rez = rez * 10.0f + (f64)d;
		}
	}
	return rez * fact;
}
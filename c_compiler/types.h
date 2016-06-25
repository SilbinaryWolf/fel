#ifndef TYPES_INCLUDE_H
#define TYPES_INCLUDE_H

#define assert(Expression) if(!(Expression)) {*(int *)0 = 0;}

// NOTE: May not be supported on older C/C++ standards
#define _CRT_SECURE_NO_DEPRECATE
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <float.h>
 
typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef size_t memory_index;
    
typedef float f32;
typedef double f64;

#define Real32Maximum FLT_MAX

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#if !defined(internal)
#define internal static
#endif
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f
#define Tau32 6.28318530717958647692f

#endif
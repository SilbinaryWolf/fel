#ifndef MEMORY_INCLUDE_H
#define MEMORY_INCLUDE_H

#include "types.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MEMORY_DEFAULT_ALIGNMENT 4

enum AllocatorTypes {
	ALLOCATOR_POOL = 1,
	ALLOCATOR_POOL_TRANSIENT = 2,
};

inline void zeroMemory(void* src, memory_index size) {
	u8 *byte = (u8*)src;
    while(size--)
    {
        *byte++ = 0;
    }
}

struct Allocator {
	void* base;
	memory_index size;
	memory_index used;
};

struct AllocatorPool : Allocator {
	s32 tempCount;
	inline static AllocatorPool* createFromOS(memory_index size);
	inline AllocatorPool* create(memory_index size);
	// NOTE(Jake): Once freed, do not make any calls/stop using the Allocator as it will
	//			   no longer exist.
	inline void free();
};

struct TemporaryPool
{
    AllocatorPool* _allocator;
    memory_index used;

	inline void begin(AllocatorPool* allocator)
	{
		assert(allocator != NULL);
		assert(allocator->size == 8388608); // NOTE(Jake): Put in place to detect when I used the wrong pool to fix.
		zeroMemory(this, sizeof(this));
		this->_allocator = allocator;
		this->used = allocator->used;
		++allocator->tempCount;
	}

	inline void end()
	{
		assert(_allocator->used >= used);
		_allocator->used = used;
		assert(_allocator->tempCount > 0);
		--_allocator->tempCount;
	}
};

struct TemporaryPoolScope : TemporaryPool
{
	TemporaryPoolScope(AllocatorPool* pool)
	{
		begin(pool);
	}
	~TemporaryPoolScope()
	{
		end();
	}
};

inline internal memory_index getAlignmentOffset(memory_index base, memory_index used, memory_index alignment = MEMORY_DEFAULT_ALIGNMENT)
{
    memory_index alignmentOffset = 0;
    memory_index resultPointer = base + used;
    memory_index alignmentMask = alignment - 1;
    if(resultPointer & alignmentMask)
    {
        alignmentOffset = alignment - (resultPointer & alignmentMask);
    }
    return(alignmentOffset);
}

inline void* pushSize_(AllocatorPool* allocator, memory_index sizeInit) {
	assert(allocator != NULL);
	memory_index size = sizeInit;
	memory_index alignmentOffset = getAlignmentOffset((memory_index)allocator->base, allocator->used, MEMORY_DEFAULT_ALIGNMENT);
    size += alignmentOffset;
    
    assert((allocator->used + size) <= allocator->size);
    void* result = (char *)allocator->base + allocator->used + alignmentOffset;
    zeroMemory(result, size);
    allocator->used += size;

    assert(size >= sizeInit);
    
    return(result);
}

inline void* pushSize_(TemporaryPool temp, memory_index sizeInit) {
	return pushSize_(temp._allocator, sizeInit);
}

#define pushSize(size, allocator, ...) pushSize_(allocator, size, ## __VA_ARGS__)
#define pushStruct(type, allocator, ...) (type *)pushSize_(allocator, sizeof(type), ## __VA_ARGS__)
#define pushArrayStruct(type, size, allocator, ...) (type *)pushSize_(allocator, sizeof(type) * size, ## __VA_ARGS__)

#define pushSizeCurrent(size, ...) pushSize(size, g_allocator, ## __VA_ARGS__)
#define pushStructCurrent(type, ...) pushStruct(type, g_allocator, ## __VA_ARGS__)
#define pushArrayStructCurrent(type, size, ...) pushArrayStruct(type, size, g_allocator, ## __VA_ARGS__)

AllocatorPool* AllocatorPool::createFromOS(memory_index size) 
{
	AllocatorPool stackPool;
	zeroMemory(&stackPool, sizeof(stackPool));
	stackPool.base = malloc(size);
	stackPool.size = size;

	AllocatorPool* pool = pushStruct(AllocatorPool, &stackPool);
	*pool = stackPool;
	return pool;
}

AllocatorPool* AllocatorPool::create(memory_index size)
{
	AllocatorPool* pool = pushStruct(AllocatorPool, this);
	pool->base = pushSize(size, this);
	pool->size = size;
	return pool;
}

void AllocatorPool::free() 
{
	assert(tempCount == 0);
	::free(base);
}

#endif
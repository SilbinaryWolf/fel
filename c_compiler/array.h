#ifndef ARRAY_INCLUDE
#define ARRAY_INCLUDE

// Foreach Macro: http://stackoverflow.com/questions/400951/does-c-have-a-foreach-loop-construct
// Credits: Johannes Schaub - litb
/*#define foreach(item, arr) \
	item = arr->data[0] \
    for(s32 _index = 0; _index < arr->used; ++_index, item = arr->data[_index])*/
/*#define foreach(item, array) \
    for(size_t _breakKeywordPatch = 1, count = 0, _breakKeywordPatch && count != array->used; _breakKeywordPatch = !_breakKeywordPatch, count++) \
    for(item = (array->data) + count; _breakKeywordPatch; _breakKeywordPatch = !_breakKeywordPatch)*/

template <typename T> 
struct Array { 
	s32 size;
	s32 used;
	T* data;
	bool isTemporary;
	AllocatorPool* _pool;
	Array(s32 arrSize, TemporaryPool pool) {
		// todo(Jake): Update Array to be consistent with StringBuilder temporarypool allocate, the reason
		//			   StringBuilder does it that way is for the evaluiateExpression function as it needs to
		//			   pass the pointer around.
		//
		// deprecatethis(Jake)
		//
		init(arrSize, pool._allocator);
		allocate();
	}
	inline static Array* create(s32 size, AllocatorPool* pool) {
		Array* result = pushStruct(Array, pool);
		result->init(size, pool);
		assert(result != NULL);
		return result;
	}
	inline static Array* create(s32 size, TemporaryPool pool) {
		Array* result = pushStruct(Array, pool._allocator);
		result->init(size, pool._allocator);
		result->allocate();
		assert(result != NULL);
		result->isTemporary = true;
		return result;
	}
	// Creates a copy of the array on an alternate allocator and lowers the size.
	// ie. You can allocate a huge array on a temporary pool, then copy only what
	//	   you used into the non-temporary pool.
	inline Array* createCopyShrinkToFit(AllocatorPool* poolToAllocateNewArrayOn) {
		if (used == 0)
		{
			return NULL;
		}
		Array* result = pushStruct(Array, poolToAllocateNewArrayOn);
		result->init(used, poolToAllocateNewArrayOn);
		result->allocate();
		result->used = used;
		memcpy(result->data, data, used * sizeof(T));
		return result;
	}
	inline Array* createCopy(s32 newSize, AllocatorPool* poolToAllocateNewArrayOn) {
		if (used == 0)
		{
			return NULL;
		}
		if (newSize == 0)
		{
			newSize = size;
		}
		assert(newSize > 0);
		Array* result = pushStruct(Array, poolToAllocateNewArrayOn);
		result->init(newSize, poolToAllocateNewArrayOn);
		result->allocate();
		result->used = used;
		memcpy(result->data, data, used * sizeof(T));
		return result;
	}
	inline Array* createCopy(s32 newSize, TemporaryPool temporaryPoolToAllocateNewArrayOn) {
		Array* result = createCopy(newSize, temporaryPoolToAllocateNewArrayOn._allocator);
		result->isTemporary = true;
		return result;
	}
	// Generic
	inline void resize(s32 newSize) {
		assert(newSize > size); // can only resize upwards for Pool
		assert(used > 0);

		void* oldData = data;
		s32 oldSize = size;
		// Prepare for 'allocate()' function
		size = newSize;
		allocate();
		memcpy(data, oldData, used * sizeof(T));
	}
	inline s32 find(String value) {
		for (s32 i = 0; i < used; ++i)
		{
			if (value.cmp(data[i]))
			{
				return i;
			}
		}
		return -1;
	}
	inline s32 find(void* value) {
		for (s32 i = 0; i < used; ++i)
		{
			if (data[i] == value)
			{
				return i;
			}
		}
		return -1;
	}
	inline T top() {
		assert(used > 0);
		return data[used-1];
	}
	inline T pop() {
		assert(used > 0);
		return data[--used];
	}
	inline void push(T item) {
		assert(used < size);
		if (data == 0)
		{
			allocate();
		}
		data[used] = item;
		++used;
	}
	inline void add(Array* arr)
	{
		for (s32 i = 0; i < arr->used; ++i)
		{
			this->push(arr->data[i]);
		}
	}
private:
	inline void init(s32 arrSize, AllocatorPool* pool) {
		zeroMemory(this, sizeof(*this));
		size = arrSize;
		_pool = pool;
	}
	inline void allocate() {
		data = pushArrayStruct(T, size, _pool);
		assert(data);
	}
};

/*#define Foreach(type, arr)	\
							type _end = arr.data + arr.used; \
							for (type it = arr.data; it < _end; ++it)

*/					

#endif
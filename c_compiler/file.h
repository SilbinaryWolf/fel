#ifndef PLATFORM_FILE_INCLUDE
#define PLATFORM_FILE_INCLUDE

#include "memory.h"
#include "buffer.h"
#include "string.h"
#include "string_builder.h"

namespace File
{
	enum ErrorCodes {
		FILE_NO_ERROR = 0,
		FILE_NO_MEMORY = 1,
		FILE_CANT_OPEN = 2,
		FILE_NO_WRITE = 3,
	};

	struct Error {
		u8 errorCode;
	};

	String readEntireFile(String filepath, Error* error = NULL);
	void writeEntireFile(String filename, Buffer* buffer, Error* error = NULL);
}

namespace Directory
{
	enum ErrorCodes {
		DIRECTORY_NO_ERROR = 0,
		DIRECTORY_NO_FILES = 1,
		DIRECTORY_INVALID_DIR = 2,
	};

	struct Error {
		u8 errorCode;
	};

	StringLinkedList getFilesRecursive(AllocatorPool* pool, String directory, Error* error = NULL);
}

#endif
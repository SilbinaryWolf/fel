#include "file.h"

namespace File
{
	String readEntireFile(String filename, Error* error) {
		zeroMemory(error, sizeof(File::Error));

		FILE* f = fopen(filename.data, "rb");
		if (f == 0)
		{
			print("readEntireFile:: Invalid file. File = %s\n", &filename);
			if (error) {
				error->errorCode = FILE_CANT_OPEN;
			}
			String sNull = {};
			return sNull;
		}
		fseek(f, 0, SEEK_END);
		u32 fsize = ftell(f);
		fseek(f, 0, SEEK_SET);

		if (fsize == 0) {
			String sNull = {};
			return sNull;
		}

		String sfileData = {};
		sfileData.data = (char*)malloc(fsize + 1);
		if (sfileData.data == 0)
		{
			print("readEntireFile:: Memory allocation error. Size = %d\n", fsize + 1);
			if (error) {
				error->errorCode = FILE_NO_MEMORY;
			}
			String sNull = {};
			return sNull;
		}
		sfileData.length = fsize;
		fread(sfileData.data, fsize, 1, f);
		sfileData.data[fsize] = 0; // Null terminate file contents
		fclose(f);
		return sfileData;
	}

	void writeEntireFile(String filename, Buffer* buffer, Error* error) {
		assert(buffer != NULL);
		assert(buffer->data != NULL);
		assert(buffer->used >= 0);

		char cFilename[260];
		filename.toCString(cFilename, ArrayCount(cFilename));

		FILE* f = fopen(cFilename, "wb");
		if (f == 0)
		{
			print("writeEntireFile:: Unable to open file for writing. File = %s\n", &filename);
			if (error) {
				error->errorCode = FILE_CANT_OPEN;
			}
			return;
		}
		s32 bytesWritten = fwrite(buffer->data, sizeof(char), buffer->used, f);
		if (bytesWritten != buffer->used)
		{
			print("writeEntireFile:: Failed to write. File = %s\n", &filename);
			if (error) {
				error->errorCode = FILE_NO_WRITE;
			}
			return;
		}
		fclose(f);
	}
}
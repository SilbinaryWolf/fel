#include "file.h"
#include <windows.h>
#include "win32_string.h"
#include <strsafe.h>

namespace Directory
{
	StringLinkedList getFilesRecursive(AllocatorPool* pool, String directory, Error* error)
	{
		if (directory.length == 0)
		{
			StringLinkedList nullResult = {};
			return nullResult;
		}

		char* lastCharacter = &directory.data[directory.length - 1];
		if (lastCharacter[0] == '/' || lastCharacter[0] == '\\')
		{
			// Trim trailing back-or-forward slash
			directory.length -= 1;
		}

		assert(directory.length < MAX_PATH);

		// Convert String to C-String
		char baseDir[MAX_PATH];
		memcpy(baseDir, directory.data, directory.length);
		baseDir[directory.length] = '\0';

		// Convert C-String to WChar-String
		wchar_t szBaseDir[MAX_PATH];
		mbstowcs(szBaseDir, baseDir, MAX_PATH);

		const u32 directoryLimit = 100;
		wchar_t szDirectories[directoryLimit][MAX_PATH+10];
		u32 directories = 0;
		u32 files = 0;
		StringCchCopy(szDirectories[directories++], MAX_PATH, szBaseDir);

		StringLinkedList list = {};
		while (directories > 0)
		{
			--directories;

			wchar_t currentDirectory[MAX_PATH];
			StringCchCopy(currentDirectory, MAX_PATH, szDirectories[directories]);

			wchar_t currentDirectoryWSearch[MAX_PATH];
			StringCchCopy(currentDirectoryWSearch, MAX_PATH, currentDirectory);
			StringCchCat(currentDirectoryWSearch, MAX_PATH, TEXT("\\*"));

			WIN32_FIND_DATA ffd;
			HANDLE hFind = FindFirstFile(currentDirectoryWSearch, &ffd);
			DWORD dLastError = GetLastError();
			if (hFind == INVALID_HANDLE_VALUE || dLastError == ERROR_FILE_NOT_FOUND) 
			{
				if (error)
				{
					if (dLastError == ERROR_FILE_NOT_FOUND)
					{
						error->errorCode = DIRECTORY_NO_FILES;
					}
					else
					{
						error->errorCode = DIRECTORY_INVALID_DIR;
					}
				}
				StringLinkedList list_null = {};
				return list_null;
			}

			do
			{
				if (wcscmp(ffd.cFileName, TEXT(".")) != 0 && wcscmp(ffd.cFileName, TEXT("..")) != 0)
				{
					if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
					{
						// Push this directory to be searched next iteration.
						StringCchCopy(szDirectories[directories], MAX_PATH, currentDirectory);
						StringCchCat(szDirectories[directories], MAX_PATH, TEXT("\\"));
						StringCchCat(szDirectories[directories], MAX_PATH, ffd.cFileName);
						++directories;
					}
					else
					{
						wchar_t filepath[MAX_PATH];
						StringCchCopy(filepath, ArrayCount(filepath), currentDirectory);
						StringCchCat(filepath, ArrayCount(filepath), TEXT("\\"));
						StringCchCat(filepath, ArrayCount(filepath), ffd.cFileName);
						
						String string = WcharToUTF8String(pool, filepath);
						list.add(string, pool);
						++files;
					}
				}
			}
			while (FindNextFile(hFind, &ffd) != 0);
			FindClose(hFind);
		}
		return list;
	}
}
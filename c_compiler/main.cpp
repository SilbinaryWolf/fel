#include "types.h"
#include "parser.h"
#include "file.h"
#include "memory.h"
#include "string.h"
#include "code_generator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char** argv){
	// Initialize compiler
	Compiler compiler;
	zeroMemory(&compiler, sizeof(compiler));
	compiler.pool =  AllocatorPool::createFromOS(Megabytes(24));
	compiler.poolTransient = compiler.pool->create(Megabytes(8));
	compiler.astFiles = Array<AST_File>::create(1024, compiler.pool);
	compiler.stack = Array<CompilerParameters*>::create(256, compiler.pool);
	compiler.componentsUsed = Array<AST_ComponentDefinition*>::create(256, compiler.pool);

	// Get all files in directory
	compiler.targetDirectory = String::create("C:\\wamp\\www\\nweb\\test\\wordpress\\wp-content\\themes\\twentysixteen\\fel\\");
	StringLinkedList files = Directory::getFilesRecursive(compiler.pool, compiler.targetDirectory);
	if (files.first == NULL)
	{
		compiler.targetDirectory = String::create("D:\\wamp\\www\\Nweb\\test\\wordpress\\wp-content\\themes\\twentysixteen\\fel\\");
		files = Directory::getFilesRecursive(compiler.pool, compiler.targetDirectory);
		if (files.first == NULL)
		{	
			compiler.targetDirectory = String::create("C:\\wamp\\www\\PersonalProjects\\fel\\test\\wordpress\\wp-content\\themes\\twentysixteen\\fel\\");
			files = Directory::getFilesRecursive(compiler.pool, compiler.targetDirectory);
			if (files.first == NULL)
			{	
				printf("Invalid directory supplied. Terminating program.\n");
				while(true) {}
				return -1;
			}
		}
	}

	if (compiler.targetDirectory.length == 0) 
	{
		printf("No directory supplied. Terminating program.\n");
		while(true) {}
		return -1;
	}

	// Setup default output directory
	if (compiler.outputDirectory.length == 0)
	{
		compiler.outputDirectory = compiler.targetDirectory.goUpDirectory();
	}
	assert(compiler.outputDirectory.length != 0);

	// Parse each file and add the AST to the compilers files
	for (StringLinkedListNode* node = files.first; node != NULL; node = node->next)
	{
		// Only parse files with the '.fel' extension
		if (node->string.fileExtension().cmp("fel"))
		{
			parse(&compiler, node->string);
		}
	}
	printf("Finished parsing.\n");

	// Run compile
	compile(&compiler);
	if (compiler.hasError)
	{
		printf("Fatal error compiling.\n");
	}
	else
	{
		print("Finished compiling successfully. Memory Used: %d, Transient Memory Used: %d (should be 0).\n", compiler.pool->used - compiler.poolTransient->size, compiler.poolTransient->used);
	}
	while(true) {}
	return 0;
}

	// Get command line arguments
	/*char* prevCommand = "";
	for (int i = 0; i < argc; ++i)
	{
		if (strcmp(prevCommand, "--file") == 0)
		{
			fileData = ReadFile(argv[i]);
		}
		prevCommand = argv[i];
	}*/
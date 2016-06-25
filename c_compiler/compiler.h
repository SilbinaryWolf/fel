#ifndef COMPILER__INCLUDE
#define COMPILER__INCLUDE

struct CompilerParameters;

struct Compiler {
	bool hasError;
	Array<AST_File>* astFiles;
	Array<AST_ComponentDefinition*>* componentsUsed;
	String targetDirectory; // the directory to compile, ie. wp-content/themes/fel
	String outputDirectory; // the directory to output to
	Array<CompilerParameters*>* stack;
	AllocatorPool* pool;
	AllocatorPool* poolTransient;
};

#endif
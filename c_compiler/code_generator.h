#ifndef CODE_GENERATOR_INCLUDE
#define CODE_GENERATOR_INCLUDE

#include "print.h"
#include "ast.h"
#include "string_builder.h"
#include <stdio.h>
#include <stdlib.h>
#include "compiler.h"

#define compileError(format, ...) compiler->hasError = true; print("Compile error: "format"\n", ## __VA_ARGS__); assert(false)
#define compileErrorSub(format, ...) print("               -- "format"\n", ## __VA_ARGS__)
#define compileErrorSubSub(format, ...) print("                  "format"\n", ## __VA_ARGS__)

/*enum CompilerContextType {
	COMPILER_CONTEXT_UNKNOWN = 0,
	COMPILER_CONTEXT_COMPONENT_DEFINITION,
};*/

enum CompilerValueType {
	COMPILER_VALUE_TYPE_UNKNOWN = 0,
	COMPILER_VALUE_TYPE_UNDEFINED,
	COMPILER_VALUE_TYPE_STRING,
	COMPILER_VALUE_TYPE_STRING_BUILDER,
	//COMPILER_VALUE_TYPE_S32,
	//COMPILER_VALUE_TYPE_S64,
	//COMPILER_VALUE_TYPE_FLOAT,
	COMPILER_VALUE_TYPE_DOUBLE,
	COMPILER_VALUE_TYPE_BACKEND_IDENTIFIER,
	COMPILER_VALUE_TYPE_BACKEND_EXPRESSION,
	COMPILER_VALUE_TYPE_BACKEND_OPERATOR,
};

struct CompilerValue;

struct CompilerValue {
	CompilerValueType type;
	union
	{
		StringBuilder* valueStringBuilder;
		String valueString;
		//s32 valueIntS32;
		//s64 valueIntS64;
		//float valueFloat;
		double valueDouble;
		Array<CompilerValue>* valueExpression;
		AST_Expression_Token valueToken;
	};
	inline bool isTrue() {
		if (type == COMPILER_VALUE_TYPE_DOUBLE)
		{
			return valueDouble != 0;
		}
		else if (type == COMPILER_VALUE_TYPE_STRING)
		{
			return valueString.length != 0;
		}
		else
		{
			// Catch handling other variable cases that haven't been accounted for yet.
			assert(false);
		}
		return false;
	}
};

struct CompilerParameters {
	Array<Token>* names;
	Array<CompilerValue>* values;
};

void compile(Compiler* compiler);

#endif
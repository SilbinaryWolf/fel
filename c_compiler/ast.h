#ifndef AST_INCLUDE
#define AST_INCLUDE

#include "tokens.h"
#include "css.h"

enum AST_Types {
	AST_UNKNOWN = 0,
	AST_PARAMETERS,
	AST_IDENTIFIER,
	AST_COMPONENT,
	AST_FUNCTION,
	AST_BACKEND_IDENTIFIER,
	AST_BACKEND_FUNCTION,
	AST_TAG,
	AST_LAYOUT,
	AST_EXPRESSION,
	AST_IF,
	AST_LOOP,
	AST_WHILE,
	AST_COMPONENTDEFINITION,
	AST_FUNCTIONDEFINITION,
	AST_STYLESHEET,
	AST_STATEMENT,
};

struct AST {
	// Generic
	AST_Types type;
	AST* parent;

	// AST_Block - might need to refactor into own data structure later
	Array<AST*>* childNodes;
	Array<Token>* variables;
};

struct AST_Expression;

struct AST_Parameters : AST {
	Array<Token>* names;
	Array<AST_Expression*>* values;
};

struct AST_Layout : AST {
};

// todo(Jake): Rename to Expression_Token
struct AST_Expression_Token {
	Token name; // NOTE(Jake): Composited instead of inheritted so 'getToken' works with this nicely. 
	bool isFunction;
	AST_Parameters* parameters;
};

struct AST_Expression : AST {
	// NOTE(Jake): Is this necessary?
	Array<AST_Expression_Token>* tokens;
};

struct AST_Expression_Block : AST {
	AST_Expression expression;
};

struct AST_Stylesheet : AST {
	CSS_Rule* rule;
};

struct AST_FunctionDefinition : AST {
	Token name;
	AST_Parameters* parameters;
};

struct AST_ComponentDefinition : AST {
	Token name;
	AST_Parameters* properties;
	AST_Layout* layout;
	AST_Stylesheet* style;
	Array<AST_ComponentDefinition*>* components;
};

struct AST_Statement : AST {
	Token name;
	Token op;
	AST_Expression expression;
};

struct AST_Identifier : AST_Expression_Token, AST {
	union
	{
		// AST_COMPONENT type only
		AST_ComponentDefinition* definition;
	};
	AST_Expression expression; // used for 'when' keyword
};

struct AST_File : AST {
	String pathname;
	Array<AST_Layout>* layouts;
	Array<AST_ComponentDefinition>* components;
};

inline AST* setupASTType_(AST* ast, AST_Types type, AllocatorPool* pool)
{
	ast->type = type;
	ast->childNodes = Array<AST*>::create(1024, pool);
	return ast;
}

//#define ASTTypeToString(type) #type
//#define pushASTCurrent(type, enum) (type*)setupASTType_(pushStructCurrent(type), enum, g_allocator)
#define pushAST(type, enum, pool) (type*)setupASTType_(pushStruct(type, pool), enum, pool)

/*#define AST_CASE(struc, ast_type) case ast_type: { ast = pushStruct(pool, struc); ast->type = ast_type; } break
inline AST* pushAST(AST_Types type, AllocatorPool* pool)
{
	AST* ast = NULL;
	switch (type)
	{
		AST_CASE(AST_Parameters, AST_PARAMETERS);
		AST_CASE(AST_Layout, AST_LAYOUT);
		AST_CASE(AST_Comp_Or_Func, AST_COMP_OR_FUNC);

		default:
			assert(false);
		break;
	}
	return ast;
}
#undef AST_CASE

#define pushASTCurrent(type) pushAST(type, g_allocator)*/

#endif
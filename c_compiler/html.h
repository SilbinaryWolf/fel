#ifndef AST_HTML_INCLUDE
#define AST_HTML_INCLUDE

#include "ast.h"
#include "code_generator.h"

enum HTML_Types {
	HTML_UNKNOWN = 0,
	// Frontend
	HTML_ELEMENT,
	HTML_TEXT,
	// Backend
	HTML_BACKEND_IDENTIFIER,
	HTML_BACKEND_FUNCTION,
	HTML_BACKEND_IF,
	HTML_BACKEND_WHILE,
	//HTML_BACKEND_LOOP, // DEPRECATED: wont be allowed because 'loop' is for foreach'ing but moving all vars into this context
	// Special
	HTML_ROOT,
	HTML_ELEMENT_VIRTUAL,
	HTML_CHILDREN,
};

struct HTML {
	HTML_Types type;
	Token name;
	AST_ComponentDefinition* component; // Component this HTML was generated from. Is NULL if not component
	inline bool canHaveChildren() {
		return (type == HTML_ROOT || type == HTML_ELEMENT_VIRTUAL || type == HTML_ELEMENT 
				|| type == HTML_BACKEND_IF || type == HTML_BACKEND_WHILE);
	}
};

struct Array_HTML : Array<HTML*> {
	// NOTE(Jake): This uses static methods from the 'Array' class to instantiate
	//			   the properties. Therefore you CANNOT add more properties to 
	//		       the class at this level without some refactoring.
	/*inline void push(HTML* item) {
		// NOTE(Jake): Used to debug adding items to the array
		Array<HTML*>::push(item);
	}*/
};

struct HTML_Backend_Function : HTML {
	CompilerParameters* parameters;
};

struct HTML_Block : HTML {
	Array_HTML* childNodes;
};

struct HTML_Element : HTML_Block {
	CompilerParameters* parameters;
};

struct HTML_Backend_Expression_Block : HTML_Block {
	Array<CompilerValue>* expression;
};

struct HTML_Root : HTML_Element {
	HTML_Element* childInsertionElement; // Denoted by 'children' keyword, hints to where to insert child elements
};

inline HTML* setupHTMLType_(HTML* html_top, HTML_Types type, AllocatorPool* pool)
{
	html_top->type = type;
	if (html_top->canHaveChildren())
	{
		HTML_Block* html = (HTML_Block*)html_top;
		html->childNodes = (Array_HTML*)Array<HTML*>::create(16, pool);
	}
	return html_top;
}

#define pushHTML(type, enum, pool) (type*)setupHTMLType_(pushStruct(type, pool), enum, pool)

#endif
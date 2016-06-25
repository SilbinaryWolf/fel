#ifndef HTML_PRINT_INCLUDE
#define HTML_PRINT_INCLUDE

#include "html.h"
#include "buffer.h"
#include "print.h"

void printPostfixToInfix(Buffer* buffer, Array<CompilerValue>* expression);

inline void printCompilerValue(Buffer* buffer, CompilerValue value) {
	if (value.type == COMPILER_VALUE_TYPE_DOUBLE)
	{
		buffer->add("%f", value.valueDouble);
	}
	else if (value.type == COMPILER_VALUE_TYPE_STRING)
	{
		buffer->add("\"%s\"", &value.valueString);
	}
	else if (value.type == COMPILER_VALUE_TYPE_BACKEND_IDENTIFIER)
	{
		if (value.valueToken.isFunction) 
		{
			buffer->add("%s()", &value.valueToken);
		}
		else
		{
			buffer->add("%s", &value.valueToken);
		}
	}
	else if (value.type == COMPILER_VALUE_TYPE_BACKEND_EXPRESSION)
	{
		buffer->add("(");
		printPostfixToInfix(buffer, value.valueExpression);
		buffer->add(")");
	}
	else if (value.type == COMPILER_VALUE_TYPE_BACKEND_OPERATOR)
	{
		buffer->add("%s", &value.valueToken);
	}
	else
	{
		assert(false);
	}
}

inline void printPostfixToInfix(Buffer* buffer, Array<CompilerValue>* expression) {
	assert(buffer->_pool != NULL);
	TemporaryPoolScope tempPoolScope(buffer->_pool);

	Token nullToken;
	zeroMemory(&nullToken, sizeof(nullToken));

	Array<CompilerValue> stack(255, tempPoolScope);
	Array<Token> stackOp(255, tempPoolScope);

	for (s32 i = 0; i < expression->used; ++i)
	{
		CompilerValue it = expression->data[i];
		if (it.type == COMPILER_VALUE_TYPE_BACKEND_OPERATOR)
		{
			Token token = it.valueToken.name;
			if (token.type == TOKEN_MULTIPLY || token.type == TOKEN_DIVIDE)
			{
				assert(false);
				/*CompilerValue rval = stack.pop();
				Token rop = stackOp.pop();
				if (stack.used > 0)
				{
					CompilerValue lval = stack.pop();
					Token lop = stackOp.pop();

					switch (lop.type)
					{
						case TOKEN_PLUS:
						case TOKEN_MINUS:
							buffer->add("(%s)", &lval.token);
						break;

						default:
							buffer->add("%s", &lval.token);
						break;
					}
				}

				buffer->add(" %s ", &it.token);

				switch (rop.type)
				{
					case TOKEN_PLUS:
					case TOKEN_MINUS:
						buffer->add("(%s)", &rval.token);
					break;

					default:
						buffer->add("%s", &rval.token);
					break;
				}*/
			}
			else
			{
				CompilerValue rval = stack.pop();
				stackOp.pop();
				if (stack.used > 0)
				{
					CompilerValue lval = stack.pop();
					stackOp.pop(); // ignored
					printCompilerValue(buffer, lval);
				}
				printCompilerValue(buffer, it);
				printCompilerValue(buffer, rval);
			}
		}
		else
		{
			stack.push(it);
			stackOp.push(nullToken);
		}
	}
	if (stack.used == 1)
	{
		CompilerValue lval = stack.pop();
		stackOp.pop();
		printCompilerValue(buffer, lval);
	}
	assert(stack.used == 0);
	assert(stackOp.used == 0);
}

void printHTML(Buffer& buffer, HTML_Element* absoluteTopHTML) {
	assert(absoluteTopHTML != NULL);

	TemporaryPoolScope tempPoolScope(buffer._pool);
	Array<HTML*> stack(512, tempPoolScope);
	stack.push(absoluteTopHTML);
	Array<HTML*> closeStack(128, tempPoolScope);

	while (stack.used)
	{
		HTML* top = stack.pop();
	
		if (top == 0)
		{
			// Closing </element>, backend if/for, etc
			HTML* topClose = closeStack.pop();
			switch (topClose->type)
			{
				case HTML_ELEMENT:
				{
					HTML_Element* ast = (HTML_Element*)topClose;
					assert(ast->type == HTML_ELEMENT);
					if (ast->childNodes != NULL && ast->childNodes->used > 0) {
						--buffer.indent;
						buffer.addNewline();
					}
					buffer.add("</%s>", &ast->name);
					buffer.addNewline();
				}
				break;

				case HTML_BACKEND_IF:
				{
					--buffer.indent; buffer.addNewline();
					buffer.add("<?php endif; ?>");
				}
				break;

				case HTML_BACKEND_WHILE:
				{
					--buffer.indent; buffer.addNewline();
					buffer.add("<?php endwhile; ?>");
				}
				break;

				default:
					assert(false);
				break;
			}
			continue;
		}

		switch (top->type)
		{
			case HTML_UNKNOWN:
				printf("[HTML_UNKNOWN]");
				assert(false);
			break;

			case HTML_ROOT:
			{
				HTML_Element* ast = (HTML_Element*)top;
				if (ast->childNodes->used <= 0 && ast->component == NULL)
				{
					// NOTE(Jake): what happens with empty 'layout' block that is not in a component context??
					assert(false);
				}
			}
			break;

			case HTML_ELEMENT:
			{
				HTML_Element* ast = (HTML_Element*)top;
				//printf("<%*.*s", ast->name.length, ast->name.length, ast->name.data);
				buffer.add("<%s", &ast->name);
				if (ast->parameters != NULL && ast->parameters->names != NULL && ast->parameters->names->used > 0)
				{
					assert(ast->parameters->values != NULL);
					assert(ast->parameters->values->used == ast->parameters->names->used);

					for (s32 i = 0; i < ast->parameters->names->used; ++i)
					{
						Token* name = &ast->parameters->names->data[i];
						//AST_Expression* expr = ast->parameters->values->data[i];
						CompilerValue compileValue = ast->parameters->values->data[i];
						assert(compileValue.type == COMPILER_VALUE_TYPE_STRING);
						String* value = &compileValue.valueString;
						buffer.add(" %s=\"%s\"", name, value);
					}
					buffer.add(">");
				}
				else
				{
					buffer.add(">");
				}

				// Add closing tag (added first as it must be in reverse)
				stack.push(NULL); 
				closeStack.push(ast);
			} 
			break;

			case HTML_TEXT:
			{
				buffer.add("%s", top->name);
				buffer.addNewline();
			} 
			break;

			case HTML_BACKEND_FUNCTION:
			{
				// NOTE(Jake): I want to make the backend output configurable in the language in
				//			   the future, for now just support PHP.
				HTML_Backend_Function* ast = (HTML_Backend_Function*)top;
				buffer.add("<?php %s(", &ast->name);
				if (ast->parameters != NULL)
				{
					if (ast->parameters->names->used == 0 && ast->parameters->values->used > 0)
					{
						// Unnamed parameters
						for (s32 i = 0; i < ast->parameters->values->used; ++i)
						{
							if (i != 0) {
								buffer.add(", ");
							}
							printCompilerValue(&buffer, ast->parameters->values->data[i]);
						}
					}
					else
					{
						// Named parameters not supported
						assert(false);
					}
				}
				buffer.add("); ?>");
				buffer.addNewline();
			}
			break;

			case HTML_BACKEND_IDENTIFIER:
			{
				HTML* ast = (HTML*)top;
				buffer.add("<?php echo $%s; ?>", &ast->name);
				buffer.addNewline();
			}
			break;

			case HTML_BACKEND_IF:
			{
				HTML_Backend_Expression_Block* ast = (HTML_Backend_Expression_Block*)top;
				buffer.add("<?php if (");
				printPostfixToInfix(&buffer, ast->expression);
				buffer.add("): ?>");

				// Add closing tag
				stack.push(NULL); 
				closeStack.push(ast);
			}
			break;

			case HTML_BACKEND_WHILE:
			{
				HTML_Backend_Expression_Block* ast = (HTML_Backend_Expression_Block*)top;
				buffer.add("<?php while (");
				printPostfixToInfix(&buffer, ast->expression);
				buffer.add("): ?>");

				// Add closing tag
				stack.push(NULL); 
				closeStack.push(ast);
			}
			break;

			case HTML_ELEMENT_VIRTUAL:
				// NOTE: add children below
			break;

			default:
				printf("[HTML_not_printable_yet]");
				assert(false);
			break;
		}

		// NOTE: HTML_ELEMENT_VIRTUAL is for simply creating an element that only outputs the HTML of its children, not itself.
		// and used for the 'children' keyword.
		// -- DO NOT PUSH HTML_ELEMENT_VIRTUAL closing tags --
		if (top->canHaveChildren())
		{
			HTML_Block* ast = (HTML_Block*)top;
			if (ast->childNodes->used > 0)
			{
				// Add in reverse to print in proper order
				for (s32 i = ast->childNodes->used - 1; i >= 0; --i)
				{
					stack.push(ast->childNodes->data[i]);
				}
				if (top->type != HTML_ROOT && top->type != HTML_ELEMENT_VIRTUAL) {
					++buffer.indent;
				}
			}
			buffer.addNewline();
		}
	}
}

#endif
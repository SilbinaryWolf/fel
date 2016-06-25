#ifndef AST_PRINT_INCLUDE
#define AST_PRINT_INCLUDE

#include "ast.h"
#include "compiler.h"

struct AST_Printer {
	s32 indent;
	void printNewlineAndIndent() 
	{
		printf("\n");
		for (s32 i = 0; i < indent; ++i)
		{
			printf("     ");
		}
	}
};

inline void printStyle(CSS_Rule* rule, AST_Printer* printer)
{
	if (rule->selectorSets != NULL && rule->selectorSets->used > 0)
	{
		for (s32 i = 0; i < rule->selectorSets->used; ++i)
		{
			if (i != 0)
			{
				printf(",");
				printer->printNewlineAndIndent();
			}
			Array<CSS_Selector>* selectors = &rule->selectorSets->data[i];
			for (s32 j = 0; j < selectors->used; ++j)
			{
				if (j != 0)
				{
					printf(" ");
				}
				CSS_Selector* selector = &selectors->data[j];
				switch (selector->type)
				{
					case CSS_SELECTOR_ALL:
						print("*");
					break;

					case CSS_SELECTOR_ID:
						printf("#%*.*s", selector->token.length, selector->token.length, selector->token.data);
					break;

					case CSS_SELECTOR_CHILD:
						print("%s", &selector->token);
					break;

					case CSS_SELECTOR_CLASS:
						printf(".%*.*s", selector->token.length, selector->token.length, selector->token.data);
					break;

					case CSS_SELECTOR_TAG:
					case CSS_SELECTOR_MEDIAQUERY_TOKEN:
						printf("%*.*s", selector->token.length, selector->token.length, selector->token.data);
					break;

					case CSS_SELECTOR_PAREN_OPEN:
						printf("(");
					break;

					case CSS_SELECTOR_PAREN_CLOSE:
						printf(")");
					break;

					// ie. '[type="text"] or '[type]'
					case CSS_SELECTOR_ATTRIBUTE:
					{
						printf("[");

						{
							Token& s = selector->attribute.name;
							printf("%*.*s", s.length, s.length, s.data);
						}
						// If has operator (ie. just [type="text"], not just [type] )
						if (selector->attribute.value.length > 0)
						{
							{
								Token& s = selector->attribute.op;
								printf("%*.*s", s.length, s.length, s.data);
							}
							{
								Token& s = selector->attribute.value;
								printf("%*.*s", s.length, s.length, s.data);
							}
						}

						printf("]");
					}
					break;

					default:
						assert(false);
					break;
				}
			}
		}
	}

	printer->printNewlineAndIndent();
	printf("{");
	if (rule->properties != NULL && rule->properties->used > 0)
	{
		for (s32 i = 0; i < rule->properties->used; ++i)
		{
			printer->printNewlineAndIndent();
			CSS_Property* prop = &rule->properties->data[i];
			printf("%*.*s:", prop->name.length, prop->name.length, prop->name.data);
			for (s32 j = 0; j < prop->tokens->used; ++j)
			{
				CSS_PropertyToken* propToken = &prop->tokens->data[j];
				printf("%*.*s ", propToken->token.length, propToken->token.length, propToken->token.data);
				/*if (propToken->arguments != NULL && propToken->arguments->used != 0)
				{
					printf("( -notprintingyet-");
					printf(")");
					assert(false);
				}*/
			}
			// todo(jake): print property tokens
		}
	}
	printer->printNewlineAndIndent();
	printf("}");
	printer->printNewlineAndIndent();
}

inline void printAST(Compiler* compiler, AST* absoluteTopAST, AST_Printer* printer)
{
	TemporaryPoolScope tempPoolScope(compiler->poolTransient);
	Array<AST*> stack(2048, tempPoolScope);
	stack.push(absoluteTopAST);
	while (stack.used)
	{
		AST* top = stack.pop();
	
		if (top == 0)
		{
			--printer->indent;
			printer->printNewlineAndIndent();
			printf("}");
			// end of block
			continue;
		}

		if (top->type != AST_PARAMETERS && top->type != AST_EXPRESSION) {
			printer->printNewlineAndIndent();
		}

		switch (top->type)
		{
			case AST_UNKNOWN:
				printf("[AST_UNKNOWN]");
			break;

			case AST_LAYOUT:
			{
				printf("layout");
			} 
			break;

			case AST_STATEMENT:
			{
				AST_Statement* statement = (AST_Statement*)top;
				print("%s %s", &statement->name, &statement->op);
				stack.push(&statement->expression);
			} 
			break;

			case AST_EXPRESSION:
			{
				AST_Expression* expression = (AST_Expression*)top;
				if (expression->tokens != NULL && expression->tokens->used > 0)
				{
					for (s32 i = 0; i < expression->tokens->used; ++i)
					{
						if (i != 0)
						{
							// Seperate expression tokens by spacing
							printf(" ");
						}
						AST_Expression_Token* exprToken = &expression->tokens->data[i];

						print("%s", &exprToken->name);
					}
					printf("     (AST_EXPRESSION)");
				}
			}
			break;

			case AST_BACKEND_FUNCTION:
			{
				AST_Identifier* ast = (AST_Identifier*)top;
				print("%s", &ast->name);
				if (ast->parameters != NULL) {
					stack.push(ast->parameters);
				}
			}
			break;

			case AST_WHILE:
			case AST_LOOP: 
			case AST_IF: 
			{ 
				AST_Expression_Block* ast = (AST_Expression_Block*)top;
				if (ast->type == AST_IF)
				{
					printf("if ");
					printAST(compiler, &ast->expression, printer);
				}
				else if (ast->type == AST_LOOP)
				{
					printf("loop (need-to-print)");
				}
				else if (ast->type == AST_WHILE)
				{
					printf("while (need-to-print)");
				}
				else
				{
					printf("AST_UNKNOWN_EXPRESSION_BLOCK");
				}
			} 
			break;

			case AST_PARAMETERS:
			{
				AST_Parameters* parameters = (AST_Parameters*)top;
				if (parameters != NULL && parameters->values != NULL && parameters->values->used > 0)
				{
					bool hasNames = (parameters->names != NULL && parameters->names->used > 0);
					printf("(");
					for (s32 i = 0; i < parameters->values->used; ++i)
					{
						if (i != 0)
						{
							// Seperate parameters by commas
							printf(",");
						}
						if (hasNames)
						{
							// Prefix with 'class = ' if the parameters passed have names
							Token* name = &parameters->names->data[i];
							print("%s = ", name);
						}
						printAST(compiler, parameters->values->data[i], printer);
						// todo(Jake): Replace with printExpression
						/*Token* value = &ast->parameters->values->data[i];
						// Print value, so it could result in 'class = "hey"' or '"hey"'
						if (value->type == TOKEN_STRING)
						{
							printf("\"");
						}
						printf("%*.*s", value->length, value->length, value->data);
						if (value->type == TOKEN_STRING)
						{
							printf("\"");
						}*/
					}
					printf(")");
				}
			}
			break;

			case AST_IDENTIFIER: 
			{ 
				AST_Identifier* ast = (AST_Identifier*)top;
				print("%s", &ast->name);
				if (ast->parameters != NULL && ast->parameters->values != NULL && ast->parameters->values->used > 0)
				{
					stack.push(ast->parameters);
				}
				printf("     (AST_COMP_OR_FUNC)");
			} 
			break;

			case AST_COMPONENTDEFINITION:
			{
				AST_ComponentDefinition* ast = (AST_ComponentDefinition*)top;
				// Add in reverse to print in proper order
				printf("def %*.*s", ast->name.length, ast->name.length, ast->name.data);
				printer->printNewlineAndIndent(); printf("{");
				stack.push(NULL); ++printer->indent;
				if (ast->components != NULL)
				{
					for (s32 i = ast->components->used - 1; i >= 0; --i)
					{
						stack.push(ast->components->data[i]);
					}
				}
				if (ast->style != NULL) {
					stack.push(ast->style);
				}
				if (ast->layout != NULL) {
					stack.push(ast->layout);
				}
			}
			break;

			case AST_STYLESHEET:
			{
				printf("style");
				printer->printNewlineAndIndent(); printf("{");
				++printer->indent;
				printer->printNewlineAndIndent();
				AST_Stylesheet* ast = (AST_Stylesheet*)top;
				if (ast->rule != NULL && ast->rule->childRules != NULL)
				{
					for (s32 i = ast->rule->childRules->used - 1; i >= 0; --i)
					{
						printStyle(ast->rule->childRules->data[i], printer);
					}
				}
				--printer->indent;
				printer->printNewlineAndIndent(); printf("}");
			}
			break;

			default:
				printf("[AST_not_printable_yet]");
				assert(false);
			break;
		}


		if (top->childNodes != NULL && top->childNodes->used > 0)
		{
			printer->printNewlineAndIndent();
			printf("{");
			// Add in reverse to print in proper order
			stack.push(NULL); ++printer->indent;
			for (s32 i = top->childNodes->used - 1; i >= 0; --i)
			{
				stack.push(top->childNodes->data[i]);
			}
		}
	}
}

inline void printAST(Compiler* compiler, AST* absoluteTopAST)
{
	AST_Printer printer = {};
	printAST(compiler, absoluteTopAST, &printer);
}


/*void printASTTree(AST* absoluteTopAST) {
	TemporaryPoolScope tempPoolScope;
	Array<AST*> stack(2048, tempPoolScope);
	stack.push(absoluteTopAST);
	s32 depthLayer = 0;
	while (stack.used)
	{
		AST* top = stack.pop();
		if (top == 0)
		{
			--depthLayer;
			// end of block
			continue;
		}
		for (s32 i = 0; i < depthLayer; ++i)
		{
			printf("-");
		}
		printAST(top);
		if (top->childNodes.used > 0)
		{
			++depthLayer;
			stack.push(NULL);
			for (s32 i =  top->childNodes.used - 1; i >= 0; --i)
			{
				stack.push(top->childNodes.data[i]);
			}
		}
	};
}*/

#endif
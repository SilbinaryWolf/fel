#ifndef PARSER_INCLUDE
#define PARSER_INCLUDE

#include "print.h"
#include "lexer.h"
#include "file.h"
#include "array.h"
#include "ast.h"
#include "ast_print.h"

enum Parameter_ReadMode {
	PARAMETER_MODE_UNKNOWN = 0,
	PARAMETER_MODE_AUTO,
	PARAMETER_MODE_NAMED,
	PARAMETER_MODE_UNNAMED,
};

enum ParserMode {
	PARSER_MODE_UNKNOWN = 0,
	PARSER_MODE_IDENTIFIER_PARAMETERS, // ie. get_header("hey", "there")
	//PARSER_MODE_FUNCTION_DEFINITION, // ie. func get_header(name, me)
	PARSER_MODE_PROPERTIES_BLOCK, // ie. def Banner { properties { source = "hey" } }
	PARSER_MODE_EXPRESSION_BLOCK, // ie. if, loop
	PARSER_MODE_STATEMENT, // ie. variable = token1 + token2
};

#define parseError(token, format, ...) print("Parse error(%d,%d): "format"\n", token.lineNumber, __LINE__, ## __VA_ARGS__); assert(false)
AST_Expression* parseExpression(Tokenizer* tokenizer, ParserMode mode);

inline bool isEndOfVariableList(Token token, ParserMode mode)
{
	return ((mode == PARSER_MODE_IDENTIFIER_PARAMETERS && (token.type == TOKEN_PAREN_CLOSE))
			//|| (mode == PARSER_MODE_FUNCTION_DEFINITION && (token.type == TOKEN_PAREN_CLOSE))
			|| (mode == PARSER_MODE_PROPERTIES_BLOCK && token.type == TOKEN_BRACE_CLOSE));
}

internal AST_Parameters* parseVariableList(Tokenizer* tokenizer, ParserMode mode) {
	if (isEndOfVariableList(peekToken(tokenizer), mode))
	{
		// The function must have been defined like so: 'Banner()'
		// ie. parameter-less
		getToken(tokenizer);
		return NULL;
	}

	Parameter_ReadMode readingMode = PARAMETER_MODE_UNKNOWN;
	switch (mode)
	{
		//case PARSER_MODE_FUNCTION_DEFINITION:
		case PARSER_MODE_IDENTIFIER_PARAMETERS:
			readingMode = PARAMETER_MODE_AUTO;
		break;

		case PARSER_MODE_PROPERTIES_BLOCK:
		{
			readingMode = PARAMETER_MODE_NAMED;
			Token braceOpen = getToken(tokenizer); // read {
			if (!requireToken(braceOpen, TOKEN_BRACE_OPEN))
			{
				// NOTE(JAke): Should not happen or throw error to user
				assert(false);
			}
		}
		break;

		default:
			assert(false);
		break;
	}
	assert(readingMode != PARSER_MODE_UNKNOWN);
	// Detect parameter format
	if (readingMode == PARAMETER_MODE_AUTO)
	{
		TokenizerState prevState = tokenizer->state;
		readingMode = PARAMETER_MODE_UNNAMED;
		Token left = getToken(tokenizer);
		if (left.type != TOKEN_STRING && getToken(tokenizer).type == TOKEN_EQUAL) 
		{
			readingMode = PARAMETER_MODE_NAMED;
		}
		tokenizer->state = prevState;
	}

	#define MAX_VARLIST_COUNT 32
	Array<Token>* names = Array<Token>::create(MAX_VARLIST_COUNT, tokenizer->pool);
	Array<AST_Expression*>* values = Array<AST_Expression*>::create(MAX_VARLIST_COUNT, tokenizer->pool);
	#undef MAX_VARLIST_COUNT
	
	AST_Parameters* ast = pushStruct(AST_Parameters, tokenizer->pool);
	ast->type = AST_PARAMETERS;

	for (s32 parameterCount = 0;;++parameterCount)
	{
		Token name; // NOTE: unused for PARAMETER_MODE_UNNAMED

		switch (readingMode)
		{
			case PARAMETER_MODE_NAMED:
			{
				name = getToken(tokenizer); // move forward with getToken for 'left' if named
				Token op = getToken(tokenizer);
				if (!requireToken(op, TOKEN_EQUAL))
				{
					parseError(name, "Parameter '%s' is invalid. Expected '=', instead got '%s'", &name, &op);
					return NULL;
				}
			}
			break;

			case PARAMETER_MODE_UNNAMED:
			{
				// no-op
			}
			break;

			default:
				assert(false);
			break;
		}

		//Token debug = peekToken(tokenizer);
		//assert(debug.type == TOKEN_IDENTIFIER || debug.type == TOKEN_STRING || debug.type == TOKEN_NUMBER || debug.isOperator());
		
		AST_Expression* expression = parseExpression(tokenizer, mode);
		assert(expression != NULL);
		expression->parent = ast;

		if (values->used == values->size)
		{
			parseError(name, "Maximum allowed parameters for function or component is %d.", values->size);
			return NULL;
		}

		if (readingMode == PARAMETER_MODE_NAMED) 
		{
			names->push(name);
		}
		values->push(expression);

		// If found end of variable list
		// ie. parameters ends with ')', eg. my_div(class = "wow")
		// ie. properties block ends with '}'
		Token left = peekToken(tokenizer);
		if (mode == PARSER_MODE_IDENTIFIER_PARAMETERS && left.type == TOKEN_BRACE_OPEN)
		{
			// NOTE(Jake): Catch any expression parsing issues. This can be commented out or deleted in the future
			//			   Written: 2016-03-07
			assert(false);
		}
		if (isEndOfVariableList(left, mode))
		{
			getToken(tokenizer);
			break;
		}
	}

	assert(values != NULL);

	ast->names = names;
	ast->values = values;

	assert(ast->names != NULL);
	assert(ast->values != NULL);
	assert((readingMode == PARAMETER_MODE_NAMED && ast->names->used == ast->values->used) || readingMode == PARAMETER_MODE_UNNAMED);
	return ast;
}

inline bool isEndOfExpression(Token token, ParserMode mode) {
	if ((mode == PARSER_MODE_EXPRESSION_BLOCK && token.type == TOKEN_BRACE_OPEN)
		|| (mode == PARSER_MODE_PROPERTIES_BLOCK && token.type == TOKEN_COMMA)
		|| (mode == PARSER_MODE_IDENTIFIER_PARAMETERS && token.type == TOKEN_COMMA))
	{
		// 'if' statement block: (PARSER_MODE_EXPRESSION_BLOCK)
		// - '{' denotes the end of the statement, ie. 'if MyValue == 1 {'
		// 'properties' block: (PARSER_MODE_PROPERTIES_BLOCK)
		// -- ',' denotes end of single property (also allow trailing comma)
		// 'get_header("hey" + "hey", "there")' block: (PARSER_MODE_IDENTIFIER_PARAMETERS)
		// -- ',' denotes end of single property (also allow trailing comma)
		// 'myVar = 1 + 1 + 3' block: (PARSER_MODE_STATEMENT)
		// -- '}' denotes end
		return true;
	}
	return false;
}

internal AST_Expression* parseExpression(Tokenizer* tokenizer, ParserMode mode) {
	TemporaryPoolScope tempPool(tokenizer->poolTransient);
	Array<AST_Expression_Token> operatorTokens = Array<AST_Expression_Token>(255, tempPool);

	Array<AST_Expression_Token>* exprTokens = Array<AST_Expression_Token>::create(255, tokenizer->pool);
	
	AST_Expression* ast = pushAST(AST_Expression, AST_EXPRESSION, tokenizer->pool);
	s32 parenOpenCount = 0;
	s32 parenCloseCount = 0;

	Token tToken = peekToken(tokenizer);
	s32 currentLine = tToken.lineNumber;
	bool prevTokenWasOperator = true;
	for (;;)
	{
		TokenizerState prevTokenizerState = tokenizer->state;

		AST_Expression_Token fullToken;
		zeroMemory(&fullToken, sizeof(fullToken));
		Token& token = fullToken.name;
		token = getToken(tokenizer);

		// If parsing a statement, ie. myVar = "hello"
		// then check if the current token is an ident/string/number and is missing
		// an operator
		if (mode == PARSER_MODE_STATEMENT 
			&& !prevTokenWasOperator
			&& (token.type == TOKEN_IDENTIFIER || token.type == TOKEN_NUMBER || token.type == TOKEN_STRING))
		{
			if (token.lineNumber == currentLine)
			{
				parseError(token, "Missing operator before \"%s\"", &token);
				assert(false);
			}
			// Revert tokenizer to previous token
			tokenizer->state = prevTokenizerState;
			break;
		}
		
		prevTokenWasOperator = false;
		currentLine = token.lineNumber;

		if (token.type == TOKEN_IDENTIFIER)
		{
			// If has parameters, read them
			AST_Expression_Token op;
			zeroMemory(&op, sizeof(op));
			op.name = peekToken(tokenizer);
			if (op.name.type == TOKEN_PAREN_OPEN) 
			{
				getToken(tokenizer);
				fullToken.isFunction = true;

				AST_Parameters* parameters = parseVariableList(tokenizer, PARSER_MODE_IDENTIFIER_PARAMETERS);
				if (parameters != NULL) {
					parameters->parent = ast;
					fullToken.parameters = parameters;

					op.name = getToken(tokenizer); // skip closing paren ')'
				}
			}
			exprTokens->push(fullToken);

			/*if (isEndOfExpression(op.token, mode) || op.token.type == TOKEN_PAREN_CLOSE)
			{
				continue;
			}
			else if (!op.token.isOperator())
			{
				parseError(token, "Expected (, ), { or operator.");
				assert(false);
				break;
			}*/
		}
		else if (token.type == TOKEN_NUMBER)
		{
			exprTokens->push(fullToken);
		}
		else if (token.isOperator())
		{
			prevTokenWasOperator = true;

			while (operatorTokens.used > 0)
			{
				AST_Expression_Token topOperator = operatorTokens.top();
				if (topOperator.name.getPrecedence() >= token.getPrecedence())
				{
					exprTokens->push(operatorTokens.pop());
				}
				else
				{
					break;
				}
			}
			operatorTokens.push(fullToken);
		}
		else if (token.type == TOKEN_STRING)
		{
			exprTokens->push(fullToken);
		}
		else if (token.type == TOKEN_PAREN_OPEN)
		{
			operatorTokens.push(fullToken);
			++parenOpenCount;
		}
		else if (token.type == TOKEN_PAREN_CLOSE)
		{
			if (mode == PARSER_MODE_IDENTIFIER_PARAMETERS
				&& parenOpenCount == 0)
			{
				// Stop parsing function parameters at TOKEN_PAREN_CLOSE
				// ie. div(class = "top" + "hey")
				//
				//						        ^
				//

				// Revert tokenizer to previous token
				tokenizer->state = prevTokenizerState;
				break;
			}

			AST_Expression_Token topOperator = operatorTokens.pop();
			if (topOperator.name.type != TOKEN_PAREN_OPEN)
			{
				exprTokens->push(topOperator);
				operatorTokens.pop(); // NOTE(Jake): copy pasted from PHP, can't recall why this is done.
			}
			++parenCloseCount;
		}
		else if (isEndOfExpression(token, mode))
		{
			// If an expression block type, ie. 'if' or 'loop' blocks, then rewind to the state just
			// before parsing the '{' so it can be consumed after this function call.
			if (mode == PARSER_MODE_EXPRESSION_BLOCK) 
			{
				tokenizer->state = prevTokenizerState;
			}
			break;
		}
		else if (isEndOfVariableList(token, mode))
		{
			// Stop if it's the end of the 'properties' list or end of parameter list for function.
			// also rewind back so the parser can see the ending token.
			tokenizer->state = prevTokenizerState;
			break;
		}
		else if (mode == PARSER_MODE_STATEMENT)
		{
			assert(token.type == TOKEN_BRACE_CLOSE);

			// Stop if the other states haven't been handled and rollback
			tokenizer->state = prevTokenizerState;
			break;
		}
		else
		{
			parseError(token, "Expected identifier, string, number or ()");
			assert(false);
		}
	}

	while (operatorTokens.used > 0)
	{
		exprTokens->push(operatorTokens.pop());
	}

	assert(exprTokens != NULL || exprTokens->used != 0);

	if (parenOpenCount != parenCloseCount)
	{
		parseError(exprTokens->data[0].name, "Mismatching parenthesis on expression.");
	}
	ast->tokens = exprTokens;
	return ast;
}

// ie. If/Loop
internal AST_Expression_Block* parseExpressionBlock(Tokenizer* tokenizer) {
	Token keywordName = getToken(tokenizer); // "if"/"loop" keyword
	AST_Expression* expr = parseExpression(tokenizer, PARSER_MODE_EXPRESSION_BLOCK);
	if (expr == NULL)
	{
		return NULL;
	}
	AST_Expression_Block* ast = pushAST(AST_Expression_Block, AST_UNKNOWN, tokenizer->pool);
	expr->parent = ast;
	if (keywordName.cmp("if"))
	{
		ast->type = AST_IF;
	}
	else if (keywordName.cmp("loop"))
	{
		// 'loop' keyword brings all the looped objects variables into scope,
		// like 'with'.
		ast->type = AST_LOOP;
	}
	else if (keywordName.cmp("while"))
	{
		ast->type = AST_WHILE;
	}
	ast->expression = *expr;
	requireToken(tokenizer, TOKEN_BRACE_OPEN);
	return ast;
}

internal AST* parseIdentifier(Tokenizer* tokenizer) {
	Token name = getToken(tokenizer);
	if (name.type == TOKEN_BRACE_CLOSE)
	{
		return NULL;
	}

	Token op = peekToken(tokenizer);
	if (op.type == TOKEN_BRACE_OPEN || op.type == TOKEN_BRACE_CLOSE || op.type == TOKEN_IDENTIFIER)
	{
		AST_Identifier* ast = pushAST(AST_Identifier, AST_IDENTIFIER, tokenizer->pool);
		ast->name = name;
		return ast;
	}
	else if (op.type == TOKEN_PAREN_OPEN)
	{
		getToken(tokenizer);
		// If component or function has parameters
		AST_Identifier* ast = pushAST(AST_Identifier, AST_IDENTIFIER, tokenizer->pool);
		ast->name = name;
		ast->isFunction = true;
		ast->parameters = parseVariableList(tokenizer, PARSER_MODE_IDENTIFIER_PARAMETERS);
		if (ast->parameters != NULL) {
			ast->parameters->parent = ast;
		}

		// If current function/component is a single line, ie.
		// 'div(id = "my_div");" or "div()" then parse function/component.
		op = peekToken(tokenizer);
		if (op.type == TOKEN_SEMICOLON)
		{
			// Skip the SEMICOLON token
			getToken(tokenizer);
			return ast;
		}
		else if (op.type == TOKEN_IDENTIFIER && op.cmp("when"))
		{
			getToken(tokenizer); // Skip 'when' token
			AST_Expression* expr = parseExpression(tokenizer, PARSER_MODE_EXPRESSION_BLOCK);
			expr->parent = ast;
			if (expr == NULL)
			{
				// if no expression found, throw error?
				assert(false);
			}
			ast->expression = *expr;
		}
		else if (op.type == TOKEN_IDENTIFIER
			|| op.type == TOKEN_BRACE_OPEN 
			|| op.type == TOKEN_BRACE_CLOSE)
		{
			// no-op
		}
		else
		{
			parseError(op, "Expected ; or identifier");
			return NULL;
		}
		return ast;
	}
	else if (op.type == TOKEN_EQUAL)
	{
		getToken(tokenizer); // get EQUAL token

		AST_Expression* expr = parseExpression(tokenizer, PARSER_MODE_STATEMENT);
		assert(expr != NULL);

		AST_Statement* ast = pushAST(AST_Statement, AST_STATEMENT, tokenizer->pool);
		ast->name = name;
		ast->op = op;
		ast->expression = *expr;
		ast->expression.parent = ast;
		return ast;
	}
	assert(false);
	return NULL;
}

internal AST_Layout* parseLayout(Tokenizer* tokenizer) {
	if (!requireToken(tokenizer, TOKEN_BRACE_OPEN))
	{
		return NULL;
	}

	TemporaryPoolScope tempPool(tokenizer->poolTransient);
	Array<AST*> nestDepth(512, tempPool);
	Array<Token> variables(255, tempPool); // Keep track of variable names/count in scope

	AST_Layout* ast = pushAST(AST_Layout, AST_LAYOUT, tokenizer->pool);
	nestDepth.push(ast);

	{
		for(;;)
		{
			AST* top = nestDepth.top();

			Token name = peekToken(tokenizer);
			if (name.type == TOKEN_BRACE_CLOSE)
			{
				getToken(tokenizer);
				// Change scope to go up one level
				nestDepth.pop();
				// If breaking out of 'layout' block
				if (nestDepth.used <= 0) {
					assert(nestDepth.used == 0); // NOTE(Jake): There should never be a case where used is lower than 0.
					break;
				}
			}
			else if (name.type == TOKEN_IDENTIFIER)
			{
				if (name.cmp("if") || name.cmp("loop") || name.cmp("while"))
				{
					AST_Expression_Block* ast = parseExpressionBlock(tokenizer);
					ast->parent = top;
					top->childNodes->push(ast);
					nestDepth.push(ast);
				}
				else if (name.cmp("else"))
				{
					// todo(Jake): probably need to store the last-if found so that the else branch can be attached to it. (ie. ->ChildNodesElse)
					assert(false);
				}
				else
				{
					AST* astTop = parseIdentifier(tokenizer);
					assert(astTop != NULL);
					astTop->parent = top;
					top->childNodes->push(astTop);

					if (astTop->type == AST_IDENTIFIER)
					{
						AST_Identifier* ast = (AST_Identifier*)astTop;
						Token nextToken = peekToken(tokenizer);
						if (nextToken.type == TOKEN_BRACE_OPEN)
						{
							// If found brace after component, add to current scope
							getToken(tokenizer);
							nestDepth.push(ast);
						}
						else if (nextToken.type == TOKEN_BRACE_CLOSE || nextToken.type == TOKEN_IDENTIFIER)
						{
							// allow other for functions on their own
						}
						else
						{
							// NOTE(Jake): Fix this to be more robust
							assert(false);
						}
					}
					else if (astTop->type == AST_STATEMENT)
					{
						AST_Statement* ast = (AST_Statement*)astTop;
						if (variables.find(ast->name) == -1)
						{
							variables.push(ast->name);
						}
					}
					else
					{
						assert(false);
					}
				}
			}
			else
			{
				parseError(name, "Expected identifier, instead got '%s'", &name);
				break;
			}
		}
	}

	if (nestDepth.used > 1)
	{
		//parseError("Stuck in %d nested scope.", nestDepth.used);
		assert(false);
	}
	ast->variables = variables.createCopyShrinkToFit(tokenizer->pool);
	return ast;
}

internal CSS_Rule* parseStyle(Tokenizer* tokenizer) {
	if (!requireToken(tokenizer, TOKEN_BRACE_OPEN))
	{
		return NULL;
	}

	CSS_Rule* resultRule = pushStruct(CSS_Rule, tokenizer->pool);
	Array<CSS_Rule*>* rules = Array<CSS_Rule*>::create(1024, tokenizer->pool);
	Array<CSS_Property>* properties = Array<CSS_Property>::create(128, tokenizer->pool);
	Array<Array<CSS_Selector>>* selectorSets = NULL;

	for (;;)
	{
		TokenizerState prevState = tokenizer->state;
		Token first = getTokenCSSProperty(tokenizer);
		Token next = getTokenCSSProperty(tokenizer);
		tokenizer->state = prevState;

		if (first.type == TOKEN_BRACE_CLOSE)
		{
			getTokenCSSProperty(tokenizer); // consume TOKEN_BRACE_CLOSE
			break;
		}
		else if (first.type == TOKEN_IDENTIFIER && first.length >= 1 && first.data[0] == '@')
		{
			// Skip @
			++first.data; --first.length;
			getTokenCSSProperty(tokenizer); // skip @{keyword] token
			// Check @ keywords
			if (first.cmp("media"))
			{
				selectorSets = Array<Array<CSS_Selector>>::create(64, tokenizer->pool);
				Array<CSS_Selector>* selectors = Array<CSS_Selector>::create(128, tokenizer->pool);
				for (;;)
				{
					TokenizerState prevState = tokenizer->state;
					Token token = getTokenCSSProperty(tokenizer);
					CSS_Selector selector = {};
					selector.type = CSS_SELECTOR_MEDIAQUERY_TOKEN;
					selector.token = token;

					if (token.type == TOKEN_BRACE_OPEN)
					{
						tokenizer->state = prevState;
						break;
					}
					else if (token.type == TOKEN_COMMA)
					{
						// Push current array of selectors to selector set
						// and then allocate a new one.
						// NOTE(Jake): Ignore empty selectors, this will allow a trailing comma nicely
						if (selectors != NULL && selectors->used > 0) {
							selectorSets->push(*selectors);
						}
						selectors = Array<CSS_Selector>::create(128, tokenizer->pool);
						continue;
					}
					else if (token.type == TOKEN_PAREN_OPEN)
					{
						selector.type = CSS_SELECTOR_PAREN_OPEN;
					}
					else if (token.type == TOKEN_PAREN_CLOSE)
					{
						selector.type = CSS_SELECTOR_PAREN_CLOSE;
					}
					selectors->push(selector);
				}

				// Push final remaining selectorSet
				if (selectors->used > 0)
				{
					selectorSets->push(*selectors);
				}

				CSS_Rule* subRule = parseStyle(tokenizer);
				assert(subRule != NULL);
				if (subRule != NULL)
				{
					subRule->type = CSS_RULE_MEDIAQUERY;
					subRule->selectorSets = selectorSets;
					subRule->parent = resultRule;
					rules->push(subRule);
				}
			}
			else
			{
				assert(false);
			}
		}
		else if (first.type == TOKEN_IDENTIFIER && next.type == TOKEN_COLON)
		{
			// Detect property, read it.
			Token token = getTokenCSSProperty(tokenizer);
			getTokenCSSProperty(tokenizer); // TOKEN_COLON

			// Read property tokens until semicolon
			// NOTE(Jake): idea, expressions can only be inside 'calc()'
			CSS_Property prop = {};
			prop.name = token;
			prop.tokens = Array<CSS_PropertyToken>::create(64, tokenizer->pool);
			for (;;)
			{
				CSS_PropertyToken propToken;
				zeroMemory(&propToken, sizeof(propToken));
				Token token = propToken.token = getTokenCSSProperty(tokenizer, GET_TOKEN_ACCEPT_NEWLINE);

				if (token.type == TOKEN_IDENTIFIER)
				{
					Token paren = peekTokenCSSProperty(tokenizer);
					if (paren.type == TOKEN_PAREN_OPEN)
					{
						getToken(tokenizer); // skip TOKEN_PAREN_OPEN
						propToken.arguments = Array<Token>::create(64, tokenizer->pool);
						s32 commaCount = 0;
						for (;;)
						{
							Token token = getTokenCSSProperty(tokenizer);
							if (token.type == TOKEN_NUMBER)
							{
								propToken.arguments->push(token);
							}
							else if (token.type == TOKEN_COMMA)
							{
								// no-op
								++commaCount;
								if (commaCount != propToken.arguments->used)
								{
									parseError(token, "Missing parameter %d.", propToken.arguments->used + 1);
								}
							}
							else if (token.type == TOKEN_PAREN_CLOSE)
							{
								// end parameter list
								break;
							}
							else
							{
								assert(false);
							}
						}
					}
					prop.tokens->push(propToken);
				}
				else if (token.type == TOKEN_NUMBER
						|| token.type == TOKEN_HEX)
				{
					// NOTE(Jake): perhaps store more info at this level, ie. TOKEN_NUMBER_PX, TOKEN_NUMBER_EM, TOKEN_NUMBER_PERCENT ?
					prop.tokens->push(propToken);
				}
				else if (token.type == TOKEN_COMMA)
				{
					prop.tokens->push(propToken);
				}
				else if (token.type == TOKEN_SEMICOLON
						|| token.type == TOKEN_NEWLINE)
				{
					break;
				}
				else if (token.type == TOKEN_BRACE_CLOSE)
				{
					assert(false);
				}
				else
				{
					assert(false);
				}
			}
			properties->push(prop);
		}
		else
		{
			// Read selectors that make up the CSS rule (ie. '.myDiv > .heyThere + .wow')
			selectorSets = Array<Array<CSS_Selector>>::create(64, tokenizer->pool);
			Array<CSS_Selector>* selectors = Array<CSS_Selector>::create(128, tokenizer->pool);
			for (;;)
			{
				TokenizerState prevState = tokenizer->state;
				Token token = getTokenCSSProperty(tokenizer);
				CSS_Selector selector = {};
				selector.token = token;
				if (token.type == TOKEN_IDENTIFIER)
				{
					if (token.data[0] == '.')
					{
						// read class (.myElement, .container)
						selector.type = CSS_SELECTOR_CLASS;
					}
					else if (token.data[0] == '#')
					{
						// read class (.myElement, .container)
						selector.type = CSS_SELECTOR_ID;
					}
					else
					{
						// read element tag (html, body, div)
						selector.type = CSS_SELECTOR_TAG;
					}
				}
				else if (token.type == TOKEN_MULTIPLY)
				{
					selector.type = CSS_SELECTOR_ALL;
				}
				else if (token.type == TOKEN_COLON)
				{
					// read state next to selector (ie. use case: "audio:not([controls])")
					selector.type = CSS_SELECTOR_MODIFIER;
				}
				else if (token.type == TOKEN_COND_ABOVE)
				{
					// read > before next selector (ie. use case: ".banner > .banner-title")
					selector.type = CSS_SELECTOR_CHILD;
				}
				else if (token.type == TOKEN_PAREN_OPEN)
				{
					// read parenthesis (ie. use case: "audio:not([controls])")
					// NOTE(Jake): :not, accepts a selector inside the ":not", so ":not(.myDiv)" is accepted.
					selector.type = CSS_SELECTOR_PAREN_OPEN;
				}
				else if (token.type == TOKEN_PAREN_CLOSE)
				{
					// read parenthesis (ie. use case: "audio:not([controls])")
					// NOTE(Jake): :not, accepts a selector inside the ":not", so ":not(.myDiv)" is accepted.
					selector.type = CSS_SELECTOR_PAREN_CLOSE;
				}
				else if (token.type == TOKEN_BRACKET_OPEN)
				{
					// read brackets for attribute selector (ie. use case: "input[type="text"] or input[type]")
					Token name = getTokenCSSProperty(tokenizer);
					Token op = getTokenCSSProperty(tokenizer);
					Token value;
					zeroMemory(&value, sizeof(value));
					if (op.type != TOKEN_BRACKET_CLOSE)
					{
						if (!requireToken(op, TOKEN_EQUAL))
						{
							assert(false);
							return NULL;
						}
						value = getTokenCSSProperty(tokenizer);
						if (value.type != TOKEN_STRING)
						{
							assert(false);
						}
						Token endBracket = getTokenCSSProperty(tokenizer);
						if (!requireToken(endBracket, TOKEN_BRACKET_CLOSE))
						{
							assert(false);
							return NULL;
						}
					}
					selector.type = CSS_SELECTOR_ATTRIBUTE;
					selector.attribute.name = name;
					selector.attribute.op = op;
					selector.attribute.value = value;
				}
				else if (token.type == TOKEN_COMMA)
				{
					// Push current array of selectors to selector set
					// and then allocate a new one.
					// NOTE(Jake): Ignore empty selectors, this will allow a trailing comma nicely
					if (selectors != NULL && selectors->used > 0) {
						selectorSets->push(*selectors);
					}
					selectors = Array<CSS_Selector>::create(128, tokenizer->pool);
					continue;
				}
				else if (token.type == TOKEN_BRACE_OPEN)
				{
					// end of selector
					tokenizer->state = prevState; // undo consuming TOKEN_BRACE_OPEN
					break;
				}
				else
				{
					assert(false);
				}
				assert(selector.type != CSS_SELECTOR_UNKNOWN);
				selectors->push(selector);
			}

			// Push current array of selectors to selector set
			if (selectors != NULL && selectors->used > 0) {
				selectorSets->push(*selectors);
			}

			CSS_Rule* subRule = parseStyle(tokenizer);
			assert(subRule != NULL);
			if (subRule != NULL)
			{
				subRule->type = CSS_RULE_SELECTOR;
				subRule->selectorSets = selectorSets;
				subRule->parent = resultRule;
				rules->push(subRule);
			}
		}
	}

	assert(rules != NULL && properties != NULL)
	
	resultRule->childRules = rules;
	resultRule->properties = properties;
	return resultRule;
}

internal AST_FunctionDefinition* parseFunctionDefinition(Tokenizer* tokenizer) {
	Token funcName = getToken(tokenizer);
	if (!requireToken(funcName, TOKEN_IDENTIFIER))
	{
		return NULL;
	}
	Token openParen = getToken(tokenizer);
	if (!requireToken(openParen, TOKEN_PAREN_OPEN))
	{
		return NULL;
	}
	AST_Parameters* parameters = parseVariableList(tokenizer, PARSER_MODE_IDENTIFIER_PARAMETERS);

	AST_FunctionDefinition* ast = pushAST(AST_FunctionDefinition, AST_FUNCTIONDEFINITION, tokenizer->pool);
	ast->parameters = parameters;
	ast->parameters->parent = ast;
	assert(false);
	return ast;
}

internal AST_ComponentDefinition* parseComponentDefinition(Tokenizer* tokenizer) {
	Token componentName = getToken(tokenizer);
	if (!requireToken(componentName, TOKEN_IDENTIFIER))
	{
		return NULL;
	}
	if (!requireToken(tokenizer, TOKEN_BRACE_OPEN))
	{
		assert(false);
		return NULL;
	}

	// Blocks
	AST_ComponentDefinition* definition = pushAST(AST_ComponentDefinition, AST_COMPONENTDEFINITION, tokenizer->pool);
	definition->name = componentName;
	definition->components = Array<AST_ComponentDefinition*>::create(1024, tokenizer->pool);
	s32 layoutBlockCount = 0;
	s32 styleBlockCount = 0;
	s32 propertiesBlockCount = 0;

	for (;;)
	{
		Token token = getToken(tokenizer);
		if (token.type == TOKEN_IDENTIFIER)
		{
			if (token.cmp("properties"))
			{
				if (styleBlockCount >= 1)
				{
					// note: dont allow mulitple 'properties' blocks
					assert(false);
					break;
				}
				AST_Parameters* properties = parseVariableList(tokenizer, PARSER_MODE_PROPERTIES_BLOCK);
				if (properties != NULL)
				{
					definition->properties = properties;
				}
				++propertiesBlockCount;
			}
			else if (token.cmp("style"))
			{
				if (styleBlockCount >= 1)
				{
					// note: dont allow mulitple 'style' blocks
					assert(false);
					break;
				}

				CSS_Rule* rule = parseStyle(tokenizer);
				if (rule != NULL)
				{
					AST_Stylesheet* stylesheet = pushAST(AST_Stylesheet, AST_STYLESHEET, tokenizer->pool);
					stylesheet->rule = rule;
					definition->style = stylesheet;
				}
				++styleBlockCount;
			}
			else if (token.cmp("script"))
			{
				assert(false);
			}
			else if (token.cmp("layout"))
			{
				if (layoutBlockCount >= 1)
				{
					// note: dont allow mulitple 'layout' blocks
					assert(false);
					break;
				}
				definition->layout = parseLayout(tokenizer);
				definition->layout->parent = definition;
				++layoutBlockCount;
			}
			else if (token.cmp("def"))
			{
				AST_ComponentDefinition* ast = parseComponentDefinition(tokenizer);
				if (ast != NULL) {
					definition->components->push(ast);
				}
			}
			else
			{
				parseError(token, "Invalid component block name. Expected 'layout', 'script', 'style' or 'properties'.");
			}
		}
		else if (token.type == TOKEN_BRACE_CLOSE)
		{
			// end of component defintion
			break;
		}
	}

	return definition;
}

void parse(Compiler* compiler, String pathname) {
	AST_File ast_file;
	zeroMemory(&ast_file, sizeof(ast_file));

	// Read filenames
	String basename = pathname.basename();
	File::Error fileError;
	String fileContents = File::readEntireFile(pathname, &fileError);
	if (fileContents.data == NULL) 
	{
		if (fileError.errorCode)
		{
			switch (fileError.errorCode)
			{
				case File::FILE_CANT_OPEN:
					printf("Skipping '%s', invalid file / cannot open. code = %d \n", basename.data, fileError.errorCode);
				break;

				case File::FILE_NO_MEMORY:
					printf("Skipping '%s', out of memory. code = %d \n", basename.data, fileError.errorCode);
				break;

				default:
					printf("Skipping '%s', unknown error. code = %d \n", basename.data, fileError.errorCode);
				break;
			}
		} 
		else 
		{
			printf("Skipping '%s', file is empty.\n", basename.data);
		}
		return;
	}

	printf("Lexing '%s'...\n", basename.data);

	// Setup tokenizer
	u32 tokenCount = 0;
	Tokenizer tokenizer;
	zeroMemory(&tokenizer, sizeof(Tokenizer));
	tokenizer.pool = compiler->pool;
	tokenizer.poolTransient = compiler->poolTransient;
	tokenizer.string = fileContents;
	tokenizer.pathName = pathname;
	tokenizer.state.at = tokenizer.string.data;
	tokenizer.state.lineNumber = 0;

	ast_file.pathname = pathname;
	ast_file.layouts = Array<AST_Layout>::create(1024, tokenizer.pool);
	ast_file.components = Array<AST_ComponentDefinition>::create(1024, tokenizer.pool);

	for(;;)
	{
		Token token = getToken(&tokenizer);
		if (token.type == TOKEN_EOF || token.type == TOKEN_UNKNOWN) 
		{
			break;
		}
		else if (token.type == TOKEN_IDENTIFIER)
		{
			if (token.cmp("layout"))
			{
				AST_Layout* layout = parseLayout(&tokenizer);
				if (layout != NULL)
				{
					ast_file.layouts->push(*layout);
				}
			}
			else if (token.cmp("def"))
			{
				AST_ComponentDefinition* componentDefinition = parseComponentDefinition(&tokenizer);
				if (componentDefinition != NULL)
				{
					ast_file.components->push(*componentDefinition);
				}
			}
			else if (token.cmp("func"))
			{
				AST_FunctionDefinition* functionDefinition = parseFunctionDefinition(&tokenizer);
				assert(false);
			}
			else
			{
				assert(false);
			}
		}
		else
		{
			assert(false);
		}
		//tokenArray.push(token);
	}

	// Debug Print info
	printf("\n");
	printf("Layouts found: %d\n", ast_file.layouts->used);
	for (s32 i = 0; i < ast_file.layouts->used; ++i)
	{
		AST_Layout* it = &ast_file.layouts->data[i];
		printAST(compiler, it);
	}
	printf("\n");
	printf("Components found: %d\n", ast_file.components->used);
	for (s32 i = 0; i < ast_file.components->used; ++i)
	{
		AST_ComponentDefinition* it = &ast_file.components->data[i];
		printAST(compiler, it);
	}
	printf("\n");

	//if (hasErrors)
	{
		//printf("'%s' contained errors. Stopping.\n", basename.data);
	}

	compiler->astFiles->push(ast_file);
}

#endif
#include "code_generator.h"
#include "html.h"
#include "html_print.h"
#include "css_print.h"
#include "file.h"

HTML_Element* compileLayout(Compiler* compiler, AST_Layout* layout, CompilerParameters* properties = NULL);
CompilerValue evaluateExpression(Compiler* compiler, AST_Expression* expression);

inline AST_ComponentDefinition* findComponentDefinition(Compiler* compiler, String name)
{
	TemporaryPoolScope tempPool(compiler->poolTransient);
	Array<AST_ComponentDefinition*> definitions(256, tempPool);

	for (s32 i = 0; i < compiler->astFiles->used; ++i)
	{
		AST_File* ast_file = &compiler->astFiles->data[i];
		if (ast_file->components != NULL)
		{
			for (s32 j = 0; j < ast_file->components->used; ++j)
			{
				AST_ComponentDefinition* definition = &ast_file->components->data[j];
				if (name.cmp(definition->name))
				{
					definitions.push(definition);
				}
			}
		}
	}

	if (definitions.used > 0)
	{
		if (definitions.used == 1)
		{
			AST_ComponentDefinition* definition = definitions.data[0];
			return definition;
		}
		else
		{
			compiler->hasError = true;
			// Detect multiple definitions error
			compileError("Multiple definitions of '%s' found.", &name);
			for (s32 i = 0; i < definitions.used; ++i)
			{
				AST_ComponentDefinition* definition = definitions.data[i];
				String basename = definition->name.pathName.basename();
				compileErrorSub("Definition #%d found on Line %d on file '%s'.", i, definition->name.lineNumber, &basename);
				compileErrorSubSub("(%s)", &definition->name.pathName);
			}
		}
	}

	return NULL;
}

CompilerValue evaluateIdentifier(Compiler* compiler, Token identifierName, AST_Expression* context)
{
	assert(context != NULL);
	assert(context->parent != NULL);

	if (identifierName.isBackend())
	{
		// Handled in other codepath in evaluateExpression(),
		// if its needed here again, move the code back here.
		assert(false);
	}

	if (compiler->stack->used > 0)
	{
		CompilerParameters* stackParameters = compiler->stack->top();
		if (stackParameters != NULL)
		{
			assert(stackParameters->names != NULL && stackParameters->names->used > 0);
			assert(stackParameters->names->used == stackParameters->values->used);
			
			s32 findIndex = stackParameters->names->find(identifierName);
			if (findIndex != -1)
			{
				CompilerValue result = stackParameters->values->data[findIndex];
				return result;
			}
			/*for (s32 i = 0; i < stackParameters->names->used; ++i)
			{
				Token name = stackParameters->names->data[i];
				if (name.cmp(identifierName))
				{
					CompilerValue result = stackParameters->values->data[i];
					return result;
				}
			}*/
		}
	}

	AST* current = context;
	while (current = current->parent)
	{
		switch (current->type)
		{
			case AST_COMPONENTDEFINITION:
			{
				// NOTE(Jake): Component 'properties' are now added to compiler->stack, this isnt necessary for the
				//			   time being. (2016-03-21)
				//
				/*AST_ComponentDefinition* definition = (AST_ComponentDefinition*)current;
				assert(definition != NULL);
				if (definition->properties != NULL 
					&& definition->properties->names != NULL
					&& definition->properties->values != NULL)
				{
					Array<Token>* names = definition->properties->names;
					for (s32 i = 0; i < names->used; ++i)
					{
						Token name = names->data[i];
						if (name.cmp(identifierName))
						{
							AST_Expression* expr = definition->properties->values->data[i];
							assert(expr != NULL);
							CompilerValue result = evaluateExpression(compiler, expr);
							return result;
						}
					}
				}*/
			}
			break;

			case AST_LAYOUT:
				// todo(Jake): Get variables from this scope 
			break;

			case AST_PARAMETERS:
			case AST_IF:
			case AST_TAG:	
				// no-op
			break;

			default:
			{
				assert(false);
			}
			break;
		}
	}

	// DEBUG TOOL: Immediately know where to code identifier issues
	//compileError("Unable to find variable \"%s\"", &identifierName); assert(false);

	CompilerValue nullResult;
	zeroMemory(&nullResult, sizeof(nullResult));

	compiler->hasError = true;
	compileError("Unable to find identifier '%s'.", &identifierName);
	return nullResult;
}

CompilerValue compute(Compiler* compiler, CompilerValue lval, CompilerValue rval, AST_Expression_Token op)
{
	CompilerValue nullResult;
	zeroMemory(&nullResult, sizeof(nullResult));

	if ((lval.type == COMPILER_VALUE_TYPE_STRING && rval.type == COMPILER_VALUE_TYPE_STRING))
	{
		assert(false);
		return nullResult;
	}

	CompilerValue result = {};
	switch (lval.type)
	{
		case COMPILER_VALUE_TYPE_DOUBLE:
		{
			result.type = COMPILER_VALUE_TYPE_DOUBLE;
			f64& resultVal = result.valueDouble;

			if (rval.type == lval.type)
			{
				f64 lreal = lval.valueDouble;
				f64 rreal = rval.valueDouble;
				switch (op.name.type)
				{
					case TOKEN_PLUS: {resultVal = lreal + rreal; } break;
					case TOKEN_MULTIPLY:  {resultVal = lreal * rreal; } break;
					case TOKEN_COND_AND: {resultVal = lreal && rreal; } break;
					case TOKEN_COND_OR: { resultVal = lreal || rreal; } break;
					default:
					{
						assert(false);
					}
					break;
				}
			}
			else
			{
				s8 numberEvaluatesTrue = -1;
				s8 stringEvaluatesTrue = -1;

				switch (lval.type)
				{
					case COMPILER_VALUE_TYPE_DOUBLE: { numberEvaluatesTrue = (lval.valueDouble != 0); } break;
					case COMPILER_VALUE_TYPE_STRING: { stringEvaluatesTrue = (lval.valueString.length != 0); } break;
					default:
					{
						assert(false);
					}
					break;
				}
				switch (rval.type)
				{
					case COMPILER_VALUE_TYPE_DOUBLE: { numberEvaluatesTrue = (rval.valueDouble != 0); } break;
					case COMPILER_VALUE_TYPE_STRING: { stringEvaluatesTrue = (rval.valueString.length != 0); } break;
					default:
					{
						assert(false);
					}
					break;
				}
				assert(numberEvaluatesTrue != -1 && stringEvaluatesTrue != -1);

				switch (op.name.type)
				{
					// Comparing different types conditionally like so:
					// - if ("myString" && 120) -- returns true
					// - if ("" && 120) -- returns false, as an empty string returns false.
					case TOKEN_COND_AND: { resultVal = numberEvaluatesTrue && stringEvaluatesTrue; } break;
					case TOKEN_COND_OR: { resultVal = numberEvaluatesTrue || stringEvaluatesTrue; } break;

					// Error cases
					// - You cannot add/subtract/mult/divide/etc a string and a number
					case TOKEN_PLUS:
					{
						compileError("Cannot add a string and number together on Line %d", op.name.lineNumber);
						assert(false);
					}
					break;

					default:
					{
						compileError("Attempted to modify two different data types together on Line %d", op.name.lineNumber);
						assert(false);
					}
					break;
				}
			}
		}
		break;

		default:
		{
			assert(false);
		}
		break;
	}
	assert(result.type != COMPILER_VALUE_TYPE_UNKNOWN);
	return result;
}

CompilerValue evaluateExpression(Compiler* compiler, AST_Expression* expression)
{
	assert(expression != NULL);
	assert(expression->tokens != NULL);
	assert(expression->tokens->used > 0);

	TemporaryPoolScope tempPool(compiler->poolTransient);

	//
	CompilerValue nullResult;
	zeroMemory(&nullResult, sizeof(nullResult));

	// Used for storing values until an operator is found, then right and left values are popped and
	// operated on with +, /, *, -.
	Array<CompilerValue> stack(expression->tokens->used, tempPool);

	// Get strings count used in expression
	s32 stringEstimateCount = 0;
	bool allocatedStringBuilder = false;
	for (s32 i = 0; i < expression->tokens->used; ++i)
	{
		AST_Expression_Token it = expression->tokens->data[i];
		stringEstimateCount += (int)(it.name.type == TOKEN_STRING || it.name.type == TOKEN_IDENTIFIER);
	}

	// Stores raw values as their processed in the expression
	Array<CompilerValue> tempBackendValues(255, tempPool);

	s32 tokenIndex;
	for (tokenIndex = 0; tokenIndex < expression->tokens->used; ++tokenIndex)
	{
		AST_Expression_Token it = expression->tokens->data[tokenIndex];
		Token token = it.name;
		if (token.isOperator())
		{
			if (stack.used == 0) {
				compileError("Operator \"%s\" found at end of statement, no number or identifier after it at Line %d.", &token, token.lineNumber);
				return nullResult;
			}
			CompilerValue rval = stack.pop();
			if (stack.used == 0) {
				if (tempBackendValues.used == 0)
				{
					assert(false);
				}
				CompilerValue op;
				zeroMemory(&op, sizeof(op));
				op.type = COMPILER_VALUE_TYPE_BACKEND_OPERATOR;
				op.valueToken.name = token;

				tempBackendValues.push(rval);
				tempBackendValues.push(op);
				continue;
			}
			CompilerValue lval = stack.pop();

			if (lval.type == COMPILER_VALUE_TYPE_BACKEND_IDENTIFIER || rval.type == COMPILER_VALUE_TYPE_BACKEND_IDENTIFIER 
				|| lval.type == COMPILER_VALUE_TYPE_BACKEND_EXPRESSION || rval.type == COMPILER_VALUE_TYPE_BACKEND_EXPRESSION)
			{
				CompilerValue op;
				zeroMemory(&op, sizeof(op));
				op.type = COMPILER_VALUE_TYPE_BACKEND_OPERATOR;
				op.valueToken.name = token;
				tempBackendValues.push(lval);
				tempBackendValues.push(rval);
				tempBackendValues.push(op);
			}
			else if (lval.type == COMPILER_VALUE_TYPE_STRING || lval.type == COMPILER_VALUE_TYPE_STRING_BUILDER)
			{
				assert(lval.valueStringBuilder != NULL);

				if (rval.type != COMPILER_VALUE_TYPE_STRING && rval.type != COMPILER_VALUE_TYPE_STRING_BUILDER)
				{
					compileError("Cannot compare string to numeric on Line %d", token.lineNumber);
					assert(false);
				}

				// Convert string to string builder
				if (lval.type == COMPILER_VALUE_TYPE_STRING 
					&& it.name.type == TOKEN_PLUS)
				{
					String str = lval.valueString;

					lval.type = COMPILER_VALUE_TYPE_STRING_BUILDER;
					lval.valueStringBuilder = StringBuilder::create(stringEstimateCount, tempPool);
					lval.valueStringBuilder->add(str);
				}

				CompilerValue result = lval;

				String rstr;
				if (rval.type == COMPILER_VALUE_TYPE_STRING) {
					rstr = rval.valueString;
				} else if (rval.type == COMPILER_VALUE_TYPE_STRING_BUILDER) {
					rstr = rval.valueStringBuilder->toString(tempPool);
				} else {
					assert(false);
				}
				switch (it.name.type)
				{
					case TOKEN_PLUS:
					{
						assert(result.type == COMPILER_VALUE_TYPE_STRING_BUILDER);
						result.valueStringBuilder->add(rstr);
					}
					break;

					case TOKEN_COND_EQUAL:
					{
						s8 isTrue = -1;
						if (result.type == COMPILER_VALUE_TYPE_STRING_BUILDER)
						{
							if (result.valueStringBuilder->totalLength != rstr.length)
							{
								isTrue = false;
							}
							else
							{
								TemporaryPoolScope tempStrPool(compiler->pool);
								String lcurrVal = result.valueStringBuilder->toString(tempStrPool);
								isTrue = lcurrVal.cmp(rstr);
							}
						}
						else if (result.type == COMPILER_VALUE_TYPE_STRING)
						{
							isTrue = result.valueString.cmp(rstr);
						}
						else
						{
							assert(false);
						}
						assert(isTrue != -1);

						result.type = COMPILER_VALUE_TYPE_DOUBLE;
						result.valueDouble = isTrue;
					}
					break;

					default:
					{
						assert(false);
					}
					break;
				}
				stack.push(result);
			}
			else
			{
				assert(rval.type != COMPILER_VALUE_TYPE_STRING_BUILDER);
				CompilerValue result = compute(compiler, lval, rval, it);
				stack.push(result);
			}
		}
		else
		{
			// Determine the identifier
			if (token.type == TOKEN_IDENTIFIER)
			{
				if (token.isBackend())
				{
					CompilerValue backendResult;
					zeroMemory(&backendResult, sizeof(backendResult));
					backendResult.type = COMPILER_VALUE_TYPE_BACKEND_IDENTIFIER;
					backendResult.valueToken = it;
					backendResult.valueToken.name.data += 1;
					backendResult.valueToken.name.length -= 1;
					stack.push(backendResult);
				}
				else if (it.isFunction == false)
				{
					CompilerValue newValue = evaluateIdentifier(compiler, token, expression);
					if (compiler->hasError)
					{
						assert(false);
					}
					stack.push(newValue);
				}
				else
				{
					assert(false);
				}
			}
			else if (token.type == TOKEN_NUMBER)
			{
				CompilerValue newValue = {};
				newValue.type = COMPILER_VALUE_TYPE_DOUBLE;
				newValue.valueDouble = token.toDouble();
				stack.push(newValue);
			}
			else if (token.type == TOKEN_STRING)
			{
				CompilerValue newValue = {};
				newValue.type = COMPILER_VALUE_TYPE_STRING;
				newValue.valueString = token;
				stack.push(newValue);
			}
			else
			{
				assert(false);
			}
		}
	}

	// If one identifier and its backend based, coerce into to backend expression.
	if (stack.used == 1 && stack.top().type == COMPILER_VALUE_TYPE_BACKEND_IDENTIFIER)
	{
		tempBackendValues.push(stack.pop());
	}

	// If using any backend identifiers, return it as a backend expression.
	if (tempBackendValues.used > 0)
	{
		if (stack.used > 0)
		{
			compileError("Invalid items in backend expression");
			for (s32 i = 0; i < stack.used; ++i) {
				//compileErrorSub(stack.data[i]);
			}
			return nullResult;
		}

		CompilerValue result;
		zeroMemory(&result, sizeof(result));
		result.type = COMPILER_VALUE_TYPE_BACKEND_EXPRESSION;
		result.valueExpression = tempBackendValues.createCopyShrinkToFit(compiler->pool);
		return result;
	}
	
	// Expression should only have one value left, the result.
	// NOTE(Jake): This is subject to change with 'backend expressions'
	assert(stack.used == 1);

	CompilerValue result = stack.pop();
	if (result.type == COMPILER_VALUE_TYPE_STRING_BUILDER)
	{
		assert(result.valueStringBuilder->used == result.valueStringBuilder->size);

		String str = result.valueStringBuilder->toString(compiler->pool);
		result.type = COMPILER_VALUE_TYPE_STRING;
		result.valueString = str;
	}

	assert(result.type != COMPILER_VALUE_TYPE_STRING_BUILDER);
	return result;
}

CompilerParameters* evaluateParameters(Compiler* compiler, AST_Parameters* parameters) {
	assert(parameters != NULL);
	CompilerParameters* result = pushStruct(CompilerParameters, compiler->pool);
	result->names = parameters->names;
	result->values = Array<CompilerValue>::create(parameters->values->used, compiler->pool);
	for (s32 i = 0; i < parameters->values->used; ++i)
	{
		AST_Expression* it = parameters->values->data[i];
		assert(it->tokens != NULL);
	
		CompilerValue value = evaluateExpression(compiler, it);
		result->values->push(value);
	}
	return result;
}

void evaluateParametersAndModifyExistingParameters(Compiler* compiler, AST_Parameters* parameters, CompilerParameters* existingParameters) {
	assert(existingParameters != NULL);
	if (parameters == NULL)
	{
		return;
	}
	for (s32 i = 0; i < parameters->names->used; ++i)
	{
		Token& it = parameters->names->data[i];
		s32 findIndex = existingParameters->names->find(it);
		if (findIndex != -1)
		{
			// If found property
			AST_Expression* expression = parameters->values->data[i];
			existingParameters->values->data[findIndex] = evaluateExpression(compiler, expression);
		}
		else
		{
			// If haven't found property
			AST* parentLoose = parameters->parent;
			compiler->hasError = true;
			switch (parentLoose->type)
			{
				case AST_COMPONENT:
				{
					AST_Identifier* parent = (AST_Identifier*)parentLoose;
					compileError("Unable to override property '%s' in component '%s' on Line %d as it does not exist.", &it, &parent->definition->name, it.lineNumber);
					assert(false);
				}
				break;

				default:
					assert(false);
				break;
			}
		}
		/*bool foundProperty = false;
		for (s32 j = 0; j < existingParameters->names->used; ++j)
		{
			Token& innerIt = existingParameters->names->data[j];
			if (it.cmp(innerIt))
			{
				AST_Expression* expression = parameters->values->data[i];
				existingParameters->values->data[j] = evaluateExpression(compiler, expression);
				foundProperty = true;
				break;
			}
		}

		if (!foundProperty)
		{
			AST* parentLoose = parameters->parent;
			compiler->hasError = true;
			switch (parentLoose->type)
			{
				case AST_COMPONENT:
				{
					AST_Identifier* parent = (AST_Identifier*)parentLoose;
					compileError("Unable to override property '%s' in component '%s' on Line %d as it does not exist.", &it, &parent->definition->name, it.lineNumber);
					assert(false);
				}
				break;

				default:
					assert(false);
				break;
			}
		}*/
	}
}

inline HTML_Element* compileLayout(Compiler* compiler, AST_Layout* layout, CompilerParameters* parameters)
{
	assert(layout != NULL);

	TemporaryPoolScope tempPool(compiler->poolTransient);
	Array<AST*> stack(256, tempPool);
	for (s32 i = layout->childNodes->used - 1; i >= 0; --i)
	{
		stack.push(layout->childNodes->data[i]);
	}

	//
	// Setup variables in this scope
	//
	CompilerParameters* variableStack = NULL;
	if ((layout->variables != NULL && layout->variables->used > 0)
		|| (parameters != NULL && parameters->values->used > 0))
	{
		s32 variableCount = 0;
		if (layout->variables != NULL) {
			variableCount += layout->variables->used;
		}
		if (parameters != NULL) {
			assert(parameters->names->used == parameters->values->used);
			variableCount += parameters->values->used;
		}

		variableStack = pushStruct(CompilerParameters, tempPool);
		variableStack->names = Array<Token>::create(variableCount, tempPool._allocator);
		variableStack->values = Array<CompilerValue>::create(variableCount, tempPool._allocator);
		compiler->stack->push(variableStack);

		if (layout->variables == NULL) 
		{
			// If no statements in this 'layout' block then simply
			// add passed in parameters for the component identifier.
			for (s32 i = 0; i < parameters->names->used; ++i)
			{
				variableStack->names->push(parameters->names->data[i]);
				variableStack->values->push(parameters->values->data[i]);
			}
		}
		else
		{
			CompilerValue undefValue = {};
			undefValue.type = COMPILER_VALUE_TYPE_UNDEFINED;

			// Setup the variables in scope (based off AST_STATEMENT count)
			for (s32 i = 0; i < layout->variables->used; ++i)
			{
				variableStack->names->push(layout->variables->data[i]);
				variableStack->values->push(undefValue);
			}

			if (parameters != NULL && parameters->names->used > 0)
			{
				// Override the variables in scope initial value with the passed in
				// component properties. If they don't exist as an AST_STATEMENT then
				// just simply add them in.
				for (s32 i = 0; i < parameters->names->used; ++i)
				{
					Token& name = parameters->names->data[i];
					for (s32 j = 0; j < variableStack->names->used; ++j)
					{
						if (name.cmp(variableStack->names->data[j]))
						{
							variableStack->values->data[j] = parameters->values->data[i];
							continue;
						}
					}
					variableStack->names->push(name);
					variableStack->values->push(parameters->values->data[i]);
				}
			}
		}
	}

	// Get context (ie. root or under component layout)
	// NOTE(Jake): This can probably be simplified by just passing 'ast->definition' to the function
	//			   will wait to see if that's the best idea.
	AST_ComponentDefinition* component = NULL;
	if (layout->parent != NULL)
	{
		switch (layout->parent->type)
		{
			case AST_COMPONENTDEFINITION:
			{
				AST_Identifier* ast = (AST_Identifier*)layout->parent;
				component = ast->definition;
			}
			break;

			default:
				assert(false);
			break;
		}
	}

	// Current top element
	Array<HTML_Block*> elementTopStack(256, tempPool);
	HTML_Root* elementResult = pushHTML(HTML_Root, HTML_ROOT, compiler->pool);
	elementResult->component = component;
	elementTopStack.push(elementResult);

	AST_Expression_Block* loopElement = NULL;
	s32 loopCount = 0;

	while (stack.used > 0)
	{
		AST* ast_top = stack.top();
		if (ast_top == NULL)
		{
			stack.pop();
			elementTopStack.pop();
			continue;
		}
		HTML* newElement = NULL;

		if (ast_top->type == AST_TAG)
		{
			AST_Identifier* ast = (AST_Identifier*)ast_top;

			HTML_Element* element = pushHTML(HTML_Element, HTML_ELEMENT, compiler->pool);
			element->name = ast->name;
			element->component = component;
			if (ast->parameters != NULL) {
				element->parameters = evaluateParameters(compiler, ast->parameters);
			}
			newElement = element;
		}
		else if (ast_top->type == AST_BACKEND_IDENTIFIER)
		{
			AST_Identifier* ast = (AST_Identifier*)ast_top;

			HTML* element = pushHTML(HTML, HTML_BACKEND_IDENTIFIER, compiler->pool);
			element->name = ast->name;
			element->component = component;
			newElement = element;
		}
		else if (ast_top->type == AST_BACKEND_FUNCTION)
		{
			AST_Identifier* ast = (AST_Identifier*)ast_top;

			HTML_Backend_Function* element = pushHTML(HTML_Backend_Function, HTML_BACKEND_FUNCTION, compiler->pool);
			element->name = ast->name;
			element->component = component;
			if (ast->parameters != NULL) {
				element->parameters = evaluateParameters(compiler, ast->parameters);
			}
			newElement = element;
		}
		else if (ast_top->type == AST_IF || ast_top->type == AST_LOOP || ast_top->type == AST_WHILE)
		{
			AST_Expression_Block* ast = (AST_Expression_Block*)ast_top;
			CompilerValue value = evaluateExpression(compiler, &ast->expression);
			assert(value.type == COMPILER_VALUE_TYPE_DOUBLE || value.type == COMPILER_VALUE_TYPE_STRING || value.type == COMPILER_VALUE_TYPE_BACKEND_EXPRESSION);

			if (value.type == COMPILER_VALUE_TYPE_BACKEND_EXPRESSION)
			{
				HTML_Backend_Expression_Block* element = NULL;
				if (ast_top->type == AST_IF)
				{
					element = pushHTML(HTML_Backend_Expression_Block, HTML_BACKEND_IF, compiler->pool);
				}
				else if (ast_top->type == AST_WHILE)
				{
					element = pushHTML(HTML_Backend_Expression_Block, HTML_BACKEND_WHILE, compiler->pool);
				}
				else
				{
					assert(false);
				}
				element->expression = value.valueExpression;
				newElement = element;
			}
			else
			{
				if (ast_top->type == AST_IF)
				{
					if (!value.isTrue())
					{
						// If false, skip adding the children and stop processing this.
						stack.pop();
						continue;
					}
				}
				else if (ast_top->type == AST_WHILE)
				{
					if (!value.isTrue())
					{
						// If false, skip adding the children and stop processing this.
						stack.pop();
						continue;
					}
					else
					{
						++loopCount;
						if (loopElement != ast)
						{
							loopElement = ast;
							loopCount = 0;
						}
						if (loopCount > 500)
						{
							compileError("Reached loop limit of 500");
							stack.pop();
							continue;
						}

						// Add children and keep this AST element on the stack
						for (s32 i = ast_top->childNodes->used - 1; i >= 0; --i)
						{
							stack.push(ast_top->childNodes->data[i]);
						}
						continue;
					}
				}
				else
				{
					assert(false);
				}
			}
		}
		else if (ast_top->type == AST_COMPONENT)
		{
			AST_Identifier* ast = (AST_Identifier*)ast_top;
			
			//
			// compileComponentLayout
			//
			assert(ast->definition != NULL);

			AST_Parameters* parameters = ast->parameters;
			if (parameters != NULL && parameters->values != NULL && parameters->values->used > 0)
			{
				if (parameters->names == NULL || parameters->names->used == 0)
				{
					AST_Identifier* parent = (AST_Identifier*)parameters->parent;
					assert(parent->type == AST_COMPONENT);
					compiler->hasError = true;
					compileError("Component '%s' requires named parameters on Line %d.", &parent->name, parent->name.lineNumber);
					return NULL;
				}
			}

			AST_Layout* layout = ast->definition->layout;
			if (layout != NULL)
			{
				CompilerParameters* evaluatedParameters = NULL;
				if (ast->definition->properties != NULL) 
				{
					// Evaluate the 'properties' of the component being referenced in the 'layout'
					evaluatedParameters = evaluateParameters(compiler, ast->definition->properties);
				}
				if (ast->parameters != NULL) 
				{
					if (evaluatedParameters == NULL)
					{
						compileError("Cannot pass parameters to component that doesn't have any properties.");
						return NULL;
					}
					// Override the default 'properties' with parameters provided (ie. Banner(IsAtTop = 1), will modify 'evaluatedParameters' to have 'IsAtTop' to be equal to 1)
					evaluateParametersAndModifyExistingParameters(compiler, ast->parameters, evaluatedParameters);
				}
				if (compiler->hasError)
				{
					return NULL;
				}

				newElement = compileLayout(compiler, ast->definition->layout, evaluatedParameters);
			}
		}
		else if (ast_top->type == AST_IDENTIFIER)
		{
			//
			// Determine what this identifier is, is it a component, a function, or what?
			//
			AST_Identifier* ast = (AST_Identifier*)ast_top;

			if (ast->name.isBackend()) 
			{
				if (ast->isFunction) 
				{
					ast->type = AST_BACKEND_FUNCTION;
				}
				else
				{
					ast->type = AST_BACKEND_IDENTIFIER;
				}
				ast->name.data += 1;
				ast->name.length -= 1;
				continue; // re-iterate over this 'ast'
			}
			else if (ast->name.cmp("children"))
			{
				if (component == NULL)
				{
					compiler->hasError = true;
					compileError("Cannot use 'children' keyword in this context. It's reserved for component layouts.");
					return NULL;
				}

				HTML_Element* element = pushHTML(HTML_Element, HTML_ELEMENT_VIRTUAL, compiler->pool);
				element->name = ast->name;
				element->component = component;
				newElement = element;

				elementResult->childInsertionElement = element;
			}
			else
			{
				AST_ComponentDefinition* definition = findComponentDefinition(compiler, ast->name);
				if (compiler->hasError)
				{
					return NULL;
				}

				if (definition == NULL)
				{
					// todo(Jake): Check for 'func [name]' when that's parsed/implemented, if it has 'childNodes', then it
					//			   cannot be a function. 
					// If no definition, assume HTML tag
					ast->type = AST_TAG;
				}
				else
				{
					ast->type = AST_COMPONENT;
					ast->definition = definition;
					// Add component to list of used components
					if (compiler->componentsUsed->find(ast->definition) == -1) {
						compiler->componentsUsed->push(ast->definition);
					}
				}

				if (ast->expression.tokens != NULL && ast->expression.tokens->used > 0)
				{
					CompilerValue value = evaluateExpression(compiler, &ast->expression);
					if (value.isTrue())
					{
						continue; // re-iterate over this 'ast'
					}
					// if false, drop down below, remove this element from the stack but add
					// its children
				}
				else
				{
					continue; // re-iterate over this 'ast'
				}
			}
		}
		else if (ast_top->type == AST_STATEMENT)
		{
			AST_Statement* ast = (AST_Statement*)ast_top;
			CompilerParameters* stackVariables = compiler->stack->top();
			assert(stackVariables != NULL);
			assert(ast->op.type == TOKEN_EQUAL);

			s32 findIndex = stackVariables->names->find(ast->name);
			assert(findIndex != -1); // can't find variable
			if (findIndex != -1)
			{
				// NOTE: Setting values array as the names/values arrays should be linked.
				stackVariables->values->data[findIndex] = evaluateExpression(compiler, &ast->expression);
			}
			/*bool foundName = false;
			for (s32 i = 0; i < stackVariables->names->used; ++i)
			{
				if (ast->name.cmp(stackVariables->names->data[i]))
				{
					stackVariables->values->data[i] = evaluateExpression(compiler, &ast->expression);
					foundName = true;
					break;
				}
			}
			assert(foundName);*/
		}
		else 
		{
			// Unknown ast type in 'layout' block
			assert(false);
		}

		// Add this element to the parent
		if (newElement != NULL) {
			HTML_Block* elementTop = elementTopStack.top();
			if (elementTop->childNodes->used == elementTop->childNodes->size) {
				// Increase child element array size
				if (ast_top->parent != NULL && ast_top->parent->type == AST_LOOP) {
					// NOTE(Jake): A bit of a hack to decrease resizes underneath loops
					elementTop->childNodes->resize(elementTop->childNodes->size * 32); // approx ~500
				} else {
					elementTop->childNodes->resize(elementTop->childNodes->size * 4);
				}
			}
			elementTop->childNodes->push(newElement);
		}
			
		stack.pop();
		if (ast_top->childNodes->used > 0)
		{
			if (newElement != NULL)
			{
				// Add NULL which represents end of this block, this is used to detect when the current
				// top level element should be popped off the end. This ensures HTML elements are properly
				// nested.
				stack.push(NULL);
				if (newElement->type == HTML_ROOT)
				{
					// Inserts future elements underneath whatever element the 'children' keyword was found in.
					HTML_Root* element = (HTML_Root*)newElement;
					if (element->childInsertionElement == NULL) {
						compileError("Missing 'children' keyword in component layout. (%s)", &element->name);
						return NULL;
					}
					elementTopStack.push(element->childInsertionElement);
				}
				else if (newElement->type == HTML_ELEMENT || newElement->type == HTML_BACKEND_IF || newElement->type == HTML_BACKEND_WHILE) 
				{
					HTML_Block* element = (HTML_Block*)newElement;
					elementTopStack.push(element);
				} 
				else 
				{
					// NOTE(Jake): If on this code path, then the 'newElement' should not have childNodes
					assert(newElement->type == HTML_TEXT || newElement->type == HTML_BACKEND_FUNCTION);
					elementTopStack.push(elementTopStack.top());
				}
			}
			// Add children in reverse so that they're processed from first to last
			// (as this loop pops values off the end of the array)
			for (s32 i = ast_top->childNodes->used - 1; i >= 0; --i)
			{
				stack.push(ast_top->childNodes->data[i]);
			}
		}
	}

	if (variableStack != NULL)
	{
		compiler->stack->pop();
	}


	return elementResult;
}

inline CSS_Rule* compileStyle(Compiler* compiler, CSS_Rule* styleBlockRule)
{
	if (styleBlockRule->properties->used > 0)
	{
		CSS_Property* prop = &styleBlockRule->properties->data[0];
		compileError("No properties should exist outside a selector rule on Line %d.", prop->name.lineNumber);
		return NULL;
	}

	// Setup flat array of CSS rules
	TemporaryPoolScope tempPoolFnScope(compiler->poolTransient);
	Array<CSS_Rule*>* flatCSSRuleArray = Array<CSS_Rule*>::create(1024, tempPoolFnScope);
	
	// Recursively iterate through and add to flat array
	{
		TemporaryPoolScope stackTempPool(compiler->poolTransient);
		Array<CSS_Rule*>* stack = Array<CSS_Rule*>::create(256, stackTempPool);
		stack->push(styleBlockRule);
		while (stack->used > 0)
		{
			CSS_Rule* stackTop = stack->pop();
			flatCSSRuleArray->push(stackTop);
			for (s32 i = stackTop->childRules->used - 1; i >= 0; --i)
			{
				stack->push(stackTop->childRules->data[i]);
			}
		}
	}

	// NOTE(Jake): Skip item 0 as the first CSS_Rule will never have a parent.
	for (s32 i = 1; i < flatCSSRuleArray->used; ++i)
	{
		CSS_Rule* rule = flatCSSRuleArray->data[i];
		assert(rule->parent != NULL);

		if (rule->type == CSS_RULE_MEDIAQUERY)
		{
			if (rule->parent != styleBlockRule)
			{
				/*
					Handle @media query underneath CSS rule case
					---------------------------------------------
					.test {
						.banner {
							@media screen and (max-width: 1140px) {
								color: #000;
							}
						}
					}
				*/


				CSS_Rule* parent = rule->parent;
				while (parent->parent != styleBlockRule)
				{
					parent = parent->parent;
				}
				
				//rule->parent = styleBlockRule;
				//parent->parent = rule;


				/*CSS_Rule* newRule = pushStruct(CSS_Rule, compiler->pool);
				*newRule = *rule->parent;
				newRule->properties = rule->properties;
				newRule->parent = rule; // Move CSS rule to be underneath this @media query
				flatCSSRuleArray->push(newRule);*/
			}
			continue;
		}

		// Go up parent tree
		TemporaryPoolScope tempPool(compiler->poolTransient);
		Array<CSS_Rule*>* treeStack = Array<CSS_Rule*>::create(512, tempPool);
		Array<s32>* treeIndexStack = Array<s32>::create(512, tempPool);

		CSS_Rule* parent = rule;
		CSS_Rule* newParent = styleBlockRule; // default to expanding to top 'style' block
		while (parent != NULL && parent->parent != NULL)
		{
			if (parent->type == CSS_RULE_MEDIAQUERY) {
				// Switch to expanding inside media query
				newParent = parent;
				break;
			}
			treeStack->push(parent);
			treeIndexStack->push(0);
			parent = parent->parent;
		}

		if (treeStack->used > 1 && rule->properties != NULL && rule->properties->used > 0)
		{
			TemporaryPoolScope tempSelectorSetsPool(compiler->poolTransient);
			Array<Array<CSS_Selector>>* newSelectorSets = Array<Array<CSS_Selector>>::create(255, tempSelectorSetsPool);

			s32 ridiculousIterationCounter = 0;
			for (;;)
			{
				{
					TemporaryPoolScope tempSelectorsPool(compiler->poolTransient);
					Array<CSS_Selector>* newSelectors = Array<CSS_Selector>::create(512, tempSelectorsPool);
					for (s32 i = treeStack->used - 1; i >= 0; --i)
					{
						CSS_Rule* rule = treeStack->data[i];
						s32 selectorSetIndex = treeIndexStack->data[i];
						Array<CSS_Selector>* selectors = &rule->selectorSets->data[selectorSetIndex];
						for (s32 i = 0; i < selectors->used; ++i)
						{
							newSelectors->push(selectors->data[i]);
						}
					}

					newSelectors = newSelectors->createCopyShrinkToFit(compiler->pool);
					newSelectorSets->push(*newSelectors);
				}

				// Increment index, if index becomes bigger then array, bump up the index
				// of the next item in the tree.
				// -- parentIndexStack->data[0] == 1
				// -- parentIndexStack->data[1] == 0
				// -- parentIndexStack->data[2] == 0
				treeIndexStack->data[0] += 1;
				for (s32 i = 0; i < treeStack->used - 1; ++i)
				{
					CSS_Rule* rule = treeStack->data[i];
					if (treeIndexStack->data[i] >= rule->selectorSets->used)
					{
						treeIndexStack->data[i+1] += 1;
						treeIndexStack->data[i] = 0;
					}
				}

				if (treeIndexStack->data[treeStack->used - 1] >= treeStack->data[treeStack->used - 1]->selectorSets->used)
				{
					break;
				}

				++ridiculousIterationCounter;
				assert(ridiculousIterationCounter <= 1000000);
			}
			rule->selectorSets = newSelectorSets->createCopyShrinkToFit(compiler->pool);
			rule->parent = newParent;

			// Debug
			/*CSS_Rule tempRule = {};
			tempRule.type = CSS_RULE_SELECTOR;
			tempRule.selectorSets = newSelectorSets;
			printCSSRule(&tempRule, compiler->poolTransient);*/
		}
	}

	// Put into hierarchy
	// NOTE(Jake): Skip item 0 as the first CSS_Rule will never have a parent.
	for (s32 i = 1; i < flatCSSRuleArray->used; ++i)
	{
		CSS_Rule* rule = flatCSSRuleArray->data[i];
		assert(rule->parent != NULL);
		rule->childRules->used = 0;
		if (rule->properties != NULL && rule->properties->used > 0)
		{
			if (rule->parent->type == CSS_RULE_MEDIAQUERY)
			{
				TemporaryPoolScope tempPoolScope(compiler->poolTransient);
				Buffer buffer((s32)Kilobytes(1), tempPoolScope);
				printCSSRuleOpen(buffer, rule->parent);
				printCSSRule(buffer, rule);
				printCSSRuleClose(buffer);
				buffer.print();
			}
			else if (rule->parent->type != CSS_RULE_MEDIAQUERY) 
			{
				if (rule->type != CSS_RULE_MEDIAQUERY)
				{
					printCSSRule(rule, compiler->poolTransient);
				}
			}
			else
			{
				assert(false);
			}
		}
	}

	assert(false);
	return styleBlockRule;
}

void compile(Compiler* compiler)
{
	// Compile HTML
	for (s32 i = 0; i < compiler->astFiles->used; ++i)
	{
		AST_File* ast_file = &compiler->astFiles->data[i];
		if (ast_file->layouts != NULL)
		{
			for (s32 j = 0; j < ast_file->layouts->used; ++j)
			{
				AST_Layout* layout = &ast_file->layouts->data[j];
				HTML_Element* html = compileLayout(compiler, layout);
				if (compiler->hasError)
				{
					assert(false);
					return;
				}
				if (html != NULL)
				{
					TemporaryPoolScope tempPoolScope(compiler->poolTransient);
					Buffer buffer((s32)Megabytes(4), tempPoolScope);
					printHTML(buffer, html);

					// Trim the target path from pathname
					String partialPath = ast_file->pathname.substring(compiler->targetDirectory.length);
					if (partialPath.data[0] == '\\' || partialPath.data[0] == '/')
					{
						++partialPath.data;
						--partialPath.length;
					}
					partialPath.length -= 4; // remove '.fel'

					//
					StringBuilder builder(10, tempPoolScope);
					builder.add(compiler->outputDirectory);
					builder.add(partialPath);
					builder.add(".php");
					String outputPath = builder.toString(compiler->pool); // todo(Jake): Decide to make transient or keep

					// show output

					print("\n------------------------\n");
					print("Compiled From: %s", &ast_file->pathname);
					print("\nCompiled To: %s", &outputPath);
					print("\n------------------------\n");
					buffer.print();
					print("\n");


					//File::writeEntireFile(outputPath, &buffer);
				}
			}
		}
	}

	// Compile CSS
	for (s32 i = 0; i < compiler->componentsUsed->used; ++i)
	{
		AST_ComponentDefinition* componentDefintion = compiler->componentsUsed->data[i];
		if (componentDefintion->style != NULL && componentDefintion->style->rule != NULL)
		{
			CSS_Rule* topRule = componentDefintion->style->rule;
			topRule = compileStyle(compiler, topRule);
			if (compiler->hasError)
			{
				assert(false);
				return;
			}

			TemporaryPoolScope tempPoolScope(compiler->poolTransient);
			Buffer buffer((s32)Megabytes(4), tempPoolScope);
			for (s32 i = 0; i < topRule->childRules->used; ++i)
			{
				CSS_Rule* rule = topRule->childRules->data[i];
				printCSSRule(buffer, rule);
				buffer.add("\n");
			}
			if (buffer.used > 0)
			{
				print("\n------------------------\n");
				print("CSS From: %s\n", &componentDefintion->name);
				print("\n------------------------\n");
				buffer.print();
				print("\n\n");
			}
		}
	}
}
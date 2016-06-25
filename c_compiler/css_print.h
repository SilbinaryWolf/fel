#ifndef CSS_PRINT_INCLUDE
#define CSS_PRINT_INCLUDE

#include "css.h"

void printCSSRuleOpen(Buffer& buffer, CSS_Rule* absoluteTopCSSRule)
{
	if (absoluteTopCSSRule->type == CSS_RULE_MEDIAQUERY)
	{
		buffer.add("@media ");
		for (s32 i = 0; i < absoluteTopCSSRule->selectorSets->used; ++i)
		{
			if (i != 0)
			{
				buffer.add(",");
				buffer.addNewline();
			}
			Array<CSS_Selector>* selectors = &absoluteTopCSSRule->selectorSets->data[i];
			for (s32 i = 0; i < selectors->used; ++i)
			{
				CSS_Selector* selector = &selectors->data[i];
				if (i != 0 && selector->token.type != TOKEN_PAREN_CLOSE && selector->token.type != TOKEN_COLON)
				{
					buffer.add(" ");
				}
				buffer.add("%s", &selector->token);
			}
		}
	}
	else
	{
		assert(false);
	}

	buffer.add(" {");
	++buffer.indent;
	buffer.addNewline();
}

void printCSSRuleClose(Buffer& buffer) {
	--buffer.indent;
	buffer.addNewline();
	buffer.add("}");
}

void printCSSRule(Buffer& buffer, CSS_Rule* absoluteTopCSSRule)
{
	if (absoluteTopCSSRule->selectorSets == NULL || absoluteTopCSSRule->selectorSets->used == 0)
	{
		return;
	}

	if (absoluteTopCSSRule->type == CSS_RULE_SELECTOR)
	{
		for (s32 i = 0; i < absoluteTopCSSRule->selectorSets->used; ++i)
		{
			if (i != 0)
			{
				buffer.add(',');
				buffer.addNewline();
			}
			Array<CSS_Selector>* selectors = &absoluteTopCSSRule->selectorSets->data[i];
			for (s32 i = 0; i < selectors->used; ++i)
			{
				CSS_Selector* selector = &selectors->data[i];
				if (i != 0 && selector->type != CSS_SELECTOR_ATTRIBUTE)
				{
					buffer.add(' ');
				}
				if (selector->type == CSS_SELECTOR_ATTRIBUTE)
				{
					buffer.add("[%s=", &selector->attribute.name, &selector->attribute.op);
					if (selector->attribute.value.type == TOKEN_STRING) {
						buffer.add("\"%s\"", &selector->attribute.value);
					} else {
						buffer.add("%s", &selector->attribute.value);
					}
					buffer.add(']');
					buffer.print();
				}
				else
				{
					buffer.add("%s", &selector->token);
				}
			}
		}

		buffer.add(" {");
		++buffer.indent;
		buffer.addNewline();

		if (absoluteTopCSSRule->properties != NULL && absoluteTopCSSRule->properties->used > 0)
		{
			for (s32 p = 0; p < absoluteTopCSSRule->properties->used; ++p)
			{
				if (p != 0)
				{
					buffer.addNewline();
				}
				CSS_Property* cssProperty = &absoluteTopCSSRule->properties->data[p];
				buffer.add("%s: ", &cssProperty->name);
				for (s32 i = 0; i < cssProperty->tokens->used; ++i)
				{
					if (i != 0)
					{
						buffer.add(' ');
					}
					CSS_PropertyToken propToken = cssProperty->tokens->data[i];
					buffer.add("%s", &propToken.token);
					if (propToken.arguments != NULL && propToken.arguments->used > 0)
					{
						buffer.add("(");
						for (s32 i = 0; i < propToken.arguments->used; ++i)
						{
							if (i != 0) {
								buffer.add(",");
							}
							buffer.add("%s", &propToken.arguments->data[i]);
						}
						buffer.add(")");
					}
				}
				buffer.add(";");
			}
		}
	}
	else if (absoluteTopCSSRule->type == CSS_RULE_MEDIAQUERY)
	{
		printCSSRuleOpen(buffer, absoluteTopCSSRule);
	}
	else
	{
		assert(false);
	}

	if (absoluteTopCSSRule->childRules != NULL && absoluteTopCSSRule->childRules->used > 0)
	{
		if (absoluteTopCSSRule->properties->used > 0) 
		{
			buffer.addNewline();
		}
		buffer.addNewline();
		for (s32 i = 0; i < absoluteTopCSSRule->childRules->used; ++i)
		{
			if (i != 0)
			{
				buffer.addNewline();
			}
			CSS_Rule* rule = absoluteTopCSSRule->childRules->data[i];
			printCSSRule(buffer, rule);
		}
	}

	--buffer.indent;
	buffer.addNewline();
	buffer.add("}");
}

void printCSSRule(CSS_Rule* cssRule, AllocatorPool* pool)
{
	TemporaryPoolScope tempPoolScope(pool);
	Buffer buffer((s32)Megabytes(4), tempPoolScope);
	printCSSRule(buffer, cssRule);
	buffer.print();
}

#endif
#ifndef TOKENS_CSS_INCLUDE
#define TOKENS_CSS_INCLUDE

#include "tokens.h"

enum CSSSelectorType {
	CSS_SELECTOR_UNKNOWN = 0,
	CSS_SELECTOR_ID, // #myAnchor, #main
	CSS_SELECTOR_CLASS, // .myDiv, .container
	CSS_SELECTOR_TAG, // html, div, span, i
	CSS_SELECTOR_PAREN_OPEN, // (
	CSS_SELECTOR_PAREN_CLOSE, // )
	CSS_SELECTOR_ALL, // *
	CSS_SELECTOR_ATTRIBUTE, // ie. [hidden] or [type="text"]
	CSS_SELECTOR_MODIFIER, // ie. ":not"
	CSS_SELECTOR_CHILD, // ie. >, eg. .banner > .banner-title, direct descendant.
	CSS_SELECTOR_MEDIAQUERY_TOKEN, // For any token found after @media
};

enum CSSRuleType {
	CSS_RULE_UNKNOWN = 0,
	CSS_RULE_SELECTOR,
	CSS_RULE_MEDIAQUERY,
};

struct CSS_Selector_Attribute {
	Token name;
	Token op;
	Token value;
};

struct CSS_Selector {
	CSSSelectorType type;
	union 
	{
		// Selector (ie. `.myDiv`, `#main`)
		Token token;
		// Attribute Selector (ie. `type="text"` or `type`)
		CSS_Selector_Attribute attribute;
	};
};

struct CSS_PropertyToken {
	Token token; // ie. '100%', 'rgba(arguments)'
	Array<Token>* arguments; // ie. 0,0,0,0.7 of 'rgba' 
};

struct CSS_Property {
	Token name; // ie. 'width', 'font-size', 'color'
	Array<CSS_PropertyToken>* tokens;
};

struct CSS_Rule {
	CSSRuleType type;
	Array<Array<CSS_Selector>>* selectorSets;
	Array<CSS_Property>* properties;
	Array<CSS_Rule*>* childRules;
	CSS_Rule* parent;
};

#endif
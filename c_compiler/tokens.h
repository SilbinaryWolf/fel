#ifndef TOKENS_INCLUDE
#define TOKENS_INCLUDE

#include "string.h"
#include "array.h"

enum TokenType {
	TOKEN_UNKNOWN = 0,
	TOKEN_EOF,
	TOKEN_PAREN_OPEN, // (
	TOKEN_PAREN_CLOSE, // )
	TOKEN_BRACE_OPEN, // {
	TOKEN_BRACE_CLOSE, // }
	TOKEN_BRACKET_OPEN, // [
	TOKEN_BRACKET_CLOSE, // ]
	TOKEN_STRING,
	TOKEN_IDENTIFIER,
	TOKEN_NUMBER,
	TOKEN_HEX, // ie. #000, #666, #000000

	// Expression Tokens
	TOKEN_NOT,
	TOKEN_EQUAL,
	TOKEN_OR,
	TOKEN_AND,
	TOKEN_COND_EQUAL,
	TOKEN_COND_OR,
	TOKEN_COND_AND,
	TOKEN_COND_ABOVE,
	TOKEN_PLUS,
	TOKEN_MINUS,
	TOKEN_DIVIDE,
	TOKEN_MULTIPLY,

	TOKEN_COMMA,
	TOKEN_COLON,
	TOKEN_SEMICOLON,
	TOKEN_PERCENT,
	TOKEN_DOT,
	TOKEN_HASH,
	TOKEN_NEWLINE,
};

struct Token : String {
	TokenType type;
	u32 lineNumber;
	String pathName;
	inline bool isOperator()
	{
		return (type == TOKEN_EQUAL || type == TOKEN_OR || type == TOKEN_AND
				|| type == TOKEN_COND_EQUAL || type == TOKEN_COND_OR || type == TOKEN_COND_AND
				|| type == TOKEN_PLUS || type == TOKEN_MINUS || type == TOKEN_DIVIDE || type == TOKEN_MULTIPLY);
	}
	inline bool isBackend()
	{
		return (length >= 1 && data[0] == '$');
	}
	inline s32 getPrecedence()
	{
		switch (type)
		{
			case TOKEN_PAREN_OPEN:
			case TOKEN_PAREN_CLOSE:
				return 1;
			break;

			case TOKEN_COND_OR:
			case TOKEN_COND_AND:
				return 2;
			break;

			case TOKEN_COND_EQUAL:
			// case TOKEN_COND_NOT_EQUAL:
			// case TOKEN_COND_ABOVE_EQUAL:
			// etc. NOTE(Jake): get the rest from PHP
				return 3;
			break;

			case TOKEN_PLUS:
			case TOKEN_MINUS:
			case TOKEN_DIVIDE:
			case TOKEN_MULTIPLY:
				return 4;
			break;

			default:
				assert(false);
			break;
		}
		return -1;
	}
};

#endif
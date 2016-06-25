#ifndef LEXER_INCLUDE
#define LEXER_INCLUDE

#include "tokens.h"
#include "array.h"

#define lexError(token, format, ...) printf("Parse error(%d,%d): "format"\n", token.lineNumber, __LINE__, ## __VA_ARGS__)

struct TokenizerState {
	char* at;
	u32 lineNumber;
};

struct Tokenizer {
	TokenizerState state;
	String string;
	String pathName;
	AllocatorPool* pool;
	AllocatorPool* poolTransient;

	// Debugging vars
	Token lastGetToken;
	Token currGetToken;
};

enum GetTokenFlags {
	GET_TOKEN_NO_FLAGS = 0,
	GET_TOKEN_ACCEPT_NEWLINE = 1,
};

inline internal bool isEndOfLine(char C)
{
    return ((C == '\n') ||
           (C == '\r'));
}

inline internal bool isWhitespace(char C)
{
    return ((C == ' ') ||
			(C == '\t') ||
			(C == '\v') ||
			(C == '\f'));
}

inline internal bool isAlpha(char C)
{
	return (((C >= 'a') && (C <= 'z')) ||
           ((C >= 'A') && (C <= 'Z')));
}

inline internal bool isAlphaUnderscoreDash(char C)
{
	return isAlpha(C) || C == '_' || C == '-';
}

inline internal bool isNumber(char C)
{
    return ((C >= '0') && (C <= '9'));
}

inline internal bool isDecimalNumber(char C)
{
    return isNumber(C) || C == '.';
}

inline internal Token getString(Token token, Tokenizer* tokenizer)
{
	token.type = TOKEN_STRING;
    token.data = tokenizer->state.at;
            
    while(tokenizer->state.at[0] &&
            tokenizer->state.at[0] != '"')
    {
        if((tokenizer->state.at[0] == '\\') &&
            tokenizer->state.at[1])
        {
            ++tokenizer->state.at;
        }                
        ++tokenizer->state.at;
    }

	token.length = tokenizer->state.at - token.data;
    if(tokenizer->state.at[0] == '"')
    {
        ++tokenizer->state.at;
    }
	return token;
}

inline internal bool eatEndOfLine(Tokenizer* tokenizer)
{
	if (tokenizer->state.at[0] == '\r' && tokenizer->state.at[1] == '\n'
		|| tokenizer->state.at[0] == '\n' && tokenizer->state.at[1] == '\r') {
		++tokenizer->state.lineNumber;
		tokenizer->state.at += 2;
		return true;
	} else if (isEndOfLine(tokenizer->state.at[0])) {
		++tokenizer->state.lineNumber;
		++tokenizer->state.at;
		return true;
	}
	return false;
}

internal void eatAllWhitespace(Tokenizer* tokenizer, GetTokenFlags flags)
{
	s32 commentBlockDepth = 0;
	bool eatNewline = !(flags & GET_TOKEN_ACCEPT_NEWLINE);

    for(;;)
    {
		if (eatNewline && eatEndOfLine(tokenizer)) {
			continue;
		}
        else if(isWhitespace(tokenizer->state.at[0]))
        {
            ++tokenizer->state.at;
        }
        else if((tokenizer->state.at[0] == '/') &&
                (tokenizer->state.at[1] == '/'))
        {
            tokenizer->state.at += 2;
            while(tokenizer->state.at[0] && !isEndOfLine(tokenizer->state.at[0]))
            {
                ++tokenizer->state.at;
            }
			eatEndOfLine(tokenizer);
        }
        else if((tokenizer->state.at[0] == '/') &&
                (tokenizer->state.at[1] == '*'))
        {
			++commentBlockDepth;
            tokenizer->state.at += 2;
            while(tokenizer->state.at[0] && commentBlockDepth > 0)
            {
				if (tokenizer->state.at[0] == '/' && tokenizer->state.at[1] == '*') {
					++commentBlockDepth;
					tokenizer->state.at += 2;
				} else if (tokenizer->state.at[0] == '*' && tokenizer->state.at[1] == '/') {
					--commentBlockDepth;
					tokenizer->state.at += 2;
				} else if (!eatEndOfLine(tokenizer)) {
					++tokenizer->state.at;
				}
            }
        }
        else
        {
            break;
        }
    }
}

inline internal Token initTokenWithTokenizer(Tokenizer* tokenizer)
{
	Token token;
	zeroMemory(&token, sizeof(Token));
    token.length = 1;
	token.pathName = tokenizer->pathName;
	token.lineNumber = tokenizer->state.lineNumber + 1;
    token.data = tokenizer->state.at;
	return token;
}

internal Token getToken(Tokenizer* tokenizer, GetTokenFlags flags = GET_TOKEN_NO_FLAGS)
{
	eatAllWhitespace(tokenizer, flags);

    Token token = initTokenWithTokenizer(tokenizer);


    char C = tokenizer->state.at[0];
    ++tokenizer->state.at;
    switch(C)
    {
        case '\0': {token.type = TOKEN_EOF;} break;
        case '(': {token.type = TOKEN_PAREN_OPEN;} break;
        case ')': {token.type = TOKEN_PAREN_CLOSE;} break;
        case '[': {token.type = TOKEN_BRACKET_OPEN;} break;
        case ']': {token.type = TOKEN_BRACKET_CLOSE;} break;
        case '{': {token.type = TOKEN_BRACE_OPEN;} break;
        case '}': {token.type = TOKEN_BRACE_CLOSE;} break;
		case ',': {token.type = TOKEN_COMMA;} break;
		case ':': {token.type = TOKEN_COLON;} break;
		case ';': {token.type = TOKEN_SEMICOLON;} break;
		case '!': {token.type = TOKEN_NOT;} break;

		// NOTE(Jake): Added for CSS parsing (removed 2016-03-05)
		//case '.': {token.type = TOKEN_DOT;} break;
		//case '#': {token.type = TOKEN_HASH;} break;

		// Expressions
		case '+': { token.type = TOKEN_PLUS; } break;
		case '-': { token.type = TOKEN_MINUS; } break;
		case '/': { token.type = TOKEN_DIVIDE; } break;
		case '*': { token.type = TOKEN_MULTIPLY; } break;

		case '|':
			token.type = TOKEN_OR;
			if (tokenizer->state.at[0] == C)
			{
				token.type = TOKEN_COND_OR;
				++token.length;
				++tokenizer->state.at;
			}
		break;

		case '&':
			token.type = TOKEN_AND;
			if (tokenizer->state.at[0] == C)
			{
				token.type = TOKEN_COND_AND;
				++token.length;
				++tokenizer->state.at;
			}
		break;

		case '=':
			token.type = TOKEN_EQUAL;
			if (tokenizer->state.at[0] == C)
			{
				token.type = TOKEN_COND_EQUAL;
				++token.length;
				++tokenizer->state.at;
			}
		break;
            
        case '"':
        {
            token = getString(token, tokenizer);
        } 
		break;

        default:
        {
			if (isEndOfLine(C))
			{
				token.type = TOKEN_NEWLINE;
			}
            else if(C == '$' || isAlpha(C))
            {
                token.type = TOKEN_IDENTIFIER;
                
                while(isAlpha(tokenizer->state.at[0]) ||
                      isNumber(tokenizer->state.at[0]) ||
                      (tokenizer->state.at[0] == '_') ||
					  (tokenizer->state.at[0] == '.'))
                {
                    ++tokenizer->state.at;
                }
             
				token.length = tokenizer->state.at - token.data;
            }
            else if(isNumber(C))
            {
				token.type = TOKEN_NUMBER;

				while (isNumber(tokenizer->state.at[0]) || 
					  (tokenizer->state.at[0] == '.'))
					  // NOTE(Jake): For CSS properties, ie. '100%' in 'width: 100%', 16em
					  /*(tokenizer->state.at[0] == '%') ||
					  (tokenizer->state.at[0] == 'px') ||
					  (tokenizer->state.at[0] == 'em'))*/
				{
					++tokenizer->state.at;
				}

				token.length = tokenizer->state.at - token.data;
            }
            else
            {
                token.type = TOKEN_UNKNOWN;
				assert(false);
            }
        } break;        
    }

	// Debugging
	tokenizer->lastGetToken = tokenizer->currGetToken;
	tokenizer->currGetToken = token;

    return token;
}

inline internal Token peekToken(Tokenizer* tokenizer) {
	TokenizerState prevState = tokenizer->state;
	Token token = getToken(tokenizer);
	tokenizer->state = prevState;
	return token;
}

internal Token getTokenCSSProperty(Tokenizer* tokenizer, GetTokenFlags flags = GET_TOKEN_NO_FLAGS)
{
	eatAllWhitespace(tokenizer, flags);
	Token token = initTokenWithTokenizer(tokenizer);

    char C = tokenizer->state.at[0];
    ++tokenizer->state.at;
    switch(C)
    {
        case '\0': {token.type = TOKEN_EOF;} break;
		case ':': {token.type = TOKEN_COLON;} break;
		case '=': {token.type = TOKEN_EQUAL;} break;
		case ';': {token.type = TOKEN_SEMICOLON;} break;
		case '{': {token.type = TOKEN_BRACE_OPEN;} break;
        case '}': {token.type = TOKEN_BRACE_CLOSE;} break;
		case '[': {token.type = TOKEN_BRACKET_OPEN;} break; // use case: `input[type="text"]`
        case ']': {token.type = TOKEN_BRACKET_CLOSE;} break;
		case ',': {token.type = TOKEN_COMMA;} break; // use case: `font-family: monospace, monospace;`
		case '(': {token.type = TOKEN_PAREN_OPEN;} break; // use case: `rgba('
		case ')': {token.type = TOKEN_PAREN_CLOSE;} break; 
		case '>': {token.type = TOKEN_COND_ABOVE;} break; // use case: > for child selector
		
		case '"':
		{
			token = getString(token, tokenizer);
		}
		break;

		default:
        {
			if (isEndOfLine(C))
			{
				token.type = TOKEN_NEWLINE;
			}
			else if (C == '*' && isWhitespace(tokenizer->state.at[0]))
			{
				token.type = TOKEN_MULTIPLY;
			}
			else if((C == '@' || C == '_' || C == '-' || isAlpha(C))
					// Handle .class/#ID case but also allow .34 and #fff case below (isDecimalNumber)
					|| ((C == '.' || C == '#') && isAlpha(tokenizer->state.at[0])))
			{
				token.type = TOKEN_IDENTIFIER;
                
                while(isAlpha(tokenizer->state.at[0]) ||
                      (tokenizer->state.at[0] == '_') ||
					  (tokenizer->state.at[0] == '-') ||
					  (isNumber(tokenizer->state.at[0])))
                {
                    ++tokenizer->state.at;
                }
             
				token.length = tokenizer->state.at - token.data;
			}
			else if (C == '#')
			{
				token.type = TOKEN_HEX;
				++tokenizer->state.at;
				while(isDecimalNumber(tokenizer->state.at[0]))
				{
					++tokenizer->state.at;
				}
				token.length = tokenizer->state.at - token.data;
			}
			else if (isDecimalNumber(C))
			{
				token.type = TOKEN_NUMBER;
				while(isDecimalNumber(tokenizer->state.at[0]) 
					// for width: 100%
					|| tokenizer->state.at[0] == '%'
					// for font-size:'20px', '1em', etc
					|| isAlpha(tokenizer->state.at[0]))
				{
					++tokenizer->state.at;
				}
				token.length = tokenizer->state.at - token.data;
				/*if (isEndOfLine(tokenizer->state.at[0]))
				{
					--tokenizer->state.at;
				}*/
			}
			else
			{
				token.type = TOKEN_UNKNOWN;
				assert(false);
			}
		}
		break;
	}
	return token;
}

inline internal Token peekTokenCSSProperty(Tokenizer* tokenizer) {
	TokenizerState prevState = tokenizer->state;
	Token token = getTokenCSSProperty(tokenizer);
	tokenizer->state = prevState;
	return token;
}

/*internal Token getTokenCSSSelector(Tokenizer* tokenizer, GetTokenFlags flags = GET_TOKEN_NO_FLAGS)
{
	eatAllWhitespace(tokenizer, flags);

    Token token = initTokenWithTokenizer(tokenizer);

    char C = tokenizer->state.at[0];
    ++tokenizer->state.at;
    switch(C)
    {
		case '\0': {token.type = TOKEN_EOF;} break;
		case '{': {token.type = TOKEN_BRACE_OPEN;} break;
        case '}': {token.type = TOKEN_BRACE_CLOSE;} break;
		case '(': {token.type = TOKEN_PAREN_OPEN;} break;
        case ')': {token.type = TOKEN_PAREN_CLOSE;} break;
		case '[': {token.type = TOKEN_BRACKET_OPEN;} break;
        case ']': {token.type = TOKEN_BRACKET_CLOSE;} break;
		case ',': {token.type = TOKEN_COMMA;} break;
		case '>': {token.type = TOKEN_COND_ABOVE;} break;

		default:
        {
			if(C == ':' || C == '#' || C == '.' || isAlphaUnderscoreDash(C))
			{
				token.type = TOKEN_IDENTIFIER;
                
                while(isNumber(tokenizer->state.at[0]) || isAlphaUnderscoreDash(tokenizer->state.at[0]))
                {
                    ++tokenizer->state.at;
                }
             
				token.length = tokenizer->state.at - token.data;
			}
			else
			{
				token.type = TOKEN_UNKNOWN;
				assert(false);
			}
		}
		break;
	}
	return token;
}

inline internal Token peekTokenCSSSelector(Tokenizer* tokenizer) {
	TokenizerState prevState = tokenizer->state;
	Token token = getTokenCSSSelector(tokenizer);
	tokenizer->state = prevState;
	return token;
}*/

internal bool requireToken(Token token, TokenType desiredType)
{
	bool Result = (token.type == desiredType);
	if (!Result)
	{
		char* s = "[this-is-a-bug]";
		switch (desiredType)
		{
			case TOKEN_BRACE_OPEN: { s = "{"; } break;
			case TOKEN_PAREN_OPEN: { s = "("; } break;
			case TOKEN_EQUAL:	   { s = "="; } break;

			case TOKEN_IDENTIFIER: { s = "identifier"; } break;
			case TOKEN_COLON: { s = ":"; } break;

			default:
				assert(false);
			break;
		}
		lexError(token, "Expected %s, instead got '%*.*s'", s, token.length, token.length, token.data);
	}
    return(Result);
}

internal bool requireToken(Tokenizer* tokenizer, TokenType desiredType)
{
    Token token = getToken(tokenizer);
	return requireToken(token, desiredType);
}

#endif
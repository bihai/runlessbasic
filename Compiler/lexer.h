/***************************************************************************************************
 *
 * RunlessBASIC
 * Copyright 2013 Joshua Hawcroft <dev@joshhawcroft.com>
 *
 * lexer.h
 * (see C source file for details)
 *
 ***************************************************************************************************
 *
 * RunlessBASIC is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * RunlessBASIC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RunlessBASIC.  If not, see <http://www.gnu.org/licenses/>.
 *
 **************************************************************************************************/

#ifndef _LEXER_H
#define _LEXER_H


typedef struct Lexer Lexer;

enum LexerTokenType
{
    TOKEN_UNRECOGNISED,
    TOKEN_PRAGMA,
    TOKEN_IDENTIFIER,
    TOKEN_SPACE,
    TOKEN_NEW_LINE,
    TOKEN_LIT_STRING,
    TOKEN_LIT_INTEGER,
    TOKEN_LIT_REAL,
    TOKEN_LIT_COLOUR,
    TOKEN_IMPLEMENTS,
    TOKEN_INTERFACE,
    TOKEN_PROTECTED,
    TOKEN_INHERITS,
    TOKEN_FUNCTION,
    TOKEN_PRIVATE,
    TOKEN_DECLARE,
    TOKEN_FINALLY,
    TOKEN_INLINEC,
    TOKEN_HANDLER,
    /*TOKEN_OBJECT,*/
    TOKEN_STATIC,
    TOKEN_SHARED,
    TOKEN_PUBLIC,
    TOKEN_DOWNTO,
    TOKEN_RETURN,
    TOKEN_SELECT,
    TOKEN_ELSEIF,
    TOKEN_HASH_ENDIF,
    TOKEN_WHILE,
    TOKEN_BYREF,
    TOKEN_BYVAL,
    TOKEN_UNTIL,
    TOKEN_FALSE,
    TOKEN_SUPER,
    TOKEN_EVENT,
    TOKEN_CATCH,
    TOKEN_RAISE,
    TOKEN_REDIM,
    TOKEN_CLASS,
    TOKEN_HASH_ELSE,
    TOKEN_STEP,
    TOKEN_LOOP,
    TOKEN_THEN,
    TOKEN_CASE,
    TOKEN_WEND,
    TOKEN_ELSE,
    TOKEN_NEXT,
    TOKEN_SELF,
    TOKEN_GOTO,
    TOKEN_TRUE,
    TOKEN_EACH,
    TOKEN_CALL,
    TOKEN_NULL,
    TOKEN_EXIT,
    TOKEN_CONTINUE,
    TOKEN_MOD,
    TOKEN_HASH_IF,
    TOKEN_ISA,
    TOKEN_FOR,
    TOKEN_SUB,
    TOKEN_LIB,
    TOKEN_NEW,
    TOKEN_REM,
    TOKEN_TRY,
    TOKEN_DIM,
    TOKEN_AND,
    TOKEN_END,
    TOKEN_NOT,
    TOKEN_ME,
    TOKEN_OR,
    TOKEN_TO,
    TOKEN_DO,
    TOKEN_AS,
    TOKEN_IF,
    TOKEN_IS,
    TOKEN_OF,
    TOKEN_IN,
    TOKEN_NOT_EQUAL,
    TOKEN_LESS_EQUAL,
    TOKEN_MORE_EQUAL,
    TOKEN_PAREN_LEFT,
    TOKEN_PAREN_RIGHT,
    TOKEN_COLON,
    TOKEN_PLUS,
    TOKEN_HYPHEN,
    TOKEN_MULTIPLY,
    TOKEN_SLASH,
    TOKEN_BACK_SLASH,
    TOKEN_SQUARE_LEFT,
    TOKEN_SQUARE_RIGHT,
    TOKEN_QUOTE,
    TOKEN_EQUAL,
    TOKEN_LESS,
    TOKEN_MORE,
    TOKEN_DOT,
    TOKEN_COMMA,
    TOKEN_AMP_HEX,
    TOKEN_AMP_OCT,
    TOKEN_AMP_BIN,
    TOKEN_AMP_COLOR,
    TOKEN_AMP_UNICODE,
    TOKEN_CONST,
};


typedef struct Token
{
    enum LexerTokenType     type;
    long                    offset;
    char                    *text;
    union
    {
        long                    integer;
        double                  real;
    }                       value;
} Token;



Lexer* lexer_create(char *inSource);
Token lexer_get(Lexer *in_lexer);
Token lexer_peek(Lexer *in_lexer, int in_how_far);
long lexer_offset(Lexer *in_lexer);


#ifdef DEBUG
void lexer_debug_token(Token in_token);
void lexer_run_tests(void);
#endif


#endif

/*
 * lexer.c
 * BASIC Lexical Analyser
 *
 * (c) 2013 Joshua Hawcroft
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <math.h>

#include "lexer.h"
#include "memory.h"
#include "test.h"


#define MAX_HEX_LENGTH 32
#define MAX_OCT_LENGTH 16
#define MAX_BIN_LENGTH 128
#define MAX_DEC_LENGTH 64

#define TOKEN_BUFFER_SIZE 10
#define AUTOFREE_QUEUE 10


struct Lexer
{
    char                *source;
    char                *source_offset;
    Boolean             last_was_text;
    char                *old_source_offset;
    long                line_number;
    Boolean             textize_known;
    Token               buffer[TOKEN_BUFFER_SIZE];
    int                 buffer_start;
    char                *autofree_list[AUTOFREE_QUEUE];
    int                 autofree_index;
    long                last_valid_offset;
};


struct KnownToken
{
    const char          *text;
    enum LexerTokenType type;
};


const struct KnownToken known_tokens[] = {
    { "implements", TOKEN_IMPLEMENTS },
    { "interface", TOKEN_INTERFACE },
    { "protected", TOKEN_PROTECTED },
    { "inherits", TOKEN_INHERITS },
    { "function", TOKEN_FUNCTION },
    { "continue", TOKEN_CONTINUE },
    { "private", TOKEN_PRIVATE },
    { "declare", TOKEN_DECLARE },
    { "finally", TOKEN_FINALLY },
    { "inlinec", TOKEN_INLINEC },
    { "handler", TOKEN_HANDLER },
    { "#pragma", TOKEN_PRAGMA },
    /*{ "object", TOKEN_OBJECT },*/
    { "static", TOKEN_STATIC },
    { "shared", TOKEN_SHARED },
    { "public", TOKEN_PUBLIC },
    { "downto", TOKEN_DOWNTO },
    { "return", TOKEN_RETURN },
    { "select", TOKEN_SELECT },
    { "elseif", TOKEN_ELSEIF },
    { "#endif", TOKEN_HASH_ENDIF },
    { "while", TOKEN_WHILE },
    { "byref", TOKEN_BYREF },
    { "byval", TOKEN_BYVAL },
    { "until", TOKEN_UNTIL },
    { "false", TOKEN_FALSE },
    { "super", TOKEN_SUPER },
    { "catch", TOKEN_CATCH },
    { "raise", TOKEN_RAISE },
    { "redim", TOKEN_REDIM },
    { "class", TOKEN_CLASS },
    { "event", TOKEN_EVENT },
    { "#else", TOKEN_HASH_ELSE },
    { "const", TOKEN_CONST },
    { "step", TOKEN_STEP },
    { "loop", TOKEN_LOOP },
    { "exit", TOKEN_EXIT },
    { "then", TOKEN_THEN },
    { "case", TOKEN_CASE },
    { "wend", TOKEN_WEND },
    { "else", TOKEN_ELSE },
    { "next", TOKEN_NEXT },
    { "self", TOKEN_SELF },
    { "goto", TOKEN_GOTO },
    { "true", TOKEN_TRUE },
    { "each", TOKEN_EACH },
    { "call", TOKEN_CALL },
    { "null", TOKEN_NULL },
    { "mod", TOKEN_MOD },
    { "#if", TOKEN_HASH_IF },
    { "isa", TOKEN_ISA },
    { "for", TOKEN_FOR },
    { "sub", TOKEN_SUB },
    { "lib", TOKEN_LIB },
    { "new", TOKEN_NEW },
    { "rem", TOKEN_REM },
    { "try", TOKEN_TRY },
    { "dim", TOKEN_DIM },
    { "and", TOKEN_AND },
    { "end", TOKEN_END },
    { "not", TOKEN_NOT },
    { "me", TOKEN_ME },
    { "to", TOKEN_TO },
    { "do", TOKEN_DO },
    { "or", TOKEN_OR },
    { "as", TOKEN_AS },
    { "if", TOKEN_IF },
    { "is", TOKEN_IS },
    { "of", TOKEN_OF },
    { "in", TOKEN_IN },
    { "<>", TOKEN_NOT_EQUAL },
    { "<=", TOKEN_LESS_EQUAL },
    { ">=", TOKEN_MORE_EQUAL },
    { "&h", TOKEN_AMP_HEX },
    { "&o", TOKEN_AMP_OCT },
    { "&b", TOKEN_AMP_BIN },
    { "&c", TOKEN_AMP_COLOR },
    { "&u", TOKEN_AMP_UNICODE },
    { "//", TOKEN_REM },
    { "\r\n", TOKEN_NEW_LINE },
    { "(", TOKEN_PAREN_LEFT },
    { ")", TOKEN_PAREN_RIGHT },
    { ":", TOKEN_COLON },
    { "+", TOKEN_PLUS },
    { "-", TOKEN_HYPHEN },
    { "*", TOKEN_MULTIPLY },
    { "/", TOKEN_SLASH },
    { "\\", TOKEN_BACK_SLASH },
    { "[", TOKEN_SQUARE_LEFT },
    { "]", TOKEN_SQUARE_RIGHT },
    { "'", TOKEN_REM },
    { "\"", TOKEN_QUOTE },
    { "=", TOKEN_EQUAL },
    { "<", TOKEN_LESS },
    { ">", TOKEN_MORE },
    { ".", TOKEN_DOT },
    { ",", TOKEN_COMMA },
    { " ", TOKEN_SPACE },
    { "\t", TOKEN_SPACE },
    { "\n", TOKEN_NEW_LINE },
    { "\r", TOKEN_NEW_LINE },
    0, 0,
};


static void _lexer_fill_buffer(Lexer *in_lexer);


static Lexer* _lexer_create(char *inSource, Boolean enable_lookahead)
{
    assert(inSource != NULL);
    
    Lexer *outLexer;
    int i;
    
    outLexer = safe_malloc(sizeof(struct Lexer));
    
    outLexer->last_valid_offset = 0;
    outLexer->source = inSource;
    outLexer->source_offset = inSource;
    outLexer->last_was_text = False;
    outLexer->old_source_offset = NULL;
    outLexer->line_number = 1;
    outLexer->textize_known = False;
    outLexer->autofree_index = 0;
    for (i = 0; i < AUTOFREE_QUEUE; i++)
        outLexer->autofree_list[i] = NULL;
    
    if (enable_lookahead)
        _lexer_fill_buffer(outLexer);
    
    return outLexer;
}


Lexer* lexer_create(char *in_source)
{
    return _lexer_create(in_source, True);
}


static char* _lexer_grab_text(const char *inSource, long inLength)
{
    assert(inSource != NULL);
    assert(inLength >= 0);
    
#if DEBUG
    long source_len;
    source_len = strlen(inSource);
    assert(inLength <= source_len);
#endif
    
    char *out_text;
    out_text = safe_malloc(inLength + 1);
    memcpy(out_text, inSource, inLength);
    out_text[inLength] = 0;
    return out_text;
}


static Boolean _lexer_is_hex_char(char in_char)
{
    switch (in_char)
    {
        case 'A':
        case 'a':
        case 'B':
        case 'b':
        case 'C':
        case 'c':
        case 'D':
        case 'd':
        case 'E':
        case 'e':
        case 'F':
        case 'f':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            return True;
    }
    return False;
}


static Boolean _lexer_is_oct_char(char in_char)
{
    switch (in_char)
    {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
            return True;
    }
    return False;
}


static Boolean _lexer_is_bin_char(char in_char)
{
    switch (in_char)
    {
        case '0':
        case '1':
            return True;
    }
    return False;
}


static const char* _lexer_get_dec(Lexer *inLexer, Boolean *out_is_real)
{
    assert(inLexer);
    assert(inLexer->source_offset);
    assert(out_is_real);
    
    long c, len;
    static char dec[MAX_DEC_LENGTH + 1];
    
    len = 0;
    *out_is_real = False;
    
    for (c = 0; ( (inLexer->source_offset[c]) && isdigit(inLexer->source_offset[c])  &&
                 (c < MAX_DEC_LENGTH) && (inLexer->source_offset[c] != '.') &&
                 (inLexer->source_offset[c] != 'e') && (inLexer->source_offset[c] != 'E') ); c++)
        dec[len++] = inLexer->source_offset[c];
    if (inLexer->source_offset[c] == '.')
    {
        c++;
        dec[len++] = '.';
        *out_is_real = True;
        for (; ( (inLexer->source_offset[c]) && isdigit(inLexer->source_offset[c])  &&
                     (c < MAX_DEC_LENGTH) && (inLexer->source_offset[c] != '.') &&
                     (inLexer->source_offset[c] != 'e') && (inLexer->source_offset[c] != 'E') ); c++)
            dec[len++] = inLexer->source_offset[c];
    }
    if ((inLexer->source_offset[c] == 'e') || (inLexer->source_offset[c] == 'E'))
    {
        *out_is_real = True;
        c++;
        dec[len++] = 'e';
        if (inLexer->source_offset[c] == '-')
        {
            c++;
            dec[len++] = '-';
        }
        for (; ( (inLexer->source_offset[c]) && isdigit(inLexer->source_offset[c])  &&
                     (c < MAX_DEC_LENGTH) && (inLexer->source_offset[c] != '.') &&
                     (inLexer->source_offset[c] != 'e') && (inLexer->source_offset[c] != 'E') ); c++)
            dec[len++] = inLexer->source_offset[c];
    }
    dec[len] = 0;
        
    inLexer->source_offset += len;
    inLexer->last_was_text = False;
    
    return dec;
}


static const char* _lexer_get_hex(Lexer *inLexer, int inLimitLength)
{
    assert(inLexer);
    assert(inLexer->source_offset);
    assert(inLimitLength <= MAX_HEX_LENGTH);
    assert(inLimitLength >= -1);
    
    long c, len;
    static char hex[MAX_HEX_LENGTH + 1];
    
    if (inLimitLength < 0) inLimitLength = MAX_HEX_LENGTH;
    len = 0;
    
    for (c = 0; ( (inLexer->source_offset[c]) && _lexer_is_hex_char(inLexer->source_offset[c]) && (c < inLimitLength) ); c++)
    {
        hex[len++] = inLexer->source_offset[c];
    }
    hex[len] = 0;
    
    inLexer->source_offset += len;
    inLexer->last_was_text = False;
    
    return hex;
}


static const char* _lexer_get_oct(Lexer *inLexer)
{
    assert(inLexer);
    assert(inLexer->source_offset);
    
    long c, len;
    static char oct[MAX_OCT_LENGTH + 1];
    
    len = 0;
    
    for (c = 0; ( (inLexer->source_offset[c]) && _lexer_is_oct_char(inLexer->source_offset[c]) && (c < MAX_OCT_LENGTH) ); c++)
    {
        oct[len++] = inLexer->source_offset[c];
    }
    oct[len] = 0;
    
    inLexer->source_offset += len;
    inLexer->last_was_text = False;
    
    return oct;
}


static const char* _lexer_get_bin(Lexer *inLexer)
{
    assert(inLexer);
    assert(inLexer->source_offset);
    
    long c, len;
    static char bin[MAX_BIN_LENGTH + 1];
    
    len = 0;
    
    for (c = 0; ( (inLexer->source_offset[c]) && _lexer_is_bin_char(inLexer->source_offset[c]) && (c < MAX_BIN_LENGTH) ); c++)
    {
        bin[len++] = inLexer->source_offset[c];
    }
    bin[len] = 0;
    
    inLexer->source_offset += len;
    inLexer->last_was_text = False;
    
    return bin;
}


static Token _lexer_get_token(Lexer *inLexer)
{
    assert(inLexer);
    assert(inLexer->source_offset);
    
    const struct KnownToken *known;
    const char *token;
    char *source_start;
    long len_token, c;
    Token result;
    Boolean is_text_token;
    
    source_start = inLexer->source_offset;
    inLexer->old_source_offset = source_start;
    
    /* iterate over the characters in the source */
    while (*(inLexer->source_offset))
    {        
        /* iterate over list of recognised tokens */
        known = known_tokens;
        while ((token = known->text))
        {
            len_token = strlen(token);
            is_text_token = isalpha(token[0]);
            
            if (! (is_text_token && (inLexer->last_was_text || (inLexer->source_offset != source_start))) )
            {
                /* compare characters of known token and input text */
                for (c = 0; (
                             (c < len_token) &&
                             (inLexer->source_offset[c] != 0) &&
                             tolower(inLexer->source_offset[c]) == token[c]
                             ); c++)
                {
                    if ( (c+1 == len_token) &&
                        (
                            ( is_text_token && (!isalnum(inLexer->source_offset[c+1])) ) ||
                            (! is_text_token)
                        ) )
                    {
                        /* found a matching token */
                        
                        if (inLexer->source_offset != source_start)
                        {
                            /* got a text token first */
                            result.type = TOKEN_UNRECOGNISED;
                            result.offset = source_start - inLexer->source;
                            result.text = _lexer_grab_text(source_start, inLexer->source_offset - source_start);
                            
                            inLexer->last_was_text = True;
                            
                            return result;
                        }
                        
                        result.type = known->type;
                        result.offset = inLexer->source_offset - inLexer->source;
                        if (inLexer->textize_known)
                            result.text = _lexer_grab_text(inLexer->source_offset, len_token);
                        else
                            result.text = NULL;
                        inLexer->source_offset += len_token;
                        
                        inLexer->last_was_text = False;
                        
                        if (result.type == TOKEN_NEW_LINE) inLexer->line_number++;
                        
                        return result;
                    }
                }
            
            }
            known++;
        }
        
        /* no matching known token */
        inLexer->source_offset++;
    }
    
    /* got the last text token */
    if (inLexer->source_offset != source_start)
    {
        result.type = TOKEN_UNRECOGNISED;
        result.offset = source_start - inLexer->source;
        result.text = _lexer_grab_text(source_start, inLexer->source_offset - source_start);
        return result;
    }
    
    /* couldn't find another token */
    result.type = TOKEN_UNRECOGNISED;
    result.text = NULL;
    result.offset = -1;
    return result;
}


static void _lexer_append_string(char **inString1, const char *inString2)
{
    assert(inString1);
    assert(inString2);
    
    long len;
    if (*inString1) len = strlen(*inString1);
    else len = 0;
    *inString1 = safe_realloc(*inString1, len + strlen(inString2) + 1);
    strcpy(*inString1 + len, inString2);
}


static Boolean _lexer_is_valid_identifier(const char *inText)
{
    assert(inText);
    
    long i, len;
    len = strlen(inText);
    if ((len == 0) || (len > 1000)) return False;
    if (! ((inText[0] == '_') || isalpha(inText[0]) || (inText[0] > 127) || (inText[0] < 0)) ) return False;
    for (i = 1; i < len; i++)
        if (! ((inText[i] == '_') || isalnum(inText[i]) || (inText[i] > 127) || (inText[i] < 0)) ) return False;
    return True;
}


static const char* _lexer_encode_unicode_char(long inCodePoint)
{
#warning "_lexer_encode_unicode_char() is not properly implemented (use a Unicode library)"
    static char short_string[2];
    short_string[0] = inCodePoint;
    short_string[1] = 0;
    return short_string;
}


static Token _lexer_get_next_token(Lexer *inLexer)
{
    Token token, another_token, yet_another_token;
    const char *temp_string;
    Boolean is_real;
    
    /* skip whitespace */
    for (;;)
    {
        token = _lexer_get_token(inLexer);
        if (token.offset < 0) return token;
        if (token.type != TOKEN_SPACE) break;
    }
    
    /* handle specific cases */
    switch (token.type)
    {
        case TOKEN_QUOTE:
            /* string literal */
            token.type = TOKEN_LIT_STRING;
            inLexer->textize_known = True;
            for (;;)
            {
                another_token = _lexer_get_token(inLexer);
                if (another_token.offset < 0)
                {
                    /* found end of stream before end of string; return a single quote */
                    if (token.text) free(token.text);
                    token.text = NULL;
                    token.type = TOKEN_QUOTE;
                    break;
                }
                if (another_token.type == TOKEN_QUOTE)
                {
                    yet_another_token = _lexer_get_token(inLexer);
                    if (yet_another_token.type == TOKEN_QUOTE)
                    {
                        /* string contains a quote */
                        _lexer_append_string(&(token.text), "\"");
                        continue;
                    }
                    else
                    {
                        /* end of string has been reached */
                        inLexer->source_offset = inLexer->old_source_offset;
                        break;
                    }
                }
                
                if (another_token.type == TOKEN_AMP_UNICODE)
                {
                    /* unicode code point; followed by precisely 4-hexadecimal characters */
                    temp_string = _lexer_get_hex(inLexer, 4);
                    //printf("Hex: %ld\n", strtol(temp_string, NULL, 16));
                    _lexer_append_string(&(token.text),
                                         _lexer_encode_unicode_char( strtol(temp_string, NULL, 16) )
                                         );
                    
                    continue;
                }
                
                if (another_token.text)
                {
                    _lexer_append_string(&(token.text), another_token.text);
                    free(another_token.text);
                }
            }
            inLexer->textize_known = False;
            break;
            
        case TOKEN_REM:
            /* line comment */
            inLexer->textize_known = True;
            for (;;)
            {
                another_token = _lexer_get_token(inLexer);
                if ((another_token.type == TOKEN_NEW_LINE) || (another_token.offset < 0))
                {
                    inLexer->source_offset = inLexer->old_source_offset;
                    break;
                }
                
                if (another_token.text)
                {
                    _lexer_append_string(&(token.text), another_token.text);
                    free(another_token.text);
                }
            }
            inLexer->textize_known = False;
            break;
        
            /* hexadecimal, binary or octal numeric integer literal
             (any non-hex character signifies the end) */
        case TOKEN_AMP_BIN:
            token.type = TOKEN_LIT_INTEGER;
            temp_string = _lexer_get_bin(inLexer);
            token.value.integer = strtol(temp_string, NULL, 2);
            //printf("Bin: %ld (%s)\n", token.value_integer, temp_string);
            break;
        case TOKEN_AMP_HEX:
            token.type = TOKEN_LIT_INTEGER;
            temp_string = _lexer_get_hex(inLexer, -1);
            token.value.integer = strtol(temp_string, NULL, 16);
            //printf("Hex: %ld (%s)\n", token.value_integer, temp_string);
            break;
        case TOKEN_AMP_OCT:
            token.type = TOKEN_LIT_INTEGER;
            temp_string = _lexer_get_oct(inLexer);
            token.value.integer = strtol(temp_string, NULL, 8);
            //printf("Oct: %ld (%s)\n", token.value_integer, temp_string);
            break;
        
        case TOKEN_AMP_COLOR:
            /* colour literal; &cRRGGBBAA (any non-hex character signifies the end) */
            token.type = TOKEN_LIT_COLOUR;
            temp_string = _lexer_get_hex(inLexer, 8);
            token.value.integer = strtol(temp_string, NULL, 16);
            //printf("Color: %ld (%s)\n", token.value_integer, temp_string);
            break;
            
        default:
            /* identifier or decimal numeric literal;
             [negation (-) operators accumulate so there's no need to deal with them here] */
            if ( (token.text) && isdigit(token.text[0]) )
            {
                /* got a numeric literal */
                free(token.text);
                token.text = NULL;
                inLexer->source_offset = inLexer->old_source_offset;
                temp_string = _lexer_get_dec(inLexer, &is_real);
                if (!is_real)
                {
                    token.type = TOKEN_LIT_INTEGER;
                    token.value.integer = strtol(temp_string, NULL, 10);
                    //printf("Int: %ld\n", token.value.integer);
                }
                else
                {
                    token.type = TOKEN_LIT_REAL;
                    token.value.real = strtof(temp_string, NULL);
                    //printf("Real: %lf (%s)\n", token.value.real, temp_string);
                }
            }
            else if (token.type == TOKEN_UNRECOGNISED)
            {
                /* got an identifier; validate it */
                if (token.text && _lexer_is_valid_identifier(token.text))
                    token.type = TOKEN_IDENTIFIER;
            }
            break;
    }
    
    return token;
}


static void _lexer_fill_buffer(Lexer *in_lexer)
{
    assert(in_lexer);
    
    int i;
    Token token;
    
    for (i = 0; i < TOKEN_BUFFER_SIZE; i++)
    {
        in_lexer->buffer[i].offset = -1;
        in_lexer->buffer[i].type = TOKEN_UNRECOGNISED;
        in_lexer->buffer[i].text = NULL;
    }
    
    in_lexer->buffer_start = 0;
    for (i = 0; i < TOKEN_BUFFER_SIZE; i++)
    {
        token = _lexer_get_next_token(in_lexer);
        if (token.offset < 0) break;
        in_lexer->buffer[i] = token;
    }
}


static void _lexer_autofree(Lexer *in_lexer, char *in_string)
{
    assert(in_lexer);

    /* free string on autofree list */
    if (in_lexer->autofree_list[ in_lexer->autofree_index ])
        safe_free(in_lexer->autofree_list[ in_lexer->autofree_index ]);
    in_lexer->autofree_list[ in_lexer->autofree_index ] = NULL;
    
    if (in_string) 
    {
        /* add this string to the autofree list */
        in_lexer->autofree_list[ in_lexer->autofree_index ] = in_string;
    }
    
    /* continue to work through the queue */
    in_lexer->autofree_index++;
    if (in_lexer->autofree_index >= AUTOFREE_QUEUE)
        in_lexer->autofree_index = 0;
}


Token lexer_get(Lexer *in_lexer)
{
    Token token;
    
    assert(in_lexer);
    
    token = in_lexer->buffer[ in_lexer->buffer_start ];
    _lexer_autofree(in_lexer, token.text);
    
    in_lexer->buffer[ in_lexer->buffer_start ] = _lexer_get_next_token(in_lexer);
    
    in_lexer->buffer_start++;
    if (in_lexer->buffer_start >= TOKEN_BUFFER_SIZE)
        in_lexer->buffer_start = 0;
    
    if (token.offset > 0)
        in_lexer->last_valid_offset = token.offset;
    
    return token;
}


Token lexer_peek(Lexer *in_lexer, int in_how_far)
{
    assert(in_lexer);
    assert( (in_how_far >= 0) || (in_how_far < TOKEN_BUFFER_SIZE-1) );
    
    int index;
    
    index = in_lexer->buffer_start + in_how_far;
    if (index >= TOKEN_BUFFER_SIZE)
        index -= TOKEN_BUFFER_SIZE;
    
    if (in_lexer->buffer[index].offset > 0)
        in_lexer->last_valid_offset = in_lexer->buffer[index].offset;
    
    return in_lexer->buffer[index];
}


long lexer_offset(Lexer *in_lexer)
{
    return in_lexer->last_valid_offset;
}



#ifdef DEBUG

static const char* lexer_debug_token_type(enum LexerTokenType in_type)
{
    const struct KnownToken *known;
    
    switch (in_type)
    {
        case TOKEN_NEW_LINE:
            return "<newline>";
        case TOKEN_IDENTIFIER:
            return "<identifier>";
        case TOKEN_SPACE:
            return "<space>";
        case TOKEN_LIT_STRING:
            return "<literal-string>";
        case TOKEN_LIT_INTEGER:
            return "<literal-integer>";
        case TOKEN_LIT_REAL:
            return "<literal-real>";
        case TOKEN_LIT_COLOUR:
            return "<literal-colour>";
        default: break;
    }
    
    known = known_tokens;
    
    while (known->text)
    {
        if (known->type == in_type)
            return known->text;
        
        known++;
    }
    
    return NULL;
}


void lexer_debug_token(Token in_token)
{
    if (in_token.offset < 0)
    {
        printf("END OF SOURCE\n");
        return;
    }
    printf("%s", lexer_debug_token_type(in_token.type));
    if (in_token.text)
        printf(" \"%s\"", in_token.text);
    if ((in_token.type == TOKEN_LIT_INTEGER) || (in_token.type == TOKEN_LIT_COLOUR))
        printf(" %ld", in_token.value.integer);
    else if (in_token.type == TOKEN_LIT_REAL)
        printf(" %lf", in_token.value.real);
    printf("\n");
}


#endif



/*
 * Tests
 */

#ifdef DEBUG




/* checks that the hard-coded list of known tokens is correctly formatted:
 i)  uses lowercase letters
 ii) is sorted with the longest tokens at the top */
static const char* test_1(void)
{
    const struct KnownToken *known;
    long last_length, this_length, c;
    
    known = known_tokens;
    last_length = 0;
    
    while (known->text)
    {
        this_length = strlen(known->text);
        if ((this_length > last_length) && (last_length != 0))
            return "known_tokens[] must be manually sorted from longest to shortest token.";
        for (c = 0; c < this_length; c++)
            if (isupper(known->text[c]))
                return "known_tokens[] must only contain lowercase characters.";
        last_length = this_length;
        
        known++;
    }
    return NULL;
}


/* checks lexer_create() */
static const char* test_2(void)
{
    Lexer *lexer;
    char *source = "This is a test";
    
    lexer = lexer_create(source);
    CHECK(lexer);
    CHECK(!lexer->last_was_text);
    CHECK(lexer->source == source);
    CHECK(lexer->textize_known == False);
    CHECK(lexer->line_number == 1);
    
    return NULL;
}


/* checks _lexer_grab_text(const char *inSource, long inLength) */
static const char* test_3(void)
{
    char *source = "This is a test";
    char *result;
    
    result = _lexer_grab_text(source, 4);
    CHECK(result);
    CHECK(memcmp(result, source, 4) == 0);
    CHECK(result != source);
    
    result = _lexer_grab_text(source + 8, 6);
    CHECK(result);
    CHECK(memcmp(result, source + 8, 4) == 0);
    
    result = _lexer_grab_text(source, 0);
    CHECK(result);
    CHECK(result[0] == 0);
    
    return NULL;
}


/* check static Boolean _lexer_is_hex_char(char in_char) */
static const char* test_4(void)
{
    char hex[] = {'A','a','B','b','C','c','D','d','E','e','F','f','0','1','2','3','4','5','6','7','8','9',0};
    int i;
    for (i = 0; hex[i] != 0; i++)
        CHECK(_lexer_is_hex_char(hex[i]));
    for (i = 0; i <= 47; i++) /* null -> / */
        CHECK(!_lexer_is_hex_char(i));
    for (i = 58; i <= 64; i++) /* : -> @ */
        CHECK(!_lexer_is_hex_char(i));
    for (i = 71; i <= 96; i++) /* G -> ` */
        CHECK(!_lexer_is_hex_char(i));
    for (i = 103; i <= 255; i++) /* g -> undefined */
        CHECK(!_lexer_is_hex_char(i));
    for (i = -128; i <= -1; i++) /* 'extended ASCII' */
        CHECK(!_lexer_is_hex_char(i));
    return NULL;
}


/* static Boolean _lexer_is_oct_char(char in_char) */
static const char* test_5(void)
{
    int i;
    for (i = 48; i <= 55; i++) /* 0 -> 7 */
        CHECK(_lexer_is_oct_char(i));
    for (i = 0; i <= 47; i++) /* null -> / */
        CHECK(!_lexer_is_oct_char(i));
    for (i = 56; i <= 255; i++) /* 8 -> undefined */
        CHECK(!_lexer_is_oct_char(i));
    for (i = -128; i <= -1; i++) /* 'extended ASCII' */
        CHECK(!_lexer_is_oct_char(i));
    return NULL;
}


/* static Boolean _lexer_is_bin_char(char in_char) */
static const char* test_6(void)
{
    int i;
    for (i = 48; i <= 49; i++) /* 0 -> 1 */
        CHECK(_lexer_is_bin_char(i));
    for (i = 0; i <= 47; i++) /* null -> / */
        CHECK(!_lexer_is_bin_char(i));
    for (i = 50; i <= 255; i++) /* 2 -> undefined */
        CHECK(!_lexer_is_bin_char(i));
    for (i = -128; i <= -1; i++) /* 'extended ASCII' */
        CHECK(!_lexer_is_bin_char(i));
    return NULL;
}


/* static const char* _lexer_get_dec(Lexer *inLexer, Boolean *out_is_real) */
static const char* test_7(void)
{
    Lexer *lexer;
    const char *dec;
    Boolean is_real;
    
    lexer = _lexer_create("", False);
    CHECK(lexer);
    dec = _lexer_get_dec(lexer, &is_real);
    CHECK(dec);
    CHECK(dec[0] == 0);
    CHECK(!is_real);
    
    lexer = _lexer_create("8", False);
    CHECK(lexer);
    dec = _lexer_get_dec(lexer, &is_real);
    CHECK(dec);
    CHECK(strcmp(dec, "8") == 0);
    CHECK(!is_real);
    
    lexer = _lexer_create("12.3e5 + 2", False);
    CHECK(lexer);
    dec = _lexer_get_dec(lexer, &is_real);
    CHECK(dec);
    CHECK(strcmp(dec, "12.3e5") == 0);
    CHECK(is_real);
    
    lexer = _lexer_create("3e10 + 2", False);
    CHECK(lexer);
    dec = _lexer_get_dec(lexer, &is_real);
    CHECK(dec);
    CHECK(strcmp(dec, "3e10") == 0);
    CHECK(is_real);
    
    lexer = _lexer_create("24e2 + 2", False);
    CHECK(lexer);
    dec = _lexer_get_dec(lexer, &is_real);
    CHECK(dec);
    CHECK(strcmp(dec, "24e2") == 0);
    CHECK(is_real);
    
    lexer = _lexer_create("12.3e-17 + 2", False);
    CHECK(lexer);
    dec = _lexer_get_dec(lexer, &is_real);
    CHECK(dec);
    CHECK(strcmp(dec, "12.3e-17") == 0);
    CHECK(is_real);
    
    lexer = _lexer_create("0234.317 + 2", False);
    CHECK(lexer);
    dec = _lexer_get_dec(lexer, &is_real);
    CHECK(dec);
    CHECK(strcmp(dec, "0234.317") == 0);
    CHECK(is_real);
    
    lexer = _lexer_create("42 + 2", False);
    CHECK(lexer);
    dec = _lexer_get_dec(lexer, &is_real);
    CHECK(dec);
    CHECK(strcmp(dec, "42") == 0);
    CHECK(!is_real);
    
    lexer = _lexer_create("0 + 2", False);
    CHECK(lexer);
    dec = _lexer_get_dec(lexer, &is_real);
    CHECK(dec);
    CHECK(strcmp(dec, "0") == 0);
    CHECK(!is_real);
    
    lexer = _lexer_create(".2 + 2", False);
    CHECK(lexer);
    dec = _lexer_get_dec(lexer, &is_real);
    CHECK(dec);
    CHECK(strcmp(dec, ".2") == 0);
    CHECK(is_real);
    
    lexer = _lexer_create("z12.3e5 + 2", False);
    CHECK(lexer);
    dec = _lexer_get_dec(lexer, &is_real);
    CHECK(dec);
    CHECK(dec[0] == 0);
    CHECK(!is_real);
    
    lexer = _lexer_create("A0 + 2", False);
    CHECK(lexer);
    dec = _lexer_get_dec(lexer, &is_real);
    CHECK(dec);
    CHECK(dec[0] == 0);
    CHECK(!is_real);
    
    lexer = _lexer_create("9..1 + 2", False);
    CHECK(lexer);
    dec = _lexer_get_dec(lexer, &is_real);
    CHECK(dec);
    CHECK(strcmp(dec, "9.") == 0);
    CHECK(is_real);
    
    lexer = _lexer_create("-5 + 2", False);
    CHECK(lexer);
    dec = _lexer_get_dec(lexer, &is_real);
    CHECK(dec);
    CHECK(dec[0] == 0);
    CHECK(!is_real);
    
    lexer = _lexer_create("9ee + 2", False);
    CHECK(lexer);
    dec = _lexer_get_dec(lexer, &is_real);
    CHECK(dec);
    CHECK(strcmp(dec, "9e") == 0);
    CHECK(is_real);
    
    return NULL;
}


/* static const char* _lexer_get_hex(Lexer *inLexer, int inLimitLength) */
static const char* test_8(void)
{
    Lexer *lexer;
    const char *hex;
    
    lexer = _lexer_create("", False);
    CHECK(lexer);
    hex = _lexer_get_hex(lexer, -1);
    CHECK(hex);
    CHECK(hex[0] == 0);
    
    lexer = _lexer_create("Z0f8", False);
    CHECK(lexer);
    hex = _lexer_get_hex(lexer, 0);
    CHECK(hex);
    CHECK(hex[0] == 0);
    
    lexer = _lexer_create("Z0f8", False);
    CHECK(lexer);
    hex = _lexer_get_hex(lexer, -1);
    CHECK(hex);
    CHECK(hex[0] == 0);
    
    lexer = _lexer_create("0f8", False);
    CHECK(lexer);
    hex = _lexer_get_hex(lexer, -1);
    CHECK(hex);
    CHECK(strcmp(hex, "0f8") == 0);
    
    lexer = _lexer_create("0f8z", False);
    CHECK(lexer);
    hex = _lexer_get_hex(lexer, -1);
    CHECK(hex);
    CHECK(strcmp(hex, "0f8") == 0);
    
    lexer = _lexer_create("9", False);
    CHECK(lexer);
    hex = _lexer_get_hex(lexer, -1);
    CHECK(hex);
    CHECK(strcmp(hex, "9") == 0);
    
    lexer = _lexer_create("F.E42=Pods", False);
    CHECK(lexer);
    hex = _lexer_get_hex(lexer, -1);
    CHECK(hex);
    CHECK(strcmp(hex, "F") == 0);
    
    lexer = _lexer_create("F-E42=Pods", False);
    CHECK(lexer);
    hex = _lexer_get_hex(lexer, -1);
    CHECK(hex);
    CHECK(strcmp(hex, "F") == 0);
    
    lexer = _lexer_create("F00D15600D41 ", False);
    CHECK(lexer);
    hex = _lexer_get_hex(lexer, -1);
    CHECK(hex);
    CHECK(strcmp(hex, "F00D15600D41") == 0);
    
    lexer = _lexer_create("F00D156piano00D41 ", False);
    CHECK(lexer);
    hex = _lexer_get_hex(lexer, 4);
    CHECK(hex);
    CHECK(strcmp(hex, "F00D") == 0);
    
    return NULL;
}


/* static const char* _lexer_get_oct(Lexer *inLexer) */
static const char* test_9(void)
{
    Lexer *lexer;
    const char *hex;
    
    lexer = _lexer_create("", False);
    CHECK(lexer);
    hex = _lexer_get_oct(lexer);
    CHECK(hex);
    CHECK(hex[0] == 0);
    
    lexer = _lexer_create("428", False);
    CHECK(lexer);
    hex = _lexer_get_oct(lexer);
    CHECK(hex);
    CHECK(strcmp(hex, "42") == 0);
    
    lexer = _lexer_create("42.", False);
    CHECK(lexer);
    hex = _lexer_get_oct(lexer);
    CHECK(hex);
    CHECK(strcmp(hex, "42") == 0);
    
    lexer = _lexer_create("0e-", False);
    CHECK(lexer);
    hex = _lexer_get_oct(lexer);
    CHECK(hex);
    CHECK(strcmp(hex, "0") == 0);
    
    lexer = _lexer_create("42-", False);
    CHECK(lexer);
    hex = _lexer_get_oct(lexer);
    CHECK(hex);
    CHECK(strcmp(hex, "42") == 0);
    
    lexer = _lexer_create("42z", False);
    CHECK(lexer);
    hex = _lexer_get_oct(lexer);
    CHECK(hex);
    CHECK(strcmp(hex, "42") == 0);
    
    lexer = _lexer_create("9", False);
    CHECK(lexer);
    hex = _lexer_get_oct(lexer);
    CHECK(hex);
    CHECK(strcmp(hex, "") == 0);
    
    return NULL;
}


/* static const char* _lexer_get_bin(Lexer *inLexer) */
static const char* test_10(void)
{
    Lexer *lexer;
    const char *bin;
    
    lexer = _lexer_create("", False);
    CHECK(lexer);
    bin = _lexer_get_bin(lexer);
    CHECK(bin);
    CHECK(bin[0] == 0);
    
    lexer = _lexer_create("208", False);
    CHECK(lexer);
    bin = _lexer_get_bin(lexer);
    CHECK(bin);
    CHECK(bin[0] == 0);
    
    lexer = _lexer_create("108", False);
    CHECK(lexer);
    bin = _lexer_get_bin(lexer);
    CHECK(bin);
    CHECK(strcmp(bin, "10") == 0);
    
    lexer = _lexer_create("01.", False);
    CHECK(lexer);
    bin = _lexer_get_bin(lexer);
    CHECK(bin);
    CHECK(strcmp(bin, "01") == 0);
    
    lexer = _lexer_create("0e-", False);
    CHECK(lexer);
    bin = _lexer_get_bin(lexer);
    CHECK(bin);
    CHECK(strcmp(bin, "0") == 0);
    
    lexer = _lexer_create("10-", False);
    CHECK(lexer);
    bin = _lexer_get_bin(lexer);
    CHECK(bin);
    CHECK(strcmp(bin, "10") == 0);
    
    lexer = _lexer_create("11z", False);
    CHECK(lexer);
    bin = _lexer_get_bin(lexer);
    CHECK(bin);
    CHECK(strcmp(bin, "11") == 0);
    
    lexer = _lexer_create("4", False);
    CHECK(lexer);
    bin = _lexer_get_bin(lexer);
    CHECK(bin);
    CHECK(strcmp(bin, "") == 0);
    
    return NULL;
}


/* check static Token _lexer_get_token(Lexer *inLexer) */
static const char* test_11(void)
{
    Lexer *lexer;
    Token token;
    
    lexer = _lexer_create("DiM name // this is comment\r\n"
                         "name = \"Hello &u0022cruel\"\" world!\"\r"
                         "cOnsT 3.14159\n"
                         "&b1001ANd\t 私はガラ=\"pickle 食べ\"eND", False);
    CHECK(lexer);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_DIM);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_SPACE);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_UNRECOGNISED);
    CHECK(token.text);
    CHECK(strcmp(token.text, "name") == 0);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_SPACE);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_REM);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_SPACE);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_UNRECOGNISED);
    CHECK(token.text);
    CHECK(strcmp(token.text, "this") == 0);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_SPACE);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_IS);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_SPACE);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_UNRECOGNISED);
    CHECK(token.text);
    CHECK(strcmp(token.text, "comment") == 0);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_NEW_LINE);

    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_UNRECOGNISED);
    CHECK(token.text);
    CHECK(strcmp(token.text, "name") == 0);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_SPACE);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_EQUAL);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_SPACE);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_QUOTE);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_UNRECOGNISED);
    CHECK(token.text);
    CHECK(strcmp(token.text, "Hello") == 0);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_SPACE);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_AMP_UNICODE);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_UNRECOGNISED);
    CHECK(token.text);
    CHECK(strcmp(token.text, "0022cruel") == 0);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_QUOTE);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_QUOTE);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_SPACE);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_UNRECOGNISED);
    CHECK(token.text);
    CHECK(strcmp(token.text, "world!") == 0);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_QUOTE);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_NEW_LINE);
    
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_CONST);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_SPACE);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_UNRECOGNISED);
    CHECK(token.text);
    CHECK(strcmp(token.text, "3") == 0);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_DOT);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_UNRECOGNISED);
    CHECK(token.text);
    CHECK(strcmp(token.text, "14159") == 0);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_NEW_LINE);
    
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_AMP_BIN);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_UNRECOGNISED);
    CHECK(token.text);
    CHECK(strcmp(token.text, "1001ANd") == 0);
    
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_SPACE);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_SPACE);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_UNRECOGNISED);
    CHECK(token.text);
    CHECK(strcmp(token.text, "私はガラ") == 0);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_EQUAL);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_QUOTE);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_UNRECOGNISED);
    CHECK(token.text);
    CHECK(strcmp(token.text, "pickle") == 0);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_SPACE);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_UNRECOGNISED);
    CHECK(token.text);
    CHECK(strcmp(token.text, "食べ") == 0);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_QUOTE);
    token = _lexer_get_token(lexer);
    CHECK(token.type == TOKEN_END);
    
    return NULL;
}


/* static void _lexer_append_string(char **inString1, const char *inString2) */
static const char* test_12(void)
{
    char *the_mutating;
    
    the_mutating = NULL;
    
    _lexer_append_string(&the_mutating, "hello");
    CHECK(the_mutating);
    CHECK(strcmp(the_mutating, "hello") == 0);
    
    _lexer_append_string(&the_mutating, " world");
    CHECK(the_mutating);
    CHECK(strcmp(the_mutating, "hello world") == 0);
    
    return NULL;
}


/* static Boolean _lexer_is_valid_identifier(const char *inText) */
static const char* test_13(void)
{
    CHECK(_lexer_is_valid_identifier("_pumpkin"));
    CHECK(!_lexer_is_valid_identifier("2_pumpkin"));
    CHECK(!_lexer_is_valid_identifier("$_pumpkin"));
    CHECK(!_lexer_is_valid_identifier(" _pumpkin"));
    CHECK(!_lexer_is_valid_identifier(""));
    CHECK(!_lexer_is_valid_identifier("iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                      "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                      "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                      "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                      "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                      "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                      "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                      "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                      "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
                                      "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiz"));
    CHECK(_lexer_is_valid_identifier("hello_there"));
    CHECK(!_lexer_is_valid_identifier("hello_$there"));
    CHECK(!_lexer_is_valid_identifier("hello there"));
    CHECK(_lexer_is_valid_identifier("buns4you"));
    CHECK(_lexer_is_valid_identifier("buns4you_"));
    
    CHECK(_lexer_is_valid_identifier("buώσσαyou_"));
    CHECK(_lexer_is_valid_identifier("ہیںou_test"));
    CHECK(_lexer_is_valid_identifier("ہیںou_𣎏世咹_test"));
    CHECK(_lexer_is_valid_identifier("ہیںou_𣎏世咹_5test"));
    CHECK(!_lexer_is_valid_identifier("3ہیںou_𣎏世咹_5test"));
    CHECK(!_lexer_is_valid_identifier("ہیںou_𣎏世咹_5test$"));
    CHECK(_lexer_is_valid_identifier("私はガラ_"));
    
    return NULL;
}


/* static const char* _lexer_encode_unicode_char(long inCodePoint) */
static const char* test_14(void)
{
    const char *result;
    
    result = _lexer_encode_unicode_char(32);
    CHECK(result);
    CHECK(strcmp(result, " ") == 0);
    
    result = _lexer_encode_unicode_char(78);
    CHECK(result);
    CHECK(strcmp(result, "N") == 0);
    
    /* TODO: this needs additional tests once the function is properly fixed to work with Unicode */
    
    return NULL;
}


/* check static Token _lexer_get_next_token(Lexer *inLexer) */
static const char* test_15(void)
{
    Lexer *lexer;
    Token token;
    
    lexer = _lexer_create("Dim x,y As Integer\r\n"
                         "Dim name As String // this is a comment\r\n"
                         "name = \"Hello &u0022cruel\"\" world!\"\r\n"
                         "cOnsT PI = 3.14159\r\n"
                         "29 +  &b1001ANd&o74 &hEF17A3\t "
                         "私はガラ=\"pickle 食べ\"eND", False);
    CHECK(lexer);
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_DIM);
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_IDENTIFIER);
    CHECK(token.text);
    CHECK(strcmp(token.text, "x") == 0);
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_COMMA);
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_IDENTIFIER);
    CHECK(token.text);
    CHECK(strcmp(token.text, "y") == 0);
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_AS);
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_IDENTIFIER);
    CHECK(token.text);
    CHECK(strcmp(token.text, "Integer") == 0);
    
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_NEW_LINE);
    
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_DIM);
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_IDENTIFIER);
    CHECK(token.text);
    CHECK(strcmp(token.text, "name") == 0);
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_AS);
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_IDENTIFIER);
    CHECK(token.text);
    CHECK(strcmp(token.text, "String") == 0);
    
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_REM);
    CHECK(token.text);
    CHECK(strcmp(token.text, " this is a comment") == 0);
    
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_NEW_LINE);
    
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_IDENTIFIER);
    CHECK(token.text);
    CHECK(strcmp(token.text, "name") == 0);
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_EQUAL);
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_LIT_STRING);
    CHECK(token.text);
    CHECK(strcmp(token.text, "Hello \"cruel\" world!") == 0);
    
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_NEW_LINE);
    
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_CONST);
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_IDENTIFIER);
    CHECK(token.text);
    CHECK(strcmp(token.text, "PI") == 0);
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_EQUAL);
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_LIT_REAL);
    CHECK(abs(token.value.real - 3.14159) < 0.001);
    
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_NEW_LINE);
    
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_LIT_INTEGER);
    CHECK(token.value.integer = 29);
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_PLUS);
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_LIT_INTEGER);
    CHECK(token.value.integer = 9);
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_AND);
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_LIT_INTEGER);
    CHECK(token.value.integer = 7 * 8 + 4);
    
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_LIT_INTEGER);
    CHECK(token.value.integer = 0xEF17A3);
    
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_IDENTIFIER);
    CHECK(token.text);
    CHECK(strcmp(token.text, "私はガラ") == 0);
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_EQUAL);
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_LIT_STRING);
    CHECK(token.text);
    CHECK(strcmp(token.text, "pickle 食べ") == 0);
    token = _lexer_get_next_token(lexer);
    CHECK(token.type == TOKEN_END);
    
    return NULL;
}


/* static void _lexer_fill_buffer(Lexer *in_lexer)*/
static const char* test_16()
{
    Lexer *lexer;
    
    lexer = _lexer_create("Dim x,y As Integer\r\n"
                          "Dim name As String // this is a comment\r\n", False);
    
    CHECK(lexer);
    CHECK(TOKEN_BUFFER_SIZE >= 10);
    
    _lexer_fill_buffer(lexer);
    CHECK(lexer->buffer_start == 0);
    
    CHECK(lexer->buffer[0].type == TOKEN_DIM);
    CHECK(lexer->buffer[1].type == TOKEN_IDENTIFIER);
    CHECK(lexer->buffer[2].type == TOKEN_COMMA);
    CHECK(lexer->buffer[3].type == TOKEN_IDENTIFIER);
    CHECK(lexer->buffer[4].type == TOKEN_AS);
    CHECK(lexer->buffer[5].type == TOKEN_IDENTIFIER);
    CHECK(lexer->buffer[6].type == TOKEN_NEW_LINE);
    CHECK(lexer->buffer[7].type == TOKEN_DIM);
    CHECK(lexer->buffer[8].type == TOKEN_IDENTIFIER);
    CHECK(lexer->buffer[9].type == TOKEN_AS);
    
    return NULL;
}


/* static void _lexer_autofree(Lexer *in_lexer, char *in_string) */
static const char* test_17()
{
    Lexer *lexer;
    long old_frees;
    char *str1, *str2, *str3;
    
    lexer = _lexer_create("", False);
    
    CHECK(lexer);
    CHECK(AUTOFREE_QUEUE >= 10);
    
    str1 = safe_malloc(10);
    strcpy(str1, "debugA");
    str2 = safe_malloc(10);
    strcpy(str2, "debugB");
    str3 = safe_malloc(10);
    strcpy(str3, "debugC");
    
    old_frees = debug_memory_frees();
    
    _lexer_autofree(lexer, str1);
    CHECK(debug_memory_frees() == old_frees);
    
    _lexer_autofree(lexer, str2);
    CHECK(debug_memory_frees() == old_frees);
    
    _lexer_autofree(lexer, NULL);
    CHECK(debug_memory_frees() == old_frees);
    
    _lexer_autofree(lexer, str3);
    CHECK(debug_memory_frees() == old_frees);
    
    _lexer_autofree(lexer, NULL);
    CHECK(debug_memory_frees() == old_frees);
    _lexer_autofree(lexer, NULL);
    CHECK(debug_memory_frees() == old_frees);
    _lexer_autofree(lexer, NULL);
    CHECK(debug_memory_frees() == old_frees);
    _lexer_autofree(lexer, NULL);
    CHECK(debug_memory_frees() == old_frees);
    _lexer_autofree(lexer, NULL);
    CHECK(debug_memory_frees() == old_frees);
    _lexer_autofree(lexer, NULL);
    CHECK(debug_memory_frees() == old_frees);
    
    _lexer_autofree(lexer, NULL);
    CHECK(debug_memory_frees() - old_frees == 1);
    CHECK(debug_memory_last_ptr() == str1);
    _lexer_autofree(lexer, NULL);
    CHECK(debug_memory_frees() - old_frees == 2);
    CHECK(debug_memory_last_ptr() == str2);
    _lexer_autofree(lexer, NULL);
    CHECK(debug_memory_frees() - old_frees == 2);
    _lexer_autofree(lexer, NULL);
    CHECK(debug_memory_frees() - old_frees == 3);
    CHECK(debug_memory_last_ptr() == str3);
    _lexer_autofree(lexer, NULL);
    CHECK(debug_memory_frees() - old_frees == 3);
    
    return NULL;
}


/* Token lexer_get(Lexer *in_lexer)*/
static const char* test_18()
{
    Lexer *lexer;
    Token token;
    
    lexer = lexer_create("29 +  &b1001ANd&o74 &hEF17A3\t "
                         "私はガラ=\"pickle 食べ\"");
    CHECK(lexer);
    
    token = lexer_get(lexer);
    CHECK(token.type == TOKEN_LIT_INTEGER);
    CHECK(lexer->buffer_start == 1);
    CHECK(lexer->buffer[1].type == TOKEN_PLUS);
    CHECK(lexer->buffer[2].type == TOKEN_LIT_INTEGER);
    CHECK(lexer->buffer[3].type == TOKEN_AND);
    CHECK(lexer->buffer[4].type == TOKEN_LIT_INTEGER);
    CHECK(lexer->buffer[5].type == TOKEN_LIT_INTEGER);
    CHECK(lexer->buffer[6].type == TOKEN_IDENTIFIER);
    CHECK(lexer->buffer[7].type == TOKEN_EQUAL);
    CHECK(lexer->buffer[8].type == TOKEN_LIT_STRING);
    CHECK(lexer->buffer[9].offset == -1);
    
    token = lexer_get(lexer);
    CHECK(token.type == TOKEN_PLUS);
    CHECK(lexer->buffer_start == 2);
    CHECK(lexer->buffer[2].type == TOKEN_LIT_INTEGER);
    CHECK(lexer->buffer[3].type == TOKEN_AND);
    CHECK(lexer->buffer[4].type == TOKEN_LIT_INTEGER);
    CHECK(lexer->buffer[5].type == TOKEN_LIT_INTEGER);
    CHECK(lexer->buffer[6].type == TOKEN_IDENTIFIER);
    CHECK(lexer->buffer[7].type == TOKEN_EQUAL);
    CHECK(lexer->buffer[8].type == TOKEN_LIT_STRING);
    CHECK(lexer->buffer[9].offset == -1);
    CHECK(lexer->buffer[0].offset == -1);
    
    token = lexer_get(lexer);
    CHECK(token.type == TOKEN_LIT_INTEGER);
    CHECK(lexer->buffer_start == 3);
    CHECK(lexer->buffer[3].type == TOKEN_AND);
    CHECK(lexer->buffer[4].type == TOKEN_LIT_INTEGER);
    CHECK(lexer->buffer[5].type == TOKEN_LIT_INTEGER);
    CHECK(lexer->buffer[6].type == TOKEN_IDENTIFIER);
    CHECK(lexer->buffer[7].type == TOKEN_EQUAL);
    CHECK(lexer->buffer[8].type == TOKEN_LIT_STRING);
    CHECK(lexer->buffer[9].offset == -1);
    CHECK(lexer->buffer[0].offset == -1);
    CHECK(lexer->buffer[1].offset == -1);
    
    return NULL;
}


/* Token lexer_peek(Lexer *in_lexer, int in_how_far)*/
static const char* test_19()
{
    Lexer *lexer;
    Token token;
    
    lexer = lexer_create("29 +  &b1001ANd&o74 &hEF17A3\t "
                         "私はガラ=\"pickle 食べ\"");
    CHECK(lexer);
    
    token = lexer_peek(lexer, 0);
    CHECK(token.type == TOKEN_LIT_INTEGER);
    CHECK(lexer->buffer_start == 0);
    CHECK(lexer->buffer[1].type == TOKEN_PLUS);
    CHECK(lexer->buffer[2].type == TOKEN_LIT_INTEGER);
    CHECK(lexer->buffer[3].type == TOKEN_AND);
    CHECK(lexer->buffer[4].type == TOKEN_LIT_INTEGER);
    CHECK(lexer->buffer[5].type == TOKEN_LIT_INTEGER);
    CHECK(lexer->buffer[6].type == TOKEN_IDENTIFIER);
    CHECK(lexer->buffer[7].type == TOKEN_EQUAL);
    CHECK(lexer->buffer[8].type == TOKEN_LIT_STRING);
    CHECK(lexer->buffer[9].offset == -1);
    
    token = lexer_get(lexer);
    CHECK(token.type == TOKEN_LIT_INTEGER);
    CHECK(lexer->buffer_start == 1);
    CHECK(lexer->buffer[1].type == TOKEN_PLUS);
    CHECK(lexer->buffer[2].type == TOKEN_LIT_INTEGER);
    CHECK(lexer->buffer[3].type == TOKEN_AND);
    CHECK(lexer->buffer[4].type == TOKEN_LIT_INTEGER);
    CHECK(lexer->buffer[5].type == TOKEN_LIT_INTEGER);
    CHECK(lexer->buffer[6].type == TOKEN_IDENTIFIER);
    CHECK(lexer->buffer[7].type == TOKEN_EQUAL);
    CHECK(lexer->buffer[8].type == TOKEN_LIT_STRING);
    CHECK(lexer->buffer[9].offset == -1);
    
    token = lexer_peek(lexer, 0);
    CHECK(token.type == TOKEN_PLUS);
    CHECK(lexer->buffer_start == 1);
    
    token = lexer_peek(lexer, 1);
    CHECK(token.type == TOKEN_LIT_INTEGER);
    CHECK(lexer->buffer_start == 1);
    
    token = lexer_peek(lexer, 2);
    CHECK(token.type == TOKEN_AND);
    token = lexer_peek(lexer, 3);
    CHECK(token.type == TOKEN_LIT_INTEGER);
    token = lexer_peek(lexer, 4);
    CHECK(token.type == TOKEN_LIT_INTEGER);
    token = lexer_peek(lexer, 5);
    CHECK(token.type == TOKEN_IDENTIFIER);
    
    
    lexer = lexer_create("Namespace1.Class1.MethodA");
    CHECK(lexer);
    token = lexer_peek(lexer, 0);
    CHECK(token.type == TOKEN_IDENTIFIER);
    token = lexer_peek(lexer, 1);
    CHECK(token.type == TOKEN_DOT);
    token = lexer_peek(lexer, 2);
    CHECK(token.type == TOKEN_IDENTIFIER);
    token = lexer_peek(lexer, 3);
    CHECK(token.type == TOKEN_DOT);
    token = lexer_peek(lexer, 4);
    CHECK(token.type == TOKEN_IDENTIFIER);
    
    
    return NULL;
}


/* TODO: I fixed a bug which I found by running this through the lexer:
    "self.Dog(7) = new Dog(\"Fido\", 3)\r\n"
   The initial loading loop was filling the buffer incorrectly and dropping "Fido"
   from the token stream.
 
   Suggest adding a test case.
 */

void lexer_run_tests(void)
{
    const char *test_error;
    test_error = NULL;
    
    if (!test_error) test_error = test_1();
    if (!test_error) test_error = test_2();
    if (!test_error) test_error = test_3();
    if (!test_error) test_error = test_4();
    if (!test_error) test_error = test_5();
    if (!test_error) test_error = test_6();
    if (!test_error) test_error = test_7();
    if (!test_error) test_error = test_8();
    if (!test_error) test_error = test_9();
    if (!test_error) test_error = test_10();
    if (!test_error) test_error = test_11();
    if (!test_error) test_error = test_12();
    if (!test_error) test_error = test_13();
    if (!test_error) test_error = test_14();
    if (!test_error) test_error = test_15();
    if (!test_error) test_error = test_16();
    if (!test_error) test_error = test_17();
    if (!test_error) test_error = test_18();
    if (!test_error) test_error = test_19();
    
    if (test_error)
    {
        fprintf(stderr, "lexer_run_tests(): Failed: %s\n", test_error);
        exit(1);
    }
    else
    {
        fprintf(stdout, "lexer_run_tests(): OK\n");
    }
}


#endif


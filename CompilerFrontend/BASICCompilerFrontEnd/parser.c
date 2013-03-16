/*
 * parser.c
 * BASIC Parser
 *
 * (c) 2013 Joshua Hawcroft
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "parser.h"
#include "lexer.h"
#include "memory.h"
#include "test.h"


struct Parser
{
    AstNode* (*init) (Parser*);
    Lexer *lexer;
    char *error_message;
    long error_offset;
    AstNode *ast;
    AstNode *statement;
};


static void _error(Parser *in_parser, Token in_token, char *in_message)
{
    in_parser->error_message = in_message;
    in_parser->error_offset = in_token.offset;
}

#define SYNTAX(err) _error(in_parser, token, err);


static AstNode* _parse_path(Parser *in_parser);
static AstNode* _parse_expression(Parser *in_parser);
static AstNode* _parse_list(Parser *in_parser, AstNode*(*in_of)(Parser*), Boolean no_parens);


static AstNode* _parse_operand(Parser *in_parser)
{
    Token token;
    AstNode *result;
    AstNode *negate;
    
    token = lexer_peek(in_parser->lexer, 0);
    
    negate = NULL;
    if (token.type == TOKEN_HYPHEN)
    {
        /* got negation operator */
        lexer_get(in_parser->lexer);
        negate = ast_create(AST_EXPRESSION);
        ast_append(negate, ast_create_operator("negate"));
        token = lexer_peek(in_parser->lexer, 0);
    }
    
    switch (token.type)
    {
        case TOKEN_PAREN_LEFT:
            /* subexpression */
            lexer_get(in_parser->lexer);
            result = _parse_expression(in_parser);
            token = lexer_get(in_parser->lexer);
            if (token.type != TOKEN_PAREN_RIGHT)
                SYNTAX("Expecting )");
            break;
            
        case TOKEN_IDENTIFIER:
            /* path */
            result = _parse_path(in_parser);
            break;
            
        case TOKEN_LIT_STRING:
            lexer_get(in_parser->lexer);
            result = ast_create_string(token.text);
            break;
            
        case TOKEN_LIT_INTEGER:
            lexer_get(in_parser->lexer);
            result = ast_create_integer(token.value.integer);
            break;
            
        case TOKEN_LIT_REAL:
            lexer_get(in_parser->lexer);
            result = ast_create_real(token.value.real);
            break;
            
        case TOKEN_LIT_COLOUR:
            lexer_get(in_parser->lexer);
            result = ast_create_colour(token.value.integer);
            break;
            
        case TOKEN_TRUE:
            lexer_get(in_parser->lexer);
            result = ast_create_boolean(True);
            break;
            
        case TOKEN_FALSE:
            lexer_get(in_parser->lexer);
            result = ast_create_boolean(False);
            break;
        
        case TOKEN_NULL:
            lexer_get(in_parser->lexer);
            result = ast_create(AST_NULL);
            break;
            
        default:
            SYNTAX("Expecting expression");
            break;
    }
    
    if (negate)
    {
        ast_append(negate, result);
        return negate;
    }
    return result;
}


static Boolean _is_binary_operator(enum LexerTokenType in_type)
{
    switch (in_type)
    {
        case TOKEN_AND:
        case TOKEN_EQUAL:
        case TOKEN_NOT_EQUAL:
        case TOKEN_HYPHEN:
        case TOKEN_IS:
        case TOKEN_ISA:
        case TOKEN_LESS:
        case TOKEN_LESS_EQUAL:
        case TOKEN_MORE:
        case TOKEN_MORE_EQUAL:
        case TOKEN_MOD:
        case TOKEN_MULTIPLY:
        case TOKEN_SLASH:
        case TOKEN_BACK_SLASH:
        case TOKEN_PLUS:
        case TOKEN_OR:
            return True;
        default:
            return False;
    }
}


/* parsing: operand operator operand ...
    operand can be a subexpression (...) or a path (name().name().name()...)
    or a literal colour, boolean, string or integer;
 there must be an expression or this is a syntax error. */
static AstNode* _parse_expression(Parser *in_parser)
{
    AstNode *expr;
    Token token;
    
    /* create expression */
    expr = ast_create(AST_EXPRESSION);
    
    /* peek at what's next */
    token = lexer_peek(in_parser->lexer, 0);
    
    /* parse unary NOT */
    if (token.type == TOKEN_NOT)
    {
        lexer_get(in_parser->lexer);
        ast_append(expr, ast_create_operator("logical-not"));
        
        token = lexer_peek(in_parser->lexer, 0);
    }
    else if (token.type == TOKEN_NEW)
    {
        lexer_get(in_parser->lexer);
        ast_append(expr, ast_create_operator("new"));
        
        /* expect identifier */
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_IDENTIFIER)
            SYNTAX("Expected class name");
        ast_append(expr, ast_create_string(token.text));
        
        /* optional argument list */
        token = lexer_peek(in_parser->lexer, 0);
        if (token.type == TOKEN_PAREN_LEFT)
            ast_append(expr, _parse_list(in_parser, _parse_expression, False));
        
        return expr;
    }
    
    /* expecting an operand:
     subexpression, path, or literal */
    ast_append(expr, _parse_operand(in_parser));
    
    /* parse remaining terms (if any) */
    for (;;)
    {
        /* expecting end of expression or an operator */
        token = lexer_peek(in_parser->lexer, 0);
        if (_is_binary_operator(token.type))
        {
            lexer_get(in_parser->lexer);
            switch (token.type)
            {
                case TOKEN_AND:
                    ast_append(expr, ast_create_operator("logical-and")); break;
                case TOKEN_EQUAL:
                    ast_append(expr, ast_create_operator("equal")); break;
                case TOKEN_NOT_EQUAL:
                    ast_append(expr, ast_create_operator("not-equal")); break;
                case TOKEN_HYPHEN:
                    ast_append(expr, ast_create_operator("subtract")); break;
                case TOKEN_IS:
                    ast_append(expr, ast_create_operator("is")); break;
                case TOKEN_ISA:
                    ast_append(expr, ast_create_operator("is-a")); break;
                case TOKEN_LESS:
                    ast_append(expr, ast_create_operator("less-than")); break;
                case TOKEN_LESS_EQUAL:
                    ast_append(expr, ast_create_operator("less-or-equal")); break;
                case TOKEN_MORE:
                    ast_append(expr, ast_create_operator("more-than")); break;
                case TOKEN_MORE_EQUAL:
                    ast_append(expr, ast_create_operator("more-or-equal")); break;
                case TOKEN_MOD:
                    ast_append(expr, ast_create_operator("modulus")); break;
                case TOKEN_MULTIPLY:
                    ast_append(expr, ast_create_operator("multiply")); break;
                case TOKEN_SLASH:
                    ast_append(expr, ast_create_operator("divide")); break;
                case TOKEN_BACK_SLASH:
                    ast_append(expr, ast_create_operator("int-divide")); break;
                case TOKEN_PLUS:
                    ast_append(expr, ast_create_operator("add")); break;
                case TOKEN_OR:
                    ast_append(expr, ast_create_operator("logical-or")); break;
                    break;
                    
                default:
                    SYNTAX("Internal compiler error; parsing undeclared binary operator");
                    break;
            }
        }
        else break;
        
        /* expecting another operand */
        ast_append(expr, _parse_operand(in_parser));
    }
    
    return expr;
}


/* parsing: (a,b,c,...) etc.
 the elements are parsed with in_of(), 
 so the function can handle lists of almost anything! */
static AstNode* _parse_list(Parser *in_parser, AstNode*(*in_of)(Parser*), Boolean no_parens)
{
    AstNode *list;
    Token token;
    
    if (!no_parens)
    {
        /* expect ( */
        token = lexer_peek(in_parser->lexer, 0);
        if (token.type != TOKEN_PAREN_LEFT) return NULL;
        lexer_get(in_parser->lexer);
    }
    
    /* create list */
    list = ast_create(AST_LIST);
    
    /* is list empty? */
    token = lexer_peek(in_parser->lexer, 0);
    if (token.type == TOKEN_PAREN_RIGHT)
    {
        lexer_get(in_parser->lexer);
        return list;
    }
    
    for (;;)
    {
        /* parse whatever it is we have a list of */
        ast_append(list, in_of(in_parser));
        
        /* look for , */
        token = lexer_peek(in_parser->lexer, 0);
        if (token.type != TOKEN_COMMA) break;
        lexer_get(in_parser->lexer);
    }
    
    if (!no_parens)
    {
        /* expect ) */
        token = lexer_peek(in_parser->lexer, 0);
        if (token.type != TOKEN_PAREN_RIGHT)
            SYNTAX("Expecting )");
        lexer_get(in_parser->lexer);
    }
    
    return list;
}


/* parsing: name().name().name() etc.
 lists after each name are optional, path may be empty */
static AstNode* _parse_path(Parser *in_parser)
{
    AstNode *path;
    Token token;
    Boolean can_index;
    
    path = ast_create(AST_PATH);
    
    token = lexer_peek(in_parser->lexer, 0);
    while ((token.type == TOKEN_IDENTIFIER) || (token.type == TOKEN_SELF) ||
           (token.type == TOKEN_ME) || (token.type == TOKEN_SUPER))
    {
        /* record name */
        lexer_get(in_parser->lexer);
        can_index = ((token.type == TOKEN_IDENTIFIER) || (token.type == TOKEN_SUPER));
        if (token.type == TOKEN_IDENTIFIER)
            ast_append(path, ast_create_string( token.text ));
        else if (token.type == TOKEN_SUPER)
            ast_append(path, ast_create_string( "super" ));
        else if (token.type == TOKEN_SELF)
            ast_append(path, ast_create_string( "self" ));
        else if (token.type == TOKEN_ME)
            ast_append(path, ast_create_string( "me" ));
        
        /* expecting: ( OR . */
        token = lexer_peek(in_parser->lexer, 0);
        if (token.type == TOKEN_DOT)
            lexer_get(in_parser->lexer);
        else if (token.type == TOKEN_PAREN_LEFT)
        {
            /* parse (...) */
            ast_append(path, _parse_list(in_parser, _parse_expression, False));
            
            /* expect . */
            token = lexer_peek(in_parser->lexer, 0);
            if (token.type != TOKEN_DOT) break;
            lexer_get(in_parser->lexer);
        }
        
        token = lexer_peek(in_parser->lexer, 0);
    }
    
    return path;
}

/* TODO: must also parse pragmas here */

/* parsing: a single line statement (already determined to not be a control structure)
 line can be blank or must be a valid statement */
static AstNode* _parse_statement(Parser *in_parser)
{
    Token token;
    AstNode *path;
    
    /* begin statement */
    in_parser->statement = ast_create(AST_STATEMENT);
    
    /* statement must start with a path */
    token = lexer_peek(in_parser->lexer, 0);
    if ((token.type == TOKEN_IDENTIFIER) || (token.type == TOKEN_SELF) ||
        (token.type == TOKEN_ME) || (token.type == TOKEN_SUPER))
    {
        /* parse path */
        path = _parse_path(in_parser);
        ast_append(in_parser->statement, path);
        
        /* see if statement is an assignment */
        token = lexer_peek(in_parser->lexer, 0);
        if (token.type == TOKEN_EQUAL)
        {
            lexer_get(in_parser->lexer);
            ast_append(in_parser->statement, _parse_expression(in_parser));
        }
        /* see if statement is a method call with unbracketed parameters */
        else if (token.type != TOKEN_NEW_LINE)
        {
            ast_append(path, _parse_list(in_parser, _parse_expression, True));
        }
        
        /* expect end of line */
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_NEW_LINE)
            SYNTAX("Expected end of line");
        
    }
    else if (token.type == TOKEN_NEW_LINE)
    {
        /* statement can be empty */
    }
    else
    {
        SYNTAX("Expected identifier");
    }
    
    return in_parser->statement;
}


static AstNode* _parse_if(Parser *in_parser)
{
    
    
    return NULL;
}


static AstNode* _parse_compiler_if(Parser *in_parser)
{
    
    
    return NULL;
}


static AstNode* _parse_select(Parser *in_parser)
{
    
    
    return NULL;
}


static AstNode* _parse_do(Parser *in_parser)
{
    
    
    return NULL;
}


static AstNode* _parse_while(Parser *in_parser)
{
    
    
    return NULL;
}


static AstNode* _parse_for(Parser *in_parser)
{
    
    
    return NULL;
}



/* parsing # compiler conditionals must occur in blocks too;
 and thus will probably have to occur at each level where they're valid,
 ie. inside any _parse_block() */

static AstNode* _parse_block(Parser *in_parser)
{
    
    
    return NULL;
}


static AstNode* _parse_routine(Parser *in_parser)
{
    
    
    return NULL;
}


static AstNode* _parse_property(Parser *in_parser)
{
    
    
    return NULL;
}


static AstNode* _parse_class(Parser *in_parser)
{
    
    
    return NULL;
}







void parser_parse(Parser *in_parser, char *in_source)
{
    in_parser->lexer = lexer_create(in_source);
    in_parser->ast = in_parser->init(in_parser);
}


Parser* parser_create(void)
{
    Parser *parser;
    
    parser = safe_malloc(sizeof(struct Parser));
    
    parser->error_message = NULL;
    parser->error_offset = 0;
    parser->lexer = NULL;
    parser->ast = NULL;
    parser->statement = NULL;
    parser->init = &_parse_statement;
    
    return parser;
}




#ifdef DEBUG


static const char* _test_1()
{
    Parser *parser;
    
    parser = parser_create();
    CHECK(parser);
    
    parser->init = &_parse_statement;
    
    parser_parse(parser,
                 "Beep\r\n"
                 );
    ast_walk(parser->ast, ast_debug_walker);
    
    parser_parse(parser,
                 "Beep(3)\r\n"
                 );
    ast_walk(parser->ast, ast_debug_walker);

    parser_parse(parser,
                 "System.Beep\r\n"
                 );
    ast_walk(parser->ast, ast_debug_walker);
    
    parser_parse(parser,
                 "System.Beep()\r\n"
                 );
    ast_walk(parser->ast, ast_debug_walker);

    parser_parse(parser,
                 "System.Beep(3)\r\n"
                 );
    ast_walk(parser->ast, ast_debug_walker);

    parser_parse(parser,
                 "System.UI.Beep\r\n"
                 );
    ast_walk(parser->ast, ast_debug_walker);

    parser_parse(parser,
                 "System.Process(1).Activate\r\n"
                 );
    ast_walk(parser->ast, ast_debug_walker);

    parser_parse(parser,
                 "System.Process(1).Activate(System.Process.Behind, System.Process.Now)\r\n"
                 );
    ast_walk(parser->ast, ast_debug_walker);

    parser_parse(parser,
                 "Console.WriteLine \"Hello World!\"\r\n"
                 );
    ast_walk(parser->ast, ast_debug_walker);

    parser_parse(parser,
                 "Console.WriteLine(\"Hello World!\")\r\n"
                 );
    ast_walk(parser->ast, ast_debug_walker);

    parser_parse(parser,
                 "self.DogsAge = 1 * inX + 2\r\n"
                 );
    ast_walk(parser->ast, ast_debug_walker);

    parser_parse(parser,
                 "self.Dog(7) = new Dog(\"Fido\", 3)\r\n"
                 );
    ast_walk(parser->ast, ast_debug_walker);

    parser_parse(parser,
                 "self.Dog(3) = inAnimals(16).getDog(14)"
                 );
    ast_walk(parser->ast, ast_debug_walker);

    parser_parse(parser,
                 "self.Title = inMofset.getName() + \" \" + inBoggle.getSex()"
                 );
    ast_walk(parser->ast, ast_debug_walker);
    
    
    return NULL;
}



void parser_run_tests()
{
    const char *err;
    
    /* lexer must pass before we can test parser */
    lexer_run_tests();
    err = NULL;
    
    if (!err) err = _test_1();
    
    if (err)
    {
        fprintf(stderr, "parser_run_tests(): Failed: %s\n", err);
        exit(1);
    }
    
    printf("parser_run_tests(): OK\n");
}


#endif


/*Lexer *lexer;
 Token token;
 
 
 // can probably use recursive decent here rather than doing a loop, etc.
 // may need minor modifications in the lexer to allow repeating a token... we'll see
 
 lexer = lexer_create(in_source);
 for (token = lexer_get_next_token(lexer); token.offset >= 0; token = lexer_get_next_token(lexer))
 {
 
 }*/


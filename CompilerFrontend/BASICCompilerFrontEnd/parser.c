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


static AstNode* _error(Parser *in_parser, long in_offset, char *in_message)
{
    if (in_parser->error_message) return NULL;
    in_parser->error_message = in_message;
    in_parser->error_offset = in_offset;
    //abort();
    return NULL;
}

#define SYNTAX(err) return _error(in_parser, lexer_offset(in_parser->lexer), err);


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
            
        case TOKEN_SELF:
        case TOKEN_SUPER:
        case TOKEN_ME:
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
            if (negate)
                ast_dispose(negate);
            return NULL;
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
    AstNode *expr, *operand;
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
    operand = _parse_operand(in_parser);
    if (!operand) return NULL;
    ast_append(expr, operand);
    
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
        operand = _parse_operand(in_parser);
        if (!operand)
            SYNTAX("Expected operand");
        ast_append(expr, operand);
    }
    
    return expr;
}


/* parsing: (a,b,c,...) etc.
 the elements are parsed with in_of(), 
 so the function can handle lists of almost anything! */
static AstNode* _parse_list(Parser *in_parser, AstNode*(*in_of)(Parser*), Boolean no_parens)
{
    AstNode *list, *item;
    Token token;
    Boolean require_item;
    
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
    
    require_item = False;
    for (;;)
    {
        /* parse whatever it is we have a list of */
        item = in_of(in_parser);
        if (require_item && (!item))
            SYNTAX("Expected expression here");
        ast_append(list, item);
        
        /* look for , */
        token = lexer_peek(in_parser->lexer, 0);
        if (token.type != TOKEN_COMMA) break;
        lexer_get(in_parser->lexer);
        require_item = True;
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
        else break;
        
        token = lexer_peek(in_parser->lexer, 0);
    }
    
    return path;
}


const char* long_to_string(long in_int)
{
    static char buffer[100];
    snprintf(buffer, 100, "%ld", in_int);
    return buffer;
}


const char* double_to_string(double in_double)
{
    static char buffer[100];
    snprintf(buffer, 100, "%lf", in_double);
    return buffer;
}


/* TODO: must also parse pragmas here */

/* parsing: a single line statement (already determined to not be a control structure)
 line can be blank or must be a valid statement;
 a valid statement can consist of:
 -  an assignment x = ...
 -  a call ()
 -  a compiler #pragma
 -  control keywords: break, continue
 */
static AstNode* _parse_statement(Parser *in_parser)
{
    Token token;
    AstNode *stmt;
    AstNode *path;
    AstNode *list;
    AstNode *last;
    int i, c;
    
    /* begin statement */
    stmt = in_parser->statement = ast_create(AST_STATEMENT);
    
    /* peek at the first token */
    token = lexer_peek(in_parser->lexer, 0);
    
    /* handle #pragma */
    if (token.type == TOKEN_PRAGMA)
    {
        /* expect an identifier, followed by a constant, followed by end of line */
        lexer_get(in_parser->lexer);
        ast_append(stmt, ast_create_string("pragma"));
        
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_IDENTIFIER)
            SYNTAX("Expected pragma identifier");
        ast_append(stmt, ast_create_string(token.text));
        
        token = lexer_get(in_parser->lexer);
        switch (token.type)
        {
            case TOKEN_IDENTIFIER:
                ast_append(stmt, ast_create_string(token.text));
                break;
            case TOKEN_LIT_STRING:
                ast_append(stmt, ast_create_string(token.text));
                break;
            case TOKEN_LIT_INTEGER:
                ast_append(stmt, ast_create_string( long_to_string(token.value.integer) ));
                break;
            case TOKEN_LIT_REAL:
                ast_append(stmt, ast_create_string( double_to_string(token.value.real) ));
                break;
            case TOKEN_TRUE:
                ast_append(stmt, ast_create_string("true"));
                break;
            case TOKEN_FALSE:
                ast_append(stmt, ast_create_string("false"));
                break;
            default:
                SYNTAX("Expected pragma value");
                break;
        }
        
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_NEW_LINE)
            SYNTAX("Expected end of line");
    }
    
    /* handle exit */
    else if (token.type == TOKEN_EXIT)
    {
        /* expect end of line */
        ast_append(stmt, ast_create_string("break"));
        lexer_get(in_parser->lexer);
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_NEW_LINE)
            SYNTAX("Expected end of line");
    }
    
    /* handle continue */
    else if (token.type == TOKEN_CONTINUE)
    {
        /* expect end of line */
        ast_append(stmt, ast_create_string("continue"));
        lexer_get(in_parser->lexer);
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_NEW_LINE)
            SYNTAX("Expected end of line");
    }
    
    /* handle assignment or call() */
    else if ((token.type == TOKEN_IDENTIFIER) || (token.type == TOKEN_SELF) ||
        (token.type == TOKEN_ME) || (token.type == TOKEN_SUPER))
    {
        /* parse path */
        path = _parse_path(in_parser);
        ast_append(in_parser->statement, path);
        
        //ast_walk(path, ast_debug_walker, NULL);
        
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
            //ast_walk(path, ast_debug_walker, NULL);
            
            if (token.type == TOKEN_COMMA)
            {
                /* the first argument has already been accidentally parsed on to the end
                 of the path */
                lexer_get(in_parser->lexer);
            }
            
            /* parse the argument list */
            list = _parse_list(in_parser, _parse_expression, True);
            
            //ast_walk(list, ast_debug_walker, NULL);
            
            /* check for an existing parameter mistaken as part of the preceeding path */
            last = ast_child(path, AST_LAST);
            if (ast_is(last, AST_LIST))
            {
                /* move each of the items from the parsed list into the end of the existing list */
                c = ast_count(list);
                for (i = 0; i < c; i++)
                    ast_append(last, ast_remove(list, 0));
                ast_dispose(list);
                //ast_prepend(list, ast_remove(path, AST_LAST));
            }
            else
                /* append the argument list */
                ast_append(path, list);
        }
        
        /* expect end of line */
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_NEW_LINE)
            SYNTAX("Expected end of line");
        
    }
    
    /* statement can be empty */
    else if (token.type == TOKEN_NEW_LINE)
    {
        /* ignore newline */
        lexer_get(in_parser->lexer);
    }
    
    /* anything else is a syntax error */
    else
    {
        SYNTAX("Expected identifier");
    }
    
    return in_parser->statement;
}



static AstNode* _parse_block(Parser *in_parser);


static AstNode* _parse_if(Parser *in_parser)
{
    Token token, token2;
    AstNode *cond, *expr;
    
    /* create the If node */
    cond = ast_create(AST_CONTROL);
    ast_append(cond, ast_create_string("if"));
    
    /* skip the If keyword */
    token = lexer_get(in_parser->lexer);
    
    /* expect an expression */
    expr = _parse_expression(in_parser);
    if (!expr) SYNTAX("Expected conditional expression");
    ast_append(cond, expr);
    
    /* expect Then */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_THEN) SYNTAX("Expected Then");
    
    /* expect either a block or a newline */
    token = lexer_peek(in_parser->lexer, 0);
    if (token.type == TOKEN_NEW_LINE)
    {
        /* parsing a normal multiline If block */
        lexer_get(in_parser->lexer);
        
        /* parse block */
        expr = _parse_block(in_parser);
        if (!expr) return NULL;
        ast_append(cond, expr);
        
        /* handle ElseIf */
        for (;;)
        {
            token = lexer_peek(in_parser->lexer, 0);
            token2 = lexer_peek(in_parser->lexer, 1);
            if ((token.type == TOKEN_ELSE) && (token2.type == TOKEN_IF))
            {
                lexer_get(in_parser->lexer);
                lexer_get(in_parser->lexer);
                
                /* expect an expression */
                expr = _parse_expression(in_parser);
                if (!expr) SYNTAX("Expected conditional expression");
                ast_append(cond, expr);
                
                /* expect Then */
                token = lexer_get(in_parser->lexer);
                if (token.type != TOKEN_THEN) SYNTAX("Expected Then");
                
                /* parse block */
                expr = _parse_block(in_parser);
                if (!expr) return NULL;
                ast_append(cond, expr);
            }
            else break;
        }
        
        /* handle Else */
        token = lexer_peek(in_parser->lexer, 0);
        if (token.type == TOKEN_ELSE)
        {
            lexer_get(in_parser->lexer);
            token = lexer_get(in_parser->lexer);
            if (token.type != TOKEN_NEW_LINE)
                SYNTAX("Expected end of line");
            
            /* parse block */
            expr = _parse_block(in_parser);
            if (!expr) return NULL;
            ast_append(cond, expr);
        }
        
        /* expect End If */
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_END)
            SYNTAX("Expected End If");
        token = lexer_peek(in_parser->lexer, 0);
        if (token.type == TOKEN_IF)
            lexer_get(in_parser->lexer);
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_NEW_LINE)
            SYNTAX("Expected end of line");
        return cond;
    }
    else
    {
        /* parsing a single line If statement */
        
    }
    
    return cond;
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
    Token token;
    AstNode *block, *result;
    
    /* create a block */
    block = ast_create(AST_LIST);
    
    for (;;)
    {
        /* peek at next token */
        token = lexer_peek(in_parser->lexer, 0);
        if (token.offset < 0) break;
        
        /* handle If...Then...Else block */
        if (token.type == TOKEN_IF)
            result = _parse_if(in_parser);
        
        /* handle Select Case block */
        else if (token.type == TOKEN_SELECT)
            result = _parse_select(in_parser);
        
        /* handle While block */
        else if (token.type == TOKEN_WHILE)
            result = _parse_while(in_parser);
        
        /* handle Do loop */
        else if (token.type == TOKEN_DO)
            result = _parse_do(in_parser);
        
        /* handle For loop */
        else if (token.type == TOKEN_FOR)
            result = _parse_for(in_parser);
        
        /* handle #If...#Else block */
        else if (token.type == TOKEN_HASH_IF)
            result = _parse_compiler_if(in_parser);
        
        /* handle end of block */
        else if ((token.type == TOKEN_END) || (token.type == TOKEN_ELSE))
            break;
        
        /* handle blank line */
        else if (token.type == TOKEN_NEW_LINE)
        {
            lexer_get(in_parser->lexer);
            continue;
        }
        
        /* anything else is assumed to be a statement */
        else
            result = _parse_statement(in_parser);
        
        /* check whatever we handled was successful */
        if (!result) return NULL;
        ast_append(block, result);
    }
    
    return block;
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




static void _reset(Parser *in_parser)
{
    in_parser->error_message = NULL;
    if (in_parser->ast) ast_dispose(in_parser->ast);
}


Boolean parser_parse(Parser *in_parser, char *in_source)
{
    _reset(in_parser);
    
    in_parser->lexer = lexer_create(in_source);
    in_parser->ast = in_parser->init(in_parser);
    if (in_parser->error_message)
    {
        ast_dispose(in_parser->ast);
        in_parser->ast = NULL;
        return False;
    }
    return True;
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


static Parser *g_test_parser;


static void _test_case_result(void *in_user, const char *in_file, int in_case_number, long in_line_number, const char *in_error)
{
    if (in_error)
        printf("%s: case %d, line %ld: failed: %s\n", in_file, in_case_number, in_line_number, in_error);
    else
        printf("%s: case %d, line %ld: OK\n", in_file, in_case_number, in_line_number);
}


static const char* _test_case_runner(void *in_user, const char *in_file, int in_case_number, const char *in_input, const char *in_output)
{
    char *result;
    char *err;
    static char err_msg_buffer[1024];
    
    err = NULL;
    
//    if (in_case_number == 10) /* debugging breakpoint hook */
//    {
//        err = NULL;
//    }

    parser_parse(g_test_parser, (char*)in_input);
    result = NULL;
    ast_walk(g_test_parser->ast, ast_string_walker, &result);
    
    if (result == NULL)
    {
        snprintf(err_msg_buffer, 1024, "%ld: %s", g_test_parser->error_offset, g_test_parser->error_message);
        if (strcmp(err_msg_buffer, in_output) != 0)
            err = err_msg_buffer;
    }
    else
    {
        if (strcmp(result, in_output) != 0)
            err = result;
    }
    
    return err;
}


void parser_run_tests()
{
    g_test_parser = parser_create();
    g_test_parser->init = _parse_statement;
    if (!g_test_parser)
    {
        fprintf(stderr, "Couldn't initalize parser test environment.\n");
        return;
    }
    
    test_run_cases("/Users/josh/Projects/Active/runlessbasic/CompilerFrontend/parser-statement.tests",
                   _test_case_runner, _test_case_result, NULL);
    
    g_test_parser->init = _parse_block;
    test_run_cases("/Users/josh/Projects/Active/runlessbasic/CompilerFrontend/parser-control.tests",
                   _test_case_runner, _test_case_result, NULL);
}


#endif




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


static AstNode* _error(Parser *in_parser, Token in_token, char *in_message)
{
    in_parser->error_message = in_message;
    in_parser->error_offset = in_token.offset;
    //abort();
    return NULL;
}

#define SYNTAX(err) return _error(in_parser, token, err);


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
        else break;
        
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
    AstNode *list;
    AstNode *last;
    int i, c;
    
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


static void _test_case_result(void *in_user, int in_case_number, long in_line_number, const char *in_error)
{
    if (in_error)
        printf("parser: case %d, line %ld: failed: %s\n", in_case_number, in_line_number, in_error);
    else
        printf("parser: case %d, line %ld: OK\n", in_case_number, in_line_number);
}


static const char* _test_case_runner(void *in_user, int in_case_number, const char *in_input, const char *in_output)
{
    char *result;
    char *err;
    err = NULL;
    parser_parse(g_test_parser, (char*)in_input);
    result = NULL;
    ast_walk(g_test_parser->ast, ast_string_walker, &result);
    if (result == NULL)
    {
        if (strcmp(g_test_parser->error_message, in_output) != 0)
        //if (strlen(in_output) != 0)
            err = "error";
    }
    else
    {
        if (strcmp(result, in_output) != 0)
            err = result;
        //free(result);
    }
    return err;
}


static const char* _test_2()
{
    g_test_parser = parser_create();
    CHECK(g_test_parser);
    
    return test_run_cases("/Users/josh/Projects/Active/runlessbasic/CompilerFrontend/parser.tests",
                          _test_case_runner, _test_case_result, NULL);
    
    /*Parser *parser;
    char *result;
    
    parser = parser_create();
    CHECK(parser);
    
    parser->init = &_parse_statement;
    
    parser_parse(parser,
                 "local "The answer is " + Str(42 + (0 * anotherLocal)) + ("." + (" ")) + Str(Not (bob.type = "builder")) \n"
                 );
    result = NULL;
    ast_walk(parser->ast, ast_string_walker, &result);
    CHECK(strcmp("<statement> {\n"
                 "  <path> {\n"
                 "    <string:\"z5\">\n"
                 "    <list> {\n"
                 "      <expression> {\n"
                 "        <string:\"cool\">\n"
                 "      }\n"
                 "      <expression> {\n"
                 "        <path> {\n"
                 "          <string:\"pickle\">\n"
                 "        }\n"
                 "      }\n"
                 "    }\n"
                 "  }\n"
                 "}\n", result) == 0);
    safe_free(result);
    */
    return NULL;
}




static const char* _test_1()
{
    Parser *parser;
    char *result;
    
    parser = parser_create();
    CHECK(parser);
    
    parser->init = &_parse_statement;
    
    parser_parse(parser,
                 "Beep\r\n"
                 );
    result = NULL;
    ast_walk(parser->ast, ast_string_walker, &result);
    CHECK(strcmp("<statement> {\n"
                 "  <path> {\n"
                 "    <string:\"Beep\">\n"
                 "  }\n"
                 "}\n", result) == 0);
    safe_free(result);
    
    parser_parse(parser,
                 "Beep(3)\r\n"
                 );
    result = NULL;
    ast_walk(parser->ast, ast_string_walker, &result);
    CHECK(strcmp("<statement> {\n"
                 "  <path> {\n"
                 "    <string:\"Beep\">\n"
                 "    <list> {\n"
                 "      <expression> {\n"
                 "        <integer:3>\n"
                 "      }\n"
                 "    }\n"
                 "  }\n"
                 "}\n", result) == 0);
    safe_free(result);

    parser_parse(parser,
                 "System.Beep\r\n"
                 );
    result = NULL;
    ast_walk(parser->ast, ast_string_walker, &result);
    CHECK(strcmp("<statement> {\n"
                 "  <path> {\n"
                 "    <string:\"System\">\n"
                 "    <string:\"Beep\">\n"
                 "  }\n"
                 "}\n", result) == 0);
    safe_free(result);
        
    parser_parse(parser,
                 "System.Beep()\r\n"
                 );
    result = NULL;
    ast_walk(parser->ast, ast_string_walker, &result);
    CHECK(strcmp("<statement> {\n"
                 "  <path> {\n"
                 "    <string:\"System\">\n"
                 "    <string:\"Beep\">\n"
                 "    <list> {\n"
                 "    }\n"
                 "  }\n"
                 "}\n", result) == 0);
    safe_free(result);

    parser_parse(parser,
                 "System.Beep(3)\r\n"
                 );
    result = NULL;
    ast_walk(parser->ast, ast_string_walker, &result);
    CHECK(strcmp("<statement> {\n"
                 "  <path> {\n"
                 "    <string:\"System\">\n"
                 "    <string:\"Beep\">\n"
                 "    <list> {\n"
                 "      <expression> {\n"
                 "        <integer:3>\n"
                 "      }\n"
                 "    }\n"
                 "  }\n"
                 "}\n", result) == 0);
    safe_free(result);
    
    parser_parse(parser,
                 "System.UI.Beep\r\n"
                 );
    result = NULL;
    ast_walk(parser->ast, ast_string_walker, &result);
    CHECK(strcmp("<statement> {\n"
                 "  <path> {\n"
                 "    <string:\"System\">\n"
                 "    <string:\"UI\">\n"
                 "    <string:\"Beep\">\n"
                 "  }\n"
                 "}\n", result) == 0);
    safe_free(result);
    
    parser_parse(parser,
                 "System.Process(1).Activate\r\n"
                 );
    result = NULL;
    ast_walk(parser->ast, ast_string_walker, &result);
    CHECK(strcmp("<statement> {\n"
                 "  <path> {\n"
                 "    <string:\"System\">\n"
                 "    <string:\"Process\">\n"
                 "    <list> {\n"
                 "      <expression> {\n"
                 "        <integer:1>\n"
                 "      }\n"
                 "    }\n"
                 "    <string:\"Activate\">\n"
                 "  }\n"
                 "}\n", result) == 0);
    safe_free(result);
    
    parser_parse(parser,
                 "System.Process(1).Activate(System.Process.Behind, System.Process.Now)\r\n"
                 );
    result = NULL;
    ast_walk(parser->ast, ast_string_walker, &result);
    CHECK(strcmp("<statement> {\n"
                 "  <path> {\n"
                 "    <string:\"System\">\n"
                 "    <string:\"Process\">\n"
                 "    <list> {\n"
                 "      <expression> {\n"
                 "        <integer:1>\n"
                 "      }\n"
                 "    }\n"
                 "    <string:\"Activate\">\n"
                 "    <list> {\n"
                 "      <expression> {\n"
                 "        <path> {\n"
                 "          <string:\"System\">\n"
                 "          <string:\"Process\">\n"
                 "          <string:\"Behind\">\n"
                 "        }\n"
                 "      }\n"
                 "      <expression> {\n"
                 "        <path> {\n"
                 "          <string:\"System\">\n"
                 "          <string:\"Process\">\n"
                 "          <string:\"Now\">\n"
                 "        }\n"
                 "      }\n"
                 "    }\n"
                 "  }\n"
                 "}\n", result) == 0);
    safe_free(result);
    
    parser_parse(parser,
                 "Console.WriteLine \"Hello World!\"\r\n"
                 );
    result = NULL;
    ast_walk(parser->ast, ast_string_walker, &result);
    CHECK(strcmp("<statement> {\n"
                 "  <path> {\n"
                 "    <string:\"Console\">\n"
                 "    <string:\"WriteLine\">\n"
                 "    <list> {\n"
                 "      <expression> {\n"
                 "        <string:\"Hello World!\">\n"
                 "      }\n"
                 "    }\n"
                 "  }\n"
                 "}\n", result) == 0);
    safe_free(result);

    parser_parse(parser,
                 "Console.WriteLine(\"Hello World!\")\r\n"
                 );
    result = NULL;
    ast_walk(parser->ast, ast_string_walker, &result);
    CHECK(strcmp("<statement> {\n"
                 "  <path> {\n"
                 "    <string:\"Console\">\n"
                 "    <string:\"WriteLine\">\n"
                 "    <list> {\n"
                 "      <expression> {\n"
                 "        <string:\"Hello World!\">\n"
                 "      }\n"
                 "    }\n"
                 "  }\n"
                 "}\n", result) == 0);
    safe_free(result);

    parser_parse(parser,
                 "self.DogsAge = 1 * inX + 2\r\n"
                 );
    result = NULL;
    ast_walk(parser->ast, ast_string_walker, &result);
    CHECK(strcmp("<statement> {\n"
                 "  <path> {\n"
                 "    <string:\"self\">\n"
                 "    <string:\"DogsAge\">\n"
                 "  }\n"
                 "  <expression> {\n"
                 "    <integer:1>\n"
                 "    <operator:multiply>\n"
                 "    <path> {\n"
                 "      <string:\"inX\">\n"
                 "    }\n"
                 "    <operator:add>\n"
                 "    <integer:2>\n"
                 "  }\n"
                 "}\n", result) == 0);
    safe_free(result);
    
    parser_parse(parser,
                 "self.Dog(7) = new Dog(\"Fido\", 3)\r\n"
                 );
    result = NULL;
    ast_walk(parser->ast, ast_string_walker, &result);
    CHECK(strcmp("<statement> {\n"
                 "  <path> {\n"
                 "    <string:\"self\">\n"
                 "    <string:\"Dog\">\n"
                 "    <list> {\n"
                 "      <expression> {\n"
                 "        <integer:7>\n"
                 "      }\n"
                 "    }\n"
                 "  }\n"
                 "  <expression> {\n"
                 "    <operator:new>\n"
                 "    <string:\"Dog\">\n"
                 "    <list> {\n"
                 "      <expression> {\n"
                 "        <string:\"Fido\">\n"
                 "      }\n"
                 "      <expression> {\n"
                 "        <integer:3>\n"
                 "      }\n"
                 "    }\n"
                 "  }\n"
                 "}\n", result) == 0);
    safe_free(result);

    parser_parse(parser,
                 "self.Dog(3) = inAnimals(16).getDog(14)\n"
                 );
    result = NULL;
    ast_walk(parser->ast, ast_string_walker, &result);
    CHECK(strcmp("<statement> {\n"
                 "  <path> {\n"
                 "    <string:\"self\">\n"
                 "    <string:\"Dog\">\n"
                 "    <list> {\n"
                 "      <expression> {\n"
                 "        <integer:3>\n"
                 "      }\n"
                 "    }\n"
                 "  }\n"
                 "  <expression> {\n"
                 "    <path> {\n"
                 "      <string:\"inAnimals\">\n"
                 "      <list> {\n"
                 "        <expression> {\n"
                 "          <integer:16>\n"
                 "        }\n"
                 "      }\n"
                 "      <string:\"getDog\">\n"
                 "      <list> {\n"
                 "        <expression> {\n"
                 "          <integer:14>\n"
                 "        }\n"
                 "      }\n"
                 "    }\n"
                 "  }\n"
                 "}\n", result) == 0);
    safe_free(result);

    parser_parse(parser,
                 "self.Title = inMofset.getName() + \" \" + inBoggle.getSex()\n"
                 );
    result = NULL;
    ast_walk(parser->ast, ast_string_walker, &result);
    CHECK(strcmp("<statement> {\n"
                 "  <path> {\n"
                 "    <string:\"self\">\n"
                 "    <string:\"Title\">\n"
                 "  }\n"
                 "  <expression> {\n"
                 "    <path> {\n"
                 "      <string:\"inMofset\">\n"
                 "      <string:\"getName\">\n"
                 "      <list> {\n"
                 "      }\n"
                 "    }\n"
                 "    <operator:add>\n"
                 "    <string:\" \">\n"
                 "    <operator:add>\n"
                 "    <path> {\n"
                 "      <string:\"inBoggle\">\n"
                 "      <string:\"getSex\">\n"
                 "      <list> {\n"
                 "      }\n"
                 "    }\n"
                 "  }\n"
                 "}\n", result) == 0);
    safe_free(result);
    
    ///
    
    parser_parse(parser,
                 "x = z5 ((2 - -y) And (bob.theBuilder = \"cool\")) + 9\n"
                 );
    result = NULL;
    ast_walk(parser->ast, ast_string_walker, &result);
    CHECK(strcmp("<statement> {\n"
                 "  <path> {\n"
                 "    <string:\"x\">\n"
                 "  }\n"
                 "  <expression> {\n"
                 "    <path> {\n"
                 "      <string:\"z5\">\n"
                 "      <list> {\n"
                 "        <expression> {\n"
                 "          <expression> {\n"
                 "            <integer:2>\n"
                 "            <operator:subtract>\n"
                 "            <expression> {\n"
                 "              <operator:negate>\n"
                 "              <path> {\n"
                 "                <string:\"y\">\n"
                 "              }\n"
                 "            }\n"
                 "          }\n"
                 "          <operator:logical-and>\n"
                 "          <expression> {\n"
                 "            <path> {\n"
                 "              <string:\"bob\">\n"
                 "              <string:\"theBuilder\">\n"
                 "            }\n"
                 "            <operator:equal>\n"
                 "            <string:\"cool\">\n"
                 "          }\n"
                 "        }\n" // first item expr
                 "      }\n"// list - args to z5
                 "    }\n"//path
                 "    <operator:add>\n"
                 "    <integer:9>\n"
                 "  }\n"
                 "}\n", result) == 0);
    safe_free(result);
    
    CHECK(! parser_parse(parser,
                         "x = z5((2 - -y) And (bob.theBuilder = \"cool\")), picle\n"
                         ));
    
    parser_parse(parser,
                 "z5 ((\"cool\") + str(5)), pickle \n"
                 );
    result = NULL;
    ast_walk(parser->ast, ast_string_walker, &result);
    CHECK(strcmp("<statement> {\n"
                 "  <path> {\n"
                 "    <string:\"z5\">\n"
                 "    <list> {\n"
                 "      <expression> {\n"
                 "        <expression> {\n"
                 "          <string:\"cool\">\n"
                 "        }\n"
                 "        <operator:add>\n"
                 "        <path> {\n"
                 "          <string:\"str\">\n"
                 "          <list> {\n"
                 "            <expression> {\n"
                 "              <integer:5>\n"
                 "            }\n"
                 "          }\n"
                 "        }\n"
                 "      }\n"
                 "      <expression> {\n"
                 "        <path> {\n"
                 "          <string:\"pickle\">\n"
                 "        }\n"
                 "      }\n"
                 "    }\n"
                 "  }\n"
                 "}\n", result) == 0);
    safe_free(result);
        
    parser_parse(parser,
                 "z5 (\"cool\"), pickle \n"
                 );
    result = NULL;
    ast_walk(parser->ast, ast_string_walker, &result);
    CHECK(strcmp("<statement> {\n"
                 "  <path> {\n"
                 "    <string:\"z5\">\n"
                 "    <list> {\n"
                 "      <expression> {\n"
                 "        <string:\"cool\">\n"
                 "      }\n"
                 "      <expression> {\n"
                 "        <path> {\n"
                 "          <string:\"pickle\">\n"
                 "        }\n"
                 "      }\n"
                 "    }\n"
                 "  }\n"
                 "}\n", result) == 0);
    safe_free(result);
    
    CHECK( !parser_parse(parser,
                 "z5 + (2 - -y) = 7 And (bob.theBuilder = \"cool\")\n"
                 ) );
 
    
    return NULL;
}

/*Queue error messages so we can pick the earliest one
 and clear errors if all ok?*/


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
    
    if (!err) err = _test_2();
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


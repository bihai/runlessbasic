/***************************************************************************************************
 *
 * RunlessBASIC
 * Copyright 2013 Joshua Hawcroft <dev@joshhawcroft.com>
 *
 * parser.c
 * Parser for the BASIC programming language.
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
        {
            lexer_get(in_parser->lexer);
            token = lexer_peek(in_parser->lexer, 0);
            if (token.type != TOKEN_IDENTIFIER)
                SYNTAX("Expected identifier");
        }
        else if (token.type == TOKEN_PAREN_LEFT)
        {
            /* parse (...) */
            ast_append(path, _parse_list(in_parser, _parse_expression, False));
            
            /* check for . */
            token = lexer_peek(in_parser->lexer, 0);
            if (token.type != TOKEN_DOT) break;
            lexer_get(in_parser->lexer);
            token = lexer_peek(in_parser->lexer, 0);
            if (token.type != TOKEN_IDENTIFIER)
                SYNTAX("Expected identifier");
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


static AstNode* _parse_dim_identifier(Parser *in_parser)
{
    Token token;
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_IDENTIFIER)
        SYNTAX("Expected identifier");
    return ast_create_string(token.text);
}


static AstNode* _parse_dim(Parser *in_parser)
{
    Token token;
    AstNode *cond, *expr;
    
    /* create the Dim node */
    cond = ast_create(AST_CONTROL);
    ast_append(cond, ast_create_string("dim"));
    
    /* skip the Dim keyword */
    token = lexer_get(in_parser->lexer);
    
    /* check if array dim or normal dim */
    token = lexer_peek(in_parser->lexer, 1);
    if (token.type != TOKEN_PAREN_LEFT)
    {
        /* expect a list of identifiers */
        expr = _parse_list(in_parser, _parse_dim_identifier, True);
        if (!expr) SYNTAX("Expected identifier");
        ast_append(cond, expr);
        
        /* expect As */
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_AS)
            SYNTAX("Expected As");
        
        /* handle New */
        token = lexer_peek(in_parser->lexer, 0);
        if (token.type == TOKEN_NEW)
        {
            lexer_get(in_parser->lexer);
            ast_append(cond, ast_create_operator("new"));
        }

        /* expect type path */
        expr = _parse_path(in_parser);
        if (!expr) SYNTAX("Expected type or class");
        ast_append(cond, expr);
        
        /* handle initalization */
        token = lexer_peek(in_parser->lexer, 0);
        if (token.type == TOKEN_EQUAL)
        {
            lexer_get(in_parser->lexer);
            expr = _parse_expression(in_parser);
            if (!expr) SYNTAX("Expected initalisation expression");
            ast_append(cond, expr);
        }
    }
    else
    {
        /* expect an array identifier */
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_IDENTIFIER)
            SYNTAX("Expected identifier");
        ast_append(cond, ast_create_string(token.text));
        
        /* expect array dimension list */
        expr = _parse_list(in_parser, _parse_expression, False);
        if (!expr) SYNTAX("Expected constant or literal");
        ast_append(cond, expr);
        
        /* expect As */
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_AS)
            SYNTAX("Expected As");
        
        /* expect type path */
        expr = _parse_path(in_parser);
        if (!expr) SYNTAX("Expected type or class");
        ast_append(cond, expr);
    }
    
    /* expect end of line */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_NEW_LINE)
        SYNTAX("Expected end of line");
    
    return cond;
}


static AstNode* _parse_redim(Parser *in_parser)
{
    Token token;
    AstNode *cond, *expr;
    
    /* create the ReDim node */
    cond = ast_create(AST_CONTROL);
    ast_append(cond, ast_create_string("redim"));
    
    /* skip the ReDim keyword */
    token = lexer_get(in_parser->lexer);
    
    /* expect a path */
    expr = _parse_path(in_parser);
    if (!expr) SYNTAX("Expected identifier and new dimensions");
    ast_append(cond, expr);
    
    /* expect end of line */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_NEW_LINE)
        SYNTAX("Expected end of line");
    
    return NULL;
}


static AstNode* _parse_return(Parser *in_parser)
{
    Token token;
    AstNode *ret, *expr;
    
    /* create return node */
    ret = ast_create(AST_CONTROL);
    ast_append(ret, ast_create_string("return"));
    
    /* skip Return */
    lexer_get(in_parser->lexer);
    
    /* handle optional expression */
    token = lexer_peek(in_parser->lexer, 0);
    if (token.type != TOKEN_NEW_LINE)
    {
        expr = _parse_expression(in_parser);
        if (!expr) return NULL;
        ast_append(ret, expr);
    }
    
    /* expect end of line */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_NEW_LINE)
        SYNTAX("Expected end of line");
    
    return ret;
}

/* parsing: a single line statement (already determined to not be a control structure)
 line can be blank or must be a valid statement;
 a valid statement can consist of:
 -  an assignment x = ...
 -  a call ()
 -  a compiler #pragma
 -  control keywords: exit, continue
 -  variable declaration (Dim, Redim)
 -  return
 */
/* TODO: Need to implement parsing of Return statements */
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
        
        token = lexer_peek(in_parser->lexer, 0);
        if ((token.type != TOKEN_NEW_LINE) && (token.type != TOKEN_ELSE))
            SYNTAX("Expected end of line");
    }
    
    /* handle return */
    else if (token.type == TOKEN_RETURN)
    {
        path = _parse_return(in_parser);
        if (!path) return NULL;
        ast_append(stmt, path);
    }
    
    /* handle dim */
    else if (token.type == TOKEN_DIM)
    {
        /* expect end of line */
        path = _parse_dim(in_parser);
        if (!path) return NULL;
        ast_append(stmt, path);
    }
    
    /* handle redim */
    else if (token.type == TOKEN_REDIM)
    {
        /* expect end of line */
        path = _parse_redim(in_parser);
        if (!path) return NULL;
        ast_append(stmt, path);
    }
    
    /* handle exit */
    else if (token.type == TOKEN_EXIT)
    {
        /* expect end of line */
        ast_append(stmt, ast_create_string("break"));
        lexer_get(in_parser->lexer);
        
        token = lexer_peek(in_parser->lexer, 0);
        if ((token.type != TOKEN_NEW_LINE) && (token.type != TOKEN_ELSE))
            SYNTAX("Expected end of line");
    }
    
    /* handle continue */
    else if (token.type == TOKEN_CONTINUE)
    {
        /* expect end of line */
        ast_append(stmt, ast_create_string("continue"));
        lexer_get(in_parser->lexer);
        
        token = lexer_peek(in_parser->lexer, 0);
        if ((token.type != TOKEN_NEW_LINE) && (token.type != TOKEN_ELSE))
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
        token = lexer_peek(in_parser->lexer, 0);
        if ((token.type != TOKEN_NEW_LINE) && (token.type != TOKEN_ELSE))
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
    }
    else
    {
        /* parsing a single line If statement */
        expr = _parse_statement(in_parser);
        if (!expr) return NULL;
        ast_append(cond, expr);
        
        /* handle Else */
        token = lexer_peek(in_parser->lexer, 0);
        if (token.type == TOKEN_ELSE)
        {
            lexer_get(in_parser->lexer);
            expr = _parse_statement(in_parser);
            if (!expr) return NULL;
            ast_append(cond, expr);
        }
        
        /* expect end of line */
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_NEW_LINE)
            SYNTAX("Expected end of line");
    }
    
    return cond;
}


static AstNode* _parse_compiler_if(Parser *in_parser)
{
    /* TODO: implement this later - we don't need it right away;
     and not for an early version of the completed compiler front-end */
    
    return NULL;
}


static AstNode* _parse_select(Parser *in_parser)
{
    Token token;
    AstNode *cond, *expr;
    
    /* create the Select node */
    cond = ast_create(AST_CONTROL);
    ast_append(cond, ast_create_string("select"));
    
    /* skip the Select Case keywords */
    lexer_get(in_parser->lexer);
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_CASE)
        SYNTAX("Expected Case");
    
    /* expect an expression */
    expr = _parse_expression(in_parser);
    if (!expr) SYNTAX("Expected expression here");
    ast_append(cond, expr);
    
    /* expect end of line */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_NEW_LINE)
        SYNTAX("Expected end of line");
    
    /* handle Cases */
    for (;;)
    {
        token = lexer_peek(in_parser->lexer, 0);
        if (token.type == TOKEN_CASE)
        {
            lexer_get(in_parser->lexer);
            
            /* handle Case Else */
            token = lexer_peek(in_parser->lexer, 0);
            if (token.type == TOKEN_ELSE) break;
            
            /* expect an expression */
            expr = _parse_expression(in_parser);
            if (!expr) SYNTAX("Expected case expression");
            ast_append(cond, expr);
            
            /* expect end of line */
            token = lexer_get(in_parser->lexer);
            if (token.type != TOKEN_NEW_LINE) SYNTAX("Expected end of line");
            
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
        
        /* parse block */
        expr = _parse_block(in_parser);
        if (!expr) return NULL;
        ast_append(cond, expr);
    }
    
    /* expect End Select */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_END)
        SYNTAX("Expected End Select");
    token = lexer_peek(in_parser->lexer, 0);
    if (token.type == TOKEN_SELECT)
        lexer_get(in_parser->lexer);
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_NEW_LINE)
        SYNTAX("Expected end of line");
    return cond;
}


static AstNode* _parse_for(Parser *in_parser)
{
    Token token;
    AstNode *cond, *expr;
    
    /* create the For node */
    cond = ast_create(AST_CONTROL);
    
    /* skip the For keyword */
    lexer_get(in_parser->lexer);
    
    /* expect counter variable name or Each keyword */
    token = lexer_get(in_parser->lexer);
    if (token.type == TOKEN_IDENTIFIER)
    {
        /* parse For Next loop */
        ast_append(cond, ast_create_string("for"));
        ast_append(cond, ast_create_string(token.text));
        
        /* expect = */
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_EQUAL)
            SYNTAX("Expected =");
        
        /* expect expression for start */
        expr = _parse_expression(in_parser);
        if (!expr) SYNTAX("Expected counter initalisation expression");
        ast_append(cond, expr);
        
        /* expect To or DownTo */
        token = lexer_get(in_parser->lexer);
        if (token.type == TOKEN_TO)
            ast_append(cond, ast_create_string("increment"));
        else if (token.type == TOKEN_DOWNTO)
            ast_append(cond, ast_create_string("decrement"));
        else
            SYNTAX("Expected To");
        
        /* expect expression for end */
        expr = _parse_expression(in_parser);
        if (!expr) SYNTAX("Expected counter limit expression");
        ast_append(cond, expr);
        
        /* check for Step */
        token = lexer_peek(in_parser->lexer, 0);
        if (token.type == TOKEN_STEP)
        {
            lexer_get(in_parser->lexer);
            expr = _parse_expression(in_parser);
            if (!expr) SYNTAX("Expected step expression");
            ast_append(cond, expr);
        }
        else
        {
            expr = ast_create(AST_EXPRESSION);
            ast_append(expr, ast_create_integer(1));
            ast_append(cond, expr);
        }
        
        /* expect end of line */
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_NEW_LINE)
            SYNTAX("Expected end of line");
        
        /* expect a block */
        expr = _parse_block(in_parser);
        if (!expr) return NULL;
        ast_append(cond, expr);
        
        /* expect Next */
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_NEXT)
            SYNTAX("Expected Next");
        token = lexer_peek(in_parser->lexer, 0);
        if (token.type == TOKEN_IDENTIFIER)
        {
            lexer_get(in_parser->lexer);
            if (!ast_text_is(ast_child(cond, 1), token.text))
                SYNTAX("Counter variable must match For");
        }
        
        /* expect end of line */
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_NEW_LINE)
            SYNTAX("Expected end of line");
    }
    else if (token.type == TOKEN_EACH)
    {
        /* parse For Each loop */
        ast_append(cond, ast_create_string("foreach"));
        
        /* expect local identifier */
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_IDENTIFIER)
            SYNTAX("Expected identifier");
        ast_append(cond, ast_create_string(token.text));
        
        /* expect In */
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_IN)
            SYNTAX("Expected identifier");
        
        /* expect iterable expression */
        expr = _parse_expression(in_parser);
        if (!expr) SYNTAX("Expected iterable expression");
        ast_append(cond, expr);
        
        /* expect end of line */
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_NEW_LINE)
            SYNTAX("Expected end of line");
        
        /* expect a block */
        expr = _parse_block(in_parser);
        if (!expr) return NULL;
        ast_append(cond, expr);
        
        /* expect Next */
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_NEXT)
            SYNTAX("Expected Next");
        
        /* expect end of line */
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_NEW_LINE)
            SYNTAX("Expected end of line");
    }
    else
        SYNTAX("Expected identifier");
    
    return cond;
}


static AstNode* _parse_while(Parser *in_parser)
{
    Token token;
    AstNode *cond, *expr;
    
    /* create the While node */
    cond = ast_create(AST_CONTROL);
    ast_append(cond, ast_create_string("while"));
    
    /* skip the While keyword */
    lexer_get(in_parser->lexer);
    
    /* expect the loop condition */
    expr = _parse_expression(in_parser);
    if (!expr) SYNTAX("Expected conditional expression");
    ast_append(cond, expr);
    
    /* expect end of line */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_NEW_LINE)
        SYNTAX("Expected end of line");
    
    /* expect a block */
    expr = _parse_block(in_parser);
    if (!expr) return NULL;
    ast_append(cond, expr);
    
    /* expect Wend */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_WEND)
        SYNTAX("Expected Wend");
    
    /* expect end of line */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_NEW_LINE)
        SYNTAX("Expected end of line");
    
    return cond;
}


static AstNode* _parse_do(Parser *in_parser)
{
    Token token;
    AstNode *cond, *expr;
    
    /* create the Do node */
    cond = ast_create(AST_CONTROL);
    ast_append(cond, ast_create_string("do"));
    
    /* skip Do */
    lexer_get(in_parser->lexer);
    
    /* handle pre-condition */
    token = lexer_peek(in_parser->lexer, 0);
    if (token.type == TOKEN_UNTIL)
    {
        lexer_get(in_parser->lexer);
        
        /* expect the loop condition */
        expr = _parse_expression(in_parser);
        if (!expr) SYNTAX("Expected conditional expression");
        ast_append(cond, expr);
    }
    
    /* expect end of line */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_NEW_LINE)
        SYNTAX("Expected end of line");
    
    /* expect a block */
    expr = _parse_block(in_parser);
    if (!expr) return NULL;
    ast_append(cond, expr);
    
    /* expect Loop */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_LOOP)
        SYNTAX("Expected Loop");
    
    /* handle post-condition */
    token = lexer_peek(in_parser->lexer, 0);
    if (token.type == TOKEN_UNTIL)
    {
        lexer_get(in_parser->lexer);
        
        /* expect the loop condition */
        expr = _parse_expression(in_parser);
        if (!expr) SYNTAX("Expected conditional expression");
        ast_append(cond, expr);
    }
    
    /* expect end of line */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_NEW_LINE)
        SYNTAX("Expected end of line");
    
    return cond;
}


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
        else if ((token.type == TOKEN_END) || (token.type == TOKEN_ELSE) ||
                 (token.type == TOKEN_CASE) || (token.type == TOKEN_NEXT) ||
                 (token.type == TOKEN_WEND) || (token.type == TOKEN_LOOP))
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


static AstNode* _parse_routine_arg(Parser *in_parser)
{
    Token token;
    AstNode *arg, *result;
    Boolean by_ref;
    
    /* create an argument */
    arg = ast_create(AST_LIST);
    
    /* check for byref/byval */
    by_ref = False;
    token = lexer_peek(in_parser->lexer, 0);
    if ((token.type == TOKEN_BYREF) || (token.type == TOKEN_BYVAL))
    {
        lexer_get(in_parser->lexer);
        if (token.type == TOKEN_BYREF) by_ref = True;
    }
    
    /* expect argument name */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_IDENTIFIER)
        SYNTAX("Expected argument name");
    ast_append(arg, ast_create_string(token.text));
    
    /* handle array designator () */
    token = lexer_peek(in_parser->lexer, 0);
    if (token.type == TOKEN_PAREN_LEFT)
    {
        lexer_get(in_parser->lexer);
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_PAREN_RIGHT)
            SYNTAX("Expected )");
        ast_append(arg, ast_create_string("array"));
    }
    else
    {
        if (by_ref) ast_append(arg, ast_create_string("reference"));
        else ast_append(arg, ast_create_string("value"));
    }
    
    /* expect As */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_AS)
        SYNTAX("Expected As");
    
    /* expect argument type path */
    result = _parse_path(in_parser);
    if (!result) SYNTAX("Expected argument type");
    ast_append(arg, result);

    return arg;
}


static AstNode* _parse_routine(Parser *in_parser)
{
    Token token, token2;
    AstNode *routine, *result, *access, *shared;
    Boolean is_function;
    
    /* create a routine */
    routine = ast_create(AST_CONTROL);
    
    /* read access modifier: Public | Protected | Private */
    token = lexer_get(in_parser->lexer);
    if (token.type == TOKEN_PUBLIC)
        access = ast_create_string("public");
    else if (token.type == TOKEN_PROTECTED)
        access = ast_create_string("protected");
    else if (token.type == TOKEN_PRIVATE)
        access = ast_create_string("private");
    else
        SYNTAX("Expected access modifier");
    
    /* check for Shared */
    token = lexer_peek(in_parser->lexer, 0);
    if (token.type == TOKEN_SHARED)
    {
        lexer_get(in_parser->lexer);
        shared = ast_create_string("class");
    }
    else
        shared = ast_create_string("instance");
    
    /* check if Sub or Function and skip keyword */
    token = lexer_get(in_parser->lexer);
    is_function = (token.type == TOKEN_FUNCTION);
    if (!is_function) ast_append(routine, ast_create_string("subroutine"));
    else ast_append(routine, ast_create_string("function"));
    
    /* expect routine name */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_IDENTIFIER)
    {
        if (!is_function) {
            SYNTAX("Expected subroutine name");
        }
        else {
            SYNTAX("Expected function name");
        }
    }
    ast_append(routine, ast_create_string(token.text));
    
    /* append access modifiers and shared modifier */
    ast_append(routine, access);
    ast_append(routine, shared);
    
    /* handle optional argument list */
    token = lexer_peek(in_parser->lexer, 0);
    if (token.type == TOKEN_PAREN_LEFT)
    {
        result = _parse_list(in_parser, _parse_routine_arg, False);
        if (!result) return NULL;
        ast_append(routine, result);
    }
    
    if (is_function)
    {
        /* expect As */
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_AS)
            SYNTAX("Expected As");
        
        /* expect return type */
        result = _parse_path(in_parser);
        if (!result) return NULL;
        ast_append(routine, result);
    }
    
    /* expect end of line */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_NEW_LINE)
        SYNTAX("Expected end of line");
    
    /* expect block */
    result = _parse_block(in_parser);
    if (!result) return NULL;
    ast_append(routine, result);
    
    /* expect End... */
    token = lexer_get(in_parser->lexer);
    token2 = lexer_get(in_parser->lexer);
    if ( (token.type != TOKEN_END) || ((!is_function) && (token2.type != TOKEN_SUB))
        || ((is_function) && (token2.type != TOKEN_FUNCTION)) )
    {
        if (!is_function)
        {
            SYNTAX("Expected End Sub");
        }
        else
        {
            SYNTAX("Expected End Function");
        }
    }
    
    /* expected end of line */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_NEW_LINE)
        SYNTAX("Expected end of line");
    
    return routine;
}


static AstNode* _parse_property(Parser *in_parser)
{
    /* syntax: [<access-modifier>] [Shared] <name>[(<dimensions>)] As [<package>.]<type> */
    
    Token token;
    AstNode *prop, *access, *shared, *expr;
    
    /* create proeprty */
    prop = ast_create(AST_CONTROL);
    ast_append(prop, ast_create_string("property"));
    
    /* read access modifier: Public | Protected | Private */
    token = lexer_get(in_parser->lexer);
    if (token.type == TOKEN_PUBLIC)
        access = ast_create_string("public");
    else if (token.type == TOKEN_PROTECTED)
        access = ast_create_string("protected");
    else if (token.type == TOKEN_PRIVATE)
        access = ast_create_string("private");
    else
        SYNTAX("Expected access modifier");
    
    /* check for Shared */
    token = lexer_peek(in_parser->lexer, 0);
    if (token.type == TOKEN_SHARED)
    {
        lexer_get(in_parser->lexer);
        shared = ast_create_string("class");
    }
    else
        shared = ast_create_string("instance");
    
    /* expect property name */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_IDENTIFIER)
        SYNTAX("Expected property identifier");
    ast_append(prop, ast_create_string(token.text));
    
    /* append access modifiers and shared modifier */
    ast_append(prop, access);
    ast_append(prop, shared);
    
    /* check for array dimensions */
    token = lexer_peek(in_parser->lexer, 0);
    if (token.type == TOKEN_PAREN_LEFT)
    {
        /* expect array dimension list */
        expr = _parse_list(in_parser, _parse_expression, False);
        if (!expr) SYNTAX("Expected constant or literal");
        ast_append(prop, expr);
    }
    
    /* expect As */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_AS)
        SYNTAX("Expected As");
    
    /* expect type */
    expr = _parse_path(in_parser);
    if (!expr) SYNTAX("Expected type or class");
    ast_append(prop, expr);
    
    /* expect end of line */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_NEW_LINE)
        SYNTAX("Expected end of line");
    
    return prop;
}


static AstNode* _parse_event_decl(Parser *in_parser)
{
    Token token;
    AstNode *event, *result;
    
    /* create event declaration */
    event = ast_create(AST_CONTROL);
    ast_append(event, ast_create_string("event"));
    
    /* skip Event */
    lexer_get(in_parser->lexer);
    
    /* expect event identifier */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_IDENTIFIER) SYNTAX("Expected event identifier");
    ast_append(event, ast_create_string(token.text));
    
    /* handle optional argument list */
    token = lexer_peek(in_parser->lexer, 0);
    if (token.type == TOKEN_PAREN_LEFT)
    {
        result = _parse_list(in_parser, _parse_routine_arg, False);
        if (!result) return NULL;
        ast_append(event, result);
    }
    
    /* handle optional return type */
    token = lexer_peek(in_parser->lexer, 0);
    if (token.type == TOKEN_AS)
    {
        /* expect As */
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_AS)
            SYNTAX("Expected As");
        
        /* expect return type */
        result = _parse_path(in_parser);
        if (!result) return NULL;
        ast_append(event, result);
    }
    
    /* expect end of line */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_NEW_LINE)
        SYNTAX("Expected end of line");
    
    return event;
}


static AstNode* _parse_event_hdlr(Parser *in_parser)
{
    Token token, token2;
    AstNode *event, *result;
    
    /* create event declaration */
    event = ast_create(AST_CONTROL);
    ast_append(event, ast_create_string("handler"));
    
    /* skip Handler */
    lexer_get(in_parser->lexer);
    
    /* expect event identifier */
    token = lexer_get(in_parser->lexer);
    token2 = lexer_peek(in_parser->lexer, 0);
    if ((token.type == TOKEN_IDENTIFIER) && (token2.type == TOKEN_DOT))
    {
        ast_append(event, ast_create_string(token.text));
        lexer_get(in_parser->lexer);
        token = lexer_get(in_parser->lexer);
        if (token.type != TOKEN_IDENTIFIER) SYNTAX("Expected event identifier");
        ast_append(event, ast_create_string(token.text));
    }
    else
    {
        if (token.type != TOKEN_IDENTIFIER) SYNTAX("Expected event identifier");
        ast_append(event, ast_create_string(token.text));
    }
    
    
    /* handle optional argument list */
    token = lexer_peek(in_parser->lexer, 0);
    if (token.type == TOKEN_PAREN_LEFT)
    {
        result = _parse_list(in_parser, _parse_routine_arg, False);
        if (!result) return NULL;
        ast_append(event, result);
    }
    
    /* handle optional return type */
    token = lexer_peek(in_parser->lexer, 0);
    if (token.type == TOKEN_AS)
    {
        /* expect As */
        token = lexer_get(in_parser->lexer);
        
        /* expect return type */
        result = _parse_path(in_parser);
        if ( (!result) || (ast_count(result) == 0) )
            SYNTAX("Expected return type");
        ast_append(event, result);
    }
    
    /* expect end of line */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_NEW_LINE)
        SYNTAX("Expected end of line");
    
    /* expect block */
    result = _parse_block(in_parser);
    if (!result) return NULL;
    ast_append(event, result);
    
    /* expect End Handler */
    token = lexer_get(in_parser->lexer);
    token2 = lexer_get(in_parser->lexer);
    if ((token.type != TOKEN_END) || (token2.type != TOKEN_HANDLER))
        SYNTAX("Expected End Handler");
    
    /* expected end of line */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_NEW_LINE)
        SYNTAX("Expected end of line");

    return event;
}


/* TODO: replace all this general path stuff with something designed to parse package.class */

static AstNode* _parse_class(Parser *in_parser)
{ 
    /* syntax: Class <name> [Inherits [<package>.]<name>] [Implements [<package>.]<name> [, ...]] <EOL> */
    Token token, token2, token3;
    AstNode *class, *routine, *path;
    
    /* create class */
    class = ast_create(AST_CONTROL);
    ast_append(class, ast_create_string("class"));
    
    /* skip Class */
    lexer_get(in_parser->lexer);
    
    /* expect class name */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_IDENTIFIER)
        SYNTAX("Expected class identifier");
    ast_append(class, ast_create_string(token.text));
    
    /* handle Inherits */
    token = lexer_peek(in_parser->lexer, 0);
    if (token.type == TOKEN_INHERITS)
    {
        lexer_get(in_parser->lexer);
        path = _parse_path(in_parser);
        if ((!path) || (ast_count(path) == 0))
            SYNTAX("Expected class identifier");
        ast_append(class, path);
    }

    /* handle Implements */
    token = lexer_peek(in_parser->lexer, 0);
    if (token.type == TOKEN_IMPLEMENTS)
    {
        lexer_get(in_parser->lexer);
        path = _parse_list(in_parser, _parse_path, True);
        if ((!path) || (ast_count(ast_child(path, 0)) == 0))
            SYNTAX("Expected class identifier");
        ast_append(class, path);
    }
    
    /* expect end of line */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_NEW_LINE)
        SYNTAX("Expected end of line");
    
    for (;;)
    {
        token = lexer_peek(in_parser->lexer, 0);
        if ((token.type == TOKEN_FUNCTION) || (token.type == TOKEN_SUB) || (token.type == TOKEN_SHARED))
        {
            SYNTAX("Expected access specifier");
        }
        if (token.type == TOKEN_NEW_LINE)
        {
            lexer_get(in_parser->lexer);
        }
        else if (token.type == TOKEN_END)
        {
            break;
        }
        else if (token.type == TOKEN_EVENT)
        {
            routine = _parse_event_decl(in_parser);
            if (!routine) return NULL;
            ast_append(class, routine);
        }
        else if (token.type == TOKEN_HANDLER)
        {
            routine = _parse_event_hdlr(in_parser);
            if (!routine) return NULL;
            ast_append(class, routine);
        }
        else if ((token.type == TOKEN_PUBLIC) || (token.type == TOKEN_PROTECTED) || (token.type == TOKEN_PRIVATE))
        {
            token2 = lexer_peek(in_parser->lexer, 1);
            token3 = lexer_peek(in_parser->lexer, 2);
            if ((token2.type == TOKEN_SUB) || (token2.type == TOKEN_FUNCTION) ||
                (token3.type == TOKEN_SUB) || (token3.type == TOKEN_FUNCTION))
            {
                routine = _parse_routine(in_parser);
                if (!routine) return NULL;
                ast_append(class, routine);
            }
            else
            {
                routine = _parse_property(in_parser);
                if (!routine) return NULL;
                ast_append(class, routine);
            }
        }
        else
            SYNTAX("Expected subroutine or property");
    }
    
    /* expect End Class */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_END)
        SYNTAX("Expected End");
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_CLASS)
        SYNTAX("Expected Class");
    
    /* expect end of line */
    token = lexer_get(in_parser->lexer);
    if (token.type != TOKEN_NEW_LINE)
        SYNTAX("Expected end of line");
    
    return class;
}


static AstNode* _parse_file(Parser *in_parser)
{
    /* expect class definitions only */
    Token token;
    AstNode *file, *class;
    
    /* create list of file structures */
    file = ast_create(AST_LIST);
    
    /* iterate over file contents */
    for (;;)
    {
        token = lexer_peek(in_parser->lexer, 0);
        if (token.type == TOKEN_CLASS)
        {
            class = _parse_class(in_parser);
            if (!class) return NULL;
            ast_append(file, class);
        }
        else if (token.type == TOKEN_NEW_LINE)
        {
            lexer_get(in_parser->lexer);
        }
        else if (token.offset < 0) break;
        else
            SYNTAX("Expected Class");
    }
    
    return file;
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
    {
        printf("%s: case %d, line %ld: failed: %s\n", in_file, in_case_number, in_line_number, in_error);
        exit(1);
    }
    else
        printf("%s: case %d, line %ld: OK\n", in_file, in_case_number, in_line_number);
}


static const char* _test_case_runner(void *in_user, const char *in_file, int in_case_number, const char *in_input, const char *in_output)
{
    char *result;
    char *err;
    static char err_msg_buffer[1024];
    
    err = NULL;
    
//    if (in_case_number == 4) /* debugging breakpoint hook */
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
    
    test_run_cases(TESTSDIR "parser-statement.tests",
                   _test_case_runner, _test_case_result, NULL);
    
    g_test_parser->init = _parse_block;
    test_run_cases(TESTSDIR "parser-control.tests",
                   _test_case_runner, _test_case_result, NULL);
    
    g_test_parser->init = _parse_file;
    test_run_cases(TESTSDIR "parser-class.tests",
                   _test_case_runner, _test_case_result, NULL);
}


#endif




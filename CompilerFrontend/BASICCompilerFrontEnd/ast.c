//
//  ast.c
//  BASICCompilerFrontEnd
//
//  Created by Joshua Hawcroft on 15/03/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ast.h"
#include "memory.h"


static char* _string_clone(const char *in_string)
{
    char *result;
    long length;
    length = strlen(in_string);
    result = safe_malloc(length + 1);
    strcpy(result, in_string);
    return result;
}


AstNode* ast_create(AstNodeType in_type)
{
    AstNode *node;
    node = safe_malloc(sizeof(struct AstNode));
    memset(node, 0, sizeof(struct AstNode));
    node->type = in_type;
    return node;
}


AstNode* ast_create_string(const char *in_string)
{
    AstNode *node;
    node = ast_create(AST_STRING);
    node->value.string = _string_clone(in_string);
    return node;
}


AstNode* ast_create_integer(long in_integer)
{
    AstNode *node;
    node = ast_create(AST_INTEGER);
    node->value.integer = in_integer;
    return node;
}


AstNode* ast_create_boolean(Boolean in_bool)
{
    AstNode *node;
    node = ast_create(AST_BOOLEAN);
    node->value.integer = in_bool;
    return node;
}


AstNode* ast_create_colour(long in_colour)
{
    AstNode *node;
    node = ast_create(AST_COLOUR);
    node->value.integer = in_colour;
    return node;
}


AstNode* ast_create_real(double in_real)
{
    AstNode *node;
    node = ast_create(AST_REAL);
    node->value.real = in_real;
    return node;
}


AstNode* ast_create_operator(const char *in_desc)
{
    AstNode *node;
    node = ast_create(AST_OPERATOR);
    node->value.string = _string_clone(in_desc);
    return node;
}


void ast_append(AstNode *in_parent, AstNode *in_child)
{
    in_parent->value.list.nodes = safe_realloc(in_parent->value.list.nodes,
                                               sizeof(AstNode*) * (in_parent->value.list.count + 1));
    in_parent->value.list.nodes[ in_parent->value.list.count++ ] = in_child;
}


static Boolean _ast_walk_int(AstNode *in_node, Boolean (*in_walker)(AstNode*, Boolean, int), int in_level)
{
    int i;
    if (in_walker(in_node, False, in_level)) return True;
    switch (in_node->type)
    {
        case AST_LIST:
        case AST_PATH:
        case AST_STATEMENT:
        case AST_EXPRESSION:
            for (i = 0; i < in_node->value.list.count; i++)
            {
                if (_ast_walk_int(in_node->value.list.nodes[i], in_walker, in_level+1)) return True;
            }
            break;
        default: break;
    }
    if (in_walker(in_node, True, in_level)) return True;
    return False;
}


void ast_walk(AstNode *in_tree, Boolean (*in_walker)(AstNode*, Boolean, int))
{
    _ast_walk_int(in_tree, in_walker, 0);
}


static const char* _ast_padding(int in_amount)
{
    if (in_amount > 99)
        in_amount = 99;
    static char whitespace[1000] =
    "                                                                                                    "
    "                                                                                                    "
    "                                                                                                    "
    "                                                                                                    "
    "                                                                                                    "
    "                                                                                                    "
    "                                                                                                    "
    "                                                                                                    "
    "                                                                                                    "
    "                                                                                                    ";
    static int last_amount = 0;
    whitespace[last_amount] = ' ';
    last_amount = in_amount;
    whitespace[last_amount] = 0;
    return whitespace;
}


Boolean ast_debug_walker(AstNode *in_node, Boolean in_end, int in_level)
{
    if (in_end)
    {
        switch (in_node->type)
        {
            case AST_STATEMENT:
            case AST_PATH:
            case AST_LIST:
            case AST_EXPRESSION:
                printf("%s}\n", _ast_padding(in_level * 2));
                break;
            default:
                break;
        }
        return False;
    }
    printf("%s<", _ast_padding(in_level * 2));
    switch (in_node->type)
    {
        case AST_STATEMENT:
            printf("statement");
            break;
        case AST_PATH:
            printf("path");
            break;
        case AST_LIST:
            printf("list");
            break;
        case AST_EXPRESSION:
            printf("expression");
            break;
            
        case AST_NULL:
            printf("null>\n");
            return False;
        case AST_STRING:
            printf("string:\"%s\">\n", in_node->value.string);
            return False;
        case AST_INTEGER:
            printf("integer: %ld>\n", in_node->value.integer);
            return False;
        case AST_REAL:
            printf("real: %fd>\n", in_node->value.real);
            return False;
        case AST_OPERATOR:
            printf("operator: %s>\n", in_node->value.string);
            return False;
        case AST_COLOUR:
            printf("colour: %ld>\n", in_node->value.integer);
            return False;
        case AST_BOOLEAN:
            printf("boolean: %s>\n", ((in_node->value.integer)?"true":"false"));
            return False;
        default:
            if (!in_end) printf("unknown>\n");
            return False;
    }
    printf("> {\n");
    return False;
}





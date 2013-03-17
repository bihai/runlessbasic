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
#include "test.h"


struct AstNode
{
    AstNodeType     type;
    union
    {
        struct
        {
            int             count;
            AstNode         **nodes;
        }               list;
        char            *string;
        long            integer;
        double          real;
    }               value;
};



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


static Boolean _ast_walk_int(AstNode *in_node, AstWalker in_walker, int in_level, void *io_user)
{
    int i;
    if (!in_node) return False;
    if (in_walker(in_node, False, in_level, io_user)) return True;
    switch (in_node->type)
    {
        case AST_LIST:
        case AST_PATH:
        case AST_STATEMENT:
        case AST_EXPRESSION:
            for (i = 0; i < in_node->value.list.count; i++)
            {
                if (_ast_walk_int(in_node->value.list.nodes[i], in_walker, in_level+1, io_user)) return True;
            }
            break;
        default: break;
    }
    if (in_walker(in_node, True, in_level, io_user)) return True;
    return False;
}


void ast_walk(AstNode *in_tree, AstWalker in_walker, void *io_user)
{
    _ast_walk_int(in_tree, in_walker, 0, io_user);
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


static const char* _ast_node_desc(AstNode *in_node, Boolean in_end, int in_level)
{
    static char buffer[2048];
    long offset;
    
    buffer[0] = 0;
    
    if (in_end)
    {
        switch (in_node->type)
        {
            case AST_STATEMENT:
            case AST_PATH:
            case AST_LIST:
            case AST_EXPRESSION:
                sprintf(buffer, "%s}\n", _ast_padding(in_level * 2));
                break;
            default:
                break;
        }
        return buffer;
    }
    
    offset = sprintf(buffer, "%s<", _ast_padding(in_level * 2));
    switch (in_node->type)
    {
        case AST_STATEMENT:
            offset += sprintf(buffer + offset, "statement");
            break;
        case AST_PATH:
            offset += sprintf(buffer + offset, "path");
            break;
        case AST_LIST:
            offset += sprintf(buffer + offset, "list");
            break;
        case AST_EXPRESSION:
            offset += sprintf(buffer + offset, "expression");
            break;
            
        case AST_NULL:
            offset += sprintf(buffer + offset, "null>\n");
            return buffer;
        case AST_STRING:
            offset += sprintf(buffer + offset, "string:\"%s\">\n", in_node->value.string);
            return buffer;
        case AST_INTEGER:
            offset += sprintf(buffer + offset, "integer:%ld>\n", in_node->value.integer);
            return buffer;
        case AST_REAL:
            offset += sprintf(buffer + offset, "real:%fd>\n", in_node->value.real);
            return buffer;
        case AST_OPERATOR:
            offset += sprintf(buffer + offset, "operator:%s>\n", in_node->value.string);
            return buffer;
        case AST_COLOUR:
            offset += sprintf(buffer + offset, "colour:%ld>\n", in_node->value.integer);
            return buffer;
        case AST_BOOLEAN:
            offset += sprintf(buffer + offset, "boolean:%s>\n", ((in_node->value.integer)?"true":"false"));
            return buffer;
        default:
            if (!in_end) printf(buffer + offset, "unknown>\n");
            return buffer;
    }
    offset += sprintf(buffer + offset, "> {\n");
    
    return buffer;
}


Boolean ast_string_walker(AstNode *in_node, Boolean in_end, int in_level, void *io_user)
{
    const char *text;
    long text_length, string_length;
    
    text = _ast_node_desc(in_node, in_end, in_level);
    text_length = strlen(text);
    
    if (! (*(char**)io_user)) string_length = 0;
    else string_length = strlen((const char *)*(char**)io_user);
    *(char**)io_user = safe_realloc(*(char**)io_user, text_length + string_length + 1);
    strcpy(*(char**)io_user + string_length, text);
    
    return False;
}


Boolean ast_debug_walker(AstNode *in_node, Boolean in_end, int in_level, void *io_user)
{
    const char *text;
    text = _ast_node_desc(in_node, in_end, in_level);
    printf("%s", text);
    return False;
}


static Boolean _has_list(AstNode *in_node)
{
    switch (in_node->type)
    {
        case AST_EXPRESSION:
        case AST_LIST:
        case AST_PATH:
        case AST_STATEMENT:
            return True;
        default:
            return False;
    }
}


static Boolean _has_string(AstNode *in_node)
{
    switch (in_node->type)
    {
        case AST_OPERATOR:
        case AST_STRING:
            return True;
        default:
            return False;
    }
}


void ast_dispose(AstNode *in_tree)
{
    int i;
    if (!in_tree) return;
    if (_has_list(in_tree))
    {
        for (i = 0; i < in_tree->value.list.count; i++)
            ast_dispose(in_tree->value.list.nodes[i]);
    }
    else if (_has_string(in_tree))
    {
        if (in_tree->value.string)
            safe_free(in_tree->value.string);
    }
    safe_free(in_tree);
}


AstNode* ast_child(AstNode *in_node, int in_child)
{
    assert(in_node);
    assert(in_child >= -1);
    if (!_has_list(in_node)) return NULL;
    if (in_node->value.list.count == 0) return NULL;
    if (in_child == AST_LAST)
        in_child = in_node->value.list.count-1;
    else if (in_child >= in_node->value.list.count)
        return NULL;
    return in_node->value.list.nodes[in_child];
}


AstNode* ast_remove(AstNode *in_node, int in_child)
{
    assert(in_node);
    assert(in_child >= -1);
    
    AstNode *result;
    
    if (!_has_list(in_node)) return NULL;
    if (in_node->value.list.count == 0) return NULL;
    if (in_child == AST_LAST)
        in_child = in_node->value.list.count-1;
    else if (in_child >= in_node->value.list.count)
        return NULL;
    
    result = in_node->value.list.nodes[in_child];
    
    /* don't actually remove the child, just insert a NULL pointer,
     our walker function will skip over this anyway and our tree should
     be short-lived enough that it doesn't matter */
    in_node->value.list.nodes[in_child] = NULL;
    
    return result;
}


Boolean ast_is(AstNode *in_node, AstNodeType in_type)
{
    if (!in_node)
        return (in_type == AST_NULL);
    else
        return (in_node->type == in_type);
}


void ast_insert(AstNode *in_node, int in_before, AstNode *in_child)
{
    assert(in_node);
    assert(in_before >= 0);
    assert(in_child);
    
    in_node->value.list.nodes = safe_realloc(in_node->value.list.nodes,
                                             sizeof(AstNode*) * (++in_node->value.list.count));
    
    if (in_before < in_node->value.list.count-1)
        memmove(in_node->value.list.nodes + in_before + 1, in_node->value.list.nodes + in_before,
                sizeof(AstNode*) * ( in_node->value.list.count - in_before - 1 ));
    else
        in_before = in_node->value.list.count-1;
    
    in_node->value.list.nodes[ in_before ] = in_child;
}


void ast_prepend(AstNode *in_node, AstNode *in_child)
{
    ast_insert(in_node, 0, in_child);
}


int ast_count(AstNode *in_node)
{
    return in_node->value.list.count;
}


/* TODO: write tests for AST module and include assertions,
  finish sanity checks in functions and decide what level to include */




//
//  ast.h
//  BASICCompilerFrontEnd
//
//  Created by Joshua Hawcroft on 15/03/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#include "memory.h"

#ifndef _AST_H
#define _AST_H


typedef enum {
    AST_NULL,
    AST_STATEMENT,
    AST_PATH,
    AST_LIST,
    AST_EXPRESSION,
    AST_STRING,
    AST_INTEGER,
    AST_REAL,
    AST_OPERATOR,
    AST_COLOUR,
    AST_BOOLEAN,
    
} AstNodeType;


struct AstNode;
typedef struct AstNode AstNode;

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



AstNode* ast_create(AstNodeType in_type);
AstNode* ast_create_string(const char *inString);
AstNode* ast_create_operator(const char *in_desc);
AstNode* ast_create_integer(long in_integer);
AstNode* ast_create_boolean(Boolean in_bool);
AstNode* ast_create_colour(long in_colour);
AstNode* ast_create_real(double in_real);
void ast_append(AstNode *in_parent, AstNode *in_child);

void ast_walk(AstNode *in_tree, Boolean (*in_walker)(AstNode *in_node, Boolean in_end, int in_level));

Boolean ast_debug_walker(AstNode *in_node, Boolean in_end, int in_level);


#endif

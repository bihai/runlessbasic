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

/*
 AST_STATEMENT:
 
    Contains only: (<path> [<expression>]) | <string:"Continue"> | <string:"Exit"> | 
        (<string:"Pragma"> <string:<identifier>> <string:<value>>)
 
    If it's just a <path> - it must be executable and not return a value.
    If it includes an <expression>, it should be assigned to the writable property given by <path>.
    (ie. the expression must return a value and the path must be a property or variable.
 
 
 AST_PATH:
 
    Takes the form: <string> [<list>] ...
    <string> is the name of the variable, constant, namespace, class, property, target, function or method.
    <list> is an optional list of indicies or arguments to index an array or pass to a function/method.
 
 AST_EXPRESSION:
 
    Takes the form: <unary-operator> <operand1> | <operand1> [<binary-operator> <operand2> ...]
    <unary-operator> will be either: new or not
    operators are just like strings.
    <operandN> may be another expression, a path or a literal
 */


struct AstNode;
typedef struct AstNode AstNode;


AstNode* ast_create(AstNodeType in_type);
AstNode* ast_create_string(const char *inString);
AstNode* ast_create_operator(const char *in_desc);
AstNode* ast_create_integer(long in_integer);
AstNode* ast_create_boolean(Boolean in_bool);
AstNode* ast_create_colour(long in_colour);
AstNode* ast_create_real(double in_real);
void ast_append(AstNode *in_parent, AstNode *in_child);

typedef Boolean (*AstWalker) (AstNode *in_node, Boolean in_end, int in_level, void *io_user);

void ast_walk(AstNode *in_tree, AstWalker in_walker, void *io_user);

Boolean ast_debug_walker(AstNode *in_node, Boolean in_end, int in_level, void *io_user);

/* io_user should be a char** and initalized to NULL before the call to ast_walk();
 upon return it will contain the text representation of the AST */
Boolean ast_string_walker(AstNode *in_node, Boolean in_end, int in_level, void *io_user);

void ast_dispose(AstNode *in_tree);

enum {
    AST_FIRST = 0,
    AST_LAST = -1,
};

AstNode* ast_child(AstNode *in_node, int in_child);
AstNode* ast_remove(AstNode *in_node, int in_child);
void ast_insert(AstNode *in_node, int in_before, AstNode *in_child);
void ast_prepend(AstNode *in_node, AstNode *in_child);

Boolean ast_is(AstNode *in_node, AstNodeType in_type);

int ast_count(AstNode *in_node);



#endif

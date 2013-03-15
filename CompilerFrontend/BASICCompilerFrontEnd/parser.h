/*
 * parser.h
 * BASIC Parser
 *
 * (c) 2013 Joshua Hawcroft
 *
 */

#include "ast.h"

#ifndef _PARSER_H
#define _PARSER_H


typedef struct Parser Parser;

Parser* parser_create(void);

void parser_parse(Parser *in_parser, char *in_source);


#ifdef DEBUG

void parser_run_tests();

#endif


#endif

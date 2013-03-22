/***************************************************************************************************
 *
 * RunlessBASIC
 * Copyright 2013 Joshua Hawcroft <dev@joshhawcroft.com>
 *
 * run-tool.c
 * Main entry point for the compiler.
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

#include "parser.h"
#include "index.h"
#include "readfile.h"
#include "memory.h"


int main(int argc, const char * argv[])
{
    Index *index;
    Parser *parser;
    char *source;
    AstNode *ast;
    
    
    /* for testing, currently assumed to be in indexing mode as if invoked with appropriate
     command line arguments */
    
    source = readfile("/Users/josh/Desktop/test.bas");
    
    index = index_open("/Users/josh/Desktop/test.index");
    
    parser = parser_create();
    
    if (!parser_parse(parser, source))
    {
        fail(parser_error_message(parser)); /* need fail to support arguments like printf! */
    }
    
    ast = parser_ast(parser);
    
    ast_walk(ast, ast_debug_walker, NULL);
    
    /* 
     need to check the modification date of the file against the one we stored when it was last
     indexed.  if same, exit this process.  otherwise, continue...
     
     need to walk the AST and index all the symbols into the sym table of the index 
     */
    
    
    index_close(index);
    
    return 0;
}


// compilation to Obj-C & Cocoa is done in two phases:
// 1) index each file
// 2) translate each file

// compilation direct to machine-code or JVM, etc.
// will follow a similar pattern in future but will probably include a packaging/linking
// phase to combine all the individual file/outputs


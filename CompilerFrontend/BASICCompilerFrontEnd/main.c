/*
 * parser.h
 * BASIC Compiler Front-end
 *
 * (c) 2013 Joshua Hawcroft
 *
 */

#include <stdio.h>


#include "parser.h"


int main(int argc, const char * argv[])
{
    //Lexer *theLexer;
    //Token tok;
    

    parser_run_tests();
    
    //parser_parse(parser_create(), "Dim x As Integer");
    
    /*theLexer = lexer_create("Dim test As Integer ' this is a comment\r\nDim y As String = \"Hello &u0022cruel\"\" World!\"\r\nConst bed = 3e2 (\r\n");
    return 0;
    do {
        tok = lexer_get_next_token(theLexer);
        lexer_debug_token(tok);
        
    } while (tok.offset >= 0);

    printf("Done\n");*/
    
    return 0;
}


/*
 
 Quoted strings,
 numbers - including hexadecimal constants, scientific notation, etc.
 comments
 
 
 Syntax examples
 
 Class <identifier> Inherits <identifier> Implements <identifier>
 
     <access modifier> [Static] <identifier> [()] As <class>
     
     <access modifier> [Static] Function <identifier> ( <identifier> [()] As <class>, ... ) As <class> [()]
     
        <statements>
 
***
        <expression returning void>
            outer parenthesis are optional
 
        <expression writable> = <expression returning non-void>
            = is outside of any parenthesis

Can we parse without doing multiple passes of the token stream?
 
Need to look at all the possibilities and see what we have... each case...
 
 LocalOrLangMethodName().etc
 LocalVariable.ObjectPropertyOrMethodCall().etc
 Me.ObjectPropertyOrMethodCall().etc
 Self.ObjectPropertyOrMethodCal().etc
 
***
 
        If <conditional expression> Then
 
        ElseIf <conditional expression> Then
 
        Else
 
        EndIf
 
        Select Case <expression>
        Case <expression>
        End Select
 
        For <writable expression> = <init expression> To / DownTo <final expression> Step <expr>
 
        Next <writable expression>
 
        While <cond>
 
        Wend
 
        Do
 
        Until <cond>
 
        Do
 
        Loop
 
        
     
     End Function
 
 End Class
 
 
 Mandatory namespaces on all extension modules/plug-ins.  Optional namespace Lang on built-in stuff.
 No namespace on your actual project.
 
 Maybe eventually support type restrictions on collections classes - ie. Dim x As Dictionary<Integer>
 which enables you to only accept Integers in the dictionary, and to assume an Integer when dealing
 with it (without casting or type checks).  A kind of C++ templates mechanism only not extending
 to other stuff.
 
 */

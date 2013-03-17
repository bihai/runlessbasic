//
//  test.c
//  BASICCompilerFrontEnd
//
//  Created by Joshua Hawcroft on 17/03/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"

#ifdef DEBUG


#define MAX_LINE_READ 4096





const char* test_run_cases(const char *in_filename, TestCaseRunner in_case_runner, TestCaseResult in_result_handler, void *in_user)
{
    FILE *fp;
    char line[MAX_LINE_READ];
    char *input, *output, **buffer;
    long len_buffer, len_line;
    const char *result;
    int case_number;
    long line_number;
    
    input = NULL;
    output = NULL;
    buffer = NULL;
    result = NULL;
    case_number = 0;
    line_number = 0;
    
    fp = fopen(in_filename, "r");
    while (!feof(fp))
    {
        if (!fgets(line, MAX_LINE_READ, fp)) break;
        line_number++;
        
        if (memcmp(line, "####INPUT", 9) == 0)
        {
            buffer = &input;
            if (input) free(input);
            input = malloc(1);
            input[0] = 0;
        }
        else if (memcmp(line, "####OUTPUT", 10) == 0)
        {
            buffer = &output;
            if (output) free(output);
            output = malloc(1);
            output[0] = 0;
        }
        else if (memcmp(line, "####TEST", 8) == 0)
        {
            /* run the test */
            case_number++;
            buffer = NULL;
            len_buffer = (input?strlen(input):0);
            if (len_buffer > 0)
                input[len_buffer-1] = 0;
            len_buffer = (output?strlen(output):0);
            if (len_buffer > 0)
                output[len_buffer-1] = 0;
            
            result = in_case_runner(in_user, case_number, input, output);
            if (result)
            {
                in_result_handler(in_user, case_number, line_number, result);
                break;
            }
            in_result_handler(in_user, case_number, line_number, NULL);
        }
        else if (buffer)
        {
            /* append line to buffer */
            len_buffer = (*buffer?strlen(*buffer):0);
            len_line = strlen(line);
            *buffer = realloc(*buffer, len_buffer + len_line + 1);
            strcpy(*buffer + len_buffer, line);
        }
    }
    
    fclose(fp);
    return result;
}





#endif



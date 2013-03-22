/***************************************************************************************************
 *
 * RunlessBASIC
 * Copyright 2013 Joshua Hawcroft <dev@joshhawcroft.com>
 *
 * test.c
 * Simple automated testing framework.
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
            
            result = in_case_runner(in_user, in_filename, case_number, input, output);
            if (result)
            {
                in_result_handler(in_user, in_filename, case_number, line_number, result);
                break;
            }
            in_result_handler(in_user, in_filename, case_number, line_number, NULL);
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



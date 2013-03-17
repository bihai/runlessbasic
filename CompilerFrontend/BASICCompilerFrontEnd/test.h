//
//  test.h
//  BASICCompilerFrontEnd
//
//  Created by Joshua Hawcroft on 15/03/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#include <assert.h>

#ifndef _TEST_H
#define _TEST_H

#ifdef DEBUG

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)
#define CHECK(cond) if (!(cond)) return "Condition not satisfied at line " LINE_STRING;

typedef const char* (*TestCaseRunner)(void *in_user, int in_case_number, const char *in_input, const char *in_output);
typedef void (*TestCaseResult)(void *in_user, int in_case_number, long in_line_number, const char *in_error);
const char* test_run_cases(const char *in_filename, TestCaseRunner in_case_runner, TestCaseResult in_result_handler, void *in_user);

#endif

#endif

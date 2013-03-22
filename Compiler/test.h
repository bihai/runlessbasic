/***************************************************************************************************
 *
 * RunlessBASIC
 * Copyright 2013 Joshua Hawcroft <dev@joshhawcroft.com>
 *
 * test.h
 * (see C source file for details)
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

#include <assert.h>

#ifndef _TEST_H
#define _TEST_H

#ifdef DEBUG

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)
#define CHECK(cond) if (!(cond)) return "Condition not satisfied at line " LINE_STRING;

typedef const char* (*TestCaseRunner)(void *in_user, const char *in_file, int in_case_number, const char *in_input, const char *in_output);
typedef void (*TestCaseResult)(void *in_user, const char *in_file, int in_case_number, long in_line_number, const char *in_error);
const char* test_run_cases(const char *in_filename, TestCaseRunner in_case_runner, TestCaseResult in_result_handler, void *in_user);

#endif

#endif

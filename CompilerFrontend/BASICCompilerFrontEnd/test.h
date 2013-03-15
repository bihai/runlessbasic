//
//  test.h
//  BASICCompilerFrontEnd
//
//  Created by Joshua Hawcroft on 15/03/13.
//  Copyright (c) 2013 Joshua Hawcroft. All rights reserved.
//

#ifndef _TEST_H
#define _TEST_H

#ifdef DEBUG

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)
#define CHECK(cond) if (!(cond)) return "Condition not satisfied at line " LINE_STRING;

#endif

#endif

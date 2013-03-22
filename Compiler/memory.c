/***************************************************************************************************
 *
 * RunlessBASIC
 * Copyright 2013 Joshua Hawcroft <dev@joshhawcroft.com>
 *
 * memory.c
 * Utilities to handle memory allocations and debugging safely within the project.
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
#include <stdarg.h>

static long gFrees = 0;
static void* gLastPtr = NULL;


void fail(const char *in_msg)
{
    fprintf(stderr, "RunlessBASIC: fatal: %s\n", in_msg);
    abort();
}

/*
void fail(const char *in_msg, ...)
{
    va_list args;
    void *arg;
    int i;
    
    va_start(args, in_msg);
    while ((arg = va_arg(args,))
    va_end(args);
    
}*/


void* safe_malloc(long inSize)
{
    void *outMemory;
    outMemory = malloc(inSize);
    if (!outMemory) fail("Out of memory");
#ifdef DEBUG
    gLastPtr = outMemory;
#endif
    return outMemory;
}


void* safe_realloc(void* in_memory, long in_new_size)
{
    void *out_memory;
    out_memory = realloc(in_memory, in_new_size);
    if (!out_memory) fail("Out of memory");
#ifdef DEBUG
    gLastPtr = out_memory;
#endif
    return out_memory;
}


void safe_free(void *in_memory)
{
#ifdef DEBUG
    gFrees++;
    gLastPtr = in_memory;
#endif
    free(in_memory);
}


#ifdef DEBUG

long debug_memory_frees(void)
{
    return gFrees;
}

void* debug_memory_last_ptr(void)
{
    return gLastPtr;
}

#endif




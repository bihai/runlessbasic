/*
 * memory.c
 * Memory utilities
 *
 * (c) 2013 Joshua Hawcroft
 *
 */

#include <stdio.h>
#include <stdlib.h>


static long gFrees = 0;
static void* gLastPtr = NULL;


void* safe_malloc(long inSize)
{
    void *outMemory;
    outMemory = malloc(inSize);
    if (!outMemory)
    {
        fprintf(stderr, "Out of memory.\n");
        abort();
    }
#ifdef DEBUG
    gLastPtr = outMemory;
#endif
    return outMemory;
}


void* safe_realloc(void* in_memory, long in_new_size)
{
    void *out_memory;
    out_memory = realloc(in_memory, in_new_size);
    if (!out_memory)
    {
        fprintf(stderr, "Out of memory.\n");
        abort();
    }
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




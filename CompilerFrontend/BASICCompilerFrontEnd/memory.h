/*
 * memory.h
 * Memory utilities
 *
 * (c) 2013 Joshua Hawcroft
 *
 */


#ifndef BASICCompilerFrontEnd_memory_h
#define BASICCompilerFrontEnd_memory_h


void* safe_malloc(long inSize);
void* safe_realloc(void* in_memory, long in_new_size);

void safe_free(void *in_memory);


typedef enum 
{
    False = 0,
    True = 1
} Boolean;



#ifdef DEBUG
long debug_memory_frees(void);
void* debug_memory_last_ptr(void);
#endif


#endif

/***************************************************************************************************
 *
 * RunlessBASIC
 * Copyright 2013 Joshua Hawcroft <dev@joshhawcroft.com>
 *
 * memory.h
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

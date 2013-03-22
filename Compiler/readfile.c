/***************************************************************************************************
 *
 * RunlessBASIC
 * Copyright 2013 Joshua Hawcroft <dev@joshhawcroft.com>
 *
 * readfile.c
 * Read an entire file into memory (up to a maximum file size.)
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
#include <string.h>
#include <stdlib.h>

#include "memory.h"

#define MAX_SOURCE_FILE_SIZE        250 * 1024 * 1024


char* readfile(const char *in_pathname)
{
    FILE    *fh;
    long    size;
    char    *buffer;
    size_t  bytes;
    
    /* attempt to open file */
    fh = fopen(in_pathname, "rb");
    if (!fh) fail("Can't open file for reading");
    
    /* determine file size */
    fseek(fh, 0, SEEK_END);
    size = ftell(fh);
    rewind(fh);
    
    /* check file size */
    if (size > MAX_SOURCE_FILE_SIZE)
        fail("Out of memory reading file");
    
    /* allocate memory to contain the entire file */
    buffer = safe_malloc(size + 1);
    
    /* read the file into the buffer */
    bytes = fread(buffer, 1, size, fh);
    if (size != bytes)
        fail("Couldn't read whole file");
    
    /* close file */
    fclose(fh);
    
    /* return the result */
    buffer[size] = 0;
    return buffer;
}





/***************************************************************************************************
 *
 * RunlessBASIC
 * Copyright 2013 Joshua Hawcroft <dev@joshhawcroft.com>
 *
 * index.h
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

#ifndef rlb_index_h
#define rlb_index_h

// database:

// header table
// allow checking when the database was last written
// by which version of the compiler, etc.

// file table, track what has been added/edited/removed
// symbol table, track non-local names
// search table, full-text index of file contents

// symbol and search related back to the file table
// if a file is edited/removed, the relevant data must be recomputed/purged


struct Index;
typedef struct Index Index;

Index* index_open(const char *in_path);
void index_close(Index *in_index);




#endif

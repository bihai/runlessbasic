/***************************************************************************************************
 *
 * RunlessBASIC
 * Copyright 2013 Joshua Hawcroft <dev@joshhawcroft.com>
 *
 * index.c
 * Compiler generated index of symbols and freetext for a project, or symbols for a built-in
 * API or modular extension API.
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

#include <stdlib.h>
#include <string.h>

#include "sqlite/sqlite3.h"

#include "index.h"
#include "memory.h"
#include "rlb.h"


struct Index
{
    sqlite3 *db;
    long build;
};


static void _index_init(Index *in_index)
{
    int             err;
    
    err = sqlite3_exec(in_index->db,
                       "CREATE TABLE rlb ("
                       " signature TEXT PRIMARY KEY,"
                       " version_major INTEGER,"
                       " version_minor INTEGER,"
                       " version_bugfix INTEGER,"
                       " build INTEGER"
                       ")",
                       NULL,NULL,NULL);
    if (err != SQLITE_OK) fail("Couldn't initalise index (1)");
    
    err = sqlite3_exec(in_index->db,
                       "INSERT INTO rlb VALUES ("
                       " '" RLB "-Compiler-Index',"
                       " " RLB_VERSION_MAJOR_S ","
                       " " RLB_VERSION_MINOR_S ","
                       " " RLB_VERSION_BUG_S ","
                       " 1"
                       ")",
                       NULL,NULL,NULL);
    if (err != SQLITE_OK) fail("Couldn't initalise index (2)");
    
    err = sqlite3_exec(in_index->db,
                       "CREATE TABLE file ("
                       " id INTEGER PRIMARY KEY AUTOINCREMENT,"
                       " pathname TEXT UNIQUE,"
                       " build INTEGER"
                       ")",
                       NULL,NULL,NULL);
    if (err != SQLITE_OK) fail("Couldn't initalise index (3)");
    
    err = sqlite3_exec(in_index->db,
                       "CREATE TABLE sym ("
                       " id INTEGER PRIMARY KEY AUTOINCREMENT,"
                       " file_id INTEGER,"
                       " name TEXT,"
                       " type TEXT,"
                       " parent_id INTEGER"
                       ")",
                       NULL,NULL,NULL);
    if (err != SQLITE_OK) fail("Couldn't initalise index (4)");
}


static void _check_signature(Index *in_index)
{
    int             err;
    sqlite3_stmt    *stmt;
    
    err = sqlite3_prepare_v2(in_index->db,
                             "SELECT build FROM rlb WHERE signature=?1 AND version_major = ?2 "
                             "AND version_minor = ?3 AND version_bugfix = ?4",
                             -1,
                             &stmt,
                             NULL);
    if (err != SQLITE_OK) fail("Couldn't check index signature (1)");
    err = sqlite3_bind_text(stmt, 1, RLB "-Compiler-Index", -1, SQLITE_STATIC);
    if (err != SQLITE_OK) fail("Couldn't check index signature (2)");
    err = sqlite3_bind_int(stmt, 2, RLB_VERSION_MAJOR);
    if (err != SQLITE_OK) fail("Couldn't check index signature (3)");
    err = sqlite3_bind_int(stmt, 3, RLB_VERSION_MINOR);
    if (err != SQLITE_OK) fail("Couldn't check index signature (4)");
    err = sqlite3_bind_int(stmt, 4, RLB_VERSION_BUG);
    
    err = sqlite3_step(stmt);
    if (err != SQLITE_ROW) fail("Couldn't create/update index; index file is invalid");
    
    in_index->build = sqlite3_column_int(stmt, 0);
    
    sqlite3_finalize(stmt);
}


Index* index_open(const char *in_path)
{
    Index   *index;
    int     err;
    
    index = safe_malloc(sizeof(struct Index));
    
    err = sqlite3_open_v2(in_path, &(index->db), SQLITE_OPEN_READWRITE, NULL);
    if (err != SQLITE_OK)
    {
        err = sqlite3_open_v2(in_path, &(index->db), SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
        if (err != SQLITE_OK) return NULL;
        
        _index_init(index);
    }
    
    _check_signature(index);
    
    return index;
}


void index_close(Index *in_index)
{
    sqlite3_close_v2(in_index->db);
    safe_free(in_index);
}


/*err = sqlite3_prepare_v2(in_index->db,
 "CREATE TABLE file ("
 " id INTEGER PRIMARY KEY AUTOINCREMENT, "
 " pathname TEXT UNIQUE"
 ")",
 -1,
 &stmt,
 NULL);
 if (err != SQLITE_OK) fail("Couldn't initalise index (1)");
 err = sqlite3_step(stmt);
 if (err != SQLITE_DONE) fail("Couldn't initalise index (2)");
 
 
 
 if (strcmp((const char*)sqlite3_column_text(stmt, 0), RLB "-Compiler-Index") != 0)
 fail("Couldn't create/update index; file already exists");
 
 sqlite3_finalize(stmt);*/






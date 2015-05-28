/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1996-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
#ifndef HSPARSER_H
#define HSPARSER_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         hs_parser.h
 * Description:  Functions used by parser.
 * Created:      04/27/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "hs_globals.h"

Lng32 HSFuncParseStmt();

Lng32 AddTableName( const hs_table_type type,
                   const char *table   = NULL,
                   const char *schema  = NULL,
                   const char *catalog = NULL
                 );

Lng32 AddEveryColumn(const char *startColumn = NULL, 
                    const char *endColumn   = NULL);
Lng32 AddKeyGroups();
Lng32 AddSaltToIndexPrefixes();
Lng32 AddColumnSet(HSColSet &colSet);
Lng32 AddExistingColumns();
NABoolean ColumnExists(const Lng32 colNumber);
NABoolean GroupExists(HSColSet &colSet);

#endif /* HSPARSER_H */

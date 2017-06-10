/* -*-C++-*-
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@
 *****************************************************************************
 *
 * File:         readRealArk.C
 * Description:  This module converts SOL based meta-data to TrafDesc
 *               based meta-data.
 *
 * Created:      11/20/96
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */

#define   SQLPARSERGLOBALS_FLAGS        // should precede all other #include's
#define   SQLPARSERGLOBALS_NADEFAULTS   // should precede all other #include's

#include "Platform.h"

#include "NAAssert.h"
#include "CmpCommon.h"
#include "CmpStatement.h"	   
#include "BaseTypes.h"
#include "readRealArk.h"
#include "TrafDDLdesc.h"

// Copy a string.  A null terminated buffer is returned.
// Uses HEAP (StatementHeap) of CmpCommon!
static char *copyString(const ComString &sourceString,
			       ComBoolean wideNull = FALSE)
{
  static char emptyString[] = "\0\0\0\0";	// empty, works for narrow OR wide null
  return (sourceString.length() == 0) ?
    emptyString :
    convertNAString(sourceString, HEAP, wideNull);
}


// -----------------------------------------------------------------------
// Allocate a column_desc and do simple initialization of several fields,
// based on what's passed in.  Many of the fields we just default,
// to either hardcoded values or to zero.  The callers,
// in arkcmplib + generator + optimizer, can set additional fields afterwards.
// -----------------------------------------------------------------------
TrafDesc *TrafMakeColumnDesc(const char *tablename,
                                const char *colname,
                                Lng32 &colnumber,	// INOUT
                                Int32 datatype,
                                Lng32 length,
                                Lng32 &offset,	// INOUT
                                NABoolean null_flag,
                                SQLCHARSET_CODE datacharset,
                                NAMemory * space
                                )
{
  #undef  COLUMN
  #define COLUMN returnDesc->columnsDesc()

  // Pass in the optional "passedDesc" if you just want to overwrite an
  // already existing desc.
  TrafDesc *returnDesc = 
    TrafAllocateDDLdesc(DESC_COLUMNS_TYPE, space);

  COLUMN->colname = (char *)colname;

  COLUMN->colnumber = colnumber;
  COLUMN->datatype = datatype;
  COLUMN->length = length;
  COLUMN->offset = offset;
  COLUMN->setNullable(null_flag);

  // Hardcode some fields here.
  // All other fields (scale, precision, etc) default to zero!

  COLUMN->colclass = 'U';
  COLUMN->setDefaultClass(COM_NO_DEFAULT);

  if (DFS2REC::isAnyCharacter(datatype)) {
    if (datacharset == SQLCHARSETCODE_UNKNOWN) {
    COLUMN->character_set      = CharInfo::DefaultCharSet;
    COLUMN->encoding_charset   = CharInfo::DefaultCharSet;
    }
    else {
    COLUMN->character_set      = (CharInfo::CharSet)datacharset;
    COLUMN->encoding_charset   = (CharInfo::CharSet)datacharset;
    }
    COLUMN->collation_sequence = CharInfo::DefaultCollation;
    if (DFS2REC::isSQLVarChar(datatype))
      offset += SQL_VARCHAR_HDR_SIZE;
  }
  else {	// datetime, interval, numeric, etc.
    COLUMN->datetimestart = COLUMN->datetimeend = REC_DATE_UNKNOWN;
  }

  colnumber++;

  offset += length;
  if (null_flag) offset += SQL_NULL_HDR_SIZE;

  return returnDesc;
}


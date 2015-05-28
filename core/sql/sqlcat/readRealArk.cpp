/* -*-C++-*-
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1996-2015 Hewlett-Packard Development Company, L.P.
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
 *****************************************************************************
 *
 * File:         readRealArk.C
 * Description:  This module converts SOL based meta-data to desc_struct
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
// Allocate one of the primitive structs and initialize to all zeroes.
// Uses HEAP (StatementHeap) of CmpCommon!
// -----------------------------------------------------------------------
desc_struct *readtabledef_allocate_desc(desc_nodetype nodetype)
{
  size_t size = 0;

  switch (nodetype)
    {
    case DESC_CHECK_CONSTRNTS_TYPE:
      size = sizeof(check_constrnts_desc_struct);
      break;
    case DESC_COLUMNS_TYPE:
      size = sizeof(columns_desc_struct);
      break;
    case DESC_CONSTRNTS_TYPE:
      size = sizeof(constrnts_desc_struct);
      break;
    case DESC_CONSTRNT_KEY_COLS_TYPE:
      size = sizeof(constrnt_key_cols_desc_struct);
      break;
    case DESC_FILES_TYPE:
      size = sizeof(files_desc_struct);
      break;
    case DESC_HISTOGRAM_TYPE:
      size = sizeof(histogram_desc_struct);
      break;
    case DESC_HIST_INTERVAL_TYPE:
      size = sizeof(hist_interval_desc_struct);
      break;
    case DESC_INDEXES_TYPE:
      size = sizeof(indexes_desc_struct);
      break;
    case DESC_KEYS_TYPE:
      size = sizeof(keys_desc_struct);
      break;
    case DESC_PARTNS_TYPE:
      size = sizeof(partns_desc_struct);
      break;
    case DESC_REF_CONSTRNTS_TYPE:
      size = sizeof(ref_constrnts_desc_struct);
      break;
    case DESC_TABLE_TYPE:
      size = sizeof(table_desc_struct);
      break;
    case DESC_VIEW_TYPE:
      size = sizeof(view_desc_struct);
      break;	       
    case DESC_USING_MV_TYPE:  // MV 
      size = sizeof(using_mv_desc_struct);
      break;
   case DESC_SEQUENCE_GENERATOR_TYPE:   
      size = sizeof(sequence_generator_desc_struct);
      break;
   case DESC_ROUTINE_TYPE:
      size = sizeof(routine_desc_struct);
      break;
   case DESC_LIBRARY_TYPE:
      size = sizeof(library_desc_struct);
      break;
    default:
      assert(FALSE);
      break;
    }
  // DBG( cerr << "... rtd_allocate_desc " << nodetype << " " << size << " "; )

  size += sizeof(header_desc_struct);
  char *desc_buf = new HEAP char[size];

  // Note that this puts literal zero bytes into floats and pointer fields,
  // which are not portably equivalent to (float)0 and (void *)NULL.
  // This is a better default than not initializing at all, however.
  memset(desc_buf, 0, size);

  desc_struct *desc_ptr = (desc_struct *)desc_buf;
  desc_ptr->header.nodetype = nodetype;
  desc_ptr->header.next     = NULL;                // example of portable initlzn.

  // DBG( cerr << size << " " << desc_ptr << endl; )
  return desc_ptr;

}

// -----------------------------------------------------------------------
// Allocate a column_desc and do simple initialization of several fields,
// based on what's passed in.  Many of the fields we just default,
// to either hardcoded values or to zero.  The callers,
// in arkcmplib + generator + optimizer, can set additional fields afterwards.
// -----------------------------------------------------------------------
desc_struct *readtabledef_make_column_desc(const char *tablename,
					   const char *colname,
					   Lng32 &colnumber,	// INOUT
					   DataType datatype,
					   Lng32 length,
					   Lng32 &offset,	// INOUT
					   short null_flag,
					   NABoolean tablenameMustBeAllocated,
					   desc_struct *passedDesc,
					   SQLCHARSET_CODE datacharset
					  )
{
  #undef  COLUMN
  #define COLUMN returnDesc->body.columns_desc

  // Pass in the optional "passedDesc" if you just want to overwrite an
  // already existing desc.
  desc_struct *returnDesc = passedDesc;
  if (!returnDesc) returnDesc = readtabledef_allocate_desc(DESC_COLUMNS_TYPE);

  if (tablenameMustBeAllocated)
    COLUMN.tablename = copyString(NAString(tablename));
  else
    COLUMN.tablename = (char *)tablename;	// just copy the pointer!

  COLUMN.colname = copyString(colname);
  COLUMN.colnumber = colnumber;
  COLUMN.datatype = datatype;
  COLUMN.length = length;
  COLUMN.offset = offset;
  COLUMN.null_flag = null_flag;

  // Hardcode some fields here.
  // All other fields (scale, precision, etc) default to zero!

  COLUMN.colclass = 'U';
  COLUMN.defaultClass = COM_NO_DEFAULT;

  if (DFS2REC::isAnyCharacter(datatype)) {
    if (datacharset == SQLCHARSETCODE_UNKNOWN) {
    COLUMN.character_set      = CharInfo::DefaultCharSet;
    COLUMN.encoding_charset   = CharInfo::DefaultCharSet;
    }
    else {
    COLUMN.character_set      = (CharInfo::CharSet)datacharset;
    COLUMN.encoding_charset   = (CharInfo::CharSet)datacharset;
    }
    COLUMN.collation_sequence = CharInfo::DefaultCollation;
    if (DFS2REC::isSQLVarChar(datatype))
      offset += SQL_VARCHAR_HDR_SIZE;
  }
  else {	// datetime, interval, numeric, etc.
    COLUMN.datetimestart = COLUMN.datetimeend = REC_DATE_UNKNOWN;
  }

  colnumber++;

  offset += length;
  if (null_flag) offset += SQL_NULL_HDR_SIZE;

  return returnDesc;
}


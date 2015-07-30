/**********************************************************************
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
**********************************************************************/
/* -*-C++-*-
******************************************************************************
*
* File:         ReadTableDef.cpp
* Description:	
*   This class and set of functions provide an interface between the 
*   compiler and metadata.  It gets information from catman cache through
*   the SOL layer and puts it into a desc structure.  The desc structure is
*   returned to the compiler to be placed in the NATable structure.
*
* Created:	01/18/96
* Language:     C++
*
*
******************************************************************************
*/

//------------------------------------------------------------------------
// Include files
//------------------------------------------------------------------------
#include "Platform.h"			// must be first

#define  SQLPARSERGLOBALS_NADEFAULTS	// must precede other #include's
#define  READTABLEDEF_IMPLEMENTATION	// for ReadTableDef.h and dfs2rec.h

#include "ReadTableDef.h"
#include "readRealArk.h"
#include "SQLCLIdev.h"
#include <ctype.h>
#include <iostream>
#include <iomanip>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

  
#include "NLSConversion.h"

#ifdef NA_LITTLE_ENDIAN
  #include "ComSysUtils.h"
  #define  ENCODE(xxx)		xxx->encode()
  #define  REVERSEBYTES(xxx)	xxx = (Lng32) reversebytes((ULng32) xxx)
#else
  #define  ENCODE(xxx)
  #define  REVERSEBYTES(xxx)
#endif

#include "fs/feerrors.h"	// FEOK, FENOTOPEN, FEPATHDOWN, etc.

extern "C" {
int_16 TMF_SETTXHANDLE_(short *);
int_16 TMF_GETTXHANDLE_(short *);
int_16 GETTRANSID(short *);
int_16 TMF_GETCURRENTTCBREF_(short *);
int_16 BEGINTRANSACTION(int *tag);
}

#include "CmpCommon.h"		// HEAP stuff
#include "CmpContext.h"		// CmpCommon::context()->readTableDef_
#include "DefaultValidator.h"	// for ValidateCollationList
#include "ExSqlComp.h"		// for NAExecTrans
#include "IntervalType.h"
#include "DatetimeType.h"
#include "SchemaDB.h"

#include "NAAssert.h"
#include "SqlParserGlobals.h"	// must be last #include!


//------------------------------------------------------------------------
// Global variables declarations
//------------------------------------------------------------------------
extern Lng32 SQLCODE;

//------------------------------------------------------------------------
// Static variables declarations
//------------------------------------------------------------------------


#define NODETYPE_MX	header.nodetype	  // desc_structPtr->NODETYPE_MX

// MX SqlCat simulator xxx_desc_struct
#define STRUCTMX(xxx)	xxx ## _desc_struct

// MX SqlCat simulator DESC_XXX_TYPE
#define TYPEMX(XXX)	DESC_ ## XXX ## _TYPE

// xxx_desc_struct * xxx = (xxx_desc_struct *) &top->body
#define CASTMX(xxx)	STRUCTMX(xxx) * xxx = (STRUCTMX(xxx) *)&top->body

// Guardian LargeInt is little-endian:  least significant bits are on the right.
// So mpvar[0] is to left of i.e. more significant than mpvar[1].
// NNNN Truncation error (64 to 32 bits overflow) in ReadTableDef $String0.
// (If this #define is ever used, then create error message for NNNN.)
#define FMT_LARGEINT(arr)						\
			arr[0] << "," << arr[1] << endl

#define COUT(var,fld)							\
			cout << "  " << setw(20) 			\
			     << #fld << "  " << var->fld << endl

// For compilers that don't cast enums to int 	// NT_PORT mhr 03/14/97
#define COUT_ENUM(var,fld)						\
			cout << "  " << setw(20) 			\
			     << #fld << "  " << (Int32)var->fld << endl

#define COUT_STRING(var,fld)						\
			if (var->fld) COUT(var,fld);			\
			else						\
			  cout << "  " << setw(20)			\
			       << #fld << " is null" << endl

#define COUT_LARGEINT(var,fld)						\
			cout << "  " << setw(20) 			\
			     << #fld << "  " << FMT_LARGEINT(var->fld)

//------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------

ReadTableDef::ReadTableDef()
: transactionState_   (NO_TXN),
  transInProgress_    (FALSE),
  transId_            (-1)
{
} // ctor

//------------------------------------------------------------------------
// Destructor
//------------------------------------------------------------------------

ReadTableDef::~ReadTableDef()
{
  // end any transactions started by "me"
}



//------------------------------------------------------------------------
// displayTreeMX()
//------------------------------------------------------------------------
void ReadTableDef::displayTree(const desc_struct * top) const
{
  displayTreeMX(top, "");
}

void ReadTableDef::displayTreeMX(const desc_struct * top,
                                  const char * caller) const
{
  if (top == NULL) return;

  char title[50];
  snprintf(title, sizeof(title),
          ", type %d, address %p, parent %s\n",
          top->NODETYPE_MX, (void *)top, caller);
  #undef  TITLE
  #define TITLE(XXX)    cout << "### " << XXX << title;         \
                        strcpy(title, XXX)

  cout.setf(ios::right);

  switch (top->NODETYPE_MX)
    {
    case TYPEMX(CHECK_CONSTRNTS):
      {
        TITLE("CHECK_CONSTRNTS");
        CASTMX(check_constrnts);
        COUT(check_constrnts, seqnumber);
        COUT_STRING(check_constrnts, constrnt_text);
      }
      break;

    case TYPEMX(COLUMNS):
      {
        TITLE("COLUMNS");
        CASTMX(columns);
        COUT(columns, tablename);
        COUT(columns, colname);
        COUT(columns, colnumber);
        COUT(columns, datatype);
        COUT(columns, length);
        COUT(columns, scale);
        COUT(columns, precision);
        COUT_ENUM(columns, datetimestart);
        COUT_ENUM(columns, datetimeend);
        COUT(columns, datetimefractprec);
        COUT(columns, intervalleadingprec);
        COUT(columns, offset);
        COUT(columns, null_flag);
        COUT(columns, upshift);
        COUT(columns, colclass);
        COUT(columns, uec);
        COUT_STRING(columns, highval);
        COUT_STRING(columns, lowval);

        Int32 defaultValueInLocaleLen = 
#pragma nowarn(1506)   // warning elimination 
              NAWstrlen((NAWchar*)(columns->defaultvalue));
#pragma warn(1506)  // warning elimination 
        char* defaultValueInLocale = new HEAP char[defaultValueInLocaleLen+1];

        CharInfo::CharSet mapCharSet = SqlParser_ISO_MAPPING;

        Int32 x = UnicodeStringToLocale(mapCharSet,
                 (NAWchar*)(columns->defaultvalue), defaultValueInLocaleLen,
                 defaultValueInLocale, defaultValueInLocaleLen+1
                             );
        if (columns->defaultvalue)  {
          cout << "  " << setw(20) << "defaultvalue"
               << "  " << defaultValueInLocale << endl;
        } else {
          cout << "  " << setw(20) << "defaultvalue" << " is null" << endl;
        }
        NADELETEBASIC(defaultValueInLocale, HEAP);
      }
      break;

    case TYPEMX(CONSTRNTS):
      {
        TITLE("CONSTRNTS");
        CASTMX(constrnts);
        COUT(constrnts, constrntname);
        COUT(constrnts, tablename);
        COUT_ENUM(constrnts, type);
        COUT(constrnts, colcount);
        COUT_STRING(constrnts, indexname);
        displayTreeMX(constrnts->check_constrnts_desc, title);
        displayTreeMX(constrnts->constr_key_cols_desc, title);
        displayTreeMX(constrnts->referenced_constrnts_desc, title);
        displayTreeMX(constrnts->referencing_constrnts_desc, title);
      }
      break;

    case TYPEMX(CONSTRNT_KEY_COLS):
      {
        TITLE("CONSTRNT_KEY_COLS");
        CASTMX(constrnt_key_cols);
        COUT(constrnt_key_cols, colname);
        COUT(constrnt_key_cols, position);
      }
      break;

    case TYPEMX(FILES):
      {
        TITLE("FILES");
        CASTMX(files);
        COUT_ENUM(files, fileorganization);
        COUT(files, audit);
        COUT(files, auditcompress);
        COUT(files, compressed);
        displayTreeMX(files->partns_desc, title);
      }
      break;

    case TYPEMX(HISTOGRAM):
      {
        TITLE("HISTOGRAM");
        CASTMX(histogram);
        COUT(histogram, tablename);
        COUT(histogram, tablecolnumber);
        COUT(histogram, histid);
        COUT(histogram, colposition);
        COUT(histogram, rowcount);
        COUT(histogram, uec);
        COUT_STRING(histogram, highval);
        COUT_STRING(histogram, lowval);
        displayTreeMX(histogram->hist_interval_desc, title);
      }
      break;

    case TYPEMX(HIST_INTERVAL):
      {
        TITLE("HIST_INTERVAL");
        CASTMX(hist_interval);
        COUT(hist_interval, histid);
        COUT(hist_interval, intnum);
        COUT(hist_interval, intboundary);
        COUT(hist_interval, rowcount);
        COUT(hist_interval, uec);
      }
      break;

    case TYPEMX(INDEXES):
      {
        TITLE("INDEXES");
        CASTMX(indexes);
        COUT(indexes, tablename);
        COUT(indexes, indexname);
        COUT(indexes, keytag);
        COUT(indexes, record_length);
        COUT(indexes, colcount);
        COUT(indexes, unique);
        displayTreeMX(indexes->files_desc, title);
        displayTreeMX(indexes->keys_desc, title);
    displayTreeMX(indexes->non_keys_desc, title);
      }
      break;

    case TYPEMX(KEYS):
      {
        TITLE("KEYS");
        CASTMX(keys);
        COUT(keys, indexname);
        COUT(keys, keyseqnumber);
        COUT(keys, tablecolnumber);
        COUT(keys, ordering);
      }
      break;

    case TYPEMX(PARTNS):
      {
        TITLE("PARTNS");
        CASTMX(partns);
        COUT(partns, tablename);
        COUT(partns, primarypartition);
        COUT(partns, partitionname);
        COUT_STRING(partns, firstkey);
      }
      break;

    case TYPEMX(REF_CONSTRNTS):
      {
        TITLE("REF_CONSTRNTS");
        CASTMX(ref_constrnts);
        COUT(ref_constrnts, constrntname);
        COUT(ref_constrnts, tablename);
      }
      break;

    case TYPEMX(TABLE):
      {
        TITLE("TABLE");
        CASTMX(table);
        COUT_LARGEINT(table, createtime);
        COUT_LARGEINT(table, redeftime);
        COUT(table, tablename);
        COUT(table, record_length);
        COUT(table, colcount);
        COUT(table, constr_count);
        COUT(table, rowcount);
        displayTreeMX(table->files_desc, title);
        displayTreeMX(table->columns_desc, title);
        displayTreeMX(table->views_desc, title);
        displayTreeMX(table->indexes_desc, title);
        displayTreeMX(table->constrnts_desc, title);
        if (table->constrnts_tables_desc != top)
          displayTreeMX(table->constrnts_tables_desc, title);
        displayTreeMX(table->referenced_tables_desc, title);
        displayTreeMX(table->referencing_tables_desc, title);
        displayTreeMX(table->histograms_desc, title);
      }
      break;

    case TYPEMX(VIEW):
      {
        TITLE("VIEW");
        CASTMX(view);
        COUT(view, viewname);
        COUT_STRING(view, viewtext);
        COUT_STRING(view, viewchecktext);
        COUT(view, updatable);
        COUT(view, insertable);
      }
      break;

    default:
      {
        TITLE("??? UNKNOWN ???");
      }
      break;
    }  // switch

  displayTreeMX (top->header.next, title);

} // displayTreeMX


//------------------------------------------------------------------------
// deleteTreeMX()
//------------------------------------------------------------------------
void ReadTableDef::deleteTree(desc_struct * top) const
{
  deleteTreeMX(top);
}

void ReadTableDef::deleteTreeMX(desc_struct * top) const
{
  if (top == NULL) return;

  switch (top->NODETYPE_MX)
    {
    case TYPEMX(CHECK_CONSTRNTS):
      break;

    case TYPEMX(COLUMNS):
      break;

    case TYPEMX(CONSTRNTS):
      {
	CASTMX(constrnts);
	deleteTreeMX(constrnts->check_constrnts_desc);
	deleteTreeMX(constrnts->constr_key_cols_desc);
	deleteTreeMX(constrnts->referenced_constrnts_desc);
	deleteTreeMX(constrnts->referencing_constrnts_desc);
      }
      break;

    case TYPEMX(CONSTRNT_KEY_COLS):
      break;

    case TYPEMX(FILES):
      {
	CASTMX(files);
	deleteTreeMX(files->partns_desc);
      }
      break;

    case TYPEMX(HISTOGRAM):
      {
	CASTMX(histogram);
	deleteTreeMX(histogram->hist_interval_desc);
      }
      break;

    case TYPEMX(HIST_INTERVAL):
      break;

    case TYPEMX(INDEXES):
      {
	CASTMX(indexes);
	deleteTreeMX(indexes->keys_desc);
    deleteTreeMX(indexes->non_keys_desc);
	deleteTreeMX(indexes->files_desc);
      }
      break;

    case TYPEMX(KEYS):
      break;

    case TYPEMX(PARTNS):
      break;

    case TYPEMX(REF_CONSTRNTS):
      break;

    case TYPEMX(TABLE):
      {
	CASTMX(table);
	deleteTreeMX(table->columns_desc);
	deleteTreeMX(table->views_desc);
	deleteTreeMX(table->files_desc);
	deleteTreeMX(table->indexes_desc);
	deleteTreeMX(table->constrnts_desc);
	if (table->constrnts_tables_desc != top)
	  deleteTreeMX(table->constrnts_tables_desc);
	deleteTreeMX(table->referenced_tables_desc);
	deleteTreeMX(table->referencing_tables_desc);
	deleteTreeMX(table->histograms_desc);
      }
      break;

    case TYPEMX(VIEW):
      break;

    default:
      {
	cerr << "ReadTableDef: memory leak at node type " << top->NODETYPE_MX
	     << endl;
      }
      break;
    }  // switch

  deleteTreeMX (top->header.next);
  NADELETEBASIC(top, HEAP);

} // deleteTreeWMX


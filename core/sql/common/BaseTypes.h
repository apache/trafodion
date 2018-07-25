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
******************************************************************************
*
* File:         BaseTypes.h
* Description:  common definitions of basic data types
*
* Created:      4/27/94
* Language:     C++
*
*
*
******************************************************************************
*/

#ifndef BASETYPES_H
#define BASETYPES_H

#include "Platform.h"


#include <stdio.h>
#include <stdlib.h>

#include <iostream>
using namespace std;

#include "NAAssert.h"
#include "NAHeap.h"

typedef std::string NAText;

// 32-bit unsigned should be sufficient
typedef UInt32 CollIndex;	// 64-bit

// -----------------------------------------------------------------------
// Declare a NULL pointer (if not already defined)
// -----------------------------------------------------------------------
#ifndef NULL
#define NULL 0
#endif

// Declare an illegal index
#ifndef ILLEGAL_INDEX
#define ILLEGAL_INDEX -1
#endif

// -----------------------------------------------------------------------
// A boolean number.
// -----------------------------------------------------------------------
#include "NABoolean.h"

// -----------------------------------------------------------------------
// An unsigned number.
// -----------------------------------------------------------------------
typedef ULng32 NAUnsigned;

// -----------------------------------------------------------------------
// Any other floating number
// -----------------------------------------------------------------------
typedef float NAFloat;

// -----------------------------------------------------------------------
// The cardinality of a relation (rowcount), its row length and its total
// size in Bytes
// -----------------------------------------------------------------------
typedef NAFloat     Cardinality; // $$$ get rid of this one!
typedef NAUnsigned  ColumnSize;
typedef NAUnsigned  RowSize;
typedef NAFloat     TableSize;

// could also use IEEE "infinity" value
// NOTE: this value MUST be larger than any reasonable cardinality
// value, don't use negative values or comparisons will do the wrong thing!!
#define INFINITE_CARDINALITY 1E20

// -----------------------------------------------------------------------
// A simple hash value
// -----------------------------------------------------------------------
typedef NAUnsigned SimpleHashValue;

// -----------------------------------------------------------------------
// C++ operators in a more readable form
// -----------------------------------------------------------------------

#ifndef NOT
#define NOT		!
#endif
#ifndef AND
#define AND		&&
#endif
#ifndef OR
#define OR		||
#endif
#ifndef LAND
#define LAND		&
#endif
#ifndef LOR
#define LOR		|
#endif
#ifndef XOR
#define XOR		^
#endif
#ifndef LNOT
#define LNOT		~
#endif
#ifndef YesNo
#define YesNo(B)	((B) ? "Yes" : "No")
#endif
#ifndef TrueFalse
#define TrueFalse(B)	((B) ? "True" : "False")
#endif
#ifndef YN
#define YN(B)           ((B) ? "Y" : "N")
#endif
#ifndef TRUEFALSE
#define TRUEFALSE(B)	((*B == 'Y') ? TRUE : FALSE)
#endif
#ifndef CONCAT
#define CONCAT(A,B)     (NAString(A) += B)
#endif
#ifndef IFX
#define IFX
#endif
#ifndef THENX
#define THENX           ?
#endif
#ifndef ELSEX
#define ELSEX           :
#endif

// -----------------------------------------------------------------------
#ifndef IN_RANGE
#define IN_RANGE(x, lower, upper) ( lower <= x ) && ( x <= upper )
#endif
// Macro definitions
// -----------------------------------------------------------------------
#ifndef ABS
#define ABS(X)	   (X >= 0 ? X : -(X))
#endif
#ifndef MAXOF
#define MAXOF(X,Y) (X >= Y ? X : Y)
#endif
#ifndef MINOF
#define MINOF(X,Y) (X <= Y ? X : Y)
#endif
#ifndef MIN_ONE /* denoting "at least one ..." */
#define MIN_ONE(X) MAXOF(X,1)
#endif
#ifndef STRINGIZE
#define STRINGIZE(X)	#X
#endif

// -----------------------------------------------------------------------
// Macros for formatting the output of print functions used for debugging
// -----------------------------------------------------------------------
#define LINE_SIZE 80
#define DEFAULT_INDENT ""
#define MIN_INDENT_SIZE 0
#define MAX_INDENT_SIZE 40

#define BLANK_SPACE " "

#define BUMP_INDENT(X) \
  char newindent[MAX_INDENT_SIZE + 1]; \
  Int32 indentlen = strlen(X) + MIN_INDENT_SIZE;\
  indentlen = (indentlen < MAX_INDENT_SIZE ? indentlen : MAX_INDENT_SIZE);\
  for (Int32 fli = 0; fli < indentlen; fli++) \
    newindent[fli] = ' ';\
  newindent[indentlen] = '\0';

#define NEW_INDENT newindent

#define LINE_STRING    ("===========")

#define PRINTIT(file,heap,space,buf,mybuf) \
  if (heap) { \
    Space::outputBuffer(space, buf, mybuf); \
  } else { fprintf(file, mybuf); fflush(file); }


// Macros to work around c89's limited support of ANSI C++ features
#  define CONST_CAST(t,e) const_cast< t >(e)

// -----------------------------------------------------------------------
// the enums needed by unparse()
// -----------------------------------------------------------------------
enum PhaseEnum { PARSER_PHASE, BINDER_PHASE, TRANSFORM_PHASE,
		 NORMALIZER_PHASE, OPTIMIZER_PHASE,
		 DEFAULT_PHASE = OPTIMIZER_PHASE };

enum UnparseFormatEnum { USER_FORMAT, EXPLAIN_FORMAT, FILE_FORMAT,
			 USER_FORMAT_DELUXE, ERROR_MSG_FORMAT,
			 MVINFO_FORMAT, MV_SHOWDDL_FORMAT,
                         QUERY_FORMAT, COMPUTED_COLUMN_FORMAT,
			 HIVE_MD_FORMAT };

// -----------------------------------------------------------------------
// Used to display optimizer statistics, and other debugging statements
// -----------------------------------------------------------------------

#define report          printf ("%s,%d ", __FILE__, __LINE__), \
                        printf

// -----------------------------------------------------------------------
// give a debugger a chance to run (simply causes an endless loop, waiting
// for the debugger to be attached to the process)
// -----------------------------------------------------------------------

extern void NADebug();

// -----------------------------------------------------------------------
// Abnormal program termination
// -----------------------------------------------------------------------

#define ABORT(msg)	NAAbort (__FILE__, __LINE__, (msg))
extern void NAAbort (const char *, Int32, const char *);

// -----------------------------------------------------------------------
// the NAString datatype used to be Tools.h++'s RWCString; now it's a
// distinct class, similar in functionality to RWCString except that it
// allocates memory w.r.t. CollHeap *'s (instead of just putting 'em in
// global space)
// -----------------------------------------------------------------------
//#include "NAStringDef.h"

// Helper functions to map between FS types and ANSI types. ANSI types
// are defined by the SQLTYPE_CODE enumeration in cli/sqlcli.h
Lng32 getAnsiTypeFromFSType(Lng32 datatype);
Lng32 getDatetimeCodeFromFSType(Lng32 datatype);
Lng32 getFSTypeFromDatetimeCode(Lng32 datetime_code);
Lng32 getFSTypeFromANSIType(Lng32 ansitype);
const char * getAnsiTypeStrFromFSType(Lng32 datatype);

#define DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName&);               \
  void operator=(const TypeName&)

#endif /* BASETYPES_H */

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
 *****************************************************************************
 *
 * File:         CmpCommon.h
 * Description:  The error/memory/exception handling routines for SQL compiler
 *               components.
 *
 * Created:      8/29/96
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */


#ifndef CMPCOMMON_H
#define CMPCOMMON_H

#define ARKCMP_PRIVATE

// Forward references -- preceding any #include's that might actually define
// them, because there appear to be circular dependencies somewhere.
class ComDiagsArea;

#include "Platform.h"
#include "ComDiags.h"
#include "ComTransInfo.h"       
#include "ComObjectName.h"       
#include "DefaultConstants.h"
#include "EHException.h"
#include "NAAssert.h"

// CmpContext contains the global information for arkcmp, defined in arkcmp

enum CompilationMode { STMT_STATIC, STMT_DYNAMIC };

class NAMemory;
class CmpContext;
class CmpContextInfo;
class CmpStatement;

// For ComDiagsArea and memory handling, use the following static functions 
// in CmpCommon.

class CmpCommon
{
public:

  // To get the current ComDiagsArea*, use diags(). This area will be 
  // cleanup up in the beginning of the statement compilation. In the
  // case of CMPASSERT called, an internal error ComCondition will be
  // put into the diags() with the file name and line number specified.

  static ComDiagsArea* diags();

  // This routine dumps all diags messages in CmpCommon::diags() out to a given ostream.
 
  static void dumpDiags(ostream&, NABoolean newline = FALSE);

  // Some notes about memory management
  // arkcmp (compiler main program) maintains two kind of heaps, 
  // contextHeap and statementHeap.
  // For variables that will stay accross the statements, they should
  // be allocated in the contextHeap, because statementHeap will be
  // wiped out at the end of statement to avoid memory leak. arkcmp looks like
  //
  // main()
  // { contextHeap;
  //    for each statement
  //      {
  //         contextHeap->init();
  //         statementHeap;
  //          .....
  //         contextHeap->cleanup(); 
  //      }  the statementHeap is deleted at this point
  // }
  //
  // Note 1: If there are storages (variables) need to be initialized,
  //         or cleaned up, put them into init() and cleanup() routines.
  // Note 2: VERY IMPORTANT
  //         e.g. an instance of class A is in the scope of contextHeap
  //         class A { char* p; }, 
  //         - if p is allocated in the scope of statementHeap ( which will
  //           be deleted in the end of statement), it can not be deleted in 
  //           the the next statement loop. The suggestion will be delete p
  //           in cleanup(), or if you need to use p for next statement, put
  //           it into the scope of contextHeap (i.e. new(contextHeap) char[])
  //
  // To avoid memory leak, most of the variables should be allocated from
  // statementHeap, so that they will be wiped at the end of each statement.
  // Only very few should be put into contextHeap, then delete has to be
  // performed to avoid memory leak.
  // **IMPORTANT NOTE** : The memory allocated from this heap should be 
  // deleted accordingly when not in use, otherwise memory leak.
  //  **IMPORTANT NOTE** : The variables allocated through this heap can not
  // be reused in next statement, otherwise memory fault. 
  // To allocate the space for contextHeap, use the overloaded new with
  // CmpCommon::contextHeap() as the NAMemory*, 
  // To allocate the space for statementHeap, use the overloaded new with
  // CmpCommon::statementHeap() as the NAMemory*
  // use global new/delete will get the space from global new/delete.
  // for more information about the usage of new/delete, see sqlcomp/NAHeap.h

  // These #defines are just to make coding a little less crowded, more clear.
  // "HEAP" is a synonym for STMTHEAP, a default as it were, for the most
  // commonly new'd-from heap.

  #define CTXTHEAP	(CmpCommon::contextHeap())
  #define STMTHEAP	(CmpCommon::statementHeap())
  #define HEAP		STMTHEAP			// the commonest case

  static NAMemory * contextHeap();
  static NAHeap * statementHeap();

  #define CURRENTSTMT (CmpCommon::statement())
  #define CURRSTMT_OPTGLOBALS  (CmpCommon::statement()->getOptGlobals())
  #define CURRSTMT_CQSWA  (CmpCommon::statement()->getCqsWA())
  #define CURRCONTEXT_OPTDEBUG (CmpCommon::context()->getOptDbg())
  #define CURRCONTEXT_HISTCACHE (CmpCommon::context()->getHistogramCache())
  #define CURRCONTEXT_OPTSIMULATOR (CmpCommon::context()->getOptimizerSimulator())
  #define GLOBAL_EMPTY_INPUT_LOGPROP (CmpCommon::context()->getGEILP())
  #define CURRSTMT_OPTDEFAULTS (CmpCommon::context()->getOptDefaults())

  // For some routines that do care about the current CmpContext*. 
  // If you need to declare some global/static variables in the scope of
  // context, put them into CmpContext class. (arkcmp/CmpContext.[Ch])
  // To access the variables in current Context, use
  // CmpCommon::context()->...
  static CmpContext* context();

  #define CURRENTQCACHE (CmpCommon::context()->getQueryCache())
  #define CURROPTPCODECACHE (CmpCommon::context()->getOptPCodeCache() )

  #define GlobalRuleSet (CmpCommon::context()->getRuleSet())

  // The following functions allow access to the defaults table
  // in sqlcomp/NADefaults.cpp.  Given the id number of the default,
  // the first returns the value as a double (-1 if invalid),
  // the second puts the NORMALIZED (trimmed, upcased) string representation
  // into 'result' and returns the Token
  // (DF_ON, etc.; negative if not a known Token, in which case an error diag
  // is emitted if errOrWarn is -1, a warning if +1, no diag if 0).
  // The third one doesn't have the overhead of returning the value
  // as a string. This one should be used most of the time.
  //
  static Lng32         getDefaultLong(DefaultConstants id);
  static double        getDefaultNumeric(DefaultConstants id);
  static NAString       getDefaultString(DefaultConstants id);
  static DefaultToken getDefault(DefaultConstants id,
  				 NAString &result,
				 Int32 errOrWarn =-1);
  static DefaultToken getDefault(DefaultConstants id,
				 Int32 errOrWarn = -1);
  static NABoolean wantCharSetInference();

  static void applyDefaults(ComObjectName &name);

  // The current statement under process. 
  // If you need to declare some global/static variables in the scope of
  // statement, put them into CmpStatement (arkcmp/CmpStatement.[Ch])
  // To access the variables in current statement, use
  // CmpCommon::statement()->....
  static CmpStatement* statement();

  // The TransMode for the current statement.  Was in generator.cpp previously.
  static TransMode * transMode(); 

  static const NAString * getControlSessionValue(const NAString &token);
};

// For exception handling, use CMPASSERT, CMPBREAK and CMPABORT macros

// CMPASSERT is used in the case of internal error. It will
// put an internal error into the global diags area (the diags that
// arkcmp passes into each routine) with the file name and line specified. 
// Clean up the statement heap and wait for next statement 
// request if possible.
// Notice this *always* asserts something, even if NDEBUG is TRUE.
//
#define CMPASSERT(b)                                                    \
  { if (!(b)) CmpAssertInternal("" # b "", __FILE__, __LINE__); }

#define CMPASSERT_STRING(b,str)                         \
  { if (!(b)) { cerr << str << endl; CMPASSERT(b); }}

// The following DCMPASSERT is for supporting an assert mechanism for the
// compiler that gets disabled in RELEASE (NDEBUG) code:
//
#ifndef NDEBUG
#define DCMPASSERT(x)	 CMPASSERT(x); 

#else
  #define DCMPASSERT(x) 
#endif

#define CMPBREAK CMPASSERT(FALSE)

// CMPABORT will just exit the program. It is used in the case of
// server error, so the program (process) can not continue anymore.
//
#define CMPABORT            CmpAbortInternal("", __FILE__, __LINE__);
#define CMPABORT_MSG(b)     CmpAbortInternal(b, __FILE__, __LINE__);

extern ARKCMP_PRIVATE void CmpAssertInternal(const char*, const char*, Int32);
extern ARKCMP_PRIVATE void CmpAbortInternal(const char*, const char*, Int32);
extern THREAD_P ARKCMP_PRIVATE CmpContext* cmpCurrentContext;

extern THREAD_P ARKCMP_PRIVATE jmp_buf* ExportJmpBufPtr;
extern THREAD_P ARKCMP_PRIVATE jmp_buf* CmpInternalErrorJmpBufPtr;

extern void deinitializeArkcmp();

#endif

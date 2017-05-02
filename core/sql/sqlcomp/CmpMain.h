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
 * File:         cmpmain.h
 * Description:  This file contains the declarations for the routines used
 *               by executor(in single process mode) or arkcmp to perform
 *               the statement compilation or DDL statement execution.
 *
 *
 * Created:      7/10/95
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *
 *
 *****************************************************************************
 */


#ifndef __CMPMAIN__H
#define __CMPMAIN__H

#include "dfs2rec.h"

#ifdef NA_DEBUG_GUI
#include "ComSqlcmpdbg.h"
#endif

#include "ComSmallDefs.h"
#include "RelScan.h"
#include "sqlcli.h"
#include "CmpMessage.h"
#include "ComSysUtils.h"

class Key;
class CacheKey;
class ComDiagsArea;
class NAMemory;
class NormWA;
class RelExpr;
class RelRoot;
class FragmentDir;
class QueryText;

const SQLATTR_TYPE  SQL_ATTR_SQLPARSERFLAGS=(SQLATTR_TYPE)0; 
// NB: SQL_ATTR_SQLPARSERFLAGS  must be distinct & different
//     from all other SQLATTR_TYPE values in cli/sqlcli.h
const SQLATTR_TYPE  SQL_ATTR_NOT_SET=(SQLATTR_TYPE)666; 
const ULng32 SQL_ATTR_NO_VALUE=999; 

class QryStmtAttribute : public NABasicObject {
  friend class QryStmtAttributeSet;
  friend class CmpMain;
  SQLATTR_TYPE  attr; // name of query statement attribute
  ULng32 value;// value of query statement attribute

 public:
  // default constructor
  QryStmtAttribute() : attr(SQL_ATTR_NOT_SET), value(SQL_ATTR_NO_VALUE) {}

  // constructor
  QryStmtAttribute(SQLATTR_TYPE a, ULng32 v) : attr(a), value(v) {}

  // copy constructor
  QryStmtAttribute(const QryStmtAttribute& s) : attr(s.attr), value(s.value) {}

  // destructor 
  virtual ~QryStmtAttribute() {}  // LCOV_EXCL_LINE  

  // equality comparison used by CompilerEnv::isEqual
  NABoolean isEqual(const QryStmtAttribute &o) const 
    { return attr == o.attr && value == o.value; }
};

const Int32 MAX_STMT_ATTR=4;

class QryStmtAttributeSet : public NABasicObject {
  friend class CmpMain;
  friend class CompilerEnv;
  // array of query statement attributes
  QryStmtAttribute attrs[MAX_STMT_ATTR];
  Int32              nEntries; // number of entries in attrs

 public:
  // constructor for query statement attribute settings
  QryStmtAttributeSet();

  // copy constructor for query statement attribute settings
  QryStmtAttributeSet(const QryStmtAttributeSet& s);

  // destructor frees all query statement attribute settings
  virtual ~QryStmtAttributeSet() {} // LCOV_EXCL_LINE  

  // add a query statement attribute to attrs
  void addStmtAttribute(SQLATTR_TYPE a, ULng32 v);

  // return true if set has given statement attribute
  NABoolean has(const QryStmtAttribute& a) const;

  // return xth statement attribute
  const QryStmtAttribute& at(Int32 x) const { return attrs[x]; }
};

// -----------------------------------------------------------------------
// CmpMain class contains the interface to main sql compiler functions.
// -----------------------------------------------------------------------

class CmpMain
{
public:

  // The return code from the sqlcomp procedure.
  enum ReturnStatus
  { SUCCESS = 0, PARSERERROR, BINDERERROR, TRANSFORMERERROR,
    NORMALIZERERROR, CHECKRWERROR, OPTIMIZERERROR, PREGENERROR, 
    GENERATORERROR, QCACHINGERROR, DISPLAYDONE, EXCEPTIONERROR, PRIVILEGEERROR,
  };

  // Optional phase after which to stop compiling.
  // Not entirely identical to enum PhaseEnum of common/BaseTypes.h
  // which is not really being used and could be gotten rid of...
  // (Also not identical to -- less finely grained than --
  // enum Sqlcmpdbg::CompilationPhase.)
  //
  enum CompilerPhase
  { PREPARSE, PARSE, BIND, TRANSFORM, NORMALIZE, SEMANTIC_OPTIMIZE, ANALYSIS,
    OPTIMIZE, PRECODEGEN, GENERATOR, END };

  CmpMain();
  virtual ~CmpMain() {} // LCOV_EXCL_LINE  

#ifdef NA_DEBUG_GUI

  // Load the GUI display DLL.
  void initGuiDisplay(RelExpr *queryExpr);

  // Initialize Winsock code.
  static void initWSA();

  // conditionally display the query tree, or tdb's, or execution
  static NABoolean guiDisplay(Sqlcmpdbg::CompilationPhase phase, RelExpr *q);
  static void loadSqlCmpDbgDLL_msGui();
#endif

  // sqlcomp will compile a string of sql text into code from generator
  ReturnStatus sqlcomp (QueryText& input,
                        Int32 /*unused*/, 
                        char **gen_code, ULng32 *gen_code_len,
                        NAMemory *h = NULL,
                        CompilerPhase = END,
			FragmentDir **framentDir = NULL,
                        IpcMessageObjType op=CmpMessageObj::SQLTEXT_COMPILE,
                        NABoolean useQueryCache=TRUE);

  // sqlcomp will compile a RelExpr into code from generator
  ReturnStatus sqlcomp (const char *input_str, Lng32 charset,
                        RelExpr *&queryExpr,
                        char **gen_code, ULng32 *gen_code_len,
                        NAMemory *h = NULL,
			CompilerPhase p= END,
			FragmentDir **fragmentDir = NULL,
                        IpcMessageObjType op=CmpMessageObj::SQLTEXT_COMPILE,
                        NABoolean useQueryCache=FALSE,
                        NABoolean* cacheable=NULL,
                        TimeVal* begTime=NULL,
                        NABoolean shouldLog=FALSE);

  // sqlcomp will compile a string of sql text into code from generator.
  // This string is from a static program and is being recompiled
  // dynamically at runtime.
  ReturnStatus sqlcompStatic (QueryText& input,
                              Int32 /*unused*/,
                              char **gen_code, ULng32 *gen_code_len,
                              NAMemory *h = NULL,
                              CompilerPhase = END,
                              IpcMessageObjType op=CmpMessageObj::SQLTEXT_COMPILE
                              );

  RelExpr *transform(NormWA &normWA, RelExpr *queryExpr);

  // QSTUFF
  // needed to check when creating a view
  RelExpr *normalize(NormWA &normWA, RelExpr *queryExpr);
  // QSTUFF

  // throw EH_INTERNAL_EXCEPTION ,  in this case CmpStatement::exceptionRaised
  // method should be called for the current CmpStatement to perform cleanup,
  // the main logic should be able to compile next statement.
  // throw EH_BREAK_EXCEPTION , in this case the main program should exit,
  // the environment is not suitable for furthur compilation.
  //
  // will end the transaction if endTrans is TRUE. It should
  // be called before returning to caller in sqlcomp procedures.
  void sqlcompCleanup(const char *input_str,
                      RelExpr *queryExpr,
                      NABoolean endTrans);

  // input arrays maximum size. Required to bind rowsets from ODBC. 
  // Value is obtained from CLI statement attribute.
  void setInputArrayMaxsize(const ULng32 maxsize);
  ULng32 getInputArrayMaxsize() const;

  // save non-zero parserFlags
  void setSqlParserFlags(ULng32 f);

  // get query statement attributes
  const QryStmtAttributeSet& getStmtAttributes() const { return attrs; }

  void setRowsetAtomicity(const short atomicity) ;
  RelExpr::AtomicityType getRowsetAtomicity() const;
  void setHoldableAttr(const short holdable) ;
  SQLATTRHOLDABLE_INTERNAL_TYPE getHoldableAttr() ;

  void FlushQueryCachesIfLongTime(Lng32 begTimeInSec);

#ifdef NA_DEBUG_GUI
  // GSH : The following variable is used by the tdm_sqlcmpdbg.

  static CmpContext        * msGui_     ;
  static SqlcmpdbgExpFuncs* pExpFuncs_;
#endif

#ifndef NDEBUG
static Lng32       prev_QI_Priv_Value ;
#endif

private:

  CmpMain(const CmpMain&);
  const CmpMain& operator=(const CmpMain&);
  QryStmtAttributeSet attrs;

  ReturnStatus compile(const char *input_str, Lng32 charset,
		       RelExpr *&queryExpr,
		       char **gen_code, 
		       ULng32 *gen_code_len,
		       NAMemory *h,
		       CompilerPhase p,
		       FragmentDir **fragmentDir,
		       IpcMessageObjType op,
		       NABoolean useQueryCache,
		       NABoolean* cacheable,
		       TimeVal* begTime,
                       NABoolean shouldLog);

  // try to compile a query using the cache
  NABoolean compileFromCache
  (const char* sText,  // (IN) : sql statement text
   Lng32     charset,   // (IN) : character set of  sql statement text
   RelExpr* queryExpr, // (IN) : the query to be compiled
   BindWA&  bindWA,    // (IN) : work area (used by backpatchParams)
   CacheWA& cachewa,   // (IN) : work area for normalizeForCache
   char**   plan,      // (OUT): compiled plan or NULL
   ULng32 *pLen,// (OUT): length of compiled plan or 0
   NAHeap*  heap,      // (IN) : heap to use for compiled plan
   IpcMessageObjType op,//(IN): SQLTEXT_COMPILE or SQLTEXT_RECOMPILE
   NABoolean& bPatchOK, //(OUT): true iff backpatch succeeded
   TimeVal&   begTime); //(IN):  start time for this compile

   void getAndProcessAnySiKeys(TimeVal begTime);

   Int32 getAnySiKeys( 
     TimeVal   begTime,           // (IN) start time for compilation
     TimeVal   prev_QI_inval_time, // (IN) previous Query Invalidation time
     Int32 *   retNumSiKeys,       // (OUT) Rtn'd size of results array
     TimeVal * pMaxTimestamp,      // (OUT) Rtn'd max Time Stamp
     SQL_QIKEY *qiKeyArray,        // (OUT) Rtn'd keys stored here
     Int32 qiKeyArraySize );       // (IN) Size of of results array

   void InvalidateNATableCacheEntries(Int32 returnedNumSiKeys, 
                                      SQL_QIKEY *qiKeyArray);
   void InvalidateNARoutineCacheEntries(Int32 returnedNumSiKeys, 
                                        SQL_QIKEY *qiKeyArray);
   void InvalidateHistogramCacheEntries(Int32 returnedNumSiKeys, 
                                        SQL_QIKEY *qiKeyArray);
   void RemoveMarkedNATableCacheEntries();
   void UnmarkMarkedNATableCacheEntries();

   // NOTE: We don't currently put the NARoutine objects associated with SPJs
   //       into the NARoutine Cache.   The UDF Proof-Of-Concept projects have
   //       added code to put NARoutine objects associated with UDFs into the
   //       NARoutine Cache, but we don't currently support UDFs.
   //       If we ever support UDFs or if we start caching the NARoutine objects
   //       associated with SPJs, we will probably need the folowing:

   // void InvalidateNARoutineCacheEntries( Int32 NumSiKeys, SQL_QIKEY * pSiKeyArray );
   // void RemoveMarkedNARoutineCacheEntries();
   // void UnmarkMarkedNARoutineCacheEntries();
};

RelRoot *CmpTransform(RelExpr *queryExpr);      // global func for convenience

const Int32 CmpDescribeSpaceCountPrefix = sizeof(short);

short CmpDescribe        (const char *describeStmt,
                          const RelExpr *queryExpr,
                          char *&outbuf,
                          ULng32 &outbuflen,
                          NAMemory *h);

short CmpDescribeControl (Describe *d,
                          char *&outbuf,
                          ULng32 &outbuflen,
                          NAMemory *h);


#endif


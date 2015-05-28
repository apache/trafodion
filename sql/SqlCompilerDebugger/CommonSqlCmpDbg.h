// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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
#ifndef _COMMMONSQLCMPDBG_H
#define _COMMMONSQLCMPDBG_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         CommonSqlCmpDbg.h
 * Description:  This file contains declarations common to arkcmp components
 *               and tdm_sqlcmpdbg, the GUI tool used to display query
 *		 compilation.
 *****************************************************************************
 */

#include "seaquest/sqtypes.h"
#include "Platform.h"

#include "GroupAttr.h"
#include "Cost.h"
#include "opt.h"

#include "ExprNode.h"
#include "ItemColRef.h"
#include "OperTypeEnum.h"
#include "RelExpr.h"
#include "Analyzer.h"

// SqldbgTDBView.h : header file
#include "ex_stdh.h"
#include "ex_queue.h"
#include "FragDir.h"
#include "ComTdb.h"
#include "ex_tcb.h"

#ifdef _c
#undef _c
#endif /*  */

#ifdef COMPILER_GUI_DEBUG_LOG_ON
/*print debug log to file*/
#include <ctime>
#include <cstring>
#define LOG_FILE "~/CompGUIdbg.log"
#define dbgprint(format,args...) fprintf(stderr, format, ##args)
#define logprint(format,args...) {char buf[128]; time_t rawtime;\
                                  time(&rawtime); sprintf(buf, "%s", ctime(&rawtime));\
                                  int len = strlen(buf);buf[len-1] = '\0'; \
                                  FILE* fp=fopen(LOG_FILE,"a+"); \
                                  fprintf(fp,"[%s:%d@@%s] "format,\
                                   __FILE__,__LINE__,buf,##args);fclose(fp);}
#else
#define dbgprint(format,args...)
#define logprint(format,args...)
#endif

#define IDX_ROOT 0
#define IDX_JOIN 1
#define IDX_SCAN 2
#define IDX_GENERIC 3

#include "ComSqlcmpdbg.h"
#if 0
class Sqlcmpdbg
{
  // This class exists merely to give a nice naming scope for this enum
public:
  enum CompilationPhase
  {
    AFTER_PARSING,
    AFTER_BINDING,
    AFTER_TRANSFORMATION,
    AFTER_NORMALIZATION,
    AFTER_SEMANTIC_QUERY_OPTIMIZATION,
    DURING_MVQR,
    AFTER_ANALYZE,
    AFTER_OPT1,
    AFTER_OPT2,
    AFTER_PRECODEGEN,
    AFTER_CODEGEN,
    AFTER_TDBGEN,
    DURING_EXECUTION,
    DURING_MEMOIZATION,
    FROM_MSDEV
  };
};

typedef struct tagSqlcmpdbgExpFuncs
{
  void (*fpDisplayQueryTree) (Sqlcmpdbg::CompilationPhase, void *, void *);
  void (*fpSqldbgSetPointers) (void *, void *, void *, void *, void *);
  void (*fpDoMemoStep) (Int32, Int32, Int32, void *, void *, void *);
  void (*fpHideQueryTree) (BOOL);
  void (*fpDisplayTDBTree) (Sqlcmpdbg::CompilationPhase, void *, void *);
  NABoolean (*fpDisplayExecution) (void);
  void (*fpCleanUp) (void);
} SqlcmpdbgExpFuncs;
typedef SqlcmpdbgExpFuncs *(*fpGetSqlcmpdbgExpFuncs) (void);
#endif

typedef struct tagSQLDebugBrkPts
{
  tagSQLDebugBrkPts ():brkAfterParsing (FALSE),
    brkAfterBinding (FALSE),
    brkAfterTransform (FALSE),
    brkAfterNormalize (FALSE),
    brkAfterSemanticQueryOptimization (FALSE),
    brkAfterAnalyze (FALSE),
    brkAfterOpt1 (FALSE),
    brkAfterOpt2 (FALSE),
    brkAfterPreCodegen (FALSE),
    brkAfterCodegen (FALSE),
    brkAfterTDBgen (FALSE), brkDuringExecution (FALSE)
  {  }
  NABoolean brkAfterParsing;
  NABoolean brkAfterBinding;
  NABoolean brkAfterTransform;
  NABoolean brkAfterNormalize;
  NABoolean brkAfterSemanticQueryOptimization;
  NABoolean brkAfterAnalyze;
  NABoolean brkAfterOpt1;
  NABoolean brkAfterOpt2;
  NABoolean brkAfterPreCodegen;
  NABoolean brkAfterCodegen;
  NABoolean brkAfterTDBgen;
  NABoolean brkDuringExecution;
} SQLDebugBrkPts;

#endif // _COMMONSQLCMPDBG_H

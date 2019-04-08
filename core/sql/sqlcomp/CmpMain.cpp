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
 * File:         cmpmain.C
 * Description:  The compiler main function, sqlcomp, which calls parser,
 *               binder, ....etc. for SQL compilation.
 *
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *****************************************************************************
 */


#define   SQLPARSERGLOBALS_FLAGS        // should precede all other #include's

#include "Platform.h"
#include <stdlib.h>
#include <fstream>

#ifdef _DEBUG
#include <string.h>
#endif

#include <dlfcn.h>  


#include "ComMemoryDiags.h"		// ComMemoryDiags::DumpMemoryInfo()

#include "CmpContext.h"
#include "Analyzer.h"
#include "AllRelExpr.h"
#include "BindWA.h"
#include "CacheWA.h"

#include "ComDiags.h"
#include "CmpCommon.h"
#include "CmpErrors.h"
#include "CmpMain.h"
#include "CmpMemoryMonitor.h"
#include "CompilerTracking.h"
#include "CompilationStats.h"
#include "ComSysUtils.h"
#include "ComTdbRoot.h"
#include "FragDir.h"
#include "Generator.h"
#include "NAExit.h"                     // NAExit()
#include "NormWA.h"
#include "opt.h"
#include "OptimizerSimulator.h"
#include "parser.h"
#include "PhyProp.h"
#include "SQLCLIdev.h"
#include "Sqlcomp.h"
#include "StmtNode.h"
#include "QueryText.h"
#include "ControlDB.h"                  // ActiveControlDB()
#include "CmpISPInterface.h"
#include "ComUser.h"
#include "ComDistribution.h"

#define   SQLPARSERGLOBALS_NADEFAULTS
#include "SqlParserGlobalsCmn.h"
#define SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#include "SqlParserGlobals.h"           // should be last #include

#include "CompException.h"

#include "logmxevent.h"
#include "hs_util.h"
#include "CmpStatement.h"
#include "PCodeExprCache.h"


// -----------------------------------------------------------------------
// some variables for debugging (command line options)
// -----------------------------------------------------------------------
extern NABoolean DontOpt;
extern NABoolean FindLeaks;

//
// The following two variables get zeroed and incremented ONLY ... never
// decremented.  No code currently examines them, but they could be 
// examined with gdb.  Having them be declared as THREAD_P should be 
// sufficient.  We might even make them global in the future since it 
// would seem that the counts don't necessarily have to be accurate.
//
THREAD_P Int32 CostScalar::ovflwCount_ = 0;
THREAD_P Int32 CostScalar::udflwCount_ = 0;

#if !defined(NDEBUG)
  NABoolean TraceCatManMemAlloc = FALSE;
#endif

// -----------------------------------------------------------------------
// GSH : Definition of the static member variables of CmpMain.
// -----------------------------------------------------------------------
#ifdef NA_DEBUG_GUI

//
// CmpMain::msGui_ is left as a 'global' because the SqlCompilerDebugger
// is not thread-safe and there is no plan to make it such.  We would,
// however, like to be able to use the debugger with any thread within
// sqlci.  So, we make CmpMain::msGui_ be a pointer which tells us which
// compiler instance currently "owns" the SqlCompilerDebugger interface.
// ONLY one sqlci thread is allowed to own this interface at a time.
//
CmpContext * CmpMain::msGui_ = NULL ;


//
// Since only one thread can own the SqlCompilerDebugger interface at a
// time, CmpMain::pExpFuncs_ can be a 'global' as well.
//
SqlcmpdbgExpFuncs* CmpMain::pExpFuncs_ = NULL;
void initializeGUIData(SqlcmpdbgExpFuncs* expFuncs);


//
// Since only one thread can own the SqlCompilerDebugger interface at a
// time, dlptr can be a 'global' as well.
//
static void* dlptr = NULL;
#define DISPLAY_IN_ODBC_JDBC -2243
#endif

#ifndef NDEBUG
Lng32  CmpMain::prev_QI_Priv_Value = 0 ;
#endif

// in release mode and if running regressions and within special parserflags
// mode, enable TESTEXIT.
// This is to force mxcmp abend to test certain queries, like recovery
// after mxcmp failure.
// +7 below is to get past "insert " in input_str, e.g. "insert INTOINSPECT;"
static Int32 sqlcompTestExit(QueryText& input)
{
  if (! Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
    return 0;

  char *input_str;
  if (!input.canBeUsedBySqlcompTest(&input_str)) {
    // all the test hooks below work only for ISO88591 (ie, ascii)
    return 0;
  }
  input_str += 7;

  if (strncmp(input_str, "TESTEXIT", 8) == 0)
    NAExit(0);

  return 0;
}

#ifdef _DEBUG
// for debugging only
// +7 below is to get past "insert " in input_str, e.g. "insert INTOINSPECT;"
static Int32 sqlcompTest(QueryText& input)
{
  char *input_str;
  if (!input.canBeUsedBySqlcompTest(&input_str)) {
    // all the test hooks below work only for ISO88591 (ie, ascii)
    return 0;
  }
  input_str += 7;

  if (strncmp(input_str, "INTOINSPECT", strlen("INTOINSPECT")) == 0)
  {
    NADebug();
    return 1;
  }

  if ( strncmp(input_str, "TEST", 4) == 0 )
  {
    CMPASSERT(strncmp(input_str, "TESTASSERT", 10) != 0);
    assert(strncmp(input_str, "TESTSYSASSERT", 13) != 0);

    if ( strncmp(input_str, "TESTNAASSERT", 12) == 0 )
      NAAssert("", "sqlcompTest", 188);
    else if ( strncmp(input_str, "TESTNAABORT", 11) == 0 )
      NAAbort("", 200, "sqlcompTest");
    else if (strncmp(input_str, "TESTEXIT", 8) == 0)
      NAExit(0);

    else if (strncmp(input_str, "TESTBREAK", 9) == 0)
    {
      *CmpCommon::diags() << DgSqlCode(2990);
      CMPBREAK;
    }
    else if (strncmp(input_str, "TESTABORT", 9) == 0)
    {
      CMPABORT;
    }
    else if (strncmp(input_str, "TESTNOMEMSTMT", 13) == 0)
    {
      char* sp;
      while ( sp = new(CmpCommon::statementHeap()) char[300000000] ) {
        sp++; // to silence c89's warning: 'variable sp was set but never used'
      }
    }
    else if (strncmp(input_str, "TESTNOMEMCX", 11) == 0)
    {
      char* sp =0;
      while ( sp = new(CmpCommon::contextHeap()) char[300000000] );
    }
    else if (strncmp(input_str, "TESTNOMEM", 9) == 0 )
    {
      // intentional allocation to cause out-of-memory condition
      // coverity[returned_pointer]
      // coverity[overwrite_var]
      // coverity[leaked_storage]
      char* sp = new char[300000000];
    }
    else if (strncmp(input_str, "TESTCHARSET", 11) == 0 )
    {
      NAString csname(&input_str[11]);
      size_t p = csname.index(';');
      if (p != NA_NPOS) csname.remove(p);
      TrimNAStringSpace(csname);
      csname.toUpper();
      CharInfo::toggleCharSetSupport(CharInfo::getCharSetEnum(csname));
    }

    return 1;
  }

  static const char *cmd = "TRACECATMANMEMALLOC ";
  if (strncmp(input_str, cmd, strlen(cmd)) == 0)
  {
    return 1;
  }

  return 0;
}
#endif  // !defined(NDEBUG)

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
#ifdef NA_DEBUG_GUI
void CmpMain::initGuiDisplay(RelExpr *queryExpr)
{
}
#endif  // NA_DEBUG_GUI

  // Static global convenient function to log query expression to output.
  static void logQueryExpr( DefaultConstants nadef,
                            const char *txt,
                            RelExpr *queryExpr )
  {
    // for NSK only
    if (( CmpCommon::getDefault(NSK_DBG) == DF_ON ) &&
        ( CmpCommon::getDefault(nadef) == DF_ON ))
    {
      CURRCONTEXT_OPTDEBUG->stream() << "Query expression after " << txt << ":" << endl;
      CURRCONTEXT_OPTDEBUG->showTree( queryExpr, NULL, "  " );
      CURRCONTEXT_OPTDEBUG->stream() << endl;
    }
  }

#ifdef NA_DEBUG_GUI
  NABoolean CmpMain::guiDisplay(Sqlcmpdbg::CompilationPhase phase, RelExpr* q)
  {
      if (((RelRoot *)q)->getDisplayTree() && CmpMain::pExpFuncs_)
      {
        initializeGUIData(CmpMain::pExpFuncs_);
        CmpMain::pExpFuncs_->fpDisplayQueryTree(phase, q, NULL);
      }
    return FALSE;
  }
#endif

#ifdef NA_DEBUG_GUI

#define GUI_or_LOG_DISPLAY(phase,nadef,txt) {if(CmpMain::msGui_ == CmpCommon::context())guiDisplay(phase,queryExpr);logQueryExpr(nadef,txt,queryExpr);}

#else
#define GUI_or_LOG_DISPLAY(phase,nadef,txt) {logQueryExpr(nadef,txt,queryExpr);}
#endif

#ifdef NA_DEBUG_GUI
//--------------------------------------------------------------------
// GSH : If the user wishes to use tdm_sqlcmpdbg then load
// the appropriate DLL and get the pointer to the structure which
// contains all the exported functions in the DLL.
//--------------------------------------------------------------------
void CmpMain::loadSqlCmpDbgDLL_msGui()
{
  if ( CmpMain::msGui_ == NULL ) // If no Compiler Instance is currently using debugger
  {
     CmpMain::msGui_ = CmpCommon::context();     // Claim ownership of debugger
     fpGetSqlcmpdbgExpFuncs GetExportedFunctions;
  
     dlptr = dlopen("libSqlCompilerDebugger.so",RTLD_NOW); //TODO
     if(dlptr != NULL)
     {
        GetExportedFunctions = (fpGetSqlcmpdbgExpFuncs) dlsym(dlptr, "GetSqlcmpdbgExpFuncs");
        if (GetExportedFunctions)
             CmpMain::pExpFuncs_ = GetExportedFunctions();
        if ( CmpMain::pExpFuncs_ == NULL )
        {
           int ret = dlclose(dlptr);
           dlptr = NULL;
        }
     }
     else // dlopen() failed 
     { 
        char *msg = dlerror(); 
     }
     if ( dlptr == NULL )
        CmpMain::msGui_ = NULL ;  //This Compiler Instance cannot use debugger
  }
  // free the DLL module
}
#endif // NA_DEBUG_GUI


#ifndef NDEBUG
#define ENTER_TASK_MONITOR(x) \
  { if (CURRSTMT_OPTDEFAULTS->compileTimeMonitor()) x.enter(); }
#define EXIT_TASK_MONITOR(x) \
  { if (CURRSTMT_OPTDEFAULTS->compileTimeMonitor()) x.exit(); }
#else
#define ENTER_TASK_MONITOR(x)
#define EXIT_TASK_MONITOR(x)
#endif

#define ENTERMAIN_TASK_MONITOR(x) \
  { if (CURRSTMT_OPTDEFAULTS->compileTimeMonitor()) x.enter(); }
#define EXITMAIN_TASK_MONITOR(x) \
  { if (CURRSTMT_OPTDEFAULTS->compileTimeMonitor()) x.exit(); }

    
extern char *__progname;

static const char *getCOMPILE_TIME_MONITOR_OUTPUT_FILEname()
{
  const char * fname = ActiveSchemaDB()->getDefaults().
			 getValue(COMPILE_TIME_MONITOR_OUTPUT_FILE);

  static char fnameBuf[100];
  if (fname && stricmp(fname,"NONE") != 0)  {

    pid_t pid = getpid();

    NAString x(fname);
    x.append(".");
    x.append(__progname);
    x.append(".");

    char buf[30];
    str_itoa((ULng32)pid, buf);

    x.append(buf);

    if ( x.length() < sizeof(fnameBuf) ) {
      strncpy(fnameBuf, x.data(), x.length());
      return fnameBuf;
    }
  } 

  return NULL;
}

void CmpMain::sqlcompCleanup(const char *input_str,
                             RelExpr *queryExpr,
                             NABoolean endTran)
{
  CURRCONTEXT_HISTCACHE->resetAfterStatement();
  
  CURRENTQCACHE->getHQC()->setPlanNoAQROrHiveAccess(FALSE);
  CURRENTQCACHE->getHQC()->setCurrKey(NULL);
  if (input_str)                // pass in as NULL if success cleanup
    if (CmpCommon::diags() &&
        CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) <= 0)
      *CmpCommon::diags() << DgSqlCode(arkcmpErrorNoDiags)
                          << DgString0(input_str);

  // Cleanup the remains left over by optimizer and previous steps
  if (queryExpr)
    queryExpr->cleanupAfterCompilation();

  // Unconditionally do this on exit -- parseDML does not do this for us since
  // we set DELAYED_RESET, so flag values are preserved for remainder of
  // the successful compilation phases.
  //
  // (And even if that were not the case,
  // one of these later phases, namely the Binder, could have set the flags
  // but returned before itself calling parseDML; we must reset here so
  // the next statement compiled does not erroneously inherit such settings.)
  //
  Set_SqlParser_Flags(0);      
  //
  // tracking compilers...  
  Lng32 trackingInterval = 
    ActiveSchemaDB()->getDefaults().getAsLong(COMPILER_TRACKING_INTERVAL);

  if( 0 < trackingInterval &&
     ! CmpCommon::context()->isSecondaryMxcmp() )
  {    
    CompilerTrackingInfo *cmpTracker = CmpCommon::context()->getCompilerTrackingInfo();

    //
    // mark the system, statement, and qcache interval heap sizes since these
    // may not live across the entire interval
    cmpTracker->updateSystemHeapWtrMark();
    cmpTracker->updateStmtIntervalWaterMark();            
    //
    // add to our compiled queries counter with TRUE if successfully compiled
    // or FALSE if not
    cmpTracker->incrementQueriesCompiled(
                        0==CmpCommon::diags()->getNumber(DgSqlCode::ERROR_));   
    //
    // add these exceptions to caught exception counter
    //
    if( CmpCommon::diags()->contains(arkcmpErrorAfterPassOne) ||
        CmpCommon::diags()->contains(arkcmpOptimizerAssertionWarning) )
    {
      cmpTracker->incrementCaughtExceptions();
    }
    // pass the cpu time for this compilation to the tracker. it will determine
    // whether its the longest
    cmpTracker->updateLongestCompile(
      CURRENTSTMT->getCompilationStats()->cmpPhaseLength(CompilationStats::CMP_PHASE_ALL));
    //
    // monitor the tracking interval and do the log and reset if the
    // interval has expired
    cmpTracker->logCompilerStatusOnInterval(trackingInterval);
  }
  
  if( dlptr  &&  CmpMain::msGui_ == CmpCommon::context() && CURRENTSTMT->displayGraph())
  {	
    int ret = dlclose(dlptr);
    dlptr = NULL;
    CmpMain::msGui_ = NULL;
    CmpMain::pExpFuncs_ = NULL;
  }	

  CURRENTSTMT->clearDisplayGraph();
}

RelExpr *CmpMain::transform(NormWA &normWA, RelExpr *queryExpr)
{
  CMPASSERT(queryExpr && queryExpr->nodeIsBound());
  ExprGroupId eg(queryExpr);
  queryExpr->transformNode(normWA, eg);
  return eg.getPtr();
}

RelRoot *CmpTransform(RelExpr *queryExpr)	// global convenience func
{
  CmpMain cmp;
  NormWA normWA(CmpCommon::context());

  CMPASSERT(queryExpr->getOperatorType() == REL_ROOT);
  ((RelRoot *)queryExpr)->setRootFlag(TRUE);

  queryExpr = cmp.transform(normWA, queryExpr);

  CMPASSERT(queryExpr->getOperatorType() == REL_ROOT);
  return (RelRoot *)queryExpr;
}

// QSTUFF
RelExpr *CmpMain::normalize(NormWA &normWA, RelExpr *queryExpr)
{
  CMPASSERT(queryExpr && queryExpr->nodeIsBound());
  ExprGroupId eg(queryExpr);
  return queryExpr->normalizeNode(normWA);
}
// QSTUFF

// Another global convenience function
// This function dumps process memory usage info into
// the given stream
void DumpProcessMemoryInfo(ostream *outstream)
{
}


// return a query's localized statement text
static
char* localizedText(QueryText& qt, CharInfo::CharSet &localizedTextCharSet, NAMemory* heap)
{
  localizedTextCharSet = (CharInfo::CharSet)qt.charSet();
  if (qt.charSet() == SQLCHARSETCODE_UCS2) {
    // convert UCS2 to locale-based text
    size_t ucs2Len = qt.length();
    NAWcharBuf tmpNAWcharBuf((NAWchar *)qt.wText(), (Int32)ucs2Len, heap);
    Int32 iErrorCode = 0;
    Int32 targetCS = (Int32)CharInfo::UTF8;
    charBuf *pCharBuf = NULL; // must be set to NULL to allocate new space
    pCharBuf = parserUTF16ToCharSet ( tmpNAWcharBuf     // in  - const NAWcharBuf&
                                    , heap              // in  - CollHeap *
                                    , pCharBuf          // in/out - charBuf* &
                                    , targetCS          // out - Int32 &   pr_iTargetCharSet
                                    , iErrorCode        // out - Int32 &   pr_iErrrorCode
                                    , TRUE              // in  - NABoolean pv_bAddNullAtEnd
                                    , FALSE             // in  - NABoolean pv_bAllowInvalidCodePoint
                                    );

    char * localizedTextBufp = CURRENTSTMT->getLocalizedTextBuf() ;
    if (iErrorCode == 0 && pCharBuf != NULL && pCharBuf->getStrLen() > 0)
    {
      Int32 spaceNeeded = pCharBuf->getStrLen() + 1; // includes the null terminator
      if ( spaceNeeded > CURRENTSTMT->getLocalizedTextBufSize() ) {
         if ( localizedTextBufp ) {
            NADELETEBASIC( localizedTextBufp , heap );
         }
         localizedTextBufp = new (heap) char[ spaceNeeded ] ;
         CURRENTSTMT->setLocalizedTextBuf( localizedTextBufp ) ;
         CURRENTSTMT->setLocalizedTextBufSize( spaceNeeded ) ;
      }
      memcpy((void *)localizedTextBufp, (void *)pCharBuf->data(), spaceNeeded );
      NADELETE(pCharBuf, charBuf, heap);
      pCharBuf = NULL;
      localizedTextCharSet = (CharInfo::CharSet)targetCS;
    }
    return localizedTextBufp ;
  }
  // charset must be one of the locale-based character sets
  return qt.text();
}

void CmpMain::FlushQueryCachesIfLongTime(Lng32 begTimeInSec)
{

  static Lng32 max_cache_flush_time = 24 * 60 * 60;  // 24 hours by default
  static bool gotSikGcInterval = FALSE;

  static char *sct = getenv("RMS_SIK_GC_INTERVAL_SECONDS");
  if (sct != NULL)
  {
         max_cache_flush_time = (Lng32)str_atoi(sct, str_len(sct)) ;
         if (max_cache_flush_time < 10)
             max_cache_flush_time = 10;
  }

  // NOTE: For this purpose, we can ignore the microseconds part of the TimeVal.
  Lng32 amt_time_passed = begTimeInSec - cmpCurrentContext->getPrev_QI_sec();

  if (amt_time_passed > max_cache_flush_time )
  {
     TimeVal tmpTime ;
     tmpTime.tv_sec  = begTimeInSec ;
     tmpTime.tv_usec = 0 ;
     cmpCurrentContext->setPrev_QI_time( tmpTime );
     CURRENTQCACHE->makeEmpty();    // Flush entire query cache

     // Flush entire NATable cache
     CmpCommon::context()->schemaDB_->getNATableDB()->setCachingOFF();
     if ( CmpCommon::context()->schemaDB_->getNATableDB()->cachingMetaData() )
          CmpCommon::context()->schemaDB_->getNATableDB()->setCachingON();
  }

}

// -----------------------------------------------------------------------
// This sqlcomp procedure compiles a string of sql text into code from
// generator. It will call parser to parse the input string into a RelExpr*
// and call sqlcomp(char*, RelExpr*, ...) for code generation.
//
// ...Unless phase is passed in, in which case the tree for that compiler phase
// is returned in the gen_code pointer (I know, wrong name), which the caller
// needs to cast back to RelExpr*.
// -----------------------------------------------------------------------

CmpMain::ReturnStatus CmpMain::sqlcomp(QueryText& input,            //IN
                                       Int32 /*input_strlen*/,        //UNUSED
                                       char **gen_code,             //OUT
                                       ULng32 *gen_code_len, //OUT
                                       CollHeap *heap,              //IN
                                       CompilerPhase phase,         //IN
                                       FragmentDir **fragmentDir,   //OUT
                                       IpcMessageObjType op,        //IN
                                       QueryCachingOption useQueryCache) //IN
{
  TimeVal begTime;
  GETTIMEOFDAY(&begTime, 0);

  FlushQueryCachesIfLongTime(begTime.tv_sec);

  // initialize the return values
  *gen_code = 0;
  *gen_code_len = 0;
  RelExpr *queryExpr = NULL;

  // Init Query Analysis Global Instance
  QueryAnalysis* queryAnalysis = CmpCommon::statement()->initQueryAnalysis();

  //CompGlobals* globes = CompGlobals::InitGlobalInstance();
  CompilationStats* stats = CURRENTSTMT->getCompilationStats();
  stats->takeSnapshotOfCounters();

  if( op == CmpMessageObj::SQLTEXT_RECOMPILE ||
      op == CmpMessageObj::SQLTEXT_STATIC_RECOMPILE)
  {
    stats->setIsRecompile();
  }

  // Log the query for which the memory usage is being monitored 
  // using Optimizer Memory Monitor(OMM).
  CharInfo::CharSet localizedTextCharSet = CharInfo::UnknownCharSet;
  MonitorMemoryUsage_QueryText(localizedText(input, localizedTextCharSet, heap));

  MonitorMemoryUsage_Enter("Compiler Total");

  stats->enterCmpPhase(CompilationStats::CMP_PHASE_ALL);
  ENTERMAIN_TASK_MONITOR( queryAnalysis->compilerMonitor() );

#ifdef _DEBUG
  sqlcompTest(input);
#else
  sqlcompTestExit(input);
#endif

  ExprNode *expr;
  Set_SqlParser_Flags(DELAYED_RESET);	// sqlcompCleanup resets for us
  Parser parser(CmpCommon::context());
  //set a reference to construct hybrid cache key during parsing 
  if(CmpCommon::getDefault(HYBRID_QUERY_CACHE) == DF_ON)
  {
      HQCParseKey* key = new (STMTHEAP)  HQCParseKey (new (STMTHEAP) CompilerEnv(STMTHEAP, PARSE, getStmtAttributes()), STMTHEAP);
      parser.setHQCKey(key);
      CURRENTQCACHE->getHQC()->setCurrKey(key);
  }
  
  if (input.isNullText()) {
    sqlcompCleanup("", queryExpr, FALSE);
    return PARSERERROR;
  }
  // make at most 2 tries to compile a query: 1st with caching on
  // (assuming our caller wants it on), and
  // on any error, retry it with caching off if query was cacheable
  NABoolean cacheable=FALSE, useTextCache=TRUE;

  // if running in OSIM capture mode or simulation, don't use the cache
  if (OSIM_runningInCaptureMode()||OSIM_runningSimulation())
  {
    useQueryCache = NOCACHE;
    useTextCache = FALSE;
  }

  // text cache backpatching errors should never happen -- we'll
  // assert if they do. There are no known cases where a text-cache-on
  // sqlcomp fails but a text-cache-off sqlcomp succeeds.
  Lng32 here = CmpCommon::diags()->mark();
  CmpMain::ReturnStatus rs = PARSERERROR;

  ULng32 originalParserFlags = Get_SqlParser_Flags(0xFFFFFFFF);

  NABoolean Retried_for_priv_failure = FALSE;
  NABoolean Retried_without_QC       = FALSE;

  for (Int32 x=0; x < 3; x++) {
    //
    // Do any Query Invalidation that must be done due to recent REVOKE
    // commands being done.
    //
    getAndProcessAnySiKeys(begTime);

    // try to compile query via (pre parse stage) cache hit
    NABoolean bPatchOK=FALSE;
    char *sqlText = localizedText(input, localizedTextCharSet, heap);
    if (useTextCache && phase == END) {
      CURRENTQCACHE->incNOfCompiles(op);
      CacheWA cachewa(CmpCommon::statementHeap());
      cachewa.setPhase(PREPARSE);
      BindWA bindWA(ActiveSchemaDB(), CmpCommon::context());
      // coverity thinks compileFromCache will dereference the null queryExpr
      // pointer. coverity drills down one level of calls but that's not good
      // enough. queryExpr is passed down several levels of calls until it
      // ends up in QCache::deCacheAll(CacheKey*,RelExpr*) where queryExpr
      // is first tested for non-null-ness before it is dereferenced. Yes,
      // compileFromCache has one code path which dereferences queryExpr
      // without a non-null guard. But, that code path is guarded by an
      // "if (phase == CmpMain::PREPARSE)" test. Too deep for coverity.
      // To be safe, we also changed compileFromCache to first check
      // queryExpr before dereferencing it. Redundant but safer.
      // coverity[var_deref_model]
      if (compileFromCache(sqlText, input.charSet(), queryExpr, bindWA,
                           cachewa, gen_code,
                           gen_code_len, (NAHeap*)heap, op,
                           bPatchOK, begTime)) {
        if (bPatchOK) {
          return SUCCESS;
        }
        else {
          // a text cache backpatch should never happen, so we
          // assert in debug. In any case, we fall thru but make
          // sure we don't retry the text cache lookup/backpatch
          CMPASSERT(bPatchOK);
          useTextCache = FALSE;
        }
      }
    }

    //Turn 'ON' use of metadata (i.e. NATable) cache. The cache
    //will only be used if metadata caching is 'ON' (i.e.
    //CmpCommon::context()->schemaDB_->getNATableDB()->cachingMetaData==TRUE)
    //We only want to use the cache for dynamic compiles, for all other
    //operations we should reconstruct the NATable object by reading the
    //metadata from disk.
    CmpCommon::context()->schemaDB_->getNATableDB()->useCache();

    //set the parser flags to the original value.
    Assign_SqlParser_Flags(originalParserFlags);

    //if using special tables e.g. using index as base table
    //select * from table (index_table T018ibc);
    //then refresh metadata cache.  Make an exception for internal exeutil
    // statements that use this parserflag. It causes too many long compiles 
    // and affects performance - for eg LOB access which uses ghost tables and 
    // has the SPECIALTABLETYPE flag set..  
    if(Get_SqlParser_Flags(ALLOW_SPECIALTABLETYPE) &&
       CmpCommon::context()->schemaDB_->getNATableDB()->cachingMetaData() &&
       !Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL) )
      CmpCommon::context()->schemaDB_->getNATableDB()->refreshCacheInThisStatement();

    MonitorMemoryUsage_Enter("Parser");
    ENTERMAIN_TASK_MONITOR( queryAnalysis->parserMonitor() );
    CompilationStats* stats = CURRENTSTMT->getCompilationStats();
    stats->enterCmpPhase(CompilationStats::CMP_PHASE_PARSER);
    if (parser.parseDML(input, &expr, 0, NULL)) {
      sqlcompCleanup(localizedText(input, localizedTextCharSet, heap), queryExpr, FALSE);
      return PARSERERROR;
    }
    EXITMAIN_TASK_MONITOR( queryAnalysis->parserMonitor() );
    MonitorMemoryUsage_Exit("Parser");
    stats->exitCmpPhase(CompilationStats::CMP_PHASE_PARSER);

    // The parser produces a Statement.
    StmtNode *stmt = expr->castToStatementExpr();
    CMPASSERT(stmt);

    // The Statement that we process here must be a query expression.
    // (In the future, we will support DDL expressions also. We are
    //  also capable of evaluating scalar expressions that can be
    //  compiled and executed dynamically. However, the parser
    //  only produces query expressions currently.)
    CMPASSERT(stmt->isAQueryStatement());
    queryExpr = stmt->getQueryExpression();

    // A query expression MUST have a RelRoot at its root.
    CMPASSERT(queryExpr->getOperatorType() == REL_ROOT);

    if (phase == PARSE)
      {
        *gen_code = (char *)queryExpr;
        return SUCCESS;
      }

    rs = sqlcomp(sqlText, input.charSet(), queryExpr, gen_code,
                 gen_code_len, heap, phase,
                 fragmentDir, op, useQueryCache,
		 &cacheable, &begTime, Retried_for_priv_failure);

    if ( rs == SUCCESS )
    {
        // the retry worked; issue a warning about the successful retry.
        // *CmpCommon::diags() << DgSqlCode(2108);
        if ( Retried_without_QC )
           SQLMXLoggingArea::logCompNQCretryEvent(input.text());
        return rs; // we're done
    }
    if ( x == 0 )
    {
       // Query Analysis Global Instance needs to be re-initialized
       // solution 10-061030-0161
       queryAnalysis = CmpCommon::statement()->initQueryAnalysis();
    }
    if ( rs != PRIVILEGEERROR )
    {
      if ( cacheable )
      {
         // retry any error (once) if query was cacheable
         useQueryCache = NOCACHE; // retry with caching off
         Retried_without_QC = TRUE;
         Retried_for_priv_failure = FALSE;
         CURRENTQCACHE->incNOfRetries(); // count retries
      }
      else
         return rs; // No additional compilation attempt!
    }
    else if ( rs == PRIVILEGEERROR )
    {
       if ( x >= 2 || Retried_for_priv_failure )
       {
          //
          // We are about to return an error. If we did a recompilation attempt,
          // we probably left NATable Cache entries marked to be deleted.
          // However, since we are not going to attempt another recompilation,
          // we no longer need those deleted.  It doesn't hurt to leave them
          // in the NATable cache and at least one regression test expects us to do so.
          //
          if ( Retried_for_priv_failure )
             UnmarkMarkedNATableCacheEntries();
          return rs;
       }

       // Update the list of roles incase a grant came in since last attempt
       // Ignore errors from Reset.  A check is made later that returns
       // an error if unable to reset the list at this time
       SQL_EXEC_ResetRoleList_Internal();

       RemoveMarkedNATableCacheEntries();
       Retried_for_priv_failure = TRUE;
    }

    CmpCommon::statement()->prepareForCompilationRetry();

    // We will retry Compilation, so toss pre-retry errors/warnings
    CmpCommon::diags()->rewind(here, TRUE/*update maxDiagsId_*/);
    if (queryExpr) {
       delete queryExpr;
       queryExpr = NULL;
    }
  }
  return rs;
}

CmpMain::ReturnStatus CmpMain::sqlcompStatic
      (QueryText& input,            //IN
       Int32 /*input_strlen*/,        //UNUSED
       char **gen_code,             //OUT
       ULng32 *gen_code_len, //OUT
       CollHeap *heap,              //IN
       CompilerPhase phase,         //IN
       IpcMessageObjType op)        //IN
{
  TimeVal begTime;
  GETTIMEOFDAY(&begTime, 0);

  FlushQueryCachesIfLongTime(begTime.tv_sec);

  // initialize the return values
  *gen_code = 0;
  *gen_code_len = 0;
  RelExpr *queryExpr = NULL;


  QueryAnalysis* queryAnalysis = CmpCommon::statement()->initQueryAnalysis();
  //CompGlobals* globes = CompGlobals::InitGlobalInstance();
  CompilationStats* stats = CURRENTSTMT->getCompilationStats();
  stats->takeSnapshotOfCounters();

  if( op == CmpMessageObj::SQLTEXT_RECOMPILE ||
      op == CmpMessageObj::SQLTEXT_STATIC_RECOMPILE)
  {
    stats->setIsRecompile();
  }

  // Log the query for which the memory usage is being monitored 
  // using Optimizer Memory Monitor(OMM).
  CharInfo::CharSet localizedTextCharSet = CharInfo::UnknownCharSet;
  MonitorMemoryUsage_QueryText(localizedText(input, localizedTextCharSet, heap));

  MonitorMemoryUsage_Enter("Compiler Total");

  stats->enterCmpPhase(CompilationStats::CMP_PHASE_ALL);
  ENTERMAIN_TASK_MONITOR( queryAnalysis->compilerMonitor() );

  ExprNode *parseTree;
  Set_SqlParser_Flags(DELAYED_RESET);   // sqlcompCleanup resets for us
  Parser parser(CmpCommon::context());
  //set a reference to construct hybrid cache key during parsing 
  if(CmpCommon::getDefault(HYBRID_QUERY_CACHE) == DF_ON)
  {
      HQCParseKey* key = new (STMTHEAP)  HQCParseKey (new (STMTHEAP) CompilerEnv(STMTHEAP, PARSE, getStmtAttributes()), STMTHEAP);
      parser.setHQCKey(key);
  }
  
  CompilationMode mode = CmpCommon::context()->GetMode();
  #ifdef _DEBUG
    CMPASSERT(mode == STMT_DYNAMIC);
    // The code below actually runs if this is False,
    // but let's assert so we actually know about the False case.
  #endif

  // make at most 2 tries to compile a query: 1st with caching on, and
  // on any error, retry it with caching off if query was cacheable
  QueryCachingOption useQueryCache = NORMAL;
  NABoolean cacheable=FALSE, useTextCache=TRUE;

  Lng32 here = CmpCommon::diags()->mark();
  CmpMain::ReturnStatus rc = PARSERERROR;
  ULng32 originalParserFlags = Get_SqlParser_Flags(0xFFFFFFFF);
  for (Int32 x=0; x<2; x++) {

    //
    // Do any Query Invalidation that must be done due to recent REVOKE
    // commands being done.
    //
    getAndProcessAnySiKeys(begTime);

    // try to compile query via (pre parse stage) cache hit
    NABoolean bPatchOK=FALSE;
    char *sqlText = localizedText(input, localizedTextCharSet, heap);
    if (useTextCache && phase == END && mode == STMT_DYNAMIC) {
      CURRENTQCACHE->incNOfCompiles(op);
      CacheWA cachewa(CmpCommon::statementHeap());
      cachewa.setPhase(PREPARSE);
      BindWA bindWA(ActiveSchemaDB(), CmpCommon::context());
      // coverity thinks compileFromCache will dereference the null queryExpr
      // pointer. coverity drills down one level of calls but that's not good
      // enough. queryExpr is passed down several levels of calls until it
      // ends up in QCache::deCacheAll(CacheKey*,RelExpr*) where queryExpr
      // is first tested for non-null-ness before it is dereferenced. Yes,
      // compileFromCache has one code path which dereferences queryExpr
      // without a non-null guard. But, that code path is guarded by an
      // "if (phase == CmpMain::PREPARSE)" test. Too deep for coverity.
      // To be safe, we also changed compileFromCache to first check
      // queryExpr before dereferencing it. Redundant but safer.
      // coverity[var_deref_model]
      if (compileFromCache(sqlText, input.charSet(), queryExpr, bindWA,
                           cachewa, gen_code,
                           gen_code_len, (NAHeap*)heap, op,
                           bPatchOK, begTime)) {
        if (bPatchOK) {
          return SUCCESS;
        }
        else {
          // a text cache backpatch should never happen, so we
          // assert in debug. In any case, we fall thru but make
          // sure we don't retry the text cache lookup/backpatch
          CMPASSERT(bPatchOK);
          useTextCache = FALSE;
        }
      }
    }

    //set the parser flags to the original value.
    Assign_SqlParser_Flags(originalParserFlags);

    CmpCommon::context()->SetMode(STMT_STATIC);
    MonitorMemoryUsage_Enter("Parser");
    ENTERMAIN_TASK_MONITOR( queryAnalysis->parserMonitor() );
    CompilationStats* stats = CURRENTSTMT->getCompilationStats();
    stats->enterCmpPhase(CompilationStats::CMP_PHASE_PARSER);
    Int32 error = parser.parseDML(input, &parseTree, 0, NULL);
    EXITMAIN_TASK_MONITOR( queryAnalysis->parserMonitor() );
    stats->exitCmpPhase(CompilationStats::CMP_PHASE_PARSER);
    MonitorMemoryUsage_Exit("Parser");
    CmpCommon::context()->SetMode(mode);
    if (error)
      {
        sqlcompCleanup(sqlText, queryExpr, FALSE);
        // "parseTree" is eventually freed by PARSERHEAP's destructor.
        // "parseTree" is aliased to global live variable "TheParseTree".
        // coverity[leaked_storage]
        return PARSERERROR;
      }

    // If this is an embedded static query, parseTree points to a STM_PROCEDURE,
    // like so:  StmtProcedure->(bodyStatement)StmtQuery->RelRoot->DDLExpr.
    //
    // Otherwise, this is a StmtQuery coming directly from (fabricated by)
    // the Executor.

    StmtNode *stmt = NULL;
    if (parseTree->getOperatorType() == STM_PROCEDURE)
      {
        ExprNode *expr = ((StmtProcedure *)parseTree)->bodyStatement;
        CMPASSERT(expr);
        if (expr->getOperatorType() == STM_DECL_STATCURS)
          {
            StmtDeclStatCurs *cursor = (StmtDeclStatCurs *)expr;
            CMPASSERT(cursor);

            // The cursorSpec field of cursor points to a query expression.
            CMPASSERT(cursor->cursorSpec);
            queryExpr = cursor->cursorSpec;
          }
        else
          stmt = expr->castToStatementExpr();
      }
    else if (parseTree->getOperatorType() == STM_QUERY)
      stmt = (StmtQuery *)parseTree;

    if (!queryExpr) {
      // The Statement that we process here must be a query expression.
      CMPASSERT(stmt);
      CMPASSERT(stmt->isAQueryStatement());
      queryExpr = stmt->getQueryExpression();
    }

    // A query expression MUST have a RelRoot at its root.
    CMPASSERT(queryExpr->getOperatorType() == REL_ROOT);

    CmpCommon::context()->SetMode(STMT_STATIC);

    rc = sqlcomp(sqlText, input.charSet(), queryExpr, gen_code,
                 gen_code_len, heap, phase,
                 NULL, op, useQueryCache,
		 &cacheable, &begTime);

    CmpCommon::context()->SetMode(mode);
    if (rc != SUCCESS && x==0 && cacheable) {
      // retry any error (once) if query was cacheable
      useQueryCache = NOCACHE; // retry with caching off
      // toss pre-retry errors/warnings
      CmpCommon::diags()->rewind(here, TRUE/*update maxDiagsId_*/);
      CURRENTQCACHE->incNOfRetries(); // count retries
      if (queryExpr) {
        delete queryExpr;
        queryExpr = NULL;
      }
    }
    else {
      if (rc == SUCCESS && x==1) {
        // the retry worked; issue a warning about the successful retry.
        // *CmpCommon::diags() << DgSqlCode(2108);
        SQLMXLoggingArea::logCompNQCretryEvent(input.text());
      }
      // "parseTree" is eventually freed by PARSERHEAP's destructor.
      // "parseTree" is aliased to global live variable "TheParseTree".
      // coverity[leaked_storage]
      return rc; // we're done
    }
  }
  // "parseTree" is eventually freed by PARSERHEAP's destructor.
  // "parseTree" is aliased to global live variable "TheParseTree".
  // coverity[leaked_storage]
  return rc;
}



// default constructor
CmpMain::CmpMain() : attrs()
{
  cmpISPInterface.InitISPFuncs();

  // make sure we have a query cache
  CURRENTQCACHE->resizeCache
    (getDefaultInK(QUERY_CACHE),
     getDefaultAsLong(QUERY_CACHE_MAX_VICTIMS),
     getDefaultInK(QUERY_CACHE_AVERAGE_PLAN_SIZE));

  // Ensure PCEC is sized according to latest CQD value
  Int32 newsiz = getDefaultAsLong(PCODE_EXPR_CACHE_SIZE);
  if (newsiz >= 0) // Note: If negative, just leave cache alone
  {
     if ( newsiz != CURROPTPCODECACHE->getMaxSize() )
     {
        CURROPTPCODECACHE->resizeCache( newsiz );
        CURROPTPCODECACHE->clearStats(); // Do this after resizeCache(...)
     }
  }
}

void CmpMain::setInputArrayMaxsize(const ULng32 maxsize)
{
  attrs.addStmtAttribute(SQL_ATTR_INPUT_ARRAY_MAXSIZE, maxsize);
}

ULng32 CmpMain::getInputArrayMaxsize() const
{
  for (Int32 x = 0; x < attrs.nEntries; x++) {
    if (attrs.attrs[x].attr == SQL_ATTR_INPUT_ARRAY_MAXSIZE) {
      return attrs.attrs[x].value;
    }
  }
  return 0; // stmt attribute not found
}

void CmpMain::setRowsetAtomicity(const short atomicity)
{
  attrs.addStmtAttribute(SQL_ATTR_ROWSET_ATOMICITY, atomicity);
}

void CmpMain:: getAndProcessAnySiKeys(TimeVal begTime)
{
   Int32   returnedNumSiKeys = 0;
   TimeVal maxTimestamp = begTime;
 
   Int32 arraySize = 
#ifdef _DEBUG
      1      // we want to execute & test the resize code in DEBUG
#else
      32     // we want release code to resize infrequently
#endif
      ;
    
   Int32 sqlcode = -1;
   while (sqlcode < 0)
   {
     SQL_QIKEY sikKeyArray[arraySize];

     sqlcode = getAnySiKeys( begTime,
                     cmpCurrentContext->getPrev_QI_time(),
                      &returnedNumSiKeys,
                      &maxTimestamp,
                      sikKeyArray, 
                      arraySize );
     if (sqlcode == -8929)
     {
       arraySize *= 2;
     }
     else if (sqlcode < 0)
     {
       // No other error is expected. The CLI call 
       // simply accesses memory. If the implementation
       // changes, then the abort below should remind us
       // to change the error handling.
       abort();
     }
     else if ( returnedNumSiKeys > 0 )
     {
       bool resetRoleList = false;
       bool updateCaches = false;
       qiInvalidationType(returnedNumSiKeys, sikKeyArray, 
                          ComUser::getCurrentUser(), 
                          resetRoleList, updateCaches); 

       if (updateCaches)
       {
         CURRENTQCACHE->free_entries_with_QI_keys(returnedNumSiKeys, 
           sikKeyArray);
         InvalidateNATableCacheEntries(returnedNumSiKeys, sikKeyArray);
         InvalidateNARoutineCacheEntries(returnedNumSiKeys, sikKeyArray);
         InvalidateHistogramCacheEntries(returnedNumSiKeys, sikKeyArray);
       }
      
       // Ignore errors from ResetRoleList.  A check is made later that returns
       // an error if unable to reset the list at this time
       if (resetRoleList)
         SQL_EXEC_ResetRoleList_Internal();
     }
   }
  // Always update previous QI time 
  cmpCurrentContext->setPrev_QI_time( maxTimestamp ); 
}

Int32 CmpMain::getAnySiKeys(TimeVal      begTime,
                            TimeVal      prev_QI_inval_time,
                            Int32 *      retNumSiKeys,
                            TimeVal *    pMaxTimestamp,
                            SQL_QIKEY *  qiKeyArray,
                            Int32        qiKeyArraySize )
{
  Int32   sqlcode = 0;
  Int64 prev_QI_Time = prev_QI_inval_time.tv_usec; // Start with number of microseconds
  Int64 prev_QI_Sec = prev_QI_inval_time.tv_sec;   // put seconds into an Int64
  prev_QI_Time += prev_QI_Sec * (Int64)1000000 ;   // Multiply by 100000 & add to tv_usec

  // SQL_EXEC_GetSecInvalidKeys() is coded to work in JulianTimeStamp values
  // so add number of microseconds between 4713 B.C., Jan 1 and the Epoc (Jan 1, 1970).
  prev_QI_Time += HS_EPOCH_TIMESTAMP ;
  Int64 Max_QI_Time = prev_QI_Time ;

  sqlcode = SQL_EXEC_GetSecInvalidKeys( prev_QI_Time,
                                     qiKeyArray ,
                                     qiKeyArraySize ,
                                     retNumSiKeys ,
                                     &Max_QI_Time );
  Max_QI_Time -= HS_EPOCH_TIMESTAMP ; // Convert back to "Time since the Epoc"
  prev_QI_Time -= HS_EPOCH_TIMESTAMP ; // Convert back to "Time since the Epoc"
 
  if (sqlcode == 0)
  {
     if ( prev_QI_Time != Max_QI_Time )
     {
       // Most-recent-key time has been updated by CLI, so pass it back
       // to the caller.
       pMaxTimestamp->tv_sec  = (Lng32)(Max_QI_Time / 1000000);
       pMaxTimestamp->tv_usec = (Lng32)(Max_QI_Time % 1000000);
     }
     else
     {
       // No new key. Update caller using the time we began this latest
       // SQL statement compilation.
       *pMaxTimestamp = begTime;
     }

     // Some debugging code follows.

#ifndef NDEBUG
    if ( *retNumSiKeys == 0 )
    {
        // Look for Debug Mechanism
      if ( prev_QI_Priv_Value == 0 ) // If previously set to "off"
      {
        NAString qiPath = "";
        Lng32 new_QI_Priv_Value = getDefaultAsLong(QI_PRIV);
        if ( new_QI_Priv_Value != prev_QI_Priv_Value )
        {
           // NOTE: This debug mechanism assumes:
           // (a) There will be an NATable entry for QI_PATH [If there is not,
           //     the debug mechanism just doesn't work.]
           // (b) We need to deal ONLY with pathnames that can be CQD values.
           //     So, no UTF8 names, etc., but that's probably OK for a debug
           //     mechanism.
           //
           CmpCommon::getDefault(QI_PATH, qiPath, FALSE);
           if ( qiPath.length() <= 0 )
              new_QI_Priv_Value = prev_QI_Priv_Value; //Ignore new QI_PRIV value !!
        }
        if ( new_QI_Priv_Value != prev_QI_Priv_Value )
        {
           char sikOpLit[4];

           //
           // The SQL_EXEC_GetSecInvalidKeys() call above did NOT return any SQL_QIKEYs.
           // So, we can choose to return some "hand-built" SQL_QIKEYs.
           // We choose to do so if the user has set up the CQDs QI_PATH and set QI_PRIV > 0.
           // The idea of this debug mechanism is to hand-build the SQL_QIKEYs
           // requested by the user's value of QI_PRIV and for the table/view specified
           // by QI_PATH.   See the definition of the enum ComQIActionType for what values
           // of QI_PRIV correspond to the various privileges.  Note: If the user specifies
           // a QI_PRIV value of 255, then we hand-build several SQL_QIKEYs ... one for
           // each of several privileges.
           //
           prev_QI_Priv_Value = new_QI_Priv_Value ;
           //ComUserID thisUserID = ((NAUserInfo)CatProcess.getSessionUserId());
           Int32 thisUserID = ComUser::getCurrentUser();

           QualifiedName thisQN ( qiPath, 3);
           ExtendedQualName thisEQN ( thisQN );
           NATable *tab = ActiveSchemaDB()->getNATableDB()->get(&thisEQN, NULL, TRUE);
           Int32  realObjHashVal = 0;
           //ComUID objectUID = -1;
           int64_t objectUID = -1;
           if ( tab != NULL )
              objectUID = tab->objectUid().get_value();
           if (objectUID == 0)  // Weird, but sometimes true for VIEW objects
           {
              if ( tab->getSecKeySet().entries() > 0 )
                 realObjHashVal = tab->getSecKeySet()[0].getObjectHashValue();
           }

           Int32 SiKeyDebugEntries = 1;
           if ( new_QI_Priv_Value == 255 )
              SiKeyDebugEntries = NBR_DML_PRIVS;
           SQL_QIKEY debugQiKeys[SiKeyDebugEntries];

           if ( SiKeyDebugEntries == 1 )
           {
              // NOTE: For now, this Debug/Regression test Mechanism supports specifying ONLY values
              // for QI_PRIV corresponding to COM_QI_OBJECT_* enum values (within enum ComQIActionType)

              ComSecurityKey  secKey(thisUserID, objectUID,
                                     SELECT_PRIV,  // Just a dummy value
                                     ComSecurityKey::OBJECT_IS_OBJECT);
              debugQiKeys[0].revokeKey.subject = (Int32) secKey.getSubjectHashValue();
              debugQiKeys[0].revokeKey.object = (Int32) secKey.getObjectHashValue();

              ComQIActionTypeEnumToLiteral( (ComQIActionType) new_QI_Priv_Value,
                                            sikOpLit  );
              debugQiKeys[0].operation[0] = sikOpLit[0];
              debugQiKeys[0].operation[1] = sikOpLit[1];

              if ( objectUID == 0 )
                debugQiKeys[0].revokeKey.object = realObjHashVal;
           }
           else
           {
             for (int_32 i = FIRST_DML_PRIV; i <= LAST_DML_PRIV; i++)
             {
               ComSecurityKey  secKey0(thisUserID, objectUID, (PrivType)i,
                                       ComSecurityKey::OBJECT_IS_OBJECT);
               debugQiKeys[i].revokeKey.subject = (Int32) secKey0.getSubjectHashValue();
               debugQiKeys[i].revokeKey.object = (Int32) secKey0.getObjectHashValue();

               ComQIActionTypeEnumToLiteral(
                             secKey0.getSecurityKeyType(), sikOpLit  );
               debugQiKeys[i].operation[0] = sikOpLit[0];
               debugQiKeys[i].operation[1] = sikOpLit[1];
               if ( objectUID == 0 )
                 debugQiKeys[i].revokeKey.object = realObjHashVal;
             }
           }
           *retNumSiKeys = SiKeyDebugEntries;
           *pMaxTimestamp = begTime;
        }
      }
      else
      {
        prev_QI_Priv_Value = 
          getDefaultAsLong(QI_PRIV); // Set prev value to latest value
        // Note: We will keep this up until user sets value to 0, then
        // the next time we will go through the code in the 'if' block above.
      }
    }
#endif
  }
  return sqlcode;
} 

void CmpMain::InvalidateNATableCacheEntries(Int32 returnedNumQiKeys,
                                            SQL_QIKEY * qiKeyArray)
{
   ActiveSchemaDB()->getNATableDB()->free_entries_with_QI_key(
     returnedNumQiKeys, qiKeyArray);
   return ;
}

void CmpMain::InvalidateNARoutineCacheEntries(Int32 returnedNumQiKeys,
                                              SQL_QIKEY * qiKeyArray)
{
   ActiveSchemaDB()->getNARoutineDB()->free_entries_with_QI_key(
     returnedNumQiKeys, qiKeyArray);
   return ;
}

void CmpMain::InvalidateHistogramCacheEntries(Int32 returnedNumQiKeys,
                                              SQL_QIKEY * qiKeyArray)
{
   if (CURRCONTEXT_HISTCACHE)
     CURRCONTEXT_HISTCACHE->freeInvalidEntries(returnedNumQiKeys,qiKeyArray);
   return ;
}

void CmpMain::RemoveMarkedNATableCacheEntries()
{
   ActiveSchemaDB()->getNATableDB()->remove_entries_marked_for_removal();
   return ;
}

void CmpMain::UnmarkMarkedNATableCacheEntries()
{
   ActiveSchemaDB()->getNATableDB()->unmark_entries_marked_for_removal();
   return ;
}

RelExpr::AtomicityType CmpMain::getRowsetAtomicity() const
{
  for (Int32 x = 0; x < attrs.nEntries; x++) {
    if (attrs.attrs[x].attr == SQL_ATTR_ROWSET_ATOMICITY) {
      return (RelExpr::AtomicityType) attrs.attrs[x].value;
    }
  }
  return RelExpr::UNSPECIFIED_; // stmt attribute not found
}

void CmpMain::setHoldableAttr(const short holdable)
{
  attrs.addStmtAttribute(SQL_ATTR_CURSOR_HOLDABLE, holdable);
}

SQLATTRHOLDABLE_INTERNAL_TYPE CmpMain::getHoldableAttr()
{
  for (Int32 x = 0; x < attrs.nEntries; x++) {
    if (attrs.attrs[x].attr == SQL_ATTR_CURSOR_HOLDABLE) {
      return (SQLATTRHOLDABLE_INTERNAL_TYPE) attrs.attrs[x].value;
    }
  }
  return (SQLATTRHOLDABLE_INTERNAL_TYPE)SQLCLIDEV_NONHOLDABLE; // stmt attribute not found
}

// try to compile query via a cache hit
NABoolean CmpMain::compileFromCache
  (const char* sText,  // (IN) : sql statement text
   Lng32     charset,   // (IN) : character set of  sql statement text
   RelExpr* queryExpr, // (IN) : the query to be compiled
   BindWA&  bindWA,    // (IN) : work area (used by backpatchParams)
   CacheWA& cachewa,   // (IN) : work area for normalizeForCache
   char**   plan,      // (OUT): compiled plan or NULL
   ULng32 *pLen,// (OUT): length of compiled plan or 0
   NAHeap*  heap,     // (IN) : heap to use for compiled plan
   IpcMessageObjType op,//(IN): SQLTEXT_{STATIC_}COMPILE or
                        //      SQLTEXT_{STATIC_}RECOMPILE
   NABoolean& bPatchOK, //(OUT): true iff backpatch succeeded
   TimeVal&   begTime)  //(IN):  start time for this compile
{
  NABoolean retcode = FALSE;
  CData * cachedData = NULL;
  bPatchOK = FALSE; // until proven otherwise
  NABoolean textCached = FALSE;

  if (!CURRENTQCACHE->isCachingOn()) 
    return FALSE;

  CmpPhase phase = cachewa.getPhase();
    
    // do not do pre-binding cache if public schema is used
    NAString pubSchema = "";
    CmpCommon::getDefault(PUBLIC_SCHEMA_NAME, pubSchema, FALSE);
    if ( (pubSchema != "") && (cachewa.getPhase() < CmpMain::BIND) )
      return FALSE;

    if (phase == CmpMain::PREPARSE) {
      if (!CURRENTQCACHE->isPreparserCachingOn()) {
        return FALSE;
      }
      // lookup this query's sql statement text+env in preparser cache
      TextKey *tkey = cachewa.getTextKey(sText, charset, getStmtAttributes());
      if (tkey) {
        TextData *tdata;
        if ((op == CmpMessageObj::SQLTEXT_RECOMPILE)
            || (op == CmpMessageObj::SQLTEXT_STATIC_RECOMPILE)) {
          // fix for genesis case 10-090514-3953 soln 10-090514-1580:
          // Do NOT try to actively decache. Otherwise, SPJ trigger 
          // tests may leave saveabend files and QA open more cases.
          // Leave the outdated entries alone. They will eventually 
          // age out of the cache.
        }
        // look up query in cache
        else if (CURRENTQCACHE->lookUp(tkey, tdata)) {
          // backpatch parameters in plan
          bPatchOK = tdata->backpatchParams();
          if (bPatchOK) {

	    // if the previously generated plan did not have auto qry retry
	    // enabled, then we cannot use text cached data for this query.
	    // AQR is needed to correctly execute text cached plans at runtime.
	    if (tdata && tdata->getPlan() && 
		tdata->getPlan()->getPlan() && 
		(tdata->getPlan()->getPlanLen() > 0))
	      {
		ComTdbRoot * rootTdb = (ComTdbRoot *)tdata->getPlan()->getPlan();
		if ((NOT rootTdb->aqrEnabled())||(rootTdb->hiveAccess()))
		  {
		    bPatchOK = FALSE;
		    return FALSE;
		  }
	      }

            tdata->allocNcopyPlan(heap, plan, pLen);
            tdata->addHitTime(begTime);
          }
	  textCached = TRUE;
	  cachedData = tdata;
	  retcode = TRUE; // a successful cache hit
          //return TRUE; // a successful cache hit
        }
      }
    } else { // after parser
      // is this query cacheable after parsing or binding?
      cachewa.resetNofExprs();
      //cachewa.resetCacheable();

      CacheKey *ckey = NULL;
      CacheData *cdata;
      CacheEntry *entry;

      NABoolean hqcHit = FALSE;

      ostream* hqc_ostream=CURRENTQCACHE->getHQCLogFile();
      HQCParseKey* hkey = NULL;

      // Post Parser Caching: HQC first so that we can avoid expensive
      // call to normalizeForCache() if there is a HQC hit.
      if(CmpCommon::getDefault(HYBRID_QUERY_CACHE) == DF_ON &&
         phase == PARSE)
      {
        hkey = SqlParser_CurrentParser->getHQCKey();
        if(hkey && hkey->isCacheable()){
          hkey->getNormalizedQueryString();
          cachewa.setHQCKey(hkey);
          hqcHit = CURRENTQCACHE->HQCLookUp(hkey, ckey);

          if ( hqc_ostream ) {
             if (hqcHit) {
                *hqc_ostream  << "\nFound in HQC:" << endl ;
                *hqc_ostream  << "SQL query=" << sText << endl
                              << "HQC key=" << hkey->getKey() << endl;
             }
          }
        }
      }

      // Post Parser Cache: simple insert 
      if (!hqcHit && queryExpr && queryExpr->isCacheableExpr(cachewa)) {
        cachewa.setCacheable();
        // parameterize query
        queryExpr = queryExpr->normalizeForCache(cachewa, bindWA);
        if (!queryExpr) { // should never happen, but if it does
          return TRUE; // to tell caller to retry with caching off
        }
        // mark root as mvqr noncacheable if it has:
        // 1) rewriteable MV, and 
        // 2) has parameterized equality predicate
        if (cachewa.hasParameterizedPred()) {
          ((RelRoot*)queryExpr)->setMVQRqueryNonCacheable();
        }
        if (op != CmpMessageObj::SQLTEXT_RECOMPILE &&
            op != CmpMessageObj::SQLTEXT_STATIC_RECOMPILE)
        {
            // get its key into cachewa
            cachewa.generateCacheKey(queryExpr, sText, charset, 
                                 bindWA.getViewsUsed());

            // fix for genesis case 10-090514-3953 soln 10-090514-1580:
            // Do NOT try to actively decache. Otherwise, SPJ trigger 
            // tests will leave saveabend files and QA open more cases.
            // Leave the outdated entries alone. They will eventually 
            // age out of the cache.
            ckey = cachewa.getCacheKey(getStmtAttributes());
        }
     } 

     // look up query in domain cache
     if (ckey && CURRENTQCACHE->lookUp(ckey, cdata, entry, phase)) {
            // backpatch parameters in plan
            char *params; ULng32 parmSize;
            if (hqcHit) // HQC case
               bPatchOK = cdata->backpatchParams
                  (hkey->getParams().getConstantList(),
                   hkey->getParams().getDynParamList(),
                   bindWA, params, parmSize);
            else // non-HQC case (inserts)
               bPatchOK = cdata->backpatchParams
                 (cachewa.getConstParams(), cachewa.getSelParams(),
                  cachewa.getConstParamPositionsInSql(),
                  cachewa.getSelParamPositionsInSql(),
                  bindWA, params, parmSize);
            if (bPatchOK) {

              if ( hqcHit && hqc_ostream ) {
               *hqc_ostream << "\nHQC backpatch OK:" << endl 
                            << "SQL query=" << sText << endl
                            << "HQC key=" << hkey->getKey() << endl;
              }
              else if(!hqcHit && hkey == NULL && hqc_ostream)
              {//usually, a query in SQC, it should have an entry in HQC, let's see why it is not
               *hqc_ostream << "\nNot in HQC but in SQC:" << endl 
                            << "SQL query=" << sText << endl;
              }

              // if the previously generated plan did not have auto qry retry
	      // enabled, then we cannot use text cached data for this query.
	      // AQR is needed to correctly execute pre-bind cached plans at runtime.
	      if (cdata && cdata->getPlan() && 
		  cdata->getPlan()->getPlan() && 
		  (cdata->getPlan()->getPlanLen() > 0))
	      {
		  ComTdbRoot * rootTdb = (ComTdbRoot *)cdata->getPlan()->getPlan();
		  if ((NOT rootTdb->aqrEnabled())||(rootTdb->hiveAccess()))
		  {
		    if(hqcHit)
		        CURRENTQCACHE->getHQC()->setPlanNoAQROrHiveAccess(TRUE);
		    bPatchOK = FALSE;
		    return FALSE;
		  }
	      }
              cdata->allocNcopyPlan(heap, plan, pLen);
              cdata->addHitTime(begTime);
              // we have a preparser cache miss and a postparser cache hit.
              // make room for and add this query's preparser cache entry.
              if (CURRENTQCACHE->isPreparserCachingOn()) {
		ComTdbRoot * rootTdb = (ComTdbRoot *)*plan;
		if ((rootTdb->aqrEnabled())&&(!(rootTdb->hiveAccess()))) {
                CURRENTQCACHE->addPreParserEntry
                  (cachewa.getTextKey(sText, charset, getStmtAttributes()),
                   params, parmSize, entry, begTime);
              }
            }
            }
       else 
           return FALSE; //back patch failed
	    cachedData = cdata;
	    retcode = TRUE; // a successful cache hit
      }
    }
  
  if ((retcode) &&
      (textCached) &&
      (bPatchOK) &&
      (*plan != NULL) &&
      (cachedData != NULL))
    {
      // a successful cache hit
      // Put query id and other cache related info in generated plan.
      ComTdbRoot * rootTdb = (ComTdbRoot *)*plan;
      if (rootTdb->qCacheInfoIsClass())
	{
	  NABasicPtr qCacheInfoPtr = rootTdb->getQCInfoPtr();
	  (void)qCacheInfoPtr.unpack(rootTdb);
	  QCacheInfo * qcInfo = (QCacheInfo *)qCacheInfoPtr.getPointer();
	  qcInfo->setPlanId(cachedData->getPlan()->getId());
	  qcInfo->setCacheWasHit(TRUE);
	}
    }
  else
    {
      // no cache or not cacheable or a cache miss or a recompile
    }

  return retcode;
}

// -----------------------------------------------------------------------
// This sqlcomp procedure compiles an RelExpr* into code from generator.
// The steps are binder, transform, normalize, optimize, generator
//
// ...Unless phase is passed in, in which case the tree for that compiler phase
// is returned in the gen_code pointer (I know, wrong name), which the caller
// needs to cast back to RelExpr*.
// -----------------------------------------------------------------------

class CmpExceptionEnvWatcher{
public:
  CmpExceptionEnvWatcher() {
    CmpExceptionEnv::registerCallBack();
  }

  ~CmpExceptionEnvWatcher() {
    CmpExceptionEnv::unRegisterCallBack();
  }
};



CmpMain::ReturnStatus CmpMain::sqlcomp(const char *input_str,           //IN
                                       Lng32 charset,                    //IN
                                       RelExpr *&queryExpr,             //INOUT
                                       char **gen_code,                 //OUT
                                       ULng32 *gen_code_len,     //OUT
                                       CollHeap *heap,                  //IN
                                       CompilerPhase phase,             //IN
				       FragmentDir **fragmentDir,       //OUT
                                       IpcMessageObjType op,            //IN
                                       QueryCachingOption useQueryCache, //IN
                                       NABoolean* cacheable,            //OUT
                                       TimeVal* begTime,                //IN
                                       NABoolean shouldLog)             //IN
{                               // 
  CmpExceptionEnvWatcher guard;

  ComDiagsArea & d = *CmpCommon::diags();
  try{

#ifdef NA_DEBUG_GUI
    CURRENTSTMT->setDisplayGraph(((RelRoot *)queryExpr)->getDisplayTree());
#endif // NA_DEBUG_GUI

    CmpMain::ReturnStatus  ok =
          compile(input_str, charset,
		   queryExpr,
		   gen_code,
		   gen_code_len,
		   heap,
		   phase,
		   fragmentDir,
		   op,
		   useQueryCache,
		   cacheable,
                   begTime,
                   shouldLog);
                   
    return ok;
  }
  catch(UserException & ){
    // This is a user error detected & thrown by GenExit() callers like
    // like Generator::verifyUpdatableTransMode(), ItmBalance::preCodeGen(),
    // and ExpGenerator::addDevaultValues(). All these callers have already
    // deposited their error messages into the diags area. So, do nothing
    // here. Our caller should check the diags area & dump its contents.
  }
  catch(FatalException & e){
    // MX event should be added by the thrower,
    // because the thrower has more information about the
    // occurrance of the unrecoverable error and can
    // choose among different MX events.

    // Temporarily disable this feature when there is already
    // an error in the diagnostics area.
    Lng32 numErrors = d.getNumber(DgSqlCode::ERROR_);
    if(numErrors == 0){
      d << DgSqlCode(arkcmpErrorFatal)
	<< DgString0(e.getMsg())
	<< DgString1(e.getFileName())
	<< DgInt0((Lng32)e.getLineNum());
    if(e.getStackTrace())
     SQLMXLoggingArea::logSQLMXAssertionFailureEvent(e.getFileName(),
                                                     e.getLineNum(),
                                                     "Compiler Fatal Exception",
                                                     e.getMsg(),
                                                     NULL,
                                                     e.getStackTrace());
    }
  }
  catch(AssertException & e){
    d << DgSqlCode(arkcmpErrorAssert)
      << DgString0(e.getCondition())
      << DgString1(e.getFileName())
      << DgInt0((Lng32)e.getLineNum());
    if(e.getStackTrace())
      SQLMXLoggingArea::logSQLMXAssertionFailureEvent(e.getFileName(),
                                                      e.getLineNum(),
                                                      "Compiler Internal Assert Failure",
                                                      e.getCondition(),
                                                      NULL,
                                                      e.getStackTrace());
      
  }
  catch(BaseException & e){
    // Probably not reach here, just a safe net
    d << DgSqlCode(arkcmpErrorFatal)
      << DgString0("An unknown error")
      << DgString1(e.getFileName())
      << DgInt0((Lng32)e.getLineNum());
  }
  sqlcompCleanup(input_str, queryExpr, TRUE);

  return EXCEPTIONERROR;
}
// -----------------------------------------------------------------------
// fixupCompilationStats 
//
// Helper function used to put CompilationStats which could not be determined
// prior to generator phase back into the root tdb.  This requires 
// converting the CompilationStatsDataPtr in the root tdb from an offset
// back into a pointer.  
//
// -----------------------------------------------------------------------
static void
fixupCompilationStats(ComTdbRoot *rootTdb,
                      Space *rootSpace)
{
  if( rootTdb && rootSpace )
  {
    CompilationStatsDataPtr cmpStatsPtr = 
      rootTdb->getCompilationStatsDataPtr();
    //
    // getting the offset.
    Int64 offset = cmpStatsPtr.getOffset();

#ifndef NA_LITTLE_ENDIAN
      swapInt64((char *)(&offset));
#endif
    //
    // converting the offset to a pointer.
    CompilationStatsData *cmpStatsData = 
      (CompilationStatsData*)rootSpace->convertToPtr(offset);
              
    CMPASSERT(NULL != cmpStatsData);
    //
    // fixing up the compilation stats data
    CompilationStats* stats = CURRENTSTMT->getCompilationStats();
    cmpStatsData->setCmpCpuGenerator(
      stats->cmpPhaseLength(CompilationStats::CMP_PHASE_GENERATOR));
  
    cmpStatsData->setCmpCpuTotal(
      stats->cmpPhaseLength(CompilationStats::CMP_PHASE_ALL)); 

    cmpStatsData->setCompileEndTime(stats->compileEndTime());
  }
}

void CmpMain::setSqlParserFlags(ULng32 f)
{
  // set special flags
  if (CmpCommon::getDefault(MODE_SPECIAL_4) == DF_ON)
    {
      f |= IN_MODE_SPECIAL_4;

      Set_SqlParser_Flags(IN_MODE_SPECIAL_4);
    }

  if (f)
    {
      attrs.addStmtAttribute(SQL_ATTR_SQLPARSERFLAGS, f);
    }

}

CmpMain::ReturnStatus CmpMain::compile(const char *input_str,           //IN
                                       Lng32 charset,                    //IN
                                       RelExpr *&queryExpr,             //INOUT
                                       char **gen_code,                 //OUT
                                       ULng32 *gen_code_len,     //OUT
                                       CollHeap *heap,                  //IN
                                       CompilerPhase phase,             //IN
				       FragmentDir **fragmentDir,       //OUT
                                       IpcMessageObjType op,            //IN
                                       QueryCachingOption useQueryCache, //IN
                                       NABoolean* cacheable,            //OUT
                                       TimeVal* begTime,                //IN
                                       NABoolean shouldLog)             //IN
{
  // initialize the return values
  ReturnStatus retval = SUCCESS;
  *gen_code = 0;
  *gen_code_len = 0;
  
  QueryAnalysis* queryAnalysis = QueryAnalysis::Instance();  
  CompilationStats* stats = CURRENTSTMT->getCompilationStats();
  stats->takeSnapshotOfCounters();
  //
  // if haven't entered compilation phase all yet, start now
  if( 0 == stats->compileStartTime() )
  {
    stats->enterCmpPhase(CompilationStats::CMP_PHASE_ALL);
  }

  if( op == CmpMessageObj::SQLTEXT_RECOMPILE ||
      op == CmpMessageObj::SQLTEXT_STATIC_RECOMPILE)
  {
    stats->setIsRecompile();
  }

  InitSchemaDB();

  ActiveSchemaDB()->createStmtTables();
  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context());


  if (useQueryCache != NOCACHE && CmpCommon::getDefault(NSK_DBG) == DF_ON) {
    useQueryCache = NOCACHE;
  }

  // Every compile goes through here so we are guranteed that the
  //doNotAbort gloabal flag is reset appropriately for each statement.
  CmpCommon::context()->setDoNotAbort(useQueryCache != NOCACHE);

  //if this is a secondary MXCMP set NATable Caching OFF
  if(CmpCommon::context()->isSecondaryMxcmp())
  {
    CmpCommon::context()->schemaDB_->getNATableDB()->setCachingOFF();
  }
  else{
    //if this the primary MXCMP and the metadata caching
    //default is ON then turn on metadata caching. Of
    //course it might already be ON.
    if((getDefaultAsLong(METADATA_CACHE_SIZE)>0)&&
       (!OSIM_runningInCaptureMode()))
    {
      CmpCommon::context()->schemaDB_->getNATableDB()->setCachingON();
    }
    else{
      CmpCommon::context()->schemaDB_->getNATableDB()->setCachingOFF();
    }
  }

  // retrieve OVERRIDE_SCHEMA settings only when necessary
  // check if this is from EXPLAIN
  NABoolean inExplain = FALSE ;
  const NAString * val =
    ActiveControlDB()->getControlSessionValue("EXPLAIN");
  if ( (val) && (*val == "ON") )
    inExplain = TRUE ;
  if (( // not internal queries issued from executor, like insert query in CTAS
       ( !Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL) )
       // not from a second mxcmp, like utility or ustat
    && ( !CmpCommon::context()->isSecondaryMxcmp() ) ) 
       // do override if this is from EXPLAIN
    || inExplain) 
     bindWA.initializeOverrideSchema();

  CacheWA cachewa(CmpCommon::statementHeap());


  NABoolean bPatchOK=FALSE;
  if (useQueryCache == NORMAL && phase == END) {
    cachewa.setPhase(PARSE);
    TaskMonitor hqcCmpBackPatch;
    hqcCmpBackPatch.enter();
    if (compileFromCache(input_str, charset, queryExpr, bindWA, cachewa,
                         gen_code, gen_code_len, (NAHeap*)heap, op,
                         bPatchOK, *begTime)) {
      if (cacheable) 
        *cacheable = cachewa.isCacheable();
      hqcCmpBackPatch.exit();
      //Added, this is crucial for authorization to work right.
      sqlcompCleanup(NULL/*i.e. success*/, queryExpr, TRUE);
      EXITMAIN_TASK_MONITOR( queryAnalysis->compilerMonitor() );  

      NABoolean onlyLogCompilerAllTime = (CmpCommon::getDefault(COMPILE_TIME_MONITOR_LOG_ALLTIME_ONLY) == DF_ON);

      if (CURRSTMT_OPTDEFAULTS->compileTimeMonitor() && !onlyLogCompilerAllTime)
      {
        const char * fname = getCOMPILE_TIME_MONITOR_OUTPUT_FILEname();

        if (fname)
        {
          ofstream fileout(fname, ios::app);

          fileout<<"Query : \n"<<input_str<<endl;
          fileout<<"----------------"<<endl;

          fileout<<"\tCompiler all: "<<queryAnalysis->compilerMonitor()<<endl;
          fileout<<"\tParser      : "<<queryAnalysis->parserMonitor()<<endl;
          fileout<<"\tbackPatch   : "<<hqcCmpBackPatch <<endl;
          fileout<<"--------------------------------------------"<<endl;
        }
      }
      return bPatchOK ? retval : QCACHINGERROR;
    }
  }
  if (cacheable) *cacheable = cachewa.isCacheable();

  TaskMonitor compilerAll;
  TaskMonitor binderTime;
  TaskMonitor transformerTime;
  TaskMonitor normalizerTime;
  TaskMonitor semanticQryOptTime;
  TaskMonitor analyzerTime;
  TaskMonitor optimizerTime;
  TaskMonitor preGenTime;
  TaskMonitor generatorTime;

//  if ( CmpCommon::getDefault(COMP_BOOL_32) == DF_ON )
// now, after testing, unconditionallly use new logic in
// CostScalar operations * and /. Kepp this for awhile until
// IEEE will be fully implemented in Optimizer.


  if (CURRSTMT_OPTDEFAULTS->compileTimeMonitor())
  {
    const char * fname = getCOMPILE_TIME_MONITOR_OUTPUT_FILEname();
    if (fname)
    {
	  ofstream fileout(fname, ios::app);
	  fileout<<"Query : \n"<<input_str<<endl;
	  fileout<<"----------------"<<endl;
    }
    else
    {
	  cout<<"Query : \n"<<input_str<<endl;
	  cout<<"----------------"<<endl;
    }

    compilerAll.enter();
  }


#ifdef _DEBUG
  if (const char* streamName = getenv("DUMP_MEMORY_INFO"))
  {
    delete ComMemoryDiags::DumpMemoryInfo();
    ComMemoryDiags::DumpMemoryInfo() = new /*(CmpCommon::contextHeap())*/
				       fstream(streamName, ios::out);

    DumpProcessMemoryInfo(ComMemoryDiags::DumpMemoryInfo());
    ((NAHeap*) CmpCommon::contextHeap())->dump(
    			  ComMemoryDiags::DumpMemoryInfo(), 0);
  }
#endif

#ifdef NA_DEBUG_GUI


  NABoolean ODBC = (CmpCommon::getDefault(ODBC_PROCESS) == DF_ON);  // is the request coming through odbc
  NABoolean JDBC = (CmpCommon::getDefault(JDBC_PROCESS) == DF_ON);  // is the request coming through jdbc
  //hpdci and whiteboard should not load GUI Debugger, by checking DISPLAY
  if (((RelRoot *)queryExpr)->getDisplayTree() && !ODBC && !JDBC)
    loadSqlCmpDbgDLL_msGui();
  else if(((RelRoot *)queryExpr)->getDisplayTree() && (ODBC||JDBC))
  {//request with display from ODBC or JDBC
    *CmpCommon::diags() << DgSqlCode(DISPLAY_IN_ODBC_JDBC); 
  }
#endif

  // for NSK only
  if ( CmpCommon::getDefault(NSK_DBG) == DF_ON )
  {
    NABoolean queryOnly = 
        ( CmpCommon::getDefault(NSK_DBG_QUERY_LOGGING_ONLY) == DF_ON );

    const char* qPrefix = ActiveSchemaDB()->getDefaults().getValue(NSK_DBG_QUERY_PREFIX);

    NABoolean doPrint = TRUE;

    if ( qPrefix && strcmp(qPrefix, "") != 0 ) {

       const char* p = input_str;
       while (*p == ' ')
         p++;

       if ( strncmp(p, qPrefix, strlen(qPrefix)) != 0 )
         doPrint = FALSE;
    }

    if ( doPrint ) {
      CURRCONTEXT_OPTDEBUG->stream() << endl;

      if ( !queryOnly ) 
        CURRCONTEXT_OPTDEBUG->stream() << "Query to optimize:" << endl;

      CURRCONTEXT_OPTDEBUG->stream() << input_str ;

      if ( queryOnly ) 
        CURRCONTEXT_OPTDEBUG->stream() << ";" << endl;

      CURRCONTEXT_OPTDEBUG->stream() << endl << endl;
    }
  }

#ifdef NA_DEBUG
#ifdef NA_DEBUG_GUI


  initGuiDisplay(queryExpr);
#endif  // NA_DEBUG_GUI
#endif  // NA_DEBUG

  MonitorMemoryUsage_Enter("Binder");
  // bind
  ENTERMAIN_TASK_MONITOR( binderTime );
  stats->enterCmpPhase(CompilationStats::CMP_PHASE_BINDER);
  queryAnalysis->setCompilerPhase(QueryAnalysis::BINDER);
  
  // -------------------------------------------------------------------
  // Rowset transformation
  // -------------------------------------------------------------------
  queryExpr = queryExpr->xformRowsetsInTree(bindWA,
					    getInputArrayMaxsize(),
					    getRowsetAtomicity());
  if (bindWA.errStatus())
    return BINDERERROR;

  CMPASSERT(queryExpr);

  // A query expression MUST have a RelRoot at its root.
  CMPASSERT(queryExpr->getOperatorType() == REL_ROOT);

  bindWA.setHoldableType(getHoldableAttr());

  // RelRoot whose child is not a compoundstmt has its own hostArraysArea.
  // Tell bindWA about it (hostArraysArea is a global variable in disguise).
  if (queryExpr->child(0) &&
      queryExpr->child(0)->getOperatorType() != REL_COMPOUND_STMT)
    bindWA.setHostArraysArea(((RelRoot*)queryExpr)->getHostArraysArea());

  GUI_or_LOG_DISPLAY(
    Sqlcmpdbg::AFTER_PARSING, NSK_DBG_SHOW_TREE_AFTER_PARSING, "parsing");

  // Capture NAClusterInfo, VPROC, Query text etc.
  if (OSIM_runningInCaptureMode())
  {
    OSIM_capturePrologue();
  }

  bindWA.setFailedForPrivileges(FALSE);


  // *************************************************************************
  // *                                                                       *
  // *                       B I N D E R                                     *
  // *                                                                       *
  // *************************************************************************


  queryExpr = queryExpr->bindNode(&bindWA);

  if (!queryExpr || bindWA.errStatus())
  {
      sqlcompCleanup(input_str, queryExpr, TRUE);
      if ( bindWA.failedForPrivileges() )
         return PRIVILEGEERROR;
      return BINDERERROR;
  }

  EXITMAIN_TASK_MONITOR( binderTime );
  stats->exitCmpPhase(CompilationStats::CMP_PHASE_BINDER);
  MonitorMemoryUsage_Exit("Binder");

  GUI_or_LOG_DISPLAY(
    Sqlcmpdbg::AFTER_BINDING, NSK_DBG_SHOW_TREE_AFTER_BINDING, "binding");

  if (phase == BIND)
    {
      *gen_code = (char *)queryExpr;
      return SUCCESS;
    }

#ifdef NA_DEBUG_GUI
  if (useQueryCache == NORMAL && ((RelRoot *)queryExpr)->getDisplayTree()
      && CmpMain::pExpFuncs_
      ) {
  }
#endif

  if (useQueryCache != NOCACHE && CmpCommon::getDefault(NSK_DBG) == DF_ON) {
    useQueryCache = NOCACHE;
  }

  // -------------------------------------------------------------------
  // try to compile query via (after BIND stage) cache hit
  // -------------------------------------------------------------------
  if (!CURRENTQCACHE->getHQC()->isPlanNoAQROrHiveAccess()
      && useQueryCache != NOCACHE && phase == END) {
    cachewa.setPhase(BIND);
    TaskMonitor hqcCmpBackPatch;
    hqcCmpBackPatch.enter();
    if (compileFromCache(input_str, charset, queryExpr, bindWA, cachewa,
                         gen_code, gen_code_len, (NAHeap*)heap, op,
                         bPatchOK, *begTime)) {
      if (cacheable) *cacheable = cachewa.isCacheable();
      if (!bPatchOK) {
        sqlcompCleanup(input_str, queryExpr, TRUE);
        retval = QCACHINGERROR;
      }
      else {
         hqcCmpBackPatch.exit();
         sqlcompCleanup(NULL/*i.e. success*/, queryExpr, TRUE);
         EXITMAIN_TASK_MONITOR( queryAnalysis->compilerMonitor() );

         NABoolean onlyLogCompilerAllTime = (CmpCommon::getDefault(COMPILE_TIME_MONITOR_LOG_ALLTIME_ONLY) == DF_ON);

         if (CURRSTMT_OPTDEFAULTS->compileTimeMonitor() && !onlyLogCompilerAllTime)
         {
           const char * fname = getCOMPILE_TIME_MONITOR_OUTPUT_FILEname();

           if (fname)
           {
             ofstream fileout(fname, ios::app);

             fileout<<"\tCompiler all: "<<queryAnalysis->compilerMonitor()<<endl;
             fileout<<"\tParser      : "<<queryAnalysis->parserMonitor()<<endl;
             fileout<<"\tBinder      : "<<binderTime<<endl;
             fileout<<"\tbackPatch   : "<<hqcCmpBackPatch <<endl;
             fileout<<"--------------------------------------------"<<endl;
           }
         }
      }
      return retval;
    }
  }
  if (cacheable) *cacheable = cachewa.isCacheable();

  FILE* tfd=CURRCONTEXT_HISTCACHE->getTraceFileDesc();
  if (tfd)
    fprintf(tfd,"query:%s\n", input_str);

  FILE* mfd=CURRCONTEXT_HISTCACHE->getMonitorFileDesc();
  if (mfd)
    fprintf(mfd,"query:%s\n", input_str);
    
  MonitorMemoryUsage_Enter("Transformer");
  // transform and normalize
  ENTERMAIN_TASK_MONITOR( transformerTime );
  stats->enterCmpPhase(CompilationStats::CMP_PHASE_TRANSFORMER);
  //normalizer in the line below means both normalizer and transformer
  queryAnalysis->setCompilerPhase(QueryAnalysis::NORMALIZER);


  // *************************************************************************
  // *                                                                       *
  // *      N O R M A L I Z E R :  T R A N S F O R M                         *
  // *                                                                       *
  // *************************************************************************


  NormWA normWA(CmpCommon::context());
  queryExpr = transform(normWA, queryExpr);
  if (!queryExpr)
    {
      sqlcompCleanup(input_str, queryExpr, TRUE);
      return TRANSFORMERERROR;
    }

  EXITMAIN_TASK_MONITOR( transformerTime );
  stats->exitCmpPhase(CompilationStats::CMP_PHASE_TRANSFORMER);
  MonitorMemoryUsage_Exit("Transformer");

  GUI_or_LOG_DISPLAY(
    Sqlcmpdbg::AFTER_TRANSFORMATION, NSK_DBG_SHOW_TREE_AFTER_TRANSFORMATION, "transformation");

  MonitorMemoryUsage_Enter("Normalizer");
  ENTERMAIN_TASK_MONITOR( normalizerTime );
  stats->enterCmpPhase(CompilationStats::CMP_PHASE_NORMALIZER);


  // *************************************************************************
  // *                                                                       *
  // *      N O R M A L I Z E R :  N O R M A L I Z E                         *
  // *                                                                       *
  // *************************************************************************


  queryExpr = queryExpr->normalizeNode(normWA);

  if (!queryExpr)
    {
      sqlcompCleanup(input_str, queryExpr, TRUE);
      return NORMALIZERERROR;
    }

  // QSTUFF: check whether a table is both read and updated
  if ( queryExpr->checkReadWriteConflicts(normWA) != RelExpr::RWOKAY )
    {
      sqlcompCleanup(input_str, queryExpr, TRUE);
      return CHECKRWERROR;
    }

  EXITMAIN_TASK_MONITOR( normalizerTime );
  stats->exitCmpPhase(CompilationStats::CMP_PHASE_NORMALIZER);
  MonitorMemoryUsage_Exit("Normalizer");

  GUI_or_LOG_DISPLAY(
    Sqlcmpdbg::AFTER_NORMALIZATION, NSK_DBG_SHOW_TREE_AFTER_NORMALIZATION, "normalization");

  ENTERMAIN_TASK_MONITOR( semanticQryOptTime );
  stats->enterCmpPhase(CompilationStats::CMP_PHASE_SEMANTIC_QUERY_OPTIMIZATION);

  // -------------------------------------------------------------------
  // semanticQueryOptimize
  // -------------------------------------------------------------------
  queryExpr = queryExpr->semanticQueryOptimizeNode(normWA);

  EXITMAIN_TASK_MONITOR( semanticQryOptTime );
  stats->exitCmpPhase(CompilationStats::CMP_PHASE_SEMANTIC_QUERY_OPTIMIZATION);

  GUI_or_LOG_DISPLAY(
    Sqlcmpdbg::AFTER_SEMANTIC_QUERY_OPTIMIZATION, NSK_DBG_SHOW_TREE_AFTER_SEMANTIC_QUERY_OPTIMIZATION, "semanticQueryOptimization");


  MonitorMemoryUsage_Enter("Analyzer");
  // analyze
  ENTERMAIN_TASK_MONITOR( analyzerTime );
  stats->enterCmpPhase(CompilationStats::CMP_PHASE_ANALYZER);
  queryAnalysis->setCompilerPhase(QueryAnalysis::ANALYZER);

  CURRENTSTMT->clearCqsWA();


  // *************************************************************************
  // *                                                                       *
  // *      A N A L Y Z E R                                                  *
  // *                                                                       *
  // *************************************************************************


  queryExpr = queryAnalysis->analyzeThis(queryExpr);

  // fix 10-061128-0744 (SQL statement does not compile first time but
  // compiles sec). Do not continue if there are SQL errors in the diag. area.
  if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) > 0) {
    sqlcompCleanup(input_str, queryExpr, TRUE);
    return OPTIMIZERERROR;
  }

  EXITMAIN_TASK_MONITOR( analyzerTime );
  stats->exitCmpPhase(CompilationStats::CMP_PHASE_ANALYZER);
  MonitorMemoryUsage_Exit("Analyzer");

  GUI_or_LOG_DISPLAY(
    Sqlcmpdbg::AFTER_ANALYZE, NSK_DBG_SHOW_TREE_AFTER_ANALYSIS, "analysis");

  MonitorMemoryUsage_Enter("Optimizer");
  // optimize
  ENTERMAIN_TASK_MONITOR( optimizerTime );
  stats->enterCmpPhase(CompilationStats::CMP_PHASE_OPTIMIZER);
  queryAnalysis->setCompilerPhase(QueryAnalysis::OPTIMIZER);


  // *************************************************************************
  // *                                                                       *
  // *      O P T I M I Z E R                                                *
  // *                                                                       *
  // *************************************************************************


  queryExpr = queryExpr->optimizeNode();

  CURRENTSTMT->clearCqsWA();
  EXITMAIN_TASK_MONITOR( optimizerTime );
  stats->exitCmpPhase(CompilationStats::CMP_PHASE_OPTIMIZER);
  MonitorMemoryUsage_Exit("Optimizer");

  if (!queryExpr)
    {
      sqlcompCleanup(input_str, queryExpr, TRUE);
      return OPTIMIZERERROR;
    }

  if (phase == OPTIMIZE)
    {
      *gen_code = (char *)queryExpr;
      return SUCCESS;
    }

  // The optimized query tree is displayed at the end of each
  // optimization pass, invoked from RelExpr::optimize()

  MonitorMemoryUsage_Enter("PreCodeGen");
  // generate code
  ENTERMAIN_TASK_MONITOR( preGenTime );
  stats->enterCmpPhase(CompilationStats::CMP_PHASE_PRECODE_GENERATOR);
  queryAnalysis->setCompilerPhase(QueryAnalysis::PRECODE_GENERATOR);


  Generator generator(CmpCommon::context());

  // set bindWA in the generator object. This is done to bind/type propagate
  // the expressions 'created' in the generator (for ex. missing keys)
  generator.setBindWA(&bindWA);



  // *************************************************************************
  // *                                                                       *
  // *      P R E C O D E G E N                                              *
  // *                                                                       *
  // *************************************************************************


  queryExpr = generator.preGenCode(queryExpr);

  EXITMAIN_TASK_MONITOR( preGenTime );
  stats->exitCmpPhase(CompilationStats::CMP_PHASE_PRECODE_GENERATOR);
  MonitorMemoryUsage_Exit("PreCodeGen");

  if (!queryExpr)
    {
      sqlcompCleanup(input_str, queryExpr, FALSE);
      return PREGENERROR;
    }

  if (phase == PRECODEGEN)
    {
      *gen_code = (char *)queryExpr;
      return SUCCESS;
    }

  GUI_or_LOG_DISPLAY(
    Sqlcmpdbg::AFTER_PRECODEGEN, NSK_DBG_SHOW_TREE_AFTER_PRE_CODEGEN, "preCodegen");

  MonitorMemoryUsage_Enter("Generator");
  // generate code
  ENTERMAIN_TASK_MONITOR( generatorTime );
  stats->enterCmpPhase(CompilationStats::CMP_PHASE_GENERATOR);
  queryAnalysis->setCompilerPhase(QueryAnalysis::GENERATOR);


  // *************************************************************************
  // *                                                                       *
  // *      G E N E R A T O R                                                *
  // *                                                                       *
  // *************************************************************************


  generator.genCode(input_str, queryExpr);

  EXITMAIN_TASK_MONITOR( generatorTime );
  stats->exitCmpPhase(CompilationStats::CMP_PHASE_GENERATOR);
  MonitorMemoryUsage_Exit("Generator");
  
  GUI_or_LOG_DISPLAY(
    Sqlcmpdbg::AFTER_CODEGEN, NSK_DBG_SHOW_TREE_AFTER_CODEGEN, "codegen");

  Lng32 objLength = generator.getFinalObjLength();
  if (objLength <= 0) {
    sqlcompCleanup(input_str, queryExpr, TRUE);
    return GENERATORERROR;
  }
  else
    {
      retval = SUCCESS;

      TextKey *tkey = NULL;

      // ---------------------------------------------------------------
      // cache generated plan in query cache
      // ---------------------------------------------------------------

      // If this plan has been normalized for query plan caching and
      // is to be considered for inserting into the cache, attempt to
      // insert it and back patch the copy that is to be returned to
      // the caller.
      //
      // fix 10-061128-0744 (SQL statement does not compile first time but
      // compiles sec). Do not cache a plan if there are SQL errors in the
      // diag. area.
      if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0 
          && !CURRENTQCACHE->getHQC()->isPlanNoAQROrHiveAccess())
      {
        CacheKey *ckey = cachewa.getCacheKey(getStmtAttributes());
        if (ckey && cachewa.isCacheable()) {
          // get & save referenced tables' histograms' timestamps
          ckey->updateStatsTimes(cachewa.getTables());
          // try to cache this query
          CacheData data(&generator,
                         *cachewa.getFormalParamTypes(),
                         *cachewa.getFormalSelParamTypes(),
                         cachewa.getConstParamPositionsInSql(),
                         cachewa.getSelParamPositionsInSql(),
                         cachewa.getHQCConstPositionsInSql(),
                         generator.getPlanId(), input_str, charset,
                         CmpCommon::statementHeap());
          // backpatch parameters in query plan
          char * params; ULng32 paramSize;
          if (data.backpatchParams
              (cachewa.getConstParams(), cachewa.getSelParams(),
               cachewa.getConstParamPositionsInSql(),
               cachewa.getSelParamPositionsInSql(),
               bindWA, params, paramSize)) {
            // if winning plan is an MVQR plan and 
            //    has a parameterized equality predicate then
            // do NOT cache it because it can result in a false hit
            // that can cause wrong results
            // do NOT cache it if this is for EXPLAIN (useQueryCache == EXPLAIN)
            // to avoid security holes
            if ((!generator.isNonCacheablePlan()) && (useQueryCache == NORMAL)) {
              tkey = 
                cachewa.getTextKey(input_str, charset, getStmtAttributes());

              // cKeyInQCache, if not NULL, points at the domain cache key
              // allocated on CmpContext heap. It is safe to use it in HQC.
              CacheKey* ckeyInQCache = 
                          CURRENTQCACHE->addEntry(tkey,
                                    ckey, &data, *begTime, params, paramSize);
              //add a HybridQueryKey to cache, corresponding ckey is added internally.
              HQCParseKey* hkey = SqlParser_CurrentParser->getHQCKey();

              if ( CmpCommon::getDefault(HYBRID_QUERY_CACHE) == DF_ON && 
                   hkey && ckeyInQCache )
              {
                hkey->verifyCacheability(ckeyInQCache);

                ostream* hqc_ostream=CURRENTQCACHE->getHQCLogFile(); 
                if ( hqc_ostream ) 
                {
                    if(!hkey->isCacheable())
                      *hqc_ostream << "\nNot HQC Cacheable but added to SQC:" << endl
                                            << "SQL query=" << input_str << endl
                                            << "HQC key=" << hkey->getKey() << endl;
                }


                if ( hkey->isCacheable() && ckeyInQCache->useView() == FALSE)
                {
                   NAString hqcAddResult = "failed";
                   if (CURRENTQCACHE->HQCAddEntry(hkey, ckeyInQCache))
                      hqcAddResult = "passed";
                   
                   ostream* hqc_ostream=CURRENTQCACHE->getHQCLogFile();
                   if ( hqc_ostream ) 
                   {
                      *hqc_ostream << "\nHQC::AddEntry(): " << hqcAddResult << endl
                                   << "SQL query=" << input_str << endl
                                   << "HQC key=" << hkey->getKey() << endl;
                   }
                }
              }
            }
          }
          else { // backpatch failed!
            // announce an error to give caller a chance to retry
            // this sql compilation with query caching turned off
            sqlcompCleanup(input_str, queryExpr, TRUE);
            // code inspectors wants us to ASSERT here in DEBUG build
            DCMPASSERT("backpatchParams fails" == 0);
            return QCACHINGERROR;
          }
        }
      }

      // generated code is allocated as a list of contiguous
      // blocks. All these blocks have to be 'glued' together
      // to make the generated code contiguous.
      // Allocate a contiguous buffer (call it CB) with size equal to the
      // total size of the generated code and copy each of the
      // blocks into it. Then return this new buffer pointer
      // back to the caller.
      //
      // Later, when SQLCOMP becomes a separate process,
      // allocation of CB in this proc is no
      // longer necessary. Each of the contiguous buffers in the list could
      // be either written out to disk or sent back to executor
      // one at a time. Writing them out to disk one after the other will
      // make them contiguous on the disk. While sending them to executor,
      // they will be moved into a contiguous buffer in the executor one
      // at a time.
      char * cb = NULL;
      NABoolean allocFinalObj = FALSE;
      if (fragmentDir == NULL)
	allocFinalObj = TRUE;
#ifdef NA_DEBUG_GUI
      if ( CmpMain::msGui_ == CmpCommon::context() )
	allocFinalObj = TRUE;
#endif

      if (allocFinalObj)
	{
	  cb = new(heap) char[objLength];
	  *gen_code = generator.getFinalObj(cb, objLength);

	}
      else
	{
	  *gen_code = NULL;
	}

      *gen_code_len = objLength;

      // if the generated plan did not have auto qry retry
      // enabled, then we cannot text cache this query.
      // Remove it from preprocessor cache.
      if ((tkey) &&
	  (objLength > 0) &&
	  (generator.getFragmentDir()->getTopObj(0)))
	{
	  ComTdbRoot * rootTdb =
	    (ComTdbRoot *) generator.getFragmentDir()->getTopObj(0);
	  
	  if ((NOT rootTdb->aqrEnabled())||(rootTdb->hiveAccess()))
	    CURRENTQCACHE->deCachePreParserEntry(tkey);
	}

#ifdef NA_DEBUG_GUI
        if ( CmpMain::msGui_ == CmpCommon::context() )
          {
            // make a new copy of the generated code so we can unpack it
            char *newCopy = new char[*gen_code_len];
            str_cpy_all(newCopy, *gen_code, *gen_code_len);

            void * baseAddr         = (void *)newCopy;

            // unpack the baseAddr (the root) first.
            // This fragment corresponds to the MASTER.
            // This is done so we can get to the fragment directory.
            ComTdb tdb;
            ((ComTdb *)baseAddr)->driveUnpack(baseAddr,&tdb,NULL);

            ExFragDir *fragDir = ((ComTdbRoot *)baseAddr)->getFragDir();

            // a sanity check to see that it was the MASTER fragment.
            CMPASSERT(fragDir->getType(0) == ExFragDir::MASTER);

            // unpack each fragment independently;
            // unpacking converts offsets to actual pointers.
            for (CollIndex i=0; i < (CollIndex)fragDir->getNumEntries(); i++)
              {
                void * fragBase = (void *)((char *)baseAddr + fragDir->getGlobalOffset(i));
                void * fragStart = (void *)((char *)fragBase + fragDir->getTopNodeOffset(i));

                switch (fragDir->getType(i))
                {
                case ExFragDir::ESP:
                  ((ComTdb *)fragStart)->driveUnpack(fragBase,&tdb,NULL);
                  break;
                case ExFragDir::DP2:
		  {
                    CMPASSERT(fragDir->getType(i) != ExFragDir::DP2);
		  }
                  break;
                case ExFragDir::MASTER:
                  // already been unpacked. Nothing to be done.
                  break;
                case ExFragDir::EXPLAIN:
                  // Not using explain frag here!
                  break;
                }

              } // for each fragment
             if(((RelRoot *)queryExpr)->getDisplayTree())
               CmpMain::pExpFuncs_->fpDisplayTDBTree(Sqlcmpdbg::AFTER_TDBGEN,
                                                                                     (ComTdb *)baseAddr,
                                                                                     fragDir);

             // delete the copied object
             delete newCopy;

          } // display TDBs

        if (((RelRoot *)queryExpr)->getDisplayTree() && CmpMain::msGui_ == CmpCommon::context() )
          {
            if (CmpMain::pExpFuncs_->fpExecutionDisplayIsEnabled())
              {
                //------------------------------------------------------------
                // GSH: User has set a breakpoint for display in execution
                // so we need to set a flag bit in the generated TDB which
                // will tell the executor to display.
                //------------------------------------------------------------
		ComTdbRoot * originalRootTdb =
		  (ComTdbRoot *) generator.getFragmentDir()->getTopObj(0);
                originalRootTdb->setDisplayExecution(1);
              }
            else
              {
                //------------------------------------------------------------
                // GSH : User is using MS/LINUX gui but not interested in
                // execution/TCB display.
                //------------------------------------------------------------
                NADELETEBASIC(cb, heap);
                *gen_code = 0;
                *gen_code_len = 0;
                *CmpCommon::diags() << DgSqlCode(1032);
                retval = DISPLAYDONE;
              }
        } // display TCB tree
#endif // NA_DEBUG_GUI
        // "cb" is eventually freed by heap's destructor.
        // "cb" cannot be safely freed here because mxcmp-to-CLI IPC code
        // references "cb" until the generated plan is sent to the CLI.
        // coverity[leaked_storage]
    } // success, code generated



    // Capture the query shape if running in capture mode
    if (OSIM_runningInCaptureMode())
    {
      NAString * shapeStr = new (CmpCommon::statementHeap()) NAString(CmpCommon::statementHeap());
      (*shapeStr) += "control query shape ";

      queryExpr->generateShape(NULL, NULL, shapeStr);
      const char * qShape = shapeStr->data();
      OSIM_captureQueryShape(qShape);
    }  

  //
  // Ending CMP_PHASE_ALL will also mark the compileEndTime   
  stats->exitCmpPhase(CompilationStats::CMP_PHASE_ALL);
  //stats->dumpToFile();
  //
  // Update fields which couldn't be completed until now
  // This has to happen before sqlcompCleanup and 
  // after exitCmpPhase(CMP_PHASE_ALL)
  fixupCompilationStats((ComTdbRoot *)generator.getFragmentDir()->getTopObj(0),
                        (Space*) generator.getFragmentDir()->getSpace(0));

  queryAnalysis->setCompilerPhase(QueryAnalysis::POST_GENERATOR);

  ENTERMAIN_TASK_MONITOR( queryAnalysis->compCleanupMonitor() );
  sqlcompCleanup(NULL/*i.e. success*/, queryExpr, FALSE);
  EXITMAIN_TASK_MONITOR( queryAnalysis->compCleanupMonitor() );

  EXITMAIN_TASK_MONITOR( queryAnalysis->compilerMonitor() );  

  MonitorMemoryUsage_Exit("Compiler Total");
  MonitorMemoryUsage_LogAll();

/////////////////// little monitors //////////////////////
//if (OptDefaults::optimizerHeuristic2()) { //#ifdef _DEBUG  /* } make vi happy */

          NABoolean onlyLogCompilerAllTime = 
                 (CmpCommon::getDefault(COMPILE_TIME_MONITOR_LOG_ALLTIME_ONLY) == DF_ON);

          if (CURRSTMT_OPTDEFAULTS->compileTimeMonitor() && !onlyLogCompilerAllTime)
	  {
            const char * fname = getCOMPILE_TIME_MONITOR_OUTPUT_FILEname();

	    if (fname)
	    {
              ofstream fileout(fname, ios::app);
              fileout<<"\tOptimization Passes: "
                     <<(*CURRSTMT_OPTGLOBALS->cascadesPassMonitor)<<endl;
              fileout<<"\tOptimize Group Task: "
                     <<(*CURRSTMT_OPTGLOBALS->cascadesTasksMonitor[0])<<endl;
              fileout<<"\tOptimize Expr Task : "
                     <<(*CURRSTMT_OPTGLOBALS->cascadesTasksMonitor[1])<<endl;
              fileout<<"\tApply Rule Task    : "
                     <<(*CURRSTMT_OPTGLOBALS->cascadesTasksMonitor[2])<<endl;
              fileout<<"\tCreate Plan Task   : "
                     <<(*CURRSTMT_OPTGLOBALS->cascadesTasksMonitor[3])<<endl;
              fileout<<"\tExplore Group Task : "
                     <<(*CURRSTMT_OPTGLOBALS->cascadesTasksMonitor[4])<<endl;
              fileout<<"\tExplore Expr Task  : "
                     <<(*CURRSTMT_OPTGLOBALS->cascadesTasksMonitor[5])<<endl;
              fileout<<"\tGbg Collection Task: "
                     <<(*CURRSTMT_OPTGLOBALS->cascadesTasksMonitor[6])<<endl<<endl;
              fileout<<"\tFileScanOptimizer  : "
                     <<(*CURRSTMT_OPTGLOBALS->fileScanMonitor)<<endl;
/*
              fileout<<"\tUnderNestedJoin    : "
                     <<(*CURRSTMT_OPTGLOBALS->nestedJoinMonitor)<<endl;
              fileout<<"\tIndexNestedJoin    : "
                     <<(*CURRSTMT_OPTGLOBALS->indexJoinMonitor)<<endl;
              fileout<<"\tAsynchrMonitor     : "
                     <<(*CURRSTMT_OPTGLOBALS->asynchrMonitor)<<endl;
              fileout<<"\tSingleSubsetCost   : "
                     <<(*CURRSTMT_OPTGLOBALS->singleSubsetCostMonitor)<<endl;
              fileout<<"\tSingleVectorCost   : "
                     <<(*CURRSTMT_OPTGLOBALS->singleVectorCostMonitor)<<endl;
              fileout<<"\tSingleObjectCost   : "
                     <<(*CURRSTMT_OPTGLOBALS->singleObjectCostMonitor)<<endl;
              fileout<<"\tMultSubsetCost     : "
                     <<(*CURRSTMT_OPTGLOBALS->multSubsetCostMonitor)<<endl;
              fileout<<"\tMultVectorCost     : "
                     <<(*CURRSTMT_OPTGLOBALS->multVectorCostMonitor)<<endl;
              fileout<<"\tMultObjectCost     : "
                     <<(*CURRSTMT_OPTGLOBALS->multObjectCostMonitor)<<endl;
              fileout<<"\tOverflowNumber     : "
                     <<CostScalar::ovflwCount()<<endl;
              fileout<<"\tUnderflowNumber    : "
                     <<CostScalar::udflwCount()<<endl<<endl;
              //CURRCONTEXT_OPTDEBUG->showMemoStats(CURRSTMT_OPTGLOBALS->memo, " ", fileout);
*/
	    }
	    else
	    {
              cout<<"\tOptimization Passes: "<<(*CURRSTMT_OPTGLOBALS->cascadesPassMonitor)<<endl;
              cout<<"\tOptimize Group Task: "<<(*CURRSTMT_OPTGLOBALS->cascadesTasksMonitor[0])<<endl;
              cout<<"\tOptimize Expr Task : "<<(*CURRSTMT_OPTGLOBALS->cascadesTasksMonitor[1])<<endl;
              cout<<"\tApply Rule Task    : "<<(*CURRSTMT_OPTGLOBALS->cascadesTasksMonitor[2])<<endl;
              cout<<"\tCreate Plan Task   : "<<(*CURRSTMT_OPTGLOBALS->cascadesTasksMonitor[3])<<endl;
              cout<<"\tExplore Group Task : "<<(*CURRSTMT_OPTGLOBALS->cascadesTasksMonitor[4])<<endl;
              cout<<"\tExplore Expr Task  : "<<(*CURRSTMT_OPTGLOBALS->cascadesTasksMonitor[5])<<endl;
              cout<<"\tGbg Collection Task: "<<(*CURRSTMT_OPTGLOBALS->cascadesTasksMonitor[6])<<endl<<endl;
              cout<<"\tFileScanOptimizer  : "<<(*CURRSTMT_OPTGLOBALS->fileScanMonitor)<<endl;
/*
              cout<<"\tUnderNestedJoin    : "<<(*CURRSTMT_OPTGLOBALS->nestedJoinMonitor)<<endl;
              cout<<"\tIndexNestedJoin    : "<<(*CURRSTMT_OPTGLOBALS->indexJoinMonitor)<<endl;
              cout<<"\tAsynchrMonitor     : "<<(*CURRSTMT_OPTGLOBALS->asynchrMonitor)<<endl;
              cout<<"\tSingleSubsetCost   : "<<(*CURRSTMT_OPTGLOBALS->singleSubsetCostMonitor)<<endl;
              cout<<"\tSingleVectorCost   : "<<(*CURRSTMT_OPTGLOBALS->singleVectorCostMonitor)<<endl;
              cout<<"\tSingleObjectCost   : "<<(*CURRSTMT_OPTGLOBALS->singleObjectCostMonitor)<<endl;
              cout<<"\tMultSubsetCost     : "<<(*CURRSTMT_OPTGLOBALS->multSubsetCostMonitor)<<endl;
              cout<<"\tMultVectorCost     : "<<(*CURRSTMT_OPTGLOBALS->multVectorCostMonitor)<<endl;
              cout<<"\tMultObjectCost     : "<<(*CURRSTMT_OPTGLOBALS->multObjectCostMonitor)<<endl;
              cout<<"\tOverflowNumber     : "<<CostScalar::ovflwCount()<<endl;
              cout<<"\tUnderflowNumber    : "<<CostScalar::udflwCount()<<endl<<endl;
              //CURRCONTEXT_OPTDEBUG->showMemoStats(CURRSTMT_OPTGLOBALS->memo, " ", cout);
*/
	    }
	  }
//#endif
//////////////////////////////////////////////////////////
  if (CURRSTMT_OPTDEFAULTS->compileTimeMonitor())
  {
    compilerAll.exit();

    NABoolean onlyLogCompilerAllTime = 
            (CmpCommon::getDefault(COMPILE_TIME_MONITOR_LOG_ALLTIME_ONLY) == DF_ON);

    const char * fname = getCOMPILE_TIME_MONITOR_OUTPUT_FILEname();
    if (fname)
    {
      ofstream fileout(fname, ios::app);
      fileout<<"\tCompiler all: "<<queryAnalysis->compilerMonitor()<<endl;

      if ( !onlyLogCompilerAllTime ) {
         fileout<<"\tSqlcomp time: "<<compilerAll<<endl;
         fileout<<"\tParser      : "<<queryAnalysis->parserMonitor()<<endl;
         fileout<<"\tBinder      : "<<binderTime<<endl;
         fileout<<"\tTransformer : "<<transformerTime<<endl;
         fileout<<"\tNormalizer  : "<<normalizerTime<<endl;
         fileout<<"\tSemanticQOpt: "<<semanticQryOptTime<<endl;
         fileout<<"\tAnalyzer    : "<<analyzerTime<<endl;
         fileout<<"\tPilot Phase : "<<queryAnalysis->pilotPhaseMonitor()<<endl;
         fileout<<"\tJBB Setup   : "<<queryAnalysis->jbbSetupMonitor()<<endl;
         fileout<<"\tSynthLogProp: "<<queryAnalysis->synthLogPropMonitor()<<endl;
         fileout<<"\tASM Precomp : "<<queryAnalysis->asmPrecompMonitor()<<endl;
         fileout<<"\tQuery Graph : "<<queryAnalysis->queryGraphMonitor()<<endl;
         fileout<<"\tOptimizer   : "<<optimizerTime<<endl;
         fileout<<"\tPreCodeGen  : "<<preGenTime<<endl;
         fileout<<"\tGenerator   : "<<generatorTime<<endl;
         fileout<<"\tComp Cleanup: "<<queryAnalysis->compCleanupMonitor()<<endl;
         fileout<<"\tNodeMap     : "<<queryAnalysis->tempMonitor()<<endl;
      }
      fileout<<"--------------------------------------------"<<endl;
    }

    #ifndef NDEBUG
    else
    {
      cout<<"\tCompiler all: "<<queryAnalysis->compilerMonitor()<<endl;

      if ( !onlyLogCompilerAllTime ) {
         cout<<"\tSqlcomp time: "<<compilerAll<<endl;
         cout<<"\tParser      : "<<queryAnalysis->parserMonitor()<<endl;
         cout<<"\tBinder      : "<<binderTime<<endl;
         cout<<"\tTransformer : "<<transformerTime<<endl;
         cout<<"\tNormalizer  : "<<normalizerTime<<endl;
         cout<<"\tSemanticQOpt: "<<semanticQryOptTime<<endl;
         cout<<"\tAnalyzer    : "<<analyzerTime<<endl;
         cout<<"\tPilot Phase : "<<queryAnalysis->pilotPhaseMonitor()<<endl;
         cout<<"\tJBB Setup   : "<<queryAnalysis->jbbSetupMonitor()<<endl;
         cout<<"\tSynthLogProp: "<<queryAnalysis->synthLogPropMonitor()<<endl;
         cout<<"\tASM Precomp : "<<queryAnalysis->asmPrecompMonitor()<<endl;
         cout<<"\tQuery Graph : "<<queryAnalysis->queryGraphMonitor()<<endl;
         cout<<"\tOptimizer   : "<<optimizerTime<<endl;
         cout<<"\tPreCodeGen  : "<<preGenTime<<endl;
         cout<<"\tGenerator   : "<<generatorTime<<endl;
         cout<<"\tComp Cleanup: "<<queryAnalysis->compCleanupMonitor()<<endl;
         cout<<"\tNodeMap     : "<<queryAnalysis->tempMonitor()<<endl;
      }
      cout<<"--------------------------------------------"<<endl;
    }
    #endif
  }


  if (fragmentDir)
    *fragmentDir = generator.removeFragmentDir();

  return retval;
}

// constructor for query stmt attribute settings
QryStmtAttributeSet::QryStmtAttributeSet() : nEntries(0) {}

// copy constructor for query stmt attribute settings
QryStmtAttributeSet::QryStmtAttributeSet(const QryStmtAttributeSet& s)
  : nEntries(s.nEntries)
{
  if (nEntries > 0) {
    for (Int32 x = 0; x < nEntries; x++) {
      attrs[x] = s.attrs[x];
    }
  }
}

// add a query stmt attribute to attrs
void QryStmtAttributeSet::addStmtAttribute(SQLATTR_TYPE a, ULng32 v)
{
  Int32 x; NABoolean found = FALSE;
  for (x = 0; x < nEntries; x++) {
    if (attrs[x].attr == a) {
      found = TRUE;
      break;
    }
  }
  if (!found) {
    CMPASSERT(nEntries < MAX_STMT_ATTR);
    x = nEntries++;
  }
  attrs[x].attr = a;
  attrs[x].value = v;
}

// return true if set has given statement attribute
NABoolean QryStmtAttributeSet::has(const QryStmtAttribute& a) const
{
  NABoolean found=FALSE;
  for (Int32 x=0; x<nEntries && !found; x++) {
    found = attrs[x].isEqual(a);
  }
  return found;
}

#if defined(NA_DEBUG_GUI) 
void ExprNode::displayTree()
{
  //if libSqlCompilerDebugger.so not loaded, then load it.
  if ( !CmpMain::pExpFuncs_ )
  {
     if ( CmpMain::msGui_ == NULL ) // If no Compiler Instance is currently using debugger
     {
        CmpMain::msGui_ = CmpCommon::context();     // Claim ownership of debugger
        fpGetSqlcmpdbgExpFuncs GetExportedFunctions;
     
        dlptr = dlopen("libSqlCompilerDebugger.so",RTLD_NOW); //TODO
        if(dlptr != NULL)
        {
           GetExportedFunctions = (fpGetSqlcmpdbgExpFuncs) dlsym(dlptr, "GetSqlcmpdbgExpFuncs");
           if (GetExportedFunctions)
             CmpMain::pExpFuncs_ = GetExportedFunctions();
           if ( CmpMain::pExpFuncs_ == NULL )
           {
              int ret = dlclose(dlptr);
              dlptr = NULL;
           }
        }
        else // dlopen() failed 
        { 
           char *msg = dlerror();
           *CmpCommon::diags() << DgSqlCode(-2245)
                               << DgString0(msg);
        }
        if ( dlptr == NULL )
           CmpMain::msGui_ = NULL ;  //This Compiler Instance cannot use debugger
     }
     if ( CmpMain::pExpFuncs_ == NULL )
        return;
  }
  //open Gui Debugger
  initializeGUIData(CmpMain::pExpFuncs_);
  CmpMain::pExpFuncs_->fpDisplayQueryTree(Sqlcmpdbg::FROM_MSDEV, this, NULL);
} // displayTree()


void CascadesPlan::displayTree()
{
  //if libSqlCompilerDebugger.so not loaded, then load it.
  if ( !CmpMain::pExpFuncs_ )
  {
     if ( CmpMain::msGui_ == NULL ) // If no Compiler Instance is currently using debugger
     {
        CmpMain::msGui_ = CmpCommon::context();     // Claim ownership of debugger
        fpGetSqlcmpdbgExpFuncs GetExportedFunctions;
     
        dlptr = dlopen("libSqlCompilerDebugger.so",RTLD_NOW); //TODO
        if(dlptr != NULL)
        {
           GetExportedFunctions = (fpGetSqlcmpdbgExpFuncs) dlsym(dlptr, "GetSqlcmpdbgExpFuncs");
           if (GetExportedFunctions)
                CmpMain::pExpFuncs_ = GetExportedFunctions();
           if ( CmpMain::pExpFuncs_ == NULL )
           {
              int ret = dlclose(dlptr);
              dlptr = NULL;
           }
        }
        else // dlopen() failed 
        { 
           char *msg = dlerror(); 
           *CmpCommon::diags() << DgSqlCode(-2245)
                               << DgString0(msg);
        }
        if ( dlptr == NULL )
           CmpMain::msGui_ = NULL ;  //This Compiler Instance cannot use debugger
     }
     if ( CmpMain::pExpFuncs_ == NULL )
        return;
  }
  initializeGUIData(CmpMain::pExpFuncs_);
  CmpMain::pExpFuncs_->fpDisplayQueryTree(Sqlcmpdbg::FROM_MSDEV, NULL, this);

} // displayTree()
#endif

#if defined(NA_DEBUG_GUI)
void initializeGUIData(SqlcmpdbgExpFuncs* expFuncs)
{
        // Initialize gpClusterInfo if needed to pass it to the GUI
      if (!gpClusterInfo)
        {
          setUpClusterInfo(CmpCommon::contextHeap());
        }

      // Pass information about the plan to be displayed to the GUI:
      expFuncs->fpSqldbgSetCmpPointers(CURRSTMT_OPTGLOBALS->memo,
                                       CURRSTMT_OPTGLOBALS->task_list,
                                       QueryAnalysis::Instance(),
                                       cmpCurrentContext,
                                       gpClusterInfo);
} // initializeGUIData(...)

#endif



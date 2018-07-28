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
 * File:         CmpContext.C
 * Description:  The implementation of CmpContext class, this class contains
 *               the information of the global (across statement) variables
 *               for compiler components.
 *
 *
 * Created:      9/05/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */



#include "Platform.h"
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS	// first #include

#include "CmpCommon.h"
#include "CmpContext.h"
#include "CmpStatement.h"
#include "ControlDB.h"
#include "CmpMemoryMonitor.h"
#include "NAHeap.h"
#include "NewDel.h"
#include "OptimizerSimulator.h"
#include "SchemaDB.h"
#include "CmpCommon.h"
#include "Rule.h"               
#include "ImplRule.h"           // for CreateImplementationRules()
#include "TransRule.h"          // for CreateTransformationRules()
#include "PhyProp.h"            // for InitCostVariables()
#include "NAClusterInfo.h"
#include "UdfDllInteraction.h"
#define   SQLPARSERGLOBALS_FLAGS   // needed to set SqlParser_Flags
#include "SqlParserGlobals.h"			// last #include
#include "CmpErrLog.h"
#include "QRLogger.h"
#include "logmxevent.h"
#include "CmpSeabaseDDL.h"
#include "Globals.h"
#include "sqludr.h"
#include "hs_globals.h"

#include "PCodeExprCache.h"
#include "HBaseClient_JNI.h"
#include "CompException.h"
#include "CostMethod.h"

//++MV
extern "C" {
#include "cextdecs/cextdecs.h"
}


//--MV
  #define ENVIRON environ
  #define PUTENV  putenv

#include "NATable.h"
#include "CompilerTracking.h"
#include "HDFSHook.h"
ostream &operator<<(ostream &dest, const ComDiagsArea& da);

UInt32 hashCursor( const NAString& s ) { return s.hash(); }

// Global pointer to the Optimzer Memory Monitor
extern THREAD_P CmpMemoryMonitor *cmpMemMonitor;


// Global classes/structs initialized when CmpContext is constructed
extern THREAD_P NABoolean GU_DEBUG;
extern THREAD_P const SqlParser_NADefaults *SqlParser_NADefaults_Glob;


// Global classes/structs used in CmpContext scope
extern THREAD_P jmp_buf ExportJmpBuf;
extern THREAD_P jmp_buf CmpInternalErrorJmpBuf;

// -----------------------------------------------------------------------
// Methods for CmpContext
// -----------------------------------------------------------------------

CmpContext::CmpContext(UInt32 f, CollHeap * h)
: heap_((NAHeap *)h), 
  statements_(h, 16),
  flags_(f),
  internalCompile_(NOT_INTERNAL_COMPILE),
  // Template changes for Yosemite compiler incompatible with others
  staticCursors_(hashCursor),
  schemaDB_(NULL), controlDB_(NULL),tmpFileNumber_(-1),
  isRuntimeCompile_(TRUE),
  mxcmpPrimarySecondaryStatusSet_(FALSE),
  sqlmxRegress_(0),
  reservedMemory_(NULL),
  showQueryStats_(FALSE),
  recursionLevel_(0),
  parserResetIsNeeded_(FALSE),
  sqlTextBuf_(NULL),
  uninitializedSeabaseErrNum_(0),
  hbaseErrNum_(0),
  numSQNodes_(0),
  hasVirtualSQNodes_(FALSE),
  trafMDDescsInfo_(NULL),
  transMode_(TransMode::IL_NOT_SPECIFIED_,    // init'd below
             TransMode::READ_WRITE_,
             TransMode::OFF_),
  ciClass_(CmpContextInfo::CMPCONTEXT_TYPE_NONE),
  qcache_(NULL),                              // just to be safe ...
  optPCodeCache_(NULL),                       // just to be safe ...
  CDBList_(NULL),
  allControlCount_(0),
  optSimulator_(NULL),
  hosts_(h),
  invocationInfos_(h),
  planInfos_(h),
  routineHandles_(h),
  ddlObjs_(h),
  statementNum_(0),
  hiveClient_(NULL)
{
  SetMode(isDynamicSQL() ? STMT_DYNAMIC : STMT_STATIC);

  cmpCurrentContext = this;
  CMPASSERT(heap_ != NULL);

  heap_->setErrorCallback(&CmpErrLog::CmpErrLogCallback);

  // Reserve memory that can be used for out-of-memory reporting.
  reserveMemory();
  envs_ = new (heap_) ProcessEnv(heap_);

  sqlSession_ = new (heap_) CmpSqlSession(heap_);

// MV
// Adding support for multi threaded requestor (multi transactions) handling
// in the arkcmp
// This is needed because arkcmp is also serves as a server for the utilities
// store procedures
  OpenTMFFile();
// MV

  // initialize CmpStatement related members.
  statements_.insert(0);
  currentStatement_ = statements_.index(0);
  currentStatementPtrCache_ = statements_[currentStatement_];

  diags_ = ComDiagsArea::allocate(heap_);
  SqlParser_Diags = diags_;

  char* streamName;
  outFstream_ = NULL;

  // Set up the Optimizer Memory Monitor environment
  cmpMemMonitor_ = new(CmpCommon::contextHeap()) CmpMemoryMonitor(CmpCommon::contextHeap());
  
  cmpMemMonitor = cmpMemMonitor_;
  
  //## Commenting this out for now.  Eventually all ISP-arkcmp's we start up
  //## will need to have the *originating* arkcmp's context passed in,
  //## else the two arkcmp's *could* be inconsistent.
  //##	if (!isISP())
  {
    if (!isStandalone())
    {
      gpClusterInfo = NULL;
      setUpClusterInfo(CmpCommon::contextHeap());	// global cluster info
      // Store the cluster info in CmpContext
      clusterInfo_ = gpClusterInfo;
    }

    CDBList_ = new (heap_) CollationDBList(heap_);

    // globals for Optimizer -- also causes NADefaults table to be read in
    schemaDB_ = new(heap_) SchemaDB();

    // error during nadefault creation. Cannot proceed. Return.
    if (! schemaDB_->getDefaults().getSqlParser_NADefaults_Ptr())
      return;

    size_t memLimit = (size_t) 1024 * CmpCommon::getDefaultLong(MEMORY_LIMIT_NATABLECACHE_UPPER_KB);
    schemaDB_->getNATableDB()->getHeap()->setUpperLimit(memLimit);

    memLimit = (size_t) 1024 * CmpCommon::getDefaultLong(MEMORY_LIMIT_HISTCACHE_UPPER_KB);
    const Lng32 initHeapSize = 16 * 1024;    // ## 16K
    NAHeap *histogramCacheHeap = new (heap_) 
                                 NAHeap((const char *)"HistogramCache Heap",
                                 heap_,
                                 initHeapSize,
                                 memLimit);

    // Setting up the cache for histogram
    histogramCache_ = new(histogramCacheHeap) HistogramCache(histogramCacheHeap, 107);


    //  extern TransMode controlTransInfo;  // defined in Generator.C, should
    //				  // come from Compiler Stmt Context in future.
    //  schemaDB_->
    //    getDefaults().initAccessOption(   controlTransInfo.accessOption());
    //  schemaDB_->
    //    getDefaults().initIsolationLevel( controlTransInfo.isolationLevel());
  }

  CloseTMFFile();

  // tzset() sets extern var timezone to the diff(in seconds) between gmt and local.
  tzset();
  gmtDiff_ = timezone / 60;

  CmpProcess p;
  p.getCompilerId(compilerId_, COMPILER_ID_LEN);


  //  CmpSeabaseDDL cmpSBD;
  //  cmpSBD.processSystemCatalog();

  // initialize thread local variables (used to be globals) BEGIN
  USER_WANTS_DIVZERO_FAILURES = ( getenv( DIVZERO_ENV_VAR ) == NULL ) ? FALSE : TRUE ;
#ifndef NDEBUG
  GU_DEBUG = (getenv("GU_DEBUG") != NULL);
#endif
   optDbg_ = new (heap_) OptDebug();

  // initialize the Optimizer rules
  ruleSet_ = new (heap_) RuleSet(90, heap_) ;

  //
  // The creation of the implementaion and transformation
  // rules has to be waited until ruleSet_ is pointing at 
  // a true RuleSet. This is because during the creation,
  // the GlobalRulSet (aka CmpCommon::context()->getRuleSet(), or
  // this->ruleSet_) is referenced. 
  //
  CreateImplementationRules(ruleSet_);
  CreateTransformationRules(ruleSet_);

  // initialize thread local variables (used to be globals) END

  qcache_ = new (heap_) QueryCache();
  
  // set the QCache memory upper limit - currently only used to test setjmp/lonjmp logic
  qcache_->setHeapUpperLimit((size_t) 1024 * CmpCommon::getDefaultLong(MEMORY_LIMIT_QCACHE_UPPER_KB));

  tableIdent_ = 0;
  
  //
  // Initialize context-local optimized PCode Expression cache
  //
  optPCodeCache_ = new (heap_) OptPCodeCache();

  // Allocate (static) host_data_ if it's not already allocated.
  // This will never be deleted and stays until the process dies.

  tmfudf_dll_interface_host_data_ =  new (CmpCommon::contextHeap())
          char[SQLUDR_STATEAREA_BUFFER_SIZE + 128];

  CMPASSERT(tmfudf_dll_interface_host_data_);
  memset((char *)tmfudf_dll_interface_host_data_, 0,
           SQLUDR_STATEAREA_BUFFER_SIZE + 128);

  TimeVal currTime ;
  GETTIMEOFDAY(&currTime, 0);
  setPrev_QI_time( currTime ); // Initialize previous Query Invalidation time.

  lastUpdateStatsTime_ = -1;
  
  HHDFSMasterHostList::resetNumSQNodes();
  HHDFSMasterHostList::resethasVirtualSQNodes();

  optDefaults_ = new (heap_) OptDefaults(heap_);

  // create dynamic metadata descriptors
  CmpSeabaseDDL cmpSeabaseDDL(heap_);
  cmpSeabaseDDL.createMDdescs(trafMDDescsInfo_);
}

// MV
// ---------------------------------------------------------------------------
// Adding support for multi threaded requestor (multi transactions) handling
// in the arkcmp
// This is needed because arkcmp is also serves as a server for the utilities
// store procedures
// ---------------------------------------------------------------------------

void CmpContext::OpenTMFFile()
{
  if (FALSE == isISP())
    return;

  // now open $RECEIVE
}

void CmpContext::CloseTMFFile()
{
  if (FALSE == isISP())
    return;

}

// -----------------------------------------------------------------------
// destructor
// -----------------------------------------------------------------------

// MV
CmpContext::~CmpContext()
{
  // cleanup the optimizer rules
  //    delete ruleSet_;   -- don't do this, it causes access violations
  //                       -- (due to delete'ing same memory twice:  our
  //                       -- trees are more often DAGs...)!
  ruleSet_ = NULL;

  if (diags_)
    diags_->decrRefCount();

  delete schemaDB_;
  delete controlDB_;
  NADELETE(optDefaults_, OptDefaults, heap_);
  // trafMDDescsInfo_ is of type MDDescsInfo but it is created as an array of char
  // look at the creation of trafMDDescsInfo_ in method CmpSeabaseDDL::createMDdescs
  NADELETEBASIC((char *)trafMDDescsInfo_, heap_);
  
  if(optSimulator_)
      delete  optSimulator_;
   optSimulator_ = NULL;
  
  schemaDB_ = 0;
  controlDB_ = 0;
  optDefaults_ = 0;
  trafMDDescsInfo_ = NULL;

  delete envs_;
  envs_ = 0;
  if (outFstream_ != NULL)
     delete outFstream_;
  currentStatementPtrCache_ = NULL;

  if ( isRuntimeCompile_) // Seems like QueryCache is not initialized for static compiler  
    qcache_->finalize((char *)"Drop Session");

  if ( CTXTHEAP ) // Defensive coding - if NULL, the Context Heap is gone. 
    delete optPCodeCache_ ;

  optPCodeCache_ = NULL ;

  resetContext();
  // reset thread global variables
  HSGlobalsClass::resetJitLogThresholdHash();
}

NABoolean CmpContext::initContextGlobals()
{
  NABoolean rtnStatus = TRUE ;      // assume the best

    // globals for ustat
  uStatID_ = 0;

  // retrieve SQLMX_REGRESS environmnet variable
  const char *env;
  env = getenv("SQLMX_REGRESS");
  if (env)
  {
    sqlmxRegress_ = atoi(env);
  }

  return rtnStatus;
}

NAHeap* CmpContext::statementHeap()
{
  return statement() ? statement()->heap() : 0;
}

void CmpContext::setSecondaryMxcmp(){
  if(mxcmpPrimarySecondaryStatusSet_)
    return;

  mxcmpPrimarySecondaryStatusSet_ = TRUE;

  // Check for env var that indicates this is a secondary mxcmp process.
  // Keep this value in CmpContext so that secondary mxcmp on NSK
  // will allow MP DDL issued from a parent mxcmp process.
  NABoolean IsSecondaryMxcmp = FALSE;
  if (getenv("SECONDARY_MXCMP") != NULL)
  {
    IsSecondaryMxcmp = TRUE;
  }

  flags_ = flags_ | (IsSecondaryMxcmp ? CmpContext::IS_SECONDARY_MXCMP : 0);

  // Any downstream process will be a "SECONDARY_MXCMP".
  PUTENV((char *)"SECONDARY_MXCMP=1");
  Lng32 rc = SQL_EXEC_SetEnviron_Internal(0);

}

// ----------------------------------------------------------------------------
// method: setAuthorizationState
//
// This method is called during compiler context startup to determine if 
// authorization has been enabled.  It sets two flags in the compiler context 
// structure based on the passed in state:
//
//    IS_AUTHORIZATION_ENABLED
//      set to TRUE if one or more privmgr metadata tables exists
//
//    IS_AUTHORIZATION_READY
//      set to TRUE if all privmgr metadata tables exist
//
// input: state
//   0: no metadata tables exist, authorization is not enabled
//   1: all metadata tables exist, authorization is enabled
//   2: some metadata tables exist, privmgr metadata is not ready
//  -nnnn: an unexpected error occurred
// ----------------------------------------------------------------------------
void CmpContext::setAuthorizationState (Int32 state)
{
  switch (state)
    {
      // state = 0, not initialized
      case 0:
        setIsAuthorizationEnabled(FALSE);
        setIsAuthorizationReady(FALSE);
        break;

      // state = 1, initialized
      case 1:
        setIsAuthorizationEnabled(TRUE);
        setIsAuthorizationReady(TRUE);
        break;

      // state = 2, partially initialized
      case 2:
        setIsAuthorizationEnabled(TRUE);
        setIsAuthorizationReady(FALSE);
        break;

      // else unexpected error 
      default:

        // Unable to determine authorization status, set 
        // authorizationReady to FALSE.
        setIsAuthorizationReady(FALSE);

        // Get status from the TRAFODION_ENABLE_AUTHORIZATION envvar.
        // The TRAFODION_ENABLE_AUTHENTICATION envvar defined in the 
        // sqenvcom.sh file.  Since authorization can be initialized
        // independently from setting the envvar, we cannot necessarily
        // depend on its value. So we only use it if we can't get the 
        // information from anywhere else.
        char * env = getenv("TRAFODION_ENABLE_AUTHENTICATION");
        if (env)
          {
            setIsAuthorizationEnabled
              ((strcmp(env, "YES") == 0) ? TRUE : FALSE);
          }
        else
          {
            // Can't determine status, so be on the safe side and set 
            // to TRUE
            setIsAuthorizationEnabled(TRUE);
          }
        
         // setup an internal error
         *diags_
           << DgSqlCode(-1001)
           << DgString0(__FILE__)
           << DgInt0(__LINE__)
           << DgString1("Unable to determine authorization status");
    }
}

// ----------------------------------------------------------------------------
// method: isAuthorizationEnabled
//
// Return the value of IS_AUTHORIZATION_ENABLED
//
// input:
//   errIfNotReady:
//     TRUE - generate an error if authorization is enabled and privmgr
//            metadata table(s) are  missing or inaccessible (incomplete)
// ----------------------------------------------------------------------------     
NABoolean CmpContext::isAuthorizationEnabled( NABoolean errIfNotReady)
{
  if (flags_ & IS_AUTHORIZATION_ENABLED)
  {
    if (errIfNotReady && !isAuthorizationReady())
    {
      if (!diags_->contains(-1234))
      {
        *diags_ << DgSqlCode(-1234);
         CMPASSERT (FALSE);
      }
    }   
    return TRUE;
  }
  return FALSE;
}

// -----------------------------------------------------------------------
// The CmpStatement related methods
// -----------------------------------------------------------------------

void CmpContext::setStatement(CmpStatement* s)
{
  init();
  statements_.insert(s);
  s->setPrvCmpStatement(statements_[currentStatement_]);
  currentStatement_ = statements_.index(s);
  currentStatementPtrCache_ = statements_[currentStatement_];

  // Commented this out, as init() is now a no-op:
  //	if (statements_.entries() == 2) init();
}

void CmpContext::unsetStatement(CmpStatement* s, NABoolean exceptionRaised)
{
  CollIndex i = statements_.index(s);
  statements_.removeAt(i);
  currentStatement_ = statements_.index(s->prvCmpStatement());
  currentStatementPtrCache_ = statements_[currentStatement_];
  for ( i = 0; i < statements_.entries(); i++ )
    if (statements_[i] && statements_[i]->prvCmpStatement() == s)
      statements_[i]->setPrvCmpStatement(s->prvCmpStatement());
  if ( statements_.entries() == 1 ) {
    cleanup(exceptionRaised);
    if (s->diags()) s->diags()->clear();
  }
}

void CmpContext::setCurrentStatement(CmpStatement* s)
{
  CollIndex i = statements_.index(s);
  currentStatement_ = ( i == NULL_COLL_INDEX ) ? 0 : i;
  currentStatementPtrCache_ = statements_[currentStatement_];
  CMPASSERT(s->diags()->getNumber() == 0);
  // diags()->clear();
}

// Method to initialize the context at the beginning of statement
void CmpContext::init()
{
  // initSchemaDB();		-- This was done in the ctor.
  // diags()->clear();		-- This loses any initialization errors;
  //				-- clear() is done in unsetStatement above.
  statementNum_++;
}

// -----------------------------------------------------------------------
// Method to cleanup the context contents at the end of each statement
// -----------------------------------------------------------------------

void CmpContext::cleanup(NABoolean exception)
{
  //if(isISP()) return;
  if (exception)
    {
	  // TODO, we should consider cleaning up other members in CmpCOntext once
	  // exception raised since they might not be reliable anymore.
    }
  schemaDB_->cleanupPerStatement();
  if(optSimulator_)
    optSimulator_->cleanupAfterStatement();
  if(cmpMemMonitor)
    cmpMemMonitor->cleanupPerStatement();
  if (invocationInfos_.entries() > 0)
    {
      // Clean up UDR structures that are allocated from the
      // system heap. Plan infos can only exist if we also have
      // invocation infos.
      for (CollIndex i=0; i<invocationInfos_.entries(); i++)
        TMUDFInternalSetup::deleteUDRInvocationInfo(invocationInfos_[i]);
      for (CollIndex p=0; p<planInfos_.entries(); p++)
        TMUDFInternalSetup::deleteUDRPlanInfo(planInfos_[p]);
      invocationInfos_.clear();
      planInfos_.clear();
    }
  if (routineHandles_.entries() > 0)
    {
      ExeCliInterface cliInterface(
           statementHeap(),
           NULL,
           NULL, 
           CmpCommon::context()->sqlSession()->getParentQid());

      for (CollIndex r=0; r<routineHandles_.entries(); r++)
        cliInterface.putRoutine(routineHandles_[r],
                                CmpCommon::diags());
      routineHandles_.clear();
    }
}

// -----------------------------------------------------------------------
// Methods for optimizer globals
// -----------------------------------------------------------------------

void CmpContext::initControlDB()
{
  delete controlDB_;
  controlDB_ = new(heap_) ControlDB;
  controlDB_->initPerStatement();
}

void CmpContext::initSchemaDB()
{
  // TODO the initialization of valueDArray_ at the beginning of each statement.

  if (schemaDB_)  schemaDB_->initPerStatement();
  if (controlDB_) controlDB_->initPerStatement();
}

// CmpContext::reserveMemory() reserves some memory in the context heap
// that is freed on certain out-of-memory situations. This allows the
// condition to be reported to MXCI or MXOSRVR.
void CmpContext::reserveMemory()
{
  if (reservedMemory_ == NULL)
    reservedMemory_ = heap_->allocateMemory(2048);
}

// CmpContext::freeReservedMemory() frees the memory that was reserved for
// out-of-memory reporting.
void CmpContext::freeReservedMemory()
{
  if (reservedMemory_ != NULL && heap_ != NULL)
  {
    heap_->deallocateMemory(reservedMemory_);
    reservedMemory_ = NULL;
  }
}

void CmpContext::switchContext()
{
  SqlParser_NADefaults_Glob =
      ActiveSchemaDB()->getDefaults().getSqlParser_NADefaults_Ptr();
  gpClusterInfo = clusterInfo_;
  SqlParser_Diags = diags();
  cmpMemMonitor = cmpMemMonitor_;
}

void CmpContext::switchBackContext()
{
  SqlParser_NADefaults_Glob =
      ActiveSchemaDB()->getDefaults().getSqlParser_NADefaults_Ptr();
  gpClusterInfo = clusterInfo_;
  SqlParser_Diags = diags();
  cmpMemMonitor = cmpMemMonitor_;
}

void CmpContext::resetContext()
{
  SqlParser_NADefaults_Glob = NULL;
  gpClusterInfo = NULL;
  SqlParser_Diags = NULL;
  if (CmpCommon::diags())
     CmpCommon::diags()->clear();
  cmpMemMonitor = NULL;
}

CmpStatementISP* CmpContext::getISPStatement(Int64 id)
{
  NAList<CmpStatement*> & statements = this->statements();
  CmpStatementISP* ispStatement;
  for ( CollIndex i = 0; i < statements.entries(); i++)
    if ( statements[i] && ( ispStatement = (statements[i])->ISPStatement() ) 
      && ispStatement->ISPReqId() == id )
      return ispStatement;
  return 0;
}

// Interface to the embedded arkcmp, used for executor master to compile
// query statement using this SQL compiler inside the same process.
//
// Return value:  (should sync with ExSqlComp::ReturnStatus)
//     0 - SUCCESS:
//     1 - WARNING: (not used)
//     2 - ERROR: any compiler internal errors
//     3 - MORE_DATA: only for REPLY_ISP_
Int32
CmpContext::compileDirect(char *data, UInt32 data_len, CollHeap *outHeap,
                          Int32 charset, CmpMessageObj::MessageTypeEnum op,
                          char *&gen_code, UInt32 &gen_code_len,
                          UInt32 parserFlags, const char *parentQid,
                          Int32 parentQidLen,
                          ComDiagsArea *&diagsArea)
{

  CmpStatement::ReturnStatus rs = CmpStatement::CmpStatement_SUCCESS;
  CmpStatement *cmpStatement = NULL;
  NABoolean copyFrags = FALSE, copyData = FALSE;

  // save the callers cin/cout and redirect compiler's cin/cout
  std::streambuf *savedCoutBuf = std::cout.rdbuf() ; // Save cout's streambuf
  std::streambuf *savedCinBuf = std::cin.rdbuf(); //Save cin's streambuf 

  incrRecursionLevel();
  if (outFstream_)
    {
     
      cout.rdbuf(outFstream_->rdbuf());
      cin.rdbuf(outFstream_->rdbuf());
    }
  // keep track of the parser flags we started with and passed in
  // to the compiler.
  UInt32 savedCliParserFlags = parserFlags;
  switchContext();
  // set jmpbuf for compilation of the statement
  // the reference of the ExportJmpBuf will be hept in CmpStatement heap
  // shortly in the construction of CmpStatement
  jmp_buf savedJB, savedInternalErrJB;
  memcmp(savedJB, ExportJmpBuf, sizeof(jmp_buf));
  memcmp(savedInternalErrJB, CmpInternalErrorJmpBuf, sizeof(jmp_buf));

  Int32 jRc = setjmp(ExportJmpBuf);
  if (jRc)
    {  // longjmp here
      if (cmpStatement)
        delete cmpStatement;

      // populate error
      if (diagsArea)
        if (jRc == MEMALLOC_FAILURE)
          *diagsArea << DgSqlCode(arkcmpErrorOutOfMemory);
        else
          *diagsArea << DgSqlCode(arkcmpErrorAssert);

      // Log this error to the file indicated by CMP_ERR_LOG_FILE CQD.
      if (jRc == MEMALLOC_FAILURE)
          CmpErrLog("Memory allocation failure");

      decrRecursionLevel();

      // restore jmp_buf contents
      memcpy(ExportJmpBuf, savedJB, sizeof(jmp_buf));
      memcpy(CmpInternalErrorJmpBuf, savedInternalErrJB, sizeof(jmp_buf));

      // restore callers cin and cout 
      if(outFstream_)
        {
          //std::cout << std::endl ;       // Flush anything in the data buffer
          cout.rdbuf( savedCoutBuf ) ; // Restore cout's streambuf
          cin.rdbuf(savedCinBuf);
        }

      return CmpStatement::CmpStatement_ERROR; // temp use of this error;
    }

  Int32 jRc2 = setjmp(CmpInternalErrorJmpBuf);
  if (jRc2)
    {  // longjmp here
      if (cmpStatement)
        delete cmpStatement;

      CostMethod::cleanUpAllCostMethods();

      // populate error
      if (diagsArea)
        if (jRc2 == MEMALLOC_FAILURE)
          {
            freeReservedMemory();
            *diagsArea << DgSqlCode(arkcmpErrorOutOfMemory);
          }
        else
          *diagsArea << DgSqlCode(arkcmpErrorAssert);

      // Log this error to the file indicated by CMP_ERR_LOG_FILE CQD.
      if (jRc2 == MEMALLOC_FAILURE)
          CmpErrLog("Memory allocation failure");

      decrRecursionLevel();

      // restore jmp_buf contents
      memcpy(ExportJmpBuf, savedJB, sizeof(jmp_buf));
      memcpy(CmpInternalErrorJmpBuf, savedInternalErrJB, sizeof(jmp_buf));

      // restore callers cin and cout 
      if(outFstream_)
        {
          //std::cout << std::endl ;       // Flush anything in the data buffer
          cout.rdbuf( savedCoutBuf ) ; // Restore cout's streambuf
          cin.rdbuf(savedCinBuf);
        }
      
      return CmpStatement::CmpStatement_ERROR; // temp use of this error;
    }

  // make sure we have reserved memory in the context
  reserveMemory();

  try
  {
    switch (op)
    {
      case CmpMessageObj::SQLTEXT_RECOMPILE :
      case CmpMessageObj::SQLTEXT_COMPILE :
      {
        // request is from cli interface for compiling most sql statements
        // NAMemory.h.
        cmpStatement = new CTXTHEAP CmpStatement(this);
        CmpMessageSQLText sqltext(data, data_len, CTXTHEAP, charset, op);
        Assign_SqlParser_Flags(parserFlags);
        rs = cmpStatement->process(sqltext);
        copyFrags = TRUE;
        break;
      }
      case CmpMessageObj::SQLTEXT_STATIC_RECOMPILE :
      case CmpMessageObj::SQLTEXT_STATIC_COMPILE :
      {
        // request is from ex_control_tcb::work() for setting compiler CQDs
        cmpStatement = new CTXTHEAP CmpStatement(this);
        CmpMessageCompileStmt compileStmt(data, data_len, op, CTXTHEAP, charset);
        Assign_SqlParser_Flags(parserFlags);
        rs = cmpStatement->process(compileStmt);
        copyData = TRUE;
        break;
      }
      case (CmpMessageObj::DATABASE_USER) :
      {
        // request is from ContextCli::createMxcmpSession() to set user id
        cmpStatement = new CTXTHEAP CmpStatement(this);
        CmpMessageDatabaseUser databaseUserStmt(data, data_len, CTXTHEAP);
        Assign_SqlParser_Flags(parserFlags);
        rs = cmpStatement->process(databaseUserStmt);
        copyData = TRUE;
        break;
      } // end of case (CmpMessageObj::DATABASE_USER)
      case (CmpMessageObj::DDL) :
      {
        // request is from ExDDLTcb::work() to get statement explain
        cmpStatement = new CTXTHEAP CmpStatement(this);
        CmpMessageDDL ddlStmt(data, data_len, CTXTHEAP, charset,
                             parentQid, parentQidLen);
 
        Assign_SqlParser_Flags(parserFlags);
        rs = cmpStatement->process(ddlStmt);
        copyData = TRUE;
        break;
      } // end of case (CmpMessageObj::DDL)
      case (CmpMessageObj::DDL_WITH_STATUS) :
      {
        // request is from ExDDLTcb::work() to get statement explain
        cmpStatement = new CTXTHEAP CmpStatement(this);
        CmpMessageDDLwithStatus ddlStmt(data, data_len, CTXTHEAP);
 
        Assign_SqlParser_Flags(parserFlags);
        rs = cmpStatement->process(ddlStmt);
        copyData = TRUE;
        break;
      } // end of case (CmpMessageObj::DDL)
      case (CmpMessageObj::DESCRIBE) :
      {
        // request is from ExDescribeTcb::work() to get statement explain
        cmpStatement = new CTXTHEAP CmpStatement(this);
        CmpMessageDescribe describeStmt(data, data_len, CTXTHEAP, charset);
        Assign_SqlParser_Flags(parserFlags);
        rs = cmpStatement->process(describeStmt);
        copyData = TRUE;
        break;
      } // end of case (CmpMessageObj::DESCRIBE)
      case (CmpMessageObj::END_SESSION) :
      {
        // request is from ContextCli::endMxcmpSession()
        cmpStatement = new CTXTHEAP CmpStatement(this);
        CmpMessageEndSession endSessionStmt(data, data_len, CTXTHEAP);
        Assign_SqlParser_Flags(parserFlags);
        rs = cmpStatement->process(endSessionStmt);
        copyData = TRUE;
        break;
      } // end of case (CmpMessageObj::END_SESSION)
      case (CmpMessageObj::SET_TRANS) :
      {
        // request is from ExDescribeTcb::work() for getting compiler CQDs
        cmpStatement = new CTXTHEAP CmpStatement(this);
        CmpMessageSetTrans setTransStmt(data, data_len, CTXTHEAP);
        Assign_SqlParser_Flags(parserFlags);
        rs = cmpStatement->process(setTransStmt);
        copyData = TRUE;
        break;
      } // end of case (CmpMessageObj::SET_TRANS)

       case (CmpMessageObj::DDL_NATABLE_INVALIDATE) :
      {
        cmpStatement = new CTXTHEAP CmpStatement(this);
        CmpMessageDDLNATableInvalidate ddlInvalidateStmt(data, data_len, CTXTHEAP);
        Assign_SqlParser_Flags(parserFlags);
        rs = cmpStatement->process(ddlInvalidateStmt);
        copyData = TRUE;
        break;
      } // end of case (CmpMessageObj::DDL_NATABLE_INVALIDATE)

     case (CmpMessageObj::INTERNALSP_REQUEST) :
      { 
        //request is from ExStoredProcTcb::work(), 
        cmpStatement = new CTXTHEAP CmpStatementISP(this);
        //ISP request is passed as data argument
        CMPASSERT(data);
        CmpMessageISPRequest & ispRequest = *(CmpMessageISPRequest *)data;
        Assign_SqlParser_Flags(parserFlags);
        //process request
        rs = cmpStatement->process(ispRequest);
        copyData = TRUE;
        break;
      }
      
      case (CmpMessageObj::INTERNALSP_GETNEXT) :
      { 
         //request is from ExStoredProcTcb::work()
         CMPASSERT(data);
          //ISPGetNext request is passed as data argument
         CmpMessageISPGetNext & ispGetNext = *(CmpMessageISPGetNext *)data;
         //do not create new statement for GetNext, 
         //find previous-created statement in current context through requestId
         cmpStatement = getISPStatement(ispGetNext.ispRequest());
         if (!cmpStatement)
         {
            // There must be a previous ispStatement, otherwise it is an 
            // internal error. Instantiate a dummy CmpStatement here, just
            // to place the error information.
            cmpStatement = new CTXTHEAP CmpStatement(this);
            CMPASSERT(FALSE);
         }
         Assign_SqlParser_Flags(parserFlags);//What is this for??
         //process request
         rs = cmpStatement->process(ispGetNext);
         copyData = TRUE;
         break;
      }
      
      default :  // the embedded compiler can not handle other operation yet
      {
        char emsText[120];
        str_sprintf(emsText,
                    "Embedded arkcmp does not support this operation: %d.",
                    op);
        SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, emsText, 0);
        rs = CmpStatement::CmpStatement_ERROR; // temp use of this error
      }
    } // switch (op)
  }
  catch (CmpInternalException &exInternal)
  {
      if (cmpStatement)
      {
        cmpStatement->error(arkcmpErrorAssert, exInternal.getMsg());
        cmpStatement->exceptionRaised();
      }
      else
      {
        ComDiagsArea *diags = ComDiagsArea::allocate(heap_);
        *diags << DgSqlCode(arkcmpErrorAssert)
          << DgInt0(0) << DgString0("from CMPASSERT")
          << DgString1("CmpConnection::actOnReceive,EH_INTERNAL_EXCEPTION");
      }
      rs = CmpStatement::CmpStatement_ERROR; // temp use of this error
  }
  catch (...)
  {
    rs = CmpStatement::CmpStatement_ERROR; // temp use of this error
  }

  char *result;
  IpcMessageObjSize rp_len;
  // todo: need to translate CmpStatement::ReturnStatus to
  //       ExSqlComp::ReturnStatus
  if (rs == CmpStatement::CmpStatement_SUCCESS && cmpStatement->reply())
  {
    // get the plan
    Int32 objType = cmpStatement->reply()->getType();
    switch (objType)
      {
      case CmpMessageObj::REPLY_CODE :
        {
          CmpMessageReplyCode *rp = (CmpMessageReplyCode*)cmpStatement->reply();

          rp_len = rp->getSize(); // get the size of all fragments
          result = (char *)outHeap->allocateMemory(rp_len); 
          gen_code = result;
          if (copyFrags == TRUE)// copy all frags into the buffer
            {
              // for regular dynamic statement, the generated plan fragments
              // are saved in the fragmentDir_ unless the plan was from cache
              if (rp->getFragmentDir())
                IpcMessageObjSize rt_len = rp->copyFragsToBuffer(result);
              else // cached plan was linked to data_
                memcpy(result, rp->getData(), rp->getSize());
            }
          else if (copyData == TRUE)
            memcpy(result, rp->getData(), rp->getSize());
          gen_code_len = rp_len;
          break;
        }
        
      case CmpMessageObj::REPLY_ISP :
        {
          if(copyData == TRUE && rs == CmpStatement::CmpStatement_SUCCESS)
          {
              CmpMessageReplyISP *rp = (CmpMessageReplyISP*)cmpStatement->reply();
              //Receiving buffer is pointed by gen_code, which should be allocated by caller ( ExStoredProcTcb::work() ).
              //Check receiving buffer length is larger than data length.
              if(gen_code_len >= rp->getSize())
                memcpy(gen_code, rp->getData(), rp->getSize());
              else
                rs = CmpStatement::CmpStatement_ERROR;
              //caller shoud check if replied data length exceeds the gen_code_len passed in.
              gen_code_len = rp->getSize();
          }
          break;
        }
      default :
        break;
      }
  }

  ComDiagsArea *compileDiagsArea = CmpCommon::diags();
  if (compileDiagsArea->getNumber() > 0)
  {
     // get any errors or warnings from compilation out before distroy it
     if (diagsArea == NULL)
       diagsArea = ComDiagsArea::allocate(outHeap);
     diagsArea->mergeAfter(*compileDiagsArea);
     compileDiagsArea->clear();
  }
  // cleanup and return
  if (cmpStatement && cmpStatement->readyToDie())
    delete cmpStatement;

  // Restore the CLI flags as they were. The compiler may have changed the CLI
  // parser flags using SQL_EXEC_SetParserFlagsExSqlComp_Internal.
  SQL_EXEC_AssignParserFlagsForExSqlComp_Internal(savedCliParserFlags);

  // Restore jmp_buf
  memcpy(ExportJmpBuf, savedJB, sizeof(jmp_buf));
  memcpy(CmpInternalErrorJmpBuf, savedInternalErrJB, sizeof(jmp_buf));

  // restore callers cin and cout 
  if(outFstream_)
    {
      //std::cout << std::endl ;       // Flush anything in the data buffer
      cout.rdbuf( savedCoutBuf ) ; // Restore cout's streambuf
      cin.rdbuf(savedCinBuf);
    }

  decrRecursionLevel();
  if (rs == CmpStatement::CmpStatement_SUCCESS)
    return 0; // SUCCESS
  return 2; // FAILURE
}

// set/reset an env in compiler envs
void
CmpContext::setArkcmpEnvDirect(const char *name,
                               const char *value,
                               NABoolean unset)
{
  if (unset)
    {
      // unset means remove the env name from CmpContext::envs_ list and call
      // putenv("<name>"), see ProcessEnv::removeEnv() (not ::unsetEnv())
      envs()->resetEnv((char*)name);
      return;  // we done.
    }

  // prepare the env string "name=value" first
  char *envStr;
  Int32 strLen;
  strLen = str_len(name) + str_len(value) + 1; // name=value
  if (strLen <= 0)
    return;  // do nothing
  envStr = new (heap_) char[strLen + 1];
  str_sprintf(envStr, "%s=%s", name, value);
  // set the env
  envs()->addOrChangeEnv(&envStr, 1);
  NADELETEBASIC(envStr, heap_);

  // the following is copied from arkcmp/CmpStatement.cpp as the side-effect
  // altering env in compiler.
  // see CmpStatement::process(const CmpMessageEnvs& envMessage) for details
  const char * env;
  env = getenv("SQLMX_REGRESS");
  if (env)
    {
      setSqlmxRegress(atoi(env));

      // turn mode_special_1 OFF during regressions run.
      // Special1 features cause
      // many regressions to return mismatches due to special TD semantics.
      // When some
      // of the special1 features are externalized and enabled for general
      // NEO users, then we can remove these lines.
      NAString value("OFF");
      ActiveSchemaDB()->getDefaults().validateAndInsert(
           "MODE_SPECIAL_1", value, FALSE);
    }
}

// return compiler tracking information
// there is one per compiler context
CompilerTrackingInfo*
CmpContext::getCompilerTrackingInfo()
{
   if (compilerTrackingInfo_ == NULL)
       compilerTrackingInfo_ = new (heap_) CompilerTrackingInfo(heap_);

   compilerTrackingInfo_->resetIntervalIfNeeded();

   return compilerTrackingInfo_;
}

ULng32 CmpContext::getTMFUDF_DLL_InterfaceHostDataBufferLen()
{ return SQLUDR_STATEAREA_BUFFER_SIZE; }

// Set the SQL text for later use. If the buffer is not empty, this
// call does nothing.
//
void CmpContext::setLogmxEventSqlText(const NAWString& x)
{
   if ( sqlTextBuf_ == NULL )
        sqlTextBuf_ = new NAWString(x) ;
}

//
// clear up the SQL text so that next setSqlText() call can have effect.
//
void CmpContext::resetLogmxEventSqlText()
{
   delete sqlTextBuf_ ;
   sqlTextBuf_ = NULL ;
}

void CmpContext::clearAllCaches()
{
   qcache_->makeEmpty();
   schemaDB_->getNATableDB()->setCachingOFF();
   schemaDB_->getNATableDB()->setCachingON();
   schemaDB_->getNARoutineDB()->setCachingOFF();
   schemaDB_->getNARoutineDB()->setCachingON();
   if(histogramCache_)
      histogramCache_->invalidateCache();
}

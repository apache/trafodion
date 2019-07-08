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
 * File:         Globals.cpp
 * Description:  CLI globals. For each process that uses the CLI there
 *               should be exactly one object of type CliGlobals.
 *               
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#include "Platform.h"


#include <stdlib.h>
#include <sys/syscall.h>
#include "cli_stdh.h"
#include "Ipc.h"
#include "ex_stdh.h"
#include "ex_frag_rt.h"
#include "memorymonitor.h"
#include "ExStats.h"
#include "ExUdrServer.h"
#include "ExSqlComp.h"
#include "ExControlArea.h"
#include "Context.h"
#include "ex_transaction.h"
#include "Statement.h"
#include "ex_root.h"
#include "ComRtUtils.h"
#include <semaphore.h>
#include <pthread.h>
#include "HBaseClient_JNI.h"
#include "HdfsClient_JNI.h"
#include "HiveClient_JNI.h"
#include "LmLangManagerC.h"
#include "LmLangManagerJava.h"
#include "CliSemaphore.h"

#include "ExCextdecs.h"
CliGlobals * cli_globals = NULL;
__thread ContextTidMap *tsCurrentContextMap = NULL;

CLISemaphore globalSemaphore ;

#include "CmpContext.h"

CliGlobals::CliGlobals(NABoolean espProcess)
     : inConstructor_(TRUE),
       executorMemory_((const char *)"Global Executor Memory"),
       contextList_(NULL),
       envvars_(NULL),
       envvarsContext_(0),
       sharedArkcmp_(NULL),
       arkcmpInitFailed_(arkcmpIS_OK_),
       processIsStopping_(FALSE),
       totalCliCalls_(0),
       savedCompilerVersion_ (COM_VERS_COMPILER_VERSION),
       globalSbbCount_(0),
       priorityChanged_(FALSE),
       currRootTcb_(NULL),
       processStats_(NULL),
       savedPriority_(148), // Set it to some valid priority to start with
       tidList_(NULL),
       cliSemaphore_(NULL),
       defaultContext_(NULL),
       langManC_(NULL),
       langManJava_(NULL)
       , myVerifier_(-1)
       , espProcess_(espProcess)
       , hbaseClientJNI_(NULL)
{
  globalsAreInitialized_ = FALSE;
  executorMemory_.setThreadSafe();
  init(espProcess, NULL);
  globalsAreInitialized_ = TRUE;
}

void CliGlobals::init( NABoolean espProcess,
                       StatsGlobals *statsGlobals
                       )
{
  int threadCount = NAAssertMutexCreate();
  if (threadCount != 1) // The main executor thread must be first
    abort();
  SQLMXLoggingArea::init();

#if !(defined(__SSCP) || defined(__SSMP))
  sharedCtrl_ = new(&executorMemory_) ExControlArea(NULL /*context */,
						    &executorMemory_);
#else
  sharedCtrl_ = NULL;
#endif

  char *_sqptr = 0;
  _sqptr = new (&executorMemory_) char[10];

  numCliCalls_ = 0;
  nodeName_[0] = '\0';

  breakEnabled_ = FALSE;
  
  SPBreakReceived_ = FALSE;
  isESPProcess_ = FALSE;
 
  logReclaimEventDone_ = FALSE;

  // find and initialize the directory this program is being run from.
  // Max length of oss dirname is 1K (process_getinfolist_ limit).
  // Also initialize the node, cpu and pin my process is running at.
  
  programDir_ = new (&executorMemory_) char[1024 + 1];
  short nodeNameLen;
  Lng32 retcomrt = 0;
  retcomrt =  ComRtGetProgramInfo(programDir_, 1024, processType_,
				  myCpu_, myPin_, 
				  myNodeNumber_, myNodeName_, nodeNameLen, 
				  myStartTime_, myProcessNameString_,
				  parentProcessNameString_
                                  , &myVerifier_
                                 );

  if (retcomrt)
  {
    char errStr[128];
    sprintf (errStr, "Could not initialize CLI globals.ComRtGetProgramInfo returned an error :%d.", retcomrt);
    ex_assert(0,errStr);
  }

  ComRtGetProcessPriority(myPriority_);
  savedPriority_ = (short)myPriority_;
  myNumCpus_ = ComRtGetCPUArray(cpuArray_, (NAHeap *)&executorMemory_);

  // create global structures for IPC environment
#if !(defined(__SSCP) || defined(__SSMP))

  ipcHeap_ = new(&executorMemory_) NAHeap("IPC Heap",
                                     NAMemory::IPC_MEMORY, 2048 * 1024);
    ipcHeap_->setThreadSafe();
  if (! espProcess)
  {
    // Create the process global ARKCMP server.
    sharedArkcmp_ = NULL;
    nextUniqueContextHandle = DEFAULT_CONTEXT_HANDLE;
    lastUniqueNumber_ = 0;
    sessionUniqueNumber_ = 0;
    // It is not thread safe to set the globals cli_globals
    // before cli_globals is fully initialized, but it is being done
    // here because the code below expects it 
    cli_globals = this;
    int error;
    statsGlobals_ = (StatsGlobals *)shareStatsSegment(shmId_);
    NABoolean reportError = FALSE;
    char msg[256];;
    if ((statsGlobals_ == NULL)
      || ((statsGlobals_ != NULL) && (statsGlobals_->getInitError(myPin_, reportError))))
    {
      if (reportError) {
         snprintf(msg, sizeof(msg),
          "Version mismatch or Pid %d,%d is higher than the configured pid max %d",
           myCpu_, myPin_, statsGlobals_->getConfiguredPidMax());
         SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, msg, 0);
      }
      statsGlobals_ = NULL;
      statsHeap_ = new (getExecutorMemory()) 
        NAHeap("Process Stats Heap", getExecutorMemory(),
	       8192,
	       0);
      statsHeap_->setThreadSafe();
    }
    else
    {
      error = statsGlobals_->openStatsSemaphore(semId_);
      // Behave like as if stats is not available
      if (error != 0)
      {
	statsGlobals_ = NULL;
	statsHeap_ = getExecutorMemory();
      }
      else
      {
        bool reincarnated;
        error = statsGlobals_->getStatsSemaphore(semId_, myPin_);

        statsHeap_ = (NAHeap *)statsGlobals_->
               getStatsHeap()->allocateHeapMemory(sizeof *statsHeap_, FALSE);

        // The following assertion may be hit if the RTS shared memory
        // segment is full.  The stop catcher code will be responsible
        // for releasing the RTS semaphore.
        ex_assert(statsHeap_, "allocateHeapMemory returned NULL.");

        // This next allocation, a placement "new" will not fail.
	statsHeap_ = new (statsHeap_, statsGlobals_->getStatsHeap()) 
	  NAHeap("Process Stats Heap", statsGlobals_->getStatsHeap(),
		 8192,
		 0);
	reincarnated = statsGlobals_->addProcess(myPin_, statsHeap_);
        processStats_ = statsGlobals_->getExProcessStats(myPin_);
        processStats_->setStartTime(myStartTime_);
	statsGlobals_->releaseStatsSemaphore(semId_, myPin_);
        if (reincarnated)
           statsGlobals_->logProcessDeath(myCpu_, myPin_, "Process reincarnated before RIP");
      }
    }
    // create a default context and make it the current context
    cliSemaphore_ = new (&executorMemory_) CLISemaphore();
    defaultContext_ = new (&executorMemory_) ContextCli(this);
    contextList_  = new(&executorMemory_) HashQueue(&executorMemory_);
    tidList_  = new(&executorMemory_) HashQueue(&executorMemory_);
    SQLCTX_HANDLE ch = defaultContext_->getContextHandle();
    contextList_->insert((char*)&ch, sizeof(SQLCTX_HANDLE), (void*)defaultContext_);
    if (statsGlobals_ != NULL) 
       memMonitor_ = statsGlobals_->getMemoryMonitor();
    else
    {
       // create the process global memory monitor. For now with
       // defaults of 10 window entries and sampling every 1 second
       Lng32 memMonitorWindowSize = 10;
       Lng32 memMonitorSampleInterval = 1; // reduced from 10 (for M5 - May 2011)
       memMonitor_ = new (&executorMemory_) MemoryMonitor(memMonitorWindowSize,
						      memMonitorSampleInterval,
    						      &executorMemory_);
    }
  } // (!espProcess)

  else
  {
    // For ESPs do not create the default context here. At this point
    // the ESP has not created an IpcEnvironment object yet so the
    // result context will have an invalid ExSqlComp object that
    // points to a NULL IpcEnvironment. In bin/ex_esp_main.cpp, the
    // following objects are created at ESP startup time:
    //   - CliGlobals
    //   - IpcEnvironment
    //   - Default ContextCli in CliGlobals
    //   - MemoryMonitor
    //   - ExEspFragInstanceDir
    //   - ExEspControl Message
    //   - Global UDR server manager
    cliSemaphore_ = new (&executorMemory_) CLISemaphore();
    statsGlobals_ = NULL;
    semId_ = -1;
    statsHeap_ = NULL;
    lastUniqueNumber_ = 0;

  } // if (!espProcess) else ...

#else // (defined(__SSCP) || defined(__SSMP)) 
  cliSemaphore_ = new (&executorMemory_) CLISemaphore();
  statsGlobals_ = statsGlobals;
  semId_ = -1;
  statsHeap_ = NULL;
  lastUniqueNumber_ = 0;
#endif 

  inConstructor_ = FALSE;
  //
  // could initialize the program file name here but ...
  myProgName_[0] = '\0';
}
CliGlobals::~CliGlobals()
{
  arkcmpInitFailed_ = arkcmpERROR_; // (it's corrupt after deleting, anyway...)
  short error ;

  if (sharedArkcmp_)
  {
    delete sharedArkcmp_;
    sharedArkcmp_ = NULL;
  }
  if (statsGlobals_ != NULL)
  {
    error = statsGlobals_->getStatsSemaphore(semId_, myPin_);
    statsGlobals_->removeProcess(myPin_);
    statsGlobals_->releaseStatsSemaphore(semId_, myPin_);
    statsGlobals_->logProcessDeath(myCpu_, myPin_, "Normal process death");
    sem_close((sem_t *)semId_);
  }
}

Lng32 CliGlobals::getNextUniqueContextHandle()
{
    Lng32 contextHandle;
    cliSemaphore_->get();
    contextHandle = nextUniqueContextHandle++;
    cliSemaphore_->release();
    return contextHandle;
}

IpcPriority CliGlobals::myCurrentPriority()
{
  IpcPriority myPriority;
  
  Lng32 retcode = ComRtGetProcessPriority(myPriority);
  if (retcode)
    return -2;

  return myPriority;
}


// NOTE: Unlike REFPARAM_BOUNDSCHECK, this method does not verify that
//       the pointer "startAddress" actually points to a valid address
//       in the user address space (means that dereferencing the
//       pointer may cause a segmentation violation).
//
// Here are some DEFINEs from files DMEM and JMEMH in product T9050 that
// perform the PRIV address check:
// LOG2_BYTES_IN_T16PAGE      = 11,    ! 2048 bytes in a T16 page size
// LOG2_T16PAGES_IN_SEGMENT   =  6,    ! 64 T16 pages in a segment
// LOG2_BYTES_IN_SEGMENT      = LOG2_BYTES_IN_T16PAGE +
//                              LOG2_T16PAGES_IN_SEGMENT,
// SYSTEMDATASEG  = 1,  !  System Data Segment (always absolute segment 1)
//
// #define ADDR_IS_IN_KSEG0_1_2( a )         \
//    ( (int32) (a) < 0 )
//
// #define ADDR_IS_PRIV(a) (ADDR_IS_IN_KSEG0_1_2(a)               \
//              || ((vaddr_t)(a) >> LOG2_BYTES_IN_SEGMENT) == SYSTEMDATASEG)
//
// NOTE: we'll need to recompile if the NSK architecture changes, which
// should be infrequent as Charles Landau assures me.

Lng32 CliGlobals::boundsCheck(void          *startAddress,
			     ULng32 length,
			     Lng32          &retcode)
{
  // no bounds checking on NT because we're not PRIV
  return 0;
}

NAHeap *CliGlobals::getIpcHeap()
{
   return currContext()->getIpcHeap();
}

IpcEnvironment *CliGlobals::getEnvironment()
{
   ContextCli *currentContext = currContext();
   if (currentContext != NULL)
      return currentContext->getEnvironment();
   else
      return NULL;
}

ExEspManager *CliGlobals::getEspManager()
{
  return currContext()->getEspManager();
}

ExSsmpManager *CliGlobals::getSsmpManager()
{
  return currContext()->getSsmpManager();
}

LmLanguageManager * CliGlobals::getLanguageManager(ComRoutineLanguage language)
{
  switch (language)
    {
    case COM_LANGUAGE_JAVA:
      return getLanguageManagerJava();
      break;
    case COM_LANGUAGE_C:
    case COM_LANGUAGE_CPP:
      return getLanguageManagerC();
      break;
    default:
      ex_assert(0, "Invalid language in CliGlobals::getLanguageManager()");
    }
  return NULL;
}

LmLanguageManagerC * CliGlobals::getLanguageManagerC()
{
  if (!langManC_)
    {
      LmResult result;

      langManC_ = new(&executorMemory_)
        LmLanguageManagerC(result,
                           FALSE,
                           &(currContext()->diags()));

      if (result != LM_OK)
        {
          delete langManC_;
          langManC_ = NULL;
        }
    }

  return langManC_;
}

LmLanguageManagerJava * CliGlobals::getLanguageManagerJava()
{
  if (!langManJava_)
    {
      LmResult result;

      langManJava_ = new(&executorMemory_)
        LmLanguageManagerJava(result,
                              FALSE,
                              1,
                              NULL, // Java options should have been
                                    // provided for earlier JNI calls
                                    // in Trafodion
                              &(currContext()->diags()));

      if (result != LM_OK)
        {
          delete langManJava_;
          langManJava_ = NULL;
        }
    }

  return langManJava_;
}

ExeTraceInfo *CliGlobals::getExeTraceInfo()
{
  return currContext()->getExeTraceInfo();
}

ExSqlComp * CliGlobals::getArkcmp(short index)
{
  //return sharedArkcmp_;
  return currContext()->getArkcmp(index);
}

CliGlobals * CliGlobals::createCliGlobals(NABoolean espProcess)
{
  CliGlobals *result;

  result =  new CliGlobals(espProcess);
  //pthread_key_create(&thread_key, SQ_CleanupThread);
  cli_globals = result;
  HBaseClient_JNI::getInstance();
  return result;
}

void * CliGlobals::getSegmentStartAddrOnNSK()
{

  // this method should only be called on NSK, return NULL on other platforms
  return NULL;

}

CliGlobals *GetCliGlobals()
{ return cli_globals; }


// used by ESP only
void CliGlobals::initiateDefaultContext()
{
  // create a default context and make it the current context
  defaultContext_ = new (&executorMemory_) ContextCli(this);
  contextList_  = new(&executorMemory_) HashQueue(&executorMemory_);
  tidList_  = new(&executorMemory_) HashQueue(&executorMemory_);
  cliSemaphore_ = new (&executorMemory_) CLISemaphore();
  SQLCTX_HANDLE ch = defaultContext_->getContextHandle();
  contextList_->insert((char*)&ch, sizeof(SQLCTX_HANDLE),
                       (void*)defaultContext_);
}

ContextCli *CliGlobals::currContext()
{
  if (tsCurrentContextMap == NULL ||
               tsCurrentContextMap->context_ == NULL)
     return defaultContext_; 
  else 
     return tsCurrentContextMap->context_; 
}


Lng32 CliGlobals::createContext(ContextCli* &newContext)
{
  newContext = new (&executorMemory_) ContextCli(this);
  SQLCTX_HANDLE ch = newContext->getContextHandle();
  cliSemaphore_->get();
  contextList_->insert((char*)&ch, sizeof(SQLCTX_HANDLE), (void*)newContext);
  cliSemaphore_->release();
   
  return 0;
}
Lng32 CliGlobals::dropContext(ContextCli* context)
{
  if (!context)
    return -1;

  if (context == getDefaultContext())
      return 0;

  CLISemaphore *tmpSemaphore = context->getSemaphore();
  tmpSemaphore->get();
  try
  {
     context->deleteMe();
  }
  catch (...)
  {
     tmpSemaphore->release();
     return -1;
  }
  tmpSemaphore->release();
  pid_t tid = syscall(SYS_gettid);
  cliSemaphore_->get();
  contextList_->remove((void*)context);
  tidList_->position();
  ContextTidMap *contextTidMap;
  while ((contextTidMap = (ContextTidMap *)tidList_->getNext()) != NULL)
  {
     if (contextTidMap->context_ == context)
     {
        if (contextTidMap->tid_  == tid)
        {
           tidList_->remove((char*)&contextTidMap->tid_, sizeof(pid_t), 
                          contextTidMap);
           NADELETE(contextTidMap, ContextTidMap, getExecutorMemory());
           tsCurrentContextMap = NULL; 
        }
        else
           contextTidMap->context_ = NULL;
     }
  } 
  delete context;
  cliSemaphore_->release();
  return 0;
}

ContextCli * CliGlobals::getContext(SQLCTX_HANDLE context_handle, 
                                    NABoolean calledFromDrop)
{
  ContextCli * context;
  cliSemaphore_->get();
  contextList_->position((char*)&context_handle, sizeof(SQLCTX_HANDLE));
  while ((context = (ContextCli *)contextList_->getNext()) != NULL)
  {
     if (context_handle == context->getContextHandle())
     {
        if (context->isDropInProgress())
           context = NULL;
        else if (calledFromDrop)
           context->setDropInProgress();
        cliSemaphore_->release();
        return context;
     }
  }
  cliSemaphore_->release();
  return NULL;
}

ContextTidMap * CliGlobals::getThreadContext(pid_t tid)
{
  SQLCTX_HANDLE ch;
  ContextTidMap *contextTidMap;

  if (tidList_ == NULL)
     return NULL;
  cliSemaphore_->get();
  tidList_->position((char*)&tid, sizeof(pid_t));
  while ((contextTidMap = (ContextTidMap *)tidList_->getNext()) != NULL)
  {
      if (contextTidMap->tid_ == tid)
      {
         if (contextTidMap->context_ == NULL)
         {
             NADELETE(contextTidMap, ContextTidMap, getExecutorMemory());
             tidList_->remove((char*)&tid, sizeof(pid_t), contextTidMap);
             contextTidMap = NULL;
         }
         cliSemaphore_->release();
         return contextTidMap;
      }
  }
  cliSemaphore_->release();
  return NULL;
}

Lng32 CliGlobals::switchContext(ContextCli * newContext)
{
  Lng32 retcode;

  pid_t tid;
  SQLCTX_HANDLE ch, currCh;

  tid = syscall(SYS_gettid);
  if (newContext != defaultContext_  && 
        tsCurrentContextMap != NULL && 
           newContext == tsCurrentContextMap->context_)
     return 0;
  if (newContext == defaultContext_)
    if (tsCurrentContextMap)
      tsCurrentContextMap->context_ = NULL;
  retcode = currContext()->getTransaction()->suspendTransaction();
  if (retcode != 0)
     return retcode;
  cliSemaphore_->get();
  tidList_->position((char*)&tid, sizeof(pid_t));
  ContextTidMap *contextTidMap;
  NABoolean tidFound = FALSE;
  while ((contextTidMap = (ContextTidMap *)tidList_->getNext()) != NULL)
  {
     if (tid == contextTidMap->tid_)
     {
        contextTidMap->context_ = newContext;
        tidFound = TRUE;
        tsCurrentContextMap = contextTidMap;
        break;
     }
  } 
  if (! tidFound)
  {
     contextTidMap = new  (getExecutorMemory()) ContextTidMap(tid, newContext);
     tidList_->insert((char *)&tid, sizeof(pid_t), (void *)contextTidMap);
     tsCurrentContextMap = contextTidMap;
  }
  cliSemaphore_->release();
  retcode = currContext()->getTransaction()->resumeTransaction();
  return retcode;
}

Lng32 CliGlobals::sendEnvironToMxcmp()
{
  ComDiagsArea & diags = currContext()->diags();
  
  if (NOT getArkcmp()->isConnected())
    return 0;
  
  // send the current environment to mxcmp
  ExSqlComp::ReturnStatus sendStatus = 
    getArkcmp()->sendRequest(CmpMessageObj::ENVS_REFRESH, NULL,0);
  if (sendStatus != ExSqlComp::SUCCESS)
    {
      if (sendStatus == ExSqlComp::ERROR)
	{
	  diags << DgSqlCode(-CLI_SEND_REQUEST_ERROR) 
		<< DgString0("SET ENVIRON");
	  return -CLI_SEND_REQUEST_ERROR;
	  //	  return SQLCLI_ReturnCode(&currContext,-CLI_SEND_REQUEST_ERROR);
	}
      //else
      //  retcode = WARNING;
    }
  
  if (getArkcmp()->status() != ExSqlComp::FINISHED)
    {
      diags << DgSqlCode(-CLI_IO_REQUESTS_PENDING)
	    << DgString0("SET ENVIRON");
      return -CLI_IO_REQUESTS_PENDING;
      //return SQLCLI_ReturnCode(&currContext,-CLI_IO_REQUESTS_PENDING); 
    }
  
  return 0;
}

Lng32 CliGlobals::setEnvVars(char ** envvars)
{
  if ((! envvars) || (isESPProcess_))
    return 0;

  Int32 nEnvs = 0;
  if (envvars_)
    {
      // deallocate the current set of envvars
      ipcHeap_->deallocateMemory(envvars_);
      envvars_ = NULL;
    }

  for (nEnvs=0; envvars[nEnvs]; nEnvs++);

  // one extra to null terminate envvar list
  Lng32 envvarsLen = (nEnvs + 1) * sizeof(char*);

  Int32 count;
  for (count=0; count < nEnvs; count++) 
    { 
      envvarsLen += str_len(envvars[count])+1;
    } 

  // allocate contiguous space for envvars
  envvars_ = (char**)(new(ipcHeap_) char[envvarsLen]);

  char * envvarsValue = (char *)envvars_ + (nEnvs + 1) * sizeof(char*);

  // and copy input envvars to envvars_
  for (count=0; count < nEnvs; count++) 
    { 
      envvars_[count] = envvarsValue;
      Lng32 l = str_len(envvars[count])+1;
      str_cpy_all(envvarsValue, envvars[count], l);
      envvarsValue = envvarsValue + l;
    } 

  envvars_[nEnvs] = 0;

  // also set it in IpcEnvironment so it could be used to send
  // it to mxcmp.
  getEnvironment()->setEnvVars(envvars_);
  getEnvironment()->setEnvVarsLen(envvarsLen);

  envvarsContext_++;

  return sendEnvironToMxcmp();
}

Lng32 CliGlobals::setEnvVar(const char * name, const char * value,
			   NABoolean reset)
{
  if ((! name) || (! value) || (isESPProcess_))
    return 0;

  NABoolean found = FALSE;
  Lng32 envvarPos = -1;
  if (ComRtGetEnvValueFromEnvvars((const char**)envvars_, name, &envvarPos))
    found = TRUE;

  if ((NOT found) && (reset))
    return 0;

  Int32 nEnvs = 0;
  if (envvars_)
    {
      for (nEnvs=0; envvars_[nEnvs]; nEnvs++);
    }

  if (reset)
    {
      //      nEnvs--;
    }
  else if (NOT found)
    {
      envvarPos = nEnvs;
      nEnvs++;
    }

  // one extra entry, if envvar not found.
  // one extra to null terminate envvar list.
  //  long envvarsLen = (nEnvs + (NOT found ? 1 : 0) + 1) * sizeof(char*);
  Lng32 newEnvvarsLen = (nEnvs + 1) * sizeof(char*);

  Int32 count;
  for (count=0; count < nEnvs; count++) 
    { 
      if (count == envvarPos)
	//      if ((found) && (count == envvarPos))
	{
	  if (NOT reset)
	    newEnvvarsLen += strlen(name) + strlen("=") + strlen(value) + 1;
	}
      else if (NULL != envvars_)
	newEnvvarsLen += str_len(envvars_[count])+1;
    } 
  
  /*  if (NOT found)
    {
      nEnvs++;
      newEnvvarsLen += strlen(name) + strlen("=") + strlen(value) + 1;
    }
    */

  // allocate contiguous space for envvars
  char ** newEnvvars = (char**)(new(ipcHeap_) char[newEnvvarsLen]);

  char * newEnvvarsValue = 
    (char*)(newEnvvars + 
	    ((reset ? (nEnvs-1) : nEnvs) + 1));

  // and copy envvars_ to newEnvvars
  Int32 tgtCount = 0;
  for (count=0; count < nEnvs; count++) 
    { 
      newEnvvars[tgtCount] = newEnvvarsValue;
      Lng32 l = 0;
      if (count == envvarPos)
	{
	  if (NOT reset)
	    {
	      strcpy(newEnvvarsValue, name);
	      strcat(newEnvvarsValue, "=");
	      strcat(newEnvvarsValue, value);
	      l = strlen(name) + strlen("=") + strlen(value) + 1;

	      tgtCount++;
	    }
	}
      else if (NULL != envvars_)
	{
	  l = str_len(envvars_[count])+1;
	  str_cpy_all(newEnvvarsValue, envvars_[count], l);
	  tgtCount++;
	}
      newEnvvarsValue = newEnvvarsValue + l;
    } 

  if (reset)
    {
      nEnvs--;
    }

  newEnvvars[nEnvs] = 0;

  if (envvars_)
    {
      // deallocate the current set of envvars
      ipcHeap_->deallocateMemory(envvars_);
      envvars_ = NULL;
    }
  envvars_ = newEnvvars;

  // set or reset this envvar in SessionDefaults.
  SessionEnvvar * se = 
    new(ipcHeap_) SessionEnvvar(ipcHeap_, (char*)name, (char*)value);

  // remove if an entry exists
  currContext()->getSessionDefaults()->sessionEnvvars()->remove(*se);
  
  // insert a new entry, if this is not a RESET operation.
  if (NOT reset)
    {
      currContext()->getSessionDefaults()->sessionEnvvars()->insert(*se);
    }

  delete se;

  // also set it in IpcEnvironment so it could be used to send
  // it to mxcmp.
  getEnvironment()->setEnvVars(envvars_);
  getEnvironment()->setEnvVarsLen(newEnvvarsLen);

  envvarsContext_++;

  // need to set the env to the embedded compiler too
  if (currContext()->isEmbeddedArkcmpInitialized())
    {
      currContext()->getEmbeddedArkcmpContext()
                   ->setArkcmpEnvDirect(name, value, reset);
    }

  return sendEnvironToMxcmp();
}

char * CliGlobals::getEnv(const char * name)
{
  return (char*)ComRtGetEnvValueFromEnvvars((const char**)envvars_, name);
}
//
Lng32 CliGlobals::resetContext(ContextCli *theContext, void *contextMsg)
{
    theContext->reset(contextMsg);  
    return SUCCESS;
}

NAHeap *CliGlobals::getCurrContextHeap()
{
   return currContext()->exHeap();
}


ExUdrServerManager *CliGlobals::getUdrServerManager()
{ return currContext()->getUdrServerManager(); }

NABoolean CliGlobals::getUdrErrorChecksEnabled() 
{ return currContext()->getUdrErrorChecksEnabled(); }

Lng32 CliGlobals::getUdrSQLAccessMode() 
{ return currContext()->getUdrSQLAccessMode(); }

NABoolean CliGlobals::getUdrAccessModeViolation() 
{ return currContext()->getUdrAccessModeViolation(); }

NABoolean CliGlobals::getUdrXactViolation() 
{ return currContext()->getUdrXactViolation(); }

NABoolean CliGlobals::getUdrXactAborted() 
{ return currContext()->getUdrXactAborted(); }

void CliGlobals::setUdrErrorChecksEnabled(NABoolean b) 
{ currContext()->setUdrErrorChecksEnabled(b); }

void CliGlobals::setUdrSQLAccessMode(Lng32 mode) 
{ currContext()->setUdrSQLAccessMode(mode); }

void CliGlobals::setUdrAccessModeViolation(NABoolean b) 
{ currContext()->setUdrAccessModeViolation(b); }

void CliGlobals::setUdrXactViolation(NABoolean b)
{ currContext()->setUdrXactViolation(b); }

void CliGlobals::setUdrXactAborted(Int64 currTransId, NABoolean b)
{ currContext()->setUdrXactAborted(currTransId, b); }

void CliGlobals::clearUdrErrorFlags()
{ currContext()->clearUdrErrorFlags(); }

NABoolean CliGlobals::sqlAccessAllowed() 
{ return currContext()->sqlAccessAllowed(); }

void CliGlobals::getUdrErrorFlags(NABoolean &sqlViolation,
                         NABoolean &xactViolation,
                        NABoolean &xactAborted) 
{
    currContext()->getUdrErrorFlags(sqlViolation, xactViolation,
                                  xactAborted);
}

void CliGlobals::updateTransMode(TransMode *transMode)
{
  currContext()->getTransaction()->getTransMode()->
           updateTransMode(transMode);
}


void CliGlobals::initMyProgName()
{
  char    statusFileName[128];
  FILE    *status_file = 0;
  size_t  bytesRead, bytesCopy;
  char    buf[1024];
  char    *beginPtr, *endPtr;

  sprintf(statusFileName, "/proc/%d/status",getpid());
  status_file = fopen(statusFileName, "r");
  if (status_file == NULL)
    return;  //ignore error and return

  buf[0] = 0;
  if (fseek(status_file, 0, SEEK_SET))
  {
    fclose(status_file);
    return;  //ignore error and return
  }
  bytesRead = fread(buf, 1, 1024, status_file);
  if (ferror(status_file))
  {
    fclose(status_file);
    return;  //ignore error and return
  }
  fclose(status_file);
  beginPtr = strstr(buf, "Name:\t");
  if (!beginPtr)
    return;  //not found, return
  beginPtr += 6;
  endPtr = strstr(beginPtr, "\n");
  if (!endPtr || beginPtr == endPtr)
    return;  //no name found, return
  // myProgName_ is 64 bytes long, see Globals.h
  bytesCopy = (endPtr - beginPtr) < PROGRAM_NAME_LEN ?
               (endPtr - beginPtr) : PROGRAM_NAME_LEN - 1;
  memcpy(myProgName_, beginPtr, bytesCopy);
  *(myProgName_ + bytesCopy) = '\0';  // null terminates
}

#ifdef _DEBUG
// Delete the default context and all associated embedded CMP contexts
// This eventually causes dumping of all heap debug info if enabled
// Should ONLY be called right before process exits
void CliGlobals::deleteContexts()
{
  if (defaultContext_)
    {
      defaultContext_->deleteMe();
      delete defaultContext_;
      defaultContext_ = NULL;
    }
}
#endif  // _DEBUG

// The unused BMO memory quota can now be utilized by the other
// BMO instances from the same or different fragment
// In case of ESP process, the unused memory quota is maintained
// at the default context. In case of master process, the ununsed
// memory quota is maintained in statement globals

NABoolean CliGlobals::grabMemoryQuotaIfAvailable(ULng32 size)
{
  ContextCli *context;
  if (espProcess_)
     context = defaultContext_;
  else
     context = currContext();
  return context->grabMemoryQuotaIfAvailable(size);
}

void CliGlobals::resetMemoryQuota() 
{
  ContextCli *context;
  if (espProcess_)
     context = defaultContext_;
  else
     context = currContext();
  return context->resetMemoryQuota();
}

ULng32 CliGlobals::unusedMemoryQuota() 
{ 
  ContextCli *context;
  if (espProcess_)
     context = defaultContext_;
  else
     context = currContext();
  return context->unusedMemoryQuota();
}

void CliGlobals::yieldMemoryQuota(ULng32 size)
{
  ContextCli *context;
  if (espProcess_)
     context = defaultContext_;
  else
     context = currContext();
  return context->yieldMemoryQuota(size);
}

void SQ_CleanupThread(void *arg)
{
  HBaseClient_JNI::deleteInstance();
  HiveClient_JNI::deleteInstance();
}



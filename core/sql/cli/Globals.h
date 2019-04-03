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
#ifndef GLOBALS_H
#define GLOBALS_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         Globals.h
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

/* this file is also sourced in from a C program, cli/fixup.c */
#include "NAAssert.h"
/* -----------------------------------------------------------------------
 * Some parameters for the first privileged NSK flat segment used by CLI
 * and executor (NOTE: this part can be sourced in from a C program)
 * -----------------------------------------------------------------------
 */
#define NA_CLI_FIRST_PRIV_SEG_ID            NAASSERT_FIRST_PRIV_SEG_ID
#define NA_CLI_FIRST_PRIV_SEG_SIZE          4*1024*1024
#define NA_CLI_FIRST_PRIV_SEG_MAX_SIZE      64*1024*1024
/* The removal of spoofing has made these two #defines history
#define NA_CLI_GLOBALS_OFFSET_IN_PRIV_SEG   NAASSERT_GLOBALS_OFFSET_IN_PRIV_SEG
#define NA_CLI_FIRST_PRIV_SEG_START_ADDR    0x42000000
 */
#define BYTEALIGN 16  // 16-byte align entire first flat seg on all platforms
#define NA_CLI_GLOBALS_SIZE \
          (((sizeof(CliGlobals)+BYTEALIGN-1)/BYTEALIGN)*BYTEALIGN)

#ifdef __cplusplus

// -----------------------------------------------------------------------
#include <setjmp.h>

#include "NAMemory.h"
#include "sqlcli.h"
#include "Ipc.h"
#include "ComQueue.h"
#include "logmxevent.h"
#include "ComExeTrace.h"
#include "ComRtUtils.h"
#include "ComSmallDefs.h"
#include "JavaObjectInterface.h"

class ContextCli;
class Statement;  
class ComDiagsArea; 
class ExEspManager;
class ExSsmpManager;
class ExSqlComp;
class IpcEnvironment;
class MemoryMonitor;
class HashQueue;
class ExUdrServerManager;
class ExControlArea;
class StatsGlobals;
class ex_tcb; // for keeping the root (split bottom, in ESP) tcb 
class ExProcessStats;
class CliGlobals;
class CLISemaphore;
class HBaseClient_JNI;
class HiveClient_JNI;
class HdfsClient;
class TransMode;
class ContextTidMap;
class LmLanguageManager;
class LmLanguageManagerC;
class LmLanguageManagerJava;
extern CliGlobals *cli_globals;
extern __thread ContextTidMap *tsCurrentContextMap;
static  pthread_key_t thread_key;

// A cleanup function when thread exits
void SQ_CleanupThread(void *arg);

enum ArkcmpFailMode { arkcmpIS_OK_ = FALSE/*no failure*/,
arkcmpWARN_,
arkcmpERROR_ };

class CliGlobals : public NAAssertGlobals
{
public:
  Lng32 getNextUniqueContextHandle();
     
  ExControlArea * getSharedControl() { return sharedCtrl_; }
  CollHeap * exCollHeap() { return &executorMemory_; }

  // old interface, changed to the get/set methods below
  ArkcmpFailMode & arkcmpInitFailed()
	{ return arkcmpInitFailed_; }

  ArkcmpFailMode getArkcmpInitFailMode() 
  { return arkcmpInitFailed_; }

  void setArkcmpInitFailMode(ArkcmpFailMode arkcmpFailMode)
  {
    arkcmpInitFailed_ = arkcmpFailMode;
  }
  
  void deleteAndCreateNewArkcmp();

  void  init( NABoolean espProcess,
             StatsGlobals *statsGlobals);
 
  CliGlobals(NABoolean espProcess);

  ~CliGlobals();

  void initiateDefaultContext();

  ContextCli * currContext();

  ContextCli * getDefaultContext() { return defaultContext_; }

  inline ExProcessStats *getExProcessStats() { return processStats_; }
  void setExProcessStats(ExProcessStats *processStats)
  { processStats_ = processStats; }

  ExSqlComp * getArkcmp(short index = 0);

  IpcEnvironment *getEnvironment();
  char ** getEnvVars()                          { return envvars_; } ;
  char * getEnv(const char * envvar);
  Lng32 setEnvVars(char ** envvars);
  Lng32 setEnvVar(const char * name, const char * value,
		 NABoolean reset = FALSE);
  Lng32 sendEnvironToMxcmp();

  ExEspManager * getEspManager();
  ExUdrServerManager *getUdrServerManager();
  inline MemoryMonitor * getMemoryMonitor()     { return memMonitor_; }
  inline void setMemoryMonitor(MemoryMonitor *memMon) { memMonitor_ = memMon; }

  inline NAHeap * getExecutorMemory()      { return &executorMemory_; }
  inline void setNodeName(const char *nodeName)
                    { strncpy(nodeName_, nodeName, sizeof(nodeName_)); }
  inline char *getNodeName() { return nodeName_; }

  inline Lng32 incrNumOfCliCalls()                   { return ++numCliCalls_; }
  inline Lng32 decrNumOfCliCalls()                   
  { 
     if (numCliCalls_ > 0)
        return --numCliCalls_;
     else
        return numCliCalls_;
  }
  inline Int64 incrTotalOfCliCalls()               
  { 
    return ++totalCliCalls_; 
  }

  inline UInt32 * getEventConsumed()   { return &eventConsumed_; }
  inline NABoolean processIsStopping() { return processIsStopping_; }
  
  // create the CLI globals (caller may determine their address)
  static CliGlobals * createCliGlobals(NABoolean espProcess = FALSE);
  static void * getSegmentStartAddrOnNSK();

  // perform a bounds check for a parameter passed into the CLI, note that
  // our bounds check does not prevent access violations if the address
  // points into unallocated memory that doesn't violate the bounds
  // (note that the method always returns a SQLCODE value, AND that it
  // side-effects retcode in case of an error)
  Lng32 boundsCheck(void          *startAddress,
		   ULng32 length,
		   Lng32          &retcode);

  inline NABoolean breakEnabled() { return breakEnabled_; }
  inline void setBreakEnabled(NABoolean enabled) 
               { breakEnabled_ = enabled; 
	         getEnvironment()->setBreakEnabled(enabled);
	       }
  
  inline NABoolean SPBreakReceived() { return SPBreakReceived_; }
  inline void setSPBreakReceived(NABoolean val)
               { SPBreakReceived_ = val; }
 
  inline NABoolean isESPProcess() { return isESPProcess_; }
  inline void setIsESPProcess(NABoolean val)
               { isESPProcess_ = val; }

  Lng32 createContext(ContextCli* &newContext);
  Lng32 dropContext(ContextCli* context);
  ContextCli * getContext(SQLCTX_HANDLE context_handle, 
                          NABoolean calledFromDrop = FALSE);
  ContextTidMap * getThreadContext(pid_t tid);
  Lng32 switchContext(ContextCli*newContext);

  //
  // Context management functions added to implement user-defined routines
  //
  // Lng32 deleteContext(SQLCTX_HANDLE contextHandle);
  Lng32 resetContext(ContextCli *context, void *contextMsg);
  inline HashQueue * getContextList() { return contextList_; }

  NAHeap * getIpcHeap();
  NAHeap * getProcessIpcHeap() { return ipcHeap_; }

  StatsGlobals *getStatsGlobals() { return statsGlobals_; }
  void setStatsGlobals(StatsGlobals *statsGlobals) { statsGlobals_ = statsGlobals; }

  NAHeap *getStatsHeap() { return statsHeap_; }
  void setStatsHeap(NAHeap *statsHeap) { statsHeap_ = statsHeap; }

  // EMS event generation functions
  inline void setEMSBeginnerExperienceLevel() 
                   {emsEventExperienceLevel_ = SQLMXLoggingArea::eBeginnerEL;}
  inline SQLMXLoggingArea::ExperienceLevel getEMSEventExperienceLevel() 
                   {return emsEventExperienceLevel_;}
  inline void setUncProcess() { isUncProcess_ = TRUE; }
  inline NABoolean isUncProcess() {return isUncProcess_;}
  NAHeap *getCurrContextHeap();
  void setJniErrorStr(NAString errorStr) { setSqlJniErrorStr(errorStr);  }
  void setJniErrorStr(const char *errorStr)  { setSqlJniErrorStr(errorStr); }
  const char* getJniErrorStr() { return getSqlJniErrorStr(); }
  void updateTransMode(TransMode *transMode);


inline
  short getGlobalSbbCount()
      { return globalSbbCount_; }  
inline
  void incGlobalSbbCount()
      { globalSbbCount_++; }       

inline
  void resetGlobalSbbCount()
      { globalSbbCount_ = 0; }       
  //
  // Accessor and mutator functions for UDR error checking. Notes on
  // UDR error checking appear below with the data member
  // declarations.
  //
  NABoolean getUdrErrorChecksEnabled();
  Lng32 getUdrSQLAccessMode();
  NABoolean getUdrAccessModeViolation();
  NABoolean getUdrXactViolation();
  NABoolean getUdrXactAborted();

  void setUdrErrorChecksEnabled(NABoolean b);
  void setUdrSQLAccessMode(Lng32 mode);
  void setUdrAccessModeViolation(NABoolean b);
  void setUdrXactViolation(NABoolean b);
  void setUdrXactAborted(Int64 currTransId, NABoolean b);

  //
  // A few useful wrapper functions to ease management of the UDR
  // error checking fields
  //
  void clearUdrErrorFlags();

  NABoolean sqlAccessAllowed();

  void getUdrErrorFlags(NABoolean &sqlViolation,
                        NABoolean &xactViolation,
                        NABoolean &xactAborted);
  char * programDir() { return programDir_;};

  // 0, oss process.  1, guardian process.
  short  processType() { return processType_;}; 
  NABoolean ossProcess() { return (processType_ == 0); };
  inline NABoolean logReclaimEventDone() { return logReclaimEventDone_; };
  inline void setLogReclaimEventDone(NABoolean x) { logReclaimEventDone_ = x; };

  char * myNodeName() { return myNodeName_; }
  Int32 myCpu() { return myCpu_; };
  Int32 myPin() { return myPin_; };
  SB_Verif_Type myVerifier() const {return myVerifier_;}

  Lng32 myNodeNumber() { return myNodeNumber_; };
  Int64 myStartTime() { return myStartTime_; };

  // number of CPUs (Linux nodes) in the cluster
  Lng32 myNumCpus() { return myNumCpus_; };
  // an array of myNumCpus_ node ids, the index into
  // this array is a "logical" CPU id (0 ... myNumCpus_-1)
  // and the content of the array is a physical id,
  // a node id (nid) of a node that actually exists
  // (but still may be down)
  const Int32 *myCPUArray() { return cpuArray_; }
  // return the physical node id for a logical id,
  // the physical id is always >= the logical id
  Int32 mapLogicalToPhysicalCPUNum(Int32 ix)
                          { return cpuArray_[ix % myNumCpus_]; }
  // are logical/physical node ids the same?
  NABoolean nodeIdsAreContiguous()
           { return cpuArray_[myNumCpus_-1] == myNumCpus_ - 1; }

  IpcPriority myPriority() { return myPriority_; }
  void setMyPriority(IpcPriority p) { myPriority_= p; }
  NABoolean priorityChanged() { return priorityChanged_;}
  void setPriorityChanged(NABoolean v) { priorityChanged_ = v; }
  IpcPriority myCurrentPriority();

  Int64 getNextUniqueNumber()
  { return ++lastUniqueNumber_; }

  void genSessionUniqueNumber()
  { sessionUniqueNumber_++; }
  Int64 getSessionUniqueNumber()
  { return sessionUniqueNumber_; }

  // returns the current ENVVAR context.
  Int64 getCurrentEnvvarsContext() { return envvarsContext_;};

  void incrCurrentEnvvarsContext() { envvarsContext_++;};
  Long &getSemId() { return semId_; };
  void setSemId(Long semId) { semId_ = semId; }

  void setSavedVersionOfCompiler(short version) { savedCompilerVersion_ = version;}
  short getSavedVersionOfCompiler() { return savedCompilerVersion_;}
  void setSavedSqlTerminateAction(short val) { savedSqlTerminateAction_ = val; }
  short getSavedSqlTerminateAction() { return savedSqlTerminateAction_; }

  void setSavedPriority(short val) { savedPriority_ = val; }
  short getSavedPriority() { return savedPriority_; }
  ExSsmpManager *getSsmpManager();
  NABoolean getIsBeingInitialized() { return inConstructor_; }

  // For debugging (dumps) only -- keep current fragment root tcb
  void setRootTcb( ex_tcb * root ) { currRootTcb_ = root; }
  ex_tcb * getRootTcb() { return currRootTcb_; }
  
  char *myProcessNameString() { return myProcessNameString_; }
  char *myParentProcessNameString() { return parentProcessNameString_;}
  Int32 getSharedMemId() { return shmId_; }
  void setSharedMemId(Int32 shmId) { shmId_ = shmId; }
  
  void initMyProgName();
  char * myProgName() { return myProgName_; }
  ExeTraceInfo * getExeTraceInfo();
  CLISemaphore *getSemaphore() { return cliSemaphore_; }

  // for trusted UDR invocations from executor and compiler
  LmLanguageManager * getLanguageManager(ComRoutineLanguage language);
  LmLanguageManagerC * getLanguageManagerC();
  LmLanguageManagerJava * getLanguageManagerJava();

#ifdef _DEBUG
  void deleteContexts();
#endif  // _DEBUG
  NABoolean grabMemoryQuotaIfAvailable(ULng32 size);
  void resetMemoryQuota();
  ULng32 unusedMemoryQuota();
  void yieldMemoryQuota(ULng32 size);
  NABoolean isEspProcess() { return espProcess_; }

  void setHbaseClient(HBaseClient_JNI *hbaseClientJNI)
     { hbaseClientJNI_ = hbaseClientJNI; }
  HBaseClient_JNI *getHBaseClient() { return hbaseClientJNI_; }

private:
  enum {
    DEFAULT_CONTEXT_HANDLE = 2000
  };

  ex_tcb * currRootTcb_ ; // for keeping the root (split bottom, in ESP) tcb 

  // pointer to the server used to communicate with ARKCMP.
  ExSqlComp * sharedArkcmp_;

  ArkcmpFailMode arkcmpInitFailed_;

  ExControlArea *sharedCtrl_;

  // The globals are placed at a fixed address in the first priv
  // segment allocated by the executor. To find out whether they
  // have been initialized, check this data member which will
  // be set to TRUE by the constructor (it is 0 when the memory
  // is unchanged since allocation of the segment)

  HashQueue * contextList_;


  // this is the the default context for executor.
  // Created on the first call to CLI when CliGlobals
  // is allocated.
  ContextCli * defaultContext_;

  // executor memory that maintains all heap memory for this executor
  NAHeap executorMemory_;

  // heap used by the IPC procedures
  NAHeap * ipcHeap_;
  
  // memory monitor for this process
  MemoryMonitor *memMonitor_;

  // copy of the oss envvars
  char ** envvars_;

  Int64 envvarsContext_;

  // to return a unique SQLCTX_HANDLE on a new ContextCli
  SQLCTX_HANDLE nextUniqueContextHandle; 
  // indicator for Sql_Qfo_IOComp() that a WAIT operation completed on LDONE
  // also contains indicator that IpcSetOfConnections::wait consumed LSIG
  UInt32 eventConsumed_;
  NABoolean processIsStopping_;
/*
  //
  // Fields to enforce SQL semantics inside the UDR server. These
  // fields are checked by the UDR server after a user-defined routine
  // returns control, to see if that routine violated any SQL
  // semantics for UDRs. Some examples of violations include a NO SQL
  // routine attempting to access SQL, and a routine attempting to
  // issue SQL transaction statements such as BEGIN WORK.
  //
  // If the udrErrorChecksEnabled_ flag is FALSE then none of this UDR
  // error checking is performed. The only SQL application that
  // enables the UDR error checking is the UDR server.
  // 
  //
  NABoolean udrErrorChecksEnabled_;
  Lng32 udrSqlAccessMode_;
  NABoolean udrAccessModeViolation_;
  NABoolean udrXactViolation_;
  NABoolean udrXactAborted_;
  Int64 udrXactId_;  // transid that UDR Server is interested to know
                     // if aborted
*/
  Int64 totalCliCalls_;
  short globalSbbCount_;
  short savedSqlTerminateAction_;

  NABoolean breakEnabled_;
  NABoolean SPBreakReceived_;
  NABoolean isESPProcess_;

  // location of the application program which is calling SQL.
  // Fully qualified oss pathname for OSS processes.
  char * programDir_;
  short  processType_; // 0, oss process.  1, guardian process.
  NABoolean logReclaimEventDone_;
  short savedCompilerVersion_; // saved version from previous CLI call
  // node, cpu and pin this process is running at.
  char myNodeName_[8];
  Int32 myCpu_;
  SB_Verif_Type myVerifier_;
  pid_t myPin_;
  Lng32  myNodeNumber_;

  // number of configured cpus (Linux nodes) on this system
  // and an array with their node ids (nids)
  Lng32 myNumCpus_;
  Int32 *cpuArray_;

  IpcPriority myPriority_;
  NABoolean priorityChanged_;

  // timestamp when cli globals were initialized. Start of master executor.
  Int64 myStartTime_;

  // remember the last unique number.
  // Starts at 0 and increments when needed.
  Int64 lastUniqueNumber_;

  // remember the last session unique number.
  // Starts at 0 and increments when needed.
  Int64 sessionUniqueNumber_;

  // EMS event descriptor
  SQLMXLoggingArea::ExperienceLevel emsEventExperienceLevel_;

  char nodeName_[9];

  StatsGlobals *statsGlobals_;
// heap used for the Stats collection
  NAHeap *statsHeap_;
  Long semId_;
  NABoolean inConstructor_; //IsExecutor should return TRUE while cliGlobals is being
                            // constructed. CliGlobals is constructed outside of CLI
                            // calls and no. of cliCalls is not yet incremented
  short savedPriority_;
  Int32 shmId_;
  NABoolean isUncProcess_;
  char myProcessNameString_[PROCESSNAME_STRING_LEN]; // PROCESSNAME_STRING_LEN in ComRtUtils.h =40 in ms.h the equiv seabed limit is 32
  char parentProcessNameString_[PROCESSNAME_STRING_LEN]; 
  // For executor trace.
  char myProgName_[PROGRAM_NAME_LEN];   // 64, see define in ComRtUtils.h
  HashQueue *tidList_;
  CLISemaphore *cliSemaphore_;
  ExProcessStats *processStats_;
  // for trusted UDR invocations from executor and compiler
  LmLanguageManagerC *langManC_;
  LmLanguageManagerJava *langManJava_;
  NABoolean espProcess_;
  HBaseClient_JNI *hbaseClientJNI_;
};

// -----------------------------------------------------------------------
// A global method to get a pointer to the CLI globals. The method
// will create the globals if they don't exist already (it will
// not create the executor segment, though).
//
// Note that this method is semi-expensive and shouldn't be called
// a lot of times. It is better to cache the pointer to the globals.
// -----------------------------------------------------------------------

CliGlobals * GetCliGlobals();

#endif /* __cplusplus */

#endif

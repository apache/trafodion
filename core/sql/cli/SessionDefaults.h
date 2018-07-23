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
#ifndef CLI_SESSION_DEFAULTS_H
#define CLI_SESSION_DEFAULTS_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         CliSessionDefaults.h
 * Description:  Default settings for cli.
 *               
 * Created:      9/8/2005
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "ex_god.h"
#include "Statement.h"
class CliGlobals;
class AQRInfo;
class AQRStatementAttributes;

#define DEFAULT_RECLAIM_MEMORY_AFTER      800
#define DEFAULT_RECLAIM_FREE_MEMORY_RATIO 25
#define DEFAULT_RECLAIM_FREE_PFS_RATIO    50 

class SessionEnvvar : public NABasicObject {
public:
  SessionEnvvar(CollHeap * heap, char * envvarName, char * envvarValue);
  SessionEnvvar();
  ~SessionEnvvar();

  SessionEnvvar(const SessionEnvvar &other);

  NABoolean operator ==(const SessionEnvvar &other) const;
  SessionEnvvar& operator =(const SessionEnvvar &other);

  char * envvarName() { return envvarName_; }
  char * envvarValue() { return envvarValue_; }

private:
  CollHeap * heap_;
  char * envvarName_;
  char * envvarValue_;
};

class SessionDefaults : public NABasicObject {
public:
  enum SessionDefaultType
  {
    SDT_BOOLEAN = 0,
    SDT_BINARY_SIGNED = 1,
    SDT_ASCII = 2
  };
  
  // keep this list in alphabetical order starting at the
  // first attr after 'INVALID_SESSION_DEFAULT and LAST_SESSION_DEFAULT_ATTRIBUTE'.
  // Keep SessionDefaultMap in SessionDefaults consistent with
  // this enum list.
  enum SessionDefaultAttribute
  {
    INVALID_SESSION_DEFAULT    = -1,
    ALTPRI_ESP,
    ALTPRI_FIRST_FETCH,
    ALTPRI_MASTER,
    ALTPRI_MASTER_SEQ_EXE,
    AQR_ENTRIES,
    AUTO_QUERY_RETRY_WARNINGS,
    CALL_EMBEDDED_ARKCMP,
    CANCEL_ESCALATION_INTERVAL,
    CANCEL_ESCALATION_MXOSRVR_INTERVAL,
    CANCEL_ESCALATION_SAVEABEND,
    CANCEL_LOGGING,
    CANCEL_QUERY_ALLOWED,
    CANCEL_UNIQUE_QUERY,
    CATALOG,
    COMPILER_IDLE_TIMEOUT,
    DBTR_PROCESS,
    ESP_ASSIGN_DEPTH,
    ESP_ASSIGN_TIME_WINDOW,
    ESP_CLOSE_ERROR_LOGGING,
    ESP_FIXUP_PRIORITY,
    ESP_FIXUP_PRIORITY_DELTA,
    ESP_FREEMEM_TIMEOUT,
    ESP_IDLE_TIMEOUT,
    ESP_INACTIVE_TIMEOUT,
    ESP_PRIORITY,
    ESP_PRIORITY_DELTA,
    ESP_STOP_IDLE_TIMEOUT,
    ESP_RELEASE_WORK_TIMEOUT,
    INTERNAL_FORMAT_IO,
    ISO_MAPPING,
    MASTER_PRIORITY,
    MASTER_PRIORITY_DELTA,
    MAX_POLLING_INTERVAL,
    MXCMP_PRIORITY,
    MXCMP_PRIORITY_DELTA,
    PARENT_QID,
    PARENT_QID_SYSTEM,
    PARSER_FLAGS,
    PERSISTENT_OPENS,
    RECLAIM_FREE_MEMORY_RATIO,
    RECLAIM_FREE_PFS_RATIO,
    RECLAIM_MEMORY_AFTER,
    ROWSET_ATOMICITY,
    RTS_TIMEOUT,
    SCHEMA,
    STATISTICS_VIEW_TYPE,
    SUSPEND_LOGGING,
    USE_LIBHDFS,
    USER_EXPERIENCE_LEVEL,
    WMS_PROCESS,
    LAST_SESSION_DEFAULT_ATTRIBUTE  // This enum entry should be last always. Add new enums before this entry
  };
  
  struct SessionDefaultMap {
  public:
    SessionDefaultAttribute attribute;
    const char * attributeString;
    SessionDefaultType attributeType;
    NABoolean isCQD;
    NABoolean fromDefaultsTable;
    NABoolean isSSD;
    NABoolean externalized;
  };
  
  SessionDefaults(CollHeap * heap);
  
  ~SessionDefaults();

  void setDbtrProcess(NABoolean v = TRUE) 
  { 
    dbtrProcess_ = v; 
  }

  void setJdbcProcess(NABoolean v = TRUE) 
  { 
    jdbcProcess_ = v; 
  }

  void setOdbcProcess(NABoolean v = TRUE) 
  { 
    odbcProcess_ = v; 
  }

  void setMxciProcess(NABoolean v = TRUE) 
  { 
    mxciProcess_ = v; 
  }

  void setNvciProcess(NABoolean v = TRUE) 
  { 
    mxciProcess_ = v; 
  }

  void setMariaQuestProcess(NABoolean v = TRUE)
  { 
    mariaQuestProcess_ = v; 
  }
 
  void setInternalCli(NABoolean v = TRUE) 
  { 
    internalCli_ = v; 
  }

  void setWmsProcess(NABoolean v = TRUE)
  {
    const Int16 DisAmbiguate = 0;
    wmsProcess_ = v;
    updateDefaultsValueString(WMS_PROCESS, DisAmbiguate, wmsProcess_);
  }

  void setCompilerIdleTimeout(Lng32 compilerIdleTimeout)
  {
    compilerIdleTimeout_ = compilerIdleTimeout;
    updateDefaultsValueString(COMPILER_IDLE_TIMEOUT, compilerIdleTimeout_);
  }
  
  void setIsoMappingName(const char * attrValue, Lng32 attrValueLen);
  void setIsoMappingEnum();

  void setEspPriority(Lng32 espPriority) 
  { 
    espPriority_ = espPriority;

    updateDefaultsValueString(ESP_PRIORITY, espPriority_);
  }

  void setMxcmpPriority(Lng32 mxcmpPriority) 
  { 
    mxcmpPriority_ = mxcmpPriority;

    updateDefaultsValueString(MXCMP_PRIORITY, mxcmpPriority_);
  }

  void setEspPriorityDelta(Lng32 espPriorityDelta) 
  { 
    espPriorityDelta_ = espPriorityDelta;

    updateDefaultsValueString(ESP_PRIORITY_DELTA, espPriorityDelta_);
  }

  void setMxcmpPriorityDelta(Lng32 mxcmpPriorityDelta) 
  { 
    mxcmpPriorityDelta_ = mxcmpPriorityDelta;

    updateDefaultsValueString(MXCMP_PRIORITY_DELTA, mxcmpPriorityDelta_);
  }

  void setEspFixupPriority(Lng32 espFixupPriority) 
  { 
    espFixupPriority_ = espFixupPriority;

    updateDefaultsValueString(ESP_FIXUP_PRIORITY, espFixupPriority_);
  }

  void setEspFixupPriorityDelta(Lng32 espFixupPriorityDelta) 
  { 
    espFixupPriorityDelta_ = espFixupPriorityDelta;

    updateDefaultsValueString(ESP_FIXUP_PRIORITY_DELTA, espFixupPriorityDelta_);
  }

  void setCatalog(char * attrValue, Lng32 attrValueLen)
  {
    if (catalog_)
      {
	NADELETEBASIC(catalog_, heap_);
      }
    
    catalog_ = new(heap_) char[attrValueLen + 1];
    strncpy(catalog_, attrValue, attrValueLen);
    catalog_[attrValueLen] = '\0';

    updateDefaultsValueString(CATALOG, catalog_);
  }

  void setSchema(char * attrValue, Lng32 attrValueLen)
  {
    if (schema_)
      {
	NADELETEBASIC(schema_, heap_);
      }
    
    schema_ = new(heap_) char[attrValueLen + 1];
    strncpy(schema_, attrValue, attrValueLen);
    schema_[attrValueLen] = '\0';
    updateDefaultsValueString(SCHEMA, schema_);
  }

  void setUEL(char * attrValue, Lng32 attrValueLen)
  {
    if (uel_)
      {
	NADELETEBASIC(uel_, heap_);
      }
    
    uel_ = new(heap_) char[attrValueLen + 1];
    strncpy(uel_, attrValue, attrValueLen);
    uel_[attrValueLen] = '\0';
    updateDefaultsValueString(USER_EXPERIENCE_LEVEL, uel_);
  }
  void setEspAssignDepth(Lng32 espAssignDepth) 
  { 
    espAssignDepth_ = espAssignDepth;

    updateDefaultsValueString(ESP_ASSIGN_DEPTH, espAssignDepth_);
  }

  void setEspAssignTimeWindow(Lng32 espAssignTimeWindow) 
  { 
    espAssignTimeWindow_ = espAssignTimeWindow;

    updateDefaultsValueString(ESP_ASSIGN_TIME_WINDOW, espAssignTimeWindow_);
  }

  void setEspStopIdleTimeout(Lng32 espStopIdleTimeout)
  {
    espStopIdleTimeout_ = espStopIdleTimeout;

    updateDefaultsValueString(ESP_STOP_IDLE_TIMEOUT, espStopIdleTimeout_);
  }

  void setEspIdleTimeout(Lng32 espIdleTimeout)
  {
    espIdleTimeout_ = espIdleTimeout;

    updateDefaultsValueString(ESP_IDLE_TIMEOUT, espIdleTimeout_);
  }

  void setEspInactiveTimeout(Lng32 espInactiveTimeout)
  {
    espInactiveTimeout_ = espInactiveTimeout;

    updateDefaultsValueString(ESP_INACTIVE_TIMEOUT, espInactiveTimeout_);
  }

  void setEspReleaseWorkTimeout(Lng32 espReleaseWorkTimeout)
  {
    espReleaseWorkTimeout_ = espReleaseWorkTimeout;

    updateDefaultsValueString(ESP_RELEASE_WORK_TIMEOUT, espReleaseWorkTimeout_);
  }

  void setMaxPollingInterval(Lng32 maxPollingInterval)
  {
    maxPollingInterval_ = maxPollingInterval;
    updateDefaultsValueString(MAX_POLLING_INTERVAL, maxPollingInterval_);
  }
  void setPersistentOpens(Lng32 persistentOpens)
  {
    persistentOpens_ = persistentOpens;
    updateDefaultsValueString(PERSISTENT_OPENS, persistentOpens_);
  }

  void setEspCloseErrorLogging(NABoolean espCloseErrorLogging)
  {
    const Int16 DisAmbiguate = 0;
    espCloseErrorLogging_ = espCloseErrorLogging;

    updateDefaultsValueString(ESP_CLOSE_ERROR_LOGGING, DisAmbiguate,
                              espCloseErrorLogging_);
  }

  void setUseLibHdfs(NABoolean useLibHdfs)
  {
    const Int16 DisAmbiguate = 0;
    useLibHdfs_ = useLibHdfs;
    updateDefaultsValueString(USE_LIBHDFS, DisAmbiguate,
                              useLibHdfs_);
  }

  void setEspFreeMemTimeout(Lng32 espFreeMemTimeout)
  {
    espFreeMemTimeout_ = espFreeMemTimeout;

    updateDefaultsValueString(ESP_FREEMEM_TIMEOUT, espFreeMemTimeout_);
  }

  void setAltpriMaster(NABoolean v)
  {
    const Int16 DisAmbiguate = 0;
    altpriMaster_ = v;

    updateDefaultsValueString(ALTPRI_MASTER, DisAmbiguate, altpriMaster_);
  }

  void setAltpriMasterSeqExe(NABoolean v)
  {
    const Int16 DisAmbiguate = 0;
    altpriMasterSeqExe_ = v;

    updateDefaultsValueString(ALTPRI_MASTER_SEQ_EXE, DisAmbiguate, altpriMasterSeqExe_);
  }

  void setAltpriEsp(NABoolean v)
  {
    const Int16 DisAmbiguate = 0;
    altpriEsp_ = v;

    updateDefaultsValueString(ALTPRI_ESP, DisAmbiguate, altpriEsp_);
  }

  void setAltpriFirstFetch(NABoolean v)
  {
    const Int16 DisAmbiguate = 0;
    altpriFirstFetch_ = v;

    updateDefaultsValueString(ALTPRI_FIRST_FETCH, DisAmbiguate, altpriFirstFetch_);
  }

  void setInternalFormatIO(NABoolean v)
  {
    internalFormatIO_ = v;
  }

  void setRowsetAtomicity(Lng32 rsa)
  {
    rowsetAtomicity_ = rsa;
  }

  void setCancelEscalationInterval(Lng32 cei)
  {
    cancelEscalationInterval_ = cei;
    updateDefaultsValueString(CANCEL_ESCALATION_INTERVAL, 
                              cancelEscalationInterval_);
  }

  void setCancelEscalationMxosrvrInterval(Lng32 cei)
  {
    cancelEscalationMxosrvrInterval_ = cei;
    updateDefaultsValueString(CANCEL_ESCALATION_MXOSRVR_INTERVAL, 
                              cancelEscalationMxosrvrInterval_);
  }

  void setCancelEscalationSaveabend(NABoolean v)
  {
    const Int16 DisAmbiguate = 0;
    cancelEscalationSaveabend_ = v;
    updateDefaultsValueString(CANCEL_ESCALATION_SAVEABEND, DisAmbiguate, 
                              cancelEscalationSaveabend_);
  }

  void setCancelLogging(NABoolean v)
  {
    const Int16 DisAmbiguate = 0;
    cancelLogging_ = v;
    updateDefaultsValueString(CANCEL_LOGGING, DisAmbiguate, 
                              cancelLogging_);
  }

  void setSuspendLogging(NABoolean v)
  {
    const Int16 DisAmbiguate = 0;
    suspendLogging_ = v;
    updateDefaultsValueString(SUSPEND_LOGGING, DisAmbiguate, 
                              cancelLogging_);
  }

  void setCancelQueryAllowed(NABoolean v)
  {
    const Int16 DisAmbiguate = 0;
    cancelQueryAllowed_ = v;
    updateDefaultsValueString(CANCEL_QUERY_ALLOWED, DisAmbiguate, 
                              cancelQueryAllowed_);
  }

  void setCancelUniqueQuery(NABoolean v)
  {
    const Int16 DisAmbiguate = 0;
    cancelUniqueQuery_ = v;
    updateDefaultsValueString(CANCEL_UNIQUE_QUERY, DisAmbiguate, 
                              cancelUniqueQuery_);
  }

 void setCallEmbeddedArkcmp(NABoolean v)
  {
    const Int16 DisAmbiguate = 0;
    callEmbeddedArkcmp_ = v;
    updateDefaultsValueString(CALL_EMBEDDED_ARKCMP, DisAmbiguate, 
                              callEmbeddedArkcmp_);
  }

  void setModeSeabase(NABoolean v)
  { modeSeabase_ = v; }

  NABoolean getDbtrProcess() { return dbtrProcess_; }
  NABoolean getOdbcProcess() { return odbcProcess_; }
  NABoolean getJdbcProcess() { return jdbcProcess_; }
  NABoolean getMxciProcess() { return mxciProcess_; }
  NABoolean getNvciProcess() { return nvciProcess_; }
  NABoolean getWmsProcess()  { return wmsProcess_; }
  NABoolean getMariaQuestProcess() { return mariaQuestProcess_; }
  NABoolean getModeSeabase() { return modeSeabase_; }

  Lng32 getEspPriority(){ return espPriority_; }
  Lng32 getMxcmpPriority(){ return mxcmpPriority_; }
  Lng32 getEspPriorityDelta(){ return espPriorityDelta_; }
  Lng32 getMxcmpPriorityDelta(){ return mxcmpPriorityDelta_; }
  Lng32 getEspFixupPriority(){ return espFixupPriority_; }
  Lng32 getEspFixupPriorityDelta(){ return espFixupPriorityDelta_; }
  Lng32 getEspAssignDepth(){ return espAssignDepth_; }
  Lng32 getEspAssignTimeWindow(){ return espAssignTimeWindow_; }
  Lng32 getEspStopIdleTimeout() { return espStopIdleTimeout_; }
  Lng32 getEspIdleTimeout() { return espIdleTimeout_; }
  Lng32 getCompilerIdleTimeout() { return compilerIdleTimeout_; }
  Lng32 getEspInactiveTimeout() { return espInactiveTimeout_; }
  Lng32 getEspReleaseWorkTimeout() { return espReleaseWorkTimeout_; }
  Lng32 getMaxPollingInterval() { return maxPollingInterval_; }
  Lng32 getPersistentOpens() { return persistentOpens_; }
  Lng32 getEspFreeMemTimeout() { return espFreeMemTimeout_; }
  Lng32 getEspCloseErrorLogging() { return espCloseErrorLogging_; }

  NABoolean getAltpriMaster() { return altpriMaster_; }
  NABoolean getAltpriMasterSeqExe() { return altpriMasterSeqExe_; }
  NABoolean getAltpriFirstFetch() { return altpriFirstFetch_; }
  NABoolean getAltpriEsp()    { return FALSE; }

  NABoolean getInternalFormatIO() { return internalFormatIO_;}
  char * getIsoMappingName() { return isoMappingName_; }
  Lng32 getIsoMappingEnum() { return isoMappingEnum_; }

  Lng32 getRowsetAtomicity() { return rowsetAtomicity_;}

  void setCliBulkMove(NABoolean cbm) { cliBulkMove_ = cbm; }
  NABoolean getCliBulkMove() { return cliBulkMove_; }
  
  void setAqrEmsEvent(NABoolean aem) { aqrEmsEvent_ = aem; }
  NABoolean getAqrEmsEvent() { return aqrEmsEvent_; }
  
  void setAqrType(Lng32 aqrt) { aqrType_ = aqrt; }
  NABoolean getAqrType() { return aqrType_; }
   
  void setRtsTimeout(Lng32 rtsTimeout)
  {
    rtsTimeout_ = rtsTimeout;

    updateDefaultsValueString(RTS_TIMEOUT, rtsTimeout_);
  }
  Lng32 getRtsTimeout(){ return rtsTimeout_; }
  NABoolean getCallEmbeddedArkcmp() { return callEmbeddedArkcmp_;}
  void setParserFlags(Lng32 parserFlags)
  {
    parserFlags_ = parserFlags;

    updateDefaultsValueString(PARSER_FLAGS, parserFlags_);
  }
  Lng32 getParserflags(){ return parserFlags_; }

  Int32 *userId() { return &userId_; }

  void setAQRWarnings(Lng32 v) { aqrWarn_ = v; }
  Lng32 aqrWarnings() { return aqrWarn_; }

  Lng32 getCancelEscalationInterval() {  return cancelEscalationInterval_;  }

  Lng32 getCancelEscalationMxosrvrInterval()
                                { return cancelEscalationMxosrvrInterval_; }

  NABoolean getCancelEscalationSaveabend()
                                      { return cancelEscalationSaveabend_; }

  NABoolean getCancelQueryAllowed()         { return  cancelQueryAllowed_; }

  NABoolean getCancelUniqueQuery()            { return cancelUniqueQuery_; }

  NABoolean getCancelLogging()                    { return cancelLogging_; }

  NABoolean getSuspendLogging()                  { return suspendLogging_; }

  NABoolean getUseLibHdfs()                  { return useLibHdfs_; }

  Lng32 readFromDefaultsTable(CliGlobals * cliGlobals);
  Lng32 setIsoMappingDefine();

  SessionDefaultMap getSessionDefaultMap
  (char * attribute, Lng32 attrLen);

  void setSessionDefaultAttributeValue
  (SessionDefaultMap sda, char * attrValue, Lng32 attrValueLen);

  void initializeSessionDefault
  (char * attribute, Lng32 attrLen,
   char * attrValue, Lng32 attrValueLen);

  void position();

  short getNextSessionDefault(char* &attributeString, 
			      char* &attributeValue,
			      Lng32  &isCQD,
			      Lng32  &inDefTab,
			      Lng32  &isSSD,
			      Lng32  &isExternalized);
 
  void saveSessionDefaults();
  void restoreSessionDefaults();

  void resetSessionOnlyAttributes();
  void setParentQid(char *attrValue, Lng32 attrValueLen);
  char * getParentQid() { return parentQid_; }
  void setParentQidSystem(char *attrValue, Lng32 attrValueLen);
  char * getParentQidSystem() { return parentQidSystem_; }
  void beginSession();

  NAList<SessionEnvvar> * sessionEnvvars()
  {return sessionEnvvars_;}

  AQRInfo * aqrInfo() { return aqrInfo_; }

  Lng32 getStatisticsViewType() { return statisticsViewType_; }
  void setStatisticsViewType(Lng32 type) 
  { 
    statisticsViewType_ = type; 
    updateDefaultsValueString(STATISTICS_VIEW_TYPE, statisticsViewType_);
  }
  Lng32 getReclaimTotalMemorySize() { return reclaimTotalMemorySize_; }
  // Memory Size in MB
  void setReclaimTotalMemorySize(Lng32 memorySize) 
  {
    reclaimTotalMemorySize_ = memorySize * 1024 * 1024;
    updateDefaultsValueString(RECLAIM_MEMORY_AFTER, memorySize);
  }

  Lng32 getReclaimFreeMemoryRatio() { return reclaimFreeMemoryRatio_; }
  void setReclaimFreeMemoryRatio(Lng32 freeMemoryRatio)
  {
    reclaimFreeMemoryRatio_ = freeMemoryRatio;
    updateDefaultsValueString(RECLAIM_FREE_MEMORY_RATIO, reclaimFreeMemoryRatio_);
  }
  
  NABoolean getRedriveCTAS() { return redriveCTAS_; }
  void setRedriveCTAS(NABoolean v)
  {
    redriveCTAS_ = v;
  }

  // For SeaMonster
  char *getExSMTraceFilePrefix() { return exsmTraceFilePrefix_; }
  void setExSMTraceFilePrefix(const char *pref, Int32 prefLen);
  UInt32 getExSMTraceLevel() { return exsmTraceLevel_; }
  void setExSMTraceLevel(UInt32 lvl) { exsmTraceLevel_ = lvl; }

  Lng32 getReclaimFreePFSRatio() { return reclaimFreePFSRatio_; }
  void setReclaimFreePFSRatio(Lng32 freePFSRatio)
  {
    reclaimFreePFSRatio_ = freePFSRatio;
    updateDefaultsValueString(RECLAIM_FREE_PFS_RATIO, reclaimFreePFSRatio_);
  }
  NABoolean callEmbeddedArkcmp() { return callEmbeddedArkcmp_;}

private:
  void updateDefaultsValueString(SessionDefaultAttribute sda, 
				 const Int16  DisAmbiguate,
				 NABoolean value);
  void updateDefaultsValueString(SessionDefaultAttribute sda, 
				 Lng32 value);
  void updateDefaultsValueString(SessionDefaultAttribute sda, 
				 char * value);

private:
  CollHeap * heap_;

  char ** defaultsValueString_;
  char ** savedDefaultsValueString_;

  // not set as yet
  NABoolean jdbcProcess_;
  NABoolean odbcProcess_;

  NABoolean dbtrProcess_; //db transporter
  NABoolean mxciProcess_;
  NABoolean nvciProcess_;
  NABoolean internalCli_;
  
  NABoolean mariaQuestProcess_;
  

  // current user id
  Int32 userId_;

  // cat & sch
  char * catalog_;
  char * schema_;

  // priorities
  // priorities of ESP and MXCMP
  Lng32 espPriority_;
  Lng32 mxcmpPriority_;
  // incremental priorities of ESP and MXCMP relative to current process.
  // Valid if espPriority_ and mxcmpPriority_ are set to -1.
  Lng32 espPriorityDelta_;
  Lng32 mxcmpPriorityDelta_;

  // absolute priorities of ESPs during plan fixup time.
  Lng32 espFixupPriority_;

  // incremental priorities of ESP during plan fixup time relative to current
  // irrespect to whether espPriority_ and/or espPriorityDelta_ is set or not
  Lng32 espFixupPriorityDelta_;

  // Limit on number of statements with ESPs assigned. When the limit is
  // exceeded, releaseSpace() will be called on the closed statement whose
  // ESPs were assigned earliest
  Lng32 espAssignDepth_;
  // ESP won't get assigned to new query if it has this much or less time left
  // before idle timed out
  Lng32 espAssignTimeWindow_;
  // number of seconds that idle esps are kept alive before killed by master
  Lng32 espStopIdleTimeout_;
  // number of seconds an esp should wait idle before it times out
  Lng32 espIdleTimeout_;
  // number of seconds the compiler process can remain idle before it is killed by the master 
  Lng32 compilerIdleTimeout_;
  // numcompilerIber of seconds an esp should remain inactive before times out
  Lng32 espInactiveTimeout_;
  // number of seconds that master waits for release work reply from esps
  Lng32 espReleaseWorkTimeout_;
  // The maximum priv stack size to request when calling HEADROOM_ENSURE_

  Lng32 maxPollingInterval_;
  Lng32 persistentOpens_;
  // secs after which deallocated memory is freed up and given back to kernel
  Lng32 espFreeMemTimeout_;
  // Generate an EMS event if a connection is placed in error state due to
  // a close message with a request outstanding
  NABoolean espCloseErrorLogging_;

  // user experience level
   char * uel_;

  // master selfadjusts its priority after fixup to bring it to the
  // same value as the esp. This stmt does the priority change for
  // parallel plans.
  NABoolean altpriMaster_;

  // master selfadjusts its priority after fixup to bring it to the
  // same value as the esp. This stmts does the priority change for
  // sequential, non-olt plans.
  NABoolean altpriMasterSeqExe_;

  // change master priority back to its fixup priority after first
  // row has been returned to application.
  NABoolean altpriFirstFetch_;

  // esp selfadjusts its priority after fixup to bring it down
  // to its execute priority.
  NABoolean altpriEsp_;

  // others
  NABoolean cliBulkMove_;

  // if set to true, an ems msg is logged when a query is retried.
  // Default is TRUE.
  NABoolean aqrEmsEvent_;

  // type of aqr: 0, none. 1, system. 2, On. 3, All
  Lng32 aqrType_;

  // input & output of datatypes to be done in sql internal format.
  // Currently done for datetime and internal datatypes.
  // External string format is returned, if this SSD is not set.
  // Used for bulk moves.
  NABoolean internalFormatIO_;

  // value of isoMapping. Set in defaults table.
  char * isoMappingName_;
  Lng32 isoMappingEnum_;

  //Timeout when requesting run-time stats from the SSMP process, in seconds.
  Lng32 rtsTimeout_;

  Lng32 parserFlags_;

  Lng32 rowsetAtomicity_;

  // if warnings should be returned after recompilation/retry.
  // 0, no warnings. 1, all warnings.
  Lng32 aqrWarn_;

  // variable to return default values for display
  Lng32 currDef_;

  NAList<SessionEnvvar> * sessionEnvvars_;
  
  char *parentQid_;         // Parent Query id at session level
  char parentQidSystem_[25];   // Parent QueryId system at session level
  NABoolean wmsProcess_;    // Flag to trigger deallocating StmtStats in the shared segment
                            // via MXSSMP when RMS stats is collected after the statement has 
                            // been deallocated

  Lng32 cancelEscalationInterval_;         // canceler's session
  Lng32 cancelEscalationMxosrvrInterval_;  // canceler's session
  NABoolean cancelEscalationSaveabend_;   // canceler's session
  NABoolean cancelQueryAllowed_;          // target query's session
  NABoolean cancelUniqueQuery_;           // target query's session
  NABoolean cancelLogging_;               // canceler's session
  NABoolean suspendLogging_;              // suspended's session
  NABoolean callEmbeddedArkcmp_;       // call the procedural interface and don't send a message to the arkcmp process.
  AQRInfo * aqrInfo_;
  Lng32 statisticsViewType_;     // Statistics view type which could be different from the collection statistics type
/*
  Memory manager will start to reclaim space when the below conditions are met
        a)  Total memory size in executor segments is above the set value (800 MB)
        b)  When free to total size memory ratio is less than the set value (25%)
*/
  Lng32 reclaimTotalMemorySize_;  // Total Memory size after which memory manager might trigger reclaim memory space
  Lng32 reclaimFreeMemoryRatio_;  // Free to Total memory ratio

  NABoolean redriveCTAS_;

  // For SeaMonster
  char *exsmTraceFilePrefix_;
  UInt32 exsmTraceLevel_;

  Lng32 reclaimFreePFSRatio_; // 100 - (PFS current use / PFS size))
  
  NABoolean modeSeabase_;
  Lng32 jniDebugPort_;     // port to attache JNI debugger, <=0 to disable
  Lng32 jniDebugTimeout_;  // timeout (msec) to wait for debugger to attach
  NABoolean useLibHdfs_;
};


// information, status and other details related to auto-query-retry.
#define AQREntry(sqlcode, nskcode, retries, delay, type, numCQDs, cqdStr, cmpInfo, intAQR) \
     {sqlcode, nskcode, retries, delay, type, numCQDs, cqdStr, cmpInfo, intAQR}

class AQRInfo : public NABasicObject {
public:

  struct AQRErrorMap {
  public:
    Lng32 sqlcode;
    Lng32 nskcode;
    Lng32 retries;
    Lng32 delay;
    Lng32 type;
    Lng32 numCQDs;
    const char * cqdStr;
    Lng32 cmpInfo;
    Lng32 intAQR;
  };

  enum RetryType
  {
    // no retry
    RETRY_NONE    = -1,

    // retry immediately. unavailable partition...
    RETRY = 0,
    RETRY_MIN_VALID_TYPE = RETRY,

    // retry, but don't use query cache during recompile. 
    // Decache all entries. Blown away open, timestamp mismatch, ...
    RETRY_WITH_DECACHE = 1,

    // retry, but cleanup and verify esps.
    // Done after ESP deaths.
    RETRY_WITH_ESP_CLEANUP = 2,

    // retry but only if there is no user transaction and autocommit is on.
    // Invalid transid is an example.
    RETRY_NO_XN  = 3,

    RETRY_MXCMP_KILL=4,
    RETRY_DECACHE_HTABLE = 5,

    RETRY_MAX_VALID_TYPE = RETRY_DECACHE_HTABLE
  };
  
  AQRInfo(CollHeap * heap);
  
  ~AQRInfo();

  void position();

  short getNextAQREntry(Lng32 &sqlcode, Lng32 &nskcode,
			Lng32 &retries, Lng32 &delay, Lng32 &type,
			Lng32 &intAQR);
 
  void saveAQRErrors();
  NABoolean restoreAQRErrors();

  short getAQREntry(Lng32 sqlcode, Lng32 nskcode,
		    Int32 &retries, Int32 &delay, Int32 &type,
		    Int32 &numCQDs, char* &cqdStr,
		    Int32 &cmpInfo, Int32 &intAQR);

  short setAQREntry(Lng32 task,
		    Lng32 sqlcode, Lng32 nskcode,
		    Int32 retries, Int32 delay, Int32 type,
		    Int32 numCQDs, char * cqdStr,
		    Int32 cmpInfo, Int32 intAQR);

  short setAQREntriesFromInputStr(char * inStr, Lng32 inStrLen);

  AQRStatementInfo * aqrStmtInfo() { return aqrStmtInfo_; }
  void setAqrStmtInfo(AQRStatementInfo * v) { aqrStmtInfo_ = v; }

  void clearRetryInfo();

  Lng32 setCQDs(Lng32 numCQDs, char * cqdStr,
	       ContextCli * context);
  Lng32 resetCQDs(Lng32 numCQDs, char * cqdStr,
		 ContextCli * context);
  Lng32 setCompilerInfo(char * queryId,
		       ComCondition * errCond,
		       ContextCli * context);
  Lng32 resetCompilerInfo(char * queryId,
			 ComCondition * errCond,
			 ContextCli * context);

  NABoolean xnStartedAtPrepare() { return (flags_ & PREPARE_XN) != 0; ; }
  void setXnStartedAtPrepare(NABoolean v) 
  { (v ? flags_ |= PREPARE_XN : flags_ &= ~PREPARE_XN); }

  NABoolean espCleanup() { return (flags_ & ESP_CLEANUP) != 0; ; }
  void setEspCleanup(NABoolean v) 
  { (v ? flags_ |= ESP_CLEANUP : flags_ &= ~ESP_CLEANUP); }

  NABoolean abortedTransWasImplicit() 
      { return (flags_ & ABORTED_TX_WAS_IMPLICIT) != 0; ; }

  // Reset in clearRetryInfo().
  void setAbortedTransWasImplicit() { flags_ |= ABORTED_TX_WAS_IMPLICIT; }

  NABoolean noAQR() { return (flags_ & NO_AQR) != 0; ; }
  void setNoAQR(NABoolean v) 
  { (v ? flags_ |= NO_AQR : flags_ &= ~NO_AQR); }

private:
  enum AQRFlags
  {
    PREPARE_XN              = 0x0001,
    ESP_CLEANUP             = 0x0002,
    ABORTED_TX_WAS_IMPLICIT = 0x0004,
    NO_AQR                  = 0x0008
  };

  CollHeap * heap_;
  LIST(AQRErrorMap) *aqrErrorList_;
  LIST(AQRErrorMap) *origAqrErrorList_;
  // unused LIST(AQRErrorMap) *savedAqrErrorList_;

  // error information set when a retryable error is returned as part
  // auto query retry feature.
  // Caller must call back to reprepare the query. 

  AQRStatementInfo * aqrStmtInfo_;

  Lng32 currErr_;

  UInt32 flags_;
};

class  AQRStatementAttributes : public NABasicObject {
 public:
  AQRStatementAttributes(CollHeap * heap);
  ~AQRStatementAttributes();
  void setAttributesInStatement(Statement *targetStmt);
  void getAttributesFromStatement(Statement *fromStmt);
  void clear();
  CollHeap * heap_;
  SQLATTRHOLDABLE_INTERNAL_TYPE holdable_;
  Statement::AtomicityType rowsetAtomicity_;
  ULng32 inputArrayMaxsize_;
  char * uniqueStmtId_;
  Lng32 uniqueStmtIdLen_;
  char *parentQID_;
  char parentQIDSystem_[25];
  Int64 exeStartTime_;
 
};

class AQRStatementInfo : public NABasicObject {
public:
  AQRStatementInfo(CollHeap * heap);
  ~AQRStatementInfo()
  {
    if (savedStmtAttributes_) 
      NADELETE(savedStmtAttributes_, AQRStatementAttributes,heap_);
    savedStmtAttributes_ = NULL;
  }

  ULng32 getRetryPrepareFlags() { return retryPrepareFlags_; }

  SQLSTMT_ID * getRetryStatementId() { return retryStatementId_; }
  SQLDESC_ID * getRetrySqlSource() { return retrySqlSource_; }
  SQLDESC_ID * getRetryInputDesc() { return retryInputDesc_; }
  SQLDESC_ID * getRetryOutputDesc() { return retryOutputDesc_; }
  SQLDESC_ID * getRetryTempInputDesc() { return retryTempInputDesc_; }
  SQLDESC_ID * getRetryTempOutputDesc() { return retryTempOutputDesc_; }
  AQRStatementAttributes *getAQRStatementAttributes() { return savedStmtAttributes_;}
  
void saveStatementHandle(SQLSTMT_ID *stmt_id)
    {
      retryStatementHandle_ = stmt_id->handle;
    }
  void * getSavedStmtHandle() {return retryStatementHandle_;}
  NABoolean retryFirstFetch() { return (retryFlags_ & RETRY_FIRST_FETCH) != 0; ; }
  NABoolean retryAfterExec() { return (retryFlags_ & RETRY_AFTER_EXEC) != 0; ; }
  void setRetryStatementId(SQLSTMT_ID * v) { retryStatementId_ = v; }
  void setRetrySqlSource(SQLDESC_ID * v) { retrySqlSource_ = v; }
  void setRetryInputDesc(SQLDESC_ID * v) { retryInputDesc_ = v; }
  void setRetryOutputDesc(SQLDESC_ID * v) { retryOutputDesc_ = v; }
  void setRetryPrepareFlags(ULng32 v) { retryPrepareFlags_ = v; }

  void setRetryTempInputDesc(SQLDESC_ID * v) { retryTempInputDesc_ = v; }
  void setRetryTempOutputDesc(SQLDESC_ID * v) { retryTempOutputDesc_ = v; }

 

  void clearRetryInfo();
  
  void setRetryFirstFetch(NABoolean v) 
    { (v ? retryFlags_ |= RETRY_FIRST_FETCH : retryFlags_ &= ~RETRY_FIRST_FETCH); }
 
  void setRetryAfterExec(NABoolean v) 
    { (v ? retryFlags_ |= RETRY_AFTER_EXEC : retryFlags_ &= ~RETRY_AFTER_EXEC); }

  void copySavedStatementhandle(SQLSTMT_ID *stmt_id)
    {
     stmt_id->handle = retryStatementHandle_;
    }
  void saveAttributesFromStmt(Statement *stmt);
  void copyAttributesToStmt(Statement *stmt);

private:
  CollHeap *heap_;
  SQLSTMT_ID * retryStatementId_;
  SQLDESC_ID * retrySqlSource_;
  SQLDESC_ID * retryInputDesc_;
  SQLDESC_ID * retryOutputDesc_;

  SQLDESC_ID * retryTempInputDesc_;
  SQLDESC_ID * retryTempOutputDesc_;

  ULng32 retryPrepareFlags_;
  UInt32 retryFlags_;
  enum RetryFlags
  {
    // first fetch, stmt could be retried.
    RETRY_FIRST_FETCH = 0x0001,
    RETRY_AFTER_EXEC = 0x0002
  };

  void * retryStatementHandle_;
  AQRStatementAttributes *savedStmtAttributes_;
  
};



#endif





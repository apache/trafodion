//**********************************************************************
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
//**********************************************************************/
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         Context.cpp
 * Description:  Procedures to add/update/delete/manipulate executor context.
 *
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "Platform.h"



#include "cli_stdh.h"
#include "NAStdlib.h"
#include "stdio.h"
#include "ComQueue.h"
#include "ExSqlComp.h"
#include "ex_transaction.h"
#include "ComRtUtils.h"
#include "ExStats.h"
#include "sql_id.h"
#include "ex_control.h"
#include "ExControlArea.h"
#include "ex_root.h"
#include "ExExeUtil.h"
#include "ex_frag_rt.h"
#include "ExExeUtil.h"
#include "ComExeTrace.h"
#include "exp_clause_derived.h"
#include "ComUser.h"
#include "CmpSeabaseDDLauth.h"

#include "StmtCompilationMode.h"

#include "ExCextdecs.h"

#include "ComMemoryDiags.h"             // ComMemoryDiags::DumpMemoryInfo()

#include "DllExportDefines.h"

#include <fstream>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "ComplexObject.h"
#include "CliMsgObj.h"
#include "UdrExeIpc.h"
#include "ComTransInfo.h"
#include "ExUdrServer.h"
#include "ComSqlId.h"
#include "NAUserId.h"

#include "stringBuf.h"
#include "NLSConversion.h"

#include <sys/types.h>
#include <sys/syscall.h>
#include <pwd.h>

#include "CmpCommon.h"
#include "arkcmp_proc.h"

#include "ExRsInfo.h"
#include "../../dbsecurity/auth/inc/dbUserAuth.h"
#include "ComDistribution.h"
#include "LmRoutine.h"
#include "HiveClient_JNI.h"

// Printf-style tracing macros for the debug build. The macros are
// no-ops in the release build.
#ifdef _DEBUG
#include <stdarg.h>
#define StmtListDebug0(s,f) StmtListPrintf((s),(f))
#define StmtListDebug1(s,f,a) StmtListPrintf((s),(f),(a))
#define StmtListDebug2(s,f,a,b) StmtListPrintf((s),(f),(a),(b))
#define StmtListDebug3(s,f,a,b,c) StmtListPrintf((s),(f),(a),(b),(c))
#define StmtListDebug4(s,f,a,b,c,d) StmtListPrintf((s),(f),(a),(b),(c),(d))
#define StmtListDebug5(s,f,a,b,c,d,e) StmtListPrintf((s),(f),(a),(b),(c),(d),(e))
#else
#define StmtListDebug0(s,f)
#define StmtListDebug1(s,f,a)
#define StmtListDebug2(s,f,a,b)
#define StmtListDebug3(s,f,a,b,c)
#define StmtListDebug4(s,f,a,b,c,d)
#define StmtListDebug5(s,f,a,b,c,d,e)
#endif

#include "dtm/tm.h"

#include  "CmpContext.h"

ContextCli::ContextCli(CliGlobals *cliGlobals)
  // the context heap's parent is the basic executor memory
  : cliGlobals_(cliGlobals),
   
    exHeap_((char *) "Heap in ContextCli", cliGlobals->getExecutorMemory(), 
            ((! cliGlobals->isESPProcess()) ? 524288 : 32768) /* block size */, 
            0 /* upperLimit */),
    diagsArea_(&exHeap_),
    timeouts_( NULL ),           
    timeoutChangeCounter_( 0 ), 
    validAuthID_(TRUE),
    externalUsername_(NULL),
    sqlParserFlags_(0),
    closeStatementList_(NULL),
    nextReclaimStatement_(NULL),
    closeStmtListSize_(0),
    stmtReclaimCount_(0),
    currSequence_(2),
    prevFixupSequence_(0),
    catmanInfo_(NULL),
    flags_(0),
    ssmpManager_(NULL),
    udrServerList_(NULL),
    udrRuntimeOptions_(NULL),
    udrRuntimeOptionDelimiters_(NULL),
    sessionDefaults_(NULL),
    bdrConfigInfoList_(NULL),
    //    aqrInfo_(NULL),
    userSpecifiedSessionName_(NULL),
    sessionID_(NULL),
    sessionInUse_(FALSE),
    mxcmpSessionInUse_(FALSE),
    volatileSchemaCreated_(FALSE),
    prevStmtStats_(NULL),
    hdfsHandleList_(NULL),
    seqGen_(NULL),
    dropInProgress_(FALSE),
    isEmbeddedArkcmpInitialized_(FALSE),
    embeddedArkcmpContext_(NULL),
    prevCmpContext_(NULL),
    ddlStmtsExecuted_(FALSE),
    numCliCalls_(0),
    jniErrorStr_(&exHeap_),
    hiveClientJNI_(NULL),
    hdfsClientJNI_(NULL),
    arkcmpArray_(&exHeap_),
    cmpContextInfo_(&exHeap_),
    cmpContextInUse_(&exHeap_),
    arkcmpInitFailed_(&exHeap_),
    trustedRoutines_(&exHeap_),
    roleIDs_(NULL),
    granteeIDs_(NULL),
    numRoles_(0),
    unusedBMOsMemoryQuota_(0)
{
  cliSemaphore_ = new (&exHeap_) CLISemaphore();
  ipcHeap_ = new (cliGlobals_->getProcessIpcHeap())
                  NAHeap("IPC Context Heap",
                  cliGlobals_->getProcessIpcHeap(),
                  (cliGlobals->isESPProcess() ? (2048 * 1024) :
                  (512 * 1024)));
  if ( ! cliGlobals->isESPProcess())
     env_ = new(ipcHeap_) IpcEnvironment(ipcHeap_, &eventConsumed_, 
          breakEnabled_);
  else
     env_ = new(ipcHeap_) IpcEnvironment(ipcHeap_, &eventConsumed_, 
                    FALSE, IPC_SQLESP_SERVER, TRUE);
  env_->setCliGlobals(cliGlobals_);
  exeTraceInfo_ = new (&exHeap_) ExeTraceInfo();
  // esp register IPC data trace at runESP()
  env_->registTraceInfo(exeTraceInfo_);
  // We are not worrying about context handle wrap around for now.
  // contextHandle_ is a long (32 bits) that allows us to create 4
  // billion contexts without having a conflict.
  contextHandle_ = cliGlobals_->getNextUniqueContextHandle();

  // For now, in open source, set the user to the root  user (DB__ROOT)
  databaseUserID_ = ComUser::getRootUserID();
  databaseUserName_ = new(exCollHeap()) char[MAX_DBUSERNAME_LEN+1];
  strcpy(databaseUserName_, ComUser::getRootUserName());
  sessionUserID_ = databaseUserID_;

  // moduleList_ is a HashQueue with a hashtable of size 53
  // the other have default size of the hash table
  moduleList_     = new(exCollHeap()) HashQueue(exCollHeap(), 53);
  statementList_  = new(exCollHeap()) HashQueue(exCollHeap());
  descriptorList_ = new(exCollHeap()) HashQueue(exCollHeap());
  cursorList_     = new(exCollHeap()) HashQueue(exCollHeap());
  // openStatementList_ is short (TRUE?!). Use only a small
  // hash table
  openStatementList_ = new(exCollHeap()) HashQueue(exCollHeap(), 23);
  statementWithEspsList_ = new(exCollHeap()) Queue(exCollHeap());
  // nonAuditedLockedTableList_ is short. Use only a small
  // hash table
  nonAuditedLockedTableList_ = new(exCollHeap()) HashQueue(exCollHeap(), 23);

  trafSElist_     = new(exCollHeap()) HashQueue(exCollHeap());

  authID_       = 0;
  authToken_    = 0;
  authIDType_   = SQLAUTHID_TYPE_INVALID;

  arkcmpArray_.insertAt(0,  new(exCollHeap()) ExSqlComp(0,
                               exCollHeap(),
                               cliGlobals,0,
                               COM_VERS_COMPILER_VERSION,
                               NULL,
                               env_));
  arkcmpInitFailed_.insertAt(0,arkcmpIS_OK_);
  versionOfMxcmp_ = COM_VERS_COMPILER_VERSION;
  mxcmpNodeName_ = NULL;
  indexIntoCompilerArray_ = 0;
  transaction_  = 0;
  externalUsername_ = new(exCollHeap()) char[ComSqlId::MAX_LDAP_USER_NAME_LEN+1];
  memset(externalUsername_, 0, ComSqlId::MAX_LDAP_USER_NAME_LEN+1);

  userNameChanged_ = FALSE;

  userSpecifiedSessionName_ = new(exCollHeap()) char[ComSqlId::MAX_SESSION_NAME_LEN+1];
  setUserSpecifiedSessionName((char *) "");
  lastDescriptorHandle_ = 0;
  lastStatementHandle_ = 0;
  transaction_ = new(exCollHeap()) ExTransaction(cliGlobals, exCollHeap());

  controlArea_ = new(exCollHeap()) ExControlArea(this, exCollHeap());

  stats_ = NULL;

  defineContext_ = getCurrentDefineContext();

  catmanInfo_ = NULL;

  //  aqrInfo_ = new(exCollHeap()) AQRInfo(exCollHeap());

  volTabList_ = NULL;

  beginSession(NULL);
  mergedStats_ = NULL;
  
 char *envVar = getenv("RECLAIM_FREE_MEMORY_RATIO");
 if (envVar)
   stmtReclaimMinPctFree_ = atol(envVar);
 else
   stmtReclaimMinPctFree_ =25;
 envVar = getenv("ESP_RELEASE_TIMEOUT");
 if (envVar)
  espReleaseTimeout_ = atol(envVar);
 else
  espReleaseTimeout_ = 5;
  processStats_ = cliGlobals_->getExProcessStats();
  //if we are in ESP, this espManager_ has to be NULL, see "else" part
  if (cliGlobals_->isESPProcess())
     espManager_ = NULL;
  else
     espManager_ = new(ipcHeap_) ExEspManager(env_, cliGlobals_);
    
  udrServerManager_ = new (ipcHeap_) ExUdrServerManager(env_);

  ssmpManager_ = new(ipcHeap_) ExSsmpManager(env_);

  seqGen_ = new(exCollHeap()) SequenceValueGenerator(exCollHeap());

  hdfsHandleList_ = new(exCollHeap()) HashQueue(exCollHeap(), 50); // The hfsHandleList_ represents a list of distict hdfs Handles with unique hdfs port numbers and server names. Assume not more than 50 hdfsServers could be connected in the Trafodion setup.  These will get initialized the first time access is made to a particular hdfs server. This list gets cleaned up when the thread exits. 
  
}  


ContextCli::~ContextCli()
{
  NADELETE(cliSemaphore_, CLISemaphore, &exHeap_);  
}

void ContextCli::deleteMe()
{
  ComDiagsArea *diags = NULL;

  if (volatileSchemaCreated_)
    {
      // drop volatile schema, if one exists
      short rc =
        ExExeUtilCleanupVolatileTablesTcb::dropVolatileSchema
        (this, NULL, exCollHeap(), diags);
      if (rc < 0 && diags != NULL && diags->getNumber(DgSqlCode::ERROR_) > 0) {
         ComCondition *condition = diags->getErrorEntry(0);
         logAnMXEventForError(*condition, GetCliGlobals()->getEMSEventExperienceLevel()); 
      } 
      SQL_EXEC_ClearDiagnostics(NULL);
      volatileSchemaCreated_ = FALSE;
    }

  SQL_EXEC_ClearDiagnostics(NULL);

  delete moduleList_;
  statementList_->position();
  delete statementList_;
  delete descriptorList_;
  delete cursorList_;
  delete openStatementList_;
  // cannot use 'delete statementWithEspsList_' since it is not an NABasicObject.
  // Need to add a Queue::cleanup method that will deallocate all the local
  // members of Queue. Call that first and then call deallocateMemory.  TBD.
  exCollHeap()->deallocateMemory((void *)statementWithEspsList_);

  moduleList_ = 0;
  statementList_ = 0;
  descriptorList_ = 0;
  cursorList_ = 0;
  openStatementList_ = 0;
  statementWithEspsList_ = 0;
 
  delete transaction_;
  transaction_ = 0;

  stats_ = NULL;
  authIDType_   = SQLAUTHID_TYPE_INVALID;

  if (authID_)
    {
      exHeap()->deallocateMemory(authID_);
      authID_ = 0;
    }

  if (authToken_)
    {
      exHeap()->deallocateMemory(authToken_);
      authToken_ = 0;
    }

  // Release all ExUdrServer references
  releaseUdrServers();

  // delete all trusted routines in this context
  CollIndex maxTrustedRoutineIx = trustedRoutines_.getUsedLength();

  for (CollIndex rx=0; rx<maxTrustedRoutineIx; rx++)
    if (trustedRoutines_.used(rx))
      putTrustedRoutine(rx);
  trustedRoutines_.clear();

  closeStatementList_ = NULL;
  nextReclaimStatement_ = NULL;
  closeStmtListSize_ = 0;
  stmtReclaimCount_ = 0;
  currSequence_ = 0;
  prevFixupSequence_ = 0;

  clearTimeoutData() ; 

  // delete elements use for CmpContext switch
  for (short i = 0; i < cmpContextInfo_.entries(); i++)
    {
      CmpContextInfo *cci = cmpContextInfo_[i];
      cmpContextInfo_.remove(i);
      NAHeap *heap;
      CmpContext *cmpContext = cci->getCmpContext();
      heap = cmpContext->heap();
      // Set the current cmp Context as the one that is being deleted
      // because CmpContext destructor expects it that way
      cmpCurrentContext = cmpContext;
      NADELETE(cmpContext, CmpContext, heap); 
      NADELETE(heap, NAHeap, &exHeap_);
      NADELETE(cci, CmpContextInfo, &exHeap_);
      cmpCurrentContext = NULL;
    }

  // CLI context should only be destructed outside recursive CmpContext calls
  ex_assert(cmpContextInUse_.entries() == 0,
            "Should not be inside recursive compilation");

  for (short i = 0; i < arkcmpArray_.entries(); i++)
    delete arkcmpArray_[i];

  arkcmpArray_.clear();   

  arkcmpInitFailed_.clear();

  if (espManager_ != NULL)
     NADELETE(espManager_, ExEspManager, ipcHeap_);
  if (udrServerManager_ != NULL)
     NADELETE(udrServerManager_, ExUdrServerManager, ipcHeap_);
  if (ssmpManager_ != NULL)
     NADELETE(ssmpManager_, ExSsmpManager, ipcHeap_);

  if (exeTraceInfo_ != NULL)
  {
    delete exeTraceInfo_;
    exeTraceInfo_ = NULL;
  }
  NADELETE(env_, IpcEnvironment, ipcHeap_);
  NAHeap *parentHeap = cliGlobals_->getProcessIpcHeap();
  NADELETE(ipcHeap_, NAHeap, parentHeap);
  HiveClient_JNI::deleteInstance();
  disconnectHdfsConnections();
  delete hdfsHandleList_;
  hdfsHandleList_ = NULL;
}

Lng32 ContextCli::initializeSessionDefaults()
{
  Lng32 rc = 0;

  /* session attributes */
  sessionDefaults_ = new(exCollHeap()) SessionDefaults(exCollHeap());

  // read defaults table, if in master exe process
  if (NOT cliGlobals_->isESPProcess())
    {
      // should allow reading of global session defaults here
    }
  else
    {
      // ESP process. Set isoMapping using the define that was propagated
      // from master exe.
      char * imn = ComRtGetIsoMappingName();
      sessionDefaults_->setIsoMappingName(ComRtGetIsoMappingName(),
                                          (Lng32)strlen(ComRtGetIsoMappingName()));
    }

  return 0;
}


/* return -1, if module has already been added. 0, if not */
short ContextCli::moduleAdded(const SQLMODULE_ID * module_id)
{
  Module * module;
  Lng32 module_nm_len = getModNameLen(module_id);
  
  moduleList()->position(module_id->module_name, module_nm_len);
  while (module = (Module *)moduleList()->getNext())
    {
      if ((module_nm_len == module->getModuleNameLen()) &&
          (str_cmp(module_id->module_name, 
                   module->getModuleName(),
                   module_nm_len) == 0) )
        return -1;  // found
    }
  // not found
  return 0;
}


// common for NT and NSK
static Int32 allocAndReadPos(ModuleOSFile &file,
                           char *&buf, Lng32 pos, Lng32 len,
                           NAMemory *heap, ComDiagsArea &diagsArea,
                           const char *mname)
{
  // allocate memory and read the requested section into it
  buf = (char *) (heap->allocateMemory((size_t) len));
  short countRead;
  Int32 retcode = file.readpos(buf, pos, len, countRead);
  
  if (retcode)
    diagsArea << DgSqlCode(-CLI_MODULEFILE_CORRUPTED) << DgString0(mname);
  return retcode;
}

RETCODE ContextCli::allocateDesc(SQLDESC_ID * descriptor_id, Lng32 max_entries)
{
  const char * hashData;
  ULng32 hashDataLength;

  if (descriptor_id->name_mode != desc_handle)
    {
      /* find if this descriptor exists. Return error, if so */
      if (getDescriptor(descriptor_id))
        {
          diagsArea_ << DgSqlCode(- CLI_DUPLICATE_DESC);
          return ERROR;
        }
    }
  
  Descriptor * descriptor = 
    new(exCollHeap()) Descriptor(descriptor_id, max_entries, this);

  if (descriptor_id->name_mode == desc_handle)
    {
      descriptor_id->handle = descriptor->getDescHandle();
      hashData = (char *) &descriptor_id->handle;
      hashDataLength = sizeof(descriptor_id->handle);
    }
  else
    {
      hashData = descriptor->getDescName()->identifier;
      hashDataLength = getIdLen(descriptor->getDescName());
    }
  
  descriptorList()->insert(hashData, hashDataLength, (void*)descriptor);
  return SUCCESS;
}

RETCODE ContextCli::deallocDesc(SQLDESC_ID * desc_id,
                                NABoolean deallocStaticDesc)
{
  
  Descriptor *desc = getDescriptor(desc_id);
  /* desc must exist and must have been allocated by call to AllocDesc*/
  if (!desc)
    {
      diagsArea_ << DgSqlCode(- CLI_DESC_NOT_EXISTS);
      return ERROR;
    }
  else if ((!desc->dynAllocated()) &&
           (NOT deallocStaticDesc))
    {
      diagsArea_ << DgSqlCode(- CLI_NOT_DYNAMIC_DESC)
                 << DgString0("deallocate");
      return ERROR;
    }

  // remove this descriptor from the descriptor list
  descriptorList()->remove(desc);
  
  // delete the descriptor itself
  delete desc;
  
  return SUCCESS;
}

/* returns the statement, if it exists. NULL(0), otherwise */
Descriptor *ContextCli::getDescriptor(SQLDESC_ID * descriptor_id)
{
    if (!descriptor_id)
        return 0;

    // NOTE: we don't bounds check the SQLDESC_ID that is passed in
    // although it may come from the user. However, we don't modify
    // any memory in it and don't return any information if the
    // descriptor id should point into PRIV memory.

    const SQLMODULE_ID* module1 = descriptor_id->module;

    SQLDESC_ID* desc1 = 0;
    const SQLMODULE_ID* module2 = 0;
    SQLDESC_ID* desc2 = 0;

    const char * hashData = NULL;
    ULng32 hashDataLength = 0;

    //
    // First, get the descriptor name into descName1...
    // Get it *outside* of the loop, since it's invariant.
    //
    switch (descriptor_id->name_mode)
    {
    case desc_handle:
      // no need for a name here!
      hashData = (char *) &(descriptor_id->handle);
      hashDataLength = sizeof(descriptor_id->handle);
      break;
      
    case desc_name:
      desc1 = descriptor_id;
      hashData = desc1->identifier;
      //hashDataLength = strlen(hashData);
      hashDataLength = getIdLen(desc1); 
      break;

    case desc_via_desc:
      {
        // descriptr_id->identifier is the name of the descriptor
        // containing the actual name of this statement/cursor

        // decode the value in the descriptor into the statement name
        //  NOTE: the returned value is dynamically allocated from
        //  exHeap() and will need to be deleted before returning.

        desc1 = (SQLDESC_ID *)Descriptor::GetNameViaDesc(descriptor_id,this,*exHeap());
        if (desc1)
        {
                hashData = desc1->identifier;
                hashDataLength = getIdLen(desc1);
        }
        else
         return 0;

      }
    
    break;

    default:
      // must be an error
      return 0;
      break;
    }

    Descriptor * descriptor;
    descriptorList()->position(hashData, hashDataLength);
    while (descriptor = (Descriptor *)descriptorList()->getNext())
    {
      switch (descriptor_id->name_mode)
        {
        case desc_handle:
          if (descriptor_id->handle == descriptor->getDescHandle()) {
            return descriptor;
          }
          break;

        case desc_name:
        case desc_via_desc:

          module2 = descriptor->getModuleId();
          desc2 = descriptor->getDescName();

          if ( isEqualByName(module1, module2) && 
               isEqualByName(desc1, desc2) )
            {
              if ((descriptor_id->name_mode == desc_via_desc) && desc1) {
                // fix memory leak (genesis case 10-981230-3244)
                // cause by not freeing desc1.
                if (desc1->identifier)
                {
                   exHeap()->deallocateMemory((char *)desc1->identifier);
                   setNameForId(desc1,0,0);
                } 
                exHeap()->deallocateMemory(desc1);
                desc1 = 0;
              }
              return descriptor;
            }
          break;
          
        default:
          // error
          break;
        }
    }

    // fix memory leak (genesis case 10-981230-3244) caused by not
    // freeing the desc1 returned by Descriptor::GetNameViaDesc(desc).
    if ((descriptor_id->name_mode == desc_via_desc) && desc1) {
      exHeap()->deallocateMemory(desc1);
    }

    return 0;
}
  
/* returns the statement, if it exists. NULL(0), otherwise */
Statement *ContextCli::getNextStatement(SQLSTMT_ID * statement_id,
                                        NABoolean position,
                                        NABoolean advance)
{
  if ((! statement_id) || (!statement_id->module))
    return 0;

  Statement * statement;

  HashQueue * stmtList = statementList();

  if (position)
    stmtList->position();

  const SQLMODULE_ID* module1 = statement_id->module;
  const SQLMODULE_ID* module2 = 0;

  while (statement = (Statement *)stmtList->getCurr())
    {
      switch (statement_id->name_mode)
        {
        case stmt_name:
        case cursor_name:
          {
            module2 = statement->getModuleId();
            
            if (isEqualByName(module1, module2))
              {
                if (advance)
                  stmtList->advance();

                return statement;
              }
            
          }
        break;

        default:
          return 0;
        }

      stmtList->advance();
    }
      
  return 0;
}

/* returns the statement, if it exists. NULL(0), otherwise */
Statement *ContextCli::getStatement(SQLSTMT_ID * statement_id, HashQueue * stmtList)
{
    if (!statement_id || !(statement_id->module))
        return 0;

    // NOTE: we don't bounds check the SQLSTMT_ID that is passed in
    // although it may come from the user. However, we don't modify
    // any memory in it and don't return any information if the
    // descriptor id should point into PRIV memory.

    if (ComMemoryDiags::DumpMemoryInfo())
    {
      dumpStatementAndGlobalInfo(ComMemoryDiags::DumpMemoryInfo());
      delete ComMemoryDiags::DumpMemoryInfo();
      ComMemoryDiags::DumpMemoryInfo() = NULL; // doit once per statement only
    }
    const SQLMODULE_ID* module1 = statement_id->module;
    SQLSTMT_ID* stmt1 = 0;
    const SQLMODULE_ID* module2 = 0;
    SQLSTMT_ID* stmt2 = 0;

    // the following two variables are used to hash into
    // the statmentlist
    const char * hashData = NULL;
    ULng32 hashDataLength = 0;

    //
    // First, get the statement name.
    // Get it *outside* of the loop, since it's invariant.
    //
    switch (statement_id->name_mode)
    {
        case stmt_handle:
          // no need for a name here!
          hashData = (char *) &(statement_id->handle);
          hashDataLength = sizeof(statement_id->handle);
          break;

        case stmt_name:
        case cursor_name:
          stmt1 = statement_id;

          hashData = stmt1->identifier;
          hashDataLength = getIdLen(stmt1);
          break;

        case stmt_via_desc:
        case curs_via_desc:
          {
            // statement_id->identifier is the name of the descriptor
            // containing the actual name of this statement/cursor
            SQLDESC_ID desc_id;
            init_SQLCLI_OBJ_ID(&desc_id);

            desc_id.name_mode  = desc_via_desc;
            desc_id.identifier = statement_id->identifier;
            desc_id.identifier_len = getIdLen(statement_id);
            desc_id.module     = statement_id->module;


            // decode the value in the descriptor into the statement name
            //  the returned value is dynamically allocated, it will need
            //  to be deleted before returning.
            stmt1 = Descriptor::GetNameViaDesc(&desc_id,this,*exHeap());
            if (stmt1)
             {
               hashData = stmt1 -> identifier;
               hashDataLength = getIdLen(stmt1);
             }
            else
                return 0;

          }
        break;

      default:
        // must be an error
      break;
    }

    // if the input param stmtList is NULL, we do the lookup from
    // statementList() or cursorList(). Otherwise we lookup stmtList.
    if (!stmtList)
      if ((statement_id->name_mode == cursor_name) ||
          (statement_id->name_mode == curs_via_desc))
        stmtList = cursorList();
      else
        stmtList = statementList();

    Statement * statement;

    stmtList->position(hashData, hashDataLength);
    while (statement = (Statement *)stmtList->getNext())
    {
        switch (statement_id->name_mode)
        {
        case stmt_handle:
          if (statement_id->handle == statement->getStmtHandle())
          {
            return statement;
          }
          break;

        case stmt_name:
        case cursor_name:
          if (statement_id->name_mode == cursor_name)
            stmt2 = statement->getCursorName();
          else
            stmt2 = statement->getStmtId();
	  
	  if (stmt2 != NULL && 
	      (stmt2->name_mode != stmt_handle))
	  {
            module2 = statement->getModuleId();
            if (isEqualByName(module1, module2) && isEqualByName(stmt1, stmt2))
              {
                return statement;
              }
	  }
          break;

        case stmt_via_desc:
        case curs_via_desc:

          if (statement_id->name_mode == curs_via_desc)
            stmt2 = statement->getCursorName();
          else
            stmt2 = statement->getStmtId();

	  if (stmt2 != NULL && 
	      (stmt2->name_mode != stmt_handle))
	  {
            module2 = statement->getModuleId();
            if ( isEqualByName(module1, module2) &&
                 isEqualByName(stmt1, stmt2))
             {
               if (stmt1) {
                 if (stmt1->identifier) {
                   exHeap()->deallocateMemory((char *)stmt1->identifier);
                   setNameForId(stmt1, 0, 0);
                 }
                 // fix memory leak (genesis case 10-981230-3244) 
                 // caused by not freeing stmt1.
                 exHeap()->deallocateMemory(stmt1);
               }
               return statement;
             }
	  }
          break;

        default:
          break;
        }
    }

    // fix memory leak (genesis case 10-981230-3244) caused by not
    // freeing the stmt1 returned by Descriptor::GetNameViaDesc(desc).
    if (((statement_id->name_mode == stmt_via_desc) ||
         (statement_id->name_mode == curs_via_desc)) && stmt1) {
      if (stmt1->identifier) {
        exHeap()->deallocateMemory((char *)stmt1->identifier);
        setNameForId(stmt1, 0, 0);
      }
      exHeap()->deallocateMemory(stmt1);
    }

    return 0;
}

RETCODE ContextCli::allocateStmt(SQLSTMT_ID * statement_id,
                                 Statement::StatementType stmt_type,
                                 SQLSTMT_ID * cloned_from_stmt_id)
{
  const char * hashData = NULL;
  ULng32 hashDataLength = 0;

  if (statement_id->name_mode != stmt_handle)
    {
      /* find if this statement exists. Return error, if so */
      if (getStatement(statement_id))
        {
          diagsArea_ << DgSqlCode(- CLI_DUPLICATE_STMT);
          return ERROR;
        }
    }

  // when 'cloning' a statement, we are in the following situation
  //  a) we are doing an ALLOCATE CURSOR - extended dynamic SQL
  //  b) we are binding to that statement - right now!
  //  c) that statement must already exist and be in a prepared state...
  //
  // there is another way to 'bind' a statement and a cursor
  // besides cloning.  By 'bind'ing a cursor to a statement, I mean that
  // cursor has to acquire a root_tdb from a prepared statement, and then
  // has to build its own tcb tree from the tdb tree.
  //
  // in the ordinary dynamic SQL case - DECLARE CURSOR C1 FOR S1;
  // S1 will not have been prepared initially.  Binding will not
  // take place until the cursor is OPENed.
  //
  // See Statement::execute() and for details of this type of
  // binding.
  //
  Statement * cloned_from_stmt = 0;

  if (cloned_from_stmt_id)
    {
      cloned_from_stmt = getStatement(cloned_from_stmt_id);
      /* statement we're cloning from must exists. Return error, if not */
      if (!cloned_from_stmt)
        {
          diagsArea_ << DgSqlCode(-CLI_STMT_NOT_EXISTS);
          return ERROR;
        }
    }
  
  Statement * statement = 
    new(exCollHeap()) Statement(statement_id, cliGlobals_, stmt_type, NULL, NULL);

  if (cloned_from_stmt)
    statement->bindTo(cloned_from_stmt);

  if (statement_id->name_mode == stmt_handle) {
    statement_id->handle = statement->getStmtHandle();
    hashData = (char *) &(statement_id->handle);
    hashDataLength = sizeof(statement_id->handle);
  }
  else {
     hashData = statement->getIdentifier();
     hashDataLength = getIdLen(statement->getStmtId());
  }
  
  StmtListDebug1(statement, "Adding %p to statementList_", statement);

  semaphoreLock();
  if (hashData)
    statementList()->insert(hashData, hashDataLength, (void *) statement);
  if (processStats_!= NULL)
     processStats_->incStmtCount(stmt_type);
  semaphoreRelease();

  return SUCCESS;
}

RETCODE ContextCli::deallocStmt(SQLSTMT_ID * statement_id,
                                NABoolean deallocStaticStmt)
{
  // for now, dump memory info before deleting the statement
#if defined(_DEBUG)
  char * filename = getenv("MEMDUMP");
  if (filename) {
    // open output file sytream.
    ofstream outstream(filename);
    cliGlobals_->getExecutorMemory()->dump(&outstream, 0);
    // also dump the memory map
    outstream.flush();
    outstream.close();
  };
#endif

  Statement *stmt = getStatement(statement_id);
  // Check if the name modes match
  /* stmt must exist and must have been allocated by call to AllocStmt*/
  if (!stmt)
    {
      diagsArea_ << DgSqlCode(- CLI_STMT_NOT_EXISTS);
      return ERROR;
    }
  else if ((!stmt->allocated()) &&
           (NOT deallocStaticStmt))
    {
      diagsArea_ << DgSqlCode(- CLI_NOT_DYNAMIC_STMT)
                 << DgString0("deallocate");
      return ERROR;
    }
   else if (((stmt->getStmtId())->name_mode == cursor_name) &&
            (NOT deallocStaticStmt))
     {
       // This is really a cloned statment entry
       // Don't deallocate this since there is a cursor entry
       // in the cursor list
       diagsArea_ << DgSqlCode(- CLI_STMT_NOT_EXISTS);
       return ERROR;
     }    


  // if this is part of an auto query retry and esp need to be cleaned up,
  // set that indication in root tcb.
  if (aqrInfo() && aqrInfo()->espCleanup())
    {
      stmt->getGlobals()->setVerifyESP();
      if (stmt->getRootTcb())
        stmt->getRootTcb()->setFatalError();
    }

  if (stmt->stmtInfo() != NULL &&
      stmt->stmtInfo()->statement() == stmt)
  {
    if (statement_id->name_mode != desc_handle &&
        (StatementInfo *)statement_id->handle == stmt->stmtInfo())
      statement_id->handle = 0;
    exHeap_.deallocateMemory((void *)stmt->stmtInfo());
  }
  
  if ((stmt->getStmtStats()) && (stmt->getStmtStats()->getMasterStats()))
    {
      stmt->getStmtStats()->getMasterStats()->
        setFreeupStartTime(NA_JulianTimestamp());
    }

  // Remove statement from statementList
  // Update stmtGlobals transid with current transid.
  // The stmtGlobals transid is used in sending release msg to remote esps.
  // This function can be called by CliGlobals::closeAllOpenCursors 
  // when ExTransaction::inheritTransaction detects the context transid
  // is not the same as the current transid.  The context transid can be
  // obsolete if the transaction has been committed/aborted by the user.
  Int64 transid;
  if (!getTransaction()->getCurrentXnId(&transid))
    stmt->getGlobals()->castToExExeStmtGlobals()->getTransid() = transid;
  else
    stmt->getGlobals()->castToExExeStmtGlobals()->getTransid() = (Int64)-1;

  StmtListDebug1(stmt, "Removing %p from statementList_", stmt);

  semaphoreLock();
  statementList()->remove(stmt);
  if (processStats_!= NULL)
     processStats_->decStmtCount(stmt->getStatementType());
  semaphoreRelease();
  
  // the statement might have a cursorName. If it has, it is also in the
  // cursorList_. Remove it from this list;
  removeCursor(stmt->getCursorName(), stmt->getModuleId());
  
  // the statement might be on the openStatementList. Remove it.
  removeFromOpenStatementList(statement_id);
  removeFromCloseStatementList(stmt, FALSE);

  // remove any cloned statements.
  Queue * clonedStatements = stmt->getClonedStatements();
  clonedStatements->position();
  Statement * clone = (Statement*)clonedStatements->getNext();
  for (; clone != NULL; clone = (Statement*)clonedStatements->getNext())
  {
    RETCODE cloneRetcode = cleanupChildStmt(clone);
    if (cloneRetcode == ERROR)
      return ERROR;
  }

  ExRsInfo *rsInfo = stmt->getResultSetInfo();
  if (rsInfo)
  {
    ULng32 numChildren = rsInfo->getNumEntries();
    for (ULng32 i = 1; i <= numChildren; i++)
    {
      Statement *rsChild = NULL;
      const char *proxySyntax;
      NABoolean open;
      NABoolean closed;
      NABoolean prepared;
      NABoolean found = rsInfo->getRsInfo(i, rsChild, proxySyntax,
                                          open, closed, prepared); 
      if (found && rsChild)
      {
        RETCODE proxyRetcode = cleanupChildStmt(rsChild);
        if (proxyRetcode == ERROR)
          return ERROR;
      }
    }
  }

  // if deallocStaticStmt is TRUE, then remove the default descriptors
  // from the descriptor list. The actual descriptor will be destructed
  // when the stmt is deleted.
  if (deallocStaticStmt)
    {
      Descriptor * desc = stmt->getDefaultDesc(SQLWHAT_INPUT_DESC);
      if (desc)
        {
          desc = getDescriptor(desc->getDescName());
          descriptorList()->remove(desc);
        }

      desc = stmt->getDefaultDesc(SQLWHAT_OUTPUT_DESC);
      if (desc)
        {
          desc = getDescriptor(desc->getDescName());
          descriptorList()->remove(desc);
        }
    }

  if ((stmt->getStmtStats()) && (stmt->getStmtStats()->getMasterStats()))
    {
      stmt->getStmtStats()->getMasterStats()->
        setFreeupEndTime(NA_JulianTimestamp());
    }

  // delete the statement itself. This will delete child statements
  // such as clones or stored procedure result sets too.
  delete stmt;

  // close all open cursors referencing this statement...
  // Not sure why this needs to be called
  closeAllCursorsFor(statement_id);

  return SUCCESS;
}

short ContextCli::commitTransaction()
{
  releaseAllTransactionalRequests();

  // now do the actual commit
  return transaction_->commitTransaction();
}

short ContextCli::releaseAllTransactionalRequests()
{
  // for all statements in the context that need a transaction,
  // make sure they have finished their outstanding I/Os
  Statement * statement;
  short result = SUCCESS;

  openStatementList()->position(); // to head of list
  statement = (Statement *)openStatementList()->getNext();
  while (statement)
    {
      if (statement->transactionReqd() || statement->getState() == Statement::PREPARE_)
        {
          if (statement->releaseTransaction())
            result = ERROR;
        }

      statement = (Statement *)openStatementList()->getNext();
    }
  return result;
}

void ContextCli::clearAllTransactionInfoInClosedStmt(Int64 transid)
{
  // for all closed statements in the context, clear transaction Id
  // kept in the statement global

    Statement * statement;

    statementList()->position();
    while (statement = (Statement *)statementList()->getNext())
    {
      if (statement->getState() == Statement::CLOSE_)
      {
        if (statement->getGlobals()->castToExExeStmtGlobals()->getTransid()
            == transid)
          statement->getGlobals()->castToExExeStmtGlobals()->getTransid() =
                                              (Int64) -1;
      }
    }
}

void ContextCli::closeAllCursors(enum CloseCursorType type, 
                                 enum closeTransactionType transType, 
                                 const Int64 executorXnId,
                                 NABoolean inRollback)
{
  // for all statements in the context close all of them that are open;
  // only the cursor(s) should be open at this point.

  // VO, Dec. 2005: if the executorXnId parameter is non-zero, we use that as the current
  // transid instead of asking TMF. This allows us to use the CLOSE_CURR_XN functionality
  // in cases where TMF cannot or will not return the current txn id. 
  // Currently used by ExTransaction::validateTransaction (executor/ex_transaction.cpp).

  Statement * statement;
  Statement **openStmtArray = NULL;
  Int32 i;
  Lng32 noOfStmts;
  Int64 currentXnId = executorXnId;

  if ((transType == CLOSE_CURR_XN) && (transaction_->xnInProgress()) && (executorXnId == 0))
  {
    short retcode = transaction_->getCurrentXnId((Int64 *)&currentXnId);
    // Use the transaction id from context if the curret
    // transaction is already closed.
    if (retcode == 78 || retcode == 75)
      currentXnId = transaction_->getExeXnId();
  }

  // Get the open Statements in a array
  // This will avoid repositioning the hashqueue to the begining of openStatementList
  // when the cursor is closed and also avoid issuing the releaseTransaction
  // repeatedly when the hashqueue is repositioned
  noOfStmts = openStatementList_->numEntries();
  if (noOfStmts > 0)
  {
    openStmtArray = new (exHeap()) Statement *[noOfStmts];
    openStatementList_->position();
    for (i = 0; (statement = (Statement *)openStatementList_->getNext()) != NULL && i < noOfStmts; i++)
    {
      openStmtArray[i] = statement;
    }
    noOfStmts = i;
  }
  NABoolean gotStatusTrans = FALSE;
  NABoolean isTransActive = FALSE;
  short rc;
  short status;
  for (i = 0; isTransActive == FALSE && i < noOfStmts ; i++)
  {
    statement = openStmtArray[i];
    Statement::State st = statement->getState();
    
    switch ( st )
      {
      case Statement::OPEN_:
      case Statement::FETCH_:         
      case Statement::EOF_:
      case Statement::RELEASE_TRANS_:
        {
          NABoolean closeStmt = FALSE;
          NABoolean releaseStmt = FALSE;
        
          switch (transType)
          {
          case CLOSE_CURR_XN:
            if (currentXnId == statement->getGlobals()->castToExExeStmtGlobals()->getTransid())
            {
              checkIfNeedToClose(statement, type, closeStmt, releaseStmt);
            }
            break;
          case CLOSE_ENDED_XN:
            // Close statement if Statement TCBREF matches with context TCBREF
            // or if Statement TCBREF is -1 (for browse access)
            // There is a still problem that we could end up closing
            // browse access cursors that belong to a different transaction
            // in the context or browse access cursors that are never opened in a 
            // transaction. STATUSTRANSACTION slows down the response time
            // and hence avoid calling it more than necessary.
            // If the transaction is active(not aborted or committed), end the loop
            currentXnId = statement->getGlobals()->castToExExeStmtGlobals()->getTransid();
            if (currentXnId == transaction_->getExeXnId() || currentXnId == -1)
            {
              if (! gotStatusTrans)
              {
                  rc = STATUSTRANSACTION(&status, transaction_->getTransid());
                  if (rc == 0 && status != 3 && status != 4 && status != 5)
                    {
                      isTransActive = TRUE;
                      break;
                    }
                  gotStatusTrans = TRUE;
              }
              checkIfNeedToClose(statement, type, closeStmt, releaseStmt);
            }
            break;
          case CLOSE_ALL_XN:
            checkIfNeedToClose(statement, type, closeStmt, releaseStmt);
            break;
          }
          if (closeStmt)
          {
            // only close those statements which were constructed in this
            // scope.
            if (statement->getCliLevel() == getNumOfCliCalls())
            {
              statement->close(diagsArea_, inRollback);            
            }
            // STATUSTRANSACTION slows down the response time
            // Browse access cursor that are started under a transaction
            // will have the transid set in its statement globals. These browse
            // access cursors will be closed and the others (ones that aren't)
            // started under transaction will not be closed
            if (currentXnId == transaction_->getExeXnId())
            {  }  // TMM: Do Nothing. 
          }
          else
          if (releaseStmt)
            statement->releaseTransaction(TRUE, TRUE, !closeStmt);
        }
        break;
      default:
        break;
      }
  }
  if (openStmtArray)
  {
    NADELETEBASIC(openStmtArray, exHeap());
  }
}

void ContextCli::checkIfNeedToClose(Statement *stmt, enum CloseCursorType type, NABoolean &closeStmt,
                                    NABoolean &releaseStmt)
{
  closeStmt = FALSE;
  releaseStmt = FALSE;
  
  if (stmt->isAnsiHoldable())
  {
    if (type == CLOSE_ALL_INCLUDING_ANSI_HOLDABLE
            || type == CLOSE_ALL_INCLUDING_ANSI_PUBSUB_HOLDABLE
            || type == CLOSE_ALL_INCLUDING_ANSI_PUBSUB_HOLDABLE_WHEN_CQD)
      closeStmt = TRUE;
    else
      releaseStmt = TRUE;
  }
  else
  if (stmt->isPubsubHoldable())
  {
    if (type == CLOSE_ALL_INCLUDING_PUBSUB_HOLDABLE
            || type == CLOSE_ALL_INCLUDING_ANSI_PUBSUB_HOLDABLE)
      closeStmt = TRUE;
    else if (type == CLOSE_ALL_INCLUDING_ANSI_PUBSUB_HOLDABLE_WHEN_CQD && 
      stmt->getRootTdb()->getPsholdCloseOnRollback())
        closeStmt = TRUE;
    else
      releaseStmt = TRUE;
  }
  else
    closeStmt = TRUE;
  return;
}

void ContextCli::closeAllCursorsFor(SQLSTMT_ID * statement_id)
{
  // for all statements in the context, close all of them for the
  // given statement id...
  
  Statement * statement;
  
  statementList()->position();
  while (statement = (Statement *)statementList()->getNext())
    {
      if (statement->getCursorName() &&
          statement->getIdentifier() &&
          statement_id->identifier &&
          !strcmp(statement->getIdentifier(), statement_id->identifier) &&
          ((statement_id->module &&
            statement_id->module->module_name &&
            statement->getModuleId()->module_name &&
            isEqualByName(statement_id->module, statement->getModuleId())) ||
           ((statement_id->module == NULL) &&
            (statement->getModuleId()->module_name == NULL))))
        {
          statement->close(diagsArea_);
        }
    }
}



#pragma page "ContextCli::setAuthID"
// *****************************************************************************
// *                                                                           *
// * Function: ContextCli::setAuthID                                           *
// *                                                                           *
// *    Authenticates a user on a Trafodion database instance.                 *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <externalUsername>              const char *                    In       *
// *    is the external username of the user that was authenticated.           *
// *                                                                           *
// *  <databaseUsername>              const char *                    In       *
// *    is the database username of the user that was authenticated.           *
// *                                                                           *
// *  <authToken>                     const char *                    In       *
// *    is the token generated for this user session.                          *
// *                                                                           *
// *  <authTokenLen>                  Int32                           In       *
// *    is the length of the token in bytes.                                   *
// *                                                                           *
// *  <effectiveUserID>               Int32                           In       *
// *    is the user ID to use in authorization decisions.                      *
// *                                                                           *
// *  <sessionUserID>                 Int32                           In       *
// *    is the user ID to record for accountability.                           *
// *                                                                           *
// *****************************************************************************

Lng32 ContextCli::setAuthID(
   const char * externalUsername,
   const char * databaseUsername,
   const char * authToken,
   Int32        authTokenLen,
   Int32        effectiveUserID,
   Int32        sessionUserID)
   
{
   validAuthID_ = FALSE;  // Gets to TRUE only if a valid user found.

// Note authID being NULL is only to support mxci processes and embedded apps
   if (externalUsername == NULL)
   {
      // for now, in open source make the root user (DB__ROOT) the default
      Int32 userAsInt =  ComUser::getRootUserID();
      const char *rootUserName = ComUser::getRootUserName();
      
      if (externalUsername_)
         exHeap()->deallocateMemory(externalUsername_);
      externalUsername_ = (char *)
        exHeap()->allocateMemory(strlen(rootUserName) + 1);
      strcpy(externalUsername_, rootUserName);

      completeSetAuthID(userAsInt,
                        userAsInt,
                        rootUserName,
                        NULL,0,
                        rootUserName,
                        false,
                        true,
                        false,
                        true,
                        false);
      // allow setOnce cqds.
      setSqlParserFlags(0x400000);
      return SUCCESS;
   }
   
// Mainline of function, we have a real username.   
bool recreateMXCMP = false;
bool resetCQDs = true;
  
bool eraseCQDs = false;
bool dropVolatileSchema = false;

// Get the external user name
char logonUserName[ComSqlId::MAX_LDAP_USER_NAME_LEN + 1];

   strcpy(logonUserName,externalUsername);

   if (externalUsername_)
      exHeap()->deallocateMemory(externalUsername_);
      
   externalUsername_ = (char *)exHeap()->allocateMemory(strlen(logonUserName) + 1);
      
   strcpy(externalUsername_,logonUserName);
   
   if ((effectiveUserID != databaseUserID_) || (sessionUserID != sessionUserID_))
   {
      // user id has changed.
      // Drop any volatile schema created in this context
      // before switching the user.
      dropSession(TRUE); // dropSession() clears the this->sqlParserFlags_

      // close all tables that were opened by the previous user
      closeAllTables();
      userNameChanged_ = TRUE;
      eraseCQDs = true;
   }
   else 
      userNameChanged_ = FALSE;

   completeSetAuthID(effectiveUserID,
                     sessionUserID,
                     logonUserName,
                     authToken,
                     authTokenLen,
                     databaseUsername,
                     eraseCQDs,
                     recreateMXCMP,
                     true,
                     dropVolatileSchema,
                     resetCQDs);

   // allow setOnce cqds.
   setSqlParserFlags(0x400000); 
   return SUCCESS;

}
//*********************** End of ContextCli::setAuthID *************************






#pragma page "ContextCli::completeSetAuthID"
// *****************************************************************************
// *                                                                           *
// * Function: ContextCli::completeSetAuthID                                   *
// *                                                                           *
// *    Sets fields in CliContext related to authentication and takes actions  *
// * when user ID has changed.                                                 *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <userID>                        Int32                           In       *
// *    is the Seaquest assigned database user ID.                             *
// *                                                                           *
// *  <sessionID>                     Int32                           In       *
// *    is the Seaquest assigned database user ID.                             *
// *                                                                           *
// *  <logonUserName>                 const char *                    In       *
// *    is the name the user supplied at logon time.                           *
// *                                                                           *
// *  <authToken>                     const char *                    In       *
// *    is the credentials used for authenticating this user, specifically     *
// *  a valid token key.                                                       *
// *                                                                           *
// *  <authTokenLen>                  Int32                           In       *
// *    is the credentials used for authenticating this user, specifically     *
// *  a valid token key.                                                       *
// *                                                                           *
// *  <databaseUserName>              const char *                    In       *
// *    is the name assigned to the user at registration time.  This is the    *
// *  name that is used for privileges.                                        *
// *                                                                           *
// *  <eraseCQDs>                     bool                            In       *
// *    if true, all existing CQDs are erased.                                 *
// *                                                                           *
// *  <recreateMXCMP>                 bool                            In       *
// *    if true, any existing MXCMP is restarted.                              *
// *                                                                           *
// *  <releaseUDRServers>             bool                            In       *
// *    if true, any existing UDR servers are released.                        *
// *                                                                           *
// *  <dropVolatileSchema>            bool                            In       *
// *    if true, any existing volatile schemas are dropped.                    *
// *                                                                           *
// *  <resetCQDs>                     bool                            In       *
// *    if true, all existing CQDs are reset.                                  *
// *                                                                           *
// *****************************************************************************
void ContextCli::completeSetAuthID(
   Int32        userID,
   Int32        sessionUserID,
   const char  *logonUserName,
   const char  *authToken,
   Int32        authTokenLen,
   const char  *databaseUserName,
   bool         eraseCQDs,
   bool         recreateMXCMP,
   bool         releaseUDRServers,
   bool         dropVolatileSchema,
   bool         resetCQDs)

{

   authIDType_ = SQLAUTHID_TYPE_ASCII_USERNAME;
   if (authID_)
      exHeap()->deallocateMemory(authID_);
   authID_ = (char *)exHeap()->allocateMemory(strlen(logonUserName) + 1);
   strcpy(authID_,logonUserName);

   if (authToken_)
   {
      exHeap()->deallocateMemory(authToken_);
      authToken_ = 0;
   }

   if (authToken)
   {
      authToken_ = (char *)exHeap()->allocateMemory(authTokenLen + 1);
      strcpy(authToken_,authToken);
   }

   validAuthID_ = TRUE;

   if (databaseUserName_)
   {
      strncpy(databaseUserName_, databaseUserName, MAX_DBUSERNAME_LEN);
      databaseUserName_[MAX_DBUSERNAME_LEN] = 0;
   }

   if ((dropVolatileSchema) ||
      (userID != databaseUserID_) ||
      (sessionUserID != sessionUserID_) ||
      (recreateMXCMP))
   {
      // user id has changed.
      // Drop any volatile schema created in this context.
     // Also kill and restart child mxcmp and compiler context.
     if ((userID != databaseUserID_) ||
         (sessionUserID != sessionUserID_) ||
         (recreateMXCMP))
       dropSession(TRUE); // remove compiler context cache
     else
       dropSession();
   }

   if (releaseUDRServers)
   {
      // Release all ExUdrServer references
      releaseUdrServers();
   }

   if (resetCQDs)
   {
      ExeCliInterface *cliInterface = new (exHeap()) ExeCliInterface(exHeap(),
                                                                     SQLCHARSETCODE_UTF8
                                                                     ,this,NULL);   
      ComDiagsArea *tmpDiagsArea = &diagsArea_;
      cliInterface->executeImmediate((char *) "control query default * reset;", NULL, NULL, TRUE, NULL, 0, &tmpDiagsArea); 
   }
  
   if ((userID != databaseUserID_) ||
       (sessionUserID != sessionUserID_))
      recreateMXCMP = true;
   else 
      //don't need to recreate mxcmp if it's the same user
      if (!recreateMXCMP) 
         return;

   // save userid in context.
   databaseUserID_ = userID;

   // save sessionid in context.
   sessionUserID_ = sessionUserID;

// probably a better way than just deleting it, but it should work until
// we find a better way...

// Recreate MXCMP if previously connected and currently connected user id's 
// are different.
   if ( recreateMXCMP )
   {
      // reset rolelist in anticipation of the new user
      resetRoleList();

      // create all the caches
      CmpContextInfo *cmpCntxtInfo;
      for (int i = 0; i < cmpContextInfo_.entries(); i++)
      {
         cmpCntxtInfo = cmpContextInfo_[i];
         cmpCntxtInfo->getCmpContext()->clearAllCaches();
      }

      // clear caches in secondary arkcmps
      killAndRecreateMxcmp();
   }
 
   if (eraseCQDs)
   {
      if (controlArea_)
         NADELETE( controlArea_,ExControlArea, exCollHeap());
      controlArea_ = NULL;
      // create a fresh one 
      controlArea_ = new(exCollHeap()) ExControlArea(this, exCollHeap()); 
   }

}
//******************* End of ContextCli::completeSetAuthID *********************


Lng32 ContextCli::boundsCheckMemory(void *startAddress,
                                   ULng32 length)
{
  Lng32 retcode = 0;

  if (cliGlobals_->boundsCheck(startAddress, length, retcode))
    diagsArea_ << DgSqlCode(retcode);
  return retcode;
}

void 
ContextCli::removeCursor(SQLSTMT_ID* cursorName, 
                         const SQLMODULE_ID* moduleId) 
{
  if (!cursorName)
    return;

  Statement * statement;
  cursorList()->position(cursorName->identifier, getIdLen(cursorName));

  while (statement = (Statement *)cursorList()->getNext()) {
    if (isEqualByName(cursorName, statement->getCursorName()) 
        &&
        isEqualByName(statement->getModuleId(), moduleId)
       )
    {
      StmtListDebug1(statement, "Removing %p from cursorList_", statement);
      cursorList()->remove(statement);
      return;
    }
  }
}

void ContextCli::addToOpenStatementList(SQLSTMT_ID * statement_id,
                                        Statement * statement) {

  const char * hashData = NULL;
  ULng32 hashDataLength = 0;

  if (statement_id->name_mode == stmt_handle) {
    hashData = (char *) &(statement_id->handle);
    hashDataLength = sizeof(statement_id->handle);
  }
  else {
    hashData = statement->getIdentifier();
    //hashDataLength = strlen(hashData);
    hashDataLength = getIdLen(statement->getStmtId());
  }
  
  StmtListDebug1(statement, "Adding %p to openStatementList_", statement);
  openStatementList()->insert(hashData, hashDataLength, (void *) statement);
  if (processStats_ != NULL)
      processStats_->incOpenStmtCount();
  removeFromCloseStatementList(statement, TRUE/*forOpen*/);
}

void ContextCli::removeFromOpenStatementList(SQLSTMT_ID * statement_id) {

  Statement * statement = getStatement(statement_id, openStatementList());
  if (statement)
    {
      // Make sure we complete all outstanding nowaited transactional
      // ESP messages to the statement. This is so we need only check
      // the open statements for such messages at commit time.
      if (statement->transactionReqd())
        statement->releaseTransaction();

      StmtListDebug1(statement, "Removing %p from openStatementList_",
                     statement);
      openStatementList()->remove(statement);
      if (processStats_ != NULL)
         processStats_->decOpenStmtCount();
      addToCloseStatementList(statement);
    }
}

// Add to closeStatementList_ and reclaim other statements if possible.
// [EL]
void ContextCli::addToCloseStatementList(Statement * statement)
{
  // Get the Heap usage at the Statement Level
  size_t lastBlockSize;
  size_t freeSize;
  size_t totalSize;
  NAHeap *stmtHeap = statement->stmtHeap();
  NABoolean crowded = stmtHeap->getUsage(&lastBlockSize, &freeSize, &totalSize);

  if ((((statementList_->numEntries() < 50) &&
       (!crowded || freeSize > totalSize / (100 / stmtReclaimMinPctFree_))) &&
      closeStatementList_ == NULL) ||
      statement->closeSequence() > 0)
    return;


  // Add to the head of closeStatementList.
  statement->nextCloseStatement() = closeStatementList_;
  statement->prevCloseStatement() = NULL;
  incCloseStmtListSize();

  // Update the head of the list.
  if (closeStatementList_ != NULL)
    closeStatementList_->prevCloseStatement() = statement;
  closeStatementList_ = statement;

  StmtListDebug1(statement,
                 "Stmt %p is now at the head of closeStatementList_",
                 statement);

  if (nextReclaimStatement_ == NULL)
    nextReclaimStatement_ = statement;
  
  // Rewind the sequence counter to avoid overflow.
  if (currSequence_ == INT_MAX)
    {
      // Re-number closeSequence numbers.
      Lng32 i = 2;
      Statement *st2, *st = nextReclaimStatement_;
      if (st != NULL)
        st2 = st->nextCloseStatement();
      else
        st2 = closeStatementList_;
      
      while (st2 != NULL)
        { // Set to 1 for statements older than nextReclaimStatement_.
          st2->closeSequence() = 1;
          st2 = st2->nextCloseStatement();
        }
      while (st != NULL)
        { // Set to consecutive numbers 3, 4, ... 
          st->closeSequence() = ++i;
          st = st->prevCloseStatement();
        }
      // Set to the largest closeSequence.
      currSequence_ = i;
    }

  // Set the closeSequence_ number.
  if (statement->closeSequence() < 0)
    // This is the first time close of the dynamic statement. 
    statement->closeSequence() = currSequence_;
  else
    statement->closeSequence() = ++currSequence_;
}

// True if OK to redrive.  False if no more statements to reclaim.
NABoolean ContextCli::reclaimStatements() 
{
  NABoolean someStmtReclaimed = FALSE;
  
  if (nextReclaimStatement_ == NULL)
    return FALSE;
  

#if defined(_DEBUG)
  char *reclaimAfter = NULL;
  Lng32 reclaimIf = (reclaimAfter == NULL)? 0: atol(reclaimAfter);

  if (reclaimIf > 0 && reclaimIf < closeStmtListSize_)
    {
    someStmtReclaimed = reclaimStatementSpace();
    return someStmtReclaimed;
    }
#endif
  // Get the Heap usage at the Context Level
  size_t lastBlockSize;
  size_t freeSize;
  size_t totalSize;
  NAHeap *heap = exHeap(); // Get the context Heap
  NABoolean crowded = heap->getUsage(&lastBlockSize, &freeSize, &totalSize);
  Lng32 reclaimTotalMemorySize = getSessionDefaults()->getReclaimTotalMemorySize();
  Lng32 reclaimFreeMemoryRatio;
  double freeRatio;
  NABoolean retcode;
  if (((Lng32)totalSize) > reclaimTotalMemorySize)
  {
    reclaimFreeMemoryRatio = getSessionDefaults()->getReclaimFreeMemoryRatio();
    retcode = TRUE;
    freeRatio = (double)(freeSize) * 100 / (double)totalSize;
    while (retcode && 
      ((Lng32)totalSize) > reclaimTotalMemorySize &&
      freeRatio < reclaimFreeMemoryRatio) 
    {
      retcode = reclaimStatementSpace();
      if (retcode)
        someStmtReclaimed = TRUE;
      crowded = heap->getUsage(&lastBlockSize, &freeSize, &totalSize);
      freeRatio = (double)(freeSize) * 100 / (double)totalSize;
    }
  }
  else 
  // We have observed that we are reclaiming more aggressively than
  // needed due to the conditions below. Hence, it is now controlled
  // via a variable now
  if (getenv("SQL_RECLAIM_AGGRESSIVELY") != NULL)
  {
  // Reclaim space from one other statement under certain conditions.
     Lng32  cutoff = MINOF(closeStmtListSize_ * 8, 800);
  // Reclaim if
  //  heap is crowded or
  //  less than 25% of the heap is free and
  //  nextReclaimStatement_ is closed earlier than cutoff
  //  or the recent fixup of a closed statement occurred earlier than
  //  the past 50 sequences.
  //  This is to prevent excessive fixup due to statement reclaim.
     if (crowded ||
         freeSize <= (totalSize / (100 / stmtReclaimMinPctFree_)))
     {
         if (((currSequence_ - 
            nextReclaimStatement_->closeSequence()) >= cutoff) ||
          ((currSequence_ - prevFixupSequence_) >= 50))
         {
            retcode = reclaimStatementSpace();
            if (retcode)
             someStmtReclaimed = TRUE;
         }
     }
  }
  if (someStmtReclaimed)
  {
    logReclaimEvent((Lng32)freeSize,(Lng32)totalSize);
  }
  return someStmtReclaimed;
}

// True if OK to redrive.  False if no more statements to reclaim.
NABoolean ContextCli::reclaimStatementSpace() 
{
  while (nextReclaimStatement_ != NULL && (! nextReclaimStatement_->isReclaimable()))
    nextReclaimStatement_ = nextReclaimStatement_->prevCloseStatement();
  if (nextReclaimStatement_ == NULL)
    return FALSE;
  nextReclaimStatement_->releaseSpace();        
  nextReclaimStatement_->closeSequence() -= 1;
  assert(nextReclaimStatement_->closeSequence() > 0);
  incStmtReclaimCount();
  nextReclaimStatement_ = nextReclaimStatement_->prevCloseStatement();
  return TRUE;
}

// Called at open or dealloc.
// [EL]
void ContextCli::removeFromCloseStatementList(Statement * statement,
                                              NABoolean forOpen) {
  
  if (statement->closeSequence() <= 0)
    return;

  // Relink the neighbors.
  if (statement->prevCloseStatement() != NULL)
    statement->prevCloseStatement()->nextCloseStatement() = 
                          statement->nextCloseStatement();
  if (statement->nextCloseStatement() != NULL)
    statement->nextCloseStatement()->prevCloseStatement() =   
                          statement->prevCloseStatement();

  // Decrement counters for closeStmtList_. 
  decCloseStmtListSize();

  if (nextReclaimStatement_ == NULL ||
      statement->closeSequence() < nextReclaimStatement_->closeSequence())
    { // Indicates space has been reclaimed.
      // Then also decrement stmtReclaimCount_. 
      decStmtReclaimCount();
      if (forOpen) // fixup has occurred; record the current sequence.
        prevFixupSequence_ = currSequence_;
    } 
  
  // Move forward the head of closeStatementList_.
  if (statement == closeStatementList_)
    closeStatementList_ = statement->nextCloseStatement();
  // Move backward nextReclaimedStatement_.
  if (statement == nextReclaimStatement_)
    nextReclaimStatement_ = statement->prevCloseStatement();

  // Clear related fields.
  statement->prevCloseStatement() = NULL;
  statement->nextCloseStatement() = NULL;
  statement->closeSequence() = 0; 

  StmtListDebug1(statement,
                 "Stmt %p has been removed from closeStatementList_",
                 statement);
}

#if defined(_DEBUG)

void ContextCli::dumpStatementInfo()
{
  // open output "stmtInfo" file.
  ofstream outstream("stmtInfo");

  Statement * statement;
  statementList()->position();
  while (statement = (Statement *)statementList()->getNext()) {
    statement->dump(&outstream);
  };
  outstream.flush();
  outstream.close();
};
#endif

void ContextCli::dumpStatementAndGlobalInfo(ostream * outstream) 
{
  NAHeap* globalExecutorHeap = exHeap_.getParent();
  globalExecutorHeap->dump(outstream, 0);
  //exHeap_.dump(outstream, 0);

  Statement * statement;
  statementList()->position();

  while (statement = (Statement *)statementList()->getNext()) {
    statement->dump(outstream);
  };
};

unsigned short ContextCli::getCurrentDefineContext()
{
  unsigned short context_id = 0;

  return context_id;
}

NABoolean ContextCli::checkAndSetCurrentDefineContext()
{
  unsigned short context_id = getCurrentDefineContext();
  if (defineContext() != context_id)
    {
      defineContext() =  context_id;
      return TRUE;
    }
  else
    return FALSE;
}


TimeoutData * ContextCli::getTimeouts( NABoolean allocate )
{ 
  if ( NULL == timeouts_  && allocate ) {
    timeouts_ = new (&exHeap_) TimeoutData( &exHeap_ );
  }
  return timeouts_; 
} 

void ContextCli::clearTimeoutData()  // deallocate the TimeoutData 
{
  if ( timeouts_ ) {
    delete timeouts_ ;
    timeouts_ = NULL;
  }
}

void ContextCli::incrementTimeoutChangeCounter()
{
  timeoutChangeCounter_++ ; 
} 

UInt32 ContextCli::getTimeoutChangeCounter() 
{ 
  return timeoutChangeCounter_;
}



void ContextCli::reset(void *contextMsg)
{
  closeAllStatementsAndCursors();
/*  retrieveContextInfo(contextMsg); */
}

void ContextCli::closeAllStatementsAndCursors()
{
  // for all statements in the context close all of them that are open;
  Statement * statement;

  openStatementList()->position();
  while ((statement = (Statement *)openStatementList()->getNext()))
  {
        statement->close(diagsArea_);

        // statement->close() sets the state to close. As a side effect
        // the statement is removed from the openStatementList. Here
        // is the catch: Removing an entry from a hashQueue invalidates
        // the current_ in the HashQueue. Thus, we have to re-position
        
        openStatementList()->position();
  }
} 

// Method to acquire a reference to a UDR server associated with a
// given set of JVM startup options
ExUdrServer *ContextCli::acquireUdrServer(const char *runtimeOptions,
                                          const char *optionDelimiters,
                                          NABoolean dedicated)
{
  ExUdrServer *udrServer = NULL;
  CliGlobals *cliGlobals = getCliGlobals();
  ExUdrServerManager *udrManager = cliGlobals->getUdrServerManager();

  if (udrRuntimeOptions_)
  {
    runtimeOptions = udrRuntimeOptions_;
    optionDelimiters = (udrRuntimeOptionDelimiters_ ?
                        udrRuntimeOptionDelimiters_ : " ");
  }


  // First see if there is a matching server already servicing this
  // context
  udrServer = findUdrServer(runtimeOptions, optionDelimiters, dedicated);

  if (udrServer == NULL)
  {

    // Ask the UDR server manager for a matching server
    udrServer = udrManager->acquireUdrServer(databaseUserID_,
                                             runtimeOptions, optionDelimiters,
                                             authID_,
                                             authToken_,
                                             dedicated);

    ex_assert(udrServer, "Unexpected error while acquiring a UDR server");
    
    if (udrServerList_ == NULL)
    {
      // The UDR server list will typically be short so we can use a
      // small hash table size to save memory
      udrServerList_ = new (exCollHeap()) HashQueue(exCollHeap(), 23);
    }

    const char *hashData = runtimeOptions;
    ULng32 hashDataLength = str_len(runtimeOptions);
    udrServerList_->insert(hashData, hashDataLength, (void *) udrServer);
    
    if(dedicated)
    {
      udrServer->setDedicated(TRUE);

      // Note that the match logic used above in finding or acquiring a server
      // always looks for a server with reference count 0 to get a dedicated
      // server.  
      ex_assert(udrServer->getRefCount() == 1, "Dedicated server request failed with reference count > 1.");
    }
  }

  if(dedicated)
  {
    udrServer->setDedicated(TRUE);

    // Note that the match logic used above in finding or acquiring a server
    // always looks for a server with reference count 0 to get a dedicated
    // server. Set the reference count to 1 now. Inserting a server flag
    // as dedicated prevents any other client( whether scalar or tmudf) from 
    // sharing the server. 
    udrServer->setRefCount(1);
  }

  return udrServer;
}

// When a context is going away or is changing its user identity, we
// need to release all references on ExUdrServer instances by calling
// the ExUdrServerManager interface. Note that the ExUdrServerManager
// may not stop a UDR server process when the reference is
// released. The server may be servicing other contexts or the
// ExUdrServerManager may have its own internal reasons for retaining
// the server.
void ContextCli::releaseUdrServers()
{
  if (udrServerList_)
  {
    ExUdrServerManager *manager = getCliGlobals()->getUdrServerManager();
    ExUdrServer *s;

    udrServerList_->position();
    while ((s = (ExUdrServer *) udrServerList_->getNext()) != NULL)
    {
      manager->releaseUdrServer(s);
    }

    delete udrServerList_;
    udrServerList_ = NULL;
  }
}

// We support a method to dynamically set UDR runtime options
// (currently the only supported options are JVM startup
// options). Once UDR options have been set, those options take
// precedence over options that were compiled into a plan.
Lng32 ContextCli::setUdrRuntimeOptions(const char *options,
                                      ULng32 optionsLen,
                                      const char *delimiters,
                                      ULng32 delimsLen)
{
  // A NULL or empty option string tells us to clear the current UDR
  // options
  if (options == NULL || optionsLen == 0)
  {
    if (udrRuntimeOptions_)
    {
      exHeap()->deallocateMemory(udrRuntimeOptions_);
    }
    udrRuntimeOptions_ = NULL;

    if (udrRuntimeOptionDelimiters_)
    {
      exHeap()->deallocateMemory(udrRuntimeOptionDelimiters_);
    }
    udrRuntimeOptionDelimiters_ = NULL;

    return SUCCESS;
  }

  // A NULL or empty delimiter string gets changed to the default
  // single space delimiter string here
  if (delimiters == NULL || delimsLen == 0)
  {
    delimiters = " ";
    delimsLen = 1;
  }

  char *newOptions = (char *) exHeap()->allocateMemory(optionsLen + 1);
  char *newDelims = (char *) exHeap()->allocateMemory(delimsLen + 1);

  str_cpy_all(newOptions, options, optionsLen);
  newOptions[optionsLen] = 0;

  str_cpy_all(newDelims, delimiters, delimsLen);
  newDelims[delimsLen] = 0;

  if (udrRuntimeOptions_)
  {
    exHeap()->deallocateMemory(udrRuntimeOptions_);
  }
  udrRuntimeOptions_ = newOptions;

  if (udrRuntimeOptionDelimiters_)
  {
    exHeap()->deallocateMemory(udrRuntimeOptionDelimiters_);
  }
  udrRuntimeOptionDelimiters_ = newDelims;

  return SUCCESS;
}

// A lookup function to see if a UDR server with a given set of
// runtime options is already servicing this context
ExUdrServer *ContextCli::findUdrServer(const char *runtimeOptions,
                                       const char *optionDelimiters,
                                       NABoolean dedicated)
{
  if (udrServerList_)
  {
    const char *hashData = runtimeOptions;
    ULng32 hashDataLength = str_len(runtimeOptions);
    ExUdrServer *s;

    udrServerList_->position(hashData, hashDataLength);
    while ((s = (ExUdrServer *) udrServerList_->getNext()) != NULL)
    {
      //We are looking for servers that have been acquired from server manager
      //as dedicated. We don't want to mix shared and dedicated servers.
      if(dedicated != s->isDedicated())
      {
        continue;
      }

      //If dedicated server is being requested, make sure it is not being used within 
      //contextCli scope by others.
      if(dedicated && s->inUse())
      {
        continue;
      }

      //Also make sure other runtimeOptions match.
      if (s->match(databaseUserID_, runtimeOptions, optionDelimiters))
      {
        return s;
      }
    }
  }

  return NULL;
}
void ContextCli::logReclaimEvent(Lng32 freeSize, Lng32 totalSize)
{
  Lng32 totalContexts = 0;
  Lng32 totalStatements = 0;
  ContextCli *context;
  
  if (! cliGlobals_->logReclaimEventDone())
  {
    totalContexts = cliGlobals_->getContextList()->numEntries();
    cliGlobals_->getContextList()->position();
    while ((context = (ContextCli *)cliGlobals_->getContextList()->getNext()) != NULL)
        totalStatements += context->statementList()->numEntries();
    SQLMXLoggingArea::logCliReclaimSpaceEvent(freeSize, totalSize,
              totalContexts, totalStatements);
    cliGlobals_->setLogReclaimEventDone(TRUE);
  }
}
// The following new function sets the version of the compiler to be 
// used by the context. If the compiler had already been started, it
// uses the one in the context. If not, it starts one and  returns the 
// index into the compiler array for the compiler that has been 
// started.

Lng32 ContextCli::setOrStartCompiler(short mxcmpVersionToUse, char *remoteNodeName, short &indexIntoCompilerArray)
{
    
  short found = FALSE;
  short i = 0;
  Lng32 retcode = SUCCESS;
  
  if (mxcmpVersionToUse == COM_VERS_R2_FCS || mxcmpVersionToUse == COM_VERS_R2_1)
    // Use R2.2 compiler for R2.0 and R2.1 nodes
    mxcmpVersionToUse = COM_VERS_R2_2;

  for ( i = 0; i < getNumArkcmps(); i++)
    {
      
      if (remoteNodeName &&(str_cmp_ne(cliGlobals_->getArkcmp(i)->getNodeName(), remoteNodeName ) == 0))
        {
          setVersionOfMxcmp(mxcmpVersionToUse);
          setMxcmpNodeName(remoteNodeName);
          setIndexToCompilerArray(i);
          found = TRUE;
          break;
        }
      else
        if ( (cliGlobals_->getArkcmp(i))->getVersion()== mxcmpVersionToUse) 
        {
          setVersionOfMxcmp(mxcmpVersionToUse);
          setMxcmpNodeName(NULL);
          setIndexToCompilerArray(i);
          found = TRUE;
          break;
        }
    }
  if (!found)

    {
      // Create a new ExSqlComp and insert into the arrary
      // of Sqlcomps in the context.
    
        setArkcmp( new (exCollHeap())
                   ExSqlComp(0,
                             exCollHeap(),
                             cliGlobals_,0,
                             mxcmpVersionToUse,remoteNodeName, env_));

      setArkcmpFailMode(ContextCli::arkcmpIS_OK_);
      short numArkCmps = getNumArkcmps();
      setIndexToCompilerArray((short)(getNumArkcmps() - 1));                    
      setVersionOfMxcmp(mxcmpVersionToUse);
      setMxcmpNodeName(remoteNodeName);
      i = (short) (getNumArkcmps() - 1);
    }
  indexIntoCompilerArray = i;
  return retcode;
}

 
void ContextCli::addToStatementWithEspsList(Statement *statement)
{
  statementWithEspsList_->insert((void *)statement);
}

void ContextCli::removeFromStatementWithEspsList(Statement *statement)
{
  statementWithEspsList_->remove((void *)statement);
}

bool ContextCli::unassignEsps(bool force)
{
  bool didUnassign = false;
  Lng32 espAssignDepth = getSessionDefaults()->getEspAssignDepth();
  Statement *lastClosedStatement = NULL;

  if (force ||
      ((espAssignDepth != -1) &&
       (statementWithEspsList_->numEntries() >= espAssignDepth)))
  {
    statementWithEspsList_->position();
    Statement *lastStatement = (Statement *)statementWithEspsList_->getNext();
    while (lastStatement)
    {
      if ((lastStatement->getState() == Statement::CLOSE_) &&
          (lastStatement->isReclaimable()))
        lastClosedStatement = lastStatement;
      lastStatement = (Statement *)statementWithEspsList_->getNext();
    }
  }
  if (lastClosedStatement)
  {
    lastClosedStatement->releaseSpace();
    // Need to do same things that reclaimStatementSpace would do
    incStmtReclaimCount();
    if (lastClosedStatement == nextReclaimStatement_)
    {
      nextReclaimStatement_->closeSequence() -= 1;
      nextReclaimStatement_ = nextReclaimStatement_->prevCloseStatement();
    }
    statementWithEspsList_->remove((void *)lastClosedStatement);
    didUnassign = true;
  }
  return didUnassign;
}

void ContextCli::addToCursorList(Statement &s)
{
  SQLSTMT_ID *cursorName = s.getCursorName();
  const char *id = cursorName->identifier;
  Lng32 idLen = getIdLen(cursorName);

  StmtListDebug1(&s, "Adding %p to cursorList_", &s);
  cursorList_->insert(id, idLen, &s);
}

// Remove a child statement such as a clone or a stored procedure
// result set proxy from the context's statement lists. Do not call
// the child's destructor (that job is left to the parent
// statement's destructor).
RETCODE ContextCli::cleanupChildStmt(Statement *child)
{
  StmtListDebug1(child, "[BEGIN cleanupChildStmt] child %p", child);

  if (!child)
    return SUCCESS;

  // One goal in this method is to remove child from the
  // statementList() list. Instead of using a global search in
  // statementList() to locate and remove child, we build a hash key
  // first and do a hash-based lookup. The hash key is build from
  // components of child's SQLSTMT_ID structure.

  SQLSTMT_ID *statement_id = child->getStmtId();
  Statement *lookupResult = getStatement(statement_id, statementList());
  
  StmtListDebug1(child, "  lookupResult is %p", lookupResult);

  if (!lookupResult)
  {
    StmtListDebug0(child, "  *** lookupResult is NULL, returning ERROR");
    StmtListDebug1(child, "[END cleanupChildStmt] child %p", child);
    diagsArea_ << DgSqlCode(- CLI_STMT_NOT_EXISTS);
    return ERROR;
  }

  StmtListDebug1(lookupResult, "  Removing %p from statementList_",
                 lookupResult);
  semaphoreLock();
  statementList()->remove(lookupResult);
  if (processStats_!= NULL)
     processStats_->decStmtCount(lookupResult->getStatementType());
  semaphoreRelease();
  
  if (child->getStatementType() != Statement::STATIC_STMT)
    removeCursor(child->getCursorName(), child->getModuleId());
  
  removeFromCloseStatementList(child, FALSE);

  StmtListDebug1(child, "[END cleanupChildStmt] child %p", child);
  return SUCCESS;

} // ContextCLI::cleanupChildStmt()

#ifdef _DEBUG
void ContextCli::StmtListPrintf(const Statement *s,
                                const char *formatString, ...) const
{
  if (s)
  {
    if (s->stmtDebugEnabled() || s->stmtListDebugEnabled())
    {
      FILE *f = stdout;
      va_list args;
      va_start(args, formatString);
      fprintf(f, "[STMTLIST] ");
      vfprintf(f, formatString, args);
      fprintf(f, "\n");
      fflush(f);
    }
  }
}
#endif
void ContextCli::genSessionId()
{
  getCliGlobals()->genSessionUniqueNumber();
  char si[ComSqlId::MAX_SESSION_ID_LEN];


  // On Linux, database user names can potentially be too long to fit
  // into the session ID.
  // 
  // If the user ID is >= 0: The user identifier inside the session ID
  // will be "U<user-id>".
  //
  // If the user ID is less than zero: The identifier will be
  // "U_NEG_<absolute value of user-id>".
  //
  //  The U_NEG_ prefix is used, rather than putting a minus sign into
  // the string, so that the generated string is a valid regular
  // identifier.

  // The userID type is unsigned. To test for negative numbers we
  // have to first cast the value to a signed data type.
  Int32 localUserID = (Int32) databaseUserID_;

  char userName[32];
  if (localUserID >= 0)
    sprintf(userName, "U%d", (int) localUserID);
  else
    sprintf(userName, "U_NEG_%d", (int) -localUserID);


  Int32 userNameLen = strlen(userName);
  
  // generate a unique session ID and set it in context.
  Int32 sessionIdLen;
  ComSqlId::createSqlSessionId
    (si, 
     ComSqlId::MAX_SESSION_ID_LEN,
     sessionIdLen,
     getCliGlobals()->myNodeNumber(), 
     getCliGlobals()->myCpu(),        
     getCliGlobals()->myPin(),        
     getCliGlobals()->myStartTime(),  
     (Lng32)getCliGlobals()->getSessionUniqueNumber(),
     userNameLen,
     userName,
     (Lng32)strlen(userSpecifiedSessionName_),
     userSpecifiedSessionName_);

  setSessionId(si);
}

void ContextCli::createMxcmpSession()
{
  if ((mxcmpSessionInUse_) &&
      (NOT userNameChanged_))
    return;

  // Steps to perform
  // 1. Send the user details to mxcmp
  // 2. Send CQD's  to mxcmp
  // 3. Set the mxcmpSessionInUse_ flag to TRUE

  // Send the user details (auth state, userID, and username
  CmpContext *cmpCntxt = CmpCommon::context();
  ex_assert(cmpCntxt, "No compiler context exists");
  NABoolean authOn = cmpCntxt->isAuthorizationEnabled();

  // The message contains the following:
  //   (auth state and user ID are delimited by commas)
  //     authorization state (0 - off, 1 - on)
  //     integer user ID
  //     database user name
  // See CmpStatement::process (CmpMessageDatabaseUser) for more details
  Int32 userAsInt = (Int32) databaseUserID_;
  char userMessage [MAX_AUTHID_AS_STRING_LEN + 1 + MAX_USERNAME_LEN + 1 + 2];
  str_sprintf(userMessage, "%d,%d,%s", authOn, userAsInt, databaseUserName_);
  char *pMessage = (char *)&userMessage;


  Int32 cmpStatus = 2;  // assume failure
  if (getSessionDefaults()->callEmbeddedArkcmp() &&
      isEmbeddedArkcmpInitialized() &&
      CmpCommon::context() && (CmpCommon::context()->getRecursionLevel() == 0) )
    {
      char *dummyReply = NULL;
      ULng32 dummyLen;
      ComDiagsArea *diagsArea = NULL;
      cmpStatus = CmpCommon::context()->compileDirect(pMessage,
                               (ULng32) sizeof(userMessage), &exHeap_,
                               SQLCHARSETCODE_UTF8, EXSQLCOMP::DATABASE_USER,
                               dummyReply, dummyLen, getSqlParserFlags(),
                               NULL, 0, diagsArea);
      if (cmpStatus != 0)
        {
          char emsText[120];
          str_sprintf(emsText,
                      "Set DATABASE_USER in embedded arkcmp failed, return code %d",
                      cmpStatus);
          SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, emsText, 0);
        }

      if (dummyReply != NULL)
        {
          exHeap_.deallocateMemory((void*)dummyReply);
          dummyReply = NULL;
        }
      if (diagsArea != NULL)
      {
         diagsArea->decrRefCount();
         diagsArea = NULL;
      }
    }

  // if there is an error using embedded compiler or we are already in the 
  // embedded compiler, send this to a regular compiler .
  if (!getSessionDefaults()->callEmbeddedArkcmp() ||
      (getSessionDefaults()->callEmbeddedArkcmp() && CmpCommon::context() &&
       (cmpStatus != 0 || (CmpCommon::context()->getRecursionLevel() > 0) ||
        getArkcmp()->getServer())))
    {
      short indexIntoCompilerArray = getIndexToCompilerArray();  
      ExSqlComp *exSqlComp = cliGlobals_->getArkcmp(indexIntoCompilerArray);
      ex_assert(exSqlComp, "CliGlobals::getArkcmp() returned NULL");
      exSqlComp->sendRequest(EXSQLCOMP::DATABASE_USER,
                             (const char *) pMessage,
                             (ULng32) strlen(pMessage));
    }
  
  // Send one of two CQDs to the compiler
  // 
  // If mxcmpSessionInUse_ is FALSE: 
  //   CQD SESSION_ID '<ID>'
  //     where <ID> is getSessionId()
  // 
  // Otherwise:
  //   CQD SESSION_USERNAME '<name>'
  //     where <name> is databaseUserName_

  const char *cqdName = "";
  const char *cqdValue = "";

  if (NOT mxcmpSessionInUse_)
  {
    cqdName = "SESSION_ID";
    cqdValue = getSessionId();
  }
  else
  {
    cqdName = "SESSION_USERNAME";
    cqdValue = databaseUserName_;
  }

  UInt32 numBytes = strlen("CONTROL QUERY DEFAULT ")
    + strlen(cqdName)
    + strlen(" '")
    + strlen(cqdValue)
    + strlen("';") + 1;

  char *sendCQD = new (exHeap()) char[numBytes];

  strcpy(sendCQD, "CONTROL QUERY DEFAULT ");
  strcat(sendCQD, cqdName);
  strcat(sendCQD, " '");
  strcat(sendCQD, cqdValue);
  strcat(sendCQD, "';");
  
  ExeCliInterface cliInterface(exHeap(), 0, this);
  Lng32 cliRC = cliInterface.executeImmediate(sendCQD);
  NADELETEBASIC(sendCQD, exHeap());

  // Send the LDAP user name
  char *userName = new(exHeap()) char[ComSqlId::MAX_LDAP_USER_NAME_LEN+1];
  memset(userName, 0, ComSqlId::MAX_LDAP_USER_NAME_LEN+1);
  if (externalUsername_)
    strcpy(userName, externalUsername_);
  else if (databaseUserName_)
    strcpy(userName, databaseUserName_);

  if (userName)
  {
    char * sendCQD1
      = new(exHeap()) char[ strlen("CONTROL QUERY DEFAULT ")
                                          + strlen("LDAP_USERNAME ")
                          + strlen("'")
                          + strlen(userName)
                          + strlen("';")
                          + 1 ];
    strcpy(sendCQD1, "CONTROL QUERY DEFAULT ");
                strcat(sendCQD1, "LDAP_USERNAME '");
                strcat(sendCQD1, userName);
    strcat(sendCQD1, "';");

    cliRC = cliInterface.executeImmediate(sendCQD1);
    NADELETEBASIC(sendCQD1, exHeap());
    NADELETEBASIC(userName, exHeap());
  }

  // Set the "mxcmp in use" flag
  mxcmpSessionInUse_ = TRUE;
}

// ----------------------------------------------------------------------------
// Method:  updateMxcmpSession
//
// Updates security attributes in child arkcmp
//
// Returns: 0 = succeeded; -1 = failed
// ----------------------------------------------------------------------------
Int32 ContextCli::updateMxcmpSession()
{
  // If no child arkcmp, just return
  if (getArkcmp()->getServer() == NULL)
    return 0;

  // Send changed user information to arkcmp process
  CmpContext *cmpCntxt = CmpCommon::context();
  ex_assert(cmpCntxt, "No compiler context exists");
  NABoolean authOn = cmpCntxt->isAuthorizationEnabled();

  // The message contains the following:
  //   (auth state and user ID are delimited by commas)
  //     authorization state (0 - off, 1 - on)
  //     integer user ID
  //     database user name
  // See CmpStatement::process (CmpMessageDatabaseUser) for more details
  Int32 userAsInt = (Int32) databaseUserID_;
  char userMessage [MAX_AUTHID_AS_STRING_LEN + 1 + MAX_USERNAME_LEN + 1 + 2];
  str_sprintf(userMessage, "%d,%d,%s", authOn, userAsInt, databaseUserName_);
  char *pMessage = (char *)&userMessage;

  // Send message to child arkcmp, if one exists
  ExSqlComp::ReturnStatus cmpStatus;
  ExSqlComp *exSqlComp = getArkcmp();
  ex_assert(exSqlComp, "CliGlobals::getArkcmp() returned NULL");
  cmpStatus = exSqlComp->sendRequest(EXSQLCOMP::DATABASE_USER,
                     (const char *) pMessage,
                     (ULng32) strlen(pMessage));
  if (cmpStatus == ExSqlComp::ERROR)
    return -1;

  return 0;
}

void ContextCli::beginSession(char * userSpecifiedSessionName)
{
  if (userSpecifiedSessionName)
    strcpy(userSpecifiedSessionName_, userSpecifiedSessionName);
  else
    strcpy(userSpecifiedSessionName_, "");

  genSessionId();
  
  initVolTabList();

  // save the current session default settings.
  // We restore these settings at session end or if user issues a
  // SSD reset stmt.
  if (sessionDefaults_)
    sessionDefaults_->saveSessionDefaults();

  if (aqrInfo())
    aqrInfo()->saveAQRErrors();

  sessionInUse_ = TRUE;
  if (getSessionDefaults())
    getSessionDefaults()->beginSession();

  setInMemoryObjectDefn(FALSE);
}

void ContextCli::endMxcmpSession(NABoolean cleanupEsps, 
                                 NABoolean clearCmpCache)
{
  Lng32 flags = 0;
  char* dummyReply = NULL;
  ULng32 dummyLength;
  
  if (cleanupEsps)
    flags |= CmpMessageEndSession::CLEANUP_ESPS;

  flags |= CmpMessageEndSession::RESET_ATTRS;

  if (clearCmpCache)
    flags |= CmpMessageEndSession::CLEAR_CACHE;

  Int32 cmpStatus = 2;  // assume failure
  if (getSessionDefaults()->callEmbeddedArkcmp() &&
      isEmbeddedArkcmpInitialized() &&
      CmpCommon::context() && (CmpCommon::context()->getRecursionLevel() == 0) )
    {
      char *dummyReply = NULL;
      ULng32 dummyLen;
      ComDiagsArea *diagsArea = NULL;
      cmpStatus = CmpCommon::context()->compileDirect((char *) &flags,
                               (ULng32) sizeof(Lng32), &exHeap_,
                               SQLCHARSETCODE_UTF8, EXSQLCOMP::END_SESSION,
                               dummyReply, dummyLen, getSqlParserFlags(),
                               NULL, 0, diagsArea);
      if (cmpStatus != 0)
        {
          char emsText[120];
          str_sprintf(emsText,
                      "Set END_SESSION in embedded arkcmp failed, return code %d",
                      cmpStatus);
          SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, emsText, 0);
        }

      if (dummyReply != NULL)
        {
          exHeap_.deallocateMemory((void*)dummyReply);
          dummyReply = NULL;
        }
      if (diagsArea != NULL)
      {
         diagsArea->decrRefCount();
         diagsArea = NULL;
      }
    }

  // if there is an error using embedded compiler or we are already in the 
  // embedded compiler, send this to a regular compiler .
  if (!getSessionDefaults()->callEmbeddedArkcmp() ||
      (getSessionDefaults()->callEmbeddedArkcmp() && CmpCommon::context() &&
       (cmpStatus != 0 || (CmpCommon::context()->getRecursionLevel() > 0) ||
        getArkcmp()->getServer())))
    {
      // send request to mxcmp so it can cleanup esps started by it
      // and reset nonResetable attributes.
      // Ignore errors.
      if (getNumArkcmps() > 0)
        {
          short indexIntoCompilerArray = getIndexToCompilerArray();  
          ExSqlComp::ReturnStatus status = 
            cliGlobals_->getArkcmp(indexIntoCompilerArray)->sendRequest
            (EXSQLCOMP::END_SESSION, (char*)&flags, sizeof(Lng32));
          
          if (status == ExSqlComp::SUCCESS)
            {
              status = 
                cliGlobals_->getArkcmp(indexIntoCompilerArray)->getReply(
                     dummyReply, dummyLength);
              cliGlobals_->getArkcmp(indexIntoCompilerArray)->
                getHeap()->deallocateMemory((void*)dummyReply);
            }
        }
    }  // end if (getSessionDefaults()->callEmbeddedArkcmp() && ...
}

void ContextCli::resetAttributes()
{
  // reset all parserflags
  sqlParserFlags_ = 0;

  // reset attributes which are set for the session based on 
  // the caller. These do not persist across multiple session.
  // For example,  scripts, or defaults.
  if (sessionDefaults_)
    sessionDefaults_->resetSessionOnlyAttributes();
}

Lng32 ContextCli::reduceEsps()
{
  Lng32 countBefore = cliGlobals_->getEspManager()->getNumOfEsps();
  // make assigned ESPs to be idle.
  while (unassignEsps(true))
    ;
  // get rid of idle ESPs. 
  cliGlobals_->getEspManager()->endSession(this);
  return countBefore - cliGlobals_->getEspManager()->getNumOfEsps();
}

void ContextCli::endSession(NABoolean cleanupEsps,
                            NABoolean cleanupEspsOnly,
                            NABoolean cleanupOpens)
{
  short rc = 0;
  if (NOT cleanupEspsOnly)
    {
      rc = ExExeUtilCleanupVolatileTablesTcb::dropVolatileTables
        (this, exHeap());
      SQL_EXEC_ClearDiagnostics(NULL);
    }

  // normally when user disconnects, by default odbc calls cli end session
  // without the cleanup esp parameter. so master will only kill idle esps.
  // however, user can set two parameters at data source level:
  //
  //   - number of disconnects (e.g. 10)
  //   - session cleanup time (e.g. 30 minutes)
  //
  // so after user disconnects 10 times or disconnects after session alive
  // for 30 minutes, odbc will call cli end session with the cleanup esp
  // parameter. the result is master will kill all esps in cache that
  // currently are not being used.
  //
  // SET SESSION DEFAULT SQL_SESSION 'END:CLEANUP_ESPS';
  // if cleanupEsps flag is set from above session default, then kill all esps
  // in esp cache that are not currently being used. otherwise, kill esps that
  // have been idle longer than session default ESP_STOP_IDLE_TIMEOUT.
  if (cleanupEsps)
    cliGlobals_->getEspManager()->endSession(this);
  else
    cliGlobals_->getEspManager()->stopIdleEsps(this);

  if (cleanupOpens)
    {
      closeAllTables();
    }

 

  // restore all SSD settings to their initial value
  if (sessionDefaults_)
    sessionDefaults_->restoreSessionDefaults();
  
  // restore all entries for aqr that were set in this session
  // to their initial value.
  if (aqrInfo())
    {
      if (( aqrInfo()->restoreAQRErrors()==FALSE) && sessionInUse_)
        {
          diagsArea_<< DgSqlCode(-CLI_INTERNAL_ERROR);
          diagsArea_ << DgString0(":Saved AQR error map is empty");
        }

    }
  if (NOT cleanupEspsOnly)
    {
      strcpy(userSpecifiedSessionName_, "");
      
      sessionInUse_ = FALSE;
    }
    killAndRecreateMxcmp();
  if (rc < 0) 
    {
      // an error was returned during drop of tables in the volatile schema.
      // Drop the schema and mark it as obsolete.
      dropSession();
    }
  else
    {
      endMxcmpSession(cleanupEsps);
      resetAttributes();
    }
  

  // With ldap user identities, user identity token changes everytime a user
  // logs on. So UDR Server needs to get token everytime an mxcs connection 
  // is reassigned which is not being done currently.
  // Instead, since UDR Server always gets the token duing startup, it
  // is being killed when the client logs off and restared when a client
  // logs on and assigned the same connection.
  releaseUdrServers();
  cliGlobals_->setLogReclaimEventDone(FALSE);
  // Reset the stats area to ensure that the reference count in conext_->prevStmtStats_ is
  // decremented so that it can be freed up when GC happens in ssmp
  setStatsArea(NULL, FALSE, FALSE, TRUE);
}

void ContextCli::dropSession(NABoolean clearCmpCache)
{
  short rc = 0;
  ComDiagsArea *diags = NULL;
  if (volatileSchemaCreated_)
    {
      rc = ExExeUtilCleanupVolatileTablesTcb::dropVolatileSchema
        (this, NULL, exHeap(), diags);
      if (rc < 0 && diags != NULL && diags->getNumber(DgSqlCode::ERROR_) > 0) {
         ComCondition *condition = diags->getErrorEntry(0);
         logAnMXEventForError(*condition, GetCliGlobals()->getEMSEventExperienceLevel()); 
      } 
      volatileSchemaCreated_ = FALSE;
      SQL_EXEC_ClearDiagnostics(NULL);
    }

  if (mxcmpSessionInUse_)
    {
      // now send NULL session id to mxcmp
      char * sendCQD 
        = new(exHeap()) char[strlen("CONTROL QUERY DEFAULT SESSION_ID '';")+1]; 
      strcpy(sendCQD, "CONTROL QUERY DEFAULT SESSION_ID '';");
      
      ExeCliInterface cliInterface(exHeap(),0,this);
      Lng32 cliRC = cliInterface.executeImmediate(sendCQD);
      NADELETEBASIC(sendCQD, exHeap());
      
      endMxcmpSession(TRUE, clearCmpCache);
    }

  resetAttributes();

  mxcmpSessionInUse_ = FALSE;

  sessionInUse_ = FALSE;

  // Reset the stats area to ensure that the reference count in
  // prevStmtStats_ is decremented so that it can be freed up when
  // GC happens in mxssmp
  setStatsArea(NULL, FALSE, FALSE, TRUE);
  HiveClient_JNI::deleteInstance();
  disconnectHdfsConnections();
}

void ContextCli::resetVolatileSchemaState()
{
  ComDiagsArea * savedDiagsArea = NULL;
  if (diagsArea_.getNumber() > 0)
    {
      savedDiagsArea = ComDiagsArea::allocate(exHeap());
      savedDiagsArea->mergeAfter(diagsArea_);
    }

  // reset volatile schema usage in mxcmp
  char * sendCQD 
    = new(exHeap()) char[strlen("CONTROL QUERY DEFAULT VOLATILE_SCHEMA_IN_USE 'FALSE';") + 1];
  strcpy(sendCQD, "CONTROL QUERY DEFAULT VOLATILE_SCHEMA_IN_USE 'FALSE';");
  
  ExeCliInterface cliInterface(exHeap());
  Lng32 cliRC = cliInterface.executeImmediate(sendCQD);
  NADELETEBASIC(sendCQD, exHeap());
  
  if (savedDiagsArea)
    {
      diagsArea_.mergeAfter(*savedDiagsArea);
    }

  if (savedDiagsArea)
    {
      savedDiagsArea->clear();
      savedDiagsArea->deAllocate();
    }
}

void ContextCli::setSessionId(char * si)
{
  if (sessionID_)
    exHeap()->deallocateMemory(sessionID_);
  
  if (si)
    {
      sessionID_ = (char*)exHeap()->allocateMemory(strlen(si)+1);
      strcpy(sessionID_, si);
    }
  else
    sessionID_ = NULL;
}

void ContextCli::initVolTabList()
{
  if (volTabList_)
    delete volTabList_;

  volTabList_ = new(exCollHeap()) HashQueue(exCollHeap());
}

void ContextCli::resetVolTabList()
{
  if (volTabList_)
    delete volTabList_;
  
  volTabList_ = NULL;
}

void ContextCli::closeAllTables()
{
}

// this method is used to send a transaction operation specific message
// to arkcmp by executor.
// Based on that, arkcmp takes certain actions.
// Called from executor/ex_transaction.cpp.
// Note: most of the code in this method has been moved from ex_transaction.cpp
ExSqlComp::ReturnStatus ContextCli::sendXnMsgToArkcmp
(char * data, Lng32 dataSize, 
 Lng32 xnMsgType, ComDiagsArea* &diagsArea)
{
  // send the set trans request to arkcmp so compiler can
  // set this trans mode in its memory. This is done so any
  // statement compiled after this could be compiled with the
  // current trans mode.
  ExSqlComp *cmp = NULL;
  ExSqlComp::ReturnStatus cmpStatus ;
  // the dummyReply is moved up because otherwise the 
  // compiler would complain about
  // initialization of variables afer goto statements.
  
  char* dummyReply = NULL;
  ULng32 dummyLength;
  ContextCli *currCtxt = this;
  Int32 cmpRet = 0;

  // If use embedded compiler, send the settings to it
  if (currCtxt->getSessionDefaults()->callEmbeddedArkcmp() &&
      currCtxt->isEmbeddedArkcmpInitialized() &&  
      (CmpCommon::context()) &&
      (CmpCommon::context()->getRecursionLevel() == 0))
    {
      NAHeap *arkcmpHeap = currCtxt->exHeap();
      
      cmpRet = CmpCommon::context()->compileDirect(
           data, dataSize,
           arkcmpHeap,
           SQLCHARSETCODE_UTF8,
           CmpMessageObj::MessageTypeEnum(xnMsgType),
           dummyReply, dummyLength,
           currCtxt->getSqlParserFlags(),
           NULL, 0, diagsArea);
      if (cmpRet != 0)
        {
          char emsText[120];
          str_sprintf(emsText,
                      "Set transaction mode to embedded arkcmp failed, return code %d",
                      cmpRet);
          SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, emsText, 0);
          diagsArea = CmpCommon::diags();
        }
      
      if (dummyReply != NULL)
        {
          arkcmpHeap->deallocateMemory((void*)dummyReply);
          dummyReply = NULL;
        }
    }

  if (!currCtxt->getSessionDefaults()->callEmbeddedArkcmp()  || 
      (currCtxt->getSessionDefaults()->callEmbeddedArkcmp() && 
       ((cmpRet != 0) || 
        (CmpCommon::context()->getRecursionLevel() > 0) ||
        currCtxt->getArkcmp()->getServer())
       )
      )
    {
      for (short i = 0; i < currCtxt->getNumArkcmps();i++)
        {
          cmp = currCtxt->getArkcmp(i);
          cmpStatus = cmp->sendRequest(CmpMessageObj::MessageTypeEnum(xnMsgType),
                                       data, dataSize,
                                       TRUE, NULL,
                                       SQLCHARSETCODE_UTF8,
                                       TRUE /*resend, if needed*/
                                       );
          
          if (cmpStatus != ExSqlComp::SUCCESS) {
            diagsArea = cmp->getDiags();
            // If its an error don't proceed further.
            if (cmpStatus == ExSqlComp::ERROR)
              return cmpStatus;
          }
	  
          cmpStatus = cmp->getReply(dummyReply, dummyLength);
          cmp->getHeap()->deallocateMemory((void*)dummyReply);
          if (cmpStatus != ExSqlComp::SUCCESS) {
            diagsArea = cmp->getDiags();
            //Don't proceed if its an error.
            if (cmpStatus == ExSqlComp::ERROR)
              return cmpStatus;
          }
	  
          if (cmp->status() != ExSqlComp::FETCHED)
            diagsArea = cmp->getDiags();
        } // for
    }
  
  return ExSqlComp::SUCCESS;
}

Lng32 ContextCli::setSecInvalidKeys(
           /* IN */    Int32 numSiKeys,
           /* IN */    SQL_QIKEY siKeys[])
{
  CliGlobals *cliGlobals = getCliGlobals();
  if (cliGlobals->getStatsGlobals() == NULL)
  {
    (diagsArea_) << DgSqlCode(-EXE_RTS_NOT_STARTED);
    return diagsArea_.mainSQLCODE();
  }
  for (int i = 0; i < numSiKeys; i++)
  {
    // Initialize the filler so that functions like memcmp can be 
    // used.
    memset(siKeys[i].filler, 0, sizeof(siKeys[i].filler));
    // Bad operation values cause problems down-stream, so catch
    // them here by letting ComQIActionTypeLiteralToEnum assert.
    ComQIActionTypeLiteralToEnum(siKeys[i].operation);
  }

  ComDiagsArea *tempDiagsArea = &diagsArea_;
  tempDiagsArea->clear();
 
  IpcServer *ssmpServer = ssmpManager_->getSsmpServer(exHeap(),
                                 cliGlobals->myNodeName(), 
                                 cliGlobals->myCpu(), tempDiagsArea);
  if (ssmpServer == NULL)
    return diagsArea_.mainSQLCODE();

  SsmpClientMsgStream *ssmpMsgStream  = new (cliGlobals->getIpcHeap())
        SsmpClientMsgStream((NAHeap *)cliGlobals->getIpcHeap(), 
                            ssmpManager_, tempDiagsArea);

  ssmpMsgStream->addRecipient(ssmpServer->getControlConnection());

  SecInvalidKeyRequest *sikMsg = 
    new (cliGlobals->getIpcHeap()) SecInvalidKeyRequest(
                                      cliGlobals->getIpcHeap(), 
                                      numSiKeys, siKeys);

  *ssmpMsgStream << *sikMsg;

  // Call send with no timeout.  
  ssmpMsgStream->send(); 

  // I/O is now complete.  
  sikMsg->decrRefCount();

  cliGlobals->getEnvironment()->deleteCompletedMessages();
  ssmpManager_->cleanupDeletedSsmpServers();
  return diagsArea_.mainSQLCODE();

}

Int32 ContextCli::checkLobLock(char *inLobLockId, NABoolean *found)
{
  Int32 retcode = 0;
  *found = FALSE;
  CliGlobals *cliGlobals = getCliGlobals();
  StatsGlobals *statsGlobals = GetCliGlobals()->getStatsGlobals();
  if (cliGlobals->getStatsGlobals() == NULL)
  {
    (diagsArea_) << DgSqlCode(-EXE_RTS_NOT_STARTED);
    return diagsArea_.mainSQLCODE();
  }
  statsGlobals->checkLobLock(cliGlobals,inLobLockId);
  if (inLobLockId != NULL)
    *found = TRUE;
  return retcode;
}
Lng32 ContextCli::setLobLock(
     /* IN */    char *lobLockId // objID+column number
                             )
{
  CliGlobals *cliGlobals = getCliGlobals();
  NABoolean releasingLock = FALSE;
  if (cliGlobals->getStatsGlobals() == NULL)
  {
    (diagsArea_) << DgSqlCode(-EXE_RTS_NOT_STARTED);
    return diagsArea_.mainSQLCODE();
  }
  ComDiagsArea *tempDiagsArea = &diagsArea_;
  IpcServer *ssmpServer = NULL;
  tempDiagsArea->clear();
  if (lobLockId[0] == '-')
    releasingLock = TRUE;
  // Get an ssmp node to talk to. Picking one based off of lobLockId should
  // make it unique and avoid clash with another node that may be attempting 
  // to lock the same lob
  if (!releasingLock)
    {
      Int32 nodeCount = 0;
      Int32 rc = msg_mon_get_node_info(&nodeCount, 0, NULL);
      Int32 targetNodeId = 0;
      Int32 lockHash = 0;
      char myNodeName[MAX_SEGMENT_NAME_LEN+1];
      MS_Mon_Node_Info_Type nodeInfo;
      for (int i = 0; i < LOB_LOCK_ID_SIZE; i++)
        lockHash +=(unsigned char)lobLockId[i];
      if (nodeCount)
        targetNodeId = lockHash%nodeCount;
      rc = msg_mon_get_node_info_detail(targetNodeId, &nodeInfo);
      if (rc == 0)
        strcpy(myNodeName, nodeInfo.node[0].node_name);
      else
        myNodeName[0] = '\0';

      ssmpServer = ssmpManager_->getSsmpServer(exHeap(),
                                               //cliGlobals->myNodeName(), 
                                               //cliGlobals->myCpu(), 
                                               myNodeName,
                                               targetNodeId,
                                               tempDiagsArea);
    }
  else
    ssmpServer = ssmpManager_->getSsmpServer(exHeap(),
                                             cliGlobals->myNodeName(), 
                                             cliGlobals->myCpu(), 
                                             tempDiagsArea);
  if (ssmpServer == NULL)
    return diagsArea_.mainSQLCODE();

  SsmpClientMsgStream *ssmpMsgStream  = new (cliGlobals->getIpcHeap())
        SsmpClientMsgStream((NAHeap *)cliGlobals->getIpcHeap(), 
                            ssmpManager_, tempDiagsArea);
  ssmpMsgStream->addRecipient(ssmpServer->getControlConnection());
  LobLockRequest *llMsg = 
    new (cliGlobals->getIpcHeap()) LobLockRequest(
                                      cliGlobals->getIpcHeap(), 
                                      lobLockId);
  *ssmpMsgStream << *llMsg;
  // Call send with no timeout.  
  ssmpMsgStream->send(); 
  // I/O is now complete.  
  llMsg->decrRefCount();
  cliGlobals->getEnvironment()->deleteCompletedMessages();
  ssmpManager_->cleanupDeletedSsmpServers();
  return diagsArea_.mainSQLCODE();

}
ExStatisticsArea *ContextCli::getMergedStats(
            /* IN */    short statsReqType,
            /* IN */    char *statsReqStr,
            /* IN */   Lng32 statsReqStrLen,
            /* IN */    short activeQueryNum,
            /* IN */    short statsMergeType,
                        short &retryAttempts)
{
  ExStatisticsArea *stats = NULL;
  ExStatisticsArea *statsTmp = NULL;
  short tmpStatsMergeType;

  CliGlobals *cliGlobals = getCliGlobals();
  if (statsMergeType == SQLCLI_DEFAULT_STATS)
  {
    if (sessionDefaults_ != NULL)
      tmpStatsMergeType = (short)sessionDefaults_->getStatisticsViewType();
    else
      tmpStatsMergeType = SQLCLI_SAME_STATS;
  }
  else
      tmpStatsMergeType = statsMergeType;
  NABoolean deleteStats = FALSE;
  if (statsReqType == SQLCLI_STATS_REQ_STMT ||
          statsReqType == SQLCLI_STATS_REQ_QID_CURRENT)
  {
    if (statsReqType == SQLCLI_STATS_REQ_STMT)
    {
       SQLMODULE_ID module;
       SQLSTMT_ID stmt_id;
       init_SQLMODULE_ID(&module);
       init_SQLCLI_OBJ_ID(&stmt_id, SQLCLI_CURRENT_VERSION, stmt_name, &module,
                     statsReqStr, NULL, SQLCHARSETSTRING_ISO88591, statsReqStrLen);
       Statement *stmt = getStatement(&stmt_id);
       ExMasterStats *masterStats, *tmpMasterStats;

       if (stmt != NULL)
       {
          statsTmp = stmt->getStatsArea();
          if (statsTmp == NULL && stmt->getStmtStats() != NULL &&
                       (masterStats = stmt->getStmtStats()->getMasterStats()) != NULL)
          {
             tmpStatsMergeType = masterStats->compilerStatsInfo().collectStatsType();
             statsTmp = new (exCollHeap()) ExStatisticsArea((NAMemory *)exCollHeap(), 0,
                        (ComTdb::CollectStatsType)tmpStatsMergeType,
                        (ComTdb::CollectStatsType)tmpStatsMergeType);
             tmpMasterStats = new(exHeap())
             ExMasterStats(exHeap());
             tmpMasterStats->copyContents(masterStats);
             statsTmp->setMasterStats(tmpMasterStats);
             deleteStats = TRUE;
          }
       }
     }
     else if (statsReqType == SQLCLI_STATS_REQ_QID_CURRENT)
        statsTmp = getStats(); 
    if (statsTmp != NULL)
    {
      if (statsTmp->getCollectStatsType() ==
          (ComTdb::CollectStatsType) SQLCLI_ALL_STATS)
        tmpStatsMergeType = SQLCLI_SAME_STATS;
      if (tmpStatsMergeType == SQLCLI_SAME_STATS  || 
          (tmpStatsMergeType == statsTmp->getCollectStatsType()))
      {
        if (deleteStats)
           setDeleteStats(TRUE);
        else
           setDeleteStats(FALSE);
        stats = statsTmp;
      }
      else
      {
        stats = new (exCollHeap()) ExStatisticsArea((NAMemory *)exCollHeap(), 0, 
                        (ComTdb::CollectStatsType)tmpStatsMergeType,
                        statsTmp->getOrigCollectStatsType());
        StatsGlobals *statsGlobals = cliGlobals_->getStatsGlobals();
        if (statsGlobals != NULL)
        {
          int error = statsGlobals->getStatsSemaphore(cliGlobals_->getSemId(),
                                                      cliGlobals_->myPin());
          stats->merge(statsTmp, tmpStatsMergeType);
          setDeleteStats(TRUE);
          statsGlobals->releaseStatsSemaphore(cliGlobals_->getSemId(),cliGlobals_->myPin());
        }
        else
        {
          stats->merge(statsTmp, tmpStatsMergeType);
          setDeleteStats(TRUE);
        }
      }
      return stats;
    }
    else
      return NULL;
  }
  short cpu = -1;
  pid_t pid = (pid_t)-1;
  Int64 timeStamp = -1;
  Lng32 queryNumber = -1;
  char nodeName[MAX_SEGMENT_NAME_LEN+1];
  if (cliGlobals->getStatsGlobals() == NULL)
  {
    (diagsArea_) << DgSqlCode(-EXE_RTS_NOT_STARTED);
    return NULL;
  }
  switch (statsReqType)
  {
    case SQLCLI_STATS_REQ_QID:
      if (statsReqStr == NULL)
      {
        (diagsArea_) << DgSqlCode(-EXE_RTS_INVALID_QID) << DgString0("NULL");
        return NULL;
      }
      if (getMasterCpu(statsReqStr, statsReqStrLen, nodeName, MAX_SEGMENT_NAME_LEN+1, cpu) == -1)
      {
        (diagsArea_) << DgSqlCode(-EXE_RTS_INVALID_QID) << DgString0(statsReqStr);
        return NULL;
      }
      break;
    case SQLCLI_STATS_REQ_CPU:
    case SQLCLI_STATS_REQ_PID:
    case SQLCLI_STATS_REQ_RMS_INFO:
    case SQLCLI_STATS_REQ_PROCESS_INFO:
    case SQLCLI_STATS_REQ_QID_INTERNAL:
      if (parse_statsReq(statsReqType, statsReqStr, statsReqStrLen, nodeName,
                cpu, pid, timeStamp, queryNumber) == -1)
      {
        (diagsArea_) << DgSqlCode(-EXE_RTS_INVALID_CPU_PID);
        return NULL;
      }
      break;
    default:
      (diagsArea_) << DgSqlCode(-EXE_RTS_INVALID_QID) << DgString0(statsReqStr);
      return NULL;
  }
  ComDiagsArea *tempDiagsArea = &diagsArea_;
  ExSsmpManager *ssmpManager = cliGlobals->getSsmpManager();
  IpcServer *ssmpServer = ssmpManager->getSsmpServer(exHeap(), nodeName, 
           (cpu == -1 ?  cliGlobals->myCpu() : cpu), tempDiagsArea);
  if (ssmpServer == NULL)
    return NULL; // diags are in diagsArea_

 //Create the SsmpClientMsgStream on the IpcHeap, since we don't dispose of it immediately.
 //We just add it to the list of completed messages in the IpcEnv, and it is disposed of later.
 //If we create it on the ExStatsTcb's heap, that heap gets deallocated when the statement is
 //finished, and we can corrupt some other statement's heap later on when we deallocate this stream.
  SsmpClientMsgStream *ssmpMsgStream  = new (cliGlobals->getIpcHeap())
        SsmpClientMsgStream((NAHeap *)cliGlobals->getIpcHeap(), ssmpManager);

  ssmpMsgStream->addRecipient(ssmpServer->getControlConnection());
  RtsHandle rtsHandle = (RtsHandle) this;
  SessionDefaults *sd = getSessionDefaults();
  Lng32 RtsTimeout;
  // Retrieve the Rts collection interval and active queries. If they are valid, calculate the timeout
  // and send to the SSMP process.
  NABoolean wmsProcess;
  if (sd)
  {
    RtsTimeout = sd->getRtsTimeout();
    wmsProcess = sd->getWmsProcess();
  }
  else
  {
    RtsTimeout = 0;
    wmsProcess = FALSE;
  }
  RtsCpuStatsReq *cpuStatsReq = NULL;
  RtsStatsReq *statsReq = NULL;
  RtsQueryId *rtsQueryId = NULL;
  setDeleteStats(TRUE);
  if (statsReqType == SQLCLI_STATS_REQ_RMS_INFO)
  {
    cpuStatsReq = new (cliGlobals->getIpcHeap())RtsCpuStatsReq(rtsHandle, cliGlobals->getIpcHeap(),
        nodeName, cpu, activeQueryNum, statsReqType);
    *ssmpMsgStream << *cpuStatsReq;
  }
  else
  {
    statsReq = new (cliGlobals->getIpcHeap())RtsStatsReq(rtsHandle,
                cliGlobals->getIpcHeap(), wmsProcess);
    *ssmpMsgStream << *statsReq;
    switch (statsReqType)
    {
      case SQLCLI_STATS_REQ_QID:
        rtsQueryId = new (cliGlobals->getIpcHeap()) RtsQueryId(cliGlobals->getIpcHeap(),
              statsReqStr, statsReqStrLen,
              (UInt16)tmpStatsMergeType, activeQueryNum);
        break;
      case SQLCLI_STATS_REQ_CPU:
        rtsQueryId = new (cliGlobals->getIpcHeap()) RtsQueryId(cliGlobals->getIpcHeap(),
          nodeName, cpu,
          (UInt16)tmpStatsMergeType, activeQueryNum);
        break;
      case SQLCLI_STATS_REQ_PID:
      case SQLCLI_STATS_REQ_PROCESS_INFO:
        rtsQueryId = new (cliGlobals->getIpcHeap()) RtsQueryId(cliGlobals->getIpcHeap(),
          nodeName,  cpu, pid,
          (UInt16)tmpStatsMergeType, activeQueryNum, statsReqType);
        break;
      case SQLCLI_STATS_REQ_QID_INTERNAL:
        rtsQueryId = new (cliGlobals->getIpcHeap()) RtsQueryId(cliGlobals->getIpcHeap(),
           nodeName, cpu, pid, timeStamp, queryNumber,
           (UInt16)tmpStatsMergeType, activeQueryNum);
        break;
      default:
        rtsQueryId = NULL;
        break;
      break;
    }
    if (NULL != rtsQueryId)
      *ssmpMsgStream << *rtsQueryId;
  }
  if (RtsTimeout != 0)
  {
   // We have a valid value for the timeout, so we use it by converting it to centiseconds.
   RtsTimeout = RtsTimeout * 100;
  }
  else
    //Use the default value of 4 seconds, or 400 centiseconds.
    RtsTimeout = 400;
  // Send the message
  ssmpMsgStream->send(FALSE, -1);
  Int64 startTime = NA_JulianTimestamp();
  Int64 currTime;
  Int64 elapsedTime;
  IpcTimeout timeout = (IpcTimeout) RtsTimeout;
  while (timeout > 0 && ssmpMsgStream->hasIOPending())
  {
    ssmpMsgStream->waitOnMsgStream(timeout);
    currTime = NA_JulianTimestamp();
    elapsedTime = (Int64)(currTime - startTime) / 10000;
    timeout = (IpcTimeout)(RtsTimeout - elapsedTime);
  }
  // Callbacks would have placed broken connections into
  // ExSsmpManager::deletedSsmps_.  Delete them now.
  ssmpManager->cleanupDeletedSsmpServers();
  if (ssmpMsgStream->getState() == IpcMessageStream::ERROR_STATE && retryAttempts < 3)
  {
    if (statsReqType != SQLCLI_STATS_REQ_RMS_INFO)
    {
      rtsQueryId->decrRefCount();
      statsReq->decrRefCount();
    }
    else
      cpuStatsReq->decrRefCount();
    DELAY(100);
    retryAttempts++;
    stats = getMergedStats(statsReqType,
                          statsReqStr,
                          statsReqStrLen,
                          activeQueryNum,
                          statsMergeType,
                          retryAttempts);
    return stats;
  }
 if (ssmpMsgStream->getState() == IpcMessageStream::BREAK_RECEIVED)
  {
   // Break received - set diags area
   (diagsArea_) << DgSqlCode(-EXE_CANCELED);
    return NULL;
  }
  if (! ssmpMsgStream->isReplyReceived())
  {
    char lcStatsReqStr[ComSqlId::MAX_QUERY_ID_LEN+1];
    Lng32 len;
    if (statsReqStrLen > ComSqlId::MAX_QUERY_ID_LEN)
      len = ComSqlId::MAX_QUERY_ID_LEN;
    else
      len = statsReqStrLen;
    str_cpy_all(lcStatsReqStr, statsReqStr, len);
    lcStatsReqStr[len] = '\0';
    (diagsArea_) << DgSqlCode(-EXE_RTS_TIMED_OUT)
        << DgString0(lcStatsReqStr) << DgInt0(RtsTimeout/100);
    return NULL;
  }
  if (ssmpMsgStream->getRtsQueryId() != NULL)
  {
    rtsQueryId->decrRefCount();
    statsReq->decrRefCount();
    retryAttempts = 0;
    stats = getMergedStats(SQLCLI_STATS_REQ_QID,
                          ssmpMsgStream->getRtsQueryId()->getQueryId(),
                          ssmpMsgStream->getRtsQueryId()->getQueryIdLen(),
                          1,
                          statsMergeType,
                          retryAttempts);
    ssmpMsgStream->getRtsQueryId()->decrRefCount();
    return stats;
  }
  stats = ssmpMsgStream->getStats();
  if (stats != NULL && stats->getMasterStats() != NULL)
  {
    if (tmpStatsMergeType == SQLCLIDEV_SAME_STATS)
      stats->getMasterStats()->setCollectStatsType(stats->getCollectStatsType());
    else
      stats->getMasterStats()->setCollectStatsType(( ComTdb::CollectStatsType)tmpStatsMergeType);
  }
  if (statsReqType != SQLCLI_STATS_REQ_RMS_INFO)
  {
    rtsQueryId->decrRefCount();
    statsReq->decrRefCount();
  }
  else
    cpuStatsReq->decrRefCount();
  if (ssmpMsgStream->getNumSscpReqFailed() > 0)
  {
    (diagsArea_) << DgSqlCode(EXE_RTS_REQ_PARTIALY_SATISFIED)
        << DgInt0(ssmpMsgStream->getNumSscpReqFailed());
    if (stats != NULL && stats->getMasterStats() != NULL)
        stats->getMasterStats()->setStatsErrorCode(EXE_RTS_REQ_PARTIALY_SATISFIED);
  }
  return stats;
}


Lng32 ContextCli::GetStatistics2(
            /* IN */    short statsReqType,
            /* IN */    char *statsReqStr,
            /* IN */   Lng32 statsReqStrLen,
            /* IN */    short activeQueryNum,
            /* IN */    short statsMergeType,
            /* OUT */   short *statsCollectType,
            /* IN/OUT */        SQLSTATS_DESC sqlstats_desc[],
            /* IN */  Lng32 max_stats_desc,
            /* OUT */ Lng32 *no_returned_stats_desc)
{
  Lng32 retcode;
  short retryAttempts = 0;
  
  if (mergedStats_ != NULL && deleteStats())
  {
    NADELETE(mergedStats_, ExStatisticsArea, mergedStats_->getHeap());
    setDeleteStats(FALSE);
    mergedStats_ = NULL;
  }
  
  mergedStats_ = getMergedStats(statsReqType, statsReqStr, statsReqStrLen, 
                  activeQueryNum, statsMergeType, retryAttempts);
  
  cliGlobals_ -> getEnvironment() -> deleteCompletedMessages();

  if (mergedStats_ == NULL)
  {
    if (diagsArea_.getNumber(DgSqlCode::ERROR_) > 0)    
      return diagsArea_.mainSQLCODE();
    else
    if (statsReqType == SQLCLI_STATS_REQ_QID)
    {
      (diagsArea_) << DgSqlCode(-EXE_RTS_QID_NOT_FOUND) << DgString0(statsReqStr);
      return diagsArea_.mainSQLCODE();
    }
    else
    {
      if (no_returned_stats_desc != NULL)
        *no_returned_stats_desc = 0;
      return 0;
    }
  }
  if (statsCollectType != NULL)
    *statsCollectType = mergedStats_->getOrigCollectStatsType();
  retcode = mergedStats_->getStatsDesc(statsCollectType,
              sqlstats_desc,
              max_stats_desc,
              no_returned_stats_desc);
  if (retcode != 0)
    (diagsArea_) << DgSqlCode(retcode);
  return retcode;
}

void ContextCli::setStatsArea(ExStatisticsArea *stats, NABoolean isStatsCopy,
                              NABoolean inSharedSegment, NABoolean getSemaphore)
{
  int error;
  StatsGlobals *statsGlobals = cliGlobals_->getStatsGlobals();

  // Delete Stats only when it is different from the incomng stats
  if (statsCopy() && stats_ != NULL && stats != stats_)
  {
    if (statsInSharedSegment() && getSemaphore)
    {
      if (statsGlobals != NULL)
      {
        error = statsGlobals->getStatsSemaphore(cliGlobals_->getSemId(),
                                                      cliGlobals_->myPin());
      }
      NADELETE(stats_, ExStatisticsArea, stats_->getHeap());
      if (statsGlobals != NULL)
        statsGlobals->releaseStatsSemaphore(cliGlobals_->getSemId(),cliGlobals_->myPin());
    }
    else
      NADELETE(stats_, ExStatisticsArea, stats_->getHeap());
  }
  stats_ = stats;
  setStatsCopy(isStatsCopy);
  setStatsInSharedSegment(inSharedSegment);
  if (prevStmtStats_ != NULL)
  {
    if (prevStmtStats_->canbeGCed() && 
             (!prevStmtStats_->isWMSMonitoredCliQuery()))
    {
      if (statsGlobals != NULL)
      {
        if (getSemaphore)
        {
           error = statsGlobals->getStatsSemaphore(cliGlobals_->getSemId(),
                  cliGlobals_->myPin());
        }
        prevStmtStats_->setStmtStatsUsed(FALSE);
        statsGlobals->removeQuery(cliGlobals_->myPin(), prevStmtStats_); 
        if (getSemaphore)
           statsGlobals->releaseStatsSemaphore(cliGlobals_->getSemId(),
                                   cliGlobals_->myPin());
      }
    }
    else
       prevStmtStats_->setStmtStatsUsed(FALSE);
    prevStmtStats_ = NULL;
  }
}

Lng32 ContextCli::holdAndSetCQD(const char * defaultName, const char * defaultValue)
{
  ExeCliInterface cliInterface(exHeap(), 0, this);

  return ExExeUtilTcb::holdAndSetCQD(defaultName, defaultValue, &cliInterface);
}

Lng32 ContextCli::restoreCQD(const char * defaultName)
{
  ExeCliInterface cliInterface(exHeap(), 0, this);

  return ExExeUtilTcb::restoreCQD(defaultName, &cliInterface,&this->diags());
}

Lng32 ContextCli::setCS(const char * csName, char * csValue)
{
  ExeCliInterface cliInterface(exHeap(), 0, this);

  return ExExeUtilTcb::setCS(csName, csValue, &cliInterface);
}

Lng32 ContextCli::resetCS(const char * csName)
{
  ExeCliInterface cliInterface(exHeap(), 0, this);

  return ExExeUtilTcb::resetCS(csName, &cliInterface,&this->diags());
}

Lng32 parse_statsReq(short statsReqType,char *statsReqStr, Lng32 statsReqStrLen,
                     char *nodeName, short &cpu, pid_t &pid,
                     Int64 &timeStamp, Lng32 &queryNumber)
{
  char tempStr[ComSqlId::MAX_QUERY_ID_LEN+1];
  char *ptr;
  Int64 tempNum;
  char *cpuTempPtr;
  char *pinTempPtr;
  char *timeTempPtr = (char *)NULL;
  char *queryNumTempPtr = (char *)NULL;
  short pHandle[10];
  Int32 error;
  Int32 tempCpu;
  pid_t tempPid;
  Int64 tempTime = -1;
  Lng32 tempQnum = -1;
  short len;
  Int32 cpuMinRange;
  CliGlobals *cliGlobals;

  if (statsReqStr == NULL ||nodeName == NULL || 
          statsReqStrLen > ComSqlId::MAX_QUERY_ID_LEN)
    return -1;
  cpu = 0;
  pid = 0;
  switch (statsReqType)
  {
  case SQLCLI_STATS_REQ_QID:
    if (str_cmp(statsReqStr, "MXID", 4) != 0)
       return -1;
    break;
  case SQLCLI_STATS_REQ_CPU:
  case SQLCLI_STATS_REQ_RMS_INFO:
    str_cpy_all(tempStr, statsReqStr, statsReqStrLen);
    tempStr[statsReqStrLen] = '\0';
    if ((ptr = str_chr(tempStr, '.')) != NULL)
    {
      *ptr++ = '\0';
      str_cpy_all(nodeName, tempStr, str_len(tempStr));
      nodeName[str_len(tempStr)] = '\0';
    }
    else
    {
      nodeName[0] ='\0';
      ptr = tempStr;
    }
    tempNum = str_atoi(ptr, str_len(ptr));
    if (statsReqType == SQLCLI_STATS_REQ_CPU)
      cpuMinRange = 0;
    else 
      cpuMinRange = -1;
    if (tempNum < cpuMinRange)
      return -1;
    cpu = (short)tempNum;
    pid = -1;
    break;
  case SQLCLI_STATS_REQ_PID:
  case SQLCLI_STATS_REQ_PROCESS_INFO:
    if (strncasecmp(statsReqStr, "CURRENT", 7) == 0)
    {
       cliGlobals = GetCliGlobals();
       pid = cliGlobals->myPin();
       cpu = cliGlobals->myCpu();
       break;
    }
    str_cpy_all(tempStr, statsReqStr, statsReqStrLen);
    tempStr[statsReqStrLen] = '\0';
    if ((ptr = str_chr(tempStr, '.')) != NULL)
    {
      *ptr++ = '\0';
      str_cpy_all(nodeName, tempStr, str_len(tempStr));
      nodeName[str_len(tempStr)] = '\0';
    }
    else
    {
      nodeName[0] = '\0';
      ptr = tempStr;
    }
    if (*ptr != '$')
    {
      cpuTempPtr = ptr;
      if ((ptr = str_chr(cpuTempPtr, ',')) != NULL)
      {
        *ptr++ = '\0';
        pinTempPtr = ptr;
      }
      else
        return -1;
      tempNum = str_atoi(cpuTempPtr, str_len(cpuTempPtr));
      if (tempNum < 0)
         return -1;
      cpu = (short)tempNum;
      tempNum = str_atoi(pinTempPtr, str_len(pinTempPtr));
      pid = (pid_t)tempNum;
    }
    else
    {
      if ((error = msg_mon_get_process_info(ptr, &tempCpu, &tempPid)) != XZFIL_ERR_OK)
         return -1;
      cpu = tempCpu;                                  
      pid = tempPid;                            
    }
    break;
   case SQLCLI_STATS_REQ_QID_INTERNAL:
    str_cpy_all(tempStr, statsReqStr, statsReqStrLen);
    tempStr[statsReqStrLen] = '\0';
    nodeName[0] = '\0';
    ptr = tempStr;
    cpuTempPtr = ptr;
    if ((ptr = str_chr(cpuTempPtr, ',')) != NULL)
    {
      *ptr++ = '\0';
      pinTempPtr = ptr;
    }
    else
      return -1;
    tempCpu = str_atoi(cpuTempPtr, str_len(cpuTempPtr));
    if (tempCpu < 0)
             return -1;
    cpu = tempCpu;
    if ((ptr = str_chr(pinTempPtr, ',')) != NULL)
    {
       *ptr++ = '\0';
       timeTempPtr = ptr;
    }
    else
       return -1;
    tempPid = str_atoi(pinTempPtr, str_len(pinTempPtr));
    if (tempPid < 0 )
        return -1;
    pid = (pid_t)tempPid;
    if ((ptr = str_chr(timeTempPtr, ',')) != NULL)
    {
       *ptr++ = '\0';
       queryNumTempPtr = ptr;
    }
    else
       return -1;
    tempTime = str_atoi(timeTempPtr, str_len(timeTempPtr));
    if (tempTime < 0)
       return -1;
    timeStamp = tempTime;
    tempQnum = str_atoi(queryNumTempPtr, str_len(queryNumTempPtr));
    if (tempQnum < 0)
       return -1;
    queryNumber = tempQnum;
    break;
  default:
    return -1;
  }
  return 0;

}

void ContextCli::killIdleMxcmp() 
{
  Int64 currentTimestamp;
  Int32 compilerIdleTimeout;
  Int64 recentIpcTimestamp ;
 
  if (arkcmpArray_.entries() == 0)
     return;
  if (arkcmpArray_[0]->getServer() == NULL)
     return;
  compilerIdleTimeout = getSessionDefaults()->getCompilerIdleTimeout();
  if (compilerIdleTimeout == 0)
     return;
  currentTimestamp = NA_JulianTimestamp();
  recentIpcTimestamp  = arkcmpArray_[0]->getRecentIpcTimestamp();
  if (recentIpcTimestamp != -1 && (((currentTimestamp - recentIpcTimestamp)/1000000)  >= compilerIdleTimeout))
     killAndRecreateMxcmp();
}

void ContextCli::killAndRecreateMxcmp()
{
  for (unsigned short i = 0; i < arkcmpArray_.entries(); i++)
     delete arkcmpArray_[i];

  arkcmpArray_.clear();
  arkcmpInitFailed_.clear();

  arkcmpArray_.insertAt(0,  new(exCollHeap()) ExSqlComp(0,
                               exCollHeap(),
                               cliGlobals_,0,
                               COM_VERS_COMPILER_VERSION, NULL, env_));
  arkcmpInitFailed_.insertAt(0, arkcmpIS_OK_);
}
   


// Initialize the database user ID from the OS user ID. If a row in
// the USERS table contains the OS user ID in the EXTERNAL_USER_NAME
// column, we switch to the user associated with that row. Otherwise
// this method has no side effects. Errors and warnings are written
// into diagsArea_.
void ContextCli::initializeUserInfoFromOS()
{


}

#pragma page "ContextCli::authQuery"
// *****************************************************************************
// *                                                                           *
// * Function: ContextCli::authQuery                                           *
// *                                                                           *
// *    Queries the ROLES or USERS tables for the specified authorization      *
// * name or ID.                                                               *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <queryType>                     AuthQueryType                   In       *
// *    is the type of query to perform.                                       *
// *                                                                           *
// *  <authName>                      const char *                    In       *
// *    is the name of the authorization entry to retrieve.  Optional field    *
// *  only required for queries that lookup by name, otherwise NULL.           *
// *                                                                           *
// *  <authID>                        int                             In       *
// *    is the name of the authorization entry to retrieve.  Optional field    *
// *  only supplied for queries that lookup by numeric ID.                     *
// *                                                                           *
// *  <authNameFromTable>             char *                          Out      *
// *    passes back the name of the authorization entry.                       *
// *                                                                           *
// *  <authNameMaxLen>                int                             In       *
// *    is the maximum number of characters that can be written to             *
// *  authNameFromTable.                                                       *
// *                                                                           *
// *  <authIDFromTable>              int                              Out      *
// *    passes back the numeric ID of the authorization entry.                 *
// *                                                                           *
// *  <roleIDs>                      std::vector<int32_t> &           Out      *
// *    passes back the list of number ID's for the user                       *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Returns: RETCODE                                                         *
// *                                                                           *
// *  SUCCESS: auth ID/name found, auth name/ID returned                       *
// *  ERROR: auth ID/name not found                                            *
// *                                                                           *
// *****************************************************************************

RETCODE ContextCli::authQuery(
   AuthQueryType          queryType,         
   const char           * authName,          
   Int32                  authID,            
   char                 * authNameFromTable, 
   Int32                  authNameMaxLen,
   Int32                & authIDFromTable,
   std::vector<int32_t> & roleIDs)   
   
{

   // We may need to perform a transactional lookup into metadata.
   // The following steps will be taken to manage the
   // transaction. The same steps are used in the Statement class when
   // we read the authorization information
   //
   //  1. Disable autocommit
   //  2. Note whether a transaction is already in progress
   //  3. Do the work
   //  4. Commit the transaction if a new one was started
   //  5. Enable autocommit if it was disabled

   // If we hit errors and need to inject the user name in error
   // messages, but the caller passed in a user ID not a name, we will
   // use the string form of the user ID.

   const char *nameForDiags = authName;
   char localNameBuf[32];
   char isValidFromUsersTable[3];

   if (queryType == USERS_QUERY_BY_USER_ID ||
       queryType == ROLE_QUERY_BY_ROLE_ID ||
       queryType == ROLES_QUERY_BY_AUTH_ID) 
   {
      sprintf(localNameBuf, "%d", (int) authID);
      nameForDiags = localNameBuf;
   }

   //  1. Disable autocommit 
   NABoolean autoCommitDisabled = FALSE;

   if (transaction_->autoCommit())
   {
      autoCommitDisabled = TRUE;
      transaction_->disableAutoCommit();
   }

   //  2. Note whether a transaction is already in progress
   NABoolean txWasInProgress = transaction_->xnInProgress();

   //  3. Do the work
   CmpSeabaseDDLauth::AuthStatus authStatus = CmpSeabaseDDLauth::STATUS_GOOD;
   RETCODE result = SUCCESS;
   CmpSeabaseDDLuser userInfo;
   CmpSeabaseDDLauth authInfo;
   CmpSeabaseDDLrole roleInfo;
   CmpSeabaseDDLauth *authInfoPtr = NULL;

   switch (queryType)
   {
      case USERS_QUERY_BY_USER_NAME:
      {
         authInfoPtr = &userInfo;
         authStatus = userInfo.getUserDetails(authName,false);
      }
      break;

      case USERS_QUERY_BY_EXTERNAL_NAME:
      {
         authInfoPtr = &userInfo;
         authStatus = userInfo.getUserDetails(authName,true);
      }
      break;

      case USERS_QUERY_BY_USER_ID:
      {
         authInfoPtr = &userInfo;
         authStatus = userInfo.getUserDetails(authID);
      }
      break;

      case AUTH_QUERY_BY_NAME:
      {
         authInfoPtr = &authInfo;
         authStatus = authInfo.getAuthDetails(authName,false);
      }
      break;
      
      case ROLE_QUERY_BY_ROLE_ID:
      {
         authInfoPtr = &roleInfo;
         authStatus = roleInfo.getAuthDetails(authID);
      }
      break;

      case ROLES_QUERY_BY_AUTH_ID:
      {
         authInfoPtr = &authInfo;
         std::vector<int32_t> roleIDs;
         std::vector<int32_t> granteeIDs;
         authStatus = authInfo.getRoleIDs(authID, roleIDs, granteeIDs);
         ex_assert((roleIDs.size() == granteeIDs.size()), "mismatch between roleIDs and granteeIDs");
         numRoles_ = roleIDs.size() + 1; // extra for public role
         roleIDs_ = new (&exHeap_) Int32[numRoles_];
         granteeIDs_ = new (&exHeap_) Int32[numRoles_];

         for (size_t i = 0; i < roleIDs.size(); i++)
         {
           roleIDs_[i] = roleIDs[i];
           granteeIDs_[i] = granteeIDs[i];
         }

         // Add the public user to the last entry
         Int32 lastEntry = numRoles_ - 1;
         roleIDs_[lastEntry] = PUBLIC_USER;
         granteeIDs_[lastEntry] = databaseUserID_;
      }   
      break;

      default:
      {
         ex_assert(0, "Invalid query type");
      }
      break;
   }
   
   if (authStatus == CmpSeabaseDDLauth::STATUS_GOOD)
   {
      //TODO: Check for valid auth 
      authIDFromTable = authInfoPtr->getAuthID();
      result = storeName(authInfoPtr->getAuthDbName().data(),authNameFromTable,
      			 authNameMaxLen);
                         
      if (result == ERROR)
         diagsArea_ << DgSqlCode(-CLI_USERNAME_BUFFER_TOO_SMALL);
   }
  
//  4. Commit the transaction if a new one was started
   if (!txWasInProgress && transaction_->xnInProgress())
      transaction_->commitTransaction();
  
//  5. Enable autocommit if it was disabled
   if (autoCommitDisabled)
      transaction_->enableAutoCommit();
    
// Errors to consider:
// * an unexpected error (authStatus == CmpSeabaseDDLauth::STATUS_ERROR)
// * the row does not exist (authStatus == CmpSeabaseDDLauth::STATUS_NOTFOUND)
// * row exists but is marked invalid
   switch (authStatus)
   {
      case CmpSeabaseDDLauth::STATUS_ERROR:
      {
         result = ERROR;
         ex_assert (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) > 0, "error getting user details");
         Int32 primaryError = CmpCommon::diags()->getErrorEntry(1)->getSQLCODE();
         diagsArea_ << DgSqlCode(-CLI_PROBLEM_READING_USERS)
                    << DgString0(nameForDiags)
                    << DgInt1(primaryError);
         break;
      }
      case CmpSeabaseDDLauth::STATUS_NOTFOUND:
      {
         result = ERROR;
         diagsArea_.clear();
         diagsArea_ << DgSqlCode(-CLI_USER_NOT_REGISTERED)
                    << DgString0(nameForDiags);
         break;
      }
      case CmpSeabaseDDLauth::STATUS_WARNING:
      {  
         // If warnings were generated, do not propagate them to the caller
         diagsArea_.clear();
         break;
      }
      case CmpSeabaseDDLauth::STATUS_GOOD:
      //TODO: Check for invalid user?
         break;
      case CmpSeabaseDDLauth::STATUS_UNKNOWN:
      default:
      {  
         *CmpCommon::diags() << DgSqlCode(-CLI_INTERNAL_ERROR) 	   
                             << DgString0("Unknown auth status in ContextCLI::authQuery"); 			   	  
      }
   }
          
   return result;

}
//*********************** End of ContextCli::authQuery *************************


// Public method to update the databaseUserID_ and databaseUserName_
// members and at the same time. It also calls dropSession() to create a
// session boundary.
void ContextCli::setDatabaseUser(const Int32 &uid, const char *uname)
{
  // Since this was moved from private to public, do some sanity checks
  // to make sure the passed in parameters are valid.  We don't want to
  // read any metadata since this could get into an infinte loop.
  ex_assert ((uid >= MIN_USERID && uid <= MAX_USERID), "Invalid userID was specified");
  ex_assert ((uname != NULL && strlen(uname) > 0), "No username was specified");

  // If the passed in credentials match what is stored, nothing needs
  // to be done.
  if (databaseUserID_ == uid &&
      strlen(uname) == strlen(databaseUserName_) &&
      str_cmp(databaseUserName_, uname, strlen(uname)) == 0)
    return;

  // Is this the place to drop any compiler contexts?
  dropSession();

  // Save changed user credentials
  databaseUserID_ = uid;
  strncpy(databaseUserName_, uname, MAX_DBUSERNAME_LEN);
  databaseUserName_[MAX_DBUSERNAME_LEN] = 0;
}

// Public method to establish a new user identity. userID will be
// verified against metadata in the AUTHS table. If a matching row is
// not found an error code will be returned and diagsArea_ populated.
RETCODE ContextCli::setDatabaseUserByID(Int32 userID)
{
  char username[MAX_USERNAME_LEN +1];
  Int32 userIDFromMetadata;
  std::vector<int32_t> roleIDs;

  // See if the USERS row exists
  RETCODE result = authQuery(USERS_QUERY_BY_USER_ID,
                             NULL,        // IN user name (ignored)
                             userID,      // IN user ID
                             username, //OUT
                             sizeof(username),
                             userIDFromMetadata,
                             roleIDs);    //OUT

  // Update the instance if the USERS lookup was successful
  if (result != ERROR)
    setDatabaseUser(userIDFromMetadata, username);

  return result;

}

// Public method to establish a new user identity. userName will be
// verified against metadata in the USERS table. If a matching row is
// not found an error code will be returned and diagsArea_ populated.
RETCODE ContextCli::setDatabaseUserByName(const char *userName)
{
  char usersNameFromUsersTable[MAX_USERNAME_LEN +1];
  Int32 userIDFromUsersTable;
  std::vector<int32_t> roleIDs;

  RETCODE result = authQuery(USERS_QUERY_BY_USER_NAME,
                             userName,    // IN user name
                             0,           // IN user ID (ignored)
                             usersNameFromUsersTable, //OUT
                             sizeof(usersNameFromUsersTable),
                             userIDFromUsersTable,
                             roleIDs);  // OUT

  // Update the instance if the lookup was successful
  if (result != ERROR)
     setDatabaseUser(userIDFromUsersTable, usersNameFromUsersTable);

  return result;
}


// ****************************************************************************
// *
// * Function: ContextCli::getRoleList
// *
// * Return the role IDs and their grantees for the current user.  
// *   If the list of roles is already stored, just return this list.
// *   If the list of roles does not exist extract the roles granted to the
// *     current user from the Metadata and store in roleIDs_.
// *
// ****************************************************************************
RETCODE ContextCli::getRoleList(
  Int32 &numEntries,
  Int32 *& roleIDs,
  Int32 *& granteeIDs)
{
  // If role list has not been created, go read metadata
  if (roleIDs_ == NULL)
  {
    // If authorization is not enabled, just setup the PUBLIC role
    CmpContext *cmpCntxt = CmpCommon::context();
    ex_assert(cmpCntxt, "No compiler context exists");
    if (!cmpCntxt->isAuthorizationEnabled())
    {
      numRoles_ = 1;
      roleIDs_ = new (&exHeap_) Int32[numRoles_];
      roleIDs_[0] = PUBLIC_USER;
      granteeIDs = new (&exHeap_) Int32[numRoles_];
      granteeIDs[0] = databaseUserID_;
      numEntries = numRoles_;
      roleIDs = roleIDs_;
      return SUCCESS;
    }

    // Get roles for userID
    char usersNameFromUsersTable[MAX_USERNAME_LEN +1];
    Int32 userIDFromUsersTable;
    std::vector<int32_t> myRoles;
    RETCODE result = authQuery (ROLES_QUERY_BY_AUTH_ID,
                                NULL,  // user name
                                databaseUserID_,
                                usersNameFromUsersTable, //OUT
                                sizeof(usersNameFromUsersTable),
                                userIDFromUsersTable,
                                myRoles);  // OUT
    if (result != SUCCESS)
      return result;
  }

  numEntries = numRoles_;
  roleIDs = roleIDs_;
  granteeIDs = granteeIDs_;

  return SUCCESS;
}
  
// ****************************************************************************
// *
// * Function: ContextCli::resetRoleList
// *
// * Deletes the current role list, a subsequent call to getRoleList reads the 
// * metadata to get the list of roles associated with the current user and, 
// * for the current user, stores the list in roleIDs_ and numRoles_ members
// *
// ****************************************************************************
RETCODE ContextCli::resetRoleList()
{
  if (roleIDs_)
    NADELETEBASIC(roleIDs_, &exHeap_);
  roleIDs_ = NULL;

  if (granteeIDs_)
    NADELETEBASIC(granteeIDs_, &exHeap_);
  granteeIDs_ = NULL;

  numRoles_ = 0;

  return SUCCESS;
}

// *****************************************************************************
// *                                                                           *
// * Function: ContextCli::getAuthIDFromName                                   *
// *                                                                           *
// *    Map an authentication name to a numeric ID.  If the name cannot be     *
// * mapped to a number, an error is returned.                                 *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <authName>                      const char *                    In       *
// *    is the authorization name to be mapped to a numeric ID.                *
// *                                                                           *
// *  <authID>                        Int32 &                         Out      *
// *    passes back the numeric ID.                                            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Returns: RETCODE                                                         *
// *                                                                           *
// *  SUCCESS: auth ID returned                                                *
// *  ERROR: auth name not found                                               *
// *                                                                           *
// *****************************************************************************

RETCODE ContextCli::getAuthIDFromName(
   const char *authName,  
   Int32 & authID)   
   
{

RETCODE result = SUCCESS;
char authNameFromTable[MAX_USERNAME_LEN + 1];

   if (authName == NULL)
      return ERROR;

// Cases to consider
// * authName is the current user name
// * SYSTEM_USER and PUBLIC_USER have special integer user IDs and
//   are not registered in the AUTHS table
// * other users

   if (databaseUserName_ && strcasecmp(authName,databaseUserName_) == 0)
   {
      authID = databaseUserID_;
      return SUCCESS;
   }

   if (strcasecmp(authName,ComUser::getPublicUserName()) == 0)
   {
      authID = ComUser::getPublicUserID();
      return SUCCESS;
   }

   if (strcasecmp(authName,ComUser::getSystemUserName()) == 0)
   {
      authID = ComUser::getSystemUserID();
      return SUCCESS;
   }
  
//TODO: If list of roles granted to user is cached in context, search there first.

   std::vector<int32_t> roleIDs;
   return authQuery(AUTH_QUERY_BY_NAME,authName,0,authNameFromTable,
                    sizeof(authNameFromTable),authID, roleIDs); 
                       
}
//******************* End of ContextCli::getAuthIDFromName *********************






// *****************************************************************************
// *                                                                           *
// * Function: ContextCli::getAuthNameFromID                                   *
// *                                                                           *
// *    Map an integer authentication ID to a name.  If the number cannot be   *
// * mapped to a name, the numeric ID is converted to a string.                *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <authID>                        Int32                           In       *
// *    is the numeric ID to be mapped to a name.                              *
// *                                                                           *
// *  <authName>                      char *                          Out      *
// *    passes back the name that the numeric ID mapped to.  If the ID does    *
// *  not map to a name, the ASCII equivalent of the ID is passed back.        *                                                                       
// *                                                                           *
// *  <maxBufLen>                     Int32                           In       *
// *    is the size of <authName>.                                             *
// *                                                                           *
// *  <requiredLen>                   Int32 &                         Out      *
// *    is the size of the auth name in the table.  If larger than <maxBufLen> *
// *  caller needs to supply a larger buffer.                                  *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Returns: RETCODE                                                         *
// *                                                                           *
// *  SUCCESS: auth name returned                                              *
// *  ERROR: auth name too small, see <requiredLen>                            *
// *                                                                           *
// *****************************************************************************

RETCODE ContextCli::getAuthNameFromID(
   Int32 authID,         // IN
   char *authName, // OUT
   Int32 maxBufLen,      // IN
   Int32 &requiredLen)   // OUT optional
   
{

   RETCODE result = SUCCESS;
   char authNameFromTable[MAX_USERNAME_LEN + 1];
   Int32 authIDFromTable;
   std::vector<int32_t> roleIDs;

   requiredLen = 0;
  
// Cases to consider
// * userID is the current user ID
// * SYSTEM_USER and PUBLIC_USER have special integer user IDs and
//   are not registered in the USERS table
// * other users

// If authID is the current user, return the name of the current database user.    
   if (authID == (Int32) databaseUserID_)
      return storeName(databaseUserName_,authName,maxBufLen,requiredLen);
      
//
// Determine the type of authID.  For PUBLIC and SYSTEM, use the hardcoded
// names (based on platform).
//
// For user and role (and eventually group, need to lookup in metadata.
//

   switch (ComUser::getAuthType(authID))
   {
      case ComUser::AUTH_USER:
         result = authQuery(USERS_QUERY_BY_USER_ID,
                            NULL,        // IN user name (ignored)
                            authID,      // IN user ID
                            authNameFromTable, //OUT
                            sizeof(authNameFromTable),
                            authIDFromTable,
                            roleIDs);  // OUT
         if (result == SUCCESS)
            return storeName(authNameFromTable,authName,maxBufLen,requiredLen);
      	 break;
      
      case ComUser::AUTH_ROLE:
         result = authQuery(ROLE_QUERY_BY_ROLE_ID,
                            NULL,        // IN user name (ignored)
                            authID,      // IN user ID
                            authNameFromTable, //OUT
                            sizeof(authNameFromTable),
                            authIDFromTable,
                            roleIDs);  // OUT
         if (result == SUCCESS)
            return storeName(authNameFromTable,authName,maxBufLen,requiredLen);
      	 break;
      default:
         ; //ACH Error?
   } 
//      
// Could not lookup auth ID.  Whether due to auth ID not defined, or SQL error,
// or any other cause, HPDM wants a valid string returned, so we use numeric 
// input value and convert it to a string.  Note, we still require the caller 
// (executor) to provide a buffer with sufficient space.
//

   sprintf(authNameFromTable, "%d", (int) authID);
   return storeName(authNameFromTable,authName,maxBufLen,requiredLen);
  
}
//******************* End of ContextCli::getAuthNameFromID *********************


// the private method to update user ID data members. On platforms
// other than Linux the method is a no-op.
void ContextCli::setDatabaseUserInESP(const Int32 &uid, const char *uname,
                                      bool closeAllOpens)
{
  // check if we need to close all opens to avoid share opens for different
  // users
  if (closeAllOpens)
    {
      if (databaseUserID_ != uid ||
          !strncmp(databaseUserName_, uname, MAX_DBUSERNAME_LEN))
        {
          setDatabaseUser(uid, uname);
        }
      // else, user hasn't changed
    }
  else
    setDatabaseUser(uid, uname);
}

void ContextCli::setAuthStateInCmpContexts(NABoolean authEnabled,
                                           NABoolean authReady)
{
  // change authorizationEnabled and authorizationReady state
  // in all the compiler contexts
  CmpContextInfo *cmpCntxtInfo;
  CmpContext *cmpCntxt;
  short i;
  for (i = 0; i < cmpContextInfo_.entries(); i++)
    {
      cmpCntxtInfo = cmpContextInfo_[i];
      cmpCntxt = cmpCntxtInfo->getCmpContext();
      cmpCntxt->setIsAuthorizationEnabled(authEnabled);
      cmpCntxt->setIsAuthorizationReady(authReady);
    }
}

void ContextCli::getAuthState(bool &authenticationEnabled,
                              bool &authorizationEnabled,
                              bool &authorizationReady,
                              bool &auditingEnabled)
{
  // Check for authentication status
  char * env = getenv("TRAFODION_ENABLE_AUTHENTICATION");
  if (env)
     authenticationEnabled = (strcmp(env, "YES") == 0) ? true : false;
  else
     authenticationEnabled = false;

  // Check authorization state
  CmpContext *cmpCntxt = CmpCommon::context();
  ex_assert(cmpCntxt, "No compiler context exists");
  authorizationEnabled = cmpCntxt->isAuthorizationEnabled();
  authorizationReady = cmpCntxt->isAuthorizationReady();

  // set auditingState to FALSE
  // TDB - add auditing support
  auditingEnabled = false;
}


void ContextCli::setUdrXactAborted(Int64 currTransId, NABoolean b)
{
  if (udrXactId_ != Int64(-1) && currTransId == udrXactId_)
    udrXactAborted_ = b;
}

void ContextCli::clearUdrErrorFlags()
{
  udrAccessModeViolation_ = FALSE;
  udrXactViolation_ = FALSE;
  udrXactAborted_ = FALSE;
  udrXactId_ = getTransaction()->getTransid();
}

NABoolean ContextCli::sqlAccessAllowed() const
{
  if (udrErrorChecksEnabled_ && (udrSqlAccessMode_ == (Lng32) COM_NO_SQL))
    return FALSE;
  return TRUE;
}

//********************** Switch CmpContext ************************************
Int32 ContextCli::switchToCmpContext(Int32 cmpCntxtType)
{
  if (cmpCntxtType < 0 || cmpCntxtType >= CmpContextInfo::CMPCONTEXT_TYPE_LAST)
    return -2;

  const char *cmpCntxtName = CmpContextInfo::getCmpContextClassName(cmpCntxtType);

  // find CmpContext_ with the same type to use
  CmpContextInfo *cmpCntxtInfo;
  CmpContext *cmpCntxt = NULL;
  short i;
  for (i = 0; i < cmpContextInfo_.entries(); i++)
    {
      cmpCntxtInfo = cmpContextInfo_[i];
      if (cmpCntxtInfo->getUseCount() == 0 &&
          (cmpCntxtType == CmpContextInfo::CMPCONTEXT_TYPE_NONE ||
           cmpCntxtInfo->isSameClass(cmpCntxtName)))
        {
          // found a CmpContext instance to switch to
          cmpCntxtInfo->incrUseCount();
          cmpCntxt = cmpCntxtInfo->getCmpContext();
          break;
        }
    }

  if (i == cmpContextInfo_.entries())
    {
      // find none to use, create new CmpContext instance
      CmpContext *savedCntxt = cmpCurrentContext;
      Int32 rc = 0;
      if (rc = arkcmp_main_entry())
        {
          cmpCurrentContext = savedCntxt;
          if (rc == 2)
            return -2; // error during NADefaults creation
          else
            return -1;  // failed to create new CmpContext instance
        }
      
      cmpCntxt = CmpCommon::context();
      cmpCntxt->setCIClass((CmpContextInfo::CmpContextClassType)cmpCntxtType);

      cmpCntxtInfo = new(exCollHeap()) CmpContextInfo(cmpCntxt, cmpCntxtName);
      cmpCntxtInfo->incrUseCount();
      cmpContextInfo_.insert(i, cmpCntxtInfo);
    }

  // save current and switch to the qualified CmpContext
  if (embeddedArkcmpContext_)
    cmpContextInUse_.insert(embeddedArkcmpContext_);
  embeddedArkcmpContext_ = cmpCntxt;
  cmpCurrentContext = cmpCntxt;  // restore the thread global

  return 0;  // success
}

Int32 ContextCli::switchToCmpContext(void *cmpCntxt)
{
  if (!cmpCntxt)
    return -1;  // invalid input

  // check if given CmpContext is known
  CmpContextInfo *cmpCntxtInfo;
  short i;
  for (i = 0; i < cmpContextInfo_.entries(); i++)
    {
      cmpCntxtInfo = cmpContextInfo_[i];
      if (cmpCntxtInfo->getCmpContext() == (CmpContext*) cmpCntxt)
        {
          // make sure the context is unused before switch
          assert(cmpCntxtInfo->getUseCount() == 0);
          cmpCntxtInfo->incrUseCount();
          break;
        }
    }

  // add the given CmpContext if not know
  if (i == cmpContextInfo_.entries())
    {
      cmpCntxtInfo = new(exCollHeap()) CmpContextInfo((CmpContext*)cmpCntxt, "NONE");
      cmpCntxtInfo->incrUseCount();
      cmpContextInfo_.insert(i, cmpCntxtInfo);
    }

  // save current and switch to given CmpContext
  if (embeddedArkcmpContext_)
    cmpContextInUse_.insert(embeddedArkcmpContext_);
  embeddedArkcmpContext_ = (CmpContext*) cmpCntxt;
  cmpCurrentContext = (CmpContext*) cmpCntxt;  // restore the thread global

  return (cmpCntxtInfo->getUseCount() == 1? 0: 1); // success
}

void ContextCli::copyDiagsAreaToPrevCmpContext()
{
  ex_assert(cmpContextInUse_.entries(), "Invalid use of switch back call");

  CmpContext *curr = embeddedArkcmpContext_;

  if (cmpContextInUse_.getLast(prevCmpContext_) == FALSE)
    return; 
  if (curr->diags()->getNumber() > 0)
     prevCmpContext_->diags()->mergeAfter(*curr->diags());
}

Int32 ContextCli::switchBackCmpContext(void)
{
  if (prevCmpContext_ == NULL) 
  {
     ex_assert(cmpContextInUse_.entries(), "Invalid use of switch back call");
     if (cmpContextInUse_.getLast(prevCmpContext_) == FALSE)
        return -1; 
  }
  // switch back
  CmpContext *curr = embeddedArkcmpContext_;

  embeddedArkcmpContext_ = prevCmpContext_;
  cmpCurrentContext = prevCmpContext_;  // restore the thread global

  // book keeping
  CmpContextInfo *cmpCntxtInfo;
  for (short i = 0; i < cmpContextInfo_.entries(); i++)
    {
      cmpCntxtInfo = cmpContextInfo_[i];
      if (cmpCntxtInfo->getCmpContext() == curr)
        {
          cmpCntxtInfo->decrUseCount();
          break;
        }
    }

  // switch back to a previous context
  cmpCurrentContext->switchBackContext();

  deinitializeArkcmp();
  prevCmpContext_ = NULL;

  return 0;  // success
}

// *****************************************************************************
// *                                                                           *
// * Function: ContextCli::storeName                                           *
// *                                                                           *
// *    Stores a name in a buffer.  If the name is too large, an error is      *
// * returned.                                                                 *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <src>                           const char *                    In       *
// *    is the name to be stored.                                              *
// *                                                                           *
// *  <dest>                          char *                          Out      *
// *    is the buffer to store the name into.                                  *
// *                                                                           *
// *  <maxLength>                     int                             In       *
// *    is the maximum number of bytes (including trailing null) that can      *
// *    be stored into <dest>.                                                 *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Returns: RETCODE                                                         *
// *                                                                           *
// *  SUCCESS: destination returned                                            *
// *  ERROR: destination too small                                             *
// *                                                                           *
// *****************************************************************************
RETCODE ContextCli::storeName(
   const char *src,
   char       *dest,
   int         maxLength)

{

int actualLength;

   return storeName(src,dest,maxLength,actualLength);

}
//*********************** End of ContextCli::storeName *************************

// *****************************************************************************
// *                                                                           *
// * Function: ContextCli::storeName                                           *
// *                                                                           *
// *    Stores a name in a buffer.  If the name is too large, an error is      *
// * returned along with the size of the name.                                 *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <src>                           const char *                    In       *
// *    is the name to be stored.                                              *
// *                                                                           *
// *  <dest>                          char *                          Out      *
// *    is the buffer to store the name into.                                  *
// *                                                                           *
// *  <maxLength>                     int                             In       *
// *    is the maximum number of bytes (including trailing null) that can      *
// *    be stored into <dest>.                                                 *
// *                                                                           *
// *  <actualLength>                  int &                           In       *
// *    is the number of bytes (not including the trailing null) in <src>,     *
// *    and <also dest>, if the store operation is successful.                 *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Returns: RETCODE                                                         *
// *                                                                           *
// *  SUCCESS: destination returned                                            *
// *  ERROR: destination too small, see <actualLength>                         *
// *                                                                           *
// *****************************************************************************
RETCODE ContextCli::storeName(
   const char *src,
   char       *dest,
   int         maxLength,
   int        &actualLength)

{

   actualLength = strlen(src);
   if (actualLength >= maxLength)
   {
      diagsArea_ << DgSqlCode(-CLI_USERNAME_BUFFER_TOO_SMALL);
      return ERROR;
   }
   
   memcpy(dest,src,actualLength);
   dest[actualLength] = 0;
   return SUCCESS;

}
//*********************** End of ContextCli::storeName *************************

CollIndex ContextCli::addTrustedRoutine(LmRoutine *r)
{
  // insert routine into a free array element and return its index
  CollIndex result = trustedRoutines_.freePos();

  ex_assert(r != NULL, "Trying to insert a NULL routine into CliContext");
  trustedRoutines_.insertAt(result, r);

  return result;
}

LmRoutine *ContextCli::findTrustedRoutine(CollIndex ix)
{
  if (trustedRoutines_.used(ix))
    return trustedRoutines_[ix];
  else
    {
      diags() << DgSqlCode(-CLI_ROUTINE_INVALID);
      return NULL;
    }
}

void ContextCli::putTrustedRoutine(CollIndex ix)
{
  // free element ix of the Routine array and delete the object
  ex_assert(trustedRoutines_[ix] != NULL,
            "Trying to delete a non-existent routine handle");

  LmResult res = LM_ERR;
  LmLanguageManager *lm = cliGlobals_->getLanguageManager(
       trustedRoutines_[ix]->getLanguage());

  if (lm)
    res = lm->putRoutine(trustedRoutines_[ix]);

  if (res != LM_OK)
    diags() << DgSqlCode(-CLI_ROUTINE_DEALLOC_ERROR);
  trustedRoutines_.remove(ix);
}

// This method looks for an hdfsServer connection with the given hdfsServer 
// name and port. If the conection does not exist, a new connection is made and 
// cached in the hdfsHandleList_ hashqueue for later use. The hdfsHandleList_ 
// gets cleaned up when the thread exits.
hdfsFS ContextCli::getHdfsServerConnection(char * hdfs_server, Int32 port)
{
  if (hdfs_server == NULL) // guard against NULL and use default value.
    {
      hdfs_server = (char *)"default";
      port = 0;
    }
  if (hdfsHandleList_)
    {
      // Look for the entry on the list
      hdfsHandleList_->position(hdfs_server,strlen(hdfs_server));
      hdfsConnectStruct *hdfsConnectEntry = NULL;
      while (hdfsConnectEntry = (hdfsConnectStruct *)(hdfsHandleList_->getNext()))
        {
          if ((strcmp(hdfsConnectEntry->hdfsServer_,hdfs_server)==0) && 
              (hdfsConnectEntry->hdfsPort_ == port))
            return (hdfsConnectEntry->hdfsHandle_);
          
        }
    }

  // If not found create a new one and add to list.
  hdfsFS newFS = hdfsConnect(hdfs_server,port);
  hdfsConnectStruct *hdfsConnectEntry = new (exCollHeap()) hdfsConnectStruct;
  memset(hdfsConnectEntry,0,sizeof(hdfsConnectStruct));  
  hdfsConnectEntry->hdfsHandle_ = newFS;
  hdfsConnectEntry->hdfsPort_ = port;
  str_cpy_all(hdfsConnectEntry->hdfsServer_,hdfs_server,str_len(hdfs_server));
  hdfsHandleList_->insert(hdfs_server,strlen(hdfs_server),hdfsConnectEntry);
   
  return newFS;   
    
}

void ContextCli::disconnectHdfsConnections()
{
  if (hdfsHandleList_)
    {
      hdfsConnectStruct * entry = NULL;
      while(entry = (hdfsConnectStruct *)hdfsHandleList_->getNext())
        {
          hdfsDisconnect(entry->hdfsHandle_);         
        }
    }
}

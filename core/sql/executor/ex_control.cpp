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
 * File:         ex_control.cpp
 * Description:  
 *               
 *               
 * Created:      5/15/1998
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include  "cli_stdh.h"
#include  "ex_stdh.h"

#include  "ComTdb.h"
#include  "ex_tcb.h"
#include  "ex_exe_stmt_globals.h"
#include  "ex_frag_rt.h"
#include  "ex_control.h"
#include  "ex_error.h"
#include  "ComQueue.h"
#include  "ComSqlId.h"
#include  "ExSqlComp.h"
#include  "ExControlArea.h"
#include  "exp_clause_derived.h"
#include  "ExpHbaseInterface.h"
#include  "ExExeUtil.h"
#include  "ex_ddl.h"
#include  "ComRtUtils.h"
#include  "ComUser.h"
#include  "CmpContext.h"

ex_tcb * ExControlTdb::build(ex_globals * glob)
{
  ExControlTcb * control_tcb = NULL;

  if (getType() == SESSION_DEFAULT_)
    control_tcb = new(glob->getSpace()) ExSetSessionDefaultTcb(*this, glob);
  else
    control_tcb = new(glob->getSpace()) ExControlTcb(*this, glob);

  control_tcb->registerSubtasks();

  return (control_tcb);
}


////////////////////////////////////////////////////////////////
// Constructor for class ExControlTcb
///////////////////////////////////////////////////////////////
ExControlTcb::ExControlTcb(const ExControlTdb & control_tdb,
		     ex_globals * glob)
  : ex_tcb( control_tdb, 1, glob)
{
  Space * space = (glob ? glob->getSpace() : 0);
  
  // Allocate the buffer pool
  pool_ = new(space) sql_buffer_pool(control_tdb.numBuffers_,
				     (Lng32) control_tdb.bufferSize_,
				     space);
  
  // Allocate the queue to communicate with parent
  qparent_.down = new(space) ex_queue(ex_queue::DOWN_QUEUE,
				      control_tdb.queueSizeDown_,
				      control_tdb.criDescDown_,
				      space);
  
  // Allocate the private state in each entry of the down queue
  ExControlPrivateState *p = new(space) ExControlPrivateState(this);
  qparent_.down->allocatePstate(p, this);
  delete p;
  
  qparent_.up = new(space) ex_queue(ex_queue::UP_QUEUE,
				    control_tdb.queueSizeUp_,
				    control_tdb.criDescUp_,
				    space);
}
  

ExControlTcb::~ExControlTcb()
{
  freeResources();
};                                                    

void ExControlTcb::freeResources()
{
  delete qparent_.up;
  delete qparent_.down;

  if (pool_)
    delete pool_;
  pool_ = 0;
}

//////////////////////////////////////////////////////
// work() for ExControlTcb
//////////////////////////////////////////////////////
short ExControlTcb::work()
{
  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;
  
  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;

  ContextCli *currContext =
      getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals()->
        getStatement()->getContext();
  
  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  // ExControlPrivateState & pstate = *((ExControlPrivateState*) pentry_down->pstate);
  
  // Construct a sql text string "CONTROL xxx token 'value';"
  // to send to arkcmp, which, in binding it, will set its (arkcmp's)
  // NADefaults state.
  //
  // These are infrequently executed, sufficiently so to NOT warrant
  // a special arkcmp request which would do away with all the extra
  // steps (parse/optimize/codegen: all but the bind step)
  // of this SQLTEXT_STATIC_COMPILE request.
  //
  // ## Will need to save charset to pass along to arkcmp, too... (?)
  //
  // ## The ExControlArea (comexe/ComTdbControl.cpp)
  // ## will need to do something similar to this....


  char *buf     = controlTdb().getSqlText();
  Int32   usedlen = buf ? str_len(buf) + 1 : 0;
  Lng32  sqlTextCharSet = (Lng32)(buf ? controlTdb().getSqlTextCharSet() : SQLCHARSETCODE_UNKNOWN);

  // array[1+max] because value() is 1-based not 0-based.
  // we do init the unreferenced [0] elements, just in case.
  //
  #define MAXVALS  3			// value(1 to 3)
  char *value[1+MAXVALS] = { NULL };
  Int32   len[1+MAXVALS]   = { 0 };

  Int32 i = 1;
  for (; i <= MAXVALS; i++)
    if (value[i] = controlTdb().getValue(i))	// assignment, not == ...!
      {
	len[i] = str_len(value[i]);
      }
    else
      len[i] = 0;

  char *dummyReply = NULL;
  ULng32 dummyLen;
  
  // if this is SET SCHEMA stmt of a HIVE schema, check that the schema
  // exists. This is to be consistent with USE <database> functionality
  // of Hive where a schema must exist before USE stmt can be issued.
  // An error is returned if it does not exist.
  if ((controlTdb().isSetStmt()) &&
      (controlTdb().isHiveSetSchema()))
    {
      // set schema hive.<sch> stmt
      // Check that it exists in Hive.
      ComSchemaName csn(value[2]);
      NAString hiveDB = ComConvertTrafHiveNameToNativeHiveName
        (csn.getCatalogNamePart().getInternalName(),
         csn.getSchemaNamePart().getInternalName(),
         NAString(""));

      NAString useDB("use " + hiveDB);
      if (HiveClient_JNI::executeHiveSQL(useDB.data()) != HVC_OK)
        {
          ComDiagsArea *da = 
            ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
          if (NAString(getSqlJniErrorStr()).contains("Database does not exist:"))
            *da << DgSqlCode(-1003)
                << DgString0(HIVE_SYSTEM_CATALOG)
                << DgString1(hiveDB);
          else
            *da << DgSqlCode(-1214)
                << DgString0(getSqlJniErrorStr())
                << DgString1(useDB.data());
          
          ExHandleArkcmpErrors(qparent_, pentry_down, 0,
                               getGlobals(), da);

          ex_queue_entry * up_entry = qparent_.up->getTailEntry();
          
          up_entry->upState.parentIndex = 
            pentry_down->downState.parentIndex;
          
          up_entry->upState.setMatchNo(0);
          up_entry->upState.status = ex_queue::Q_NO_DATA;
          
          // insert into parent
          qparent_.up->insert();
          
          qparent_.down->removeHead();
          
          return WORK_OK;
        }      
    }

  // Only a STATIC compile will actually affect Arkcmp's context.
  CmpCompileInfo c(buf, usedlen, sqlTextCharSet, NULL, 0, 0, 0);
  size_t dataLen = c.getLength();
  NABoolean saveControl = FALSE;  // if save controls in exe ControlInfoTables
  char * data = new(getHeap()) char[dataLen];
  c.pack(data);
  ContextCli *currCtxt = getGlobals()->castToExExeStmtGlobals()->
    castToExMasterStmtGlobals()->getStatement()->getContext();
  ExSqlComp *cmp = NULL;
  Int32 cmpStatus = 0;
  {
    // If use embedded compiler, send the settings to it
    if (currCtxt->getSessionDefaults()->callEmbeddedArkcmp() &&
        currCtxt->isEmbeddedArkcmpInitialized() && (CmpCommon::context()) &&
	(CmpCommon::context()->getRecursionLevel() == 0) )
      {
         
        NAHeap *arkcmpHeap = currCtxt->exHeap();
	ComDiagsArea *da = NULL;
        cmpStatus = CmpCommon::context()->compileDirect(
                               (char *) data, dataLen, arkcmpHeap,
                               SQLCHARSETCODE_UTF8,
                               EXSQLCOMP::SQLTEXT_STATIC_COMPILE,
                               dummyReply, dummyLen, 
                               currCtxt->getSqlParserFlags(), 
                               NULL, 0, da);
        if (cmpStatus != 0)
          {
            char *emsText = new (getHeap()) char[dataLen + 120];
            str_sprintf(emsText,
                        "Set control to embedded arkcmp failed, return code %d, data: %s",
                        cmpStatus, data);
            SQLMXLoggingArea::logExecRtInfo(__FILE__, __LINE__, emsText, 0);
            ExHandleArkcmpErrors(qparent_, pentry_down, 0,
                                 getGlobals(), da);
            
	    // da->clear();
            getHeap()->deallocateMemory(emsText);
            if (da != NULL)
               da->decrRefCount();
          }
        else
          saveControl = TRUE; // need to save control to exe ControlInfoTable

        if (dummyReply != NULL)
          {
            arkcmpHeap->deallocateMemory((void*)dummyReply);
            dummyReply = NULL;
          }
      }

    // if there is an error using embedded compiler or we are already in the 
    // embedded compiler, send this to a regular compiler .
    if (!currCtxt->getSessionDefaults()->callEmbeddedArkcmp()  ||
	(currCtxt->getSessionDefaults()->callEmbeddedArkcmp() && (CmpCommon::context()) &&
	 ((cmpStatus != 0) || (CmpCommon::context()->getRecursionLevel() > 0)) ||currCtxt->getArkcmp()->getServer() )

	) 
      {
	for (short i = 0; i < currCtxt->getNumArkcmps();i++)
	  {
	    // May not want to send controls to all compilers - ask context. 
	    if ( currCtxt->sendSettingsToCompiler (i) )
	      {
		cmp = currCtxt->getArkcmp(i);
		ExSqlComp::ReturnStatus cmpStatus = 
		  cmp->sendRequest(
				   EXSQLCOMP::SQLTEXT_STATIC_COMPILE, data, dataLen,
				   TRUE, NULL,
				   SQLCHARSETCODE_UTF8,
				   TRUE /*resend, if needed*/
				   );

		if (cmpStatus == ExSqlComp::SUCCESS)
		  cmpStatus = cmp->getReply(dummyReply, dummyLen);
		if (dummyReply != NULL)
		  cmp->getHeap()->deallocateMemory((void*)dummyReply);

		// Handle it if EITHER of these two cmp->Xxx calls failed.
		if (cmpStatus != ExSqlComp::SUCCESS)
		  {
		    // reset setOnce cqds.
		    currCtxt->resetSqlParserFlags(0x400000);

		    if ( 
			((*(cmp->getDiags())).contains(-2050) || 
			 (*(cmp->getDiags())).contains(-2055))  &&
			(cmp->getVersion() < COM_VERS_COMPILER_VERSION)
			 )
		      cmp->getDiags()->clear();
		    else
		      ExHandleArkcmpErrors(qparent_, pentry_down, 0,
					   getGlobals(), cmp->getDiags());
		  }
		else
		  saveControl = TRUE;
	      }  // sendSettingsToCompiler(i) is true
	  }  // i < currCtxt->getNumArkcmps()
      } //cmpStatus= 0) || (CmpCommon::context()->getRecursionLevel() > 0

    if (saveControl)
      {
        // Add control info to executor ControlInfoTable.
        currCtxt->
          getControlArea()->addControl(
               controlTdb().getType(),
               controlTdb().reset_,
               buf,      usedlen,
               value[1], len[1],
               value[2], len[2],
               value[3], len[3],
               controlTdb().getControlActionType(),
               ExControlEntry::UPON_ALL,
               controlTdb().isNonResettable());
        
        // reset processing of setOnce cqds if a cqd * reset reset
        // is seen while the setOnce parserflags is set.
        // This indicates that ndcs has sent a cqd * reset reset
        // after setting all the cqds from its DSN.
        if (((currCtxt->getSqlParserFlags() & 0x400000) != 0) &&
            (controlTdb().getType() == DEFAULT_) &&
            (controlTdb().reset_ == -1) &&
            (value[1] != NULL) &&
            (len[1] == 0) &&
            (value[2] != NULL) &&
            (! str_cmp_ne(value[2], "RESET")))
          {
            currCtxt->resetSqlParserFlags(0x400000);
          }
        
        // Add cqds which are also ssds(set session defaults) to
        // the session defaults table.
        if (controlTdb().getType() == DEFAULT_)
          {
            if (strcmp(value[1], "AUTO_QUERY_RETRY") == 0)
              {
                if (strcmp(value[2], "OFF") == 0)
                  currCtxt->getSessionDefaults()->setAqrType(0);
                else if (strcmp(value[2], "SYSTEM") == 0)
                  currCtxt->getSessionDefaults()->setAqrType(1);
                else if (strcmp(value[2], "ON") == 0)
                  currCtxt->getSessionDefaults()->setAqrType(2);
              }
            else if (strcmp(value[1], "AUTO_QUERY_RETRY_WARNINGS") == 0)
              {
                if (strcmp(value[2], "ON") == 0)
                  currCtxt->getSessionDefaults()->setAQRWarnings(1);
                else
                  currCtxt->getSessionDefaults()->setAQRWarnings(0);
              }
            else if (strcmp(value[1], "CATALOG") == 0)
              {
                currContext->getSessionDefaults()->
                  setCatalog(value[2], strlen(value[2]));
              }
            else if (strcmp(value[1], "DBTR_PROCESS") == 0)
              {
                if (strcmp(value[2], "ON") == 0)
                  currContext->getSessionDefaults()->setDbtrProcess(TRUE);
              }
            else if (strcmp(value[1], "IS_SQLCI") == 0)
              {
                if (strcmp(value[2], "ON") == 0)
                  currContext->getSessionDefaults()->setMxciProcess(TRUE);
              }
            else if (strcmp(value[1], "NVCI_PROCESS") == 0)
              {
                if (strcmp(value[2], "ON") == 0)
                  {
                    currContext->getSessionDefaults()->setNvciProcess(TRUE);
                    currContext->getSessionDefaults()->setStatisticsViewType(SQLCLI_PERTABLE_STATS);
                  }
              }
            else if (strcmp(value[1], "ODBC_PROCESS") == 0)
              {
                if (strcmp(value[2], "ON") == 0)
                  {
                    currContext->getSessionDefaults()->setOdbcProcess(TRUE);
                    currContext->getSessionDefaults()->setStatisticsViewType(SQLCLI_PERTABLE_STATS);
                  }
              }
            else if (strcmp(value[1], "MARIAQUEST_PROCESS") == 0)
              {
                if (strcmp(value[2], "ON") == 0)
                  {
                    currContext->getSessionDefaults()->setMariaQuestProcess(TRUE);
                  }
              }
            else if (strcmp(value[1], "JDBC_PROCESS") == 0)
              {
                if (strcmp(value[2], "ON") == 0)
                  {
                    currContext->getSessionDefaults()->setJdbcProcess(TRUE);
                    currContext->getSessionDefaults()->setStatisticsViewType(SQLCLI_PERTABLE_STATS);
                  }
              }
		    else if (strcmp(value[1], "CANCEL_QUERY_ALLOWED") == 0)
		      {
			if (stricmp(value[2], "OFF") == 0)
			  currContext->getSessionDefaults()->setCancelQueryAllowed(FALSE);
                        else
			  currContext->getSessionDefaults()->setCancelQueryAllowed(TRUE);
                      }
            else if (strcmp(value[1], "MODE_SEABASE") == 0)
              {
                if (strcmp(value[2], "ON") == 0)
                  currContext->getSessionDefaults()->setModeSeabase(TRUE);
                else 
                  currContext->getSessionDefaults()->setModeSeabase(FALSE);
              }
            else if (strcmp(value[1], "SCHEMA") == 0)
              {
                currContext->getSessionDefaults()->
                  setSchema(value[2], strlen(value[2]));
              }
            else if (strcmp(value[1], "USE_LIBHDFS") == 0)
              {
                if (strcmp(value[2], "ON") == 0)
                  currContext->getSessionDefaults()->setUseLibHdfs(TRUE);
                else 
                  currContext->getSessionDefaults()->setUseLibHdfs(FALSE);
              } 
            else if (strcmp(value[1], "USER_EXPERIENCE_LEVEL") == 0)
              {
                currContext->getSessionDefaults()->
                  setUEL(value[2], strlen(value[2]));
              }
            else if (strcmp(value[1], "REDRIVE_CTAS") == 0)
              {
                if (strcmp(value[2], "ON") == 0)
                  currContext->getSessionDefaults()->setRedriveCTAS(TRUE);
                else
                  currContext->getSessionDefaults()->setRedriveCTAS(FALSE);
              }
            else if (strcmp(value[1], "EXSM_TRACE_FILE_PREFIX") == 0)
              {
                char *prefix = value[2];
                if (prefix != NULL)
                  currContext->getSessionDefaults()->
                    setExSMTraceFilePrefix(prefix, strlen(prefix));
              }
            else if (strcmp(value[1], "EXSM_TRACE_LEVEL") == 0)
              {
                int lvl = (int) strtoul(value[2], NULL, 10);
                currContext->getSessionDefaults()->
                  setExSMTraceLevel(lvl);
              }
            else if (strcmp(value[1], "CALL_EMBEDDED_ARKCMP") == 0)
              {
                if ((strcmp(value[2], "TRUE") == 0) || 
                    (strcmp(value[2], "ON") == 0)
                    )
                  {
                    currContext->getSessionDefaults()->setCallEmbeddedArkcmp(TRUE);
                  }
                else if  ((strcmp(value[2], "FALSE") == 0) || 
                          (strcmp(value[2], "OFF") == 0)
                          )
                  {
                    currContext->getSessionDefaults()->setCallEmbeddedArkcmp(FALSE);
                  }
              }
            else if (strcmp(value[1], "ESP_IDLE_TIMEOUT") == 0)
              {
                int lvl = (int) strtoul(value[2], NULL, 10);
                currContext->getSessionDefaults()->
                  setEspIdleTimeout(lvl);
              }
            else if (strcmp(value[1], "COMPILER_IDLE_TIMEOUT") == 0)
              {
                int lvl = (int) strtoul(value[2], NULL, 10);
                currContext->getSessionDefaults()->
                  setCompilerIdleTimeout(lvl);
              }
          }
      }
  }
  getHeap()->deallocateMemory(data);

  // all ok. Return EOF.
  ex_queue_entry * up_entry = qparent_.up->getTailEntry();

  up_entry->upState.parentIndex = 
    pentry_down->downState.parentIndex;
  
  up_entry->upState.setMatchNo(0);
  up_entry->upState.status = ex_queue::Q_NO_DATA;
  
  // insert into parent
  qparent_.up->insert();
  
  qparent_.down->removeHead();
  
  return WORK_OK;
}

//////////////////////////////////////////////////////
// work() for ExSetSessionDefaultTcb
//////////////////////////////////////////////////////
short ExSetSessionDefaultTcb::work()
{
  // if no parent request, return
  if (getParentQueue().down->isEmpty())
    return WORK_OK;
  
  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (getParentQueue().up->isFull())
    return WORK_OK;

  NABoolean changeMasterPriority = FALSE;
  NABoolean changeMxcmpPriority = FALSE;
  NABoolean changeEspPriority = FALSE;
  NABoolean masterIsDelta = FALSE;
  NABoolean mxcmpIsDelta = FALSE;
  NABoolean espIsDelta = FALSE;
  NABoolean dropVolatileSchema = FALSE;
  Lng32 defaultValueAsLong = -1;
  NABoolean computeDefValAsLong = TRUE;
 
  ContextCli *currContext =
      getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals()->
        getStatement()->getContext();
  IpcEnvironment *ipcEnv = 
      getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals()->
        getCliGlobals()->getEnvironment();
    
  ex_queue_entry * pentry_down = getParentQueue().down->getHeadEntry();
  ex_queue_entry * up_entry = getParentQueue().up->getTailEntry();

  char * defaultName = controlTdb().getValue(1);
  char * defaultValue =  controlTdb().getValue(2);
  
  // Display error message if it's not a default we know about.

  if ((strcmp(defaultName, "ALTPRI_MASTER") != 0) &&
      (strcmp(defaultName, "ALTPRI_MASTER_SEQ_EXE") != 0) &&
      (strcmp(defaultName, "ALTPRI_ESP") != 0) &&
      (strcmp(defaultName, "IS_DB_TRANPORTER") != 0) &&
      (strcmp(defaultName, "AQR_ENTRIES") != 0) &&
      (strcmp(defaultName, "MASTER_PRIORITY") != 0) &&
      (strcmp(defaultName, "MASTER_PRIORITY_DELTA") != 0) &&
      (strcmp(defaultName, "ESP_PRIORITY") != 0) &&
      (strcmp(defaultName, "ESP_PRIORITY_DELTA") != 0) &&
      (strcmp(defaultName, "ESP_FIXUP_PRIORITY") != 0) &&
      (strcmp(defaultName, "ESP_FIXUP_PRIORITY_DELTA") != 0) &&
      (strcmp(defaultName, "ESP_ASSIGN_DEPTH") != 0) &&
      (strcmp(defaultName, "ESP_ASSIGN_TIME_WINDOW") != 0) &&
      (strcmp(defaultName, "ESP_STOP_IDLE_TIMEOUT") != 0) &&
      (strcmp(defaultName, "ESP_IDLE_TIMEOUT") != 0) &&
      (strcmp(defaultName, "COMPILER_IDLE_TIMEOUT") != 0) &&
      (strcmp(defaultName, "ESP_INACTIVE_TIMEOUT") != 0) &&
      (strcmp(defaultName, "ESP_RELEASE_WORK_TIMEOUT") != 0) &&
      (strcmp(defaultName, "MAX_POLLING_INTERVAL") != 0) &&
      (strcmp(defaultName, "PERSISTENT_OPENS") != 0) &&
      (strcmp(defaultName, "ESP_FREEMEM_TIMEOUT") != 0) &&
      (strcmp(defaultName, "ESP_CLOSE_ERROR_LOGGING") != 0) &&
      (strcmp(defaultName, "INTERNAL_FORMAT_IO") != 0) &&
      (strcmp(defaultName, "ISO_MAPPING") != 0) &&
      (strcmp(defaultName, "MXCMP_PRIORITY") != 0) &&
      (strcmp(defaultName, "MXCMP_PRIORITY_DELTA") != 0) &&
      (strcmp(defaultName, "CLI_BULKMOVE") != 0) &&
      (strcmp(defaultName, "SQL_SESSION") != 0) &&
      (strcmp(defaultName, "SQL_USER") != 0) &&
      (strcmp(defaultName, "ROWSET_ATOMICITY") != 0) &&
      (strcmp(defaultName, "RTS_TIMEOUT") != 0) &&
      (strncmp(defaultName, "SET_ENVVAR_", strlen("SET_ENVVAR_")) != 0) &&
      (strncmp(defaultName, "RESET_ENVVAR_", strlen("RESET_ENVVAR_")) != 0) &&
      (strcmp(defaultName, "SET_PARSERFLAGS") != 0) &&
      (strcmp(defaultName, "RESET_PARSERFLAGS") != 0) &&
      (strcmp(defaultName, "PARENT_QID") != 0) &&
      (strcmp(defaultName, "PARENT_QID_SYSTEM") != 0) &&
      (strcmp(defaultName, "CANCEL_ESCALATION_INTERVAL") != 0) &&
      (strcmp(defaultName, "CANCEL_ESCALATION_MXOSRVR_INTERVAL") != 0) &&
      (strcmp(defaultName, "CANCEL_ESCALATION_SAVEABEND") != 0) &&
      (strcmp(defaultName, "CANCEL_QUERY_ALLOWED") != 0) &&
      (strcmp(defaultName, "CANCEL_UNIQUE_QUERY") != 0) &&
      (strcmp(defaultName, "CANCEL_LOGGING") != 0) &&
      (strcmp(defaultName, "SUSPEND_LOGGING") != 0) &&
      (strcmp(defaultName, "STATISTICS_VIEW_TYPE") != 0) &&
      (strcmp(defaultName, "RECLAIM_MEMORY_AFTER") != 0) &&
      (strcmp(defaultName, "RECLAIM_FREE_MEMORY_RATIO") != 0) &&
      (strcmp(defaultName, "CALL_EMBEDDED_ARKCMP") != 0) &&
      (strcmp(defaultName, "RECLAIM_FREE_PFS_RATIO") != 0) && 
      (strcmp(defaultName, "EXSM_TRACE_FILE_PREFIX") != 0) &&
      (strcmp(defaultName, "EXSM_TRACE_LEVEL") != 0) &&
      (strcmp(defaultName, "OSIM") != 0)
      )
    {
      ExHandleErrors(qparent_,
		     pentry_down,
		     0,
		     getGlobals(),
		     NULL,
		     (ExeErrorCode)(-EXE_INVALID_SESSION_DEFAULT),
		     NULL,
		     defaultName);
      goto all_ok;
    }

  if ((strcmp(defaultName, "AQR_ENTRIES") == 0) ||
      (strcmp(defaultName, "CLI_BULKMOVE") == 0) ||
      (strcmp(defaultName, "SQL_SESSION") == 0) ||
      (strcmp(defaultName, "SQL_USER") == 0) ||
      (strcmp(defaultName, "ALTPRI_MASTER") == 0) ||
      (strcmp(defaultName, "ALTPRI_MASTER_SEQ_EXE") == 0) ||
      (strcmp(defaultName, "ALTPRI_ESP") == 0) ||
      (strcmp(defaultName, "IS_DB_TRANSPORTER") == 0) ||
      (strcmp(defaultName, "INTERNAL_FORMAT_IO") == 0) ||
      (strcmp(defaultName, "ISO_MAPPING") == 0) ||
      (strncmp(defaultName, "SET_ENVVAR_", strlen("SET_ENVVAR_")) == 0) ||
      (strncmp(defaultName, "RESET_ENVVAR_", strlen("RESET_ENVVAR_")) == 0) ||
      (strcmp(defaultName, "PARENT_QID") == 0) ||
      (strcmp(defaultName, "PARENT_QID_SYSTEM") == 0) ||
      (strcmp(defaultName, "CANCEL_ESCALATION_SAVEABEND") == 0) ||
      (strcmp(defaultName, "CANCEL_QUERY_ALLOWED") == 0) ||
      (strcmp(defaultName, "CANCEL_UNIQUE_QUERY") == 0) ||
      (strcmp(defaultName, "CANCEL_LOGGING") == 0) ||
      (strcmp(defaultName, "SUSPEND_LOGGING") == 0) ||
      (strcmp(defaultName, "CALL_EMBEDDED_ARKCMP") == 0) ||
      (strcmp(defaultName, "STATISTICS_VIEW_TYPE") == 0) ||
      (strcmp(defaultName, "EXSM_TRACE_LEVEL") == 0) ||
      (strcmp(defaultName, "EXSM_TRACE_FILE_PREFIX") == 0) ||
      (strcmp(defaultName, "OSIM") == 0)
      )
    computeDefValAsLong = FALSE;
  if (computeDefValAsLong)
    {
      ex_expr::exp_return_type rc =
	convDoIt(defaultValue, (Lng32)strlen(defaultValue), REC_BYTE_F_ASCII, 
		 0, 0,
		 (char*)&defaultValueAsLong, sizeof(Lng32), REC_BIN32_SIGNED, 
		 0, 0, 
		 NULL, 0);
      if (rc != ex_expr::EXPR_OK)
	{
	  ExHandleErrors(qparent_,
			 pentry_down,
			 0,
			 getGlobals(),
			 NULL,
			 (ExeErrorCode)(-EXE_NUMERIC_OVERFLOW));

          // Error details have been inserted into qparent_. Go to all_ok.
          goto all_ok;
	}
    }

  if (strcmp(defaultName, "MASTER_PRIORITY") == 0)
    {
      changeMasterPriority = TRUE;
      changeMxcmpPriority  = TRUE;
      changeEspPriority  = TRUE;
      mxcmpIsDelta = TRUE;
      espIsDelta = TRUE;
    }
  else if (strcmp(defaultName, "MASTER_PRIORITY_DELTA") == 0)
    {
      changeMasterPriority = TRUE;
      changeMxcmpPriority  = TRUE;
      changeEspPriority  = TRUE;
      masterIsDelta = TRUE;
      mxcmpIsDelta = TRUE;
      espIsDelta = TRUE;
    }
  else if (strcmp(defaultName, "ESP_PRIORITY") == 0)
    {
      currContext->getSessionDefaults()->setEspPriority(defaultValueAsLong);
      changeEspPriority = TRUE;
    }
  else if (strcmp(defaultName, "ESP_PRIORITY_DELTA") == 0)
    {
      currContext->getSessionDefaults()->setEspPriorityDelta(defaultValueAsLong);
      changeEspPriority = TRUE;
      espIsDelta = TRUE;
    }
  else if (strcmp(defaultName, "ESP_FIXUP_PRIORITY") == 0)
    {
      currContext->getSessionDefaults()
                 ->setEspFixupPriority(defaultValueAsLong);
    }
  else if (strcmp(defaultName, "ESP_FIXUP_PRIORITY_DELTA") == 0)
    {
      currContext->getSessionDefaults()
                 ->setEspFixupPriorityDelta(defaultValueAsLong);
    }
  else if (strcmp(defaultName, "ESP_ASSIGN_DEPTH") == 0)
    {
      currContext->getSessionDefaults()
                 ->setEspAssignDepth(defaultValueAsLong);
    }
  else if (strcmp(defaultName, "ESP_ASSIGN_TIME_WINDOW") == 0)
    {
      // The minimum default time window should be 30 seconds
      // to avoid EPS dying during fixup time
      Int32 val = MAXOF(defaultValueAsLong, 30);

      currContext->getSessionDefaults()
                 ->setEspAssignTimeWindow(val);
    }
  else if (strcmp(defaultName, "ESP_STOP_IDLE_TIMEOUT") == 0)
    {
      currContext->getSessionDefaults()
                 ->setEspStopIdleTimeout(defaultValueAsLong);
    }
  else if (strcmp(defaultName, "ESP_IDLE_TIMEOUT") == 0)
    {
      currContext->getSessionDefaults()
                 ->setEspIdleTimeout(defaultValueAsLong);
    }
  else if (strcmp(defaultName, "COMPILER_IDLE_TIMEOUT") == 0)
    {
      currContext->getSessionDefaults()
                 ->setCompilerIdleTimeout(defaultValueAsLong);
    }
  else if (strcmp(defaultName, "ESP_INACTIVE_TIMEOUT") == 0)
    {
      currContext->getSessionDefaults()
                 ->setEspInactiveTimeout(defaultValueAsLong);
    }
  else if (strcmp(defaultName, "ESP_RELEASE_WORK_TIMEOUT") == 0)
    {
      currContext->getSessionDefaults()
                 ->setEspReleaseWorkTimeout(defaultValueAsLong);
    }
 
  else if (strcmp(defaultName, "MAX_POLLING_INTERVAL") == 0)
    {
      if (defaultValueAsLong < 0 ||
	  defaultValueAsLong > 1000)
      {
	ExHandleErrors(qparent_,
		       pentry_down,
		       0,
		       getGlobals(),
		       NULL,
		       (ExeErrorCode)(-EXE_INVALID_SESSION_DEFAULT),
		       NULL,
		       defaultName);
	goto all_ok;
      }
      else
      {
        currContext->getSessionDefaults()
                   ->setMaxPollingInterval(defaultValueAsLong);
	ipcEnv->setMaxPollingInterval(defaultValueAsLong);
      }
    }
  else if (strcmp(defaultName, "PERSISTENT_OPENS") == 0)
    {
      if (defaultValueAsLong < 0 ||
	  defaultValueAsLong > 2)
      {
	ExHandleErrors(qparent_,
		       pentry_down,
		       0,
		       getGlobals(),
		       NULL,
		       (ExeErrorCode)(-EXE_INVALID_SESSION_DEFAULT),
		       NULL,
		       defaultName);
	goto all_ok;
      }
      else
      {
        currContext->getSessionDefaults()
                   ->setPersistentOpens(defaultValueAsLong);
	ipcEnv->setPersistentOpens(defaultValueAsLong > 0);
      }
    }

  else if (strcmp(defaultName, "MXCMP_PRIORITY") == 0)
    {
      currContext->getSessionDefaults()->setMxcmpPriority(defaultValueAsLong);
      changeMxcmpPriority = TRUE;
    }
  else if (strcmp(defaultName, "MXCMP_PRIORITY_DELTA") == 0)
    {
      currContext->getSessionDefaults()->setMxcmpPriorityDelta(defaultValueAsLong);
      changeMxcmpPriority = TRUE;
      mxcmpIsDelta = TRUE;
    }
  else if (strcmp(defaultName, "AQR_ENTRIES") == 0)
    {
      if (currContext->aqrInfo()->setAQREntriesFromInputStr(defaultValue,
							    strlen(defaultValue)))
	{
	  ExHandleErrors(qparent_,
			 pentry_down,
			 0,
			 getGlobals(),
			 NULL,
			 (ExeErrorCode)(-EXE_INVALID_SESSION_DEFAULT),
			 NULL,
			 defaultName);
	  goto all_ok;
	}
    }
  else if (strcmp(defaultName, "AQR_EMS_MSG") == 0)
    {
      if ((strcmp(defaultValue, "ON") == 0) ||
	  (strcmp(defaultValue, "ENABLE") == 0))
	currContext->getSessionDefaults()->setAqrEmsEvent(TRUE);
      else if ((strcmp(defaultValue, "OFF") == 0) ||
	       (strcmp(defaultValue, "DISABLE") == 0))
	currContext->getSessionDefaults()->setAqrEmsEvent(FALSE);
    }
  else if (strcmp(defaultName, "CLI_BULKMOVE") == 0)
    {
      if ((strcmp(defaultValue, "ON") == 0) ||
	  (strcmp(defaultValue, "ENABLE") == 0))
	currContext->getSessionDefaults()->setCliBulkMove(TRUE);
      else if ((strcmp(defaultValue, "OFF") == 0) ||
	       (strcmp(defaultValue, "DISABLE") == 0))
	currContext->getSessionDefaults()->setCliBulkMove(FALSE);
    }
  else if (strcmp(defaultName, "SQL_SESSION") == 0)
    {
      if (strncmp(defaultValue, "BEGIN", strlen("BEGIN")) == 0)
	{
	  if ((strlen(defaultValue) > strlen("BEGIN")) &&
	      (defaultValue[strlen("BEGIN")] == ':'))
            {
              if(strlen(&defaultValue[strlen("BEGIN")+1]) > ComSqlId::MAX_SESSION_NAME_LEN)
              {
                //Invalid default value in SET SESSION DEFAULT statement for user name
                ExHandleErrors(qparent_,
		          pentry_down,
                          0,
                          getGlobals(),
                          NULL,
                          (ExeErrorCode)(-EXE_INVALID_SESSION_DEFAULT),
                          NULL,
                          defaultName);
                goto all_ok;
              }
               
	      currContext->beginSession(&defaultValue[strlen("BEGIN")+1]);
            }
	  else
	    currContext->beginSession(NULL);

	  currContext->createMxcmpSession();
	}
      else if (strcmp(defaultValue, "CONTINUE:CLEANUP_ESPS") == 0)
        {
          Lng32 numStoppedEsps = currContext->reduceEsps();
          ComDiagsArea *da = 
            ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
          *da << DgSqlCode(EXE_CLEANUP_ESP)   // a warning.
              << DgInt0(numStoppedEsps);
          ex_queue_entry * up_entry = getParentQueue().up->getTailEntry();
          up_entry->setDiagsArea(da);
        }
      else if (strcmp(defaultValue, "END") == 0)
	{
	  currContext->endSession(FALSE /*do not cleanup ESPs*/,
				  FALSE /*not esps only*/,
				  FALSE /*do not cleanup opens*/);
	} // END
      else if (strcmp(defaultValue, "END:CLEANUP_ESPS") == 0)
	{
	  currContext->endSession(TRUE /*cleanup ESPs*/,
				  FALSE,/*not esps only*/
				  FALSE /*do not cleanup opens*/);
	} // END:CLEANUP_ESPS
      else if (strcmp(defaultValue, "END:CLEANUP_OPENS") == 0)
	{
	  currContext->endSession(FALSE /*do not cleanup ESPs*/, 
				  FALSE,/*not esps only*/
				  TRUE /* cleanup opens*/);
	} // END:CLEANUP_OPENS
      else if (strcmp(defaultValue, "END:CLEANUP_ESPS:CLEANUP_OPENS") == 0)
	{
	  currContext->endSession(TRUE /*cleanup ESPs*/,  
				  FALSE,/*not esps only*/
				  TRUE/* cleanup opens*/);
	} // END:CLEANUP_ESPS & OPENS
      else if (strcmp(defaultValue, "END:CLEANUP_ESPS_ONLY") == 0)
	{
	  currContext->endSession(TRUE /*cleanup ESPs*/, 
				  TRUE /*only esps*/, 
				  FALSE);
	} // END:CLEANUP_ESPS
      else if (strcmp(defaultValue, "SWITCH") == 0)
	{
	  currContext->endSession(FALSE /*do not cleanup ESPs*/,
				  FALSE,
				  FALSE /*do not cleanup opens*/);

	  currContext->beginSession(NULL);
	} // SWITCH
      else if (strcmp(defaultValue, "DROP") == 0)
	{
	  currContext->dropSession();
	} // DROP
    } // USER_SESSION
  else if (strcmp(defaultName, "SQL_USER") == 0)
    {
      NABoolean invalidInput = FALSE;
      NABoolean isRoot = ComUser::isRootUserID();

      // Only allow this setting if the current user is DB__ROOT
      if (isRoot)
      {
        // Make sure the input name is not too long
        UInt32 maxLen = MAX_DBUSERNAME_LEN;
        if (strlen(defaultValue) > maxLen)
          invalidInput = TRUE;
      }

      // Report an error if the current user is not DB__ROOT or if the
      // input is not valid
      if (!isRoot || invalidInput)
      {
        ExHandleErrors(qparent_,
                       pentry_down,
                       0,
                       getGlobals(),
                       NULL,
                       (ExeErrorCode)(-EXE_INVALID_SESSION_DEFAULT),
                       NULL,
                       defaultName);
        goto all_ok;
      }

      // Update the current user identity if the user name is changing      
      if (strcmp(defaultValue, currContext->getDatabaseUserName()) != 0)
      {
        currContext->setDatabaseUserByName(defaultValue);
      }
    }
  else if (strcmp(defaultName, "ROWSET_ATOMICITY") == 0)
    {
      if (NOT ((defaultValueAsLong >= 0) && (defaultValueAsLong <= 2)))
	{
	  //Invalid default value in SET SESSION DEFAULT statement for user name
	  ExHandleErrors(qparent_,
			 pentry_down,
			 0,
			 getGlobals(),
			 NULL,
			 (ExeErrorCode)(-EXE_INVALID_SESSION_DEFAULT),
			 NULL,
			 defaultName);
	  goto all_ok;
	}
      currContext->getSessionDefaults()->setRowsetAtomicity(defaultValueAsLong);
    }
  else if (strcmp(defaultName, "RTS_TIMEOUT") == 0)
    {
      currContext->getSessionDefaults()->setRtsTimeout(MAXOF (defaultValueAsLong, 2));
    }

  else if ((strcmp(defaultName, "SET_PARSERFLAGS") == 0) ||
	   (strcmp(defaultName, "RESET_PARSERFLAGS") == 0))
    {
      NABoolean isSet = FALSE;
      NABoolean validDefault = FALSE;
      if (strncmp(defaultName, "SET", strlen("SET")) == 0)
	{
	  currContext->setSqlParserFlags(defaultValueAsLong);
	  
	  currContext->getSessionDefaults()->setParserFlags(defaultValueAsLong);
	}
      else
	{
	  currContext->resetSqlParserFlags(defaultValueAsLong);
	  currContext->getSessionDefaults()->setParserFlags(-defaultValueAsLong);
	}
    }
  else if ((strncmp(defaultName, "SET_ENVVAR_", strlen("SET_ENVVAR_")) == 0) ||
	   (strncmp(defaultName, "RESET_ENVVAR_", strlen("RESET_ENVVAR_")) == 0))
    {
      NABoolean isSet = FALSE;
      char * envvarVal = NULL;
      char * envvarName = NULL;
      if (strncmp(defaultName, "SET", strlen("SET")) == 0)
	{
	  if (strlen(defaultName) > strlen("SET_ENVVAR_"))
	    {
	      isSet = TRUE;
	      envvarName = &defaultName[strlen("SET_ENVVAR_")];
	      envvarVal = defaultValue;
	    }
	}
      else if (strncmp(defaultName, "RESET", strlen("RESET")) == 0)
	{
	  if (strlen(defaultName) > strlen("RESET_ENVVAR_"))
	    {
	      isSet = FALSE;
	      envvarName = &defaultName[strlen("RESET_ENVVAR_")];
	      envvarVal = defaultValue;
	    }
	}

      if (envvarVal == NULL)
	{
	  //Unknown default value in SET SESSION DEFAULT statement. 
	  //Insert error and go to end.
	  ExHandleErrors(qparent_,
			 pentry_down,
			 0,
			 getGlobals(),
			 NULL,
			 (ExeErrorCode)(-EXE_INVALID_SESSION_DEFAULT),
			 NULL,
			 defaultName);
	  // Error details have been inserted into qparent_. Go to all_ok.
	  goto all_ok;
	}	  

      Lng32 rc;
      if (isSet)
	{
	  rc = currContext->getCliGlobals()->setEnvVar(envvarName, envvarVal, FALSE);
	}
      else
	{
	  rc = currContext->getCliGlobals()->setEnvVar(envvarName, envvarVal, TRUE);
	}

      if (rc)
	{
	  ExHandleErrors(qparent_,
			 pentry_down,
			 0,
			 getGlobals(),
			 NULL,
			 (ExeErrorCode)rc,
			 NULL,
			 NULL);
	}
    }
  else if (strcmp(defaultName, "ALTPRI_MASTER") == 0)
    {
      if ((strcmp(defaultValue, "ON") == 0) ||
	  (strcmp(defaultValue, "ENABLE") == 0))
	currContext->getSessionDefaults()->setAltpriMaster(TRUE);
      else if ((strcmp(defaultValue, "OFF") == 0) ||
	       (strcmp(defaultValue, "DISABLE") == 0))
	currContext->getSessionDefaults()->setAltpriMaster(FALSE);
    }
  else if (strcmp(defaultName, "ALTPRI_MASTER_SEQ_EXE") == 0)
    {
      if ((strcmp(defaultValue, "ON") == 0) ||
	  (strcmp(defaultValue, "ENABLE") == 0))
	currContext->getSessionDefaults()->setAltpriMasterSeqExe(TRUE);
      else if ((strcmp(defaultValue, "OFF") == 0) ||
	       (strcmp(defaultValue, "DISABLE") == 0))
	currContext->getSessionDefaults()->setAltpriMasterSeqExe(FALSE);
    }
  else if (strcmp(defaultName, "ALTPRI_FIRST_FETCH") == 0)
    {
      if ((strcmp(defaultValue, "ON") == 0) ||
	  (strcmp(defaultValue, "ENABLE") == 0))
	currContext->getSessionDefaults()->setAltpriFirstFetch(TRUE);
      else if ((strcmp(defaultValue, "OFF") == 0) ||
	       (strcmp(defaultValue, "DISABLE") == 0))
	currContext->getSessionDefaults()->setAltpriFirstFetch(FALSE);
    }
  else if (strcmp(defaultName, "ALTPRI_ESP") == 0)
    {
      if ((strcmp(defaultValue, "ON") == 0) ||
	  (strcmp(defaultValue, "ENABLE") == 0))
	currContext->getSessionDefaults()->setAltpriEsp(TRUE);
      else if ((strcmp(defaultValue, "OFF") == 0) ||
	       (strcmp(defaultValue, "DISABLE") == 0))
	currContext->getSessionDefaults()->setAltpriEsp(FALSE);
    }
  else if (strcmp(defaultName, "INTERNAL_FORMAT_IO") == 0)
    {
      if ((strcmp(defaultValue, "ON") == 0) ||
	  (strcmp(defaultValue, "ENABLE") == 0))
	currContext->getSessionDefaults()->setInternalFormatIO(TRUE);
      else if ((strcmp(defaultValue, "OFF") == 0) ||
	       (strcmp(defaultValue, "DISABLE") == 0))
	currContext->getSessionDefaults()->setInternalFormatIO(FALSE);
    }
  else if (strcmp(defaultName, "ISO_MAPPING") == 0)
    {
      currContext->getSessionDefaults()->setIsoMappingName(defaultValue,
							   strlen(defaultValue));
      currContext->getSessionDefaults()->setIsoMappingDefine();
    }
  else if (strcmp(defaultName, "IS_DB_TRANSPORTER") == 0)
    {
      if ((strcmp(defaultValue, "ON") == 0) ||
	  (strcmp(defaultValue, "ENABLE") == 0))
	currContext->getSessionDefaults()->setDbtrProcess(TRUE);
      else if ((strcmp(defaultValue, "OFF") == 0) ||
	       (strcmp(defaultValue, "DISABLE") == 0))
	currContext->getSessionDefaults()->setDbtrProcess(FALSE);
    }
  else if (strcmp(defaultName, "CANCEL_ESCALATION_SAVEABEND") == 0)
    {
      if ((strcmp(defaultValue, "ON") == 0) ||
          (strcmp(defaultValue, "on") == 0) ||
	  (strcmp(defaultValue, "ENABLE") == 0))
	currContext->getSessionDefaults()->setCancelEscalationSaveabend(TRUE);
      else if ((strcmp(defaultValue, "OFF") == 0) ||
               (strcmp(defaultValue, "off") == 0) ||
	       (strcmp(defaultValue, "DISABLE") == 0))
	currContext->getSessionDefaults()->
                setCancelEscalationSaveabend(FALSE);
    }
  else if (strcmp(defaultName, "CANCEL_QUERY_ALLOWED") == 0)
    {
      if ((strcmp(defaultValue, "ON") == 0) ||
          (strcmp(defaultValue, "on") == 0) ||
	  (strcmp(defaultValue, "ENABLE") == 0))
	currContext->getSessionDefaults()->setCancelQueryAllowed(TRUE);
      else if ((strcmp(defaultValue, "OFF") == 0) ||
               (strcmp(defaultValue, "off") == 0) ||
	       (strcmp(defaultValue, "DISABLE") == 0))
	currContext->getSessionDefaults()->
                setCancelQueryAllowed(FALSE);
    }
  else if (strcmp(defaultName, "CANCEL_UNIQUE_QUERY") == 0)
    {
      if ((strcmp(defaultValue, "ON") == 0) ||
          (strcmp(defaultValue, "on") == 0) ||
	  (strcmp(defaultValue, "ENABLE") == 0))
	currContext->getSessionDefaults()->setCancelUniqueQuery(TRUE);
      else if ((strcmp(defaultValue, "OFF") == 0) ||
               (strcmp(defaultValue, "off") == 0) ||
	       (strcmp(defaultValue, "DISABLE") == 0))
	currContext->getSessionDefaults()->
                setCancelUniqueQuery(FALSE);
    }
  else if (strcmp(defaultName, "CANCEL_LOGGING") == 0)
    {
      if ((strcmp(defaultValue, "ON") == 0) ||
          (strcmp(defaultValue, "on") == 0) ||
	  (strcmp(defaultValue, "ENABLE") == 0))
	currContext->getSessionDefaults()->setCancelLogging(TRUE);
      else if ((strcmp(defaultValue, "OFF") == 0) ||
               (strcmp(defaultValue, "off") == 0) ||
	       (strcmp(defaultValue, "DISABLE") == 0))
	currContext->getSessionDefaults()->
                setCancelLogging(FALSE);
    }
  else if (strcmp(defaultName, "SUSPEND_LOGGING") == 0)
    {
      if ((strcmp(defaultValue, "ON") == 0) ||
          (strcmp(defaultValue, "on") == 0) ||
	  (strcmp(defaultValue, "ENABLE") == 0))
	currContext->getSessionDefaults()->setSuspendLogging(TRUE);
      else if ((strcmp(defaultValue, "OFF") == 0) ||
               (strcmp(defaultValue, "off") == 0) ||
	       (strcmp(defaultValue, "DISABLE") == 0))
	currContext->getSessionDefaults()->
                setSuspendLogging(FALSE);
    }
  else if (strcmp(defaultName, "CANCEL_ESCALATION_INTERVAL") == 0)
    {
      currContext->getSessionDefaults()->
        setCancelEscalationInterval(defaultValueAsLong);
    }  
  else if (strcmp(defaultName, "CANCEL_ESCALATION_MXOSRVR_INTERVAL") == 0)
    {
      currContext->getSessionDefaults()->
        setCancelEscalationMxosrvrInterval(defaultValueAsLong);
    }  
  else if (strcmp(defaultName, "CALL_EMBEDDED_ARKCMP") == 0)
    {
      if ((stricmp(defaultValue, "TRUE") == 0) ||
	  (stricmp(defaultValue, "ENABLE") == 0))
	currContext->getSessionDefaults()->setCallEmbeddedArkcmp(TRUE);
      else if ((stricmp(defaultValue, "FALSE") == 0) ||
	       (stricmp(defaultValue, "DISABLE") == 0))
	currContext->getSessionDefaults()->setCallEmbeddedArkcmp(FALSE);
    }
  else if (strcmp(defaultName, "PARENT_QID") == 0)
    {
      currContext->getSessionDefaults()->setParentQid(defaultValue, (Lng32)strlen(defaultValue));
    }
  else if (strcmp(defaultName, "PARENT_QID_SYSTEM") == 0)
    {
      currContext->getSessionDefaults()->setParentQidSystem(defaultValue, (Lng32)strlen(defaultValue));
    }
  else if (strcmp(defaultName, "STATISTICS_VIEW_TYPE") == 0)
  {
    if (stricmp(defaultValue, "PERTABLE") == 0)
      currContext->getSessionDefaults()->setStatisticsViewType(SQLCLI_PERTABLE_STATS);
    else if (stricmp(defaultValue, "ACCUMULATED") == 0)
      currContext->getSessionDefaults()->setStatisticsViewType(SQLCLI_ACCUMULATED_STATS);
    else if (stricmp(defaultValue, "PROGRESS") == 0)
      currContext->getSessionDefaults()->setStatisticsViewType(SQLCLI_PROGRESS_STATS);
    else if (stricmp(defaultValue, "DEFAULT") == 0)
      currContext->getSessionDefaults()->setStatisticsViewType(SQLCLI_SAME_STATS);
    else
    {
      // Not a valid value for STATISTICS_VIEW_TYPE. Insert error and go to end.
      ExHandleErrors(qparent_,
		      pentry_down,
		      0,
		      getGlobals(),
		      NULL,
		      (ExeErrorCode)(-EXE_INVALID_SESSION_DEFAULT),
		      NULL,
		      defaultName);
      goto all_ok;
    }
  }
  else if (strcmp(defaultName, "RECLAIM_MEMORY_AFTER") == 0)
  {
    currContext->getSessionDefaults()->setReclaimTotalMemorySize(defaultValueAsLong);
  }
  else if (strcmp(defaultName, "RECLAIM_FREE_MEMORY_RATIO") == 0)
  {
    currContext->getSessionDefaults()->setReclaimFreeMemoryRatio(defaultValueAsLong);
  }
  else if (strcmp(defaultName, "RECLAIM_FREE_PFS_RATIO") == 0)
  {
    currContext->getSessionDefaults()->setReclaimFreePFSRatio(defaultValueAsLong);
  }
  else if (strcmp(defaultName, "EXSM_TRACE_FILE_PREFIX") == 0)
    {
      currContext->getSessionDefaults()->
        setExSMTraceFilePrefix(defaultValue, strlen(defaultValue));
    }
  else if (strcmp(defaultName, "EXSM_TRACE_LEVEL") == 0)
    {
      int lvl = (int) strtoul(defaultValue, NULL, 16);
      currContext->getSessionDefaults()->setExSMTraceLevel(lvl);
    }

  if (changeMasterPriority)
    {
      ComRtSetProcessPriority(defaultValueAsLong, masterIsDelta);
      
      IpcPriority p;
      ComRtGetProcessPriority(p);
      currContext->getCliGlobals()->setMyPriority(p);
    }

  if (changeMxcmpPriority)
    {
      // mxcmp priority changed, kill arkcmps, if they are runing.
      for (short i = 0; i < currContext->getNumArkcmps(); i++)
	{
	  ExSqlComp::ReturnStatus retStatus = 
	    currContext->getArkcmp(i)->changePriority
	    ((mxcmpIsDelta ? 
	      currContext->getSessionDefaults()->getMxcmpPriorityDelta() : 
	      currContext->getSessionDefaults()->getMxcmpPriority()), 
	     mxcmpIsDelta);
	  if (retStatus != ExSqlComp::SUCCESS)
	    {
	      ExHandleErrors(qparent_,
			     pentry_down,
			     0,
			     getGlobals(),
			     NULL,
			     (ExeErrorCode)(-15371),
			     NULL,
			     "MXCMP");
	    }
	}
    }

  if (changeEspPriority)
    {
      IpcPriority p = 
	(espIsDelta ? 
	 currContext->getSessionDefaults()->getEspPriorityDelta() : 
	 currContext->getSessionDefaults()->getEspPriority());

      // adjust this, if altpri is to be done in esp.
      if (currContext->getSessionDefaults()->getAltpriEsp())
	{
	  // in this case, the 'idle' esp priority is its fixup priority.
	  p += currContext->getSessionDefaults()->getEspFixupPriorityDelta();
	}

      short rc = currContext->getCliGlobals()->getEspManager()->
        changePriorities(p, espIsDelta, true); // ignore idle timed out esps
      if (rc)
	{
	  char errMsg[100];
	  strcpy(errMsg, "MXESP. Error code: ");
	  str_itoa(ABS(rc), &errMsg[strlen(errMsg)]);
          up_entry = qparent_.up->getTailEntry();

	  ComDiagsArea *  da = 
	    ExRaiseSqlError(getGlobals()->getDefaultHeap(),
			    up_entry,
			    (ExeErrorCode)(15371),
			    NULL,
			    errMsg);
	  up_entry->setDiagsArea(da);
	}
    }


all_ok:
  // all ok. Return EOF.
  up_entry = qparent_.up->getTailEntry();
  up_entry->upState.parentIndex = 
    pentry_down->downState.parentIndex;
  
  up_entry->upState.setMatchNo(0);
  up_entry->upState.status = ex_queue::Q_NO_DATA;
  
  // insert into parent
  getParentQueue().up->insert();
  
  getParentQueue().down->removeHead();
  
  return WORK_OK;
}


///////////////////////////////////////////////////////////////////////
// class ExShowEnvvarsTcb
///////////////////////////////////////////////////////////////////////
ExShowEnvvarsTcb::ExShowEnvvarsTcb(const ExDescribeTdb & describe_tdb,
				   ex_globals * glob)
     : ExDescribeTcb(describe_tdb, glob),
       step_(EMPTY_),
       currEnvvar_(0)
{
}

short ExShowEnvvarsTcb::moveRowToUpQueue(Lng32 tuppIndex,
					 const char * row, Lng32 len, 
					 short * rc)
{
  if (qparent_.up->isFull())
    {
      if (rc)
	*rc = WORK_OK;
      return -1;
    }

  Lng32 length;
  if (len <= 0)
    length = strlen(row);
  else
    length = len;

  tupp p;
  if (pool_->get_free_tuple(p, (Lng32)(SQL_VARCHAR_HDR_SIZE+ length)))
    {
      if (rc)
	*rc = WORK_POOL_BLOCKED;
      return -1;
    }
  
  char * dp = p.getDataPointer();
  *(short*)dp = (short)length;
  str_cpy_all(&dp[SQL_VARCHAR_HDR_SIZE], row, length);

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ex_queue_entry * up_entry = qparent_.up->getTailEntry();
  
  up_entry->copyAtp(pentry_down);
  up_entry->getAtp()->getTupp(tuppIndex) = p;

  up_entry->upState.parentIndex = 
    pentry_down->downState.parentIndex;
  
  up_entry->upState.setMatchNo(0);
  up_entry->upState.status = ex_queue::Q_OK_MMORE;

  // insert into parent
  qparent_.up->insert();

  return 0;
}

short ExShowEnvvarsTcb::work()
{
  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExDDLPrivateState & pstate =
    *((ExDDLPrivateState*) pentry_down->pstate);

  ContextCli *currContext =
      getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals()->
        getStatement()->getContext();

  NAList<SessionEnvvar> * sessionEnvvars = 
    currContext->getSessionDefaults()->sessionEnvvars();

  while (1)
    {
      switch (step_)
	{
	case EMPTY_:
	  {
	    currEnvvar_ = 0;

	    step_ = RETURNING_VALUE_;
	  }
	break;

	case RETURN_HEADER_:
	  {
	    // if no room in up queue for 2 display rows, 
	    // won't be able to return data/status.
	    // Come back later.
	    if ((qparent_.up->getSize() - qparent_.up->getLength()) < 3)
	      return -1;
	    
	    moveRowToUpQueue(showTdb().tuppIndex(), "  ");
	    moveRowToUpQueue(showTdb().tuppIndex(), 
			     "Current SESSION DEFAULTs");
	    
	    step_ = RETURNING_VALUE_;
	  }
	break;

	case RETURNING_VALUE_:
	  {
	    // if no room in up queue, won't be able to return the envvar.
	    // Come back later.
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    if (currEnvvar_ == sessionEnvvars->entries())
	      {
		step_ = DONE_;
		break;
	      }
	      
	    SessionEnvvar &se = (*sessionEnvvars)[currEnvvar_];

	    char * formattedStr = new(getHeap()) 
	      char[showTdb().outputRowlen()];
	    
	    str_sprintf(formattedStr, "%s\t%s", se.envvarName(),
			se.envvarValue());
	    
	    moveRowToUpQueue(showTdb().tuppIndex(), formattedStr);
	    
	    NADELETEBASIC(formattedStr, getHeap());

	    currEnvvar_++;
	  }
	break;

	case DONE_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    // all ok. Return EOF.
	    ex_queue_entry * up_entry = qparent_.up->getTailEntry();
	    
	    up_entry->upState.parentIndex = 
	      pentry_down->downState.parentIndex;
	    
	    up_entry->upState.setMatchNo(0);
	    up_entry->upState.status = ex_queue::Q_NO_DATA;
	    
	    // insert into parent
	    qparent_.up->insert();
	    
	    step_ = EMPTY_;
	    
	    qparent_.down->removeHead();
	    
	    return WORK_OK;
	  }
	  break;

	} // switch
    }

  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for *_private_state
///////////////////////////////////////////////////////////////////////////////

ExControlPrivateState::ExControlPrivateState(const ExControlTcb * /*tcb*/)
{
  step_ = ExControlTcb::EMPTY_;
}

ExControlPrivateState::~ExControlPrivateState()
{
}

ex_tcb_private_state * ExControlPrivateState::allocate_new(const ex_tcb *tcb)
{
  return new(((ex_tcb *)tcb)->getSpace()) ExControlPrivateState((ExControlTcb *) tcb);
};


// -----------------------------------------------------------------------
// Sorry, for now just compile the ControlArea together with ex_control.cpp
// -----------------------------------------------------------------------

#include "ExControlArea.cpp"


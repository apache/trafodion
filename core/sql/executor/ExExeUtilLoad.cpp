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
 * File:         ExExeUtilLoad.cpp
 * Description:  
 *               
 *               
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include <iostream>
using std::cerr;
using std::endl;

#include <fstream>
using std::ofstream;

#include <stdio.h>

#include "ComCextdecs.h"
#include  "cli_stdh.h"
#include  "ex_stdh.h"
#include  "sql_id.h"
#include  "ex_transaction.h"
#include  "ComTdb.h"
#include  "ex_tcb.h"
#include  "ComSqlId.h"

#include  "ExExeUtil.h"
#include  "ex_exe_stmt_globals.h"
#include  "exp_expr.h"
#include  "exp_clause_derived.h"
#include  "ComRtUtils.h"
#include  "ExStats.h"
#include  "ExpLOB.h"
#include "ExpLOBenums.h"
#include  "ExpLOBinterface.h"
#include  "ExpLOBexternal.h"
#include  "str.h"
#include "ExpHbaseInterface.h"
#include "ExHbaseAccess.h"
#include "ExpErrorEnums.h"
#include "ExpLOBaccess.h"
#include "HdfsClient_JNI.h"

///////////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilCreateTableAsTdb::build(ex_globals * glob)
{
  ExExeUtilCreateTableAsTcb * exe_util_tcb;

  exe_util_tcb = new(glob->getSpace()) ExExeUtilCreateTableAsTcb(*this, glob);

  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilCreateTableAsTcb
///////////////////////////////////////////////////////////////
ExExeUtilCreateTableAsTcb::ExExeUtilCreateTableAsTcb(
     const ComTdbExeUtil & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilTcb( exe_util_tdb, NULL, glob),
       step_(INITIAL_),
       tableExists_(FALSE)
{
  // Allocate the private state in each entry of the down queue
  qparent_.down->allocatePstate(this);
}


//////////////////////////////////////////////////////
// work() for ExExeUtilCreateTableAsTcb
//////////////////////////////////////////////////////
short ExExeUtilCreateTableAsTcb::work()
{
  Lng32 cliRC = 0;
  short retcode = 0;
  Int64 rowsAffected = 0;
  NABoolean redriveCTAS = FALSE;
  
  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;
  
  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;
  
  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate =
    *((ExExeUtilPrivateState*) pentry_down->pstate);

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ContextCli *currContext = masterGlob->getStatement()->getContext();
  ExTransaction *ta = currContext->getTransaction();
  
  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
	    NABoolean xnAlreadyStarted = ta->xnInProgress();

	    // allow a user transaction if NO LOAD was specified
	    if (xnAlreadyStarted && !ctaTdb().noLoad())
              {
                ExRaiseSqlError(getHeap(), &diagsArea_, -20123, NULL, NULL, NULL,
                                "This DDL operation");
                step_ = ERROR_;
                break;
              }

	    doSidetreeInsert_ = TRUE;
	    if (xnAlreadyStarted)
                doSidetreeInsert_ = FALSE;
	    else if ( ctaTdb().siQuery_ == (NABasicPtr) NULL )
	      doSidetreeInsert_ = FALSE;

	    tableExists_ = FALSE;

	    if (ctaTdb().ctQuery_)
	      step_ = CREATE_;
	    else
	      step_ = ALTER_TO_NOAUDIT_;
	  }
	break;

	case CREATE_:
	  {
	    tableExists_ = FALSE;
	    // issue the create table command 
	    cliRC = cliInterface()->executeImmediate(
		 ctaTdb().ctQuery_);
	    if (cliRC < 0)
	      {
		if (((cliRC == -1055) ||  // SQ table err msg
		     (cliRC == -1390) ||  // Traf err msg
		     (cliRC == -1387)) && // Hive err msg
		    (ctaTdb().loadIfExists()))
		  {
		    SQL_EXEC_ClearDiagnostics(NULL);
		    tableExists_ = TRUE;

		    if (ctaTdb().deleteData())
		      step_ = TRUNCATE_TABLE_;
		    else
		      step_ = ALTER_TO_NOAUDIT_;
		    break;
		  }
		else
		  {
		    cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		    step_ = ERROR_;
		    break;
		  }
	      }

	    // a transaction may not have existed prior to the create stmt,
	    // but if autocommit was off, a transaction would now exist.
	    // turn off sidetree insert.
	    if (ta->xnInProgress())
	      doSidetreeInsert_ = FALSE;

	    if (ctaTdb().noLoad())
	      step_ = DONE_;
	    else
	      step_ = ALTER_TO_NOAUDIT_;
	  }
	break;

	case TRUNCATE_TABLE_:
	case TRUNCATE_TABLE_AND_ERROR_:
	  {
	    char * ddQuery = 
	      new(getMyHeap()) char[strlen("TRUNCATE TABLE; ") + 
				   strlen(ctaTdb().getTableName()) +
				   100];
	    strcpy(ddQuery, "TRUNCATE TABLE ");
	    strcat(ddQuery, ctaTdb().getTableName());
	    strcat(ddQuery, ";");
	    cliRC = cliInterface()->executeImmediate(ddQuery, NULL,NULL,TRUE,NULL,TRUE);
	    // Delete new'd characters
	    NADELETEBASIC(ddQuery, getHeap());
	    ddQuery = NULL;

	    if (cliRC < 0)
	      {
		if (step_ == TRUNCATE_TABLE_)
		  {
		    step_ = ERROR_;
		    break;
		  }

		// delete data returned an error.
		// As a last resort, drop this table.
		SQL_EXEC_ClearDiagnostics(NULL);
		step_ = DROP_AND_ERROR_;
		break;
	      }

	    if (step_ == TRUNCATE_TABLE_AND_ERROR_)
	      {

		if (doSidetreeInsert_)
		  {
		    cliRC = changeAuditAttribute(ctaTdb().getTableName(),TRUE);
		  }
		
		step_ = ERROR_;
	      }
	    else
	      step_ = ALTER_TO_NOAUDIT_;
	  }
	break;

	case ALTER_TO_NOAUDIT_:
	  {
	    if (NOT doSidetreeInsert_)
	      {
		step_ = INSERT_VSBB_;
		break;
	      }

	    step_ = INSERT_SIDETREE_;
	  }
	break;

	case INSERT_SIDETREE_:
	  {
            ex_queue_entry * up_entry = qparent_.up->getTailEntry();
            ComDiagsArea *diagsArea = up_entry->getDiagsArea();
	    // issue the insert command
	    cliInterface()->clearGlobalDiags();

	    // All internal queries issued from CliInterface assume that
	    // they are in ISO_MAPPING.
	    // That causes mxcmp to use the default charset as iso88591
	    // for unprefixed literals.
	    // The insert...select being issued out here contains the user
	    // specified query and any literals in that should be using
	    // the default_charset.
	    // So we send the isoMapping charset instead of the
	    // enum ISO_MAPPING.
	    Int32 savedIsoMapping = 
	      currContext->getSessionDefaults()->getIsoMappingEnum();
	    cliInterface()->setIsoMapping
	      (currContext->getSessionDefaults()->getIsoMappingEnum());
            redriveCTAS = currContext->getSessionDefaults()->getRedriveCTAS();
            if (redriveCTAS)
            {
              if (childQueryId_ == NULL)
                childQueryId_ = new (getHeap()) char[ComSqlId::MAX_QUERY_ID_LEN+1];
              childQueryIdLen_ = ComSqlId::MAX_QUERY_ID_LEN;
              cliRC = cliInterface()->executeImmediatePrepare2(
		 ctaTdb().siQuery_,
                 childQueryId_,
                 &childQueryIdLen_,
                 &childQueryCostInfo_,
                 &childQueryCompStatsInfo_,
                 NULL, NULL,
                 &rowsAffected,TRUE);
              if (cliRC >= 0)
              {
                childQueryId_[childQueryIdLen_] = '\0';
                Statement *ctasStmt = masterGlob->getStatement();
                cliRC = ctasStmt->setChildQueryInfo(diagsArea,
                  childQueryId_, childQueryIdLen_,
                  &childQueryCostInfo_, &childQueryCompStatsInfo_);
                if (cliRC < 0)
                {
                  step_ = HANDLE_ERROR_;
                  break;
                }
                if (outputBuf_ == NULL)
                  outputBuf_ = new (getHeap()) char[ComSqlId::MAX_QUERY_ID_LEN+100]; // 
                str_sprintf(outputBuf_, "childQidBegin: %s ", childQueryId_);
                moveRowToUpQueue(outputBuf_);
                step_ = INSERT_SIDETREE_EXECUTE_; 
                return WORK_RESCHEDULE_AND_RETURN;
              }
            }
            else
            {
              cliRC = cliInterface()->executeImmediatePrepare(
		 ctaTdb().siQuery_,
		 NULL, NULL,
		 &rowsAffected,TRUE);
            }
	    cliInterface()->setIsoMapping(savedIsoMapping);
	    if (cliRC < 0)
	    {
              // sidetree insert prepare failed. 
              // Try vsbb insert
	      step_ = ALTER_TO_AUDIT_AND_INSERT_VSBB_;
	      break;
	    }
            step_ = INSERT_SIDETREE_EXECUTE_;
          }
          break;
        case INSERT_SIDETREE_EXECUTE_:
          {
	    cliRC = cliInterface()->executeImmediateExec(
		 ctaTdb().siQuery_,
		 NULL, NULL, TRUE,
		 &rowsAffected);
	    if (cliRC < 0)
            {
              cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
              step_ = HANDLE_ERROR_;
            }
            else
            {
	      masterGlob->setRowsAffected(rowsAffected);
	      step_ = ALTER_TO_AUDIT_;
            }
            redriveCTAS = currContext->getSessionDefaults()->getRedriveCTAS();
            if (redriveCTAS)
            {
              str_sprintf(outputBuf_, "childQidEnd: %s ", childQueryId_);
              moveRowToUpQueue(outputBuf_);
              return WORK_RESCHEDULE_AND_RETURN;
            }
	  }
	  break;
	case ALTER_TO_AUDIT_:
	case ALTER_TO_AUDIT_AND_INSERT_VSBB_:
	  {
	    if (step_ == ALTER_TO_AUDIT_AND_INSERT_VSBB_)
	      step_ = INSERT_VSBB_;
	    else
	      step_ = UPD_STATS_;
	  }
	  break;

	case INSERT_VSBB_:
	  {
            ex_queue_entry * up_entry = qparent_.up->getTailEntry();
            ComDiagsArea *diagsArea = up_entry->getDiagsArea();

	    // issue the insert command
	    Int64 rowsAffected = 0;
	    Int32 savedIsoMapping = 
	      currContext->getSessionDefaults()->getIsoMappingEnum();
	    cliInterface()->setIsoMapping
	      (currContext->getSessionDefaults()->getIsoMappingEnum());
            NABoolean redriveCTAS = currContext->getSessionDefaults()->getRedriveCTAS();
            if (redriveCTAS)
            {
              if (childQueryId_ == NULL)
                childQueryId_ = new (getHeap()) char[ComSqlId::MAX_QUERY_ID_LEN+1];
              cliRC = cliInterface()->executeImmediatePrepare2(
		 ctaTdb().viQuery_,
                 childQueryId_,
                 &childQueryIdLen_,
                 &childQueryCostInfo_,
                 &childQueryCompStatsInfo_,
                 NULL, NULL,
                 &rowsAffected,TRUE);
              cliInterface()->setIsoMapping(savedIsoMapping);
	      if (cliRC >= 0)
              {
                childQueryId_[childQueryIdLen_] = '\0';
                Statement *ctasStmt = masterGlob->getStatement();
                cliRC = ctasStmt->setChildQueryInfo(diagsArea,
                  childQueryId_, childQueryIdLen_,
                  &childQueryCostInfo_, &childQueryCompStatsInfo_);
                if (cliRC < 0)
                {
                  step_ = HANDLE_ERROR_;
                  break;
                }
                if (outputBuf_ == NULL)
                  outputBuf_ = new (getHeap()) char[ComSqlId::MAX_QUERY_ID_LEN+100]; // 
                str_sprintf(outputBuf_, "childQidBegin: %s ", childQueryId_);
                moveRowToUpQueue(outputBuf_);
                step_ = INSERT_VSBB_EXECUTE_; 
                return WORK_RESCHEDULE_AND_RETURN;
              }
              else
              {
                step_ = HANDLE_ERROR_;
                break;
              }
            }
            else
            {
	      cliRC = cliInterface()->executeImmediate(
		   ctaTdb().viQuery_,
		   NULL, NULL, TRUE,
		   &rowsAffected,TRUE);
              cliInterface()->setIsoMapping(savedIsoMapping);
	      if (cliRC < 0)
              {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
                step_ = HANDLE_ERROR_;
              }
              else
              {
                masterGlob->setRowsAffected(rowsAffected);
                step_ = UPD_STATS_;
              }
            }
          }       
	  break;
        case INSERT_VSBB_EXECUTE_:
          {
	    cliRC = cliInterface()->executeImmediateExec(
		 ctaTdb().viQuery_,
		 NULL, NULL, TRUE,
		 &rowsAffected);
	    if (cliRC < 0)
	    {
              cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
              step_ = HANDLE_ERROR_;
            }
            else
            {
              str_sprintf(outputBuf_, "childQidEnd: %s ", childQueryId_);
              moveRowToUpQueue(outputBuf_);
              masterGlob->setRowsAffected(rowsAffected);
	      step_ = UPD_STATS_;
              return WORK_RESCHEDULE_AND_RETURN;
            }
	  }
	  break;
	case UPD_STATS_:
	  {
	    if ((ctaTdb().threshold_ == -1) ||
		((ctaTdb().threshold_ > 0) &&
		 (masterGlob->getRowsAffected() < ctaTdb().threshold_)))
	      {
		step_ = DONE_;
		break;
	      }

	    // issue the upd stats command 
	    char * usQuery =
	      new(getHeap()) char[strlen(ctaTdb().usQuery_) + 10 + 1];

	    str_sprintf(usQuery, ctaTdb().usQuery_,
			masterGlob->getRowsAffected());

	    cliRC = cliInterface()->executeImmediate(usQuery,NULL,NULL,TRUE,NULL,TRUE);
	    NADELETEBASIC(usQuery, getHeap());
	    if (cliRC < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		step_ = HANDLE_ERROR_;
		break;
	      }
	    
	    step_ = DONE_;
	  }
	break;

	case HANDLE_ERROR_:
	  {
	    if ((ctaTdb().ctQuery_) &&
		(ctaTdb().loadIfExists()))
	      {
		// error case and 'load if exists' specified.
		// Do not drop the table, only delete data from it.
		step_ = TRUNCATE_TABLE_AND_ERROR_;
	      }
	    else
	      step_ = DROP_AND_ERROR_;
	  }
	break;

	case DROP_AND_ERROR_:
	  {
	    if ((ctaTdb().ctQuery_) &&
		(NOT tableExists_))
	      {
		// this is an error case, drop the table
		char * dtQuery = 
		  new(getMyHeap()) char[strlen("DROP TABLE CASCADE; ") + 
				       strlen(ctaTdb().getTableName()) +
				       100];
		strcpy(dtQuery, "DROP TABLE ");
		strcat(dtQuery, ctaTdb().getTableName());
		strcat(dtQuery, " CASCADE;");
		cliRC = cliInterface()->executeImmediate(dtQuery);

		// Delete new'd characters
		NADELETEBASIC(dtQuery, getHeap());
		dtQuery = NULL;
	      }
	    else if (doSidetreeInsert_)
	      {
		cliRC = changeAuditAttribute(ctaTdb().getTableName(), 
					     TRUE, ctaTdb().isVolatile());
	      }

	    if (step_ == DROP_AND_ERROR_)
	      step_ = ERROR_;
	    else
	      step_ = DONE_;
	  }
	break;

	case DONE_:
	  {
	    retcode = handleDone();
	    if (retcode == 1)
	       return WORK_OK;
	    step_ = INITIAL_;
	    return WORK_OK;
	  }
	break;

	case ERROR_:
	  {
	    retcode = handleError();
	    if (retcode == 1)
	       return WORK_OK;
	    step_ = DONE_;
	  }
	break;
	} // switch
    } // while
  
  return WORK_OK;

}

////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ExExeUtilCreateTableAsTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExExeUtilCreateTableAsPrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExExeUtilCreateTableAsPrivateState::ExExeUtilCreateTableAsPrivateState()
{
}

ExExeUtilCreateTableAsPrivateState::~ExExeUtilCreateTableAsPrivateState()
{
};

#define SETSTEP(s) setStep(s, __LINE__)

static THREAD_P bool sv_checked_yet = false;
static THREAD_P bool sv_logStep = false;

////////////////////////////////////////////////////////////////
// Methods for classes ExExeUtilAqrWnrInsertTdb and 
// ExExeUtilAqrWnrInsertTcb
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilAqrWnrInsertTdb::build(ex_globals * glob)
{
  // build the child first
  ex_tcb * childTcb = child_->build(glob);

  ExExeUtilAqrWnrInsertTcb *exe_util_tcb;

  exe_util_tcb = 
      new(glob->getSpace()) ExExeUtilAqrWnrInsertTcb(*this, childTcb, glob);
  
  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}


ExExeUtilAqrWnrInsertTcb::ExExeUtilAqrWnrInsertTcb(
     const ComTdbExeUtilAqrWnrInsert & exe_util_tdb,
     const ex_tcb * child_tcb, 
     ex_globals * glob)
  : ExExeUtilTcb( exe_util_tdb, child_tcb, glob)
  , step_(INITIAL_)
  , targetWasEmpty_(false)
{
}

ExExeUtilAqrWnrInsertTcb::~ExExeUtilAqrWnrInsertTcb()
{
  // mjh - tbd :
  // is base class dtor called?
}

Int32 ExExeUtilAqrWnrInsertTcb::fixup()
{
  return ex_tcb::fixup();
}


void ExExeUtilAqrWnrInsertTcb::setStep(Step newStep, int lineNum)
{
  static bool sv_checked_yet = false;
  static bool sv_logStep = false;
  
  step_ = newStep;

  if (!sv_checked_yet)
  {
    sv_checked_yet = true;
  	char *logST = getenv("LOG_AQR_WNR_INSERT");
	  if (logST && *logST == '1')
	    sv_logStep = true;
	}
  if (!sv_logStep)
    return;

  if (NULL ==
      getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals())
    return;

  char *stepStr = (char *) "UNKNOWN";
  switch (step_)
  {
    case INITIAL_:
      stepStr = (char *) "INITIAL_";
      break;
    case LOCK_TARGET_:
      stepStr = (char *) "LOCK_TARGET_";
      break;
    case IS_TARGET_EMPTY_:
      stepStr = (char *) "IS_TARGET_EMPTY_";
      break;
    case SEND_REQ_TO_CHILD_:
      stepStr = (char *) "SEND_REQ_TO_CHILD_";
      break;
    case GET_REPLY_FROM_CHILD_:
      stepStr = (char *) "GET_REPLY_FROM_CHILD_";
      break;
    case CLEANUP_CHILD_:
      stepStr = (char *) "CLEANUP_CHILD_";
      break;
    case CLEANUP_TARGET_:
      stepStr = (char *) "CLEANUP_TARGET_";
      break;
    case ERROR_:
      stepStr = (char *) "ERROR_";
      break;
    case DONE_:
      stepStr = (char *) "DONE_";
      break;
  }

  cout << stepStr << ", line " << lineNum << endl;
}

// Temporary.
#define VERIFY_CLI_UTIL 1

ExWorkProcRetcode ExExeUtilAqrWnrInsertTcb::work()
{
  ExWorkProcRetcode rc = WORK_OK;
  Lng32 cliRC = 0;

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ContextCli *currContext = masterGlob->getCliGlobals()->currContext();

  while (! (qparent_.down->isEmpty()  || qparent_.up->isFull()) )
  {
    ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
    ex_queue_entry * pentry_up = qparent_.up->getTailEntry();

    switch (step_)
    {
      case INITIAL_:
      {
        targetWasEmpty_ = false;
        masterGlob->resetAqrWnrInsertCleanedup();

        if (getDiagsArea())
          getDiagsArea()->clear();

        if (ulTdb().doLockTarget())
          SETSTEP(LOCK_TARGET_);
        else
          SETSTEP(IS_TARGET_EMPTY_);

        break;
      }

      case LOCK_TARGET_:
      {
        SQL_EXEC_ClearDiagnostics(NULL);

        query_ = new(getGlobals()->getDefaultHeap()) char[1000];
        str_sprintf(query_, "lock table %s in share mode;",
                    ulTdb().getTableName());
        Lng32 len = 0;
        char dummyArg[128];
  #ifdef VERIFY_CLI_UTIL
        for (size_t i = 0; i < sizeof(dummyArg); i++)
          dummyArg[i] = i;
  #endif
        cliRC = cliInterface()->executeImmediate(
                                    query_, dummyArg, &len, FALSE);

  #ifdef VERIFY_CLI_UTIL
        ex_assert (len == 0, "lock table returned data");
        for (size_t i = 0; i < sizeof(dummyArg); i++)
          ex_assert( dummyArg[i] == i, "lock table returned data");
  #endif

        NADELETEBASIC(query_, getMyHeap());

        if (cliRC < 0)
        {
          cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
          SETSTEP(ERROR_);
        }
        else
          SETSTEP(IS_TARGET_EMPTY_);

        // Allow the query to be canceled.
        return WORK_CALL_AGAIN;
      }

      case IS_TARGET_EMPTY_:
      {
        SQL_EXEC_ClearDiagnostics(NULL);

        query_ = new(getGlobals()->getDefaultHeap()) char[1000];
        str_sprintf(query_, "select row count from %s;",
                    ulTdb().getTableName());
        Lng32 len = 0;
        Int64 rowCount = 0;
        cliRC = cliInterface()->executeImmediate(query_,
                                  (char*)&rowCount,
                                  &len, FALSE);
        NADELETEBASIC(query_, getMyHeap());
        if (cliRC < 0)
        {
          cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
          SETSTEP(ERROR_);
        }
        else
        {
          targetWasEmpty_ = (rowCount == 0);
          SETSTEP(SEND_REQ_TO_CHILD_);
        }
	      
        // Allow the query to be canceled.
        return WORK_CALL_AGAIN;
        }

      case SEND_REQ_TO_CHILD_:
      {
        if (qchild_.down->isFull())
          return WORK_OK;

        ex_queue_entry * centry = qchild_.down->getTailEntry();

        centry->downState.request = ex_queue::GET_ALL;
        centry->downState.requestValue = 
                pentry_down->downState.requestValue;
        centry->downState.parentIndex = qparent_.down->getHeadIndex();

        // set the child's input atp
        centry->passAtp(pentry_down->getAtp());

        qchild_.down->insert();

        SETSTEP(GET_REPLY_FROM_CHILD_);
      }
      break;

      case GET_REPLY_FROM_CHILD_:
      {
        // if nothing returned from child. Get outta here.
        if (qchild_.up->isEmpty())
          return WORK_OK;

        ex_queue_entry * centry = qchild_.up->getHeadEntry();

        // DA from child, if any, is copied to parent up entry.
        pentry_up->copyAtp(centry);

        switch(centry->upState.status)
        {
          case ex_queue::Q_NO_DATA:
          {
            SETSTEP(DONE_);
            break;
          }
          case ex_queue::Q_SQLERROR:
          {
            SETSTEP(CLEANUP_CHILD_);
            break;
          }
          default:
          {
            ex_assert(0, "Invalid child_status");
            break;
          }
        }
        qchild_.up->removeHead();
        break;
      }
  
      case CLEANUP_CHILD_:
      {
        bool lookingForQnoData = true;
        do 
        {
          if (qchild_.up->isEmpty())
            return WORK_OK;
          ex_queue_entry * centry = qchild_.up->getHeadEntry();
          if (centry->upState.status == ex_queue::Q_NO_DATA)
          {
            lookingForQnoData = false;
            bool cleanupTarget = false;
            if (targetWasEmpty_ &&
                (pentry_down->downState.request != ex_queue::GET_NOMORE))
            {
              // Find out if any messages were sent to insert TSE sessions,
              // because we'd like to skip CLEANUP_TARGET_ if not.
              const ExStatisticsArea *constStatsArea = NULL;
              Lng32 cliRc = SQL_EXEC_GetStatisticsArea_Internal(
                     SQLCLI_STATS_REQ_QID,
                     masterGlob->getStatement()->getUniqueStmtId(),
                     masterGlob->getStatement()->getUniqueStmtIdLen(),
                     -1, SQLCLI_SAME_STATS,
                     constStatsArea);
              ExStatisticsArea * statsArea = 
                    (ExStatisticsArea *) constStatsArea;

              if (cliRc < 0 || !statsArea)
              {
                // Error or some problem getting stats.
                cleanupTarget = true;
              }
              else if (!statsArea->getMasterStats() ||
                    statsArea->getMasterStats()->getStatsErrorCode() != 0)
              {
                // Partial success getting stats. Can't trust results.
                cleanupTarget = true;
              }
              else if (statsArea->anyHaveSentMsgIUD())  
              {
                // Stats shows that IUD started. Must cleanup.
                cleanupTarget = true;
              }
            }

            if (cleanupTarget)
              SETSTEP(CLEANUP_TARGET_);
            else
              SETSTEP(ERROR_);
          }
          qchild_.up->removeHead();
        } while (lookingForQnoData);
        break;
      }

      case CLEANUP_TARGET_:
      {
        SQL_EXEC_ClearDiagnostics(NULL);

        query_ = new(getGlobals()->getDefaultHeap()) char[1000];
        str_sprintf(query_, "delete with no rollback from %s;", ulTdb().getTableName());
        Lng32 len = 0;
        char dummyArg[128];
        cliRC = cliInterface()->executeImmediate(
                                    query_, dummyArg, &len, FALSE);

        NADELETEBASIC(query_, getMyHeap());

        if (cliRC < 0)
        {
        // mjh - tbd - warning or EMS message to give context to error on
        // delete after error on the insert?
          cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
        }
        else
          masterGlob->setAqrWnrInsertCleanedup();
        SETSTEP(ERROR_);
        break;
      }

      case ERROR_:
      {
        if (pentry_down->downState.request != ex_queue::GET_NOMORE)
        {
          if (getDiagsArea())
          {
            // Any error from child already put a DA into pentry_up.
            if (NULL == pentry_up->getDiagsArea())
            {
              ComDiagsArea * da = ComDiagsArea::allocate(
                                    getGlobals()->getDefaultHeap());
              pentry_up->setDiagsArea(da);
            }
            pentry_up->getDiagsArea()->mergeAfter(*getDiagsArea());
            getDiagsArea()->clear();
            SQL_EXEC_ClearDiagnostics(NULL);
          }
          pentry_up->upState.status = ex_queue::Q_SQLERROR;
          pentry_up->upState.parentIndex = 
                                   pentry_down->downState.parentIndex;
          pentry_up->upState.downIndex = qparent_.down->getHeadIndex();
          qparent_.up->insert();
        }
        SETSTEP(DONE_);
        break;
      }

      case DONE_:
      {
	      // Return EOF.
	      pentry_up->upState.parentIndex =  pentry_down->downState.parentIndex;
        pentry_up->upState.setMatchNo(0);
	      pentry_up->upState.status = ex_queue::Q_NO_DATA;
	        
	      // insert into parent
	      qparent_.up->insert();
	        
	      SETSTEP(INITIAL_);
	      qparent_.down->removeHead();
	        
    	  break;
	    }
    } // switch    
  }  // while

  return WORK_OK;
} 

ExWorkProcRetcode ExExeUtilAqrWnrInsertTcb::workCancel()
{
  if (!(qparent_.down->isEmpty()) &&
      (ex_queue::GET_NOMORE == 
       qparent_.down->getHeadEntry()->downState.request))
  {
    switch (step_)
    {
    case INITIAL_:
	  SETSTEP(DONE_);
	  break;
    case LOCK_TARGET_:
	  ex_assert (0, 
          	"work method doesn't return with step_ set to LOCK_TARGET_.");
	  break;
    case IS_TARGET_EMPTY_:
          ex_assert (0,
                "work method doesn't return with step_ set "
                "to IS_TARGET_EMPTY_.");
	  break;
    case SEND_REQ_TO_CHILD_:
  	SETSTEP(DONE_);
	  break;
    case GET_REPLY_FROM_CHILD_:
	// Assuming that the entire query is canceled. Canceled queries 
	// are not AQR'd. Will cleanup without purging target.
	  qchild_.down->cancelRequest();
	  SETSTEP(CLEANUP_CHILD_);
	break;
    case CLEANUP_CHILD_:
         // CLEANUP_CHILD_ does the right thing when canceling.
	  break;
    case ERROR_:
         // ERROR_ does the right thing when canceling.
  	break;
    case CLEANUP_TARGET_:
	  ex_assert (0, 
          "work method doesn't return with step_ set to CLEANUP_TARGET_,.");
	  break;
    case DONE_:
	ex_assert (0, 
	"work method doesn't return with step_ set to DONE_.");
	break;
    }
  }
  return WORK_OK;
}

#define changeStep(x) changeAndTraceStep(x, __LINE__)

////////////////////////////////////////////////////////////////
// build for class ExExeUtilHbaseLoadTdb
///////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilHBaseBulkLoadTdb::build(ex_globals * glob)
{
  ExExeUtilHBaseBulkLoadTcb * exe_util_tcb;

  exe_util_tcb = new(glob->getSpace()) ExExeUtilHBaseBulkLoadTcb(*this, glob);

  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilHbaseLoadTcb
///////////////////////////////////////////////////////////////
ExExeUtilHBaseBulkLoadTcb::ExExeUtilHBaseBulkLoadTcb(
    const ComTdbExeUtil & exe_util_tdb,
    ex_globals * glob)
: ExExeUtilTcb( exe_util_tdb, NULL, glob),
  step_(INITIAL_),
  nextStep_(INITIAL_),
  rowsAffected_(0),
  loggingLocation_(NULL)
{
  ehi_ = NULL;
  qparent_.down->allocatePstate(this);
}

ExExeUtilHBaseBulkLoadTcb::~ExExeUtilHBaseBulkLoadTcb()
{
   if (loggingLocation_ != NULL) {
      NADELETEBASIC(loggingLocation_, getHeap());
      loggingLocation_ = NULL;
   }
}

//////////////////////////////////////////////////////
// work() for ExExeUtilHbaseLoadTcb
//////////////////////////////////////////////////////
short ExExeUtilHBaseBulkLoadTcb::work()
{
  Lng32 cliRC = 0;
  short retcode = 0;
  short rc;
  Lng32 errorRowCount = 0;
  int len;

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate = *((ExExeUtilPrivateState*) pentry_down->pstate);

  ExMasterStmtGlobals *masterGlob = getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals();
  ContextCli *currContext = masterGlob->getStatement()->getContext();
  ExTransaction *ta = currContext->getTransaction();


  NABoolean ustatNonEmptyTable = FALSE;

  while (1)
  {
    switch (step_)
    {
    case INITIAL_:
    {
      NABoolean xnAlreadyStarted = ta->xnInProgress();

      if (xnAlreadyStarted  &&
          //a transaction is active when we load/populate an indexe table
          !hblTdb().getIndexTableOnly())
      {
        //8111 - Transactions are not allowed with Bulk load.
        ExRaiseSqlError(getHeap(), &diagsArea_, -8111);
        step_ = LOAD_ERROR_;
          break;
      }

      if (setStartStatusMsgAndMoveToUpQueue(" LOAD", &rc))
        return rc;

      if (hblTdb().getUpsertUsingLoad())
        hblTdb().setPreloadCleanup(FALSE);

      if (hblTdb().getTruncateTable())
      {
        step_ = TRUNCATE_TABLE_;
        break;
      }

        // Table will not be truncated, so make sure it is empty if Update
        // Stats has been requested. We obviously have to do this check before
        // the load, but if the table is determined to be non-empty, the
        // message is deferred until the UPDATE_STATS_ step.
        if (hblTdb().getUpdateStats())
        {
          NAString selectFirstQuery = "select [first 1] 0 from ";
          selectFirstQuery.append(hblTdb().getTableName()).append(";");
          cliRC = cliInterface()->executeImmediate(selectFirstQuery.data());
          if (cliRC < 0)
          {
            step_ = LOAD_END_ERROR_;
            break;
          }
          else if (cliRC != 100)
            ustatNonEmptyTable = TRUE;  // So we can display msg later
        }

      step_ = LOAD_START_;
    }
    break;

    case TRUNCATE_TABLE_:
    {
      if (setStartStatusMsgAndMoveToUpQueue(" PURGE DATA",&rc, 0, TRUE))
        return rc;

        // Set the parserflag to prevent privilege checks in purgedata
        NABoolean parserFlagSet = FALSE;
        if ((masterGlob->getStatement()->getContext()->getSqlParserFlags() & 0x20000) == 0)
        {
          parserFlagSet = TRUE;
          masterGlob->getStatement()->getContext()->setSqlParserFlags(0x20000);
        }

      //for now the purgedata statement does not keep the partitions
      char * ttQuery =
          new(getMyHeap()) char[strlen("PURGEDATA  ; ") +
                                strlen(hblTdb().getTableName()) +
                                100];
      strcpy(ttQuery, "PURGEDATA  ");
      strcat(ttQuery, hblTdb().getTableName());
      strcat(ttQuery, ";");

      Lng32 len = 0;
      Int64 rowCount = 0;
      cliRC = cliInterface()->executeImmediate(ttQuery, NULL,NULL,TRUE,NULL,TRUE);
      NADELETEBASIC(ttQuery, getHeap());
      ttQuery = NULL;

        if (parserFlagSet)
          masterGlob->getStatement()->getContext()->resetSqlParserFlags(0x20000);

      if (cliRC < 0)
      {
        cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
        step_ = LOAD_ERROR_;
        break;
      }
      step_ = LOAD_START_;

      setEndStatusMsg(" PURGE DATA", 0, TRUE);
    }
    break;

    case LOAD_START_:
    {
      if (setCQDs() <0)
      {
        step_ = LOAD_END_ERROR_;
        break;
      }
      int jniDebugPort = 0;
      int jniDebugTimeout = 0;
      ehi_ = ExpHbaseInterface::newInstance(getGlobals()->getDefaultHeap(),
                                              (char*)"", //Later may need to change to hblTdb.server_,
                                              (char*)""); //Later may need to change to hblTdb.zkPort_);
      retcode = ehi_->initHBLC();
      if (retcode == 0) 
        retcode = ehi_->createCounterTable(hblTdb().getErrCountTable(), (char *)"ERRORS");
      if (retcode != 0 ) 
      {
         Lng32 cliError = 0;
        Lng32 intParam1 = -retcode;
        ExRaiseSqlError(getHeap(), &diagsArea_,
                          (ExeErrorCode)(8448), NULL, &intParam1,
                          &cliError, NULL,
                          " ",
                          getHbaseErrStr(retcode),
                          (char *)GetCliGlobals()->getJniErrorStr());
        step_ = LOAD_END_ERROR_;
        break;
      }
      if (hblTdb().getPreloadCleanup())
        step_ = PRE_LOAD_CLEANUP_;
      else
      {
        step_ = PREPARATION_;
        if (hblTdb().getRebuildIndexes() || hblTdb().getHasUniqueIndexes())
          step_ = DISABLE_INDEXES_;
      }
    }
    break;

    case PRE_LOAD_CLEANUP_:
    {
      if (setStartStatusMsgAndMoveToUpQueue(" CLEANUP", &rc, 0, TRUE))
           return rc;

        //Cleanup files
        char * clnpQuery =
          new(getMyHeap()) char[strlen("LOAD CLEANUP FOR TABLE ; ") +
                               strlen(hblTdb().getTableName()) +
                               100];
       
        strcpy(clnpQuery, "LOAD CLEANUP FOR TABLE ");
        if (hblTdb().getIndexTableOnly())
          strcat(clnpQuery, "TABLE(INDEX_TABLE ");
        strcat(clnpQuery, hblTdb().getTableName());
        if (hblTdb().getIndexTableOnly())
          strcat(clnpQuery, ")");
        strcat(clnpQuery, ";");

      cliRC = cliInterface()->executeImmediate(clnpQuery, NULL,NULL,TRUE,NULL,TRUE);

      NADELETEBASIC(clnpQuery, getHeap());
      clnpQuery = NULL;
      if (cliRC < 0)
      {
        cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
        step_ = LOAD_END_ERROR_;
        break;
      }

      step_ = PREPARATION_;

      if (hblTdb().getRebuildIndexes() || hblTdb().getHasUniqueIndexes())
        step_ = DISABLE_INDEXES_;

      setEndStatusMsg(" CLEANUP", 0, TRUE);
    }
    break;
    case DISABLE_INDEXES_:
    {
      if (setStartStatusMsgAndMoveToUpQueue(" DISABLE INDEXES", &rc, 0, TRUE))
        return rc;

      // disable indexes before starting the load preparation. load preparation phase will
      // give an error if indexes are not disabled
      // For constarints --disabling/enabling constarints is not supported yet. in this case the user
      // needs to disable or drop the constraints manually before starting load. If constarints
      // exist load preparation will give an error
      char * diQuery =
          new(getMyHeap()) char[strlen("ALTER TABLE DISABLE ALL INDEXES   ; ") +
                                strlen(hblTdb().getTableName()) +
                                100];
      strcpy(diQuery, "ALTER TABLE  ");
      strcat(diQuery, hblTdb().getTableName());
      if (hblTdb().getRebuildIndexes())
        strcat(diQuery, " DISABLE ALL INDEXES ;");
      else
        strcat(diQuery, " DISABLE ALL UNIQUE INDEXES ;"); // has unique indexes and rebuild not specified
      cliRC = cliInterface()->executeImmediate(diQuery, NULL,NULL,TRUE,NULL,TRUE);

      NADELETEBASIC(diQuery, getMyHeap());
      diQuery = NULL;
      if (cliRC < 0)
      {
        cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
        step_ = LOAD_END_ERROR_;
        break;
      }
      step_ = PREPARATION_;

      setEndStatusMsg(" DISABLE INDEXES", 0, TRUE);
    }
    break;

    case PREPARATION_:
    {
      short bufPos = 0;
      if (!hblTdb().getUpsertUsingLoad())
      {
        setLoggingLocation();
        bufPos = printLoggingLocation(0);
        if (setStartStatusMsgAndMoveToUpQueue(" LOADING DATA", &rc, bufPos, TRUE))
           return rc;
        else {
           step_ = LOADING_DATA_;
           return WORK_CALL_AGAIN;
        }  
      }
      else
          step_ = LOADING_DATA_;
    }
    break;

    case LOADING_DATA_:
    {
      if (!hblTdb().getUpsertUsingLoad())
      {
        if (hblTdb().getNoDuplicates())
          cliRC = holdAndSetCQD("TRAF_LOAD_PREP_SKIP_DUPLICATES", "OFF");
        else
          cliRC = holdAndSetCQD("TRAF_LOAD_PREP_SKIP_DUPLICATES", "ON");
        if (cliRC < 0)
        {
          step_ = LOAD_END_ERROR_;
          break;
        }

        if (loggingLocation_ != NULL)
           cliRC = holdAndSetCQD("TRAF_LOAD_ERROR_LOGGING_LOCATION", loggingLocation_);
        if (cliRC < 0)
        {
          step_ = LOAD_END_ERROR_;
          break;
        }

        rowsAffected_ = 0;
        char *loadQuery = hblTdb().ldQuery_;
          if (ustatNonEmptyTable)
            {
              // If the ustat option was specified, but the table to be loaded
              // is not empty, we have to retract the WITH SAMPLE option that
              // was added to the LOAD TRANSFORM statement when the original
              // bulk load statement was parsed.
              const char* sampleOpt = " WITH SAMPLE ";
              char* sampleOptPtr = strstr(loadQuery, sampleOpt);
              if (sampleOptPtr)
                memset(sampleOptPtr, ' ', strlen(sampleOpt));
            }
          //printf("*** Load stmt is %s\n",loadQuery);

          // If the WITH SAMPLE clause is included, set the internal exe util
          // parser flag to allow it.
          NABoolean parserFlagSet = FALSE;
          if (hblTdb().getUpdateStats() && !ustatNonEmptyTable)
          {
            if ((masterGlob->getStatement()->getContext()->getSqlParserFlags() & 0x20000) == 0)
            {
              parserFlagSet = TRUE;
              masterGlob->getStatement()->getContext()->setSqlParserFlags(0x20000);
            }
          }
        ComDiagsArea *diagsArea = getDiagsArea();
        cliRC = cliInterface()->executeImmediate(loadQuery,
            NULL,
            NULL,
            TRUE,
            &rowsAffected_,
            FALSE,
            &diagsArea);
        if (parserFlagSet)
            masterGlob->getStatement()->getContext()->resetSqlParserFlags(0x20000);
        setDiagsArea(diagsArea);
        if (cliRC < 0)
        {
          rowsAffected_ = 0;
          step_ = LOAD_END_ERROR_;
          break;
        }
        else {
           step_ = COMPLETE_BULK_LOAD_;
           if (diagsArea != NULL) {
              ComCondition *cond;
              Lng32 entryNumber;
              while ((cond = diagsArea->findCondition(EXE_ERROR_ROWS_FOUND, &entryNumber)) != NULL) {
                 if (errorRowCount < cond->getOptionalInteger(0))
                    errorRowCount = cond->getOptionalInteger(0);
                 diagsArea->deleteWarning(entryNumber);
              }
              diagsArea->setRowCount(0);
           }
        }
        if (rowsAffected_ == 0)
          step_ = LOAD_END_;

        sprintf(statusMsgBuf_,      "       Rows Processed: %ld %c",rowsAffected_+errorRowCount, '\n' );
        int len = strlen(statusMsgBuf_);
        sprintf(&statusMsgBuf_[len],"       Error Rows:     %d %c",errorRowCount, '\n' );
        len = strlen(statusMsgBuf_);
        setEndStatusMsg(" LOADING DATA", len, TRUE);
      }
      else
      {
        if (setStartStatusMsgAndMoveToUpQueue(" UPSERT USING LOAD ", &rc, 0, TRUE))
          return rc;

        rowsAffected_ = 0;
        char * upsQuery = hblTdb().ldQuery_;
        cliRC = cliInterface()->executeImmediate(upsQuery, NULL, NULL, TRUE, &rowsAffected_);

        upsQuery = NULL;
        if (cliRC < 0)
        {
          cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
          step_ = LOAD_ERROR_;
          break;
        }

          step_ = LOAD_END_;

          if (hblTdb().getRebuildIndexes() || hblTdb().getHasUniqueIndexes())
            step_ = POPULATE_INDEXES_;

        sprintf(statusMsgBuf_,"       Rows Processed: %ld %c",rowsAffected_, '\n' );
        int len = strlen(statusMsgBuf_);
        setEndStatusMsg(" UPSERT USING LOAD ", len, TRUE);
      }
    }
    break;


    case COMPLETE_BULK_LOAD_:
    {
      if (setStartStatusMsgAndMoveToUpQueue(" COMPLETION", &rc,0, TRUE))
        return rc;


      //TRAF_LOAD_TAKE_SNAPSHOT
      if (hblTdb().getNoRollback() )
        cliRC = holdAndSetCQD("TRAF_LOAD_TAKE_SNAPSHOT", "OFF");
      else
        cliRC = holdAndSetCQD("TRAF_LOAD_TAKE_SNAPSHOT", "ON");

      if (cliRC < 0)
      {
        step_ = LOAD_END_ERROR_;
        break;
      }

      //this case is mainly for debugging
      if (hblTdb().getKeepHFiles() &&
          !hblTdb().getSecure() )
      {
        if (holdAndSetCQD("COMPLETE_BULK_LOAD_N_KEEP_HFILES", "ON") < 0)
        {
          step_ = LOAD_END_ERROR_;
          break;
        }
      }
      //complete load query
      char * clQuery =
          new(getMyHeap()) char[strlen("LOAD COMPLETE FOR TABLE  ; ") +
                               strlen(hblTdb().getTableName()) +
                               100];
        strcpy(clQuery, "LOAD COMPLETE FOR TABLE  ");
        if (hblTdb().getIndexTableOnly())
          strcat(clQuery, "TABLE(INDEX_TABLE ");
        strcat(clQuery, hblTdb().getTableName());
        if (hblTdb().getIndexTableOnly())
          strcat(clQuery, ")");
        strcat(clQuery, ";");

      cliRC = cliInterface()->executeImmediate(clQuery, NULL,NULL,TRUE,NULL,TRUE);

      NADELETEBASIC(clQuery, getMyHeap());
      clQuery = NULL;
      if (cliRC < 0)
         rowsAffected_ = 0;
      sprintf(statusMsgBuf_,      "       Rows Loaded:    %ld %c",rowsAffected_, '\n' );
      len = strlen(statusMsgBuf_);
      if (cliRC < 0)
      {
        rowsAffected_ = 0;
        cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
        setEndStatusMsg(" COMPLETION", len, TRUE);
        step_ = LOAD_END_ERROR_;
        break;
      }
      cliRC = restoreCQD("TRAF_LOAD_TAKE_SNAPSHOT");
      if (hblTdb().getRebuildIndexes() || hblTdb().getHasUniqueIndexes())
        step_ = POPULATE_INDEXES_;
      else if (hblTdb().getUpdateStats())
        step_ = UPDATE_STATS_;
      else
        step_ = LOAD_END_;

      setEndStatusMsg(" COMPLETION", len, TRUE);
    }
    break;

    case POPULATE_INDEXES_:
    {
      if (setStartStatusMsgAndMoveToUpQueue(" POPULATE INDEXES", &rc, 0, TRUE))
        return rc;
      else {
           step_ = POPULATE_INDEXES_EXECUTE_;
           return WORK_CALL_AGAIN;
      }
    }
    break;
    case POPULATE_INDEXES_EXECUTE_:
    {
      char * piQuery =
          new(getMyHeap()) char[strlen("POPULATE ALL INDEXES ON  ; ") +
                                strlen(hblTdb().getTableName()) +
                                100];
      if (hblTdb().getRebuildIndexes())
        strcpy(piQuery, "POPULATE ALL INDEXES ON   ");
      else
        strcpy(piQuery, "POPULATE ALL UNIQUE INDEXES ON   "); // has unique indexes and rebuild not used
      strcat(piQuery, hblTdb().getTableName());
      strcat(piQuery, ";");

      cliRC = cliInterface()->executeImmediate(piQuery, NULL,NULL,TRUE,NULL,TRUE);

      NADELETEBASIC(piQuery, getHeap());
      piQuery = NULL;

      if (cliRC < 0)
      {
        cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
        step_ = LOAD_END_ERROR_;
        break;
      }

      if (hblTdb().getUpdateStats())
         step_ = UPDATE_STATS_;
      else
        step_ = LOAD_END_;

        setEndStatusMsg(" POPULATE INDEXES", 0, TRUE);
      }
      break;

      case UPDATE_STATS_:
      {
        if (setStartStatusMsgAndMoveToUpQueue(" UPDATE STATISTICS", &rc, 0, TRUE))
          return rc;
        else {
           step_ = UPDATE_STATS_EXECUTE_;
           return WORK_CALL_AGAIN;
        }
      }
      break;
      case UPDATE_STATS_EXECUTE_:
      {
        if (ustatNonEmptyTable)
        {
          // Table was not empty prior to the load.
          step_ = LOAD_END_;
          sprintf(statusMsgBuf_,
                  "       UPDATE STATISTICS not executed: table %s not empty before load. %c",
                  hblTdb().getTableName(), '\n' );
          int len = strlen(statusMsgBuf_);
          setEndStatusMsg(" UPDATE STATS", len, TRUE);
          break;
        }

        char * ustatStmt =
          new(getMyHeap()) char[strlen("UPDATE STATS FOR TABLE  ON EVERY COLUMN; ") +
                               strlen(hblTdb().getTableName()) +
                               100];
        strcpy(ustatStmt, "UPDATE STATISTICS FOR TABLE ");
        strcat(ustatStmt, hblTdb().getTableName());
        strcat(ustatStmt, " ON EVERY COLUMN;");

        cliRC = holdAndSetCQD("USTAT_USE_BACKING_SAMPLE", "ON");
        if (cliRC < 0)
        {
          step_ = LOAD_END_ERROR_;
          break;
        }

        cliRC = cliInterface()->executeImmediate(ustatStmt, NULL, NULL, TRUE, NULL, TRUE);

        NADELETEBASIC(ustatStmt, getMyHeap());
        ustatStmt = NULL;

        if (cliRC < 0)
        {
          cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
          step_ = LOAD_END_ERROR_;
        }
        else
          step_ = LOAD_END_;

        cliRC = restoreCQD("USTAT_USE_BACKING_SAMPLE");
        if (cliRC < 0)
          step_ = LOAD_END_ERROR_;

        setEndStatusMsg(" UPDATE STATS", 0, TRUE);
    }
    break;

    case RETURN_STATUS_MSG_:
    {
      if (moveRowToUpQueue(statusMsgBuf_,0,&rc))
        return rc;

      step_ = nextStep_;
    }
    break;

    case LOAD_END_:
    case LOAD_END_ERROR_:
    {
      if (restoreCQDs() < 0)
      {
        step_ = LOAD_ERROR_;
        break;
      }
      if (hblTdb().getContinueOnError() && ehi_)
      {
        ehi_->close();
        ehi_ = NULL;
      }
      if (step_ == LOAD_END_)
        step_ = DONE_;
      else
        step_ = LOAD_ERROR_;
    }
    break;

    case DONE_:
    {
      retcode = handleDone();
      if (retcode == 1)
         return WORK_OK;
      masterGlob->setRowsAffected(rowsAffected_);
      step_ = INITIAL_;
      return WORK_OK;
    }
    break;

    case LOAD_ERROR_:
    {
      retcode = handleError();
      if (retcode == 1)
         return WORK_OK;
      step_ = DONE_;
    }
    break;

    } // switch
  } // while

  return WORK_OK;

}

short ExExeUtilHBaseBulkLoadTcb::setCQDs()
{
  if (holdAndSetCQD("COMP_BOOL_226", "ON") < 0) { return -1;}
  // next cqd required to allow load into date/timestamp Traf columns from string Hive columns.
  // This is a common use case. Cqd can be removed when Traf hive access supports more Hive types.
  if (holdAndSetCQD("ALLOW_INCOMPATIBLE_OPERATIONS", "ON") < 0) { return -1;}
  if (hblTdb().getForceCIF())
  {
    if (holdAndSetCQD("COMPRESSED_INTERNAL_FORMAT", "ON") < 0) {return -1; }
    if (holdAndSetCQD("COMPRESSED_INTERNAL_FORMAT_BMO", "ON") < 0){ return -1; }
    if (holdAndSetCQD("COMPRESSED_INTERNAL_FORMAT_DEFRAG_RATIO", "100") < 0) { return -1;}
  }
  if (holdAndSetCQD("TRAF_LOAD_LOG_ERROR_ROWS", (hblTdb().getLogErrorRows()) ? "ON" : "OFF") < 0)
  { return -1;}

  if (holdAndSetCQD("TRAF_LOAD_CONTINUE_ON_ERROR", (hblTdb().getContinueOnError()) ? "ON" : "OFF") < 0)
  { return -1; }

  char strMaxRR[10];
  sprintf(strMaxRR,"%d", hblTdb().getMaxErrorRows());
  if (holdAndSetCQD("TRAF_LOAD_MAX_ERROR_ROWS", strMaxRR) < 0) { return -1;}
  if (hblTdb().getContinueOnError())
  {
    if (holdAndSetCQD("TRAF_LOAD_ERROR_COUNT_TABLE", hblTdb().getErrCountTable()) < 0)
    { return -1;}

    time_t t;
    time(&t);
    char pt[30];
    struct tm * curgmtime = gmtime(&t);
    strftime(pt, 30, "%Y%m%d_%H%M%S", curgmtime);

    if (holdAndSetCQD("TRAF_LOAD_ERROR_COUNT_ID", pt) < 0) { return -1;}
  }
  return 0;
}

short ExExeUtilHBaseBulkLoadTcb::restoreCQDs()
{
  if (restoreCQD("COMP_BOOL_226") < 0) { return -1;}
  if (restoreCQD("TRAF_LOAD_PREP_SKIP_DUPLICATES") < 0)  { return -1;}
  if (restoreCQD("ALLOW_INCOMPATIBLE_OPERATIONS") < 0)  { return -1;}
  if (hblTdb().getForceCIF())
  {
    if (restoreCQD("COMPRESSED_INTERNAL_FORMAT") < 0) { return -1;}
    if (restoreCQD("COMPRESSED_INTERNAL_FORMAT_BMO") < 0)  { return -1;}
    if (restoreCQD("COMPRESSED_INTERNAL_FORMAT_DEFRAG_RATIO") < 0)  { return -1;}
  }
  if (restoreCQD("TRAF_LOAD_LOG_ERROR_ROWS") < 0)  { return -1;}
  if (restoreCQD("TRAF_LOAD_CONTINUE_ON_ERROR") < 0)  { return -1;}
  if (restoreCQD("TRAF_LOAD_MAX_ERROR_ROWS") < 0)  { return -1;}
  if (restoreCQD("TRAF_LOAD_ERROR_COUNT_TABLE") < 0)  { return -1;}
  if (restoreCQD("TRAF_LOAD_ERROR_COUNT_ID") < 0) { return -1; }
  if (restoreCQD("TRAF_LOAD_ERROR_LOGGING_LOCATION") < 0) { return -1; }

  return 0;
}

short ExExeUtilHBaseBulkLoadTcb::moveRowToUpQueue(const char * row, Lng32 len,
    short * rc, NABoolean isVarchar)
{
  if (hblTdb().getNoOutput())
    return 0;

  return ExExeUtilTcb::moveRowToUpQueue(row, len, rc, isVarchar);
}


short ExExeUtilHBaseBulkLoadTcb::printLoggingLocation(int bufPos)
{
  short retBufPos = bufPos;
  if (hblTdb().getNoOutput())
    return 0;
  if (loggingLocation_ != NULL) {
     str_sprintf(&statusMsgBuf_[bufPos], "       Logging Location: %s", loggingLocation_);
     retBufPos = strlen(statusMsgBuf_);
     statusMsgBuf_[retBufPos] = '\n';
     retBufPos++;
  }
  return retBufPos;
}


short ExExeUtilHBaseBulkLoadTcb::setStartStatusMsgAndMoveToUpQueue(const char * operation,
    short * rc,
    int bufPos,
    NABoolean   withtime)
{

  if (hblTdb().getNoOutput())
    return 0;

  char timeBuf[200];

  if (withtime)
  {
    startTime_ = NA_JulianTimestamp();
    getTimestampAsString(startTime_, timeBuf);
  }
  getStatusString(operation, "Started",hblTdb().getTableName(), &statusMsgBuf_[bufPos], FALSE, (withtime ? timeBuf : NULL));
  return moveRowToUpQueue(statusMsgBuf_,0,rc);
}


void ExExeUtilHBaseBulkLoadTcb::setEndStatusMsg(const char * operation,
    int bufPos,
    NABoolean   withtime)
{

  if (hblTdb().getNoOutput())
    return ;

  char timeBuf[200];

  nextStep_ = step_;
  step_ = RETURN_STATUS_MSG_;

  Int64 elapsedTime;
  if (withtime)
  {
    endTime_ = NA_JulianTimestamp();
    elapsedTime = endTime_ - startTime_;
    getTimestampAsString(endTime_, timeBuf);
    getStatusString(operation, "Ended", hblTdb().getTableName(),&statusMsgBuf_[bufPos], FALSE, withtime ? timeBuf : NULL);
    bufPos = strlen(statusMsgBuf_); 
    statusMsgBuf_[bufPos] = '\n';
    bufPos++; 
    getTimeAsString(elapsedTime, timeBuf);
  }
  getStatusString(operation, "Ended", hblTdb().getTableName(),&statusMsgBuf_[bufPos], TRUE, withtime ? timeBuf : NULL);
}

void ExExeUtilHBaseBulkLoadTcb::setLoggingLocation()
{
   char * loggingLocation = hblTdb().getLoggingLocation();
   if (loggingLocation_ != NULL) {
      NADELETEBASIC(loggingLocation_, getHeap());
      loggingLocation_ = NULL;
   }
   if (loggingLocation != NULL) { 
      short logLen = strlen(loggingLocation);
      char *tableName = hblTdb().getTableName();
      short tableNameLen = strlen(tableName);
      loggingLocation_ = new (getHeap()) char[logLen+tableNameLen+100];
      ExHbaseAccessTcb::buildLoggingPath(loggingLocation, NULL, tableName, loggingLocation_);
   }
}

////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ExExeUtilHBaseBulkLoadTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExExeUtilHbaseLoadPrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExExeUtilHbaseLoadPrivateState::ExExeUtilHbaseLoadPrivateState()
{
}

ExExeUtilHbaseLoadPrivateState::~ExExeUtilHbaseLoadPrivateState()
{
};



////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////
// build for class ExExeUtilHbaseUnLoadTdb
///////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilHBaseBulkUnLoadTdb::build(ex_globals * glob)
{
  ExExeUtilHBaseBulkUnLoadTcb * exe_util_tcb;

  exe_util_tcb = new(glob->getSpace()) ExExeUtilHBaseBulkUnLoadTcb(*this, glob);

  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}
void ExExeUtilHBaseBulkUnLoadTcb::createHdfsFileError(Int32 hdfsClientRetCode)
{
  ComDiagsArea * diagsArea = NULL;
  char* errorMsg = HdfsClient::getErrorText((HDFS_Client_RetCode)hdfsClientRetCode);
  ExRaiseSqlError(getHeap(), &diagsArea, (ExeErrorCode)(8447), NULL,
                  NULL, NULL, NULL, errorMsg, (char *)GetCliGlobals()->getJniErrorStr());
  ex_queue_entry *pentry_up = qparent_.up->getTailEntry();
  pentry_up->setDiagsArea(diagsArea);
}
////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilHbaseLoadTcb
///////////////////////////////////////////////////////////////
ExExeUtilHBaseBulkUnLoadTcb::ExExeUtilHBaseBulkUnLoadTcb(
     const ComTdbExeUtil & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilTcb( exe_util_tdb, NULL, glob),
       step_(INITIAL_),
       nextStep_(INITIAL_),
       rowsAffected_(0),
       snapshotsList_(NULL),
       emptyTarget_(FALSE),
       oneFile_(FALSE)
{
  ehi_ = ExpHbaseInterface::newInstance(getGlobals()->getDefaultHeap(),
                                   (char*)"", //Later may need to change to hblTdb.server_,
                                   (char*)""); //Later may need to change to hblTdb.zkPort_);
  qparent_.down->allocatePstate(this);

}
void ExExeUtilHBaseBulkUnLoadTcb::freeResources()
{
  if (snapshotsList_)
  {
    for ( ; snapshotsList_->entries(); )
    {
      snapshotStruct *snp = snapshotsList_->at(0);
      snapshotsList_->removeAt(0);
      NADELETEBASIC(snp->fullTableName, getMyHeap());
      NADELETEBASIC(snp->snapshotName, getMyHeap());
      NADELETEBASIC( snp, getMyHeap());
      snp->fullTableName = NULL;
      snp->snapshotName = NULL;
      snp = NULL;
    }
    NADELETEBASIC (snapshotsList_, getMyHeap());
    snapshotsList_ = NULL;
  }
  NADELETE(ehi_, ExpHbaseInterface, getGlobals()->getDefaultHeap());
  ehi_ = NULL;
}

ExExeUtilHBaseBulkUnLoadTcb::~ExExeUtilHBaseBulkUnLoadTcb()
{
  freeResources();
}
short ExExeUtilHBaseBulkUnLoadTcb::resetExplainSettings()
{
  if (cliInterface()->executeImmediate("control session reset 'EXPLAIN';") < 0)
  {
    cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
    return -1;
  }
  if (restoreCQD("generate_explain") < 0)
  {
    cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
    return -1;
  }
  return 0;
}
short ExExeUtilHBaseBulkUnLoadTcb::getTrafodionScanTables()
{
  // Variables
  SQLMODULE_ID * module = NULL;
  SQLSTMT_ID   * stmt = NULL;
  SQLDESC_ID   * sql_src = NULL;
  SQLDESC_ID   * input_desc = NULL;
  SQLDESC_ID   * output_desc = NULL;
  char         * outputBuf = NULL;

  assert (snapshotsList_ != NULL);

  Lng32 cliRC = 0;

  if (holdAndSetCQD("generate_explain", "ON") < 0)
  {
    cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
    resetExplainSettings();
    return -1;
  }
  // tell mxcmp that this prepare is for explain.
  cliRC = cliInterface()->executeImmediate("control session 'EXPLAIN' 'ON';");
  if (cliRC < 0)
  {
    cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
    resetExplainSettings();
    return cliRC;
  }
  cliInterface()->clearGlobalDiags();
  cliRC = cliInterface()->allocStuff(module, stmt, sql_src, input_desc, output_desc, "__EXPL_STMT_NAME__");
  if (cliRC < 0)
  {
    cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
    resetExplainSettings();
    return cliRC;
  }

  char * stmtStr = hblTdb().uldQuery_;
  cliRC = cliInterface()->prepare(stmtStr, module, stmt, sql_src, input_desc, output_desc, NULL);
  if (cliRC < 0)
  {
    cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
    cliInterface()->deallocStuff(module, stmt, sql_src, input_desc, output_desc);
    resetExplainSettings();
    return cliRC;
  }

  resetExplainSettings();

  NAString qry_str = "";
  qry_str = qry_str + "SELECT DISTINCT ";
  qry_str = qry_str + "CASE ";
  qry_str = qry_str + "WHEN (POSITION('full_table_name:' IN description) = 0 ) OR (POSITION('snapshot_name:' IN description) = 0) ";
  qry_str = qry_str + "THEN NULL ";
  qry_str = qry_str + "ELSE ";
  qry_str = qry_str + "TRIM(SUBSTRING (description from POSITION('full_table_name:' IN description) + CHAR_LENGTH('full_table_name:') ";
  qry_str = qry_str + "FOR  POSITION('snapshot_name:' IN description) - ";
  qry_str = qry_str + "     POSITION('full_table_name:' IN description) - CHAR_LENGTH('full_table_name:'))) ";
  qry_str = qry_str + "END AS full_table_name, ";
  qry_str = qry_str + "CASE  ";
  qry_str = qry_str + "WHEN (POSITION('snapshot_temp_location:' IN description) = 0 ) OR ( POSITION('snapshot_name:' IN description) = 0)  ";
  qry_str = qry_str + "THEN  NULL ";
  qry_str = qry_str + "ELSE ";
  qry_str = qry_str + "TRIM(SUBSTRING (description from POSITION('snapshot_name:' IN description) + CHAR_LENGTH('snapshot_name:') ";
  qry_str = qry_str + "  FOR  POSITION('snapshot_temp_location:' IN description) - ";
  qry_str = qry_str + "  POSITION('snapshot_name:' IN description) - CHAR_LENGTH('snapshot_name:'))) ";
  qry_str = qry_str + "END AS snapshot_name ";
  qry_str = qry_str + "FROM TABLE (EXPLAIN (NULL,'__EXPL_STMT_NAME__'))  WHERE TRIM(OPERATOR) LIKE 'TRAFODION%SCAN%' ; ";

  Queue * tbls = NULL;
  cliRC = cliInterface()->fetchAllRows(tbls, (char*)qry_str.data(), 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
  {
    cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
    cliInterface()->deallocStuff(module, stmt, sql_src, input_desc, output_desc);
    return cliRC;
  }

  if (tbls)
  {

    if (tbls->numEntries() == 0)
    {
      cliRC = cliInterface()->deallocStuff(module, stmt, sql_src, input_desc, output_desc);
      if (cliRC < 0)
      {
        cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
        return cliRC;
      }
    }

    tbls->position();
    for (int ii = 0; ii < tbls->numEntries(); ii++)
    {
      OutputInfo * idx = (OutputInfo*) tbls->getNext();
      snapshotStruct * snap = new (getMyHeap()) snapshotStruct();
      snap->fullTableName = new (getMyHeap()) NAString(idx->get(0),getMyHeap());
      snap->snapshotName =  new (getMyHeap()) NAString(idx->get(1),getMyHeap());

      //remove trailing spaces
      snap->fullTableName->strip(NAString::trailing, ' ');
      snap->snapshotName->strip(NAString::trailing, ' ');
      ex_assert(snap->fullTableName->length()>0 &&
                snap->snapshotName->length()>0 ,
                "full table name and snapshot name cannot be empty");
      snapshotsList_->insert(snap);
    }
  }

  cliRC = cliInterface()->deallocStuff(module, stmt, sql_src, input_desc, output_desc);
  if (cliRC < 0)
  {
    cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
    return cliRC;
  }
  return snapshotsList_->entries();
}
//////////////////////////////////////////////////////
// work() for ExExeUtilHbaseLoadTcb
//////////////////////////////////////////////////////
short ExExeUtilHBaseBulkUnLoadTcb::work()
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;
  short rc;
  HDFS_Client_RetCode hdfsClientRetCode = HDFS_CLIENT_OK;
  Lng32 hbcRetCode = HBC_OK;
  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate = *((ExExeUtilPrivateState*) pentry_down->pstate);
  ExMasterStmtGlobals *masterGlob = getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals();
  ExTransaction *ta = masterGlob->getStatement()->getContext()->getTransaction();

  while (1)
  {
    switch (step_)
    {
    case INITIAL_:
    {
      NABoolean xnAlreadyStarted = ta->xnInProgress();
      if (xnAlreadyStarted  )
      {
        //8111 - Transactions are not allowed with Bulk unload.
        ExRaiseSqlError(getHeap(), &diagsArea_, -8111);
        step_ = UNLOAD_ERROR_;
        break;
      }
      setEmptyTarget(hblTdb().getEmptyTarget());
      setOneFile(hblTdb().getOneFile());
      if ((retcode = ehi_->init(NULL)) != HBASE_ACCESS_SUCCESS)
      {
         ExHbaseAccessTcb::setupError((NAHeap *)getMyHeap(),qparent_, retcode, 
                "ExpHbaseInterface_JNI::init"); 
         step_ = UNLOAD_END_ERROR_;
         break;
      }
      if (!hblTdb().getOverwriteMergeFile() &&  hblTdb().getMergePath() != NULL)
      {
        NABoolean exists = FALSE;
        hdfsClientRetCode = HdfsClient::hdfsExists( hblTdb().getMergePath(), exists);
        if (hdfsClientRetCode != HDFS_CLIENT_OK)
        {
          createHdfsFileError(hdfsClientRetCode);
          step_ = UNLOAD_END_ERROR_;
          break;
        }
        if (exists)
        {
          //EXE_UNLOAD_FILE_EXISTS
          ExRaiseSqlError(getHeap(), &diagsArea_, -EXE_UNLOAD_FILE_EXISTS,
                  NULL, NULL, NULL,
                  hblTdb().getMergePath());
          step_ = UNLOAD_END_ERROR_;
          break;
        }
      }
      if (holdAndSetCQD("COMP_BOOL_226", "ON") < 0)
      {
        step_ = UNLOAD_END_ERROR_;
        break;
      }
      if (hblTdb().getSkipWriteToFiles())
      {
        setEmptyTarget(FALSE);
        setOneFile(FALSE);
      }
      if (setStartStatusMsgAndMoveToUpQueue("UNLOAD", &rc))
        return rc;

      if (hblTdb().getCompressType() == 0)
        cliRC = holdAndSetCQD("TRAF_UNLOAD_HDFS_COMPRESS", "0");
      else
        cliRC = holdAndSetCQD("TRAF_UNLOAD_HDFS_COMPRESS", "1");
      if (cliRC < 0)
      {
        step_ = UNLOAD_END_ERROR_;
        break;
      }

      if (hblTdb().getScanType()== ComTdbExeUtilHBaseBulkUnLoad::REGULAR_SCAN)
        cliRC = holdAndSetCQD("TRAF_TABLE_SNAPSHOT_SCAN", "NONE");
      else
      {
        cliRC = holdAndSetCQD("TRAF_TABLE_SNAPSHOT_SCAN", "SUFFIX");
        if (cliRC < 0)
        {
          step_ = UNLOAD_END_ERROR_;
          break;
        }
      }
      cliRC = holdAndSetCQD("TRAF_TABLE_SNAPSHOT_SCAN_TABLE_SIZE_THRESHOLD", "0");
      if (cliRC < 0)
      {
        step_ = UNLOAD_END_ERROR_;
        break;
      }

      if (hblTdb().getSnapshotSuffix() != NULL)
      {
        cliRC = holdAndSetCQD("TRAF_TABLE_SNAPSHOT_SCAN_SNAP_SUFFIX", hblTdb().getSnapshotSuffix());
        if (cliRC < 0)
        {
          step_ = UNLOAD_END_ERROR_;
          break;
        }
      }

      step_ = UNLOAD_;
      if (hblTdb().getScanType() == ComTdbExeUtilHBaseBulkUnLoad::SNAPSHOT_SCAN_CREATE )
        step_ = CREATE_SNAPSHOTS_;
      else if (hblTdb().getScanType() == ComTdbExeUtilHBaseBulkUnLoad::SNAPSHOT_SCAN_EXISTING )
        step_ = VERIFY_SNAPSHOTS_;
      if (getEmptyTarget())
        step_ = EMPTY_TARGET_;
    }
    break;
    case EMPTY_TARGET_:
    {

      if (setStartStatusMsgAndMoveToUpQueue(" EMPTY TARGET ", &rc, 0, TRUE))
        return rc;

      NAString uldPath ( hblTdb().getExtractLocation());

      hdfsClientRetCode = HdfsClient::hdfsCleanUnloadPath( uldPath);
      if (hdfsClientRetCode != HDFS_CLIENT_OK)
      {
        createHdfsFileError(hdfsClientRetCode);
        step_ = UNLOAD_END_ERROR_;
        break;
      }
      step_ = UNLOAD_;
      if (hblTdb().getScanType() == ComTdbExeUtilHBaseBulkUnLoad::SNAPSHOT_SCAN_CREATE) //SNAPSHOT_SCAN_CREATE_ = 1
        step_ = CREATE_SNAPSHOTS_;
      else if (hblTdb().getScanType() == ComTdbExeUtilHBaseBulkUnLoad::SNAPSHOT_SCAN_EXISTING )
        step_ = VERIFY_SNAPSHOTS_;

      setEndStatusMsg(" EMPTY TARGET ", 0, TRUE);
    }
    break;

    case CREATE_SNAPSHOTS_:
    case VERIFY_SNAPSHOTS_:
    {
      NABoolean createSnp = (step_ == CREATE_SNAPSHOTS_)? TRUE : FALSE;
      NAString msg(createSnp ? " CREATE SNAPSHOTS " : " VERIFY SNAPSHOTS ");
      NAString msg2 (createSnp ? "created" : "verified");
      if (setStartStatusMsgAndMoveToUpQueue( msg.data() , &rc, 0, TRUE))
        return rc;

      assert (snapshotsList_ == NULL);
      snapshotsList_ = new (getMyHeap()) NAList<struct snapshotStruct *> (getMyHeap());
      Lng32 rc = getTrafodionScanTables();
      if (rc < 0)
      {
        step_ = UNLOAD_END_ERROR_;
        break;
      }

      for ( int i = 0 ; i < snapshotsList_->entries(); i++)
      {
        if (createSnp)
          hbcRetCode = ehi_->createSnapshot( *snapshotsList_->at(i)->fullTableName, *snapshotsList_->at(i)->snapshotName);
        else
        {
          NABoolean exist = FALSE;
          hbcRetCode = ehi_->verifySnapshot(*snapshotsList_->at(i)->fullTableName, *snapshotsList_->at(i)->snapshotName, exist);
          if ( hbcRetCode == HBC_OK && !exist)
          {
            ExRaiseSqlError(getHeap(), &diagsArea_, -8112,
                  NULL, NULL, NULL,
                  snapshotsList_->at(i)->snapshotName->data(),
                  snapshotsList_->at(i)->fullTableName->data());
            step_ = UNLOAD_END_ERROR_;
            break;
          }
        }
        if (hbcRetCode != HBC_OK)
        {
          ExHbaseAccessTcb::setupError((NAHeap *)getMyHeap(),qparent_, hbcRetCode, 
                "HBaseClient_JNI::createSnapshot/verifySnapshot", 
                snapshotsList_->at(i)->snapshotName->data() );
          step_ = UNLOAD_END_ERROR_;
          break;
        }
      }
      if (step_ == UNLOAD_END_ERROR_)
        break;

      step_ = UNLOAD_;
      sprintf(statusMsgBuf_,"       Snapshots %s: %d %c",msg2.data(), (int)snapshotsList_->entries(), '\n' );
      int len = strlen(statusMsgBuf_);
      setEndStatusMsg(msg.data(), len, TRUE);
    }
    break;

    case UNLOAD_:
    {
      if (setStartStatusMsgAndMoveToUpQueue(" EXTRACT ", &rc, 0, TRUE))
        return rc;

      rowsAffected_ = 0;
      cliRC = cliInterface()->executeImmediate(hblTdb().uldQuery_,
          NULL,
          NULL,
          TRUE,
          &rowsAffected_);
      if (cliRC < 0)
      {
        rowsAffected_ = 0;
        cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
        step_ = UNLOAD_END_ERROR_;
        break;
      }
      step_ = UNLOAD_END_;

      if (getOneFile())
        step_ = MERGE_FILES_;
      if (hblTdb().getScanType() == ComTdbExeUtilHBaseBulkUnLoad::SNAPSHOT_SCAN_CREATE ) 
          step_ = DELETE_SNAPSHOTS_;

      if (hblTdb().getSkipWriteToFiles())
        sprintf(statusMsgBuf_,"       Rows Processed but NOT Written to Disk: %ld %c",rowsAffected_, '\n' );
      else
        sprintf(statusMsgBuf_,"       Rows Processed: %ld %c",rowsAffected_, '\n' );
      int len = strlen(statusMsgBuf_);
      setEndStatusMsg(" EXTRACT ", len, TRUE);

    }
    break;

    case DELETE_SNAPSHOTS_:
    {
      if (setStartStatusMsgAndMoveToUpQueue(" DELETE SNAPSHOTS ", &rc, 0, TRUE))
        return rc;
      for ( int i = 0 ; i < snapshotsList_->entries(); i++)
      {
        hbcRetCode = ehi_->deleteSnapshot( *snapshotsList_->at(i)->snapshotName);
        if (hbcRetCode != HBC_OK)
        {
          ExHbaseAccessTcb::setupError((NAHeap *)getMyHeap(),qparent_, hbcRetCode, 
                "HBaseClient_JNI::createSnapshot/verifySnapshot", 
                snapshotsList_->at(i)->snapshotName->data() );
          step_ = UNLOAD_END_ERROR_;
          break;
        }
      }
      if (step_ == UNLOAD_END_ERROR_)
        break;

      step_ = UNLOAD_END_;
      if (getOneFile())
        step_ = MERGE_FILES_;

      sprintf(statusMsgBuf_,"       Snapshots deleted: %d %c",(int)snapshotsList_->entries(), '\n' );
      int len = strlen(statusMsgBuf_);
      setEndStatusMsg(" DELETE SNAPSHOTS ", len, TRUE);
    }
    break;

    case MERGE_FILES_:
    {
      if (setStartStatusMsgAndMoveToUpQueue(" MERGE FILES ", &rc, 0, TRUE))
        return rc;

      NAString srcPath ( hblTdb().getExtractLocation());
      NAString dstPath ( hblTdb().getMergePath());
      hdfsClientRetCode = HdfsClient::hdfsMergeFiles( srcPath, dstPath);
      if (hdfsClientRetCode != HDFS_CLIENT_OK)
      {
        createHdfsFileError(hdfsClientRetCode);
        step_ = UNLOAD_END_;
        break;
      }
      step_ = UNLOAD_END_;

      setEndStatusMsg(" MERGE FILES ", 0, TRUE);
    }
    break;

    case UNLOAD_END_:
    case UNLOAD_END_ERROR_:
    {
      ehi_->close();
      if (restoreCQD("TRAF_TABLE_SNAPSHOT_SCAN") < 0)
      {
        step_ = UNLOAD_ERROR_;
        break;
      }
      if (restoreCQD("TRAF_TABLE_SNAPSHOT_SCAN_TABLE_SIZE_THRESHOLD") < 0)
      {
        step_ = UNLOAD_ERROR_;
        break;
      }
      if (hblTdb().getSnapshotSuffix() != NULL)
      {
        if (restoreCQD("TRAF_TABLE_SNAPSHOT_SCAN_SNAP_SUFFIX") < 0)
        {
          step_ = UNLOAD_ERROR_;
          break;
        }
      }
      if (restoreCQD("COMP_BOOL_226") < 0)
      {
        step_ = UNLOAD_ERROR_;
        break;
      }
      if ( restoreCQD("TRAF_UNLOAD_HDFS_COMPRESS") < 0)
      {
        step_ = UNLOAD_ERROR_;
        break;
      }
      if (step_ == UNLOAD_END_)
        step_ = DONE_;
      else
        step_ = UNLOAD_ERROR_;
    }

    break;
    case RETURN_STATUS_MSG_:
    {
      if (moveRowToUpQueue(statusMsgBuf_,0,&rc))
        return rc;

      step_ = nextStep_;
    }
    break;

    case DONE_:
    {
      retcode = handleDone();
      if (retcode == 1)
        return WORK_OK;
      masterGlob->setRowsAffected(rowsAffected_);
      step_ = INITIAL_;
      freeResources();
      return WORK_OK;
    }
    break;

    case UNLOAD_ERROR_:
    {
      retcode = handleError();
      if (retcode == 1)
         return WORK_OK;
      step_ = DONE_;
    }
    break;

    } // switch
  } // while

  return WORK_OK;
}

short ExExeUtilHBaseBulkUnLoadTcb::moveRowToUpQueue(const char * row, Lng32 len,
                                                  short * rc, NABoolean isVarchar)
{
  if (hblTdb().getNoOutput())
    return 0;

  return ExExeUtilTcb::moveRowToUpQueue(row, len, rc, isVarchar);
}




short ExExeUtilHBaseBulkUnLoadTcb::setStartStatusMsgAndMoveToUpQueue(const char * operation,
                                     short * rc,
                                     int bufPos,
                                     NABoolean   withtime)
{

  if (hblTdb().getNoOutput())
     return 0;
  char timeBuf[200];
  if (withtime) {
    startTime_ = NA_JulianTimestamp();
    getTimestampAsString(startTime_, timeBuf);
  }
  getStatusString(operation, "Started",NULL, &statusMsgBuf_[bufPos], FALSE, (withtime ? timeBuf : NULL));
  return moveRowToUpQueue(statusMsgBuf_,0,rc);
}


void ExExeUtilHBaseBulkUnLoadTcb::setEndStatusMsg(const char * operation,
                                     int bufPos,
                                     NABoolean   withtime)
{

  if (hblTdb().getNoOutput())
     return ;

  char timeBuf[200];

  nextStep_ = step_;
  step_ = RETURN_STATUS_MSG_;

  if (withtime)
  {
    endTime_ = NA_JulianTimestamp();
    Int64 elapsedTime = endTime_ - startTime_;
    getTimestampAsString(endTime_, timeBuf);
    getStatusString(operation, "Ended", hblTdb().getTableName(),&statusMsgBuf_[bufPos], FALSE, withtime ? timeBuf : NULL);
    bufPos = strlen(statusMsgBuf_); 
    statusMsgBuf_[bufPos] = '\n';
    bufPos++; 
    getTimeAsString(elapsedTime, timeBuf);
  }

  getStatusString(operation, "Ended", NULL,&statusMsgBuf_[bufPos], TRUE, withtime ? timeBuf : NULL);
}

////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ExExeUtilHBaseBulkUnLoadTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExExeUtilHbaseUnLoadPrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExExeUtilHbaseUnLoadPrivateState::ExExeUtilHbaseUnLoadPrivateState()
{
}

ExExeUtilHbaseUnLoadPrivateState::~ExExeUtilHbaseUnLoadPrivateState()
{
};

///////////////////////////////////////////////////////////////////
// class ExExeUtilLobExtractTdb
///////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilLobExtractTdb::build(ex_globals * glob)
{
  ExExeUtilLobExtractTcb * exe_util_tcb;

  ex_tcb * childTcb = NULL;
  if (child_)
    {
      // build the child first
      childTcb = child_->build(glob);
    }

  if ((getToType() == ComTdbExeUtilLobExtract::TO_EXTERNAL_FROM_STRING_) ||
      (getToType() == ComTdbExeUtilLobExtract::TO_EXTERNAL_FROM_FILE_))
     exe_util_tcb = new(glob->getSpace()) 
       ExExeUtilFileLoadTcb(*this, childTcb, glob);
   else if (srcIsFile())
    exe_util_tcb = new(glob->getSpace()) 
      ExExeUtilFileExtractTcb(*this, childTcb, glob);
  else
    exe_util_tcb = new(glob->getSpace()) 
      ExExeUtilLobExtractTcb(*this, childTcb, glob);

  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

ExExeUtilLobExtractTcb::ExExeUtilLobExtractTcb
(
 const ComTdbExeUtilLobExtract & exe_util_tdb,
 const ex_tcb * child_tcb, 
 ex_globals * glob)
  : ExExeUtilTcb(exe_util_tdb, child_tcb, glob),
    step_(EMPTY_)    
{
  ContextCli *currContext =
    getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals()->
    getStatement()->getContext();
  lobHandleLen_ = 2050;
  lobHandle_[0] = '\0';

  lobInputHandleBuf_[0] = '\0';

  lobNameBuf_[0] = '\0';
  lobNameLen_ =1024;
  lobName_ = NULL;
  statusString_[0] = '\0';
  lobType_ = 0;

  lobData_= NULL;
  lobData2_= NULL;

  lobDataSpecifiedExtractLen_ = 0; // default. Actual value set from tdb below
  lobDataLen_= 0;
  
  remainingBytes_= 0;
  currPos_ = 0;

  numChildRows_ = 0;

  requestTag_ = -1;
  lobLoc_[0] = '\0';
  exLobGlobals_ = ExpLOBoper::initLOBglobal((NAHeap *)glob->getDefaultHeap(), currContext, exe_util_tdb.useLibHdfs());
}

void ExExeUtilLobExtractTcb::freeResources()
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  ExLobGlobals * lobGlobs = getLobGlobals();

  ContextCli *currContext =
    getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals()->
    getStatement()->getContext();
  //close any open cursors.
  if (lobHandle_ and lobName_)
    retcode = ExpLOBInterfaceSelectCursor
	      (lobGlobs,
               (getStatsEntry() != NULL ? getStatsEntry()->castToExHdfsScanStats() : NULL),
	       lobName_, 
	       lobLoc_,
	       lobType_,
	       lobTdb().getLobHdfsServer(),
	       lobTdb().getLobHdfsPort(),

	       lobHandleLen_, lobHandle_,
               0, //cursor bytes
               NULL, //cursor id
	       requestTag_, 
	       Lob_Buffer,
	       0, // not check status
	       1, // waited op

	       0, lobDataSpecifiedExtractLen_, 
	       lobDataLen_, lobData_, 
	       3, // close
               0); // open type not applicable
  ExpLOBoper::deleteLOBglobal(exLobGlobals_, (NAHeap *)(NAHeap *)getGlobals()->getDefaultHeap());
  exLobGlobals_ = NULL;
}

ExExeUtilLobExtractTcb::~ExExeUtilLobExtractTcb()
{
  freeResources();
}

ExOperStats * ExExeUtilLobExtractTcb::doAllocateStatsEntry(CollHeap *heap,
							    ComTdb *tdb)
{
  ExEspStmtGlobals *espGlobals = getGlobals()->castToExExeStmtGlobals()->castToExEspStmtGlobals();
  StmtStats *ss; 
  if (espGlobals != NULL)
     ss = espGlobals->getStmtStats();
  else
     ss = getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals()->getStatement()->getStmtStats(); 
  ExHdfsScanStats *hdfsScanStats = new(heap) ExHdfsScanStats(heap,
				   this,
				   tdb);
  if (ss != NULL) 
     hdfsScanStats->setQueryId(ss->getQueryId(), ss->getQueryIdLen());
  return hdfsScanStats;
}

short ExExeUtilLobExtractTcb::work()
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;
  Int64 lobDataOutputLen = 0;
  Int64 requestTag = -1;
  LobsSubOper so = Lob_None;
  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate =
    *((ExExeUtilPrivateState*) pentry_down->pstate);

  ContextCli *currContext =
    getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals()->
    getStatement()->getContext();

  ComDiagsArea & diags       = currContext->diags();

  

  ExLobGlobals * lobGlobs = getLobGlobals();

  ex_queue_entry * centry = NULL;
  
  while (1)
    {
      switch (step_)
	{
	case EMPTY_:
	  {
	    workAtp_->getTupp(lobTdb().workAtpIndex())
	      .setDataPointer(lobInputHandleBuf_);

	    if (getChild(0))
	      step_ = SEND_REQ_TO_CHILD_;
	    else
	      step_ = GET_NO_CHILD_HANDLE_;
	  }
	  break;
	  
	case GET_NO_CHILD_HANDLE_:
	  {
	    ex_expr::exp_return_type exprRetCode =
	      lobTdb().inputExpr_->eval(NULL, 
					workAtp_);
	    if (exprRetCode == ex_expr::EXPR_ERROR)
	      {
		step_ = CANCEL_;
		break;
	      }
	    
	    step_ = GET_LOB_HANDLE_;
	  }
	  break;
	case SEND_REQ_TO_CHILD_:
	  {
	    if (qchild_.down->isFull())
	      return WORK_OK;

	    ex_queue_entry * centry = qchild_.down->getTailEntry();

	    centry->downState.request = ex_queue::GET_ALL;
	    centry->downState.requestValue = 
	      pentry_down->downState.requestValue;
	    centry->downState.parentIndex = qparent_.down->getHeadIndex();
	    
	    // set the child's input atp
	    centry->passAtp(pentry_down->getAtp());
	    
	    qchild_.down->insert();

	    numChildRows_ = 0;

	    step_ = GET_REPLY_FROM_CHILD_;
	  }
	break;

	case GET_REPLY_FROM_CHILD_:
	  {
	    // if nothing returned from child. Get outta here.
	    if (qchild_.up->isEmpty())
	      return WORK_OK;

	    // check if we've got room in the up queue
	    if (qparent_.up->isFull())
	      return WORK_OK; // parent queue is full. Just return

	    centry = qchild_.up->getHeadEntry();
	    
	    ex_queue::up_status child_status = centry->upState.status;
	    switch(child_status)
	      {
	      case ex_queue::Q_NO_DATA:
		{
		  qchild_.up->removeHead();	    

		  if (numChildRows_ == 0)
		    step_ = DONE_;
		  else if (numChildRows_ == 1)
		    step_ = GET_LOB_HANDLE_;
		  else
		    {
		      Lng32 cliError = 0;
		      
		      Lng32 intParam1 = 0;
		      ExRaiseSqlError(getHeap(), &diagsArea_, 
				      -8448, &intParam1, &cliError, NULL, NULL);
		      step_ = HANDLE_ERROR_;
		    }
		}
	      break;

	      case ex_queue::Q_OK_MMORE:
		{
		  if (numChildRows_ == 0) // first child row
		    {
		      ex_expr::exp_return_type exprRetCode =
			lobTdb().inputExpr_->eval(centry->getAtp(), 
						  workAtp_);
		      if (exprRetCode == ex_expr::EXPR_ERROR)
			{
			  step_ = CANCEL_;
			  break;
			}

		    }

		  qchild_.up->removeHead();	    
		  numChildRows_++;
		  //		  step_ = GET_LOB_HANDLE_;
		}
		break;

	      case ex_queue::Q_INVALID:
		{
		  ex_queue_entry * up_entry = qparent_.up->getTailEntry();

		  // invalid state, should not be reached.
		  ExRaiseSqlError(getMyHeap(),
				  &diagsArea_,
				  (ExeErrorCode)(EXE_INTERNAL_ERROR));
		  step_ = CANCEL_;
		}
		break;

	      case ex_queue::Q_SQLERROR:
		{
		  step_ = ERROR_FROM_CHILD_;
		}
	      break;
	      }
	  }
	break;

	case ERROR_FROM_CHILD_:
	  {
            // check if we've got room in the up queue
	    if (qparent_.up->isFull())
	      return WORK_OK; // parent queue is full. Just return

	    ex_queue_entry *pentry_up = qparent_.up->getTailEntry();
	    ex_queue_entry * centry = qchild_.up->getHeadEntry();
	    
	    qchild_.down->cancelRequestWithParentIndex(qparent_.down->getHeadIndex());
            pentry_up->copyAtp(centry);
	    pentry_up->upState.status = ex_queue::Q_SQLERROR;
	    pentry_up->upState.parentIndex = pentry_down->downState.parentIndex;
	    pentry_up->upState.downIndex = qparent_.down->getHeadIndex();

            qparent_.up->insert();
	    qchild_.up->removeHead();	    

	    step_ = CANCEL_;
	  }
	break;

	case CANCEL_:
	  {
            // ignore all up rows from child. wait for Q_NO_DATA.
	    if (qchild_.up->isEmpty())
	      return WORK_OK;

	    ex_queue_entry * centry = qchild_.up->getHeadEntry();
	
	    switch(centry->upState.status)
	      {
	      case ex_queue::Q_OK_MMORE:
	      case ex_queue::Q_SQLERROR:
	      case ex_queue::Q_INVALID:
		{
		  qchild_.up->removeHead();
		}
	      break;
	      
	      case ex_queue::Q_NO_DATA:
		{
		  qchild_.up->removeHead();

		  step_ = HANDLE_ERROR_;
		}
	      break;
	      
	      }
	  }
	break;
	
	case GET_LOB_HANDLE_:
	  { 
	    
	    
	    if (lobTdb().handleInStringFormat())
	      {
	
		if (ExpLOBoper::genLOBhandleFromHandleString
		    (lobTdb().getHandle(),
		     lobTdb().getHandleLen(),
		     lobHandle_,
		     lobHandleLen_))
		  {
		    ExRaiseSqlError(getMyHeap(),
				    &diagsArea_,
				    (ExeErrorCode)(EXE_INVALID_LOB_HANDLE));
		    step_ = HANDLE_ERROR_;
		    break;
		  }
		  
	      }
	    else
	      {
	
		lobHandleLen_ = *(short*)&lobInputHandleBuf_[sizeof(short)]; //lobTdb().getHandleLen();
		str_cpy_all(lobHandle_, &lobInputHandleBuf_[sizeof(short)], lobHandleLen_); //lobTdb().getHandle();
		if (*(short*)lobInputHandleBuf_ != 0) //null value
		  {
		    step_ = DONE_;
		    break;
		  }

	      }
            Int16 flags;
	    Lng32  lobNum;
	    Int64 uid, inDescSyskey, descPartnKey;
	    short schNameLen;
	    char schName[1024];
	    ExpLOBoper::extractFromLOBhandle(&flags, &lobType_, &lobNum, &uid,  
					     &inDescSyskey, &descPartnKey, 
					     &schNameLen, (char *)schName,
					     (char *)lobHandle_, (Lng32)lobHandleLen_);

            //Retrieve the lobLocation for this lobNum which will be used 
            //in the other steps_ which open and read lob data file.
           
	    strcpy(lobLoc_, lobTdb().getStringParam2());
           
            if (lobTdb().getToType() == ComTdbExeUtilLobExtract::RETRIEVE_HDFSFILENAME_)
	      step_ = EXTRACT_HDFSFILENAME_;
            else if (lobTdb().getToType() == ComTdbExeUtilLobExtract::RETRIEVE_OFFSET_)
	      step_ = RETRIEVE_OFFSET_;     
	    else if (lobTdb().getToType() == ComTdbExeUtilLobExtract::TO_BUFFER_)
	      step_ = EXTRACT_LOB_DATA_;
	    else if ((lobTdb().getToType() == ComTdbExeUtilLobExtract::RETRIEVE_LENGTH_) || (lobTdb().getToType() == ComTdbExeUtilLobExtract::TO_FILE_))
	      step_ = RETRIEVE_LOB_LENGTH_;
	      else
		{
		  // invalid "toType"
		  ExRaiseSqlError(getMyHeap(),
				  &diagsArea_,
				  (ExeErrorCode)(EXE_INTERNAL_ERROR));
		  step_ = CANCEL_;
		
		break;

		}
	    break;
	  }
        case EXTRACT_HDFSFILENAME_:
          {
	    Int16 flags;
	    Lng32  lobNum;
	    Int64 uid, inDescSyskey, descPartnKey;
	    short schNameLen;
	    char schName[1024]={'\0'};
            char hdfsFileName[MAX_LOB_FILE_NAME_LEN]={'\0'};
            
            Int32 fileNameLen = 0;
	    ExpLOBoper::extractFromLOBhandle(&flags, &lobType_, &lobNum, &uid,  
					     &inDescSyskey, &descPartnKey, 
					     &schNameLen, (char *)schName,
					     (char *)lobHandle_, (Lng32)lobHandleLen_);


	    lobName_ = ExpLOBoper::ExpGetLOBname(uid, lobNum, lobNameBuf_, 1000);	   
            //Retrieve the filename of this lob using the handle info and return to the caller
            retcode = ExpLOBInterfaceGetFileName( lobGlobs,
                                                  lobName_, 
                                                  lobLoc_,
                                                  lobType_,
                                                  lobTdb().getLobHdfsServer(),
                                                  lobTdb().getLobHdfsPort(),
                                                  lobHandleLen_, lobHandle_, 
                                                  hdfsFileName,
                                                  fileNameLen);
       
            if ((lobTdb().getBufAddr() != -1) && (lobTdb().getBufAddr() != 0))
              str_cpy_all((char *)lobTdb().getBufAddr(), (char *)&lobDataLen_,sizeof(Int64));
            str_sprintf(statusString_," LOB filename : %s", hdfsFileName);
            step_ = RETURN_STATUS_;
            break;	
              
          }
          break;
	case RETRIEVE_LOB_LENGTH_ : 
	  {
	    Int16 flags;
	    Lng32  lobNum;
	    Int64 uid, inDescSyskey, descPartnKey;
	    short schNameLen;
	    char schName[1024];
         
	    ExpLOBoper::extractFromLOBhandle(&flags, &lobType_, &lobNum, &uid,  
					     &inDescSyskey, &descPartnKey, 
					     &schNameLen, (char *)schName,
					     (char *)lobHandle_, (Lng32)lobHandleLen_);


	    lobName_ = ExpLOBoper::ExpGetLOBname(uid, lobNum, lobNameBuf_, 1000);	   
       
            //Retrieve the total length of this lob using the handle info and return to the caller

            retcode = ExpLOBInterfaceGetLobLength( lobGlobs,
                                                   lobName_, 
                                                   lobLoc_,
                                                   lobType_,
                                                   lobTdb().getLobHdfsServer(),
                                                   lobTdb().getLobHdfsPort(),
                                                   lobHandleLen_, lobHandle_, 
                                                   lobDataLen_);
                                                  
            if  (lobTdb().retrieveLength())
              {
                if ((lobTdb().getBufAddr() != -1) && (lobTdb().getBufAddr() != 0))
                  str_cpy_all((char *)lobTdb().getBufAddr(), (char *)&lobDataLen_,sizeof(Int64));
                str_sprintf(statusString_," LOB Length : %ld", lobDataLen_);
                step_ = RETURN_STATUS_;
                break;	
              }
            else
              step_ = EXTRACT_LOB_DATA_;
            break;
	      
	  }
	case RETRIEVE_OFFSET_ : 
	  {
	    Int16 flags;
	    Lng32  lobNum;
	    Int64 uid, inDescSyskey, descPartnKey;
	    short schNameLen;
	    char schName[1024];
            Int64 lobOffset = 0;
         
	    ExpLOBoper::extractFromLOBhandle(&flags, &lobType_, &lobNum, &uid,  
					     &inDescSyskey, &descPartnKey, 
					     &schNameLen, (char *)schName,
					     (char *)lobHandle_, (Lng32)lobHandleLen_);


	    lobName_ = ExpLOBoper::ExpGetLOBname(uid, lobNum, lobNameBuf_, 1000);	   
       
            //Retrieve the total length of this lob using the handle info and return to the caller

            retcode = ExpLOBInterfaceGetOffset( lobGlobs,
                                                   lobName_, 
                                                   lobLoc_,
                                                   lobType_,
                                                   lobTdb().getLobHdfsServer(),
                                                   lobTdb().getLobHdfsPort(),
                                                   lobHandleLen_, lobHandle_, 
                                                   lobOffset);
                                                  
           
            if ((lobTdb().getBufAddr() != -1) && (lobTdb().getBufAddr() != 0))
              str_cpy_all((char *)lobTdb().getBufAddr(), (char *)&lobOffset,sizeof(Int64));
            str_sprintf(statusString_," LOB Offset : %ld", lobOffset);
            step_ = RETURN_STATUS_;
            break;	
              
           	      
	  }
	case EXTRACT_LOB_DATA_ :
	  {
	    Int16 flags;
	    Lng32  lobNum;
	    Int64 uid, inDescSyskey, descPartnKey;
	    short schNameLen;
	    char schName[1024];
	    
	    ExpLOBoper::extractFromLOBhandle(&flags, &lobType_, &lobNum, &uid,  
					     &inDescSyskey, &descPartnKey, 
					     &schNameLen, (char *)schName,
					     (char *)lobHandle_, (Lng32)lobHandleLen_);
	    lobName_ = ExpLOBoper::ExpGetLOBname(uid, lobNum, lobNameBuf_, 1000);

	    lobDataSpecifiedExtractLen_ = lobTdb().totalBufSize_; 
	    
            if (lobDataSpecifiedExtractLen_ == 0)
              {
                // Passed in length is 0 indicates the caller is done with 
                // this lobhandle and wants to close this cursor
                step_ = CLOSE_CURSOR_;
                break;
              }
	   
	     
	   
	    // Read the lob contents  into target file
	    

	    if (lobTdb().getToType() == ComTdbExeUtilLobExtract::TO_FILE_)
	      {
		so = Lob_File;
		LobTgtFileFlags tgtFlags = Lob_Error_Or_Create;
		if (lobTdb().errorIfNotExists() && !lobTdb().truncateExisting())
		  tgtFlags = Lob_Append_Or_Error;
		if (lobTdb().truncateExisting() &&lobTdb().errorIfNotExists() )
		  tgtFlags = Lob_Truncate_Or_Error;
		if (lobTdb().truncateExisting() && !lobTdb().errorIfNotExists())
		  tgtFlags = Lob_Truncate_Or_Create;		
		if(lobTdb().appendOrCreate())
		  tgtFlags = Lob_Append_Or_Create;
		retcode = ExpLOBInterfaceSelect(lobGlobs, 
                                                (getStatsEntry() != NULL ? getStatsEntry()->castToExHdfsScanStats() : NULL),
						lobName_,
						lobLoc_,
						lobType_,
						lobTdb().getLobHdfsServer(),
						lobTdb().getLobHdfsPort(),
						lobHandleLen_,
						lobHandle_,
						requestTag,
						so,
						-1,
						0,0,					       					
						0, lobDataLen_, lobDataOutputLen, 
						lobTdb().getFileName(),
						lobDataSpecifiedExtractLen_,
						(Int32)tgtFlags
						);
		if (retcode <0)
		  {
		    Lng32 intParam1 = -retcode;
		    Lng32 cliError;
		    ExRaiseSqlError(getHeap(), &diagsArea_, 
				    (ExeErrorCode)(8442), NULL, &intParam1, 
				    &cliError, NULL, (char*)"ExpLOBInterfaceSelect",
		                    getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
		    step_ = HANDLE_ERROR_;
		    break;
		  }
		str_sprintf(statusString_, "Success. Targetfile :%s  Length : %ld", lobTdb().getFileName(), lobDataOutputLen);
		step_ = RETURN_STATUS_;
	      }
	    else if (lobTdb().getToType() == ComTdbExeUtilLobExtract::TO_BUFFER_)
	      {
		so = Lob_Buffer;
		lobData_ =  (char *)lobTdb().getBufAddr();
		lobDataSpecifiedExtractLen_ = *((Int64 *)(lobTdb().dataExtractSizeIOAddr()));
		step_ = OPEN_CURSOR_;
	      }
	  }
	  break;

	case OPEN_CURSOR_:
	  {
	    retcode = ExpLOBInterfaceSelectCursor
	      (lobGlobs,
               (getStatsEntry() != NULL ? getStatsEntry()->castToExHdfsScanStats() : NULL),
	       lobName_, 
	       lobLoc_,
	       lobType_,
	       lobTdb().getLobHdfsServer(),
	       lobTdb().getLobHdfsPort(),
	       lobHandleLen_, lobHandle_,
               0, // cursor bytes 
               NULL, //cursor id
	       requestTag_, 
	       Lob_Buffer,
	       0, // not check status
	       1, // waited op
	       0, lobDataSpecifiedExtractLen_, 
	       lobDataOutputLen, lobData_, 
	       1, // open
	       2); // must open

	    if (retcode < 0)
	      {
		Lng32 cliError = 0;

		Lng32 intParam1 = -retcode;
		ExRaiseSqlError(getHeap(), &diagsArea_, 
				(ExeErrorCode)(8442), NULL, &intParam1, 
				&cliError, NULL, (char*)"ExpLOBInterfaceSelectCursor",
		                getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
		step_ = HANDLE_ERROR_;
		break;
	      }

	    step_ = READ_CURSOR_;
	  }
	  break;

	case READ_CURSOR_:
	  {
	    if (lobTdb().getToType() == ComTdbExeUtilLobExtract::TO_BUFFER_)
	      so = Lob_Buffer;
	    lobDataSpecifiedExtractLen_ = *((Int64 *)(lobTdb().dataExtractSizeIOAddr()));

            if (lobDataSpecifiedExtractLen_ == 0)
              {
                // Passed in length is 0 indicates the caller is done with 
                // this lobhandle and wants to close this cursor
                step_ = CLOSE_CURSOR_;
                break;
              }
                
	    retcode = ExpLOBInterfaceSelectCursor
	      (lobGlobs,
               (getStatsEntry() != NULL ? getStatsEntry()->castToExHdfsScanStats() : NULL),
	       lobName_, 
	       lobLoc_,
	       lobType_,
	       lobTdb().getLobHdfsServer(),
	       lobTdb().getLobHdfsPort(),
	       lobHandleLen_, lobHandle_,
               0 , //cursor bytes,
	       NULL, //cursor id
	       requestTag_, 
	       so,
	       0, // not check status
	       1, // waited op
	       0, 
	       lobDataSpecifiedExtractLen_, 
	       //lobDataLen_, lobData_, 
	       lobDataOutputLen,
	       lobData_,
	       2, // read
	       0); // open type not applicable

	    if (retcode < 0)
	      {
		Lng32 cliError = 0;

		Lng32 intParam1 = -retcode;
		ExRaiseSqlError(getHeap(), &diagsArea_, 
				(ExeErrorCode)(8442), NULL, &intParam1, 
				&cliError, NULL, (char*)"ExpLOBInterfaceSelectCursor",
		                getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
		step_ = HANDLE_ERROR_;
		break;
	      }

	    if (lobDataOutputLen == 0)
	      {
		step_ = CLOSE_CURSOR_;
		break;
	      }

	    remainingBytes_ = lobDataOutputLen;
	    currPos_ = 0;

            
            if (lobTdb().getToType() == ComTdbExeUtilLobExtract::TO_BUFFER_)
	      {
		str_sprintf(statusString_," Success: LOB data length returned : %ld", lobDataOutputLen);
	       
		//lobTdb().setExtractSizeIOAddr((Int64)(&lobDataOutputLen));
		memcpy((char *)lobTdb().dataExtractSizeIOAddr(), (char *)&lobDataOutputLen,sizeof(Int64));
		step_ = RETURN_STATUS_;
	      }
	    else
	      {
		// No other "toType" shoudl reach here - i.e TO_FILE_ or TO_STRING
		ExRaiseSqlError(getMyHeap(),
				&diagsArea_,
				(ExeErrorCode)(EXE_INTERNAL_ERROR));
		step_ = CANCEL_;
		
		break;
		
	      }
	  }
	  break;

	case CLOSE_CURSOR_:
	  {
	    retcode = ExpLOBInterfaceSelectCursor
	      (lobGlobs,
               (getStatsEntry() != NULL ? getStatsEntry()->castToExHdfsScanStats() : NULL),
	       lobName_, 
	       lobLoc_,
	       lobType_,
	       lobTdb().getLobHdfsServer(),
	       lobTdb().getLobHdfsPort(),

	       lobHandleLen_, lobHandle_,
               0, //cursor bytes
               NULL, //cursor id
	       requestTag_, 
	       so,
	       0, // not check status
	       1, // waited op

	       0, lobDataSpecifiedExtractLen_, 
	       lobDataLen_, lobData_, 
	       3, // close
               0); // open type not applicable

	    if (retcode < 0)
	      {
		Lng32 cliError = 0;

		Lng32 intParam1 = -retcode;
		ExRaiseSqlError(getHeap(), &diagsArea_, 
				(ExeErrorCode)(8442), NULL, &intParam1, 
				&cliError, NULL, (char*)"ExpLOBInterfaceSelectCursor",
		                getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
		step_ = HANDLE_ERROR_;
		break;
	      } 
	    step_ = DONE_;
	  }
	  break;

   
	case RETURN_STATUS_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;
	    //Return to upqueue whatever is in the lobStatusMsg_ data member
       
	    short rc; 
	    moveRowToUpQueue(statusString_, 200, &rc);

	    if ((so == Lob_Buffer) && (remainingBytes_ >= 0))
	      {
		step_ = READ_CURSOR_;
		qparent_.down->removeHead();
		return WORK_RESCHEDULE_AND_RETURN;
	      }
	    else
	      step_ = DONE_ ;
	  }
	  break;
	case HANDLE_ERROR_:
	  {
	    retcode = handleError();
	    if (retcode == 1)
	      return WORK_OK;
	    step_ = DONE_;
	  }
	  break;

	case DONE_:
	  {
	    retcode = handleDone();
	    if (retcode == 1)
	      return WORK_OK;
	    step_ = EMPTY_;
	    return WORK_OK;
	  }
	  break;

	} // switch
    }

  return 0;
}

ExExeUtilFileExtractTcb::ExExeUtilFileExtractTcb
(
 const ComTdbExeUtilLobExtract & exe_util_tdb,
 const ex_tcb * child_tcb, 
 ex_globals * glob)
  : ExExeUtilLobExtractTcb(exe_util_tdb, child_tcb, glob)
{
}



///////////////////////////////////////////////////////////////////
// class ExExeUtilLobUpdateTdb
///////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilLobUpdateTdb::build(ex_globals * glob)
{
  ExExeUtilLobUpdateTcb * exe_util_lobupdate_tcb = NULL;

  ex_tcb * childTcb = NULL;
  if (child_)
    {
      // build the child first
      childTcb = child_->build(glob);
    }
  
  
  if (getFromType() == ComTdbExeUtilLobUpdate::FROM_BUFFER_)
    exe_util_lobupdate_tcb = new(glob->getSpace()) 
      ExExeUtilLobUpdateTcb(*this, childTcb, glob);
  else
    {
      ex_assert(TRUE,"Only buffer input supported");
    }

  exe_util_lobupdate_tcb->registerSubtasks();

  return (exe_util_lobupdate_tcb);
}

ExExeUtilLobUpdateTcb::ExExeUtilLobUpdateTcb
(
 const ComTdbExeUtilLobUpdate & exe_util_lobupdate_tdb,
 const ex_tcb * child_tcb, 
 ex_globals * glob)
  : ExExeUtilTcb(exe_util_lobupdate_tdb, child_tcb, glob),
    step_(EMPTY_)    
{
  ContextCli *currContext =
    getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals()->
    getStatement()->getContext();
  lobHandleLen_ = 2050;
  lobHandle_[0] = '\0';
  exLobGlobals_=NULL;
  memset(lobLockId_,'\0',LOB_LOCK_ID_SIZE);
  exLobGlobals_ = ExpLOBoper::initLOBglobal((NAHeap *)glob->getDefaultHeap(), currContext, exe_util_lobupdate_tdb.useLibHdfs());
                                     
}
ExExeUtilLobUpdateTcb::~ExExeUtilLobUpdateTcb()
{
  freeResources();
}

void ExExeUtilLobUpdateTcb::freeResources()
{
 ExpLOBoper::deleteLOBglobal(exLobGlobals_, (NAHeap *)getGlobals()->getDefaultHeap());
 exLobGlobals_ = NULL;
}

ExOperStats * ExExeUtilLobUpdateTcb::doAllocateStatsEntry(CollHeap *heap,
							    ComTdb *tdb)
{
  ExEspStmtGlobals *espGlobals = getGlobals()->castToExExeStmtGlobals()->castToExEspStmtGlobals();
  StmtStats *ss; 
  if (espGlobals != NULL)
     ss = espGlobals->getStmtStats();
  else
     ss = getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals()->getStatement()->getStmtStats(); 
  ExHdfsScanStats *hdfsScanStats = new(heap) ExHdfsScanStats(heap,
				   this,
				   tdb);
  if (ss != NULL) 
     hdfsScanStats->setQueryId(ss->getQueryId(), ss->getQueryIdLen());
  return hdfsScanStats;
}

short ExExeUtilLobUpdateTcb::work()
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;
  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate =
    *((ExExeUtilPrivateState*) pentry_down->pstate);

  ContextCli *currContext =
    getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals()->
    getStatement()->getContext();

  ComDiagsArea & diags       = currContext->diags();

  LobsSubOper so = Lob_None;
  
  if (lobTdb().getFromType() == ComTdbExeUtilLobUpdate::FROM_STRING_)
    so = Lob_Memory;
  else if (lobTdb().getFromType() == ComTdbExeUtilLobUpdate::FROM_EXTERNAL_)
    so = Lob_External_File;
  else if (lobTdb().getFromType() == ComTdbExeUtilLobUpdate::FROM_BUFFER_)  //Only this is supported
    so= Lob_Buffer;
  
  Int64 lobLen = lobTdb().updateSize();
  char * data = (char *)(lobTdb().getBufAddr());
 
  ExLobGlobals * lobGlobs = getLobGlobals();

  while (1)
    {
      switch (step_)
	{
	case EMPTY_:
	  {    
            workAtp_->getTupp(lobTdb().workAtpIndex())
	      .setDataPointer(lobInputHandleBuf_);  
            step_ = GET_HANDLE_;   
            break;
          }
          break;
        case GET_HANDLE_:
          {
            ex_expr::exp_return_type exprRetCode =
	      lobTdb().inputExpr_->eval(NULL, 
					workAtp_);
	    if (exprRetCode == ex_expr::EXPR_ERROR)
              {	      
                step_ = CANCEL_;
                break;
              }
            if (ExpLOBoper::genLOBhandleFromHandleString
		    (lobTdb().getHandle(),
		     lobTdb().getHandleLen(),
		     lobHandle_,
		     lobHandleLen_))
		  {
		    ExRaiseSqlError(getMyHeap(),
				    &diagsArea_,
				    (ExeErrorCode)(EXE_INVALID_LOB_HANDLE));
		    step_ = HANDLE_ERROR_;
		    break;
		  }
            if (lobTdb().getFromType() == ComTdbExeUtilLobUpdate::FROM_BUFFER_)
              {
                if (lobTdb().isTruncate())
                  step_ = EMPTY_LOB_DATA_;
                else if (lobTdb().isReplace())
                  step_ = UPDATE_LOB_DATA_;
                else if(lobTdb().isAppend())
                  step_ = APPEND_LOB_DATA_;
                
              }
            else
		{
		  // invalid "fromType"
		  ExRaiseSqlError(getMyHeap(),
				  &diagsArea_,
				  (ExeErrorCode)(EXE_INTERNAL_ERROR));
		  step_ = CANCEL_;
		
		break;

		}
            
          }
          break;
        case UPDATE_LOB_DATA_:
          {
            Int32 retcode = 0;
            Int16 flags;
	    Lng32  lobNum;
	    Int64 uid, inDescSyskey, descPartnKey;
	    short schNameLen;
	    char schName[1024];
            Int64 dummy = 0;
	    
	    ExpLOBoper::extractFromLOBhandle(&flags, &lobType_, &lobNum, &uid,  
					     &inDescSyskey, &descPartnKey, 
					     &schNameLen, (char *)schName,
					     (char *)lobHandle_, (Lng32)lobHandleLen_);
	    lobName_ = ExpLOBoper::ExpGetLOBname(uid, lobNum, lobNameBuf_, 1000);

	    lobDataLen_ = lobTdb().totalBufSize_; 
           
            strcpy(lobLoc_, lobTdb().getLobLocation());
	    
            char outLobHandle[LOB_HANDLE_LEN];
            Int32 outHandleLen;
            Int64 requestTag = 0;
            if (lobTdb().lobLocking())
              {
                ExpLOBoper::genLobLockId(uid,lobNum,lobLockId_);
                NABoolean found = FALSE;
                retcode = SQL_EXEC_CheckLobLock(lobLockId_ , &found);
                if (! retcode && !found) 
                  {    
                    retcode = SQL_EXEC_SetLobLock(lobLockId_);
                  }
                else if (found)
                  {
                    memset(lobLockId_,'\0',LOB_LOCK_ID_SIZE);
                    ExRaiseSqlError(getHeap(), &diagsArea_, 
                                    (ExeErrorCode)(EXE_LOB_CONCURRENT_ACCESS_ERROR)); 
                               
                    step_=HANDLE_ERROR_;
                    break;
                  }
              }
            retcode = ExpLOBInterfaceUpdate(lobGlobs,
                                            (getStatsEntry() != NULL ? getStatsEntry()->castToExHdfsScanStats() : NULL),
                                            lobTdb().getLobHdfsServer(),
                                            lobTdb().getLobHdfsPort(),
                                            lobName_,
                                            lobLoc_,
                                            lobHandleLen_,
                                            lobHandle_,
                                            &outHandleLen, outLobHandle,
                                            requestTag,
                                            -1,
                                            0,
                                            1,
                                            so,
                                            inDescSyskey,
                                            lobLen,
                                            data,
                                            lobName_, schNameLen, schName,
                                            dummy, dummy,
                                            lobTdb().getLobMaxSize(),
                                            lobTdb().getLobMaxChunkSize(),
                                            lobTdb().getLobGCLimit());
            
            if (retcode < 0)
	      {
		Lng32 cliError = 0;

		Lng32 intParam1 = -retcode;
		ExRaiseSqlError(getHeap(), &diagsArea_, 
				(ExeErrorCode)(8442), NULL, &intParam1, 
				&cliError, NULL, (char*)"ExpLOBInterfaceUpdate",
		                getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
		step_ = HANDLE_ERROR_;
		break;
	      }  
            if (so == Lob_Buffer)
	      {
		str_sprintf(statusString_," Updated/Replaced %ld bytes of LOB data ", lobLen);
		step_ = RETURN_STATUS_;
	      }
            
            break;
          }
          break;
        case APPEND_LOB_DATA_:
          {
            Int32 retcode = 0;
            Int16 flags;
	    Lng32  lobNum;
	    Int64 uid, inDescSyskey, descPartnKey;
	    short schNameLen;
	    char schName[1024];
            Int64 dummy = 0;
	    
	    ExpLOBoper::extractFromLOBhandle(&flags, &lobType_, &lobNum, &uid,  
					     &inDescSyskey, &descPartnKey, 
					     &schNameLen, (char *)schName,
					     (char *)lobHandle_, (Lng32)lobHandleLen_);
	    lobName_ = ExpLOBoper::ExpGetLOBname(uid, lobNum, lobNameBuf_, 1000);

	    lobDataLen_ = lobTdb().totalBufSize_; 
            strcpy(lobLoc_, lobTdb().getLobLocation());
           
            if (lobTdb().lobLocking())
              {
                ExpLOBoper::genLobLockId(uid,lobNum,lobLockId_);;
                NABoolean found = FALSE;
                retcode = SQL_EXEC_CheckLobLock(lobLockId_, &found);
                if (! retcode && !found) 
                  {    
                    retcode = SQL_EXEC_SetLobLock(lobLockId_);
                  }
                else if (found)
                  {
                    memset(lobLockId_,'\0',LOB_LOCK_ID_SIZE);
                    ExRaiseSqlError(getHeap(), &diagsArea_, 
                                    (ExeErrorCode)(EXE_LOB_CONCURRENT_ACCESS_ERROR));
                    step_=HANDLE_ERROR_;
                    break;
                  }
              }
            char outLobHandle[LOB_HANDLE_LEN];
            Int32 outHandleLen;
            Int64 requestTag = 0;
            retcode = ExpLOBInterfaceUpdateAppend(lobGlobs,
                                            (getStatsEntry() != NULL ? getStatsEntry()->castToExHdfsScanStats() : NULL),
                                            lobTdb().getLobHdfsServer(),
                                            lobTdb().getLobHdfsPort(),
                                            lobName_,
                                            lobLoc_,
                                            lobHandleLen_,
                                            lobHandle_,
                                            &outHandleLen, outLobHandle,
                                            requestTag,
                                            -1,
                                            0,
                                            1,
                                            so,
                                            inDescSyskey,
                                            lobLen,
                                            data,
                                            lobName_, schNameLen, schName,
                                            dummy, dummy,
                                            lobTdb().getLobMaxSize(),
                                            lobTdb().getLobMaxChunkSize(),
                                            lobTdb().getLobGCLimit());
            
            if (retcode < 0)
	      {
		Lng32 cliError = 0;

		Lng32 intParam1 = -retcode;
		ExRaiseSqlError(getHeap(), &diagsArea_, 
				(ExeErrorCode)(8442), NULL, &intParam1, 
				&cliError, NULL, (char*)"ExpLOBInterfaceUpdate",
		                getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
		step_ = HANDLE_ERROR_;
		break;
	      }  
            if (so == Lob_Buffer)
	      {
		str_sprintf(statusString_," Updated/Appended %ld bytes of LOB data ", lobLen);
		step_ = RETURN_STATUS_;
	      }
            
            break;
          }
          break;

        case EMPTY_LOB_DATA_:
          {
            Int32 retcode = 0;
            Int16 flags;
	    Lng32  lobNum;
	    Int64 uid, inDescSyskey, descPartnKey;
	    short schNameLen;
	    char schName[1024];
            Int64 dummy = 0;
	    
	    ExpLOBoper::extractFromLOBhandle(&flags, &lobType_, &lobNum, &uid,  
					     &inDescSyskey, &descPartnKey, 
					     &schNameLen, (char *)schName,
					     (char *)lobHandle_, (Lng32)lobHandleLen_);
	    lobName_ = ExpLOBoper::ExpGetLOBname(uid, lobNum, lobNameBuf_, 1000);

	    lobDataLen_ = lobTdb().totalBufSize_; 
            strcpy(lobLoc_, lobTdb().getLobLocation());

            if (lobTdb().lobLocking())
              {
                ExpLOBoper::genLobLockId(uid,lobNum,lobLockId_);;
                NABoolean found = FALSE;
                retcode = SQL_EXEC_CheckLobLock(lobLockId_, &found);
                if (! retcode && !found) 
                  {    
                    retcode = SQL_EXEC_SetLobLock(lobLockId_);
                  }
                else if (found || retcode )
                  {
                    memset(lobLockId_,'\0',LOB_LOCK_ID_SIZE);
                    ExRaiseSqlError(getHeap(), &diagsArea_, 
                                    (ExeErrorCode)(EXE_LOB_CONCURRENT_ACCESS_ERROR));
                    step_=HANDLE_ERROR_;
                    break;
                  }
              }
            char outLobHandle[LOB_HANDLE_LEN];
            Int32 outHandleLen;
            Int64 requestTag = 0;
          
            retcode = ExpLOBInterfaceUpdate(lobGlobs,
                                            (getStatsEntry() != NULL ? getStatsEntry()->castToExHdfsScanStats() : NULL),
                                            lobTdb().getLobHdfsServer(),
                                            lobTdb().getLobHdfsPort(),
                                            lobName_,
                                            lobLoc_,
                                            lobHandleLen_,
                                            lobHandle_,
                                            &outHandleLen, outLobHandle,
                                            requestTag,
                                            -1,
                                            0,
                                            1,
                                            so,
                                            inDescSyskey,
                                            0,
                                            NULL,
                                            lobName_, schNameLen, schName,
                                            dummy, dummy,
                                            lobTdb().getLobMaxSize(),
                                            lobTdb().getLobMaxChunkSize(),
                                            lobTdb().getLobGCLimit());
            
            if (retcode < 0)
	      {
		Lng32 cliError = 0;

		Lng32 intParam1 = -retcode;
		ExRaiseSqlError(getHeap(), &diagsArea_, 
				(ExeErrorCode)(8442), NULL, &intParam1, 
				&cliError, NULL, (char*)"ExpLOBInterfaceUpdate",
		                getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
		step_ = HANDLE_ERROR_;
		break;
	      }  
            if (so == Lob_Buffer)
	      {
		str_sprintf(statusString_," Updated with empty_blob/clob  ");
		step_ = RETURN_STATUS_;
	      }
            
            break;
          }
          break;
        case CANCEL_:
          {
            break;
          }
          break;
        case RETURN_STATUS_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;
	    //Return to upqueue whatever is in the lobStatusMsg_ data member
      	    short rc; 
	    moveRowToUpQueue(statusString_, 200, &rc);	   
            step_ = DONE_ ;
	  }
	  break;
        case HANDLE_ERROR_:
          {
            retcode = handleError();

	    if (retcode == 1)
              {
                if (lobLockId_[0] && lobTdb().lobLocking())
                  retcode = SQL_EXEC_ReleaseLobLock(lobLockId_);
                return WORK_OK;
              }
	    step_ = DONE_;
            
          }
          break;
        case DONE_:
          {
            retcode = handleDone();
            if(lobLockId_[0] && lobTdb().lobLocking())
              retcode = SQL_EXEC_ReleaseLobLock(lobLockId_);
	    if (retcode == 1)
	      return WORK_OK;

	    step_ = EMPTY_;
	    return WORK_OK;
          }
          break;
        }
    }
  return 0;
}

NABoolean ExExeUtilFileExtractTcb::needStatsEntry()
{
  // stats are collected for ALL and OPERATOR options.
  if ((getGlobals()->getStatsArea()->getCollectStatsType() == 
       ComTdb::ALL_STATS) ||
      (getGlobals()->getStatsArea()->getCollectStatsType() == 
      ComTdb::OPERATOR_STATS))
    return TRUE;
  else
    return FALSE;
}

ExOperStats * ExExeUtilFileExtractTcb::doAllocateStatsEntry(CollHeap *heap,
							    ComTdb *tdb)
{
  ExEspStmtGlobals *espGlobals = getGlobals()->castToExExeStmtGlobals()->castToExEspStmtGlobals();
  StmtStats *ss; 
  if (espGlobals != NULL)
     ss = espGlobals->getStmtStats();
  else
     ss = getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals()->getStatement()->getStmtStats(); 
  ExHdfsScanStats *hdfsScanStats = new(heap) ExHdfsScanStats(heap,
				   this,
				   tdb);
  if (ss != NULL) 
     hdfsScanStats->setQueryId(ss->getQueryId(), ss->getQueryIdLen());
  return hdfsScanStats;
}



short ExExeUtilFileExtractTcb::work()
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate =
    *((ExExeUtilPrivateState*) pentry_down->pstate);

  ContextCli *currContext =
    getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals()->
    getStatement()->getContext();

  ComDiagsArea & diags       = currContext->diags();

  

  ExLobGlobals * lobGlobs = getLobGlobals();

  while (1)
    {
      switch (step_)
	{
	case EMPTY_:
	  {
	    lobName_ = lobNameBuf_;
	    strcpy(lobName_, lobTdb().getFileName());

	    strcpy(lobLoc_, lobTdb().getStringParam2());

	    lobType_ =  lobTdb().lobStorageType_; //(Lng32)Lob_External_HDFS_File;

	    lobDataSpecifiedExtractLen_ = lobTdb().totalBufSize_; 
	   

	    // allocate 2 buffers for double buffering.
	    lobData_ = new(getHeap()) char[(UInt32)lobDataSpecifiedExtractLen_];
	    lobData2_ = new(getHeap()) char[(UInt32)lobDataSpecifiedExtractLen_];

	    eodReturned_ = FALSE;

	    step_ = OPEN_CURSOR_;
	  }
	  break;
	  
	case OPEN_CURSOR_:
	  {
	    eodReturned_ = FALSE;

	    retcode = ExpLOBInterfaceSelectCursor
	      (lobGlobs,
               (getStatsEntry() != NULL ? getStatsEntry()->castToExHdfsScanStats() : NULL),
	       lobName_, 
	       lobLoc_,
	       lobType_,
	       lobTdb().getLobHdfsServer(),
	       lobTdb().getLobHdfsPort(),

	       0, NULL, // handleLen, handle
               0, NULL, //cursor bytes, cursor id
	       requestTag_, 
	       Lob_File,
	       0, // not check status
	       1, // waited op

	       0, lobDataSpecifiedExtractLen_, 
	       lobDataLen_, lobData_, 
	       1, // open
	       2); // must open

	    if (retcode < 0)
	      {
		Lng32 cliError = 0;

		Lng32 intParam1 = -retcode;
		ExRaiseSqlError(getHeap(), &diagsArea_, 
				(ExeErrorCode)(8442), NULL, &intParam1, 
				&cliError, NULL, (char*)"ExpLOBInterfaceSelectCursor/open",
		                getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
		step_ = HANDLE_ERROR_;
		break;
	      }

	    step_ = READ_CURSOR_;
	  }
	  break;

	case READ_CURSOR_:
	  {
	    if (eodReturned_)
	      {
		// eod was previously returned. close the cursor.
		step_ = CLOSE_CURSOR_;
		break;
	      }

	    retcode = ExpLOBInterfaceSelectCursor
	      (lobGlobs,
               (getStatsEntry() != NULL ? getStatsEntry()->castToExHdfsScanStats() : NULL),
	       lobName_, 
	       lobLoc_,
	       lobType_,
	       lobTdb().getLobHdfsServer(),
	       lobTdb().getLobHdfsPort(),

	       0, NULL,
               0, NULL ,//cursor bytes, cursor id
	       requestTag_, 
	       Lob_File,
	       0, // not check status
	       1, // waited op

	       0, lobDataSpecifiedExtractLen_, 
	       lobDataLen_, lobData_, 
	       2, // read
	       0); // open type not applicable

	    if (retcode < 0)
	      {
		Lng32 cliError = 0;

		Lng32 intParam1 = -retcode;
		ExRaiseSqlError(getHeap(), &diagsArea_, 
				(ExeErrorCode)(8442), NULL, &intParam1, 
				&cliError, NULL, (char*)"ExpLOBInterfaceSelectCursor/read",
		                getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
		step_ = HANDLE_ERROR_;
		break;
	      }

	    if (lobDataLen_ == 0)
	      {
		// EOD with no data: close cursor
		eodReturned_ = TRUE;

		step_ = CLOSE_CURSOR_;
		break;
	      }

	    if (lobDataLen_ < lobDataSpecifiedExtractLen_)
	      {
		// EOD with data: return data and then close cursor
		eodReturned_ = TRUE;
	      }

	    remainingBytes_ = (Lng32)lobDataLen_;
	    currPos_ = 0;

	    step_ = RETURN_STRING_;
	  }
	  break;

	case CLOSE_CURSOR_:
	  {
	    retcode = ExpLOBInterfaceSelectCursor
	      (lobGlobs,
               (getStatsEntry() != NULL ? getStatsEntry()->castToExHdfsScanStats() : NULL),
	       lobName_, 
	       lobLoc_,
	       lobType_,
	       lobTdb().getLobHdfsServer(),
	       lobTdb().getLobHdfsPort(),

	       0, NULL,
               0, NULL, //cursor bytes, cursor id
	       requestTag_, 
	       Lob_File,
	       0, // not check status
	       1, // waited op
	       0, lobDataSpecifiedExtractLen_, 
	       lobDataLen_, lobData_, 
	       3, // close
               0); // open type not applicable
	    if (retcode < 0)
	      {
		Lng32 cliError = 0;

		Lng32 intParam1 = -retcode;
		ExRaiseSqlError(getHeap(), &diagsArea_, 
				(ExeErrorCode)(8442), NULL, &intParam1, 
				&cliError, NULL, (char*)"ExpLOBInterfaceSelectCursor/close",
		                getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
		step_ = HANDLE_ERROR_;
		break;
	      }

	    step_ = DONE_;
	  }
	  break;

	case HANDLE_ERROR_:
	  {
	    retcode = handleError();
	    if (retcode == 1)
	      return WORK_OK;

	    step_ = DONE_;
	  }
	  break;

	case DONE_:
	  {
	    retcode = handleDone();
	    if (retcode == 1)
	      return WORK_OK;

	    step_ = EMPTY_;
	    return WORK_OK;
	  }
	  break;

	} // switch
    }

  return 0;
}
ExExeUtilFileLoadTcb::ExExeUtilFileLoadTcb
(
 const ComTdbExeUtilLobExtract & exe_util_tdb,
 const ex_tcb * child_tcb, 
 ex_globals * glob)
  : ExExeUtilLobExtractTcb(exe_util_tdb, child_tcb, glob)
{
}

ExOperStats * ExExeUtilFileLoadTcb::doAllocateStatsEntry(CollHeap *heap,
							    ComTdb *tdb)
{
  ExEspStmtGlobals *espGlobals = getGlobals()->castToExExeStmtGlobals()->castToExEspStmtGlobals();
  StmtStats *ss; 
  if (espGlobals != NULL)
     ss = espGlobals->getStmtStats();
  else
     ss = getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals()->getStatement()->getStmtStats(); 
  ExHdfsScanStats *hdfsScanStats = new(heap) ExHdfsScanStats(heap,
				   this,
				   tdb);
  if (ss != NULL) 
     hdfsScanStats->setQueryId(ss->getQueryId(), ss->getQueryIdLen());
  return hdfsScanStats;
}

short ExExeUtilFileLoadTcb::work()
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate =
    *((ExExeUtilPrivateState*) pentry_down->pstate);

  ContextCli *currContext =
    getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals()->
    getStatement()->getContext();

  ComDiagsArea & diags       = currContext->diags();

  

  ExLobGlobals * lobGlobs = getLobGlobals();

  while (1)
    {
      switch (step_)
	{
	case EMPTY_:
	  {
	    lobName_ = lobNameBuf_;
	    strcpy(lobName_, lobTdb().getStringParam2());

	    strcpy(lobLoc_, lobTdb().getStringParam3());

	    lobType_ =  lobTdb().lobStorageType_; //(Lng32)Lob_HDFS_File;

	    lobDataSpecifiedExtractLen_ = lobTdb().totalBufSize_; 
	    

	    lobData_ = new(getHeap()) char[(UInt32)lobDataSpecifiedExtractLen_];

	    srcFileRemainingBytes_ = 0;

	    step_ = CREATE_TARGET_FILE_;
	  }
	  break;

	case CREATE_TARGET_FILE_:
	  {
	    if (lobTdb().withCreate())
	      {
		retcode = ExpLOBinterfaceCreate
		  (lobGlobs,
		   lobName_, 
		   lobLoc_,
		   lobType_,
		   lobTdb().getLobHdfsServer(),
		   lobTdb().getLobHdfsPort());

		if (retcode < 0)
		  {
		    Lng32 cliError = 0;
		    
		    Lng32 intParam1 = -retcode;
		    ExRaiseSqlError(getHeap(), &diagsArea_, 
				    (ExeErrorCode)(8442), NULL, &intParam1, 
				    &cliError, NULL, (char*)"ExpLOBInterfaceCreate",
		                    getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
		    step_ = HANDLE_ERROR_;
		    break;
		  }
	      }
	    if (lobTdb().getToType() == ComTdbExeUtilLobExtract::TO_EXTERNAL_FROM_STRING_)
	      {
		strcpy(lobData_, lobTdb().getStringParam1());
		lobDataLen_ = strlen(lobData_);
		step_ = INSERT_FROM_STRING_;
	      }
	    else
	      step_ = INSERT_FROM_SOURCE_FILE_;
	  }
	  break;

	case INSERT_FROM_SOURCE_FILE_:
	  {
	    char fname[400];
	    str_sprintf(fname, "%s", lobTdb().getStringParam1());

	    indata_.open(fname, fstream::in | fstream::binary);
	    if (! indata_)
	      {
		Lng32 cliError = 0;

		Lng32 intParam1 = -1;
		ExRaiseSqlError(getHeap(), &diagsArea_, 
				(ExeErrorCode)(8442), NULL, &intParam1, 
				&cliError, NULL, (char*)"SourceFile open");
		step_ = HANDLE_ERROR_;
		break;
	      }

	    indata_.seekg (0, indata_.end);
	    srcFileRemainingBytes_ = indata_.tellg();
	    indata_.seekg (0, indata_.beg);

	    step_ = READ_STRING_FROM_SOURCE_FILE_;
	  }
	  break;

	case READ_STRING_FROM_SOURCE_FILE_:
	  {
	    if (! indata_.good())
	      {
		indata_.close();
		step_ = CLOSE_TARGET_FILE_;
		break;
	      }

	    if (srcFileRemainingBytes_ == 0)
	      {
		indata_.close();
		step_ = CLOSE_TARGET_FILE_;
		break;
	      }
	      
	    Int64 length = MINOF(srcFileRemainingBytes_, lobDataSpecifiedExtractLen_);

	    indata_.read (lobData_, (std::streamsize)length);
	      
	    if (indata_.fail())
	      {
		indata_.close();

		Lng32 cliError = 0;

		Lng32 intParam1 = -1;
		ExRaiseSqlError(getHeap(), &diagsArea_, 
				(ExeErrorCode)(8442), NULL, &intParam1, 
				&cliError, NULL, (char*)"SourceFile read");
		step_ = HANDLE_ERROR_;
		break;
	      }
	    
	    lobDataLen_ = length;
	    srcFileRemainingBytes_ -= length;

	    step_ = INSERT_FROM_STRING_;
	  }
	  break;

	case INSERT_FROM_STRING_:
	  {
	    Int64 requestTag;
	    Int64 dummy;
	    retcode = ExpLOBInterfaceInsert
	      (lobGlobs,
               (getStatsEntry() != NULL ? getStatsEntry()->castToExHdfsScanStats() : NULL),
	       lobName_, 
	       lobLoc_,
	       lobType_,
	       lobTdb().getLobHdfsServer(),
	       lobTdb().getLobHdfsPort(),

	       0, NULL, NULL, NULL, 0, NULL,

	       requestTag, 
	       0, // no xn id
	       dummy,Lob_InsertDataSimple,

	       NULL, Lob_Memory,
	       1, // waited

	       lobData_, lobDataLen_
	       );

	    if (retcode < 0)
	      {
		Lng32 cliError = 0;

		Lng32 intParam1 = -retcode;
		ExRaiseSqlError(getHeap(), &diagsArea_, 
				(ExeErrorCode)(8442), NULL, &intParam1, 
				&cliError, NULL, (char*)"ExpLOBInterfaceInsert",
		                getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
		step_ = HANDLE_ERROR_;
		break;
	      }

	    if (lobTdb().getToType() == ComTdbExeUtilLobExtract::TO_EXTERNAL_FROM_FILE_)
	      step_ = READ_STRING_FROM_SOURCE_FILE_;
	    else
	      step_ = CLOSE_TARGET_FILE_;
	  }
	  break;

	case CLOSE_TARGET_FILE_:
	  {
	    retcode = ExpLOBinterfaceCloseFile
	      (lobGlobs,
               (getStatsEntry() != NULL ? getStatsEntry()->castToExHdfsScanStats() : NULL),
	       lobName_, 
	       lobLoc_,
	       lobType_,
	       lobTdb().getLobHdfsServer(),
	       lobTdb().getLobHdfsPort());

	    if (retcode < 0)
	      {
		Lng32 cliError = 0;
		
		Lng32 intParam1 = -retcode;
		ExRaiseSqlError(getHeap(), &diagsArea_, 
				(ExeErrorCode)(8442), NULL, &intParam1, 
				&cliError, NULL, (char*)"ExpLOBInterfaceCloseFile",
		                getLobErrStr(intParam1), (char*)getSqlJniErrorStr());
		step_ = HANDLE_ERROR_;
		break;
	      }
	    
	    step_ = DONE_;
	  }
	  break;

	case HANDLE_ERROR_:
	  {
	    retcode = handleError();
	    if (retcode == 1)
	      return WORK_OK;

	    step_ = DONE_;
	  }
	  break;

	case DONE_:
	  {
	    retcode = handleDone();
	    if (retcode == 1)
	      return WORK_OK;

	    step_ = EMPTY_;
	    return WORK_OK;
	  }
	  break;

	} // switch
    }

  return 0;
}

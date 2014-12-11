/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2014 Hewlett-Packard Development Company, L.P.
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
#include  "ExpLOBinterface.h"
#include  "ExpLOBexternal.h"
#include  "str.h"


////////////////////////////////////////////////////////////////
// Constructor for class ExExeLoadUtilTcb
///////////////////////////////////////////////////////////////
ExExeLoadUtilTcb::ExExeLoadUtilTcb(const ComTdbExeUtil & exe_util_tdb,
				   ex_globals * glob)
     : ExExeUtilTcb( exe_util_tdb, NULL, glob)
{
  // Allocate the private state in each entry of the down queue
  qparent_.down->allocatePstate(this);
}

////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ExExeLoadUtilTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExExeLoadUtilPrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExExeLoadUtilPrivateState::ExExeLoadUtilPrivateState()
{
}

ExExeLoadUtilPrivateState::~ExExeLoadUtilPrivateState()
{
};

//////////////////////////////////////////////////////
// work() for ExExeLoadUtilTcb
//////////////////////////////////////////////////////
short ExExeLoadUtilTcb::work()
{
  
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
  Int32 savedIsoMapping = 
    currContext->getSessionDefaults()->getIsoMappingEnum();
  
  Lng32 cliRC;
  short retcode = 0;
  char * stmt = NULL;

  // Table is loaded in 3 steps:
  //  -- target table is made unaudited
  //  -- sidetree insert is done
  //  -- target table is made audited

  // make the table unaudited
  cliRC = changeAuditAttribute(exeUtilTdb().getTableName(), FALSE);
  if (cliRC < 0)
    {
      ExHandleErrors(qparent_,
		     pentry_down,
		     0,
		     getGlobals(),
		     NULL,
		     (ExeErrorCode)cliRC,
		     NULL,
		     exeUtilTdb().getTableName()
		     );
      goto endOfData;
    }

  // now issue the insert statement
  stmt = exeUtilTdb().getQuery();
  Int64 rowsAffected;

  // All internal queries issued from CliInterface assume that
  // they are in ISO_MAPPING.
  // That causes mxcmp to use the default charset as iso88591
  // for unprefixed literals.
  // The insert...select being issued out here contains the user
  // specified query and any literals in that should be using
  // the default_charset.
  // So we send the isoMapping charset instead of the
  // enum ISO_MAPPING.
  cliInterface()->setIsoMapping
    (currContext->getSessionDefaults()->getIsoMappingEnum());
  cliRC = cliInterface()->executeImmediate(stmt, NULL, NULL, TRUE, &rowsAffected,TRUE);
  cliInterface()->setIsoMapping(savedIsoMapping);
  if (cliRC < 0)
    {
      changeAuditAttribute(exeUtilTdb().getTableName(), TRUE);

      retcode = (short)cliInterface()->retrieveSQLDiagnostics(NULL);
		
      ExHandleErrors(qparent_,
		     pentry_down,
		     0,
		     getGlobals(),
		     NULL,
		     (ExeErrorCode)cliRC,
		     NULL,
		     exeUtilTdb().getTableName()
		     );
      goto endOfData;
    }

  masterGlob->setRowsAffected(rowsAffected);

  // and make the table audited
  cliRC = changeAuditAttribute(exeUtilTdb().getTableName(), TRUE);
  if (cliRC < 0)
    {
      ExHandleErrors(qparent_,
		     pentry_down,
		     0,
		     getGlobals(),
		     NULL,
		     (ExeErrorCode)cliRC,
		     NULL,
		     exeUtilTdb().getTableName()
		     );
      goto endOfData;
    }

endOfData:

  // Return EOF.
  ex_queue_entry * up_entry = qparent_.up->getTailEntry();
  
  up_entry->upState.parentIndex = 
    pentry_down->downState.parentIndex;
  
  up_entry->upState.setMatchNo(0);
  up_entry->upState.status = ex_queue::Q_NO_DATA;
  
  // insert into parent
  qparent_.up->insert();
  
  pstate.matches_ = 0;
  pstate.step_ = EMPTY_;
  qparent_.down->removeHead();
  
  return WORK_OK;
}

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
  
  ExTransaction *ta = getGlobals()->castToExExeStmtGlobals()->
    castToExMasterStmtGlobals()->getStatement()->getContext()->getTransaction();
  
  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
	    NABoolean xnAlreadyStarted = ta->xnInProgress();

	    if (xnAlreadyStarted)
              {
                *getDiagsArea() << DgSqlCode(-20123)
                                << DgString0("This DDL operation");
                
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
		if (((cliRC == -1055) || // SQ table err msg
		     (cliRC == -1390)) && // Traf err msg
		    (ctaTdb().loadIfExists()))
		  {
		    SQL_EXEC_ClearDiagnostics(NULL);
		    tableExists_ = TRUE;

		    if (ctaTdb().deleteData())
		      step_ = DELETE_DATA_;
		    else
		      step_ = ALTER_TO_NOAUDIT_;
		    break;
		  }
		else
		  {
		    cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
		    
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

	case DELETE_DATA_:
	case DELETE_DATA_AND_ERROR_:
	  {
	    char * ddQuery = 
	      new(getMyHeap()) char[strlen("DELETE DATA FROM; ") + 
				   strlen(ctaTdb().getTableName()) +
				   100];
	    strcpy(ddQuery, "DELETE DATA FROM ");
	    strcat(ddQuery, ctaTdb().getTableName());
	    strcat(ddQuery, ";");
	    cliRC = cliInterface()->executeImmediate(ddQuery, NULL,NULL,TRUE,NULL,TRUE);
	    // Delete new'd characters
	    NADELETEBASIC(ddQuery, getHeap());
	    ddQuery = NULL;

	    if (cliRC < 0)
	      {
		if (step_ == DELETE_DATA_)
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

	    if (step_ == DELETE_DATA_AND_ERROR_)
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
              cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
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
                cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
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
              cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
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
		cliInterface()->retrieveSQLDiagnostics(getDiagsArea());

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
		step_ = DELETE_DATA_AND_ERROR_;
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
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    // Return EOF.
	    ex_queue_entry * up_entry = qparent_.up->getTailEntry();
	    
	    up_entry->upState.parentIndex = 
	      pentry_down->downState.parentIndex;
	    
	    up_entry->upState.setMatchNo(0);
	    up_entry->upState.status = ex_queue::Q_NO_DATA;
	    
	    // insert into parent
	    qparent_.up->insert();
	    
	    step_ = INITIAL_;
	    qparent_.down->removeHead();
	    
	    return WORK_OK;
	  }
	break;

	case ERROR_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    // Return EOF.
	    ex_queue_entry * up_entry = qparent_.up->getTailEntry();
	    
	    up_entry->upState.parentIndex = 
	      pentry_down->downState.parentIndex;
	    
	    up_entry->upState.setMatchNo(0);
	    up_entry->upState.status = ex_queue::Q_SQLERROR;

	    ComDiagsArea *diagsArea = up_entry->getDiagsArea();
	    
	    if (diagsArea == NULL)
	      diagsArea = 
		ComDiagsArea::allocate(this->getGlobals()->getDefaultHeap());
            else
              diagsArea->incrRefCount (); // setDiagsArea call below will decr ref count
	    
	    if (getDiagsArea())
	      diagsArea->mergeAfter(*getDiagsArea());
	    
	    up_entry->setDiagsArea (diagsArea);
	    
	    // insert into parent
	    qparent_.up->insert();
	    
	    pstate.matches_ = 0;

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
                                    query_, dummyArg, &len, NULL);

  #ifdef VERIFY_CLI_UTIL
        ex_assert (len == 0, "lock table returned data");
        for (size_t i = 0; i < sizeof(dummyArg); i++)
          ex_assert( dummyArg[i] == i, "lock table returned data");
  #endif

        NADELETEBASIC(query_, getMyHeap());

        if (cliRC < 0)
        {
          cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
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
                                  &len, NULL);
        NADELETEBASIC(query_, getMyHeap());
        if (cliRC < 0)
        {
          cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
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
                                    query_, dummyArg, &len, NULL);

        NADELETEBASIC(query_, getMyHeap());

        if (cliRC < 0)
        {
        // mjh - tbd - warning or EMS message to give context to error on
        // delete after error on the insert?
          cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
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
       rowsAffected_(0)
{
  qparent_.down->allocatePstate(this);

}

//////////////////////////////////////////////////////
// work() for ExExeUtilHbaseLoadTcb
//////////////////////////////////////////////////////
short ExExeUtilHBaseBulkLoadTcb::work()
{
  Lng32 cliRC = 0;
  short retcode = 0;
  short rc;

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate = *((ExExeUtilPrivateState*) pentry_down->pstate);

  ExTransaction *ta = getGlobals()->castToExExeStmtGlobals()->
    castToExMasterStmtGlobals()->getStatement()->getContext()->getTransaction();

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
          ComDiagsArea * da = getDiagsArea();
          *da << DgSqlCode(-8111);
          step_ = LOAD_ERROR_;
        }

        if (setStartStatusMsgAndMoveToUpQueue("LOAD", &rc))
          return rc;

        if (hblTdb().getUpsertUsingLoad())
          hblTdb().setPreloadCleanup(FALSE);

        if (hblTdb().getTruncateTable())
        {
          step_ = TRUNCATE_TABLE_;
          break;
        }
        step_ = LOAD_START_;
      }
        break;

      case TRUNCATE_TABLE_:
      {
        if (setStartStatusMsgAndMoveToUpQueue(" PURGE DATA",&rc))
          return rc;

        // Set the parserflag to prevent privilege checks in purgedata
        ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
        ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
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
          cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
          step_ = LOAD_ERROR_;
          break;
        }
        step_ = LOAD_START_;

        setEndStatusMsg(" PURGE DATA");
      }
        break;

      case LOAD_START_:
      {
        if (holdAndSetCQD("COMP_BOOL_226", "ON") < 0)
        {
          step_ = LOAD_END_ERROR_;
          break;
        }
        if (hblTdb().getForceCIF())
        {
          if (holdAndSetCQD("COMPRESSED_INTERNAL_FORMAT", "ON") < 0)
          {
            step_ = LOAD_END_ERROR_;
            break;
          }
          if (holdAndSetCQD("COMPRESSED_INTERNAL_FORMAT_BMO", "ON") < 0)
          {
            step_ = LOAD_END_ERROR_;
            break;
          }
          if (holdAndSetCQD("COMPRESSED_INTERNAL_FORMAT_DEFRAG_RATIO", "100") < 0)
          {
            step_ = LOAD_END_ERROR_;
            break;
          }
        }

        if (hblTdb().getPreloadCleanup())
          step_ = PRE_LOAD_CLEANUP_;
        else
        {
          step_ = PREPARATION_;
          if (hblTdb().getIndexes())
            step_ = DISABLE_INDEXES_;
        }
      }
      break;

      case PRE_LOAD_CLEANUP_:
      {
        if (setStartStatusMsgAndMoveToUpQueue(" CLEANUP", &rc))
          return rc;

        //Cleanup files
        char * clnpQuery =
          new(getMyHeap()) char[strlen("LOAD CLEANUP FOR TABLE  ; ") +
                               strlen(hblTdb().getTableName()) +
                               100];
        strcpy(clnpQuery, "LOAD CLEANUP FOR TABLE  ");
        strcat(clnpQuery, hblTdb().getTableName());
        strcat(clnpQuery, ";");

        cliRC = cliInterface()->executeImmediate(clnpQuery, NULL,NULL,TRUE,NULL,TRUE);

        NADELETEBASIC(clnpQuery, getHeap());
        clnpQuery = NULL;
        if (cliRC < 0)
        {
          cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
          step_ = LOAD_END_ERROR_;
          break;
        }

        step_ = PREPARATION_;

        if (hblTdb().getIndexes())
          step_ = DISABLE_INDEXES_;

        setEndStatusMsg(" CLEANUP");
      }
        break;
      case DISABLE_INDEXES_:
      {
         if (setStartStatusMsgAndMoveToUpQueue(" DISABLE INDEXES", &rc))
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
        strcat(diQuery, " DISABLE ALL INDEXES ;");
        cliRC = cliInterface()->executeImmediate(diQuery, NULL,NULL,TRUE,NULL,TRUE);

        NADELETEBASIC(diQuery, getMyHeap());
        diQuery = NULL;
        if (cliRC < 0)
        {
          cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
          step_ = LOAD_END_ERROR_;
          break;
        }
        step_ = PREPARATION_;

        setEndStatusMsg(" DISABLE INDEXES");
      }
        break;

      case PREPARATION_:
      {
        if (!hblTdb().getUpsertUsingLoad())
        {
          if (setStartStatusMsgAndMoveToUpQueue(" PREPARATION", &rc, 0, TRUE))
            return rc;

          if (hblTdb().getNoDuplicates())
            cliRC = holdAndSetCQD("TRAF_LOAD_PREP_SKIP_DUPLICATES", "OFF");
          else
            cliRC = holdAndSetCQD("TRAF_LOAD_PREP_SKIP_DUPLICATES", "ON");
          if (cliRC < 0)
          {
          step_ = LOAD_END_ERROR_;
            break;
          }

          rowsAffected_ = 0;
          char * transQuery =hblTdb().ldQuery_;
          cliRC = cliInterface()->executeImmediate(transQuery,
                                                   NULL,
                                                   NULL,
                                                   TRUE,
                                                   &rowsAffected_);
          transQuery = NULL;
          if (cliRC < 0)
          {
            rowsAffected_ = 0;
            cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
            step_ = LOAD_END_ERROR_;
            break;
          }

          step_ = COMPLETE_BULK_LOAD_;
        if (rowsAffected_ == 0)
          step_ = LOAD_END_;

          sprintf(statusMsgBuf_,"       Rows Processed: %ld %c",rowsAffected_, '\n' );
          int len = strlen(statusMsgBuf_);
          setEndStatusMsg(" PREPARATION", len, TRUE);
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
            cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
            step_ = LOAD_ERROR_;
            break;
          }

          step_ = DONE_;

           if (hblTdb().getIndexes())
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
        strcat(clQuery, hblTdb().getTableName());
        strcat(clQuery, ";");

        cliRC = cliInterface()->executeImmediate(clQuery, NULL,NULL,TRUE,NULL,TRUE);

        NADELETEBASIC(clQuery, getMyHeap());
        clQuery = NULL;

        if (cliRC < 0)
        {
          rowsAffected_ = 0;  
                              
          cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
          step_ = LOAD_END_ERROR_;
          break;
        }
        cliRC = restoreCQD("TRAF_LOAD_TAKE_SNAPSHOT");
         if (cliRC < 0)
         {
           step_ = LOAD_END_ERROR_;
           break;
         }
        step_ = LOAD_END_;

        if (hblTdb().getIndexes())
          step_ = POPULATE_INDEXES_;

        setEndStatusMsg(" COMPLETION", 0, TRUE);
      }
        break;

      case POPULATE_INDEXES_:
      {
        if (setStartStatusMsgAndMoveToUpQueue(" POPULATE INDEXES", &rc))
          return rc;

        char * piQuery =
           new(getMyHeap()) char[strlen("POPULATE ALL INDEXES ON  ; ") +
                                strlen(hblTdb().getTableName()) +
                                100];
         strcpy(piQuery, "POPULATE ALL INDEXES ON   ");
         strcat(piQuery, hblTdb().getTableName());
         strcat(piQuery, ";");

         cliRC = cliInterface()->executeImmediate(piQuery, NULL,NULL,TRUE,NULL,TRUE);

         NADELETEBASIC(piQuery, getHeap());
         piQuery = NULL;

         if (cliRC < 0)
         {
           cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
           step_ = LOAD_END_ERROR_;
           break;
         }

        step_ = LOAD_END_;

        setEndStatusMsg(" POPULATE INDEXES", 0, TRUE);
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
        cliRC = restoreCQD("COMP_BOOL_226");
         if (cliRC < 0)
         {
           step_ = LOAD_ERROR_;
           break;
         }
         cliRC = restoreCQD("TRAF_LOAD_PREP_SKIP_DUPLICATES");
         if (cliRC < 0)
         {
           step_ = LOAD_ERROR_;
           break;
         }
         if (hblTdb().getForceCIF())
         {
           if (restoreCQD("COMPRESSED_INTERNAL_FORMAT") < 0)
           {
             step_ = LOAD_ERROR_;
             break;
           }
           if (restoreCQD("COMPRESSED_INTERNAL_FORMAT_BMO") < 0)
           {
             step_ = LOAD_ERROR_;
             break;
           }
           if (restoreCQD("COMPRESSED_INTERNAL_FORMAT_DEFRAG_RATIO") < 0)
           {
             step_ = LOAD_ERROR_;
             break;
           }
         }
         if (step_ == LOAD_END_)
          step_ = DONE_;
         else
           step_ = LOAD_ERROR_;
      }
      break;

      case DONE_:
      {
        if (qparent_.up->isFull())
          return WORK_OK;

        // Return EOF.
        ex_queue_entry * up_entry = qparent_.up->getTailEntry();

        up_entry->upState.parentIndex = pentry_down->downState.parentIndex;

        up_entry->upState.setMatchNo(0);
        up_entry->upState.status = ex_queue::Q_NO_DATA;

        ComDiagsArea *diagsArea = up_entry->getDiagsArea();

        if (diagsArea == NULL)
          diagsArea = ComDiagsArea::allocate(getMyHeap());
        else
          diagsArea->incrRefCount(); // setDiagsArea call below will decr ref count

        diagsArea->setRowCount(rowsAffected_);

        if (getDiagsArea())
          diagsArea->mergeAfter(*getDiagsArea());

        up_entry->setDiagsArea(diagsArea);

        // insert into parent
        qparent_.up->insert();
        step_ = INITIAL_;
        qparent_.down->removeHead();
        return WORK_OK;
      }
        break;

      case LOAD_ERROR_:
      {
        if (qparent_.up->isFull())
          return WORK_OK;

        // Return EOF.
        ex_queue_entry * up_entry = qparent_.up->getTailEntry();

        up_entry->upState.parentIndex = pentry_down->downState.parentIndex;

        up_entry->upState.setMatchNo(0);
        up_entry->upState.status = ex_queue::Q_SQLERROR;

        ComDiagsArea *diagsArea = up_entry->getDiagsArea();

        if (diagsArea == NULL)
          diagsArea = ComDiagsArea::allocate(getMyHeap());
        else
          diagsArea->incrRefCount(); // setDiagsArea call below will decr ref count

        if (getDiagsArea())
        {
          diagsArea->mergeAfter(*getDiagsArea());
          diagsArea->setRowCount(rowsAffected_);
        }

        up_entry->setDiagsArea(diagsArea);

        // insert into parent
        qparent_.up->insert();

        pstate.matches_ = 0;



        step_ = DONE_;
      }
        break;

    } // switch
  } // while

  return WORK_OK;

}

short ExExeUtilHBaseBulkLoadTcb::moveRowToUpQueue(const char * row, Lng32 len,
                                                  short * rc, NABoolean isVarchar)
{
  if (hblTdb().getNoOutput())
    return 0;

  return ExExeUtilTcb::moveRowToUpQueue(row, len, rc, isVarchar);
}




short ExExeUtilHBaseBulkLoadTcb::setStartStatusMsgAndMoveToUpQueue(const char * operation,
                                     short * rc,
                                     int bufPos,
                                     NABoolean   withtime)
{

  if (hblTdb().getNoOutput())
     return 0;

  if (withtime)
    startTime_ = NA_JulianTimestamp();

  getStatusString(operation, "Started",hblTdb().getTableName(), &statusMsgBuf_[bufPos]);
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

  if (withtime)
  {
    endTime_ = NA_JulianTimestamp();
    Int64 elapsedTime = endTime_ - startTime_;

    getTimeAsString(elapsedTime, timeBuf);
  }

  getStatusString(operation, "Ended", hblTdb().getTableName(),&statusMsgBuf_[bufPos], withtime ? timeBuf : NULL);

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
void ExExeUtilHBaseBulkUnLoadTcb::createHdfsFileError(Int32 sfwRetCode)
{
  ComDiagsArea * diagsArea = NULL;
  char* errorMsg = sequenceFileWriter_->getErrorText((SFW_RetCode)sfwRetCode);
  ExRaiseSqlError(getHeap(), &diagsArea, (ExeErrorCode)(8447), NULL,
                  NULL, NULL, NULL, errorMsg, NULL);
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
       rowsAffected_(0)
{
  sequenceFileWriter_ = NULL;
  qparent_.down->allocatePstate(this);

}

//////////////////////////////////////////////////////
// work() for ExExeUtilHbaseLoadTcb
//////////////////////////////////////////////////////
short ExExeUtilHBaseBulkUnLoadTcb::work()
{
  Lng32 cliRC = 0;
  short retcode = 0;
  short rc;
  SFW_RetCode sfwRetCode = SFW_OK;

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate = *((ExExeUtilPrivateState*) pentry_down->pstate);

  ExTransaction *ta = getGlobals()->castToExExeStmtGlobals()->
    castToExMasterStmtGlobals()->getStatement()->getContext()->getTransaction();

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
          ComDiagsArea * da = getDiagsArea();
          *da << DgSqlCode(-8111);
          step_ = UNLOAD_ERROR_;
        }
        if (!sequenceFileWriter_)
        {
          sequenceFileWriter_ = new(getSpace())
                       SequenceFileWriter((NAHeap *)getSpace());
          sfwRetCode = sequenceFileWriter_->init();
          if (sfwRetCode != SFW_OK)
            {
             createHdfsFileError(sfwRetCode);
             step_ = UNLOAD_END_ERROR_;
            break;
            }
        }
        if (!hblTdb().getOverwriteMergeFile() &&  hblTdb().getMergePath() != NULL)
        {
          NABoolean exists = FALSE;
          sfwRetCode = sequenceFileWriter_->hdfsExists( hblTdb().getMergePath(), exists);
          if (sfwRetCode != SFW_OK)
          {
            createHdfsFileError(sfwRetCode);
            step_ = UNLOAD_END_ERROR_;
            break;
          }
          if (exists)
          {
            //EXE_UNLOAD_FILE_EXISTS
            ComDiagsArea * da = getDiagsArea();
            *da << DgSqlCode(- EXE_UNLOAD_FILE_EXISTS)
                  << DgString0(hblTdb().getMergePath());
            step_ = UNLOAD_END_ERROR_;
           break;
          }
        }
        if (holdAndSetCQD("COMP_BOOL_226", "ON") < 0)
        {
          step_ = UNLOAD_END_ERROR_;
          break;
        }
        if (holdAndSetCQD("TRAF_UNLOAD_BYPASS_LIBHDFS", "ON") < 0)
        {
          step_ = UNLOAD_END_ERROR_;
          break;
        }
        if (hblTdb().getSkipWriteToFiles())
        {
          hblTdb().setEmptyTarget(FALSE);
          hblTdb().setOneFile(FALSE);
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
        step_ = UNLOAD_;
        if (hblTdb().getEmptyTarget())
          step_ = EMPTY_TARGET_;
      }
        break;
      case EMPTY_TARGET_:
       {

         if (setStartStatusMsgAndMoveToUpQueue(" EMPTY TARGET ", &rc, 0, TRUE))
           return rc;

         std::string uldPath = std::string( hblTdb().getExtractLocation());

         sfwRetCode = sequenceFileWriter_->hdfsCleanUnloadPath( uldPath);
         if (sfwRetCode != SFW_OK)
           {
            createHdfsFileError(sfwRetCode);
            step_ = UNLOAD_END_ERROR_;
           break;
           }
         step_ = UNLOAD_;

         setEndStatusMsg(" EMPTY TARGET ", 0, TRUE);
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
          cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
          step_ = UNLOAD_END_ERROR_;
          break;
        }
        step_ = UNLOAD_END_;

        if (hblTdb().getOneFile())
          step_ = MERGE_FILES_;

        if (hblTdb().getSkipWriteToFiles())
          sprintf(statusMsgBuf_,"       Rows Processed but NOT Written to Disk: %ld %c",rowsAffected_, '\n' );
        else
          sprintf(statusMsgBuf_,"       Rows Processed: %ld %c",rowsAffected_, '\n' );
        int len = strlen(statusMsgBuf_);
        setEndStatusMsg(" EXTRACT ", len, TRUE);

      }
        break;

      case MERGE_FILES_:
      {
        if (setStartStatusMsgAndMoveToUpQueue(" MERGE FILES ", &rc, 0, TRUE))
          return rc;

        std::string srcPath = std::string( hblTdb().getExtractLocation());
        std::string dstPath = std::string( hblTdb().getMergePath());
        sfwRetCode = sequenceFileWriter_->hdfsMergeFiles( srcPath, dstPath);
        if (sfwRetCode != SFW_OK)
          {
           createHdfsFileError(sfwRetCode);
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
//        if (restoreCQD("HDFS_IO_BUFFERSIZE") < 0)
//        {
//          step_ = UNLOAD_ERROR_;
//          break;
//        }
        if (restoreCQD("COMP_BOOL_226") < 0)
        {
          step_ = UNLOAD_ERROR_;
          break;
        }
        if (restoreCQD("TRAF_UNLOAD_BYPASS_LIBHDFS") < 0)
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
        if (qparent_.up->isFull())
          return WORK_OK;

        // Return EOF.
        ex_queue_entry * up_entry = qparent_.up->getTailEntry();

        up_entry->upState.parentIndex = pentry_down->downState.parentIndex;

        up_entry->upState.setMatchNo(0);
        up_entry->upState.status = ex_queue::Q_NO_DATA;

        ComDiagsArea *diagsArea = up_entry->getDiagsArea();

        if (diagsArea == NULL)
          diagsArea = ComDiagsArea::allocate(getMyHeap());
        else
          diagsArea->incrRefCount(); // setDiagsArea call below will decr ref count

        diagsArea->setRowCount(rowsAffected_);

        if (getDiagsArea())
          diagsArea->mergeAfter(*getDiagsArea());

        up_entry->setDiagsArea(diagsArea);

        // insert into parent
        qparent_.up->insert();
        step_ = INITIAL_;
        qparent_.down->removeHead();
        return WORK_OK;
      }
        break;

      case UNLOAD_ERROR_:
      {
        if (qparent_.up->isFull())
          return WORK_OK;

        // Return EOF.
        ex_queue_entry * up_entry = qparent_.up->getTailEntry();

        up_entry->upState.parentIndex = pentry_down->downState.parentIndex;

        up_entry->upState.setMatchNo(0);
        up_entry->upState.status = ex_queue::Q_SQLERROR;

        ComDiagsArea *diagsArea = up_entry->getDiagsArea();

        if (diagsArea == NULL)
          diagsArea = ComDiagsArea::allocate(getMyHeap());
        else
          diagsArea->incrRefCount(); // setDiagsArea call below will decr ref count

        if (getDiagsArea())
          diagsArea->mergeAfter(*getDiagsArea());

        up_entry->setDiagsArea(diagsArea);

        // insert into parent
        qparent_.up->insert();

        pstate.matches_ = 0;



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

  if (withtime)
    startTime_ = NA_JulianTimestamp();

  getStatusString(operation, "Started",NULL, &statusMsgBuf_[bufPos]);
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

    getTimeAsString(elapsedTime, timeBuf);
  }

  getStatusString(operation, "Ended", NULL,&statusMsgBuf_[bufPos], withtime ? timeBuf : NULL);

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
}

short ExExeUtilLobExtractTcb::work()
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

  if (! currContext->currLobGlobals())
    {
      currContext->currLobGlobals() = 
	new(currContext->exHeap()) LOBglobals(currContext->exHeap());
      ExpLOBoper::initLOBglobal
	(currContext->currLobGlobals()->lobAccessGlobals(), 
	 currContext->exHeap());
    }

  void * lobGlobs = currContext->currLobGlobals()->lobAccessGlobals();

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
		      ComDiagsArea * diagsArea = getDiagsArea();
		      ExRaiseSqlError(getHeap(), &diagsArea, 
				      (ExeErrorCode)(8444), NULL, &intParam1, 
				      &cliError, NULL, NULL);

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
		  ComDiagsArea * da = up_entry->getDiagsArea();
		  ExRaiseSqlError(getMyHeap(),
				  &da,
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
	    Int16 flags;
	    Lng32  lobNum;
	    Int64 uid, inDescSyskey, descPartnKey;
	    short schNameLen;
	    char schName[1024];
	    
	    if (lobTdb().handleInStringFormat())
	      {
		// lobInputHandleBuf_ is in varchar format.
		// Null terminate it.
		short VClen;
		str_cpy_all((char*)&VClen, (char*)lobInputHandleBuf_, sizeof(short));
		lobInputHandleBuf_[sizeof(short)+VClen]=0;

		lobHandleLen_ = 2048;
		if (ExpLOBoper::genLOBhandleFromHandleString
		    (&lobInputHandleBuf_[sizeof(short)], //lobTdb().getHandle(),
		     strlen(&lobInputHandleBuf_[sizeof(short)]), //strlen(lobTdb().getHandle()),
		     lobHandleBuf_,
		     lobHandleLen_))
		  {
		    ComDiagsArea * da = getDiagsArea();
		    ExRaiseSqlError(getMyHeap(),
				    &da,
				    (ExeErrorCode)(EXE_INVALID_LOB_HANDLE));
		    step_ = HANDLE_ERROR_;
		    break;
		  }

		lobHandle_ = lobHandleBuf_;
	      }
	    else
	      {
		lobHandle_ = &lobInputHandleBuf_[sizeof(short)+sizeof(short)]; //lobTdb().getHandle();
		lobHandleLen_ = *(short*)&lobInputHandleBuf_[sizeof(short)]; //lobTdb().getHandleLen();

		if (*(short*)lobInputHandleBuf_ != 0) //null value
		  {
		    step_ = DONE_;
		    break;
		  }

	      }

	    ExpLOBoper::extractFromLOBhandle(&flags, &lobType_, &lobNum, &uid,  
					     &inDescSyskey, &descPartnKey, 
					     &schNameLen, (char *)schName,
					     (char *)lobHandle_, (Lng32)lobHandleLen_);
	    
	    lobName_ = 
	      ExpLOBoper::ExpGetLOBname(uid, lobNum, lobNameBuf_, 1000);

	    lobDataMaxLen_ = lobTdb().size2_; //100000;
	    if (lobDataMaxLen_ == 0)
	      lobDataMaxLen_ = 100000;

	    lobData_ = new(getHeap()) char[(UInt32)lobDataMaxLen_];

	    short *lobNumList = new (getHeap()) short[1];
	    short *lobTypList = new (getHeap()) short[1];
	    char  **lobLocList = new (getHeap()) char*[1];
	    lobLocList[0] = new (getHeap()) char[1024];
	    
	    Lng32 numLobs = lobNum;
	    Lng32 cliRC = SQL_EXEC_LOBddlInterface
	      (
	       schName,
	       schNameLen,
	       uid,
	       numLobs,
	       LOB_CLI_SELECT_UNIQUE,
	       lobNumList,
	       lobTypList,
	       lobLocList);
	    if (cliRC < 0)
	      {
		getDiagsArea()->mergeAfter(diags);

		step_ = HANDLE_ERROR_;
		break;
	      }

	    strcpy(lobLoc_, lobLocList[0]);

           if (lobTdb().getToType() == ComTdbExeUtilLobExtract::TO_FILE_)
             step_ = OPEN_TARGET_FILE_;
           else
             step_ = OPEN_CURSOR_;
	  }
	  break;

        case OPEN_TARGET_FILE_:
          {
	    char fname[400];
	    str_sprintf(fname, "%s", lobTdb().getFileName());

	    indata_.open(fname, fstream::out | fstream::binary);
	    if (! indata_)
	      {
		Lng32 cliError = 0;

		Lng32 intParam1 = -1;
		ComDiagsArea * diagsArea = getDiagsArea();
		ExRaiseSqlError(getHeap(), &diagsArea, 
				(ExeErrorCode)(8442), NULL, &intParam1, 
				&cliError, NULL, (char*)"SourceFile open");
		step_ = HANDLE_ERROR_;
		break;
	      }

            //	    indata_.seekg (0, indata_.end);
	    indata_.seekg (0, indata_.beg);

            step_ = OPEN_CURSOR_;
          }
          break;

	case OPEN_CURSOR_:
	  {
	    retcode = ExpLOBInterfaceSelectCursor
	      (lobGlobs,
	       lobName_, 
	       lobLoc_,
	       lobType_,
	       lobTdb().getLobHdfsServer(),
	       lobTdb().getLobHdfsPort(),

	       lobHandleLen_, lobHandle_,
               0, // cursor bytes 
               NULL, //cursor id
	       requestTag_, 
	       0, // not check status
	       1, // waited op

	       0, lobDataMaxLen_, 
	       lobDataLen_, lobData_, 
	       1 // open
	       );

	    if (retcode < 0)
	      {
		Lng32 cliError = 0;

		Lng32 intParam1 = -retcode;
		ComDiagsArea * diagsArea = getDiagsArea();
		ExRaiseSqlError(getHeap(), &diagsArea, 
				(ExeErrorCode)(8442), NULL, &intParam1, 
				&cliError, NULL, (char*)"ExpLOBInterfaceSelectCursor",
				getLobErrStr(intParam1));
		step_ = HANDLE_ERROR_;
		break;
	      }

	    step_ = READ_CURSOR_;
	  }
	  break;

	case READ_CURSOR_:
	  {
	    retcode = ExpLOBInterfaceSelectCursor
	      (lobGlobs,
	       lobName_, 
	       lobLoc_,
	       lobType_,
	       lobTdb().getLobHdfsServer(),
	       lobTdb().getLobHdfsPort(),

	       lobHandleLen_, lobHandle_,
               0 , //cursor bytes,
	       NULL, //cursor id
	       requestTag_, 
	       0, // not check status
	       1, // waited op

	       0, lobDataMaxLen_, 
	       lobDataLen_, lobData_, 
	       2 // read
	       );

	    if (retcode < 0)
	      {
		Lng32 cliError = 0;

		Lng32 intParam1 = -retcode;
		ComDiagsArea * diagsArea = getDiagsArea();
		ExRaiseSqlError(getHeap(), &diagsArea, 
				(ExeErrorCode)(8442), NULL, &intParam1, 
				&cliError, NULL, (char*)"ExpLOBInterfaceSelectCursor",
				getLobErrStr(intParam1));
		step_ = HANDLE_ERROR_;
		break;
	      }

	    if (lobDataLen_ == 0)
	      {
		step_ = CLOSE_CURSOR_;
		break;
	      }

	    remainingBytes_ = (Lng32)lobDataLen_;
	    currPos_ = 0;

            if (lobTdb().getToType() == ComTdbExeUtilLobExtract::TO_FILE_)
              step_ = INSERT_FROM_STRING_;
            else
              step_ = RETURN_STRING_;
	  }
	  break;

	case CLOSE_CURSOR_:
	  {
	    retcode = ExpLOBInterfaceSelectCursor
	      (lobGlobs,
	       lobName_, 
	       lobLoc_,
	       lobType_,
	       lobTdb().getLobHdfsServer(),
	       lobTdb().getLobHdfsPort(),

	       lobHandleLen_, lobHandle_,
               0, //cursor bytes
               NULL, //cursor id
	       requestTag_, 
	       0, // not check status
	       1, // waited op

	       0, lobDataMaxLen_, 
	       lobDataLen_, lobData_, 
	       3); // close

	    if (retcode < 0)
	      {
		Lng32 cliError = 0;

		Lng32 intParam1 = -retcode;
		ComDiagsArea * diagsArea = getDiagsArea();
		ExRaiseSqlError(getHeap(), &diagsArea, 
				(ExeErrorCode)(8442), NULL, &intParam1, 
				&cliError, NULL, (char*)"ExpLOBInterfaceSelectCursor",
				getLobErrStr(intParam1));
		step_ = HANDLE_ERROR_;
		break;
	      }

	    step_ = DONE_;
	  }
	  break;


	case RETURN_STRING_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    Lng32 size = MINOF((Lng32)lobTdb().size_, (Lng32)remainingBytes_);

	    moveRowToUpQueue(&lobData_[currPos_], size);

	    remainingBytes_ -= size;
	    currPos_ += size;

	    if (remainingBytes_ <= 0)
	      step_ = READ_CURSOR_;

	    return WORK_RESCHEDULE_AND_RETURN;
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

ExOperStats * ExExeUtilFileExtractTcb::doAllocateStatsEntry(
							    CollHeap *heap,
							    ComTdb *tdb)
{
  ExOperStats * stat = NULL;
  ComTdb::CollectStatsType statsType = 
    getGlobals()->getStatsArea()->getCollectStatsType();
  if (statsType == ComTdb::OPERATOR_STATS)
    {
      return ex_tcb::doAllocateStatsEntry(heap, tdb);
    }
  else
    {
      return new(heap) ExHdfsScanStats(heap,
					       this,
					       tdb);
    }
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

  if (! currContext->currLobGlobals())
    {
      currContext->currLobGlobals() = 
	new(currContext->exHeap()) LOBglobals(currContext->exHeap());
      ExpLOBoper::initLOBglobal
	(currContext->currLobGlobals()->lobAccessGlobals(), 
	 currContext->exHeap());
    }

  void * lobGlobs = currContext->currLobGlobals()->lobAccessGlobals();

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

	    lobDataMaxLen_ = lobTdb().size2_; 
	    if (lobDataMaxLen_ == 0)
	      lobDataMaxLen_ = 100000; // default 100K

	    // allocate 2 buffers for double buffering.
	    lobData_ = new(getHeap()) char[(UInt32)lobDataMaxLen_];
	    lobData2_ = new(getHeap()) char[(UInt32)lobDataMaxLen_];

	    eodReturned_ = FALSE;

	    step_ = OPEN_CURSOR_;
	  }
	  break;
	  
	case OPEN_CURSOR_:
	  {
	    eodReturned_ = FALSE;

	    retcode = ExpLOBInterfaceSelectCursor
	      (lobGlobs,
	       lobName_, 
	       lobLoc_,
	       lobType_,
	       lobTdb().getLobHdfsServer(),
	       lobTdb().getLobHdfsPort(),

	       0, NULL, // handleLen, handle
               0, NULL, //cursor bytes, cursor id
	       requestTag_, 
	       0, // not check status
	       1, // waited op

	       0, lobDataMaxLen_, 
	       lobDataLen_, lobData_, 
	       1 // open
	       );

	    if (retcode < 0)
	      {
		Lng32 cliError = 0;

		Lng32 intParam1 = -retcode;
		ComDiagsArea * diagsArea = getDiagsArea();
		ExRaiseSqlError(getHeap(), &diagsArea, 
				(ExeErrorCode)(8442), NULL, &intParam1, 
				&cliError, NULL, (char*)"ExpLOBInterfaceSelectCursor/open",
				getLobErrStr(intParam1));
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
	       lobName_, 
	       lobLoc_,
	       lobType_,
	       lobTdb().getLobHdfsServer(),
	       lobTdb().getLobHdfsPort(),

	       0, NULL,
               0, NULL ,//cursor bytes, cursor id
	       requestTag_, 
	       0, // not check status
	       1, // waited op

	       0, lobDataMaxLen_, 
	       lobDataLen_, lobData_, 
	       2 // read
	       );

	    if (retcode < 0)
	      {
		Lng32 cliError = 0;

		Lng32 intParam1 = -retcode;
		ComDiagsArea * diagsArea = getDiagsArea();
		ExRaiseSqlError(getHeap(), &diagsArea, 
				(ExeErrorCode)(8442), NULL, &intParam1, 
				&cliError, NULL, (char*)"ExpLOBInterfaceSelectCursor/read",
				getLobErrStr(intParam1));
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

	    if (lobDataLen_ < lobDataMaxLen_)
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
	       lobName_, 
	       lobLoc_,
	       lobType_,
	       lobTdb().getLobHdfsServer(),
	       lobTdb().getLobHdfsPort(),

	       0, NULL,
               0, NULL, //cursor bytes, cursor id
	       requestTag_, 
	       0, // not check status
	       1, // waited op

	       0, lobDataMaxLen_, 
	       lobDataLen_, lobData_, 
	       3); // close

	    if (retcode < 0)
	      {
		Lng32 cliError = 0;

		Lng32 intParam1 = -retcode;
		ComDiagsArea * diagsArea = getDiagsArea();
		ExRaiseSqlError(getHeap(), &diagsArea, 
				(ExeErrorCode)(8442), NULL, &intParam1, 
				&cliError, NULL, (char*)"ExpLOBInterfaceSelectCursor/close",
				getLobErrStr(intParam1));
		step_ = HANDLE_ERROR_;
		break;
	      }

	    step_ = COLLECT_STATS_;
	  }
	  break;


	case RETURN_STRING_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    Lng32 size = MINOF((Lng32)lobTdb().size_, (Lng32)remainingBytes_);

	    // eval expression to convert lob data to sql row.
	    // TBD.

	    moveRowToUpQueue(&lobData_[currPos_], size);

	    remainingBytes_ -= size;
	    currPos_ += size;

	    if (remainingBytes_ <= 0)
	      step_ = READ_CURSOR_;

	    return WORK_RESCHEDULE_AND_RETURN;
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

	case COLLECT_STATS_:
	  {
	    if (! getStatsEntry())
	      {
		step_ = DONE_;
		break;
	      }

	    ExHdfsScanStats * stats =
	      getStatsEntry()->castToExHdfsScanStats();

	    retcode = ExpLOBinterfaceStats
	      (lobGlobs,
	       stats->lobStats(),
	       lobName_, 
	       lobLoc_,
	       lobType_,
	       lobTdb().getLobHdfsServer(),
	       lobTdb().getLobHdfsPort());
	    
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

  if (! currContext->currLobGlobals())
    {
      currContext->currLobGlobals() = 
	new(currContext->exHeap()) LOBglobals(currContext->exHeap());
      ExpLOBoper::initLOBglobal
	(currContext->currLobGlobals()->lobAccessGlobals(), 
	 currContext->exHeap());
    }

  void * lobGlobs = currContext->currLobGlobals()->lobAccessGlobals();

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

	    lobDataMaxLen_ = lobTdb().size2_; //100000;
	    if (lobDataMaxLen_ == 0)
	      lobDataMaxLen_ = 100000;

	    lobData_ = new(getHeap()) char[(UInt32)lobDataMaxLen_];

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
		    ComDiagsArea * diagsArea = getDiagsArea();
		    ExRaiseSqlError(getHeap(), &diagsArea, 
				    (ExeErrorCode)(8442), NULL, &intParam1, 
				    &cliError, NULL, (char*)"ExpLOBInterfaceCreate",
				    getLobErrStr(intParam1));
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
		ComDiagsArea * diagsArea = getDiagsArea();
		ExRaiseSqlError(getHeap(), &diagsArea, 
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
	      
	    Int64 length = MINOF(srcFileRemainingBytes_, lobDataMaxLen_);

	    indata_.read (lobData_, (std::streamsize)length);
	      
	    if (indata_.fail())
	      {
		indata_.close();

		Lng32 cliError = 0;

		Lng32 intParam1 = -1;
		ComDiagsArea * diagsArea = getDiagsArea();
		ExRaiseSqlError(getHeap(), &diagsArea, 
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
	       lobName_, 
	       lobLoc_,
	       lobType_,
	       lobTdb().getLobHdfsServer(),
	       lobTdb().getLobHdfsPort(),

	       0, NULL, NULL, NULL, 0, NULL,

	       requestTag, 
	       0, // no xn id
	       dummy,

	       NULL, Lob_Memory,
	       0, // not check status
	       1, // waited

	       lobData_, lobDataLen_
	       );

	    if (retcode < 0)
	      {
		Lng32 cliError = 0;

		Lng32 intParam1 = -retcode;
		ComDiagsArea * diagsArea = getDiagsArea();
		ExRaiseSqlError(getHeap(), &diagsArea, 
				(ExeErrorCode)(8442), NULL, &intParam1, 
				&cliError, NULL, (char*)"ExpLOBInterfaceInsert",
				getLobErrStr(intParam1));
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
	       lobName_, 
	       lobLoc_,
	       lobType_,
	       lobTdb().getLobHdfsServer(),
	       lobTdb().getLobHdfsPort());

	    if (retcode < 0)
	      {
		Lng32 cliError = 0;
		
		Lng32 intParam1 = -retcode;
		ComDiagsArea * diagsArea = getDiagsArea();
		ExRaiseSqlError(getHeap(), &diagsArea, 
				(ExeErrorCode)(8442), NULL, &intParam1, 
				&cliError, NULL, (char*)"ExpLOBInterfaceCloseFile",
				getLobErrStr(intParam1));
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

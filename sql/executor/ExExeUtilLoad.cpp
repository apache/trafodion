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
  /*
  stmt = new(getHeap()) char[exeUtilTdb().getQueryLen() + exeUtilTdb().getObjectNameLen() + 200];
  strcpy(stmt, "insert using sideinserts into ");
  strcat(stmt, exeUtilTdb().getTableName());
  strcat(stmt, " ");
  strcat(stmt, exeUtilTdb().getQuery());
  */
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

	    cliRC = changeAuditAttribute(ctaTdb().getTableName(),
					 FALSE, ctaTdb().isVolatile());
	    if (cliRC < 0)
	      {
		cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
		
		step_ = HANDLE_ERROR_;
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
	    cliRC = changeAuditAttribute(ctaTdb().getTableName(),
					 TRUE, ctaTdb().isVolatile());
	    if (cliRC < 0)
	      {
		cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
		
		step_ = HANDLE_ERROR_;
		break;
	      }

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


///////////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilUserLoadTdb::build(ex_globals * glob)
{
  // build the child first
  ex_tcb * childTcb = child_->build(glob);

  ExExeUtilUserLoadTcb * exe_util_tcb;

  if (doFastLoad())
    exe_util_tcb = 
      new(glob->getSpace()) ExExeUtilUserLoadFastTcb(*this, childTcb, glob);
  else
    exe_util_tcb = 
      new(glob->getSpace()) ExExeUtilUserLoadTcb(*this, childTcb, glob);
  
  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilUserLoadTcb
///////////////////////////////////////////////////////////////
ExExeUtilUserLoadTcb::ExExeUtilUserLoadTcb(
     const ComTdbExeUtilUserLoad & exe_util_tdb,
     const ex_tcb * child_tcb, // for child queue
     ex_globals * glob)
     : ExExeUtilTcb( exe_util_tdb, child_tcb, glob)
{
  // Allocate the private state in each entry of the down queue
  qparent_.down->allocatePstate(this);

  step_ = INITIAL_;

  eod_ = FALSE;
  firstReqSeen_ = FALSE;

}

ExExeUtilUserLoadTcb::~ExExeUtilUserLoadTcb()
{
}

Int32 ExExeUtilUserLoadTcb::fixup()
{
  if (ulTdb().noAlter())
    return ex_tcb::fixup();
  else
    {
      // fixup will be driven after the table has been altered
      // to unaudited.
      
      return 0;
    }
}

short ExExeUtilUserLoadTcb::checkForEOD()
{
  // already at EOD, nothing need to be done.
  if (isEOD())
    return 0;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();

  ex_expr::exp_return_type retCode;

  Lng32 eodIndicator = 0;
  if (ulTdb().inputExpr())
    {
      workAtp_->getTupp(ulTdb().workAtpIndex()).
	setDataPointer((char *)&eodIndicator);

      retCode = ulTdb().inputExpr()->eval(pentry_down->getAtp(), workAtp_);
      if (retCode == ex_expr::EXPR_ERROR)
	{
	  return -1; // error
	}
    }

  if (eodIndicator == 0)
    {
      eod_ = TRUE;
    }
  else
    {
      eod_ = FALSE;
    }

  return 0;
}

//////////////////////////////////////////////////////
// work() for ExExeUtilUserLoadTcb
//////////////////////////////////////////////////////
short ExExeUtilUserLoadTcb::work()
{
  short rc = 0;
  Lng32 cliRC = 0;
 
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
  const ex_queue::down_request & request = pentry_down->downState.request;

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();

  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
            ExTransaction *ta = getGlobals()->castToExExeStmtGlobals()->
                 castToExMasterStmtGlobals()->getStatement()->getContext()->getTransaction();
	    NABoolean xnAlreadyStarted = ta->xnInProgress();
	    if (xnAlreadyStarted)
	      {
		ExHandleErrors(qparent_,
			       pentry_down,
			       0,
			       getGlobals(),
			       NULL,
			       (ExeErrorCode)(-20123),
			       NULL,
			       "USER LOAD"
			       );
		step_ = ERROR_;
		break;
	      }

	    eod_ = FALSE;
	    firstReqSeen_ = FALSE;

	    if (ulTdb().noAlter())
	      step_ = SEND_REQ_TO_CHILD_;
	    else
	      step_ = ALTER_TO_NOAUDIT_;
	  }
	break;

	case ALTER_TO_NOAUDIT_:
	  {
	    cliRC = changeAuditAttribute(ulTdb().getTableName(),
					 FALSE);
	    if (cliRC < 0)
	      {
		cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
		
		step_ = ERROR_;
		break;
	      }

	    step_ = RE_FIXUP_;
	  }
	break;

	case RE_FIXUP_:
	case RE_FIXUP_ON_ERROR_:
	  {
	    // the alter statement would have blown away the opens.
	    // Do fixup so tables could be opened again.
	    cliRC = ((ex_tcb*)getChild(0))->fixup();
	    if (cliRC < 0)
	      {
		ExHandleErrors(qparent_,
			       pentry_down,
			       0,
			       getGlobals(),
			       NULL,
			       (ExeErrorCode)(cliRC),
			       NULL,
			       ulTdb().getTableName()
			       );
		step_ = ALTER_TO_AUDIT_AND_ALL_DONE_;
		break;
	      }

	    if (step_ == RE_FIXUP_ON_ERROR_)
	      step_ = ALTER_TO_AUDIT_AND_ALL_DONE_;
	    else
	      step_ = SEND_REQ_TO_CHILD_;
	  }
	break;

	case SEND_REQ_TO_CHILD_:
	  {
	    if (qchild_.down->isFull())
	      return WORK_OK;

	    ex_queue_entry * centry = qchild_.down->getTailEntry();

	    rc = checkForEOD();
	    if (rc)
	      {
		step_ = ALTER_TO_AUDIT_;
		break;
	      }

	    if (NOT firstReqSeen_)
	      {
		if (isEOD())
		  {
		    step_ = ALTER_TO_AUDIT_;
		    break;
		  }
		firstReqSeen_ = TRUE;
	      }

	    if (isEOD())
	      centry->downState.request = ex_queue::GET_EOD;
	    else
	      centry->downState.request = ex_queue::GET_ALL;
	    centry->downState.requestValue = 
	      pentry_down->downState.requestValue;
	    centry->downState.parentIndex = qparent_.down->getHeadIndex();
	    
	    // set the child's input atp
	    centry->passAtp(pentry_down->getAtp());
	    
	    qchild_.down->insert();

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

	    ex_queue_entry * centry = qchild_.up->getHeadEntry();
	    
	    ex_queue::up_status child_status = centry->upState.status;
	    switch(child_status)
	      {
	      case ex_queue::Q_OK_MMORE:
		{ 
		ex_assert(0,"ExExeUtilUserLoadTcb::work() not expecting a row from child");

		  qchild_.up->removeHead();	    
		}
	      break;
	      
	      case ex_queue::Q_NO_DATA:
		{
		  qchild_.up->removeHead();	    
		  if (isEOD())
		    {
		      step_ = ALTER_TO_AUDIT_AND_ALL_DONE_;
		    }
		  else
		    step_ = DONE_;
		}
	      break;

	      case ex_queue::Q_SQLERROR:
		{
		  step_ = ERROR_FROM_CHILD_;
		}
	      break;

	      case ex_queue::Q_INVALID:
		ex_assert(0,"ExExeUtilUserLoadTcb::work() Invalid state returned by child");
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
            //pentry_up->upState.setMatchNo(pstate->matchCount_);

            qparent_.up->insert();
	    qchild_.up->removeHead();	    

	    //	    step_ = ALTER_TO_AUDIT_AND_ALL_DONE_;
	    step_ = CANCEL_;
	  }
	break;

	case ALTER_TO_AUDIT_:
	case ALTER_TO_AUDIT_AND_ALL_DONE_:
	  {
	    if (NOT ulTdb().noAlter())
	      {
		cliRC = changeAuditAttribute(ulTdb().getTableName(),
					     TRUE);
		if (cliRC < 0)
		  {
		    cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
		    
		    if (step_ == ALTER_TO_AUDIT_)
		      step_ = ERROR_;
		    else
		      step_ = ALL_DONE_;
		    break;
		  }
	      }

	    step_ = ALL_DONE_;
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
	    
	    if (getDiagsArea())
	      diagsArea->mergeAfter(*getDiagsArea());
	    
	    up_entry->setDiagsArea (diagsArea);
	    
	    // insert into parent
	    qparent_.up->insert();
	    
	    //	    pstate.matches_ = 0;

	    step_ = ALL_DONE_;
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
		{
		  qchild_.up->removeHead();
		}
	      break;
	      
	      case ex_queue::Q_NO_DATA:
		{
		  qchild_.up->removeHead();

		  step_ = RE_FIXUP_ON_ERROR_;
		  //		  step_ = ALTER_TO_AUDIT_AND_ALL_DONE_;
		}
	      break;
	      
	      case ex_queue::Q_INVALID: 
		{
		  ex_assert(0, "ExExeUtilUserLoadTcb::work() Invalid state returned by child");
		}; break;
	      }
	  }
	break;
	
	case DONE_:
	case ALL_DONE_:
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
	    
	    //	    pstate.matches_ = 0;
	    if (step_ == ALL_DONE_)
	      {
		if (firstReqSeen_)
		  step_ = REALLY_ALL_DONE_;
		else
		  step_ = INITIAL_;
	      }
	    else
	      step_ = SEND_REQ_TO_CHILD_;
	    qparent_.down->removeHead();
	    
	    return WORK_OK;
	  }
	break;

	case REALLY_ALL_DONE_:
	  {
	    // this statement must be reprepared to be executed again.
	    // Hasta La Vista.
	    ExHandleErrors(qparent_,
			   pentry_down,
			   0,
			   getGlobals(),
			   NULL,
			   (ExeErrorCode)(-CLI_STMT_NOT_EXSISTS),
			   NULL,
			   ulTdb().getTableName()
			   );

	    step_ = ERROR_;
	  }
	break;

	} // switch
    } // while

} 

////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ExExeUtilUserLoadTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExExeUtilUserLoadPrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExExeUtilUserLoadPrivateState::ExExeUtilUserLoadPrivateState()
{
}

ExExeUtilUserLoadPrivateState::~ExExeUtilUserLoadPrivateState()
{
};

////////////////////////////////////////////////////////////////
// Methods for class ExExeUtilSidetreeInsertTcb
///////////////////////////////////////////////////////////////
//
#define SETSTEP(s) setStep(s, __LINE__)

static THREAD_P bool sv_checked_yet = false;
static THREAD_P bool sv_logStep = false;

void ExExeUtilSidetreeInsertTcb::setStep(Step newStep, int lineNum)
{
   step_ = newStep;

	if (!sv_checked_yet)
	{
    sv_checked_yet = true;
		char *logST = getenv("LOG_SIDETREE_UTIL");
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
    case EMPTY_TABLE_CHECK_:
      stepStr = (char *) "EMPTY_TABLE_CHECK_";
      break;
    case NO_ROLLBACK_INSERT_:
      stepStr = (char *) "NO_ROLLBACK_INSERT_";
      break;
    case NO_ROLLBACK_INSERT_THROUGH_AQR_:
      stepStr = (char *) "NO_ROLLBACK_INSERT_THROUGH_AQR_";
      break;
    case SIDETREE_INSERT_:
      stepStr = (char *) "SIDETREE_INSERT_";
      break;
    case ALTER_TO_NOAUDIT_:
      stepStr = (char *) "ALTER_TO_NOAUDIT_";
      break;
    case ALTER_TO_AUDIT_:
      stepStr = (char *) "ALTER_TO_AUDIT_";
      break;
    case SEND_REQ_TO_CHILD_:
      stepStr = (char *) "SEND_REQ_TO_CHILD_";
      break;
    case GET_REPLY_FROM_CHILD_:
      stepStr = (char *) "GET_REPLY_FROM_CHILD_";
      break;
    case ERROR_FROM_CHILD_:
      stepStr = (char *) "ERROR_FROM_CHILD_";
      break;
    case ROLLBACK_XN_:
      stepStr = (char *) "ROLLBACK_XN_";
      break;
    case ALTER_TO_AUDIT_AND_ERROR_:
      stepStr = (char *) "ALTER_TO_AUDIT_AND_ERROR_";
      break;
    case PURGEDATA_TGT_AND_ERROR_:
      stepStr = (char *) "PURGEDATA_TGT_AND_ERROR_";
      break;
    case COMMIT_XN_:
      stepStr = (char *) "COMMIT_XN_";
      break;
    case ERROR_:
      stepStr = (char *) "ERROR_";
      break;
    case CANCEL_:
      stepStr = (char *) "CANCEL_";
      break;
    case DONE_:
      stepStr = (char *) "DONE_";
      break;
  }

  cout << stepStr << ", line " << lineNum << endl;
}


///////////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilSidetreeInsertTdb::build(ex_globals * glob)
{
  // build the child first
  ex_tcb * childTcb = child_->build(glob);

  ExExeUtilSidetreeInsertTcb * exe_util_tcb;

  if (alterAudit())
    exe_util_tcb = 
      new(glob->getSpace()) ExExeUtilSidetreeInsertAlterAuditTcb(*this, childTcb, glob);
  else
    exe_util_tcb = 
      new(glob->getSpace()) ExExeUtilSidetreeInsertTcb(*this, childTcb, glob);
  
  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}


ExExeUtilSidetreeInsertTcb::ExExeUtilSidetreeInsertTcb(
     const ComTdbExeUtilSidetreeInsert & exe_util_tdb,
     const ex_tcb * child_tcb, 
     ex_globals * glob)
  : ExExeUtilTcb( exe_util_tdb, child_tcb, glob)
{
  // Allocate the private state in each entry of the down queue
  qparent_.down->allocatePstate(this);

  SETSTEP(INITIAL_);

  ddlLockWasAdded_ = FALSE;
}

ExExeUtilSidetreeInsertTcb::~ExExeUtilSidetreeInsertTcb()
{
}

ExExeUtilSidetreeInsertAlterAuditTcb::ExExeUtilSidetreeInsertAlterAuditTcb(
     const ComTdbExeUtilSidetreeInsert & exe_util_tdb,
     const ex_tcb * child_tcb, 
     ex_globals * glob)
  : ExExeUtilSidetreeInsertTcb( exe_util_tdb, child_tcb, glob)
{
  // Allocate the private state in each entry of the down queue
  qparent_.down->allocatePstate(this);

  SETSTEP(INITIAL_);

  ddlLockWasAdded_ = FALSE;
}

Int32 ExExeUtilSidetreeInsertAlterAuditTcb::fixup()
{
  return 0;
  //  return ex_tcb::fixup();
}

//////////////////////////////////////////////////////
// work() for ExExeUtilSidetreeInsertAlterAuditTcb
//////////////////////////////////////////////////////
short ExExeUtilSidetreeInsertAlterAuditTcb::work()
{
  short rc = 0;
  Lng32 cliRC = 0;

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
  const ex_queue::down_request & request = pentry_down->downState.request;

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();

  ContextCli *currContext = masterGlob->getStatement()->getContext();
  AQRInfo * aqr = currContext->aqrInfo();

  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
            ExTransaction *ta = getGlobals()->castToExExeStmtGlobals()->
                 castToExMasterStmtGlobals()->getStatement()->getContext()->getTransaction();
	    NABoolean xnAlreadyStarted = ta->xnInProgress();
	    if (xnAlreadyStarted)
	      {
		if (NOT ((ta->implicitXn()) &&
			 (ta->autoCommit()) &&
			 (ta->exeStartedXn())))
		  {
		    ExHandleErrors(qparent_,
				   pentry_down,
				   0,
				   getGlobals(),
				   NULL,
				   (ExeErrorCode)(-20123),
				   NULL,
				   "UTIL ST INSERT"
				   );
		    SETSTEP(ERROR_);
		    break;
		  }
		
		// commit the implicit transaction
		cliRC = ta->rollbackTransactionWaited();
		if (cliRC != 0)
		  {
		    ExHandleErrors(qparent_,
				   pentry_down,
				   0,
				   getGlobals(),
				   NULL,
				   (ExeErrorCode)(-20123),
				   NULL,
				   "UTIL ST INSERT"
				   );
		    SETSTEP(ERROR_);
		    break;
		  }

		// before executing the no rollback query, release any
		// transactions associated with the current statement.
		masterGlob->getStatement()->releaseTransaction();
	      }

	    cliRC = holdAndSetCQD("TRANSFORM_TO_SIDETREE_INSERT", "OFF");
	    if (cliRC < 0)
	      {
		cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
		
		SETSTEP(ERROR_);
		break;
	      }

	    if (ulTdb().sidetreeQuery() == NULL)
	      SETSTEP(NO_ROLLBACK_INSERT_);
	    else
	      SETSTEP(LOCK_TARGET_);
	  }
	break;

        case LOCK_TARGET_:
          {
#if 0
// Locking the target produced an unexpected error 73 when the step
// went to SEND_REQ_TO_CHILD_ for many of the queries in
// executor/TEST024.

            query_ = new(getGlobals()->getDefaultHeap()) char[1000];
            str_sprintf(query_, "lock table  %s in share mode;",
                        ulTdb().getTableName());
            Lng32 len = 0;
            Int64 dummyReturn = 0;
            cliRC = cliInterface()->executeImmediate(query_,
                                                     (char*)&dummyReturn,
                                                     &len, NULL);
            NADELETEBASIC(query_, getMyHeap());
            if (cliRC < 0)
              {
                cliInterface()->retrieveSQLDiagnostics(getDiagsArea());

                SETSTEP(ERROR_);
                break;
              }
#endif
            SETSTEP(EMPTY_TABLE_CHECK_);
          }
        break;

	case EMPTY_TABLE_CHECK_:
	  {
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
		break;
	      }

	    if (rowCount != 0)
	      {
		// table is not empty.
		// do regular insert
		SETSTEP(NO_ROLLBACK_INSERT_THROUGH_AQR_);
		break;
	      }

	    SETSTEP(ALTER_TO_NOAUDIT_); 
	  }
	break;

	case NO_ROLLBACK_INSERT_THROUGH_AQR_:
	  {
	    ComDiagsArea * da = getDiagsArea();
	    *da << DgSqlCode(-8587)
		<< DgString0("Table is not empty");
	    
	    SETSTEP(ERROR_);
	  }
	  break;

	case NO_ROLLBACK_INSERT_:
	  {
	    // execute the original 'no rollback' query.
	    Int64 rowsAffected;
	    cliRC = cliInterface()->executeImmediate(ulTdb().originalQuery(),
						     NULL, 0, FALSE,
						     &rowsAffected);
	    if (cliRC < 0)
	      {
		// aqr cant be done for a 'no rollback' query.
		aqr->setNoAQR(TRUE);
		cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
	    
		SETSTEP(ERROR_);
	      }
	    else
	      {
		masterGlob->setRowsAffected(rowsAffected);
		SETSTEP(DONE_);
	      }

	    if (ulTdb().displayWarnings())
	      {
		ComDiagsArea * da = getDiagsArea();
		*da << DgSqlCode(8587)
		    << DgString0("Table is not empty");
	      }
	  }
	break;

	case ALTER_TO_NOAUDIT_:
	  {
	    cliRC = cliInterface()->beginWork();
	    if (cliRC < 0)
	      {
		cliInterface()->retrieveSQLDiagnostics(getDiagsArea());

		SETSTEP(ERROR_);
		break;
	      }

	    // do not validate DDL privilege
	    masterGlob->getStatement()->getContext()->setSqlParserFlags(0x40000);

	    cliRC = changeAuditAttribute(ulTdb().getTableName(),
					 FALSE,
					 FALSE, FALSE,
					 TRUE /*parallel label op*/);
	    masterGlob->getStatement()->getContext()->resetSqlParserFlags(0x40000);
	    if (cliRC < 0)
	      {
		cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
		
		SETSTEP(ROLLBACK_XN_);
		break;
	      }


	    cliRC = cliInterface()->commitWork();
	    if (cliRC < 0)
	      {
		cliInterface()->retrieveSQLDiagnostics(getDiagsArea());

		SETSTEP(ERROR_);
		break;
	      }

	    SETSTEP(SIDETREE_INSERT_);
	  }
	break;

	case SIDETREE_INSERT_:
	  {
	    Int64 rowsAffected;
	    cliRC = cliInterface()->executeImmediate(ulTdb().sidetreeQuery(),
						     NULL, 0, FALSE,
						     &rowsAffected);
	    if (cliRC < 0)
	      {
		cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
	    
		SETSTEP(ALTER_TO_AUDIT_AND_ERROR_); //ROLLBACK_XN_;
		break;
	      }

	    masterGlob->setRowsAffected(rowsAffected);

	    SETSTEP(ALTER_TO_AUDIT_);
	  }
	break;

	case ALTER_TO_AUDIT_:
	case ALTER_TO_AUDIT_AND_ERROR_:
	  {
	    cliRC = cliInterface()->beginWork();
	    if (cliRC < 0)
	      {
		cliInterface()->retrieveSQLDiagnostics(getDiagsArea());

		SETSTEP(ERROR_);
		break;
	      }


	    // do not validate DDL privilege
	    masterGlob->getStatement()->getContext()->setSqlParserFlags(0x40000);
	    cliRC = changeAuditAttribute(ulTdb().getTableName(),
					 TRUE,
					 FALSE, FALSE,
					 TRUE /*parallal label op*/);
	    masterGlob->getStatement()->getContext()->resetSqlParserFlags(0x40000);
	    if (cliRC < 0)
	      {
		cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
		
		SETSTEP(ROLLBACK_XN_);
		break;
	      }

	    cliRC = cliInterface()->commitWork();
	    if (cliRC < 0)
	      {
		cliInterface()->retrieveSQLDiagnostics(getDiagsArea());

		SETSTEP(ERROR_);
		break;
	      }

	    if (step_ == ALTER_TO_AUDIT_AND_ERROR_)
	      SETSTEP(PURGEDATA_TGT_AND_ERROR_);
	    else
	      SETSTEP(DONE_);
	  }
	break;

	case ROLLBACK_XN_:
	  {
	    cliRC = cliInterface()->rollbackWork();
	    
	    SQL_EXEC_ClearDiagnostics(NULL);
	    
	    SETSTEP(ERROR_);
	  }
	  break;

	case COMMIT_XN_:
	  {
	    cliRC = cliInterface()->commitWork();
	    
	    SQL_EXEC_ClearDiagnostics(NULL);
	    
	    SETSTEP(DONE_);
	  }
	  break;

	case PURGEDATA_TGT_AND_ERROR_:
	  {
	    // table was empty to start with.
	    // purgedata in case of this error.
	    query_ = new(getGlobals()->getDefaultHeap()) char[1000];
	    str_sprintf(query_, "purgedata %s;",
			ulTdb().getTableName());
	    cliRC = cliInterface()->executeImmediate(query_);
	    NADELETEBASIC(query_, getMyHeap());
	    
	    SETSTEP(ERROR_);
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
	    
	    if (getDiagsArea())
	      diagsArea->mergeAfter(*getDiagsArea());
	    
	    up_entry->setDiagsArea (diagsArea);
	    
	    // insert into parent
	    qparent_.up->insert();
	    
	    SETSTEP(DONE_);
	  }
	break;

	case DONE_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    restoreCQD("TRANSFORM_TO_SIDETREE_INSERT");

	    // Return EOF.
	    ex_queue_entry * up_entry = qparent_.up->getTailEntry();
	    
	    up_entry->upState.parentIndex = 
	      pentry_down->downState.parentIndex;
	    
	    up_entry->upState.setMatchNo(0);
	    up_entry->upState.status = ex_queue::Q_NO_DATA;
	    
	    if (getDiagsArea()->getNumber(DgSqlCode::WARNING_) > 0) // must be a warning
	      {
		ComDiagsArea *diagsArea = up_entry->getDiagsArea();
		
		if (diagsArea == NULL)
		  diagsArea = 
		    ComDiagsArea::allocate(this->getGlobals()->getDefaultHeap());
                else
                  diagsArea->incrRefCount (); // setDiagsArea call below will decr ref count
		
		if (getDiagsArea())
		  diagsArea->mergeAfter(*getDiagsArea());
		
		up_entry->setDiagsArea (diagsArea);
	      }

	    // insert into parent
	    qparent_.up->insert();
	    
	    SETSTEP(INITIAL_);
	    qparent_.down->removeHead();
	    
	    return WORK_OK;
	  }
	break;

	} // switch
    } // while

} 

Int32 ExExeUtilSidetreeInsertTcb::fixup()
{
  return ex_tcb::fixup();
}

//////////////////////////////////////////////////////
// work() for ExExeUtilSidetreeInsertTcb
//////////////////////////////////////////////////////
short ExExeUtilSidetreeInsertTcb::work()
{
  short rc = 0;
  Lng32 cliRC = 0;

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
  const ex_queue::down_request & request = pentry_down->downState.request;

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ExTransaction *ta = getGlobals()->castToExExeStmtGlobals()->
      castToExMasterStmtGlobals()->getStatement()->getContext()->getTransaction();
  
  ContextCli *currContext = masterGlob->getStatement()->getContext();
  AQRInfo * aqr = currContext->aqrInfo();

  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
	    if (ta->xnInProgress())
	      {
		if (NOT ((ta->implicitXn()) &&
			 (ta->autoCommit()) &&
			 (ta->exeStartedXn())))
		  {
		    ExHandleErrors(qparent_,
				   pentry_down,
				   0,
				   getGlobals(),
				   NULL,
				   (ExeErrorCode)(-20123),
				   NULL,
				   "UTIL ST INSERT"
				   );
		    SETSTEP(ERROR_);
		    break;
		  }
	      }

	    ddlLockWasAdded_ = FALSE;

	    SQL_EXEC_ClearDiagnostics(NULL);
	    if (getDiagsArea())
	      {
		getDiagsArea()->clear();
	      }
	    
	    if (ulTdb().sidetreeQuery() == NULL)
	      SETSTEP(NO_ROLLBACK_INSERT_THROUGH_AQR_);
	    //	      SETSTEP(NO_ROLLBACK_INSERT_);
	    else
	      SETSTEP(LOCK_TARGET_);
          }
        break;

        case LOCK_TARGET_:
          {
#if 0
// Locking the target produced an unexpected error 73 when the step
// went to SEND_REQ_TO_CHILD_ for many of the queries in
// executor/TEST024.
            query_ = new(getGlobals()->getDefaultHeap()) char[1000];
            str_sprintf(query_, "lock table  %s in share mode;",
                        ulTdb().getTableName());
            Lng32 len = 0;
            Int64 dummyReturn = 0;
            cliRC = cliInterface()->executeImmediate(query_,
                                                     (char*)&dummyReturn,
                                                     &len, NULL);
            NADELETEBASIC(query_, getMyHeap());
            if (cliRC < 0)
              {
                cliInterface()->retrieveSQLDiagnostics(getDiagsArea());

                SETSTEP(ERROR_);
                break;
              }
#endif
            SETSTEP(EMPTY_TABLE_CHECK_);
	  }
	break;

	case EMPTY_TABLE_CHECK_:
	  {
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
		break;
	      }

	    if (rowCount != 0)
	      {
		// table is not empty.
		// do regular insert
		SETSTEP(NO_ROLLBACK_INSERT_THROUGH_AQR_);
		break;
	      }

	    SETSTEP(SIDETREE_INSERT_);
	  }
	break;

	case NO_ROLLBACK_INSERT_THROUGH_AQR_:
	  {
	    ComDiagsArea * da = getDiagsArea();
	    *da << DgSqlCode(-8587)
		<< DgString0("Table is not empty");
	    
	    SETSTEP(ERROR_);
	  }
	  break;

	case SIDETREE_INSERT_:
	  {
	    SETSTEP(SEND_REQ_TO_CHILD_);
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

	    SETSTEP(GET_REPLY_FROM_CHILD_);
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

	    ex_queue_entry * centry = qchild_.up->getHeadEntry();
	    
	    ex_queue::up_status child_status = centry->upState.status;
	    switch(child_status)
	      {
	      case ex_queue::Q_NO_DATA:
		{
		  ex_queue_entry * up_entry = qparent_.up->getTailEntry();
		  up_entry->copyAtp(centry);

		  qchild_.up->removeHead();	    
		  SETSTEP(DONE_);
		}
	      break;

	      case ex_queue::Q_OK_MMORE:
	      case ex_queue::Q_INVALID:
		{
		  ex_queue_entry * up_entry = qparent_.up->getTailEntry();

		  // invalid state, should not be reached.
		  ComDiagsArea * da = up_entry->getDiagsArea();
		  ExRaiseSqlError(getMyHeap(),
				  &da,
				  (ExeErrorCode)(EXE_INTERNAL_ERROR));
		  SETSTEP(CANCEL_);
		}
		break;

	      case ex_queue::Q_SQLERROR:
		{
		  SETSTEP(ERROR_FROM_CHILD_);
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

	    SETSTEP(CANCEL_);
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

		  SETSTEP(ALTER_TO_AUDIT_AND_ERROR_);
		}
	      break;
	      
	      }
	  }
	break;
	
	case ALTER_TO_AUDIT_AND_ERROR_:
	  {
	    // release any transactions associated with the current statement.
	    masterGlob->getStatement()->releaseTransaction();

	    if (ta->xnInProgress())
	      {
		cliRC = cliInterface()->commitWork();
		if (cliRC < 0)
		  {
		    cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
		    
		    SETSTEP(ERROR_);
		    break;
		  }
	      }

	    NABoolean xnStartedHere = FALSE;
	    if (NOT ta->xnInProgress())
	      {
		cliRC = cliInterface()->beginWork();
		if (cliRC < 0)
		  {
		    cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
		    
		    SETSTEP(ERROR_);
		    break;
		  }
		xnStartedHere = TRUE;
	      }

	    // do not validate DDL privilege
	    masterGlob->getStatement()->getContext()->setSqlParserFlags(0x40000);
	    cliRC = changeAuditAttribute(ulTdb().getTableName(),
					 TRUE,
					 FALSE, FALSE,
					 TRUE /*parallal label op*/);

	    masterGlob->getStatement()->getContext()->resetSqlParserFlags(0x40000);
	    if (cliRC < 0)
	      {
		cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
		
		if (xnStartedHere)
		  SETSTEP(ROLLBACK_XN_);
		else
		  SETSTEP(ERROR_);
		break;
	      }

	    if (xnStartedHere)
	      {
		cliRC = cliInterface()->commitWork();
		if (cliRC < 0)
		  {
		    cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
		    
		    SETSTEP(ERROR_);
		    break;
		  }
	      }

	    SETSTEP(PURGEDATA_TGT_AND_ERROR_);
	  }
	break;

	case ROLLBACK_XN_:
	  {
	    cliRC = cliInterface()->rollbackWork();
	    
	    SQL_EXEC_ClearDiagnostics(NULL);
	    
	    SETSTEP(ERROR_);
	  }
	  break;
	  
	case PURGEDATA_TGT_AND_ERROR_:
	  {
	    // table was empty to start with.
	    // purgedata in case of this error.
	    query_ = new(getGlobals()->getDefaultHeap()) char[1000];
	    str_sprintf(query_, "purgedata %s;",
			ulTdb().getTableName());
	    cliRC = cliInterface()->executeImmediate(query_);
	    NADELETEBASIC(query_, getMyHeap());
	    
	    SETSTEP(ERROR_);
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
	    
	    //	    up_entry->upState.setMatchNo(0);
	    up_entry->upState.status = ex_queue::Q_SQLERROR;

	    ComDiagsArea *diagsArea = up_entry->getDiagsArea();
	    
	    if (diagsArea == NULL)
	      diagsArea = 
		ComDiagsArea::allocate(this->getGlobals()->getDefaultHeap());
	    
	    if (getDiagsArea())
	      diagsArea->mergeAfter(*getDiagsArea());
	    
	    up_entry->setDiagsArea (diagsArea);
	    
	    // insert into parent
	    qparent_.up->insert();
	    
	    SETSTEP(DONE_);
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
	    
	    if (getDiagsArea()->getNumber(DgSqlCode::WARNING_) > 0) // must be a warning
	      {
		ComDiagsArea *diagsArea = up_entry->getDiagsArea();
		
		if (diagsArea == NULL)
		  diagsArea = 
		    ComDiagsArea::allocate(this->getGlobals()->getDefaultHeap());
                else
                  diagsArea->incrRefCount (); // setDiagsArea call below will decr ref count
		
		if (getDiagsArea())
		  diagsArea->mergeAfter(*getDiagsArea());
		
		up_entry->setDiagsArea (diagsArea);
	      }

	    // insert into parent
	    qparent_.up->insert();
	    
	    SETSTEP(INITIAL_);
	    qparent_.down->removeHead();
	    
	    return WORK_OK;
	  }
	break;

	} // switch
    } // while

} 

////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ExExeUtilSidetreeInsertTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExExeUtilSidetreeInsertPrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExExeUtilSidetreeInsertPrivateState::ExExeUtilSidetreeInsertPrivateState()
{
}

ExExeUtilSidetreeInsertPrivateState::~ExExeUtilSidetreeInsertPrivateState()
{
};

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
// Constructor for class ExExeUtilUserLoadFastTcb
///////////////////////////////////////////////////////////////
ExExeUtilUserLoadFastTcb::ExExeUtilUserLoadFastTcb(
     const ComTdbExeUtilUserLoad & exe_util_tdb,
     const ex_tcb * child_tcb, // for child queue
     ex_globals * glob)
     : ExExeUtilUserLoadTcb( exe_util_tdb, child_tcb, glob)
{
  for (UInt32 tidx = 0; tidx < NumTraces; tidx++)
  {
    stepTrace_[tidx].oldStep_ = TRACE_NEVER_TRACED;
    stepTrace_[tidx].changedOnLine_ = __LINE__;
  }
  lastStep_ = NumTraces - 1;  // let first trace start at 0;
  
  changeStep(INITIAL_);

  // fixup expressions
  (void) ulTdb().rwrsInputSizeExpr()->fixup
    (0, getExpressionMode(), this, 
     glob->getSpace(), glob->getDefaultHeap());

  (void) ulTdb().rwrsMaxInputRowlenExpr()->fixup
    (0, getExpressionMode(), this, 
     glob->getSpace(), glob->getDefaultHeap());

  (void) ulTdb().rwrsBufferAddrExpr()->fixup
    (0, getExpressionMode(), this, 
     glob->getSpace(), glob->getDefaultHeap());

  if (ulTdb().excpRowExpr_)
    {
      (void) ulTdb().excpRowExpr_->fixup
	(0, getExpressionMode(), this, 
	 glob->getSpace(), glob->getDefaultHeap());

      pool_->get_free_tuple(excpRowTupp_, exe_util_tdb.excpRowLen_); 
      
      pool_->get_free_tuple(workAtp_->getTupp(ulTdb().excpRowAtpIndex_), 0);
    }

  // rows will be copied to child.
  // Allocate target atps in child's down queue.
  qchild_.down->allocateAtps(glob->getSpace());

  if (ulTdb().excpTabInsertStmt())
    excpRecNum_ = 0;
  else
    excpRecNum_ = -1;
}

//////////////////////////////////////////////////////
// work() for ExExeUtilUserLoadFastTcb
//////////////////////////////////////////////////////
short ExExeUtilUserLoadFastTcb::work()
{
  short rc = 0;
  Lng32 cliRC = 0;
  ex_expr::exp_return_type retCode;

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
  const ex_queue::down_request & request = pentry_down->downState.request;

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ContextCli *currContext = masterGlob->getStatement()->getContext();

  if (pentry_down->downState.request == ex_queue::GET_NOMORE)
    {
      // cancel request
      changeStep(CANCEL_CHILD_);
    }

  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
            ExTransaction *ta = getGlobals()->castToExExeStmtGlobals()->
               castToExMasterStmtGlobals()->getStatement()->getContext()->getTransaction();
	    NABoolean xnAlreadyStarted = ta->xnInProgress();
	    if (xnAlreadyStarted)
	      {
		ExHandleErrors(qparent_,
			       pentry_down,
			       0,
			       getGlobals(),
			       NULL,
			       (ExeErrorCode)(-20123),
			       NULL,
			       "USER LOAD"
			       );
		changeStep(DONE_);
		break;
	      }

	    rwrsNumRows_ = 0;
	    rwrsMaxInputRowlen_ = -1;
	    rwrsBufferAddr_ = NULL;
	    
	    currentRowNum_ = 0;
	    numRqstsToChild_ = 0;

	    eodToChild_ = FALSE;

	    narExcepInitialized_ = FALSE;

	    changeStep(GET_INPUT_VALUES_);
	  }
	break;

	case GET_INPUT_VALUES_:
	  {
	    // get number of rows in the input rwrs buffer
	    workAtp_->getTupp(ulTdb().workAtpIndex()).
	      setDataPointer((char *)&rwrsNumRows_);
	    retCode = 
	      ulTdb().rwrsInputSizeExpr()->eval(pentry_down->getAtp(), workAtp_);
	    if (retCode == ex_expr::EXPR_ERROR)
	      {
		changeStep(ERROR_);
		break;
	      }
	    
	    if (rwrsNumRows_ == 0)
	      {
		// Send GET_EOD to child if this is sidetree insert.
		// STI expects a GET_EOD to be sent to commit.
		if ((ulTdb().sortFromTop()) ||
		    (ulTdb().userSidetreeInsert()))
		  {
		    changeStep(SEND_EOD_TO_CHILD_);
		  }
		else
		  {
		    changeStep(DONE_);
		  }

		break;
	      }

	    if (rwrsNumRows_ < 0)
	      {
		ExHandleErrors(qparent_,
			       pentry_down,
			       0,
			       getGlobals(),
			       NULL,
			       EXE_NUMERIC_OVERFLOW,
			       NULL,
			       NULL
			       );
		changeStep(DONE_);
		break;
	      }

	    // get max length of each row
	    workAtp_->getTupp(ulTdb().workAtpIndex()).
	      setDataPointer((char *)&rwrsMaxInputRowlen_);
	    retCode = 
	      ulTdb().rwrsMaxInputRowlenExpr()->eval(pentry_down->getAtp(), 
						     workAtp_);
	    if (retCode == ex_expr::EXPR_ERROR)
	      {
		changeStep(ERROR_);
		break;
	      }
	    
	    if (rwrsMaxInputRowlen_ <= 0)
	      {
		ExHandleErrors(qparent_,
			       pentry_down,
			       0,
			       getGlobals(),
			       NULL,
			       EXE_NUMERIC_OVERFLOW,
			       NULL,
			       NULL
			       );
		changeStep(DONE_);
		break;
	      }

	    // get address of rwrs buffer
	    workAtp_->getTupp(ulTdb().workAtpIndex()).
	      setDataPointer((char *)&rwrsBufferAddr_);
	    retCode = 
	      ulTdb().rwrsBufferAddrExpr()->eval(pentry_down->getAtp(), 
						  workAtp_);
	    if (retCode == ex_expr::EXPR_ERROR)
	      {
		changeStep(ERROR_);
		break;
	      }

	    changeStep(SEND_REQ_TO_CHILD_);
	  }
	break;

	case SEND_REQ_TO_CHILD_:
	  {
	    if (qchild_.down->isFull())
	      {
		if (NOT qchild_.up->isEmpty())
		  {
		    changeStep(GET_REPLY_FROM_CHILD_);
		    break;
		  }

		return WORK_OK;
	      }

	    ex_queue_entry * centry = qchild_.down->getTailEntry();

	    // get pointer to current row
	    char * currRow = (char*)(rwrsBufferAddr_ + 
				     currentRowNum_ * rwrsMaxInputRowlen_);

	    // allocate an empty tupp descriptor
	    tupp_descriptor * td = pool_->get_free_tupp_descriptor(0);
	    if (! td)
	      {
		// if rows in child's up queue, consume them.
		if (NOT qchild_.up->isEmpty())
		  {
		    changeStep(GET_REPLY_FROM_CHILD_);
		    break;
		  }

		return WORK_POOL_BLOCKED; // couldn't allocate, try again later.
	      }

	    // initialize it with the addr of the row to be returned
	    td->init(rwrsMaxInputRowlen_, NULL, currRow);

	    // set the child's input atp
	    centry->copyAtp(pentry_down->getAtp());
	    
	    centry->getTupp(ulTdb().childTuppIndex()) = td;

	    centry->downState.request = ex_queue::GET_ALL;
	    centry->downState.requestValue = 
	      pentry_down->downState.requestValue;
	    centry->downState.parentIndex = qparent_.down->getHeadIndex();
	    
	    qchild_.down->insert();

	    currentRowNum_++;
	    numRqstsToChild_++;

	    if (currentRowNum_ == rwrsNumRows_)
	      {
		if (NOT ulTdb().sortFromTop())
		  {
		    if (ulTdb().userSidetreeInsert())
		      changeStep(SEND_EOD_NO_ST_COMMIT_TO_CHILD_);
		    else
		      changeStep(SEND_EOD_TO_CHILD_);
		  }
		else
		  {
		    eodToChild_ = TRUE;
		    changeStep(GET_REPLY_FROM_CHILD_);
		  }
	      }
	    else
	      {
		changeStep(GET_REPLY_FROM_CHILD_);
	      }
	  }
	break;

	case GET_REPLY_FROM_CHILD_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

            // Fix for bugzilla 1563.  Check if it is now okay to go back
            // to the SEND_EOD_NO_ST_COMMIT_TO_CHILD_ or 
            // SEND_EOD_TO_CHILD_ step_.
            if ((numRqstsToChild_ == 0) &&
                (currentRowNum_ == rwrsNumRows_) &&
                 !qchild_.down->isFull())
              {
                if (NOT ulTdb().sortFromTop())
                  {
                    if (ulTdb().userSidetreeInsert())
                      changeStep(SEND_EOD_NO_ST_COMMIT_TO_CHILD_);
                    else
                      changeStep(SEND_EOD_TO_CHILD_);
                    break;
                  }
              }

	    if (qchild_.up->isEmpty())
	      {
		// child hasn't returned any reply rows. If more rows are to
		// be sent to child, process that.
		if (currentRowNum_ < rwrsNumRows_)

		  {
		    changeStep(SEND_REQ_TO_CHILD_);
		    break;
		  }

		return WORK_OK;
	      }

	    ex_queue_entry * centry = qchild_.up->getHeadEntry();
	    
	    ex_queue::up_status child_status = centry->upState.status;
	    switch(child_status)
	      {
	      case ex_queue::Q_OK_MMORE:
		{ 
		  ex_queue::up_status upStatus = ex_queue::Q_OK_MMORE;

		  if ((ulTdb().excpTabInsertStmt()) &&
		      (centry->getDiagsArea()))
		    {
		      excpRecNum_++;

		      if ((ulTdb().excpRowsNumber_ >= 0) &&
			  (excpRecNum_ > ulTdb().excpRowsNumber_))
			{
			  ComDiagsArea *diagsArea = centry->getDiagsArea();
			  
			  Lng32 index = 
			    diagsArea->returnIndex(-CLI_NAR_ERROR_DETAILS);
			  // remove the NAR details error entry.
			  if (index != NULL_COLL_INDEX)
			    {
			      diagsArea->deleteError(index);
			    }
			  
			  //			  ComDiagsArea * da = getDiagsArea();
			  *diagsArea << DgSqlCode(-30048)
				     << DgInt0((Lng32)excpRecNum_)
				     << DgInt1((Lng32)ulTdb().excpRowsNumber_);
			  
			  upStatus = ex_queue::Q_SQLERROR;
			}
		      else
			{
			  changeStep(HANDLE_NAR_EXCEPTION_);
			  break;
			}
		    }

		  ex_queue_entry *pentry = qparent_.up->getTailEntry();
		  
		  pentry->copyAtp(centry);
		  
		  pentry->upState.status = upStatus;

		  pentry->upState.parentIndex = pentry_down->downState.parentIndex;
		  pentry->upState.downIndex = qparent_.down->getHeadIndex();
		  pentry->upState.setMatchNo(0);
		  
		  qparent_.up->insert();
		  qchild_.up->removeHead();

		  if (upStatus == ex_queue::Q_SQLERROR)
		    {
		      changeStep(CANCEL_CHILD_);
		      break;
		    }
		}
	      break;
	      
	      case ex_queue::Q_NO_DATA:
		{
		  qchild_.up->removeHead();	    

		  numRqstsToChild_--;

		  if (numRqstsToChild_ == 0)
		    {
		      if (eodToChild_)
			changeStep(DONE_);
		      else if (currentRowNum_ < rwrsNumRows_)
			changeStep(SEND_REQ_TO_CHILD_);
		    }
		}
	      break;

	      case ex_queue::Q_SQLERROR:
		{
		  ex_queue_entry *pentry = qparent_.up->getTailEntry();
		  
		  pentry->copyAtp(centry);
		  pentry->upState.status = ex_queue::Q_SQLERROR;
		  pentry->upState.parentIndex = pentry_down->downState.parentIndex;
		  pentry->upState.downIndex = qparent_.down->getHeadIndex();
		  pentry->upState.setMatchNo(0);
		  
		  qparent_.up->insert();
		  qchild_.up->removeHead();

		  changeStep(CANCEL_CHILD_);
		  break;
		}
	      break;

	      case ex_queue::Q_INVALID:
		ex_assert(0,"ExExeUtilUserLoadTcb::work() Invalid state returned by child");
		break;
	      }
	  }
	break;

	case SEND_EOD_TO_CHILD_:
	case SEND_EOD_NO_ST_COMMIT_TO_CHILD_:
	  {
	    if (qchild_.down->isFull())
	      {
		if (NOT qchild_.up->isEmpty())
		  {
		    changeStep(GET_REPLY_FROM_CHILD_);
		    break;
		  }

		return WORK_OK;
	      }

	    pentry_down = qparent_.down->getHeadEntry();
	    ex_queue_entry * centry = qchild_.down->getTailEntry();
	    if (step_ == SEND_EOD_NO_ST_COMMIT_TO_CHILD_)
	      centry->downState.request = ex_queue::GET_EOD_NO_ST_COMMIT;
	    else
	      centry->downState.request = ex_queue::GET_EOD;
	      
	    centry->downState.requestValue = 
	      pentry_down->downState.requestValue;
	    centry->downState.parentIndex = 
	      qparent_.down->getHeadIndex();
	    qchild_.down->insert();

	    eodToChild_ = TRUE;
	    numRqstsToChild_++;

	    changeStep(GET_REPLY_FROM_CHILD_);
	  }
	break;

	case HANDLE_NAR_EXCEPTION_:
	  {
	    // if first error, prepare the insert into excp table stmt.
	    if ((NOT narExcepInitialized_) &&
		(ulTdb().excpTabInsertStmt()))
	      {
		cliRC = holdAndSetCQD("UPD_ABORT_ON_ERROR", "ON",
				      currContext->getDiagsArea());
		if (cliRC < 0)
		  {
		    cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
		    changeStep(ERROR_);
		    break;
		  }
		
		cliRC = cliInterface()->executeImmediate("set transaction autocommit on;");
		if (cliRC < 0)
		  {
		    cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
		    changeStep(ERROR_);
		    break;
		  }
		
		NABoolean savedIFIO = 
		  currContext->getSessionDefaults()->getInternalFormatIO();
		currContext->getSessionDefaults()->setInternalFormatIO(TRUE);
		cliRC = 
		  cliInterface()->fetchRowsPrologue(
		       ulTdb().excpTabInsertStmt(), TRUE);
		currContext->getSessionDefaults()->setInternalFormatIO(savedIFIO);
		
		if (cliRC < 0)
		  {
		    cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
		    changeStep(ERROR_);
		    break;
		  }

		narExcepInitialized_ = TRUE;
	      }

	    ex_queue_entry * centry = qchild_.up->getHeadEntry();

	    ComDiagsArea *diagsArea = centry->getDiagsArea();

	    Lng32 index;
	    index = diagsArea->returnIndex(-CLI_NAR_ERROR_DETAILS);
	    char * origRow = NULL;
	    const char * physTabNam = NULL;
	    Lng32 errNum = -1;
	    if (index == NULL_COLL_INDEX)
	      errNum = diagsArea->mainSQLCODE();
	    else
	      {
		ComCondition &cond = (*diagsArea)[index+1];
		
		// convert hex'd row to original format.
		const char * hexdRow = cond.getOptionalString(0);

		errNum = cond.getOptionalInteger(0);

		physTabNam = cond.getOptionalString(2);

		if (hexdRow && strlen(hexdRow) > 0)
		  origRow = new(getHeap()) char[strlen(hexdRow)/2];
	    
		UInt32 i = 0;
		Int32 j = 0;
		NABoolean error = FALSE;
		while ((origRow) && (NOT error) && (i < strlen(hexdRow)))
		  {
		    if (((hexdRow[i] >= '0') && (hexdRow[i] <= '9')) ||
			((hexdRow[i] >= 'A') && (hexdRow[i] <= 'F')) &&
			(((hexdRow[i+1] >= '0') && (hexdRow[i+1] <= '9')) ||
			 ((hexdRow[i+1] >= 'A') && (hexdRow[i+1] <= 'F'))))
		      {
			unsigned char upper4Bits;
			unsigned char lower4Bits;
			if ((hexdRow[i] >= '0') && (hexdRow[i] <= '9'))
			  upper4Bits = (unsigned char)(hexdRow[i]) - '0';
			else
			  upper4Bits = (unsigned char)(hexdRow[i]) - 'A' + 10;
			
			if ((hexdRow[i+1] >= '0') && (hexdRow[i+1] <= '9'))
			  lower4Bits = (unsigned char)(hexdRow[i+1]) - '0';
			else
			  lower4Bits = (unsigned char)(hexdRow[i+1]) - 'A' + 10;
			
			origRow[j] = (upper4Bits << 4) | lower4Bits;
			
			i += 2;
			j++;
		      }
		    else
		      {
			if (qparent_.up->isFull())
			  return WORK_OK;

			ExHandleErrors(qparent_,
				       pentry_down,
				       0,
				       getGlobals(),
				       NULL,
				       (ExeErrorCode)(-EXE_NUMERIC_OVERFLOW),
				       NULL,
				       NULL
				       );
			error = TRUE;
		      }
		  } // while


		if (error)
		  {
		    if (origRow)
		      NADELETEBASIC(origRow, getHeap());
		    changeStep(CANCEL_CHILD_);
		    break;
		  }

		if (origRow)
		  {
		    workAtp_->getTupp(ulTdb().workAtpIndex()).
		      setDataPointer(origRow);
		    workAtp_->getTupp(ulTdb().excpRowAtpIndex_).
		      setDataPointer(excpRowTupp_.getDataPointer());
		    retCode = 
		      ulTdb().excpRowExpr_->eval(pentry_down->getAtp(), 
						 workAtp_);
		    
		    NADELETEBASIC(origRow, getHeap());
		    if (retCode == ex_expr::EXPR_ERROR)
		      {
			changeStep(ERROR_);
			break;
		      }
		  }
	      } // else
	    
	    // add values in the target exploded row
	    // corresponding to the following fields. First field is at
	    // offset excpFirstOtherEntryOffset_.
	    //
	    //     _excp_err_num int, 
	    //     _excp_object_name varchr(ComMAX_ANSI_NAME_EXTERNAL_LEN)
	    //     _excp_partn_name char(ComMAX_FULLY_QUALIFIED_GUARDIAN_NAME_LEN)
	    //     _excp_file_num int
	    //     _excp_rec_num largeint
	    //     _excp_load_id largeint
	    //     _excp_time_run timestamp
	    //     
	    
	    Lng32 numInputs, numOutputs;
	    cliRC = cliInterface()->getNumEntries(numInputs, numOutputs);
	    if (cliRC < 0)
	      {
		cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
		
		changeStep(CANCEL_CHILD_AND_ERROR_);
		break;
	      }
	    
	    char * otherRow = excpRowTupp_.getDataPointer();
	    
	    Int32 ii = 0;
	    if (origRow == NULL)
	      {
		// no row was sent as part of NAR error. Insert a NULL entry for
		// each of the load table columns.
		for (ii = 1; ii <= ulTdb().numTableCols_; ii++)
		  {
		    Lng32 temp, indOffset, varOffset;
		    cliRC = cliInterface()->
		      getAttributes((short) ii, TRUE, // for input
				    temp, temp,
				    &indOffset, &varOffset);
		    if (cliRC < 0)
		      {
			cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
			
			changeStep(CANCEL_CHILD_AND_ERROR_);
			break;
		      }
		    
		    *(short*)&otherRow[indOffset] = -1;
		  } // for
	      } // origRow = NULL
	    
	    for (ii = ulTdb().numTableCols_+1; ii <= numInputs; ii++)
	      {
		Lng32 temp, indOffset, varOffset;
		cliRC = cliInterface()->
		  getAttributes((short) ii, TRUE, // for input
				temp, temp,
				&indOffset, &varOffset);
		if (cliRC < 0)
		  {
		    cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
		    
		    changeStep(CANCEL_CHILD_AND_ERROR_);
		    break;
		  }
		
		if (ii == ulTdb().numTableCols_+1) //excp_err_num
		  {
		    *(short*)&otherRow[indOffset] = 0;
		    *(Int32*)&otherRow[varOffset] = errNum; 
		  }
		else if (ii == ulTdb().numTableCols_+2) // excp_object_name
		  {
		    *(short*)&otherRow[indOffset] = -1;
		    if (ulTdb().getTableName())
		      {
			*(short*)&otherRow[indOffset] = 0;
			*(short*)&otherRow[varOffset] = strlen(ulTdb().getTableName());
			str_cpy_all(&otherRow[varOffset+2], ulTdb().getTableName(),
				    strlen(ulTdb().getTableName()));
		      }
		  }
		else if (ii == ulTdb().numTableCols_+3) // excp_partn_name
		  {
		    *(short*)&otherRow[indOffset] = -1;
		    if (physTabNam && (strlen(physTabNam) > 0))
		      {
			*(short*)&otherRow[indOffset] = 0;
			str_pad(&otherRow[varOffset], 
				ComMAX_FULLY_QUALIFIED_GUARDIAN_NAME_LEN, ' ');
			str_cpy_all(&otherRow[varOffset], physTabNam,
				    strlen(physTabNam));
		      }
		  }
		else if (ii == ulTdb().numTableCols_+4) // excp_file_num
		  {
		    *(short*)&otherRow[indOffset] = -1;
		    *(Int32*)&otherRow[varOffset] = -1;
		  }
		else if (ii == ulTdb().numTableCols_+5) // excp_rec_num
		  {
		    *(short*)&otherRow[indOffset] = 0;
		    *(Int64*)&otherRow[varOffset] = excpRecNum_;
		  }
		else if (ii == ulTdb().numTableCols_+6) // excp_load_id
		  {
		    if (ulTdb().loadId_ == -1)
		      *(short*)&otherRow[indOffset] = -1;
		    else
		      *(short*)&otherRow[indOffset] = 0;
		    *(Int64*)&otherRow[varOffset] = ulTdb().loadId_;
		  }
		
	      } // for ii
	    
	    qchild_.up->removeHead();
	    
	    NABoolean savedIFIO = 
	      currContext->getSessionDefaults()->getInternalFormatIO();
	    currContext->getSessionDefaults()->setInternalFormatIO(TRUE);
	    cliRC = cliInterface()->clearExecFetchClose(excpRowTupp_.getDataPointer(), 
							ulTdb().excpRowLen_);
	    currContext->getSessionDefaults()->setInternalFormatIO(savedIFIO);
	    if (cliRC < 0)
	      {
		cliInterface()->retrieveSQLDiagnostics(getDiagsArea());
		
		changeStep(CANCEL_CHILD_AND_ERROR_);
		break;
	      }
	    
	    changeStep(GET_REPLY_FROM_CHILD_);
	  }
	break;
	
	case CANCEL_CHILD_:
	case CANCEL_CHILD_AND_ERROR_:
	  {
            // this state is reached when child returns an error,
            // and when my parent cancels.
            qchild_.down->cancelRequestWithParentIndex(
                                          qparent_.down->getHeadIndex());

            if (qchild_.up->isEmpty())
              {
                if (numRqstsToChild_ == 0)
                  {
                    // Solution 10-100426-9767 
                    if (step_ == CANCEL_CHILD_)
                      changeStep(DONE_);
                    else
                      changeStep(ERROR_);
                    break;
                  }
                else 
                  return WORK_OK;
              }

	    ex_queue_entry * centry = qchild_.up->getHeadEntry();
	    switch(centry->upState.status)
	      {
	      case ex_queue::Q_SQLERROR:
	      case ex_queue::Q_OK_MMORE:
		{
		  qchild_.up->removeHead();
		}
	      break;
	      
	      case ex_queue::Q_NO_DATA:
		{
		  qchild_.up->removeHead();

		  numRqstsToChild_--;
		  if (numRqstsToChild_ == 0)
		    {
		      if (step_ == CANCEL_CHILD_)
			changeStep(DONE_);
		      else
			changeStep(ERROR_);
		    }
		}
	      break;
	      
	      default:
		{
		  ex_assert(0, "ExSortFromTopTcb::work() Error state returned from child");
		}
	      break;
	      }
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
	    
	    if (getDiagsArea())
	      diagsArea->mergeAfter(*getDiagsArea());
	    
	    up_entry->setDiagsArea (diagsArea);
	    
	    // insert into parent
	    qparent_.up->insert();
	    
	    changeStep(ERROR_DONE_);
	  }
	break;

	case ERROR_DONE_:
	case DONE_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    if ((ulTdb().excpTabInsertStmt()) &&
		(narExcepInitialized_))
	      {
		cliInterface()->fetchRowsEpilogue(
		     ulTdb().excpTabInsertStmt(), TRUE);

		restoreCQD("UPD_ABORT_ON_ERROR",
			   currContext->getDiagsArea());

		if (getDiagsArea() &&
                    (getDiagsArea()->getNumber(DgSqlCode::ERROR_) == 0) &&
		    (currContext->getDiagsArea()->getNumber(DgSqlCode::ERROR_) == 0))
		  SQL_EXEC_ClearDiagnostics(NULL);
		
	      }

	    if ((step_ != ERROR_DONE_) &&
		(excpRecNum_ > 0) &&
		(rwrsNumRows_ == 0)&&
                 getDiagsArea())
	      {
		// add a warning to indicate the number of rows inserted into the exception table.
		ComDiagsArea * da = getDiagsArea();
		*da << DgSqlCode(30049)
		    << DgInt0((Lng32)excpRecNum_);
	      }
	      
	    // Return EOF.
	    ex_queue_entry * up_entry = qparent_.up->getTailEntry();
	    
	    up_entry->upState.parentIndex = 
	      pentry_down->downState.parentIndex;
	    
	    up_entry->upState.setMatchNo(0);
	    up_entry->upState.status = ex_queue::Q_NO_DATA;

	    if (getDiagsArea() && 
               (getDiagsArea()->getNumber(DgSqlCode::WARNING_) > 0)) // must be a warning
	      {
		ComDiagsArea *diagsArea = up_entry->getDiagsArea();
		
		if (diagsArea == NULL)
		  diagsArea = 
		    ComDiagsArea::allocate(this->getGlobals()->getDefaultHeap());
                else
                  diagsArea->incrRefCount (); // setDiagsArea call below will decr ref count
		
		if (getDiagsArea())
		  diagsArea->mergeAfter(*getDiagsArea());
		
		up_entry->setDiagsArea (diagsArea);
	      }
	    
	    // insert into parent
	    qparent_.up->insert();
	    
	    changeStep(INITIAL_);
	    qparent_.down->removeHead();
	    
	    return WORK_OK;
	  }
	break;

	} // switch
    } // while

} 

void ExExeUtilUserLoadFastTcb:: changeAndTraceStep(Step newStep, Int32 l)
{
  lastStep_++;

  if (lastStep_ >= NumTraces)
    lastStep_ = 0;
  
  stepTrace_[lastStep_].oldStep_ = step_;
  stepTrace_[lastStep_].changedOnLine_ = l;
  stepTrace_[lastStep_].childDownTail_ = qchild_.down->getTailIndex();
  stepTrace_[lastStep_].childDownHead_ = qchild_.down->getHeadIndex();
  stepTrace_[lastStep_].childUpTail_ = qchild_.up->getTailIndex();
  stepTrace_[lastStep_].childUpHead_ = qchild_.up->getHeadIndex();
  stepTrace_[lastStep_].currentRowNum_ = currentRowNum_;
  stepTrace_[lastStep_].numRqstsToChild_ = numRqstsToChild_;
  step_ = newStep;
  return;
}

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
  ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();
  pentry_down->setDiagsArea(diagsArea);
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


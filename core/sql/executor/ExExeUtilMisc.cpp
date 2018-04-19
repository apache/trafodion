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
 * File:         ExExeUtilMisc.cpp
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
#include  "ExpLOB.h"
#include  "ComRtUtils.h"
#include  "ExStats.h"
#include  "ComSmallDefs.h"
#include <unistd.h>

//////////////////////////////////////////////////////////
// classes defined in this file:
//
// class ExExeUtilFastDelete
//
//////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilFastDeleteTdb::build(ex_globals * glob)
{
  ExExeUtilTcb * exe_util_tcb;

  exe_util_tcb = new(glob->getSpace()) ExExeUtilFastDeleteTcb(*this, glob);
  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilFastDeleteTcb
///////////////////////////////////////////////////////////////
ExExeUtilFastDeleteTcb::ExExeUtilFastDeleteTcb(
     const ComTdbExeUtilFastDelete & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilTcb( exe_util_tdb, NULL, glob),
       fastDelUsingResetEOF_(FALSE),
       xnWasStarted_(FALSE)
{
  // Allocate the private state in each entry of the down queue
  qparent_.down->allocatePstate(this);

  step_ = INITIAL_;
}

ExExeUtilFastDeleteTcb::~ExExeUtilFastDeleteTcb()
{
}

short ExExeUtilFastDeleteTcb::doPurgedataCat(char * stmt)
{
  Lng32 cliRC = 0;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();

  cliRC = holdAndSetCQD("EXE_PARALLEL_PURGEDATA", "OFF");
  if (cliRC < 0)
    {
      cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
      return -1;
    }

  cliRC = cliInterface()->executeImmediate(stmt);
  //  NADELETEBASIC(stmt, getHeap());

  if (cliRC < 0)
    {
      cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
      restoreCQD("EXE_PARALLEL_PURGEDATA");
      return -1;
    }
  
  restoreCQD("EXE_PARALLEL_PURGEDATA");

  return 0;
}

short ExExeUtilFastDeleteTcb::doLabelPurgedata(char * objectName,
					       NABoolean isIndex)
{
  Lng32 cliRC;
  short retcode = 0;

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();

  SQL_EXEC_ClearDiagnostics(NULL);

  char * stmt = NULL;

  // set sqlparserflags to allow special label_purgedata syntax
  masterGlob->getStatement()->getContext()->setSqlParserFlags(0x1); // ALLOW_SPECIALTABLETYPE

  stmt = new(getHeap()) char[4000];

  if (isIndex)
    str_sprintf(stmt, "label_purgedata index_table %s parallel execution on",
		objectName);
  else
    str_sprintf(stmt, "label_purgedata table %s parallel execution on",
		objectName);
    
  cliRC = cliInterface()->executeImmediate(stmt);
  NADELETEBASIC(stmt, getHeap());

  masterGlob->getStatement()->getContext()->resetSqlParserFlags(0x1);

  if (cliRC < 0)
    {
      cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
      retcode = -1;
      goto cleanUpAndReturn;
    }
  
cleanUpAndReturn:

  if (retcode == 0)
    {
      if (NOT isIndex)
	// Inject error to cause fastdelete of table to fail.
	retcode = injectError("13");
      else
	// Inject error to cause fastdelete of index to fail.
	retcode = injectError("14");
    }

  if (retcode < 0)
    {
      strcpy(failReason_, "Error during fast delete using reset EOF operation.");
    }

  return retcode;
}

short ExExeUtilFastDeleteTcb::doFastDelete(char * objectName,
					   NABoolean isIndex,
					   NABoolean fastDelUsingResetEOF)
{
  Lng32 cliRC;
  short retcode = 0;
  char * stmt = NULL;
  Lng32 pneBufLen = 0;

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();

  SQL_EXEC_ClearDiagnostics(NULL);

  // turn ON parallelism to force execution using ESPs.
  // But first, hold and save the current value for attempt_esp_parallelism.
  retcode = holdAndSetCQD("ATTEMPT_ESP_PARALLELISM", "ON");
  if (retcode < 0)
    {
      goto cleanUpAndReturn_restore_CQS;
    }


  // Issue a control query shape to use ESPs.
  // 'Hold' any previously issued CQS.
  cliRC = 
    cliInterface()->
    executeImmediate("control query shape hold;");
  if (cliRC < 0)
    {
      cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
      retcode = -1;
      goto cleanUpAndReturn;
    }

  cliRC = 
    cliInterface()->executeImmediate("control query shape esp_exchange(cut);");
  if (cliRC < 0)
    {
      cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
      retcode = -1;
      goto cleanUpAndReturn;
    }
  
  if (fdTdb().isMV())
    {
      cliRC = 
	cliInterface()->
	executeImmediate("control query default mv_internal_ignore_uninitialized 'ON';");
      if (cliRC < 0)
	{
          cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
	  retcode = -1;
	  goto cleanUpAndReturn;
	}
    }

  stmt = new(getHeap()) char[strlen("delete ")
			    + (fastDelUsingResetEOF ? strlen("using purgedata ")
			       : strlen("no purgedata "))
			    + strlen("from table ( ") 
			    + (isIndex ? strlen("index_table ") 
			       : strlen("table "))
			    + strlen(objectName) + 
			    + strlen (" ) ") 
			    + 200];
  strcpy(stmt, "delete ");
  if (fastDelUsingResetEOF)
    strcat(stmt, "using purgedata ");
  else
    strcat(stmt, "no purgedata ");
  strcat(stmt, "from table ( ");
  if (isIndex)
    strcat(stmt, "index_table ");
  else
    strcat(stmt, "table ");
  
  strcat(stmt, objectName);
  strcat(stmt, " ) ;");

  // set sqlparserflags to allow special INDEX_TABLE name
  if (isIndex)
    masterGlob->getStatement()->getContext()->setSqlParserFlags(0x1); // ALLOW_SPECIALTABLETYPE
  
  cliRC = cliInterface()->executeImmediate(stmt,NULL,NULL,TRUE,NULL,TRUE);

  if (isIndex)
   masterGlob->getStatement()->getContext()->resetSqlParserFlags(0x1); // ALLOW_SPECIALTABLETYPE

  if (cliRC < 0)
    {
      cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
      retcode = -1;
      goto cleanUpAndReturn;
    }
  
cleanUpAndReturn:

  // restore original value for attempt_esp_parallelism
  cliRC = restoreCQD("attempt_esp_parallelism");


cleanUpAndReturn_restore_CQS:

  // restore original CQS
  cliRC = 
    cliInterface()->
    executeImmediate("control query shape restore;");

  if (fdTdb().isMV())
    {
      cliRC = 
	cliInterface()->
	executeImmediate("control query default mv_internal_ignore_uninitialized 'OFF';");
    }

  if (retcode == 0)
    {
      if (NOT isIndex)
	// Inject error to cause fastdelete of table to fail.
	retcode = injectError("13");
      else
	// Inject error to cause fastdelete of index to fail.
	retcode = injectError("14");
    }

  if (retcode < 0)
    {
      if (fastDelUsingResetEOF)
	strcpy(failReason_, "Error during fast delete using reset EOF operation.");
      else
	strcpy(failReason_, "Error during fast delete operation.");
    }

  return retcode;
}

short ExExeUtilFastDeleteTcb::injectError(const char * val)
{
  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();

  char * e1 = masterGlob->getCliGlobals()->getEnv("SQLMX_TEST_POINT");
  char * e2 = masterGlob->getCliGlobals()->getEnv("SQLMX_PPD_ERR_TEST_POINT");
  if (((e1) && (strcmp(e1, val) == 0)) ||
      ((e2) && (strcmp(e2, val) == 0)))
    {
      Lng32 errNumParam = ((Lng32)str_atoi(val, strlen(val)));
      ExRaiseSqlError(getHeap(), &diagsArea_, -EXE_ERROR_INJECTED,
          &errNumParam, NULL, NULL,   
	  (e1 ? "SQLMX_TEST_POINT" : "SQLMX_PPD_ERR_TEST_POINT"));
      return -EXE_ERROR_INJECTED;
    }
  return 0;
}


short ExExeUtilFastDeleteTcb::purgedataLOBs()
{
  // purgedata from lobs for this table
  if (fdTdb().numLOBs() > 0)
    {
      for (Lng32 i = 1; i <= fdTdb().numLOBs(); i++)
	{
	  Lng32 rc = ExpLOBoper::purgedataLOB
	    (NULL, NULL, fdTdb().getObjectUID(), fdTdb().getLOBnum(i));
	}
    }

  return 0;
}


//////////////////////////////////////////////////////
// work() for ExExePurgedataUtilTcb
//////////////////////////////////////////////////////
short ExExeUtilFastDeleteTcb::work()
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

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ExTransaction *ta = masterGlob->getStatement()->getContext()->getTransaction();

  char buf[4000];

  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
	    fastDelUsingResetEOF_ = FALSE;
	    xnWasStarted_ = FALSE;
	    parallelDeleteDone_ = FALSE;
	    strcpy(failReason_, " ");

	    if (fdTdb().doPurgedataCat())
	      step_ = PURGEDATA_CAT_;
	    else if (fdTdb().doParallelDelete())
	      {
		if (NOT ta->xnInProgress())
		  {
		    cliRC = cliInterface()->beginWork();
		    if (cliRC < 0)
		      {
                        cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
			step_ = ERROR_;
			break;
		      }
		    
		    xnWasStarted_ = TRUE;
		  }

		step_ = FASTDEL_TABLE_;

		parallelDeleteDone_ = TRUE;
	      }
	    else if (NOT ta->xnInProgress())
	      {
		fastDelUsingResetEOF_ = TRUE;
		step_ = ADD_DDL_LOCK_;
	      }
	    else
	      {
		// User transaction is in progress.
		// We need to do fast delete without purgedata mode unless
		// it has been turned off.
		if (fdTdb().doParallelDeleteIfXn())
		  {
		    parallelDeleteDone_ = TRUE;
		    step_ = FASTDEL_TABLE_;
		  }
		else
		  step_ = PURGEDATA_CAT_;
	      }

	    if (step_ != PURGEDATA_CAT_)
	      {
		if (cliRC < 0)
		  {
                    cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		    step_ = ERROR_;
		    break;
		  }
	      }
	  }
	break;

	case ADD_DDL_LOCK_:
	  {
	    cliRC = cliInterface()->beginWork();
	    if (cliRC < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		step_ = ERROR_;
		break;
	      }

	    xnWasStarted_ = TRUE;

	    // first drop this ddl lock, if it exists. Ignore errors.
	    rc = alterDDLLock(FALSE, fdTdb().getTableName(), failReason_,
			      fdTdb().isMV(), COM_UTIL_PURGEDATA);
	    if (rc < 0)
	      {
		SQL_EXEC_ClearDiagnostics(NULL);
		
		strcpy(failReason_, " ");

		rc = 0;

		// if this table was offline, and there was no ddl lock,
		// let catman purgedata handle it.
		// In this case, don't want to change the table back to
		// online which is what will happen if exe purgedata is
		// used.

		if (fdTdb().offlineTable())
		  {
		    // Abort transaction which was started.
		    // Ignore errors, and clear diags area.
		    if ((xnWasStarted_) &&
			(ta->xnInProgress()))
		      {
			cliRC = cliInterface()->rollbackWork();
			
			SQL_EXEC_ClearDiagnostics(NULL);
			
			xnWasStarted_ = FALSE;
		      }
		    
		    step_ = PURGEDATA_CAT_;
		    break;
		  }
	      }

	    // now add a ddl lock.
	    rc = alterDDLLock(TRUE, fdTdb().getTableName(), failReason_,
			      fdTdb().isMV(), COM_UTIL_PURGEDATA);
	    if (rc == 0)
	      rc = injectError("10");
	    if (rc < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		// could not acquire a ddl lock.
		// try to purgedata using catman sequential purgedata.

		// Abort transaction which was started.
		// Ignore errors, and clear diags area.
		if ((xnWasStarted_) &&
		    (ta->xnInProgress()))
		  {
		    cliRC = cliInterface()->rollbackWork();
		    
		    SQL_EXEC_ClearDiagnostics(NULL);
		    
		    xnWasStarted_ = FALSE;
		  }

		// see if catman seq purgedata can fix this.
		step_ = PURGEDATA_CAT_;
		break;
	      }

	    step_ = MAKE_OBJECT_OFFLINE_;
	  }
	break;

	case MAKE_OBJECT_OFFLINE_:
	  {
	    // make object offline
	    rc = alterObjectState(FALSE, fdTdb().getTableName(), 
				  failReason_, TRUE);
	    NABoolean rollbackNoPDErr = FALSE;
	    if (rc == 0)
	      {
		rc = injectError("11");
		if (rc == 0)
		  {
		    rc = injectError("18");
		    if (rc < 0)
		      rollbackNoPDErr = TRUE;
		  }
	      }
	    if (rc < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		// security validation failed
		if ((getDiagsArea()->contains(-1017)) ||
		    (rollbackNoPDErr))
		  step_ = ROLLBACK_WORK_AND_NO_PD_ERROR_;
		else
		  step_ = ROLLBACK_WORK_AND_ERROR_;
		break;
	      }

	    step_ = SET_CORRUPT_BIT_;
	  }
	break;

	case SET_CORRUPT_BIT_:
	  {
	    // set the corrupt bit in the label of table and all indices
	    rc = alterCorruptBit(1, fdTdb().getTableName(), failReason_,
				 fdTdb().getIndexList());
	    if (rc == 0)
	      rc = injectError("12");
	    if (rc < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		step_ = ROLLBACK_WORK_AND_ERROR_;
		break;
	      }

	    if ((xnWasStarted_) &&
		(ta->xnInProgress()))
	      {
		cliRC = cliInterface()->commitWork();
		
		xnWasStarted_ = FALSE;
		
		if (cliRC < 0)
		  {
		    strcpy(failReason_, "Error during commit work.");

                    cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		    step_ = ROLLBACK_WORK_AND_ERROR_;
		    break;
		  }
	      }

	    // inject error to test RECOVER.
	    // DDL lock has been acquired and object has been made offline
	    // but operation has not yet begun.
	    if (injectError("1") || injectError("2"))
	      {
		// kill mxcmp
		strcpy(buf, "SELECT TESTEXIT;");
		cliRC = cliInterface()->executeImmediate(buf);
		if (cliRC < 0)
                  cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		  
		strcpy(failReason_, " ");

		step_ = ROLLBACK_WORK_AND_ERROR_;
		break;
	      }

	    // now start parallel purgedata
	    step_ = BEGIN_WORK_;
	  }
	break;

	case BEGIN_WORK_:
	  {
	    cliRC = cliInterface()->beginWork();
	    if (cliRC < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		step_ = ERROR_;
		break;
	      }

	    xnWasStarted_ = TRUE;
	    step_ = FASTDEL_TABLE_;
	  }
	break;

	case FASTDEL_TABLE_:
	  {
	    if ((fdTdb().doLabelPurgedata()) &&
		(fastDelUsingResetEOF_))
	      rc = doLabelPurgedata(fdTdb().getTableName(), 
				    FALSE);
	    else
	      rc = doFastDelete(fdTdb().getTableName(), 
				FALSE, fastDelUsingResetEOF_);

	    if (rc)
	      {
		if (fastDelUsingResetEOF_)
		  step_ = KILL_MXCMP_AND_ERROR_;
		else
		  step_ = ROLLBACK_WORK_AND_ERROR_;
		break;
	      }
	    
	    if (fdTdb().getIndexList())
	      {
		fdTdb().getIndexList()->position();
		step_ = FASTDEL_INDEX_;
	      }
	    else 
	      step_ = COMMIT_WORK_;
	  }
	break;

	case FASTDEL_INDEX_:
	  {
	    if (fdTdb().getIndexList()->atEnd())
	      {
		step_ = COMMIT_WORK_;
		break;
	      }

	    char * indexName = (char*)fdTdb().getIndexList()->getNext();
	    if ((fdTdb().doLabelPurgedata()) &&
		(fastDelUsingResetEOF_))
	      rc = doLabelPurgedata(indexName, 
				    TRUE);
	    else
	      rc = doFastDelete(indexName, TRUE,
				fastDelUsingResetEOF_);
	    if (rc)
	      {
		if (fastDelUsingResetEOF_)
		  step_ = KILL_MXCMP_AND_ERROR_;
		else
		  step_ = ROLLBACK_WORK_AND_ERROR_;
		break;
	      }
	  }
	break;

	case KILL_MXCMP_AND_ERROR_:
	  {
	    // Abort transaction which was started.
	    // Ignore errors, and clear diags area.
	    if ((xnWasStarted_) &&
		(ta->xnInProgress()))
	      {
		cliRC = cliInterface()->rollbackWork();
		
		SQL_EXEC_ClearDiagnostics(NULL);

		xnWasStarted_ = FALSE;
	      }

	    // kill mxcmp so a subsequent recover operation
	    // can succeed. Ignore errors.
	    strcpy(buf, "SELECT TESTEXIT;");
	    cliRC = cliInterface()->executeImmediate(buf);
	    SQL_EXEC_ClearDiagnostics(NULL);

	    step_ = ROLLBACK_WORK_AND_ERROR_;
	  }
	break;

	case ROLLBACK_WORK_AND_ERROR_:
	case ROLLBACK_WORK_AND_NO_PD_ERROR_:
	  {
	    // We come here only if fastDeleteUsingResetEOF failed.
	    // Abort transaction which was started.
	    // Ignore errors, and clear diags area.
	    if ((xnWasStarted_) &&
		(ta->xnInProgress()))
	      {
		cliRC = cliInterface()->rollbackWork();
		
		SQL_EXEC_ClearDiagnostics(NULL);

		xnWasStarted_ = FALSE;
	      }

	    if (step_ == ROLLBACK_WORK_AND_ERROR_)
	      {
		ComDiagsArea * diagsArea = getDiagsArea();
		// convert all errors into warnings
		NegateAllErrors(diagsArea);
                ExRaiseSqlError(getHeap(), &diagsArea_, -EXE_PARALLEL_PURGEDATA_FAILED,
                    NULL, NULL, NULL,
		    failReason_);
	      }

	    step_ = ERROR_;
	  }
	break;

	case COMMIT_WORK_:
	  {
	    if ((xnWasStarted_) &&
		(ta->xnInProgress()))
	      {
		cliRC = cliInterface()->commitWork();

		xnWasStarted_ = FALSE;
	      }

	    if (injectError("3"))
	      {
		// kill mxcmp
		strcpy(buf, "SELECT TESTEXIT;");
		cliInterface()->executeImmediate(buf);
		if (cliRC < 0)
                  cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		
		step_ = ERROR_;
		break;
	      }

	    if (parallelDeleteDone_) //fdTdb().doParallelDelete())
	      {
		// raise a warning that parallel purgedata was performed.
		if (fdTdb().returnPurgedataWarn())
		  {
                    ExRaiseSqlError(getHeap(), &diagsArea_, EXE_PURGEDATA_CAT,
                        NULL, NULL, NULL,
			"Parallel", "");
		  }
		step_ = DONE_;
	      }
	    else
	      step_ = RESET_CORRUPT_BIT_;
	  }
	break;

	case RESET_CORRUPT_BIT_:
	  {
	    cliRC = cliInterface()->beginWork();
	    if (cliRC < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		step_ = ERROR_;
		break;
	      }

	    xnWasStarted_ = TRUE;

	    // reset the corrupt bit in the label of table and all indices.
	    rc = alterCorruptBit(0, fdTdb().getTableName(), failReason_,
				 fdTdb().getIndexList());
	    if (rc == 0)
	      rc = injectError("15");

	    if (rc < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		step_ = KILL_MXCMP_AND_ERROR_;
		break;
	      }

	    step_ = MAKE_OBJECT_ONLINE_;
	  }
	break;

	case MAKE_OBJECT_ONLINE_:
	  {
	    rc = alterObjectState(TRUE, fdTdb().getTableName(), 
				  failReason_, TRUE);
	    if (rc == 0)
	      rc = injectError("16");
	    if (rc < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		step_ = KILL_MXCMP_AND_ERROR_;
		break;
	      }
	    
	    step_ = DROP_DDL_LOCK_;
	  }
	break;

	case DROP_DDL_LOCK_:
	  {
	    if (fastDelUsingResetEOF_)
	      {
		rc = alterDDLLock(FALSE, fdTdb().getTableName(), failReason_,
				  fdTdb().isMV(), COM_UTIL_PURGEDATA);
		if (rc == 0)
		  rc = injectError("17");
		if (rc < 0)
		  {
                    cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		    step_ = KILL_MXCMP_AND_ERROR_;
		    break;
		  }
	      }

	    cliRC = cliInterface()->commitWork();
	    if (cliRC < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		step_ = ERROR_;
		break;
	      }
	    
	    step_ = DONE_;

	    // raise a warning that parallel purgedata was performed.
	    if (fdTdb().returnPurgedataWarn())
	      {
                ExRaiseSqlError(getHeap(), &diagsArea_, EXE_PURGEDATA_CAT,
                    NULL, NULL, NULL,
		    "Parallel", "");
	      }
	  }
	break;

	case PURGEDATA_CAT_:
	  {
	    if ((getDiagsArea()) &&
		(getDiagsArea()->getNumber(DgSqlCode::ERROR_) > 0))
	      {
		// convert all errors into warnings
		NegateAllErrors(getDiagsArea());
	      }

	    // raise a warning that regular purgedata is being performed,
	    // either because the query/object didn't meet the criteria
	    // for parallel purgedata, or because parallel purgedata failed.
	    if (fdTdb().returnPurgedataWarn())
	      {
		if (fdTdb().doPurgedataCat())
                  ExRaiseSqlError(getHeap(), &diagsArea_, EXE_PURGEDATA_CAT,
                     NULL, NULL, NULL,
                     "Regular", "Reason: Query or the object did not meet the criteria for parallel purgedata.");
		else
                  ExRaiseSqlError(getHeap(), &diagsArea_, EXE_PURGEDATA_CAT,
                     NULL, NULL, NULL,
                     "Regular", "Reason: Parallel purgedata failed.");
	      }

	    rc = doPurgedataCat(fdTdb().purgedataStmt());
	    if (rc)
	      step_ = ERROR_;
	    else
	      step_ = DONE_;
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

	    getDiagsArea()->clear();

	    // insert into parent
	    qparent_.up->insert();
	    
	    step_ = DONE_;
	  }
	break;

	case DONE_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;


	    // restore cqd and ignore errors
	    SQL_EXEC_ClearDiagnostics(NULL);
	    
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
	    
	    pstate.matches_ = 0;
	    step_ = INITIAL_;
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
ex_tcb_private_state * ExExeUtilFastDeleteTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExExeUtilFastDeletePrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExExeUtilFastDeletePrivateState::ExExeUtilFastDeletePrivateState()
{
}

ExExeUtilFastDeletePrivateState::~ExExeUtilFastDeletePrivateState()
{
};

///////////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilLongRunningTdb::build(ex_globals * glob)
{
  ExExeUtilLongRunningTcb * exe_util_tcb;

  exe_util_tcb = 
    new(glob->getSpace()) ExExeUtilLongRunningTcb(*this, glob);

  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilLongRunningTcb
///////////////////////////////////////////////////////////////
ExExeUtilLongRunningTcb::ExExeUtilLongRunningTcb(
     const ComTdbExeUtilLongRunning & exe_util_tdb,
     ex_globals * glob)
  : ExExeUtilTcb( exe_util_tdb, NULL, glob) 
{
  // Allocate the private state in each entry of the down queue
  qparent_.down->allocatePstate(this);

  step_ = INITIAL_;
  rowsDeleted_ = 0;
  transactions_ = 0;
  initial_ = 1;
  initialOutputVarPtrList_ = NULL;
  continuingOutputVarPtrList_ = NULL;
  lruStmtAndPartInfo_= NULL;
  lruStmtWithCKAndPartInfo_= NULL;
  currTransaction_ = NULL;
}

ExExeUtilLongRunningTcb::~ExExeUtilLongRunningTcb()
{
}

Int32 ExExeUtilLongRunningTcb::fixup()
{
    return ex_tcb::fixup();
}


short ExExeUtilLongRunningTcb::doLongRunning()
{
  Lng32 cliRC =0;
  short retcode = 0;
  NABoolean xnAlreadyStarted = FALSE;
     
  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate =
    *((ExExeUtilPrivateState*) pentry_down->pstate);


  CliGlobals *cliGlobals = 0;
  cliGlobals = GetCliGlobals();

  ex_assert(cliGlobals != NULL, "Cli globals is NULL - should have been allocated already");

  if (cliGlobals->isESPProcess())
  {
     if (!currTransaction_)
        currTransaction_ = new (getHeap()) ExTransaction (cliGlobals, getHeap());
  }
  else  // in master executor
  {
     currTransaction_ = masterGlob->getStatement()->getContext()->getTransaction();
  }


  if (currTransaction_->xnInProgress())
  {
     xnAlreadyStarted = TRUE;
  }

  // cannot run LRU when a user transaction is in progress
  if (xnAlreadyStarted)
  {
     ExHandleErrors(qparent_,
         pentry_down,
         0,
         getGlobals(),
         NULL,
         (ExeErrorCode)(-8603),
         NULL,
         exeUtilTdb().getTableName()
         );
     return (-8603);
  }

  SQL_EXEC_ClearDiagnostics(NULL);

  // no Xn in progress. Start one.
  cliRC = currTransaction_->beginTransaction ();

  if (cliRC < 0)
  {
      ExHandleErrors(qparent_,
        pentry_down,
        0,
        getGlobals(),
        NULL,
        (ExeErrorCode)(cliRC),
        NULL,
        exeUtilTdb().getTableName()
        );
       return (short) cliRC;
  }

  retcode = executeLongRunningQuery();

  // Rollback the transaction, if there is an error.
  if (retcode < 0)
  {
     // rollback the transaction
     cliRC = currTransaction_->rollbackTransaction ();

     return retcode;
  }
  else 
  {
    // commit the transaction
    cliRC = currTransaction_->commitTransaction ();

    if (cliRC < 0)
    {
       ExHandleErrors(qparent_,
           pentry_down,
           0,
           getGlobals(),
           NULL,
          (ExeErrorCode)(cliRC),
           NULL,
           exeUtilTdb().getTableName()
           );

       return short(cliRC);
    }

    addTransactionCount();
  }

  return retcode;
}


//////////////////////////////////////////////////////
// work() for ExExeUtilLongRunningTcb
//////////////////////////////////////////////////////
short ExExeUtilLongRunningTcb::work()
{
  short rc = 0;
  Lng32 cliRC = 0;
  Int64 rowsDeleted = 0;
  Int64 transactions = 0;

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

  // Get the globals stucture of the ESP if this is an ESP
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExEspStmtGlobals *espGlob = exeGlob->castToExEspStmtGlobals();


  Int32 espNum = 1;
 
  // this is an ESP?
  if (espGlob != NULL)
  {
     espNum = (Int32) espGlob->getMyInstanceNumber();
  }

  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
	    step_ = LONG_RUNNING_;
	  }
	break;

	
	case LONG_RUNNING_:
	  {
	    rc = doLongRunning();
	    if ((rc < 0) || (rc == 100)) 
	      {
		finalizeDoLongRunning();
		if (rc <0)
		  step_ = ERROR_;
		else                       // rc == 100 - done with all the transactions.
		  step_ = DONE_;
	      }

            // continue in LONG_RUNNING_ state if (rc >= 0) - success and warning.
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

	    // before sending the Q_NO_DATA, send the rowcount as well thro'
	    // the diagsArea.
	    
	    getDiagsArea()->setRowCount(getRowsDeleted());

            ComDiagsArea *diagsArea = getDiagAreaFromUpQueueTail();
            if (lrTdb().longRunningQueryPlan())
            {
               (*diagsArea) << DgSqlCode(8450)
                            << DgString0((char*)exeUtilTdb().getTableName())
                            << DgInt0(espNum)
                            << DgInt1((Lng32)getTransactionCount());
            }
	    
	    // insert into parent
	    qparent_.up->insert();
	    
	    //pstate.matches_ = 0;
	    // reset the parameters.
	    step_ = INITIAL_;
	    transactions_ = 0;
	    rowsDeleted_ = 0;
	    initial_ = 1;
	    
	    // clear diags if any
            if (getDiagsArea())
            {
                getDiagsArea()->clear();
            }
    
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
	    // get rows deleted so far.
	    getDiagsArea()->setRowCount(getRowsDeleted());
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

	    // clear diags if any, since we already sent the information
	    // up and don't want to send it again as part of DONE_
            if (getDiagsArea())
            {
	      rowsDeleted_ = 0;
	      getDiagsArea()->clear();
            }
	    step_ = DONE_;
	  }
	break;

	} // switch
    } // while

}


void ExExeUtilLongRunningTcb::registerSubtasks()
{

  // register events for parent queue
  ex_tcb::registerSubtasks();
}

ComDiagsArea *ExExeUtilLongRunningTcb::getDiagAreaFromUpQueueTail()
{
  ex_queue_entry * up_entry = qparent_.up->getTailEntry();
  ComDiagsArea *diagsArea = up_entry->getDiagsArea();

  if (diagsArea == NULL)
     diagsArea = ComDiagsArea::allocate(this->getGlobals()->getDefaultHeap());
  else
    diagsArea->incrRefCount (); // setDiagsArea call below will decr ref count

  // this is the side-effect of this function. Merge in this object's
  // diagsarea.
  if (getDiagsArea())
    diagsArea->mergeAfter(*getDiagsArea());

  up_entry->setDiagsArea (diagsArea);

  return diagsArea;
}

short ExExeUtilLongRunningTcb::executeLongRunningQuery()
{
  Lng32 rc = 0;

  if (getInitial())
    {
      Lng32 cliRC = 
      cliInterface()->executeImmediate(
        "control query default HIST_ON_DEMAND_STATS_SIZE '0'");
      if (cliRC < 0) 
        {
          cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
	  return cliRC;
        }
      

      // Perform the initial processing
      short rtc = processInitial(rc);
      if ((rc != 0) && (rc != 100))
        {
          cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
        }

      cliRC = 
        cliInterface()->executeImmediate(
            "control query default HIST_ON_DEMAND_STATS_SIZE 'RESET'");
      if (cliRC < 0) 
        {
          cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
	  return cliRC;
        }
      

#ifdef _DEBUG
      if (lrTdb().longRunningQueryPlan()) {

          Int32 bufSize = 100 + 
                strlen(lruStmtAndPartInfo_)+ strlen(lruStmtWithCKAndPartInfo_);
          char* lruQPInfo = new (getHeap()) char[bufSize];

          // str_printf() does not support %ld. Use %d instead.
          str_sprintf(lruQPInfo, 
	             "Queries to be processed: \n\n%s\n\n%s\n\n\n Initial rows deleted: %ld",
                      lruStmtAndPartInfo_,
                      lruStmtWithCKAndPartInfo_,
                      getRowsDeleted());

          ComDiagsArea * diagsArea = getDiagAreaFromUpQueueTail();
          (*diagsArea) << DgSqlCode(8427) << DgString0(lruQPInfo);

          NADELETEBASIC(lruQPInfo, getHeap());

      }
#endif

      setInitial(0);
    
    }
  else
    {
      // Perform the continuing processing
      short rtc = processContinuing(rc);
      if ((rc != 0) && (rc != 100))
        {
          cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
        }

#ifdef _DEBUG
      if ((rc == 100 || rc > 0) && lrTdb().longRunningQueryPlan()) {

        char lruQPInfo [100];
        str_sprintf(lruQPInfo, "Total rows deleted: %ld\n\n", getRowsDeleted());

        ComDiagsArea * diagsArea = getDiagAreaFromUpQueueTail();
        (*diagsArea) << DgSqlCode(8427) << DgString0(lruQPInfo);
      }
#endif
    }
  

  return (short )rc;
}

short ExExeUtilLongRunningTcb::finalizeDoLongRunning() 
{

  // Close the statements
  Lng32 rc = cliInterface()->prepareAndExecRowsEpilogue();

  NADELETE(initialOutputVarPtrList_, Queue, getHeap());
  NADELETE(continuingOutputVarPtrList_, Queue, getHeap());
  NADELETEBASIC(lruStmtAndPartInfo_, getHeap());
  NADELETEBASIC(lruStmtWithCKAndPartInfo_, getHeap());

  // if this is an ESP, deallocate transaction
  CliGlobals *cliGlobals = GetCliGlobals();
  if (cliGlobals->isESPProcess())
  {
     NADELETEBASIC(currTransaction_, getHeap());
     currTransaction_ = NULL;
  }

  return 0;
}

short ExExeUtilLongRunningTcb::processInitial(Lng32 &rc)
{

  Int64 rowsAffected = 0;

  setInitialOutputVarPtrList(new(getHeap()) Queue(getHeap()));
  setContinuingOutputVarPtrList(new(getHeap()) Queue(getHeap()));

  lruStmtAndPartInfo_ = new(getHeap()) char[(UInt32)(lrTdb().getLruStmtLen() + lrTdb().getPredicateLen() + 10)];

  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExEspStmtGlobals *espGlob = exeGlob->castToExEspStmtGlobals();

  ContextCli *currContext = NULL;
  CliGlobals *cliGlobals = NULL;

  if (espGlob)
  {
    cliGlobals = GetCliGlobals();
    currContext = cliGlobals->currContext();
  }
  else
  {
     currContext = exeGlob->castToExMasterStmtGlobals()->getStatement()->getContext();
  }

  Int32 espNum = 1;
  
  // we are executing inside an ESP go ahead and set the partition number to the 
  // ESP instance number. The added one is because ESP instances are 0 based
  if (espGlob)
  {
     espNum = (Int32) espGlob->getMyInstanceNumber() + 1;
  }

  if (lrTdb().getPredicate() != NULL)
      str_sprintf(lruStmtAndPartInfo_, 
	      lrTdb().getLruStmt(), 
	      espNum,
              lrTdb().getPredicate());
  else
      str_sprintf(lruStmtAndPartInfo_, 
	      lrTdb().getLruStmt(), 
	      espNum);

  lruStmtWithCKAndPartInfo_ = new(getHeap()) char[(UInt32)(lrTdb().getLruStmtWithCKLen() + lrTdb().getPredicateLen() + 10)];

  if (lrTdb().getPredicate() != NULL)
     str_sprintf(lruStmtWithCKAndPartInfo_, 
	      lrTdb().getLruStmtWithCK(), 
	      espNum,
              lrTdb().getPredicate());
  else
     str_sprintf(lruStmtWithCKAndPartInfo_, 
	      lrTdb().getLruStmtWithCK(), 
	      espNum);

  // All internal queries issued from CliInterface assume that
  // they are in ISO_MAPPING.
  // For LongRunning we need to send the actual ISO_MAPPING.
  // Save it and restore after the prepare

  Int32 savedIsoMapping = 
    cliInterface()->getIsoMapping();

  cliInterface()->setIsoMapping
    (currContext->getSessionDefaults()->getIsoMappingEnum());

  // If the table we are deleting from is an IUD log table,
  // we need to set the parserflags to accept the special
  // table type and the quoted column names
  if (lrTdb().useParserflags())
    currContext->setSqlParserFlags(0x3);

  rc = cliInterface()->prepareAndExecRowsPrologue(lruStmtAndPartInfo_,
						  lruStmtWithCKAndPartInfo_, 
                                                  getInitialOutputVarPtrList(),
                                                  getContinuingOutputVarPtrList(),
                                                  rowsAffected);

  cliInterface()->setIsoMapping(savedIsoMapping);

  if (rc < 0)
    {
      return -1;
    }

  if (rc >= 0 && rowsAffected > 0)
    addRowsDeleted(rowsAffected);
 
  return 0;
}

short ExExeUtilLongRunningTcb::processContinuing(Lng32 &rc) 
{

  Int64 rowsAffected = 0;

  rc = cliInterface()->execContinuingRows(getContinuingOutputVarPtrList(),
                                          rowsAffected);

  if (rc < 0)
    {
      return -1;
    }

  if (rowsAffected > 0)
    addRowsDeleted(rowsAffected);

#ifdef _DEBUG

   if ((rowsAffected > 0) && lrTdb().longRunningQueryPlan()) {

        char lruQPInfo[100];

        str_sprintf(lruQPInfo, "Continuing rows deleted: %ld\n\n", 
                                rowsAffected);

        ComDiagsArea * diagsArea = getDiagAreaFromUpQueueTail();
        (*diagsArea) << DgSqlCode(8427) << DgString0(lruQPInfo);
   }
#endif
 
  return 0;
}


////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ExExeUtilLongRunningTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExExeUtilLongRunningPrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExExeUtilLongRunningPrivateState::ExExeUtilLongRunningPrivateState()
{
}

ExExeUtilLongRunningPrivateState::~ExExeUtilLongRunningPrivateState()
{
};

///////////////////////////////////////////////////////////////////
// class ExExeUtilAQRTdb
///////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilAQRTdb::build(ex_globals * glob)
{
  ExExeUtilAQRTcb * exe_util_tcb;

  exe_util_tcb = new(glob->getSpace()) ExExeUtilAQRTcb(*this, glob);

  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

ExExeUtilAQRTcb::ExExeUtilAQRTcb(const ComTdbExeUtilAQR & exe_util_tdb,
				 ex_globals * glob)
     : ExExeUtilTcb(exe_util_tdb, NULL, glob),
       step_(EMPTY_)
{
}

short ExExeUtilAQRTcb::work()
{
  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExExeUtilPrivateState & pstate =
    *((ExExeUtilPrivateState*) pentry_down->pstate);

  ContextCli *currContext =
      getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals()->
        getStatement()->getContext();
    
  AQRInfo * aqr = currContext->aqrInfo();

  while (1)
    {
      switch (step_)
	{
	case EMPTY_:
	  {
	    if (aqrTdb().getTask() == ComTdbExeUtilAQR::GET_)
	      {
		// GET aqr entries and display them
		aqr->position();
		
		step_ = RETURN_HEADER_;
	      }
	    else
	      {
		step_ = SET_ENTRY_;
	      }
	  }
	break;

	case SET_ENTRY_:
	  {
	    if (aqr->setAQREntry(aqrTdb().getTask(), 
				 aqrTdb().sqlcode_, aqrTdb().nskcode_,
				 aqrTdb().retries_, aqrTdb().delay_,
				 aqrTdb().type_, 
				 0, NULL, 0, 0))
	      {
		// error
		step_ = HANDLE_ERROR_;
		break;
	      }

	    step_ = DONE_;
	  }
	break;

	case RETURN_HEADER_:
	  {
	    // if no room in up queue for 4 display rows, 
	    // won't be able to return data/status.
	    // Come back later.
	    if ((qparent_.up->getSize() - qparent_.up->getLength()) < 5)
	      return -1;
	    
	    moveRowToUpQueue("  ");
	    moveRowToUpQueue("  SQLCODE    PLATFORMERROR    RETRIES      DELAY       TYPE ");
	    moveRowToUpQueue("============================================================");
	    moveRowToUpQueue("  ");
	    
	    step_ = RETURNING_ENTRY_;
	  }
	break;

	case RETURNING_ENTRY_:
	  {
	    // if no room in up queue, won't be able to return data/status.
	    // Come back later.
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    Lng32 sqlcode, nskcode, retries, delay, type, intAQR;
	    Lng32 eof = 0;
	    eof = 
	      aqr->getNextAQREntry(sqlcode, nskcode, retries, delay, type,
				   intAQR);
	    if (eof)
	      {
		step_ = DONE_;
		break;
	      }

	    // internal aqr entry, no need to display
	    if (intAQR)
	      break;

	    char formattedStr[400];
	    str_sprintf(formattedStr, "%9d  %13d  %9d  %9d  %9d",
			sqlcode, nskcode, retries, delay, type);

	    moveRowToUpQueue(formattedStr);
	  }
	break;

	case HANDLE_ERROR_:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    ExHandleErrors(qparent_,
			   pentry_down,
			   0,
			   getGlobals(),
			   NULL,
			   (ExeErrorCode)8931,
			   NULL,
			   NULL
			   );

	    step_ = DONE_;
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

///////////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilPopulateInMemStatsTdb::build(ex_globals * glob)
{
  ex_tcb * exe_util_tcb;

  exe_util_tcb = new(glob->getSpace()) ExExeUtilPopulateInMemStatsTcb(*this, glob);

  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilPopulateInMemStatsTcb
///////////////////////////////////////////////////////////////
ExExeUtilPopulateInMemStatsTcb::ExExeUtilPopulateInMemStatsTcb(
     const ComTdbExeUtilPopulateInMemStats & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilTcb( exe_util_tdb, NULL, glob)
{
  // Allocate the private state in each entry of the down queue
  qparent_.down->allocatePstate(this);

  step_ = INITIAL_;
}

ExExeUtilPopulateInMemStatsTcb::~ExExeUtilPopulateInMemStatsTcb()
{
}

//////////////////////////////////////////////////////
// work() for ExExeUtilPopulateInMemStatsTcb
//////////////////////////////////////////////////////
static const QueryString deleteStatsQuery[] =
{
  {" delete from %s "},
  {"  where table_uid = %ld"}
};

static const QueryString populateHistogramsStatsQuery[] =
{
  {" insert into %s "},
  {" select  "},
  {"   %ld, "},
  {"   histogram_id, "},
  {"   col_position, column_number, colcount, interval_count, rowcount, "},
  {"   total_uec, stats_time, low_value, high_value, read_time, read_count, "},
  {"   sample_secs, col_secs, sample_percent, "},
  {"   cv, reason, v1, v2, v3, v4, v5, v6 "},
  {" from "},
  {"    %s "},
  {"    where table_uid =  "},
  {"     ( "},
  {"       select object_uid from  "},
  {"       HP_SYSTEM_CATALOG.system_schema.catsys C, "},
  {"       HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"       %s.HP_DEFINITION_SCHEMA.objects O "},
  {"       where "},
  {"             C.cat_name = '%s' "},
  {"       and S.schema_name = '%s' "},
  {"       and C.cat_uid = S.cat_uid "},
  {"       and S.schema_uid = O.schema_uid "},
  {"       and O.object_name = '%s' "},
  {"     ) "}
};

static const QueryString populateHistintsStatsQuery[] =
{
  {" insert into %s "},
  {" select  "},
  {"   %ld, "},
  {"   histogram_id, "},
  {"   interval_number, interval_rowcount, interval_uec, interval_boundary, "},
  {"   std_dev_of_freq, "},
  {"   v1, v2, v3, v4, v5, v6 "},
  {" from "},
  {"    %s "},
  {"    where table_uid =  "},
  {"     ( "},
  {"       select object_uid from  "},
  {"       HP_SYSTEM_CATALOG.system_schema.catsys C, "},
  {"       HP_SYSTEM_CATALOG.system_schema.schemata S, "},
  {"       %s.HP_DEFINITION_SCHEMA.objects O "},
  {"       where "},
  {"             C.cat_name = '%s' "},
  {"       and S.schema_name = '%s' "},
  {"       and C.cat_uid = S.cat_uid "},
  {"       and S.schema_uid = O.schema_uid "},
  {"       and O.object_name = '%s' "},
  {"     ) "}
};

short ExExeUtilPopulateInMemStatsTcb::work()
{
  //  short rc = 0;
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

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ContextCli * currContext = masterGlob->getStatement()->getContext();

  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
	    if (getDiagsArea())
	      {
		getDiagsArea()->clear();
		getDiagsArea()->deAllocate();
	      }
	    
	    setDiagsArea(ComDiagsArea::allocate(getHeap()));

	    step_ = PROLOGUE_;
	  }
	break;

	case PROLOGUE_:
	  {
	    if (disableCQS())
	      {
		step_ = ERROR_;
		break;
	      }

	    if (setSchemaVersion(pimsTdb().sourceTableCatName_))
	      {
		step_ = ERROR_;
		break;
	      }

	    // set sqlparserflags to allow use of volatile schema in queries.
	    masterGlob->getStatement()->getContext()->
	      setSqlParserFlags(0x10000);//ALLOW_VOLATILE_SCHEMA_IN_TABLE_NAME

	    step_ = DELETE_STATS_;
	  }
	break;

	case DELETE_STATS_:
	  {
	    Int32 qry_array_size = sizeof(deleteStatsQuery) 
	      / sizeof(QueryString);
	    
	    const QueryString * queryString = deleteStatsQuery;;
	    
	    char * gluedQuery;
	    Lng32 gluedQuerySize;
	    glueQueryFragments(qry_array_size, queryString,
			       gluedQuery, gluedQuerySize);
	    
	    Lng32 extraSpace = 
	      ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES /* fullyQualTableName */
	      + 20 /* UID */
	      + 200 /* overhead */;

	    char * query = new(getHeap()) char[gluedQuerySize + extraSpace];
	    str_sprintf(query, gluedQuery, 
			(char*)pimsTdb().inMemHistogramsTableName_,
			pimsTdb().uid_);
	    
	    cliRC = 
	      cliInterface()->executeImmediate(query);
	    
	    if (cliRC >= 0)
	      {
		str_sprintf(query, gluedQuery, 
			    (char*)pimsTdb().inMemHistintsTableName_,
			    pimsTdb().uid_);
		
		cliRC = 
		  cliInterface()->executeImmediate(query);
	      }

	    // Delete new'd string
	    NADELETEBASIC(gluedQuery, getHeap());
	    gluedQuery = NULL;
	    
	    NADELETEBASIC(query, getHeap());
	    query = NULL;
	    
	    if (cliRC < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		step_ = ERROR_;
	      }
	    else
	      step_ = POPULATE_HISTOGRAMS_STATS_;
	  }
	break;

	case POPULATE_HISTOGRAMS_STATS_:
	  {
	    Int32 qry_array_size = sizeof(populateHistogramsStatsQuery) 
	      / sizeof(QueryString);
	    
	    const QueryString * queryString = populateHistogramsStatsQuery;;
	    
	    char * gluedQuery;
	    Lng32 gluedQuerySize;
	    glueQueryFragments(qry_array_size, queryString,
			       gluedQuery, gluedQuerySize);
	    
	    Lng32 extraSpace =
	        ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES /* fullyQualInMemHistTableName */
	      + 20 /* UID */
	      + ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES /* fullyQualSourceHistTableName */
	      + 2 * 10 /*segment name*/
	      + ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES /*cat name*/
	      + 10  /*version*/ 
	      + ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES /*cat name*/
	      + ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES /*sch name*/
	      + ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES /*obj name*/
	      + 200 /* overhead */;

	    char * query = new(getHeap()) char[gluedQuerySize + extraSpace];
	    
	    str_sprintf(query, gluedQuery, 
			(char*)pimsTdb().inMemHistogramsTableName_,
			pimsTdb().uid_,
			(char*)pimsTdb().sourceHistogramsTableName_,
			(char*)pimsTdb().sourceTableCatName_,
			(char*)pimsTdb().sourceTableCatName_,
			(char*)pimsTdb().sourceTableSchName_,
			(char*)pimsTdb().sourceTableObjName_
			);
	    
	    cliRC = 
	      cliInterface()->executeImmediate(query);
	    
	    // Delete new'd string
	    NADELETEBASIC(gluedQuery, getHeap());
	    gluedQuery = NULL;
	    
	    NADELETEBASIC(query, getHeap());
	    query = NULL;
	    
	    if (cliRC < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		step_ = ERROR_;
	      }
	    else
	      step_ = POPULATE_HISTINTS_STATS_;
	  }
	break;
	  
	case POPULATE_HISTINTS_STATS_:
	  {
	    Int32 qry_array_size = sizeof(populateHistintsStatsQuery) 
	      / sizeof(QueryString);
	    
	    const QueryString * queryString = populateHistintsStatsQuery;;
	    
	    char * gluedQuery;
	    Lng32 gluedQuerySize;
	    glueQueryFragments(qry_array_size, queryString,
			       gluedQuery, gluedQuerySize);
	    
	    Lng32 extraSpace =
	        ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES /* fullyQualInMemHistTableName */
	      + 20 /* UID */
	      + ComMAX_3_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES /* fullyQualSourceHistTableName */
	      + 2 * 10 /*segment name*/
	      + ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES /*cat name*/
	      + 10  /*version*/ 
	      + ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES /*cat name*/
	      + ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES /*sch name*/
	      + ComMAX_1_PART_EXTERNAL_UTF8_NAME_LEN_IN_BYTES /*obj name*/
	      + 200 /* overhead */;

	    char * query = new(getHeap()) char[gluedQuerySize + extraSpace];
	    
	    str_sprintf(query, gluedQuery, 
			(char*)pimsTdb().inMemHistintsTableName_,
			pimsTdb().uid_,
			(char*)pimsTdb().sourceHistintsTableName_,
			(char*)pimsTdb().sourceTableCatName_,
			(char*)pimsTdb().sourceTableCatName_,
			(char*)pimsTdb().sourceTableSchName_,
			(char*)pimsTdb().sourceTableObjName_
			);
	    
	    cliRC = 
	      cliInterface()->executeImmediate(query);
	    
	    // Delete new'd string
	    NADELETEBASIC(gluedQuery, getHeap());
	    gluedQuery = NULL;
	    
	    NADELETEBASIC(query, getHeap());
	    query = NULL;
	    
	    if (cliRC < 0)
	      {
                cliInterface()->allocAndRetrieveSQLDiagnostics(diagsArea_);
		step_ = ERROR_;
	      }
	    else
	      step_ = EPILOGUE_;
	  }
	break;

	case EPILOGUE_:
	case EPILOGUE_AND_ERROR_RETURN_:
	  {
	    // reset sqlparserflags
	    masterGlob->getStatement()->getContext()->
	      resetSqlParserFlags(0x10000);//ALLOW_VOLATILE_SCHEMA_IN_TABLE_NAME

	    restoreCQS();

	    if (step_ == EPILOGUE_AND_ERROR_RETURN_)
	      step_ = ERROR_RETURN_;
	    else
	      step_ = DONE_;
	  }
	  break;

	case ERROR_:
	  {
	    step_ = EPILOGUE_AND_ERROR_RETURN_;
	  }
	  break;

	case ERROR_RETURN_:
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
	    
	    qparent_.down->removeHead();
	    
	    step_ = INITIAL_;
	    return WORK_OK;
	  }
	
	break;
	
	default:
	  break;

	}

    }

  return 0;
}


////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ExExeUtilPopulateInMemStatsTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExExeUtilPopulateInMemStatsPrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExExeUtilPopulateInMemStatsPrivateState::ExExeUtilPopulateInMemStatsPrivateState()
{
}

ExExeUtilPopulateInMemStatsPrivateState::~ExExeUtilPopulateInMemStatsPrivateState()
{
};


////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilHiveTruncateTdb
///////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilHiveTruncateTdb::build(ex_globals * glob)
{
  ExExeUtilTcb * exe_util_tcb;

  exe_util_tcb = new(glob->getSpace()) ExExeUtilHiveTruncateTcb(*this, glob);
  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}


////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilHiveTruncateTcb
///////////////////////////////////////////////////////////////
ExExeUtilHiveTruncateTcb::ExExeUtilHiveTruncateTcb(
     const ComTdbExeUtilHiveTruncate & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilTcb( exe_util_tdb, NULL, glob)
{
  // Allocate the private state in each entry of the down queue
  qparent_.down->allocatePstate(this);

  numExistingFiles_ = 0;

  step_ = INITIAL_;
}

ExExeUtilHiveTruncateTcb::~ExExeUtilHiveTruncateTcb()
{
  freeResources();
}

void ExExeUtilHiveTruncateTcb::freeResources()
{
  if (htTdb().getDropOnDealloc())
  {
      NAString hiveDropDDL("drop table ");
      hiveDropDDL += htTdb().getHiveTableName();

      // TODO: is it ok to ignore the error 
      HiveClient_JNI::executeHiveSQL(hiveDropDDL);
  }
  if (lobGlob_) {
    ExpLOBinterfaceCleanup(lobGlob_);
    lobGlob_ = NULL;
  }
}

Int32 ExExeUtilHiveTruncateTcb::fixup()
{
  lobGlob_ = NULL;

  ExpLOBinterfaceInit
    (lobGlob_, (NAHeap *)getGlobals()->getDefaultHeap(),
     getGlobals()->castToExExeStmtGlobals()->getContext(),FALSE, 
     htTdb().getHdfsHost(),
     htTdb().getHdfsPort());

  return 0;
}
//////////////////////////////////////////////////////
// work() for ExExeUtilHiveTruncateTsb
//////////////////////////////////////////////////////
short ExExeUtilHiveTruncateTcb::work()
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
  ExExeUtilPrivateState & pstate = *((ExExeUtilPrivateState*) pentry_down->pstate);

  while (1)
  {
    switch (step_)
    {
      case INITIAL_:
      {

        if (htTdb().getModTS() > 0)
          step_ = DATA_MOD_CHECK_;
        else
          step_ = EMPTY_DIRECTORY_;
      }
      break;

      case DATA_MOD_CHECK_:
      {
        Int64 failedModTS = -1;
        Lng32 failedLocBufLen = 1000;
        char failedLocBuf[failedLocBufLen];
        cliRC = ExpLOBinterfaceDataModCheck
          (lobGlob_,
           (htTdb().getPartnLocation() ? 
            htTdb().getPartnLocation() : 
            htTdb().getTableLocation()),
           htTdb().getHdfsHost(),
           htTdb().getHdfsPort(),
           htTdb().getModTS(),
           0,
           failedModTS,
           failedLocBuf, failedLocBufLen);

        if (cliRC < 0)
        {
          Lng32 cliError = 0;
          
          Lng32 intParam1 = -cliRC;
          ExRaiseSqlError(getHeap(), &diagsArea_, 
                          (ExeErrorCode)(EXE_ERROR_FROM_LOB_INTERFACE),
                          NULL, &intParam1, 
                          &cliError, 
                          NULL, 
                          "HDFS",
                          (char*)"ExpLOBInterfaceEmptyDirectory",
                          getLobErrStr(intParam1));
          step_ = ERROR_;
          break;
        }

        if (cliRC == 1) // data mod check failed
        {
          char errStr[200];
          str_sprintf(errStr, "genModTS = %ld, failedModTS = %ld", 
                      htTdb().getModTS(), failedModTS);
          
          ExRaiseSqlError(getHeap(), &diagsArea_, 
                          (ExeErrorCode)(EXE_HIVE_DATA_MOD_CHECK_ERROR), NULL,
                          NULL, NULL, NULL,
                          errStr);
          step_ = ERROR_;
          break;
        }
   
        step_ = EMPTY_DIRECTORY_;
      }
      break;

      case EMPTY_DIRECTORY_:
      {
        cliRC = ExpLOBinterfaceEmptyDirectory(
             lobGlob_,
             (char*)"",                  //name is empty
             (htTdb().getPartnLocation() ? 
              htTdb().getPartnLocation() : 
              htTdb().getTableLocation()),
             Lob_HDFS_File,
             htTdb().getHdfsHost(),
             htTdb().getHdfsPort(),
             0 ,
             1 ,
             0);
        if (cliRC != 0)
        {
          Lng32 cliError = 0;
          
          Lng32 intParam1 = -cliRC;
          ExRaiseSqlError(getHeap(), &diagsArea_, 
                          (ExeErrorCode)(EXE_ERROR_FROM_LOB_INTERFACE),
                          NULL, &intParam1, 
                          &cliError, 
                          NULL, 
                          "HDFS",
                          (char*)"ExpLOBInterfaceEmptyDirectory",
                          getLobErrStr(intParam1));

          char reason[200];
          
          strcpy(reason, " ");
          if (intParam1 == LOB_DIR_NAME_ERROR)
            {
              if (htTdb().getPartnLocation())
                strcpy(reason, "Reason: specified partition does not exist");
              else
                strcpy(reason, "Reason: specified table location does not exist");
            }
          else if (intParam1 == LOB_DATA_FILE_DELETE_ERROR)
            {
              strcpy(reason, "Reason: error occurred during deletion of one or more files at the specified location");
            }
          
          ExRaiseSqlError(getHeap(), &diagsArea_, 
                          (ExeErrorCode)(EXE_HIVE_TRUNCATE_ERROR), NULL,
                          NULL, NULL, NULL,
                          reason,
                          NULL, NULL);
          step_ = ERROR_;
        }
        else
        {
          step_= DONE_;
        }
      }
      break;

      case ERROR_:
      {
        if (handleError())
           return WORK_OK;
        step_ = DONE_;
      }
      break;

      case DONE_:
      {
        if (handleDone())
           return WORK_OK;
        step_ = INITIAL_;

        return WORK_OK;
      }
      break;

    } // switch
  } // while

}


ex_tcb_private_state * ExExeUtilHiveTruncateTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExExeUtilHiveTruncatePrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExExeUtilHiveTruncatePrivateState::ExExeUtilHiveTruncatePrivateState()
{
}

ExExeUtilHiveTruncatePrivateState::~ExExeUtilHiveTruncatePrivateState()
{
};

////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilHiveQueryTdb
///////////////////////////////////////////////////////////////
ex_tcb * ExExeUtilHiveQueryTdb::build(ex_globals * glob)
{
  ExExeUtilTcb * exe_util_tcb;

  exe_util_tcb = new(glob->getSpace()) ExExeUtilHiveQueryTcb(*this, glob);
  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}


////////////////////////////////////////////////////////////////
// Constructor for class ExExeUtilHiveQueryTcb
///////////////////////////////////////////////////////////////
ExExeUtilHiveQueryTcb::ExExeUtilHiveQueryTcb(
     const ComTdbExeUtilHiveQuery & exe_util_tdb,
     ex_globals * glob)
     : ExExeUtilTcb( exe_util_tdb, NULL, glob)
{
  // Allocate the private state in each entry of the down queue
  qparent_.down->allocatePstate(this);

  step_ = INITIAL_;
}

ExExeUtilHiveQueryTcb::~ExExeUtilHiveQueryTcb()
{
}

//////////////////////////////////////////////////////
// work() for ExExeUtilHiveQueryTsb
//////////////////////////////////////////////////////
short ExExeUtilHiveQueryTcb::work()
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
  ExExeUtilPrivateState & pstate = *((ExExeUtilPrivateState*) pentry_down->pstate);

  while (1)
    {
      switch (step_)
        {
        case INITIAL_:
          {
            step_ = PROCESS_QUERY_;
          }
          break;
          
        case PROCESS_QUERY_:
          {
            if (HiveClient_JNI::executeHiveSQL(htTdb().getHiveQuery()) != HVC_OK)
            {
                ExRaiseSqlError(getHeap(), &diagsArea_, -1214,
                        NULL, NULL, NULL,
                        getSqlJniErrorStr(), htTdb().getHiveQuery()); 
                step_ = ERROR_;
                break;
            }
            step_ = DONE_;
          }
          break;
          
        case ERROR_:
          {
            if (handleError())
              return WORK_OK;
            step_ = DONE_;
          }
          break;
          
        case DONE_:
          {
            if (handleDone())
              return WORK_OK;
            
            step_ = INITIAL_;
            
            return WORK_OK;
          }
          break;
          
        } // switch
    } // while
  
}


ex_tcb_private_state * ExExeUtilHiveQueryTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExExeUtilHiveQueryPrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExExeUtilHiveQueryPrivateState::ExExeUtilHiveQueryPrivateState()
{
}

ExExeUtilHiveQueryPrivateState::~ExExeUtilHiveQueryPrivateState()
{
};

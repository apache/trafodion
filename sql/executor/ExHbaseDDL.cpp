// **********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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
// **********************************************************************
	
#include "Platform.h"

#include  "cli_stdh.h"
#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ExHbaseAccess.h"
#include "ex_exe_stmt_globals.h"
#include "SQLTypeDefs.h"
#include "ExpHbaseInterface.h"
#include "ExExeUtilCli.h"
#include "ComDiags.h"
#include "ExSqlComp.h"

ExHbaseAccessDDLTcb::ExHbaseAccessDDLTcb(
          const ExHbaseAccessTdb &hbaseAccessTdb, 
          ex_globals * glob ) :
  ExHbaseAccessTcb(hbaseAccessTdb, glob)
  , step_(NOT_STARTED)
{
}

  short ExHbaseAccessDDLTcb::createHbaseTable(HbaseStr &table,
					    const char * cf1, const char * cf2, const char * cf3)
{
  return 0;
}

short ExHbaseAccessDDLTcb::dropMDtable(const char * name)
{
  return 0;
}

ExWorkProcRetcode ExHbaseAccessDDLTcb::work()
{
  short retcode = 0;
  short rc = 0;

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  while (1)
    {
      switch (step_)
	{
	case NOT_STARTED:
	  {
	    matches_ = 0;
	    if (hbaseAccessTdb().accessType_ == ComTdbHbaseAccess::CREATE_)
	      step_ = CREATE_TABLE;
	   else if (hbaseAccessTdb().accessType_ == ComTdbHbaseAccess::DROP_)
	      step_ = DROP_TABLE;
	   else
	     step_ = HANDLE_ERROR;
	  }
	  break;

	case CREATE_TABLE:
	  {
	    table_.val = hbaseAccessTdb().getTableName();
	    table_.len = strlen(hbaseAccessTdb().getTableName());

	    Queue * cfnl = hbaseAccessTdb().getColFamNameList();
	    cfnl->position();
	    HBASE_NAMELIST colFamList;
	    HbaseStr colFam;
	    while(NOT cfnl->atEnd())
	      {
		char * cfName = (char*)cfnl->getCurr();
		colFam.val = cfName;
		colFam.len = strlen(cfName);

		colFamList.insert(colFam);
		cfnl->advance();
	      }

	    step_ = DONE;
	  }
	  break;

	case DROP_TABLE:
	  {
	    step_ = DONE;
	  }
	  break;

	case HANDLE_ERROR:
	  {
	    if (handleError(rc))
	      return rc;
	    
	    step_ = DONE;
	  }
	  break;

	case DONE:
	  {
	    if (handleDone(rc))
	      return rc;

	    step_ = NOT_STARTED;

	    return WORK_OK;
	  }
	  break;

	}// switch

    } // while
}

ExHbaseAccessInitMDTcb::ExHbaseAccessInitMDTcb(
          const ExHbaseAccessTdb &hbaseAccessTdb, 
          ex_globals * glob ) :
  ExHbaseAccessDDLTcb(hbaseAccessTdb, glob)
  , step_(NOT_STARTED)
{
}

ExWorkProcRetcode ExHbaseAccessInitMDTcb::work()
{
  short retcode = 0;
  short rc = 0;
  Lng32 cliRC = 0;

  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ComDiagsArea *da = exeGlob->getDiagsArea();
  
  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();
 
  while (1)
    {
      switch (step_)
	{
	case NOT_STARTED:
	  {
	    if (! masterGlob)
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    matches_ = 0;
	   if (hbaseAccessTdb().accessType_ == ComTdbHbaseAccess::INIT_MD_)
	      step_ = INIT_MD;
	   else if (hbaseAccessTdb().accessType_ == ComTdbHbaseAccess::DROP_MD_)
	      step_ = DROP_MD;
	   else
	     step_ = HANDLE_ERROR;
	  }
	  break;

	case INIT_MD:
	  {
	    step_ = UPDATE_MD;
	  }
	  break;

	case UPDATE_MD:
	  {
	    step_ = DONE;
	  }
	  break;

	case DROP_MD:
	  {
	    step_ = DONE;
	  }
	  break;

	case HANDLE_ERROR:
	  {
	    if (handleError(rc))
	      return rc;
	    
	    step_ = DONE;
	  }
	  break;

	case DONE:
	  {
	    if (handleDone(rc))
	      return rc;

	    step_ = NOT_STARTED;

	    return WORK_OK;
	  }
	  break;

	}// switch

    } // while
}

ExHbaseAccessGetTablesTcb::ExHbaseAccessGetTablesTcb(
          const ExHbaseAccessTdb &hbaseAccessTdb, 
          ex_globals * glob ) :
  ExHbaseAccessTcb(hbaseAccessTdb, glob)
  , step_(NOT_STARTED)
{
}

ExWorkProcRetcode ExHbaseAccessGetTablesTcb::work()
{
  Lng32 retcode = 0;
  short rc = 0;

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  while (1)
    {
      switch (step_)
	{
	case NOT_STARTED:
	  {
	    matches_ = 0;
	    step_ = GET_TABLE;
	  }
	  break;

	case GET_TABLE:
	  {
	    retcode = ehi_->getTable(table_);
	    if (retcode == HBASE_ACCESS_EOD)
	      {
		step_ = CLOSE;
		break;
	      }

	    if (setupError(retcode, "ExpHbaseInterface::getTable"))
	      step_ = HANDLE_ERROR;
	    else
	      step_ = RETURN_ROW;
	  }
	  break;

	case RETURN_ROW:
	  {
	    char * ptr = table_.val;
	    short len = (short)table_.len;
	    if (moveRowToUpQueue(ptr, len, &rc, TRUE))
	      {
		return rc;
	      }
	    
	    step_ = GET_TABLE;
	  }
	  break;

	case CLOSE:
	  {
	    step_ = DONE;
	  }
	  break;
	
	case HANDLE_ERROR:
	  {
	    if (handleError(rc))
	      return rc;

	    step_ = DONE;
	  }
	  break;

	case DONE:
	  {
	    if (handleDone(rc))
	      return rc;

	    step_ = NOT_STARTED;

	    return WORK_OK;
	  }
	  break;

	}// switch

    } // while
}

ex_tcb * ExExeUtilMetadataUpgradeTdb::build(ex_globals * glob)
{
  ExExeStmtGlobals * exe_glob = glob->castToExExeStmtGlobals();
  
  ExExeUtilMetadataUpgradeTcb *tcb = NULL;

  tcb = new(exe_glob->getSpace()) 
    ExExeUtilMetadataUpgradeTcb(
			 *this,
			 exe_glob);
  
  ex_assert(tcb, "Error building ExExeUtilMetadataUpgradeTcb.");

  tcb->registerSubtasks();

  return (tcb);
}

ExExeUtilMetadataUpgradeTcb::ExExeUtilMetadataUpgradeTcb(
          const ExExeUtilMetadataUpgradeTdb &mdUpgradeTdb, 
          ex_globals * glob ) :
  ExExeUtilTcb(mdUpgradeTdb, NULL, glob),
  step_(NOT_STARTED_)
{
}

ExWorkProcRetcode ExExeUtilMetadataUpgradeTcb::work()
{
  short retcode = 0;
  short rc = 0;
  Lng32 cliRC = 0;
  
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ComDiagsArea *da = exeGlob->getDiagsArea();
  ContextCli * currContext = masterGlob->getStatement()->getContext();
  
#ifdef NA_CMPDLL
  if (!currContext->getNumArkcmps())
    return NULL;  // no regular compiler exists
#endif // NA_CMPDLL
  
  short indexIntoCompilerArray = currContext->getIndexToCompilerArray();
  cmp_ = currContext->getArkcmp(indexIntoCompilerArray);
  
  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;
  
  ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();
  
  while (1)
    {
      switch (step_)
	{
	case NOT_STARTED_:
	  {
	    upgdStep_ = 0;
	    upgdSubstep_ = 0;
	    
	    startTime_ = 0;
	    endTime_ = 0;
	    
	    mdi_ = new(getHeap()) CmpMDupgradeInfo();
	    
	    step_ = SETUP_NEXT_STEP_;
	  }
	  break;
	  
	case SETUP_NEXT_STEP_:
	  {
	    if (mdUpgdTdb().getMDVersion())
	      mdi_->setGetMDVersion(TRUE);

	    else if (mdUpgdTdb().getSWVersion())
	      mdi_->setGetSWVersion(TRUE);

	    step_ = SEND_REQ_TO_CMP_;
	  }
	  break;
	  
	case SEND_REQ_TO_CMP_:
	  {
	    ExSqlComp::ReturnStatus cmpStatus;
	    
	    size_t dataLen = sizeof(*mdi_);
	    char * data = new(getHeap()) char[dataLen];
	    str_cpy_all(data, (char*)mdi_, dataLen);
	    
	    replyBuf_ = NULL;
	    ULng32 dummyLength;
	    
	    cmpStatus = 
	      cmp_->sendRequest(EXSQLCOMP::MD_UPGRADE, data, dataLen,
				TRUE, NULL,
				SQLCHARSETCODE_UTF8,
				TRUE /*resend, if needed*/);
	    
	    getHeap()->deallocateMemory(data);
	    
	    if (cmpStatus != ExSqlComp::SUCCESS)
	      {
		// If its an error don't proceed further.
		getDiagsArea()->mergeAfter(*cmp_->getDiags());
		if (cmpStatus == ExSqlComp::ERROR)
		  {
		    step_ = HANDLE_ERROR_;
		    break;
		  }
	      }
	    
	    cmpStatus = cmp_->getReply(replyBuf_, dummyLength, 0, 0, TRUE);
	    
	    if ((cmp_->getDiags()) &&
		((cmp_->getDiags()->getNumber(DgSqlCode::WARNING_) > 0) ||
		 (cmp_->getDiags()->getNumber(DgSqlCode::ERROR_) > 0)))
	      {
		getDiagsArea()->mergeAfter(*cmp_->getDiags());
	      }
	    
	    if (replyBuf_)
	      {
		memcpy(mdi_, replyBuf_, sizeof(*mdi_));
		
		if (mdi_->computeST())
		  startTime_ = NA_JulianTimestamp();
		else if (mdi_->computeET())
		  endTime_ = NA_JulianTimestamp();
	      }
	    
	    step_ = RETURN_STATUS_;
	  }
	  break;

	case RETURN_STATUS_:
	  {
	    char buf[1000];
	    if (strlen(mdi_->msg()) > 0)
	      {
		str_sprintf(buf, "%s", mdi_->msg());
		if (moveRowToUpQueue(buf, 0, &rc))
		  return rc;
	      }

	    if (mdi_->endStep())
	      {
		if (moveRowToUpQueue(" ", 0, &rc))
		  return rc;
	      }

	    step_ = RETURN_STATUS_END_STEP_;
	    return WORK_RESCHEDULE_AND_RETURN;
	  }
	  break;

	case RETURN_STATUS_END_STEP_:
	  {
	    if (getenv("WAITED_UPGD"))
	      {
		if (mdi_->endStep())
		  {
		    char buf[100];
		    
		    cout << "";
		    cin >> buf;
		    cout << endl;
		  }
	      }

	    cmp_->getHeap()->deallocateMemory((void*)replyBuf_);
	    replyBuf_ = NULL;

	    if (mdi_->done())
	      {
		if ((getDiagsArea()) &&
		    (getDiagsArea()->getNumber(DgSqlCode::ERROR_) > 0))
		  step_ = HANDLE_ERROR_;
		else
		  step_ = DONE_;
		break;
	      }

	    step_ = SETUP_NEXT_STEP_;
	    return WORK_RESCHEDULE_AND_RETURN;
	  }
	  break;

	case HANDLE_ERROR_:
	  {
	    if (handleError() == 1)
	      return WORK_OK;

	    step_ = DONE_;
	  }
	  break;

	case DONE_:
	  {
	    if (handleDone() == 1)
	      return WORK_OK;

	    if (replyBuf_)
	      cmp_->getHeap()->deallocateMemory((void*)replyBuf_);
	      
	    step_ = NOT_STARTED_;

	    return WORK_OK;
	  }
	  break;

	}// switch

    } // while

  return WORK_OK;
}

ExHbaseAccessBulkLoadTaskTcb::ExHbaseAccessBulkLoadTaskTcb(const ExHbaseAccessTdb &hbaseAccessTdb, ex_globals * glob) :
    ExHbaseAccessTcb(hbaseAccessTdb, glob), step_(NOT_STARTED)
{
}

ExWorkProcRetcode ExHbaseAccessBulkLoadTaskTcb::work()
{
  short retcode = 0;
  short rc = 0;

  ExMasterStmtGlobals *g = getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals();

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  while (1)
  {
    switch (step_)
    {
      case NOT_STARTED:
      {
        matches_ = 0;

        retcode = ehi_->initHBLC();
        if (setupError(retcode, "ExpHbaseInterface::initHBLC"))
        {
          step_ = HANDLE_ERROR_AND_CLOSE;
          break;
        }

        table_.val = hbaseAccessTdb().getTableName();
        table_.len = strlen(hbaseAccessTdb().getTableName());

        hBulkLoadPrepPath_ = std::string(((ExHbaseAccessTdb&) hbaseAccessTdb()).getLoadPrepLocation())
            + ((ExHbaseAccessTdb&) hbaseAccessTdb()).getTableName();

        if (((ExHbaseAccessTdb&) hbaseAccessTdb()).getIsTrafLoadCleanup())
          step_ = LOAD_CLEANUP;
        else
          step_ = COMPLETE_LOAD;
      }
        break;
      case LOAD_CLEANUP:
      {
        //cleanup
        retcode = ehi_->bulkLoadCleanup(table_, hBulkLoadPrepPath_);

        if (setupError(retcode, "ExpHbaseInterface::bulkLoadCleanup"))
        {
          step_ = HANDLE_ERROR_AND_CLOSE;
          break;
        }
        step_ = LOAD_CLOSE;
      }
        break;

      case COMPLETE_LOAD:
      {
        Text tabName = ((ExHbaseAccessTdb&) hbaseAccessTdb()).getTableName();
        retcode = ehi_->doBulkLoad(table_,
                                   hBulkLoadPrepPath_,
                                   tabName,
                                   hbaseAccessTdb().getUseQuasiSecure(),
                                   hbaseAccessTdb().getTakeSnapshot());

        if (setupError(retcode, "ExpHbaseInterface::doBulkLoad"))
        {
          step_ = HANDLE_ERROR_AND_CLOSE;
          break;
        }

        if (((ExHbaseAccessTdb&) hbaseAccessTdb()).getIsTrafLoadKeepHFiles())
        {
          step_ = LOAD_CLOSE;
          break;
        }

        step_ = LOAD_CLEANUP;
      }
        break;

      case LOAD_CLOSE:
      {
        retcode = ehi_->close();
        if (setupError(retcode, "ExpHbaseInterface::close"))
        {
          step_ = HANDLE_ERROR;
          break;
        }
        step_ = DONE;
      }
      break;

      case HANDLE_ERROR:
      case HANDLE_ERROR_AND_CLOSE:
      {
        if (handleError(rc))
          return rc;
        if (step_==HANDLE_ERROR_AND_CLOSE)
        {
          step_ = LOAD_CLOSE;
          break;
        }
        step_ = DONE;
      }
        break;

      case DONE:
      {
        if (handleDone(rc))
          return rc;

        step_ = NOT_STARTED;
        retcode = ehi_->close();

        return WORK_OK;
      }
        break;

    } // switch

  } // while
}

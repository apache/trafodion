// **********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
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

#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ExHbaseAccess.h"
#include "ex_exe_stmt_globals.h"
#include "ExpHbaseInterface.h"
#include "hs_util.h"

ExHbaseAccessInsertTcb::ExHbaseAccessInsertTcb(
          const ExHbaseAccessTdb &hbaseAccessTdb, 
          ex_globals * glob ) :
  ExHbaseAccessTcb( hbaseAccessTdb, glob),
  step_(NOT_STARTED)
{
  insertRowlen_ = 0;
}

ExWorkProcRetcode ExHbaseAccessInsertTcb::work()
{
  Lng32 retcode = 0;
  short rc = 0;

  while (!qparent_.down->isEmpty())
    {
      ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();
      if (pentry_down->downState.request == ex_queue::GET_NOMORE)
	step_ = DONE;

      switch (step_)
	{
	case NOT_STARTED:
	  {
	    matches_ = 0;

	    step_ = INSERT_INIT;
	  }
	  break;
	  
	case INSERT_INIT:
	  {
	    retcode = ehi_->init(getHbaseAccessStats());
	    if (setupError(retcode, "ExpHbaseInterface::init"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    table_.val = hbaseAccessTdb().getTableName();
	    table_.len = strlen(hbaseAccessTdb().getTableName());

	    step_ = SETUP_INSERT;
	  }
	  break;

	case SETUP_INSERT:
	  {
	    step_ = EVAL_INSERT_EXPR;
	  }
	  break;

	case EVAL_INSERT_EXPR:
	  {
	    workAtp_->getTupp(hbaseAccessTdb().convertTuppIndex_)
	      .setDataPointer(convertRow_);
	    
	    if (convertExpr())
	      {
		ex_expr::exp_return_type evalRetCode =
		  convertExpr()->eval(pentry_down->getAtp(), workAtp_);
		if (evalRetCode == ex_expr::EXPR_ERROR)
		  {
                    step_ = HANDLE_ERROR;
                    break;
		  }
	      }
	  
	    ExpTupleDesc * convertRowTD =
	      hbaseAccessTdb().workCriDesc_->getTupleDescriptor
	      (hbaseAccessTdb().convertTuppIndex_);
	    
	    for (Lng32 i = 0; i <  convertRowTD->numAttrs(); i++)
	      {
		Attributes * attr = convertRowTD->getAttr(i);
		Lng32 len = 0;
		if (attr)
		  {
                    if (attr->getVCIndicatorLength() == sizeof(short))
                      len = *(short*)&convertRow_[attr->getVCLenIndOffset()];
                    else
                     len = *(Lng32*)&convertRow_[attr->getVCLenIndOffset()];
                      
		    switch (i)
		      {
		      case HBASE_ROW_ID_INDEX:
			{
			  insRowId_.assign(&convertRow_[attr->getOffset()], len);
			}
			break;
			
		      case HBASE_COL_FAMILY_INDEX:
			{
			  insColFam_.assign(&convertRow_[attr->getOffset()], len);
			}
			break;
			
		      case HBASE_COL_NAME_INDEX:
			{
			  insColNam_.assign(&convertRow_[attr->getOffset()], len);
			}
			break;
			
		      case HBASE_COL_VALUE_INDEX:
			{
			  insColVal_.assign(&convertRow_[attr->getOffset()], len);
			}
			break;
			
		      case HBASE_COL_TS_INDEX:
			{
			  insColTS_ = (Int64*)&convertRow_[attr->getOffset()];
			}
			break;
			
		      } // switch
		  } // if attr
	      }	// convertExpr
	    
	    step_ = PROCESS_INSERT;
	  }
	  break;

	case PROCESS_INSERT:
	  {
            createDirectRowBuffer(insColFam_, insColNam_, insColVal_);
            HbaseStr rowID;
            rowID.val = (char *)insRowId_.data();
            rowID.len = insRowId_.size();
	    retcode = ehi_->insertRow(table_,
				      rowID, 
				      row_,
                                      hbaseAccessTdb().useHbaseXn(),
				      *insColTS_);

	    if (setupError(retcode, "ExpHbaseInterface::insertRow"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    if (getHbaseAccessStats())
	      getHbaseAccessStats()->incUsedRows();

	    matches_++;

	    step_ = INSERT_CLOSE;
	  }
	  break;

	case INSERT_CLOSE:
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
	  {
	    if (handleError(rc))
	      return rc;

	    step_ = DONE;
	  }
	  break;

	case DONE:
	  {
	    if (handleDone(rc, matches_))
	      return rc;

	    step_ = NOT_STARTED;
	  }
	  break;

	} // switch

    } // while

  return WORK_OK;
}

ExHbaseAccessInsertRowwiseTcb::ExHbaseAccessInsertRowwiseTcb(
          const ExHbaseAccessTdb &hbaseAccessTdb, 
          ex_globals * glob ) :
  ExHbaseAccessInsertTcb( hbaseAccessTdb, glob)
{
}

ExWorkProcRetcode ExHbaseAccessInsertRowwiseTcb::work()
{
  Lng32 retcode = 0;
  short rc = 0;

  while (!qparent_.down->isEmpty())
    {
      ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();
      if (pentry_down->downState.request == ex_queue::GET_NOMORE)
	step_ = DONE;

      switch (step_)
	{
	case NOT_STARTED:
	  {
	    matches_ = 0;

	    step_ = INSERT_INIT;
	  }
	  break;
	  
	case INSERT_INIT:
	  {
	    retcode = ehi_->init(getHbaseAccessStats());
	    if (setupError(retcode, "ExpHbaseInterface::init"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    table_.val = hbaseAccessTdb().getTableName();
	    table_.len = strlen(hbaseAccessTdb().getTableName());

	    step_ = SETUP_INSERT;
	  }
	  break;

	case SETUP_INSERT:
	  {
	    step_ = EVAL_INSERT_EXPR;
	  }
	  break;

	case EVAL_INSERT_EXPR:
	  {
	    workAtp_->getTupp(hbaseAccessTdb().convertTuppIndex_)
	      .setDataPointer(convertRow_);
	    
	    if (convertExpr())
	      {
		ex_expr::exp_return_type evalRetCode =
		  convertExpr()->eval(pentry_down->getAtp(), workAtp_);
		if (evalRetCode == ex_expr::EXPR_ERROR)
		  {
                    step_ = HANDLE_ERROR;
                    break;
		  }
	      }
	  
	    ExpTupleDesc * convertRowTD =
	      hbaseAccessTdb().workCriDesc_->getTupleDescriptor
	      (hbaseAccessTdb().convertTuppIndex_);

	    for (Lng32 i = 0; i <  convertRowTD->numAttrs(); i++)
	      {
		Attributes * attr = convertRowTD->getAttr(i);
		short len = 0;
		if (attr)
		  {
		    len = *(short*)&convertRow_[attr->getVCLenIndOffset()];

		    switch (i)
		      {
		      case HBASE_ROW_ID_INDEX:
			{
			  insRowId_.assign(&convertRow_[attr->getOffset()], len);
			}
			break;
			
		      case HBASE_COL_DETAILS_INDEX:
			{
			  char * convRow = &convertRow_[attr->getOffset()];

			  retcode = createDirectRowwiseBuffer(convRow);
			}
			break;
			
		      } // switch
		  } // if attr
	      }	// for

	    step_ = PROCESS_INSERT;
	  }
	  break;

	case PROCESS_INSERT:
	  {
	    if (numColsInDirectBuffer() > 0)
	      {
                HbaseStr rowID;
                rowID.val = (char *)insRowId_.data();
                rowID.len = insRowId_.size();
                retcode = ehi_->insertRow(table_,
					  rowID,
					  row_,
                                          hbaseAccessTdb().useHbaseXn(),
					  
					  -1 //*insColTS_
					  );

		if (setupError(retcode, "ExpHbaseInterface::insertRow"))
		  {
		    step_ = HANDLE_ERROR;
		    break;
		  }
		
		if (getHbaseAccessStats())
		  getHbaseAccessStats()->incUsedRows();

		matches_++;
	      }

	    step_ = INSERT_CLOSE;
	  }
	  break;

	case INSERT_CLOSE:
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
	  {
	    if (handleError(rc))
	      return rc;

	    step_ = DONE;
	  }
	  break;

	case DONE:
	  {
	    if (handleDone(rc, matches_))
	      return rc;

	    step_ = NOT_STARTED;
	  }
	  break;

	} // switch

    } // while

  return WORK_OK;
}

ExHbaseAccessInsertSQTcb::ExHbaseAccessInsertSQTcb(
          const ExHbaseAccessTdb &hbaseAccessTdb, 
          ex_globals * glob ) :
  ExHbaseAccessInsertTcb( hbaseAccessTdb, glob)
{
}

ExWorkProcRetcode ExHbaseAccessInsertSQTcb::work()
{
  Lng32 retcode = 0;
  short rc = 0;

  while (!qparent_.down->isEmpty())
    {
      ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();
      if (pentry_down->downState.request == ex_queue::GET_NOMORE)
	step_ = DONE;

      switch (step_)
	{
	case NOT_STARTED:
	  {
	    matches_ = 0;

	    step_ = INSERT_INIT;
	  }
	  break;
	  
	case INSERT_INIT:
	  {
	    retcode = ehi_->init(getHbaseAccessStats());
	    if (setupError(retcode, "ExpHbaseInterface::init"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    table_.val = hbaseAccessTdb().getTableName();
	    table_.len = strlen(hbaseAccessTdb().getTableName());

	    step_ = SETUP_INSERT;
	  }
	  break;

	case SETUP_INSERT:
	  {
	    step_ = EVAL_INSERT_EXPR;
	  }
	  break;

	case EVAL_INSERT_EXPR:
	  {
	    workAtp_->getTupp(hbaseAccessTdb().convertTuppIndex_)
	      .setDataPointer(convertRow_);
	    
	    if (convertExpr())
	      {
                insertRowlen_ = hbaseAccessTdb().convertRowLen_;
		ex_expr::exp_return_type evalRetCode =
		  convertExpr()->eval(pentry_down->getAtp(), workAtp_,
                                      NULL, -1, &insertRowlen_);
		if (evalRetCode == ex_expr::EXPR_ERROR)
		  {
		    step_ = HANDLE_ERROR;
		    break;
		  }
	      }

            genAndAssignSyskey(hbaseAccessTdb().convertTuppIndex_, convertRow_);

	    step_ = EVAL_CONSTRAINT;
	  }
	  break;

	case EVAL_CONSTRAINT:
	  {
	    rc = applyPred(scanExpr());
	    if (rc == 1) // expr is true or no expr
	      step_ = CREATE_MUTATIONS;
	    else if (rc == 0) // expr is false
	      step_ = INSERT_CLOSE;
	    else // error
	      step_ = HANDLE_ERROR;
	  }
	  break;

	case CREATE_MUTATIONS:
	  {
	    retcode = createDirectRowBuffer( hbaseAccessTdb().convertTuppIndex_,
                                             convertRow_,
                                             hbaseAccessTdb().listOfUpdatedColNames(),
                                             (hbaseAccessTdb().hbaseSqlIUD() ? FALSE : TRUE));

	    if (retcode == -1)
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    insColTSval_ = -1;

	    step_ = EVAL_ROWID_EXPR;
	  }
	  break;

	case EVAL_ROWID_EXPR:
	  {
	    if (evalRowIdExpr(TRUE) == -1)
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    insRowId_.assign(rowId_.val, rowId_.len);

	    if (hbaseAccessTdb().hbaseSqlIUD())
	      step_ = CHECK_AND_INSERT;
	    else
	      step_ = PROCESS_INSERT;
	  }
	  break;

	case CHECK_AND_INSERT:
	  {
            HbaseStr rowID;
            rowID.val = (char *)insRowId_.data();
            rowID.len = insRowId_.size();
            retcode = ehi_->checkAndInsertRow(table_,
                                              rowID,
	                                      row_,
                                              hbaseAccessTdb().useHbaseXn(),
                                              insColTSval_);

	    if (retcode == HBASE_DUP_ROW_ERROR) // row exists, return error
	      {
		ComDiagsArea * diagsArea = NULL;
		ExRaiseSqlError(getHeap(), &diagsArea, 
				(ExeErrorCode)(8102));
		pentry_down->setDiagsArea(diagsArea);
		step_ = HANDLE_ERROR;
		break;
	      }
	      
	    if (setupError(retcode, "ExpHbaseInterface::rowExists"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }
	    
	    if (getHbaseAccessStats())
	      getHbaseAccessStats()->incUsedRows();

	    if (hbaseAccessTdb().returnRow())
		step_ = RETURN_ROW;
	    else
	      {
	        step_ = INSERT_CLOSE;
	        matches_++;
	      }
	        
	  }
	  break;

	case PROCESS_INSERT:
	  {
            HbaseStr rowID;
            rowID.val = (char *)insRowId_.data();
            rowID.len = insRowId_.size();
            retcode = ehi_->insertRow(table_,
				      rowID,
				      row_,
                                      hbaseAccessTdb().useHbaseXn(),
				      insColTSval_);



	    if (getHbaseAccessStats())
	      getHbaseAccessStats()->incUsedRows();

	    if (setupError(retcode, "ExpHbaseInterface::insertRow"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    if (hbaseAccessTdb().returnRow())
	      {
		step_ = RETURN_ROW;
		break;
	      }

	    matches_++;

	    step_ = INSERT_CLOSE;
	  }
	  break;

	case RETURN_ROW:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;
	    
	    if (returnUpdateExpr())
	      {
		ex_queue_entry * up_entry = qparent_.up->getTailEntry();

		// allocate tupps where returned rows will be created
		if (allocateUpEntryTupps(
					 -1,
					 0,
					 hbaseAccessTdb().returnedTuppIndex_,
					 hbaseAccessTdb().returnUpdatedRowLen_,
					 FALSE,
					 &rc))
		  return 1;

		ex_expr::exp_return_type exprRetCode =
		  returnUpdateExpr()->eval(up_entry->getAtp(), workAtp_);
		if (exprRetCode == ex_expr::EXPR_ERROR)
		  {
		    step_ = HANDLE_ERROR;
		    break;
		  }
		
		rc = 0;
		// moveRowToUpQueue also increments matches_
		if (moveRowToUpQueue(&rc))
		  return 1;
	      }
	    else
	      {
		rc = 0;
		// moveRowToUpQueue also increments matches_
		if (moveRowToUpQueue(convertRow_, hbaseAccessTdb().convertRowLen(), 
				     &rc, FALSE))
		  return 1;
	      }

	    step_ = INSERT_CLOSE;
	  }
	  break;

	case INSERT_CLOSE:
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
	  {
	    if (handleError(rc))
	      return rc;

	    retcode = ehi_->close();

	    step_ = DONE;
	  }
	  break;

	case DONE:
	  {
	    if (NOT hbaseAccessTdb().computeRowsAffected())
	      matches_ = 0;

	    if (handleDone(rc, matches_))
	      return rc;

	    step_ = NOT_STARTED;
	  }
	  break;

	} // switch

    } // while

  return WORK_OK;
}

ExHbaseAccessUpsertVsbbSQTcb::ExHbaseAccessUpsertVsbbSQTcb(
          const ExHbaseAccessTdb &hbaseAccessTdb, 
          ex_globals * glob ) :
  ExHbaseAccessInsertTcb( hbaseAccessTdb, glob)
{
  prevTailIndex_ = 0;

  nextRequest_ = qparent_.down->getHeadIndex();

  numRetries_ = 0;
  rowsInserted_ = 0;
  lastHandledStep_ = NOT_STARTED;
}

ExWorkProcRetcode ExHbaseAccessUpsertVsbbSQTcb::work()
{
  Lng32 retcode = 0;
  short rc = 0;

  ExMasterStmtGlobals *g = getGlobals()->
    castToExExeStmtGlobals()->castToExMasterStmtGlobals();


  while (!qparent_.down->isEmpty())
    {
      nextRequest_ = qparent_.down->getHeadIndex();

      ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();
      if (pentry_down->downState.request == ex_queue::GET_NOMORE)
	step_ = ALL_DONE;
     else if (pentry_down->downState.request == ex_queue::GET_EOD)
          if (currRowNum_ > rowsInserted_)
	{
	  step_ = PROCESS_INSERT_FLUSH_AND_CLOSE;

	}
          else
          {
            if (lastHandledStep_ == ALL_DONE)
               matches_=0;
            step_ = ALL_DONE;
          }
      switch (step_)
	{
	case NOT_STARTED:
	  {
	    matches_ = 0;
	    currRowNum_ = 0;
	    numRetries_ = 0;

	    prevTailIndex_ = 0;
	    lastHandledStep_ = NOT_STARTED;

	    nextRequest_ = qparent_.down->getHeadIndex();

	    rowsInserted_ = 0;

	    step_ = INSERT_INIT;
	  }
	  break;
	  
	case INSERT_INIT:
	  {
	    retcode = ehi_->init(getHbaseAccessStats());
	    if (setupError(retcode, "ExpHbaseInterface::init"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    table_.val = hbaseAccessTdb().getTableName();
	    table_.len = strlen(hbaseAccessTdb().getTableName());

            ExpTupleDesc * rowTD =
    		hbaseAccessTdb().workCriDesc_->getTupleDescriptor
                (hbaseAccessTdb().convertTuppIndex_);
            allocateDirectRowBufferForJNI(rowTD->numAttrs(), ROWSET_MAX_NO_ROWS);
            allocateDirectRowIDBufferForJNI(ROWSET_MAX_NO_ROWS);
            if (hbaseAccessTdb().getCanAdjustTrafParams())
            {
              if (hbaseAccessTdb().getWBSize() > 0)
              {
                retcode = ehi_->setWriteBufferSize(table_,
                                               hbaseAccessTdb().getWBSize());
                if (setupError(retcode, "ExpHbaseInterface::setWriteBufferSize"))
                {
                  step_ = HANDLE_ERROR;
                  break;
                }
              }
              retcode = ehi_->setWriteToWAL(table_,
                                               hbaseAccessTdb().getTrafWriteToWAL());
              if (setupError(retcode, "ExpHbaseInterface::setWriteToWAL"))
                {
                  step_ = HANDLE_ERROR;
                  break;
                }
            }

	    step_ = SETUP_INSERT;
	  }
	  break;

	case SETUP_INSERT:
	  {
	    step_ = EVAL_INSERT_EXPR;
	  }
	  break;

	case EVAL_INSERT_EXPR:
	  {
	    workAtp_->getTupp(hbaseAccessTdb().convertTuppIndex_)
	      .setDataPointer(convertRow_);
	    
	    if (convertExpr())
	      {
                insertRowlen_ = hbaseAccessTdb().convertRowLen_;
		ex_expr::exp_return_type evalRetCode =
		  convertExpr()->eval(pentry_down->getAtp(), workAtp_,
                                      NULL, -1, &insertRowlen_);
		if (evalRetCode == ex_expr::EXPR_ERROR)
		  {
		    step_ = HANDLE_ERROR;
		    break;
		  }
	      }

            genAndAssignSyskey(hbaseAccessTdb().convertTuppIndex_, convertRow_);
	    step_ = EVAL_CONSTRAINT;
	  }
	  break;

	case EVAL_CONSTRAINT:
	  {
	    rc = applyPred(scanExpr());
	    if (rc == 1) // expr is true or no expr
	      step_ = CREATE_MUTATIONS;
	    else if (rc == 0) // expr is false
	      step_ = INSERT_CLOSE;
	    else // error
	      step_ = HANDLE_ERROR;
	  }
	  break;

	case CREATE_MUTATIONS:
	  {
	    retcode = createDirectRowBuffer(
				      hbaseAccessTdb().convertTuppIndex_,
				      convertRow_,
				      hbaseAccessTdb().listOfUpdatedColNames(),
				      TRUE);
	    if (retcode == -1)
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    insColTSval_ = -1;
	    step_ = EVAL_ROWID_EXPR;
	  }
	  break;

	case EVAL_ROWID_EXPR:
	  {
	    if (evalRowIdExpr(TRUE) == -1)
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    copyRowIDToDirectBuffer(rowId_);

	    currRowNum_++;
	    matches_++;

	    if (currRowNum_ < ROWSET_MAX_NO_ROWS)
	      {
		step_ = DONE;
		break;
	      }

	    step_ = PROCESS_INSERT;
	  }
	  break;

	case PROCESS_INSERT:
	case PROCESS_INSERT_AND_CLOSE:
	case PROCESS_INSERT_FLUSH_AND_CLOSE:
	  {
            short numRowsInBuffer = patchDirectRowBuffers();
	    
	    retcode = ehi_->insertRows(table_,
                                       hbaseAccessTdb().getRowIDLen(),
				       rowIDs_,
                                       rows_,
				       hbaseAccessTdb().useHbaseXn(),
                                       insColTSval_,
                                       hbaseAccessTdb().getIsTrafLoadAutoFlush());
	    
	    if (getHbaseAccessStats())
	      {
		getHbaseAccessStats()->lobStats()->numReadReqs++;
		getHbaseAccessStats()->incUsedRows(numRowsInBuffer);
	      }
	    
	    if (setupError(retcode, "ExpHbaseInterface::insertRows"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }
            rowsInserted_ += numRowsInBuffer; 
	    if (step_ == PROCESS_INSERT_FLUSH_AND_CLOSE)
	      step_ = FLUSH_BUFFERS;
	    else if (step_ == PROCESS_INSERT_AND_CLOSE)
	      step_ = INSERT_CLOSE;
	    else
              step_ = ALL_DONE;
	  }
	  break;

	case INSERT_CLOSE:
	  {
	    retcode = ehi_->close();
	    if (setupError(retcode, "ExpHbaseInterface::close"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    step_ = ALL_DONE;
	  }
	  break;

	case HANDLE_ERROR:
	  {
	    if (handleError(rc))
	      return rc;

	    retcode = ehi_->close();

	    step_ = ALL_DONE;
	  }
	  break;

	case FLUSH_BUFFERS:
	  {
	    // add call to flushBuffers for this table. TBD.
	    retcode = ehi_->flushTable();
	    if (setupError(retcode, "ExpHbaseInterface::flushTable"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    step_ = INSERT_CLOSE;
	  }
	  break;

	case DONE:
	case ALL_DONE:
	  {
	    if (NOT hbaseAccessTdb().computeRowsAffected())
	      matches_ = 0;

	    if ((step_ == DONE) &&
		(qparent_.down->getLength() == 1))
	      {
		// only one row in the down queue.

		// Before we send input buffer to hbase, give parent
		// another chance in case there is more input data.
		// If parent doesn't input any more data on second (or
		// later) chances, then process the request.
		if (numRetries_ == 3)
		  {
		    numRetries_ = 0;

		    // Insert the current batch and then done.
		    step_ = PROCESS_INSERT_AND_CLOSE;
		    break;
		  }

		numRetries_++;
		return WORK_CALL_AGAIN;
	      }

	    if (handleDone(rc, (step_ == ALL_DONE ? matches_ : 0)))
	      return rc;
	    lastHandledStep_ = step_;

	    if (step_ == DONE)
	      step_ = SETUP_INSERT;
	    else
              step_ = NOT_STARTED;
	  }
	  break;

	} // switch

    } // while

  return WORK_OK;
}

ExHbaseAccessBulkLoadPrepSQTcb::ExHbaseAccessBulkLoadPrepSQTcb(
          const ExHbaseAccessTdb &hbaseAccessTdb,
          ex_globals * glob ) :
    ExHbaseAccessUpsertVsbbSQTcb( hbaseAccessTdb, glob),
    prevRowId_ (NULL),
    hdfs_(NULL),
    hdfsSampleFile_(NULL)
{
   hFileParamsInitialized_ = false;  ////temporary-- need better mechanism later
   //sortedListOfColNames_ = NULL;
   posVec_.clear();
}

ExHbaseAccessBulkLoadPrepSQTcb::~ExHbaseAccessBulkLoadPrepSQTcb()
{
  // Flush and close sample file if used, and disconnect from HDFS.
  if (hdfs_)
    {
      if (hdfsSampleFile_)
        {
          hdfsFlush(hdfs_, hdfsSampleFile_);
          hdfsCloseFile(hdfs_, hdfsSampleFile_);
        }
      hdfsDisconnect(hdfs_);
    }

}

// Given the type information available via the argument, return the name of
// the Hive type we use to represent it in the Hive sample table created by
// the bulk load utility.
static const char* TrafToHiveType(Attributes* attrs)
{
  Int64 maxValue = 0;
  Int16 precision = 0;
  Int16 scale = 0;
  Int16 datatype = attrs->getDatatype();

  if (DFS2REC::isInterval(datatype))
  {
    precision = dynamic_cast<SimpleType*>(attrs)->getPrecision();
    scale = dynamic_cast<SimpleType*>(attrs)->getScale();
  }

  switch (datatype)
  {
    case REC_BIN16_SIGNED:
    case REC_BIN16_UNSIGNED:
    case REC_BPINT_UNSIGNED:
      return "smallint";

    case REC_BIN32_SIGNED:
    case REC_BIN32_UNSIGNED:
      return "int";

    case REC_BIN64_SIGNED:
      return "bigint";

    case REC_TDM_FLOAT32:
    case REC_IEEE_FLOAT32:
      return "float";

    case REC_TDM_FLOAT64:
    case REC_IEEE_FLOAT64:
      return "double";

    case REC_DECIMAL_UNSIGNED:
    case REC_DECIMAL_LS:
    case REC_DECIMAL_LSE:
      maxValue = (Int64)pow(10, dynamic_cast<SimpleType*>(attrs)->getPrecision());
      break;

    //case REC_NUM_BIG_UNSIGNED: return extFormat? (char *)"NUMERIC":(char *)"REC_NUM_BIG_UNSIGNED";
    //case REC_NUM_BIG_SIGNED: return extFormat? (char *)"NUMERIC":(char *)"REC_NUM_BIG_SIGNED";

    case REC_BYTE_F_ASCII:
    case REC_NCHAR_F_UNICODE:
    case REC_BYTE_V_ASCII:
    case REC_NCHAR_V_UNICODE:
    case REC_BYTE_V_ASCII_LONG:
    case REC_BYTE_V_ANSI:
    case REC_BYTE_V_ANSI_DOUBLE:
    case REC_SBYTE_LOCALE_F:
    case REC_MBYTE_LOCALE_F:
    case REC_MBYTE_F_SJIS:
    case REC_MBYTE_V_SJIS:
      return "string";

    case REC_DATETIME:
      return "timestamp";

    case REC_INT_YEAR:
    case REC_INT_MONTH:
    case REC_INT_DAY:
    case REC_INT_HOUR:
    case REC_INT_MINUTE:
      maxValue = (Int64)pow(10, precision);
      break;

    case REC_INT_SECOND:
      maxValue = (Int64)pow(10, precision + scale);
      break;

    case REC_INT_YEAR_MONTH:
      maxValue = 12 * (Int64)pow(10, precision);
      break;

    case REC_INT_DAY_HOUR:
      maxValue = 24 * (Int64)pow(10, precision);
      break;

    case REC_INT_HOUR_MINUTE:
      maxValue = 60 * (Int64)pow(10, precision);
      break;

    case REC_INT_DAY_MINUTE:
      maxValue = 24 * 60 * (Int64)pow(10, precision);
      break;

    case REC_INT_MINUTE_SECOND:
      maxValue = (Int64)pow(10, precision + 2 + scale);
      break;

    case REC_INT_HOUR_SECOND:
      maxValue = (Int64)pow(10, precision + 4 + scale);
      break;

    case REC_INT_DAY_SECOND:
      maxValue = (Int64)pow(10, precision + 5 + scale);
      break;

    default:

      break;
  }  // switch

  //assert(maxValue > 0);
  if (maxValue < SHRT_MAX)
    return "smallint";
  else if (maxValue <= INT_MAX)
    return "int";
  else
    return "bigint";
}

// Return in ddlText the Hive statement to create the Hive external table that
// that will hold a sample for the Trafodion table being loaded. The files
// containing the sample data are written independently to HDFS and linked to
// the Hive table by the location clause in the generated Hive DDL.
void ExHbaseAccessBulkLoadPrepSQTcb::getHiveCreateTableDDL(NAString& hiveSampleTblNm, NAString& ddlText)
{
  ExHbaseAccessTdb& hbaTdb = ((ExHbaseAccessTdb&)hbaseAccessTdb());
  ddlText = "create external table ";
  ddlText.append(hiveSampleTblNm).append("(");

  ExpTupleDesc* td = hbaTdb.workCriDesc_->getTupleDescriptor(hbaTdb.convertTuppIndex_);
  hbaTdb.listOfUpdatedColNames()->position();
  Attributes* attrs;
  char colNumBuf[12];
  for (UInt32 i = 0; i < td->numAttrs(); i++)
    {
      attrs = td->getAttr(i);
      sprintf(colNumBuf, "%d", *(UInt32*) hbaTdb.listOfUpdatedColNames()->getCurr());
      ddlText.append("col").append(colNumBuf).append(" ");
      ddlText.append(TrafToHiveType(attrs));
      if (i < td->numAttrs() - 1)
        ddlText.append(", ");
      else
        ddlText.append(") row format delimited fields terminated by '|' location '")
               .append(hbaTdb.getSampleLocation())
               .append((const char*)hbaTdb.getTableName())
               .append("/'");
      hbaTdb.listOfUpdatedColNames()->advance();
    }
}

ExWorkProcRetcode ExHbaseAccessBulkLoadPrepSQTcb::work()
{
  Lng32 retcode = 0;
  short rc = 0;

  ExMasterStmtGlobals *g = getGlobals()->
    castToExExeStmtGlobals()->castToExMasterStmtGlobals();

  NABoolean eodSeen = false;

  // Get the percentage of rows to include in the ustat sample table. A value of
  // 0 indicates that no sample table is to be created.
  static NABoolean displayed = FALSE;
  double samplingRate = ((ExHbaseAccessTdb&)hbaseAccessTdb()).getSamplingRate();

  while (!qparent_.down->isEmpty())
  {
    nextRequest_ = qparent_.down->getHeadIndex();

    ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();
    if (pentry_down->downState.request == ex_queue::GET_NOMORE)
      step_ = ALL_DONE;
    else if (pentry_down->downState.request == ex_queue::GET_EOD &&
             step_ != HANDLE_ERROR && lastHandledStep_ != HANDLE_ERROR)
      if (currRowNum_ > rowsInserted_)
      {
        step_ = PROCESS_INSERT;
      }
      else
      {
        if (lastHandledStep_ == ALL_DONE)
          matches_ = 0;
        step_ = ALL_DONE;
        eodSeen = true;
      }

    switch (step_)
    {
      case NOT_STARTED:
      {
        matches_ = 0;
        currRowNum_ = 0;
        numRetries_ = 0;

        prevTailIndex_ = 0;
        lastHandledStep_ = NOT_STARTED;

        nextRequest_ = qparent_.down->getHeadIndex();

        rowsInserted_ = 0;
        step_ = INSERT_INIT;
      }
        break;

      case INSERT_INIT:
      {
        retcode = ehi_->initHBLC(getHbaseAccessStats());

        if (setupError(retcode, "ExpHbaseInterface::initHBLC"))
        {
          step_ = HANDLE_ERROR;
          break;
        }

        table_.val = hbaseAccessTdb().getTableName();
        table_.len = strlen(hbaseAccessTdb().getTableName());
        short numCols = 0;

        if (!hFileParamsInitialized_)
        {
          importLocation_= std::string(((ExHbaseAccessTdb&)hbaseAccessTdb()).getLoadPrepLocation()) +
                                        ((ExHbaseAccessTdb&)hbaseAccessTdb()).getTableName() ;
          familyLocation_ = std::string(importLocation_ + "/#1");
          Lng32 fileNum = getGlobals()->castToExExeStmtGlobals()->getMyInstanceNumber();
          hFileName_ = std::string("hfile");
          char hFileName[50];
          snprintf(hFileName, 50, "hfile%d", fileNum);
          hFileName_ = hFileName;

          NAString hiveDDL;
          NAString hiveSampleTblNm;
          if (samplingRate > 0 && fileNum == 0) // master exec creates hive sample table
          {
            hiveSampleTblNm = ((ExHbaseAccessTdb&)hbaseAccessTdb()).getTableName();
            TrafToHiveSampleTableName(hiveSampleTblNm);
            getHiveCreateTableDDL(hiveSampleTblNm, hiveDDL);
          }
          retcode = ehi_->initHFileParams(table_, familyLocation_, hFileName_,
                                          hbaseAccessTdb().getMaxHFileSize(),
                                          hiveSampleTblNm.data(), hiveDDL.data());
          hFileParamsInitialized_ = true;

          if (samplingRate > 0)
          {
            // Seed random number generator (used to select rows to write to sample table).
            srand(time(0));

            // Set up HDFS file for sample table.
            hdfs_ = hdfsConnect("default", 0);
            Text samplePath = std::string(((ExHbaseAccessTdb&)hbaseAccessTdb()).getSampleLocation()) +
                                          ((ExHbaseAccessTdb&)hbaseAccessTdb()).getTableName() ;
            char filePart[10];
            sprintf(filePart, "/%d", fileNum);
            samplePath.append(filePart);
            hdfsSampleFile_ = hdfsOpenFile(hdfs_, samplePath.data(), O_WRONLY|O_CREAT, 0, 0, 0);
            //if (!hdfsSampleFile_)
            //  printf("*** Failed to open %s for writing.\n", samplePath.data());
            //else
            //  printf("*** File %s was opened successfully.\n", samplePath.data());
          }

//              sortedListOfColNames_ = new  Queue(); //delete wehn done
          posVec_.clear();
          hbaseAccessTdb().listOfUpdatedColNames()->position();
          while (NOT hbaseAccessTdb().listOfUpdatedColNames()->atEnd())
          {
            UInt32 pos = *(UInt32*) hbaseAccessTdb().listOfUpdatedColNames()->getCurr();
            posVec_.push_back(pos);
            hbaseAccessTdb().listOfUpdatedColNames()->advance();
            numCols++;
          }
//              sortQualifiers(hbaseAccessTdb().listOfUpdatedColNames(),sortedListOfColNames_, posVec_);
        }
        if (setupError(retcode, "ExpHbaseInterface::createHFile"))
        {
          step_ = HANDLE_ERROR;
          break;
        }
        allocateDirectRowBufferForJNI(
                 numCols,
                 ROWSET_MAX_NO_ROWS);
        allocateDirectRowIDBufferForJNI(ROWSET_MAX_NO_ROWS);
        step_ = SETUP_INSERT;
       }
       break;

      case SETUP_INSERT:
      {
        step_ = EVAL_INSERT_EXPR;
      }
        break;

      case EVAL_INSERT_EXPR:
      {
        workAtp_->getTupp(hbaseAccessTdb().convertTuppIndex_)
          .setDataPointer(convertRow_);

        if (convertExpr())
          {
            insertRowlen_ = hbaseAccessTdb().convertRowLen_;
            ex_expr::exp_return_type evalRetCode =
              convertExpr()->eval(pentry_down->getAtp(), workAtp_,
                                  NULL, -1, &insertRowlen_);
            if (evalRetCode == ex_expr::EXPR_ERROR)
              {
                step_ = HANDLE_ERROR;
                break;
              }
          }

        genAndAssignSyskey(hbaseAccessTdb().convertTuppIndex_, convertRow_);

        step_ = EVAL_ROWID_EXPR;
      }
        break;

      case EVAL_ROWID_EXPR:
      {
        if (evalRowIdExpr(TRUE) == -1)
        {
          step_ = HANDLE_ERROR;
          break;
        }

        // duplicates (same rowid) are not allowed in Hfiles. adding duplicates causes Hfiles to generate
        // errors
        if (prevRowId_ == NULL)
        {
          prevRowId_ = new char[rowId_.len + 1];
          memmove(prevRowId_, rowId_.val, rowId_.len);
        }
        else
        {
          // rows are supposed to sorted by rowId and to detect duplicates
          // compare the current rowId to the previous one
          if (memcmp(prevRowId_, rowId_.val, rowId_.len) == 0)
          {
            if (((ExHbaseAccessTdb&) hbaseAccessTdb()).getNoDuplicates())
           {
              //8110 Duplicate rows detected.
              ComDiagsArea * diagsArea = NULL;
              ExRaiseSqlError(getHeap(), &diagsArea,
                              (ExeErrorCode)(8110));
              pentry_down->setDiagsArea(diagsArea);
              step_ = HANDLE_ERROR;
              break;
           }
            else
            {
              //skip duplicate
              step_ = DONE;
              break;
            }
          }
          memmove(prevRowId_, rowId_.val, rowId_.len);
        }
        step_ = CREATE_MUTATIONS;
      }
      break;

      case CREATE_MUTATIONS:
      {
          retcode = createDirectRowBuffer(
                                      hbaseAccessTdb().convertTuppIndex_,
                                      convertRow_,
                                      hbaseAccessTdb().listOfUpdatedColNames(),
                                      FALSE, //TRUE,
                                      &posVec_,
                                      samplingRate);
        if (retcode == -1)
        {
          //need to re-verify error handling
          step_ = HANDLE_ERROR;
          break;
        }

        //insColTSval_ = -1;

        copyRowIDToDirectBuffer( rowId_);
        currRowNum_++;
        matches_++;
        if (currRowNum_ < ROWSET_MAX_NO_ROWS)
        {
          step_ = DONE;
          break;
        }
        step_ = PROCESS_INSERT;
      }
        break;

      case PROCESS_INSERT:
      {
        short numRowsInBuffer = patchDirectRowBuffers();
        retcode = ehi_->addToHFile(hbaseAccessTdb().getRowIDLen(),
                                   rowIDs_,
                                   rows_);

        if (setupError(retcode, "ExpHbaseInterface::addToHFile"))
        {
           step_ = HANDLE_ERROR;
           break;
        }
        rowsInserted_ += numRowsInBuffer;

        if (getHbaseAccessStats())
        {
          getHbaseAccessStats()->lobStats()->numReadReqs++;
          getHbaseAccessStats()->incUsedRows(numRowsInBuffer);
        }

        step_ = ALL_DONE;
      }
        break;

      case HANDLE_ERROR:
      {
        // maybe we can continue if error is not fatal and logs the execptions-- will be done later
        //
        if (handleError(rc))
          return rc;

        retcode = ehi_->close();

        lastHandledStep_ =HANDLE_ERROR;
        step_ = ALL_DONE;
      }
        break;

      case HANDLE_EXCEPTION:
      {
        // -- log the exception rows in a hdfs
        //rows that don't pass the check constarints criteria and others
        ex_assert(0, " state not handled yet")
        step_ = SETUP_INSERT;
      }
        break;
      case DONE:
      case ALL_DONE:
      {
        if (handleDone(rc, (step_ == ALL_DONE ? matches_ : 0)))
          return rc;
        lastHandledStep_ = step_;

        if (step_ == DONE)
          step_ = SETUP_INSERT;
        else
        {
          step_ = NOT_STARTED;
          if (eodSeen)
          {
            ehi_->closeHFile(table_);
            hFileParamsInitialized_ = false;
            retcode = ehi_->close();
          }
        }
      }
        break;

    } // switch

  } // while

  return WORK_OK;
}


// UMD (unique UpdMergeDel on Trafodion tables)
ExHbaseUMDtrafUniqueTaskTcb::ExHbaseUMDtrafUniqueTaskTcb
(ExHbaseAccessUMDTcb * tcb)
  :  ExHbaseTaskTcb(tcb)
  , step_(NOT_STARTED)
{
}

void ExHbaseUMDtrafUniqueTaskTcb::init() 
{
  step_ = NOT_STARTED;
}

ExWorkProcRetcode ExHbaseUMDtrafUniqueTaskTcb::work(short &rc)
{
  Lng32 retcode = 0;
  rc = 0;

  while (1)
    {
      ex_queue_entry *pentry_down = tcb_->qparent_.down->getHeadEntry();

      switch (step_)
	{
	case NOT_STARTED:
	  {
	    rowUpdated_ = FALSE;

	    step_ = SETUP_UMD;
	  }
	  break;

	case SETUP_UMD:
	  {
	     tcb_->currRowidIdx_ = 0;

	     step_ = GET_NEXT_ROWID;
	  }
	  break;

	case GET_NEXT_ROWID:
	  {
	    if (tcb_->currRowidIdx_ ==  tcb_->rowIds_.entries())
	      {
		step_ = GET_CLOSE;
		break;
	      }

	    if ((tcb_->hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::DELETE_) &&
		(tcb_->hbaseAccessTdb().canDoCheckAndUpdel()))
	      {
		if (tcb_->hbaseAccessTdb().hbaseSqlIUD())
		  step_ = CHECK_AND_DELETE_ROW;
		else
		  step_ = DELETE_ROW;
		break;
	      }
	    else if ((tcb_->hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::UPDATE_) &&
		     (tcb_->hbaseAccessTdb().canDoCheckAndUpdel()))
	      {
		step_ = CREATE_UPDATED_ROW;
		break;
	      }

	    StrVec columns;
	    retcode =  tcb_->ehi_->getRowOpen( tcb_->table_,  
					       tcb_->rowIds_[tcb_->currRowidIdx_],
					       tcb_->columns_, -1);
	    if ( tcb_->setupError(retcode, "ExpHbaseInterface::getRowOpen"))
	      step_ = HANDLE_ERROR;
	    else
	      step_ = NEXT_ROW;
	  }
	  break;

	case NEXT_ROW:
	  {
	    retcode =  tcb_->ehi_->nextRow();
	    if ( (retcode == HBASE_ACCESS_EOD) || (retcode == HBASE_ACCESS_EOR) )
	      {
		if (tcb_->hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::MERGE_)
		  {
		    // didn't find the row, cannot update.
		    // evaluate the mergeInsert expr and insert the row.
		    step_ = CREATE_MERGE_INSERTED_ROW;
		    break;
		  }

		tcb_->currRowidIdx_++;
		step_ = GET_NEXT_ROWID;
		break;
	      }

	    if ( tcb_->setupError(retcode, "ExpHbaseInterface::nextRow"))
	      step_ = HANDLE_ERROR;
	    else if ((tcb_->hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::DELETE_) &&
		     (! tcb_->scanExpr()) &&
		     (NOT tcb_->hbaseAccessTdb().returnRow()))
	      step_ = DELETE_ROW;
	    else
	      step_ = CREATE_FETCHED_ROW;
	  }
	  break;

	  case CREATE_FETCHED_ROW:
	    {
	    retcode =  tcb_->createSQRowDirect();
            if (retcode == HBASE_ACCESS_NO_ROW)
	    {
	       step_ = NEXT_ROW;
	       break;
	    }
	    if (retcode < 0)
	    {
	        rc = (short)retcode;
	        tcb_->setupError(rc, "createSQRowDirect");
	        step_ = HANDLE_ERROR;
	        break;
	    }
	    if (retcode != HBASE_ACCESS_SUCCESS)
	    {
	        step_ = HANDLE_ERROR;
	        break;
	    }

	    step_ = APPLY_PRED;
	  }
	  break;

	  case APPLY_PRED:
	  {
	    rc =  tcb_->applyPred(tcb_->scanExpr());
	    if (rc == 1)
	      {
		if (tcb_->hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::DELETE_)
		  step_ = DELETE_ROW;
		else if ((tcb_->hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::MERGE_) &&
			 (tcb_->mergeUpdScanExpr()))
		  step_ = APPLY_MERGE_UPD_SCAN_PRED;
		else
		  step_ = CREATE_UPDATED_ROW; 
	      }
	    else if (rc == -1)
	      step_ = HANDLE_ERROR;
	    else
	      {
		if (tcb_->hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::MERGE_)
		  {
		    // didn't find the row, cannot update.
		    // evaluate the mergeInsert expr and insert the row.
		    step_ = CREATE_MERGE_INSERTED_ROW;
		    break;
		  }

		tcb_->currRowidIdx_++;
		step_ = GET_NEXT_ROWID;
	      }
	  }
	  break;

	  case APPLY_MERGE_UPD_SCAN_PRED:
	  {
	    rc =  tcb_->applyPred(tcb_->mergeUpdScanExpr());
	    if (rc == 1)
	      {
		step_ = CREATE_UPDATED_ROW; 
	      }
	    else if (rc == -1)
	      step_ = HANDLE_ERROR;
	    else
	      {
		tcb_->currRowidIdx_++;
		step_ = GET_NEXT_ROWID;
	      }
	  }
	  break;

	case CREATE_UPDATED_ROW:
	  {
	    if (! tcb_->updateExpr())
	      {
		tcb_->currRowidIdx_++;
		
		step_ = GET_NEXT_ROWID;
		break;
	      }

	    tcb_->workAtp_->getTupp(tcb_->hbaseAccessTdb().updateTuppIndex_)
	      .setDataPointer(tcb_->updateRow_);
	    
	    if (tcb_->updateExpr())
	      {
                tcb_->insertRowlen_ = tcb_->hbaseAccessTdb().updateRowLen_;
		ex_expr::exp_return_type evalRetCode =
		  tcb_->updateExpr()->eval(pentry_down->getAtp(), tcb_->workAtp_,
                                           NULL, -1, &tcb_->insertRowlen_);
		if (evalRetCode == ex_expr::EXPR_ERROR)
		  {
		    step_ = HANDLE_ERROR;
		    break;
		  }
	      }

	    step_ = EVAL_CONSTRAINT;
	  }
	  break;

	case EVAL_CONSTRAINT:
	  {
	    rc = tcb_->applyPred(tcb_->mergeUpdScanExpr());
	    if (rc == 1) // expr is true or no expr
	      step_ = CREATE_MUTATIONS;
	    else if (rc == 0) // expr is false
	      step_ = NEXT_ROW_AFTER_UPDATE;
	    else // error
	      step_ = HANDLE_ERROR;
	  }
	  break;

	case CREATE_MUTATIONS:
	  {
	    rowUpdated_ = TRUE;
            // Merge can result in inserting rows.
            // Use Number of columns in insert rather number
            // of columns in update if an insert is involved in this tcb
            if (tcb_->hbaseAccessTdb().getAccessType() 
                  == ComTdbHbaseAccess::MERGE_)
            {
              ExpTupleDesc * rowTD = NULL;
              if (tcb_->mergeInsertExpr())
                {
                  rowTD = tcb_->hbaseAccessTdb().workCriDesc_->getTupleDescriptor
                    (tcb_->hbaseAccessTdb().mergeInsertTuppIndex_);
                }
              else
                {
                  rowTD = tcb_->hbaseAccessTdb().workCriDesc_->getTupleDescriptor
                    (tcb_->hbaseAccessTdb().updateTuppIndex_);
                }
                
               if (rowTD->numAttrs() > 0)
                  tcb_->allocateDirectRowBufferForJNI(rowTD->numAttrs());
            } 

	    retcode = tcb_->createDirectRowBuffer( tcb_->hbaseAccessTdb().updateTuppIndex_,
					    tcb_->updateRow_, 
					    tcb_->hbaseAccessTdb().listOfUpdatedColNames(),
					    TRUE);
	    if (retcode == -1)
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    if (tcb_->hbaseAccessTdb().canDoCheckAndUpdel())
	      step_ = CHECK_AND_UPDATE_ROW;
	    else
	      step_ = UPDATE_ROW;
	  }
	  break;

	case CREATE_MERGE_INSERTED_ROW:
	  {
	    if (! tcb_->mergeInsertExpr())
	      {
		tcb_->currRowidIdx_++;
		
		step_ = GET_NEXT_ROWID;
		break;
	      }

	    tcb_->workAtp_->getTupp(tcb_->hbaseAccessTdb().mergeInsertTuppIndex_)
	      .setDataPointer(tcb_->mergeInsertRow_);
	    
	    if (tcb_->mergeInsertExpr())
	      {
                tcb_->insertRowlen_ = tcb_->hbaseAccessTdb().mergeInsertRowLen_;
		ex_expr::exp_return_type evalRetCode =
		  tcb_->mergeInsertExpr()->eval(pentry_down->getAtp(), tcb_->workAtp_,
                                                NULL, -1, &tcb_->insertRowlen_);
		if (evalRetCode == ex_expr::EXPR_ERROR)
		  {
		    step_ = HANDLE_ERROR;
		    break;
		  }
	      }

	    if (tcb_->hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::MERGE_)
	      rowUpdated_ = FALSE;

	    retcode = tcb_->createDirectRowBuffer( tcb_->hbaseAccessTdb().mergeInsertTuppIndex_,
					    tcb_->mergeInsertRow_,
					    tcb_->hbaseAccessTdb().listOfMergedColNames());
	    if (retcode == -1)
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    if (tcb_->hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::MERGE_)
	      step_ = CHECK_AND_INSERT_ROW;
	    else
	    step_ = UPDATE_ROW;
	  }
	  break;

	case UPDATE_ROW:
	  {
            retcode =  tcb_->ehi_->insertRow(tcb_->table_,
                                             tcb_->rowIds_[tcb_->currRowidIdx_],
	                                     tcb_->row_,
					     (tcb_->hbaseAccessTdb().useHbaseXn() ? TRUE : FALSE),
					     -1); //colTS_
	    if ( tcb_->setupError(retcode, "ExpHbaseInterface::insertRow"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    if (tcb_->getHbaseAccessStats())
	      tcb_->getHbaseAccessStats()->incUsedRows();

	    // matches will get incremented during return row.
	    if (NOT tcb_->hbaseAccessTdb().returnRow())
	      tcb_->matches_++;

	    step_ = NEXT_ROW_AFTER_UPDATE;
	  }
	  break;

	case CHECK_AND_UPDATE_ROW:
	  {
	    Text columnToCheck;
	    Text colValToCheck;
	    rc = tcb_->evalKeyColValExpr(columnToCheck, colValToCheck);
	    if (rc == -1)
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    retcode =  tcb_->ehi_->checkAndUpdateRow(tcb_->table_,
                                                     tcb_->rowIds_[tcb_->currRowidIdx_],
						     tcb_->row_,
						     columnToCheck,
						     colValToCheck,

                                                     tcb_->hbaseAccessTdb().useHbaseXn(),
						     -1 //colTS_,
						     );


	    if (retcode == HBASE_ROW_NOTFOUND_ERROR)
	      {
		step_ = NEXT_ROW_AFTER_UPDATE;
		break;
	      }

	    if ( tcb_->setupError(retcode, "ExpHbaseInterface::checkAndUpdateRow"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    if (tcb_->getHbaseAccessStats())
	      tcb_->getHbaseAccessStats()->incUsedRows();

	    // matches will get incremented during return row.
	    if (NOT tcb_->hbaseAccessTdb().returnRow())
	      tcb_->matches_++;

	    step_ = NEXT_ROW_AFTER_UPDATE;
	  }
	  break;

	case CHECK_AND_INSERT_ROW:
	  {
	    Text rowIdRow;
	    if (tcb_->mergeInsertRowIdExpr())
	      {
		tcb_->workAtp_->getTupp(tcb_->hbaseAccessTdb().mergeInsertRowIdTuppIndex_)
		  .setDataPointer(tcb_->rowIdRow_);

		ex_expr::exp_return_type evalRetCode =
		  tcb_->mergeInsertRowIdExpr()->eval(pentry_down->getAtp(), tcb_->workAtp_);
		if (evalRetCode == ex_expr::EXPR_ERROR)
		  {
		    step_ = HANDLE_ERROR;
		    break;
		  }

		rowIdRow.assign(tcb_->rowIdRow_, tcb_->hbaseAccessTdb().getRowIDLen());
	      }
            HbaseStr rowID;
            if (tcb_->mergeInsertRowIdExpr())
            {
               rowID.val = (char *)rowIdRow.data();
               rowID.len = rowIdRow.size();
            }
            else
            {
               rowID.val = (char *)tcb_->rowIds_[tcb_->currRowidIdx_].val;
               rowID.len = tcb_->rowIds_[tcb_->currRowidIdx_].len;
            }
	    retcode =  tcb_->ehi_->checkAndInsertRow(tcb_->table_,
                                                     rowID,
                                                     tcb_->row_,
                                                     tcb_->hbaseAccessTdb().useHbaseXn(),
						     -1 // colTS_
                                                     );

	    if (retcode == HBASE_DUP_ROW_ERROR)
	      {
		ComDiagsArea * diagsArea = NULL;
		ExRaiseSqlError(tcb_->getHeap(), &diagsArea, 
				(ExeErrorCode)(8102));
		pentry_down->setDiagsArea(diagsArea);
		step_ = HANDLE_ERROR;
		break;
	      }
	    else if (tcb_->setupError(retcode, "ExpHbaseInterface::insertRow"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }
	    
	    if (tcb_->getHbaseAccessStats())
	      tcb_->getHbaseAccessStats()->incUsedRows();

	    // matches will get incremented during return row.
	    if (NOT tcb_->hbaseAccessTdb().returnRow())
	      tcb_->matches_++;

	    step_ = NEXT_ROW_AFTER_UPDATE;
	  }
	  break;

	case NEXT_ROW_AFTER_UPDATE:
	  {
	    tcb_->currRowidIdx_++;
	    
	    if (tcb_->hbaseAccessTdb().returnRow())
	      {
		step_ = EVAL_RETURN_ROW_EXPRS;
		break;
	      }

	    step_ = GET_NEXT_ROWID;
	  }
	  break;

	case DELETE_ROW:
	  {
	    LIST(HbaseStr) columns(tcb_->getHeap());
            retcode =  tcb_->ehi_->deleteRow(tcb_->table_,
                                             tcb_->rowIds_[tcb_->currRowidIdx_],
	                                     columns,
                                             tcb_->hbaseAccessTdb().useHbaseXn(),
 
                                             -1 //colTS_
                                             );



	    if ( tcb_->setupError(retcode, "ExpHbaseInterface::deleteRow"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }
	    
	    if (tcb_->getHbaseAccessStats())
	      tcb_->getHbaseAccessStats()->incUsedRows();

	    tcb_->currRowidIdx_++;
	    
	    if (tcb_->hbaseAccessTdb().returnRow())
	      {
		step_ = RETURN_ROW;
		break;
	      }

	    tcb_->matches_++;

	    step_ = GET_NEXT_ROWID;
	  }
	  break;

	case CHECK_AND_DELETE_ROW:
	  {
	    Text columnToCheck;
	    Text colValToCheck;
	    rc = tcb_->evalKeyColValExpr(columnToCheck, colValToCheck);
	    if (rc == -1)
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    retcode =  tcb_->ehi_->checkAndDeleteRow(tcb_->table_,
                                                     tcb_->rowIds_[tcb_->currRowidIdx_],
						     columnToCheck, 
						     colValToCheck,
                                                     tcb_->hbaseAccessTdb().useHbaseXn(),


						     -1 //colTS_
						     );



	    if (retcode == HBASE_ROW_NOTFOUND_ERROR)
	      {
		tcb_->currRowidIdx_++;
		step_ = GET_NEXT_ROWID;
		break;
	      }

	    if ( tcb_->setupError(retcode, "ExpHbaseInterface::deleteRow"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }
	    
	    tcb_->currRowidIdx_++;
	    
	    if (tcb_->getHbaseAccessStats())
	      tcb_->getHbaseAccessStats()->incUsedRows();

	    if (tcb_->hbaseAccessTdb().returnRow())
	      {
		step_ = RETURN_ROW;
		break;
	      }

	    tcb_->matches_++;

	    step_ = GET_NEXT_ROWID;
	  }
	  break;

	case RETURN_ROW:
	  {
	    if (tcb_->qparent_.up->isFull())
	      {
		rc = WORK_OK;
		return 1;
	      }
	    
	    rc = 0;
	    // moveRowToUpQueue also increments matches_
	    if (tcb_->moveRowToUpQueue(tcb_->convertRow_, tcb_->hbaseAccessTdb().convertRowLen(), 
				       &rc, FALSE))
	      return 1;

	    if ((pentry_down->downState.request == ex_queue::GET_N) &&
		(pentry_down->downState.requestValue == tcb_->matches_))
	      {
		step_ = GET_CLOSE;
		break;
	      }

	    step_ = GET_NEXT_ROWID;
	  }
	  break;

	case EVAL_RETURN_ROW_EXPRS:
	  {
	    ex_queue_entry * up_entry = tcb_->qparent_.up->getTailEntry();

	    rc = 0;

	    // allocate tupps where returned rows will be created
	    if (tcb_->allocateUpEntryTupps(
					   tcb_->hbaseAccessTdb().returnedFetchedTuppIndex_,
					   tcb_->hbaseAccessTdb().returnFetchedRowLen_,
					   tcb_->hbaseAccessTdb().returnedUpdatedTuppIndex_,
					   tcb_->hbaseAccessTdb().returnUpdatedRowLen_,
					   FALSE,
					   &rc))
	      return 1;
	    
	    ex_expr::exp_return_type exprRetCode;

	    char * fetchedDataPtr = NULL;
	    char * updatedDataPtr = NULL;
	    if (tcb_->returnFetchExpr())
	      {
		exprRetCode =
		  tcb_->returnFetchExpr()->eval(up_entry->getAtp(), tcb_->workAtp_);
		if (exprRetCode == ex_expr::EXPR_ERROR)
		  {
		    step_ = HANDLE_ERROR;
		    break;
		  }
		fetchedDataPtr = up_entry->getAtp()->getTupp(tcb_->hbaseAccessTdb().returnedFetchedTuppIndex_).getDataPointer();
		
	      }

	    if (rowUpdated_)
	      {
		if (tcb_->returnUpdateExpr())
		  {
		    exprRetCode =
		      tcb_->returnUpdateExpr()->eval(up_entry->getAtp(), tcb_->workAtp_);
		    if (exprRetCode == ex_expr::EXPR_ERROR)
		      {
			step_ = HANDLE_ERROR;
			break;
		      }
		    updatedDataPtr = 
		      up_entry->getAtp()->getTupp(tcb_->hbaseAccessTdb().returnedUpdatedTuppIndex_).getDataPointer();
		  }
	      }
	    else
	      {
		if (tcb_->returnMergeInsertExpr())
		  {
		    exprRetCode =
		      tcb_->returnMergeInsertExpr()->eval(up_entry->getAtp(), tcb_->workAtp_);
		    if (exprRetCode == ex_expr::EXPR_ERROR)
		      {
			step_ = HANDLE_ERROR;
			break;
		      }
		    updatedDataPtr = 
		      up_entry->getAtp()->getTupp(tcb_->hbaseAccessTdb().returnedUpdatedTuppIndex_).getDataPointer();
		  }
	      }

	    step_ = RETURN_UPDATED_ROWS;
	  }
	  break;

	case RETURN_UPDATED_ROWS:
	  {
	    rc = 0;
	    // moveRowToUpQueue also increments matches_
	    if (tcb_->moveRowToUpQueue(&rc))
	      return 1;

	    if ((pentry_down->downState.request == ex_queue::GET_N) &&
		(pentry_down->downState.requestValue == tcb_->matches_))
	      {
		step_ = GET_CLOSE;
		break;
	      }

	    step_ = GET_NEXT_ROWID;
	  }
	  break;

	case GET_CLOSE:
	  {
	    retcode =  tcb_->ehi_->getClose();
	    if ( tcb_->setupError(retcode, "ExpHbaseInterface::getClose"))
	      step_ = HANDLE_ERROR;
	    else
	      step_ = DONE;
	  }
	  break;

	case HANDLE_ERROR:
	  {
	    step_ = NOT_STARTED;
	    return -1;
	  }
	  break;

	case DONE:
	  {
	    step_ = NOT_STARTED;
	    return 0;
	  }
	  break;

	}// switch

    } // while

}

// UMD (unique UpdMergeDel on hbase tables. Well, Merge not supported yet)
ExHbaseUMDnativeUniqueTaskTcb::ExHbaseUMDnativeUniqueTaskTcb
(ExHbaseAccessUMDTcb * tcb)
  :  ExHbaseUMDtrafUniqueTaskTcb(tcb)
  , step_(NOT_STARTED)
{
}

void ExHbaseUMDnativeUniqueTaskTcb::init() 
{
  step_ = NOT_STARTED;
}

ExWorkProcRetcode ExHbaseUMDnativeUniqueTaskTcb::work(short &rc)
{
  Lng32 retcode = 0;
  rc = 0;

  while (1)
    {
      ex_queue_entry *pentry_down = tcb_->qparent_.down->getHeadEntry();

      switch (step_)
	{
	case NOT_STARTED:
	  {
	    rowUpdated_ = FALSE;

	    step_ = SETUP_UMD;
	  }
	  break;

	case SETUP_UMD:
	  {
	     tcb_->currRowidIdx_ = 0;

	     tcb_->setupListOfColNames(tcb_->hbaseAccessTdb().listOfDeletedColNames(),
				       tcb_->deletedColumns_);

	     tcb_->setupListOfColNames(tcb_->hbaseAccessTdb().listOfFetchedColNames(),
				       tcb_->columns_);
	     
	     step_ = GET_NEXT_ROWID;
	  }
	  break;

	case GET_NEXT_ROWID:
	  {
	    if (tcb_->currRowidIdx_ ==  tcb_->rowIds_.entries())
	      {
		step_ = GET_CLOSE;
		break;
	      }

	    // retrieve columns to be deleted. If none of the columns exist, then
	    // this row cannot be deleted.
	    // But if there is a scan expr, then we need to also retrieve the columns used
	    // in the pred. Add those.
	    LIST(HbaseStr) columns(tcb_->getHeap());
	    if (tcb_->hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::DELETE_)
	      {
		columns = tcb_->deletedColumns_;
		if (tcb_->scanExpr())
		  {
		    // retrieve all columns if none is specified.
		    if (tcb_->columns_.entries() == 0)
		      columns.clear();
		    else
		      // append retrieved columns to deleted columns.
		      columns.insert(tcb_->columns_);
		  }
	      }

	    retcode =  tcb_->ehi_->getRowOpen( tcb_->table_,  
					       tcb_->rowIds_[tcb_->currRowidIdx_],
					       columns, -1);
	    if ( tcb_->setupError(retcode, "ExpHbaseInterface::getRowOpen"))
	      step_ = HANDLE_ERROR;
	    else
	      step_ = NEXT_ROW;
	  }
	  break;

	case NEXT_ROW:
	  {
	    retcode = tcb_->ehi_->nextRow();
	    if (retcode == HBASE_ACCESS_EOD || retcode == HBASE_ACCESS_EOR)
	    {
	       step_ = GET_CLOSE;
	       break;
	    }
	    if (tcb_->setupError(retcode, "ExpHbaseInterface::nextRow"))
	       step_ = HANDLE_ERROR;
	    else
	       step_ = NEXT_CELL;
	  }
	  break;

	case NEXT_CELL:
	  {
	    if (tcb_->colVal_.val == NULL)
	        tcb_->colVal_.val = new (tcb_->getHeap())
	            char[tcb_->hbaseAccessTdb().convertRowLen()];
	    tcb_->colVal_.len = tcb_->hbaseAccessTdb().convertRowLen();
	    retcode = tcb_->ehi_->nextCell(tcb_->rowId_, tcb_->colFamName_, 
	            tcb_->colName_, tcb_->colVal_, tcb_->colTS_);
	    if (retcode == HBASE_ACCESS_EOD) 
	    {
	      if ((tcb_->hbaseAccessTdb().getAccessType() 
		    == ComTdbHbaseAccess::DELETE_) && (! tcb_->scanExpr()))
		step_ = DELETE_ROW;
	      else
		step_ = CREATE_FETCHED_ROW;
	      break;
	    }
	    if (tcb_->setupError(retcode, "ExpHbaseInterface::nextCell"))
	       step_ = HANDLE_ERROR;
	    else
	       step_ = APPEND_CELL_TO_ROW;
	   }
	   break;

	case APPEND_CELL_TO_ROW:
	  {
	    tcb_->copyCell();
	    step_ = NEXT_CELL;
	  }
	  break;

	case CREATE_FETCHED_ROW:
	    {
	    rc =  tcb_->createRowwiseRow();
	    if (rc < 0)
	      {
		if (rc != -1)
		   tcb_->setupError(rc, "createRowwiseRow");
		step_ = HANDLE_ERROR;
		break;
	      }

	    step_ = APPLY_PRED;
	  }
	  break;

	case APPLY_PRED:
	  {
	    rc =  tcb_->applyPred(tcb_->scanExpr());
	    if (rc == 1)
	      {
		if (tcb_->hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::DELETE_)
		  step_ = DELETE_ROW;
		else
		  step_ = CREATE_UPDATED_ROW; 
	      }
	    else if (rc == -1)
	      step_ = HANDLE_ERROR;
	    else
	      {
		tcb_->currRowidIdx_++;
		step_ = GET_NEXT_ROWID;
	      }
	  }
	  break;

	case CREATE_UPDATED_ROW:
	  {
	    if (! tcb_->updateExpr())
	      {
		tcb_->currRowidIdx_++;
		
		step_ = GET_NEXT_ROWID;
		break;
	      }

	    tcb_->workAtp_->getTupp(tcb_->hbaseAccessTdb().updateTuppIndex_)
	      .setDataPointer(tcb_->updateRow_);
	    
	    if (tcb_->updateExpr())
	      {
		ex_expr::exp_return_type evalRetCode =
		  tcb_->updateExpr()->eval(pentry_down->getAtp(), tcb_->workAtp_,
                                           NULL, -1, &tcb_->insertRowlen_);
		if (evalRetCode == ex_expr::EXPR_ERROR)
		  {
		    step_ = HANDLE_ERROR;
		    break;
		  }
	      }

	    ExpTupleDesc * rowTD =
	      tcb_->hbaseAccessTdb().workCriDesc_->getTupleDescriptor
	      (tcb_->hbaseAccessTdb().updateTuppIndex_);
	    
	    Attributes * attr = rowTD->getAttr(0);
 
	    rowUpdated_ = TRUE;
	    retcode = tcb_->createDirectRowwiseBuffer(
			   &tcb_->updateRow_[attr->getOffset()]);
	    if (retcode == -1)
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    step_ = UPDATE_ROW;
	  }
	  break;

	case DELETE_ROW:
	  {
            retcode =  tcb_->ehi_->deleteRow(tcb_->table_,
                                             tcb_->rowIds_[tcb_->currRowidIdx_],
                                             tcb_->deletedColumns_,
                                             tcb_->hbaseAccessTdb().useHbaseXn(),
 
                                             -1 //colTS_
                                             );



	    if ( tcb_->setupError(retcode, "ExpHbaseInterface::deleteRow"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }
	    
	    tcb_->currRowidIdx_++;
	    
	    if (tcb_->getHbaseAccessStats())
	      tcb_->getHbaseAccessStats()->incUsedRows();

	    tcb_->matches_++;

	    step_ = GET_NEXT_ROWID;
	  }
	  break;

	case UPDATE_ROW:
	  {
	    if (tcb_->numColsInDirectBuffer() > 0)
	      {
                retcode =  tcb_->ehi_->insertRow(tcb_->table_,
                                                 tcb_->rowIds_[tcb_->currRowidIdx_],
                                                 tcb_->row_,

                                                 tcb_->hbaseAccessTdb().useHbaseXn(),
    
                                                 -1 // colTS_
                                                 );

		if ( tcb_->setupError(retcode, "ExpHbaseInterface::insertRow"))
		  {
		    step_ = HANDLE_ERROR;
		    break;
		  }

		if (tcb_->getHbaseAccessStats())
		  tcb_->getHbaseAccessStats()->incUsedRows();

		tcb_->matches_++;
	      }
	    tcb_->currRowidIdx_++;
	    
	    step_ = GET_NEXT_ROWID;
	  }
	  break;

	case GET_CLOSE:
	  {
	    retcode =  tcb_->ehi_->getClose();
	    if ( tcb_->setupError(retcode, "ExpHbaseInterface::getClose"))
	      step_ = HANDLE_ERROR;
	    else
	      step_ = DONE;
	  }
	  break;

	case HANDLE_ERROR:
	  {
	    step_ = NOT_STARTED;
	    return -1;
	  }
	  break;

	case DONE:
	  {
	    step_ = NOT_STARTED;
	    return 0;
	  }
	  break;

	}// switch

    } // while

}

ExHbaseUMDtrafSubsetTaskTcb::ExHbaseUMDtrafSubsetTaskTcb
(ExHbaseAccessUMDTcb * tcb)
  :  ExHbaseTaskTcb(tcb)
  , step_(NOT_STARTED)
{
}

void ExHbaseUMDtrafSubsetTaskTcb::init() 
{
  step_ = NOT_STARTED;
}

ExWorkProcRetcode ExHbaseUMDtrafSubsetTaskTcb::work(short &rc)
{
  Lng32 retcode = 0;
  HbaseStr rowID;
  rc = 0;
  Lng32 remainingInBatch = batchSize_;

  while (1)
    {
     ex_queue_entry *pentry_down = tcb_->qparent_.down->getHeadEntry();
 
      switch (step_)
	{
	case NOT_STARTED:
	  {
	    step_ = SCAN_OPEN;
	  }
	  break;

	case SCAN_OPEN:
	  {
            // Pre-fetch is disabled because it interfers with
            // Delete operations
	    retcode = tcb_->ehi_->scanOpen(tcb_->table_, 
					   tcb_->beginRowId_, tcb_->endRowId_,
					   tcb_->columns_, -1,
					   tcb_->hbaseAccessTdb().readUncommittedScan(),
					   tcb_->hbaseAccessTdb().getHbasePerfAttributes()->cacheBlocks(),
					   tcb_->hbaseAccessTdb().getHbasePerfAttributes()->numCacheRows(),
					   FALSE, NULL, NULL, NULL);
	    if (tcb_->setupError(retcode, "ExpHbaseInterface::scanOpen"))
	      step_ = HANDLE_ERROR;
	    else
	      step_ = NEXT_ROW;
	  }
	  break;

	case NEXT_ROW:
	  {
            if (--remainingInBatch <= 0)
              {
                rc = WORK_CALL_AGAIN;
                return 1;
              }

	    retcode = tcb_->ehi_->nextRow();
	    if (retcode == HBASE_ACCESS_EOD || retcode == HBASE_ACCESS_EOR)
	      {
		step_ = SCAN_CLOSE;
		break;
	      }
	    if (tcb_->setupError(retcode, "ExpHbaseInterface::nextRow"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    if (tcb_->hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::DELETE_)
	      {
		if ((! tcb_->scanExpr()) &&
		    (NOT tcb_->hbaseAccessTdb().returnRow()))
		  {
		    step_ = DELETE_ROW;
		    break;
		  }
	      }

	      step_ = CREATE_FETCHED_ROW;
	  }
	  break;

	  case CREATE_FETCHED_ROW:
	  {
	    retcode = tcb_->createSQRowDirect();
	    if (retcode == HBASE_ACCESS_NO_ROW)
	    {
	       step_ = NEXT_ROW;
	       break;
	    }
	    if (retcode < 0)
	    {
                rc = (short)retcode;
	        tcb_->setupError(rc, "createSQRowDirect");
	        step_ = HANDLE_ERROR;
	        break;
	    }
	    if (retcode != HBASE_ACCESS_SUCCESS)
	    {
	        step_ = HANDLE_ERROR;
	        break;
	    }
	    step_ = APPLY_PRED;
	  }
	  break;

	  case APPLY_PRED:
	  {
	    rc = tcb_->applyPred(tcb_->scanExpr());
	    if (rc == 1)
	      {
		if (tcb_->hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::DELETE_)
		  step_ = DELETE_ROW;
		else
		  step_ = CREATE_UPDATED_ROW; 
	      }
	    else if (rc == -1)
	      step_ = HANDLE_ERROR;
	    else
	      step_ = NEXT_ROW;
	  }
	  break;

	case CREATE_UPDATED_ROW:
	  {
	    tcb_->workAtp_->getTupp(tcb_->hbaseAccessTdb().updateTuppIndex_)
	      .setDataPointer(tcb_->updateRow_);
	    
	    if (tcb_->updateExpr())
	      {
                tcb_->insertRowlen_ = tcb_->hbaseAccessTdb().updateRowLen_;
		ex_expr::exp_return_type evalRetCode =
		  tcb_->updateExpr()->eval(pentry_down->getAtp(), tcb_->workAtp_,
                                           NULL, -1, &tcb_->insertRowlen_);
		if (evalRetCode == ex_expr::EXPR_ERROR)
		  {
		    step_ = HANDLE_ERROR;
		    break;
		  }
	      }

	    step_ = EVAL_CONSTRAINT;
	  }
	  break;

	case EVAL_CONSTRAINT:
	  {
	    rc = tcb_->applyPred(tcb_->mergeUpdScanExpr(), 
				 tcb_->hbaseAccessTdb().updateTuppIndex_, tcb_->updateRow_);
	    if (rc == 1) // expr is true or no expr
	      step_ = CREATE_MUTATIONS;
	    else if (rc == 0) // expr is false
	      step_ = NEXT_ROW; 
	    else // error
	      step_ = HANDLE_ERROR;
	  }
	  break;

	case CREATE_MUTATIONS:
	  {
            // Merge can result in inserting rows
            // Use Number of columns in insert rather number
            // of columns in update if an insert is involved in this tcb
            if (tcb_->hbaseAccessTdb().getAccessType() 
                  == ComTdbHbaseAccess::MERGE_)
            {
              ExpTupleDesc * rowTD = NULL;
              if (tcb_->mergeInsertExpr())
                {
                  rowTD = tcb_->hbaseAccessTdb().workCriDesc_->getTupleDescriptor
                    (tcb_->hbaseAccessTdb().mergeInsertTuppIndex_);
                }
              else
                {
                  rowTD = tcb_->hbaseAccessTdb().workCriDesc_->getTupleDescriptor
                    (tcb_->hbaseAccessTdb().updateTuppIndex_);
                }
                
               if (rowTD->numAttrs() > 0)
                  tcb_->allocateDirectRowBufferForJNI(rowTD->numAttrs());
            } 

	    retcode = tcb_->createDirectRowBuffer(
				 tcb_->hbaseAccessTdb().updateTuppIndex_,
				 tcb_->updateRow_,
				 tcb_->hbaseAccessTdb().listOfUpdatedColNames(),
				 TRUE);
	    if (retcode == -1)
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    step_ = UPDATE_ROW;
	  }
	  break;

	case UPDATE_ROW:
	  {
            retcode = tcb_->ehi_->getRowID(rowID);
	    if (tcb_->setupError(retcode, "ExpHbaseInterface::insertRow"))
	    {
		step_ = HANDLE_ERROR;
		break;
	    }
	    retcode = tcb_->ehi_->insertRow(tcb_->table_,
					    rowID,
					    tcb_->row_,
                                            tcb_->hbaseAccessTdb().useHbaseXn(),
					    -1 // colTS_
					    );

	    if (tcb_->setupError(retcode, "ExpHbaseInterface::insertRow"))
	    {
		step_ = HANDLE_ERROR;
		break;
	    }

	    if (tcb_->hbaseAccessTdb().returnRow())
	      {
		step_ = EVAL_RETURN_ROW_EXPRS;
		break;
	      }

	    if (tcb_->getHbaseAccessStats())
	      tcb_->getHbaseAccessStats()->incUsedRows();

	    tcb_->matches_++;

	    step_ = NEXT_ROW;
	  }
	  break;

	case DELETE_ROW:
	  {
	    LIST(HbaseStr) columns(tcb_->getHeap());
            retcode = tcb_->ehi_->getRowID(rowID);
	    if (tcb_->setupError(retcode, "ExpHbaseInterface::insertRow"))
	    {
		step_ = HANDLE_ERROR;
		break;
            }
	    retcode =  tcb_->ehi_->deleteRow(tcb_->table_,
					     rowID,
					     columns,
                                             tcb_->hbaseAccessTdb().useHbaseXn(),


					     -1 //colTS_
					     );

	    if ( tcb_->setupError(retcode, "ExpHbaseInterface::deleteRow"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }
	    
	    if (tcb_->getHbaseAccessStats())
	      tcb_->getHbaseAccessStats()->incUsedRows();

	    tcb_->currRowidIdx_++;
	    
	    if (tcb_->hbaseAccessTdb().returnRow())
	      {
		step_ = RETURN_ROW;
		break;
	      }

	    tcb_->matches_++;

	    step_ = NEXT_ROW;
	  }
	  break;

	case RETURN_ROW:
	  {
	    if (tcb_->qparent_.up->isFull())
	      {
		rc = WORK_OK;
		return 1;
	      }
	    
	    rc = 0;
	    // moveRowToUpQueue also increments matches_
	    if (tcb_->moveRowToUpQueue(tcb_->convertRow_, tcb_->hbaseAccessTdb().convertRowLen(), 
				       &rc, FALSE))
	      return 1;

	    if ((pentry_down->downState.request == ex_queue::GET_N) &&
		(pentry_down->downState.requestValue == tcb_->matches_))
	      {
		step_ = SCAN_CLOSE;
		break;
	      }

	    step_ = NEXT_ROW;
	  }
	  break;

	case EVAL_RETURN_ROW_EXPRS:
	  {
	    ex_queue_entry * up_entry = tcb_->qparent_.up->getTailEntry();

	    rc = 0;

	    // allocate tupps where returned rows will be created
	    if (tcb_->allocateUpEntryTupps(
					   tcb_->hbaseAccessTdb().returnedFetchedTuppIndex_,
					   tcb_->hbaseAccessTdb().returnFetchedRowLen_,
					   tcb_->hbaseAccessTdb().returnedUpdatedTuppIndex_,
					   tcb_->hbaseAccessTdb().returnUpdatedRowLen_,
					   FALSE,
					   &rc))
	      return 1;
	    
	    ex_expr::exp_return_type exprRetCode;

	    char * fetchedDataPtr = NULL;
	    char * updatedDataPtr = NULL;
	    if (tcb_->returnFetchExpr())
	      {
		exprRetCode =
		  tcb_->returnFetchExpr()->eval(up_entry->getAtp(), tcb_->workAtp_);
		if (exprRetCode == ex_expr::EXPR_ERROR)
		  {
		    step_ = HANDLE_ERROR;
		    break;
		  }
		fetchedDataPtr = up_entry->getAtp()->getTupp(tcb_->hbaseAccessTdb().returnedFetchedTuppIndex_).getDataPointer();
		
	      }

	    if (tcb_->returnUpdateExpr())
	      {
		exprRetCode =
		  tcb_->returnUpdateExpr()->eval(up_entry->getAtp(), tcb_->workAtp_);
		if (exprRetCode == ex_expr::EXPR_ERROR)
		  {
		    step_ = HANDLE_ERROR;
		    break;
		  }
		updatedDataPtr = up_entry->getAtp()->getTupp(tcb_->hbaseAccessTdb().returnedUpdatedTuppIndex_).getDataPointer();
	      }

	    step_ = RETURN_UPDATED_ROWS;
	  }
	  break;

	case RETURN_UPDATED_ROWS:
	  {
	    rc = 0;
	    // moveRowToUpQueue also increments matches_
	    if (tcb_->moveRowToUpQueue(&rc))
	      return 1;

	    if ((pentry_down->downState.request == ex_queue::GET_N) &&
		(pentry_down->downState.requestValue == tcb_->matches_))
	      {
		step_ = SCAN_CLOSE;
		break;
	      }

	    step_ = NEXT_ROW;
	  }
	  break;

	case SCAN_CLOSE:
	  {
	    retcode = tcb_->ehi_->scanClose();
	    if (tcb_->setupError(retcode, "ExpHbaseInterface::scanClose"))
	      step_ = HANDLE_ERROR;
	    else
	      step_ = DONE;
	  }
	  break;

	case HANDLE_ERROR:
	  {
	    step_ = NOT_STARTED;
	    return -1;
	  }
	  break;

	case DONE:
	  {
	    step_ = NOT_STARTED;
	    return 0;
	  }
	  break;

	}// switch

    } // while

}

ExHbaseUMDnativeSubsetTaskTcb::ExHbaseUMDnativeSubsetTaskTcb
(ExHbaseAccessUMDTcb * tcb)
  :  ExHbaseUMDtrafSubsetTaskTcb(tcb)
  , step_(NOT_STARTED)
{
}

void ExHbaseUMDnativeSubsetTaskTcb::init() 
{
  step_ = NOT_STARTED;
}

ExWorkProcRetcode ExHbaseUMDnativeSubsetTaskTcb::work(short &rc)
{
  Lng32 retcode = 0;
  rc = 0;
  Lng32 remainingInBatch = batchSize_;

  while (1)
    {
     ex_queue_entry *pentry_down = tcb_->qparent_.down->getHeadEntry();
 
      switch (step_)
	{
	case NOT_STARTED:
	  {
	     tcb_->setupListOfColNames(tcb_->hbaseAccessTdb().listOfDeletedColNames(),
				       tcb_->deletedColumns_);

	     tcb_->setupListOfColNames(tcb_->hbaseAccessTdb().listOfFetchedColNames(),
				       tcb_->columns_);
	     
	    step_ = SCAN_OPEN;
	  }
	  break;

	case SCAN_OPEN:
	  {
	    // retrieve columns to be deleted. If the column doesn't exist, then
	    // this row cannot be deleted.
	    // But if there is a scan expr, then we need to also retrieve the columns used
	    // in the pred. Add those.
	    LIST(HbaseStr) columns(tcb_->getHeap());
	    if (tcb_->hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::DELETE_)
	      {
		columns = tcb_->deletedColumns_;
		if (tcb_->scanExpr())
		  {
		    // retrieve all columns if none is specified.
		    if (tcb_->columns_.entries() == 0)
		      columns.clear();
		    else
		      // append retrieved columns to deleted columns.
		      columns.insert(tcb_->columns_);
		  }
	      }

	    retcode = tcb_->ehi_->scanOpen(tcb_->table_, 
					   tcb_->beginRowId_, tcb_->endRowId_,
					   columns, -1,
					   tcb_->hbaseAccessTdb().readUncommittedScan(),
					   tcb_->hbaseAccessTdb().getHbasePerfAttributes()->cacheBlocks(),
					   tcb_->hbaseAccessTdb().getHbasePerfAttributes()->numCacheRows(),
					   FALSE, NULL, NULL, NULL);
	    if (tcb_->setupError(retcode, "ExpHbaseInterface::scanOpen"))
	      step_ = HANDLE_ERROR;
	    else
	      {
		step_ = NEXT_ROW;
		tcb_->isEOD_ = FALSE;
	      }
	  }
	  break;

	case NEXT_ROW:
	  {
            if (--remainingInBatch <= 0)
              {
                rc = WORK_CALL_AGAIN;
                return 1;
              }
	    retcode = tcb_->ehi_->nextRow();
	    if (retcode == HBASE_ACCESS_EOD || retcode == HBASE_ACCESS_EOR)
	    {
	       tcb_->isEOD_ = TRUE;
	       step_ = SCAN_CLOSE;
	       break;
	    } 
	    if (tcb_->setupError(retcode, "ExpHbaseInterface::nextRow"))
	       step_ = HANDLE_ERROR;
	    else
	       step_ = NEXT_CELL;
	  }
	  break;

	case NEXT_CELL:
	  {
            if (tcb_->colVal_.val == NULL)
                tcb_->colVal_.val = new (tcb_->getHeap())
                     char[tcb_->hbaseAccessTdb().convertRowLen()];
            tcb_->colVal_.len = tcb_->hbaseAccessTdb().convertRowLen();
	    retcode = tcb_->ehi_->nextCell( tcb_->rowId_, tcb_->colFamName_, 
					    tcb_->colName_, tcb_->colVal_,
					    tcb_->colTS_);
	    if (retcode == HBASE_ACCESS_EOD)
	    {
	      if (tcb_->hbaseAccessTdb().getAccessType() 
	           == ComTdbHbaseAccess::DELETE_)
	       {
	          if (! tcb_->scanExpr())
	          {
	             step_ = DELETE_ROW;
		     break;
	          }
		}
	        step_ = CREATE_FETCHED_ROWWISE_ROW;
                break;
            }
	    if (tcb_->setupError(retcode, "ExpHbaseInterface::nextCell"))
	    {
		step_ = HANDLE_ERROR;
		break;
	    }
	    step_ = APPEND_CELL_TO_ROW;
	  }
	  break;

	case APPEND_CELL_TO_ROW:
	  {
            tcb_->copyCell();
	    step_ = NEXT_CELL;
	  }
	  break;

	case CREATE_FETCHED_ROWWISE_ROW:
	  {
	    rc = tcb_->createRowwiseRow();
	    if (rc < 0)
	      {
		if (rc != -1)
		  tcb_->setupError(rc, "createRowwiseRow");
		step_ = HANDLE_ERROR;
		break;
	      }

	    step_ = APPLY_PRED;
	  }
	  break;

	  case APPLY_PRED:
	  {
	    rc = tcb_->applyPred(tcb_->scanExpr());
	    if (rc == 1)
	      {
		if (tcb_->hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::DELETE_)
		  step_ = DELETE_ROW;
		else
		  step_ = CREATE_UPDATED_ROWWISE_ROW; 
	      }
	    else if (rc == -1)
	       step_ = HANDLE_ERROR;
	    else
               step_ = NEXT_ROW; 
	  }
	  break;

	case CREATE_UPDATED_ROWWISE_ROW:
	  {
	    tcb_->workAtp_->getTupp(tcb_->hbaseAccessTdb().updateTuppIndex_)
	      .setDataPointer(tcb_->updateRow_);
	    
	    if (tcb_->updateExpr())
	      {
		ex_expr::exp_return_type evalRetCode =
		  tcb_->updateExpr()->eval(pentry_down->getAtp(), tcb_->workAtp_);
		if (evalRetCode == ex_expr::EXPR_ERROR)
		  {
		    step_ = HANDLE_ERROR;
		    break;
		  }
	      }

	    step_ = CREATE_MUTATIONS;
	  }
	  break;

	case CREATE_MUTATIONS:
	  {
	    ExpTupleDesc * rowTD =
	      tcb_->hbaseAccessTdb().workCriDesc_->getTupleDescriptor
	      (tcb_->hbaseAccessTdb().updateTuppIndex_);
	    
	    Attributes * attr = rowTD->getAttr(0);
 
	    retcode = tcb_->createDirectRowwiseBuffer(
						   &tcb_->updateRow_[attr->getOffset()]);

	    if (retcode == -1)
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    step_ = UPDATE_ROW;
	  }
	  break;

	case UPDATE_ROW:
	  {
	    if (tcb_->numColsInDirectBuffer() > 0)
	      {
		retcode = tcb_->ehi_->insertRow(tcb_->table_,
						tcb_->rowId_,
						tcb_->row_,
                                                tcb_->hbaseAccessTdb().useHbaseXn(),						
					
						-1 // colTS_
						);

		if (tcb_->setupError(retcode, "ExpHbaseInterface::insertRow"))
		  {
		    step_ = HANDLE_ERROR;
		    break;
		  }
		
		if (tcb_->getHbaseAccessStats())
		  tcb_->getHbaseAccessStats()->incUsedRows();

		tcb_->matches_++;
	      }

	    step_ = NEXT_ROW; 
	  }
	  break;

	case DELETE_ROW:
	  {
	    retcode =  tcb_->ehi_->deleteRow(tcb_->table_,
					     tcb_->rowId_,
					     tcb_->deletedColumns_,
                                             tcb_->hbaseAccessTdb().useHbaseXn(),

					     -1 //colTS_
					     );

	    if ( tcb_->setupError(retcode, "ExpHbaseInterface::deleteRow"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }
	    
	    tcb_->currRowidIdx_++;
	    
	    if (tcb_->getHbaseAccessStats())
	      tcb_->getHbaseAccessStats()->incUsedRows();

	    tcb_->matches_++;

            step_ = NEXT_ROW;
	  }
	  break;

	case SCAN_CLOSE:
	  {
	    retcode = tcb_->ehi_->scanClose();
	    if (tcb_->setupError(retcode, "ExpHbaseInterface::scanClose"))
	      step_ = HANDLE_ERROR;
	    else
	      step_ = DONE;
	  }
	  break;

	case HANDLE_ERROR:
	  {
	    step_ = NOT_STARTED;
	    return -1;
	  }
	  break;

	case DONE:
	  {
	    step_ = NOT_STARTED;
	    return 0;
	  }
	  break;

	}// switch

    } // while
}

ExHbaseAccessUMDTcb::ExHbaseAccessUMDTcb(
          const ExHbaseAccessTdb &hbaseAccessTdb, 
          ex_globals * glob ) :
  ExHbaseAccessTcb(hbaseAccessTdb, glob),
  step_(NOT_STARTED)
{
  umdSQSubsetTaskTcb_ = NULL;
  umdSQUniqueTaskTcb_ = NULL;

  for (Lng32 i = 0; i < UMD_MAX_TASKS; i++)
    {
      tasks_[i] = FALSE;
    }

  ExHbaseAccessTdb &hbaseTdb = (ExHbaseAccessTdb&)hbaseAccessTdb;

  if (hbaseTdb.listOfScanRows())
    {
      tasks_[UMD_SUBSET_TASK] = TRUE;
  
      if (hbaseTdb.sqHbaseTable())
	umdSQSubsetTaskTcb_ = 
	  new(getGlobals()->getDefaultHeap()) ExHbaseUMDtrafSubsetTaskTcb(this);
      else
	umdSQSubsetTaskTcb_ = 
	  new(getGlobals()->getDefaultHeap()) ExHbaseUMDnativeSubsetTaskTcb(this);
     }

  if ((hbaseTdb.keySubsetGen()) &&
      (NOT hbaseTdb.uniqueKeyInfo()))
    {
      tasks_[UMD_SUBSET_KEY_TASK] = TRUE;

      if (hbaseTdb.sqHbaseTable())
	umdSQSubsetTaskTcb_ = 
	  new(getGlobals()->getDefaultHeap()) ExHbaseUMDtrafSubsetTaskTcb(this);
      else
	umdSQSubsetTaskTcb_ = 
	  new(getGlobals()->getDefaultHeap()) ExHbaseUMDnativeSubsetTaskTcb(this);
    }

  if (hbaseTdb.listOfGetRows())
    {
      tasks_[UMD_UNIQUE_TASK] = TRUE;
      
     if (hbaseTdb.sqHbaseTable())
       umdSQUniqueTaskTcb_ = 
	 new(getGlobals()->getDefaultHeap()) ExHbaseUMDtrafUniqueTaskTcb(this);
     else
       umdSQUniqueTaskTcb_ = 
	 new(getGlobals()->getDefaultHeap()) ExHbaseUMDnativeUniqueTaskTcb(this);
    }

  if ((hbaseTdb.keySubsetGen()) &&
      (hbaseTdb.uniqueKeyInfo()))
    {
      tasks_[UMD_UNIQUE_KEY_TASK] = TRUE;

     if (hbaseTdb.sqHbaseTable())
      umdSQUniqueTaskTcb_ = 
	new(getGlobals()->getDefaultHeap()) ExHbaseUMDtrafUniqueTaskTcb(this);
     else
      umdSQUniqueTaskTcb_ = 
	new(getGlobals()->getDefaultHeap()) ExHbaseUMDnativeUniqueTaskTcb(this);
    }
}

ExWorkProcRetcode ExHbaseAccessUMDTcb::work()
{
  Lng32 retcode = 0;
  short rc = 0;

  ExMasterStmtGlobals *g = getGlobals()->
    castToExExeStmtGlobals()->castToExMasterStmtGlobals();

  while (!qparent_.down->isEmpty())
    {
      ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();
      if ((pentry_down->downState.request == ex_queue::GET_NOMORE) &&
	  (step_ != DONE))
	{
	  step_ = UMD_CLOSE_NO_ERROR; //DONE;
	}

      switch (step_)
	{
	case NOT_STARTED:
	  {
	    matches_ = 0;

	    step_ = UMD_INIT;
	  }
	  break;

	case UMD_INIT:
	  {
	    retcode = ehi_->init(getHbaseAccessStats());
	    if (setupError(retcode, "ExpHbaseInterface::init"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    if (hbaseAccessTdb().listOfScanRows())
	      hbaseAccessTdb().listOfScanRows()->position();

	    if (hbaseAccessTdb().listOfGetRows())
	      {
		if (!  rowIdExpr())
		  {
		    setupError(-HBASE_OPEN_ERROR, "", "RowId Expr is empty");
		    step_ = HANDLE_ERROR;
		    break;
		  }

		hbaseAccessTdb().listOfGetRows()->position();
	      }

	    table_.val = hbaseAccessTdb().getTableName();
	    table_.len = strlen(hbaseAccessTdb().getTableName());

	    if (umdSQSubsetTaskTcb_)
	      umdSQSubsetTaskTcb_->init();

	    if (umdSQUniqueTaskTcb_)
	      umdSQUniqueTaskTcb_->init();

	    step_ = SETUP_SUBSET;
	  }
	  break;

	case SETUP_SUBSET:
	  {
	    if (NOT tasks_[UMD_SUBSET_TASK])
	      {
		step_ = SETUP_UNIQUE;
		break;
	      }

	    hsr_ = 
	      (ComTdbHbaseAccess::HbaseScanRows*)hbaseAccessTdb().listOfScanRows()
	      ->getCurr();

	    retcode = setupSubsetRowIdsAndCols(hsr_);
	    if (retcode == -1)
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    step_ = PROCESS_SUBSET;
	  }
	  break;

	case PROCESS_SUBSET:
	  {
	    rc = 0;
	    retcode = umdSQSubsetTaskTcb_->work(rc);
	    if (retcode == 1)
	      return rc;
	    else if (retcode < 0)
	      step_ = HANDLE_ERROR;
	    else
	      step_ = NEXT_SUBSET;
	  }
	  break;

	case NEXT_SUBSET:
	  {
	    hbaseAccessTdb().listOfScanRows()->advance();

	    if (! hbaseAccessTdb().listOfScanRows()->atEnd())
	      {
		step_ = SETUP_SUBSET;
		break;
	      }

	    step_ = SETUP_UNIQUE;
	  }
	  break;

	case SETUP_UNIQUE:
	  {
	    if (NOT tasks_[UMD_UNIQUE_TASK])
	      {
		step_ = SETUP_SUBSET_KEY; 
		break;
	      }

	    hgr_ = 
	      (ComTdbHbaseAccess::HbaseGetRows*)hbaseAccessTdb().listOfGetRows()
	      ->getCurr();

	    retcode = setupUniqueRowIdsAndCols(hgr_);
	    if (retcode == -1)
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    step_ = PROCESS_UNIQUE;
	  }
	  break;

	case PROCESS_UNIQUE:
	  {
	    rc = 0;
	    retcode = umdSQUniqueTaskTcb_->work(rc);
	    if (retcode == 1)
	      return rc;
	    else if (retcode < 0)
	      step_ = HANDLE_ERROR;
	    else
	      step_ = NEXT_UNIQUE;
	  }
	  break;

	case NEXT_UNIQUE:
	  {
	    hbaseAccessTdb().listOfGetRows()->advance();

	    if (! hbaseAccessTdb().listOfGetRows()->atEnd())
	      {
		step_ = SETUP_UNIQUE;
		break;
	      }

	    step_ = SETUP_SUBSET_KEY;
	  }
	  break;

	case SETUP_SUBSET_KEY:
	  {
	    if (NOT tasks_[UMD_SUBSET_KEY_TASK])
	      {
		step_ = SETUP_UNIQUE_KEY;
		break;
	      }

	    retcode = setupSubsetKeysAndCols();
	    if (retcode == -1)
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    step_ = PROCESS_SUBSET_KEY;
	  }
	  break;

	case PROCESS_SUBSET_KEY:
	  {
	    rc = 0;
	    retcode = umdSQSubsetTaskTcb_->work(rc);
	    if (retcode == 1)
	      return rc;
	    else if (retcode < 0)
	      step_ = HANDLE_ERROR;
	    else 
	      step_ = SETUP_UNIQUE_KEY;
	  }
	  break;

	case SETUP_UNIQUE_KEY:
	  {
	    if (NOT tasks_[UMD_UNIQUE_KEY_TASK])
	      {
		step_ = UMD_CLOSE;
		break;
	      }

	    retcode = setupUniqueKeyAndCols(TRUE);
	    if (retcode == -1)
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    step_ = PROCESS_UNIQUE_KEY;
	  }
	  break;

	case PROCESS_UNIQUE_KEY:
	  {
	    rc = 0;
	    retcode = umdSQUniqueTaskTcb_->work(rc);
	    if (retcode == 1)
	      return rc;
	    else if (retcode < 0)
	      step_ = HANDLE_ERROR;
	    else 
	      step_ = UMD_CLOSE;
	  }
	  break;

	case UMD_CLOSE:
	case UMD_CLOSE_NO_ERROR:
	  {
	    retcode = ehi_->close();
	    if (step_ == UMD_CLOSE)
	      {
		if (setupError(retcode, "ExpHbaseInterface::close"))
		  {
		    step_ = HANDLE_ERROR;
		    break;
		  }
	      }

	    step_ = DONE;
	  }
	  break;

	case HANDLE_ERROR:
	  {
	    if (handleError(rc))
	      return rc;

	    retcode = ehi_->close();

	    step_ = DONE;
	  }
	  break;

	case DONE:
	  {
	    if (NOT hbaseAccessTdb().computeRowsAffected())
	      matches_ = 0;

	    if (handleDone(rc, matches_))
	      return rc;

	    if (umdSQSubsetTaskTcb_)
	      umdSQSubsetTaskTcb_->init();

	    if (umdSQUniqueTaskTcb_)
	      umdSQUniqueTaskTcb_->init();

	    step_ = NOT_STARTED;
	  }
	  break;

	} // switch
    } // while

  return WORK_OK;
}

ExHbaseAccessSQRowsetTcb::ExHbaseAccessSQRowsetTcb(
          const ExHbaseAccessTdb &hbaseAccessTdb, 
          ex_globals * glob ) :
  ExHbaseAccessTcb( hbaseAccessTdb, glob)
  , step_(NOT_STARTED)
{
  if (getHbaseAccessStats())
    getHbaseAccessStats()->init();

  prevTailIndex_ = 0;

  nextRequest_ = qparent_.down->getHeadIndex();

  numRetries_ = 0;

  getSQTaskTcb_ = 
    new(getGlobals()->getDefaultHeap()) ExHbaseGetSQTaskTcb(this);
 
}

ExWorkProcRetcode ExHbaseAccessSQRowsetTcb::work()
{
  Lng32 retcode = 0;
  short rc = 0;

  ExMasterStmtGlobals *g = getGlobals()->
    castToExExeStmtGlobals()->castToExMasterStmtGlobals();

  while (!qparent_.down->isEmpty())
    {
      nextRequest_ = qparent_.down->getHeadIndex();

      ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();
      if (pentry_down->downState.request == ex_queue::GET_NOMORE)
	step_ = ALL_DONE;

      switch (step_)
	{
	case NOT_STARTED:
	  {
	    matches_ = 0;
	    currRowNum_ = 0;
	    numRetries_ = 0;

	    prevTailIndex_ = 0;
	    
	    nextRequest_ = qparent_.down->getHeadIndex();

	    step_ = RS_INIT;
	  }
	  break;
	  
	case RS_INIT:
	  {
	    retcode = ehi_->init(getHbaseAccessStats());
	    if (setupError(retcode, "ExpHbaseInterface::init"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    table_.val = hbaseAccessTdb().getTableName();
	    table_.len = strlen(hbaseAccessTdb().getTableName());

            if (hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::UPDATE_)
            {
               ExpTupleDesc * rowTD =
    		hbaseAccessTdb().workCriDesc_->getTupleDescriptor
                (hbaseAccessTdb().updateTuppIndex_);
                allocateDirectRowBufferForJNI(rowTD->numAttrs(),
                                   ROWSET_MAX_NO_ROWS);
            }
	    if (hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::UPDATE_
                 || hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::DELETE_)
                allocateDirectRowIDBufferForJNI(ROWSET_MAX_NO_ROWS);

            //	    rowIds_.clear();

	    setupListOfColNames(hbaseAccessTdb().listOfFetchedColNames(),
				columns_);

	    if (hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::SELECT_)
	      step_ = SETUP_SELECT;
	    else
	      step_ = SETUP_UMD;
	  }
	  break;

	case SETUP_SELECT:
	  {
	    retcode = setupUniqueKeyAndCols(FALSE);
	    if (retcode == -1)
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	      step_ = NEXT_ROW;
	  }
	  break;

	case SETUP_UMD:
	  {
	    rowIds_.clear();
	    retcode = setupUniqueKeyAndCols(FALSE);
	    if (retcode == -1)
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    copyRowIDToDirectBuffer(rowIds_[0]);

	    if ((hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::DELETE_) ||
		(hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::SELECT_))
	      step_ = NEXT_ROW;
	    else if (hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::UPDATE_)
	      step_ = CREATE_UPDATED_ROW;
	    else
	      step_ = HANDLE_ERROR;

	  }
	  break;

	case NEXT_ROW:
	  {
	    currRowNum_++;
	    matches_++;

	    if (currRowNum_ < ROWSET_MAX_NO_ROWS)
	      {
		step_ = DONE;
		break;
	      }

	    if (hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::DELETE_)
	      step_ = PROCESS_DELETE;
	    else if (hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::SELECT_)
	      step_ = PROCESS_SELECT;
	    else if (hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::UPDATE_)
	      step_ = PROCESS_UPDATE;
	    else
	      step_ = HANDLE_ERROR;
	  }
	  break;

	case PROCESS_DELETE:
	case PROCESS_DELETE_AND_CLOSE:
	  {
            short numRowsInBuffer = patchDirectRowIDBuffers();
	    retcode = ehi_->deleteRows(table_,
                                       hbaseAccessTdb().getRowIDLen(),
                                       rowIDs_,
                                       hbaseAccessTdb().useHbaseXn(),
				       -1 // ColTS_
				       );



	    if (setupError(retcode, "ExpHbaseInterface::deleteRows"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

          if (getHbaseAccessStats())
	    {
	      getHbaseAccessStats()->lobStats()->numReadReqs++;
	      getHbaseAccessStats()->incUsedRows(numRowsInBuffer);

	    }

	    if (step_ == PROCESS_DELETE_AND_CLOSE)
	      step_ = RS_CLOSE;
	    else
	      step_ = DONE;
	  }
	  break;

	case PROCESS_SELECT:
	case PROCESS_SELECT_AND_CLOSE:
	  {
	   rc = 0;
	   retcode = getSQTaskTcb_->work(rc);
	   if (retcode == 1)
	     return rc;
	   else if (retcode < 0)
	     {
	       step_ = HANDLE_ERROR;
	       break;
	     }

	   if (getHbaseAccessStats())
	     {
	       getHbaseAccessStats()->lobStats()->numReadReqs++;
	     }
	   
	   if (step_ == PROCESS_SELECT_AND_CLOSE)
	     step_ = RS_CLOSE;
	   else
	     step_ = DONE;
	  }
	  break;

	case CREATE_UPDATED_ROW:
	  {
	    workAtp_->getTupp(hbaseAccessTdb().updateTuppIndex_)
	      .setDataPointer(updateRow_);
	    
	    if (updateExpr())
	      {
		ex_expr::exp_return_type evalRetCode =
		  updateExpr()->eval(pentry_down->getAtp(), workAtp_);
		if (evalRetCode == ex_expr::EXPR_ERROR)
		  {
		    step_ = HANDLE_ERROR;
		    break;
		  }
	      }

	    retcode = createDirectRowBuffer(
				      hbaseAccessTdb().updateTuppIndex_,
				      updateRow_,
				      hbaseAccessTdb().listOfUpdatedColNames(),
				      TRUE);
	    if (retcode == -1)
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    step_ = NEXT_ROW;
	  }
	  break;

	case PROCESS_UPDATE:
	case PROCESS_UPDATE_AND_CLOSE:
	  {
            short numRowsInBuffer = patchDirectRowBuffers();

	    retcode = ehi_->insertRows(table_,
				       hbaseAccessTdb().getRowIDLen(),
                                       rowIDs_,
                                       rows_,
                                       hbaseAccessTdb().useHbaseXn(),
				       -1);

	    
	    if (setupError(retcode, "ExpHbaseInterface::insertRows"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

          if (getHbaseAccessStats())
	    {
	      getHbaseAccessStats()->lobStats()->numReadReqs++;
	      getHbaseAccessStats()->incUsedRows(numRowsInBuffer);

	    }

	    if (step_ == PROCESS_UPDATE_AND_CLOSE)
	      step_ = RS_CLOSE;
	    else
	      step_ = DONE;
	  }
	  break;

	case RS_CLOSE:
	  {
	    retcode = ehi_->close();
	    if (setupError(retcode, "ExpHbaseInterface::close"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    step_ = ALL_DONE;
	  }
	  break;

	case HANDLE_ERROR:
	  {
	    if (handleError(rc))
	      return rc;

	    retcode = ehi_->close();

	    step_ = ALL_DONE;
	  }
	  break;

	case DONE:
	case ALL_DONE:
	  {
	    if (NOT hbaseAccessTdb().computeRowsAffected())
	      matches_ = 0;

	    if ((step_ == DONE) &&
		(qparent_.down->getLength() == 1))
	      {
		// only one row in the down queue.

		// Before we send input buffer to hbase, give parent
		// another chance in case there is more input data.
		// If parent doesn't input any more data on second (or
		// later) chances, then process the request.
		if (numRetries_ == 3)
		  {
		    numRetries_ = 0;

		    // Delete/update the current batch and then done.
		    if (hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::DELETE_)
		      step_ = PROCESS_DELETE_AND_CLOSE;
		    else if (hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::UPDATE_)
		      step_ = PROCESS_UPDATE_AND_CLOSE;
		    else if (hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::SELECT_)
		      step_ = PROCESS_SELECT_AND_CLOSE;
		    else
		      {
			// should never reach here
		      }
		    break;
		  }

		numRetries_++;
		return WORK_CALL_AGAIN;
	      }

	    if (handleDone(rc, (step_ == ALL_DONE ? matches_ : 0)))
	      return rc;

	    if (step_ == DONE)
	      {
		if (hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::SELECT_)
		  step_ = SETUP_SELECT;
		else
		  step_ = SETUP_UMD;
	      }
	    else
	      {
		step_ = NOT_STARTED;
	      }
	  }
	  break;

	} // switch

    } // while

  return WORK_OK;
}


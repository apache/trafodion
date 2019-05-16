// **********************************************************************
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
// **********************************************************************

#include "Platform.h"

#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ExHbaseAccess.h"
#include "ex_exe_stmt_globals.h"
#include "ExpHbaseInterface.h"
#include "hs_util.h"
#include "NLSConversion.h"
#include "ExHdfsScan.h"
#include "Context.h"
#include "HdfsClient_JNI.h"
#include "ExStats.h"

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
	step_ = CLOSE_AND_DONE;

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
                                      hbaseAccessTdb().useRegionXn(),
				      *insColTS_,
                                      FALSE); // AsyncOperations is always FALSE for native HBase

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
	    step_ = CLOSE_AND_DONE;
	  }
	  break;

	case DONE:
        case CLOSE_AND_DONE:
	  {
            if (step_ == CLOSE_AND_DONE)
               ehi_->close();
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
	step_ = CLOSE_AND_DONE;

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
                                          hbaseAccessTdb().useRegionXn(),
					  -1,  //*insColTS_
                                          FALSE); // AsyncOperations is always FALSE for native HBase

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
	    step_ = CLOSE_AND_DONE;
	  }
	  break;

	case DONE:
        case CLOSE_AND_DONE:
	  {
            if (step_ == CLOSE_AND_DONE)
               ehi_->close();
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
	step_ = CLOSE_AND_DONE;

      switch (step_)
	{
	case NOT_STARTED:
	  {
	    matches_ = 0;
            asyncCompleteRetryCount_ = 0;
            asyncOperationTimeout_ = 1;
            asyncOperation_ = hbaseAccessTdb().asyncOperations() && getTransactionIDFromContext();
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
	    rc = evalInsDelPreCondExpr();
	    if (rc == -1)
	      step_ = HANDLE_ERROR;
	    else if (rc == 0)
	      step_ = INSERT_CLOSE;
	    else // expr is true or does not exist
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
	    rc = evalConstraintExpr(insConstraintExpr());
	    if (rc == 1)
	      step_ = CREATE_MUTATIONS;
	    else if (rc == 0) 
	      step_ = INSERT_CLOSE;
	    else 
	      step_ = HANDLE_ERROR;
	  }
	  break;

	case CREATE_MUTATIONS:
	  {
	    retcode = createDirectRowBuffer( hbaseAccessTdb().convertTuppIndex_,
                                             convertRow_,
                                             hbaseAccessTdb().listOfUpdatedColNames(),
                                             hbaseAccessTdb().listOfOmittedColNames(),
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
            NABoolean isVC = hbaseAccessTdb().keyInVCformat();
	    if (evalRowIdExpr(isVC) == -1)
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
                                              hbaseAccessTdb().useRegionXn(),
                                              insColTSval_,
                                              asyncOperation_,
					      hbaseAccessTdb().getColIndexOfPK1());

	    if (retcode == HBASE_DUP_ROW_ERROR) // row exists, return error
	      {
		ComDiagsArea * diagsArea = NULL;
		ExRaiseSqlError(getHeap(), &diagsArea, 
				(ExeErrorCode)(8102));
		pentry_down->setDiagsArea(diagsArea);
		step_ = HANDLE_ERROR;
		break;
	      }
	      
	    if (setupError(retcode, "ExpHbaseInterface::checkAndInsertRow"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }
	    
	    if (getHbaseAccessStats())
	      getHbaseAccessStats()->incUsedRows();

	    if (hbaseAccessTdb().returnRow()) {
		step_ = RETURN_ROW;
                break;
            }
	    matches_++;
	    if (asyncOperation_) {
                step_ = COMPLETE_ASYNC_INSERT;
                return WORK_CALL_AGAIN;
            }
            else {
	        step_ = INSERT_CLOSE;
	    }
	  }
	  break;
        case COMPLETE_ASYNC_INSERT:
          {
            if (resultArray_  == NULL)
                resultArray_ = new (getHeap()) NABoolean[1];
            Int32 timeout;
            if (asyncCompleteRetryCount_ < 10)
               timeout = -1;
            else {
               asyncOperationTimeout_ = asyncOperationTimeout_ * 2; 
               timeout = asyncOperationTimeout_;
            }
            retcode = ehi_->completeAsyncOperation(timeout, resultArray_, 1);
            if (retcode == HBASE_RETRY_AGAIN) {
               asyncCompleteRetryCount_++;
               return WORK_CALL_AGAIN;
            }
            asyncCompleteRetryCount_ = 0;
	    if (setupError(retcode, "ExpHbaseInterface::completeAsyncOperation")) {
		step_ = HANDLE_ERROR;
		break;
            }
            if (resultArray_[0] == FALSE) {
		ComDiagsArea * diagsArea = NULL;
		ExRaiseSqlError(getHeap(), &diagsArea, 
				(ExeErrorCode)(8102));
		pentry_down->setDiagsArea(diagsArea);
		step_ = HANDLE_ERROR;
		break;
            }
	    step_ = INSERT_CLOSE;
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
                                      hbaseAccessTdb().useRegionXn(),
				      insColTSval_,
                                      asyncOperation_);

	    if (setupError(retcode, "ExpHbaseInterface::insertRow")) {
		step_ = HANDLE_ERROR;
		break;
	    }
	    if (getHbaseAccessStats())
	      getHbaseAccessStats()->incUsedRows();
	    if (hbaseAccessTdb().returnRow()) {
		step_ = RETURN_ROW;
		break;
	    }
	    matches_++;
	    if (asyncOperation_) {
                step_ = COMPLETE_ASYNC_INSERT;
                return WORK_CALL_AGAIN;
            }
            else {
	        step_ = INSERT_CLOSE;
            }
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
	    if (asyncOperation_) {
               step_ = COMPLETE_ASYNC_INSERT;
               return WORK_CALL_AGAIN;
            }
            else
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
	    step_ = CLOSE_AND_DONE;
	  }
	  break;

	case DONE:
        case CLOSE_AND_DONE:
	  {
            if (step_ == CLOSE_AND_DONE)
               ehi_->close();
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
  numRowsInVsbbBuffer_ = 0;
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
      if ((step_ == HANDLE_ERROR) ||
          (step_ == CLOSE_AND_DONE))
        {
          // move down to error/close case.
        }
      else if (pentry_down->downState.request == ex_queue::GET_NOMORE)
	step_ = CLOSE_AND_DONE;
      else if (pentry_down->downState.request == ex_queue::GET_EOD)
        {
          if (currRowNum_ > rowsInserted_)
            {
              step_ = PROCESS_INSERT_AND_CLOSE;
            }
          else
            {
              if (lastHandledStep_ == ALL_DONE)
                matches_=0;
              step_ = ALL_DONE;
            }
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
            asyncCompleteRetryCount_ = 0;
            asyncOperationTimeout_ = 1;
            asyncOperation_ = hbaseAccessTdb().asyncOperations() && getTransactionIDFromContext();
            numRowsInVsbbBuffer_ = 0;
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
            allocateDirectRowBufferForJNI(rowTD->numAttrs(), hbaseAccessTdb().getHbaseRowsetVsbbSize());
            allocateDirectRowIDBufferForJNI(hbaseAccessTdb().getHbaseRowsetVsbbSize());
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
	    rc = evalConstraintExpr(insConstraintExpr());
	    if (rc == 1) 
	      step_ = CREATE_MUTATIONS;
	    else if (rc == 0) 
	      step_ = INSERT_CLOSE;
	    else 
	      step_ = HANDLE_ERROR;
	  }
	  break;

	case CREATE_MUTATIONS:
	  {
	    retcode = createDirectRowBuffer(
                 hbaseAccessTdb().convertTuppIndex_,
                 convertRow_,
                 hbaseAccessTdb().listOfUpdatedColNames(),
                 hbaseAccessTdb().listOfOmittedColNames(),
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
            NABoolean isVC = hbaseAccessTdb().keyInVCformat();
	    if (evalRowIdExpr(isVC) == -1)
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    copyRowIDToDirectBuffer(rowId_);

	    currRowNum_++;
            
              
            if (!hbaseAccessTdb().returnRow())
              matches_++;
            // if we are returning a row moveRowToUpQueue will increment matches_
            else
              {
                step_ = RETURN_ROW;
                break;
              } 
                        
	    if (currRowNum_ < hbaseAccessTdb().getHbaseRowsetVsbbSize())
	      {
		step_ = DONE;
		break;
	      }

	    step_ = PROCESS_INSERT_AND_CLOSE;
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
                  return rc;

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
		  return rc;
              }
            else
	      {
		rc = 0;
		// moveRowToUpQueue also increments matches_
		if (moveRowToUpQueue(convertRow_, hbaseAccessTdb().convertRowLen(), 
				     &rc, FALSE))
		  return rc;
	      }

            if (currRowNum_ < hbaseAccessTdb().getHbaseRowsetVsbbSize())
              step_ = DONE;
            else
              step_ = PROCESS_INSERT_AND_CLOSE;

            break;



          }
          break;
	case PROCESS_INSERT_AND_CLOSE:
	  {
            numRowsInVsbbBuffer_ = patchDirectRowBuffers();
	    
	    retcode = ehi_->insertRows(table_,
                                       hbaseAccessTdb().getRowIDLen(),
				       rowIDs_,
                                       rows_,
				       hbaseAccessTdb().useHbaseXn(),
                                       insColTSval_,
				       asyncOperation_);

	    if (setupError(retcode, "ExpHbaseInterface::insertRows")) {
              step_ = HANDLE_ERROR;
              break;
	    }
	    if (getHbaseAccessStats()) {
              getHbaseAccessStats()->incUsedRows((Int64)numRowsInVsbbBuffer_);
	    }
            rowsInserted_ += numRowsInVsbbBuffer_; 
            if (asyncOperation_) {
              lastHandledStep_ = step_;
              step_ = COMPLETE_ASYNC_INSERT;
            }
            else
              step_ = INSERT_CLOSE;
	  }
	  break;
        case COMPLETE_ASYNC_INSERT:
          {
            if (resultArray_  == NULL)
              resultArray_ = new (getHeap()) NABoolean[hbaseAccessTdb().getHbaseRowsetVsbbSize()];
            Int32 timeout;
            if (asyncCompleteRetryCount_ < 10)
              timeout = -1;
            else {
              asyncOperationTimeout_ = asyncOperationTimeout_ * 2;
              timeout = asyncOperationTimeout_;
            }
            retcode = ehi_->completeAsyncOperation(timeout, resultArray_, numRowsInVsbbBuffer_);
            if (retcode == HBASE_RETRY_AGAIN) {
              asyncCompleteRetryCount_++;
              return WORK_CALL_AGAIN;
            }
            asyncCompleteRetryCount_ = 0;
            if (setupError(retcode, "ExpHbaseInterface::completeAsyncOperation")) {
              step_ = HANDLE_ERROR;
              break;
            }
            for (int i = 0 ; i < numRowsInVsbbBuffer_; i++) {
              if (resultArray_[i] == FALSE) {
                ComDiagsArea * diagsArea = NULL;
                ExRaiseSqlError(getHeap(), &diagsArea,
                                (ExeErrorCode)(8102));
                pentry_down->setDiagsArea(diagsArea);
                step_ = HANDLE_ERROR;
                break;
              }
            }
            if (step_ == HANDLE_ERROR)
              break;
            if (lastHandledStep_ == PROCESS_INSERT_AND_CLOSE)
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
	    step_ = CLOSE_AND_DONE;
	  }
	  break;

	case DONE:
        case CLOSE_AND_DONE:
	case ALL_DONE:
	  {
            if (step_ == CLOSE_AND_DONE)
              ehi_->close();
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

                break;
              }

	    if (handleDone(rc, (step_ == ALL_DONE  ? matches_ : 0)))
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
    lastErrorCnd_(NULL),
    sampleFileHdfsClient_(NULL)
{
   hFileParamsInitialized_ = false;
   //sortedListOfColNames_ = NULL;
   posVec_.clear();

   Lng32 fileNum = getGlobals()->castToExExeStmtGlobals()->getMyInstanceNumber();


    ExHbaseAccessTcb::buildLoggingFileName((NAHeap *)getHeap(), ((ExHbaseAccessTdb &)hbaseAccessTdb).getLoggingLocation(),
                      // (char *)((ExHbaseAccessTdb &)hbaseAccessTdb).getErrCountRowId(),
                      ((ExHbaseAccessTdb &)hbaseAccessTdb).getTableName(),
                      "traf_upsert_err",
                      fileNum,
                      loggingFileName_);
   loggingRow_ =  new(glob->getDefaultHeap()) char[hbaseAccessTdb.updateRowLen_];
}

ExHbaseAccessBulkLoadPrepSQTcb::~ExHbaseAccessBulkLoadPrepSQTcb()
{
  if (sampleFileHdfsClient_ != NULL)
     NADELETE(sampleFileHdfsClient_, HdfsClient, getHeap()); 
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
    case REC_BIN8_SIGNED:
    case REC_BIN8_UNSIGNED:
      return "tinyint";

    case REC_BIN16_SIGNED:
    case REC_BIN16_UNSIGNED:
    case REC_BPINT_UNSIGNED:
      return "smallint";

    case REC_BIN32_SIGNED:
    case REC_BIN32_UNSIGNED:
      return "int";

    case REC_BIN64_SIGNED:
      return "bigint";

    case REC_IEEE_FLOAT32:
      return "float";

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
  Int64 exceptionCount;
  Lng32 errorRowCount;

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
        step_ != HANDLE_ERROR && lastHandledStep_ != HANDLE_ERROR) {
      eodSeen = true;
      if (currRowNum_ > rowsInserted_)
        step_ = PROCESS_INSERT;
      else
      {
        if (lastHandledStep_ == ALL_DONE)
          matches_ = 0;
        step_ = ALL_DONE;
      }
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
           
            ContextCli *currContext = getGlobals()->castToExExeStmtGlobals()->getCliGlobals()->currContext();
            Text samplePath = std::string(((ExHbaseAccessTdb&)hbaseAccessTdb()).getSampleLocation()) +
                                          ((ExHbaseAccessTdb&)hbaseAccessTdb()).getTableName() ;
            char filePart[10];
            sprintf(filePart, "/%d", fileNum);
            HDFS_Client_RetCode hdfsClientRetcode;
            samplePath.append(filePart);
            if (sampleFileHdfsClient_ == NULL)
                sampleFileHdfsClient_ = HdfsClient::newInstance((NAHeap *)getHeap(), NULL, hdfsClientRetcode);
            if (hdfsClientRetcode == HDFS_CLIENT_OK) {
                hdfsClientRetcode = sampleFileHdfsClient_->hdfsOpen(samplePath.data(), FALSE);
                if (hdfsClientRetcode != HDFS_CLIENT_OK) {
                    NADELETE(sampleFileHdfsClient_, HdfsClient, getHeap());
                    sampleFileHdfsClient_ = NULL;
                }
            } 
            if (hdfsClientRetcode != HDFS_CLIENT_OK) {
              ComDiagsArea * diagsArea = NULL;
              ExRaiseSqlError(getHeap(), &diagsArea,
                              (ExeErrorCode)(8110));
              pentry_down->setDiagsArea(diagsArea);
              step_ = HANDLE_ERROR;
              break;
            }
          }
          posVec_.clear();
          hbaseAccessTdb().listOfUpdatedColNames()->position();
          while (NOT hbaseAccessTdb().listOfUpdatedColNames()->atEnd())
          {
            UInt32 pos = *(UInt32*) hbaseAccessTdb().listOfUpdatedColNames()->getCurr();
            posVec_.push_back(pos);
            hbaseAccessTdb().listOfUpdatedColNames()->advance();
            numCols++;
          }
        }
        if (setupError(retcode, "ExpHbaseInterface::createHFile"))
        {
          step_ = HANDLE_ERROR;
          break;
        }
        allocateDirectRowBufferForJNI(
                 numCols,
                 hbaseAccessTdb().getTrafLoadFlushSize());
        allocateDirectRowIDBufferForJNI(hbaseAccessTdb().getTrafLoadFlushSize());
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
        lastErrorCnd_ = NULL;
        if (convertExpr())
          {
            insertRowlen_ = hbaseAccessTdb().convertRowLen_;
            ex_expr::exp_return_type evalRetCode =
              convertExpr()->eval(pentry_down->getAtp(), workAtp_,
                                  NULL, -1, &insertRowlen_);
            if (evalRetCode == ex_expr::EXPR_ERROR) {
               if (hbaseAccessTdb().getContinueOnError()) {
                  if (pentry_down->getDiagsArea()) {
                     Lng32 errorCount = pentry_down->getDiagsArea()->getNumber(DgSqlCode::ERROR_);
                    lastErrorCnd_ = pentry_down->getDiagsArea()->getErrorEntry(errorCount);
                  }
                step_= HANDLE_EXCEPTION;
                break;
              }
              else
              { 
                step_ = HANDLE_ERROR;
                break;
              }
          }
        }

        genAndAssignSyskey(hbaseAccessTdb().convertTuppIndex_, convertRow_);

        step_ = EVAL_ROWID_EXPR;
      }
    
        break;

      case EVAL_ROWID_EXPR:
      {
        NABoolean isVC = hbaseAccessTdb().keyInVCformat();
        if (evalRowIdExpr(isVC) == -1)
        {
          step_ = HANDLE_ERROR;
          break;
        }
        lastErrorCnd_ = NULL;
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
            if (((ExHbaseAccessTdb&) hbaseAccessTdb()).getNoDuplicates()  ||
               ((NOT  ((ExHbaseAccessTdb&) hbaseAccessTdb()).getNoDuplicates()) && 
                      hbaseAccessTdb().getContinueOnError())) { 
              //8110 Duplicate rows detected.
              ComDiagsArea * diagsArea = NULL;
              ExRaiseSqlError(getHeap(), &diagsArea,
                              (ExeErrorCode)(8110));
              pentry_down->setDiagsArea(diagsArea);
              if (hbaseAccessTdb().getContinueOnError()) {
                  if (pentry_down->getDiagsArea()) {
                     Lng32 errorCount = pentry_down->getDiagsArea()->getNumber(DgSqlCode::ERROR_);
                    lastErrorCnd_ = pentry_down->getDiagsArea()->getErrorEntry(errorCount);
                  }
                  step_= HANDLE_EXCEPTION;
                  break;
              }
              else {
                 step_ = HANDLE_ERROR;
                 break;
              }
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
                                      hbaseAccessTdb().listOfOmittedColNames(),
                                      FALSE, //TRUE,
                                      &posVec_,
                                      samplingRate);
        if (retcode == -1)
        {
          //need to re-verify error handling
          step_ = HANDLE_ERROR;
          break;
        }

        copyRowIDToDirectBuffer( rowId_);
        currRowNum_++;
        if (!hbaseAccessTdb().returnRow()) {
          matches_++; // if we are returning a row moveRowToUpQueue 
        //will increment matches_
        }
        else {
            step_ = RETURN_ROW;
            break ;
        }

        if (currRowNum_ < hbaseAccessTdb().getTrafLoadFlushSize())
        {
          step_ = DONE;
          break;
        }
        step_ = PROCESS_INSERT; // currRowNum_ == rowset size && we are not returning a row
      }
        break;

      case PROCESS_INSERT:
      {
        numRowsInVsbbBuffer_ = patchDirectRowBuffers();
        retcode = ehi_->addToHFile(hbaseAccessTdb().getRowIDLen(),
                                   rowIDs_,
                                   rows_);

        if (setupError(retcode, "ExpHbaseInterface::addToHFile"))
        {
           step_ = HANDLE_ERROR;
           break;
        }
        rowsInserted_ += numRowsInVsbbBuffer_;

        if (getHbaseAccessStats())
        {
          getHbaseAccessStats()->incUsedRows(numRowsInVsbbBuffer_);
        }

        step_ = ALL_DONE;
      }
        break;

      case HANDLE_ERROR:
      {
        if (handleError(rc))
          return rc;

        lastHandledStep_ =HANDLE_ERROR;
        eodSeen = true;
        matches_ = 0;
        step_ = ALL_DONE;
      }
        break;

      case HANDLE_EXCEPTION:
      {
      exceptionCount = 0;
      ExHbaseAccessTcb::incrErrorCount( ehi_,exceptionCount, hbaseAccessTdb().getErrCountTab(), 
                      hbaseAccessTdb().getErrCountRowId());
      if (hbaseAccessTdb().getMaxErrorRows() > 0)
      {
        if (exceptionCount >  hbaseAccessTdb().getMaxErrorRows())
        {
          if (pentry_down->getDiagsArea())
            pentry_down->getDiagsArea()->clear();
          if (workAtp_->getDiagsArea())
            workAtp_->getDiagsArea()->clear();
          errorRowCount = (Lng32) exceptionCount;
          ComDiagsArea * diagsArea = NULL;
          ExRaiseSqlWarning(getHeap(), &diagsArea,
              (ExeErrorCode)(EXE_ERROR_ROWS_FOUND), NULL, &errorRowCount);
          //8113 max number of error rows exceeded.
          ExRaiseSqlError(getHeap(), &diagsArea,
              (ExeErrorCode)(EXE_MAX_ERROR_ROWS_EXCEEDED));
          pentry_down->setDiagsArea(diagsArea);
          step_ = HANDLE_ERROR;
          break;
        }
      }

      if (hbaseAccessTdb().getLogErrorRows())
      {
        workAtp_->getTupp(hbaseAccessTdb().updateTuppIndex_).setDataPointer(updateRow_);

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
        int loggingRowLen = 0;
        Lng32 errorMsgLen = 0;
        createLoggingRow( hbaseAccessTdb().updateTuppIndex_,  updateRow_,
            loggingRow_ , loggingRowLen);
        ExHbaseAccessTcb::handleException((NAHeap *)getHeap(), loggingRow_, loggingRowLen,
               lastErrorCnd_);
      }
      if (pentry_down->getDiagsArea())
        pentry_down->getDiagsArea()->clear();
      if (workAtp_->getDiagsArea())
        workAtp_->getDiagsArea()->clear();

        step_ = DONE;
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
            return rc;
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
	    return rc;
	}
	else
	{
	  rc = 0;
	  // moveRowToUpQueue also increments matches_
	  if (moveRowToUpQueue(convertRow_, hbaseAccessTdb().convertRowLen(), 
			       &rc, FALSE))
	    return rc;
	}
        if (currRowNum_ < hbaseAccessTdb().getHbaseRowsetVsbbSize())
          step_ = DONE;
        else
          step_ = PROCESS_INSERT;

        break;
      }
      case DONE:
      case ALL_DONE:
      {
        if (step_ == ALL_DONE && eodSeen) {
           exceptionCount = 0;
           ExHbaseAccessTcb::getErrorCount( ehi_,exceptionCount, hbaseAccessTdb().getErrCountTab(), 
                      hbaseAccessTdb().getErrCountRowId());
           errorRowCount = (Lng32) exceptionCount;
           if (errorRowCount != 0 || loggingErrorDiags_ != NULL) {
	      ex_queue_entry * down_entry = qparent_.down->getHeadEntry();
              ComDiagsArea * diagsArea = down_entry->getDiagsArea();
              if (!diagsArea) {
                 diagsArea = ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
                down_entry->setDiagsArea(diagsArea);
              }
              if (loggingErrorDiags_ != NULL) {
                 diagsArea->mergeAfter(*loggingErrorDiags_);
                 loggingErrorDiags_->clear();
              }
              if (errorRowCount > 0) 
                 ExRaiseSqlWarning((NAMemory *)getHeap(), &diagsArea,
                   (ExeErrorCode)(EXE_ERROR_ROWS_FOUND), (ComCondition **)NULL, &errorRowCount);
           }
        }
 
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
            if (logFileHdfsClient_ != NULL)
               logFileHdfsClient_->hdfsClose();
            if (sampleFileHdfsClient_ != NULL)
               sampleFileHdfsClient_->hdfsClose();
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

short ExHbaseAccessBulkLoadPrepSQTcb::createLoggingRow( UInt16 tuppIndex,  char * tuppRow, char * targetRow, int &targetRowLen)
{

  ExpTupleDesc * rowTD =
      hbaseAccessTdb().workCriDesc_->getTupleDescriptor
      (tuppIndex);

  short colNameLen;
  char * colName;
  short nullVal = 0;
  short nullValLen = 0;
  short colValLen;
  char *colVal;

  Attributes * attr;
  short *numColsPtr;

  char * tmpTargetRow = targetRow;
  for (Lng32 i = 0; i <  rowTD->numAttrs(); i++)
  {
    Attributes * attr = rowTD->getAttr(i);

    if (attr)
    {
      colVal = &tuppRow[attr->getOffset()];
      nullVal = 0;
      if (attr->getNullFlag() &&
          (*(short*)&tuppRow[attr->getNullIndOffset()]))
      {
        targetRow[0] = '|';
        targetRow++;
      }
      else
      {
        colValLen =  attr->getLength(&tuppRow[attr->getVCLenIndOffset()]);
        memcpy(targetRow,colVal, colValLen);
        targetRow +=colValLen;
        if (i != rowTD->numAttrs() -1)
          targetRow[0] = '|';
        else
          targetRow[0] = '\n';
        targetRow++;
      }
    }
    else
    {
      ex_assert(false, "Unable to obtain column descriptor");
    }

  }   // for

  targetRowLen= targetRow - tmpTargetRow;

  return 0;
}

// UMD (unique UpdMergeDel on Trafodion tables)
ExHbaseUMDtrafUniqueTaskTcb::ExHbaseUMDtrafUniqueTaskTcb
(ExHbaseAccessUMDTcb * tcb)
  :  ExHbaseTaskTcb(tcb)
  , step_(NOT_STARTED)
{
  latestRowTimestamp_ = -1;
  columnToCheck_.val = (char *)(new (tcb->getHeap()) BYTE[MAX_COLNAME_LEN]);
  columnToCheck_.len = MAX_COLNAME_LEN;
  colValToCheck_.val = (char *)(new (tcb->getHeap()) BYTE[tcb->hbaseAccessTdb().getRowIDLen()]);
  colValToCheck_.len = tcb->hbaseAccessTdb().getRowIDLen();
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
            latestRowTimestamp_ = -1;

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
                     (! tcb_->lobDelExpr()) &&
		     (NOT tcb_->hbaseAccessTdb().returnRow()))
	      step_ = DELETE_ROW;
	    else
	      step_ = CREATE_FETCHED_ROW;
	  }
	  break;

	  case CREATE_FETCHED_ROW:
	    {
	    retcode =  tcb_->createSQRowDirect(&latestRowTimestamp_);
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

	    step_ = EVAL_UPD_CONSTRAINT;
	  }
	  break;

	case EVAL_UPD_CONSTRAINT:
	  {
	    rc = tcb_->evalConstraintExpr(tcb_->updConstraintExpr(), tcb_->hbaseAccessTdb().updateTuppIndex_,
              tcb_->updateRow_);
	    if (rc == 1)
	      step_ = CREATE_MUTATIONS;
	    else if (rc == 0) 
	      step_ = GET_CLOSE;
	    else 
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
                                            tcb_->hbaseAccessTdb().listOfOmittedColNames(),
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
              step_ = EVAL_INS_CONSTRAINT;
            }
            break;
         case EVAL_INS_CONSTRAINT:
          {
             rc = tcb_->evalConstraintExpr(tcb_->insConstraintExpr());
             if (rc == 0) {
               step_ = GET_CLOSE;
               break;
            }
            else if (rc != 1) {
               step_ = HANDLE_ERROR;
               break;
            }

	    retcode = tcb_->createDirectRowBuffer( tcb_->hbaseAccessTdb().mergeInsertTuppIndex_,
					    tcb_->mergeInsertRow_,
					    tcb_->hbaseAccessTdb().listOfMergedColNames(),
				            tcb_->hbaseAccessTdb().listOfOmittedColNames());
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
					     tcb_->hbaseAccessTdb().useHbaseXn(),
					     tcb_->hbaseAccessTdb().useRegionXn(),
					     -1, //colTS_
                                             tcb_->asyncOperation_);
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
	    rc = tcb_->evalKeyColValExpr(columnToCheck_, colValToCheck_);
	    if (rc == -1)
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    retcode =  tcb_->ehi_->checkAndUpdateRow(tcb_->table_,
                                                     tcb_->rowIds_[tcb_->currRowidIdx_],
						     tcb_->row_,
						     columnToCheck_,
						     colValToCheck_,
                                                     tcb_->hbaseAccessTdb().useHbaseXn(),
                                                     tcb_->hbaseAccessTdb().useRegionXn(),
						     -1, //colTS_
                                                     tcb_->asyncOperation_);

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
                                                     tcb_->hbaseAccessTdb().useRegionXn(),
						     -1, // colTS
                                                     tcb_->asyncOperation_,
						     tcb_->hbaseAccessTdb().getColIndexOfPK1()); 

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
            rc = tcb_->evalInsDelPreCondExpr();
	    if (rc == -1) {
                step_ = HANDLE_ERROR;
                break;
	    }
            if (rc == 0) { // No need to delete
               tcb_->currRowidIdx_++;
               step_ = GET_NEXT_ROWID;
               break;
	    }
            retcode =  tcb_->ehi_->deleteRow(tcb_->table_,
                                             tcb_->rowIds_[tcb_->currRowidIdx_],
                                             NULL,
                                             tcb_->hbaseAccessTdb().useHbaseXn(),
                                             tcb_->hbaseAccessTdb().useRegionXn(),
                                             latestRowTimestamp_,
                                             tcb_->asyncOperation_);
	    if ( tcb_->setupError(retcode, "ExpHbaseInterface::deleteRow"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

            // delete entries from LOB desc table, if needed
            if (tcb_->lobDelExpr())
              {
                ex_expr::exp_return_type exprRetCode =
		  tcb_->lobDelExpr()->eval(pentry_down->getAtp(), tcb_->workAtp_);
		if (exprRetCode == ex_expr::EXPR_ERROR)
		  {
		    step_ = HANDLE_ERROR;
		    break;
		  }
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
            rc = tcb_->evalInsDelPreCondExpr();
	    if (rc == -1) {
                step_ = HANDLE_ERROR;
                break;
	    }
            if (rc == 0) { // donot delete
               tcb_->currRowidIdx_++;
               step_ = GET_NEXT_ROWID;
               break;
	    }
               
	    rc = tcb_->evalKeyColValExpr(columnToCheck_, colValToCheck_);
	    if (rc == -1)
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    retcode =  tcb_->ehi_->checkAndDeleteRow(tcb_->table_,
                                                     tcb_->rowIds_[tcb_->currRowidIdx_],
						     columnToCheck_, 
						     colValToCheck_,
                                                     tcb_->hbaseAccessTdb().useHbaseXn(),
                                                     tcb_->hbaseAccessTdb().useRegionXn(),
						     -1 //colTS_
						     );

	    if (retcode == HBASE_ROW_NOTFOUND_ERROR)
	      {
		tcb_->currRowidIdx_++;
		step_ = GET_NEXT_ROWID;
		break;
	      }

	    if ( tcb_->setupError(retcode, "ExpHbaseInterface::checkAndDeleteRow"))
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
	    char * mergeIUDIndicatorDataPtr = NULL;
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
	    if (tcb_->hbaseAccessTdb().mergeIUDIndicatorTuppIndex_ > 0)
	      mergeIUDIndicatorDataPtr = 
		tcb_->workAtp_->
		getTupp(tcb_->hbaseAccessTdb().mergeIUDIndicatorTuppIndex_).
		getDataPointer();
	    
	    if (rowUpdated_)
	      {
		if (tcb_->returnUpdateExpr())
		  {
		    if (mergeIUDIndicatorDataPtr)
		      *mergeIUDIndicatorDataPtr = 'U';
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
		if (mergeIUDIndicatorDataPtr)
		  *mergeIUDIndicatorDataPtr = 'I';
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
                                             &tcb_->deletedColumns_,
                                             tcb_->hbaseAccessTdb().useHbaseXn(),
                                             tcb_->hbaseAccessTdb().useRegionXn(),
                                             -1 ,
                                             tcb_->asyncOperation_);
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
                                                 tcb_->hbaseAccessTdb().useRegionXn(),
                                                 -1, // colTS_
                                                 tcb_->asyncOperation_);

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
            // Bypass scan when beginRowId_ is less than endRowId_
            if (tcb_->compareRowIds() < 0) {
               step_ = DONE;
               break;
            }
            // Pre-fetch is disabled because it interfers with
            // Delete operations
	    retcode = tcb_->ehi_->scanOpen(tcb_->table_, 
					   tcb_->beginRowId_, tcb_->endRowId_,
					   tcb_->columns_, -1,
					   tcb_->hbaseAccessTdb().readUncommittedScan(),
					   tcb_->hbaseAccessTdb().getHbasePerfAttributes()->cacheBlocks(),
					   tcb_->hbaseAccessTdb().getHbasePerfAttributes()->useSmallScanner(),
					   tcb_->hbaseAccessTdb().getHbasePerfAttributes()->numCacheRows(),
					   FALSE, NULL, NULL, NULL,
	                   tcb_->hbaseAccessTdb().getHbasePerfAttributes()->dopParallelScanner());
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

	    step_ = EVAL_UPD_CONSTRAINT;
	  }
	  break;

	case EVAL_UPD_CONSTRAINT:
	  {
	    rc = tcb_->evalConstraintExpr(tcb_->updConstraintExpr(), tcb_->hbaseAccessTdb().updateTuppIndex_,
                     tcb_->updateRow_);
	    if (rc == 1) 
	      step_ = CREATE_MUTATIONS;
	    else if (rc == 0)
	      step_ = SCAN_CLOSE; 
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
				 tcb_->hbaseAccessTdb().listOfOmittedColNames(),
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
                                            tcb_->hbaseAccessTdb().useRegionXn(),
					    -1, // colTS_
                                            tcb_->asyncOperation_);
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
            retcode = tcb_->ehi_->getRowID(rowID);
	    if (tcb_->setupError(retcode, "ExpHbaseInterface::insertRow"))
	    {
		step_ = HANDLE_ERROR;
		break;
            }
	    retcode =  tcb_->ehi_->deleteRow(tcb_->table_,
					     rowID,
					     NULL,
                                             tcb_->hbaseAccessTdb().useHbaseXn(),
                                             tcb_->hbaseAccessTdb().useRegionXn(),
					     -1,
					     tcb_->asyncOperation_);
	    if ( tcb_->setupError(retcode, "ExpHbaseInterface::deleteRow"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }
	    
            // delete entries from LOB desc table, if needed
            if (tcb_->lobDelExpr())
              {
                ex_expr::exp_return_type exprRetCode =
		  tcb_->lobDelExpr()->eval(pentry_down->getAtp(), tcb_->workAtp_);
		if (exprRetCode == ex_expr::EXPR_ERROR)
		  {
		    step_ = HANDLE_ERROR;
		    break;
		  }
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
            // Bypass scan when beginRowId_ is less than endRowId_
            if (tcb_->compareRowIds() < 0) {
               step_ = DONE;
               break;
            }
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
					   tcb_->hbaseAccessTdb().getHbasePerfAttributes()->useSmallScanner(),
					   tcb_->hbaseAccessTdb().getHbasePerfAttributes()->numCacheRows(),
					   FALSE, NULL, NULL, NULL,
	                   tcb_->hbaseAccessTdb().getHbasePerfAttributes()->dopParallelScanner());
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
                                                tcb_->hbaseAccessTdb().useRegionXn(),
						-1,// colTS_
                                                tcb_->asyncOperation_); 
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
					     &tcb_->deletedColumns_,
                                             tcb_->hbaseAccessTdb().useHbaseXn(),
                                             tcb_->hbaseAccessTdb().useRegionXn(),
					     -1,
					     tcb_->asyncOperation_);
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
  prevTailIndex_ = 0;

  nextRequest_ = qparent_.down->getHeadIndex();

  numRetries_ = 0;
  lastHandledStep_ = NOT_STARTED;
  numRowsInVsbbBuffer_ = 0;
}

Lng32 ExHbaseAccessSQRowsetTcb::setupUniqueKey()
{
  ex_queue_entry *pentry_down = qparent_.down->getQueueEntry(nextRequest_);

  if (pentry_down->downState.request == ex_queue::GET_NOMORE
     || pentry_down->downState.request == ex_queue::GET_EOD)
     return 1;

  ex_expr::exp_return_type exprRetCode = ex_expr::EXPR_OK;
  keyRangeEx::getNextKeyRangeReturnType keyRangeStatus;

  initNextKeyRange(pool_, pentry_down->getAtp());

  keyRangeStatus =
    keySubsetExeExpr_->getNextKeyRange(pentry_down->getAtp(), FALSE, TRUE);

  if (keyRangeStatus == keyRangeEx::EXPRESSION_ERROR)
     return -1;

  tupp &keyData = keySubsetExeExpr_->getBkData();
  char * beginKeyRow = keyData.getDataPointer();
  HbaseStr rowIdRowText;

  if ((NOT hbaseAccessTdb().sqHbaseTable()) ||
      (hbaseAccessTdb().keyInVCformat())) {
      // Key is in varchar format.
      short keyLen = *(short*)beginKeyRow;
      rowIdRowText.val = beginKeyRow + sizeof(short);
      rowIdRowText.len = keyLen;
   }
  else {
    rowIdRowText.val = beginKeyRow;
    rowIdRowText.len = hbaseAccessTdb().keyLen_;
  }

  if (keyRangeStatus == keyRangeEx::NO_MORE_RANGES)		
  {		
      // To ensure no row is found, add extra byte with "0" value 		
      rowIdRowText.val[rowIdRowText.len] = '\0';		
      rowIdRowText.len += 1;		
  }		
  copyRowIDToDirectBuffer(rowIdRowText);
  return 0;
}


Lng32 ExHbaseAccessSQRowsetTcb::setupRowIds()
{
  Lng32 retcode;
  UInt16 rowsetMaxRows = hbaseAccessTdb().getHbaseRowsetVsbbSize();
  
  queue_index tlindex = qparent_.down->getTailIndex();
  while (nextRequest_ != tlindex) {
     retcode = setupUniqueKey();
     if (retcode != 0)
        return retcode;
     nextRequest_++;
    // Don't buffer more than HBASE_ROWSET_VSBB_SIZE
    if (numRowsInDirectBuffer()  >= rowsetMaxRows)
        return 1;
  }
  return 0;
}

ExWorkProcRetcode ExHbaseAccessSQRowsetTcb::work()
{
  Lng32 retcode = 0;
  short rc = 0;

  ExMasterStmtGlobals *g = getGlobals()->
    castToExExeStmtGlobals()->castToExMasterStmtGlobals();

  while (!qparent_.down->isEmpty())
    {

      ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();
      if (pentry_down->downState.request == ex_queue::GET_NOMORE)
	step_ = CLOSE_AND_DONE;
      else if (pentry_down->downState.request == ex_queue::GET_EOD) {
         if (numRowsInDirectBuffer() > 0) {
            if (hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::UPDATE_)
               step_ = PROCESS_UPDATE_AND_CLOSE;
            else if (hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::DELETE_)
               step_ = PROCESS_DELETE_AND_CLOSE;
            else 
               ex_assert(0, "EOD and Select is not handled here"); 
          }
          else
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
            asyncCompleteRetryCount_ = 0;
            asyncOperationTimeout_ = 1;
            asyncOperation_ = hbaseAccessTdb().asyncOperations() && getTransactionIDFromContext();
            numRowsInVsbbBuffer_ = 0;
            lastHandledStep_ = NOT_STARTED;
	    
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
                                    hbaseAccessTdb().getHbaseRowsetVsbbSize());
            }
	    if (hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::UPDATE_
                 || hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::SELECT_
                 || hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::DELETE_)
                allocateDirectRowIDBufferForJNI(hbaseAccessTdb().getHbaseRowsetVsbbSize());

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
            retcode = setupRowIds();
            switch (retcode) {
               case 0:
                  if (qparent_.down->getLength() == 1) {
                     // only one row in the down queue.
                     // Before we send input buffer to hbase, give parent
                     // another chance in case there is more input data.
                     // If parent doesn't input any more data on second (or
                     // later) chances, then process the request.
                     if (numRetries_ == 3)    {
                         numRetries_ = 0;
                         step_ = PROCESS_SELECT;
                      } else {
                          numRetries_++;
                          return WORK_CALL_AGAIN;
                      }
                  }
                  else
                      step_ = PROCESS_SELECT;
                  break;
               case 1:
                  // Reached the max. number of rowIds
                  // Process the rowIds in the buffer 
                  step_ = PROCESS_SELECT;
                  break;
               default:
                  step_ = HANDLE_ERROR;
                  break;
            }
	  }
	  break;
	case SETUP_UMD:
	  {
            rc = evalInsDelPreCondExpr();
            if (rc == -1) {
                step_ = HANDLE_ERROR;
                break;
            }
            if (rc == 0) { // No need to delete
               step_ = NEXT_ROW;
               break;
            }
	    rowIds_.clear();
	    retcode = setupUniqueKeyAndCols(FALSE);
	    if (retcode == -1) {
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
	    if (hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::SELECT_) {
              // matches_ is set to 1 when the row is projected  by moveRowToUpQueue
              // to denote that there is a matching entry
               matches_ = 0;
               retcode = ehi_->nextRow();
              // EOR is end of result set for the current Rowset
              // EOD is no data for the current row
              // But EOD is never returned, instead HBASE_ACCESS_NO_ROW is returned
              // when no row is found in CREATE_ROW step
              if (retcode == HBASE_ACCESS_EOR) {
                 step_ = RS_CLOSE;
                 break;
              }
              if (retcode == HBASE_ACCESS_EOD) {
                 step_ = ROW_DONE;
                 break;
              }
              if (setupError(retcode, "ExpHbaseInterface::nextRow"))
                 step_ = HANDLE_ERROR;
              else
                 step_ = CREATE_ROW;
              break;
            }
	    matches_++;
	    if (numRowsInDirectBuffer() < hbaseAccessTdb().getHbaseRowsetVsbbSize()) {
		step_ = DONE;
		break;
	    }
	    if (hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::DELETE_)
	      step_ = PROCESS_DELETE_AND_CLOSE;
	    else if (hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::UPDATE_)
	      step_ = PROCESS_UPDATE_AND_CLOSE;
	    else
	      step_ = HANDLE_ERROR;
	  }
	  break;
       case CREATE_ROW:
          {
            retcode = createSQRowDirect();
            if (retcode == HBASE_ACCESS_NO_ROW) {
               step_ = ROW_DONE;
               break;
            }
            if (retcode < 0)
            {
                rc = (short)retcode;
                setupError(rc, "createSQRowDirect");
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
            rc = applyPred(scanExpr());
            if (rc == 1)
              step_ = RETURN_ROW;
            else if (rc == -1)
              step_ = HANDLE_ERROR;
            else
              step_ = ROW_DONE;
          }
          break;
        case RETURN_ROW:
          {
            rc = 0;
            if (moveRowToUpQueue(convertRow_, hbaseAccessTdb().convertRowLen(),
                                       &rc, FALSE))
              return rc;
            if (getHbaseAccessStats())
               getHbaseAccessStats()->incUsedRows();
            step_ = ROW_DONE;
          }
          break;
	case PROCESS_DELETE_AND_CLOSE:
	  {
            numRowsInVsbbBuffer_ = patchDirectRowIDBuffers();
	    retcode = ehi_->deleteRows(table_,
                                       hbaseAccessTdb().getRowIDLen(),
                                       rowIDs_,
                                       hbaseAccessTdb().useHbaseXn(),
				       -1,
				       asyncOperation_);
            currRowNum_ = 0;	    
	    if (setupError(retcode, "ExpHbaseInterface::deleteRows"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }
            if (asyncOperation_) {
               lastHandledStep_ = step_;
               step_ = COMPLETE_ASYNC_OPERATION;
               break;
            }
            if (getHbaseAccessStats()) {
	      getHbaseAccessStats()->incUsedRows(numRowsInVsbbBuffer_);
	    }
	    step_ = RS_CLOSE;
	  }
	  break;

	case PROCESS_SELECT:
	  {
           if (numRowsInDirectBuffer() > 0) {
		      numRowsInVsbbBuffer_ = patchDirectRowIDBuffers();
		      retcode = ehi_->getRowsOpen(
				    table_,
                            hbaseAccessTdb().getRowIDLen(),
                            rowIDs_, 
                            columns_);
              currRowNum_ = 0;
	      if (setupError(retcode, "ExpHbaseInterface::getRowsOpen"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }
              step_ = NEXT_ROW;

           }
           else
               step_ = SETUP_SELECT;
	  }
	  break;

	case CREATE_UPDATED_ROW:
	  {
	    workAtp_->getTupp(hbaseAccessTdb().updateTuppIndex_)
	      .setDataPointer(updateRow_);
	    
	    if (updateExpr()) {
		ex_expr::exp_return_type evalRetCode =
		  updateExpr()->eval(pentry_down->getAtp(), workAtp_);
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
            rc = evalConstraintExpr(updConstraintExpr(), hbaseAccessTdb().updateTuppIndex_,updateRow_);
            if (rc == 0) {
              step_ = RS_CLOSE; 
              break;
            }
            else if (rc != 1) {
              step_ = HANDLE_ERROR;
              break;
            }
	    retcode = createDirectRowBuffer(
				      hbaseAccessTdb().updateTuppIndex_,
				      updateRow_,
			  	      hbaseAccessTdb().listOfUpdatedColNames(),
				      hbaseAccessTdb().listOfOmittedColNames(),
				      TRUE);
	    if (retcode == -1) {
		step_ = HANDLE_ERROR;
		break;
	    }
	    step_ = NEXT_ROW;
	  }
	  break;
	case PROCESS_UPDATE_AND_CLOSE:
	  {
            numRowsInVsbbBuffer_ = patchDirectRowBuffers();

	    retcode = ehi_->insertRows(table_,
				       hbaseAccessTdb().getRowIDLen(),
                                       rowIDs_,
                                       rows_,
                                       hbaseAccessTdb().useHbaseXn(),
				       -1,
				       asyncOperation_);
            currRowNum_ = 0;	    
	    if (setupError(retcode, "ExpHbaseInterface::insertRows"))
	      {
		step_ = HANDLE_ERROR;
		break;
	      }
            if (asyncOperation_) {
               lastHandledStep_ = step_;
               step_ = COMPLETE_ASYNC_OPERATION;
               break;
            }
            if (getHbaseAccessStats()) {
	      getHbaseAccessStats()->incUsedRows(numRowsInVsbbBuffer_);
	    }
	    step_ = RS_CLOSE;
	  }
	  break;
      case COMPLETE_ASYNC_OPERATION:
         {
            if (resultArray_  == NULL)
                resultArray_ = new (getHeap()) NABoolean[hbaseAccessTdb().getHbaseRowsetVsbbSize()];
            Int32 timeout;
            if (asyncCompleteRetryCount_ < 10)
               timeout = -1;
            else {
               asyncOperationTimeout_ = asyncOperationTimeout_ * 2;
               timeout = asyncOperationTimeout_;
            }
            retcode = ehi_->completeAsyncOperation(timeout, resultArray_, numRowsInVsbbBuffer_);
            if (retcode == HBASE_RETRY_AGAIN) {
               asyncCompleteRetryCount_++;
               return WORK_CALL_AGAIN;
            }
            asyncCompleteRetryCount_ = 0;
            if (setupError(retcode, "ExpHbaseInterface::completeAsyncOperation")) {
                step_ = HANDLE_ERROR;
                break;
            }
            for (int i = 0 ; i < numRowsInVsbbBuffer_; i++) {
                if (resultArray_[i] == FALSE) {
                    ComDiagsArea * diagsArea = NULL;
                    ExRaiseSqlError(getHeap(), &diagsArea,
                                (ExeErrorCode)(8102));
                    pentry_down->setDiagsArea(diagsArea);
                    step_ = HANDLE_ERROR;
                    break;
               }
            }
            if (step_ == HANDLE_ERROR)
               break;
            if (getHbaseAccessStats()) {
	      getHbaseAccessStats()->incUsedRows(numRowsInVsbbBuffer_);
            }
            step_ = RS_CLOSE;
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
 	    if (hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::SELECT_)
               step_ = NOT_STARTED;
            else
	       step_ = ALL_DONE;
	  }
	  break;

	case HANDLE_ERROR:
	  {
	    if (handleError(rc))
	      return rc;
	    step_ = CLOSE_AND_DONE;
	  }
	  break;
        case ROW_DONE:
          {
	    if (handleDone(rc, 0))
	      return rc;
            step_ = NEXT_ROW;
          }
          break;
	case DONE:
        case CLOSE_AND_DONE:
	case ALL_DONE:
	  {
            if (step_ == CLOSE_AND_DONE)
               ehi_->close();
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
		if (numRetries_ == 3 || numRowsInDirectBuffer() > 1)
		  {
		    numRetries_ = 0;

		    // Delete/update the current batch and then done.
		    if (hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::DELETE_)
		      step_ = PROCESS_DELETE_AND_CLOSE;
		    else if (hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::UPDATE_)
		      step_ = PROCESS_UPDATE_AND_CLOSE;
		    else
		      {
                         ex_assert(false, "DONE state is invalid in Rowset SELECT");
		      }
		    break;
		  }
		numRetries_++;
		return WORK_CALL_AGAIN;
	      }

	    if (handleDone(rc, (step_ == ALL_DONE ? matches_ : 0)))
	      return rc;

	    if (step_ == DONE)
	       step_ = SETUP_UMD;
	    else  
	       step_ = NOT_STARTED;
	  }
	  break;
	} // switch

    } // while
    if (qparent_.down->isEmpty()
           && (hbaseAccessTdb().getAccessType() == ComTdbHbaseAccess::SELECT_)) {
        ehi_->close();
        step_ = NOT_STARTED;
    } 

  return WORK_OK;
}


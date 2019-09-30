/*********************************************************************
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
 * File:         ex_transaction.cpp
 * Description:  
 *
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include  <stdlib.h>

// TEMP TEMP TEMP
#include <stdio.h>
// End of TEMP

#include  "Platform.h"

#include  "cli_stdh.h"

#include  "ex_stdh.h"
#include  "ComTdb.h"
#include  "ex_tcb.h"
#include  "ex_transaction.h"
#include  "ex_exe_stmt_globals.h"
#include  "exp_expr.h"
#include  "ex_error.h"
#include  "ExSqlComp.h"

#include  "CmpContext.h"
#include "ExCextdecs.h"
#include "dtm/tm.h"

ExTransaction::ExTransaction(CliGlobals * cliGlob, CollHeap *heap)
     : cliGlob_(cliGlob),
       heap_(heap),
       errorCond_(NULL),
       transDiagsArea_(NULL),
       userTransMode_(NULL),
       local_or_encompassing_trans(0),
       autoCommitDisabled_(FALSE),
       savepointId_(0),
       volatileSchemaExists_(FALSE),
       transtag_(-1)
{
  transMode_ = new(heap) TransMode(TransMode::SERIALIZABLE_, 
			     TransMode::READ_WRITE_,
			     TransMode::OFF_);
  resetXnState();
}

ExTransaction::~ExTransaction()
{
  if (transMode_)
    NADELETE(transMode_, TransMode, heap_);
  transMode_ = 0;
  if (userTransMode_)
    NADELETE(userTransMode_, TransMode, heap_);
  userTransMode_ = 0;
  heap_ = NULL;
}

TransMode * ExTransaction::getUserTransMode()
{
  if (transMode_->userTransMode())
    // user has altered transaction mode other than AUTO COMMIT
    // current trans mode should have all the information
    return transMode_;

  // otherwise, we have to set isolation level to not specified
  // because it is constructed to SERIALIZABLE, see above
  if (userTransMode_ == 0)
    {
      userTransMode_ = new(heap_) TransMode();
      userTransMode_->updateTransMode(transMode_);
      userTransMode_->isolationLevel() = TransMode::IL_NOT_SPECIFIED_;
    }

  return userTransMode_; 
}

void ExTransaction::resetXnState()
{
  exeXnId_ = Int64(-1);
  transid_ = Int64(-1);		// Soln 10-050210-4624

  if (cliGlob_->isESPProcess())
    {
      // during esp startup, this method will be called from constructor of
      // ContextCli, but control connection has not been created yet.
      IpcEnvironment *ipcEnv = cliGlob_->getEnvironment();
      IpcControlConnection *cc =
        (ipcEnv ? ipcEnv->getControlConnection() : NULL);
      GuaReceiveControlConnection *grcc =
        (cc ? cc->castToGuaReceiveControlConnection() : NULL);
      if (grcc)
        grcc->clearBeginTransTag();
    }

  exeStartedXn_ = FALSE;
  exeStartedXnDuringRecursiveCLICall_ = FALSE;
  xnInProgress_ = FALSE;
  userEndedExeXn_ = FALSE;
  implicitXn_ = FALSE;
  enableAutoCommit();
  roVal_ = TransMode::READ_WRITE_ ;
  rbVal_ = TransMode::ROLLBACK_MODE_NOT_SPECIFIED_ ;
  aiVal_ = -1 ;
  setMayAlterDb(FALSE);
  setMayHoldLock(FALSE);

  // See ExControlArea::addControl().
  if (getTransModeNext()->getIsolationLevel() != TransMode::IL_NOT_SPECIFIED_)
    {
      getTransMode()->updateAccessModeFromIsolationLevel(
	   getTransModeNext()->getIsolationLevel());
      getTransModeNext()->updateAccessModeAndResetIsolationLevel();
    }
}

// Soln 10-050210-4624
short ExTransaction::getCurrentXnId(Int64 * tcbref, Int64 *transId, 
				    short *txHandle)
{
  /* This method retrieves from TMF and returns the TCBREF associated with the
   * currently active transaction. If the optional parameters, transId and
   * txHandle, are supplied, the transaction identifier and handle also are
   * returned.
   */

  // delcare local tx_handle for calling TMF functions.
  // cannot use indirect structure/array, will cause coredump.
  short tx_handle[10] = {0,0,0,0,0,0,0,0,0,0};
  short retcode = 0;


  retcode = GETTRANSID((short *) tcbref);
  if (txHandle != NULL )
    memcpy (txHandle, tx_handle, sizeof(TmfPhandle_Struct));
  if (transId)
    *transId = *tcbref;

  return retcode;
}

short ExTransaction::getCurrentTxHandle(short * txHandle)
{ 
  const short NULL_HANDLE[10] = {0,0,0,0,0,0,0,0,0,0};
  short retcode;

  memcpy (txHandle, NULL_HANDLE, sizeof(TmfPhandle_Struct));
  retcode = 0;

  return retcode;
}


// Soln 10-050210-4624
short ExTransaction::waitForRollbackCompletion(Int64 transid)
{
  if (transMode_->rollbackMode() == TransMode::ROLLBACK_MODE_NOWAITED_)
    return 0;

  short status;
  // Soln 10-050210-4624
  short rc = 0;

  if (transid)
    rc = STATUSTRANSACTION(&status, transid); // using param transid
  else
    rc = STATUSTRANSACTION(&status);

  if ((rc == 0) && (status != 5))
    {
      // check for return status in a loop until the transaction
      // is aborted.
      Lng32 delayTime = 100; // units of 1/100th of a seconds.
                            // 100 = 1 sec.
      NABoolean done = FALSE;
      while (! done)
	{
	  DELAY(delayTime);
	  delayTime += 100;
	  // Soln 10-050210-4624
	  if (transid)
	    rc = STATUSTRANSACTION(&status, transid); // using param transid.
	  else
    rc = STATUSTRANSACTION(&status);

	  if (! ((rc == 0) && (status != 5)))
	    done = TRUE;
	}
    }

  return 0;
}

static void setSpecialAIValues(Lng32 &aivalue)
{
  // on Linux we get these definitions from tm.h

  //  0 => default
  // -1 => user did not specify autoabort
  // -2 => user specifically set autoabort to 0 (SET TRANSACTION autoabort 0)
  // -3 => user specifically reset autoabort (SET TRANSACTION autoabort RESET)
  if (aivalue > 0)
    return ;

  if (aivalue < -3)
  {
    ex_assert(0, "Invalid autoabort interval specified");
    return;
  }

  aivalue = TM_NEVERABORT;  


  return ;
}

static void setNoRollBackAndReadOnlyFlags(NABoolean autoCommit, Int16 roval, 
                                   Int16 rbval, int_64 &type_flags)
{
  if (autoCommit) // set bits only if autocommit is on
    {
    // Set this Read_Only bit for implicit txns, such as in SELECT stmts
    // and the user does not specifically start a txn. 
    if (roval == TransMode::READ_ONLY_) // access mode enum
        type_flags |= TM_TT_READ_ONLY;
    }

  // Set the Read_Only bit if the user specifically wants read_only
  // regardless of autocommit setting, such as in SET TRANSACTION READ ONLY.
  if (roval == TransMode::READ_ONLY_SPECIFIED_BY_USER_)
      type_flags |= TM_TT_READ_ONLY;


  // Set the no_undo_needed bit regardless of autocommit setting.
  if (rbval == TransMode::NO_ROLLBACK_) // rollback mode enum
      type_flags |= TM_TT_NO_UNDO; // this is a bit flag, setting the second bit.

return;
}



short ExTransaction::beginTransaction()
{
  if (xnInProgress())
    {
      // Set the transaDiagsArea.
      // This is the first error. So reset the diags area.
      if (transDiagsArea_)
      {
	transDiagsArea_->decrRefCount();
	transDiagsArea_ = NULL;
      }
      ExRaiseSqlError(heap_, &transDiagsArea_,
		      EXE_BEGIN_TRANSACTION_ERROR, &errorCond_);
      return -1;
    }

  // If multiple contexts exist, a transaction cannot be started by Executor,
  // unless we guarantee that the transaction will be committed before
  // return from the current top-level CLI call. We do this to avoid having
  // other contexts accidentally inheriting this transaction, committing
  // it, and causing data integrity problems due to an incomplete INSERT,
  // UPDATE or DELETE statement as a result. 
  //
  // Sometimes we do need to start a transaction in a recursive CLI call 
  // for such things as metadata lookup or resource fork read during fixup. 
  // We flag such transactions so that we guarantee they are committed 
  // before the top-level CLI call returns.
  NABoolean mustCommitTransBeforeTopLevelCLIReturn = FALSE;
#if 0
  if (getCliGlobals()->getContextList()->numEntries() > 1)
    {
      if (getCliGlobals()->currContext()->getNumOfCliCalls() == 1)
        {
          // We are not in a recursive CLI call; force the user
          // to explicitly begin the transaction above the CLI.
	  if (transDiagsArea_)
	  {
	    transDiagsArea_->decrRefCount();
	    transDiagsArea_ = NULL;
	  }
          ExRaiseSqlError(heap_, &transDiagsArea_,
                          EXE_CANT_BEGIN_WITH_MULTIPLE_CONTEXTS, &errorCond_);
          return -1;
        }
      else
        {
          // We are in a recursive CLI call. Allow the transaction
          // to proceed, but flag it so we insure it is committed
          // before top-level CLI return.
          mustCommitTransBeforeTopLevelCLIReturn = TRUE;
        }
    }
#endif

  if (cliGlob_->currContext()->volatileSchemaCreated())
    {
      volatileSchemaExists_ = TRUE;
    }
  else
    {
      volatileSchemaExists_ = FALSE;
    }


  Lng32 aivalue = (Lng32) getAIVal(); 
  // set aivalue to TM_AUTOABORT_DEFAULT or TM_NEVERABORT if aivalue has the value
  // 0,-1,-2,or -3.
  setSpecialAIValues(aivalue);  


  int_64 transtag;
  int_64 type_flags = 0;
  setNoRollBackAndReadOnlyFlags(autoCommit(),getROVal(),getRBVal(),type_flags);
  short rc = BEGINTX(&transtag_, aivalue, type_flags);
  transtag = transtag_;


  if (rc == 0)
    {
      // Soln 10-050210-4624
      getCurrentXnId((Int64 *)&exeXnId_, (Int64 *)&transid_
                    );


      // If BEGINTRANSACTION is being called from within an ESP, save
      // the transtag returned by BEGINTRANSACTION. Later this transtag can be
      // used to invoke RESUMETRANSACTION to resume a transaction, which is
      // needed when a message is received on $RECEIVE and transid in PFS is
      // cleared. Without a call to RESUMETRANSACTION the esp can lose the
      // transaction it has initiated.
      if (cliGlob_->isESPProcess())
        {
          IpcEnvironment *ipcEnv = cliGlob_->getEnvironment();
          IpcControlConnection *cc =
            (ipcEnv ? ipcEnv->getControlConnection() : NULL);
          GuaReceiveControlConnection *grcc =
            (cc ? cc->castToGuaReceiveControlConnection() : NULL);
          if (grcc)
            grcc->setBeginTransTag(transtag);
        }
    }
  else
    {
      createDiagsArea (EXE_BEGIN_ERROR_FROM_TRANS_SUBSYS, rc, "TMF");
      return -1;
    }

  // reset values in ExTransaction class
  updateROVal(TransMode::READ_WRITE_);
  updateAIVal(-1);

  exeStartedXn_ = TRUE;
  exeStartedXnDuringRecursiveCLICall_ =
    mustCommitTransBeforeTopLevelCLIReturn;
  xnInProgress_ = TRUE;
  return 0;
}

short ExTransaction::suspendTransaction()
{
  short retcode = FEOK;
  if (transid_ != -1 && transtag_ != -1)
  {
    //     retcode = RESUMETRANSACTION(0);
    retcode = SUSPENDTRANSACTION((short*)&transid_);
     if (retcode == FENOTRANSID)
        retcode = FEOK;
  }
  return retcode; 
}

short ExTransaction::resumeTransaction()
{
  short retcode = 0;
  if (transid_ != -1 && transtag_ != -1)
  {
     retcode = RESUMETRANSACTION(transtag_);
  }
  return retcode;
}

short ExTransaction::rollbackStatement()
{
  if (! xnInProgress())
    {
      if (transDiagsArea_)
      {
	transDiagsArea_->decrRefCount();
	transDiagsArea_ = NULL;
      }
      ExRaiseSqlError(heap_, &transDiagsArea_,
		      EXE_ROLLBACK_TRANSACTION_ERROR, &errorCond_);
      return -1;
    }

  Int32 rc = ABORTTRANSACTION();
  if (rc != 0)
    {
      createDiagsArea (EXE_ROLLBACK_ERROR_FROM_TRANS_SUBSYS, rc,
		       "TMF");
      return -1;
    }


  waitForRollbackCompletion(transid_);
  
  if ((NOT volatileSchemaExists_) &&
     cliGlob_->currContext()->volatileSchemaCreated())
    {
       cliGlob_->currContext()->resetVolatileSchemaState();
    }

  resetXnState();

  return 0;
}


//This method does nowaited rollback transaction.
short ExTransaction::rollbackTransaction(NABoolean isWaited)
{
  if (! xnInProgress())
    {
      if (transDiagsArea_)
      {
	transDiagsArea_->decrRefCount();
	transDiagsArea_ = NULL;
      }
      ExRaiseSqlError(heap_, &transDiagsArea_,
                      (isWaited ? EXE_ROLLBACK_TRANSACTION_WAITED_ERROR :
                       EXE_ROLLBACK_TRANSACTION_ERROR), &errorCond_);
      return -1;
    }

  if (! exeStartedXn_)
    {
      if (transDiagsArea_)
      {
	transDiagsArea_->decrRefCount();
	transDiagsArea_ = NULL;
      }
      ExRaiseSqlError(heap_, &transDiagsArea_,
		      EXE_CANT_COMMIT_OR_ROLLBACK, &errorCond_);
      return -1;
    }

  Int32 rc = ABORTTRANSACTION();
  if (rc != 0 && rc != FENOTRANSID)
    {
      createDiagsArea (EXE_ROLLBACK_ERROR_FROM_TRANS_SUBSYS, rc,
		       "TMF");
      return -1;
    }

  if (isWaited)
    waitForRollbackCompletion(transid_);

  if ((NOT volatileSchemaExists_) &&
     cliGlob_->currContext()->volatileSchemaCreated())
    {
       cliGlob_->currContext()->resetVolatileSchemaState();
    }

  resetXnState();

  return 0;
}

short ExTransaction::rollbackTransactionWaited()
{
  return rollbackTransaction(TRUE);
}

short ExTransaction::doomTransaction()
{
  Int32 rc = 0;

  if (! xnInProgress())
    {
      if (transDiagsArea_)
      {
	transDiagsArea_->decrRefCount();
	transDiagsArea_ = NULL;
      }
      ExRaiseSqlError(heap_, &transDiagsArea_,
		      EXE_ROLLBACK_TRANSACTION_ERROR, &errorCond_);
      return -1;
    }

  // cannot doom a Xn if we started it. Need to abort in this case.
  if (exeStartedXn_)
    {
      if (transDiagsArea_)
      {
	transDiagsArea_->decrRefCount();
	transDiagsArea_ = NULL;
      }
      ExRaiseSqlError(heap_, &transDiagsArea_,
		      EXE_CANT_COMMIT_OR_ROLLBACK, &errorCond_);
      return -1;
    }

  // Save current txHandle before switching to context txHandle
  TmfPhandle_Struct currentTxHandle;

  // Soln 10-040727-8266
  NABoolean switchTrans = FALSE;

  // Make this transaction the active one. If there has been a switch and some
  // other transaction is currently active, it will be restored after this
  // transaction has been doomed.
  rc = setProcessTransToContextTrans(&currentTxHandle, switchTrans);


  if (rc != 0 && rc != FENOTRANSID)
    {
      createDiagsArea (EXE_ROLLBACK_ERROR_FROM_TRANS_SUBSYS, rc,
		       "TMF");
      return -1;
    }; 

  // doom the current transaction
  rc = TMF_DOOMTRANSACTION_();

  // switch back to the saved TxHandle.
  if (switchTrans)
    {
      Int32 rc2 = TMF_SETTXHANDLE_ ((short *)&currentTxHandle);

      // calls assert since unable to restore the previous handle.
      ex_assert(!rc2, "TMF_SETTXHANDLE ERROR");  // call assert if error
    }

  if (rc != 0)  // handle DOOMTRANSACTION error
    {
      createDiagsArea (EXE_ROLLBACK_ERROR_FROM_TRANS_SUBSYS, rc,
		       "TMF");
      return -1;
    }



  if (getCliGlobals()->getUdrErrorChecksEnabled())
  {
    getCliGlobals()->setUdrXactAborted(getTransid(), TRUE);
  }

  resetXnState();

  return 0;
}

void ExTransaction::cleanupTransaction()
{
  // commit the transaction but don't do anything with errors.
  // This is called if a transaction is 'doomed' and 
  // cleanup is being done.

  Int32 rc = 0; 

  rc = ENDTRANSACTION();
  
  resetXnState();
}

short ExTransaction::commitTransaction()
{
  if (! xnInProgress())
    {
      // Set the transaDiagsArea.
      // This is the first error. So reset the diags area.
      if (transDiagsArea_)
      {
	transDiagsArea_->decrRefCount();
	transDiagsArea_ = NULL;
      }
      ExRaiseSqlError(heap_, &transDiagsArea_,
		      EXE_COMMIT_TRANSACTION_ERROR, &errorCond_);
      return -1;
    }
 
  if (! exeStartedXn_)
    {
      if (transDiagsArea_)
      {
	transDiagsArea_->decrRefCount();
	transDiagsArea_ = NULL;
      }
      ExRaiseSqlError(heap_, &transDiagsArea_,
		      EXE_CANT_COMMIT_OR_ROLLBACK, &errorCond_);
      return -1;
    }
 
  Int32 rc = 0;
  char *errStr = NULL;
  Int32 errlen = 0;
  rc = ENDTRANSACTION_ERR(errStr,errlen);
  if (rc != 0)
    {
      if (rc == FETRANSNOWAITOUT)
	{
	  // TMF returns error 81 when there is an outstanding
	  // I/O with a transaction. In this special case, the
	  // transaction is neither aborted nor committed after
	  // the call to ENDTRANSACTION. Try to abort the
	  // transaction now if it was started internally.
	  rollbackTransactionWaited();
	}

      if (rc == FEHASCONFLICT)
        createDiagsArea (EXE_COMMIT_CONFLICT_FROM_TRANS_SUBSYS, rc,
                         (errStr && errlen)? "CONFLICT ERROR" : "DTM");
      else
        createDiagsArea (EXE_COMMIT_ERROR_FROM_TRANS_SUBSYS, rc,
                          (errStr && errlen)? errStr: "DTM");
    }
  
  //Call to ENDTRANSACTION_ERR must always be followed by
  //calling DEALLOCATE_ERR so memory of allocated error str
  //is deallocated appropriately.
  DEALLOCATE_ERR(errStr);
 
  resetXnState();

  return rc;
}

////////////////////////////////////////////////////////////
// This method inherits an encompassing transaction
// and makes it the current executor transaction.
// It also closes all cursors in case the transaction
// was aborted or committed between the last time executor
// started or inherited a transaction and this time.
//
// Note: This should at most be called when entering the CLI
// for new operations. It should not be called for redrives,
// since the process transid at FILE_COMPLETE_ time is not
// relevant to no-wait statements that we wish to complete.
// Likewise, it should not be called on recursive calls to
// the CLI, because such calls can occur during no-wait
// redrives.
////////////////////////////////////////////////////////////
short ExTransaction::inheritTransaction()
{
  // Do nothing if application has not yet 'cleaned' up
  // the executor transaction.
  if (userEndedExeXn() == TRUE)
    return 0;

  // get TransId() can return:
  //     -- invalid Xn (FEINVTRANSID = 78, or others)
  //     -- no Xn (FENOTRANSID = 75)
  //     -- valid Xn (FEOK = 0)
  Int64 currentXnId = (Int64) 0;
  Int64 transid = (Int64) 0;		// Soln 10-050210-4624
  TmfPhandle_Struct currentTxHandle;

  short retcode = getCurrentXnId((Int64 *)&currentXnId, (Int64 *)&transid,
				 (short *)&currentTxHandle);

  /* TEMP TEMP TEMP 
     printf("current transaction id: %d\n", currentXnId);
  End of TEMP */
  if (xnInProgress_) // a transaction was in progress
    {
      if ((retcode != FEOK) || 
	  ((retcode == FEOK) && ((currentXnId != exeXnId_))))
        { 
	  // caller aborted/committed/started a new Xn.
	  // close all nonholdable cursors for the current context.
	  cliGlob_->currContext()->closeAllCursors(ContextCli::CLOSE_ALL, 
                      ContextCli::CLOSE_ENDED_XN);
          
	  if (exeStartedXn_)
	    {
	      // If executor has previously started the transaction,
	      // should we give an error? ANSI says only an SQL Agent
	      // that started the transaction can commit/abort it.
	      // Caller will 'clean up' this state by calling
	      // cli abort/commit work.
	      // Mark this state as a user-ended xn state.
	      // Later, if rollback or commit is called and we are in
	      // this state, clean up.
	      resetXnState();
	      userEndedExeXn_ = TRUE;
	      if (retcode == 78)
		{
		  // reset the current transid to the initial state,
		  // it is currently invalid which may cause trouble
		  ABORTTRANSACTION();
		  // may need to move this piece of code up?? (hjz, 2/14/01)
		}
	    }
	  else
	    {
	      if (retcode == FEOK)
		{
		  xnInProgress_ = TRUE;
		  exeXnId_      = currentXnId;
		  transid_      = transid;	// Soln 10-050210-4624
		  exeTxHandle_  = currentTxHandle;
		}
	      else
                {
		  xnInProgress_ = FALSE;
		  if (retcode == 75)      //  No transaction.
		    {
		      exeXnId_ = Int64(-1);
		      transid_ = Int64(-1);
		    }
                }
	      
	      exeStartedXn_ = FALSE;
              exeStartedXnDuringRecursiveCLICall_ = FALSE;
	      implicitXn_   = FALSE;
	    }
	}
        else
        {
          // The transaction handle may have changed, even though the
          // transaction is still in progress, if the transtag changed.
          // The transaction handle is made up of the transid, and the
          // transtag, something the FS (and only the FS) keeps track of
          // to track requests sent on behalf of a transaction. While TMF
          // doesn't care about the transtag, changes in the transtag do
          // cause the transaction handle to change, even though the
          // transid is the same.
	  exeTxHandle_  = currentTxHandle;
        }
    } // Xn was in progress
  else
    {
      if (retcode == FEOK)
	{
	  exeXnId_      = currentXnId;
	  transid_      = transid;		// Soln 10-050210-4624
	  exeTxHandle_  = currentTxHandle;
	  xnInProgress_ = TRUE;
	  exeStartedXn_ = FALSE;
          exeStartedXnDuringRecursiveCLICall_ = FALSE;
	  implicitXn_   = FALSE;
	}
    }

  return 0;
} // inherit transaction

////////////////////////////////////////////////////////////
// This method validates if a transaction in progress is
// still running or if it has been aborted/committed by
// a child process.
// It closes all cursors in case the transaction
// was aborted or committed.
//
// Note: This should at most be called when exiting the top
// level CLI call for new operations. It should not be called 
// for redrives, since the process transid at FILE_COMPLETE_ 
// time is not relevant to no-wait statements that we wish 
// to complete. Likewise, it should not be called on returns
// from recursive calls to the CLI, because such calls can 
// occur during no-wait redrives.
////////////////////////////////////////////////////////////
short ExTransaction::validateTransaction()
{

  // Do nothing if application has not yet 'cleaned' up
  // the executor transaction.
  if (userEndedExeXn() == TRUE)
    return 0;

  if (xnInProgress_)
    {
      // getCurrentTransId() can return:
      //     -- invalid Xn (FEINVTRANSID = 78) or others.
      //     -- no Xn (FENOTRANSID = 75)
      //     -- valid Xn (FEOK = 0)
      Int64 currentXnId = 0;

      short retcode = getCurrentXnId((Int64 *)&currentXnId);
      if (retcode == FEOK) 
	{
	  if (currentXnId != exeXnId_)
	    {
	      // Can this condition happen? Can someone called by
	      // executor start a new valid transaction visible to
	      // me? 
	      return -1;
	    }
	}
      else
	{
         
          // VO, Dec. 2005.

          // We get here because TMF has refused to deliver a current transaction id to us. 
          // That is bad. There can be two reasons why TMF won't deliver current transid:
          //   1) Something's rotten in the state of TMF (like a crash)
          //   2) Our transaction got aborted or committed behind our back
          // We want to close all cursors that belong to our transid, but we can't involve TMF.
          // The ContextCli::closeAllCursors can close all cursors, however a second parameter 
          // to that method has been added, to allow the executor's notion of
          // the current txn to be used, instead of TMF's notion. 
          
          Int64 executorTx = exeXnId_;           // save our txn id

	  if (retcode == FEINVTRANSID)
	    {
	      // Someone else aborted/committed the transaction.
	      // Clean up the transaction. Ignore errors.
	      cleanupTransaction();
	    }
	  
          cliGlob_->currContext()->closeAllCursors(ContextCli::CLOSE_ALL, 
                     ContextCli::CLOSE_CURR_XN, executorTx);
	  resetXnState();
	  return -1;
	}
    }
  return 0;
}


////////////////////////////////////////////////////////////////
// This method checks to see if the current process transaction 
// is the same as the context transaction. If  not, it attempts 
// to change the current process transaction to the context
// transaction.
//
// Ordinarily, the Executor doesn't mess with the process
// transaction, because in most code paths, the Executor and
// its file system control which transids are sent with
// each message. And it is dangerous to do so: If we take
// an exception, for example, after changing the process
// transaction and before putting the old one back, and we
// don't have proper exception handling in place, we can
// cause data integrity problems because the process has
// a different transid than expected.
//
// Presently, however, the Executor doesn't have a way
// to set the transid on open messages. So, for now
// the Executor resorts to changing the process transaction
// in this case.
//
// Returns FEOK (= 0) if successful. Returns FE error from
// TMF routines if not.
//
// If the process transaction is changed, the old process
// transaction handle is returned in oldProcessTxHandle and 
// the processTransactionChanged flag is set to TRUE. 
// Otherwise, the oldProcessTxHandle is meaningless and the 
// processTransactionChanged flag is set to FALSE.
////////////////////////////////////////////////////////////
short ExTransaction::setProcessTransToContextTrans
  (TmfPhandle_Struct * oldProcessTxHandle, /* out */
   NABoolean & processTransactionChanged /* out */)
{
  short retcode = FEOK;  // assume success


  return retcode;
}


////////////////////////////////////////////////////////////
// This method sets the process transaction to the value
// passed in by the caller.
//
// This method would be used to restore the process 
// transaction to its former value after the 
// setProcessTransToContextTrans method above was used 
// to change the process transaction.
//
// On entry, oldProcessTxHandle points to the old process
// transaction handle.
//
// Returns FEOK (= 0) if successful. Asserts if not, since
// data integrity problems would be a likely result of any
// failure.
////////////////////////////////////////////////////////////
short ExTransaction::resetProcessTrans
  (TmfPhandle_Struct * oldProcessTxHandle)
{
  short retcode = 0;

  return retcode;
}

void   ExTransaction::setTransId(Int64 transid)
{
  exeXnId_ = transid;
}

short ExTransaction::setTransMode(TransMode * trans_mode)
{
  transMode_->updateTransMode(trans_mode);
  if (trans_mode->autoCommit() == TransMode::AC_NOT_SPECIFIED_)
    transMode_->setUserTransMode(TRUE);  // user altered trans mode
  return 0;
}

void ExTransaction::disableAutoCommit()
{
  if (autoCommit() == TRUE)
    {
      // user specified AUTO COMMIT in effect.
      // Turn it off, but remember that it was turned
      // off internally.
      transMode_->autoCommit() = TransMode::OFF_;
      
      autoCommitDisabled_ = TRUE;
    }
}

void ExTransaction::enableAutoCommit()
{
  if (autoCommitDisabled_ == TRUE)
    {
      // AUTO COMMIT was internally disabled.
      // Enable it.
      transMode_->autoCommit() = TransMode::ON_;
      autoCommitDisabled_ = FALSE;
    }
}

void ExTransaction::createDiagsArea(Int32 error1, Int32 retCode,
				    const char *string)
{
  if (transDiagsArea_)
  {
    transDiagsArea_->decrRefCount();
    transDiagsArea_ = NULL;
  }

  errorCond_ = NULL;

  ExRaiseSqlError(heap_, &transDiagsArea_,
		  (ExeErrorCode)error1, &errorCond_);
  (errorCond_)->setOptionalInteger (0, retCode);
  (errorCond_)->setOptionalString (0, string);
  (errorCond_)->setNskCode(retCode);
}

// this method generates a savepoint id. Its a juliantimestamp.
// If a transaction is not in progress, no savepoint id is generated.
void ExTransaction::generateSavepointId()
{
  if (xnInProgress())
    savepointId_++;


}

/////////////////////////////////////////////////////////////////
// class ExTransTdb, ExTransTcb, ExTransPrivateState
/////////////////////////////////////////////////////////////////

ex_tcb * ExTransTdb::build(ex_globals * glob)
{
  ExTransTcb * lock_tcb = new(glob->getSpace()) ExTransTcb(*this, glob);
  
  lock_tcb->registerSubtasks();

  return (lock_tcb);
}


////////////////////////////////////////////////////////////////
// Constructor for class ExTransTcb
///////////////////////////////////////////////////////////////
ExTransTcb::ExTransTcb(const ExTransTdb & trans_tdb,
		       ex_globals * glob)
  : ex_tcb( trans_tdb, 1, glob)
{
  Space * space = (glob ? glob->getSpace() : 0);
  CollHeap * heap = (glob ? glob->getDefaultHeap() : 0);
  
  // Allocate the buffer pool
  pool_ = new(space) sql_buffer_pool(trans_tdb.numBuffers_,
				     trans_tdb.bufferSize_,
				     space);
  
  // Allocate the queue to communicate with parent
  qparent_.down = new(space) ex_queue(ex_queue::DOWN_QUEUE,
				      trans_tdb.queueSizeDown_,
				      trans_tdb.criDescDown_,
				      space);
  
  // Allocate the private state in each entry of the down queue
  ExTransPrivateState *p = new(space) ExTransPrivateState(this);
  qparent_.down->allocatePstate(p, this);
  delete p;
  
  qparent_.up = new(space) ex_queue(ex_queue::UP_QUEUE,
				    trans_tdb.queueSizeUp_,
				    trans_tdb.criDescUp_,
				    space);
  
  if (diagAreaSizeExpr())
    {
      // allocate work atp to compute the diag area size
      workAtp_ = allocateAtp(trans_tdb.workCriDesc_, space);

      // allocate tuple where the diag area size will be moved
      pool_->get_free_tuple(workAtp_->getTupp(trans_tdb.workCriDesc_->
					      noTuples() - 1), 
			    sizeof(Lng32));

      (void) diagAreaSizeExpr()->fixup(0, getExpressionMode(), this,
				       space, heap, FALSE, glob);
    }
  else
    workAtp_ = 0;
}
  

ExTransTcb::~ExTransTcb()
{
  delete qparent_.up;
  delete qparent_.down;
  delete pool_;
  pool_ = 0;
};

//////////////////////////////////////////////////////
// work() for ExTransTcb
//////////////////////////////////////////////////////
short ExTransTcb::work()
{
  while (1) {
    // if no parent request, return
    if (qparent_.down->isEmpty())
      return WORK_OK;
      
    // if no room in up queue, won't be able to return data/status.
    // Come back later.
    if (qparent_.up->isFull())
      return WORK_OK;
      
    ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
    ExTransPrivateState & pstate = 
      *((ExTransPrivateState*) pentry_down->pstate);

    ExTransaction *ta = getGlobals()->castToExExeStmtGlobals()->
      castToExMasterStmtGlobals()->getStatement()->
      getContext()->getTransaction();

    // Check if transaction is blocked in the current context
    ExExeStmtGlobals *stmtGlob = getGlobals()->castToExExeStmtGlobals();
    CliGlobals *cliGlobals =
      stmtGlob->castToExMasterStmtGlobals()->getCliGlobals();

    // if inside of a UDR, let a SET XN stmt get through if it
    // was compiled with setAllowedInXn option. This is allowed
    // for jdbc/odbc/sqlj sql statements.
    if ((cliGlobals->getUdrErrorChecksEnabled()) &&
	((transTdb().transType_ != SET_TRANSACTION_) ||
	 (NOT transTdb().setAllowedInXn())))
    {
      cliGlobals->setUdrXactViolation(TRUE);
      
      ex_queue_entry *up_entry = qparent_.up->getTailEntry();

      up_entry->upState.parentIndex = pentry_down->downState.parentIndex;
      up_entry->upState.setMatchNo(0);
      up_entry->upState.status = ex_queue::Q_SQLERROR;
      
      ComDiagsArea *diags_my = up_entry->getDiagsArea();
      
      if (!diags_my){
        diags_my = ComDiagsArea::allocate(stmtGlob->getDefaultHeap());
        up_entry->setDiagsArea(diags_my);
      }
      *diags_my << DgSqlCode(- CLI_NO_TRANS_STMT_VIOLATION);
      
      // insert into parent
      qparent_.up->insert();
      
    }	
    else{

    short rc;
    switch (transTdb().transType_)  {
      case BEGIN_: {
        if (ta->userEndedExeXn())
        {
          handleErrors(pentry_down, NULL, 
             (ExeErrorCode)(-CLI_USER_ENDED_EXE_XN));
          break;
        }
	
        rc = ta->beginTransaction();
        if (rc != 0)  {
          // beginTransaction returned an error.
          // create a diagsArea and return it to the parent.
          handleErrors(pentry_down, ta->getDiagsArea());
        }

	// if user specified auto commit was on, turn it off.
	// It will remain turned off until a COMMIT or ROLLBACK statement
	// is issued.
        ta->disableAutoCommit();
        ta->implicitXn() = FALSE;
      }
      break;
	  
      case COMMIT_: {
        if (ta->userEndedExeXn()) {
          ta->cleanupTransaction();
          handleErrors(pentry_down, NULL, 
             (ExeErrorCode)(-CLI_USER_ENDED_XN_CLEANUP));

          break;
        }

        // close all open cursors that are part of this xn-- ANSI requirement.
        // get current context and close all statements.
        getGlobals()->castToExExeStmtGlobals()->
          castToExMasterStmtGlobals()->getStatement()->
          getContext()->closeAllCursors(ContextCli::CLOSE_ALL, ContextCli::CLOSE_CURR_XN);

        rc = ta->commitTransaction();
        if (rc != 0)
          handleErrors(pentry_down, ta->getDiagsArea());

        if (cliGlobals->currContext()->ddlStmtsExecuted())
          {
            ComDiagsArea * diagsArea = NULL;
            ExSqlComp::ReturnStatus cmpStatus = 
              cliGlobals->currContext()->sendXnMsgToArkcmp
              (NULL, 0,
               EXSQLCOMP::DDL_NATABLE_INVALIDATE,
               diagsArea);
            if (cmpStatus == ExSqlComp::ERROR)
              {
                cliGlobals->currContext()->ddlStmtsExecuted() = FALSE;

                handleErrors(pentry_down, NULL, 
                             (ExeErrorCode)(-EXE_CANT_COMMIT_OR_ROLLBACK));
 
                return -1;
              }
          }
        
        cliGlobals->currContext()->ddlStmtsExecuted() = FALSE;
	      	    
        // if user had specified AUTO COMMIT, turn it back on.
        ta->enableAutoCommit();
      }
      break;
      
      case ROLLBACK_:  {
        if (ta->userEndedExeXn()) {
          ta->cleanupTransaction();
          handleErrors(pentry_down, NULL, 
             (ExeErrorCode)(-CLI_USER_ENDED_XN_CLEANUP));
          break;
        }
        
        // close all open cursors that are part of this xn --ANSI requirement.
        // get current context and close all statements.
        getGlobals()->castToExExeStmtGlobals()->
          castToExMasterStmtGlobals()->getStatement()->
          getContext()->closeAllCursors(ContextCli::CLOSE_ALL_INCLUDING_ANSI_PUBSUB_HOLDABLE_WHEN_CQD,
                    ContextCli::CLOSE_CURR_XN);

        rc = ta->rollbackTransaction();
        if (rc != 0)
          // rollbackTransaction returned an error.
          // create a diagsArea and return it to the parent.
          handleErrors(pentry_down, ta->getDiagsArea());

        if (cliGlobals->currContext()->ddlStmtsExecuted())
          {
            ComDiagsArea * diagsArea = NULL;
            ExSqlComp::ReturnStatus cmpStatus = 
              cliGlobals->currContext()->sendXnMsgToArkcmp
              (NULL, 0,
               EXSQLCOMP::DDL_NATABLE_INVALIDATE,
               diagsArea);
            if (cmpStatus == ExSqlComp::ERROR)
              {
                cliGlobals->currContext()->ddlStmtsExecuted() = FALSE;

                handleErrors(pentry_down, NULL, 
                             (ExeErrorCode)(-EXE_CANT_COMMIT_OR_ROLLBACK));
 
                return -1;
              }
          }
        
        cliGlobals->currContext()->ddlStmtsExecuted() = FALSE;

        // if user had specified AUTO COMMIT, turn it back on.
        ta->enableAutoCommit();
      }
      break;

      case ROLLBACK_WAITED_:  {
        if (ta->userEndedExeXn()) {
          ta->cleanupTransaction();
          handleErrors(pentry_down, NULL, 
            (ExeErrorCode)(-CLI_USER_ENDED_XN_CLEANUP));
          break;
        }
        // close all open cursors that are part of this xn --ANSI requirement.
        // get current context and close all statements.
        getGlobals()->castToExExeStmtGlobals()->
          castToExMasterStmtGlobals()->getStatement()->
          getContext()->closeAllCursors(ContextCli::CLOSE_ALL_INCLUDING_ANSI_PUBSUB_HOLDABLE_WHEN_CQD,
                                ContextCli::CLOSE_CURR_XN);

        rc = ta->rollbackTransactionWaited();
        if (rc != 0) 
          handleErrors(pentry_down, ta->getDiagsArea());
	      
        if (cliGlobals->currContext()->ddlStmtsExecuted())
          {
            ComDiagsArea * diagsArea = NULL;
            ExSqlComp::ReturnStatus cmpStatus = 
              cliGlobals->currContext()->sendXnMsgToArkcmp
              (NULL, 0,
               EXSQLCOMP::DDL_NATABLE_INVALIDATE,
               diagsArea);
            if (cmpStatus == ExSqlComp::ERROR)
              {
                cliGlobals->currContext()->ddlStmtsExecuted() = FALSE;

                handleErrors(pentry_down, NULL, 
                             (ExeErrorCode)(-EXE_CANT_COMMIT_OR_ROLLBACK));
 
                return -1;
              }
          }
        
        cliGlobals->currContext()->ddlStmtsExecuted() = FALSE;

        // if user had specified AUTO COMMIT, turn it back on.
        ta->enableAutoCommit();
      }
      break;
	 
      case SET_TRANSACTION_:{
	   
	if ((ta->xnInProgress()) &&
	    (NOT transTdb().setAllowedInXn()))
	  {
	    handleErrors(pentry_down, NULL,
			 (ExeErrorCode)-EXE_SET_TRANS_ERROR_FROM_TRANS_SUBSYS);
	    break;
	  }

        char * data = (char *)(transTdb().transMode_.getPointer());
        ComDiagsArea * diagsArea = NULL;
        ExSqlComp::ReturnStatus cmpStatus =
          getGlobals()->castToExExeStmtGlobals()->
          castToExMasterStmtGlobals()->getStatement()->
          getContext()->sendXnMsgToArkcmp(data, sizeof(TransMode),
                                          EXSQLCOMP::SET_TRANS,
                                          diagsArea);
        if (cmpStatus == ExSqlComp::ERROR)
          handleErrors(pentry_down, diagsArea);
          
        if (diagAreaSizeExpr()) {
          // compute the diag area size
          if (diagAreaSizeExpr()->eval(pentry_down->getAtp(), workAtp_) 
              == ex_expr::EXPR_ERROR)  {
            // handle errors
            handleErrors(pentry_down, pentry_down->getAtp()->getDiagsArea()); 
          }

          ta->getTransMode()->diagAreaSize() = 
            *(Lng32 *)(workAtp_->getTupp
                       (transTdb().workCriDesc_->noTuples()-1).getDataPointer());
        }
        
        if (cmpStatus != ExSqlComp::ERROR) {
          if(ta->setTransMode(transTdb().transMode_)) {
            handleErrors(pentry_down, ta->getDiagsArea());		
            break;
          }
        }
      } 
      break;
	  
      default:   {
        ex_assert(0, "Option not supported,");
      }
      break;
	  
    }	// switch 
    } // else :  not check transaction violation
    
    // all ok. Return EOF.
    ex_queue_entry * up_entry = qparent_.up->getTailEntry();
      
    up_entry->upState.parentIndex = 
      pentry_down->downState.parentIndex;
      
    up_entry->upState.setMatchNo(0);
    up_entry->upState.status = ex_queue::Q_NO_DATA;
      
    // insert into parent
    qparent_.up->insert();
    
    qparent_.down->removeHead();
  }  
  return WORK_OK;
}

// if diagsArea is not NULL, then its error code is used.
// Otherwise, err is used to handle error.
void ExTransTcb::handleErrors(ex_queue_entry *pentry_down, 
			      ComDiagsArea *da,
			      ExeErrorCode err)
{
  ExHandleArkcmpErrors(qparent_, pentry_down, 0, getGlobals(), da, err);
}

///////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for Lock_private_state
///////////////////////////////////////////////////////////////////////////////

ExTransPrivateState::ExTransPrivateState(const ExTransTcb * /*tcb*/)
{
  step_ = ExTransTcb::EMPTY_;
}

ExTransPrivateState::~ExTransPrivateState()
{
};

ex_tcb_private_state * ExTransPrivateState::allocate_new(const ex_tcb *tcb)
{
  return new(((ex_tcb *)tcb)->getSpace()) 
    ExTransPrivateState((ExTransTcb *) tcb);
};


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
 * File:         NoWaitOp.cpp
 * Description:  Functions of NowaitOp class.
 *               
 * Created:      3/26/2002
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"
#include "ComCextdecs.h"

#include "cextdecs/cextdecs.h"

#include <stdlib.h>
#include "cli_stdh.h"
#include "Ipc.h"
#include "ex_stdh.h"
#include "SQLCLI.h"
#include "NoWaitOp.h"
#include "Statement.h"
#include "Descriptor.h"
#include "ExStats.h"
#include "ex_exe_stmt_globals.h"

// Methods for class NoWaitOp

NoWaitOp::NoWaitOp(Statement * stmt,
             Descriptor * inputDesc, Descriptor * outputDesc,
             Lng32 tag, NoWaitOp::opType op, NABoolean initiated)
             : stmt_(stmt), inputDesc_(inputDesc),
             outputDesc_(outputDesc),tag_(tag),op_(op),
             initiated_(initiated)
  {
  // lock Descriptors ($$$ what happens if already locked?)
  if (inputDesc)
    inputDesc->lockForNoWaitOp(); // $$$ ignores possible error return code

  if (outputDesc)
    outputDesc->lockForNoWaitOp(); // $$$ ignores possible error return code
  }

NoWaitOp::~NoWaitOp(void)
  {
  // unlock Descriptors ($$$ what happens if already unlocked?)
  if (inputDesc_)
    inputDesc_->unlockForNoWaitOp(); // $$$ ignores possible error return code
  
  if (outputDesc_)
    outputDesc_->unlockForNoWaitOp(); // $$$ ignores possible error return code
  }


RETCODE NoWaitOp::awaitIox(Lng32 * tag)
  {

  RETCODE rc = NOT_FINISHED;  // assume not finished yet
  RETCODE rc1;

  // set current context to that of this Statement

  // $$$ note that the diagsArea might be from a different context;
  // I think this is OK, but it bears closer inspection.

  // $$$ this code should be bracketed with a try-catch block to
  // restore the original context in the event of an exception.
  
  ContextCli * stmtContext = stmt_->getContext();
  ComDiagsArea &diagsArea = stmtContext->diags();
  CliGlobals * cliGlobals = stmtContext->getCliGlobals();
  ContextCli * oldCurrentContext = cliGlobals->currContext();
  cliGlobals->setCurrentContext(stmtContext);
  jmp_buf jmpBuf, *oldJmpBufPtr;
  oldJmpBufPtr = cliGlobals->getJmpBufPtr();

  cliGlobals->setJmpBufPtr(&jmpBuf);
  Lng32 jmpRetcode;
  Int32 jmpRc = setjmp(jmpBuf);
  if (jmpRc)
    {
    if (jmpRc == MEMALLOC_FAILURE)
      jmpRetcode = -EXE_NO_MEM_TO_EXEC;
    else
    {
      stmt_->resetNoWaitOpPending();
      jmpRetcode = -CLI_INTERNAL_ERROR;
    }
    *tag = tag_;
    diagsArea << DgSqlCode(jmpRetcode);
    rc = ERROR;
    }
  else
    {
    Int64 startTime = NA_JulianTimestamp();
  
    switch (op_)
      {
      case FETCH:
      case FETCH_CLOSE:
	{
	// drive the fetch with a zero time limit (since we drive
	// the IPC wait in this layer instead of in the Executor
	// layer), but drive it only if there is dispatchable work
	// to do
	if (stmt_->mightHaveWorkToDo()) //Do this for fetch but not prepare
	  {
	  // redrive the fetch (FALSE indicates not a new fetch)

	  rc = stmt_->fetch(cliGlobals,outputDesc_,diagsArea,FALSE);
        
	  // $$$ for the moment, assume at most one no-wait op per
	  // Statement; can relax this later
	  if (rc != NOT_FINISHED)
	    {
	    stmt_->resetNoWaitOpPending();
	    *tag = tag_;

	    // Fixup the diags area and outputDesc.
	    // if bulk move was done, remember the statement it was done for.
	    if ((outputDesc_) && (NOT outputDesc_->bulkMoveDisabled()))
	      {
	      //if (getenv("BULKMOVEWARN"))
	      //	diags << DgSqlCode(EXE_ERROR_NOT_IN_USE_8350);

	      outputDesc_->bulkMoveStmt() = stmt_;
	      }

	    // if select into query, then make sure that atmost one
	    // row is returned by executor. More than one would
	    // result in an error.
	    if ((stmt_->isSelectInto()) &&
		(rc == SUCCESS))
	      {
		// BertBert VV
	      if (stmt_->isEmbeddedUpdateOrDelete() || stmt_->isStreamScan())
		{
		// For streams and destructive selects, we don't want the
		// abovebehavior,instead, we should just return the first row.
		}
	      // BertBert ^^
	      else
		{
		// select into and a row was returned.
		// See if we can get one more row.
		// Do not send in an output desc. We want
		// to return the first row to application.
		// This is being consistent with SQL/MP behavior.
		stmt_->resetNoWaitOpEnabled();  // waited mode
		rc = stmt_->fetch(cliGlobals, 0 /*no output desc*/,
				  diagsArea, TRUE);

		if (rc == SUCCESS)
		  {
		  diagsArea << DgSqlCode(-CLI_SELECT_INTO_ERROR);
		  if (op_ == FETCH)
		    stmt_->close(diagsArea);
		  rc = ERROR;
		  }
	    
		if (rc == SQL_EOF)
		  {
		  // remove warning 100 from diags.
		  diagsArea.removeFinalCondition100();

		  rc = SUCCESS;
		  }
		}
	      }

	    if ((rc == SQL_EOF) && (outputDesc_ == NULL))
	      {
	      // remove warning 100 from diags.
	      diagsArea.removeFinalCondition100();
	  
	      rc = SUCCESS;
	      }
	    }   // end if rc != NOT_FINISHED

	  }

	  if (rc != NOT_FINISHED && op_ == FETCH_CLOSE)
	  {
	    rc1 = stmt_->close(diagsArea);
	    if (rc1 == ERROR)
		rc = rc1;
	  }

	  if ((op_ == FETCH_CLOSE) &&
	      (rc != NOT_FINISHED) && 
	      (rc != ERROR) &&
	      (stmt_->noRowsAffected(diagsArea)))
	    {
	      rc = 
		((diagsArea.getNumber(DgSqlCode::WARNING_) > 0) ? SUCCESS : SQL_EOF);
	      // move EOF warning to diags area.
	      if (rc == SQL_EOF)
		diagsArea << DgSqlCode(SQL_EOF);
	    }

	break;
	}

      case PREPARE:
	{
	// drive the prepare with a zero time limit
	ULng32 flags = PREPARE_NOT_A_NEW_OPERATION;
	rc = stmt_->prepare(NULL,diagsArea,NULL,0,0,TRUE,flags);
      
	// $$$ for the moment, assume at most one no-wait op per
	// Statement; can relax this later
	if (rc != NOT_FINISHED)
	  {
          stmt_->issuePlanVersioningWarnings (diagsArea);
	  stmt_->resetNoWaitOpPending();
	  *tag = tag_;
	  }
	break;
	}

      default:
	{
	// $$$ operation invalid or not supported yet
	rc = ERROR;
	diagsArea << DgSqlCode(-EXE_INTERNAL_ERROR); 
	break;
	}
      }
    }


  // restore original current context
  cliGlobals->setCurrentContext(oldCurrentContext);
  cliGlobals->setJmpBufPtr(oldJmpBufPtr);
  return rc;
  }


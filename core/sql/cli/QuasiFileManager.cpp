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
 * File:         QuasiFileManager.cpp
 * Description:  Functions of QuasiFileManager and QuasiFileber.
 *               
 * Created:      3/26/2002
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------


#include "Platform.h"


#include <stdlib.h>
#include "cli_stdh.h"
#include "Ipc.h"
#include "ex_stdh.h"
#include "QuasiFileManager.h"
#include "NoWaitOp.h"

#define SQL_QFO_FUNCTION_ATTRIBUTES __declspec(dllexport)
#include "cextdecs/cextdecs.h"
#include "guardian/dpcbz.h"


SQL_QFO_FUNCTION_ATTRIBUTES short Sql_Qfo_IOComp(short quasi_file_number /*in*/,
				     Lng32 *tag /*out*/,
				     unsigned short *waitmask /*out*/,
				     short userstop /*in*/);
SQL_QFO_FUNCTION_ATTRIBUTES short Sql_Qfo_Close(short quasi_file_number /*in*/);

//*************************************************************
// Methods of QuasiFileManager
//*************************************************************
 
QuasiFileManager::QuasiFileManager(NAHeap * noWaitHeap,
                               IpcEnvironment * ipcEnv) : 
pendingNoWaitOperations_(0), ipcEnv_(ipcEnv), noWaitHeap_(noWaitHeap)
  {
  quasiFileList_ = new(noWaitHeap_) Queue(noWaitHeap_);
  }

QuasiFileManager::~QuasiFileManager(void)
  {
  // delete quasiFile list
  assert (quasiFileList_->isEmpty());

  //  delete quasiFileList_;
  NADELETE(quasiFileList_, Queue, noWaitHeap_);


  // $$$ need to think about policy here... do we do disassociates
  // on all Statements? Or do we assume we only get called after
  // contexts and statements are destroyed?
  }

RETCODE QuasiFileManager::assocFileNumber(ComDiagsArea &diagsArea,
                                           short fileNumber,
                                           Statement * statement)
  {
  RETCODE rc = SUCCESS; 
  QuasiFile *fn = NULL;

  if (statement->getFileNumber() != -1)
    {
    // Statement is already associated with some file number --
    // generate error
    rc = ERROR;
    // $$$ for now, just raise an internal error
    diagsArea << DgSqlCode(-CLI_STATEMENT_ASSOCIATED_WITH_QFO);
    }
  else 
    {
    if ((fn = getQuasiFile(fileNumber)) == NULL)  // quasiFile entry not exist
       {
       
       // first check if filename is $QFO


	// add new quasiFile to list
	fn = new (noWaitHeap_) 
	        QuasiFile (noWaitHeap_, fileNumber, this);
	quasiFileList_ -> insert((void *)fn);
	} // create a new entry
       
    // associate this statement with this file number
    fn->associateStatement(statement);
   
    } // else
   
  return rc;
  }



RETCODE QuasiFileManager::disassocFileNumber(ComDiagsArea &diagsArea,
					     Statement * statement,
					     NABoolean force)
  { 
  RETCODE rc = SUCCESS;  // assume success

  short fileNumber = statement->getFileNumber();

  if (fileNumber == -1)
    {
    diagsArea << DgSqlCode(-CLI_STATEMENT_WITH_NO_QFO);
    rc = ERROR;
    }
  else
  {
    if (statement->noWaitOpPending() && force)
      deleteNoWaitOps(diagsArea, fileNumber, statement);
  
    if (statement->noWaitOpPending())
      {
      // Statement has an incompleted no-wait op
      // $$$ Later can consider cancelling or completing, or raising
      // a user error but for now raise an internal error in this case
      diagsArea << DgSqlCode(-CLI_OPERATION_WITH_PENDING_OPS);
      rc = ERROR;
      }
    else
      {
      QuasiFile * fn = getQuasiFile (fileNumber);

      if (fn == NULL)
	{
	diagsArea << DgSqlCode(-CLI_INVALID_QFO_NUMBER);
	rc = ERROR;
	}
      else
	{
	if (fn->disassociateStatement(statement))
	  {
	  // last associated statement
	  quasiFileList_->remove((void *)fn);
	  delete fn;
	  }
	}
      }
  }

  return rc;
  }


RETCODE QuasiFileManager::deleteNoWaitOps(ComDiagsArea &diagsArea,
					short fileNumber, 
					Statement * stmt)
  {
  RETCODE rc = SUCCESS;  // assume success

  QuasiFile * fn = getQuasiFile(fileNumber);
    
  if (fn == NULL)
    {
    // trying to delete no-wait ops for a file number that is not allocated --
    // generate error
    rc = ERROR;
    diagsArea << DgSqlCode(-CLI_INVALID_QFO_NUMBER);
    }
  else
    {
    // delete outstanding nowait ops
    fn->deleteNoWaitOps(stmt);
    }
  return rc;
  }

RETCODE QuasiFileManager::awaitIox(Lng32 fileNumber,
				 Lng32 * tag,
				 short * feError)
  {
  RETCODE rc = NOT_FINISHED;  // show no completions yet
  QuasiFile *quasiFile;
 
  quasiFile = getQuasiFile(fileNumber);
  if (quasiFile != NULL)
    rc = quasiFile->awaitIox(ipcEnv_, tag, feError);
  else
    *feError = FEBADPARMVALUE; // shouldn't be called with this file number

  return rc;
  }



QuasiFile * QuasiFileManager::getQuasiFile(short fileNumber)
  {
  QuasiFile * fn = NULL; // assume failure
  quasiFileList_->position();
  fn = (QuasiFile *)quasiFileList_->getNext();

  // go through the quasiFileList and find a match.
  while (fn)
    {
    if (fileNumber == fn->getFileNumber())
      return fn;
    else
      fn = (QuasiFile *)quasiFileList_->getNext();
    }
  return fn;
  }


void QuasiFileManager::notifyOfNewNoWaitOp(void)
  {
  pendingNoWaitOperations_++;
  }

void QuasiFileManager::closeQuasiFile(short fileNumber)
  {
  QuasiFile *quasiFile = getQuasiFile(fileNumber);
  if (quasiFile)
    {
    quasiFile->closeNoWaitOpsPending();
    quasiFileList_->remove((void *)quasiFile);
    delete quasiFile;
    }
  }

//***************************************************************************
// Methods for class QuasiFile
//***************************************************************************

QuasiFile::QuasiFile(NAHeap * noWaitHeap,
		       short fileNumber,
		       QuasiFileManager *fnm) 
  : fileNumber_(fileNumber), noWaitHeap_(noWaitHeap),
     quasiFileManager_(fnm)
  {
  associatedStatements_ = new(noWaitHeap_) HashQueue(noWaitHeap_);
  pendingNoWaitOps_ = new(noWaitHeap_) Queue(noWaitHeap_);
  }

QuasiFile::~QuasiFile(void)
  {
  assert(pendingNoWaitOps_->isEmpty());

  // cannot use 'delete pendingNoWaitOps_' since it is not an NABasicObject.
  // Need to add a Queue::cleanup method that will deallocate all the local
  // members of Queue. Call that first and then call deallocateMemory.  TBD.
  noWaitHeap_->deallocateMemory((void *)pendingNoWaitOps_);
  //  delete pendingNoWaitOps_;
  
  // iterate through all associated Statements, disassociating them
  
  associatedStatements_->position();
  Statement * stmt = (Statement *)associatedStatements_->getNext();
  while (stmt)
    {
    // Disassociate statement, but without removing it from the list
    
    // We do this to defer calling Queue::remove(). Calling it
    // now would force us to do another Queue::position() call.
    // Also, the Queue destructor already contains logic to
    // remove queue entries, so just deleting the Queue will
    // do the trick.
    stmt->resetFileNumber();
    stmt->resetNoWaitOpEnabled();
       
    stmt = (Statement *)associatedStatements_->getNext();
    }
  delete associatedStatements_;
  }

// Note: These methods assume the caller has already validated that
// the operation is a valid thing to do.

void QuasiFile::associateStatement(Statement * stmt)
  {
  // associate this statement with this file number
  stmt->setFileNumber(fileNumber_);
  associatedStatements_->insert((char*)&stmt, 
				sizeof(char *), 
				(void *)stmt);
  // Set the nowait enabled state in the Statement object
  // stmt->setNoWaitOpEnableStatus(TRUE);
  }

NABoolean QuasiFile::disassociateStatement(Statement * stmt)
  {
  // disassociate this statement with this file number
  stmt->resetFileNumber();
  stmt->resetNoWaitOpEnabled();
  associatedStatements_->position((char*)&stmt,
                                  sizeof(char *));
  associatedStatements_->getNext();
  associatedStatements_->remove((void *)stmt);
  // nothing to delete because the statement remains
  return associatedStatements_->isEmpty();
  }

void QuasiFile::disableNoWaitOps(void)
  {
  // disable no-wait operations (updating cached flags in Statement 
  // objects too)
  associatedStatements_->position();
  Statement * stmt = (Statement *)associatedStatements_->getNext();
  while (stmt)
    {
    stmt->resetNoWaitOpEnabled();
    stmt = (Statement *)associatedStatements_->getNext();
    }
  }


void QuasiFile::deleteNoWaitOps(Statement * stmt)
  {
  // delete no-wait operations associated with the current statement
  // (this is done when Statement level methods are about to do a cancel)

  // $$$ at the moment, the code deletes all no-wait ops; it probably should
  // only delete no-wait fetches. It works, though, because at the moment,
  // the only no-wait ops *are* fetches.
  
  pendingNoWaitOps_->position();
  NoWaitOp * nwo = (NoWaitOp *)pendingNoWaitOps_->getNext();
  while (nwo)
    {
    if (stmt == nwo->getStatement())
      {
      // this no-wait op is on the current statement
      pendingNoWaitOps_->remove((void *)nwo); // remove it
      pendingNoWaitOps_->position();          // position to beginning
      delete nwo;  // destroy it
      quasiFileManager_->notifyOfDeletedNoWaitOp();
      }
    nwo = (NoWaitOp *)pendingNoWaitOps_->getNext();      
    }

  // indicate no no-wait ops pending now
  stmt->resetNoWaitOpPending();
  }

void QuasiFile::closeNoWaitOpsPending()
  {
  // remove any pending nowait objects and set the flag in the statement 
  // object to indicate that the QFO file was closed while a nowait
  // operation was incomplete

  pendingNoWaitOps_->position();
  NoWaitOp * noWaitOp = (NoWaitOp *)pendingNoWaitOps_->getNext();
  while (noWaitOp)
    {
    noWaitOp->getStatement()->setNoWaitOpIncomplete(); // mark the statement
    noWaitOp->getStatement()->resetNoWaitOpPending(); // mark the statement
    pendingNoWaitOps_->remove((void *)noWaitOp); // remove it
    pendingNoWaitOps_->position();               // position to beginning
    delete noWaitOp;  // destroy it
    quasiFileManager_->notifyOfDeletedNoWaitOp();
    noWaitOp = (NoWaitOp *)pendingNoWaitOps_->getNext();     
    }
  }

RETCODE QuasiFile::awaitIox(IpcEnvironment * ipcEnv,
			    Lng32 * tag,
			    short * feError)
  {

  RETCODE rc = NOT_FINISHED;  // assume nothing finished
  
  pendingNoWaitOps_->position();
  NoWaitOp * noWaitOp = (NoWaitOp *)pendingNoWaitOps_->getNext();
  
  if (noWaitOp == NULL)
    {
    // This can happen if awaitiox is called with this filenum (user error),
    // or filenum -1 (might be normal usage)
    *feError = FENONEOUT;
    }
  else
    {

    //Future: Will mark statement dispatchable if a message "is done"
    ipcEnv->getAllConnections()->waitOnAll(0);

    // clean up the completed MasterEspMessages
    ipcEnv->deleteCompletedMessages();

    while (noWaitOp)
      {
      Lng32 numPendingBeforeRedrive =
	quasiFileManager_->getPendingNowaitOps();
      rc = noWaitOp->awaitIox(tag);
      if (rc == NOT_FINISHED)
        {
        noWaitOp = (NoWaitOp *)pendingNoWaitOps_->getNext();
        }
      else // it completed
        {       
        // remove NoWaitOp object from our list and destroy it and
	// decr the pending count, if the redrive hasn't done it all
	if (!pendingNoWaitOps_->remove((void *)noWaitOp))
	  {
	  assert(quasiFileManager_->getPendingNowaitOps() ==
	         numPendingBeforeRedrive - 1);
	  }
	else
	  {
	  delete noWaitOp;
	  quasiFileManager_->notifyOfDeletedNoWaitOp();
	  }

        noWaitOp = NULL;  // to exit loop without further processing
        }
      }
    }      

  return rc;
  }


RETCODE QuasiFile::queueNoWaitOp(ComDiagsArea &diagsArea,
				 Statement * stmt,
				 Descriptor * inputDesc,
				 Descriptor * outputDesc,
				 NoWaitOp::opType op,
				 NABoolean operationStarted,
				 Lng32 tag )
  {
  RETCODE rc = SUCCESS; // assume we are successful
  
    // Create a NoWaitOp object to represent the incompleted operation
    // and queue it
    
  NoWaitOp * nwo = new(noWaitHeap_)
    NoWaitOp(stmt,
    inputDesc,
    outputDesc,
    tag, 
    op, 
    operationStarted);

  pendingNoWaitOps_->insert((void *)nwo);
  quasiFileManager_->notifyOfNewNoWaitOp();
  return rc;
  }

// Code that does SEGMENT_REVEAL_ appears in three places:
//   switchToPriv() in cli/CliLayerForNsk.cpp
//   QfoRevealSegs() in QuasiFileManager.cpp
//   stopCatcher() in cli/CliLayerForNsk.cpp
short QfoRevealSegs(CliGlobals *&cliGlobals)
  {
  cliGlobals = GetCliGlobals();
  cliGlobals->incrNumOfCliCalls();
  return 0;
  }

//Code that does SEGMENT_HIDE_ appears in two places
//  switchToNonPriv() in cli/CliLayerForNsk.cpp
//  QfoHideSegs() in cli/CliLayerForNsk.cpp
short QfoHideSegs(CliGlobals *cliGlobals)
  {

  cliGlobals->decrNumOfCliCalls();

  return 0;
  }

SQL_QFO_FUNCTION_ATTRIBUTES short Sql_Qfo_IOComp(short quasi_file_number /*in*/,
				     Lng32 *tag /*out*/,
				     unsigned short *waitmask /*out*/,
				     short userstop /*in*/)
  {
  short retVal, feError = FEOK;
  RETCODE retcode;
  QuasiFileManager *quasiFileManager;
  *waitmask = LDONE;
  CliGlobals *cliGlobals;
  if (QfoRevealSegs(cliGlobals) != 0)
    return FEBADPARMVALUE;
  jmp_buf jmpBuf;
  short oldStop;
  oldStop = SETSTOP(1);
  cliGlobals->setJmpBufPtr(&jmpBuf);
  Int32 jmpRc = setjmp(jmpBuf);
  if (jmpRc)
    {
    QfoHideSegs(cliGlobals);
    SETSTOP(oldStop);
    return FEBADPARMVALUE; // longjmp not associated with statement
    }
  quasiFileManager = cliGlobals->getQuasiFileManager();
  if (quasiFileManager->getPendingNowaitOps() > 0)
    retcode = quasiFileManager->awaitIox(quasi_file_number, tag, &feError);
  else
    {
    QfoHideSegs(cliGlobals);
    SETSTOP(oldStop);
    return FENONEOUT;
    }
  if (feError != FEOK)
    retVal = feError; // May be FEBADPARMVALUE, or FENONEOUT
  else
    {
    if (1) // Not used but is compiled on NT
      retVal = FEQFOEVENTCONSUMED;
    else
      switch (retcode)
      {
      case SUCCESS:
	retVal = FEOK;
	break;
      case ERROR:
	retVal = FESQLERR;
	break;
      case SQL_EOF:
      case WARNING:
	retVal = FESQLWARN;
	break;
      case NOT_FINISHED:
	retVal = FEQFONOTCOMPLETE;
	break;
      default:
	retVal = FEBADPARMVALUE;
      }
    }
  QfoHideSegs(cliGlobals);
  SETSTOP(oldStop);
  return retVal;
  }


SQL_QFO_FUNCTION_ATTRIBUTES short Sql_Qfo_Close(short quasi_file_number)
  {
  CliGlobals *cliGlobals;
  if (QfoRevealSegs(cliGlobals) != 0)
    return 0;
  cliGlobals->setLogEmsEvents(FALSE);
  jmp_buf jmpBuf;
  cliGlobals->setJmpBufPtr(&jmpBuf);
  Int32 jmpRc = setjmp(jmpBuf);
  if (jmpRc)
    {
    cliGlobals->setLogEmsEvents(TRUE);
    QfoHideSegs(cliGlobals);
    return 0;
    }
  QuasiFileManager *quasiFileManager = cliGlobals->getQuasiFileManager();
  quasiFileManager->closeQuasiFile(quasi_file_number);
  cliGlobals->setLogEmsEvents(TRUE);
  QfoHideSegs(cliGlobals);
  return 0;
  }


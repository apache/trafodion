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
******************************************************************************
*
* File:         ExSequenceGenerator.cpp
* Description:  Methods for the tdb and tcb of the Sequence Generator
*
*
* Created:      03/04/2008
* Language:     C++
*
*
*
*
******************************************************************************
*/

//
// This file contains all the executor methods associated
// with the sequence generator
//

#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ex_root.h"
#include "ex_queue.h"
#include "ExSequenceGenerator.h"
#include  "ex_transaction.h"


extern "C" {
#include "cextdecs/cextdecs.h"
}


extern "C" {
int_16 TMF_SETTXHANDLE_(short *);
int_16 TMF_GETTXHANDLE_(short *);
}

static const void BreakIntoDebugger(char *str1, char *str2)
{
  // make sure you uncomment the #define TRACE_ESP_ACCESS 1
  // in executor/ex_exe_stmt_globals.h to enable logging.
#if defined(_DEBUG) && defined(TRACE_ESP_ACCESS)

      // bring up INSPECT if this is NSK (OSS as well as Guardian)
      DEBUG();

#endif  // ifdef _DEBUG && TRACE_ESP_ACCESS

} 

//
// Build a sequence generator tcb from a sequence generator TDB.
// Allocates a new ExSequenceGeneratorTcb.
// Adds this tcb to the schedulers task list.
ex_tcb *
ExSequenceGeneratorTdb::build(ex_globals * glob)
{

  // Build the Tcb tree below the sequence generator node.
  //
  ex_tcb *childTcb = childTdb_->build(glob);

  // Allocate and initialize a new sequence generator TCB.
  ExSequenceGeneratorTcb *sequenceGeneratorTcb =
    new(glob->getSpace()) ExSequenceGeneratorTcb(*this, *childTcb, glob);


  // add the sequence generator tcb to the scheduler's task list.
  sequenceGeneratorTcb->registerSubtasks();

  return sequenceGeneratorTcb;
}

//
//  TCB procedures
//

//
// Constructor for ExSequenceGeneratorTcb.  Called by the build
// function of the ExSequenceGeneratorTdb.  This will initialize the
// internal state of the sequence generator Tcb and allocate the
// queues used to communicate with the parent.

ExSequenceGeneratorTcb::ExSequenceGeneratorTcb
(const ExSequenceGeneratorTdb &sequenceGeneratorTdb,
 const ex_tcb &childTcb,
 ex_globals *glob)
  : ex_tcb(sequenceGeneratorTdb, 1, glob),
    workState_(SEQGEN_NEWBLOCK),
    blockSize_(0),
    transactions_(0),
    currTransaction_(NULL),
    xnReturnCode_(0),
    retryTimeout_(0)
{
  childTcb_ = &childTcb;

   ex_assert(glob != NULL,
	     "ExSequenceGenerator::ExSequenceGenerator ex_globals must be present");
	
  Space * space = glob->getSpace();
  CollHeap * heap = glob->getDefaultHeap();

  // Allocate the buffer pool
  // Allocate the specified number of buffers each can hold 5 tuples.
  pool_ = new(space) sql_buffer_pool(sequenceGeneratorTdb.numBuffers_,
                                     sequenceGeneratorTdb.bufferSize_,
                                     space);

  // If we are in an ESP, then start a new transaction
  CliGlobals *cliGlobals = 0;
  cliGlobals = GetCliGlobals();
  Lng32 xnRC =0;

  ex_assert(cliGlobals->isESPProcess(), "the SG Operator is not in an ESP_Access process");

  xnRC = beginTransaction();
  
  // If the begin transaction failed,
  // cleanup and let the work method 
  // begin the transaction
  
  if (xnRC < 0)
    {
      cleanTransaction();
    }

  // get the queue used by my child to communicate with me
  //
  childQueue_  = childTcb_->getParentQueue();

  // the SequenceGenerator TCB adds input tupps to its down queues,
  // therefore we need to allocate new ATPs for its children
  //
  childQueue_.down->allocateAtps(space);

  // Allocate the queues used to communicate with parent
  qParent_.down = new(space) ex_queue(ex_queue::DOWN_QUEUE,
                                      sequenceGeneratorTdb.queueSizeDown_,
                                      sequenceGeneratorTdb.criDescDown_,
                                      space);


  qParent_.up = new(space) ex_queue(ex_queue::UP_QUEUE,
                                    sequenceGeneratorTdb.queueSizeUp_,
                                    sequenceGeneratorTdb.criDescUp_,
                                    space);

}

// Destructor for sequence generator tcb
ExSequenceGeneratorTcb::~ExSequenceGeneratorTcb()
{

  delete qParent_.up;
  delete qParent_.down;
  freeResources();
}

// Free Resources
void ExSequenceGeneratorTcb::freeResources()
{
  if (pool_)
    delete pool_;

  pool_ = 0;
  
  cleanTransaction();

}

// Cleanup transaction resources
void ExSequenceGeneratorTcb::cleanTransaction()
{
  if (currTransaction_ && currTransaction_->xnInProgress())
  {
    // Since we start a transaction during fixup time
    // for Sql table open, we can have an outstanding transaction
    // if the query gets re-compiled during a similarity check.
    // Example: create table T;
    //          prepare s1 INSERT.. 
    //          DROP T;
    //          CREATE T;
    //          execute s1;  --- this will cause deleting the query plan for s1
    //                       --- thus hitting this code. After deleting, the query
    //                       --- is recompiled.

    // Rollback the  active transaction
     currTransaction_->rollbackTransaction();
   }

  if (currTransaction_)
  {
     NADELETE(currTransaction_, ExTransaction, getHeap());
     currTransaction_ = NULL;
  }
}

// Begin transaction
short ExSequenceGeneratorTcb::beginTransaction()
{
  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExEspStmtGlobals *espGlob = exeGlob->castToExEspStmtGlobals();

  CliGlobals *cliGlobals = 0;
  cliGlobals = GetCliGlobals();
  Lng32 xnRC =0;

  // This operator must only reside in an ESP Access process.
  ex_assert((cliGlobals != NULL) && (cliGlobals->isESPProcess()), "We must be an ESP Access process");

  SB_Transid_Type txHandle;
  short retcode = TMF_GETTXHANDLE_((short *)&txHandle);
  cliGlobals->getEnvironment()->getControlConnection()->castToGuaReceiveControlConnection()->setOriginalTransaction((short *)&txHandle);

  // We are in an ESP, continue by starting a transaction
  if (!currTransaction_)
    currTransaction_ = new (getHeap()) ExTransaction (cliGlobals, getHeap());
  
  // The SG operator should not have a user transaction in progress
  if (currTransaction_->xnInProgress())
  {
     // A transaction is already in progress from
     // the constructor.  Just ensure the globals are
     // properly set.

     espGlob->getTransid() = currTransaction_->getTransid();
     return 0;
  }

  // no transaction is in progress. Start one.
  xnRC = currTransaction_->beginTransaction ();

  if (xnRC < 0)
  {
     // Exit returning the error code from the begin
     xnReturnCode_ = xnRC;
     return (short) xnRC;
  }

  //set this transaction ID in all the process globals.
  espGlob->getTransid() = currTransaction_->getTransid();

#if !defined(__EID) && defined(_DEBUG) && defined(TRACE_ESP_ACCESS)
  char msg[200];
  sprintf (msg,
	   "SG Operator: Begin Transaction - Transid = %d ", 
	   espGlob->getTransid() 
	   );
  
  espGlob->getESPTraceList()->insertNewTraceEntry(msg);

#endif

  cliGlobals->getEnvironment()->getControlConnection()->castToGuaReceiveControlConnection()->setTxHandleValid();
  return 0;
}

// Commit transaction
short ExSequenceGeneratorTcb::commitTransaction()
{
  CliGlobals *cliGlobals = 0;
  cliGlobals = GetCliGlobals();
   
  short xnRC = 0;
  short xnRCR = 0;

  ex_assert(cliGlobals != NULL, "commitTransaction: Cli globals is NULL - should have been allocated already");

  // We only handle transactions if we are in an ESP process.
  ex_assert(cliGlobals->isESPProcess(), "the SG Operator is not in an ESP_Access process");

  ex_assert(currTransaction_ != NULL, "commitTransaction: A transaction was never started");
  
  // commit the transaction
  xnRC = currTransaction_->commitTransaction ();

#if !defined(__EID) && defined(_DEBUG) && defined(TRACE_ESP_ACCESS)

  // Check the transid to see if it is -1. (commited)
  Int64 tID = currTransaction_->getTransid();

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExEspStmtGlobals *espGlob = exeGlob->castToExEspStmtGlobals();

  char msg[200];
  sprintf (msg,
	   "SG Operator: CommitTransaction Completed %d", 
	   tID
	   );
  
  espGlob->getESPTraceList()->insertNewTraceEntry(msg);

#endif
  /*
  if (xnRC < 0)
  {
     // Attempt to rollback the transaction
     xnRCR = currTransaction_->rollbackTransaction ();

     // Return the commit failure
     xnReturnCode_ = xnRC;
     return short(xnRC);
  }
  */

  addTransactionCount();

  // Reset the "current" transaction to the one sent by the requestor, if any.
  short *txHandle = cliGlobals->getEnvironment()->getControlConnection()->castToGuaReceiveControlConnection()->getOriginalTransaction();
  TMF_SETTXHANDLE_((short *)txHandle);
  cliGlobals->getEnvironment()->getControlConnection()->castToGuaReceiveControlConnection()->clearOriginalTransaction();
  cliGlobals->getEnvironment()->getControlConnection()->castToGuaReceiveControlConnection()->clearTxHandleValid();

  //  return 0;
  return xnRC;
}

// Rollback transaction
short ExSequenceGeneratorTcb::rollbackTransaction()
{
  CliGlobals *cliGlobals = 0;
  cliGlobals = GetCliGlobals();
   
  Lng32 xnRC = 0;

  ex_assert(currTransaction_ != NULL, "rollbackTransaction: A transaction was never started");

  xnRC = currTransaction_->rollbackTransaction ();

  if (xnRC < 0)
    {
       // return the error
       xnReturnCode_ = xnRC;
       return short(xnRC);
    }
  
  // Reset the "current" transaction to the one sent by the requestor, if any.
  short *txHandle = cliGlobals->getEnvironment()->getControlConnection()->castToGuaReceiveControlConnection()->getOriginalTransaction();
  TMF_SETTXHANDLE_((short *)txHandle);
  cliGlobals->getEnvironment()->getControlConnection()->castToGuaReceiveControlConnection()->clearOriginalTransaction();
  cliGlobals->getEnvironment()->getControlConnection()->castToGuaReceiveControlConnection()->clearTxHandleValid();

  return 0;
}

// Sequence generator has no children
Int32
ExSequenceGeneratorTcb::numChildren() const
{
  return(1);
}

const ex_tcb *ExSequenceGeneratorTcb::getChild(Int32 pos) const
{
  if(pos == 0) return childTcb_;
  return NULL;
}

ex_queue_pair
ExSequenceGeneratorTcb::getParentQueue() const
{
  return (qParent_);
};

short ExSequenceGeneratorTcb::work()
{

  Lng32 xnRC =0;
  short retcode = 0;

  // State Machine Loop
  while(1)
    {
      if (qParent_.down->isEmpty())
	return WORK_OK;

      // Pointer to request entry in parent down queue
      ex_queue_entry *pEntryDown = qParent_.down->getHeadEntry();
  
      switch(workState_)
        {
        case SEQGEN_NEWBLOCK:
          {
	    if (childQueue_.down->isFull()) 
	      return WORK_OK;

 	     xnRC = beginTransaction();

              // If the begin transaction failed,
	      // proceed to the error state and
	      // process the failure.

	      if (xnRC < 0)
	      {
                workState_ = SEQGEN_ERROR_IN_SG;
	        break;
	      }
              
            ex_queue_entry *childEntry = childQueue_.down->getTailEntry();
            childEntry->downState.request = ex_queue::GET_ALL;
            childEntry->downState.requestValue = 2;
            childEntry->downState.parentIndex = 21;

            childEntry->getAtp()->copyPartialAtp(pEntryDown->getAtp(), 0, 1);

            // We are sending two inputs - block size and increment.
            //
            if (pool_->get_free_tuple(childEntry->getTupp(2), 16))
              return WORK_POOL_BLOCKED;

	    UInt32 sgCacheOverride = sequenceGeneratorTdb().getSGCacheOverride();

	    // The blocksize has been set through the SEQUENCE_GENERATOR_CACHE CQD
	    if (sgCacheOverride > 0)
	      {
	        blockSize_ = sgCacheOverride;
	      }
	    else
	      {
                // Dynamically calculate the blocksize to try

                if(blockSize_ == 0) {
                  blockSize_ = sequenceGeneratorTdb().getSGCacheInitial();
                } else if(blockSize_ < sequenceGeneratorTdb().getSGCacheMaximum()) {
                  blockSize_ *= sequenceGeneratorTdb().getSGCacheIncrement();
		  if (blockSize_ <0 ) // overflows
		    blockSize_ = sequenceGeneratorTdb().getSGCacheMaximum();
		  else
		    blockSize_ = MINOF(blockSize_,
				       sequenceGeneratorTdb().getSGCacheMaximum());
		  
                } else {
                  blockSize_ = sequenceGeneratorTdb().getSGCacheMaximum();
                }
	      }

            char * dp = childEntry->getTupp(2).getDataPointer();
	    
	    	    
            str_cpy_all(dp, (char *)&blockSize_, sizeof(blockSize_));
            Int64 incr = sequenceGeneratorTdb().sgAttributes_ ->getSGIncrement();
            str_cpy_all(dp + sizeof(blockSize_),
                        (char *)&incr,
                        sizeof(incr));

            // Insert request in child queue.
            //
            childQueue_.down->insert();

            workState_ = SEQGEN_WAITFORBLOCK;
          }
          break;

        case SEQGEN_WAITFORBLOCK:
          {
            if(childQueue_.up->isEmpty()) {
              return WORK_OK;
            }

            // Must insert new block in parent up queue.
            // If Parent can't take anymore, try again later.

            if (qParent_.up->isFull())
              return WORK_OK;


            ex_queue_entry *cEntry = childQueue_.up->getHeadEntry();

            switch(cEntry->upState.status)
              {
              case ex_queue::Q_OK_MMORE:
                {
                  ex_queue_entry *pEntryUp = qParent_.up->getTailEntry();

                  // copy the atp from child (update node).
                  pEntryUp->copyAtp(cEntry);

                  unsigned short tuppIdx = sequenceGeneratorTdb().getDstTuppIndex();

                  // We are sending the results from the child.
                  // Plus the sgOutputTuple - which has only blockSize

                  if (pool_->get_free_tuple(pEntryUp->getTupp(tuppIdx), sizeof(blockSize_)))
                  {
                    pEntryUp->getAtp()->release();
                    return WORK_POOL_BLOCKED;
                  }

                  // Add to the up entry a tupp containing blockSize_
                  str_cpy_all(pEntryUp->getTupp(tuppIdx).getDataPointer(),
                              (char *)&blockSize_,
                              sizeof(blockSize_));

                  // Finalize the queue entry, then insert it
                  pEntryUp->upState.status = ex_queue::Q_OK_MMORE;
                  pEntryUp->upState.parentIndex = pEntryDown->downState.parentIndex;
                  pEntryUp->upState.downIndex = qParent_.down->getHeadIndex();
                  pEntryUp->upState.setMatchNo(0);

                  qParent_.up->insert();

                  // Commit the transaction. 
                  short  xnRC = commitTransaction();

                  if (xnRC < 0)
                    {
                      // Process this as an error.
                      // The update to the table will
                      // have ultimately failed
                      workState_ = SEQGEN_ERROR_IN_SG;
                      break;
                    }

                  workState_ = SEQGEN_DONE;

                  // remove Q_OK_MMORE
                  childQueue_.up->removeHead();
                  break;
                }
              case ex_queue::Q_SQLERROR:
		{
#if !defined(__EID) && defined(_DEBUG) && defined(TRACE_ESP_ACCESS)
		  BreakIntoDebugger("ExSequenceGenerator = Update Returned An Error",
				    "Esp_Access_Operator");
#endif
                  short xnRRC = rollbackTransaction();
                  // If the rollback transaction failed,
                  // proceed to the error state and
                  // process the failure.
	    
                  if (xnRRC < 0)
                    {
                      workState_ = SEQGEN_ERROR_IN_SG;
                      break;
                    }
                  // Continue and process the error
                  workState_ = SEQGEN_ERROR_FROM_CHILD;

		  break;
		}

              case ex_queue::Q_NO_DATA:
                ex_assert(FALSE, "SEQGEN_WAITFORBLOCK - unexpected Q_NO_DATA");
                break;

              default:
                ex_assert(FALSE, "SEQGEN_WAITFORBLOCK - unexpected State");
                break;

              } // end of switch

          }
           
          break;

        case SEQGEN_SEND_Q_NO_DATA:
	  {
            // Must insert NO_DATA in parent up queue.
            // If Parent can't take anymore, try again later.
	    
            if (qParent_.up->isFull())
              return WORK_OK;

            ex_queue_entry *pEntryUp = qParent_.up->getTailEntry();

            pEntryUp->upState.status = ex_queue::Q_NO_DATA;
            pEntryUp->upState.parentIndex = pEntryDown->downState.parentIndex;
            pEntryUp->upState.downIndex = qParent_.down->getHeadIndex();
            pEntryUp->upState.setMatchNo(0);

 	    qParent_.up->insert();

            // this parent request has been processed.
            qParent_.down->removeHead();
            pEntryDown = NULL;

            // Wait for another request
            workState_ = SEQGEN_NEWBLOCK;

	    
	  }
	  break;

        case SEQGEN_DONE:
	  {

            // Must remove NO_DATA from child up queue.
            // If child is empty, try again later.

            if(childQueue_.up->isEmpty()) {
              return WORK_OK;
            }

            // Must insert NO_DATA in parent up queue.
            // If Parent can't take anymore, try again later.

            if (qParent_.up->isFull())
              return WORK_OK;

            ex_queue_entry *cEntry = childQueue_.up->getHeadEntry();
	    
            switch(cEntry->upState.status)
              {
               case ex_queue::Q_SQLERROR:
                {
                  // We have an error being returned from the child.
                  // Process the error.
                  workState_ = SEQGEN_ERROR_FROM_CHILD;
                  break;
                }

               case ex_queue::Q_NO_DATA:
                {
                  // Q_NO_DATA
                  childQueue_.up->removeHead();

                  ex_queue_entry *pEntryUp = qParent_.up->getTailEntry();

                  pEntryUp->upState.status = ex_queue::Q_NO_DATA;
                  pEntryUp->upState.parentIndex = pEntryDown->downState.parentIndex;
                  pEntryUp->upState.downIndex = qParent_.down->getHeadIndex();
                  pEntryUp->upState.setMatchNo(0);

                  qParent_.up->insert();

                   // this parent request has been processed.
                  qParent_.down->removeHead();
                  pEntryDown = NULL;

                  // Wait for another request
                  workState_ = SEQGEN_NEWBLOCK;

#if !defined(__EID) && defined(_DEBUG) && defined(TRACE_ESP_ACCESS)
  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExEspStmtGlobals *espGlob = exeGlob->castToExEspStmtGlobals();

  espGlob->getESPTraceList()->logESPTraceToFile
    ("c:\\temp\\sglog.txt", // file name 
     "SG", // signature 
     *espGlob->getESPTraceList()//  ESPTraceList 
     );
#endif
                break;  
                }

                default:
                  ex_assert(FALSE, "SEQGEN_DONE - unexpected State");
                  break;

              } // end of switch
            }
            break;  // end of SEQ_DONE
           
      
	case SEQGEN_ERROR_IN_SG:
	  {
            if (qParent_.up->isFull())
              return WORK_OK;

	    ex_queue_entry *pEntryUp = qParent_.up->getTailEntry();
	    ex_queue_entry *pEntryDown = qParent_.down->getHeadEntry();

	    ComDiagsArea *sgDiags = pEntryUp->getAtp()->getDiagsArea();
	    if (sgDiags == NULL)
	      {
		// allocate a new diags area.
		sgDiags = ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
		pEntryUp->setDiagsArea(sgDiags);
	      }

	    // Get the error from the transaction. 
	    ComDiagsArea *txnDiags = currTransaction_->getDiagsArea();
	    if (txnDiags)
	      {
		sgDiags->mergeAfter(*txnDiags);
	      }
	    // Indicate that the failure is from the Update to the SG Table.
	    *sgDiags << DgSqlCode(- EXE_SG_UPDATE_FAILURE);
	           
	    pEntryUp->upState.status = ex_queue::Q_SQLERROR;
	    pEntryUp->upState.parentIndex = pEntryDown->downState.parentIndex;
	    pEntryUp->upState.downIndex = qParent_.down->getHeadIndex();
	    pEntryUp->upState.setMatchNo(0);
	    
	    qParent_.up->insert();

	    // cleanup any pending transactions.
	    cleanTransaction();

	    workState_ = SEQGEN_SEND_Q_NO_DATA;
	  }
	  break;

	case SEQGEN_ERROR_FROM_CHILD:
	  {
            ex_assert(NOT childQueue_.up->isEmpty(), 
		      "SEQGEN_ERROR_FROM_CHILD: Must have a child entry");
 
            ex_queue_entry *cEntry = childQueue_.up->getHeadEntry();

	    ex_assert(cEntry->upState.status == ex_queue::Q_SQLERROR, 
		      "SEQGEN_ERROR_FROM_CHILD: Child entry must be a Q_SQLERROR");

	    ex_queue_entry *pEntryUp = qParent_.up->getTailEntry();
	    
	    // copy the atp from child (update node).
	    pEntryUp->copyAtp(cEntry);
	    
	    // Set the diagnostics area in the up queue entry.
	    ComDiagsArea *updateDiags = pEntryUp->getDiagsArea();
	    Lng32 updateMainSQLCODE = 0;
	
	    ex_assert(updateDiags != NULL, "SEQGEN_ERROR_FROM_CHILD: Must have a diags entry");

	    updateMainSQLCODE = updateDiags->mainSQLCODE();

	    // If the returned diagnostic sqlcode is NUMERIC
	    // OVERFLOW, then we may be able to retry with a
	    // smaller blocksize.
	    if (updateMainSQLCODE == -EXE_NUMERIC_OVERFLOW) 
	      {
		    updateDiags->clear();

		    // If the blockSize_ is greater than the initial cache size, then
		    // there may be a few numbers left before we finally
		    // overflow.  Clear the child up queue, reset
		    // the state to SEQGEN_NEWBLOCK and try again.
		    // Otherwise, we have reached the last
		    // value possible to add for the LARGEINT.
		    // Post the MAXVALUE exceeded error.

		    if (blockSize_ > sequenceGeneratorTdb().getSGCacheInitial())
		      {
			blockSize_ = 0;
			
			childQueue_.up->removeHead();			

			// clean up before redriving.
			workState_ = SEQGEN_CLEANUP_AND_RETRY;
		      }
		    else // else it's a real overflow.
		      {
			// If the returned diagnostic sqlcode is NUMERIC OVERFLOW,
			// then the maximum for the sequence generator current value
			// of a LARGEINT has been reached.  Clear the -8411 error and post
			// the sequence generator MAXVALUE exceeded -8934 error.
			
			*updateDiags << DgSqlCode(- EXE_SG_MAXVALUE_EXCEEDED); 

			pEntryUp->upState.status = ex_queue::Q_SQLERROR;
			
			pEntryUp->upState.parentIndex = pEntryDown->downState.parentIndex;
			pEntryUp->upState.downIndex = qParent_.down->getHeadIndex();
			pEntryUp->upState.setMatchNo(0);
			
			qParent_.up->insert();
			
			childQueue_.up->removeHead();

			workState_ = SEQGEN_DONE;
		      }
	      }
	    else
	      {
	        // The error from the child is not a numeric overflow.
	        // Send the error up to the parent.
	        // The transaction has already been rolled back.

	        // Add a general SG error indicating
		// the update failed.  The diagnostics from
		// the update are retained to provide additional
		// information on the true failure.
	        *updateDiags << DgSqlCode(- EXE_SG_UPDATE_FAILURE);

	        pEntryUp->upState.status = ex_queue::Q_SQLERROR;
			
	        pEntryUp->upState.parentIndex = pEntryDown->downState.parentIndex;
	        pEntryUp->upState.downIndex = qParent_.down->getHeadIndex();
	        pEntryUp->upState.setMatchNo(0);
			
	        qParent_.up->insert();
			
	        childQueue_.up->removeHead();

	        workState_ = SEQGEN_DONE;
	      }
 
	  }
	  break;

      
	case SEQGEN_CLEANUP_AND_RETRY:
	  {
	    // We expect only Q_NODATA here.
	    if(childQueue_.up->isEmpty()) {
              return WORK_OK;
	    }

            ex_queue_entry *cEntry = childQueue_.up->getHeadEntry();
	    
	    ex_assert(cEntry->upState.status == ex_queue::Q_NO_DATA,
		      "SEQGEN_CLEANUP_AND_RETRY: Child entry must be a Q_NO_DATA");

	    childQueue_.up->removeHead();

	    // Try again
	    workState_ = SEQGEN_NEWBLOCK;
	  }
	  break;


	  /*
	case SEQGEN_ERROR:
          {
            if(childQueue_.up->isEmpty()) {
              return WORK_OK;
            }
            
            // Must insert new block in parent up queue.
            // If Parent can't take anymore, try again later.

            if (qParent_.up->isFull())
              return WORK_OK;

            ex_queue_entry *cEntry = childQueue_.up->getHeadEntry();

            switch(cEntry->upState.status)
              {

              case ex_queue::Q_NO_DATA:
              case ex_queue::Q_SQLERROR:
		{
               
                  ex_queue_entry *pEntryUp = qParent_.up->getTailEntry();

                  // copy the atp from child (update node).
                  pEntryUp->copyAtp(cEntry);

                  // Set the diagnostics area in the up queue entry.
                  ComDiagsArea *updateDiags = pEntryUp->getDiagsArea();

		  long updateMainSQLCODE = 0;
	
		  if (updateDiags)
		    updateMainSQLCODE = updateDiags->mainSQLCODE();

 		  // If the returned diagnostic sqlcode is NUMERIC
		  // OVERFLOW, then we may be able to retry with a
		  // smaller blocksize.

		  // If the returned diagnostic sqlcode is FS error 73
		  // (FELOCKED), then a table timeout occurred for the
		  // SG Update.  TDB: We need to fix this either by a
		  // setting a longer timeout to begin with or retry
		  // automatically with a longer timeout.
		  
		  if (updateDiags && (updateMainSQLCODE == -EXE_NUMERIC_OVERFLOW))
		  {
		    updateDiags->clear();

		    // If the blockSize_ is greater than the initial cache size, then
		    // there may be a few numbers left before we finally
		    // overflow.  Clear the child up queue, reset
		    // the state to SEQGEN_NEWBLOCK and try again.
		    // Otherwise, we have reached the last
		    // value possible to add for the LARGEINT.
		    // Post the MAXVALUE exceeded error.

		    if ((updateMainSQLCODE == -EXE_NUMERIC_OVERFLOW &&
		         blockSize_ > sequenceGeneratorTdb().getSGCacheInitial()))
		    {
		      if (updateMainSQLCODE == -EXE_NUMERIC_OVERFLOW)
		        blockSize_ = 0;

		      workState_ = SEQGEN_NEWBLOCK;

		      // We are retrying.
		      // Leave the current transaction active
		      // and don't cancel.  Do remove update entries
		      // for the redrive.

                      while(!childQueue_.up->isEmpty()) {
                        childQueue_.up->removeHead();
		      }

		      short xnRRC = rollbackTransaction();
		      
		      // If the rollback transaction failed,
		      // proceed to the error state and
		      // process the failure.
		      
		      if (xnRRC < 0)
			
			{
			  workState_ = SEQGEN_ERROR;
			  break;
			}
		      
 	              // Try again
		      break;
        	    }
		  }

		  if (updateDiags && updateMainSQLCODE < 0)
		  {
                      // If the returned diagnostic sqlcode is NUMERIC OVERFLOW,
		      // then the maximum for the sequence generator current value
		      // of a LARGEINT has been reached.  Clear the -8411 error and post
		      // the sequence generator MAXVALUE exceeded -8934 error.

		      if (updateMainSQLCODE == -EXE_NUMERIC_OVERFLOW)
		        *updateDiags << DgSqlCode(- EXE_SG_MAXVALUE_EXCEEDED); 
		      else
		      {
		 
                        // This was not an overflow error.

                        // If this was a transaction failure,
		        // Add the failure first.

		        if (xnReturnCode_ < 0)
		        {
                          *updateDiags << DgSqlCode(xnReturnCode_)
			                << DgString0("Accesing the Sequence generator for IDENTITY column failed");
			  
		          // Clean up error code
                          xnReturnCode_ = 0;
		        }

                        // Add a general SG error indicating
		        // the update failed.  The diagnostics from
		        // the update are retained to provide additional
		        // information on the true failure.
		        *updateDiags << DgSqlCode(- EXE_SG_UPDATE_FAILURE);
		    
 		      }
		  }

                  // Rollback transaction and clean up its resources.
		  
		  short xnRC = rollbackTransaction();

		  cleanTransaction();

		  // If we had a problem rolling back the transaction,
		  // add the diagnostics

		  if (xnRC < 0 && updateDiags)
		  {
		    *updateDiags << DgSqlCode(xnRC)
		                  << DgString0("Sequence generator for IDENTITY column");
		  }

                  // Finalize the queue entry, then insert it
		  // The send bottom node expects the status
		  // to be Q_NO_DATA.
		  // The NVF node expects the status to
		  // be Q_SQLERROR.

                  if (cliGlobals->isESPProcess())
                    pEntryUp->upState.status = ex_queue::Q_NO_DATA;  
		  else
                    pEntryUp->upState.status = ex_queue::Q_SQLERROR;
                  
		  pEntryUp->upState.parentIndex = pEntryDown->downState.parentIndex;
                  pEntryUp->upState.downIndex = qParent_.down->getHeadIndex();
                  pEntryUp->upState.setMatchNo(0);

                  qParent_.up->insert();

                  childQueue_.up->removeHead();
	
	          // remove update entries
 
                  while(!childQueue_.up->isEmpty()) {
                    childQueue_.up->removeHead();
		  }
  	 
                  // this parent request has been processed.
		  qParent_.down->removeHead();
                  pEntryDown = NULL;

		 // Wait for another request
                  workState_ = SEQGEN_NEWBLOCK;

                  // If no more work to be done, return.
                  if (qParent_.down->isEmpty())
                    return WORK_OK;

                  pEntryDown = qParent_.down->getHeadEntry();

		  break;
		  }

              case ex_queue::Q_OK_MMORE:
                ex_assert(FALSE, "SEQGEN_ERROR - unexpected Q_OK_MMORE");
                break;

              default:
                ex_assert(FALSE, "SEQGEN_ERROR - unexpected State");
                break;

              } // end of switch

          }
          
          break;
	  */
          
          default:
            ex_assert(FALSE, "ExSequenceGeneratorTcb::work:: undefined workstate_.");
            break;

          }  // switch(workState_)
    }
}




/******************* ExNextValueFor methods  *******************/

//
// Build a next value for tcb from a next value for TDB.
// Allocates a new ExNextValueForTcb.
// Adds this tcb to the schedulers task list.
ex_tcb *
ExNextValueForTdb::build(ex_globals * glob)
{

  // Build the Tcb tree below the next value for node.
  //
  ex_tcb *leftChildTcb = leftChildTdb_->build(glob);
  ex_tcb *rightChildTcb = rightChildTdb_ ? rightChildTdb_->build(glob) : NULL;

  // Allocate and initialize a new next value for TCB.
  ExNextValueForTcb *nvf_tcb =
    new(glob->getSpace()) ExNextValueForTcb(*this, leftChildTcb, rightChildTcb, glob);


  // add the NextValueFor tcb to the scheduler's task list.
  nvf_tcb->registerSubtasks();

  // enable this operator to use dynamic queue resizing.
  nvf_tcb->registerResizeSubtasks();

  return nvf_tcb;
}

//
//  TCB procedures
//

//
// Constructor for ExNextValueForTcb.  Called by the build
// function of the ExNextValueForTdb.  This will initialize the
// internal state of the next value for Tcb and allocate the
// queues used to communicate with the parent.

ExNextValueForTcb::ExNextValueForTcb
(const ExNextValueForTdb &nvf_tdb,
 const ex_tcb *leftChildTcb,
 const ex_tcb *rightChildTcb,
 ex_globals *glob)
  : ex_tcb(nvf_tdb, 1, glob),
    leftChildTcb_(leftChildTcb),
    rightChildTcb_(rightChildTcb),
    leftDone_(FALSE),
    rightDone_(TRUE),
    maxValueWillExceed_(FALSE)
{

  Space * space = (glob ? glob->getSpace() : 0);
  CollHeap * heap = (glob ? glob->getDefaultHeap() : 0);

  // Allocate the buffer pool
  // Allocate the specified number of buffers each can hold 5 tuples.
  pool_ = new(space) sql_buffer_pool(nvf_tdb.numBuffers_,
                                     nvf_tdb.bufferSize_,
                                     space);

  // get the queue used by my child to communicate with me
  //
  qLeft_  = leftChildTcb_->getParentQueue();
  if(rightChildTcb_)
    qRight_ = rightChildTcb_->getParentQueue();

  // Allocate the queues to communicate with parent
  allocateParentQueues(qParent_,
                       TRUE // allocate the pstate
                       );

  if (nvf_tdb.moveSGOutputExpr_)
    (void) nvf_tdb.moveSGOutputExpr_->fixup
      (0,
       getExpressionMode(),
       this,
       glob->getSpace(),
       glob->getDefaultHeap()
       );

  // Allocate the work ATP.
  workAtp_ = allocateAtp(nvf_tdb.workCriDesc_, space);

  // initialize tupp descriptor for the update values
  valuesFromUpdateTupp_.init(sizeof(valuesFromUpdate_),
                             NULL,
                             (char *) (&valuesFromUpdate_));

  // assign the values from update tupp
  // atp index 0 - constants
  //           1 for temps
  //           2 for the sgOutputs
  workAtp_->getTupp(2) = &valuesFromUpdateTupp_;

  numInstances_ = glob->getNumOfInstances();
  instanceNum_ = glob->getMyInstanceNumber();
  pid_ = glob->getPid();

  if(numInstances_ == 0) {
    numInstances_ = 1;
    instanceNum_ = 0;
  }

  valuesFromUpdate_[NVF_CURRENT_VALUE] = 0;
  valuesFromUpdate_[NVF_NUM_VALUES] = 1;
  needNextN_ = TRUE;
  counterN_ = 0;
}


// Destructor for next value for tcb
ExNextValueForTcb::~ExNextValueForTcb()
{
  freeResources();
  if (workAtp_)
  {
    workAtp_->release();
    deallocateAtp(workAtp_, getSpace());
  }
  // reset local varaibles


}

// Free Resources
void ExNextValueForTcb::freeResources()
{
  if (pool_)
    {
    delete pool_;
    pool_ = 0;
    }

 if (qParent_.up)
   {
     delete qParent_.up;
     qParent_.up = NULL;
   }

 if (qParent_.down)
   {
     delete qParent_.down;
     qParent_.down = NULL;
   }
}

// Next Value For has 2 children
Int32
ExNextValueForTcb::numChildren() const
{
  if(rightChildTcb_)
    return 2;
  else
    return 1;
}

const ex_tcb *ExNextValueForTcb::getChild(Int32 pos) const
{
  if(pos == 0)
    return leftChildTcb_;

  if(pos == 1)
    return rightChildTcb_;

  return NULL;
}

ex_queue_pair
ExNextValueForTcb::getParentQueue() const
{
  return (qParent_);
};

ex_tcb_private_state * ExNextValueForTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExNextValueForPrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}


short ExNextValueForTcb::work()
{
  while (1) {

    if (qParent_.down->isEmpty())
      return WORK_OK;
    
    ex_queue_entry * pEntryDown = qParent_.down->getHeadEntry();
    ExNextValueForPrivateState &  pstate =
      *((ExNextValueForPrivateState*) pEntryDown->pstate);
    
    ex_queue::down_request request = pEntryDown->downState.request;
    
    // Cancel, if the parent cancelled or if we have completed the request (returned the required N rows).
    if ((pstate.step_ != NVF_CANCEL) &&
        (pstate.step_ != NVF_DONE) &&
        ((request == ex_queue::GET_NOMORE) ||
         ((request == ex_queue::GET_N) &&
          (pEntryDown->downState.requestValue <= (Lng32)pstate.matchCount_)))
        )
      {
        qLeft_.down->cancelRequestWithParentIndex(qParent_.down->getHeadIndex());
        if(rightChildTcb_)
	{
          qRight_.down->
            cancelRequestWithParentIndex(qParent_.down->getHeadIndex());
	}

        pstate.step_ = NVF_CANCEL;
      }

    switch (pstate.step_)
      {
      case NVF_START:
        {
          // send the down request to the left child to
          // get the row for which the next value is generated.
          if (qLeft_.down->isFull() || qParent_.down->isEmpty())
            return WORK_OK;

          ex_queue_entry * left_entry = qLeft_.down->getTailEntry();

          left_entry->downState.request = pEntryDown->downState.request;
          left_entry->downState.requestValue =
            pEntryDown->downState.requestValue;

          left_entry->downState.parentIndex =
            qParent_.down->getHeadIndex();

          left_entry->passAtp(pEntryDown);

          qLeft_.down->insert();
          pstate.matchCount_ = 0;
          pstate.step_ = NVF_SEND_RESULT_ROWS_UP;

          // Reset for any needed queue cleanup
	  leftDone_ = FALSE;

	  maxValueWillExceed_ = FALSE;
        }
        break;

      case NVF_SENT_REQUEST_TO_SG:
        {
          // Send a request to the right child(SequenceGenerator)
          // to obtain (1) current Value
          //           (2) N - denoting N values
 
          if (qRight_.down->isFull())
            return WORK_OK;

          ex_queue_entry * right_entry = qRight_.down->getTailEntry();

          right_entry->downState.request = ex_queue::GET_ALL;
          right_entry->downState.requestValue =
            pEntryDown->downState.requestValue;

          right_entry->downState.parentIndex =
            qParent_.down->getHeadIndex();

          right_entry->passAtp(pEntryDown);

          qRight_.down->insert();

	  // We have just sent a request to the right child.
	  // So, we are certainly not done.
	  rightDone_ = FALSE;

          pstate.step_ = NVF_GET_RESULT_FROM_SG;

          break;
        }

      case NVF_GET_RESULT_FROM_SG:
        {
          if (qRight_.up->isEmpty())
            return WORK_OK;
          workRefreshFromSGResult(pstate);
          break;
        }

      case NVF_SG_REFRESH_DONE:
        {
          if (qRight_.up->isEmpty())
            return WORK_OK;

          // We always expect Q_NO_DATA now.
	  // Assert if we did not receive one.
          if (qRight_.up->getHeadEntry()->upState.status != ex_queue::Q_NO_DATA) {
            ex_assert(FALSE, "NVF_GET_REFRESH_DONE - unexpected State");
          }
            
          pstate.step_ = NVF_SEND_RESULT_ROWS_UP;

          // consume the right up queue Q_NO_DATA entry
          qRight_.up->removeHead() ;

          rightDone_ = TRUE;
          break;
        }

      case NVF_SEND_RESULT_ROWS_UP:
        {
          if (qParent_.up->isFull() || qLeft_.up->isEmpty())
            return WORK_OK;

          short rc = workSendResultsToParent(pstate);

          if (rc != WORK_OK)
            return rc;

          break;
        }

      case NVF_DONE:
        {
          // No more SRC (left child) rows to process.

          if (qParent_.up->isFull())
            return WORK_OK;

          ex_queue_entry * pentry_up = qParent_.up->getTailEntry();

          pentry_up->upState.status = ex_queue::Q_NO_DATA;
          pentry_up->upState.downIndex = qParent_.down->getHeadIndex();
          pentry_up->upState.parentIndex = pEntryDown->downState.parentIndex;
          pentry_up->upState.setMatchNo(pstate.matchCount_);

          qParent_.up->insert();

          // this parent request has been processed.
          qParent_.down->removeHead();

          // Wait for another request
          pstate.step_ = NVF_START;

          break;
        }

      case NVF_ERROR:
        {

          if (qParent_.up->isFull())
            return WORK_OK;

          ex_queue_entry *pEntryUp = qParent_.up->getTailEntry();
          pEntryUp->upState.status = ex_queue::Q_SQLERROR;
          pEntryUp->upState.downIndex = qParent_.down->getHeadIndex();
          pEntryUp->upState.parentIndex = pEntryDown->downState.parentIndex;
          pEntryUp->upState.setMatchNo(pstate.matchCount_);

          qParent_.up->insert();

          pstate.step_ = NVF_CANCEL;

          break;
        }

      case NVF_CANCEL:
        {

          ex_queue_entry *pEntryDown = qParent_.down->getHeadEntry();
          // Cancel the requests to the left child.
          qLeft_.down->cancelRequestWithParentIndex(qParent_.down->getHeadIndex());

          // do we really need to send cancel to the right (Sequence generator) child?
          // We only expect one reply from it anyway. 
	  if(rightChildTcb_ && !rightDone_)
            qRight_.down->
              cancelRequestWithParentIndex(qParent_.down->getHeadIndex());

          short rc = processCancel(pstate);
	  if (rc != WORK_OK)
	    return WORK_OK;
          break;
        }

      default:
         ex_assert(FALSE, "NVF work - unexpected State");
         break;
      }

  } // While (1)
  return WORK_OK;
}

// This step does the following
// 1. Get the row from the SRC and send to the parent.
// 2. Compute the Next Value - currentBaseValue = currentBaseValue + increment;
// 4. send the result to the parent

short ExNextValueForTcb::workSendResultsToParent(ExNextValueForPrivateState & pstate)
{
  ex_queue_entry * pentry_down = qParent_.down->getHeadEntry();

  //Int64 fsDataType = (ComFSDataType)nextValueForTdb().nvSGAttributes_->getSGDataType();
  Int64 maxValue = nextValueForTdb().nvSGAttributes_ ->getSGMaxValue();
  Int64 increment = nextValueForTdb().nvSGAttributes_ ->getSGIncrement();

  if(!rightChildTcb_)
    increment = numInstances_;

  unsigned short tuppIdx = nextValueForTdb().nextValueReturnTuppIndex_;

  NABoolean maxValueWillExceed = FALSE;

  ex_queue_entry * lentry_up = NULL;
  ex_queue_entry * pentry_up = NULL;

  Int64 nvfCurrentValue = valuesFromUpdate_[NVF_CURRENT_VALUE];
  Int64 nvfNumValues = valuesFromUpdate_[NVF_NUM_VALUES];

  while (!qParent_.up->isFull() && !qLeft_.up->isEmpty())
    {

      lentry_up = qLeft_.up->getHeadEntry();
      pentry_up = qParent_.up->getTailEntry();

      // allocate the return tupp
      //tupp nextValueTupp;

      // 1. Get the row from the SRC (left child) and send to the parent.

      // If this is not a Q_OK_MMORE, then break out of loop and handle
      if(lentry_up->upState.status != ex_queue::Q_OK_MMORE) 
        break;
      
      // Do this step till we have exhausted N_.
      if (needNextN_)
        break;

      // 2. Compute the Next Value - nextValueFor()->eval()
      // We use the currentBaseValue from the SG_TABLE as the
      // first value.

      // Before continuing, see if this value will
      // exceed the maximum for the SG data type.
      // If it will, then exit with the maxvalue
      // exceeded error.
	    
      // The MAXVALUE can be driven by the maximum of
      // the datatype, or a pre-configured maximum set
      // through the create table statement.
 
      if (nvfCurrentValue > maxValue ||
          nvfCurrentValue < 0)  //is negative. 
        {
          // if the precalculated value exceeds the maxValue or
          // if the value is negative, an
          // overflow has occured for LARGEINT datatype.
          maxValueWillExceed = TRUE;
          break;
        }

      pentry_up->copyAtp(lentry_up);

      if (pool_->get_free_tuple(pentry_up->getTupp(tuppIdx),
                                sizeof(nvfCurrentValue)))
        {
          valuesFromUpdate_[NVF_CURRENT_VALUE] = nvfCurrentValue;
          pentry_up->getAtp()->release();
          return WORK_POOL_BLOCKED;
        }

      str_cpy_all(pentry_up->getTupp(tuppIdx).getDataPointer(),
                  (char *)&nvfCurrentValue,
                  sizeof(nvfCurrentValue));

      pentry_up->upState.status = ex_queue::Q_OK_MMORE;
      pentry_up->upState.parentIndex = pentry_down->downState.parentIndex;
      pstate.matchCount_++;
      pentry_up->upState.setMatchNo(pstate.matchCount_);

      // 3. send the result to the parent
      // insert into parent up queue
      qParent_.up->insert();

      qLeft_.up->removeHead();

      // Generate the next value
      nvfCurrentValue += increment;
	    
      counterN_ = counterN_ + 1;
      if(counterN_ >= nvfNumValues)
        needNextN_ = TRUE;

    } // while (1)

  valuesFromUpdate_[NVF_CURRENT_VALUE] = nvfCurrentValue;

  if (qParent_.up->isFull() || qLeft_.up->isEmpty())
    return WORK_OK;

  if(lentry_up->upState.status != ex_queue::Q_OK_MMORE) 
    {
      switch(lentry_up->upState.status)
        {

        case ex_queue::Q_NO_DATA:
          {
            qLeft_.up->removeHead();
            pstate.step_ = NVF_DONE;
	    leftDone_ = TRUE;
            return WORK_OK;
            break;
          }

        case ex_queue::Q_SQLERROR:
          {
            // left child returned an error.
            short rc = processDiagsArea(lentry_up->getAtp(), pstate);
            return rc;
            break;
          }

        default:
          ex_assert(FALSE, "NVF workSendResultsToParent - unexpected State");
          break;
        }
    }

  // Do this step till we have exhausted N_.
  if (needNextN_)
    {
      if(rightChildTcb_) {
        // time to refresh from SG.
        // time to send a request to SG to get
        // another batch of N_

        pstate.step_ = NVF_SENT_REQUEST_TO_SG;
        // reset counter
        counterN_ = 0;
        // We have requested next N values
        // so let's reset the needNextN_ flag.
        needNextN_ = FALSE;
        return WORK_OK;
      } else {
        // there is no right child. This is the IDENTITY INTERNAL case
        // The right side is done.
        rightDone_ = TRUE; 

        Int64 ts = CONVERTTIMESTAMP(JULIANTIMESTAMP(0,0,0,-1),0,-1,0);

		  // Start with a base timestamp representing Jan. 1, 2008
                  // (i.e. on Jan. 1, 2008, ts would be zero.)
                  ts = ts - 212065905600000000LL; // 2008-01-01
		  
        // Shift the base value right by 8 bits making it have a 
        // resolution of approximately 1/4000 secs.
        ts = (ts >> 8);
		  
        // Each instance generates its own sequence on values.
        // The sequences from all the instances are interleaved.
        // For example if there are 4 instances,
        //  instance 0 might generate values 0, 4, 8, 12, ...
        //  instance 1 might generate values 1, 5, 9, 13, ...
        //  instance 2 might generate values 2, 6, 10, 14, ...
        //  instance 1 might generate values 3, 7, 11, 15, ...
		  
        // Also, add in 8 bits of the PID in the upper 8
        // bits of the value.  to further distinguish
        // instances and distinguish different inserters.
        //
        Int64 newBaseValue = (ts / numInstances_);
        newBaseValue = ((newBaseValue * numInstances_) 
                        + instanceNum_
                        + (((Int64)(pid_ + 1)) << 48)) ;
		  
        // If the newBaseValue represents a new region of the value space
        // start using the new value.
        // Otherwise, if the current value has gone beyond the new value
        // stick with the current value.  Do not want to back up and risk
        // generating duplicates.
        if(newBaseValue > valuesFromUpdate_[NVF_CURRENT_VALUE])
          valuesFromUpdate_[NVF_CURRENT_VALUE] = newBaseValue;
		  
        // Come back again after 10000 inserts to get a new base value.
        valuesFromUpdate_[NVF_NUM_VALUES] = 10000;
		  
        // reset counter
        counterN_ = 0;
		  
        needNextN_ = FALSE;
        
        return WORK_OK;
		  
      }
    }

  if (maxValueWillExceed)
    {
      ComDiagsArea *diags_my = pentry_up->getDiagsArea();
		
      if (!diags_my)
        {
          diags_my = ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
          pentry_up->setDiagsArea(diags_my);
        }
		
      *diags_my << DgSqlCode(- EXE_SG_MAXVALUE_EXCEEDED);
      pstate.step_ = NVF_ERROR;
      return WORK_OK;
    }


  return WORK_OK;
}

// copies (1) N
// (2) the current base value from SG
// into the workAtp_
// populate the respective local variables
// with values from the tuple.

void ExNextValueForTcb::workRefreshFromSGResult(ExNextValueForPrivateState &pstate)
{
  ex_queue_entry *pRightEntryUp = qRight_.up->getHeadEntry();
  switch(pRightEntryUp->upState.status )
    {
    case ex_queue::Q_OK_MMORE:
      {
        ex_expr::exp_return_type retCode;

	// There must be a valid SG output move expression
        ex_assert(nextValueForTdb().moveSGOutputExpr(),
	          "workRefreshFromSGResult - moveSGOutputExpr not found");

        // atpIndex is set to 1 in the codeGen()

        retCode = nextValueForTdb().moveSGOutputExpr()->eval
          (pRightEntryUp->getAtp(), workAtp_);

	// processDiagsArea sets all necessary states

        if (retCode == ex_expr::EXPR_ERROR)
          {
            short rc = processDiagsArea(pRightEntryUp->getAtp(),pstate);
            return;
          }
        
	// We've gotten out outputs from the SG, so let's clean up
        // the right up queue.
        pstate.step_ = NVF_SG_REFRESH_DONE;

        // consume the right up queue Q_OK_MMORE entry
        qRight_.up->removeHead() ;

        break;
      }

    case ex_queue::Q_NO_DATA:
      {
        // At this point we don't expect a Q_NO_DATA
        // we expect Q_OK_MMORE.
        // Follow through to error.
        ex_assert(FALSE, "workRefreshFromSGResult - unexpected Q_NO_DATA");

      }
    case ex_queue::Q_SQLERROR:
      {
        // Right Child (the SG Operator) returned an error.
        short rc = processDiagsArea(pRightEntryUp->getAtp(),pstate);
        return;

        break;
      }

     default:
       ex_assert(FALSE, "workRefreshFromSGResult - unexpected State");
       break;
 
    } // pRightEntryUp->upState.status

  return;
}

short ExNextValueForTcb::processDiagsArea(atp_struct* atp,
                                          ExNextValueForPrivateState &pstate)
{
   ex_queue_entry *pEntryUp = qParent_.up->getTailEntry();
   ex_queue_entry *pEntryDown = qParent_.down->getHeadEntry();

   if (atp)
     {
       pEntryUp->copyAtp(atp);
     }

   // Set the diagnostics area in the up queue entry.
   ComDiagsArea *upEntryDiags = pEntryUp->getDiagsArea();
   ComDiagsArea *prevDiags = pEntryDown->getAtp()->getDiagsArea();
   if (!upEntryDiags)
     {
       upEntryDiags =
         ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
       pEntryUp->setDiagsArea(upEntryDiags);
     }
   if (prevDiags)
     upEntryDiags->mergeAfter(*prevDiags);

  pstate.step_ = NVF_ERROR;

  return WORK_OK;
}

short ExNextValueForTcb::processCancel(ExNextValueForPrivateState & pstate)
{
  // Ensure Q_NO_DATA is processed for the left queue,
  // and right queue if necessary.

  if (rightDone_ && leftDone_)
    {
      pstate.step_ = NVF_DONE;
      return WORK_OK;
    }

  // There is work to be done on at least one child.

  // Let's do the qRight_ first.
  NABoolean done = FALSE;
  while (NOT done)
  {

    if((!rightChildTcb_ || qRight_.up->isEmpty()))
    {
      done = TRUE;
     }
    else
      {
        ex_queue_entry * rentry = qRight_.up->getHeadEntry();

        switch(rentry->upState.status)
          {
          case ex_queue::Q_OK_MMORE:
          case ex_queue::Q_SQLERROR:
            {
              // just consume the child row
              qRight_.up->removeHead();
            }
            break;

          case ex_queue::Q_NO_DATA:
            {
              // Done with right child.
	      if (!qRight_.up->isEmpty())
                qRight_.up->removeHead();

              rightDone_ = TRUE;
              done = TRUE;
            }
            break;

          case ex_queue::Q_INVALID:
            ex_assert(FALSE, "ExNextValueForTcb::processCancel() "
                      "Invalid state returned by right child");
            break;

          default:
            ex_assert(FALSE, "ExNextValueForTcb::processCancel() "
                      "Unexpected state returned by right child");
            break;

          }; // switch(rentry->upState.status)
      }

  }  // while


  // Now do the left child.
  done = FALSE;
 
  while (NOT done)
  {
    if (qLeft_.up->isEmpty())
      done = TRUE;
    else
      {
        ex_queue_entry * lentry = qLeft_.up->getHeadEntry();

        switch(lentry->upState.status)
          {
          case ex_queue::Q_OK_MMORE:
          case ex_queue::Q_SQLERROR:
            {
              // just consume the child row
              qLeft_.up->removeHead();
            }
            break;

          case ex_queue::Q_NO_DATA:
            {
              // just consume the child row
              qLeft_.up->removeHead();

              // Done with left child.
              leftDone_ = TRUE;
              done = TRUE;
            }
            break;

          case ex_queue::Q_INVALID:
            ex_assert(FALSE, "ExNextValueForTcb::processCancel() "
                      "Invalid state returned by left child");
            break;

          default:
            ex_assert(FALSE, "ExNextValueForTcb::processCancel() "
                      "Unexpected state returned by left child");
            break;

          }; // switch(rentry->upState.status)
      }
  }  // while

  if (rightDone_ && leftDone_)
    {
      pstate.step_ = NVF_DONE;
      return WORK_OK;
    }

  return -1;
}

///////////////////////////////////////////////////////////////////
// Methods for the ExNextValueForPrivateState
///////////////////////////////////////////////////////////////////

ExNextValueForPrivateState::ExNextValueForPrivateState()
{
  init();
}

ExNextValueForPrivateState::~ExNextValueForPrivateState() {}

void ExNextValueForPrivateState::init()
{
  step_ = ExNextValueForTcb::NVF_START;
  matchCount_ = 0;
}

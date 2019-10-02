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
 * File:         ex_ddl.cpp
 * Description:  
 *               
 *               
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include  "cli_stdh.h"
#include  "ex_stdh.h"

#include  "ComTdb.h"
#include  "ex_tcb.h"

#include  "ex_ddl.h"
#include  "ex_exe_stmt_globals.h"
#include  "exp_expr.h"

#include  "ExSqlComp.h"
#include  "HeapLog.h"

#include  "CmpContext.h"
#include  "ComSmallDefs.h"

#include "ExExeUtil.h"

ex_tcb * ExDDLTdb::build(ex_globals * glob)
{
  ExDDLTcb * ddl_tcb = NULL;

  if (returnStatus())
    ddl_tcb = new(glob->getSpace()) ExDDLwithStatusTcb(*this, glob);
  else
    ddl_tcb = new(glob->getSpace()) ExDDLTcb(*this, glob);
  
  ddl_tcb->registerSubtasks();

  return (ddl_tcb);
}

ex_tcb * ExDDLwithStatusTdb::build(ex_globals * glob)
{
  ExDDLTcb * ddl_tcb = NULL;

  ddl_tcb = new(glob->getSpace()) ExDDLwithStatusTcb(*this, glob);
  
  ddl_tcb->registerSubtasks();

  return (ddl_tcb);
}


////////////////////////////////////////////////////////////////
// Constructor for class ExDDLTcb
///////////////////////////////////////////////////////////////
ExDDLTcb::ExDDLTcb(const ComTdbDDL & ddl_tdb,
		   ex_globals * glob)
  : ex_tcb( ddl_tdb, 1, glob),
    compilerVersion_ ( (COM_VERSION) currContext()->getVersionOfCompiler() ),
    workAtp_(NULL)
{
  Space * space = (glob ? glob->getSpace() : 0);
  CollHeap * heap = (glob ? glob->getDefaultHeap() : 0);
  
  // Allocate the buffer pool
  pool_ = new(space) sql_buffer_pool(ddl_tdb.numBuffers_,
				     ddl_tdb.bufferSize_,
				     space);
  
  // Allocate the queue to communicate with parent
  qparent_.down = new(space) ex_queue(ex_queue::DOWN_QUEUE,
				      ddl_tdb.queueSizeDown_,
				      ddl_tdb.criDescDown_,
				      space);
  
  // Allocate the private state in each entry of the down queue
  ExDDLPrivateState *p = new(space) ExDDLPrivateState(this);
  qparent_.down->allocatePstate(p, this);
  delete p;
  
  qparent_.up = new(space) ex_queue(ex_queue::UP_QUEUE,
				    ddl_tdb.queueSizeUp_,
				    ddl_tdb.criDescUp_,
				    space);
  
  if (ddl_tdb.workCriDesc_)
    workAtp_ = allocateAtp(ddl_tdb.workCriDesc_, glob->getSpace());
  
  tcbFlags_ = 0;
  
  if (ddl_tdb.inputExpr_)
    (void)ddl_tdb.inputExpr_->fixup(0, getExpressionMode(), this,
				    space, heap, FALSE, glob);
  if (ddl_tdb.outputExpr_)
    (void)ddl_tdb.outputExpr_->fixup(0, getExpressionMode(), this, 
				     space, heap, FALSE, glob);
};


ExDDLTcb::~ExDDLTcb()
{
  delete qparent_.up;
  delete qparent_.down;
  if (workAtp_)
    {
      workAtp_->release();
      deallocateAtp(workAtp_, getGlobals()->getSpace());
      workAtp_ = NULL;
    }
  freeResources();
};

ex_queue_pair ExDDLTcb::getParentQueue() const {return qparent_;}

Int32 ExDDLTcb::orderedQueueProtocol() const
{
  return ((const ExDDLTdb &)tdb).orderedQueueProtocol();
}

Int32 ExDDLTcb::numChildren() const { return 0; }   

const ex_tcb* ExDDLTcb::getChild(Int32 /*pos*/) const {return 0;};

void ExDDLTcb::freeResources()
{
  delete pool_;
  pool_ = 0;
}

//////////////////////////////////////////////////////
// work() for ExDDLTcb
//////////////////////////////////////////////////////
short ExDDLTcb::work()
{
  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ContextCli *currContext = masterGlob->getStatement()->getContext();
  ExTransaction *ta = currContext->getTransaction();

  short indexToCmp = 0;
  // If there are no master globals, this is not the master executor.
  // The optimizer should ensure that this node executes in the master.
  ex_assert(masterGlob,"ExDDLTcb : No master globals Available\n");

  ComDiagsArea *cpDiagsArea = NULL;

  while(1)
    {
      // if no parent request, return
      if (qparent_.down->isEmpty())
	return WORK_OK;
      
      // if no room in up queue, won't be able to return data/status.
      // Come back later.
      if (qparent_.up->isFull())
	return WORK_OK;

      ExSqlComp *cmp = NULL;
      ExSqlComp::ReturnStatus cmpStatus;

      // the dummyReply is moved up because otherwise the compiler would complain about
      // initialization of variables afer goto statements.
      char* dummyReply = NULL;
      ULng32 dummyLength;
      
      ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
      ExDDLPrivateState & pstate =
	*((ExDDLPrivateState*) pentry_down->pstate);

      CmpCompileInfo c(ddlTdb().query_, ddlTdb().queryLen_ + 1, 
                       (Lng32)ddlTdb().queryCharSet_,
                       ddlTdb().objectName_, ddlTdb().objectNameLen_+1,
                       0, 0);
      
      size_t dataLen = c.getLength();
      char * data = new(getHeap()) char[dataLen];

      if (ddlTdb().hbaseDDL())
	{
	  if (ddlTdb().hbaseDDLNoUserXn())
	    {
	      // this seabase DDL cannot run under a user transaction or if autocommit
	      // is off.
	      if ((!ta->autoCommit()) || (ta->xnInProgress() &&
		   (NOT ta->implicitXn())))
		{
		  if (cpDiagsArea == NULL)
		    cpDiagsArea = ComDiagsArea::allocate(getHeap());
		  
 		  if (ta->xnInProgress())
		    *cpDiagsArea << DgSqlCode(-20123)
				 << DgString0("This DDL operation");
		  else
		    *cpDiagsArea << DgSqlCode(-20124)
				 << DgString0("This DDL");

		  handleErrors (pentry_down, cpDiagsArea, ExSqlComp::ERROR);
		  goto endOfData;
		}
	    }
	  
	  c.setHbaseDDL(TRUE);
	}
      
      c.pack(data);
      
      if (ddlTdb().inputExpr_)
	{
	  pool_->get_free_tuple(workAtp_->getTupp(ddlTdb().workAtpIndex_),
				ddlTdb().inputRowlen_);
	  if (ddlTdb().inputExpr_->eval(pentry_down->getAtp(), workAtp_) == ex_expr::EXPR_ERROR)
 	    {
	      // Using ExSqlComp::ERROR for uniformity in handleErrors.
 	      handleErrors (pentry_down, pentry_down->getDiagsArea(), ExSqlComp::ERROR);
	      goto endOfData;
 	    }
	}

      // Call either the embedded arkcmp, if exists, or external arkcmp
      // but not both
      if ( currContext->isEmbeddedArkcmpInitialized() &&
	   currContext->getSessionDefaults()->callEmbeddedArkcmp() 
	   && ddlTdb().hbaseDDL() &&
	   CmpCommon::context() && (CmpCommon::context()->getRecursionLevel() == 0)
	  )
        {
          const char *parentQid = masterGlob->getStatement()->
            getUniqueStmtId();
          CmpCommon::context()->sqlSession()->setParentQid(parentQid);
          // Despite its name, the compileDirect method is where 
          // the DDL is actually performed. 
          Int32 cpStatus = CmpCommon::context()->compileDirect(
                                 data, dataLen,
                                 currContext->exHeap(),
                                 ddlTdb().queryCharSet_,
                                 EXSQLCOMP::PROCESSDDL,
                                 dummyReply, dummyLength,
                                 currContext->getSqlParserFlags(),
                                 parentQid, str_len(parentQid),
                                 cpDiagsArea);
          getHeap()->deallocateMemory(data);
          if (dummyReply != NULL)
            currContext->exHeap()->deallocateMemory((void*)dummyReply);
          if (cpStatus == ExSqlComp::SUCCESS)
            {
              goto endOfData;
            }
          else
            {
              handleErrors(pentry_down, cpDiagsArea, cpStatus);
              //Don't proceed if its an error.
              if (cpStatus == ExSqlComp::ERROR) 
                 goto endOfData;
            }
        }
      else if (getArkcmp())  // regular arkcmp exists
	{  // start of calling the standard arkcmp process
	  cmp = getArkcmp();

	  // This mxcmp will be used to process the ddl command.
	  // Change the priority of mxcmp to be the same as 'this' process.
	  // That will ensure that the processing in mxcmp and any esps it
	  // starts follow the same priority settings as 'this' master executor.
	  // One note: esp priority delta will be set to the system default for any
	  // esps that are started by the mxcmp. This is because the SET PRIORITY
	  // settings are currently not propagated to mxcmp.
	  cmpStatus = cmp->changePriority(0, TRUE);
	  if (cmpStatus != ExSqlComp::SUCCESS)
	    {
	      // Add a warning that change priority failed.
	      // Continue processing with the original default.
	      cpDiagsArea = ComDiagsArea::allocate(getHeap());

	      *cpDiagsArea << DgSqlCode(15371) << DgString0("MXCMP");

	      //	  handleErrors(pentry_down, cmp->getDiags(), (int) cmpStatus);
	      //	  goto endOfData;
	    }

	  // ddl data is already in iso mapping default value.
	  // Indicate that.
	  // We cannot use the enum SQLCHARSETCODE_ISO_MAPPING out here as that 
	  // will cause mxcmp to use the default charset as iso88591.
	  // So we send the charset of this input string.
	  cmpStatus = 
	    cmp->sendRequest(
			     EXSQLCOMP::PROCESSDDL, data, dataLen,
			     TRUE, NULL,
			     (Lng32)ddlTdb().queryCharSet_,
			     TRUE, /*resend, if needed*/
			     masterGlob->getStatement()->getUniqueStmtId(),
			     masterGlob->getStatement()->getUniqueStmtIdLen());

	  getHeap()->deallocateMemory(data);

	  if (cmpStatus != ExSqlComp::SUCCESS)
	    {
	      handleErrors(pentry_down, cmp->getDiags(), (Int32) cmpStatus);
	      // If its an error don't proceed further.
	      if (cmpStatus == ExSqlComp::ERROR)
		goto endOfData;
	    }

	  cmpStatus = cmp->getReply(dummyReply, dummyLength);
	  cmp->getHeap()->deallocateMemory((void*)dummyReply);
	  if (cmpStatus != ExSqlComp::SUCCESS)
	    {
	      handleErrors(pentry_down, cmp->getDiags(), (Int32) cmpStatus);
	      //Don't proceed if its an error.
	      if (cmpStatus == ExSqlComp::ERROR)
		goto endOfData;
	    }
      
	  if (cmp->status() != ExSqlComp::FETCHED)
	    {
	      handleErrors(pentry_down, cmp->getDiags(), ExSqlComp::ERROR);
	      goto endOfData;
	    }
 
	}
    endOfData:
      // all ok. Return EOF.
      ex_queue_entry * up_entry = qparent_.up->getTailEntry();

      // restore mxcmp priority back to its original value
      if (cmp)
        {
	  cmpStatus = 
	    cmp->changePriority(
				currContext->getSessionDefaults()->getMxcmpPriorityDelta(), TRUE);
	  if (cmpStatus != ExSqlComp::SUCCESS)
	    {
	      // Add a warning that change priority failed.
	      // Continue processing with the original default.
	      if (cpDiagsArea == NULL)
		cpDiagsArea = ComDiagsArea::allocate(getHeap());

	      *cpDiagsArea << DgSqlCode(15371) << DgString0("MXCMP");
	    }
        }

      if (cpDiagsArea)
        {
          ComDiagsArea *diagsArea = up_entry->getDiagsArea();
          if (diagsArea == NULL)
            diagsArea = ComDiagsArea::allocate(this->getGlobals()->getDefaultHeap());
	  
          diagsArea->mergeAfter (*cpDiagsArea);
          up_entry->setDiagsArea(diagsArea);
          cpDiagsArea->decrRefCount();
        }

      up_entry->upState.parentIndex = 
	pentry_down->downState.parentIndex;
      
      up_entry->upState.setMatchNo(pstate.matches_);
      up_entry->upState.status = ex_queue::Q_NO_DATA;
      
      // insert into parent
      qparent_.up->insert();
      
      pstate.init();
      qparent_.down->removeHead();
      
      if (ddlTdb().workCriDesc_)
	workAtp_->release();

      currContext->ddlStmtsExecuted() = TRUE;
       
    } // while 
  return WORK_OK;
}

ExDDLwithStatusTcb::ExDDLwithStatusTcb(const ComTdbDDL & ddl_tdb,
                                       ex_globals * glob)
  : ExDDLTcb(ddl_tdb, glob),
    ddlStep_(0),
    ddlSubstep_(0),
    cmp_(NULL),
    replyBuf_(NULL),
    replyBufLen_(0),
    mdi_(NULL),
    replyDWS_(NULL),
    data_(NULL),
    dataLen_(0),
    callEmbeddedCmp_(FALSE),
    diagsArea_(NULL),
    step_(NOT_STARTED_)
{
  diagsArea_ = ComDiagsArea::allocate(getHeap());
}


//////////////////////////////////////////////////////
// work() for ExDDLTcb
//////////////////////////////////////////////////////
short ExDDLwithStatusTcb::work()
{
  short rc = 0;

  // Get the globals stucture of the master executor.
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();
  ContextCli *currContext = masterGlob->getStatement()->getContext();
  ExTransaction *ta = currContext->getTransaction();

  short indexToCmp = 0;

  // If there are no master globals, this is not the master executor.
  // The optimizer should ensure that this node executes in the master.
  ex_assert(masterGlob,"ExDDLTcb : No master globals Available\n");

  ComDiagsArea *cpDiagsArea = NULL;

  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;
  
  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  if (qparent_.up->isFull())
    return WORK_OK;
  
  ExSqlComp::ReturnStatus cmpStatus;

  char* dummyReply = NULL;
  ULng32 dummyLength;
  
  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExDDLPrivateState & pstate =
    *((ExDDLPrivateState*) pentry_down->pstate);
  
  while(1)
    {
      switch (step_)
	{
	case NOT_STARTED_:
	  {
            cmp_ = NULL;

	    ddlStep_ = 0;
	    ddlSubstep_ = 0;
	    
	    startTime_ = 0;
	    endTime_ = 0;

            numEntries_ = 0;
            currEntry_ = 0;
            currPtr_ = NULL;

            queryStartTime_ = NA_JulianTimestamp();

            if (ddlTdb().hbaseDDL())
              {
                if (ddlTdb().hbaseDDLNoUserXn())
                  {
                    // this seabase DDL cannot run under a user transaction or if autocommit
                    // is off.
                    if ((!ta->autoCommit()) || (ta->xnInProgress() &&
                                                (NOT ta->implicitXn())))
                      {
                        if (cpDiagsArea == NULL)
                          cpDiagsArea = ComDiagsArea::allocate(getHeap());
                        
                        if (ta->xnInProgress())
                          *cpDiagsArea << DgSqlCode(-20123)
                                       << DgString0("This DDL operation");
                        else
                          *cpDiagsArea << DgSqlCode(-20124)
                                       << DgString0("This DDL");
                        
                        handleErrors (pentry_down, cpDiagsArea, ExSqlComp::ERROR);
                        step_ = HANDLE_ERROR_;
                        break;
                      }
                  }
              } // hbaseddl
            
            step_ = SETUP_INITIAL_REQ_;
          }
          break;

        case SETUP_INITIAL_REQ_:
          {
            mdi_ = new(getHeap()) 
              CmpDDLwithStatusInfo(ddlTdb().query_, ddlTdb().queryLen_ + 1, 
                                   (Lng32)ddlTdb().queryCharSet_,
                                   ddlTdb().objectName_, ddlTdb().objectNameLen_+1);

            if (ddlTdb().getMDVersion())
              mdi_->setGetMDVersion(TRUE);
            else if (ddlTdb().getSWVersion())
              mdi_->setGetSWVersion(TRUE);
            else if (ddlTdb().getMDupgrade())
              mdi_->setMDupgrade(TRUE);
            else if (ddlTdb().getMDcleanup())
              {
                mdi_->setMDcleanup(TRUE);
                if (ddlTdb().getCheckOnly())
                  mdi_->setCheckOnly(TRUE);
                if (ddlTdb().getReturnDetails())
                  mdi_->setReturnDetails(TRUE);
              }
            else if (ddlTdb().getInitTraf())
              mdi_->setInitTraf(TRUE);

            mdi_->setHbaseDDL(TRUE);            

            if (ddlTdb().inputExpr_)
              {
                pool_->get_free_tuple(workAtp_->getTupp(ddlTdb().workAtpIndex_),
                                      ddlTdb().inputRowlen_);
                if (ddlTdb().inputExpr_->eval(pentry_down->getAtp(), workAtp_) 
                    == ex_expr::EXPR_ERROR)
                  {
                    // Using ExSqlComp::ERROR for uniformity in handleErrors.
                    handleErrors (pentry_down, pentry_down->getDiagsArea(), 
                                  ExSqlComp::ERROR);
                    step_ = HANDLE_ERROR_;
                    break;
                  }
              }

           // Call either the embedded arkcmp, if exists, or external arkcmp
            // but not both
            if ( currContext->isEmbeddedArkcmpInitialized() &&
                 currContext->getSessionDefaults()->callEmbeddedArkcmp() 
                 && ddlTdb().hbaseDDL() &&
                 CmpCommon::context() && (CmpCommon::context()->getRecursionLevel() 
                                          == 0))
              {
                callEmbeddedCmp_ = TRUE;
              }
            else
              {
                callEmbeddedCmp_ = FALSE;
              }

            step_ = SETUP_NEXT_STEP_;
         }
          break;
          
        case SETUP_NEXT_STEP_:
          {
            if (data_)
              {
                NADELETEBASIC(data_, getHeap());
              }

            dataLen_ = mdi_->getLength();
            data_ = new(getHeap()) char[dataLen_];

            mdi_->pack(data_);
 
            if (callEmbeddedCmp_)
              {
                step_ = CALL_EMBEDDED_CMP_;
              }
            else
              {
                step_ = SEND_REQ_TO_CMP_;
              }
            }
          break;
          
        case CALL_EMBEDDED_CMP_:
          {
            Int32 cmpStatus;
            const char *parentQid = masterGlob->getStatement()->
              getUniqueStmtId();
            CmpCommon::context()->sqlSession()->setParentQid(parentQid);
            
            cmpStatus = CmpCommon::context()->compileDirect(
               data_, dataLen_,
               currContext->exHeap(),
               ddlTdb().queryCharSet_,
               EXSQLCOMP::DDL_WITH_STATUS,
               replyBuf_, replyBufLen_,
               currContext->getSqlParserFlags(),
               parentQid, str_len(parentQid), cpDiagsArea);

            if (currContext->getDiagsArea())
              currContext->getDiagsArea()->clear();

            if ((cpDiagsArea) &&
                ((cpDiagsArea->getNumber(DgSqlCode::WARNING_) > 0) ||
                 (cpDiagsArea->getNumber(DgSqlCode::ERROR_) > 0)))
              {
                getDiagsArea()->mergeAfter(*cpDiagsArea);
              }

            step_ = PROCESS_REPLY_;
          }
          break;

        case SEND_REQ_TO_CMP_:
          {
            if (! getArkcmp())
              {
                step_ = HANDLE_ERROR_;
                break;
              }

            cmp_ = getArkcmp();
            
            cmpStatus = 
              cmp_->sendRequest(
                                EXSQLCOMP::DDL_WITH_STATUS, data_, dataLen_,
                                TRUE, NULL,
                                (Lng32)ddlTdb().queryCharSet_,
                                TRUE, /*resend, if needed*/
                                masterGlob->getStatement()->getUniqueStmtId(),
                                masterGlob->getStatement()->getUniqueStmtIdLen());

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
            
            cmpStatus = cmp_->getReply(replyBuf_, replyBufLen_, 0, 0, TRUE);
            if ((cmp_->getDiags()) &&
                ((cmp_->getDiags()->getNumber(DgSqlCode::WARNING_) > 0) ||
                 (cmp_->getDiags()->getNumber(DgSqlCode::ERROR_) > 0)))
              {
                getDiagsArea()->mergeAfter(*cmp_->getDiags());
                if (cmpStatus == ExSqlComp::ERROR)
                  {
                    step_ = HANDLE_ERROR_;
                    break;
                  }
              }

            step_ = PROCESS_REPLY_;
          }
          break;

        case PROCESS_REPLY_:
          {            
            if (replyBuf_)
              {
                if (replyDWS_)
                  {
                    NADELETEBASIC(replyDWS_, getHeap());
                    replyDWS_ = NULL;
                  }
                
                replyDWS_ = (CmpDDLwithStatusInfo*)(new(getHeap()) char[replyBufLen_]);
                memcpy((char*)replyDWS_, replyBuf_, replyBufLen_);
                if (cmp_)
                  cmp_->getHeap()->deallocateMemory((void*)replyBuf_);
                else
                  currContext->exHeap()->deallocateMemory((void*)replyBuf_);
                replyBuf_ = NULL;
                replyBufLen_ = 0;

                replyDWS_->unpack((char*)replyDWS_);

                if (replyDWS_->computeST())
                  startTime_ = NA_JulianTimestamp();
                else if (replyDWS_->computeET())
                  endTime_ = NA_JulianTimestamp();
              }
            
            if ((replyDWS_->blackBoxLen() > 0) && (replyDWS_->blackBox()))
              {
                Lng32 blackBoxLen = replyDWS_->blackBoxLen();
                char * blackBox = replyDWS_->blackBox();
                currPtr_ = blackBox;
                
                numEntries_ = *(Int32*)currPtr_;
                currEntry_ = 0;
                currPtr_ += sizeof(Int32);
                
                step_ = RETURN_DETAILS_;
              }
            else
              step_ = RETURN_STATUS_;
          }
          break;
          
        case RETURN_STATUS_:
          {
	    // if no room in up queue, won't be able to return data/status.
	    // Come back later.
	    if (qparent_.up->isFull())
	      return WORK_OK;

            char buf[1000];
            if (strlen(replyDWS_->msg()) > 0)
              {
                if (replyDWS_->returnET())
                  {
                    if (replyDWS_->done())
                      startTime_ = queryStartTime_;

                    char timeBuf[100];
                    ExExeUtilTcb::getTimeAsString((endTime_-startTime_), timeBuf,
                                                  TRUE);
                    str_sprintf(buf, "%s {ET: %s}", replyDWS_->msg(),
                                timeBuf);
                  }
                else
                  str_sprintf(buf, "%s", replyDWS_->msg());
                if (moveRowToUpQueue(&qparent_, ddlTdb().tuppIndex(), buf, 0, &rc))
                  return rc;
              }
            
             if (replyDWS_->endStep())
              {
                if (moveRowToUpQueue(&qparent_, ddlTdb().tuppIndex(), " ", 0, &rc))
                  return rc;
              }
            
             step_ = RETURN_STATUS_END_STEP_;
            return WORK_RESCHEDULE_AND_RETURN;
          }
          break;
          
       case RETURN_DETAILS_:
          {
	    // if no room in up queue, won't be able to return data/status.
	    // Come back later.
	    if (qparent_.up->isFull())
	      return WORK_OK;

            if (currEntry_ == numEntries_)
              {
                step_ = RETURN_STATUS_;
                break;
              }

            Int32 currEntrySize = *(Int32*)currPtr_;
            char * currValue = currPtr_ + sizeof(Int32);
            if (moveRowToUpQueue(&qparent_, ddlTdb().tuppIndex(), currValue, 0, &rc))
              return rc;

            currPtr_ += sizeof(Int32);
            currPtr_ += ROUND4(currEntrySize+1);
            currEntry_++;

            return WORK_RESCHEDULE_AND_RETURN;
          }
          break;
          
        case RETURN_STATUS_END_STEP_:
          {
            if (replyDWS_->done())
              {
                if ((getDiagsArea()) &&
                    (getDiagsArea()->getNumber(DgSqlCode::ERROR_) > 0))
                  step_ = HANDLE_ERROR_;
                else
                  step_ = DONE_;
                break;
              }

            mdi_->init();
            mdi_->copyStatusInfo(replyDWS_);    

            step_ = SETUP_NEXT_STEP_;
            return WORK_RESCHEDULE_AND_RETURN;
          }
          break;
          
        case HANDLE_ERROR_:
          {
            if (handleError(&qparent_, getDiagsArea()) == 1)
              return WORK_OK;
            
            step_ = DONE_;
          }
          break;
          
        case DONE_:
          {
            if (handleDone(&qparent_, getDiagsArea()) == 1)
              return WORK_OK;

            step_ = NOT_STARTED_;
            
            return WORK_OK;
          }
          break;
          
        } // switch
    } // while
 
  return WORK_OK;
}

void ExDDLTcb::handleErrors(ex_queue_entry *pentry_down, ComDiagsArea *da, Int32 error)
{
  ex_queue_entry * up_entry = qparent_.up->getTailEntry();
  ExDDLPrivateState & pstate = *((ExDDLPrivateState*) pentry_down->pstate);

  up_entry->upState.parentIndex = pentry_down->downState.parentIndex;
  up_entry->upState.setMatchNo(pstate.matches_);
  if (error == ExSqlComp::ERROR)
    up_entry->upState.status = ex_queue::Q_SQLERROR;
  else
   up_entry->upState.status = ex_queue::Q_OK_MMORE; 

  // Merge the diagsArea and do no pass what is got from the
  // compiler. If the diagsArea from the compiler is passed
  // thro' the queue entry the diagsArea gets deleted when 
  // its merged with the cli. So, when the next error occurs 
  // ExSqlComp uses the same diagArea pointer to insert the
  // the new error_. Only now the memory does not exist because
  // it was deleted by the previous error processing.
  //  up_entry->setDiagsArea(diagsArea);

  ComDiagsArea *diagsArea = up_entry->getDiagsArea();
  
  if (diagsArea == NULL)
    // refCount is set to 1 in allocate(), no need to call incrRefCount().
    diagsArea = ComDiagsArea::allocate(this->getGlobals()->getDefaultHeap());
  else
    diagsArea->incrRefCount();

  diagsArea->mergeAfter (*da);
  up_entry->setDiagsArea(diagsArea);
  
  // insert into parent
  qparent_.up->insert();
}

ExSqlComp * ExDDLTcb::getArkcmp (void)
{
  // Pick the correct version compiler
  short indexIntoCompilerArray = currContext()->getIndexToCompilerArray();
  const COM_VERSION compilerVersionOnEntry = (COM_VERSION) currContext()->getVersionOfCompiler();

  if (!currContext()->getNumArkcmps())
    return NULL;  // no regular compiler exists

  if (compilerVersionOnEntry != getCompilerVersion())
  {
    // The version of the current compiler is different from the version we require.
    // Go set our required version as current.
    currContext()->setOrStartCompiler(getCompilerVersion(), NULL, indexIntoCompilerArray);
  }

  ExSqlComp *cmp = currContext()->getArkcmp(indexIntoCompilerArray);

  if (compilerVersionOnEntry != getCompilerVersion())
  {
    // We changed the compiler version - reset it.
    currContext()->setOrStartCompiler(compilerVersionOnEntry, NULL, indexIntoCompilerArray);
  }

  return cmp;
}

///////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ddl_private_state
///////////////////////////////////////////////////////////////////////////////

ExDDLPrivateState::ExDDLPrivateState(const ExDDLTcb * /*tcb*/)
{
  init();
}

ExDDLPrivateState::~ExDDLPrivateState()
{
};

ex_tcb_private_state * ExDDLPrivateState::allocate_new(const ex_tcb *tcb)
{
  return new(((ex_tcb *)tcb)->getSpace()) ExDDLPrivateState((ExDDLTcb *) tcb);
};

void ExDDLPrivateState::init()
{
  step_ = ExDDLTcb::EMPTY_;
  matches_ = 0;
  request_ = NULL;
  reply_ = NULL;
  dataPtr_ = NULL;
  dataLen_ = 0;
  currLen_ = 0;
}

///////////////////////////////////////////////////////////////////////////
//
// Methods for class ExDescribeTdb, ExDescribeTcb, ExDescribePrivateState
//
///////////////////////////////////////////////////////////////////////////
  
ex_tcb * ExDescribeTdb::build(ex_globals * glob)
{
  ExDescribeTcb * describe_tcb = NULL;
  
  if ((ComTdbDescribe::DescribeType)type_ ==
      ComTdbDescribe::ENVVARS_)
    describe_tcb = new(glob->getSpace()) ExShowEnvvarsTcb(*this, glob);
  else
    describe_tcb = new(glob->getSpace()) ExDescribeTcb(*this, glob);
  
  describe_tcb->registerSubtasks();
  return (describe_tcb);
}

ExDescribeTcb::ExDescribeTcb(const ExDescribeTdb & describe_tdb,
			     ex_globals * glob)
  : ExDDLTcb(describe_tdb, glob)
{
}


short ExDescribeTcb::work()
{
  // if no parent request, return
  if (qparent_.down->isEmpty())
    return WORK_OK;

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExDDLPrivateState & pstate =
    *((ExDDLPrivateState*) pentry_down->pstate);
  
  ExExeStmtGlobals *exeGlob = getGlobals()->castToExExeStmtGlobals();
  ExMasterStmtGlobals *masterGlob = exeGlob->castToExMasterStmtGlobals();

  ComDiagsArea *da = NULL;  
  ComDiagsArea *diagsArea;  
  NAHeap *arkcmpHeap = currContext()->exHeap(); // same heap, see cli/Context.h
  NABoolean deleteTmpDa = FALSE;
  while (1)
    {
      switch (pstate.step_)
	{
        case EMPTY_:
          {
            if (describeTdb().queryCharSet_ == SQLCHARSETCODE_UNKNOWN ||
                describeTdb().queryCharSet_ == SQLCHARSETCODE_ISO88591) // should be set to UTF8 somewhere else,
              describeTdb().queryCharSet_ = SQLCHARSETCODE_UTF8;        // but i do not know where so i set it here.

#ifdef USING_HOSTVAR_AND_PARAMS_FOR_TABLE_NAMES
	    if (describeTdb().inputExpr_)
	      {
		pool_->get_free_tuple(
                      workAtp_->getTupp(describeTdb().workAtpIndex_),
	              describeTdb().inputRowlen_);
		if (describeTdb().inputExpr_->eval(
                     pentry_down->getAtp(), workAtp_) == ex_expr::EXPR_ERROR)
 		  {
 		    da = pentry_down->getDiagsArea();
 		    pstate.step_ = HANDLE_ERROR_;  
 		    break;
 		  }
	      }
#endif
            // Call either the embedded arkcmp, if exists, or external arkcmp
            // but not both
            if (currContext() && currContext()->isEmbeddedArkcmpInitialized() &&
                currContext()->getSessionDefaults()->callEmbeddedArkcmp() &&
		CmpCommon::context() && (CmpCommon::context()->getRecursionLevel() == 0))
              {
                Int32 compStatus;
                const char *parentQid = masterGlob->getStatement()->
                  getUniqueStmtId();
                CmpCommon::context()->sqlSession()->setParentQid(parentQid);

                compStatus = CmpCommon::context()->compileDirect(
                                 describeTdb().query_,
                                 describeTdb().queryLen_,
                                 arkcmpHeap,
                                 describeTdb().queryCharSet_,
                                 EXSQLCOMP::DESCRIBE,
                                 pstate.dataPtr_, pstate.dataLen_,
                                 currContext()->getSqlParserFlags(), 
                                 parentQid, str_len(parentQid), da);
                if (compStatus == ExSqlComp::SUCCESS)
                  {
                    // clear diagsArea of cli context which may have warnings
                    // set when calling cli inside the embedded compiler
                    if (!currContext()->diags().getNumber(DgSqlCode::ERROR_))
                      currContext()->diags().clear();
                    pstate.currLen_ = 0;
                    if (pstate.dataLen_ == pstate.currLen_)
                      pstate.step_ = DONE_;
                    else
                      pstate.step_ = RETURNING_DATA_; // success
                    if ((ComTdbDescribe::DescribeType)describeTdb().type_ ==
                        ComTdbDescribe::LEAKS_)
                      {
                        short error;
                        if (returnLeaks(error))
                          return error;
                      }
                  }
                else
                  {
                    pstate.step_ = HANDLE_ERROR_;
                  }
                  // ComDiagsDa is allocated in compileDirect, needs to be deallocated
                  if (da != NULL)
                    deleteTmpDa = TRUE; 
              }
            else if (getArkcmp())  // regular arkcmp exists
              {
                // Build request and send to arkcmp. Wait for reply.
	        // This could be a waited or a nowaited call.
	        ExSqlComp *cmp = getArkcmp();

                ExSqlComp::ReturnStatus sendStatus =
	            cmp->sendRequest(EXSQLCOMP::DESCRIBE,
				     describeTdb().query_,
				     describeTdb().queryLen_,
				     TRUE, NULL,
				     describeTdb().queryCharSet_
				     );
	        if (sendStatus != ExSqlComp::SUCCESS)
	          {
                    da =  cmp->getDiags();
                    if (sendStatus == ExSqlComp::ERROR)
                      {
                        pstate.step_ = HANDLE_ERROR_;
                        break;
                      }
                  }

	        if (sendStatus != ExSqlComp::ERROR)
	          pstate.step_ = REQUEST_SENT_;
	      }
	  }
	  break;

	case REQUEST_SENT_:
	  {
	    ExSqlComp *cmp = getArkcmp();

	    // check for completion of request
	    if (cmp->status() != ExSqlComp::FINISHED)
	      {
		// still waiting. Come back later.
		return WORK_OK;
	      }

	    // initialize pointer to returned data(from reply)
	    // in private state.
            ExSqlComp::ReturnStatus replyStatus = cmp->getReply(
                                   pstate.dataPtr_, pstate.dataLen_); 
	    if (replyStatus != ExSqlComp::SUCCESS)
	      {
		da =  cmp->getDiags();
		if (replyStatus == ExSqlComp::ERROR)
		  {
		    pstate.step_ = HANDLE_ERROR_;
		    break;
		  }
	      }	
	    if (replyStatus != ExSqlComp::ERROR)
	      {
		pstate.currLen_ = 0;
		if (pstate.dataLen_ == pstate.currLen_)
		  pstate.step_ = DONE_;
		else
		  {
		    pstate.step_ = RETURNING_DATA_;
		    if ((ComTdbDescribe::DescribeType)describeTdb().type_ ==
			ComTdbDescribe::LEAKS_)
		      {
			short error;
			if (returnLeaks(error))
			  return error;
		      }
		  }
	      }
	  }
	  break;
	  
	case RETURNING_DATA_:
	  {
	    // if no room in up queue, won't be able to return data/status.
	    // Come back later.
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    char *srcPtr = (char *)&pstate.dataPtr_[pstate.currLen_];

            // get the length of the source from the first two bytes.
            unsigned short actualLen = *(short *)srcPtr + sizeof(short);

            unsigned short copyLen = actualLen;
	    
            // copy only upto the max length
            if (copyLen > describeTdb().outputRowlen_)
              copyLen = (unsigned short)describeTdb().outputRowlen_;
            
            ex_queue_entry * up_entry = qparent_.up->getTailEntry();

            // allocate space 
	    if (pool_->get_free_tuple(up_entry->getTupp(describeTdb().tuppIndex_),
                                      copyLen))
	      {
		// no more space in the pool.
		// return and come back later. 
		return WORK_POOL_BLOCKED;
	      }

            char *dataPtr = up_entry->getTupp(describeTdb().tuppIndex_).getDataPointer();
    	    
	    // do the copy
            str_cpy_all(dataPtr, srcPtr, copyLen);

            // update the length if needed
            if (copyLen != actualLen) 
              *(short *)dataPtr = copyLen - sizeof(short);

	    pstate.matches_++;
	    
	    up_entry->upState.downIndex   = qparent_.down->getHeadIndex();
	    up_entry->upState.parentIndex = pentry_down->downState.parentIndex;
	    up_entry->upState.setMatchNo(pstate.matches_);
	    up_entry->upState.status      = ex_queue::Q_OK_MMORE;

            // Flow the warning messages if any to the application.
            if (da != NULL)
              {
                diagsArea = up_entry->getDiagsArea();
 	        if (diagsArea == NULL)
 	          diagsArea = ComDiagsArea::allocate(this->getGlobals()->getDefaultHeap());
 	        diagsArea->mergeAfter (*da);
 	        up_entry->setDiagsArea(da);
	        up_entry->getAtp()->getDiagsArea()->incrRefCount();
                // Reset the da for the next error/warning.
                if (deleteTmpDa)
                   da->decrRefCount();
                da = NULL;
              }

            // insert into parent
	    qparent_.up->insert();

            // increment to the next 8 byte boundary
            pstate.currLen_ += (((actualLen-1)/8)+1)*8;
 	    if (pstate.currLen_ >= pstate.dataLen_)
              pstate.step_ = DONE_;
	  }
	  break;

	case RETURNING_LEAKS_:
	  {
	    short error;
	    if (returnLeaks(error))
	      return error;
	  }
	  break;

 	case HANDLE_ERROR_:
 	  {
            if (qparent_.up->isFull())
 	      return WORK_OK;
	    
 	    ex_queue_entry * up_entry = qparent_.up->getTailEntry();
	    
 	    up_entry->upState.parentIndex = pentry_down->downState.parentIndex;
 	    up_entry->upState.setMatchNo(pstate.matches_);
 	    up_entry->upState.status = ex_queue::Q_SQLERROR;
	    
 	    diagsArea = up_entry->getDiagsArea();
 	    
 	    if (diagsArea == NULL)
 	      diagsArea = ComDiagsArea::allocate(this->getGlobals()->getDefaultHeap());
 	    
 	    diagsArea->mergeAfter (*da);
 	    up_entry->setDiagsArea(da);
	    up_entry->getAtp()->getDiagsArea()->incrRefCount();
   
 	    // insert into parent
	    qparent_.up->insert();
	    
            if (deleteTmpDa)
               da->decrRefCount();
 	    // reset the diagsArea for the next error to be set properly.
 	    da = NULL;
 	    pstate.step_ = DONE_;
 	  }
 	  break;

	case DONE_:
	  {
	    if ((pstate.dataPtr_ != NULL) && (pstate.dataLen_ != 0))
	      {  // Delete the reply buffer.
		 arkcmpHeap->deallocateMemory((void*)pstate.dataPtr_);
		 pstate.dataPtr_ = NULL;
                 pstate.dataLen_ = 0;
	      }
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    // all ok. Return EOF.
	    ex_queue_entry * up_entry = qparent_.up->getTailEntry();
	    
	    up_entry->upState.parentIndex = 
	      pentry_down->downState.parentIndex;
	    
	    up_entry->upState.setMatchNo(pstate.matches_);
	    up_entry->upState.status = ex_queue::Q_NO_DATA;
	    
	    // insert into parent
	    qparent_.up->insert();
	    
	    pstate.init();
	    qparent_.down->removeHead();
	    
	    if (describeTdb().workCriDesc_)
	      workAtp_->release();

	    if (qparent_.down->isEmpty())
	      return WORK_OK;
	    
	    pentry_down = qparent_.down->getHeadEntry();
	    pstate = *((ExDDLPrivateState*) pentry_down->pstate);
	  }
	  break;

	case CANCELLED_:
	  break;
	  
	} // switch on pstate.step_
    } // while 
}

Lng32 ExDescribeTcb::returnLeaks(short &error)
{
  error = 0; 
  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ExDDLPrivateState & pstate =
    *((ExDDLPrivateState*) pentry_down->pstate);
#ifndef NA_DEBUG_HEAPLOG
  pstate.step_ = DONE_;
  return 0;
#else
  // if no room in up queue, won't be able to return data/status.
  // Come back later.
  pstate.step_ = RETURNING_LEAKS_;
  if (qparent_.up->isFull())
    {
      error = WORK_OK;
      return 1;
    }

  ex_queue_entry * up_entry = qparent_.up->getTailEntry();
	    
  // allocate space 
  if (pool_->get_free_tuple(up_entry->getTupp(describeTdb().tuppIndex_),
			    describeTdb().outputRowlen_))
	      
    {
      // no more space in the pool.
      // return and come back later. 
      error = WORK_POOL_BLOCKED;
      return 1;
    }
	
  char *entry = up_entry->
		getTupp(describeTdb().tuppIndex_).getDataPointer();

  // For ARKCMP and/or SQLCI
  if (HeapLogRoot::fetchLine(entry,
			     describeTdb().flags_, 
			     pstate.dataPtr_,
			     pstate.dataLen_) == 1)
    { // eof
      pstate.currLen_ = pstate.dataLen_;
      pstate.step_ = DONE_;
      return 0;
    }
  // else push row up the queue

  pstate.matches_++;
	    
  up_entry->upState.downIndex   = qparent_.down->getHeadIndex();
  up_entry->upState.parentIndex = pentry_down->downState.parentIndex;
  up_entry->upState.setMatchNo(pstate.matches_);
  up_entry->upState.status      = ex_queue::Q_OK_MMORE;

  // insert into parent
  qparent_.up->insert();

  return 0; 
#endif
} 

///////////////////////////////////////////////////////////////////
ex_tcb * ExProcessVolatileTableTdb::build(ex_globals * glob)
{
  ExProcessVolatileTableTcb * exe_util_tcb;

  exe_util_tcb = new(glob->getSpace()) ExProcessVolatileTableTcb(*this, glob);

  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

////////////////////////////////////////////////////////////////
// Constructor for class ExProcessVolatileTableTcb
///////////////////////////////////////////////////////////////
ExProcessVolatileTableTcb::ExProcessVolatileTableTcb(
     const ComTdbProcessVolatileTable & exe_util_tdb,
     ex_globals * glob)
     : ExDDLTcb( exe_util_tdb, glob),
       step_(INITIAL_)
{
  // Allocate the private state in each entry of the down queue
  qparent_.down->allocatePstate(this);
}


//////////////////////////////////////////////////////
// work() for ExProcessVolatileTableTcb
//////////////////////////////////////////////////////
short ExProcessVolatileTableTcb::work()
{
  Lng32 cliRC = 0;
  short retcode = 0;

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
  ExTransaction *ta = currContext->getTransaction();
  ComDiagsArea *embCmpDiagsArea = NULL;
  
  ExeCliInterface cliInterface(getHeap(), 0, currContext, 
    masterGlob->getStatement()->getUniqueStmtId());

  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
	    if ((pvtTdb().isCreate()) &&
		(pvtTdb().isSchema()) &&
		(masterGlob->getStatement()->getContext()->volatileSchemaCreated()))
	      {
		// volatile schema already exists, we are done.
		step_ = DONE_;
		break;
	      }

	    if ((pvtTdb().isCreate()) &&
		((pvtTdb().isTable()) ||
		 (pvtTdb().isIndex())))
	      step_ = CREATE_VOLATILE_SCHEMA_;
	    else
	      step_ = SEND_DDL_EXPR_;
	  }
	break;

	case CREATE_VOLATILE_SCHEMA_:
	  {
	    if (NOT masterGlob->getStatement()->getContext()->volatileSchemaCreated())
	      {
		char * createVS 
		  = new(getHeap()) char[strlen("CREATE VOLATILE SCHEMA;") + 1];
		strcpy(createVS, "CREATE VOLATILE SCHEMA;");
		
		cliRC = cliInterface.executeImmediate(createVS);
		NADELETEBASIC(createVS, getHeap());

		if (cliRC < 0)
		  {
		    ExHandleErrors(qparent_,
				   pentry_down,
				   0,
				   getGlobals(),
				   NULL,
				   (ExeErrorCode)cliRC,
				   NULL,
				   NULL
				   );
		    step_ = ERROR_;
		    break;
		  }
	      }

	    step_ = SET_VOLATILE_SCHEMA_USAGE_CQD_;
	  }
	break;

	case SET_VOLATILE_SCHEMA_USAGE_CQD_:
	  {
	    char * sendCQD 
	      = new(getHeap()) char[strlen("CONTROL QUERY DEFAULT VOLATILE_SCHEMA_IN_USE 'TRUE';") + 1];
	    strcpy(sendCQD, "CONTROL QUERY DEFAULT VOLATILE_SCHEMA_IN_USE 'TRUE';");

	    // send CQD to mxcmp
	    cliRC = cliInterface.executeImmediate(sendCQD);
	    NADELETEBASIC(sendCQD, getHeap());
	    if (cliRC < 0)
	      {
		ExHandleErrors(qparent_,
			       pentry_down,
			       0,
			       getGlobals(),
			       NULL,
			       (ExeErrorCode)cliRC,
			       NULL,
			       NULL
			       );
		step_ = ERROR_;
		break;
	      }
	    
	    step_ = SEND_DDL_EXPR_;
	  }
	break;

	case SEND_DDL_EXPR_:
	  {
	    ExSqlComp *cmp = NULL;
	    ExSqlComp::ReturnStatus cmpStatus;
	    // the dummyReply is moved up because otherwise the compiler 
	    // would complain about
	    // initialization of variables afer goto statements.
	    char* dummyReply = NULL;
	    ULng32 dummyLength;
	    
	    CmpCompileInfo c(pvtTdb().query_, pvtTdb().queryLen_ + 1,
			     pvtTdb().queryCharSet_,
			     pvtTdb().objectName_, pvtTdb().objectNameLen_+1,
			     0, 0);
	    
	    if (pvtTdb().hbaseDDL())
	      {
		if (pvtTdb().hbaseDDLNoUserXn())
		  {
		    // this seabase DDL cannot run under a user transaction or if autocommit
		    // is off.
	          if ((!ta->autoCommit()) || (ta->xnInProgress() &&
		          (NOT ta->implicitXn())))
		      {
			ComDiagsArea * cpDiagsArea = ComDiagsArea::allocate(getHeap());
			
			if (ta->xnInProgress())
			  *cpDiagsArea << DgSqlCode(-20123)
				       << DgString0("This DDL operation");
			else
			  *cpDiagsArea << DgSqlCode(-20124)
				       << DgString0("This DDL");
			
			handleErrors (pentry_down, cpDiagsArea, ExSqlComp::ERROR);
			step_ = ERROR_;
			break;
		      }
		  }
		
		c.setHbaseDDL(TRUE);
	      }
	    
	    size_t dataLen = c.getLength();
	    char * data = new(getHeap()) char[dataLen];
	    c.pack(data);

            // Call either the embedded arkcmp, if exists, or external arkcmp
            // but not both
            if ( currContext->isEmbeddedArkcmpInitialized() &&
                 currContext->getSessionDefaults()->callEmbeddedArkcmp() 
                 && pvtTdb().hbaseDDL() &&
                 CmpCommon::context() && (CmpCommon::context()->getRecursionLevel() == 0)
                 )
              {
                Int32 embCmpStatus;
                const char *parentQid = masterGlob->getStatement()->
                  getUniqueStmtId();
                CmpCommon::context()->sqlSession()->setParentQid(parentQid);
                if (embCmpDiagsArea == NULL)
                  embCmpDiagsArea = ComDiagsArea::allocate(getHeap());
                embCmpStatus = CmpCommon::context()->compileDirect(
                     data, dataLen,
                     currContext->exHeap(),
                     SQLCHARSETCODE_UTF8,
                     EXSQLCOMP::PROCESSDDL,
                     dummyReply, dummyLength,
                     currContext->getSqlParserFlags(),
                     parentQid, str_len(parentQid),
                     embCmpDiagsArea);
                getHeap()->deallocateMemory(data);
                if (dummyReply != NULL)
                  currContext->exHeap()->deallocateMemory((void*)dummyReply);
                if (embCmpStatus == ExSqlComp::SUCCESS)
                  {
                    
                    // clear diagsArea of cli context which may have warnings
                    // set when calling cli inside the embedded compiler
                    if (!currContext->diags().getNumber(DgSqlCode::ERROR_))
                      currContext->diags().clear();
                    
                  }
                else
                  {
                    handleErrors(pentry_down, embCmpDiagsArea, embCmpStatus);
                    //Don't proceed if its an error.
                    if (embCmpStatus == ExSqlComp::ERROR)
                      {
                        step_ = ERROR_;
                        break;
                      }
                    
                  }
              }
            else if (getArkcmp())  // regular arkcmp exists
              {	    
                cmp = getGlobals()->castToExExeStmtGlobals()->
                  castToExMasterStmtGlobals()->getCliGlobals()->getArkcmp();
                
                // ddl data is already in iso mapping default value.
                // Indicate that.
                // We cannot use the enum SQLCHARSETCODE_ISO_MAPPING out here as that 
                // will cause mxcmp to use the default charset as iso88591.
                // So we send the charset of this input string.
                cmpStatus = 
                  cmp->sendRequest(EXSQLCOMP::PROCESSDDL, data, dataLen,
                                   TRUE, NULL,
                                   SQLCHARSETCODE_UTF8,
                                   TRUE, /*resend, if needed*/
                                   masterGlob->getStatement()->getUniqueStmtId(),
                                   masterGlob->getStatement()->getUniqueStmtIdLen());
                
                getHeap()->deallocateMemory(data);
                
                if (cmpStatus != ExSqlComp::SUCCESS)
                  {
                    // If its an error don't proceed further.
                    handleErrors(pentry_down, cmp->getDiags(), (Int32) cmpStatus);
                    if (cmpStatus == ExSqlComp::ERROR)
                      {
                        step_ = ERROR_;
                        break;
                      }
                  }
                
                cmpStatus = cmp->getReply(dummyReply, dummyLength);
                cmp->getHeap()->deallocateMemory((void*)dummyReply);
                if (cmpStatus != ExSqlComp::SUCCESS)
                  {
                    // If its an error don't proceed further.
                    handleErrors(pentry_down, cmp->getDiags(), (Int32) cmpStatus);
                    if (cmpStatus == ExSqlComp::ERROR)
                      {
                        step_ = ERROR_;
                        break;
                      }
                  }
                
                if (cmp->status() != ExSqlComp::FETCHED)
                  {
                    handleErrors(pentry_down, cmp->getDiags(), (Int32) cmpStatus);
                    
                    step_ = ERROR_;
                    break;
                  }
              }
            
            if ((pvtTdb().isCreate()) &&
                (pvtTdb().isSchema()))
              {
                masterGlob->getStatement()->getContext()->
                  setVolatileSchemaCreated(TRUE);
              }
            
            if ((NOT pvtTdb().isCreate()) &&
                (pvtTdb().isSchema()))
              {
                masterGlob->getStatement()->getContext()->
                  setVolatileSchemaCreated(FALSE);
              }
            
            if ((pvtTdb().isCreate()) &&
                (pvtTdb().isTable()))
              step_ = ADD_TO_VOL_TAB_LIST_;
            else if ((NOT pvtTdb().isCreate()) &&
                     (pvtTdb().isTable()))
              step_ = REMOVE_FROM_VOL_TAB_LIST_;
            else
              step_ = DONE_;
	  }
	  break;
          
	case ADD_TO_VOL_TAB_LIST_:
	  {
	    HashQueue * volTabList = currContext->getVolTabList();
	    if (! volTabList)
	      currContext->initVolTabList();
	    
	    volTabList = currContext->getVolTabList();

	    volTabList->insert(pvtTdb().volTabName_, 
			       pvtTdb().volTabNameLen_,
			       (void*)pvtTdb().volTabName_);

	    step_ = DONE_;
	  }
	break;

	case REMOVE_FROM_VOL_TAB_LIST_:
	  {
	    HashQueue * volTabList = currContext->getVolTabList();
	    if (volTabList == NULL) {
	       step_ = DONE_;
	       break;
	    }
        
	    volTabList->position(pvtTdb().volTabName_,
				 pvtTdb().volTabNameLen_);
	    void * name = volTabList->getNext();
	    volTabList->remove(name);
	    
	    //	    if (volTabList->entries() == 0)
	    //step_ = RESET_VOLATILE_SCHEMA_USAGE_CQD_;
	    //else
	    step_ = DONE_;
	  }
	break;

	case RESET_VOLATILE_SCHEMA_USAGE_CQD_:
	  {
	    char * sendCQD 
	      = new(getHeap()) char[strlen("CONTROL QUERY DEFAULT VOLATILE_SCHEMA_IN_USE 'FALSE';") + 1];
	    strcpy(sendCQD, "CONTROL QUERY DEFAULT VOLATILE_SCHEMA_IN_USE 'FALSE';");

	    // send CQD to mxcmp
	    cliRC = cliInterface.executeImmediate(sendCQD);
	    NADELETEBASIC(sendCQD, getHeap());
	    if (cliRC < 0)
	      {
		ExHandleErrors(qparent_,
			       pentry_down,
			       0,
			       getGlobals(),
			       NULL,
			       (ExeErrorCode)cliRC,
			       NULL,
			       NULL
			       );
		step_ = ERROR_;
		break;
	      }
	    
	    step_ = DONE_;
	  }
	break;

	case ERROR_:
	  {
	    step_ = DONE_;
	  }
	break;

	case DONE_:
	  {
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

	} // switch
    } // while
  
  return WORK_OK;
}

////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ExProcessVolatileTableTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExProcessVolatileTablePrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExProcessVolatileTablePrivateState::ExProcessVolatileTablePrivateState()
{
}

ExProcessVolatileTablePrivateState::~ExProcessVolatileTablePrivateState()
{
};

///////////////////////////////////////////////////////////////////
ex_tcb * ExProcessInMemoryTableTdb::build(ex_globals * glob)
{
  ExProcessInMemoryTableTcb * exe_util_tcb;

  exe_util_tcb = new(glob->getSpace()) ExProcessInMemoryTableTcb(*this, glob);

  exe_util_tcb->registerSubtasks();

  return (exe_util_tcb);
}

////////////////////////////////////////////////////////////////
// Constructor for class ExProcessInMemoryTableTcb
///////////////////////////////////////////////////////////////
ExProcessInMemoryTableTcb::ExProcessInMemoryTableTcb(
     const ComTdbProcessInMemoryTable & exe_util_tdb,
     ex_globals * glob)
     : ExDDLTcb( exe_util_tdb, glob),
       step_(INITIAL_)
{
  // Allocate the private state in each entry of the down queue
  qparent_.down->allocatePstate(this);
}

//////////////////////////////////////////////////////
// work() for ExProcessInMemoryTableTcb
//////////////////////////////////////////////////////
short ExProcessInMemoryTableTcb::work()
{
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

  ExeCliInterface cliInterface(getHeap(), 0, currContext,
    masterGlob->getStatement()->getUniqueStmtId());
  ExSqlComp *cmp = NULL;

  while (1)
    {
      switch (step_)
	{
	case INITIAL_:
	  {
	    // his DDL is to create an inMemory object,
	    // mark that in our context.
	    // Mxcmp will be killed at the end of this session.
	    currContext->setInMemoryObjectDefn(TRUE);

	    volSchCreatedHere_ = FALSE;

	    step_ = CREATE_VOLATILE_SCHEMA_;
	  }
	break;

	case CREATE_VOLATILE_SCHEMA_:
	  {
	    if (NOT masterGlob->getStatement()->getContext()->volatileSchemaCreated())
	      {
		char * createVS 
		  = new(getHeap()) char[strlen("CREATE VOLATILE SCHEMA;") + 1];
		strcpy(createVS, "CREATE VOLATILE SCHEMA;");
		
		cliRC = cliInterface.executeImmediate(createVS);
		NADELETEBASIC(createVS, getHeap());

		if (cliRC < 0)
		  {
		    ExHandleErrors(qparent_,
				   pentry_down,
				   0,
				   getGlobals(),
				   NULL,
				   (ExeErrorCode)cliRC,
				   NULL,
				   NULL
				   );
		    step_ = ERROR_;
		    break;
		  }

		volSchCreatedHere_ = TRUE;

		step_ = SET_VOLATILE_SCHEMA_USAGE_CQD_;
	      }
	    else
	      step_ = TURN_QUERY_CACHE_OFF_;
	  }
	break;
	    
	case SET_VOLATILE_SCHEMA_USAGE_CQD_:
	  {
	    char * sendCQD 
	      = new(getHeap()) char[strlen("CONTROL QUERY DEFAULT VOLATILE_SCHEMA_IN_USE 'TRUE';") + 1];
	    strcpy(sendCQD, "CONTROL QUERY DEFAULT VOLATILE_SCHEMA_IN_USE 'TRUE';");
	    
	    // send CQD to mxcmp
	    cliRC = cliInterface.executeImmediate(sendCQD);
	    NADELETEBASIC(sendCQD, getHeap());
	    if (cliRC < 0)
	      {
		ExHandleErrors(qparent_,
			       pentry_down,
			       0,
			       getGlobals(),
			       NULL,
			       (ExeErrorCode)cliRC,
			       NULL,
			       NULL
			       );
		step_ = ERROR_;
		break;
	      }
	    
	    step_ = TURN_QUERY_CACHE_OFF_;
	  }
	break;

	case TURN_QUERY_CACHE_OFF_:
	  {
	    // cannot use query cache if InMemory objects are created.
	    // turn it off.
	    char * sendCQD 
	      = new(getHeap()) char[strlen("CONTROL QUERY DEFAULT QUERY_CACHE '0';") + 1];
	    strcpy(sendCQD, "CONTROL QUERY DEFAULT QUERY_CACHE '0';");
	    
	    // send CQD to mxcmp
	    cliRC = cliInterface.executeImmediate(sendCQD);
	    NADELETEBASIC(sendCQD, getHeap());
	    if (cliRC < 0)
	      {
		ExHandleErrors(qparent_,
			       pentry_down,
			       0,
			       getGlobals(),
			       NULL,
			       (ExeErrorCode)cliRC,
			       NULL,
			       NULL
			       );
		step_ = ERROR_;
		break;
	      }
	    
	    step_ = SEND_DDL_EXPR_;
	  }
	break;

	case SEND_DDL_EXPR_:
	  {
	    ExSqlComp::ReturnStatus cmpStatus;
	    // the dummyReply is moved up because otherwise the compiler 
	    // would complain about
	    // initialization of variables afer goto statements.
	    char* dummyReply = NULL;
	    ULng32 dummyLength;
	    
	    CmpCompileInfo c(pimtTdb().query_, pimtTdb().queryLen_ + 1,
			     pimtTdb().queryCharSet_,
			     pimtTdb().objectName_, pimtTdb().objectNameLen_+1,
			     0, 0);
	    
	    size_t dataLen = c.getLength();
	    char * data = new(getHeap()) char[dataLen];
	    c.pack(data);
	    
	    cmp = getGlobals()->castToExExeStmtGlobals()->
	      castToExMasterStmtGlobals()->getCliGlobals()->getArkcmp();
	    cmpStatus = 
	      cmp->sendRequest(EXSQLCOMP::PROCESSDDL, data, dataLen,
			       TRUE, NULL,
			       SQLCHARSETCODE_UTF8,
			       TRUE /*resend, if needed*/);
			       
	    getHeap()->deallocateMemory(data);
	    
	    if (cmpStatus != ExSqlComp::SUCCESS)
	      {
		// If its an error don't proceed further.
		handleErrors(pentry_down, cmp->getDiags(), (Int32) cmpStatus);
		if (cmpStatus == ExSqlComp::ERROR)
		  {
		    step_ = DROP_;
		    break;
		  }
	      }
	    
	    cmpStatus = cmp->getReply(dummyReply, dummyLength);
	    cmp->getHeap()->deallocateMemory((void*)dummyReply);
	    if (cmpStatus != ExSqlComp::SUCCESS)
	      {
		// If its an error don't proceed further.
		handleErrors(pentry_down, cmp->getDiags(), (Int32) cmpStatus);
		if (cmpStatus == ExSqlComp::ERROR)
		  {
		    step_ = DROP_;
		    break;
		  }
	      }
	    
	    if (cmp->status() != ExSqlComp::FETCHED)
	      {
		handleErrors(pentry_down, cmp->getDiags(), (Int32) cmpStatus);

		step_ = DROP_;
		break;
	      }


	    masterGlob->getStatement()->getContext()->
	      setVolatileSchemaCreated(TRUE);

	    step_ = DONE_;
	  }
	break;

	case DROP_:
	  {
	    // in memory objects are not automaticalle removed from
	    // in-memory metadata in case of errors.
	    // This is because they are not 'aborted' using tmf as
	    // they don't touch any physical files/metadata.
	    // So we need to manually remove the created object by
	    // dropping it.

	    // do not drop if table already exists.
	    if ((pimtTdb().isCreate()) &&
		(cmp->getDiags()->mainSQLCODE() != -1055)) 
	      {
		char * dtQuery = 
		  new(getHeap()) char[strlen("DROP VOLATILE TABLE ; ") + 
				     strlen(pimtTdb().objName_) +
				     100];

		if (pimtTdb().isTable())
		  {
		    if (pimtTdb().isVolatile())
		      strcpy(dtQuery, "DROP VOLATILE TABLE ");
		    else
		      strcpy(dtQuery, "DROP TABLE ");
		  }
		else if (pimtTdb().isIndex())
		  {
		    if (pimtTdb().isVolatile())
		      strcpy(dtQuery, "DROP VOLATILE INDEX ");
		    else
		      strcpy(dtQuery, "DROP INDEX ");
		  }
		else if (pimtTdb().isMV())
		  {
		    strcpy(dtQuery, "DROP MV ");
		  }
		
		strcat(dtQuery, pimtTdb().objName_);
		strcat(dtQuery, ";");
		cliRC = cliInterface.executeImmediate(dtQuery);
		if (cliRC < 0)
		  {
		    // ignore errors.
		    SQL_EXEC_ClearDiagnostics(NULL);
		  }
		
		// Delete new'd characters
		NADELETEBASIC(dtQuery, getHeap());
	      }

	    step_ = ERROR_;
	  }
	break;

	case ERROR_:
	  {
	    if (volSchCreatedHere_)
	      {
		char * dtQuery = 
		  new(getHeap()) char[strlen("DROP IMPLICIT VOLATILE SCHEMA CASCADE; ") + 
				     100];
		strcpy(dtQuery, "DROP IMPLICIT VOLATILE SCHEMA CASCADE;");

		cliRC = cliInterface.executeImmediate(dtQuery);
		if (cliRC < 0)
		  {
		    // ignore errors.
		    SQL_EXEC_ClearDiagnostics(NULL);
		  }
		
		// Delete new'd characters
		NADELETEBASIC(dtQuery, getHeap());
	      }

	    step_ = DONE_;
	  }
	break;

	case DONE_:
	  {

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

	} // switch
    } // while
  
  return WORK_OK;
}

////////////////////////////////////////////////////////////////////////
// Redefine virtual method allocatePstates, to be used by dynamic queue
// resizing, as well as the initial queue construction.
////////////////////////////////////////////////////////////////////////
ex_tcb_private_state * ExProcessInMemoryTableTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ExProcessInMemoryTablePrivateState> pa;

  return pa.allocatePstates(this, numElems, pstateLength);
}

/////////////////////////////////////////////////////////////////////////////
// Constructor and destructor for ExeUtil_private_state
/////////////////////////////////////////////////////////////////////////////
ExProcessInMemoryTablePrivateState::ExProcessInMemoryTablePrivateState()
{
}

ExProcessInMemoryTablePrivateState::~ExProcessInMemoryTablePrivateState()
{
};


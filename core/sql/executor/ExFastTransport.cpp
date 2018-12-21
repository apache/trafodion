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
 * File:         ExFastTransport.cpp
 * Description:  TDB/TCB for Fast transport
 *
 * Created:      08/28/2012
 * Language:     C++
 *
 *
 *****************************************************************************
 */
#include "ex_stdh.h"
#include "ExFastTransport.h"
#include "ex_globals.h"
#include "ex_exe_stmt_globals.h"
#include "exp_attrs.h"
#include "exp_clause_derived.h"
#include "ex_error.h"
#include "ExStats.h"
#include "ExCextdecs.h"
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <pthread.h>
#include "ComSysUtils.h"
#include "SequenceFileReader.h" 
#include "HdfsClient_JNI.h" 
#include  "cli_stdh.h"
#include "ComSmallDefs.h"


//----------------------------------------------------------------------
// ExFastExtractTcb methods
//----------------------------------------------------------------------
ex_tcb *ExFastExtractTdb::build(ex_globals *glob)
{
  // first build the child TCBs
  ex_tcb *childTcb ;
  ExFastExtractTcb *feTcb;

  childTcb = childTdb_->build(glob);

    feTcb = new (glob->getSpace()) ExHdfsFastExtractTcb(*this, *childTcb,glob);

  feTcb->registerSubtasks();
  feTcb->registerResizeSubtasks();

  return feTcb;

}

ExFastExtractTcb::ExFastExtractTcb(
    const ExFastExtractTdb &fteTdb,
    const ex_tcb & childTcb,
    ex_globals *glob)
  : ex_tcb(fteTdb, 1, glob),
    workAtp_(NULL),
    outputPool_(NULL),
    inputPool_(NULL),
    childTcb_(&childTcb)
  , inSqlBuffer_(NULL)
  , childOutputTD_(NULL)
  , sourceFieldsConvIndex_(NULL)
  , currBuffer_(NULL)
  , bufferAllocFailuresCount_(0)
  , modTS_(-1)
{
  
  ex_globals *stmtGlobals = getGlobals();

  Space *globSpace = getSpace();
  CollHeap *globHeap = getHeap();

  heap_ = globHeap;

  //convert to non constant to access the members.
  ExFastExtractTdb *mytdb = (ExFastExtractTdb*)&fteTdb;
  numBuffers_ = mytdb->getNumIOBuffers();

  // Allocate queues to communicate with parent
  allocateParentQueues(qParent_);

  // get the queue that child use to communicate with me
  qChild_  = childTcb.getParentQueue();
    
  // Allocate the work ATP
  if (myTdb().getWorkCriDesc())
    workAtp_ = allocateAtp(myTdb().getWorkCriDesc(), globSpace);

  // Fixup expressions
  // NOT USED in M9
  /*
  if (myTdb().getInputExpression())
    myTdb().getInputExpression()->fixup(0, getExpressionMode(), this,
                                       globSpace, globHeap);

  if (myTdb().getOutputExpression())
    myTdb().getOutputExpression()->fixup(0, getExpressionMode(), this,
                                        globSpace, globHeap);
  */

  if (myTdb().getChildDataExpr())
    myTdb().getChildDataExpr()->fixup(0,getExpressionMode(),this,
                                       globSpace, globHeap, FALSE, glob);


  //maybe we can move the below few line to the init section od work methods??
  UInt32 numAttrs = myTdb().getChildTuple()->numAttrs();

   sourceFieldsConvIndex_ = (int *)((NAHeap *)heap_)->allocateAlignedHeapMemory((UInt32)(sizeof(int) * numAttrs), 512, FALSE);

  maxExtractRowLength_ = ROUND8(myTdb().getChildDataRowLen()) ;

  const ULng32 sqlBufferSize = maxExtractRowLength_ +
                               ROUND8(sizeof(SqlBufferNormal)) +
                               sizeof(tupp_descriptor) +
                               16 ;//just in case

  inSqlBuffer_ = (SqlBuffer *) new (heap_) char[sqlBufferSize];
  inSqlBuffer_->driveInit(sqlBufferSize, TRUE, SqlBuffer::NORMAL_);
  childOutputTD_ = inSqlBuffer_->add_tuple_desc(maxExtractRowLength_);

  endOfData_ = FALSE;

} // ExFastExtractTcb::ExFastExtractTcb

ExFastExtractTcb::~ExFastExtractTcb()
{
  // Release resources acquired
  //
  freeResources();

  delete qParent_.up;
  delete qParent_.down;
 
  if (workAtp_)
  {
    workAtp_->release();
    deallocateAtp(workAtp_, getSpace());
  }

  if (inSqlBuffer_ && getHeap())
  {
    getHeap()->deallocateMemory(inSqlBuffer_);
    inSqlBuffer_ = NULL;
    childOutputTD_ = NULL;
  }
  if (sourceFieldsConvIndex_)
    getHeap()->deallocateMemory(sourceFieldsConvIndex_);

} // ExFastExtractTcb::~ExFastExtractTcb()

//
// This function frees any resources acquired.
// It should only be called by the TCB destructor.
//
void ExFastExtractTcb::freeResources()
{
  for(Int16 i=0; i < numBuffers_; i++)
  {
    if(bufferPool_[i])
    {
      NADELETE(bufferPool_[i], IOBuffer, getHeap());
      bufferPool_[i] = NULL;
    }
  }
}

void ExFastExtractTcb::registerSubtasks()
{
  ex_tcb :: registerSubtasks();
}

ex_tcb_private_state *ExFastExtractTcb::allocatePstates(
  Lng32 &numElems,      // [IN/OUT] desired/actual elements
  Lng32 &pstateLength)  // [OUT] length of one element
{
  PstateAllocator<ExFastExtractPrivateState> pa;
  return pa.allocatePstates(this, numElems, pstateLength);
}

// Insert a single entry into the up queue and optionally
// remove the head of the down queue
//
// Right now this function does not handle data rows, only error
// and end-of-data. It could possibly be extended to handle a data
// row. I have not looked at that closely enough yet.
//
void ExFastExtractTcb::insertUpQueueEntry(ex_queue::up_status status,
                                  ComDiagsArea *diags,
                                  NABoolean popDownQueue)
{

  ex_queue_entry *upEntry = qParent_.up->getTailEntry();
  ex_queue_entry *downEntry = qParent_.down->getHeadEntry();
  ExFastExtractPrivateState &privateState =
    *((ExFastExtractPrivateState *) downEntry->pstate);

  // Initialize the up queue entry. 
  //
  // copyAtp() will copy all tuple pointers and the diags area from
  // the down queue entry to the up queue entry.
  //
  // When we return Q_NO_DATA if the match count is > 0:
  // * assume down queue diags were returned with the Q_OK_MMORE entries
  // * release down queue diags before copyAtp()
  //
  if (status == ex_queue::Q_NO_DATA && privateState.matchCount_ > 0)
  {
    downEntry->setDiagsArea(NULL);
    upEntry->copyAtp(downEntry);
  }
  else
  {
    upEntry->copyAtp(downEntry);
    downEntry->setDiagsArea(NULL);
  }

  upEntry->upState.status = status;
  upEntry->upState.parentIndex = downEntry->downState.parentIndex;
  upEntry->upState.downIndex = qParent_.down->getHeadIndex();
  upEntry->upState.setMatchNo(privateState.matchCount_);
  
  // Move any diags to the up queue entry
  if (diags != NULL)
  {
    ComDiagsArea *atpDiags = upEntry->getDiagsArea();
    if (atpDiags == NULL)
    {
      // setDiagsArea() does not increment the reference count
      upEntry->setDiagsArea(diags);
      diags->incrRefCount();
    }
    else
    {
      atpDiags->mergeAfter(*diags);
    }
  }
  
  // Insert into up queue
  qParent_.up->insert();
 
  // Optionally remove the head of the down queue
  if (popDownQueue)
  {
    privateState.init();
    qParent_.down->removeHead();
  }
}


const char *ExFastExtractTcb::getTcbStateString(FastExtractStates s)
{
  switch (s)
  {
    case EXTRACT_NOT_STARTED:           return "EXTRACT_NOT_STARTED";
    case EXTRACT_INITIALIZE:            return "EXTRACT_INITIALIZE";
    case EXTRACT_PASS_REQUEST_TO_CHILD: return "EXTRACT_PASS_REQUEST_TO_CHILD";
    case EXTRACT_RETURN_ROWS_FROM_CHILD:return "EXTRACT_RETURN_ROWS_FROM_CHILD";
    case EXTRACT_DATA_READY_TO_SEND:    return "EXTRACT_DATA_READY_TO_SEND";
    case EXTRACT_SYNC_WITH_IO_THREAD:   return "EXTRACT_SYNC_WITH_IO_THREAD";  
    case EXTRACT_ERROR:                 return "EXTRACT_ERROR";
    case EXTRACT_DONE:                  return "EXTRACT_DONE";
    case EXTRACT_CANCELED:              return "EXTRACT_CANCELED";
    default:                            return "UNKNOWN";
  }
}

/* Work method, where all the action happens
The stick figure shows transitions between various states.
Transitions to EXTRACT_ERROR are not shown for clarity.

EXTRACT_NOT_STARTED
        |
        |
        V
EXTRACT_INITIALIZE
        |
        |
        V
EXTRACT_PASS_REQUEST_TO_CHILD                 
        |                                             
        |                   |----|
        V                   V    |            
EXTRACT_RETURN_ROWS_FROM_CHILD ->|    EXTRACT_ERROR 
        |      ^           |                | 
        |      |       |------|             |
        V      |       V   |  |             |   
EXTRACT_SYNC_WITH_IO_THREAD|->|             |
        |      ^           |                | 
        |      |           |                |
        V      |           |                V         
EXTRACT_DATA_READY_TO_SEND<-           EXTRACT_CANCELLED  
                       \               /
                        \             /
                         V           V
                         EXTRACT_DONE



*/
ExWorkProcRetcode ExFastExtractTcb::work()
{

  assert(0);
  return WORK_OK;

}//ExFastExtractTcb::work()



void ExFastExtractTcb::updateWorkATPDiagsArea(ComDiagsArea *da)
{
    if (da)
    {
      if (workAtp_->getDiagsArea())
      {
        workAtp_->getDiagsArea()->mergeAfter(*da);
      }
      else
      {
        ComDiagsArea * da1 = da;
        workAtp_->setDiagsArea(da1);
        da1->incrRefCount();
      }
    }
}
void ExFastExtractTcb::updateWorkATPDiagsArea(ex_queue_entry * centry)
{
    if (centry->getDiagsArea())
    {
      if (workAtp_->getDiagsArea())
      {
        workAtp_->getDiagsArea()->mergeAfter(*centry->getDiagsArea());
      }
      else
      {
        ComDiagsArea * da = centry->getDiagsArea();
        workAtp_->setDiagsArea(da);
        da->incrRefCount();
        centry->setDiagsArea(0);
      }
    }
}

void ExFastExtractTcb::updateWorkATPDiagsArea(ExeErrorCode rc, const char *msg)
{
    ComDiagsArea *da = workAtp_->getDiagsArea();
    if(!da)
    {
      da = ComDiagsArea::allocate(getHeap());
      workAtp_->setDiagsArea(da);
    }
   if(msg != NULL)
   {
	   *da << DgSqlCode(-rc)
    	   << DgString0(msg);
   }
   else
   {
	  *da << DgSqlCode(-rc);
   }

}

void ExFastExtractTcb::updateWorkATPDiagsArea(const char *file, 
                                              int line, const char *msg)
{
    ComDiagsArea *da = workAtp_->getDiagsArea();
    if(!da)
    {
      da = ComDiagsArea::allocate(getHeap());
      workAtp_->setDiagsArea(da);
    }
   
    *da << DgSqlCode(-1001)
        << DgString0(file)
        << DgInt0(line)
        << DgString1(msg);
}


NABoolean ExFastExtractTcb::needStatsEntry()
{
  if ((getGlobals()->getStatsArea()->getCollectStatsType() == ComTdb::ALL_STATS) ||
      (getGlobals()->getStatsArea()->getCollectStatsType() == ComTdb::OPERATOR_STATS))
    return TRUE;
  else
    return FALSE;
}
ExOperStats * ExFastExtractTcb::doAllocateStatsEntry(
                                              CollHeap *heap,
                                              ComTdb *tdb)
{
  ExOperStats *stat = NULL;

  ComTdb::CollectStatsType statsType = getGlobals()->getStatsArea()->getCollectStatsType();

  if (statsType == ComTdb::OPERATOR_STATS)
  {
     ExEspStmtGlobals *espGlobals = getGlobals()->castToExExeStmtGlobals()->castToExEspStmtGlobals();
     StmtStats *ss; 
     if (espGlobals != NULL)
        ss = espGlobals->getStmtStats();
     else
        ss = getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals()->getStatement()->getStmtStats(); 
     ExHdfsScanStats *hdfsScanStats = new (heap) ExHdfsScanStats(heap, this, tdb);
     if (ss != NULL)
        hdfsScanStats->setQueryId(ss->getQueryId(), ss->getQueryIdLen());
     return hdfsScanStats;
  }
  else
  {

    return new(heap) ExFastExtractStats( heap,
                                         this,
                                         tdb);
  }
}



ExHdfsFastExtractTcb::ExHdfsFastExtractTcb(
    const ExFastExtractTdb &fteTdb,
    const ex_tcb & childTcb,
    ex_globals *glob)
  : ExFastExtractTcb(
      fteTdb,
      childTcb,
      glob),
    sequenceFileWriter_(NULL)
  , hdfsClient_(NULL)
{

} // ExHdfsFastExtractTcb::ExFastExtractTcb

ExHdfsFastExtractTcb::~ExHdfsFastExtractTcb()
{


  if (sequenceFileWriter_ != NULL) {
     NADELETE(sequenceFileWriter_, SequenceFileWriter, getHeap());
     sequenceFileWriter_ = NULL;
  }

  if (hdfsClient_ != NULL) {
     NADELETE(hdfsClient_, HdfsClient, getHeap());
     hdfsClient_ = NULL;
  }

} // ExHdfsFastExtractTcb::~ExHdfsFastExtractTcb()


Int32 ExHdfsFastExtractTcb::fixup()
{
  ex_tcb::fixup();

  strncpy(hdfsHost_, myTdb().getHdfsHostName(), sizeof(hdfsHost_));
  hdfsPort_ = myTdb().getHdfsPortNum();
  
  modTS_ = myTdb().getModTSforDir();

  return 0;
}

void ExHdfsFastExtractTcb::convertSQRowToString(ULng32 nullLen,
                            ULng32 recSepLen,
                            ULng32 delimLen,
                            tupp_descriptor* dataDesc,
                            char* targetData,
                            NABoolean & convError) {
  char* childRow = dataDesc->getTupleAddress();
  ULng32 childRowLen = dataDesc->getAllocatedSize();
  UInt32 vcActualLen = 0;

  for (UInt32 i = 0; i < myTdb().getChildTuple()->numAttrs(); i++) {
    Attributes * attr = myTdb().getChildTableAttr(i);
    Attributes * attr2 = myTdb().getChildTableAttr2(i);
    char *childColData = NULL; //childRow + attr->getOffset();
    UInt32 childColLen = 0;
    UInt32 maxTargetColLen = attr2->getLength();

    //format is aligned format--
    //----------
    // field is varchar
    if (attr->getVCIndicatorLength() > 0) {
      childColData = childRow + *((UInt32*) (childRow + attr->getVoaOffset()));
      childColLen = attr->getLength(childColData);
      childColData += attr->getVCIndicatorLength();
    } else {              //source is fixed length
      childColData = childRow + attr->getOffset();
      childColLen = attr->getLength();
      if ((attr->getCharSet() == CharInfo::ISO88591
          || attr->getCharSet() == CharInfo::UTF8) && childColLen > 0) {
        // trim trailing blanks
        while (childColLen > 0 && childColData[childColLen - 1] == ' ') {
          childColLen--;
        }
      } else if (attr->getCharSet() == CharInfo::UCS2 && childColLen > 1) {
        ex_assert(childColLen % 2 == 0, "invalid ucs2");
        NAWchar* wChildColData = (NAWchar*) childColData;
        Int32 wChildColLen = childColLen / 2;
        while (wChildColLen > 0 && wChildColData[wChildColLen - 1] == L' ') {
          wChildColLen--;
        }
        childColLen = wChildColLen * 2;
      }
    }

    if (attr->getNullFlag()
        && ExpAlignedFormat::isNullValue(childRow + attr->getNullIndOffset(),
            attr->getNullBitIndex())) {
      // source is a null value.

      nullLen = 0;
      if (myTdb().getNullString()) {
        nullLen = strlen(myTdb().getNullString());
        memcpy(targetData, myTdb().getNullString(), nullLen); 
      } 

      targetData += nullLen;
      currBuffer_->bytesLeft_ -= nullLen;
    } else {
      switch ((ConvInstruction) sourceFieldsConvIndex_[i]) {
      case CONV_ASCII_V_V:
      case CONV_ASCII_F_V:
      case CONV_UNICODE_V_V:
      case CONV_UNICODE_F_V: {
        if (childColLen > 0) {
          memcpy(targetData, childColData, childColLen);
          targetData += childColLen;
          currBuffer_->bytesLeft_ -= childColLen;
        }
      }
        break;

      default:
        ex_expr::exp_return_type err = convDoIt(childColData, childColLen,
            attr->getDatatype(), attr->getPrecision(), attr->getScale(),
            targetData, attr2->getLength(), attr2->getDatatype(),
            attr2->getPrecision(), attr2->getScale(), (char*) &vcActualLen,
            sizeof(vcActualLen), 0, 0,             // diags may need to be added
            (ConvInstruction) sourceFieldsConvIndex_[i]);

        if (err == ex_expr::EXPR_ERROR) {
          convError = TRUE;
          // not exit loop -- we will log the errenous row later
          // do not cancel processing for this type of error???
        }
        targetData += vcActualLen;
        currBuffer_->bytesLeft_ -= vcActualLen;
        break;
      }                      //switch
    }

    if (i == myTdb().getChildTuple()->numAttrs() - 1) {
      strncpy(targetData, myTdb().getRecordSeparator(), recSepLen);
      targetData += recSepLen;
      currBuffer_->bytesLeft_ -= recSepLen;
    } else {
      strncpy(targetData, myTdb().getDelimiter(), delimLen);
      targetData += delimLen;
      currBuffer_->bytesLeft_ -= delimLen;
    }

  }
}

ExWorkProcRetcode ExHdfsFastExtractTcb::work()
{
  Lng32 retcode = 0;
  SFW_RetCode sfwRetCode = SFW_OK;
  HDFS_Client_RetCode hdfsClientRetCode = HDFS_CLIENT_OK;
  ULng32 recSepLen = strlen(myTdb().getRecordSeparator());
  ULng32 delimLen = strlen(myTdb().getDelimiter());
  ULng32 nullLen = 
    (myTdb().getNullString() ? strlen(myTdb().getNullString()) : 0);
  if (myTdb().getIsHiveInsert())
  {
    recSepLen = 1;
    delimLen = 1;
  }
  if (getEmptyNullString()) //covers hive null case also
    nullLen = 0;

  ExOperStats *stats = NULL;
  ExFastExtractStats *feStats = getFastExtractStats();
  ExHdfsScanStats *hdfsStats = getHdfsScanStats();

  while (TRUE)
  {
    // if no parent request, return
    if (qParent_.down->isEmpty())
      return WORK_OK;

    ex_queue_entry *pentry_down = qParent_.down->getHeadEntry();
    const ex_queue::down_request request = pentry_down->downState.request;
    const Lng32 value = pentry_down->downState.requestValue;
    ExFastExtractPrivateState &pstate = *((ExFastExtractPrivateState *) pentry_down->pstate);
    switch (pstate.step_)
    {
    case EXTRACT_NOT_STARTED:
    {
      pstate.step_ = EXTRACT_INITIALIZE;
    }
    break;
    case EXTRACT_INITIALIZE:
    {
      pstate.processingStarted_ = FALSE;
      errorOccurred_ = FALSE;

      //Allocate writeBuffers.
      numBuffers_ = 1;
      for (Int16 i = 0; i < numBuffers_; i++)
      {
        bool done = false;
        Int64 input_datalen = myTdb().getHdfsIoBufferSize();
        char * buf_addr = 0;
        while ((!done) && input_datalen >= 32 * 1024)
        {
          buf_addr = 0;
          buf_addr = (char *)((NAHeap *)heap_)->allocateAlignedHeapMemory((UInt32)input_datalen, 512, FALSE);
          if (buf_addr)
          {
            done = true;
            bufferPool_[i] = new (heap_) IOBuffer((char*) buf_addr, (Int32)input_datalen);
          }
          else
          {
            bufferAllocFailuresCount_++;
            input_datalen = input_datalen / 2;
          }
        }
        if (!done)
        {
          numBuffers_ = i;
          break ; // if too few buffers have been allocated we will raise
        }         // an error later
      }

      if (feStats)
      {
        feStats->setBufferAllocFailuresCount(bufferAllocFailuresCount_);
        feStats->setBuffersCount(numBuffers_);
      }


      ComDiagsArea *da = NULL;

      if (myTdb().getTargetFile() )
        {
          Lng32 fileNum = getGlobals()->castToExExeStmtGlobals()->getMyInstanceNumber();
          memset (hdfsHost_, '\0', sizeof(hdfsHost_));
          strncpy(hdfsHost_, myTdb().getHdfsHostName(), sizeof(hdfsHost_));
          hdfsPort_ = myTdb().getHdfsPortNum();
          memset (fileName_, '\0', sizeof(fileName_));
          memset (targetLocation_, '\0', sizeof(targetLocation_));
          snprintf(targetLocation_,999, "%s", myTdb().getTargetName());
         	 
          char pt[80]="";	
          char usec_buf[6]="";  	
          struct timeval tmnow;
          struct tm *currgmtime;
          gettimeofday(&tmnow, NULL);
          currgmtime = gmtime(&tmnow.tv_sec);
          strftime(pt, sizeof(pt), "%Y%m%d%H%M%S", currgmtime);
          sprintf(usec_buf,"%d",(int)tmnow.tv_usec);
          strcat(pt,usec_buf);
          srand(getpid());
          	                 
          if (myTdb().getIsHiveInsert())
            snprintf(fileName_,999, "%s%d-%s-%d", myTdb().getHiveTableName(), fileNum, pt,rand() % 1000);
          else
            snprintf(fileName_,999, "%s%d-%s-%d", "file", fileNum, pt,rand() % 1000);

          if (isSequenceFile() && sequenceFileWriter_ == NULL)
          {
            sequenceFileWriter_ = new(getHeap())
                                     SequenceFileWriter((NAHeap *)getHeap());
            sfwRetCode = sequenceFileWriter_->init();
            if (sfwRetCode != SFW_OK)
            {
              createSequenceFileError(sfwRetCode);
              pstate.step_ = EXTRACT_ERROR;
              break;
            }
          }
          else if (!isSequenceFile() && hdfsClient_ == NULL)
          {
             hdfsClient_ = HdfsClient::newInstance((NAHeap *)getHeap(), (ExHdfsScanStats *)getHdfsScanStats(), hdfsClientRetCode);
             if (hdfsClientRetCode != HDFS_CLIENT_OK)
             {
                createHdfsClientFileError(hdfsClientRetCode);
                pstate.step_ = EXTRACT_ERROR;
                break;
             }
          }

          strcat(targetLocation_, "//");
          strcat(targetLocation_, fileName_);
          if (isSequenceFile())
          {
            sfwRetCode = sequenceFileWriter_->open(targetLocation_, SFW_COMP_NONE);
            if (sfwRetCode != SFW_OK)
            {
              createSequenceFileError(sfwRetCode);
              pstate.step_ = EXTRACT_ERROR;
              break;
            }
          }
          else
          {
             hdfsClientRetCode = hdfsClient_->hdfsOpen(targetLocation_, isHdfsCompressed());
             if (hdfsClientRetCode != HDFS_CLIENT_OK)
             {
                createHdfsClientFileError(hdfsClientRetCode);
                NADELETE(hdfsClient_,
                       HdfsClient,
                       heap_);
                hdfsClient_ = NULL;
                pstate.step_ = EXTRACT_ERROR;
                break;
             }
           }
           if (feStats)
           {
             feStats->setPartitionNumber(fileNum);
           }
       }
       else
       {
           updateWorkATPDiagsArea(__FILE__,__LINE__,"sockets are not supported");
           pstate.step_ = EXTRACT_ERROR;
           break;
       }

      for (UInt32 i = 0; i < myTdb().getChildTuple()->numAttrs(); i++)
      {
        Attributes * attr = myTdb().getChildTableAttr(i);
        Attributes * attr2 = myTdb().getChildTableAttr2(i);

        ex_conv_clause tempClause;
        int convIndex = 0;
        sourceFieldsConvIndex_[i] =
            tempClause.findInstruction(
                attr->getDatatype(),
                0,
                attr2->getDatatype(),
                0,
                0);

      }

      pstate.step_= EXTRACT_PASS_REQUEST_TO_CHILD;
    }
    break;

    case EXTRACT_PASS_REQUEST_TO_CHILD:
    {
      // pass the parent request to the child downqueue
      if (!qChild_.down->isFull())
      {
        ex_queue_entry * centry = qChild_.down->getTailEntry();

        if (request == ex_queue::GET_N)
          centry->downState.request = ex_queue::GET_ALL;
        else
          centry->downState.request = request;

        centry->downState.requestValue = pentry_down->downState.requestValue;
        centry->downState.parentIndex = qParent_.down->getHeadIndex();
        // set the child's input atp
        centry->passAtp(pentry_down->getAtp());
        qChild_.down->insert();
        pstate.processingStarted_ = TRUE;
      }
      else
        // couldn't pass request to child, return
        return WORK_OK;

      pstate.step_ = EXTRACT_RETURN_ROWS_FROM_CHILD;
    }
    break;
    case EXTRACT_RETURN_ROWS_FROM_CHILD:
    {
      if ((qChild_.up->isEmpty()))
      {
        return WORK_OK;
      }

      if (currBuffer_ == NULL)
      {
        currBuffer_ = bufferPool_[0];
        memset(currBuffer_->data_, '\0',currBuffer_->bufSize_);
        currBuffer_->bytesLeft_ = currBuffer_->bufSize_;
      }

      ex_queue_entry * centry = qChild_.up->getHeadEntry();
      ComDiagsArea *cda = NULL;
      ex_queue::up_status child_status = centry->upState.status;

      switch (child_status)
      {
      case ex_queue::Q_OK_MMORE:
      {
        // for the very first row retruned from child
        // include the header row if necessary
        if ((pstate.matchCount_ == 0) && myTdb().getIncludeHeader())
        {
          if (!myTdb().getIsAppend())
          {
            Int32 headerLength = strlen(myTdb().getHeader());
            char * target = currBuffer_->data_;
            if (headerLength + 1 < currBuffer_->bufSize_)
            {
              strncpy(target, myTdb().getHeader(),headerLength);
              target[headerLength] = '\n' ;
              currBuffer_->bytesLeft_ -= headerLength+1 ;
            }
            else
            {
              updateWorkATPDiagsArea(__FILE__,__LINE__,"header does not fit in buffer");
              pstate.step_ = EXTRACT_ERROR;
              break;
            }
          }
        }

        tupp_descriptor *dataDesc = childOutputTD_;
        ex_expr::exp_return_type expStatus = ex_expr::EXPR_OK;
        if (myTdb().getChildDataExpr())
        {
          UInt32 childTuppIndex = myTdb().childDataTuppIndex_;

          workAtp_->getTupp(childTuppIndex) = dataDesc;

          // Evaluate the child data expression. If diags are generated they
          // will be left in the down entry ATP.
          expStatus = myTdb().getChildDataExpr()->eval(centry->getAtp(), workAtp_);
          workAtp_->getTupp(childTuppIndex).release();

          if (expStatus == ex_expr::EXPR_ERROR)
          {
            if (myTdb().getContinueOnError())
              {
                // ignore this row and continue to the next row
                if (workAtp_->getDiagsArea())
                  workAtp_->getDiagsArea()->clear();
                break;
              }
            else
              {
                updateWorkATPDiagsArea(centry);
                pstate.step_ = EXTRACT_ERROR;
                break;
              }
          }
        } // if (myTdb().getChildDataExpr())
        ///////////////////////
        char * targetData = currBuffer_->data_ + currBuffer_->bufSize_ - currBuffer_->bytesLeft_;
        if (targetData == NULL)
        {
          updateWorkATPDiagsArea(__FILE__,__LINE__,"targetData is NULL");
          pstate.step_ = EXTRACT_ERROR;
          break;
        }
        NABoolean convError = FALSE;
        convertSQRowToString(nullLen, recSepLen, delimLen, dataDesc,
            targetData, convError);
        ///////////////////////////////
        pstate.matchCount_++;
        if (!convError)
        {
          if (feStats)
          {
            feStats->incProcessedRowsCount();
          }
              if (hdfsStats != NULL) {
                 hdfsStats->incUsedRows();
                 hdfsStats->incAccessedRows();
              }
          pstate.successRowCount_ ++;
        }
        else
        {
          if (feStats)
          {
            feStats->incErrorRowsCount();
          }
              if (hdfsStats != NULL) 
                 hdfsStats->incAccessedRows();
          pstate.errorRowCount_ ++;
        }
        if (currBuffer_->bytesLeft_ < (Int32) maxExtractRowLength_)
        {
          pstate.step_ = EXTRACT_DATA_READY_TO_SEND;
        }
      }
      break;

      case ex_queue::Q_NO_DATA:
      {
        pstate.step_ = EXTRACT_DATA_READY_TO_SEND;
        endOfData_ = TRUE;
        pstate.processingStarted_ = FALSE ; // so that cancel does not
        //wait for this Q_NO_DATA
      }
      break;
      case ex_queue::Q_SQLERROR:
      {
        if ((centry->getDiagsArea()) &&
            (!pentry_down->getDiagsArea()))
          {
            ComDiagsArea *diagsArea = pentry_down->getDiagsArea();
            diagsArea = ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
            pentry_down->setDiagsArea(diagsArea);
            pentry_down->getDiagsArea()->mergeAfter(*centry->getDiagsArea());
          }

        pstate.step_ = EXTRACT_ERROR;
      }
      break;
      case ex_queue::Q_INVALID:
      {
        updateWorkATPDiagsArea(__FILE__,__LINE__,
            "ExFastExtractTcb::work() Invalid state returned by child");
        pstate.step_ = EXTRACT_ERROR;
      }
      break;

      } // switch
      qChild_.up->removeHead();
    }
    break;

    case EXTRACT_DATA_READY_TO_SEND:
    {
      ssize_t bytesToWrite = currBuffer_->bufSize_ - currBuffer_->bytesLeft_;

      if (isSequenceFile())
        {
          sfwRetCode = sequenceFileWriter_->writeBuffer(currBuffer_->data_, bytesToWrite, myTdb().getRecordSeparator());
          if (sfwRetCode != SFW_OK)
          {
            createSequenceFileError(sfwRetCode);
            pstate.step_ = EXTRACT_ERROR;
            break;
          }
        }
      else
        {
          hdfsClient_->hdfsWrite(currBuffer_->data_, bytesToWrite, hdfsClientRetCode);
          if (hdfsClientRetCode != HDFS_CLIENT_OK)
          {
            createSequenceFileError(hdfsClientRetCode);
            pstate.step_ = EXTRACT_ERROR;
            break;
          }
        }

      if (feStats)
      {
        feStats->incReadyToSendBuffersCount();
        feStats->incReadyToSendBytes(currBuffer_->bufSize_ - currBuffer_->bytesLeft_);
      }
      currBuffer_ = NULL;

      if (endOfData_)
      {
        pstate.step_ = EXTRACT_DONE;
      }
      else
      {
        pstate.step_ = EXTRACT_RETURN_ROWS_FROM_CHILD;
      }
    }
    break;

    case EXTRACT_ERROR:
    {
      // If there is no room in the parent queue for the reply,
      // try again later.
      //Later we may split this state into 2 one for cancel and one for query
      if (qParent_.up->isFull())
        return WORK_OK;
      // Cancel the child request - there must be a child request in
      // progress to get to the ERROR state.
      if (pstate.processingStarted_)
      {
        qChild_.down->cancelRequestWithParentIndex(qParent_.down->getHeadIndex());
        //pstate.processingStarted_ = FALSE;
      }
      while (pstate.processingStarted_ && pstate.step_ == EXTRACT_ERROR)
      {
        if (qChild_.up->isEmpty())
          return WORK_OK;
        ex_queue_entry * childEntry = qChild_.up->getHeadEntry();
        ex_queue::up_status childStatus = childEntry->upState.status;

        if (childStatus == ex_queue::Q_NO_DATA)
        {
          pstate.step_ = EXTRACT_DONE;
          pstate.processingStarted_ = FALSE;
        }
        qChild_.up->removeHead();
      }
      ex_queue_entry *pentry_up = qParent_.up->getTailEntry();
      pentry_up->copyAtp(pentry_down);
      // Construct and return the error row.
      //
      if (workAtp_->getDiagsArea())
      {
        ComDiagsArea *diagsArea = pentry_up->getDiagsArea();
        if (diagsArea == NULL)
        {
          diagsArea = ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
          pentry_up->setDiagsArea(diagsArea);
        }
        pentry_up->getDiagsArea()->mergeAfter(*workAtp_->getDiagsArea());
        workAtp_->setDiagsArea(NULL);
      }
      pentry_up->upState.status = ex_queue::Q_SQLERROR;
      pentry_up->upState.parentIndex
      = pentry_down->downState.parentIndex;
      pentry_up->upState.downIndex = qParent_.down->getHeadIndex();
      pentry_up->upState.setMatchNo(pstate.matchCount_);
      qParent_.up->insert();
      //
      errorOccurred_ = TRUE;
      pstate.step_ = EXTRACT_DONE;
    }
    break;

    case EXTRACT_DONE:
    {
      // If there is no room in the parent queue for the reply,
      // try again later.
      //
      if (qParent_.up->isFull())
        return WORK_OK;

      if (isSequenceFile())
      {
         if (sequenceFileWriter_) 
         {
            sfwRetCode = sequenceFileWriter_->close();
            if (!errorOccurred_ && sfwRetCode != SFW_OK )
            {
               createSequenceFileError(sfwRetCode);
               pstate.step_ = EXTRACT_ERROR;
               break;
            }
         }
      }
      else
      {
         if (hdfsClient_)
         {
            hdfsClientRetCode = hdfsClient_->hdfsClose();
            if (!errorOccurred_ && HDFS_CLIENT_OK != HDFS_CLIENT_OK )
            {
               createHdfsClientFileError(hdfsClientRetCode);
               pstate.step_ = EXTRACT_ERROR;
               break;
            }
         }   
      }

      //insertUpQueueEntry will insert Q_NO_DATA into the up queue and
      //remove the head of the down queue
      insertUpQueueEntry(ex_queue::Q_NO_DATA, NULL, TRUE);
      errorOccurred_ = FALSE;

      endOfData_ = FALSE;

      //we need to set the next state so that the query can get re-executed
      //and we start from the beginning again. Not sure if pstate will be
      //valid anymore because insertUpQueueEntry() might have cleared it
      //already.
      pstate.step_ = EXTRACT_NOT_STARTED;

      //exit out now and not break.
      return WORK_OK;
    }
    break;

    default:
    {
      ex_assert(FALSE, "Invalid state in  ExHdfsFastExtractTcb ");
    }

    break;

    } // switch(pstate.step_)
  } // while

  return WORK_OK;
}//ExHdfsFastExtractTcb::work()

void ExHdfsFastExtractTcb::insertUpQueueEntry(ex_queue::up_status status, ComDiagsArea *diags, NABoolean popDownQueue)
{

  ex_queue_entry *upEntry = qParent_.up->getTailEntry();
  ex_queue_entry *downEntry = qParent_.down->getHeadEntry();
  ExFastExtractPrivateState &privateState = *((ExFastExtractPrivateState *) downEntry->pstate);

  // Initialize the up queue entry.
  //
  // copyAtp() will copy all tuple pointers and the diags area from
  // the down queue entry to the up queue entry.
  //
  // When we return Q_NO_DATA if the match count is > 0:
  // * assume down queue diags were returned with the Q_OK_MMORE entries
  // * release down queue diags before copyAtp()
  //
  if (status == ex_queue::Q_NO_DATA && privateState.matchCount_ > 0)
  {
    downEntry->setDiagsArea(NULL);
    upEntry->copyAtp(downEntry);
  }
  else
  {
    upEntry->copyAtp(downEntry);
    downEntry->setDiagsArea(NULL);
  }

  upEntry->upState.status = status;
  upEntry->upState.parentIndex = downEntry->downState.parentIndex;
  upEntry->upState.downIndex = qParent_.down->getHeadIndex();
  upEntry->upState.setMatchNo(privateState.matchCount_);

  // rows affected code (below) medeled after ex_partn_access.cpp
  ExMasterStmtGlobals *g = getGlobals()->castToExExeStmtGlobals()->castToExMasterStmtGlobals();
  if (!g)
  {
    ComDiagsArea *da = upEntry->getDiagsArea();
    if (da == NULL)
    {
      da = ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
      upEntry->setDiagsArea(da);
    }
    da->addRowCount(privateState.matchCount_);
  }
  else
  {
    g->setRowsAffected(privateState.matchCount_);
  }

  //
  // Insert into up queue
  qParent_.up->insert();

  // Optionally remove the head of the down queue
  if (popDownQueue)
  {
    privateState.init();
    qParent_.down->removeHead();
  }
}

NABoolean ExHdfsFastExtractTcb::isSequenceFile()
{
  return myTdb().getIsSequenceFile();
}
NABoolean ExHdfsFastExtractTcb::isHdfsCompressed()
{
  return myTdb().getHdfsCompressed();
}


void ExHdfsFastExtractTcb::createSequenceFileError(Int32 sfwRetCode)
{
  ContextCli *currContext = GetCliGlobals()->currContext();

  ComDiagsArea * diagsArea = NULL;
  char* errorMsg = sequenceFileWriter_->getErrorText((SFW_RetCode)sfwRetCode);
  ExRaiseSqlError(getHeap(),
                  &diagsArea,
                  (ExeErrorCode)(8447),
                  NULL, NULL, NULL, NULL,
                  errorMsg,
                (char *)GetCliGlobals()->getJniErrorStr());
  //ex_queue_entry *pentry_down = qParent_.down->getHeadEntry();
  //pentry_down->setDiagsArea(diagsArea);
  updateWorkATPDiagsArea(diagsArea);
}

void ExHdfsFastExtractTcb::createHdfsClientFileError(Int32 hdfsClientRetCode)
{
  ContextCli *currContext = GetCliGlobals()->currContext();

  ComDiagsArea * diagsArea = NULL;
  char* errorMsg = hdfsClient_->getErrorText((HDFS_Client_RetCode)hdfsClientRetCode);
  ExRaiseSqlError(getHeap(),
                  &diagsArea,
                  (ExeErrorCode)(8447),
                  NULL, NULL, NULL, NULL,
                  errorMsg,
                  (char *)GetCliGlobals()->getJniErrorStr());
  updateWorkATPDiagsArea(diagsArea);
}

ExFastExtractPrivateState::ExFastExtractPrivateState()
{
  init();
}

ExFastExtractPrivateState::~ExFastExtractPrivateState()
{
}

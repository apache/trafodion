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

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <poll.h>

#include <iostream>

#include "ex_stdh.h"
#include "ComTdb.h"
#include "ex_tcb.h"
#include "ExHdfsScan.h"
#include "ex_exe_stmt_globals.h"
#include "ExpLOBinterface.h"
#include "SequenceFileReader.h" 
#include "Hbase_types.h"
#include "stringBuf.h"
#include "NLSConversion.h"
#include "Context.h"

#include "ExpORCinterface.h"
#include "ComSmallDefs.h"

ex_tcb * ExHdfsScanTdb::build(ex_globals * glob)
{
  ExExeStmtGlobals * exe_glob = glob->castToExExeStmtGlobals();
  
  ex_assert(exe_glob,"This operator cannot be in DP2");

  ExHdfsScanTcb *tcb = NULL;
  
  if ((isTextFile()) || (isSequenceFile()))
    {
      tcb = new(exe_glob->getSpace()) 
        ExHdfsScanTcb(
                      *this,
                      exe_glob);
    }
  else if (isOrcFile())
    {
      tcb = new(exe_glob->getSpace()) 
        ExOrcScanTcb(
                     *this,
                     exe_glob);
    }

  ex_assert(tcb, "Error building ExHdfsScanTcb.");

  return (tcb);
}

ex_tcb * ExOrcFastAggrTdb::build(ex_globals * glob)
{
  ExHdfsScanTcb *tcb = NULL;
  tcb = new(glob->getSpace()) 
    ExOrcFastAggrTcb(
                     *this,
                     glob);
  
  ex_assert(tcb, "Error building ExHdfsScanTcb.");

  return (tcb);
}

////////////////////////////////////////////////////////////////
// Constructor and initialization.
////////////////////////////////////////////////////////////////

ExHdfsScanTcb::ExHdfsScanTcb(
          const ComTdbHdfsScan &hdfsScanTdb, 
          ex_globals * glob ) :
  ex_tcb( hdfsScanTdb, 1, glob)
  , workAtp_(NULL)
  , bytesLeft_(0)
  , hdfsScanBuffer_(NULL)
  , hdfsBufNextRow_(NULL)
  , hdfsLoggingRow_(NULL)
  , hdfsLoggingRowEnd_(NULL)
  , debugPrevRow_(NULL)
  , hdfsSqlBuffer_(NULL)
  , hdfsSqlData_(NULL)
  , pool_(NULL)
  , step_(NOT_STARTED)
  , matches_(0)
  , matchBrkPoint_(0)
  , endOfRequestedRange_(NULL)
  , sequenceFileReader_(NULL)
  , seqScanAgain_(false)
  , hdfo_(NULL)
  , numBytesProcessedInRange_(0)
  , exception_(FALSE)
  , checkRangeDelimiter_(FALSE)
  , dataModCheckDone_(FALSE)
  , loggingErrorDiags_(NULL)
{
  Space * space = (glob ? glob->getSpace() : 0);
  CollHeap * heap = (glob ? glob->getDefaultHeap() : 0);

  const int readBufSize =  (Int32)hdfsScanTdb.hdfsBufSize_;
  hdfsScanBuffer_ = new(space) char[ readBufSize + 1 ]; 
  hdfsScanBuffer_[readBufSize] = '\0';

  moveExprColsBuffer_ = new(space) ExSimpleSQLBuffer( 1, // one row 
						      (Int32)hdfsScanTdb.moveExprColsRowLength_,
						      space);
  short error = moveExprColsBuffer_->getFreeTuple(moveExprColsTupp_);
  ex_assert((error == 0), "get_free_tuple cannot hold a row.");
  moveExprColsData_ = moveExprColsTupp_.getDataPointer();


  hdfsSqlBuffer_ = new(space) ExSimpleSQLBuffer( 1, // one row 
						 (Int32)hdfsScanTdb.hdfsSqlMaxRecLen_,
						 space);
  error = hdfsSqlBuffer_->getFreeTuple(hdfsSqlTupp_);
  ex_assert((error == 0), "get_free_tuple cannot hold a row.");
  hdfsSqlData_ = hdfsSqlTupp_.getDataPointer();

  hdfsAsciiSourceBuffer_ = new(space) ExSimpleSQLBuffer( 1, // one row 
							 (Int32)hdfsScanTdb.asciiRowLen_ * 2, // just in case
							 space);
  error = hdfsAsciiSourceBuffer_->getFreeTuple(hdfsAsciiSourceTupp_);
  ex_assert((error == 0), "get_free_tuple cannot hold a row.");
  hdfsAsciiSourceData_ = hdfsAsciiSourceTupp_.getDataPointer();

  pool_ = new(space) 
        sql_buffer_pool(hdfsScanTdb.numBuffers_,
        hdfsScanTdb.bufferSize_,
        space,
        ((ExHdfsScanTdb &)hdfsScanTdb).denseBuffers() ? 
        SqlBufferBase::DENSE_ : SqlBufferBase::NORMAL_);


  pool_->setStaticMode(TRUE);
        

  defragTd_ = NULL;
  // removing the cast produce a compile error
  if (((ExHdfsScanTdb &)hdfsScanTdb).useCifDefrag())
  {
    defragTd_ = pool_->addDefragTuppDescriptor(hdfsScanTdb.outputRowLength_);
  }
  // Allocate the queue to communicate with parent
  allocateParentQueues(qparent_);

  workAtp_ = allocateAtp(hdfsScanTdb.workCriDesc_, space);

  // fixup expressions
  if (selectPred())
    selectPred()->fixup(0, getExpressionMode(), this,  space, heap, FALSE, glob);
  if (moveExpr())
    moveExpr()->fixup(0, getExpressionMode(), this,  space, heap, FALSE, glob);
  if (convertExpr())
    convertExpr()->fixup(0, getExpressionMode(), this,  space, heap, FALSE, glob);
  if (moveColsConvertExpr())
    moveColsConvertExpr()->fixup(0, getExpressionMode(), this,  space, heap, FALSE, glob);


  // Register subtasks with the scheduler
  registerSubtasks();
  registerResizeSubtasks();

  Lng32 fileNum = getGlobals()->castToExExeStmtGlobals()->getMyInstanceNumber();
  ExHbaseAccessTcb::buildLoggingPath(((ExHdfsScanTdb &)hdfsScanTdb).getLoggingLocation(),
                     (char *)((ExHdfsScanTdb &)hdfsScanTdb).getErrCountRowId(),
                     ((ExHdfsScanTdb &)hdfsScanTdb).tableName(),
                     "hive_scan_err",
                     fileNum,
                     loggingFileName_);
  LoggingFileCreated_ = FALSE;

  
  //shoud be move to work method
  int jniDebugPort = 0;
  int jniDebugTimeout = 0;
  ehi_ = ExpHbaseInterface::newInstance(glob->getDefaultHeap(),
                                        (char*)"",  //Later replace with server cqd
                                        (char*)"", ////Later replace with port cqd
                                        jniDebugPort,
                                        jniDebugTimeout);

}
    
ExHdfsScanTcb::~ExHdfsScanTcb()
{
  freeResources();
}

void ExHdfsScanTcb::freeResources()
{
  if (workAtp_)
  {
    workAtp_->release();
    deallocateAtp(workAtp_, getSpace());
    workAtp_ = NULL;
  }
  if (hdfsScanBuffer_)
  {
    NADELETEBASIC(hdfsScanBuffer_, getSpace());
    hdfsScanBuffer_ = NULL;
  }
  if (hdfsAsciiSourceBuffer_)
  {
    NADELETEBASIC(hdfsAsciiSourceBuffer_, getSpace());
    hdfsAsciiSourceBuffer_ = NULL;
  }
 
  // hdfsSqlTupp_.release() ; // ??? 
  if (hdfsSqlBuffer_)
  {
    delete hdfsSqlBuffer_;
    hdfsSqlBuffer_ = NULL;
  }
  if (moveExprColsBuffer_)
  {
    delete moveExprColsBuffer_;
    moveExprColsBuffer_ = NULL;
  }
  if (pool_)
  {
    delete pool_;
    pool_ = NULL;
  }
  if (qparent_.up)
  {
    delete qparent_.up;
    qparent_.up = NULL;
  }
  if (qparent_.down)
  {
    delete qparent_.down;
    qparent_.down = NULL;
  }

   ExpLOBinterfaceCleanup
   (lobGlob_, getGlobals()->getDefaultHeap());

  
}
NABoolean ExHdfsScanTcb::needStatsEntry()
{
  // stats are collected for ALL and OPERATOR options.
  if ((getGlobals()->getStatsArea()->getCollectStatsType() == 
       ComTdb::ALL_STATS) ||
      (getGlobals()->getStatsArea()->getCollectStatsType() == 
      ComTdb::OPERATOR_STATS))
    return TRUE;
  else if ( getGlobals()->getStatsArea()->getCollectStatsType() == ComTdb::PERTABLE_STATS)
    return TRUE;
  else
    return FALSE;
}

ExOperStats * ExHdfsScanTcb::doAllocateStatsEntry(
                                                        CollHeap *heap,
                                                        ComTdb *tdb)
{
  ExOperStats * stats = NULL;

  ExHdfsScanTdb * myTdb = (ExHdfsScanTdb*) tdb;
  
  return new(heap) ExHdfsScanStats(heap,
				   this,
				   tdb);
  
  ComTdb::CollectStatsType statsType = 
                     getGlobals()->getStatsArea()->getCollectStatsType();
  if (statsType == ComTdb::OPERATOR_STATS)
    {
      return ex_tcb::doAllocateStatsEntry(heap, tdb);
    }
  else if (statsType == ComTdb::PERTABLE_STATS)
    {
      // sqlmp style per-table stats, one entry per table
      stats = new(heap) ExPertableStats(heap, 
					this,
					tdb);
      ((ExOperStatsId*)(stats->getId()))->tdbId_ = tdb->getPertableStatsTdbId();
      return stats;
    }
  else
    {
      ExHdfsScanTdb * myTdb = (ExHdfsScanTdb*) tdb;
      
      return new(heap) ExHdfsScanStats(heap,
				       this,
				       tdb);
    }
}

void ExHdfsScanTcb::registerSubtasks()
{
  ExScheduler *sched = getGlobals()->getScheduler();

  sched->registerInsertSubtask(sWork,   this, qparent_.down,"PD");
  sched->registerUnblockSubtask(sWork,    this, qparent_.up,  "PU");
  sched->registerCancelSubtask(sWork,     this, qparent_.down,"CN");

}

ex_tcb_private_state *ExHdfsScanTcb::allocatePstates(
     Lng32 &numElems,      // inout, desired/actual elements
     Lng32 &pstateLength)  // out, length of one element
{
  PstateAllocator<ex_tcb_private_state> pa;
  return pa.allocatePstates(this, numElems, pstateLength);
}

Int32 ExHdfsScanTcb::fixup()
{
  lobGlob_ = NULL;

  ExpLOBinterfaceInit
    (lobGlob_, getGlobals()->getDefaultHeap(),getGlobals()->castToExExeStmtGlobals()->getContext(),TRUE, hdfsScanTdb().hostName_,hdfsScanTdb().port_);
  
  return 0;
}

void brkpoint()
{}

short ExHdfsScanTcb::setupError(Lng32 exeError, Lng32 retcode, 
                                const char * str, const char * str2, const char * str3)
{
  // Make sure retcode is positive.
  if (retcode < 0)
    retcode = -retcode;
    
  ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();
  
  Lng32 intParam1 = retcode;
  Lng32 intParam2 = 0;
  ComDiagsArea * diagsArea = NULL;
  ExRaiseSqlError(getHeap(), &diagsArea, 
                  (ExeErrorCode)(exeError), NULL, &intParam1, 
                  &intParam2, NULL, 
                  (str ? (char*)str : (char*)" "),
                  (str2 ? (char*)str2 : (char*)" "),
                  (str3 ? (char*)str3 : (char*)" "));
                  
  pentry_down->setDiagsArea(diagsArea);
  return -1;
}

ExWorkProcRetcode ExHdfsScanTcb::work()
{
  Lng32 retcode = 0;
  SFR_RetCode sfrRetCode = SFR_OK;
  char *errorDesc = NULL;
  char cursorId[8];
  HdfsFileInfo *hdfo = NULL;
  Lng32 openType = 0;
  int changedLen = 0;
 ContextCli *currContext = getGlobals()->castToExExeStmtGlobals()->getCliGlobals()->currContext();
   hdfsFS hdfs = currContext->getHdfsServerConnection(hdfsScanTdb().hostName_,hdfsScanTdb().port_);
   hdfsFileInfo *dirInfo = NULL;
   Int32 hdfsErrorDetail = 0;//this is errno returned form underlying hdfsOpenFile call.
  while (!qparent_.down->isEmpty())
    {
      ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();
      switch (step_)
	{
	case NOT_STARTED:
	  {
	    matches_ = 0;
	    
	    hdfsStats_ = NULL;
	    if (getStatsEntry())
	      hdfsStats_ = getStatsEntry()->castToExHdfsScanStats();

	    ex_assert(hdfsStats_, "hdfs stats cannot be null");

	    if (hdfsStats_)
	      hdfsStats_->init();

	    beginRangeNum_ = -1;
	    numRanges_ = -1;
	    hdfsOffset_ = 0;
            checkRangeDelimiter_ = FALSE;

            dataModCheckDone_ = FALSE;

	    if (hdfsScanTdb().getHdfsFileInfoList()->isEmpty())
	      {
                step_ = CHECK_FOR_DATA_MOD_AND_DONE;
		break;
	      }

	    myInstNum_ = getGlobals()->getMyInstanceNumber();

	    beginRangeNum_ =  
	      *(Lng32*)hdfsScanTdb().getHdfsFileRangeBeginList()->get(myInstNum_);

	    numRanges_ =  
	      *(Lng32*)hdfsScanTdb().getHdfsFileRangeNumList()->get(myInstNum_);

	    currRangeNum_ = beginRangeNum_;

	    hdfsScanBufMaxSize_ = hdfsScanTdb().hdfsBufSize_;

	    if (numRanges_ > 0)
              step_ = CHECK_FOR_DATA_MOD;
            else
              step_ = CHECK_FOR_DATA_MOD_AND_DONE;
	  }          
	  break;

        case CHECK_FOR_DATA_MOD:
        case CHECK_FOR_DATA_MOD_AND_DONE:
          {
            char * dirPath = hdfsScanTdb().hdfsRootDir_;
            Int64 modTS = hdfsScanTdb().modTSforDir_;
            if ((dirPath == NULL) || (modTS == -1))
              dataModCheckDone_ = TRUE;

            if (NOT dataModCheckDone_)
              {
                dataModCheckDone_ = TRUE;

                Lng32 numOfPartLevels = hdfsScanTdb().numOfPartCols_;

                if (hdfsScanTdb().hdfsDirsToCheck())
                  {
                    // TBD
                  }
             
                Int64 failedModTS = -1;
                retcode = ExpLOBinterfaceDataModCheck
                  (lobGlob_,
                   dirPath,
                   hdfsScanTdb().hostName_,
                   hdfsScanTdb().port_,
                   modTS,
                   numOfPartLevels,
                   failedModTS);
              
                if (retcode < 0)
                  {
                    Lng32 cliError = 0;
		    
                    Lng32 intParam1 = -retcode;
                    ComDiagsArea * diagsArea = NULL;
                    ExRaiseSqlError(getHeap(), &diagsArea, 
                                    (ExeErrorCode)(EXE_ERROR_FROM_LOB_INTERFACE),
                                    NULL, &intParam1, 
                                    &cliError, 
                                    NULL, 
                                    "HDFS",
                                    (char*)"ExpLOBInterfaceDataModCheck",
                                    getLobErrStr(intParam1));
                    pentry_down->setDiagsArea(diagsArea);
                    step_ = HANDLE_ERROR_AND_DONE;
                    break;
                  }  

                if (retcode == 1) // check failed
                  {
                    char errStr[200];
                    str_sprintf(errStr, "genModTS = %Ld, failedModTS = %Ld", 
                                modTS, failedModTS);

                    ComDiagsArea * diagsArea = NULL;
                    ExRaiseSqlError(getHeap(), &diagsArea, 
                                    (ExeErrorCode)(EXE_HIVE_DATA_MOD_CHECK_ERROR), NULL,
                                    NULL, NULL, NULL,
                                    errStr);
                    pentry_down->setDiagsArea(diagsArea);
                    step_ = HANDLE_ERROR_AND_DONE;
                    break;
                  }
              }

            if (step_ == CHECK_FOR_DATA_MOD_AND_DONE)
              step_ = DONE;
            else
              step_ = INIT_HDFS_CURSOR;
          }        
          break;

	case INIT_HDFS_CURSOR:
	  {
            hdfo_ = (HdfsFileInfo*)
              hdfsScanTdb().getHdfsFileInfoList()->get(currRangeNum_);
            if ((hdfo_->getBytesToRead() == 0) && 
                (beginRangeNum_ == currRangeNum_) && (numRanges_ > 1))
              {
                // skip the first range if it has 0 bytes to read
                // doing this for subsequent ranges is more complex
                // since the file may neeed to be closed. The first 
                // range being 0 is common with sqoop generated files
                currRangeNum_++;
                hdfo_ = (HdfsFileInfo*)
                  hdfsScanTdb().getHdfsFileInfoList()->get(currRangeNum_);
              }
               
            hdfsOffset_ = hdfo_->getStartOffset();
            bytesLeft_ = hdfo_->getBytesToRead();

            hdfsFileName_ = hdfo_->fileName();
            sprintf(cursorId_, "%d", currRangeNum_);
            stopOffset_ = hdfsOffset_ + hdfo_->getBytesToRead();

	    step_ = OPEN_HDFS_CURSOR;
	  }
        
	  break;

	case OPEN_HDFS_CURSOR:
	  {
	    retcode = 0;
	    if (isSequenceFile() && !sequenceFileReader_)
	      {
	        sequenceFileReader_ = new(getSpace()) 
                    SequenceFileReader((NAHeap *)getSpace());
	        sfrRetCode = sequenceFileReader_->init();
	        
	        if (sfrRetCode != JNI_OK)
	          {
    		      ComDiagsArea * diagsArea = NULL;
    		      ExRaiseSqlError(getHeap(), &diagsArea, (ExeErrorCode)(8447), NULL, 
    		      		  NULL, NULL, NULL, sequenceFileReader_->getErrorText(sfrRetCode), NULL);
    		      pentry_down->setDiagsArea(diagsArea);
    		      step_ = HANDLE_ERROR;
    		      break;
	          }
	      }
	    
	    if (isSequenceFile())
	      {
	        sfrRetCode = sequenceFileReader_->open(hdfsFileName_);
	        
	        if (sfrRetCode == JNI_OK)
	          {
	            // Seek to start offset
	            sfrRetCode = sequenceFileReader_->seeknSync(hdfsOffset_);
	          }
	        
	        if (sfrRetCode != JNI_OK)
	          {
    		    ComDiagsArea * diagsArea = NULL;
    		    ExRaiseSqlError(getHeap(), &diagsArea, (ExeErrorCode)(8447), NULL, 
    		    		  NULL, NULL, NULL, sequenceFileReader_->getErrorText(sfrRetCode), NULL);
    		    pentry_down->setDiagsArea(diagsArea);
    		    step_ = HANDLE_ERROR;
    		    break;
	          }
	      }
	    else
	      {
                Int64 rangeTail  = hdfo_->fileIsSplitEnd() ? 
                  hdfsScanTdb().rangeTailIOSize_ : 0;
                openType = 2; // must open
                retcode = ExpLOBInterfaceSelectCursor
                  (lobGlob_,
                   hdfsFileName_, //hdfsScanTdb().hdfsFileName_,
                   NULL, //(char*)"",
                   (Lng32)Lob_External_HDFS_File,
                   hdfsScanTdb().hostName_,
                   hdfsScanTdb().port_,
                   0, NULL, // handle not valid for non lob access
                   bytesLeft_ + rangeTail, // max bytes
                   cursorId_, 
		       
                   requestTag_, Lob_Memory,
                   0, // not check status
                   (NOT hdfsScanTdb().hdfsPrefetch()),  //1, // waited op
		       
                   hdfsOffset_, 
                   hdfsScanBufMaxSize_,
                   bytesRead_,
                   NULL,
                   1, // open
                   openType, //
                   &hdfsErrorDetail
                   );
                
                if ((retcode < 0) &&
                    ((hdfsErrorDetail == ENOENT) || (hdfsErrorDetail == EAGAIN)))
                  {
                    ComDiagsArea * diagsArea = NULL;
                    ExRaiseSqlError(getHeap(), &diagsArea, 
                                    (ExeErrorCode)(EXE_HIVE_DATA_MOD_CHECK_ERROR));
                    pentry_down->setDiagsArea(diagsArea);
                    step_ = HANDLE_ERROR_AND_DONE;
                    break;
                  }

                // preopen next range. 
                if ( (currRangeNum_ + 1) < (beginRangeNum_ + numRanges_) ) 
                  {
                    hdfo = (HdfsFileInfo*)
                      hdfsScanTdb().getHdfsFileInfoList()->get(currRangeNum_ + 1);
                
                    hdfsFileName_ = hdfo->fileName();
                    sprintf(cursorId, "%d", currRangeNum_ + 1);
		    rangeTail  = hdfo->fileIsSplitEnd() ? 
		      hdfsScanTdb().rangeTailIOSize_ : 0;

                    openType = 1; // preOpen
                
                    retcode = ExpLOBInterfaceSelectCursor
                      (lobGlob_,
                       hdfsFileName_, //hdfsScanTdb().hdfsFileName_,
                       NULL, //(char*)"",
                       (Lng32)Lob_External_HDFS_File,
                       hdfsScanTdb().hostName_,
                       hdfsScanTdb().port_,
                       0, NULL,//handle not relevant for non lob access
                       hdfo->getBytesToRead() + rangeTail, // max bytes
                       cursorId, 
                           
                       requestTag_, Lob_Memory,
                       0, // not check status
                       (NOT hdfsScanTdb().hdfsPrefetch()),  //1, // waited op
                           
                       hdfo->getStartOffset(), 
                       hdfsScanBufMaxSize_,
                       bytesRead_,
                       NULL,
                       1,// open
                       openType,
                       &hdfsErrorDetail
                       );
                
                    hdfsFileName_ = hdfo_->fileName();
                  } 
              }
                
            if (retcode < 0)
              {
                Lng32 cliError = 0;
		    
                Lng32 intParam1 = -retcode;
                ComDiagsArea * diagsArea = NULL;
                ExRaiseSqlError(getHeap(), &diagsArea, 
                                (ExeErrorCode)(EXE_ERROR_FROM_LOB_INTERFACE), NULL, 
                                &intParam1, 
                                &hdfsErrorDetail, 
                                NULL, 
                                "HDFS",
                                (char*)"ExpLOBInterfaceSelectCursor/open",
                                getLobErrStr(intParam1));
                pentry_down->setDiagsArea(diagsArea);
                step_ = HANDLE_ERROR;
                break;
              }  
            trailingPrevRead_ = 0; 
            firstBufOfFile_ = true;
            numBytesProcessedInRange_ = 0;

            step_ = GET_HDFS_DATA;
          }        
          break;

	case GET_HDFS_DATA:
	  {
	    Int64 bytesToRead = hdfsScanBufMaxSize_ - trailingPrevRead_;
            ex_assert(bytesToRead >= 0, "bytesToRead less than zero.");
            if (hdfo_->fileIsSplitEnd() && !isSequenceFile())
            {
              if (bytesLeft_ > 0) 
                bytesToRead = min(bytesToRead, 
                          (bytesLeft_ + hdfsScanTdb().rangeTailIOSize_));
              else
                bytesToRead = hdfsScanTdb().rangeTailIOSize_;
            }
            else
            {
              ex_assert(bytesLeft_ >= 0, "Bad assumption at e-o-f");
              if (bytesToRead > bytesLeft_ +
                     1 // plus one for end-of-range files with no
                       // record delimiter at eof.
                 )
                bytesToRead = bytesLeft_ + 1;
            }

            ex_assert(bytesToRead + trailingPrevRead_ <= hdfsScanBufMaxSize_,
                     "too many bites.");

            if (hdfsStats_)
	      hdfsStats_->getHdfsTimer().start();

	    retcode = 0;
	    
	    if (isSequenceFile())
	      {
                sfrRetCode = sequenceFileReader_->fetchRowsIntoBuffer(stopOffset_, 
                                                                      hdfsScanBuffer_, 
                                                                      hdfsScanBufMaxSize_, //bytesToRead, 
                                                                      bytesRead_,
                                                                      hdfsScanTdb().recordDelimiter_);

	        if (sfrRetCode != JNI_OK && sfrRetCode != SFR_NOMORE)
	          {
    		    ComDiagsArea * diagsArea = NULL;
    		    ExRaiseSqlError(getHeap(), &diagsArea, (ExeErrorCode)(8447), NULL, 
    		    		  NULL, NULL, NULL, sequenceFileReader_->getErrorText(sfrRetCode), NULL);
    		    pentry_down->setDiagsArea(diagsArea);
    		    step_ = HANDLE_ERROR_WITH_CLOSE;
    		    break;
	          }
	        else
	          {
                    seqScanAgain_ = (sfrRetCode != SFR_NOMORE);
	          }
	      }
	    else
	      {
                Int32 hdfsErrorDetail = 0;///this is the errno returned from the underlying hdfs call.
                retcode = ExpLOBInterfaceSelectCursor
                  (lobGlob_,
                   hdfsFileName_,
                   NULL, 
                   (Lng32)Lob_External_HDFS_File,
                   hdfsScanTdb().hostName_,
                   hdfsScanTdb().port_,
                   0, NULL,		       
                   0, cursorId_,
		       
                   requestTag_, Lob_Memory,
                   0, // not check status
                   (NOT hdfsScanTdb().hdfsPrefetch()),  //1, // waited op
		       
                   hdfsOffset_,
                   bytesToRead,
                   bytesRead_,
                   hdfsScanBuffer_  + trailingPrevRead_,
                   2, // read
                   0 // openType, not applicable for read
                   &hdfsErrorDetail
                   );
                  
                if (hdfsStats_)
                  hdfsStats_->incMaxHdfsIOTime(hdfsStats_->getHdfsTimer().stop());
	          
	        if (retcode < 0)
	          {
		    Lng32 cliError = 0;
		    
		    Lng32 intParam1 = -retcode;
		    ComDiagsArea * diagsArea = NULL;
		    ExRaiseSqlError(getHeap(), &diagsArea, 
                                    (ExeErrorCode)(EXE_ERROR_FROM_LOB_INTERFACE), NULL, 
                                    &intParam1, 
                                    &hdfsErrorDetail, 
                                    NULL, 
                                    "HDFS",
                                    (char*)"ExpLOBInterfaceSelectCursor/read",
                                    getLobErrStr(intParam1));
		    pentry_down->setDiagsArea(diagsArea);
		    step_ = HANDLE_ERROR_WITH_CLOSE;
		    break;
	          }
              }
              
            if (bytesRead_ <= 0)
	      {
		// Finished with this file. Unexpected? Warning/event?
	        step_ = CLOSE_HDFS_CURSOR;
                break;
              }
	    else
              {
                char * lastByteRead = hdfsScanBuffer_  +
                  trailingPrevRead_ + bytesRead_ - 1;
                if ((bytesRead_ < bytesToRead) &&
                    (*lastByteRead != hdfsScanTdb().recordDelimiter_))
                {
                  // Some files end without a record delimiter but
                  // hive treats the end-of-file as a record delimiter.
                  lastByteRead[1] = hdfsScanTdb().recordDelimiter_;
                  bytesRead_++;
                }
                if (bytesRead_ > bytesLeft_)
                { 
                  if (isSequenceFile())
                    endOfRequestedRange_ = hdfsScanBuffer_ + bytesRead_;
                  else
                    endOfRequestedRange_ = hdfsScanBuffer_ +
                                   trailingPrevRead_ + bytesLeft_;
                }
                else
                   endOfRequestedRange_ = NULL;
                  
                if (isSequenceFile())
                  {
                    // If the file is compressed, we don't know the real value
                    // of bytesLeft_, but it doesn't really matter.
                    if (seqScanAgain_ == false)
                      bytesLeft_ = 0;
                  }
                else
	          bytesLeft_ -= bytesRead_;
              }
	    
	    if (hdfsStats_)
	      hdfsStats_->incBytesRead(bytesRead_);

	    if (firstBufOfFile_ && hdfo_->fileIsSplitBegin() && !isSequenceFile())
	      {
		// Position in the hdfsScanBuffer_ to the
		// first record delimiter.  
		hdfsBufNextRow_ = 
		  hdfs_strchr(hdfsScanBuffer_,
			      hdfsScanTdb().recordDelimiter_, 
			      hdfsScanBuffer_+trailingPrevRead_+ 
			      min(bytesRead_, hdfo_->bytesToRead_), 
			      checkRangeDelimiter_, 
			      hdfsScanTdb().getHiveScanMode(), &changedLen);
		// May be that the record is too long? Or data isn't ascii?
		// Or delimiter is incorrect.
		if (! hdfsBufNextRow_)
		  {
		    if (hdfo_->bytesToRead_ < hdfsScanTdb().rangeTailIOSize_)
		      {
			// for wide rows it is not an error if a whole range
			// does not include a record delimiter. RangeTaileIOSize
			// is set to max row size in generator by default.
			// It is also checked in the compiler that rowsize
			// is less than buffer size.
			step_ = CLOSE_HDFS_CURSOR;
		      }
		    else 
		      {
			ComDiagsArea *diagsArea = NULL;
			ExRaiseSqlError(getHeap(), &diagsArea, 
					(ExeErrorCode)(8446), NULL, 
					NULL, NULL, NULL,
					(char*)"No record delimiter found in buffer from hdfsRead.",
					NULL);
			// no need to log errors in this case (bulk load) since
			// this is a major issue and needs to be corrected
			pentry_down->setDiagsArea(diagsArea);
			step_ = HANDLE_ERROR_WITH_CLOSE;
		      }
		    break;
		  }
		
		hdfsBufNextRow_ += 1 + changedLen;   // point past record delimiter.
		//add changedLen since hdfs_strchr will remove the pointer ahead to remove the \r
	      }
	    else
	      hdfsBufNextRow_ = hdfsScanBuffer_;
	    
            debugPrevRow_ = hdfsScanBuffer_;    // By convention, at
                                                // beginning of scan, the
                                                // prev is set to next.
            debugtrailingPrevRead_ = 0; 
            debugPenultimatePrevRow_ = NULL;
            firstBufOfFile_ = false;

	    hdfsOffset_ += bytesRead_;

 	    step_ = PROCESS_HDFS_ROW;
	  }        
	  break;

	case PROCESS_HDFS_ROW:
	{
	  exception_ = FALSE;
	  nextStep_ = NOT_STARTED;
	  debugPenultimatePrevRow_ = debugPrevRow_;
	  debugPrevRow_ = hdfsBufNextRow_;

	  int formattedRowLength = 0;
	  ComDiagsArea *transformDiags = NULL;
	  int err = 0;
	  char *startOfNextRow =
	      extractAndTransformAsciiSourceToSqlRow(err, transformDiags, hdfsScanTdb().getHiveScanMode());

	  bool rowWillBeSelected = true;
	  lastErrorCnd_ = NULL;
	  if(err)
	  {
	    if (hdfsScanTdb().continueOnError())
	    {
	      Lng32 errorCount = workAtp_->getDiagsArea()->getNumber(DgSqlCode::ERROR_);
              if (errorCount>0)
	        lastErrorCnd_ = workAtp_->getDiagsArea()->getErrorEntry(errorCount);
	      exception_ = TRUE;
	      rowWillBeSelected = false;
	    }
	    else
	    {
	      if (transformDiags)
	        pentry_down->setDiagsArea(transformDiags);
	      step_ = HANDLE_ERROR_WITH_CLOSE;
	      break;
	    }
	  }

	  if (startOfNextRow == NULL)
	  {
	    step_ = REPOS_HDFS_DATA;
	    if (!exception_)
	      break;
	  }
	  else
	  {
	    numBytesProcessedInRange_ +=
	        startOfNextRow - hdfsBufNextRow_;
	    hdfsBufNextRow_ = startOfNextRow;
	  }

	  if (exception_)
	  {
	    nextStep_ = step_;
	    step_ = HANDLE_EXCEPTION;
	    break;
	  }
	  if (hdfsStats_)
	    hdfsStats_->incAccessedRows();

	  workAtp_->getTupp(hdfsScanTdb().workAtpIndex_) =
	      hdfsSqlTupp_;

	  if ((rowWillBeSelected) && (selectPred()))
	  {
	    ex_expr::exp_return_type evalRetCode =
	        selectPred()->eval(pentry_down->getAtp(), workAtp_);
	    if (evalRetCode == ex_expr::EXPR_FALSE)
	      rowWillBeSelected = false;
	    else if (evalRetCode == ex_expr::EXPR_ERROR)
	    {
	      if (hdfsScanTdb().continueOnError())
	      {
                if (pentry_down->getDiagsArea() || workAtp_->getDiagsArea())
                {
                  Lng32 errorCount = 0;
                  if (pentry_down->getDiagsArea())
                  {
                    errorCount = pentry_down->getDiagsArea()->getNumber(DgSqlCode::ERROR_);
                    if (errorCount > 0)
                      lastErrorCnd_ = pentry_down->getDiagsArea()->getErrorEntry(errorCount);
                  }
                  else
                  {
                    errorCount = workAtp_->getDiagsArea()->getNumber(DgSqlCode::ERROR_);
                    if (errorCount > 0)
                      lastErrorCnd_ = workAtp_->getDiagsArea()->getErrorEntry(errorCount);
                  }
                }
                 exception_ = TRUE;
                 nextStep_ = step_;
                 step_ = HANDLE_EXCEPTION;

	        rowWillBeSelected = false;

	        break;
	      }
	      step_ = HANDLE_ERROR_WITH_CLOSE;
	      break;
	    }
	    else
	      ex_assert(evalRetCode == ex_expr::EXPR_TRUE,
	          "invalid return code from expr eval");
	  }

	  if (rowWillBeSelected)
	  {
	    if (moveColsConvertExpr())
	    {
	      ex_expr::exp_return_type evalRetCode =
	          moveColsConvertExpr()->eval(workAtp_, workAtp_);
	      if (evalRetCode == ex_expr::EXPR_ERROR)
	      {
	        if (hdfsScanTdb().continueOnError())
	        {
                  if ( workAtp_->getDiagsArea())
                  {
                    Lng32 errorCount = 0;
                    errorCount = workAtp_->getDiagsArea()->getNumber(DgSqlCode::ERROR_);
                    if (errorCount > 0)
                      lastErrorCnd_ = workAtp_->getDiagsArea()->getErrorEntry(errorCount);
                  }
                   exception_ = TRUE;
                   nextStep_ = step_;
                   step_ = HANDLE_EXCEPTION;
	          break;
	        }
	        step_ = HANDLE_ERROR_WITH_CLOSE;
	        break;
	      }
	    }
	    if (hdfsStats_)
	      hdfsStats_->incUsedRows();

	    step_ = RETURN_ROW;
          
	    break;
	  }

	  break;
	}
	case RETURN_ROW:
	{
	  if (qparent_.up->isFull())
	    return WORK_OK;

	  lastErrorCnd_  = NULL;

	  ex_queue_entry *up_entry = qparent_.up->getTailEntry();
	  queue_index saveParentIndex = up_entry->upState.parentIndex;
	  queue_index saveDownIndex  = up_entry->upState.downIndex;

	  up_entry->copyAtp(pentry_down);

	  up_entry->upState.parentIndex =
	      pentry_down->downState.parentIndex;
	  up_entry->upState.downIndex = qparent_.down->getHeadIndex();
	  up_entry->upState.status = ex_queue::Q_OK_MMORE;


	  if (moveExpr())
	  {
	    UInt32 maxRowLen = hdfsScanTdb().outputRowLength_;
	    UInt32 rowLen = maxRowLen;

	    if (hdfsScanTdb().useCifDefrag() &&
	        !pool_->currentBufferHasEnoughSpace((Lng32)hdfsScanTdb().outputRowLength_))
	    {
	      up_entry->getTupp(hdfsScanTdb().tuppIndex_) = defragTd_;
	      defragTd_->setReferenceCount(1);
	      ex_expr::exp_return_type evalRetCode =
	          moveExpr()->eval(up_entry->getAtp(), workAtp_,0,-1,&rowLen);
	      if (evalRetCode ==  ex_expr::EXPR_ERROR)
	      {
                if (hdfsScanTdb().continueOnError())
                {
                  if ((pentry_down->downState.request == ex_queue::GET_N) &&
                      (pentry_down->downState.requestValue == matches_))
                    step_ = CLOSE_HDFS_CURSOR;
                  else
                    step_ = PROCESS_HDFS_ROW;

                  up_entry->upState.parentIndex =saveParentIndex  ;
                  up_entry->upState.downIndex = saveDownIndex  ;
                  if (up_entry->getDiagsArea() || workAtp_->getDiagsArea())
                  {
                    Lng32 errorCount = 0;
                    if (up_entry->getDiagsArea())
                    {
                      errorCount = up_entry->getDiagsArea()->getNumber(DgSqlCode::ERROR_);
                      if (errorCount > 0)
                        lastErrorCnd_ = up_entry->getDiagsArea()->getErrorEntry(errorCount);
                    }
                    else
                    {
                      errorCount = workAtp_->getDiagsArea()->getNumber(DgSqlCode::ERROR_);
                      if (errorCount > 0)
                        lastErrorCnd_ = workAtp_->getDiagsArea()->getErrorEntry(errorCount);
                    }
                  }
                   exception_ = TRUE;
                   nextStep_ = step_;
                   step_ = HANDLE_EXCEPTION;
                  break;
                }
	        else
	        {
	          // Get diags from up_entry onto pentry_down, which
	          // is where the HANDLE_ERROR step expects it.
	          ComDiagsArea *diagsArea = pentry_down->getDiagsArea();
	          if (diagsArea == NULL)
	          {
	            diagsArea =
	                ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
	            pentry_down->setDiagsArea (diagsArea);
	          }
	          pentry_down->getDiagsArea()->
	              mergeAfter(*up_entry->getDiagsArea());
	          up_entry->setDiagsArea(NULL);
	          step_ = HANDLE_ERROR_WITH_CLOSE;
	          break;
	        }
	        if (pool_->get_free_tuple(
	            up_entry->getTupp(hdfsScanTdb().tuppIndex_),
	            rowLen))
	          return WORK_POOL_BLOCKED;
	        str_cpy_all(up_entry->getTupp(hdfsScanTdb().tuppIndex_).getDataPointer(),
	            defragTd_->getTupleAddress(),
	            rowLen);
	      }
	    }
	    else
	    {
	      if (pool_->get_free_tuple(
	          up_entry->getTupp(hdfsScanTdb().tuppIndex_),
	          (Lng32)hdfsScanTdb().outputRowLength_))
	        return WORK_POOL_BLOCKED;
	      ex_expr::exp_return_type evalRetCode =
	          moveExpr()->eval(up_entry->getAtp(), workAtp_,0,-1,&rowLen);
	      if (evalRetCode ==  ex_expr::EXPR_ERROR)
	      {
	        if (hdfsScanTdb().continueOnError())
	        {
	          if ((pentry_down->downState.request == ex_queue::GET_N) &&
	              (pentry_down->downState.requestValue == matches_))
	            step_ = CLOSE_FILE;
	          else
	            step_ = PROCESS_HDFS_ROW;

                  if (up_entry->getDiagsArea() || workAtp_->getDiagsArea())
                  {
                    Lng32 errorCount = 0;
                    if (up_entry->getDiagsArea())
                    {
                      errorCount = up_entry->getDiagsArea()->getNumber(DgSqlCode::ERROR_);
                      if (errorCount > 0)
                        lastErrorCnd_ = up_entry->getDiagsArea()->getErrorEntry(errorCount);
                    }
                    else
                    {
                      errorCount = workAtp_->getDiagsArea()->getNumber(DgSqlCode::ERROR_);
                      if (errorCount > 0)
                        lastErrorCnd_ = workAtp_->getDiagsArea()->getErrorEntry(errorCount);
                    }
                  }
                  up_entry->upState.parentIndex =saveParentIndex  ;
                  up_entry->upState.downIndex = saveDownIndex  ;
                  exception_ = TRUE;
                  nextStep_ = step_;
                  step_ = HANDLE_EXCEPTION;
	          break;
	        }
	        else
	        {
	          // Get diags from up_entry onto pentry_down, which
	          // is where the HANDLE_ERROR step expects it.
	          ComDiagsArea *diagsArea = pentry_down->getDiagsArea();
	          if (diagsArea == NULL)
	          {
	            diagsArea =
	                ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
	            pentry_down->setDiagsArea (diagsArea);
	          }
	          pentry_down->getDiagsArea()->
	              mergeAfter(*up_entry->getDiagsArea());
	          up_entry->setDiagsArea(NULL);
	          step_ = HANDLE_ERROR_WITH_CLOSE;
	          break;
	        }
	      }
	      if (hdfsScanTdb().useCif() && rowLen != maxRowLen)
	      {
	        pool_->resizeLastTuple(rowLen,
	            up_entry->getTupp(hdfsScanTdb().tuppIndex_).getDataPointer());
	      }
	    }
	  }

	  up_entry->upState.setMatchNo(++matches_);
	  if (matches_ == matchBrkPoint_)
	    brkpoint();
	  qparent_.up->insert();

	  // use ExOperStats now, to cover OPERATOR stats as well as
	  // ALL stats.
	  if (getStatsEntry())
	    getStatsEntry()->incActualRowsReturned();

	  workAtp_->setDiagsArea(NULL);    // get rid of warnings.
          if (((pentry_down->downState.request == ex_queue::GET_N) &&
               (pentry_down->downState.requestValue == matches_)) ||
              (pentry_down->downState.request == ex_queue::GET_NOMORE))
              step_ = CLOSE_HDFS_CURSOR;
          else
	     step_ = PROCESS_HDFS_ROW;
	  break;      
	}
	case REPOS_HDFS_DATA:
	  {
            bool scanAgain = false;
            if (isSequenceFile())
              scanAgain = seqScanAgain_;
            else
              {
                if (hdfo_->fileIsSplitEnd())
                  {
                  if (numBytesProcessedInRange_ <  hdfo_->getBytesToRead())
                    scanAgain = true;
                  }
                else 
                  if (bytesLeft_ > 0)
                    scanAgain = true;
              }
              
            if (scanAgain)
            {
              // Get ready for another gulp of hdfs data.  
              debugtrailingPrevRead_ = trailingPrevRead_;
              trailingPrevRead_ = bytesRead_ - 
                          (hdfsBufNextRow_ - 
                           (hdfsScanBuffer_ + trailingPrevRead_));

              // Move trailing data from the end of buffer to the front.
              // The GET_HDFS_DATA step will use trailingPrevRead_ to
              // adjust the read buffer ptr so that the next read happens
              // contiguously to the final byte of the prev read. It will
              // also use trailingPrevRead_ to to adjust the size of
              // the next read so that fixed size buffer is not overrun.
              // Finally, trailingPrevRead_ is used in the 
              // extractSourceFields method to keep from processing
              // bytes left in the buffer from the previous read.
              if ((trailingPrevRead_ > 0)  && 
                  (hdfsBufNextRow_[0] == RANGE_DELIMITER))
              {
                 checkRangeDelimiter_ = FALSE;
                 step_ = CLOSE_HDFS_CURSOR;
                 break;
              }  
              memmove(hdfsScanBuffer_, hdfsBufNextRow_, 
		      (size_t)trailingPrevRead_);
              step_ = GET_HDFS_DATA;
            }            
            else
            {
              trailingPrevRead_ = 0;
              step_ = CLOSE_HDFS_CURSOR;
            }
	    break;
	  }
	case CLOSE_HDFS_CURSOR:
	  {          
	    retcode = 0;
	    if (isSequenceFile())
	      {
	        sfrRetCode = sequenceFileReader_->close();
	        if (sfrRetCode != JNI_OK)
	          {
    		    ComDiagsArea * diagsArea = NULL;
    		    ExRaiseSqlError(getHeap(), &diagsArea, (ExeErrorCode)(8447), NULL, 
    		    		  NULL, NULL, NULL, sequenceFileReader_->getErrorText(sfrRetCode), NULL);
    		    pentry_down->setDiagsArea(diagsArea);
    		    step_ = HANDLE_ERROR;
    		    break;
	          }
	      }
	    else
	      {
                retcode = ExpLOBInterfaceSelectCursor
                  (lobGlob_,
                   hdfsFileName_, 
                   NULL,
                   (Lng32)Lob_External_HDFS_File,
                   hdfsScanTdb().hostName_,
                   hdfsScanTdb().port_,
                   0,NULL, //handle not relevant for non lob access
                   0, cursorId_,
		       
                   requestTag_, Lob_Memory,
                   0, // not check status
                   (NOT hdfsScanTdb().hdfsPrefetch()),  //1, // waited op
		       
                   0, 
                   hdfsScanBufMaxSize_,
                   bytesRead_,
                   hdfsScanBuffer_,
                   3, // close
                   0); // openType, not applicable for close
                 
	        if (retcode < 0)
	          {
		    Lng32 cliError = 0;
		    
		    Lng32 intParam1 = -retcode;
		    ComDiagsArea * diagsArea = NULL;
		    ExRaiseSqlError(getHeap(), &diagsArea, 
                                    (ExeErrorCode)(EXE_ERROR_FROM_LOB_INTERFACE), NULL, 
                                    &intParam1, 
                                    &errno, 
                                    NULL, 
                                    "HDFS",
                                    (char*)"ExpLOBInterfaceSelectCursor/close",
                                    getLobErrStr(intParam1));
		    pentry_down->setDiagsArea(diagsArea);
		    step_ = HANDLE_ERROR;
		    break;
	          }
              }
              step_ = CLOSE_FILE;
	  }
          break;
	case HANDLE_EXCEPTION:
	{
	  step_ = nextStep_;
	  exception_ = FALSE;

	  if (hdfsScanTdb().getMaxErrorRows() > 0)
	  {
	    Int64 exceptionCount = 0;
            ExHbaseAccessTcb::incrErrorCount( ehi_,exceptionCount, 
                 hdfsScanTdb().getErrCountTable(),hdfsScanTdb().getErrCountRowId()); 
	    if (exceptionCount >  hdfsScanTdb().getMaxErrorRows())
	    {
	      if (pentry_down->getDiagsArea())
	        pentry_down->getDiagsArea()->clear();
	      if (workAtp_->getDiagsArea())
	        workAtp_->getDiagsArea()->clear();

	      ComDiagsArea *da = workAtp_->getDiagsArea();
	      if(!da)
	      {
	        da = ComDiagsArea::allocate(getHeap());
	        workAtp_->setDiagsArea(da);
	      }
	      *da << DgSqlCode(-EXE_MAX_ERROR_ROWS_EXCEEDED);
	      step_ = HANDLE_ERROR_WITH_CLOSE;
	      break;
	    }
	  }
          if (hdfsScanTdb().getLogErrorRows())
          {
            int loggingRowLen =  hdfsLoggingRowEnd_ - hdfsLoggingRow_ +1;
            ExHbaseAccessTcb::handleException((NAHeap *)getHeap(), hdfsLoggingRow_,
                       loggingRowLen, lastErrorCnd_, 
                       ehi_,
                       LoggingFileCreated_,
                       loggingFileName_,
                       &loggingErrorDiags_);

            
          }

          if (pentry_down->getDiagsArea())
            pentry_down->getDiagsArea()->clear();
          if (workAtp_->getDiagsArea())
            workAtp_->getDiagsArea()->clear();
	}
	break;

        case HANDLE_ERROR_WITH_CLOSE:
	case HANDLE_ERROR:
	case HANDLE_ERROR_AND_DONE:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;
	    ex_queue_entry *up_entry = qparent_.up->getTailEntry();
	    up_entry->copyAtp(pentry_down);
	    up_entry->upState.parentIndex =
	      pentry_down->downState.parentIndex;
	    up_entry->upState.downIndex = qparent_.down->getHeadIndex();
	    if (workAtp_->getDiagsArea())
	      {
		ComDiagsArea *diagsArea = up_entry->getDiagsArea();
		
		if (diagsArea == NULL)
		  {
		    diagsArea = 
		      ComDiagsArea::allocate(getGlobals()->getDefaultHeap());

		    up_entry->setDiagsArea (diagsArea);
		  }

		up_entry->getDiagsArea()->mergeAfter(*workAtp_->getDiagsArea());
		workAtp_->setDiagsArea(NULL);
	      }
	    up_entry->upState.status = ex_queue::Q_SQLERROR;
	    qparent_.up->insert();
	    
            if (step_ == HANDLE_ERROR_WITH_CLOSE)
               step_ = CLOSE_HDFS_CURSOR;
            else if (step_ == HANDLE_ERROR_AND_DONE)
              step_ = DONE;
            else
	       step_ = ERROR_CLOSE_FILE;
	    break;
           
	  }

	case CLOSE_FILE:
	case ERROR_CLOSE_FILE:
	  {
	    if (getStatsEntry())
	      {
		ExHdfsScanStats * stats =
		  getStatsEntry()->castToExHdfsScanStats();
		
		if (stats)
		  {
		    ExLobStats s;
		    s.init();

		    retcode = ExpLOBinterfaceStats
		      (lobGlob_,
		       &s, 
		       hdfsFileName_, //hdfsScanTdb().hdfsFileName_,
		       NULL, //(char*)"",
		       (Lng32)Lob_External_HDFS_File,
		       hdfsScanTdb().hostName_,
		       hdfsScanTdb().port_);

		    *stats->lobStats() = *stats->lobStats() + s;
		  }
	      }

            // if next file is not same as current file, then close the current file. 
            bool closeFile = true;

            if ( (step_ == CLOSE_FILE) && 
                 ((currRangeNum_ + 1) < (beginRangeNum_ + numRanges_))) 
                 
            {   
                hdfo = (HdfsFileInfo*) hdfsScanTdb().getHdfsFileInfoList()->get(currRangeNum_ + 1);
                if (strcmp(hdfsFileName_, hdfo->fileName()) == 0) 
                    closeFile = false;
            }

            if (closeFile) 
            {
                retcode = ExpLOBinterfaceCloseFile
                  (lobGlob_,
                   hdfsFileName_,
                   NULL, 
                   (Lng32)Lob_External_HDFS_File,
                   hdfsScanTdb().hostName_,
                   hdfsScanTdb().port_);

                if ((step_ == CLOSE_FILE) &&
                    (retcode < 0))
                  {
                    Lng32 cliError = 0;
                    
                    Lng32 intParam1 = -retcode;
                    ComDiagsArea * diagsArea = NULL;
                    ExRaiseSqlError(getHeap(), &diagsArea, 
                                    (ExeErrorCode)(EXE_ERROR_FROM_LOB_INTERFACE), NULL, 
                                    &intParam1, 
                                    &cliError, 
                                    NULL, 
                                    "HDFS",
                                    (char*)"ExpLOBinterfaceCloseFile",
                                    getLobErrStr(intParam1));
                    pentry_down->setDiagsArea(diagsArea);
                  }
                // sss This is one place that is unconditionally closing the 
                // hdfsFs that's part of this thread's JNIenv.
                // if (ehi_)
                //   retcode = ehi_->hdfsClose();
            } 
	    if (step_ == CLOSE_FILE)
	      {
                currRangeNum_++;
                if (currRangeNum_ < (beginRangeNum_ + numRanges_)) {
                    if (((pentry_down->downState.request == ex_queue::GET_N) &&
                        (pentry_down->downState.requestValue == matches_)) ||
                         (pentry_down->downState.request == ex_queue::GET_NOMORE))
                       step_ = DONE;
                    else
                       // move to the next file.
                       step_ = INIT_HDFS_CURSOR;
                    break;
                }
	      }     
	    step_ = DONE;
	  }

          
	  break;

	case DONE:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;
	    ex_queue_entry *up_entry = qparent_.up->getTailEntry();
	    up_entry->copyAtp(pentry_down);
	    up_entry->upState.parentIndex =
	      pentry_down->downState.parentIndex;
	    up_entry->upState.downIndex = qparent_.down->getHeadIndex();
	    up_entry->upState.status = ex_queue::Q_NO_DATA;
	    up_entry->upState.setMatchNo(matches_);
            if (loggingErrorDiags_ != NULL) 
            {
               ComDiagsArea * diagsArea = up_entry->getDiagsArea();
               if (!diagsArea)
               {
                  diagsArea =
                   ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
                  up_entry->setDiagsArea(diagsArea);
               }
               diagsArea->mergeAfter(*loggingErrorDiags_);
               loggingErrorDiags_->clear();
            }
	    qparent_.up->insert();
	    
	    qparent_.down->removeHead();
	    step_ = NOT_STARTED;
            dirInfo = hdfsGetPathInfo(hdfs, "/");
	    break;
	  }
	  
	default: 
	  {
	    break;
	  }
	  
	} // switch
    } // while
  
  return WORK_OK;
}

char * ExHdfsScanTcb::extractAndTransformAsciiSourceToSqlRow(int &err,
							     ComDiagsArea* &diagsArea, int mode)
{
  err = 0;
  char *sourceData = hdfsBufNextRow_;
  char *sourceRowEnd = NULL; 
  char *sourceColEnd = NULL;
  int changedLen = 0;
  NABoolean isTrailingMissingColumn = FALSE;
  ExpTupleDesc * asciiSourceTD =
     hdfsScanTdb().workCriDesc_->getTupleDescriptor(hdfsScanTdb().asciiTuppIndex_);

  ExpTupleDesc * origSourceTD = 
    hdfsScanTdb().workCriDesc_->getTupleDescriptor(hdfsScanTdb().origTuppIndex_);
  
  const char cd = hdfsScanTdb().columnDelimiter_;
  const char rd = hdfsScanTdb().recordDelimiter_;
  const char *sourceDataEnd = hdfsScanBuffer_+trailingPrevRead_+ bytesRead_;

  hdfsLoggingRow_ = hdfsBufNextRow_;
  if (asciiSourceTD->numAttrs() == 0)
  {
     sourceRowEnd = hdfs_strchr(sourceData, rd, sourceDataEnd, checkRangeDelimiter_, mode, &changedLen);
     hdfsLoggingRowEnd_  = sourceRowEnd + changedLen;

     if (!sourceRowEnd)
       return NULL; 
     if ((endOfRequestedRange_) && 
            (sourceRowEnd >= endOfRequestedRange_)) {
        checkRangeDelimiter_ = TRUE;
        *(sourceRowEnd +1)= RANGE_DELIMITER;
     }

    // no columns need to be converted. For e.g. count(*) with no predicate
    return sourceRowEnd+1;
  }

  Lng32 neededColIndex = 0;
  Attributes * attr = NULL;
  Attributes * tgtAttr = NULL;
  NABoolean rdSeen = FALSE;

  for (Lng32 i = 0; i <  hdfsScanTdb().convertSkipListSize_; i++)
    {
      // all remainin columns wil be skip columns, don't bother
      // finding their column delimiters
      if (neededColIndex == asciiSourceTD->numAttrs())
        continue;

      tgtAttr = NULL;
      if (hdfsScanTdb().convertSkipList_[i] > 0)
      {
        attr = asciiSourceTD->getAttr(neededColIndex);
        tgtAttr = origSourceTD->getAttr(neededColIndex);
        neededColIndex++;
      }
      else
        attr = NULL;
 
      if (!isTrailingMissingColumn) {
         sourceColEnd = hdfs_strchr(sourceData, rd, cd, sourceDataEnd, checkRangeDelimiter_, &rdSeen,mode, &changedLen);
         if (sourceColEnd == NULL) {
            if (rdSeen || (sourceRowEnd == NULL))
               return NULL;
            else
               return sourceRowEnd+1;
         }
         Int32 len = 0;
	 len = (Int64)sourceColEnd - (Int64)sourceData;
         if (rdSeen) {
            sourceRowEnd = sourceColEnd + changedLen; 
            hdfsLoggingRowEnd_  = sourceRowEnd;
            if ((endOfRequestedRange_) && 
                   (sourceRowEnd >= endOfRequestedRange_)) {
               checkRangeDelimiter_ = TRUE;
               *(sourceRowEnd +1)= RANGE_DELIMITER;
            }
            if (i != hdfsScanTdb().convertSkipListSize_ - 1)
               isTrailingMissingColumn = TRUE;
         }

         if (attr) // this is a needed column. We need to convert
         {
           if (attr->getVCIndicatorLength() == sizeof(short))
             *(short*)&hdfsAsciiSourceData_[attr->getVCLenIndOffset()] 
               = (short)len;
           else
             *(Int32*)&hdfsAsciiSourceData_[attr->getVCLenIndOffset()] 
               = len;

            if (attr->getNullFlag())
            {
              *(short *)&hdfsAsciiSourceData_[attr->getNullIndOffset()] = 0;
              if (hdfsScanTdb().getNullFormat()) // null format specified by user
                {
                  if (((len == 0) && (strlen(hdfsScanTdb().getNullFormat()) == 0)) ||
                      ((len > 0) && (memcmp(sourceData, hdfsScanTdb().getNullFormat(), len) == 0)))
                    {
                       *(short *)&hdfsAsciiSourceData_[attr->getNullIndOffset()] = -1;
                    }
                } // if
              else // null format not specified by user
                {
                  // Use default null format.
                  // for non-varchar, length of zero indicates a null value.
                  // For all datatypes, HIVE_DEFAULT_NULL_STRING('\N') indicates a null value.
                  if (((len == 0) && (tgtAttr && (NOT DFS2REC::isSQLVarChar(tgtAttr->getDatatype())))) ||
                      ((len > 0) && (memcmp(sourceData, HIVE_DEFAULT_NULL_STRING, len) == 0)))
                    {
                      *(short *)&hdfsAsciiSourceData_[attr->getNullIndOffset()] = -1;
                    }
                } // else
            } // if nullable attr

            if (len > 0)
            {
              // move address of data into the source operand.
              // convertExpr will dereference this addr and get to the actual
              // data.
              *(Int64*)&hdfsAsciiSourceData_[attr->getOffset()] =
                (Int64)sourceData;
            }
            else
            {
              *(Int64*)&hdfsAsciiSourceData_[attr->getOffset()] =
                (Int64)0;

            }
          } // if(attr)
	} // if (!trailingMissingColumn)
      else
	{
	  //  A delimiter was found, but not enough columns.
	  //  Treat the rest of the columns as NULL.
          if (attr && attr->getNullFlag())
            *(short *)&hdfsAsciiSourceData_[attr->getNullIndOffset()] = -1;
	}
      sourceData = sourceColEnd + 1 ;
    }
  // It is possible that the above loop came out before
  // rowDelimiter is encountered
  // So try to find the record delimiter
  if (sourceRowEnd == NULL) {
     sourceRowEnd = hdfs_strchr(sourceData, rd, sourceDataEnd, checkRangeDelimiter_,mode, &changedLen);
     if (sourceRowEnd) {
        hdfsLoggingRowEnd_  = sourceRowEnd + changedLen; //changedLen is when hdfs_strchr move the return pointer to remove the extra \r
        if ((endOfRequestedRange_) &&
              (sourceRowEnd >= endOfRequestedRange_ )) {
           checkRangeDelimiter_ = TRUE;
          *(sourceRowEnd +1)= RANGE_DELIMITER;
        }
     }
  }

  workAtp_->getTupp(hdfsScanTdb().workAtpIndex_) = hdfsSqlTupp_;
  workAtp_->getTupp(hdfsScanTdb().asciiTuppIndex_) = hdfsAsciiSourceTupp_;
  // for later
  workAtp_->getTupp(hdfsScanTdb().moveExprColsTuppIndex_) = moveExprColsTupp_;

  if (convertExpr())
  {
    ex_expr::exp_return_type evalRetCode =
      convertExpr()->eval(workAtp_, workAtp_);
    if (evalRetCode == ex_expr::EXPR_ERROR)
      err = -1;
    else
      err = 0;
  }
  if (sourceRowEnd)
     return sourceRowEnd+1;
  return NULL;
}

short ExHdfsScanTcb::moveRowToUpQueue(const char * row, Lng32 len, 
                                      short * rc, NABoolean isVarchar)
{
  if (qparent_.up->isFull())
    {
      if (rc)
	*rc = WORK_OK;
      return -1;
    }

  Lng32 length;
  if (len <= 0)
    length = strlen(row);
  else
    length = len;

  tupp p;
  if (pool_->get_free_tuple(p, (Lng32)
			    ((isVarchar ? SQL_VARCHAR_HDR_SIZE : 0)
			     + length)))
    {
      if (rc)
	*rc = WORK_POOL_BLOCKED;
      return -1;
    }
  
  char * dp = p.getDataPointer();
  if (isVarchar)
    {
      *(short*)dp = (short)length;
      str_cpy_all(&dp[SQL_VARCHAR_HDR_SIZE], row, length);
    }
  else
    {
      str_cpy_all(dp, row, length);
    }

  ex_queue_entry * pentry_down = qparent_.down->getHeadEntry();
  ex_queue_entry * up_entry = qparent_.up->getTailEntry();
  
  up_entry->copyAtp(pentry_down);
  up_entry->getAtp()->getTupp((Lng32)hdfsScanTdb().tuppIndex_) = p;

  up_entry->upState.parentIndex = 
    pentry_down->downState.parentIndex;
  
  up_entry->upState.setMatchNo(++matches_);
  up_entry->upState.status = ex_queue::Q_OK_MMORE;

  // insert into parent
  qparent_.up->insert();

  return 0;
}

short ExHdfsScanTcb::handleError(short &rc)
{
  if (qparent_.up->isFull())
    {
      rc = WORK_OK;
      return -1;
    }

  if (qparent_.up->isFull())
    return WORK_OK;
  
  ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();
  ex_queue_entry *up_entry = qparent_.up->getTailEntry();
  up_entry->copyAtp(pentry_down);
  up_entry->upState.parentIndex =
    pentry_down->downState.parentIndex;
  up_entry->upState.downIndex = qparent_.down->getHeadIndex();
  up_entry->upState.status = ex_queue::Q_SQLERROR;
  qparent_.up->insert();

  return 0;
}

short ExHdfsScanTcb::handleDone(ExWorkProcRetcode &rc)
{
  if (qparent_.up->isFull())
    {
      rc = WORK_OK;
      return -1;
    }

  ex_queue_entry *pentry_down = qparent_.down->getHeadEntry();
  ex_queue_entry *up_entry = qparent_.up->getTailEntry();
  up_entry->copyAtp(pentry_down);
  up_entry->upState.parentIndex =
    pentry_down->downState.parentIndex;
  up_entry->upState.downIndex = qparent_.down->getHeadIndex();
  up_entry->upState.status = ex_queue::Q_NO_DATA;
  up_entry->upState.setMatchNo(matches_);
  qparent_.up->insert();
  
  qparent_.down->removeHead();

  return 0;
}

////////////////////////////////////////////////////////////////////////
// ORC files
////////////////////////////////////////////////////////////////////////
ExOrcScanTcb::ExOrcScanTcb(
          const ComTdbHdfsScan &orcScanTdb, 
          ex_globals * glob ) :
  ExHdfsScanTcb( orcScanTdb, glob),
  step_(NOT_STARTED)
{
  orci_ = ExpORCinterface::newInstance(glob->getDefaultHeap(),
                                       (char*)orcScanTdb.hostName_,
                                       orcScanTdb.port_);
}

ExOrcScanTcb::~ExOrcScanTcb()
{
}

short ExOrcScanTcb::extractAndTransformOrcSourceToSqlRow(
                                                         char * orcRow,
                                                         Int64 orcRowLen,
                                                         Lng32 numOrcCols,
                                                         ComDiagsArea* &diagsArea)
{
  short err = 0;

  if ((!orcRow) || (orcRowLen <= 0))
    return -1;

  char *sourceData = orcRow;

  ExpTupleDesc * asciiSourceTD =
     hdfsScanTdb().workCriDesc_->getTupleDescriptor(hdfsScanTdb().asciiTuppIndex_);
  if (asciiSourceTD->numAttrs() == 0)
    {
      // no columns need to be converted. For e.g. count(*) with no predicate
      return 0;
    }
  
  Lng32 neededColIndex = 0;
  Attributes * attr = NULL;

  Lng32 numCurrCols = 0;
  Lng32 currColLen;
  for (Lng32 i = 0; i <  hdfsScanTdb().convertSkipListSize_; i++)
    {
      if (hdfsScanTdb().convertSkipList_[i] > 0)
        {
          attr = asciiSourceTD->getAttr(neededColIndex);
          neededColIndex++;
        }
      else
        attr = NULL;
      
      currColLen = *(Lng32*)sourceData;
      sourceData += sizeof(currColLen);

      if (attr) // this is a needed column. We need to convert
        {
          *(short*)&hdfsAsciiSourceData_[attr->getVCLenIndOffset()] = currColLen;
          if (attr->getNullFlag())
            {
              if (currColLen == 0)
                *(short *)&hdfsAsciiSourceData_[attr->getNullIndOffset()] = -1;
	      else if (memcmp(sourceData, HIVE_DEFAULT_NULL_STRING, currColLen) == 0)
                *(short *)&hdfsAsciiSourceData_[attr->getNullIndOffset()] = -1;
              else
                *(short *)&hdfsAsciiSourceData_[attr->getNullIndOffset()] = 0;
            }
          
          if (currColLen > 0)
            {
              // move address of data into the source operand.
              // convertExpr will dereference this addr and get to the actual
              // data.
              *(Int64*)&hdfsAsciiSourceData_[attr->getOffset()] =
                (Int64)sourceData;
            }

        } // if(attr)

      numCurrCols++;
      sourceData += currColLen;
    }

  if (numCurrCols != numOrcCols)
    {
      return -1;
    }

  workAtp_->getTupp(hdfsScanTdb().workAtpIndex_) = hdfsSqlTupp_;
  workAtp_->getTupp(hdfsScanTdb().asciiTuppIndex_) = hdfsAsciiSourceTupp_;
  // for later
  workAtp_->getTupp(hdfsScanTdb().moveExprColsTuppIndex_) = moveExprColsTupp_;

  err = 0;
  if (convertExpr())
  {
    ex_expr::exp_return_type evalRetCode =
      convertExpr()->eval(workAtp_, workAtp_);
    if (evalRetCode == ex_expr::EXPR_ERROR)
      err = -1;
    else
      err = 0;
  }

  return err;
}

ExWorkProcRetcode ExOrcScanTcb::work()
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
	    
	    hdfsStats_ = NULL;
	    if (getStatsEntry())
	      hdfsStats_ = getStatsEntry()->castToExHdfsScanStats();

	    ex_assert(hdfsStats_, "hdfs stats cannot be null");

	    if (hdfsStats_)
	      hdfsStats_->init();

	    beginRangeNum_ = -1;
	    numRanges_ = -1;

	    if (hdfsScanTdb().getHdfsFileInfoList()->isEmpty())
	      {
		step_ = DONE;
		break;
	      }

	    myInstNum_ = getGlobals()->getMyInstanceNumber();

	    beginRangeNum_ =  
	      *(Lng32*)hdfsScanTdb().getHdfsFileRangeBeginList()->get(myInstNum_);

	    numRanges_ =  
	      *(Lng32*)hdfsScanTdb().getHdfsFileRangeNumList()->get(myInstNum_);

	    currRangeNum_ = beginRangeNum_;

	    if (numRanges_ > 0)
              step_ = INIT_ORC_CURSOR;
            else
              step_ = DONE;
	  }
	  break;

	case INIT_ORC_CURSOR:
	  {
            /*            orci_ = ExpORCinterface::newInstance(getHeap(),
                                                 (char*)hdfsScanTdb().hostName_,
                                       
            */

            hdfo_ = (HdfsFileInfo*)
              hdfsScanTdb().getHdfsFileInfoList()->get(currRangeNum_);
            
            orcStartRowNum_ = hdfo_->getStartRow();
            orcNumRows_ = hdfo_->getNumRows();
            
            hdfsFileName_ = hdfo_->fileName();
            sprintf(cursorId_, "%d", currRangeNum_);

            if (orcNumRows_ == -1) // select all rows
              orcStopRowNum_ = -1;
            else
              orcStopRowNum_ = orcStartRowNum_ + orcNumRows_ - 1;

	    step_ = OPEN_ORC_CURSOR;
	  }
	  break;

	case OPEN_ORC_CURSOR:
	  {
            retcode = orci_->scanOpen(hdfsFileName_,
                                      orcStartRowNum_, orcStopRowNum_);
            if (retcode < 0)
              {
                setupError(EXE_ERROR_FROM_LOB_INTERFACE, retcode, "ORC", "scanOpen", 
                           orci_->getErrorText(-retcode));

                step_ = HANDLE_ERROR;
                break;
              }

	    step_ = GET_ORC_ROW;
	  }
          break;
          
        case GET_ORC_ROW:
          {
            orcRow_ = hdfsScanBuffer_;
            orcRowLen_ =  hdfsScanTdb().hdfsBufSize_;
            retcode = orci_->scanFetch(orcRow_, orcRowLen_, orcRowNum_,
                                       numOrcCols_);
            if (retcode < 0)
              {
                setupError(EXE_ERROR_FROM_LOB_INTERFACE, retcode, "ORC", "scanFetch", 
                          orci_->getErrorText(-retcode));

                step_ = HANDLE_ERROR;
                break;
              }
            
            if (retcode == 100)
              {
                step_ = CLOSE_ORC_CURSOR;
                break;
              }

            step_ = PROCESS_ORC_ROW;
          }
          break;
          
	case PROCESS_ORC_ROW:
	  {
	    int formattedRowLength = 0;
	    ComDiagsArea *transformDiags = NULL;
            short err =
	      extractAndTransformOrcSourceToSqlRow(orcRow_, orcRowLen_,
                                                   numOrcCols_, transformDiags);
            
	    if (err)
	      {
		if (transformDiags)
		  pentry_down->setDiagsArea(transformDiags);
		step_ = HANDLE_ERROR;
		break;
	      }	    
	    
	    if (hdfsStats_)
	      hdfsStats_->incAccessedRows();
	    
	    workAtp_->getTupp(hdfsScanTdb().workAtpIndex_) = 
	      hdfsSqlTupp_;

	    bool rowWillBeSelected = true;
	    if (selectPred())
	      {
		ex_expr::exp_return_type evalRetCode =
		  selectPred()->eval(pentry_down->getAtp(), workAtp_);
		if (evalRetCode == ex_expr::EXPR_FALSE)
		  rowWillBeSelected = false;
		else if (evalRetCode == ex_expr::EXPR_ERROR)
		  {
		    step_ = HANDLE_ERROR;
		    break;
		  }
		else 
		  ex_assert(evalRetCode == ex_expr::EXPR_TRUE,
			    "invalid return code from expr eval");
	      }
	    
	    if (rowWillBeSelected)
	      {
                if (moveColsConvertExpr())
                  {
                    ex_expr::exp_return_type evalRetCode =
                      moveColsConvertExpr()->eval(workAtp_, workAtp_);
                    if (evalRetCode == ex_expr::EXPR_ERROR)
                      {
                        step_ = HANDLE_ERROR;
                        break;
                      }
                  }
		if (hdfsStats_)
		  hdfsStats_->incUsedRows();

		step_ = RETURN_ROW;
		break;
	      }

            step_ = GET_ORC_ROW;
          }
          break;

	case RETURN_ROW:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;
	    
	    ex_queue_entry *up_entry = qparent_.up->getTailEntry();
	    up_entry->copyAtp(pentry_down);
	    up_entry->upState.parentIndex = 
	      pentry_down->downState.parentIndex;
	    up_entry->upState.downIndex = qparent_.down->getHeadIndex();
	    up_entry->upState.status = ex_queue::Q_OK_MMORE;
	    
	    if (moveExpr())
	      {
	        UInt32 maxRowLen = hdfsScanTdb().outputRowLength_;
	        UInt32 rowLen = maxRowLen;
                
                if (hdfsScanTdb().useCifDefrag() &&
                    !pool_->currentBufferHasEnoughSpace((Lng32)hdfsScanTdb().outputRowLength_))
                  {
                    up_entry->getTupp(hdfsScanTdb().tuppIndex_) = defragTd_;
                    defragTd_->setReferenceCount(1);
                    ex_expr::exp_return_type evalRetCode =
                      moveExpr()->eval(up_entry->getAtp(), workAtp_,0,-1,&rowLen);
                    if (evalRetCode ==  ex_expr::EXPR_ERROR)
                      {
                        // Get diags from up_entry onto pentry_down, which
                        // is where the HANDLE_ERROR step expects it.
                        ComDiagsArea *diagsArea = pentry_down->getDiagsArea();
                        if (diagsArea == NULL)
                          {
                            diagsArea =
                              ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
                            pentry_down->setDiagsArea (diagsArea);
                          }
                        pentry_down->getDiagsArea()->
                          mergeAfter(*up_entry->getDiagsArea());
                        up_entry->setDiagsArea(NULL);
                        step_ = HANDLE_ERROR;
                        break;
                      }
                    if (pool_->get_free_tuple(
                                              up_entry->getTupp(hdfsScanTdb().tuppIndex_),
                                              rowLen))
                      return WORK_POOL_BLOCKED;
                    str_cpy_all(up_entry->getTupp(hdfsScanTdb().tuppIndex_).getDataPointer(),
                                defragTd_->getTupleAddress(),
                                rowLen);
                    
                  }
                else
                  {
                    if (pool_->get_free_tuple(
                                              up_entry->getTupp(hdfsScanTdb().tuppIndex_),
                                              (Lng32)hdfsScanTdb().outputRowLength_))
                      return WORK_POOL_BLOCKED;
                    ex_expr::exp_return_type evalRetCode =
                      moveExpr()->eval(up_entry->getAtp(), workAtp_,0,-1,&rowLen);
                    if (evalRetCode ==  ex_expr::EXPR_ERROR)
                      {
                        // Get diags from up_entry onto pentry_down, which
                        // is where the HANDLE_ERROR step expects it.
                        ComDiagsArea *diagsArea = pentry_down->getDiagsArea();
                        if (diagsArea == NULL)
                          {
                            diagsArea =
                              ComDiagsArea::allocate(getGlobals()->getDefaultHeap());
                            pentry_down->setDiagsArea (diagsArea);
                          }
                        pentry_down->getDiagsArea()->
                          mergeAfter(*up_entry->getDiagsArea());
                        up_entry->setDiagsArea(NULL);
                        step_ = HANDLE_ERROR;
                        break;
                      }
                    if (hdfsScanTdb().useCif() && rowLen != maxRowLen)
                      {
                        pool_->resizeLastTuple(rowLen,
                                               up_entry->getTupp(hdfsScanTdb().tuppIndex_).getDataPointer());
                      }
                  }
	      }
	    
	    up_entry->upState.setMatchNo(++matches_);
            if (matches_ == matchBrkPoint_)
              brkpoint();
	    qparent_.up->insert();
	    
	    // use ExOperStats now, to cover OPERATOR stats as well as 
	    // ALL stats. 
	    if (getStatsEntry())
	      getStatsEntry()->incActualRowsReturned();
	    
	    workAtp_->setDiagsArea(NULL);    // get rid of warnings.
	    
	    if ((pentry_down->downState.request == ex_queue::GET_N) &&
		(pentry_down->downState.requestValue == matches_))
	      step_ = CLOSE_ORC_CURSOR;
	    else
	      step_ = GET_ORC_ROW;
	    break;
	  }

        case CLOSE_ORC_CURSOR:
          {
            retcode = orci_->scanClose();
            if (retcode < 0)
              {
                setupError(EXE_ERROR_FROM_LOB_INTERFACE, retcode, "ORC", "scanClose", 
                           orci_->getErrorText(-retcode));
                
                step_ = HANDLE_ERROR;
                break;
              }
            
            currRangeNum_++;
            
            if (currRangeNum_ < (beginRangeNum_ + numRanges_))
              {
                // move to the next file.
                step_ = INIT_ORC_CURSOR;
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
	    if (handleDone(rc))
	      return rc;

	    step_ = NOT_STARTED;
	  }
          break;
	  
	default: 
	  {
	    break;
	  }
        } // switch
      
    } // while

  return WORK_OK;
}

ExOrcFastAggrTcb::ExOrcFastAggrTcb(
          const ComTdbOrcFastAggr &orcAggrTdb, 
          ex_globals * glob ) :
  ExOrcScanTcb(orcAggrTdb, glob),
  step_(NOT_STARTED)
{
  if (orcAggrTdb.outputRowLength_ > 0)
    aggrRow_ = new(glob->getDefaultHeap()) char[orcAggrTdb.outputRowLength_];
}

ExOrcFastAggrTcb::~ExOrcFastAggrTcb()
{
}

ExWorkProcRetcode ExOrcFastAggrTcb::work()
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

	    hdfsStats_ = NULL;
	    if (getStatsEntry())
	      hdfsStats_ = getStatsEntry()->castToExHdfsScanStats();
            
	    ex_assert(hdfsStats_, "hdfs stats cannot be null");

            orcAggrTdb().getHdfsFileInfoList()->position();

            rowCount_ = 0;

	    step_ = ORC_AGGR_INIT;
	  }
	  break;

	case ORC_AGGR_INIT:
	  {
            if (orcAggrTdb().getHdfsFileInfoList()->atEnd())
              {
                step_ = ORC_AGGR_PROJECT;
                break;
              }

            hdfo_ = (HdfsFileInfo*)orcAggrTdb().getHdfsFileInfoList()->getNext();
            
            hdfsFileName_ = hdfo_->fileName();

	    step_ = ORC_AGGR_EVAL;
	  }
	  break;

	case ORC_AGGR_EVAL:
	  {
            Int64 currRowCount = 0;
            retcode = orci_->getRowCount(hdfsFileName_, currRowCount);
            if (retcode < 0)
              {
                setupError(EXE_ERROR_FROM_LOB_INTERFACE, retcode, "ORC", "getRowCount", 
                           orci_->getErrorText(-retcode));

                step_ = HANDLE_ERROR;
                break;
              }

            rowCount_ += currRowCount;

            step_ = ORC_AGGR_INIT;
          }
          break;

        case ORC_AGGR_PROJECT:
          {
	    ExpTupleDesc * projTuppTD =
	      orcAggrTdb().workCriDesc_->getTupleDescriptor
	      (orcAggrTdb().workAtpIndex_);

	    Attributes * attr = projTuppTD->getAttr(0);
	    if (! attr)
	      {
		step_ = HANDLE_ERROR;
		break;
	      }

	    if (attr->getNullFlag())
	      {
		*(short*)&aggrRow_[attr->getNullIndOffset()] = 0;
	      }

	    str_cpy_all(&aggrRow_[attr->getOffset()], (char*)&rowCount_, sizeof(rowCount_));

            step_ = ORC_AGGR_RETURN;
	  }
	  break;

	case ORC_AGGR_RETURN:
	  {
	    if (qparent_.up->isFull())
	      return WORK_OK;

	    short rc = 0;
	    if (moveRowToUpQueue(aggrRow_, orcAggrTdb().outputRowLength_, 
				 &rc, FALSE))
	      return rc;
	    
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
	  }
	  break;

	} // switch
    } // while

  return WORK_OK;
}


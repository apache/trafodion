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
#ifndef SCRATCHFILE_BASE_H
#define SCRATCHFILE_BASE_H

/******************************************************************************
*
* File:         DiskPool_base.cpp
*                               
* Description:  This file contains the member function implementation for 
*               class ScratchFile. This class is used to encapsulate all 
*               data and methods about a scratch file.  
*                                                                 
*
******************************************************************************
*/

#include "Platform.h"
// 64-bit
// NOTE: We must use "unsigned long" here (even though it violates our rule
//       of not using "long" explicitly) because  DWORD may already be
//       defined as "unsigned long" by a system header file.

#include "CommonStructs.h"
#include "Const.h"
#include "CommonUtil.h"
#include "NABasicObject.h"
#include "Int64.h"
#include "SortError.h"
#include "ScratchFileConnection.h"
#include "stfs/stfslib.h"

class IpcEnvironment;
class ScratchFileConnection;
class ex_tcb;

// forward references
class ScratchFile;
class ScratchSpace;
class ExSubtask;

enum AsyncIOBufferState { QUEUED = 0, READING, READCOMPLETE, BEING_CONSUMED, IDLE };

class AsyncIOBuffer : public NABasicObject
{
  public:
  AsyncIOBuffer(void)
  {
    reset();
  };
  ~AsyncIOBuffer(void){};
  RESULT retval_; 
  AsyncIOBuffer* next_;  //linked into scratchFile's asyncReadQueue.
  char *scrBlock_;
  ScratchFile* currentScrFile_; // scratch file associated when IO pending, else NULL 
  enum AsyncIOBufferState state_; 
  Lng32 currentIOByteOffset_; //seekoffset into currentScrFile_ for IO

  //Not used on NSK
  Lng32 currentIOByteLength_; //expected IO size read
  ULng32 tag_;   
  
  void reset(void)
  {
    state_ = IDLE;
    currentScrFile_ = NULL;
    currentIOByteOffset_ = 0;
    currentIOByteLength_ = 0;
    tag_ = 0;
    next_ = NULL;
    retval_ = SCRATCH_SUCCESS;
  }
  
  virtual void processMisc(void)=0;

  AsyncIOBufferState state(void)
  {
    return state_;
  }

  protected:
  private:
  
};


//--------------------------------------------------------------------------
// This is a virtual base class used to basically enforce similar
// functionality between the different classes that inherit from this
// class.  However, this only enforce function prototype, so pay attention
// to all the different classes that inherit from this class and make them
// functionaly the same in the functions below.  Read the comments on the
// NSK and NT versions to see how other verions should behave.
//--------------------------------------------------------------------------
class ScratchFile : public NABasicObject {

  struct FileHandle1
  {
    Int32 bufferLen;
    Int32 blockNum;
    char *IOBuffer;
    ScratchFileConnection *scratchFileConn;
    AsyncIOBuffer * associatedAsyncIOBuffer;
    stfs_fhndl_t fileNum;
    NABoolean IOPending;
    RESULT previousError; // This will be set if an error with I/O on this 
                  // scratch file is encountered at the IPC connection layer
	char *mmap;
  };

public:
  ScratchFile(ScratchSpace *scratchSpace,
              Int64 fileSize,
              SortError* sorterror,
              CollHeap* heap,
              Int32 numOpens = 1,
              NABoolean breakEnabled=FALSE);
  ScratchFile(NABoolean breakEnabled);
  virtual ~ScratchFile();

  virtual RESULT checkScratchIO(Int32 index, DWORD timeout =0, NABoolean initiateIO = FALSE)=0;
  virtual void setEventHandler(ExSubtask *eh, IpcEnvironment *ipc,ex_tcb *tcb)=0;
  virtual RESULT getError(Int32 index) {return SCRATCH_FAILURE;} ;
  virtual RESULT isEOF(Int32 index, Int64 &iowaittime, Lng32 *transfered = NULL) = 0;
  virtual RESULT readBlock(Int32 index, char *data, Lng32 length, Int64 &iowaittime,
        		   Lng32 *transfered = NULL, Int32 synchronous = 1
                           ) = 0;
  virtual RESULT queueReadRequestAndServe(AsyncIOBuffer *ab, Int64 &ioWaitTime);
  virtual RESULT seekEnd(Int32 index, DWORD &eofAddr, Int64 &iowaittime, Lng32 *transfered = NULL) = 0;
  virtual RESULT seekOffset(Int32 index, Lng32 offset, Int64 &iowaittime, 
                            Lng32 *transfered = NULL,
                            DWORD seekDirection = 0 /* FILE_BEGIN */) = 0;
  virtual RESULT writeBlock(Int32 index, char *data, Lng32 length, Int64 &iowaittime,
		      Int32 blockNum = 0, Lng32 *transfered =NULL , NABoolean waited = FALSE_L ) = 0;
  virtual RESULT serveAsynchronousReadQueue(Int64 &ioWaitTime, NABoolean onlyIfFreeHandles = FALSE, NABoolean waited = FALSE);
  virtual RESULT processAsynchronousReadCompletion(Int32 index);
  virtual RESULT completeSomeAsynchronousReadIO(DWORD timeout,ULng32 &eventIndex, Int64 &ioWaitTime)
    { return SCRATCH_FAILURE; }
  virtual RESULT completeAsynchronousReadIO(ULng32 eventIndex, Int64 &ioWaitTime)
    { return SCRATCH_FAILURE; }
  // Truncate file and cancel any pending I/O operation
  virtual void truncate(void) = 0;

  Lng32 getNumOfReads() { return numOfReads_; }
  Lng32 getNumOfWrites() { return numOfWrites_; }
  Lng32 getNumOfAwaitio() { return numOfAwaitio_; }

  virtual Int32 getFreeFileHandle(void) = 0;
  virtual NABoolean isAnyIOPending(void){ return FALSE; }
  void queueAsynchronousRead(AsyncIOBuffer *ab);
  AsyncIOBuffer * dequeueAsynchronousRead(void);

  SortError* getSortError() const { return sortError_; };
  NABoolean asynchronousReadQueueHasEntries(void) 
    { return asynchronousReadQueueHead_ != NULL; };
  virtual void setPreviousError(Int32 index, RESULT error) = 0; 
  virtual RESULT getPreviousError(Int32 index) = 0;
  Int32 getNumOpens(void){ return numOpens_; }

  virtual RESULT executeVectorIO(){return SCRATCH_FAILURE;};
  virtual NABoolean isVectorIOPending(Int32 index){ return FALSE;}
  virtual NABoolean isNewVecElemPossible(Int64 byteOffset, Int32 blockSize)
    { return FALSE;}
  virtual NABoolean isVectorPartiallyFilledAndPending(Int32 index)
  {return FALSE;}
  virtual void copyVectorElements(ScratchFile* newFile){};
  virtual Int32 getBlockNumFirstVectorElement(){return -1;};
  virtual void reset(){}
  virtual Int32 lastError() { return -1;}

  NABoolean breakEnabled() { return breakEnabled_;};
  Int64 &bytesWritten() { return bytesWritten_;};
  Int64 resultFileSize() { return resultFileSize_;}
  FileHandle1 fileHandle_[MAX_SCRATCH_FILE_OPENS];

protected:
			char fileName_[STFS_PATH_MAX];
  Int32 fileNameLen_;
  ScratchSpace *scratchSpace_;
  SortError* sortError_;
  CollHeap* heap_;
  Lng32 numReadsPending_ ;
  Lng32 numBytesTransfered_; // keep track of count returned by AWAITIOX
  Lng32 numOfReads_;
  Lng32 numOfWrites_;
  Lng32 numOfAwaitio_;
  Int64 bytesWritten_;
  Int32 primaryExtentSize_;
  Int32 secondaryExtentSize_;
  Int32 maxExtents_;
  Int64 resultFileSize_;
  Int32 numOpens_; //Indicates number of opens on file.
  NABoolean asynchReadQueue_; //indicates if the user needs asynchIObuffer use

private:
  // queue is empty if and only if both of the following members are 
  // NULL.
  AsyncIOBuffer * asynchronousReadQueueHead_;
  AsyncIOBuffer * asynchronousReadQueueTail_;
  NABoolean breakEnabled_;
};
#endif// SCRATCHFILE_BASE_H

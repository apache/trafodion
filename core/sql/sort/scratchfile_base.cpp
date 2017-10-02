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
******************************************************************************
*
* File:         ScratchFile.C
*                               
* Description:  This file contains the member function implementation for 
*               class ScratchFile. This class is used to encapsulate all 
*               data and methods about a scratch file.  
*                                                                 
*
******************************************************************************
*/
#include "ScratchFile.h"
#include "ScratchSpace.h"
#include "ex_stdh.h"
#include "ExStats.h"
#include "str.h"
#include "SortError.h"
#include <assert.h>

#ifndef FORDEBUG
#undef NDEBUG
#define NDEBUG
#endif

ScratchFile::ScratchFile(ScratchSpace *scratchSpace,
                         Int64 fileSize,
                         SortError* sorterror,
                         CollHeap* heap,
                         Int32 numOpens,
                         NABoolean breakEnabled) : 
  asynchronousReadQueueHead_(NULL),
  asynchronousReadQueueTail_(NULL) ,
  breakEnabled_(breakEnabled),
  bytesWritten_(0),
  resultFileSize_(fileSize),
  numOpens_(numOpens),
  scratchSpace_(scratchSpace),
  sortError_(sorterror),
  heap_(heap),
  numReadsPending_(0),
  numBytesTransfered_(0),
  fileNameLen_(0),
  numOfReads_(0),
  numOfWrites_(0),
  numOfAwaitio_(0),
  asynchReadQueue_(FALSE)
{
  for(Int32 i = 0; i < MAX_SCRATCH_FILE_OPENS; i++)
  {
    fileHandle_[i].fileNum = STFS_NULL_FHNDL;
    fileHandle_[i].IOBuffer = NULL;
    fileHandle_[i].IOPending = FALSE;
    fileHandle_[i].scratchFileConn = NULL;
    fileHandle_[i].associatedAsyncIOBuffer = NULL;
	fileHandle_[i].mmap= NULL;
  }
}

ScratchFile::~ScratchFile() 
{
    asynchronousReadQueueHead_ = NULL;
    asynchronousReadQueueTail_ = NULL;
}

void ScratchFile::queueAsynchronousRead(AsyncIOBuffer *ab)
{
    if (asynchronousReadQueueTail_)
    {
        // queue is non-empty; chain this one onto the end
        asynchronousReadQueueTail_->next_ = ab;
    }
    else
    {
        // queue is empty; this one becomes the only entry
        asynchronousReadQueueHead_ = ab;
    }
    
    ab->next_ = NULL;  // for safety; should already be NULL, of course
    asynchronousReadQueueTail_ = ab;
    
    ab->state_ = QUEUED;
}

AsyncIOBuffer * ScratchFile::dequeueAsynchronousRead(void)
{
    AsyncIOBuffer * ab = asynchronousReadQueueHead_;
    
    if (ab)
    {
        asynchronousReadQueueHead_ = ab->next_;
        if (!asynchronousReadQueueHead_)
          asynchronousReadQueueTail_ = NULL; // queue just went empty
        ab->next_ = NULL;  // to get rid of dangling pointer
        ab->state_ = READING;
    }
    
    return ab;  // return first in queue (or NULL if queue was empty)
}

//--------------------------------------------------------------------------
// serveAsynchronousReadQueue
//
// This function attempts to start reads for any SortMergeBuffers queued in
// the asynchronous read queue. Note that it may check for completion
// of already-started I/O, however this routine cannot be used to check
// completion in general -- if the asynchronous read queue is empty, it
// simply does nothing.
//--------------------------------------------------------------------------
RESULT ScratchFile::serveAsynchronousReadQueue(Int64 &ioWaitTime, NABoolean onlyIfFreeHandles, NABoolean waited )
{
    RESULT rc = SCRATCH_SUCCESS;        // assume success
    ioWaitTime = 0;
    Int32 index = -1;
    NABoolean atLeastOneReadInitiated = FALSE;
    NABoolean retry = FALSE;
    
    // if nothing to serve, return.
    if(!asynchronousReadQueueHasEntries())
      return SCRATCH_SUCCESS;

    //check to see if atleast one free handle is available.
    //If none available, try completing IO in nowait mode.
    if((index = getFreeFileHandle()) < 0 )
    {
      if(onlyIfFreeHandles)
      {
        return SCRATCH_SUCCESS;
      }

      for(index = 0; index < numOpens_; index++)
      {
        rc = checkScratchIO(index);
        if ((rc != SCRATCH_SUCCESS) && (rc != IO_COMPLETE) &&
           (rc != IO_NOT_COMPLETE))
          return rc;
      }
    }

    do
    {
      while(((index = getFreeFileHandle()) >= 0 ) &&
           asynchronousReadQueueHasEntries())
      {
          AsyncIOBuffer * ab = dequeueAsynchronousRead();
          ab->tag_ = 0;
          
          // Position to the current offset here
          rc = seekOffset(index, ab->currentIOByteOffset_, ioWaitTime);
          if (rc != SCRATCH_SUCCESS)
          {
            ab->state_ = READCOMPLETE;
            ab->retval_ = rc;
            return rc;
          }
          
          // Now do the read
          numReadsPending_++; 
          fileHandle_[index].associatedAsyncIOBuffer = ab;
          rc = readBlock(index, ab->scrBlock_, scratchSpace_->getBlockSize(), ioWaitTime);
          if (rc != SCRATCH_SUCCESS)
          {             
             // I/O failed
             numReadsPending_--;
             fileHandle_[index].associatedAsyncIOBuffer = NULL;
             ab->state_ = READCOMPLETE;
             ab->retval_ = rc;
             return rc;
          }
          if (scratchSpace_->bmoStats())
             scratchSpace_->bmoStats()->incScratchBufferBlockRead();
          atLeastOneReadInitiated = TRUE;
      }
      
      //if waited call and not even one Io initiated, we need to 
      //complete for IO, take the free handle and issue IO.  
      if(!atLeastOneReadInitiated && waited && !retry)
      {
        //Perform a checkScratchIO in waited mode on file handle 0.
        index = 0;
        rc = checkScratchIO(index, INFINITE);
        if ((rc != SCRATCH_SUCCESS) && (rc != IO_COMPLETE))
           return rc;
        retry = TRUE;

        //reset rc to failure and expect it to turn SUCCESS upon retry.
        //if retry fails, then default rc value of SCRATCH_FAILURE is 
        //returned.
        rc = SCRATCH_FAILURE;
      }
      else
        retry = FALSE;
    }while(retry);
  
  return rc;  
}

RESULT ScratchFile::processAsynchronousReadCompletion(Int32 index)
{
    RESULT rc = SCRATCH_SUCCESS;      // assume success
    
    // NSK can only have one I/O pending.
    numReadsPending_--;
    AsyncIOBuffer * completedAB = fileHandle_[index].associatedAsyncIOBuffer;
    
    completedAB->tag_ = -1;
    completedAB->state_ = READCOMPLETE;
    completedAB->processMisc();
    
    //Check if the numbers of bytes returned by AWAITIOX is the same as the length
    // asked for.
    if (numBytesTransfered_ == scratchSpace_->getBlockSize())
      completedAB->retval_ = SCRATCH_SUCCESS;
    else
    {
        // got back less than we asked for???
        char msg[100];
        str_sprintf(msg, "ScratchFile::processAReadCompn, volume %s",fileName_);         
        sortError_->setErrorInfo(EWrongLengthRead, // sort error
                          NULL,         // syserr: the actual FS error
                          NULL,         // syserrdetail
                          msg);
        completedAB->retval_ = OTHER_ERROR;
        rc = OTHER_ERROR;
    }
    
    
    return rc;
}


RESULT ScratchFile::queueReadRequestAndServe(AsyncIOBuffer *ab, 
      Int64 &ioWaitTime)
{
      // queue the SortMergeBuffer for asynchronous I/O
      queueAsynchronousRead(ab);
      
      // cause the queue to be served
      
      return serveAsynchronousReadQueue(ioWaitTime,FALSE);
}

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
#ifndef SCRATCHFILE_SQ_H
#define SCRATCHFILE_SQ_H

/* -*-C++-*-
******************************************************************************
*
* File:         ScratchFile_sq.h
*                               
* Description:  This file contains the member function implementation for 
*               class SQScratchFile. This class is used to encapsulate all 
*               data and methods about a scratch file for SeaQuest.  
*                                                                 
* Created:      01/02/2006
* Language:     C++
* Status:       $State: Exp $
*
*
*
******************************************************************************
*/
#include "Platform.h"
#include "stfs/stfslib.h"
#include "ScratchFile_base.h"

typedef enum {PEND_NONE, PEND_READ, PEND_WRITE} EPendingIOType;

class SQScratchFile : public ScratchFile {
public:
  SQScratchFile(char* filename, ScratchSpace *scratchSpace,
       SortError* sorterror, CollHeap* heap = 0,
       NABoolean breakEnabled = FALSE,
       Int32 scratchMaxOpens = 1,
       NABoolean asynchReadQueue = TRUE);

  //-----------------------------------------------
  // The destructor.
  //-----------------------------------------------
  virtual ~SQScratchFile(void);

  // This function returns the status of the
  // asynchronous function called right before it,
  // if any, or returns SCRATCH_SUCCESS otherwise.
  // When we say right before it we mean that
  // this function is the next public function of
  // this class called.
  //-----------------------------------------------
  virtual RESULT checkScratchIO(Int32 index, DWORD timeout =0, NABoolean initiateIO = FALSE);
  virtual void setEventHandler(ExSubtask *eh, IpcEnvironment *ipc, ex_tcb *tcb){};

  virtual RESULT getError(Int32 index);
  //-----------------------------------------------
  // This function will return wether the current
  // position is at the end of the file or not.
  // If the file pointer is equal or greater to
  // the EOF, then this function returns SCRATCH_SUCCESS,
  // else it returns SCRATCH_FAILURE.
  //-----------------------------------------------
  virtual RESULT isEOF(Int32 index, Int64 &iowaittime, Lng32 *transfered = NULL)
  {
     return SCRATCH_FAILURE;
  }

  //-----------------------------------------------
  // This function reads length bytes and put it
  // in the char array, starting at the file
  // pointer.
  //-----------------------------------------------
  virtual RESULT readBlock(Int32 index, char *data, Lng32 length, Int64 &iowaittime,
			 Lng32 *transfered = NULL, Int32 synchronous = 1
                         );
  
  //-----------------------------------------------
  // This function moves the file pointer to the
  // end of the file.
  //-----------------------------------------------
  virtual RESULT seekEnd(Int32 index, DWORD &eofAddr, Int64 &iowaittime, Lng32 *transfered = NULL)
  { return SCRATCH_SUCCESS; }
 
  //-----------------------------------------------
  // This function moves the file pointer to the
  // desired offset from the start. Note that we use O_APPEND flag during file open.
  // For write operation, this essentially moves the file pointer automatically 
  // to the end offset. Ofcourse we need seekoffset for read operations.
  //-----------------------------------------------
 virtual  RESULT seekOffset(Int32 index, Lng32 offset, Int64 &iowaittime,
			                    Lng32 *transfered = NULL,
                             DWORD seekDirection=0
                           );

  //-----------------------------------------------
  // This function writes length bytes from the
  // char array into the file, starting from
  // the file pointer.
  //-----------------------------------------------
  virtual RESULT writeBlock(Int32 index, char *data, Lng32 length, Int64 &iowaittime, Int32 blockNum = 0,
			    Lng32 *transfered = NULL, NABoolean waited = FALSE_L);

  virtual void setPreviousError(Int32 index, RESULT error) { fileHandle_[index].previousError = error;}
  virtual RESULT getPreviousError(Int32 index) { return fileHandle_[index].previousError;}
  // Truncate file and cancel any pending I/O operation
  virtual void truncate(void);
  virtual Int32 getFreeFileHandle(void)
  {
	  if (! fileHandle_[0].IOPending )
         return 0;								//means, a file handle is available for IO
      else
         return -1;								//means, no file handle is free for any new IO.
  }
    
  NABoolean checkDirectory(char *path);

  Int32 getFileNum(Int32 index)
  {
     return fileHandle_[index].fileNum;
  }

  virtual RESULT executeVectorIO();
  virtual NABoolean isVectorPartiallyFilledAndPending(Int32 index)
  { 
    return ((vectorIndex_ > 0) && (fileHandle_[index].IOPending == FALSE));
  }
  virtual NABoolean isNewVecElemPossible(Int64 byteOffset, Int32 blockSize);

  virtual void copyVectorElements(ScratchFile* newFile);
  virtual Int32 getBlockNumFirstVectorElement()
    { return blockNumFirstVectorElement_;}

  virtual Int32 lastError() { return lastError_;}

  virtual void reset()
  {
    vectorIndex_ =0;
    bytesRequested_ = 0;
    bytesCompleted_ = 0;
    remainingAddr_ = NULL;
    remainingVectorSize_ = 0;
    vectorSeekOffset_ = 0;
    blockNumFirstVectorElement_ = -1;
    type_ = PEND_NONE;
    lastError_ = 0;
  }
  
private:
  // redriveIO will return if complete all bytes request or timeout occurred.
  Int32 redriveIO(Int32 index, Lng32& count, Lng32 timeout = -1); //AWAITIOX emulation.

  RESULT doSelect(Int32 index, DWORD timeout, EPendingIOType type, Int32& err);
  Int32 redriveVectorIO(Int32 index);



  //-----------------------------------------------
  // This function is used internally to get the
  // actual error.
  //-----------------------------------------------
  short obtainError();
  
  //-----------------------------------------------
  // I have no idea what these do.
  //-----------------------------------------------
  Lng32 ioWaitTime_;

  //For vector IO
  struct iovec    *vector_;           //vector elements
  Int32           vectorSize_;        //max number of vector elements
  Int32           vectorIndex_;       //number of vector elements setup
  ssize_t         bytesRequested_;
  ssize_t         bytesCompleted_;
  void            *remainingAddr_;      //adjusting pointers for redrive IO
  Int32           remainingVectorSize_; //adjusting pointers for redrive IO
  Int64           vectorSeekOffset_;    //beginning seek offset for vector IO
  Int64           writemmapCursor_;  	//Only used in mmap as append cursor.
  Int64           readmmapCursor_; 		//Only used in mmap as read cursor.
  
  //This is the block num of the block that corresponds to
  //first element in the vector. This is especially used
  //to recover from a ENOSPC. The entire vector is copied
  //to another instance of scratchFile and block num from this
  //member is updated in the map file.
  Int32           blockNumFirstVectorElement_; 
  
  // type_ can be any of enum types defined by enum. The following state changes
  // take place.  PEND_NONE-->PEND_READ or PEND_WRITE -->PEND_NONE
  // PEND_NONE:   Vector is empty, No read or write requests is registered. 
  // PEND_WRITE:  Vector is registred with atleast one write request. No read
  //              requests can be registred now.
  // PEND_READ:   Vector is registred with atleast one read request. No write
  //              request can be registred now.
  // Notes:       Note that type_ just indicates if vector type. It does not 
  //              indicate if an actual IO is in flight. To check if IO is in 
  //              flight, check the fileHandle_[index].IOPending flag.
  EPendingIOType  type_;              //indicates type of IO vector being builtup.
  Int32           vectorWriteMax_;   //For diagnostics only
  Int32           vectorReadMax_;    //For diagnostics only
  Int32           lastError_;
#ifdef _DEBUG
  char			*envIOPending_;		//for simulating IO pending state.
  Int32          envIOBlockCount_;	//for simulating IO pending state.
#endif
};



#endif // SCRATCHFILE_SQ_H

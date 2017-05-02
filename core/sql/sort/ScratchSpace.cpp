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
* File:         ScratchSpace.C
*                               
* Description:  This file contains the implementation of all member functions
*               of class ScratchSpace which provides a virtual view of 
*               ScratchSpace to the SortAlgorithm class. Details about the
*               actual physical scratch disk file management is hidden to 
*               other components. 
*               
* Created:	    04/25/96
* Language:     C++
*
*
*
*
******************************************************************************
*/


#include "Platform.h"

//-----------------------------------------------------------------------
// Standard Include files.
//-----------------------------------------------------------------------

#include <iostream>
#include <string.h>

#include <errno.h>

//-----------------------------------------------------------------------
// Product Include files.
//-----------------------------------------------------------------------
#include "ex_stdh.h"
#include "ComDistribution.h"
#include "ScratchSpace.h"
#include "ScratchFileMap.h"
#include "ex_god.h"
#include "ex_exe_stmt_globals.h"
#include "ExpError.h"
#include "ExStats.h"
//----------------------------------------------------------------------- 
// Forward Function Declarations.
//-----------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Constructors.
//  Sometimes users do not wish to allocate scratch blocks.  Typically these
//  users wish to write data directly from users' buffer to the scratch file
//  via writeThru or read data directcly from scratch file to users' buffer 
//  via readThru.  Hence there's no need to allocate the intermediate scratch 
//  blocks and there's no need to copy users' data to/from the intermediate 
//  scratch block prior to any scratch IO. These users use a scratchSpace
// for their own purposes and do not need a run directory.
//-----------------------------------------------------------------------------

ScratchSpace::ScratchSpace(CollHeap* heap, SortError* sorterror, 
                           Lng32 blocksize,
                           Int32 scratchIOVectorSize,
                           Int32 explainNodeId, 
			   NABoolean logInfoEvent,
                           Int32 scratchMgmtOption): 
sortError_(sorterror),
heap_(heap),
explainNodeId_(explainNodeId),
diskPool_ (NULL),
scratchMgmtOption_(scratchMgmtOption)
{
    blockSize_ = blocksize;
    
    
    currentIOScrFile_ = NULL;
    currentWriteScrFile_ = NULL;
    previousWriteScrFile_ = NULL;
    currentReadScrFile_  = NULL;
    
    // set breakEnabled to FALSE for now.
    scrFilesMap_ = new (heap_) ScratchFileMap(heap_,
                                    sorterror,
                                    FALSE,  // Break Enabled?
                                    MAXSCRFILES);
    // Allocation failure here usually should result in long jmp. 
    // Retaining below code just incase jmp handler is not defined.
    if (scrFilesMap_ == NULL)
    {
        sortError_->setErrorInfo( EScrNoMemory   //sort error
            ,NULL          //syserr: the actual FS error
            ,NULL          //syserrdetail
            ,"ScratchSpace::ScratchSpace"     //methodname
            );
        
    }
    
    ex_assert(scrFilesMap_ != NULL, "ScratchSpace::ScratchSpace, scrFilesMap_ is NULL");
    totalNumOfScrBlocks_ = 1L;
    totalIoWaitTime_     = 0;
    scratchDirListSpec_ = NULL;
    numDirsSpec_ = 0;
    numEsps_ = 0;
    espInstance_ = 0;
    logInfoEvent_ = logInfoEvent;
    logDone_ = FALSE;
    preAllocateExtents_ = FALSE;
    scratchDiskLogging_ = FALSE;
    asyncReadQueue_ = FALSE;
    scratchMaxOpens_ = 1;
    bmoStats_ = NULL;
    scratchIOVectorSize_ = scratchIOVectorSize;
    ovMode_ = SCRATCH_DISK;
} 

//-----------------------------------------------------------------------
// Class Destructors.
// delete indirect space in ScratchSpace
//-----------------------------------------------------------------------
ScratchSpace::~ScratchSpace(void)
{
    if (scrFilesMap_ != NULL) {
        delete scrFilesMap_;
        scrFilesMap_ = NULL;
    }
    if (diskPool_ != NULL) {    		
        delete diskPool_;
        diskPool_ = NULL;
    }
}
void ScratchSpace::setCallingTcb(ex_tcb *tcb)
{
  callingTcb_ = tcb;
  if (callingTcb_ != NULL)
  {
    ExOperStats *stat = callingTcb_->getStatsEntry();
    if (stat)
      bmoStats_ = stat->castToExBMOStats();
  }
  if (bmoStats_ != NULL)
     bmoStats_->setScratchIOSize((scratchIOVectorSize_ * blockSize_) >> 10);
}
  
//-----------------------------------------------------------------------
// Name         : CreateANewScrFileAndWrite
// 
// Parameters   : ...
//
// Description  : This function is used when the current scratch file is
//                full. A new scratch file is automatically created and 
//                and the buffer is written to the file.
//
// Return Value : Adhering to return only this subset of values is important.
//   SORT_SUCCESS if everything goes on well.
//   SORT_FAILURE if any error encounterd.
//   IO_NOT_COMPLETE  if IO is pending
//
/* Scenarios:
  Sort:
  -------
  1.	Sort operator encounters ENOSPC error during write operation.
    a.ScratchSpace::WriteFile() encounters  this error and createsANewScratchFile() is invoked that takes of writing to new scratch file.

  2.	Sort operator overflow with vector partially filled and no more records to overflow.
    a.	Sort vector is always set to 1. This will not get into the situation of having partially filled vector. 
    b.	Sort uses double buffer, each of size 512kb. Before each buffer gets reused, any Io on that buffer is completed forcibly. 

  3.	Subsequent writes encounter 2gb file limit in scratchspace. Triggers creating a new scratch file.          How does sort react?
    a.	ScratchSpace::WriteFile() encounters  this and createsANewScratchFile().
    b.	Because, vector for sort is set to 1, followed by double buffer being written serially, there is no      scenario of completing IO on previous file.
  Hash:
  ------
  4.	Hash operator encounters ENOSPC error during write operation 
    a.	ScratchSpace::WriteFile() encounters  this error and createsANewScratchFile() handles this.

  5.	ScratchSpace::CheckIO() encounters ENOSPC error when trying to complete a pending write operation.
    a.	ScratchSpace::CheckIO() invokes createsANewScratchFile() that takes of writing to new scratch file.

  6.	Hash operator partially fills vector and no more records to overflow.
    a.	Hash operator follows the protocol of invoking scratchSpace::checkio(checkAll) when switching between write and read  operations. This causes any partially filled vector to be flushed out to disk and complete any pending writes. Any ENOSPC errors encountered during this operation becomes scenario 5.

  7.	Vector is partially filled, however subsequent writes encounter self imposed 2gb file limit in scratchspace. Triggers creating a new scratch file. What happens to partially filled vector?
    a.	ScratchSpace::WriteFile() encounters this check. CreateANewScratchFile() is invoked which will accept the buffer as new vector element in new scratch file. Vector elements in the previous scrath file that are partially filled will get flushed by scratchSpace::checkio(checkall).
*/
RESULT ScratchSpace::CreateANewScrFileAndWrite(char *buffer, Int32 blockNum, UInt32 blockLen, NABoolean waited)
{
    RESULT retval = SCRATCH_SUCCESS;
    
   
    Int64 iowaittime = 0;
   
    // Before creating the new scratch file, make sure if there are any
    // pending IOs on the current scratch file. If there is, then create 
    // preserve the scratch file pointer in previousWriteScrFile_ so that IOs
    // that are pending can be completed.
    // Also, if previousWriteScrFile_ is not NULL, complete all pending IO
    // on previousWriteScrFile_. This is a unlikely situation.

    // On NA_LINUX, this scenario of having multiple scratch files is not possible
    // because there is only one file handle and one scratch file at a time. The
    // scenario of creating a new scratch file when there is a pending IO is not
    // possible.
    if(previousWriteScrFile_ != NULL)
    {
      for(Int32 index = 0; index < scratchMaxOpens_; index++)
      {
        RESULT retval = previousWriteScrFile_->checkScratchIO(index,INFINITE);
          if((retval != IO_COMPLETE) && (retval != SCRATCH_SUCCESS))
            return retval;
      }
      previousWriteScrFile_ = NULL;
    }

    if((currentWriteScrFile_ != NULL) && (currentWriteScrFile_->isAnyIOPending()))
       previousWriteScrFile_ = currentWriteScrFile_;

    //keep a reference to this writeScratch file to help retrieve
    //a vector and transfter it to new scratch file.
    ScratchFile *tempRefScrFile = currentWriteScrFile_;

    currentIOScrFile_ = currentWriteScrFile_ 
        = scrFilesMap_->createNewScrFile(
                                this,
                                scratchMgmtOption_,
                                scratchMaxOpens_,
                                preAllocateExtents_,
                                asyncReadQueue_);

   
    if (currentIOScrFile_ == NULL)
    {
       //Note that scratch File create error is already populated in
       //sortError_.
       return SCRATCH_FAILURE;
    }
   if (bmoStats_)
    {
      bmoStats_->setScratchBufferBlockSize(getBlockSize());
      bmoStats_->incSratchFileCount();
    }
 
    currentWriteScrFile_->setEventHandler(ioEventHandler_,ipcEnv_,callingTcb_);
    
    Int32 fileIndex = currentWriteScrFile_->getFreeFileHandle();
    if(fileIndex < 0)
    {
        sortError_->setErrorInfo(EUnexpectErr// should not happen
            ,NULL            //syserr: the actual FS error
            ,NULL            //syserrdetail
            ,"ScratchSpace::CreateANewScrFileAndWrite, getFreeFileHandle" //methodname
            );
        return SCRATCH_FAILURE;
    }

    //Writing to a file may return ENOSPC error indicating that there are no
    //more extents available. When this happens, we need to copy any vector
    //vector elements into new scratch file and write. Note that in this
    //scenario, blocknum, buffer arguments passed to creatANewscratchFile
    //function are null or not valid.
    if(tempRefScrFile != NULL && tempRefScrFile->lastError() == ENOSPC)
    {
      //first retrieve the vector from the previous scratch file along with
      //the first block number in the vector. CopyVectorElements will initialize
      //currentWriteScrFile_ including the states so that it can drive the IO.
      tempRefScrFile->copyVectorElements(currentWriteScrFile_);

      //After copying all the vector elements, we need to reset the vector
      //counters of tempRefScrFile so that future read operations on this
      //file does not get confused.
      tempRefScrFile->reset();
  
      //reference the blocknum to the blocknum of first vector element.
      //this is needed to update the new scratch file map.
      blockNum = currentWriteScrFile_->getBlockNumFirstVectorElement();
  
      //Now drive the IO.
      //This method should return SCRATCH_SUCCESS if IO is initiated or
      //Io completes. Note that if IO is initiated(IOPending flag is TRUE)
      //this method does not return IO_NOT_COMPLETE. It is the job of checkScratchIO()
      //to check if IO is completed or not and interpret the error code accordingly.
      retval = currentWriteScrFile_->executeVectorIO();
  
      if (retval == SCRATCH_FAILURE)
      {
          // Some Serious problem here since we just created a new scratch
          // file and are unable to write a the first block to it.
          // It could also be that the disk is full and there is no more
          // extents to allocate. We could check for ENOSPC error here, but
          // in any case, we should return any error back to user if we reach
          // reach here.
  
          // Note that scratch File error is already populated in
          // sortError_.
          return retval; 
      }
  
      // if we reach here means that IO has either been initiated succesfully or
      // IO completed successfully. Since currentWriteScrFile_->executeVectorIO()
      // does not return IO_NOT_COMPLETE( if IO is pending) (this is by design)and
      // CreateANewScrFileAndWrite() has to return IO_NOT_COMPLETE if it is the case,
      // lets check for IO completion here before returning success.
      // Note checkScratchIO() can return IO_COMPLETE, IO_NOT_COMPLETE,
      // SCRATCH_SUCCESS, SCRATCH_FAILURE and FILE_FULL only.
      retval = currentWriteScrFile_->checkScratchIO(fileIndex);
    }
    else
    {
      //When creating a scratch file for the first time, we can reach here.
      //In this scenario, just allow writing to the file.
      //We can also reach here if the file has reached 2gb self imposed limit,
      //in which case, tempRefScrFile may or may not have vector elements populated.
      //tempRefScrFile that have vector elements populated will be flushed by
      //ScratchSpace::CheckIO(CheckAll)
      retval = currentWriteScrFile_->writeBlock(fileIndex, buffer ,blockLen,
                                iowaittime, blockNum, NULL, waited);
    }
    

    // In the case of NSK or NT, this check will validate the writeBlock()
    // return value. In the case of linux, this check will validate the
    // return value of checkScratchIO()
    if ((retval == SCRATCH_FAILURE) || (retval == FILE_FULL))
    {
        // Some Serious problem here since we just created a new scratch
        // file and are unable to write a the first block to it.

        //Note that scratch File create error is already populated in
        //sortError_.        
        return retval; 
    }

    if (bmoStats_)
      bmoStats_->incScratchBufferBlockWritten();

    //record scratch block number associated with the 1st block of the newly
    //created scratch file
    scrFilesMap_->setFirstScrBlockNum(blockNum);
    if (retval == IO_NOT_COMPLETE)
       return retval;
    else
    { // The write to the new scratch file was successful
      totalIoWaitTime_ += iowaittime;
      return SCRATCH_SUCCESS;
    }
    
}
//-----------------------------------------------------------------------
// Name         : writeThru 
// 
// Parameters   : ...
//
// Description  : Client is opt to write data directly to the scratch
//                file via this function.  Unlike the writeData method, 
//                no buffering is done.  Whatever is passed in the 1st 
//                parameter buf is directly written out to the disk as 
//                it is. When the current file or volume gets full, a 
//                new scratch file is created automatically and client's 
//                data is written to the newly created scratch file. 
//            
// Return Value :
//   SORT_SUCCESS if everything goes on well.
//   SORT_FAILURE if any error encounterd.
//
// Usage: Only used by swapspace.cpp. HJ and Sort do not use this method
//       anymore.
//
//-----------------------------------------------------------------------
RESULT ScratchSpace::writeThru(char* buf, ULng32 len,
                                  DWORD &blockNum) 								  
{
    Int64 iowaittime = 0;
    RESULT retval = WRITE_EOF;
    DWORD byteAddr = 0;
    Int64 byteOffset = 0;
    Int32 fileIndex = -1; 

    // ScratchSpace::WriteThru is only applicable for single opens to 
    // scratch file. Return error if that is not the case.
    if(scratchMaxOpens_ > 1)
    {
      sortError_->setErrorInfo( EUnexpectErr   //sort error
                       ,NULL               //syserr: the actual FS error
                       ,NULL               //syserrdetail
                       ,"ScratchSpace::writeThru" //methodname
                      );
      return SCRATCH_FAILURE;
      
    }
#if defined(_DEBUG)
    // Regression testing support for simulating running out of available
    // scratch disks.  Used for merge join overflow testing.
    char* envDiskFull = getenv("SCRATCH_TEST_THRESHOLD");
    if (envDiskFull)
    {
      Int16 maxBlocks = (Int16) str_atoi(envDiskFull, str_len(envDiskFull));
      if ((maxBlocks > 0) && (totalNumOfScrBlocks_ > maxBlocks))
      {
        sortError_->setErrorInfo( EThresholdReached   //sort error
                                 ,NULL               //syserr: the actual FS error
                                 ,NULL               //syserrdetail
                                 ,"ScratchSpace::writeThru"     //methodname
                                );
       return SCRATCH_FAILURE;
      }
    }
#endif
    blockNum = totalNumOfScrBlocks_; //block number to be written counting from 1
    
    //---------------------------------------------------------------
    // First Position to end of file
    //---------------------------------------------------------------
    if (currentWriteScrFile_ != NULL) 
    {
        fileIndex = currentWriteScrFile_->getFreeFileHandle();
        if(fileIndex != 0)
        {
            sortError_->setErrorInfo(EUnexpectErr// should not happen
                ,NULL            //syserr: the actual FS error
                ,NULL            //syserrdetail
                ,"ScratchSpace::writeThru, getFreeFileHandle" //methodname
                );
            return SCRATCH_FAILURE;
        }
        retval = currentWriteScrFile_->seekEnd(fileIndex, byteAddr, iowaittime); 
        if (retval ) 
        {
             // sys error 45 or 43 means reached EOF or no more
             // extents available for allocation.
             if ((sortError_->getSysError() == 45) ||
                 (sortError_->getSysError() == 43))
             {
               retval = FILE_FULL;
               sortError_->initSortError();
             }
            else
             {
              return SCRATCH_FAILURE;
             }
        }
        totalIoWaitTime_ += iowaittime;     
    }	
    
    byteOffset = byteAddr;    // Note !Using 64 bit addition
    byteOffset += len;
    if (byteOffset > SCRATCH_FILE_SIZE)
    {
        // Assume for now that the file is full after reaching the
        // 2GB limit. This may be changed if we use another data
        // type for the file offset.
        retval = FILE_FULL;
    }
    //---------------------------------------------------------------
    // The following logic in the if statement checks to see if a 
    // a scratchfile exists or whether a write to the scratch file 
    // fails. In either case it tries to create a new scratch file
    // and writes the block to the scratch file.
    //---------------------------------------------------------------  
    if ((currentWriteScrFile_ != NULL) && (retval != FILE_FULL) &&(fileIndex >=0) )
    {
        retval = currentWriteScrFile_->writeBlock(fileIndex, buf, len, iowaittime, blockNum);
        totalIoWaitTime_ += iowaittime;
        if (retval == SCRATCH_SUCCESS && bmoStats_)
         bmoStats_->incScratchBufferBlockWritten();

        //Note the difference. On Linux, we can get a ENOSPC when a write attempt
        //is made. On NSK, a seek operation might give no space error because NSK
        //allocates extents upon seek operation. On Linux, seek does not allocate
        //extents. Hence we need to check for ENOSPC error here.
        if(retval == SCRATCH_FAILURE)
        {
          if(sortError_->getSysError() == ENOSPC)
            retval = FILE_FULL;
        }
    }
    
    if ( (currentWriteScrFile_ == NULL) || (retval != SCRATCH_SUCCESS) )
    {   
        switch (retval)
        {
        case WRITE_EOF:
        case FILE_FULL:
            //initialize, since WRITE_EOF or FILE_FULL errors could have populated
            //error details. We can ignore them, since we are creating a new scratch
            //file now.
            sortError_->initSortError();
            retval  = CreateANewScrFileAndWrite(buf, totalNumOfScrBlocks_, len);
               if (retval == IO_NOT_COMPLETE) 
                  return IO_NOT_COMPLETE;
               if (retval != SCRATCH_SUCCESS)
                return SCRATCH_FAILURE;
               else
                retval = SCRATCH_SUCCESS;
            
            break;
        case IO_NOT_COMPLETE:
           return IO_NOT_COMPLETE;
        default :
            return SCRATCH_FAILURE;
        } // end of switch(retval) 
    }   // end of if  ((currentWriteScrFile_ ...
    totalNumOfScrBlocks_++;
    currentWriteScrFile_->bytesWritten() += len;
    return SCRATCH_SUCCESS;
}
//-----------------------------------------------------------------------
// Name         : readThru
// 
// Parameters   : ...
//
// Description  : Reads a record from the scratch file.
//  
// Return Value :
//   SORT_SUCCESS if everything goes on well.
//   SORT_FAILURE if any error encounterd. 
//
//-----------------------------------------------------------------------
RESULT ScratchSpace::readThru(char *buf, 
    Lng32 blockNum, //block# starting from one
    ULng32 buflen,
    ScratchFile *readScratchFile,
    Int32 readBlockOffset)
{
    // sanity check
    ex_assert(buflen == blockSize_,"ScratchSpace::readThru buffer size mismatch");

    if ((blockNum < 0) ||( blockNum > totalNumOfScrBlocks_)) {
        sortError_->setErrorInfo( EInvScrBlockNum       //sort error
            ,NULL                  //syserr: the actual FS err
            ,NULL                  //syserrdetail
            ,"ScratchSpace::readThru" //methodname
            );
        return SCRATCH_FAILURE;
    }

    Int64 iowaittime = 0;
    Lng32 blockOffset = 0, byteOffset = 0;
    Int32 fileIndex = -1;
  
    if(readScratchFile != NULL)
    {
      currentIOScrFile_ = currentReadScrFile_
                  = readScratchFile;
      blockOffset = readBlockOffset;
    }
    else
    { 
      currentIOScrFile_ = currentReadScrFile_
          = scrFilesMap_->mapBlockNumToScrFile(blockNum, blockOffset);
      
      if (currentReadScrFile_ == NULL)
      {
          sortError_->setErrorInfo( EScrFileNotFound      //sort error
                                   ,NULL                  //syserr: the actual FS err
                                   ,NULL                  //syserrdetail
                                   ,"ScratchSpace::readThru" //methodname
                                   );

          return SCRATCH_FAILURE;
      }
    }
    
    //byteOffset is the offset within a scratch file pointing to the exact byte.
    //blockOffset is the offset of 56kb block from the beginning of the scratch file
    byteOffset = blockOffset * blockSize_;

    fileIndex = currentReadScrFile_->getFreeFileHandle();
    if(fileIndex < 0)
    {
        sortError_->setErrorInfo(EUnexpectErr// should not happen
            ,NULL            //syserr: the actual FS error
            ,NULL            //syserrdetail
            ,"ScratchSpace::readThru, getFreeFileHandle" //methodname
            );
        return SCRATCH_FAILURE;
    }

    RESULT retval = currentReadScrFile_->seekOffset(fileIndex, byteOffset, iowaittime);
    totalIoWaitTime_ += iowaittime;
    if (retval)
        return retval;
    
    retval = currentReadScrFile_->readBlock(fileIndex, buf, blockSize_, iowaittime,
        0 /* asynchronous read */);
    if (retval != SCRATCH_SUCCESS) { return retval; } //read returns error or eof or IO_NOT_COMPLETE
    totalIoWaitTime_ += iowaittime;
   if (bmoStats_)
        bmoStats_->incScratchBufferBlockRead();
    
    return SCRATCH_SUCCESS;  
}

// Position and then write, given the file and block number.
RESULT ScratchSpace::writeFile(char* block,
                      UInt32 blockNum,
                      UInt32 blockLen )
{
    RESULT retval = WRITE_EOF;
    Lng32 blockOffset = 0;
    Int64 byteOffset = 0;  
    Int64 iowaittime = 0;
    Int32 fileIndex = -1;
    
    //---------------------------------------------------------------
    // First Position to the appropriate offset.
    //---------------------------------------------------------------
    if (currentWriteScrFile_ != NULL) {//assume writing to the current write file
        // get 1st scr block num on the current scr file
        SBN firstBlockNum =
            scrFilesMap_->getFirstScrBlockNum(currentWriteScrFile_);
        if (firstBlockNum == -1) {
            sortError_->setErrorInfo( EScrFileNotFound//sort error
                ,NULL            //syserr: the actual FS error
                ,NULL            //syserrdetail
                ,"ScratchSpace::WriteFile" //methodname
                );
            return SCRATCH_FAILURE;
        } 
        blockOffset = blockNum - firstBlockNum;
        byteOffset = blockOffset;
        byteOffset *= blockLen; // be careful to do this in 64 bit arithmetic
         
        if ((byteOffset + blockLen) > currentWriteScrFile_->resultFileSize() )
        {
            retval = FILE_FULL;
        }
        else
        {
            // Get a free file handle to perform IO. Note there could be
            // multiple file handles per scratch file based on the number of 
            // scratch file opens.
            fileIndex = currentWriteScrFile_->getFreeFileHandle();
            if(fileIndex < 0)
            {
                sortError_->setErrorInfo(EUnexpectErr// should not happen
                    ,NULL            //syserr: the actual FS error
                    ,NULL            //syserrdetail
                    ,"ScratchSpace::WriteFile, getFreeFileHandle" //methodname
                    );
                return SCRATCH_FAILURE;
            }
            Lng32 lByteOffset = (Lng32) byteOffset;
            retval = currentWriteScrFile_->seekOffset(fileIndex, lByteOffset, iowaittime);         
            if (retval)
            {
             //Error 45 or 43 means reached EOF or no more
             // free extents to allocate.
	      if ((sortError_->getSysError() == 45) ||
		  (sortError_->getSysError() == 43))
               {
               retval = FILE_FULL;
               sortError_->initSortError();
               }
              else
              {
                return SCRATCH_FAILURE;
              }
               
            }    
            totalIoWaitTime_ += iowaittime;
        }
    }
    
    //---------------------------------------------------------------
    // The following logic in the if statement checks to see if a 
    // a scratchfile exists or whether a write to the scratch file 
    // fails. In either case it tries to create a new scratch file 
    // and writes the block to the scratch file.
    //---------------------------------------------------------------   
    if (currentWriteScrFile_ != NULL && retval != FILE_FULL && (fileIndex >= 0) ) { //?? added if
        retval = currentWriteScrFile_->writeBlock(fileIndex, block, blockLen,
            iowaittime, blockNum, NULL, FALSE);
       if (retval == SCRATCH_SUCCESS  && bmoStats_)
          bmoStats_->incScratchBufferBlockWritten();
        totalIoWaitTime_ += iowaittime;

        //Note the difference. On Linux, we can get a ENOSPC when a write attempt
        //is made. On NSK, a seek operation might give no space error because NSK
        //allocates extents upon seek operation. On Linux, seek does not allocate
        //extents. Hence we need to check for ENOSPC error here.
        if(retval == SCRATCH_FAILURE)
        {
          if(sortError_->getSysError() == ENOSPC)
            retval = FILE_FULL;
        }
    }
    if ( (currentWriteScrFile_ == NULL) || (retval != SCRATCH_SUCCESS) ) {
        switch (retval)
        {
        case WRITE_EOF : 
        case FILE_FULL :
            //initialize, since WRITE_EOF or FILE_FULL errors could have populated
            //error details. We can ignore them, since we are creating a new scratch
            //file now.
            sortError_->initSortError(); //initialize, since WRITE_EOF or FILE_FULL errors 
            retval = CreateANewScrFileAndWrite(block, blockNum, blockLen, FALSE);
            if (retval != SCRATCH_SUCCESS) // if IO NOT COMPLETE or error
                return retval;
            break;
            
        case IO_NOT_COMPLETE:
            return retval;
            break;   
        default :
            // Error while writing to the scratch file either retry or abort
            //Note that error details is filled by scratchFile in 
            //sortError_. No need to generate a new message.
            return SCRATCH_FAILURE;
        } 
    }
    return SCRATCH_SUCCESS; 
}
 
//-----------------------------------------------------------------------
// Name         : checkIO
//
// Parameters   : ...
//                If checkAll is TRUE, then return IO_COMPLETE only if
//                all the IO has completed. If checkAll is FALSE, return
//                IO_COMPLETE if at least one internal IO has completed.
//
// Description  : This functions checks to see if any IO is pending and
//                returns the tag associated with the pending IO if any.
//
// Return Value :
//   IO_COMPLETE if the IO has completed.
//   IO_NOT_COMPLETE if IO not yet completed.
//-----------------------------------------------------------------------

RESULT ScratchSpace::checkIO(ScratchFile *sFile, NABoolean checkAll)
{
    Int64 iowaittime = 0;
    RESULT retval = IO_COMPLETE;
    ScratchFile * tempFile = (sFile != NULL)? sFile : currentIOScrFile_;

    if(!checkAll)
    {
      if (tempFile != NULL) 
      {
        //if there is even one free handle, return success.
        for(Int32 index = 0; index < scratchMaxOpens_; index++)
        {
          if(! tempFile->fileHandle_[index].IOPending)
          {
            //Before returning IO_COMPLETE, check to see if
            //if the IO on this file prior to its completion 
            //generated any errors. When IPC completes IO for us,
            //any errors is stored in previsous error field.
            retval = tempFile->getPreviousError(index);
            if((retval != SCRATCH_SUCCESS) && (retval != IO_NOT_COMPLETE))
              return retval;
           
            //For safety. We can remove once this assert over time. 
            ex_assert(retval != IO_NOT_COMPLETE,"ScratchSpace::checkIO previous error is IO_NOT_COMPLETE");   
            return IO_COMPLETE;
          }
        }

        //Since none of the handles are free, try checking for their 
        //IO completion.
        for(Int32 index = 0; index < scratchMaxOpens_; index++)
        {  
          RESULT retval = tempFile->checkScratchIO(index);
          // WRITE_EOF can only happen in the case of varying size blocks used by
          // ScratchSpace::WriteThru. This is only used by swapspace.cpp.
          if ((retval == WRITE_EOF) || (retval == FILE_FULL))
          {
             retval = CreateANewScrFileAndWrite(tempFile->fileHandle_[index].IOBuffer,
                                         tempFile->fileHandle_[index].blockNum,
                                         tempFile->fileHandle_[index].bufferLen);
               
             if((retval != SCRATCH_SUCCESS) && (retval != IO_NOT_COMPLETE))
               return retval;
             
             //Better to return IO_NOT_COMPLETE and reenter checkIO because 
             //the currentIOScrFile_ will be changed by now. 
             return IO_NOT_COMPLETE;
          }
          if(retval == IO_NOT_COMPLETE)
           continue;
          if((retval != IO_COMPLETE) && (retval != SCRATCH_SUCCESS))
           return retval;
         
          //Yes, one file handle is now free. 
          return IO_COMPLETE;
        }//for
        //we reach here means no free handle to perform IO.
        return IO_NOT_COMPLETE;
      }
      else
      {
        return IO_COMPLETE;
      }
    }
    else //checkAll is TRUE
    {
       for (Int32 i=0; i < scrFilesMap_->numScratchFiles_; i++)
       {
         tempFile = scrFilesMap_->fileMap_[i].scrFile_;


         for(Int32 index=0; index < scratchMaxOpens_; index++)
         {
          if(tempFile->isVectorPartiallyFilledAndPending(index))
          {
           if(tempFile->executeVectorIO() != SCRATCH_SUCCESS)
           {
              if(sortError_->getSysError() == ENOSPC)
              {
                //reset sortError_
                sortError_->initSortError();
                retval = CreateANewScrFileAndWrite(NULL, 0, 0);

                if((retval != SCRATCH_SUCCESS) &&
                    (retval != IO_NOT_COMPLETE) &&
                    (retval != IO_COMPLETE))
                   return retval;

                //Better to return IO_NOT_COMPLETE and reenter checkIO because 
                //the currentIOScrFile_ will be changed by now. Also other counts
                //such as scratch file map will have changed.
                return IO_NOT_COMPLETE;
              }
              else
                //error code and error details is populated inside
                //sortError_ automatically.
                return SCRATCH_FAILURE;
           }
              
          }
          if(tempFile->fileHandle_[index].IOPending)
           {
             RESULT retval = tempFile->checkScratchIO(index);
             // WRITE_EOF can only happen in the case of varying size blocks used by
             // ScratchSpace::WriteThru. This is only used by swapspace.cpp.
             if ((retval == WRITE_EOF) || (retval == FILE_FULL))
             {
               retval = CreateANewScrFileAndWrite(tempFile->fileHandle_[index].IOBuffer,
                                         tempFile->fileHandle_[index].blockNum,
                                         tempFile->fileHandle_[index].bufferLen);
               if((retval != SCRATCH_SUCCESS) &&
                 (retval != IO_NOT_COMPLETE) &&
                 (retval != IO_COMPLETE))
               return retval;
             
               //Better to return IO_NOT_COMPLETE and reenter checkIO because 
               //the currentIOScrFile_ will be changed by now. 
               return IO_NOT_COMPLETE;
             }
             if((retval != IO_COMPLETE) && (retval != SCRATCH_SUCCESS))
               return retval; //retval could be IO_NOT_COMPLETE, this is what we want.
           }//Io pending
           else
           {
             //check to see if the IO on this file prior to its completion 
             //generated any errors. When IPC completes IO for us,
             //any errors is stored in previsous error field.
             retval = tempFile->getPreviousError(index);
             if((retval != SCRATCH_SUCCESS) &&
                 (retval != IO_NOT_COMPLETE) &&
                 (retval != IO_COMPLETE))
             return retval;
           }
         }//for
       }//for
       //we reach here means all IO is complete on all scratch files
       return IO_COMPLETE;
    }
}

RESULT ScratchSpace::completeWriteIO(void)
{
  RESULT retval = SCRATCH_SUCCESS;

  if(! currentWriteScrFile_ )
    return SCRATCH_SUCCESS;

  Int32 index=0;
  do
  {
     if(currentWriteScrFile_->fileHandle_[index].IOPending)
     {
       RESULT retval = currentWriteScrFile_->checkScratchIO(index, INFINITE);
       // WRITE_EOF can only happen in the case of varying size blocks used by
       // ScratchSpace::WriteThru. This is only used by swapspace.cpp.
       if ((retval == WRITE_EOF) || (retval == FILE_FULL))
       {
         retval = CreateANewScrFileAndWrite(currentWriteScrFile_->fileHandle_[index].IOBuffer,
                                   currentWriteScrFile_->fileHandle_[index].blockNum,
                                   currentWriteScrFile_->fileHandle_[index].bufferLen);
         if((retval != SCRATCH_SUCCESS) &&
           (retval != IO_NOT_COMPLETE) &&
           (retval != IO_COMPLETE))
         return retval;
       
         //Better to restart because currentWriteScrFile_ will be changed by now. 
         index = 0;
         continue;
       }
       if((retval != IO_COMPLETE) && (retval != SCRATCH_SUCCESS))
         return retval; //retval not expected to be IO_NOT_COMPLETE.
     }//Io pending
     else
     {
       //check to see if the IO on this file prior to its completion 
       //generated any errors. When IPC completes IO for us,
       //any errors is stored in previsous error field.
       retval = currentWriteScrFile_->getPreviousError(index);
       if((retval != SCRATCH_SUCCESS) &&
           (retval != IO_NOT_COMPLETE) &&
           (retval != IO_COMPLETE))
       return retval;
     }
    index++;
  }while(index < currentWriteScrFile_->getNumOpens());
  return retval;
}

void ScratchSpace::close(void)
{
  scrFilesMap_->closeFiles();
}

//-----------------------------------------------------------------------
// Name         : truncate
//
// Parameters   : none
//
// Description  : Truncate current scratch file and cancel pending I/O.
//                Also close scratch files other than the current file.
//
// Return Value : none
//-----------------------------------------------------------------------

void ScratchSpace::truncate(void)
{
  if (currentIOScrFile_ != NULL) 
  {
    currentIOScrFile_->truncate();
  }
  scrFilesMap_->closeFiles(currentIOScrFile_);
}
//-----------------------------------------------------------------------
// Name         : getDiskPool
// 
// Parameters   : ...
//
// Description  : This function retrieves the pointer to the DiskPool
//   
// Return Value : Pointer to DiskPool
//
//-----------------------------------------------------------------------
DiskPool *ScratchSpace :: getDiskPool()
{ 
    return diskPool_;
}

//-----------------------------------------------------------------------
// Name         : generateDiskTable
// 
// Parameters   : ...
//
// Description  : This function generates the Disk Table
//   
// Return Value : Boolean
//
//-----------------------------------------------------------------------
NABoolean ScratchSpace:: generateDiskTable(SortError *sorterror)
{
    NABoolean retvalue =  SORT_SUCCESS;
    diskPool_ = NULL;
    
    diskPool_ = new (heap_) SQDisk(sorterror, heap_);
    if (diskPool_ == NULL)
    {
        sortError_->setErrorInfo( EScrNoMemory   //sort error
            ,NULL          //syserr: the actual FS error
            ,NULL          //syserrdetail
            ,"ScratchSpace::generateDiskTable"     //methodname
            );
        return SORT_FAILURE;
    }
    diskPool_->setScratchSpace(this);
    char diskPattern[8]; 
    strcpy(diskPattern, "*");
    retvalue = diskPool_->generateDiskTable(scratchDirListSpec_,
                                            numDirsSpec_,
        diskPattern) ;
    
    if (retvalue != SORT_SUCCESS)
    {
        
        sortError_->setErrorInfo( EScrNoDisks         //sort error
            ,NULL               //syserr: the actual FS error
            ,NULL               //syserrdetail
            ,"ScratchSpace::generateDiskTable"     //methodname
            );
        return SORT_FAILURE;
    }
    
#ifdef FORDEBUG  
    cout << " The following disk table was generated : " << endl << endl ;
    if ( diskPool_->printDiskTable() == SORT_FAILURE) { return SORT_FAILURE; } 
#endif
    
    
    return retvalue;
    
}

//-----------------------------------------------------------------------
// Name         : configure
// 
// Description  : Configure ScratchSpace attributes
//
//-----------------------------------------------------------------------
void ScratchSpace::configure(const ExExeStmtGlobals* stmtGlobals,
                             ExSubtask* ioEventHandler,
                             UInt16 scratchThresholdPct)
{
  if (stmtGlobals)
  {
    this->setIpcEnvironment(stmtGlobals->getIpcEnvironment());
    this->setEspInstance(stmtGlobals->getNumOfInstances());
    this->setNumEsps(stmtGlobals->getMyInstanceNumber());
    const ExScratchFileOptions* sfo = stmtGlobals->getScratchFileOptions();
    if (sfo)
    {
      this->setScratchDirListSpec(sfo->getSpecifiedScratchDirs());
      this->setNumDirsSpec(sfo->getNumSpecifiedDirs());
     
    }
  }

  if (ioEventHandler)
  {
    this->setIoEventHandler(ioEventHandler);  
  }

  this->setScratchThreshold(scratchThresholdPct);

  // Set calling TCB to NULL, since this information isn't used.
  // (ScratchFileConnection::callingTcb_ is unused)
  this->setCallingTcb(NULL);
}

//-----------------------------------------------------------------------
// Name         : getTotalNumOfScrBlocks
// 
// Parameters   : ...
//
// Description  : This function retrieves total number of scratch blocks
//   
// Return Value : long totalNumOfScrBlocks_
//
//-----------------------------------------------------------------------

Lng32 ScratchSpace::getTotalNumOfScrBlocks() const
{
    return totalNumOfScrBlocks_;
    
}
//-----------------------------------------------------------------------
// Name         : getTotalIoWaitTime
// 
// Parameters   : ...
//
// Description  : This function retrieves totalo wait time 
//   
// Return Value : Int64 totalIoWaitTime_
//
//-----------------------------------------------------------------------

void ScratchSpace::getTotalIoWaitTime(Int64 & iowaitTime) const
{
    iowaitTime = totalIoWaitTime_ ; 
}

//-----------------------------------------------------------------------
// Name         : getScrFilesMap
// 
// Parameters   : ...
//
// Description  : This function retrieves the scrFilesMap_ pointer
//   
// Return Value : ScratchFileMap
//
//-----------------------------------------------------------------------

ScratchFileMap* ScratchSpace::getScrFilesMap() const
{
    return scrFilesMap_;
}

//-----------------------------------------------------------------------
// Name         : getLastSqlCode
// 
// Parameters   : ...
//
// Description  : Return the last sort error as a SQLCODE value in the
//                Executor range.  ScratchSpace I/O errors are mapped to
//                the Executor's scratch I/O range (10100 .. 10199).
//                SQLCODE values that are not specific to the sort
//                operator facilitates ScratchSpace code reuse.
//  
//-----------------------------------------------------------------------

// TODO: ScratchSpace and ScratchFile classes should be rewritten
// TODO: to make them more easily reused.  Mapping from a sort error
// TODO: to an executor error would be unnecessary if the scratch
// TODO: code was independent of sort externals.

Int16
ScratchSpace::getLastSqlCode(void)
{
  Int16 exeError = 0;

  if (sortError_ && (sortError_->getSortError()))
  {
    Int16 error = -(sortError_->getSortError());
    switch (error)
    {
      case 0:
        // do nothing
        break;

      case EScrWrite:
        exeError = EXE_SCR_IO_WRITE;
        break;

      case EScrRead:
        exeError = EXE_SCR_IO_READ;
        break;

      case EInvScrBlockNum:
        exeError = EXE_SCR_IO_INVALID_BLOCKNUM;
        break;

      case EScrNoDisks:
        exeError = EXE_SCR_IO_NO_DISKS;
        break;

      case EScrNoMemory:
        exeError = EXE_NO_MEM_TO_EXEC;
        break;


      case ECreateDir:
        exeError = EXE_SCR_IO_CREATEDIR;
        break;

      case ECreateFile:
        exeError = EXE_SCR_IO_CREATEFILE;
        break;

      case EGetTmpFName:
        exeError = EXE_SCR_IO_GETTMPFNAME;
        break;

      case ECloseHandle:
        exeError = EXE_SCR_IO_CLOSEHANDLE;
        break;

      case EWriteFail:
        exeError = EXE_SCR_IO_WRITEFILE;
        break;

      case ESetFilePtr:
        exeError = EXE_SCR_IO_SETFILEPOINTER;
        break;

      case ECreateEvent:
        exeError = EXE_SCR_IO_CREATEEVENT;
        break;

      case EWaitMultObj:
        exeError = EXE_SCR_IO_WAITMULTOBJ;
        break;

      case EWaitSingleObj:
        exeError = EXE_SCR_IO_WAITSINGLEOBJ;
        break;

      case EGetOverlappedResult:
        exeError = EXE_SCR_IO_GETOVERLAPPEDRESULT;
        break;

      case EResetEvent:
        exeError = EXE_SCR_IO_RESETEVENT;
        break;

      case EDskFreeSpace:
        exeError = EXE_SCR_IO_GETDISKFREESPACE;
        break;

      case EThresholdReached:
        exeError = EXE_SCR_IO_THRESHOLD;
        break;

      case EScrFileNotFound:
        exeError = EXE_SCR_IO_UNMAPPED_BLOCKNUM;
        break;

      case EUnexpectErr:
      default:
        exeError = EXE_INTERNAL_ERROR;
        break;
    }
  }

  return exeError;
}


SortScratchSpace::SortScratchSpace(CollHeap* heap,
                         SortError* error,
                         Int32 explainNodeId,
                         Int32 scratchIOBlockSize,
                         Int32 scratchIOVectorSize,
                         NABoolean logInfoEvent,
                         Int32 scratchMgmtOption)
                         :ScratchSpace(heap,
                                   error,
                                   scratchIOBlockSize,
                                   scratchIOVectorSize,
                                   explainNodeId,
                                   logInfoEvent,
                                   scratchMgmtOption)
{
  blockHead_.thisBlockNum_ = 0L;
  blockHead_.nextBlockNum_ = NULL;
  blockHead_.runNum_       = 0;
  blockHead_.numRecs_      = 0;
  blockHead_.bytesUsed_    = OVERHEAD;
  
  scrBlock1_ = (char *)((NAHeap *)heap_)->allocateAlignedHeapMemory(blockSize_, 512, FALSE);
  scrBlock2_ = (char *)((NAHeap *)heap_)->allocateAlignedHeapMemory(blockSize_, 512, FALSE);

  ex_assert(scrBlock1_ != NULL, "SortScratchSpace::SortScratchSpace, scrBlock1_ is NULL");   
  ex_assert(scrBlock2_ != NULL, "SortScratchSpace::SortScratchSpace, scrBlock2_ is NULL");   
  
  currentBlock_ = scrBlock1_;
  nextWritePosition_   = currentBlock_ + blockHead_.bytesUsed_;
  currentRun_          = -99;
  freeSortMergeBufferPool_ = NULL;
  sortMergeBlocksPerBuffer_ = 1;

  runDirectory_  = new (heap_) RunDirectory(MAXRUNS, heap_, error);
  if (runDirectory_ == NULL)
  {
      sortError_->setErrorInfo( EScrNoMemory   //sort error
          ,NULL          //syserr: the actual FS error
          ,NULL          //syserrdetail
          ,"SortScratchSpace::SortScratchSpace"     //methodname
          );
      
  }
  ex_assert(runDirectory_ != NULL, "SortScratchSpace::SortScratchSpace, runDirectory_ is NULL");

}

SortScratchSpace::~SortScratchSpace()
{
  if (scrBlock1_ != NULL) {
        ((NAHeap *)heap_)->deallocateHeapMemory(scrBlock1_);
        scrBlock1_ = NULL;
    }
    if (scrBlock2_ != NULL) {
        ((NAHeap *)heap_)->deallocateHeapMemory(scrBlock2_);
        scrBlock2_ = NULL;
    }
    if (runDirectory_ != NULL) {
        delete runDirectory_;
        runDirectory_ = NULL;
    }

    //It is very important to cancel any pending IO before
    //cleaning up the buffer pool. This is because,a buffer
    //could be locked when IO is pending, cleanup will cause
    //memory deallocation failures.
    scrFilesMap_->closeFiles();

    cleanupSortMergeBufferPool();

}

RESULT SortScratchSpace::setupSortMergeBufferPool(Int32 numBuffers)
{
  ex_assert(freeSortMergeBufferPool_ == NULL,
    "SortScratchSpace::setupSortMergeBufferPool, freeSortMergeBufferPool_ is not NULL");

  Int32 count = numBuffers;
  while(count > 0 )
  {
    //FailureIsFatal flag set to FALSE so that we handle allocation
    //failure.
    SortMergeBuffer *mb = new (heap_, FALSE) SortMergeBuffer();
    if(mb == NULL)
    {
      //No need to populate sortError here. The caller will
      //make sure to retry or return error from his space.
      //Just cleanup what ever has been allocated upto this point
      //and return.
      cleanupSortMergeBufferPool();
      return SCRATCH_FAILURE;
    }
    mb->scrBlock_ = (char *)((NAHeap *)heap_)->allocateAlignedHeapMemory(blockSize_, 512, FALSE);
    if (mb->scrBlock_ == 0)
    {
      sortError_->setErrorInfo( EScrNoMemory   //sort error
          ,NULL          //syserr: the actual FS error
          ,NULL          //syserrdetail
          ,"SortScratchSpace::setupSortMergeBufferPool2"     //methodname
          );
      cleanupSortMergeBufferPool();
      return SCRATCH_FAILURE;
    }

    mb->mnext_ = freeSortMergeBufferPool_;
    freeSortMergeBufferPool_ = mb;
    count--;
  }
  return SCRATCH_SUCCESS;
}

void SortScratchSpace::cleanupSortMergeBufferPool(void)
{
  SortMergeBuffer *mb;
  while(freeSortMergeBufferPool_)
  {
    mb = freeSortMergeBufferPool_;
    freeSortMergeBufferPool_ = freeSortMergeBufferPool_->mnext_;
    ((NAHeap *)heap_)->deallocateHeapMemory(mb->scrBlock_);
    delete(mb);
  }
}


RESULT SortScratchSpace::writeRunData(char* data, ULng32 reclen, 
                               ULng32 run,NABoolean waited)
{  
    RESULT retcode = SCRATCH_SUCCESS;
    if (run != currentRun_) 
    {
        // Start a New Run
        currentRun_ =  runDirectory_->startNewRun(totalNumOfScrBlocks_);     
        if ( sortError_->getSortError() ) { return SCRATCH_FAILURE; }
    }
    
    // DP2 requires the last 8 bytes of 56kb block be kept free for 
    // Dp2 to use it for checksum, especially when using setmode(141,11)(see scratch file)
    // We by default leave this 8 byte free for any setmode(141,*).  This is because
    // there may be cases where setmode(141,*) switches to different value if mirror is
    // down.
    // We could do some cleanup to not perform subtraction for every row.
    if ((blockHead_.bytesUsed_ + (Lng32) reclen) <= (blockSize_ - DP2_CHECKSUM_BYTES))
    {
        memcpy(nextWritePosition_, data, (Int32)reclen);
        blockHead_.bytesUsed_ += reclen;
        blockHead_.numRecs_ += 1;
        nextWritePosition_    += reclen;
    }
    else
    {
        //---------------------------------------------------------------
        // Time to write the scratch block to disk. We just call flushdata 
        // for this purpose.  
        //---------------------------------------------------------------
        
        retcode = SortScratchSpace::flushRun(FALSE_L,waited) ;
        if (retcode != SCRATCH_SUCCESS)
            return retcode;
        
        memcpy(nextWritePosition_, data, (Int32)reclen);
        blockHead_.bytesUsed_ += reclen; 
        blockHead_.numRecs_ += 1;
        nextWritePosition_ += reclen; 
    }
    return retcode;
}


//-----------------------------------------------------------------------
// Name         : flushRun
// 
// Parameters   : ...
//
// Description  : 
//  The buffers used for accumulating records get written to the scratch 
//  file automatically when they are full. However there are times when
//  these buffers need to be written to the scratch file even when they
//  are not full. This function is useful in such situations. 
//   
// Return Value :
//   SORT_SUCCESS if everything goes on well.
//   SORT_FAILURE if any error encounterd. 
//
//-----------------------------------------------------------------------
RESULT SortScratchSpace::flushRun(NABoolean endrun,NABoolean waited)
{
    RESULT retval = SCRATCH_FAILURE;

    //Before flushing the buffer and switching buffers,
    //make sure the previous buffer write is complete.
    //This is a temporary fix until we make sort writes 
    //completely no waited, such that we can give back 
    //control to the scheduler and re-enter.
    retval = ScratchSpace::completeWriteIO();
    if((retval != SCRATCH_SUCCESS) && (retval != IO_COMPLETE))
      return retval;

    //---------------------------------------------------------------
    // Time to write the scratch block to disk. We first upadate the
    // header and then initiate the write.  
    //---------------------------------------------------------------
    blockHead_.thisBlockNum_ = totalNumOfScrBlocks_;
    if (endrun == TRUE_L) {
        blockHead_.nextBlockNum_ = -1;
    }
    else {
        blockHead_.nextBlockNum_ = totalNumOfScrBlocks_ + 1;      
    }
#pragma nowarn(1506)   // warning elimination 
    blockHead_.runNum_       = currentRun_;
#pragma warn(1506)  // warning elimination 
    memcpy(currentBlock_, &blockHead_, sizeof(ScrBlockHeader));

    retval = ScratchSpace::writeFile(currentBlock_,
                          blockHead_.thisBlockNum_,
                          blockSize_ );
   
    if(retval != SCRATCH_SUCCESS) return retval;
   
    if(waited)
    {
      retval = ScratchSpace::completeWriteIO();
      if((retval != SCRATCH_SUCCESS) && (retval != IO_COMPLETE))
        return retval;
    }
    //--------------------------------------------------------------   
    // At this point we have successfully initiated the asynchronous 
    // write of the scratchBlock<x>_ pointed to by currentBlock_ and
    // hence it is time to switch the currentBlock_ to point to the
    // other scratchBlock<x>_ and buffer the record which could not 
    // fit in the last buffer to the new scratch buffer.
    //--------------------------------------------------------------    
    
    switchScratchBuffers();
    
    
    return SCRATCH_SUCCESS;  
}  
//-----------------------------------------------------------------------
// Name         : initiateSortMergeNodeRead
// 
// Parameters   : ...
//
// Description  : Starts asynchronous read against scratch file. Used
//                during merge phase, to fire off read I/O before the
//                data is needed.
//  
// Return Value :
//   SORT_SUCCESS if everything goes on well.
//   SORT_FAILURE if any error encounterd. 
//
//-----------------------------------------------------------------------

RESULT SortScratchSpace::initiateSortMergeNodeRead(SortMergeNode *sortMergeNode, NABoolean waited)
{

  Lng32 blockOffset;
  Int64 iowaittime = 0;
  Int32 numOutStandingIO = 0;
  SortMergeBuffer *mbTemp;
  RESULT retval = SCRATCH_SUCCESS;

  // IO on the endBlockNum has been initiated already. This is the end of Run.
  // Usually we should reach here only after the lastIOBlockNum_ IO has completed.
  if(sortMergeNode->nextIOBlockNum_ > sortMergeNode->endBlockNum_)
    return SCRATCH_SUCCESS;
  
  mbTemp = sortMergeNode->readQHead_;
  while(mbTemp != NULL)
  {
    if((mbTemp->state_ == QUEUED) || (mbTemp->state_ == READING))
     numOutStandingIO++; 
    mbTemp = mbTemp->mnext_;
  }

  //We want to initiate IOs in batches. Hence wait for all outstanding IOs to complete.
  //Initiating IOs in batches will queue the requests in a series. We have observed that
  //reading buffers in a series reduces the disk seek time. Disk seek time has contributed
  //to major IO bottle necks.
  if(sortMergeNode->numOutStandingIO_ > 0)
  {
    return SCRATCH_SUCCESS;
  }
  //Note that we are maintaining double buffer concept. In order to initiate Io
  //on second buffer, we need to have enough blocks that can comprise a buffer.
  //numReadQBlocks_ can maximum be 2 * sortmergeBlocksPerBuffer because of double
  //buffering. We will only initiate next set of IOs only if there are atleast
  //that number of free blocks that comprise second buffer.
  if(sortMergeNode->numReadQBlocks_ > sortMergeBlocksPerBuffer_)
  {
    return SCRATCH_SUCCESS;
  }
  for(Int32 count=0; (count < sortMergeBlocksPerBuffer_) && (sortMergeNode->nextIOBlockNum_ <= sortMergeNode->endBlockNum_); count++)
  {
    //get a io buffer from pool.
    SortMergeBuffer *mb = this->getFreeSortMergeBuffer();
    if(mb == NULL)
    {
        sortError_->setErrorInfo(EUnexpectErr    //sort error
            ,NULL                     //syserr: the actual FS err
            ,NULL                     //syserrdetail
            ,"ScratchSpace::initiateSortMergeNodeRead" //methodname
            );
        return SCRATCH_FAILURE;
    } 
    
    mb->currentScrFile_ = scrFilesMap_->mapBlockNumToScrFile(sortMergeNode->nextIOBlockNum_,
                                              blockOffset);
    
    if (mb->currentScrFile_ == NULL) 
    {
      sortError_->setErrorInfo(EScrFileNotFound    //sort error
                              ,NULL                     //syserr: the actual FS err
                              ,NULL                     //syserrdetail
                              ,"ScratchSpace::initiateSortMergeBufferRead" //methodname
                              );
      return SCRATCH_FAILURE;
    }
    
    mb->currentIOByteOffset_ = blockOffset * blockSize_;  
    mb->currentIOByteLength_ = blockSize_;
  
    // Link this buffer to end of sortMergeNode read queue.
    sortMergeNode->linkToReadQ(mb);

    retval = mb->currentScrFile_->queueReadRequestAndServe(mb,iowaittime); 
    totalIoWaitTime_ += iowaittime;
    
    sortMergeNode->numOutStandingIO_++;
    sortMergeNode->nextIOBlockNum_++;

    if ((retval != SCRATCH_SUCCESS) && (retval != IO_COMPLETE) &&
           (retval != IO_NOT_COMPLETE))
    {
      return retval;
    }    
  }//for loop
  
  return SCRATCH_SUCCESS;
}

//-----------------------------------------------------------------------
// Name         : readSortMergeNode
// 
// Parameters   : ...
//
// Description  : Reads a record from the scratch file. Used during 
//                merge phase. 
//  
// Return Value :
//   SORT_SUCCESS if everything goes on well.
//   SORT_FAILURE if any error encounterd. 
//
//-----------------------------------------------------------------------

RESULT SortScratchSpace::readSortMergeNode(SortMergeNode *sortMergeNode, 
    char*& rec, 
    ULng32 reclen, 
    ULng32 &actRecLen,
    NABoolean waited,
    Int16 numberOfBytesForRecordSize)
{
    Int64 ioWaitTime = 0;
    RESULT retval = SCRATCH_SUCCESS;
    actRecLen = reclen;
    
    
    if (sortMergeNode->numRecsRead_ == (sortMergeNode->blockHead_).numRecs_)
    { 
        // The current buffer has been exhausted -- look for more data
        
        
        if (sortMergeNode->blockHead_.nextBlockNum_ == -1)
            return END_OF_RUN; //not an error, return EOF for end of run 
        else 
        {
          //release the current IO block if it is consumed. We may reach here 
          //if it is the first block of the run pending IO.
          if(sortMergeNode->readQHead_ && sortMergeNode->readQHead_->state() == BEING_CONSUMED)
          {  //State begins with Queued, reading, readcomplete,being_consumed, idle
             //if we reach here and the state is being_consumed,then block has been consumed.
             //we can release it.
             this->returnFreeSortMergeBuffer(sortMergeNode->delinkReadQ());
          }

          if (sortMergeNode->readQHead_ == NULL)
          {
            //We should not reach here at all. (blockHead_.nextBlockNum_ == -1) check
            //being done above should take of this.
            sortError_->setErrorInfo(EUnexpectErr    //sort error
                                      ,NULL                     //syserr: the actual FS err
                                      ,NULL                     //syserrdetail
                                      ,"SortScratchSpace::readSortMergeNode" //methodname
                                      );
             return SCRATCH_FAILURE;
          }
           
          //there is an I/O outstanding -- complete it
          //this call is a waited call. Making it nowait will
          //require the caller to handle nowaited response.
          retval = sortMergeNode->checkIO(ioWaitTime, waited);
              
          if (retval == READ_EOF) 
          {
              sortError_->setErrorInfo( EScrEOF      //sort error
                  ,NULL         //syserr: the actual FS err
                  ,NULL         //syserrdetail
                  ,"SortScratchSpace:readSortMergeBuffer" //methodname
                  );
          }
                
          if((retval != IO_COMPLETE) && (retval != SCRATCH_SUCCESS))
           return retval;  //Possible to return IO_NOT_COMPLETE.

          totalIoWaitTime_ += ioWaitTime;

          // set up data structures for returning rows; return first row

          memcpy( &sortMergeNode->blockHead_,
              sortMergeNode->readQHead_->scrBlock_,
              sizeof(ScrBlockHeader));

          sortMergeNode->nextReadPosition_ =  
              sortMergeNode->readQHead_->scrBlock_ + OVERHEAD;
          rec = sortMergeNode->nextReadPosition_;

         if (numberOfBytesForRecordSize > 0)
         {
           actRecLen = *((UInt32 *)rec);
         }

          sortMergeNode->nextReadPosition_ += actRecLen;
          sortMergeNode->numRecsRead_ = 1;           

          //Check if there is oppurtunity to initiate next back of IO 
          //corresponding to this run. 
          retval = initiateSortMergeNodeRead(sortMergeNode,waited);
          //IO_NOT_COMPLETE and IO_COMPLETE are taken care by initiateSortMergeNodeRead. 
          if(retval != SCRATCH_SUCCESS) 
            return retval;
        }   
    }
    else 
    { 
        rec = sortMergeNode->nextReadPosition_;
        if (numberOfBytesForRecordSize > 0)
        {
          actRecLen = *((UInt32 *)rec);
        }
        sortMergeNode->nextReadPosition_ += actRecLen;
        sortMergeNode->numRecsRead_ += 1;  
    }
    
    return SCRATCH_SUCCESS;
}

SortMergeBuffer *SortScratchSpace::getFreeSortMergeBuffer(void)
{
   SortMergeBuffer *mb = freeSortMergeBufferPool_;
   if(freeSortMergeBufferPool_)
    freeSortMergeBufferPool_ = freeSortMergeBufferPool_->mnext_;
   
   if(mb) mb->reset();
   return mb;
}

void SortScratchSpace::returnFreeSortMergeBuffer(SortMergeBuffer *mb)
{
  mb->mnext_ = freeSortMergeBufferPool_;
  freeSortMergeBufferPool_ = mb;
}


//-----------------------------------------------------------------------
// Name         : switchScratchBuffers
//
// Parameters   : ...
//
// Description  : 
//  ScratchSpace uses double buffering to buffer the records so that
//  buffering can continue while asynchronous writes are in progress
//  to the scratch file. As the name indicates the purpose of this 
//  function is to switch the current buffer used for accumulating the
//  records.
//   
// Return Value :
//   SORT_SUCCESS if everything goes on well.
//   SORT_FAILURE if any error encounterd. 
                                  //
//-----------------------------------------------------------------------
NABoolean SortScratchSpace::switchScratchBuffers(void)
{
    
    if (currentBlock_ == scrBlock1_)
        currentBlock_ = scrBlock2_;
    else 
        currentBlock_ = scrBlock1_;
    
    blockHead_.bytesUsed_ = OVERHEAD;  
    blockHead_.numRecs_   = 0;
    nextWritePosition_ = currentBlock_ + OVERHEAD;
    totalNumOfScrBlocks_++;
    return TRUE; 
}  

RESULT SortScratchSpace::serveAnyFreeSortMergeBufferRead(void)
{
  RESULT retval;
  Int64 ioWaitTime = 0;
  ScratchFile *tempFile;
  for (Int32 i=0; i < scrFilesMap_->numScratchFiles_; i++)
  {
     tempFile = scrFilesMap_->fileMap_[i].scrFile_;
     retval = tempFile->serveAsynchronousReadQueue(ioWaitTime, TRUE);
     if(retval != SCRATCH_SUCCESS)
      return retval;
  }
  return SCRATCH_SUCCESS;
}

//Cleanup scratch files in between intermediate merges.
//This call is not to be called for general cleanup of
//scratch files. inRun is the greatest run number that
//needs to be cleanedup from the beginning.
RESULT SortScratchSpace::cleanupScratchFiles(Lng32 inRun) 
{
  //calculate the beginning block number of the (inRun + 1).
  //Basic idea is to retain scratch file that hosts this blocknum
  //and cleanup all scratch files before this scratch file.
  //Below method gives the beginning blocknum of a run.
  SBN beginBlockNum = runDirectory_->mapRunNumberToFirstSBN(inRun + 1);
  
  //we should not reach here if cleanScratchFiles is invoked at the
  //right places. Note that cleanupScratchFiles is invoked inbetween
  //intermediate merges. So there is always valid blockNums following this one.
  if(beginBlockNum == EOF)
  {
    sortError_->setErrorInfo(EInvRunNumber   //sort error
                     ,NULL          //syserr: the actual FS error
                     ,NULL          //syserrdetail
                     ,"SortScratchSpace::cleanupScratchFiles"
                     );
    return SCRATCH_FAILURE;
  }
  
  //all scratch files that contain blocknum < beginBlockNum
  //are potential candidates for closure.
  scrFilesMap_->closeScrFilesUpto(beginBlockNum);
  return SCRATCH_SUCCESS;

}

//-----------------------------------------------------------------------
// Name         : getTotalNumOfRuns
// 
// Parameters   : ...
//
// Description  : This function returns the number of scratch runs 
//                generated. This can be used to determine the tournament
//                merge order.
//   
// Return Value :
//   SORT_SUCCESS if everything goes on well.
//   SORT_FAILURE if any error encounterd. 
//
//-----------------------------------------------------------------------

Lng32 SortScratchSpace::getTotalNumOfRuns(void)
{
    return runDirectory_->getTotalNumOfRuns();
}

//-------------------------------------------------------------
// HashScratchSpace is a specialization over ScratchSpace providing 
// Hash operator specific interface. Book keeping of cluster Ids and 
// corresponding blocks are maintained by this class. 
//-------------------------------------------------------------
HashScratchSpace::HashScratchSpace(CollHeap* heap,
                          SortError* error,
                          Int32 explainNodeId,
                          Int32 blockSize,
                          Int32 scratchIOVectorSize,
                          NABoolean logInfoEvent,
                          Int32 scratchMgmtOption)
                          :ScratchSpace(heap,
                                    error,
                                    blockSize,
                                    scratchIOVectorSize,
                                    explainNodeId,
                                    logInfoEvent,
	                            scratchMgmtOption)
{
  numClusters_ = INITIAL_MAX_CLUSTERS;
  clusterDList_ = (ClusterDirectory*) new(heap_)ClusterDirectory[numClusters_];
  memset((void*)clusterDList_, '\0', numClusters_ * sizeof(ClusterDirectory));
  currentCDir_ = NULL;
}

HashScratchSpace::~HashScratchSpace()
{
  if(clusterDList_)
  {
    //traverse thru each directory and deallocate cluster blocks
    //if any.
    for(UInt32 index = 0; index < numClusters_; index++)
    {
        while(clusterDList_[index].first != NULL)
        {
          CBlock *temp = clusterDList_[index].first;
          clusterDList_[index].first = temp->next;
          heap_->deallocateMemory(temp); 
        }
        clusterDList_[index].first = NULL;
        clusterDList_[index].last = NULL;
    }
    heap_->deallocateMemory(clusterDList_);
  } 
}

RESULT HashScratchSpace::writeThru( char* buf,
                    UInt32 clusterID)
{
  RESULT  retval = WRITE_EOF;
  retval = this->registerClusterBlock(clusterID, totalNumOfScrBlocks_);
  if(retval != SCRATCH_SUCCESS) return retval; //error populated inside. 
  
  retval = ScratchSpace::writeFile( buf,
                         totalNumOfScrBlocks_,
                         blockSize_);
  
  if(retval != SCRATCH_SUCCESS) return retval;
 
  // Note that we increment totalNumOfScrBlocks_ after a
  // successful write initiation. ScratchSpace::WriteFile will ensure
  // the block number is registered against the right scratch file
  // whether a new or old scratch file is used. 
  totalNumOfScrBlocks_++;
  currentWriteScrFile_->bytesWritten() += blockSize_;
  return SCRATCH_SUCCESS;  
}

RESULT HashScratchSpace::readThru(char *buf, UInt32 clusterID, ClusterPassBack *cPassBack)
{
  RESULT  retval = SCRATCH_SUCCESS;
  ScratchFile *readScratchFile = cPassBack->scratchFile_;
  Int32 readBlockOffset = cPassBack->blockOffset_;

  DWORD blockNumToRead = getClusterBlockNum(clusterID, cPassBack);
  if((Int32)blockNumToRead <= 0)
  {
    sortError_->setErrorInfo( EInvScrBlockNum      //sort error
                     ,NULL         //syserr: the actual FS err
                     ,NULL         //syserrdetail
                     ,"HashScratchSpace::readThru" //methodname
                    );
    
    return SCRATCH_FAILURE;
  }
  retval = ScratchSpace::readThru(buf,
                         blockNumToRead,
                         blockSize_,
                         readScratchFile,
                         readBlockOffset);
  
  
  return retval;
  
}

//Check the header file for usage protocol. 
//Writing to scratch file is always written in a serial
//fashion, meaning that once one scratch file is full, then
//next scratch file is created and continue to write.
//currentWriteScrFile_always points to the most current
//scratch file that is open for writing.
//CheckIoWrite() returns IO_COMPLETE if there is atleast
//one free handle of the scratch file is available to 
//perform a write operation.
RESULT HashScratchSpace::checkIOWrite(void)
{
  //Note that if there is a partially filled vector scheduled
  //for read, then we are not flushing the vector because of
  //checkIoWrite(). It is expected that hash operators call 
  //checkIoAll() to flush any partially filled vector before
  //switching between reads and writes.
  if(currentWriteScrFile_ != NULL)
    return  ScratchSpace::checkIO(currentWriteScrFile_);
  else
    return IO_COMPLETE;
}

//Check the header file for usage protocol.
//Returns IO_COMPLETE if atleast one free handle is available
//for initiating an IO. In order to lookup a specific scratch file,
//the caller provides clusterID indicating his intentions to initiate
//IO against. ScratchSpace maps a directory of clusterIds and corresponding
//set of blocks to each clusterID when writes are performed. When reads
//are performed against a clusterID, all the blocks that belog to the
//clusterID is read. 
//cPassBack is a cookie that gets set by scratchSpace and given back to
//the caller. The caller treats this cookie as a blackbox and provides
//the same cookie back when performing readThru() or checkIORead() call.
//This cookie facilitates efficient lookup of cluster blocks that belong
//to the same cluster. 
//1. Note that hash operators overflow a cluster that contains a series of
//   blocks and read the same set of blocks in a sequence.
//2. When hash operator intends to read a new cluster, then cPassBack is
//   initialized(to zeros) and only clusterID is valid.
//3. Once cPassBack is initialized with a specific block and corresponding 
//   scratch file, this information is used for checking pending IO on that
//   scratch file by checkIORead() and also used by readThru to avoid 
//   lookup of cluster directory.
//4. getClusterBlockNum() is a distructive call, in the sense that the 
//   contents of cPassBack that is passed as an argument keeps changing
//   to subsequenet blocks every time getClusterBlockNum() is invoked.
//   Hence this call should only be invoked by readThru. 
//   CheckIoRead() also invokes getClusterBlockNum() only for the scenario
//   where clusterID is valid and cPassBack is empty. However this scenario is
//   to just find the scratch file associated with the first block of clusterID
//   and not to reuse the cookie in subsequenet calls.
RESULT HashScratchSpace::checkIORead(ClusterPassBack *cPassBack, UInt32 clusterID)
{
  RESULT retval;
  ScratchFile *sFile = NULL;
  Int32 blockOffset = -1;

  if(cPassBack != NULL && (cPassBack->scratchFile_ != NULL))
  {
    retval = ScratchSpace::checkIO(cPassBack->scratchFile_);
    sFile = cPassBack->scratchFile_;
    blockOffset = cPassBack->blockOffset_;
  }
  else
  {
    //Since we do not know the scratch file to call checkIO against,
    //lets use the clusterID to search for the scratch file. Note that 
    //we will make a new instance of ClusterPassBack called tempPassBack 
    //here and not use the cPassBack that is passed in. This is to make
    //sure that getClusterBlockNum call does not store anything inside
    //cPassBack. getClusterBlockNum() should only be called by readThru().
    ClusterPassBack tempPassBack;
    getClusterBlockNum(clusterID, &tempPassBack, TRUE);

    if(tempPassBack.scratchFile_ != NULL)
    {
      retval = ScratchSpace::checkIO(tempPassBack.scratchFile_);
      sFile = tempPassBack.scratchFile_;
      blockOffset = tempPassBack.blockOffset_;
    }
    else
      retval = OTHER_ERROR;
  }

  //If vector IO is involved, then we need to make sure all the elements
  //inside the vector are reading contigous memory locations residing on
  //scratch file. 
  if(retval == IO_COMPLETE)
  {
    if(sFile->isNewVecElemPossible(blockOffset * blockSize_,
                                   blockSize_))
    {
      return IO_COMPLETE;
    }
    else
    {
      //Execute the vector IO. Execute vector will initiate IO and possibly
      //complete Io as well. If IO is not complete, IOPending flag is
      //set which inturn redrives the Io when checkio() is called.
      //To check if IOPending flag is set, call ScratchSpace::checkIO()
      //again to check if new Io is possible or not.
      //If we reach here means that executeVectorIO is intended to
      //perform a read operation. So there is no need to check for ENOSPC
      //error in this scenario.

      retval = sFile->executeVectorIO();

      if(retval != SCRATCH_SUCCESS) return retval;

      //now return the IO status of the previous flush.
      retval = ScratchSpace::checkIO(sFile);
    }
  }
  return retval;
}

//Check the header file for usage protocol.
//Returns IO_COMPLETE only when all IO to all scratch files
//have completed. This is especially useful while reading
//to confirm that buffers that are allocated and used for IO 
//have really got populated with the desired data. Without 
//calling this method, there is no gurantee that buffers are 
//filled with data.
//CheckIoAll() is called under the following conditions:
//1. When ever hash operator shifts from writing to reading or
//   from reading to writing.
//2. There is no gurantee that checkIoAll is called when 
//   shifting from writing to reading.
RESULT HashScratchSpace::checkIOAll(void)
{
  return ScratchSpace::checkIO(NULL, TRUE);
}

//getClusterBlockNum() is a distructive call, in the sense that the 
//contents of cPassBack that is passed as an argument keeps changing
//to subsequenet blocks every time getClusterBlockNum() is invoked.
//Hence this call should only be invoked by readThru. 
//CheckIoRead() also invokes getClusterBlockNum() only for the scenario
//where clusterID is valid and cPassBack is empty. However this scenario is
//to just find the scratch file associated with the first block of clusterID
//and not to reuse the cookie in subsequenet calls.
//see more comments for checkIORead() call.
//This method returns the following:
//1. BlockNumber -given the cluster id or cPassBack, returns the unique block
//                number that belongs to clusterID.
//2. cPassBack->endOfClusterBatch - indicates if the BlockNumber is the last
//                member of sequential batch of writes.
//3. cPassBack->cBlock_ - pointer to the next cBlock entry following the
//                current BlockNumber.
//4. cPassBack->scratchFile_ - scratch file that cPassBack->cBlock_ belongs to.
//                Note that this scratch file corresponds to cPassBack->cBlock_
//                which is the next block of BlockNumber.
//5. cPassBack->blockOffset_ - This is the block offset into the scratch file 
//                specified in cPassBack->scratchFile_ where cPassBack->cBlock_
//                (or the nexxt block of BlockNumber) is situated.
//Note that items 3,4 and 5 all represent lookup of details that belong to 
//blocknumber following item 1. If getCurrentBlock argument is true, then 
//items 4 and 5 ( not including item 3) represent lookup details of item 1.
DWORD HashScratchSpace::getClusterBlockNum(UInt32 clusterID, ClusterPassBack *cPassBack, NABoolean getCurrentBlock)
{
  //If the caller gives next block to read, just read it directly without trying
  //to map it. Note that we do not try to verify clusterID with the corresponding
  //block.
  DWORD blockNumToRead = -1;
  if(cPassBack->cBlock_ != NULL)
  {
    blockNumToRead = cPassBack->cBlock_->blockNum;
    
    if(cPassBack->cBlock_->endBatch)
    {
      cPassBack->endOfClusterBatch_ = TRUE;
    }
    else
    {
      cPassBack->endOfClusterBatch_ = FALSE;
    }
    //Update book keeping. Note that next could be NULL.
    //If NULL, it indicates end of cblock sequence.
    cPassBack->cBlock_ = cPassBack->cBlock_->next; 
  }
  else
  { //cpassback->cblock is NULL means, we need to search from the
    //beginning for a given clusterID.
    ClusterDirectory *cdtemp = clusterDList_;
    UInt32 index;
    for(index = 0; index < numClusters_; index++)
    {
      if(cdtemp[index].clusterID == clusterID)
      {
        blockNumToRead = cdtemp[index].first->blockNum;
        if(cdtemp[index].first->endBatch)
        {
          cPassBack->endOfClusterBatch_ = TRUE;
        }
        else
        {
          cPassBack->endOfClusterBatch_ = FALSE;
        }
        cPassBack->cBlock_ = cdtemp[index].first->next;
        break;
      }
      //if(cdtemp[index].clusterID == 0) //optimization, if clusterID cannot be 0.
      //  break;
    }//for    
  }
  //if scratch file for current block is asked, map the current block to
  //scratch file and return the scratch file. 
  if( getCurrentBlock && (Int32)blockNumToRead > 0 )
  {
    cPassBack->scratchFile_= 
      scrFilesMap_->mapBlockNumToScrFile(blockNumToRead, 
                                 cPassBack->blockOffset_);

    ex_assert(cPassBack->scratchFile_ != NULL,
      "HashScratchSpace::getClusterBlockNum, block number is invalid0");

  }
  else if(cPassBack->cBlock_ != NULL)
  { //store the scratch File of next cpassback for use in checkio.
    // Once a cPassBack->cBlock_ is found, map this block to the 
    // corresponding scratch file and pass back the scratch file and blockoffset
    // so that IO could be initiated on the scratch file if its handles are free.
    cPassBack->scratchFile_= 
      scrFilesMap_->mapBlockNumToScrFile(cPassBack->cBlock_->blockNum, 
                                 cPassBack->blockOffset_);

    ex_assert(cPassBack->scratchFile_ != NULL,
      "HashScratchSpace::getClusterBlockNum, block number is invalid1");
   
  }
  else if(cPassBack->cBlock_ == NULL)
  {
    //End of sequence. Reset scratchFile and blockOffset.
    cPassBack->scratchFile_ = NULL;
    cPassBack->blockOffset_ = -1;
  }
  return blockNumToRead;
}

//Note the usage of currentCDir_. If currentCDir_ is reused or cached, then
//the method assumes that the new blocknum is part of the write batch. if
//currentCDir_ changes, then it is a new batch.
RESULT HashScratchSpace::registerClusterBlock(UInt32 clusterID, DWORD blockNum)
{
  if(currentCDir_ == NULL)
  {
    CBlock *temp = (CBlock *) heap_->allocateMemory(sizeof(CBlock), FALSE);
    if (temp == NULL)   
    {
      sortError_->setErrorInfo( EScrNoMemory   //sort error
          ,NULL          //syserr: the actual FS error
          ,NULL          //syserrdetail
          ,"HashScratchSpace::registerClusterBlock"     //methodname
          ); 
      return SCRATCH_FAILURE;
    }
    temp->blockNum = blockNum;
    temp->next = NULL;
    temp->endBatch = FALSE;
    currentCDir_ = clusterDList_;  //first element of list.
    currentCDir_->clusterID = clusterID;
    currentCDir_->first = temp;
    currentCDir_->last = currentCDir_->first;
  }
  else if(currentCDir_->clusterID == clusterID)
  {
    CBlock *temp = (CBlock *) heap_->allocateMemory(sizeof(CBlock), FALSE);
    if (temp == NULL)   
    {
      sortError_->setErrorInfo( EScrNoMemory   //sort error
          ,NULL          //syserr: the actual FS error
          ,NULL          //syserrdetail
          ,"HashScratchSpace::registerClusterBlock"     //methodname
          ); 
      return SCRATCH_FAILURE;
    }
    temp->blockNum = blockNum;
    temp->next = NULL;
    temp->endBatch = FALSE;       //can directly set endbatchflag
    currentCDir_->last->next = temp;
    currentCDir_->last = temp;
  }
  else
  {
    ClusterDirectory *cdtemp = clusterDList_;
    UInt32 index;
    for(index = 0; index < numClusters_; index++)
    {
      if(cdtemp[index].clusterID == clusterID)
      {
        currentCDir_ = &cdtemp[index];
        CBlock *temp = (CBlock *) heap_->allocateMemory(sizeof(CBlock), FALSE);
        if (temp == NULL)   
        {
          sortError_->setErrorInfo( EScrNoMemory   //sort error
              ,NULL          //syserr: the actual FS error
              ,NULL          //syserrdetail
              ,"HashScratchSpace::registerClusterBlock"     //methodname
              ); 
          return SCRATCH_FAILURE;
        }
        temp->blockNum = blockNum;
        temp->next = NULL;
        temp->endBatch = FALSE; 
        currentCDir_->last->next = temp;
        currentCDir_->last->endBatch = TRUE;
        currentCDir_->last = temp;
        break;
      }
      else if(cdtemp[index].clusterID == 0) 
      { //new clusterID received, probably HJ can tell us to optimize.
        currentCDir_ = &cdtemp[index];
        currentCDir_->clusterID = clusterID;
        CBlock *temp = (CBlock *) heap_->allocateMemory(sizeof(CBlock), FALSE);
        if (temp == NULL)   
        {
          sortError_->setErrorInfo( EScrNoMemory   //sort error
              ,NULL          //syserr: the actual FS error
              ,NULL          //syserrdetail
              ,"HashScratchSpace::registerClusterBlock"     //methodname
              ); 
          return SCRATCH_FAILURE;
        }
        temp->blockNum = blockNum;
        temp->next = NULL;
        temp->endBatch = FALSE;
        currentCDir_->first = temp; 
        currentCDir_->last = temp;
        break;
      }
    }
    if(index == numClusters_)//we have reached a limit, double the cluster directory.
    {
      ClusterDirectory *tempClusterDList_ = (ClusterDirectory *) heap_->allocateMemory(numClusters_ * 2 * sizeof(ClusterDirectory), FALSE);
      if (tempClusterDList_ == NULL)   
      {
        sortError_->setErrorInfo( EScrNoMemory   //sort error
            ,NULL          //syserr: the actual FS error
            ,NULL          //syserrdetail
            ,"HashScratchSpace::registerClusterBlock"     //methodname
            ); 
        return SCRATCH_FAILURE;
      }
      memset((void*)tempClusterDList_, '\0', numClusters_ * 2 * sizeof(ClusterDirectory));
      memcpy((void*)tempClusterDList_, (void*)clusterDList_, numClusters_ * sizeof(ClusterDirectory));
      numClusters_ = numClusters_ * 2;
      heap_->deallocateMemory(clusterDList_);
      clusterDList_ = tempClusterDList_;
      currentCDir_ = &clusterDList_[index];
      currentCDir_->clusterID = clusterID;
      CBlock *temp = (CBlock *) heap_->allocateMemory(sizeof(CBlock), FALSE);
      if (temp == NULL)   
      {
        sortError_->setErrorInfo( EScrNoMemory   //sort error
            ,NULL          //syserr: the actual FS error
            ,NULL          //syserrdetail
            ,"HashScratchSpace::registerClusterBlock"     //methodname
            ); 
        return SCRATCH_FAILURE;
      }
      temp->blockNum = blockNum;
      temp->next = NULL;
      temp->endBatch = FALSE;
      currentCDir_->first = temp; 
      currentCDir_->last = temp;
    }
  }
  return SCRATCH_SUCCESS;
}

SortMergeNode::SortMergeNode(Lng32 associatedrun, SortScratchSpace* sortScratchSpace):
     associatedRun_(associatedrun), numRecsRead_ (0), nextReadPosition_(NULL),
     numReadQBlocks_(0),numOutStandingIO_(0), readQHead_(NULL),
     readQTail_(NULL), scratch_(sortScratchSpace)
{
  beginBlockNum_ = scratch_->runDirectory_->mapRunNumberToFirstSBN(associatedrun);
  endBlockNum_ = scratch_->runDirectory_->mapRunNumberToFirstSBN(associatedrun + 1);
  
  //if last run, then endBlockNum_ is EOF
  if(endBlockNum_ != EOF)
  {
    endBlockNum_--;
  }
  else
  {
    //Total scratch blocks is always ahead by 1.
    endBlockNum_ = sortScratchSpace->getTotalNumOfScrBlocks() -1;
    
  }
  nextIOBlockNum_ = beginBlockNum_;
  blockHead_.thisBlockNum_ = 
  blockHead_.nextBlockNum_ = 
  blockHead_.bytesUsed_ =   
  blockHead_.runNum_ = 
  blockHead_.numRecs_ =0;
};
SortMergeNode::~SortMergeNode()
{
  while(readQHead_)
    scratch_->returnFreeSortMergeBuffer(delinkReadQ());

}
void SortMergeNode::cleanup(void)
{
  while(readQHead_)
    scratch_->returnFreeSortMergeBuffer(delinkReadQ());
}

RESULT SortMergeNode::checkIO(Int64  &ioWaitTime, NABoolean waited)
{ 
    ioWaitTime = 0;
    
    //State should never be IDLE to begin with. IDLE is the last state of the buffer.
    ex_assert(readQHead_->state_ != IDLE, "SortMergeNode::checkIO, State_ is IDLE");
    
    // if this buffer is queued for I/O, try to serve the queue
    if (readQHead_->state_ == QUEUED)
    {
      do{
        //rearrange the queue so that this merge buffer is in the front of the
        //queue. usually we should not even reach here if double buffering and 
        //asynchronous reads takes place ahead of time. If we are reaching here 
        //means that we badly need to get this buffer filled in order to proceed.
        //PutAheadInQueue seem to introduce some latency. Hence commented out.
        //Proper fix would be do forecasting.
        //currentScrFile_->putAheadInQueue(this);
        readQHead_->retval_ = readQHead_->currentScrFile_->serveAsynchronousReadQueue(ioWaitTime,FALSE,waited);
        if((readQHead_->retval_ != IO_COMPLETE) &&
          (readQHead_->retval_ != SCRATCH_SUCCESS) &&
          (readQHead_->retval_ != IO_NOT_COMPLETE))
            return readQHead_->retval_;
        }while(waited && (readQHead_->state_ == QUEUED));
        
        if(!waited && ((readQHead_->state_ == QUEUED) || (readQHead_->state_ == READING)))
          return IO_NOT_COMPLETE;
    }
        
    // if this I/O is in progress check/wait for it to complete.
    if(readQHead_->state_ == READING) 
    {
      for(Int32 index = 0; index < readQHead_->currentScrFile_->getNumOpens(); index++)
      { 
        readQHead_->retval_ = readQHead_->currentScrFile_->checkScratchIO(index, waited? INFINITE : 0);
        
        if((readQHead_->retval_ != IO_COMPLETE) && (readQHead_->retval_ != SCRATCH_SUCCESS) &&
          (readQHead_->retval_ != IO_NOT_COMPLETE))
          return readQHead_->retval_;
        
        if(readQHead_->state_ != READING) 
          break;
      }
      if(readQHead_->state_ == READING) 
        return IO_NOT_COMPLETE;
    }
    
    if (readQHead_->state_ == READCOMPLETE)
    {
        readQHead_->state_ = BEING_CONSUMED;
        return readQHead_->retval_;
    }
    
    return SCRATCH_FAILURE;
}


void SortMergeNode::linkToReadQ(SortMergeBuffer *mb)
{
  if(readQHead_ == NULL)
  {
    readQHead_ = mb;
  }
  else
  {
    readQTail_->mnext_ = mb;
  }
  mb->sortMergeNodeRef_ = this;
  readQTail_ = mb;
  numReadQBlocks_++;
}

SortMergeBuffer* SortMergeNode::delinkReadQ(void)
{
  SortMergeBuffer *mb = readQHead_;
  if(readQHead_)
  {
    readQHead_ = readQHead_->mnext_;
    numReadQBlocks_--;
  }
  return mb;
}

void SortMergeBuffer::processMisc(void)
{
  sortMergeNodeRef_->numOutStandingIO_--;
};

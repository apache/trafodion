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
* File:         ScratchFileMap.C
* RCS:          $Id: scratchfilemap.cpp,v 1.1 2006/11/01 01:44:37  Exp $
*                               
* Description:  This file contains the implementation of all member functions
*               of the ScratchFileMap container class.
*                              
* Created:	    05/30/96
* Modified:     $ $Date: 2006/11/01 01:44:37 $ (GMT)
* Language:     C++
* Status:       $State: Exp $
*
******************************************************************************
*/


#include "Platform.h"

#include <iostream>
#include <string.h>
#include "ex_ex.h"
#include "ScratchFileMap.h"
#include "ScratchFile.h"

//----------------------------------------------------------------------
// The Class constructor. If the number of scratch files is not specified
// then a default of 128 is assumed.
//----------------------------------------------------------------------

ScratchFileMap::ScratchFileMap(CollHeap* heap, SortError* sorterror,
                               NABoolean breakEnabled, 
			       short maxscrfiles) 
{
  heap_ = heap;  
  maxScratchFiles_ = maxscrfiles;
  numScratchFiles_ = 0;
  currentScratchFiles_ = 0;
  fileMap_ = (FileMap*)heap_->allocateMemory(maxScratchFiles_ * sizeof(FileMap));
  memset((void*)fileMap_, '\0', maxScratchFiles_ * sizeof(FileMap));
  sortError_ = sorterror;
  breakEnabled_ = breakEnabled;
}

//----------------------------------------------------------------------
// The Class destructor.
// Delete all indirect space in ScratchFileMap.
//----------------------------------------------------------------------
ScratchFileMap::~ScratchFileMap(void)
{
  closeFiles();
  heap_->deallocateMemory((void*)fileMap_);
  fileMap_ = NULL;
}

void ScratchFileMap::closeFiles(ScratchFile* keepFile)
{
  FileMap savedMap;
  bool foundKeepFile = false;

  if (fileMap_ != NULL)
  {
    for (Int32 i = 0; i < numScratchFiles_; ++i)
    {
      // delete this scratchFile
      FileMap& map = fileMap_[i];
      if (keepFile !=NULL && (map.scrFile_ == keepFile))
      {
        foundKeepFile = true;
        savedMap.scrFile_ = map.scrFile_;
        savedMap.index_ = map.index_;
        savedMap.firstScrBlockWritten_ = map.firstScrBlockWritten_;
      }
      else
      {
        if(map.scrFile_ != NULL)
        {
          delete map.scrFile_;
          map.scrFile_ = NULL;
        }
      }
      map.scrFile_ = NULL;
      map.index_ = 0;
      map.firstScrBlockWritten_ = 0;
    }

    if (foundKeepFile)
    {
      numScratchFiles_ = 1;
      currentScratchFiles_ = 1;
      fileMap_->scrFile_ = savedMap.scrFile_;
      fileMap_->index_ = savedMap.index_;
      fileMap_->firstScrBlockWritten_ = savedMap.firstScrBlockWritten_;
    }
    else
    {
      numScratchFiles_ = 0;
      currentScratchFiles_ = 0;
    }
  }
}

//Close files upto the specified blocknum.The specified
//blocknum is excluded.
void ScratchFileMap::closeScrFilesUpto(SBN uptoBlockNum)
{
  ex_assert(fileMap_ != NULL, "ScratchFileMap::closeFiles, fileMap_ is NULL");

  for (Int32 i = 0; i < numScratchFiles_; i++)
  {
    //scenario 1: uptoBlocknum is first block of first(current) file.
    if ( uptoBlockNum <= fileMap_[i].firstScrBlockWritten_ )
      break;
    else 
    { //scenario 2: uptoBlockNum > first block of current file
      
      //check if current file can be deleted if uptoBlockNum
      //is >= next file's first blocknum
      
      //if current file is already the last file, then nothing 
      //to do. 
      if( (i+1) == numScratchFiles_) 
        break;

      if(uptoBlockNum >= fileMap_[i+1].firstScrBlockWritten_)
      {
        if(fileMap_[i].scrFile_ != NULL)
        {
          delete fileMap_[i].scrFile_;
          fileMap_[i].scrFile_ = NULL;
        }//if
      }//if
    }//else
  }//for
}

//-----------------------------------------------------------------------
// Name         : createNewScrFile
// 
// Parameters   : ...
//
// Description  : This function creates a new scratch file and returns a
//                pointer to the scratch file object. Currently there is 
//                no intelligence used in deciding the volume for scratch
//                file. However this is the place where volume selection 
//                logic should be implemented.
//  
// Return Value :
//  Pointer to the newly created scratch file object. 
//  NULL if unsuccessful.
//-----------------------------------------------------------------------

ScratchFile* ScratchFileMap::createNewScrFile(
                                ScratchSpace *scratchSpace,
                                Int32 scratchMgmtOption,
                                Int32 scratchMaxOpens,
                                NABoolean preAllocateExtents,
                                NABoolean asynchReadQueue)
{
  FileMap *tempFileMap; 

  if((numScratchFiles_ + 1) >= maxScratchFiles_)
  {
    sortError_->setErrorInfo( EThresholdReached   //sort error
                     ,0          //syserr: the actual FS error
                     ,0          //syserrdetail
                     ,"ScratchFileMap::createNewScrFile"    
                     );
    return NULL; 
  }
  // assuming we're going to reference the newly created scratch file
  currentScratchFiles_ = numScratchFiles_ += 1;

  
  // There is a structure of an index followed by a scratch file info.  There
  // is a list of such structures namely FileMap, each structure is associated
  // to one scratch file. 

  tempFileMap = fileMap_ + (numScratchFiles_ - 1);
  tempFileMap->index_   = numScratchFiles_;


   tempFileMap->scrFile_ = new (heap_) SQScratchFile( 
                         scratchSpace, 
                         sortError_, heap_,
                         breakEnabled_,
                         scratchMaxOpens,
                         asynchReadQueue);

  if (tempFileMap->scrFile_ == NULL)
  {
      sortError_->setErrorInfo( EScrNoMemory   //sort error
			       ,0          //syserr: the actual FS error
			       ,0          //syserrdetail
			       ,"ScratchFileMap::createNewScratchFile"     //methodname
			       );
      return NULL;
  }
  if (sortError_->getSortError() )  
  {
     return NULL;
  }
  //possible error here, see sortError_->getSortError()

  return tempFileMap->scrFile_; 
}

Lng32 ScratchFileMap::totalNumOfReads()
{
  Lng32 totalReads = 0L;
  short i = 0;

  for (i=0; i < numScratchFiles_; i++)
  {
    if(fileMap_[i].scrFile_ != NULL)
      totalReads += (fileMap_[i].scrFile_)->getNumOfReads();
  }
  return totalReads;
}

Lng32 ScratchFileMap::totalNumOfWrites()
{
  Lng32 totalWrites = 0L;
  short i = 0;

  for (i=0; i < numScratchFiles_; i++)
  {
    if(fileMap_[i].scrFile_ != NULL)
      totalWrites += (fileMap_[i].scrFile_)->getNumOfWrites();
  }
  return totalWrites;
}

//-----------------------------------------------------------------------
// Name         : mapBlockNumToScrFile 
//
// Parameters   : scratch block number 
//
// Description  : This function finds out which scratch file the passed 
//                block number resides.
//
// Return Value : Pointer to the scratch file object.
//                NULL if unsuccessful.
//-----------------------------------------------------------------------
ScratchFile* ScratchFileMap::mapBlockNumToScrFile(SBN blockNum,
                                                  Lng32 &blockOffset)
{
  ex_assert(blockNum > 0, "ScratchFileMap::mapBlockNumToScrFile, blockNum <= 0"); 

  Int32 i = 0;
  for (; i < numScratchFiles_; i++)
  {
   if (  blockNum < fileMap_[i].firstScrBlockWritten_ )
   {
     blockOffset = blockNum - fileMap_[i - 1].firstScrBlockWritten_;
      return fileMap_[i-1].scrFile_;
   }
  }

  // -- Is this block on the last scratch file ?
  //This block of code handles the scenario if the scratch file is
  //if the first one or the very last one. 
  if ( blockNum >= fileMap_[i - 1].firstScrBlockWritten_ )
  {
    blockOffset = blockNum - fileMap_[i - 1].firstScrBlockWritten_;
    return fileMap_[i-1].scrFile_;
  }
  return NULL;  // cannot not find the scratch file, caller will handle error
}

//-----------------------------------------------------------------------
// Name         : setFirstScrBlockNum 
//
// Parameters   : scratch block number
//
// Description  : This function records block number of the first block in 
//                a scratch file. 
//
// Return Value : none 
//-----------------------------------------------------------------------
void ScratchFileMap::setFirstScrBlockNum(SBN blockNum)
{
 //At this time, a new scrath file has just been created and at this time  
 //currentScratchFiles_ always reflects the newly created scratch file.
 fileMap_[currentScratchFiles_-1].firstScrBlockWritten_ = blockNum;
} 

//-----------------------------------------------------------------------
// Name         : getFirstScrBlockNum
//
// Parameters   : none 
//
// Description  : This function returns block number of the first block on
//                the current scratch file.
//
// Return Value : none
//-----------------------------------------------------------------------
SBN ScratchFileMap::getFirstScrBlockNum(ScratchFile* scr)
{
 for (Int32 i=0; i < numScratchFiles_; i++) {
  if (fileMap_[i].scrFile_ == scr) {
   return fileMap_[i].firstScrBlockWritten_;
  }
 }
 return -1; //cannot find the scr file info
}

Lng32 ScratchFileMap::totalNumOfAwaitio()
{
  Lng32 totalAwaitio = 0L;
  short i = 0;
  for (i=0; i < numScratchFiles_; i++)
  {
    if(fileMap_[i].scrFile_ != NULL)   
      totalAwaitio += fileMap_[i].scrFile_->getNumOfAwaitio();
  }
  return totalAwaitio;
}

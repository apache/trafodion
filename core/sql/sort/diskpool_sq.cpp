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
// diskpool.C -*-C++-*-
#include "Platform.h"
#include "DiskPool_sq.h"

//--------------------------------------------------------------------------
//UNIXDisk()
//    The constructor of NT version of DiskPool.
//--------------------------------------------------------------------------
SQDisk::SQDisk(SortError* sorterror, CollHeap *heap) :DiskPool(heap),
sortError_(sorterror) {
  
  factorImportanceTotalFreeSpace_ = 30;
  factorImportanceNumScrFiles_ = 70;   
}

//--------------------------------------------------------------------------
//~SQDisk()
//     The destructor.  It will destroy all the individual DiskDetails
//     but the table remains and will be destroyed by the destructor
//     for diskpo0l.
//--------------------------------------------------------------------------
SQDisk::~SQDisk()
{
  if (diskTablePtr_ != NULL) {
    for (short i=0; i < numberOfDisks_; i++) 
    {
       delete diskTablePtr_[i];
    }
    NADELETEBASIC(diskTablePtr_, heap_); //? USE NADELETEARRAY 
    diskTablePtr_ = NULL;
  }
}
//--------------------------------------------------------------------------
//generateDiskTable(char*, answer)
//    This will generate the appropriate DiskDetails and the array that
//    holds the pointer to them.  It decides on which disk to include
//    based on the sub type and the name of the volumes.  The subtype
//    must not be OpticalDisk and the name must match the mask
//    if the answer is right and mismatch if answer is wrong.
//--------------------------------------------------------------------------

NABoolean 
 SQDisk::generateDiskTable(const ExScratchDiskDrive * scratchDiskSpecified,
            ULng32 numSpecified,
            char * volumeNameMask, 
            answer including,
            NABoolean includeAuditTrailDisks
            ) 
{
   diskTablePtr_ = new (heap_) DiskDetailsPtr[1]; // YJC, local disk on /tmp
   numberOfDisks_ = 1;
   DiskDetails* disk = new (heap_) DiskDetails;
   strcpy(disk->rootPathName_, "/");
   strcpy(disk->tempPathName_, "/tmp");
   disk->diskType_ = SQDT_DEFAULT;
   disk->numOpenScratchFiles_ = 0; 
   diskTablePtr_[0] = disk;
   return SORT_SUCCESS;
}


//--------------------------------------------------------------------------
//  Boolean returnBestDisk(char* diskname)
//  This function returns the best disk to use as a scratch disk/
//   -refreshes each disk and recalculates the weight.
//   -picks the disk of the highest weight.
//--------------------------------------------------------------------------
NABoolean SQDisk::returnBestDisk(char** diskname,
             ULng32 espInstance,
             ULng32 numEsps, 
             unsigned short threshold)
{
   if (numberOfDisks_ == 0 && diskTablePtr_ == NULL)
   {
      *diskname = NULL;
      sortError_->setErrorInfo( EScrNoDisks       //sort error
        ,0             //syserr: the actual FS error
        ,0             //syserrdetail
        ,"SQDisk::returnBestDisk"     //methodname
     );
      return SORT_FAILURE;
   }
  
   const char *pEnvStr = getenv("SQ_SQL_SCRATCH_DIR");
   if (pEnvStr != NULL) 
     *diskname = (char *)pEnvStr;
   else
     *diskname = diskTablePtr_[0]->tempPathName_;
   return SORT_SUCCESS;
}

/******************************************************************************
*
* File:         DiskPool_base.h
*                               
* Description:  This file contains the member function implementation for 
*               class DiskPool. This class is used to encapsulate all 
*               data and methods about a scratch file.  
*                                                                 
* Created:      01/02/2007
* Language:     C++
* Status:       Re-write to move platform dependent implemenations out of this 
* 				base file. 
*
*
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
*
*
******************************************************************************
*/

#ifndef DISKPOOL_BASE_H
#define DISKPOOL_BASE_H

#include "Platform.h"

#include <iostream>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include "Const.h"
#include "CommonStructs.h"
#include "CommonUtil.h"
#include "NABasicObject.h"
#include "SortError.h"
#include "SortUtilCfg.h"

//--------------------------------------------------------------------------
//  This is for including the right list header when compileing.
//--------------------------------------------------------------------------
#ifdef USERW
#include <rw/gslist.h>
#else
#include "List.h"
#endif

#ifdef FORDEBUG
#include <iomanip>
#endif

// Each platform must define its own DiskDetails, which is a subclass of NABasicObject
struct DiskDetails;
class ScratchSpace;

class DiskPool : public NABasicObject {
  
public:
  DiskPool(CollHeap *heap);
  virtual ~DiskPool() = 0;  // pure virtual
  
  virtual 
  NABoolean generateDiskTable(const ExScratchDiskDrive * scratchDiskSpecified,
			      ULng32 numSpecified,			     
			      char * volumeNameMask,
			      answer including = ::right,
                              NABoolean includeAuditTrailDisks = FALSE
			      ) = 0;

  virtual NABoolean returnBestDisk(char** diskname,
                                   ULng32 espInstance, 
                                   ULng32 numEsps,
                                   unsigned short threshold
                                  ) = 0; 

  DiskDetails **getDiskTablePtr() const {return diskTablePtr_;};
  DiskDetails **getLocalDisksPtr() const {return localDisksPtr_;};
  Int32 getNumberOfDisks() {return numberOfDisks_; };
  Int32 getNumberOfLocalDisks() {return numberOfLocalDisks_; };
  void setScratchSpace(ScratchSpace *scratchSpace)
  {
    scratchSpace_ = scratchSpace;
  }


#ifdef FORDEBUG
  virtual NABoolean printDiskTable() = 0;
#endif

  protected:

  virtual void assignWeight(DiskDetails *diskPtr) = 0;

  virtual NABoolean refreshDisk(DiskDetails *diskPtr) = 0;
  virtual NABoolean computeNumScratchFiles(DiskDetails *diskPtr )= 0;
                                        // index into the diskdetails pointer
  Int32 numberOfDisks_;
  Int32 numberOfLocalDisks_;

  DiskDetails **diskTablePtr_;           // This is a pointer to an array
                                         // of <n> pointers to structures 
                                         // of the type DiskDetails.
  DiskDetails **localDisksPtr_;          //Pointer to array of local disks
                                         //pointers

  short currentDisk_;                   //This feild is deprecated on NSK.
  ScratchSpace *scratchSpace_;
  CollHeap *heap_;
};

typedef DiskDetails* DiskDetailsPtr;
const Lng32 longMaxInPageUnits =1048576;// INT_MAX/2048


#endif //DISKPOOL_BASE_H 


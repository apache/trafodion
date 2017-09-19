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
// diskpool.h -*-C++-*-
#ifndef DISKPOOL_SQ_H
#define DISKPOOL_SQ_H

#include "Platform.h"
#include "DiskPool_base.h"

class SQDisk : public DiskPool {

public:

  SQDisk(SortError *sorterror, CollHeap *heap);
  ~SQDisk();

  virtual 

  NABoolean generateDiskTable(const ExScratchDiskDrive * scratchDiskSpecified,
 			      ULng32 numSpecified,
                              char * volumeNameMask,
 			      answer including = ::right,
 			      NABoolean includeAuditTrailDisks = FALSE
 			      );
  
  virtual NABoolean returnBestDisk(char** diskname,
                                   ULng32 espInstance, 
                                   ULng32 numEsps,
                                   unsigned short threshold
                                  ); 

// LCOV_EXCL_START

#ifdef FORDEBUG
  virtual NABoolean printDiskTable(){return TRUE;};
#endif

private:

  virtual void assignWeight(DiskDetails *diskPtr){};
  virtual NABoolean refreshDisk(DiskDetails *diskPtr){ return TRUE;};
  virtual NABoolean computeNumScratchFiles(DiskDetails *diskptr ){return TRUE;};

// LCOV_EXCL_STOP

  short factorImportanceTotalFreeSpace_;
  short factorImportanceNumScrFiles_;  
  SortError *sortError_;
};

//--------------------------------------------------------------------------
//  DiskDetails varies with the implementation so the DiskDetail for NSK
//  is different then the DiskDetail for NT.
//  The ??? means that I have not been able to find a way to retrieve
//  this particular information.
//--------------------------------------------------------------------------
//----------------------------------------------------------------------
// This structure is used to store information about each physical disk
// that are part of the diskpool.
//----------------------------------------------------------------------

enum SQDiskType {SQDT_UNKNOWN, SQDT_DEFAULT};

struct DiskDetails : public NABasicObject {
	// common fields between platforms
	Lng32 weight_;
  Lng32 freeSpace_;
  short numOpenScratchFiles_;
  //----------------------------------------------
  //  Since we are doing this under NT,
  //  we will use the Win32 data types
  //  to appease the NT gods.
  //----------------------------------------------
  
  char rootPathName_[PATH_MAX];
  char tempPathName_[PATH_MAX];
    
  SQDiskType diskType_;	
};
#endif



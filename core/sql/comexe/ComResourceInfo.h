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
****************************************************************************
*
* File:         ComResourceInfo.h
* Description:  Compiled information on resources such as scratch files
*               (usable drive letters, placement, etc.)
*               

* Created:      12/30/98
* Language:     C++
*
*
*
****************************************************************************
*/

#ifndef SCRATCH_FILES_H
#define SCRATCH_FILES_H

#include "Int64.h"
#include "NAVersionedObject.h"
#include "IpcMessageType.h"

// -----------------------------------------------------------------------
// Contents of this file
// -----------------------------------------------------------------------
class ExScratchFileOptions;

// -----------------------------------------------------------------------
// Forward references
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// Constants for invalid cluster number
// -----------------------------------------------------------------------

const Int32 RES_LOCAL_CLUSTER = -1;

// -----------------------------------------------------------------------
// An entry for a specific disk drive. This is not an NAVersionedObject
// because it hangs directly below an ExScratchFileOptions object which
// is versioned. Nevertheless, this object gets stored in module files.
// -----------------------------------------------------------------------
class ExScratchDiskDrive
{
public:

  ExScratchDiskDrive(
       char  *diskName = NULL,
       Lng32 diskNameLen = 0,
       Int32 nodeNumber = IPC_CPU_DONT_CARE,
       Int32 clusterNumber = RES_LOCAL_CLUSTER,
       char *clusterName = NULL,
       Lng32 clusterNameLen = 0) : diskName_(diskName),
	 diskNameLength_(diskNameLen), nodeNumber_(nodeNumber),
	 clusterNumber_(clusterNumber), clusterName_(clusterName),
         clusterNameLength_(clusterNameLen) {}

  inline Int32 getClusterNumber() const         { return clusterNumber_; }
  inline const char * getClusterName() const      { return clusterName_; }
  inline Int32 getClusterNameLength() const { return clusterNameLength_; }
  inline Int32 getNodeNumber() const               { return nodeNumber_; }
  inline const char * getDiskName() const            { return diskName_; }
  inline Int32 getDiskNameLength() const       { return diskNameLength_; }
  inline void setClusterNumber(Int32 s)            { clusterNumber_ = s; }
  inline void setClusterName(const char * s)         { clusterName_ = s; }
  inline void setClusterNameLength(Int32 s)    { clusterNameLength_ = s; }
  inline void setNodeNumber(Int32 s)                  { nodeNumber_ = s; }
  inline void setDiskName(char *s)                      { diskName_ = s; }
  inline void setDiskNameLength(Int32 s)          { diskNameLength_ = s; }

  // Although this class is not derived from NAVersionedObject, it still
  // gets compiled as a dependent object of ExScratchFileOptions and it
  // still needs to be packed and unpacked.
  Long pack(void * space);
  Lng32 unpack(void *base, void * reallocator);

private:

  // the cluster name from which the disk is to be used
  // (EXPAND node name on NSK, unused on NT for now),
  // similar to the data member in ExEspNodeMapEntry
  NABasicPtr clusterName_;                                     // 00-07
  Int32      clusterNameLength_;                               // 08-11
  // primary CPU number on NSK, node number or primary node
  // number on NT (right now this is needed only on NT for
  // non-shared disks)
  Int32      nodeNumber_;                                      // 12-15
  // name of the disk ("$VOL" for shared disks or "C:" for
  // local NT disks).
  NABasicPtr diskName_;                                        // 16-23
  // length of disk name
  Int32      diskNameLength_;                                  // 24-27
  // the cluster number corresponding to the cluster name
  // NOTE: this will eventually go away, there are problems
  // with storing NSK node numbers on disk
  Int32      clusterNumber_;                                   // 28-31
  char       fillersExScratchDiskDrive_[16];                   // 32-47
};

// -----------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for ExScratchDiskDrive and its dependents.
//
// An ExScratchDiskDrivePtr is a pointer to a contiguous array of
// ExScratchDiskDrive object, representing a list of disk drives.
// Such arrays get stored in the module file. They don't have a
// version by themselves, so their versioning is done via the
// parent object ExScratchFileOptions.
// -----------------------------------------------------------------------
typedef
  NAOpenObjectPtrTempl<ExScratchDiskDrive> ExScratchDiskDrivePtr;

// -----------------------------------------------------------------------
// Options for scratch files (generated by the compiler)
// -----------------------------------------------------------------------

class ExScratchFileOptions : public NAVersionedObject
{
public:
  ExScratchFileOptions() : NAVersionedObject(-1)  {scratchFlags_ = 0;}
  
  virtual unsigned char getClassVersionID()
  {
    return 1;
  }

  virtual void populateImageVersionIDArray()
  {
    setImageVersionID(0,getClassVersionID());
  }

  virtual short getClassSize() { return (short)sizeof(ExScratchFileOptions); }

  // The scratch file options get generated by the generator, so we need
  // pack and unpack procedures for it (they handle the entries as well)
  //
  Long pack(void * space);
  Lng32 unpack(void *, void * reallocator);

  void setSpecifiedScratchDisks(ExScratchDiskDrive *s,Lng32 numEntries)
            { specifiedScratchDisks_ = s; numSpecifiedDisks_ = numEntries; }
  void setExcludedScratchDisks(ExScratchDiskDrive *s,Lng32 numEntries)
              { excludedScratchDisks_ = s; numExcludedDisks_ = numEntries; }
  void setPreferredScratchDisks(ExScratchDiskDrive *s,Lng32 numEntries)
            { preferredScratchDisks_ = s; numPreferredDisks_ = numEntries; }
 
  void setScratchMgmtOption(Lng32 entry)
  { switch(entry)
    {
      case 5:
        scratchFlags_ |= SCRATCH_MGMT_OPTION_5;
        break;
      case 9:
        scratchFlags_ |= SCRATCH_MGMT_OPTION_9;
        break;
      case 11:
        scratchFlags_ |= SCRATCH_MGMT_OPTION_11;
        break;
      default:
        break;
    };
  }

  Int32 getScratchMgmtOption(void)  const
  { 
    if((scratchFlags_ & SCRATCH_MGMT_OPTION_5) != 0)
      return 5;
    else if((scratchFlags_ & SCRATCH_MGMT_OPTION_9) != 0)
      return 9;
    else if((scratchFlags_ & SCRATCH_MGMT_OPTION_11) != 0)
      return 11;
    else 
      return 0;
  }
  
  void setScratchMaxOpensHash(Lng32 entry)
  { switch(entry)
    {
      case 2:
        scratchFlags_ |= SCRATCH_MAX_OPENS_HASH_2;
        break;
      case 3:
        scratchFlags_ |= SCRATCH_MAX_OPENS_HASH_3;
        break;
      case 4:
        scratchFlags_ |= SCRATCH_MAX_OPENS_HASH_4;
        break;
      default:
        break;
    };
  }

  Int32 getScratchMaxOpensHash(void)  const
  { 
    if((scratchFlags_ & SCRATCH_MAX_OPENS_HASH_2) != 0)
      return 2;
    else if((scratchFlags_ & SCRATCH_MAX_OPENS_HASH_3) != 0)
      return 3;
    else if((scratchFlags_ & SCRATCH_MAX_OPENS_HASH_4) != 0)
      return 4;
    else 
      return 1;
  }

   void setScratchMaxOpensSort(Lng32 entry)
  { switch(entry)
    {
      case 2:
        scratchFlags_ |= SCRATCH_MAX_OPENS_SORT_2;
        break;
      case 3:
        scratchFlags_ |= SCRATCH_MAX_OPENS_SORT_3;
        break;
      case 4:
        scratchFlags_ |= SCRATCH_MAX_OPENS_SORT_4;
        break;
      default:
        break;
    };
  }

  Int32 getScratchMaxOpensSort(void)  const
  { 
    if((scratchFlags_ & SCRATCH_MAX_OPENS_SORT_2) != 0)
      return 2;
    else if((scratchFlags_ & SCRATCH_MAX_OPENS_SORT_3) != 0)
      return 3;
    else if((scratchFlags_ & SCRATCH_MAX_OPENS_SORT_4) != 0)
      return 4;
    else 
      return 1;
  }

  void setScratchPreallocateExtents(NABoolean entry)
  { (entry ? scratchFlags_ |= SCRATCH_PREALLOCATE_EXTENTS : scratchFlags_ &= ~SCRATCH_PREALLOCATE_EXTENTS); }

  NABoolean getScratchPreallocateExtents(void)  const
  { return (scratchFlags_ & SCRATCH_PREALLOCATE_EXTENTS) != 0; };

  void setScratchDiskLogging(NABoolean entry)
  { (entry ? scratchFlags_ |= SCRATCH_DISK_LOGGING : scratchFlags_ &= ~SCRATCH_DISK_LOGGING); }

  NABoolean getScratchDiskLogging(void)  const
  { return (scratchFlags_ & SCRATCH_DISK_LOGGING) != 0; };
  
  inline const ExScratchDiskDrive * getSpecifiedScratchDisks() const
                                          { return specifiedScratchDisks_; }
  inline const ExScratchDiskDrive * getExcludedScratchDisks() const
                                           { return excludedScratchDisks_; }
  inline const ExScratchDiskDrive * getPreferredScratchDisks() const
                                          { return preferredScratchDisks_; }

  inline Int32 getNumSpecifiedDisks() const   { return numSpecifiedDisks_; }
  inline Int32 getNumExcludedDisks() const     { return numExcludedDisks_; }
  inline Int32 getNumPreferredDisks() const   { return numPreferredDisks_; }
  // to make the IPC methods happy, this object also supplies some
  // methods to help packing it into an IpcMessageObj
  Lng32 ipcPackedLength() const;
  Lng32 ipcGetTotalNameLength() const;
  Lng32 ipcPackObjIntoMessage(char *buffer) const;
  void ipcUnpackObj(Lng32 objSize,
		    const char *buffer,
		    CollHeap *heap,
		    Lng32 totalNameLength,
		    char *&newBufferForDependents);

private:
  enum scratchFlagsType
  {
    SCRATCH_MGMT_OPTION_5 = 0x0001, 
    SCRATCH_MGMT_OPTION_9 = 0x0002,
    SCRATCH_PREALLOCATE_EXTENTS = 0x0004,
    SCRATCH_MAX_OPENS_HASH_2 = 0x0008,
    SCRATCH_MAX_OPENS_HASH_3 = 0x0010,
    SCRATCH_MAX_OPENS_HASH_4 = 0x0020,
    SCRATCH_MAX_OPENS_SORT_2 = 0x0040,
    SCRATCH_MAX_OPENS_SORT_3 = 0x0080,
    SCRATCH_MAX_OPENS_SORT_4 = 0x0100,
    SCRATCH_MGMT_OPTION_11 = 0x0200,
    SCRATCH_DISK_LOGGING = 0x0400
  };

  // a list of disks (for different nodes in the cluster) to
  // be used (or an empty string if the system can decide)
  ExScratchDiskDrivePtr specifiedScratchDisks_;                       // 00-07

  // alternatively, a list of disks NOT to be used
  ExScratchDiskDrivePtr excludedScratchDisks_;                        // 08-15

  // a list of preferred disks (system may also use others)
  ExScratchDiskDrivePtr preferredScratchDisks_;                       // 16-23

  Int32                 numSpecifiedDisks_;                           // 24-27
  Int32                 numExcludedDisks_;                            // 28-31
  Int32                 numPreferredDisks_;                           // 32-35
  UInt32                scratchFlags_;                     //36-39
  char                  fillersExScratchFileOptions[24];              // 40-63
};

// -----------------------------------------------------------------------
// Template instantiation to produce a 64-bit pointer emulator class
// for ExScratchFileOptions and its dependents
// -----------------------------------------------------------------------
typedef
  NAVersionedObjectPtrTempl<ExScratchFileOptions> ExScratchFileOptionsPtr;


#endif /* SCRATCH_FILES_H */

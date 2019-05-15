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
* File:         FragDir.cpp
* Description:  
*
* Created:      5/6/98
* Language:     C++
*
*
*
*
****************************************************************************
*/

// -----------------------------------------------------------------------

#include "FragDir.h"
#include "ComPackDefs.h"
#include "PartInputDataDesc.h"
#include "Ipc.h"
#include "trafconf/trafconfig.h"
static const int nodeNameLen = TC_PROCESSOR_NAME_MAX;//defined in trafconf/trafconfig.h

ExFragDir::ExFragDir(Lng32 entries, Space *space,
            NABoolean multiFragments, NABoolean fragmentQuotas,
            UInt16 multiFragmentVm, UInt8 numMultiFragments)
  : NAVersionedObject(-1)
{
  numEntries_ = entries;
  scratchFileOptions_ = (ExScratchFileOptionsPtr)NULL;
  nodeMask_ = 0;
  planVersion_ = ComVersion_GetCurrentPlanVersion();

  // allocate an array of entries from "space"
  fragments_ = (ExFragDirEntryPtr *) space->allocateAlignedSpace(
                     (size_t) (sizeof(ExFragDirEntryPtr) * numEntries_) );

  for (Int32 i = 0; i < numEntries_; i++)
    fragments_[i] = new(space) ExFragDirEntry();
  flags_ = 0;
  if (multiFragments)
    flags_ |= MULTI_FRAGMENTS;
  if (fragmentQuotas)
    flags_ |= FRAGMENT_QUOTAS;
  multiFragmentVm_ = multiFragmentVm;
  numMultiFragments_ = numMultiFragments;
}

Long ExFragDir::pack(void * space)
{
  fragments_.pack(space,numEntries_);
  scratchFileOptions_.pack(space);
  return NAVersionedObject::pack(space);
}

Lng32 ExFragDir::unpack(void * base, void * reallocator)
{
  if(fragments_.unpack(base,numEntries_,reallocator)) return -1;
  if(scratchFileOptions_.unpack(base, reallocator)) return -1;
  return NAVersionedObject::unpack(base, reallocator);
}

Lng32 ExFragDir::getExplainFragDirEntry(Lng32 &fragOffset, Lng32 &fragLen, Lng32 &topNodeOffset)
{
  // Find the EXPLAIN Fragment
  CollIndex explainFragIndex = NULL_COLL_INDEX;
    
  // Loop over all the fragments
  for (CollIndex i = 0; i < (CollIndex)getNumEntries(); i++)    
  { 
    if (getType(i) == ExFragDir::EXPLAIN)
    {
      explainFragIndex = i;
      break;
    }
  }

  // There should always be just one EXPLAIN Fragment
  if (explainFragIndex == NULL_COLL_INDEX)
      return -1;
      
  fragLen = getFragmentLength(explainFragIndex);
  fragOffset = getGlobalOffset(explainFragIndex);
  topNodeOffset = getTopNodeOffset(explainFragIndex);
  return 0;
}

Long ExFragDirEntry::pack(void *space)
{
  partDescriptor_.pack(space);
  espNodeMap_.pack(space);
  return NAVersionedObject::pack(space);
}

Lng32 ExFragDirEntry::unpack(void * base, void * reallocator)
{
  if(partDescriptor_.unpack(base, reallocator)) return -1;
  if(espNodeMap_.unpack(base, reallocator)) return -1;
  return NAVersionedObject::unpack(base, reallocator); 
}

Long ExEspNodeMapEntry::pack(void *space)
{
  return clusterName_.pack(space);
}

Lng32 ExEspNodeMapEntry::unpack(void * base, void * reallocator)
{
  return clusterName_.unpack(base);
}

ExEspNodeMap::ExEspNodeMap()
  : map_(NULL), entries_(0)
{}

Lng32 ExEspNodeMap::getNodeNumber(Lng32 instance) const
{ 
  if ( map_ == (ExEspNodeMapEntryPtr) NULL )
    return IPC_CPU_DONT_CARE;
  return map_[instance].nodeNumber_;  
}

const char * ExEspNodeMap::getClusterName(Lng32 instance) const
{ 
  if ( map_ == (ExEspNodeMapEntryPtr) NULL )
    return NULL;
  return map_[instance].clusterName_; 
}

void ExEspNodeMap::setEntry(Lng32 instance,
			    const char *clusterName,
			    Lng32 nodeNumber,
			    Space *space)
{
  if (map_ && entries_ > instance)
    {
      map_[instance].clusterName_ = space->allocateMemory(nodeNameLen);
      strcpy(map_[instance].clusterName_, clusterName);
      map_[instance].nodeNumber_ = nodeNumber;
    }
}

Long ExEspNodeMap::pack(void *space)
{
  return map_.packArray(space, entries_);
}

Lng32 ExEspNodeMap::unpack(void * base, void * reallocator)
{
  return map_.unpackArray(base, entries_, reallocator);
}


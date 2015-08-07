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
* File:         ComResourceInfo.cpp
* Description:  Compiled information on resources such as scratch files
*               (usable drive letters, placement, etc.)
*
* Created:      1/12/98
* Language:     C++
*
*
*
****************************************************************************
*/

#include "ComResourceInfo.h"

Long ExScratchDiskDrive::pack(void * space)
{
  clusterName_.pack(space);
  return diskName_.pack(space);
}

Lng32 ExScratchDiskDrive::unpack(void * base, void * reallocator)
{
  clusterName_.unpack(base);
  return diskName_.unpack(base);
}

Long ExScratchFileOptions::pack(void * space)
{
  // pack the contents of the 3 arrays
  Int32 i;

  for (i=0; i < numSpecifiedDisks_; i++)
    {
      specifiedScratchDisks_[i].pack(space);
    }

  for (i=0; i < numExcludedDisks_; i++)
    {
      excludedScratchDisks_[i].pack(space);
    }

  for (i=0; i < numPreferredDisks_; i++)
    {
      preferredScratchDisks_[i].pack(space);
    }

  // convert the pointers to the 3 arrays to offsets
  specifiedScratchDisks_.packShallow(space);
  excludedScratchDisks_.packShallow(space);
  preferredScratchDisks_.packShallow(space);

  return NAVersionedObject::pack(space);
}

Lng32 ExScratchFileOptions::unpack(void * base, void * reallocator)
{
  // convert the offsets to the 3 arrays to pointers
  if (specifiedScratchDisks_.unpackShallow(base))
    return -1;
  if (excludedScratchDisks_.unpackShallow(base))
    return -1;
  if (preferredScratchDisks_.unpackShallow(base))
    return -1;
  
  // pack the contents of the 3 arrays
  Int32 i;

  for (i=0; i < numSpecifiedDisks_; i++)
    {
      if (specifiedScratchDisks_[i].unpack(base, reallocator))
	return -1;
    }

  for (i=0; i < numExcludedDisks_; i++)
    {
      if (excludedScratchDisks_[i].unpack(base, reallocator))
	return -1;
    }

  for (i=0; i < numPreferredDisks_; i++)
    {
      if (preferredScratchDisks_[i].unpack(base, reallocator))
	return -1;
    }

  return NAVersionedObject::unpack(base, reallocator);
}

Lng32 ExScratchFileOptions::ipcPackedLength() const
{
  Lng32 result = 4 * sizeof(Int32); // the 4 length fields

  // add lengths of the ExScratchDiskDrive objects
  result +=
    (numSpecifiedDisks_+numExcludedDisks_+numPreferredDisks_) * 
    sizeof(ExScratchDiskDrive);

  // on top of that add the length of all of the disk names
  result += ipcGetTotalNameLength();

  return result;
}

Lng32 ExScratchFileOptions::ipcGetTotalNameLength() const
{
  // add lengths of the cluster and disk names
  // (including the NUL terminator for the names)
  Lng32 result = 0;
  Lng32 i;

  for (i=0; i < numSpecifiedDisks_; i++)
    {
      if (specifiedScratchDisks_[i].getClusterNameLength())
	result += specifiedScratchDisks_[i].getClusterNameLength() + 1;
      result += specifiedScratchDisks_[i].getDiskNameLength() + 1;
    }

  for (i=0; i < numExcludedDisks_; i++)
    {
      if (excludedScratchDisks_[i].getClusterNameLength())
	result += excludedScratchDisks_[i].getClusterNameLength() + 1;
      result += excludedScratchDisks_[i].getDiskNameLength() + 1;
    }

  for (i=0; i < numPreferredDisks_; i++)
    {
      if (preferredScratchDisks_[i].getClusterNameLength())
	result += preferredScratchDisks_[i].getClusterNameLength() + 1;
      result += preferredScratchDisks_[i].getDiskNameLength() + 1;
    }

  return result;
}

Lng32 ExScratchFileOptions::ipcPackObjIntoMessage(char *buffer) const
{
  char *currPtr = buffer;
  Lng32 i;
  Lng32 clen;
  Lng32 dlen;

  // pack only the length fields of myself
  str_cpy_all(currPtr,(char *) &numSpecifiedDisks_, sizeof(Int32));
  currPtr += sizeof(Int32);
  str_cpy_all(currPtr,(char *) &numExcludedDisks_, sizeof(Int32));
  currPtr += sizeof(Int32);
  str_cpy_all(currPtr,(char *) &numPreferredDisks_, sizeof(Int32));
  currPtr += sizeof(Int32);
  str_cpy_all(currPtr,(char *) &scratchFlags_, sizeof(UInt32));
  currPtr += sizeof(UInt32);

  // pack the ExScratchDiskDrives objects (again, ptrs stay unchanged)
  for (i=0; i < numSpecifiedDisks_; i++)
    {
      str_cpy_all(currPtr,
		  (const char *) &specifiedScratchDisks_[i],
		  sizeof(ExScratchDiskDrive));
      currPtr += sizeof(ExScratchDiskDrive);
    }

  for (i=0; i < numExcludedDisks_; i++)
    {
      str_cpy_all(currPtr,
		  (const char *) &excludedScratchDisks_[i],
		  sizeof(ExScratchDiskDrive));
      currPtr += sizeof(ExScratchDiskDrive);
    }

  for (i=0; i < numPreferredDisks_; i++)
    {
      str_cpy_all(currPtr,
		  (const char *) &preferredScratchDisks_[i],
		  sizeof(ExScratchDiskDrive));
      currPtr += sizeof(ExScratchDiskDrive);
    }

  // pack the cluster and disk names in sequence (unpack will take them out
  // in the same sequence)
  for (i=0; i < numSpecifiedDisks_; i++)
    {
      clen = specifiedScratchDisks_[i].getClusterNameLength();
      if (clen > 0)
	{
	  str_cpy_all(currPtr,
		      specifiedScratchDisks_[i].getClusterName(),
		      clen+1);
	  currPtr += clen+1;
	}
      dlen = specifiedScratchDisks_[i].getDiskNameLength();
      if (dlen > 0)
	{
	  str_cpy_all(currPtr,
		      specifiedScratchDisks_[i].getDiskName(),
		      dlen+1);
	  currPtr += dlen+1;
	}
    }

  for (i=0; i < numExcludedDisks_; i++)
    {
      clen = excludedScratchDisks_[i].getClusterNameLength();
      if (clen > 0)
	{
	  str_cpy_all(currPtr,
		      excludedScratchDisks_[i].getClusterName(),
		      clen+1);
	  currPtr += clen+1;
	}
      dlen = excludedScratchDisks_[i].getDiskNameLength();
      if (dlen > 0)
	{
	  str_cpy_all(currPtr,
		      excludedScratchDisks_[i].getDiskName(),
		      dlen+1);
	  currPtr += dlen+1;
	}
    }

  for (i=0; i < numPreferredDisks_; i++)
    {
      clen = preferredScratchDisks_[i].getClusterNameLength();
      if (clen > 0)
	{
	  str_cpy_all(currPtr,
		      preferredScratchDisks_[i].getClusterName(),
		      clen+1);
	  currPtr += clen+1;
	}
      dlen = preferredScratchDisks_[i].getDiskNameLength();
      if (dlen > 0)
	{
	  str_cpy_all(currPtr,
		      preferredScratchDisks_[i].getDiskName(),
		      dlen+1);
	  currPtr += dlen+1;
	}
    }

  return (currPtr - buffer);
}

#pragma nowarn(770)   // warning elimination
void ExScratchFileOptions::ipcUnpackObj(Lng32 objSize,
					const char *buffer,
					CollHeap *heap,
					Lng32 totalNameLength,
					char *&newBufferForDependents)
{
  const char *currPtr = buffer;
  Lng32 i;
  Lng32 clen;
  Lng32 dlen; 
  char *startBufferForDependents;

  // unpack the 3 length fields
  str_cpy_all((char *) &numSpecifiedDisks_, currPtr, sizeof(Int32));
  currPtr += sizeof(Int32);
  str_cpy_all((char *) &numExcludedDisks_, currPtr, sizeof(Int32));
  currPtr += sizeof(Int32);
  str_cpy_all((char *) &numPreferredDisks_, currPtr, sizeof(Int32));
  currPtr += sizeof(Int32);
  str_cpy_all((char *) &scratchFlags_, currPtr, sizeof(UInt32));
  currPtr += sizeof(UInt32);

  // allocate one common buffer for the arrays of ExScratchDiskDrive
  // structs and for all the names
  Lng32 bufferLen =
#pragma nowarn(1506)   // warning elimination 
    (numSpecifiedDisks_+numExcludedDisks_+numPreferredDisks_) *
    sizeof(ExScratchDiskDrive) + totalNameLength;
#pragma warn(1506)  // warning elimination 

  if (bufferLen == 0)
    {
      // no directives are given, return after some sanity checks
      assert(totalNameLength == 0);
      assert(currPtr-buffer == objSize);
      return;
    }
  else
    {
      // there are some directives and therefore there must be some names
      assert(totalNameLength > 0);
    }

  if (heap)
    newBufferForDependents = new(heap) char[bufferLen];
  else
    newBufferForDependents = new char[bufferLen];
  startBufferForDependents = newBufferForDependents;

  // allocate the arrays of ExScratchDiskDrive structs
  
  if (numSpecifiedDisks_)
    {
      specifiedScratchDisks_ = newBufferForDependents;
      newBufferForDependents +=
	numSpecifiedDisks_ * sizeof(ExScratchDiskDrive);
    }

  if (numExcludedDisks_)
    {
      excludedScratchDisks_ = newBufferForDependents;
      newBufferForDependents +=
	numExcludedDisks_ * sizeof(ExScratchDiskDrive);
    }

  if (numPreferredDisks_)
    {
      preferredScratchDisks_ = newBufferForDependents;
      newBufferForDependents +=
	numPreferredDisks_ * sizeof(ExScratchDiskDrive);
    }

  // unpack the ExScratchDiskDrives objects (again, ptrs stay unchanged)
  for (i=0; i < numSpecifiedDisks_; i++)
    {
      str_cpy_all((char *) &specifiedScratchDisks_[i],
		  currPtr,
		  sizeof(ExScratchDiskDrive));
      currPtr += sizeof(ExScratchDiskDrive);
    }

  for (i=0; i < numExcludedDisks_; i++)
    {
      str_cpy_all((char *) &excludedScratchDisks_[i],
		  currPtr,
		  sizeof(ExScratchDiskDrive));
      currPtr += sizeof(ExScratchDiskDrive);
    }

  for (i=0; i < numPreferredDisks_; i++)
    {
      str_cpy_all((char *) &preferredScratchDisks_[i],
		  currPtr,
		  sizeof(ExScratchDiskDrive));
      currPtr += sizeof(ExScratchDiskDrive);
    }

  // unpack the disk names in sequence: first copy them all into
  // a new array, then fix up the pointers
  str_cpy_all(newBufferForDependents,currPtr,totalNameLength);
  currPtr += totalNameLength;

  for (i=0; i < numSpecifiedDisks_; i++)
    {
      clen = specifiedScratchDisks_[i].getClusterNameLength();
      if (clen)
	{
	  specifiedScratchDisks_[i].setClusterName(newBufferForDependents);
	  newBufferForDependents += clen+1;
	}
      dlen = specifiedScratchDisks_[i].getDiskNameLength();
      assert(dlen);
      specifiedScratchDisks_[i].setDiskName(newBufferForDependents);
      newBufferForDependents += dlen+1;
    }

  for (i=0; i < numExcludedDisks_; i++)
    {
      clen = excludedScratchDisks_[i].getClusterNameLength();
      if (clen)
	{
	  excludedScratchDisks_[i].setClusterName(newBufferForDependents);
	  newBufferForDependents += clen+1;
	}
      dlen = excludedScratchDisks_[i].getDiskNameLength();
      assert(dlen);
      excludedScratchDisks_[i].setDiskName(newBufferForDependents);
      newBufferForDependents += dlen+1;
    }

  for (i=0; i < numPreferredDisks_; i++)
    {
      clen = preferredScratchDisks_[i].getClusterNameLength();
      if (clen)
	{
	  preferredScratchDisks_[i].setClusterName(newBufferForDependents);
	  newBufferForDependents += clen+1;
	}
      dlen = preferredScratchDisks_[i].getDiskNameLength();
      assert(dlen);
      preferredScratchDisks_[i].setDiskName(newBufferForDependents);
      newBufferForDependents += dlen+1;
    }
  assert(currPtr-buffer == objSize);
}
#pragma warn(770)  // warning elimination 

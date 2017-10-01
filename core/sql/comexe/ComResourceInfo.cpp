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
  return dirName_.pack(space);
}

Lng32 ExScratchDiskDrive::unpack(void * base, void * reallocator)
{
  return dirName_.unpack(base);
}

Long ExScratchFileOptions::pack(void * space)
{
  // pack the contents of the array
  Int32 i;
 
  for (i=0; i < numSpecifiedDirs_; i++)
    {
      specifiedScratchDirs_[i].pack(space);
    }
  // convert the pointers to the array to offsets
  specifiedScratchDirs_.packShallow(space);
  
  return NAVersionedObject::pack(space);
}

Lng32 ExScratchFileOptions::unpack(void * base, void * reallocator)
{
  // convert the offsets to the  array to pointers
  if (specifiedScratchDirs_.unpackShallow(base))
    return -1;
 
  
  // pack the contents of the array
  Int32 i;

  for (i=0; i < numSpecifiedDirs_; i++)
    {
      if (specifiedScratchDirs_[i].unpack(base, reallocator))
	return -1;
    }

  return NAVersionedObject::unpack(base, reallocator);
}

Lng32 ExScratchFileOptions::ipcPackedLength() const
{
   Lng32 result = 2 * sizeof(Int32); // the 2 length fields

  // add lengths of the ExScratchDiskDrive object
  result +=
    (numSpecifiedDirs_) * 
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

  for (i=0; i < numSpecifiedDirs_; i++)
    {
      result += specifiedScratchDirs_[i].getDirNameLength() + 1;
    }


  return result;
}

Lng32 ExScratchFileOptions::ipcPackObjIntoMessage(char *buffer) const
{
  char *currPtr = buffer;
  Lng32 i;
 
  Lng32 dlen;

  // pack only the length fields of myself
  str_cpy_all(currPtr,(char *) &numSpecifiedDirs_, sizeof(Int32));
  currPtr += sizeof(Int32);
  str_cpy_all(currPtr,(char *) &scratchFlags_, sizeof(UInt32));
  currPtr += sizeof(UInt32);

  // pack the ExScratchDiskDrives objects (again, ptrs stay unchanged)
  for (i=0; i < numSpecifiedDirs_; i++)
    {
      str_cpy_all(currPtr,
		  (const char *) &specifiedScratchDirs_[i],
		  sizeof(ExScratchDiskDrive));
      currPtr += sizeof(ExScratchDiskDrive);
    }

 
  // pack the cluster and disk names in sequence (unpack will take them out
  // in the same sequence)
  for (i=0; i < numSpecifiedDirs_; i++)
    {
      dlen = specifiedScratchDirs_[i].getDirNameLength();
      if (dlen > 0)
	{
	  str_cpy_all(currPtr,
		      specifiedScratchDirs_[i].getDirName(),
		      dlen+1);
	  currPtr += dlen+1;
	}
    }
  return (currPtr - buffer);
}

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
  str_cpy_all((char *) &numSpecifiedDirs_, currPtr, sizeof(Int32));
  currPtr += sizeof(Int32);
 
  str_cpy_all((char *) &scratchFlags_, currPtr, sizeof(UInt32));
  currPtr += sizeof(UInt32);

  // allocate one common buffer for the arrays of ExScratchDiskDrive
  // structs and for all the names
  Lng32 bufferLen =
    (numSpecifiedDirs_) *
    sizeof(ExScratchDiskDrive) + totalNameLength;

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
  
  if (numSpecifiedDirs_)
    {
      specifiedScratchDirs_ = newBufferForDependents;
      newBufferForDependents +=
	numSpecifiedDirs_ * sizeof(ExScratchDiskDrive);
    }

  // unpack the ExScratchDiskDrives objects (again, ptrs stay unchanged)
  for (i=0; i < numSpecifiedDirs_; i++)
    {
      str_cpy_all((char *) &specifiedScratchDirs_[i],
		  currPtr,
		  sizeof(ExScratchDiskDrive));
      currPtr += sizeof(ExScratchDiskDrive);
    }


  // unpack the disk names in sequence: first copy them all into
  // a new array, then fix up the pointers
  str_cpy_all(newBufferForDependents,currPtr,totalNameLength);
  currPtr += totalNameLength;

  for (i=0; i < numSpecifiedDirs_; i++)
    {
      dlen = specifiedScratchDirs_[i].getDirNameLength();
      assert(dlen);
      specifiedScratchDirs_[i].setDirName(newBufferForDependents);
      newBufferForDependents += dlen+1;
    }

  assert(currPtr-buffer == objSize);
}

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
* File:         IpcMessageObj.cpp (previously under /common)
* Description:
*
* Created:      5/6/98
* Language:     C++
*
*
*
****************************************************************************
*/

#include "Platform.h"


#include "NAStdlib.h"
#include "Ipc.h"

#include "IpcMessageObj.h"
#include "str.h"
#include "ComDefs.h"  // to get ROUND8
#include <byteswap.h>
// -----------------------------------------------------------------------
// Methods for class IpcMessageObj
// -----------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// general constructor, creates both unpacked, packed objects 
IpcMessageObj::IpcMessageObj(IpcMessageObjType msgType,
			     IpcMessageObjVersion version)
  {
  s_.objType_ = (short) msgType;
  s_.objVersion_ = version;
  s_.refCount_ = 1; // an object comes with an initial refcount of 1
  s_.objLength_ = 0;
  s_.next_ = NULL;
  s_.endianness_  = IpcMyEndianness;
  s_.spare1_ = 0;
  s_.spare2_ = 0;
  s_.vPtrPad_ = NULL;
  }

///////////////////////////////////////////////////////////////////////////////  
// constructor used to perform copyless receive, maps packed objects in place.
IpcMessageObj::IpcMessageObj(IpcBufferedMsgStream* msgStream)
  { 
  // IpcBufferedMsgStream parm used to differentiate from default constructor
  if (s_.endianness_ != IpcMyEndianness)
    {
    swapFourBytes(s_.objType_);
    swapFourBytes(s_.objVersion_);
    swapFourBytes(s_.objLength_);
    assert(0); // didn't implement
    s_.refCount_ = 1; // an object comes with an initial refcount of 1
    s_.endianness_ = IpcMyEndianness;
    }
  }

IpcMessageObj::~IpcMessageObj()
{
  if (s_.refCount_ != 1)
    {
      // something went wrong!!!!!!!!!!!!!!!
      //
      // Take the two cases:
      // a) the variable is allocated in static memory or in the stack
      // b) the variable was allocated from the heap, using operator new
      //
      // For case a), the user isn't supposed to perform any increments
      // or decrements of the ref count. One possibility is that he did.
      // The other possibility is that there are still outstanding I/Os
      // or other processes that share the object and therefore we
      // shouldn't destroy it.
      //
      // For case b) it is similar. Either the user hasn't used increment
      // and decrement refcount and then he can delete the object or
      // get rid of it by decrementing the refcount (preferred method).
      // Again, either the user forgot to decrement the refcount or
      // the object is still shared by other components.

      // Complain about the abuse
      ABORT("trying to destroy a message object that is still in use");
    }
}

///////////////////////////////////////////////////////////////////////////////
// used to allocate a packed send object.
void* IpcMessageObj::operator new(size_t size,
                                  IpcBufferedMsgStream& msgStream,
                                  IpcMessageObjSize appendDataLen)
{
  IpcMessageObjSize appendStart = (IpcMessageObjSize) size;
  
  alignSizeForNextObj(appendStart);
  return (msgStream.sendMsgObj(appendStart + appendDataLen));
}

IpcMessageObjSize IpcMessageObj::packObjIntoMessage(
               IpcMessageBufferPtr buffer)
{
  return packObjIntoMessage(buffer, FALSE);
}

IpcMessageObjSize IpcMessageObj::packObjIntoMessage(IpcMessageBufferPtr buffer,
                            NABoolean swapBytes)
{
  // copy the base class (IpcMessageObj) data members and calculate how
  // many more bytes beyond the base class remain to be copied
  IpcMessageObjSize derivedObjLen =
    packedLength() - packBaseClassIntoMessage(buffer, swapBytes);

  // ---------------------------------------------------------------------
  // copy derivedObjLen bytes to the message buffer, assuming that
  // the packed and unpacked representations of the object are the same,
  // that whoever derived from this class did correctly override
  // the packedLength() virtual function, and that the derived data is
  // stored directly following the base class data (this may not be the
  // case if multiple inheritance is used!!!!).
  // ---------------------------------------------------------------------
#pragma nowarn(1506)   // warning elimination 
  str_cpy_all(buffer, (const char *) &this[1], derivedObjLen);
#pragma warn(1506)  // warning elimination 

  return derivedObjLen + sizeof(IpcMessageObj);
}

IpcMessageObjSize IpcMessageObj::packObjIntoMessage32(
               IpcMessageBufferPtr buffer)
{
  return packObjIntoMessage32(buffer, FALSE);
}

IpcMessageObjSize IpcMessageObj::packObjIntoMessage32(IpcMessageBufferPtr buffer,
                            NABoolean swapBytes)
{
  // copy the base class (IpcMessageObj) data members and calculate how
  // many more bytes beyond the base class remain to be copied
  IpcMessageObjSize derivedObjLen =
    packedLength32() - packBaseClassIntoMessage32(buffer, swapBytes);

  // ---------------------------------------------------------------------
  // copy derivedObjLen bytes to the message buffer, assuming that
  // the packed and unpacked representations of the object are the same,
  // that whoever derived from this class did correctly override
  // the packedLength() virtual function, and that the derived data is
  // stored directly following the base class data (this may not be the
  // case if multiple inheritance is used!!!!).
  // ---------------------------------------------------------------------
#pragma nowarn(1506)   // warning elimination 
  str_cpy_all(buffer, (const char *) &this[1], derivedObjLen);
#pragma warn(1506)  // warning elimination 

  return derivedObjLen + sizeof(IpcMessageObj);
}

void IpcMessageObj::unpackObj(IpcMessageObjType /*objType*/,
			      IpcMessageObjVersion /*objVersion*/,
			      NABoolean /*sameEndianness*/,
			      IpcMessageObjSize objSize,
			      IpcConstMessageBufferPtr buffer)
{
  assert(objSize >= sizeof(IpcMessageObj));

  // unpack the first sizeof(IpcMessageObj) bytes and fixup the virtual
  // function pointer of the base class
  unpackBaseClass(buffer);

  // copy the rest into the memory following the IpcMessageObj data members
  // (should we allow such a dangerous default implementation?)
#pragma nowarn(1506)   // warning elimination 
  str_cpy_all((char *) &this[1], buffer, objSize - sizeof(IpcMessageObj));
#pragma warn(1506)  // warning elimination 
  // - Change the offset in the next ptr to zero
  //
  s_.next_ = NULL;
}

void IpcMessageObj::unpackObj32(IpcMessageObjType /*objType*/,
			        IpcMessageObjVersion /*objVersion*/,
			        NABoolean /*sameEndianness*/,
			        IpcMessageObjSize objSize,
			        IpcConstMessageBufferPtr buffer)
{
  assert(objSize >= SQL_32BIT_IPC_MESSAGE_OBJ_SIZE);

  // unpack the first sizeof(IpcMessageObj) bytes and fixup the virtual
  // function pointer of the base class
  unpackBaseClass32(buffer);

  // copy the rest into the memory following the IpcMessageObj data members
  // (should we allow such a dangerous default implementation?)
#pragma nowarn(1506)   // warning elimination 
  str_cpy_all((char *) &this[1], buffer, objSize - SQL_32BIT_IPC_MESSAGE_OBJ_SIZE);
#pragma warn(1506)  // warning elimination 
  // - Change the offset in the next ptr to zero
  //
  s_.next_ = NULL;
}

NABoolean IpcMessageObj::checkObj(IpcMessageObjType t,
                                  IpcMessageObjVersion v,
                                  NABoolean sameEndianness,
                                  IpcMessageObjSize size,
                                  IpcConstMessageBufferPtr buffer) const
{
  return checkBaseClass(t, v, sameEndianness, size, buffer);
}

NABoolean IpcMessageObj::checkBaseClass(IpcMessageObjType t,
                                        IpcMessageObjVersion v,
                                        NABoolean sameEndianness,
                                        IpcMessageObjSize size,
                                        IpcConstMessageBufferPtr &buffer) const
{
  NABoolean result = TRUE;
  const IpcConstMessageBufferPtr lastByte = buffer + size - 1;
  if (!checkBuffer(buffer, sizeof(IpcMessageObj), lastByte) ||
      s_.objType_ != t ||
      s_.objVersion_ != v)
  {
    result = FALSE;
    ipcIntegrityCheckEpilogue(result);
  }
  return result;
}

IpcMessageRefCount IpcMessageObj::incrRefCount()
{
  return ++s_.refCount_;
}

IpcMessageRefCount IpcMessageObj::decrRefCount()
{
  // delete this object if the result falls down to 0
  // NOTE: users who allocate an IpcMessageObj on the stack should
  // never call incrRefCount() or decrRefCount() on that object.
  if (s_.refCount_ == 1)
    {
      // don't decrement the refcount; the destructor performs a
      // consistency check that an object can only be destroyed
      // when it has a refcount of 1
      delete this;
      return 0;
    }
  else
    {
      assert(s_.refCount_ > 1);
      return --s_.refCount_;
    }
}
IpcMessageObjSize IpcMessageObj::packBaseClassIntoMessage(
     IpcMessageBufferPtr &buffer)
{
   return packBaseClassIntoMessage(buffer, FALSE);
}

IpcMessageObjSize IpcMessageObj::packBaseClassIntoMessage(
     IpcMessageBufferPtr &buffer, NABoolean swapBytes)
{
  // the default implementation performs a byte-wise copy
  IpcMessageObjSize copyLen = sizeof(IpcMessageObj);
  char *savedVPtr = getMyVPtr();

  assert(copyLen == 48 AND
	 sizeof(s_) == 40 AND
	 (char *) this == ((char *) &s_ - sizeof(char *)));

  // wipe out the virtual function pointer before moving data
  // (makes sure that the copied object doesn't have a stray pointer in it)
  setMyVPtr(NULL);

  // ---------------------------------------------------------------------
  // Copy packedLength() bytes to the message buffer, assuming that
  // the packed and unpacked representations of the object are the same
  // and that whoever derived from this class did correctly override
  // the packedLength() virtual function. No copy necessary if this
  // is packing in-place.
  // ---------------------------------------------------------------------
  if (buffer == (IpcMessageBufferPtr) this)
    {
      // now that we're sending we won't allow anybody else to reference
      // the object in the message buffer
      assert(s_.refCount_ == 1);
    }
  else
    {
      // save the current refcount and set it to 1, since the copied object
      // is in a new space and therefore should have a refcount of 1
      IpcMessageRefCount saveRefCount = s_.refCount_;
      IpcMessageObjVersion saveObjVersion = 0;
      IpcMessageObjType    saveObjType = IPC_MSG_SQLCAT_FIRST;
      IpcMessageObjSize    saveObjLength = 0;
      char                 saveEndianness = 0;
      s_.refCount_ = 1;
      if (swapBytes)
      {
         saveObjType = s_.objType_;
         saveObjVersion = s_.objVersion_;
         saveEndianness = s_.endianness_;
         saveObjLength = s_.objLength_;
         
         s_.objType_ = bswap_32(s_.objType_); 
         s_.objVersion_ = bswap_32(s_.objVersion_);
         // Switch the Endian since we are swapping the bytes
#ifdef NA_LITTLE_ENDIAN
         s_.endianness_ = IpcBigEndian;
#else
         s_.endianness_ = IpcLittleEndian;
#endif
         s_.objLength_ = bswap_32(s_.objLength_); 
         s_.refCount_ = bswap_32(s_.refCount_);
      }
#pragma nowarn(1506)   // warning elimination 
      str_cpy_all(buffer,(const char *)this,copyLen);
#pragma warn(1506)  // warning elimination 

      if (swapBytes)
      {
         s_.objType_ = saveObjType;
         s_.objVersion_ = saveObjVersion;
         s_.endianness_ = saveEndianness;
         s_.objLength_ = saveObjLength;
      }
      // restore virtual function pointer and refCount_ for the in-memory copy
      s_.refCount_ = saveRefCount;
      setMyVPtr(savedVPtr);
    }

  buffer += copyLen;
  return copyLen;
}

// pack the base class to be read by 32-bit BDR client
IpcMessageObjSize IpcMessageObj::packBaseClassIntoMessage32(
     IpcMessageBufferPtr &buffer, NABoolean swapBytes)
{
  // a hard-coded 32-bit IpcMessageObj class size
  IpcMessageObjSize copyLen = SQL_32BIT_IPC_MESSAGE_OBJ_SIZE;

  // cleanup buffer first
  memset(buffer, 0, copyLen);

  // ---------------------------------------------------------------------
  // a 32 byte layout of this object:
  //
  //    +---------------------------------+     \
  //  0 | _vptr (NT) or padding (NSK)     |      |
  //    +---------------------------------+      |
  //  4 | objType_  _  (copy)             |      |
  //    +---------------------------------+      |
  //  8 | objVersion_  (copy)             |      |
  //    +---------------------------------+      |
  // 12 | refCount_    (copy)             |      |
  //    +---------------------------------+      |
  // 16 | objLength_   (copy)             |      >  IpcMessageObjStruct
  //    +---------------------------------+      |
  // 20 | next_                           |      |
  //    +---------------------------------+      |
  // 24 | endianness_  (copy)             |      |
  //    +---------------------------------+      |
  // 25 | spare1_                         |      |
  //    +---------------------------------+      |
  // 26 | spare2_                         |      |
  //    +---------------------------------+      |
  // 28 | _vptr (NSK) or padding (NT)     |      |
  //    +---------------------------------+     /
  //
  // need to copy only marked fields
  // ---------------------------------------------------------------------

  if (buffer == (IpcMessageBufferPtr) this)
    {
      // now that we're sending we won't allow anybody else to reference
      // the object in the message buffer
      assert(s_.refCount_ == 1);
    }
  else
    {
      // set refcount to 1, since the copied object
      // is in a new space and therefore should have a refcount of 1
      IpcMessageRefCount   tempRefCount = 1;
      IpcMessageObjVersion tempObjVersion = s_.objVersion_;
      IpcMessageObjType    tempObjType = s_.objType_;
      IpcMessageObjSize    tempObjLength = s_.objLength_;;
      char                 tempEndianness = s_.endianness_;

      if (swapBytes)
      {
         tempObjType = bswap_32(tempObjType); 
         tempObjVersion = bswap_32(tempObjVersion);
         // Switch the Endian since we are swapping the bytes
#ifdef NA_LITTLE_ENDIAN
         tempEndianness = IpcBigEndian;
#else
         tempEndianness = IpcLittleEndian;
#endif
         tempObjLength = bswap_32(tempObjLength); 
         tempRefCount = bswap_32(tempRefCount);
      }
      // move only needed fields, see above
      memcpy((char*)buffer + 4, (char*)&tempObjType, 4);
      memcpy((char*)buffer + 8, (char*)&tempObjVersion, 4);
      memcpy((char*)buffer +12, (char*)&tempRefCount, 4);
      memcpy((char*)buffer +16, (char*)&tempObjLength, 4);
      memcpy((char*)buffer +24, (char*)&tempEndianness, 1);
    }

  buffer += copyLen;
  return copyLen;
}

void IpcMessageObj::alignBufferForNextObj(IpcConstMessageBufferPtr &buffer)
{
  buffer = (IpcConstMessageBufferPtr) ROUND8((Long)buffer);
}

void IpcMessageObj::alignBufferForNextObj(IpcMessageBufferPtr &buffer)
{
  buffer = (IpcMessageBufferPtr) ROUND8((Long)buffer);
}

void IpcMessageObj::alignSizeForNextObj(IpcMessageObjSize &size)
{
  ULng32 s = (ULng32) size; // just for safety

  // clear the last 3 bits of the size to round it down to
  // the next size that is divisible by 8
  ULng32 roundedDown = s LAND 0xFFFFFFF8;

  // if that didn't change anything we're done, the size was
  // a multiple of 8 already
  if (s != roundedDown)
    {
      // else we have to round up and add the filler
      size = (IpcMessageObjSize) roundedDown + 8;
    }
}

void IpcMessageObj::unpackBaseClass(IpcConstMessageBufferPtr &buffer)
{
  // save the virtual function pointer (it is already set to the correct value)
  char *savedVPtr = getMyVPtr();
  
  // copy objSize bytes into the memory that this object occupies
  // this header (should we allow such a dangerous default implementation?)
  str_cpy_all((char *) this,buffer,sizeof(IpcMessageObj));

  // restore the previously saved virtual function pointer, since it got
  // overwritten with a NULL value by the copy operation above
  // NOTE: this of course assumes that derived classes don't need their
  // own virtual function pointers!!!
  setMyVPtr(savedVPtr);
  // - Change the offset in the next ptr to zero
  //
  s_.next_ = NULL;
  buffer += sizeof(IpcMessageObj);
}

// unpack base class got from 32-bit server
void IpcMessageObj::unpackBaseClass32(IpcConstMessageBufferPtr &buffer)
{
  // copy only selected fields from the buffer
  memcpy((char*)&(s_.objType_), (char*)buffer + 4, 4);
  memcpy((char*)&(s_.objVersion_), (char*)buffer + 8, 4);
  memcpy((char*)&(s_.refCount_), (char*)buffer +12, 4);
  memcpy((char*)&(s_.objLength_), (char*)buffer +16, 4);
  memcpy((char*)&(s_.endianness_), (char*)buffer +24, 1);
  
  s_.next_ = NULL;
  buffer += SQL_32BIT_IPC_MESSAGE_OBJ_SIZE;
}

IpcMessageObjSize IpcMessageObj::packDependentObjIntoMessage(
     IpcMessageBufferPtr buffer)
{
  return packDependentObjIntoMessage(buffer, FALSE);
}

IpcMessageObjSize IpcMessageObj::packDependentObjIntoMessage(
     IpcMessageBufferPtr buffer, NABoolean swapBytes)
{
  IpcMessageBufferPtr alignedBuffer = buffer;
  alignBufferForNextObj(alignedBuffer);

  // pack the object and compute the length
  IpcMessageObjSize result = packObjIntoMessage(alignedBuffer, swapBytes);

  // finish the object that is now inside the buffer, similarly to what
  // IpcMessageStream::send() does
  IpcMessageObj *bufObj = (IpcMessageObj *) alignedBuffer;

  bufObj->setMyVPtr(NULL);
  bufObj->s_.objLength_ = result;
  bufObj->s_.refCount_ = 1;
  bufObj->s_.next_ = NULL;
  
  if (swapBytes)
  {
     bufObj->s_.objLength_ = bswap_32(bufObj->s_.objLength_);
     bufObj->s_.refCount_ = bswap_32(bufObj->s_.refCount_);
  }

  // add the filler space that had to be added to the returned result
  result += (alignedBuffer - buffer);

  return result;
}

IpcMessageObjSize IpcMessageObj::packDependentObjIntoMessage32(
     IpcMessageBufferPtr buffer)
{
  return packDependentObjIntoMessage32(buffer, FALSE);
}

IpcMessageObjSize IpcMessageObj::packDependentObjIntoMessage32(
     IpcMessageBufferPtr buffer, NABoolean swapBytes)
{
  IpcMessageBufferPtr alignedBuffer = buffer;
  alignBufferForNextObj(alignedBuffer);

  // pack the object and compute the length
  IpcMessageObjSize result = packObjIntoMessage32(alignedBuffer, swapBytes);
  IpcMessageObjSize sResult = result; // swapped result if does

  Int32 refCount = 1;

  Int32 firstField = 0; // assume Linux where vPtrPad_ is at the end
  if (swapBytes)
  {
    // send to NSK where vPtrPad_ is at the beginning of the structure.
    firstField = 4;
    refCount = bswap_32(refCount);
    sResult = bswap_32(sResult);
  }

  // finish the object that is now inside the buffer by move only needed fields
  memset((char*)alignedBuffer, 0, 4);                     // vPtrPad_
  memcpy((char*)alignedBuffer + 12, (char*)&refCount, 4); // refCount_
  memcpy((char*)alignedBuffer + 16, (char*)&sResult, 4);  // objLength_
  memset((char*)alignedBuffer + 20, 0, 4);                // next_
  memset((char*)alignedBuffer + 28, 0, 4);                // vPtrPad_

  // add the filler space that had to be added to the returned result
  result += (alignedBuffer - buffer);

  return result;
}

void IpcMessageObj::unpackDependentObjFromBuffer(
     IpcConstMessageBufferPtr &buffer,
     NABoolean sameEndianness)
{
  alignBufferForNextObj(buffer);

  IpcMessageObj       *objInBuffer = (IpcMessageObj       *) buffer;
  IpcMessageObjSize   objLength    = objInBuffer->s_.objLength_;

  unpackObj(objInBuffer->s_.objType_,
	    objInBuffer->s_.objVersion_,
	    sameEndianness,
	    objLength,
	    buffer);

  buffer += objLength;
}

void IpcMessageObj::unpackDependentObjFromBuffer32(
     IpcConstMessageBufferPtr &buffer,
     NABoolean sameEndianness)
{
  alignBufferForNextObj(buffer);

  // NEEDS PORT
  //   The problem here is that IpcMessageObjStruct does not contain
  //   the 4 bytes allocated for the virtual function table pointer and
  //   on NT the vftptr is BEFORE the structure and on unix/NSK it is after.
  IpcMessageObjType    objType    = *(Int32 *)((char *)buffer + 4);  // objType_
  IpcMessageObjVersion objVersion = *(Int32 *)((char *)buffer + 8);  // objVersion_
  IpcMessageObjSize    objLength  = *(Int32 *)((char *)buffer + 16); // objLength_
   

  unpackObj32(objType,
	      objVersion,
	      sameEndianness,
	      objLength,
	      buffer);
  buffer += objLength;

}

NABoolean IpcMessageObj::checkDependentObj(IpcConstMessageBufferPtr &buffer,
                                           NABoolean sameEndianness) const
{
  alignBufferForNextObj(buffer);
  IpcMessageObj *objInBuffer = (IpcMessageObj *) buffer;
  IpcMessageObjSize objLength = objInBuffer->s_.objLength_;

  if (!checkObj(objInBuffer->s_.objType_,
                objInBuffer->s_.objVersion_,
                sameEndianness,
                objLength,
                buffer))
  {
    return FALSE;
  }

  buffer += objLength;
  return TRUE;
}

void IpcMessageObj::turnByteOrder()
{
	// NEEDS PORT (2/26/97)
	//  the code to handle endianness is not quite right. as a temporary
	//  measure for the demo, turn this code off.
	return;
}

///////////////////////////////////////////////////////////////////////////////
// merge next packed message object in IpcMessageBuffer with this object
void IpcMessageObj::mergeNextPackedObj()
  {
  IpcMessageObj* nextObj = getNextFromOffset();
  if (nextObj)
    {
    if (nextObj->s_.next_)
      {
      IpcMessageObj * offset = (IpcMessageObj*) (
                               (Long)(s_.next_) + (char *)(nextObj->s_.next_));
      s_.next_ = (IpcMessageObj*)offset;
      }
    else
      s_.next_ = NULL;
    }
  return;
  }

IpcMessageObj *IpcMessageObj::getNextFromOffset()
{
  if (s_.next_ == NULL)
    return NULL;
  else
    // add the "this" pointer to the offset
    return (IpcMessageObj *) ((char*) this + (Long) s_.next_);
}

void IpcMessageObj::convertNextToOffset()
{
  // subtract the "this" pointer from the address to make it an offset
  if (s_.next_ != NULL)
    s_.next_ = (IpcMessageObj *) ((char*) s_.next_ - (char*) this);
}

IpcMessageObjSize packCharStarIntoBuffer(IpcMessageBufferPtr &buffer,
                                 char* strPtr, NABoolean swapBytes)
{
  Lng32 length;

  if (strPtr==NULL)
     length=0;
  else
     length=str_len(strPtr)+1; // 1 is for the null-terminator char
   // NOT a recursive call.
  IpcMessageObjSize result = packIntoBuffer(buffer,length, swapBytes);         

  if (strPtr!=NULL)
     str_cpy_all((char*)buffer,strPtr,length);
  buffer += length;
  return result+length; 
}

// UR2 
IpcMessageObjSize packCharStarIntoBuffer(IpcMessageBufferPtr &buffer, 
                                 NAWchar* strPtr, NABoolean swapBytes)
{
  Lng32 length;

  if (strPtr==NULL)
     length=0;
  else
#pragma nowarn(1506)   // warning elimination 
     length=na_wcslen(strPtr)+1; // 1 is for the null-terminator char
#pragma warn(1506)  // warning elimination 
  length *= sizeof(NAWchar);

   // NOT a recursive call.
  IpcMessageObjSize result = packIntoBuffer(buffer,length, swapBytes);         

  if (strPtr!=NULL)
     na_wcscpy((NAWchar*)buffer,strPtr);
  buffer += length;
  return result+length; 
}

void unpackBuffer(IpcConstMessageBufferPtr &buffer, 
                  char* &strPtr, 
                  CollHeap* collHeapPtr)
{
  if (collHeapPtr != NULL)
    collHeapPtr->deallocateMemory(strPtr);
  else
    delete strPtr;

  Lng32 length;
   // NOT a recursive call.
  unpackBuffer(buffer,length);  // already has +1 for null terminating char
  if (length==0)
     strPtr = NULL;
  else {
     if (collHeapPtr != NULL) {
        strPtr = (char*) (collHeapPtr->allocateMemory(length)); 
        assert(strPtr!=NULL);
     }
     else {
        strPtr = new char[length];  
        assert(strPtr!=NULL);
     }

     str_cpy_all(strPtr,(char*) buffer, length);
     buffer += length;
  }
}

void skipCharStarInBuffer(IpcConstMessageBufferPtr &buffer)
{
  Lng32 length;
  // NOT a recursive call.
  unpackBuffer(buffer,length);  // already has +1 for null terminating char
  buffer += length;
}

// Verify that the next field in a message buffer does not extend
// beyond lastByte.
NABoolean checkBuffer (
    /* INOUT */ IpcConstMessageBufferPtr &buffer,
    /* IN    */ ULng32 dataLength,
    /* IN    */ IpcConstMessageBufferPtr lastByte )
{
  NABoolean result = TRUE;
  IpcConstMessageBufferPtr endOfData = buffer + dataLength - 1;
  if (endOfData > lastByte)
  {
    result = FALSE;
    ipcIntegrityCheckEpilogue(result);
  }
  else
  {
    buffer += dataLength;
  }
  return result;
}

// Verify that the next field in a message buffer does not extend
// beyond lastByte, and if it does then copy the field into dataPtr.
NABoolean checkAndUnpackBuffer (
    /* INOUT */ IpcConstMessageBufferPtr &buffer,
    /* IN    */ ULng32 dataLength,
    /* OUT   */ char *dataPtr,
    /* IN    */ IpcConstMessageBufferPtr lastByte )
{
  NABoolean result = TRUE;
  IpcConstMessageBufferPtr endOfData = buffer + dataLength - 1;
  if (endOfData > lastByte)
  {
    result = FALSE;
    ipcIntegrityCheckEpilogue(result);
  }
  else
  {
#pragma nowarn(1506)   // warning elimination 
    str_cpy_all(dataPtr, (char *) buffer, dataLength);
#pragma warn(1506)  // warning elimination 
    buffer += dataLength;
  }
  return result;
}

// Verify that the next string field in a message buffer does not
// extend beyond lastByte. Note that the packed format for a string
// includes a 4-byte length field as a prefix.
NABoolean checkCharStarInBuffer (
    /* INOUT */ IpcConstMessageBufferPtr &buffer,
    /* IN    */ NABoolean sameEndianness,
    /* IN    */ IpcConstMessageBufferPtr lastByte )
{
  NABoolean result = TRUE;
  Int32  dataLength = 0;
  if (!checkAndUnpackBuffer(buffer, sizeof(dataLength),
                            (char *) &dataLength, lastByte))
  {
    result = FALSE;
    ipcIntegrityCheckEpilogue(result);
  }
  else
  {
    if (!sameEndianness)
    {
      swapFourBytes(dataLength);
    }
    if (!checkBuffer(buffer, dataLength, lastByte))
    {
      result = FALSE;
      ipcIntegrityCheckEpilogue(result);
    }
  }
  return result;
}

// IpcMessageObj::checkObj() implementations all call this function
// immediately when they detect a failure. Setting a breakpoint here
// can help you quickly track down the cause of a checkObj()
// failure. Right now this function is just a wrapper around
// NAError_stub_for_breakpoints() but in the future we may decide it
// should also do some bookeeping or error reporting of its own.
void ipcIntegrityCheckEpilogue(NABoolean status)
{
  if (!status)
  {
    NAError_stub_for_breakpoints();
  }
}

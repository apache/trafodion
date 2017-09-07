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
* File:         IpcMessageObj.h (previously under /common)
* Description:  IpcMessageObj is the base class for objects that need to
*		be sent across an IPC interface (see file common/Ipc.h).
*		
* Created:      5/6/98
* Language:     C++
*
*
*
****************************************************************************
*/

#ifndef IPCMESSAGEOBJ_H
#define IPCMESSAGEOBJ_H

#include "Platform.h"
#include "str.h"


#include "IpcMessageType.h"
#include "CollHeap.h"

#include <byteswap.h>
// -----------------------------------------------------------------------
// Contents of this file
// -----------------------------------------------------------------------

class IpcMessageObj;

// -----------------------------------------------------------------------
// Forward references
// -----------------------------------------------------------------------

class IpcMessageStream;
class IpcBufferedMsgStream;
class IpcMessageBuffer;

// size of 32-bit IpcMessageObj base class
#define SQL_32BIT_IPC_MESSAGE_OBJ_SIZE 32

// -----------------------------------------------------------------------
// IpcMessageObj is the base class of all objects that are sent in
// messages. This class contains fields to identify the object in
// the message and to determine its length and version. It also
// contains some fields that are used internally.
// IpcMessageObj has some virtual methods that allow a user of this
// class to redefine the methods to pack and unpack data into and from
// the message. A virtual method can also be used to determine the
// length of the message when it gets packed into the message.
// -----------------------------------------------------------------------
class IpcMessageObj
{
  friend class IpcMessageStream;
  friend class IpcBufferedMsgStream;
  friend class SockSocket; // to access message header information

  // A friend function to enable internal integrity checks in
  // IpcMessageBuffer objects.
  friend NABoolean verifyIpcMessageBufferBackbone(IpcMessageBuffer &);

public:

  // ---------------------------------------------------------------------
  // Constructor, to be used by derived classes to specify type and
  // version of the object.
  // ---------------------------------------------------------------------
  IpcMessageObj(IpcMessageObjType objType,
			   IpcMessageObjVersion version);

  // constructor used to perform copyless receive, maps packed object in place.
  // Derived class should handle endianness. A message stream may route
  // a message after unpacking some of the objects so a message object must
  // deal with potentially being unpacked multiple times.
  IpcMessageObj(IpcBufferedMsgStream* msgStream);

  // destructor
  virtual ~IpcMessageObj();

  // used to allocate a packed send object. Operator "new" gets
  // packedObj from class IpcBufferedMsgStream, then sets "this =
  // packedObj" for constructor.  "AppendDataLen" allows raw data
  // allocation in the message buffer following the packed object for
  // packing raw data. "new" can be recalled multiple times during
  // construction of complex objects. The root object and all branch
  // objects must be derived from IpcMessageObj. Unpacking complex
  // objects on the receive side must be in same order as construction
  // on the send side.
  void* operator new(size_t size,
                                IpcBufferedMsgStream& msgStream,
                                IpcMessageObjSize appendDataLen = 0);

  // used to perform copyless receive. User must get packedObj from
  // IpcBufferedMsgStream::receiveMsgObj(). "new" sets "this = packedObj"
  // for constructor so object can be mapped inplace.
  void* operator new(size_t size, IpcMessageObj* packedObj)
    { return packedObj; }


  // interface to global ::operator new
  void* operator new(size_t size, CollHeap* h = NULL)
    { return (h ? h->allocateMemory(size) : ::operator new(size)); }



  void operator delete(void *ptr)       { ::operator delete(ptr); }

  // ---------------------------------------------------------------------
  // accessor methods
  // ---------------------------------------------------------------------
  Int32 getType() const                  { return (Int32) s_.objType_; }
  Int32 getObjLength() const           { return (Int32) s_.objLength_; }
  IpcMessageObjVersion getVersion() const { return s_.objVersion_; }
  void setType(IpcMessageObjType t)             { s_.objType_ = t; }
  void setVersion(IpcMessageObjVersion v)    { s_.objVersion_ = v; }

  char getEndianness() const { return s_.endianness_; }
  void setEndianness(char e) { s_.endianness_ = e; }

  // ---------------------------------------------------------------------
  // A way for the user object to tell if the actual version is OK. 
  // The default is that the actual version is OK only if it matches the object.
  // 
  virtual NABoolean isActualVersionOK (const IpcMessageObjVersion actualVersion) const
    { return (actualVersion == s_.objVersion_);}

  // ---------------------------------------------------------------------
  // A way for the user object to tell the message system how much space
  // to reserve for the packed object (always override this!!).
  // Usually, the overriding method looks like this:
  //
  // IpcMessageObjSize MyDerivedObject::packedLength()
  // {
  //   return (IpcMessageObjSize) sizeof(*this);
  // }
  //
  // Exception: if you are able to compress the object before copying
  // it into a message, or if you have to add dependent objects.
  // Use method baseClassPackedLength() to find out about the space
  // requirements of the IpcMessageObj base class (which must be included
  // in the returned result).
  // ---------------------------------------------------------------------
  virtual IpcMessageObjSize packedLength()
    { return (IpcMessageObjSize) sizeof(*this); }

  // hard-code the IpcMessageObj class size on 32-bit mode
  // maybe baseClassPackedLength32 is actually used
  virtual IpcMessageObjSize packedLength32()
    { return SQL_32BIT_IPC_MESSAGE_OBJ_SIZE; }


  // ---------------------------------------------------------------------
  // A method to take a user object and pack it into a message
  // (buffer has a length of packedLength() bytes, unless a fixed
  // length > 0 has been specified in the constructor).
  // The default implementation simply copies packedLength() bytes from
  // this object into the buffer.
  // If you override this function, make sure that you also pack the
  // object header (members of IpcMessageObj) into the message by
  // calling IpcMessageObj::packBaseClassIntoMessage(). Do not attempt to
  // pack the message header yourself, as some fields get altered
  // in the call. It is safe, however, to assume that the packed length
  // of the header is the same as its unpacked length (32 bytes, see below).
  // ---------------------------------------------------------------------
  virtual IpcMessageObjSize packObjIntoMessage(
       IpcMessageBufferPtr buffer);

  virtual IpcMessageObjSize packObjIntoMessage(
       IpcMessageBufferPtr buffer, NABoolean swapBytes);

  virtual IpcMessageObjSize packObjIntoMessage32(
       IpcMessageBufferPtr buffer);

  virtual IpcMessageObjSize packObjIntoMessage32(
       IpcMessageBufferPtr buffer, NABoolean swapBytes);

  // ---------------------------------------------------------------------
  // Unpack this object from a buffer (overwrites all existing data
  // in this object). The default implementation simply copies a data
  // block of objSize bytes over this object and then restores the
  // virtual function pointer of the base class, IpcMessageObj.
  // If your derived class also has virtual functions or if you need to
  // do something other than simply copying the data, then override this!!
  // The parameters passed in are:
  // - object type and version used for packing the object
  // - an indicator whether the object was packed on a machine with the
  //   same endianness (byte order) as this one
  // - the size of the packed object (includes space for this class,
  //   IpcMessageObj, see baseClassPackedLength() method)
  // - the array of bytes that contains the packed data
  // ---------------------------------------------------------------------
  virtual void unpackObj(IpcMessageObjType objType,
				    IpcMessageObjVersion objVersion,
				    NABoolean sameEndianness,
				    IpcMessageObjSize objSize,
				    IpcConstMessageBufferPtr buffer);

  virtual void unpackObj32(IpcMessageObjType objType,
				    IpcMessageObjVersion objVersion,
				    NABoolean sameEndianness,
				    IpcMessageObjSize objSize,
				    IpcConstMessageBufferPtr buffer);

  // ---------------------------------------------------------------------
  // increment/decrement refcounts (indicates how many users the
  // object has at this moment). The following operations affect the refcount:
  // -- The constructor sets the refcount to 1
  // -- Adding an object to a message increments the refcount, unless the
  //    message doesn't share objects
  // -- Sending a message decreases the refcount of a shared object (depending
  //    on the underlying message system, the refcount of shared objects is
  //    decreased during the send operation or when the I/O completes or when
  //    the receiving process releases the shared object.
  // -- An object that is copied from a message during a receive operation
  //    keeps the refcount of the target object.
  // -- The incrRefCount() and decrRefCount() methods can be called by
  //    the user to indicate how many references the user has to the object
  //    (increment for any additional references except the first), decrement
  //    when references go away, except when the object is allocated on the
  //    stack or in a static area and goes out of scope.
  // -- For objects allocated on the stack or for global objects, the user
  //    should NOT call the incrRefCount() or decrRefCount() methods. No
  //    special handling is necessary when those objects go out of scope.
  //    The program will abend, however, if such objects go out of scope
  //    while they are still shared with another component (like an
  //    outstanding I/O).
  // -- For objects allocated with operator new, the user should use the
  //    decrRefCount() method to indicate that the object can be deleted.
  //    Actual deletion will be delayed until the object is no longer needed.
  //    Using operator delete on such objects is not recommended. If it
  //    is used anyway, the program will abend if the refcount of the object
  //    is not equal to 1.
  // -- Override these functions, for example, if your object has an
  //    operator delete that needs to be called instead of global operator
  //    delete.
  // ---------------------------------------------------------------------
  virtual IpcMessageRefCount incrRefCount();
  virtual IpcMessageRefCount decrRefCount();
  IpcMessageRefCount getRefCount()     { return s_.refCount_; }

  // ---------------------------------------------------------------------
  // Helper methods for derived classes that override the
  // packObjIntoMessage method
  // ---------------------------------------------------------------------

  // find out the packed length of the base class part of this object
  inline IpcMessageObjSize baseClassPackedLength()
                                                   {return sizeof(*this);}

  // find out the packed length of the 32-bit base class part of this object
  inline IpcMessageObjSize baseClassPackedLength32()
                                  {return SQL_32BIT_IPC_MESSAGE_OBJ_SIZE;}

  // pack the base class part of this object into the buffer and side-effect
  // the buffer pointer to point past that packed object
  IpcMessageObjSize packBaseClassIntoMessage(
       IpcMessageBufferPtr &buffer);

  // pack the base class to be read by 32-bit (BDR) client
  IpcMessageObjSize packBaseClassIntoMessage32(
       IpcMessageBufferPtr &buffer, NABoolean swapBytes);

  // pack the base class part of this object into the buffer after swapping 
  // if swapBytes is true
  // and side-effect the buffer pointer to point past that packed object
  IpcMessageObjSize packBaseClassIntoMessage(
       IpcMessageBufferPtr &buffer, NABoolean swapBytes);

  // add filler to a buffer pointer so it points to an 8 byte aligned address
  // (must be done before packing a dependent IpcMessageObj into a buffer)
  static void alignBufferForNextObj(
       IpcConstMessageBufferPtr &buffer);
  static void alignBufferForNextObj(
       IpcMessageBufferPtr &buffer);

  // do the same as alignBufferForNextObj, but with a size instead of a buffer
  static void alignSizeForNextObj(IpcMessageObjSize &size);

  // unpack the base class part from the buffer and side-effect the buffer
  // to point to directly past the base class part
  void unpackBaseClass(IpcConstMessageBufferPtr &buffer);

  // unpack base class got from 32-bit server
  void unpackBaseClass32(IpcConstMessageBufferPtr &buffer);

  IpcMessageObjSize packDependentObjIntoMessage(
       IpcMessageBufferPtr buffer, NABoolean swapBytes);

  // pack a dependent object that is also an IpcMessageObj into a buffer
  // (don't call packObjIntoMessage() directly to do this)
  IpcMessageObjSize packDependentObjIntoMessage(
       IpcMessageBufferPtr buffer);

  IpcMessageObjSize packDependentObjIntoMessage32(
       IpcMessageBufferPtr buffer, NABoolean swapBytes);

  // pack a dependent object that is also an IpcMessageObj into a buffer
  // (don't call packObjIntoMessage() directly to do this)
  IpcMessageObjSize packDependentObjIntoMessage32(
       IpcMessageBufferPtr buffer);

  // unpack a dependent IpcMessageObj object from a buffer and side-effect
  // the buffer to point directly past the object that was unpacked
  // (this methods reads object type and version from the buffer and
  // then calls the virtual method unpackObj(), so it needs to be
  // called on the dependent object that has its virtual function ptr. set up)
  void unpackDependentObjFromBuffer(
       IpcConstMessageBufferPtr &buffer,
       NABoolean sameEndianness);

  void unpackDependentObjFromBuffer32(
       IpcConstMessageBufferPtr &buffer,
       NABoolean sameEndianness);

  // ---------------------------------------------------------------------
  // The following "check" methods are used prior to unpacking to
  // verify that the unpacking of a packed object will not create
  // inconsistent structures or bad pointers. Currently we only do
  // these checks on replies from the non-trusted UDR server.
  //
  // checkObj() is a virtual method and should be overridden in
  // message classes used ir UDR replies.
  //
  // checkBaseClass() and checkDependentObj() are non-virtual helper
  // functions that can be used in overridden checkObj() methods.
  // ---------------------------------------------------------------------
  virtual NABoolean checkObj(IpcMessageObjType t,
                             IpcMessageObjVersion v,
                             NABoolean sameEndianness,
                             IpcMessageObjSize size,
                             IpcConstMessageBufferPtr buffer) const;

  NABoolean checkDependentObj(IpcConstMessageBufferPtr &buffer,
                              NABoolean sameEndianness) const;

  NABoolean checkBaseClass(IpcMessageObjType t,
                           IpcMessageObjVersion v,
                           NABoolean sameEndianness,
                           IpcMessageObjSize size,
                           IpcConstMessageBufferPtr &buffer) const;

  IpcMessageObj *getNextFromOffset();
protected:

  // turn the representation of the internal object from little-endian
  // to big-endian and vice versa (used by communication services that
  // talk between big-endians and little-endians, e.g. sockets)
public:
  void turnByteOrder();
  
private:

  // Called to check if IpcMessageObj in IpcMessageBuffer is available for
  // recycle. Derived classes MUST define if using copyless receive and object
  // needs to persist beyond the unpacking of the complete receive message. If
  // this method is not defined by the derived class then it will be guaranteed
  // to persist until the current receive message is advanced via
  // IpcBufferedMsgStream::getNextReceiveMsg().

  virtual NABoolean msgObjIsFree()
    { return(TRUE); }

  // prepare object for send. Called prior to sending the IPC message via
  // IpcBufferedMsgStream::prepSendMsgForOutput(). Derived class may define
  // to deal with transport issues such as changing pointers to offsets, etc.

  virtual void prepMsgObjForSend()
    { return; }
                  
  // merge next packed message object in IpcMessageBuffer with this object
  void mergeNextPackedObj();

  // make the data members reside in their own struct, since this is the unit
  // of transfer between in-memory objects and messages
  // NOTE: the struct does not contain the _vptr data member of the class!!!
  struct IpcMessageObjStruct
    {
      IpcMessageObjType     objType_;    
      IpcMessageObjVersion  objVersion_; 
      IpcMessageRefCount    refCount_;
      IpcMessageObjSize     objLength_;  // used by IpcMessageStream only
      IpcMessageObj         *next_;      // IpcBufferedMsgStream - offset only
                                         // IpcMessageStream - offset and ptr
      char endianness_;                  // big-endian, little endian
      char spare1_;                      // spare for future use
      short spare2_;                     // spare for future use
      char* vPtrPad_;                    // if NT or HSC, allocate space for NSK vptr
    } s_;

  // ---------------------------------------------------------------------
  // we assume a 32 byte layout of this object that is as follows:
  //
  //    +---------------------------------+     \
  //  0 | _vptr (NT) or padding (NSK)     |      |
  //    +---------------------------------+      |
  //  4 | objType_  _                     |      |
  //    +---------------------------------+      |
  //  8 | objVersion_                     |      |
  //    +---------------------------------+      |
  // 12 | refCount_                       |      |
  //    +---------------------------------+      |
  // 16 | objLength_                      |      >  IpcMessageObjStruct
  //    +---------------------------------+      |
  // 20 | next_                           |      |
  //    +---------------------------------+      |
  // 24 | endianness_                     |      |
  //    +---------------------------------+      |
  // 25 | spare1_                         |      |
  //    +---------------------------------+      |
  // 26 | spare2_                         |      |
  //    +---------------------------------+      |
  // 28 | _vptr (NSK) or padding (NT)     |      |
  //    +---------------------------------+     /
  //
  // And, although we are ashamed to admit it, we access the virtual
  // function pointer, if necessary, by using ((char **) (&this[1]))[-1]
  //
  // Note: in the packed representation, each packed object starts on
  // an address that is aligned on an 8 byte boundary. Since
  // sizeof(IpcMessageObj) == 32, the part for a derived object also
  // starts on an address that is aligned on an 8 byte boundary.
  // ---------------------------------------------------------------------

  char *getMyVPtr()      { return ((char **) (&this[0]))[0];  }
  void setMyVPtr(char *v)   { ((char **) (&this[0]))[0] = v; }
  // return a pointer to the next object in the message by converting
  // the relative offset stored in next_ back into a pointer


  // convert the next_ pointer into an offset
  void convertNextToOffset();

};

// -----------------------------------------------------------------------
// Functions (templates, too) for packing and unpacking scalars.
// "Check" functions are also provided to determine whether a buffer
// has sufficient space for an item of a given size.
// -----------------------------------------------------------------------

IpcMessageObjSize packCharStarIntoBuffer(IpcMessageBufferPtr &buffer,
						    char* strPtr,
                                                  NABoolean swapBytes = FALSE);

// UR2
IpcMessageObjSize packCharStarIntoBuffer(IpcMessageBufferPtr &buffer, 
                                 NAWchar* strPtr,
                                 NABoolean swapBytes = FALSE);

inline IpcMessageObjSize packStrIntoBuffer(char* &buffer,
                                                      char* targetObj,
                                                      ULng32 objSize)
{
  str_cpy_all(buffer, targetObj, (Lng32)objSize);
  buffer += objSize;
  return objSize;
}

void unpackBuffer(const char* &buffer,
			     char* &strPtr, CollHeap* collHeapPtr);

inline void unpackStrFromBuffer(const char* &buffer,
                                           char* targetObj,
                                           ULng32 objSize)
{
  str_cpy_all(targetObj, buffer, (Lng32)objSize);
  buffer += objSize;
}

void skipCharStarInBuffer(const char* &buffer);

NABoolean checkBuffer (
    /* INOUT */ IpcConstMessageBufferPtr &buffer,
    /* IN    */ ULng32 dataLength,
    /* IN    */ IpcConstMessageBufferPtr lastByte );

NABoolean checkAndUnpackBuffer (
    /* INOUT */ IpcConstMessageBufferPtr &buffer,
    /* IN    */ ULng32 dataLength,
    /* OUT   */ char *dataPtr,
    /* IN    */ IpcConstMessageBufferPtr lastByte );

NABoolean checkCharStarInBuffer (
    /* INOUT */ IpcConstMessageBufferPtr &buffer,
    /* IN    */ NABoolean sameEndianness,
    /* IN    */ IpcConstMessageBufferPtr lastByte );

// An integrity check epilogue function is provided here. It should be
// called whenever an overridden IpcMessageObj::checkObj()
// implementation detects a failure. The global "check" functions
// above call this epilogue when they detect a failure. If a
// checkObj() method detects a failure by some means other than by
// calling one of the "check" functions, it should call the epilogue
// before returning FALSE. Current the epilogue's only purpose is as a
// debugging aid. By setting a breakpoint on the epilogue you can
// quickly track down the cause of a checkObj() failure.
void ipcIntegrityCheckEpilogue(NABoolean status);

template <class ScalarType>
inline IpcMessageObjSize packIntoBuffer(char* &buffer, 
		ScalarType scalarVariable)
{
  // * (ScalarType *) buffer = scalarVariable;
  str_cpy_all(buffer,(char*)&scalarVariable,sizeof(ScalarType));
  buffer += sizeof(ScalarType);
  return sizeof(ScalarType);
}

template <class ScalarType>
inline IpcMessageObjSize packIntoBuffer(char* &buffer, 
		ScalarType scalarVariable,
                NABoolean swapBytes)
{
  if (swapBytes)
  {
     switch (sizeof(ScalarType))
     {
        case 2:
          scalarVariable = (ScalarType)bswap_16(scalarVariable);
          break;
        case 4:
          scalarVariable = (ScalarType)bswap_32(scalarVariable);
          break;
        case 8:
          scalarVariable = (ScalarType)bswap_64(scalarVariable);
          break;
     }
  }
  // * (ScalarType *) buffer = scalarVariable;
  str_cpy_all(buffer,(char*)&scalarVariable,sizeof(ScalarType));
  buffer += sizeof(ScalarType);
  return sizeof(ScalarType);
}

template <class ScalarType>
inline void unpackBuffer(const char* &buffer,
				    ScalarType& scalarVariable)
{
  // scalarVariable = * (ScalarType *) buffer;
  ScalarType  temp;
  str_cpy_all((char*) &temp,buffer,sizeof(ScalarType));
  scalarVariable = temp;
  buffer += sizeof(ScalarType);
}

inline void swapFourBytes(UInt32 &b4)
{
  char *c4 = (char *) (&b4);
  char x;

  x = c4[0]; c4[0] = c4[3]; c4[3] = x;
  x = c4[1]; c4[1] = c4[2]; c4[2] = x;
}

inline void swapFourBytes(Int32 &b4)
{
  char *c4 = (char *) (&b4);
  char x;

  x = c4[0]; c4[0] = c4[3]; c4[3] = x;
  x = c4[1]; c4[1] = c4[2]; c4[2] = x;
}

inline void swapTwoBytes(unsigned short &b2)
{
  char *c2 = (char *) (&b2);
  char x;

  x = c2[0]; c2[0] = c2[1]; c2[1] = x;
}

inline void swapTwoBytes(short &b2)
{
  char *c2 = (char *) (&b2);
  char x;

  x = c2[0]; c2[0] = c2[1]; c2[1] = x;
}

#endif  /* IPCMESSAGEOBJ_H */


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
* File:         NAVersionedObject.h
*
* Description:  The NAVersionedObject class encapsulates the version header
*               added to each SQL Executor Plan object stored on disk. It
*               is the base class for the classes these objects belong. It
*               also provides driver routines for the packing and unpacking
*               processes on those objects.
*
*               During packing, the objects are converted to a predefined
*               reference layout and pointers in the objects are converted
*               into offsets. During unpacking, object images are modified
*               to fit in the local platform; older objects are migrated to
*               their current versions.
*
*               The NAVersionedObjectPtr class encapsulate a pointer to a
*               NAVersionedObject. The class is packed with fillers on a
*               32-bit platform so that the class size is always 64 bits.
*               This is done to smoothen transition to 64-bit platforms in
*               the future. Objects of the class are used inside all plan
*               objects derived from NAVersionedObject, so that these plan
*               objects will have the same size on 64-bit platforms.
*               
* Created:      9/8/98
* Language:     C++
* Status:       $State: Exp $
*
*
*
****************************************************************************
*/

#ifndef NAVERSIONEDOBJECT_H
#define NAVERSIONEDOBJECT_H

#include "Platform.h"
#include "ComSpace.h"
#include "NAAssert.h"
#include "str.h"
#include "Int64.h"

// ---------------------------------------------------------------------
// No of VersionID's supported by NAVersionedObject::versionIDArray_
// ---------------------------------------------------------------------
#define VERSION_ID_ARRAY_SIZE     8

// ---------------------------------------------------------------------
// To be stored in the eyeCatcher_ field of the object as both an eye
// catcher and an indicator that we are dealing with a believably good
// NAVersionedObject at the beginning of unpacking.
// ---------------------------------------------------------------------
#define VOBJ_EYE_CATCHER          "VO"
#define VOBJ_EYE_CATCHER_SIZE     2

// ---------------------------------------------------------------------
// Helper Macro to retrieve the virtual function table pointer of a
// given subclass of NAVersionedObject.
// ---------------------------------------------------------------------
// ---------------------------------------------------------------------
// On most platforms (NT, Unix) the pointer is stored as the first four/eight
// bytes of the object image.
// ---------------------------------------------------------------------
#define GetVTblPtr(vtblPtr,Class) \
{ \
  Class obj; \
  vtblPtr = *((char **)(&obj)); \
}

// ---------------------------------------------------------------------
// Utilities
// ---------------------------------------------------------------------
inline void swapInt64(char *c)
{
  char y;
  y=c[0]; c[0]=c[7]; c[7]=y;
  y=c[1]; c[1]=c[6]; c[6]=y;
  y=c[2]; c[2]=c[5]; c[5]=y;
  y=c[3]; c[3]=c[4]; c[4]=y;
}

inline void swapInt32(Int32 *x)
{
  char *c = (char *) x;
  char y;
  y=c[0]; c[0]=c[3]; c[3]=y;
  y=c[1]; c[1]=c[2]; c[2]=y;
}

inline void swapInt16(Int16 *x)
{
  char *c = (char *) x;
  char y;
  y=c[0]; c[0]=c[1]; c[1]=y;
}

  
// ---------------------------------------------------------------------
// Classes declared in this file
// ---------------------------------------------------------------------
class NAVersionedObject;

// ---------------------------------------------------------------------
// Classes referenced in this file by pointer
// ---------------------------------------------------------------------


// ---------------------------------------------------------------------
// Class NABasicPtrTempl<Type>
//
// This template class encapsulates a pointer with a 4 byte filler on a
// 32-bit platform. On a 64-bit platform, the filler is non-existent.
// Objects of this class are used in place of a real pointer to "Type"
// in all objects stored on disk so that those objects have the same
// images regardless of whether they have been generated on a 32-bit or
// 64-bit platform.
//
// Also, the class overloads a bunch of operators which are frequently
// used with real pointers in such a way that those operators can be
// used on an object of this class as if the object is in fact a real
// pointer.
//
// This template differs from other class templates for pointers defined
// in this file in the way that it only supports the most primitive form
// of packing and unpacking (aka shallow packing and unpacking). That is,
// it plainly converts the stored pointer to and from an offset, without
// processing any further the object referenced by the pointer. This
// level of support is enough and appropriate for pointers of basic types
// such as char, int, short, long, and etc.
// ---------------------------------------------------------------------


template<class Type> class NABasicPtrTempl
{
public:

  // -------------------------------------------------------------------
  // public member functions
  // -------------------------------------------------------------------

  // -------------------------------------------------------------------
  // Constructors
  // -------------------------------------------------------------------
  NABasicPtrTempl(Type *ptr = (Type *)NULL)
    : ptr_(ptr)
  {
    // -----------------------------------------------------------------
    // This ensures that size of the class remains to be 64 bits. No
    // virtual functions should be added to this class because of this
    // requirement.
    // -----------------------------------------------------------------
    assert(sizeof(NABasicPtrTempl<Type>) == 8);
  }

  // -------------------------------------------------------------------
  // Accessors and Mutators
  // -------------------------------------------------------------------
  inline Type *getPointer() const            { return ptr_; }
  inline Type *&pointer()                    { return ptr_; }
  inline operator Type *() const             { return ptr_; }
  inline short isNull() const{ return (ptr_==(Type *)NULL); }
  inline Type & operator *() const          { return *ptr_; }

  // -------------------------------------------------------------------
  // delete operators
  // -------------------------------------------------------------------
  // inline void remove()          { deleteObjectReferenced(); }
  // inline void deleteObjectReferenced()       { delete ptr_; }

  // -------------------------------------------------------------------
  // Assume pointer is stored as a 64-bit offset
  // -------------------------------------------------------------------
  
    inline void toggleEndianness()       { swapInt64((char *)(&ptr_)); }

  // -------------------------------------------------------------------
  // Assignment operators - support assignment of the "real" pointer
  // -------------------------------------------------------------------
  
    inline NABasicPtrTempl<Type> & operator = (const void * const ptr)
                                   { ptr_ = (Type *)ptr; return *this; }
  
    inline NABasicPtrTempl<Type> & operator = (const Long ptr)
                                   { ptr_ = (Type *)ptr; return *this; }
  
    inline NABasicPtrTempl<Type> & operator = (const Int32 ptr)
                                   { ptr_ = (Type *)((long)ptr); return *this; }
  
    inline NABasicPtrTempl<Type> & operator = (const Type * const ptr)
                                   { ptr_ = (Type *)ptr; return *this; }
  
    inline NABasicPtrTempl<Type> & operator = (
                                      const NABasicPtrTempl<Type> & ptr)
                                      { ptr_ = ptr.ptr_; return *this; }

  // -------------------------------------------------------------------
  // Comparison operators - support comparisons with the "real" pointer.
  // -------------------------------------------------------------------
  
    inline Int32 operator == (const Int32 ptr) const
                                       { return (ptr_ == (Type *)ptr); }
  
    inline Int32 operator == (const NABasicPtrTempl<Type> & other) const
                                        { return (ptr_ == other.ptr_); }
  
    inline Int32 operator == (const Type * const ptr) const
                                               { return (ptr_ == ptr); }
  
    inline Int32 operator != (const Int32 ptr) const
                                       { return (ptr_ != (Type *)((long)ptr)); }
  
    inline Int32 operator != (const NABasicPtrTempl<Type> & other) const
                                        { return (ptr_ != other.ptr_); }
  
    inline Int32 operator != (const Type * const ptr) const
                                               { return (ptr_ != ptr); }
  
    inline Int32 operator ! () const    { return (ptr_ == (Type *)NULL); }

  // -------------------------------------------------------------------
  // Arithmetic operators - support pointer arithmetic.
  // -------------------------------------------------------------------
  
    inline NABasicPtrTempl<Type> operator +(const Int32 n) const
                             { return NABasicPtrTempl<Type>(ptr_ + n); }
  
    inline NABasicPtrTempl<Type> operator +(const short n) const
                             { return NABasicPtrTempl<Type>(ptr_ + n); }
    inline NABasicPtrTempl<Type> operator +(const Long n) const
                             { return NABasicPtrTempl<Type>(ptr_ + n); }
  
    inline NABasicPtrTempl<Type> operator +(const UInt32 n) const
                             { return NABasicPtrTempl<Type>(ptr_ + n); }
  
    inline
      NABasicPtrTempl<Type> operator +(const unsigned short n) const
                             { return NABasicPtrTempl<Type>(ptr_ + n); }

    inline NABasicPtrTempl<Type> operator +(const ULong n) const
                             { return NABasicPtrTempl<Type>(ptr_ + n); }
  
    inline NABasicPtrTempl<Type> operator -(const Int32 n) const
                             { return NABasicPtrTempl<Type>(ptr_ - n); }
  
    inline NABasicPtrTempl<Type> operator -(const short n) const
                             { return NABasicPtrTempl<Type>(ptr_ - n); }
  
    inline NABasicPtrTempl<Type> operator -(const Long n) const
                             { return NABasicPtrTempl<Type>(ptr_ - n); }
  
    inline NABasicPtrTempl<Type> operator -(const UInt32 n) const
                             { return NABasicPtrTempl<Type>(ptr_ - n); }
  
    inline
      NABasicPtrTempl<Type> operator -(const unsigned short n) const
                             { return NABasicPtrTempl<Type>(ptr_ - n); }
  
    inline NABasicPtrTempl<Type> operator -(const ULong n) const
                             { return NABasicPtrTempl<Type>(ptr_ - n); }
  
    inline NABasicPtrTempl<Type> & operator +=(const Int32 n)
                                            { ptr_ += n; return *this; }
  
    inline NABasicPtrTempl<Type> & operator +=(const short n)
                                            { ptr_ += n; return *this; }
  
    inline NABasicPtrTempl<Type> & operator +=(const Long n)
                                            { ptr_ += n; return *this; }
  
    inline NABasicPtrTempl<Type> & operator +=(const UInt32 n)
                                            { ptr_ += n; return *this; }
  
    inline NABasicPtrTempl<Type> & operator +=(const unsigned short n)
                                            { ptr_ += n; return *this; }
  
    inline NABasicPtrTempl<Type> & operator +=(const ULong n)
                                            { ptr_ += n; return *this; }

    inline NABasicPtrTempl<Type> & operator -=(const Int32 n)
                                            { ptr_ -= n; return *this; }
  
    inline NABasicPtrTempl<Type> & operator -=(const short n)
                                            { ptr_ -= n; return *this; }
  
    inline NABasicPtrTempl<Type> & operator -=(const Long n)
                                            { ptr_ -= n; return *this; }
  
    inline NABasicPtrTempl<Type> & operator -=(const UInt32 n)
                                            { ptr_ -= n; return *this; }
  
    inline NABasicPtrTempl<Type> & operator -=(const unsigned short n)
                                            { ptr_ -= n; return *this; }
  
    inline NABasicPtrTempl<Type> & operator -=(const ULong n)
                                            { ptr_ -= n; return *this; }
  
    inline NABasicPtrTempl<Type> & operator ++()
                                               { ++ptr_; return *this; }
  
    inline NABasicPtrTempl<Type> & operator ++(const Int32 n)
                                               { ptr_++; return *this; }
  
    inline NABasicPtrTempl<Type> & operator --()
                                               { --ptr_; return *this; }
  
    inline NABasicPtrTempl<Type> & operator --(const Int32 n)
                                               { ptr_--; return *this; }

  // -------------------------------------------------------------------
  // Subscript operator. An NABasicPtrTempl is sometimes used to
  // represent an array of the base type. This operator is useful in
  // such cases.
  // -------------------------------------------------------------------
  inline Type & operator [] (const Int32 i) const
                                                     { return ptr_[i]; }
  inline Type & operator [] (const short i) const
                                                     { return ptr_[i]; }

  inline Type & operator [] (const Long i) const
                                                     { return ptr_[i]; }

  inline Type & operator [] (const UInt32 i) const
                                                     { return ptr_[i]; }
  inline Type & operator [] (const unsigned short i) const
                                                     { return ptr_[i]; }

  inline Type & operator [] (const ULong i) const
                                                     { return ptr_[i]; }

  // -------------------------------------------------------------------
  // Packing and Unpacking
  // -------------------------------------------------------------------
  inline Long pack(void *space, short isSpacePtr = 1)
  {
    Long offset = 0;
    if(ptr_ != 0)
    {
      if(isSpacePtr)
        offset = ((Space *)space)->convertToOffset((char *)ptr_);
      else
        offset = ((char *)space - (char *)ptr_);
      offset_ = offset;
    }
    else offset_ = 0;
    return offset;
  }

  inline Lng32 unpack(void *base, short isSpacePtr = 0)
  {
    if(offset_ != 0)
    {
      if (!isSpacePtr) {
        ptr_ = (Type *)((char *)base - offset_);
      }
      else {
        ptr_ = (Type *)(((Space*)base)->convertToPtr(offset_));
      }
    }
    return 0;
  }

protected:

  // -------------------------------------------------------------------
  // private data members
  // -------------------------------------------------------------------
  union
  {
    Type                 *ptr_;
    Int64                 offset_;
  };

};  // END of class declaration for NABasicPtrTempl --------------------


// ---------------------------------------------------------------------
// Define pointers to basic types via use of NABasicPtrTempl.
// ---------------------------------------------------------------------
typedef NABasicPtrTempl<Int16>   Int16Ptr;
typedef NABasicPtrTempl<Int32>   Int32Ptr;
typedef NABasicPtrTempl<Int64>   Int64Ptr;
typedef NABasicPtrTempl<Long>    LongPtr;
typedef NABasicPtrTempl<char>    NABasicPtr;


// ---------------------------------------------------------------------
// Class NAOpenObjectPtrTempl<Type>
//
// This template class encapsulates a pointer with a 4 byte filler on a
// 32-bit platform. On a 64-bit platform, the filler is non-existent.
// Objects of this class are used in place of a real pointer to "Type"
// in all objects stored on disk so that those objects have the same
// images regardless of whether they have been generated on a 32-bit or
// 64-bit platform.
//
// Also, the class overloads a bunch of operators which are frequently
// used with real pointers in such a way that those operators can be
// used on an object of this class as if the object is in fact a real
// pointer.
//
// This template differs from other class templates for pointers defined
// in this file in the way that the template does not provide an
// implementation for packing and unpacking. Users of this template are
// supposed to customerize such behaviors. The template, however, does
// provide helper methods such as packShallow() and unpackShallow().
//
// This level of support is appropriate for pointers to class which are
// either outside of SQL or for some reasons, could not be derived from
// NAVersionedObject.
// ---------------------------------------------------------------------


template<class Type> class NAOpenObjectPtrTempl
{
public:

  // -------------------------------------------------------------------
  // public member functions
  // -------------------------------------------------------------------

  // -------------------------------------------------------------------
  // Constructors
  // -------------------------------------------------------------------
  NAOpenObjectPtrTempl(Type *ptr = (Type *)NULL)
    : ptr_(ptr)
  {
    // -----------------------------------------------------------------
    // This ensures that size of the class remains to be 64 bits. No
    // virtual functions should be added to this class because of this
    // requirement.
    // -----------------------------------------------------------------
    assert(sizeof(NAOpenObjectPtrTempl<Type>) == 8);
  }

  // -------------------------------------------------------------------
  // Accessors and Mutators
  // -------------------------------------------------------------------
  inline Type *getPointer() const            { return ptr_; }
  inline Type *&pointer()                    { return ptr_; }
  inline operator Type *() const             { return ptr_; }
  inline short isNull() const{ return (ptr_==(Type *)NULL); }
  inline Type & operator *() const          { return *ptr_; }

  // -------------------------------------------------------------------
  // delete operators
  // -------------------------------------------------------------------
  // inline void remove()          { deleteObjectReferenced(); }
  // inline void deleteObjectReferenced()       { delete ptr_; }

  // -------------------------------------------------------------------
  // Assume pointer is stored as a 64-bit offset
  // -------------------------------------------------------------------
  inline void toggleEndianness()
                                         { swapInt64((char *)(&ptr_)); }

  // -------------------------------------------------------------------
  // Assignment operators - support assignment of the "real" pointer
  // -------------------------------------------------------------------
  
    inline NAOpenObjectPtrTempl<Type> &
      operator = (const void * const ptr)
                                   { ptr_ = (Type *)ptr; return *this; }
  
    inline NAOpenObjectPtrTempl<Type> &
      operator = (const Int32 ptr)
                                   { ptr_ = (Type *)((long)ptr); return *this; }
  
    inline
      NAOpenObjectPtrTempl<Type> & operator = (const Type * const ptr)
                                   { ptr_ = (Type *)((long)ptr); return *this; }
  
    inline
      NAOpenObjectPtrTempl<Type> &
        operator = (const NAOpenObjectPtrTempl<Type> & ptr)
                                      { ptr_ = ptr.ptr_; return *this; }

  // -------------------------------------------------------------------
  // Comparison operators - support comparisons with the "real" pointer.
  // -------------------------------------------------------------------
  
    inline Int32 operator == (const Int32 ptr) const
                                       { return (ptr_ == (Type *)ptr); }
  
    inline
      Int32 operator == (const NAOpenObjectPtrTempl<Type> & other) const
                                        { return (ptr_ == other.ptr_); }
  
    inline Int32 operator == (const Type * const ptr) const
                                               { return (ptr_ == ptr); }
  
    inline Int32 operator != (const Int32 ptr) const
                                       { return (ptr_ != (Type *)ptr); }
  
    inline
      Int32 operator != (const NAOpenObjectPtrTempl<Type> & other) const
                                        { return (ptr_ != other.ptr_); }
  
    inline Int32 operator != (const Type * const ptr) const
                                               { return (ptr_ != ptr); }
  
    inline Int32 operator ! () const    { return (ptr_ == (Type *)NULL); }

  // -------------------------------------------------------------------
  // Arithmetic operators - support pointer arithmetic.
  // -------------------------------------------------------------------
  
    inline NAOpenObjectPtrTempl<Type> operator +(const Int32 n) const
                        { return NAOpenObjectPtrTempl<Type>(ptr_ + n); }
  
    inline NAOpenObjectPtrTempl<Type> operator +(const short n) const
                        { return NAOpenObjectPtrTempl<Type>(ptr_ + n); }
  
    inline NAOpenObjectPtrTempl<Type> operator +(const Long n) const
                        { return NAOpenObjectPtrTempl<Type>(ptr_ + n); }
  
    inline
      NAOpenObjectPtrTempl<Type> operator +(const UInt32 n) const
                        { return NAOpenObjectPtrTempl<Type>(ptr_ + n); }
  
    inline
      NAOpenObjectPtrTempl<Type>
        operator +(const unsigned short n) const
                        { return NAOpenObjectPtrTempl<Type>(ptr_ + n); }
  
    inline
      NAOpenObjectPtrTempl<Type> operator +(const ULong n) const
                        { return NAOpenObjectPtrTempl<Type>(ptr_ + n); }
  
    inline NAOpenObjectPtrTempl<Type> operator -(const Int32 n) const
                        { return NAOpenObjectPtrTempl<Type>(ptr_ - n); }
  
    inline NAOpenObjectPtrTempl<Type> operator -(const short n) const
                        { return NAOpenObjectPtrTempl<Type>(ptr_ - n); }
  
    inline NAOpenObjectPtrTempl<Type> operator -(const Long n) const
                        { return NAOpenObjectPtrTempl<Type>(ptr_ - n); }
  
    inline
      NAOpenObjectPtrTempl<Type> operator -(const UInt32 n) const
                        { return NAOpenObjectPtrTempl<Type>(ptr_ - n); }
  
    inline
      NAOpenObjectPtrTempl<Type>
        operator-(const unsigned short n) const
                        { return NAOpenObjectPtrTempl<Type>(ptr_ - n); }
  
    inline
      NAOpenObjectPtrTempl<Type> operator -(const ULong n) const
                        { return NAOpenObjectPtrTempl<Type>(ptr_ - n); }
  
    inline NAOpenObjectPtrTempl<Type> & operator +=(const Int32 n)
                                            { ptr_ += n; return *this; }
  
    inline NAOpenObjectPtrTempl<Type> & operator +=(const short n)
                                            { ptr_ += n; return *this; }
  
    inline NAOpenObjectPtrTempl<Type> & operator +=(const Long n)
                                            { ptr_ += n; return *this; }
  
    inline
      NAOpenObjectPtrTempl<Type> & operator +=(const UInt32 n)
                                            { ptr_ += n; return *this; }
  
    inline
      NAOpenObjectPtrTempl<Type> & operator +=(const unsigned short n)
                                            { ptr_ += n; return *this; }

    inline
      NAOpenObjectPtrTempl<Type> & operator +=(const ULong n)
                                            { ptr_ += n; return *this; }
  
    inline
      NAOpenObjectPtrTempl<Type> & operator -=(const Int32 n)
                                            { ptr_ -= n; return *this; }
  
    inline NAOpenObjectPtrTempl<Type> & operator -=(const short n)
                                            { ptr_ -= n; return *this; }
  
    inline NAOpenObjectPtrTempl<Type> & operator -=(const Long n)
                                            { ptr_ -= n; return *this; }
  
    inline
      NAOpenObjectPtrTempl<Type> & operator -=(const UInt32 n)
                                            { ptr_ -= n; return *this; }
  
    inline
      NAOpenObjectPtrTempl<Type> & operator -=(const unsigned short n)
                                            { ptr_ -= n; return *this; }
  
    inline
      NAOpenObjectPtrTempl<Type> & operator -=(const ULong n)
                                            { ptr_ -= n; return *this; }
  
    inline NAOpenObjectPtrTempl<Type> & operator ++()
                                               { ++ptr_; return *this; }
  
    inline NAOpenObjectPtrTempl<Type> & operator ++(const Int32 n)
                                               { ptr_++; return *this; }
  
    inline NAOpenObjectPtrTempl<Type> & operator --()
                                               { --ptr_; return *this; }
  
    inline NAOpenObjectPtrTempl<Type> & operator --(const Int32 n)
                                               { ptr_--; return *this; }

  // -------------------------------------------------------------------
  // Subscript operator. An NAOpenObjectPtrTempl is sometimes used to
  // represent an array of objects. This operator is useful in such
  // cases.
  // -------------------------------------------------------------------
  inline Type & operator [] (const Int32 i) const
                                                     { return ptr_[i]; }
  inline Type & operator [] (const short i) const
                                                     { return ptr_[i]; }
  inline Type & operator [] (const Long i) const
                                                     { return ptr_[i]; }

  inline Type & operator [] (const UInt32 i) const
                                                     { return ptr_[i]; }
  inline Type & operator [] (const unsigned short i) const
                                                     { return ptr_[i]; }

  inline Type & operator [] (const ULong i) const
                                                     { return ptr_[i]; }

  // -------------------------------------------------------------------
  // Dereferencing operator
  // ------------------------------------------------------------------- 
  Type *operator ->() const
                                          { assert(ptr_); return ptr_; }

  // -------------------------------------------------------------------
  // The definition of pack() and unpack() is the responsibility of the
  // template instantiator. packShallow() and unpackShallow() are just
  // helper methods provided.
  // -------------------------------------------------------------------
  inline Long packShallow(void *space, short isSpacePtr = 1)
  {
    Long offset = 0;
    if(ptr_ != 0)
    {
      if(isSpacePtr)
        offset = ((Space *)space)->convertToOffset((char *)ptr_);
      else
        offset = ((char *)space - (char *)ptr_);
      offset_ = offset;
    }
    else offset_ = 0;
    return offset;
  }

  inline Lng32 unpackShallow(void *base)
  {
    if(offset_ != 0)
    {
      ptr_ = (Type *)((char *)base - offset_);
    }
    return 0;
  }

  // -------------------------------------------------------------------
  // The template instantiator should define all or some of these four
  // methods according to needs.
  // -------------------------------------------------------------------
  
    Long pack(void *space, short isSpacePtr = 1);
  
    Lng32 unpack(void *base);
  
    Long packArray(void *space, Lng32 numEntries, short notSpacePtr = 1);
  
    Lng32 unpackArray(void *base, Lng32 numEntries);

protected:

  // -------------------------------------------------------------------
  // private data members
  // -------------------------------------------------------------------
  union
  {
    Type                 *ptr_;
    Int64                 offset_;
  };

};  // END of class declaration for NAOpenObjectPtrTempl ---------------


// ---------------------------------------------------------------------
// Class NAVersionedObjectPtrTempl<Type>
//
// This template class encapsulates a pointer with a 4 byte filler on a
// 32-bit platform. On a 64-bit platform, the filler is non-existent.
// Objects of this class are used in place of a real pointer to "Type"
// in all objects stored on disk so that those objects have the same
// images regardless of whether they have been generated on a 32-bit or
// 64-bit platform.
//
// Also, the class overloads a bunch of operators which are frequently
// used with real pointers in such a way that those operators can be
// used on an object of this class as if the object is in fact a real
// pointer.
//
// This template differs from other class templates for pointers defined
// in this file in the way that the use of this template assumes that
// the pointer points to an NAVersionedObject, which clearly defines 
// the behaviors of packing and unpacking by means of the driver routines
// drivePack() and driveUnpack().
// ---------------------------------------------------------------------
 
template<class Type> class NAVersionedObjectPtrTempl
{
public:

  // -------------------------------------------------------------------
  // public member functions
  // -------------------------------------------------------------------

  // -------------------------------------------------------------------
  // Constructors
  // -------------------------------------------------------------------
  NAVersionedObjectPtrTempl(Type *ptr = (Type *)NULL)
    : ptr_(ptr)
  {
    // -----------------------------------------------------------------
    // This ensures that size of the class remains to be 64 bits. No
    // virtual functions should be added to this class because of this
    // requirement.
    // -----------------------------------------------------------------
    assert(sizeof(NAVersionedObjectPtrTempl<Type>) == 8);
  }

  // -------------------------------------------------------------------
  // Accessors and Mutators
  // -------------------------------------------------------------------
  inline Type *getPointer() const            { return ptr_; }
  inline Type *&pointer()                    { return ptr_; }
  inline operator Type *() const             { return ptr_; }
  inline short isNull() const      { return ((short)(ptr_ == (Type *)NULL)); }
  inline Type & operator *() const          { return *ptr_; }
  inline Int64 getOffset() const             { return offset_; }

  // -------------------------------------------------------------------
  // delete operators
  // -------------------------------------------------------------------
  // inline void remove()          { deleteObjectReferenced(); }
  // inline void deleteObjectReferenced()       { delete ptr_; }

  // -------------------------------------------------------------------
  // Assume pointer is stored as a 64-bit offset
  // -------------------------------------------------------------------
  inline void toggleEndianness()
                                         { swapInt64((char *)(&ptr_)); }

  // -------------------------------------------------------------------
  // Assignment operators - support assignment of the "real" pointer
  // -------------------------------------------------------------------
  
    inline NAVersionedObjectPtrTempl<Type> &
      operator = (const void * const ptr)
                                   { ptr_ = (Type *)ptr; return *this; }
  
    inline NAVersionedObjectPtrTempl<Type> &
      operator = (const Long ptr)
                                   { ptr_ = (Type *)ptr; return *this; }
  
    inline NAVersionedObjectPtrTempl<Type> &
      operator = (const Int32 ptr)
                                   { ptr_ = (Type *)((long)ptr); return *this; }
  
    inline
      NAVersionedObjectPtrTempl<Type> &
        operator = (const Type * const ptr)
                                   { ptr_ = (Type *)ptr; return *this; }
  
    inline
      NAVersionedObjectPtrTempl<Type> &
        operator = (const NAVersionedObjectPtrTempl<Type> & ptr)
                                      { ptr_ = ptr.ptr_; return *this; }

  // -------------------------------------------------------------------
  // Comparison operators - support comparisons with the "real" pointer.
  // -------------------------------------------------------------------
  
    inline Int32 operator ==(const Int32 ptr) const
                                       { return (ptr_ == (Type *)ptr); }
  
    inline
      Int32
        operator ==(const NAVersionedObjectPtrTempl<Type> & other) const
                                        { return (ptr_ == other.ptr_); }
  
    inline
      Int32 operator ==(const Type * const ptr) const
                                               { return (ptr_ == ptr); }
  
    inline Int32 operator !=(const Int32 ptr) const
                                       { return (ptr_ != (Type *)ptr); }
  
    inline
      Int32
        operator !=(const NAVersionedObjectPtrTempl<Type> & other) const
                                        { return (ptr_ != other.ptr_); }
  
    inline Int32 operator !=(const Type * const ptr) const
                                               { return (ptr_ != ptr); }
  
    inline Int32 operator !() const     { return (ptr_ == (Type *)NULL); }

  // -------------------------------------------------------------------
  // Arithmetic operators - support pointer arithmetic.
  // -------------------------------------------------------------------
  
    inline NAVersionedObjectPtrTempl<Type> operator +(const Int32 n) const
                   { return NAVersionedObjectPtrTempl<Type>(ptr_ + n); }
  
    inline
      NAVersionedObjectPtrTempl<Type> operator +(const short n) const
                   { return NAVersionedObjectPtrTempl<Type>(ptr_ + n); }
  
    inline
      NAVersionedObjectPtrTempl<Type> operator +(const Long n) const
                   { return NAVersionedObjectPtrTempl<Type>(ptr_ + n); }
  
    inline
      NAVersionedObjectPtrTempl<Type>
        operator +(const UInt32 n) const
                   { return NAVersionedObjectPtrTempl<Type>(ptr_ + n); }
  
    inline
      NAVersionedObjectPtrTempl<Type>
        operator +(const unsigned short n) const
                   { return NAVersionedObjectPtrTempl<Type>(ptr_ + n); }
  
    inline
      NAVersionedObjectPtrTempl<Type>
        operator +(const ULong n) const
                   { return NAVersionedObjectPtrTempl<Type>(ptr_ + n); }
  
    inline NAVersionedObjectPtrTempl<Type> operator -(const Int32 n) const
                   { return NAVersionedObjectPtrTempl<Type>(ptr_ - n); }
  
    inline
      NAVersionedObjectPtrTempl<Type> operator -(const short n) const
                   { return NAVersionedObjectPtrTempl<Type>(ptr_ - n); }
  
    inline
      NAVersionedObjectPtrTempl<Type> operator -(const Long n) const
                   { return NAVersionedObjectPtrTempl<Type>(ptr_ - n); }
  
    inline
      NAVersionedObjectPtrTempl<Type>
        operator -(const UInt32 n) const
                   { return NAVersionedObjectPtrTempl<Type>(ptr_ - n); }
  
    inline
      NAVersionedObjectPtrTempl<Type>
        operator-(const unsigned short n) const
                   { return NAVersionedObjectPtrTempl<Type>(ptr_ - n); }
  
    inline
      NAVersionedObjectPtrTempl<Type>
        operator -(const ULong n) const
                   { return NAVersionedObjectPtrTempl<Type>(ptr_ - n); }
  
    inline NAVersionedObjectPtrTempl<Type> & operator +=(const Int32 n)
                                            { ptr_ += n; return *this; }
  
    inline NAVersionedObjectPtrTempl<Type> & operator +=(const short n)
                                            { ptr_ += n; return *this; }
  
    inline NAVersionedObjectPtrTempl<Type> & operator +=(const Long n)
                                            { ptr_ += n; return *this; }
  
    inline
      NAVersionedObjectPtrTempl<Type> &
        operator +=(const UInt32 n)   { ptr_ += n; return *this; }
  
    inline
      NAVersionedObjectPtrTempl<Type> &
        operator +=(const unsigned short n) { ptr_ += n; return *this; }
  
    inline
      NAVersionedObjectPtrTempl<Type> &
        operator +=(const ULong n)  { ptr_ += n; return *this; }
  
    inline
      NAVersionedObjectPtrTempl<Type> &
        operator -=(const Int32 n)            { ptr_ -= n; return *this; }
  
    inline NAVersionedObjectPtrTempl<Type> & operator -=(const short n)
                                            { ptr_ -= n; return *this; }
  
    inline NAVersionedObjectPtrTempl<Type> & operator -=(const Long n)
                                            { ptr_ -= n; return *this; }
  
    inline
      NAVersionedObjectPtrTempl<Type> &
        operator -=(const UInt32 n)   { ptr_ -= n; return *this; }
  
    inline
      NAVersionedObjectPtrTempl<Type> &
        operator -=(const unsigned short n) { ptr_ -= n; return *this; }
  
    inline
      NAVersionedObjectPtrTempl<Type> &
        operator -=(const ULong n)  { ptr_ -= n; return *this; }
  
    inline NAVersionedObjectPtrTempl<Type> & operator ++()
                                               { ++ptr_; return *this; }
  
    inline NAVersionedObjectPtrTempl<Type> & operator ++(const Int32 n)
                                               { ptr_++; return *this; }
  
    inline NAVersionedObjectPtrTempl<Type> & operator --()
                                               { --ptr_; return *this; }
  
    inline NAVersionedObjectPtrTempl<Type> & operator --(const Int32 n)
                                               { ptr_--; return *this; }

  // -------------------------------------------------------------------
  // Subscript operator. An NAVersionedObjectPtrTempl is sometimes used
  // to represent an array of objects. This operator is useful in such
  // cases.
  // -------------------------------------------------------------------
  inline Type & operator [] (const Int32 i) const
                                                     { return ptr_[i]; }
  inline Type & operator [] (const short i) const
                                                     { return ptr_[i]; }
  inline Type & operator [] (const Long i) const
                                                     { return ptr_[i]; }

  inline Type & operator [] (const UInt32 i) const
                                                     { return ptr_[i]; }
  inline Type & operator [] (const unsigned short i) const
                                                     { return ptr_[i]; }
  inline Type & operator [] (const ULong i) const
                                                     { return ptr_[i]; }

  // -------------------------------------------------------------------
  // Dereferencing operator
  // -------------------------------------------------------------------
  Type *operator ->() const
                                          { assert(ptr_); return ptr_; }

  // -------------------------------------------------------------------
  // Packing and Unpacking
  // -------------------------------------------------------------------
  inline Long packShallow(void *space, short isSpacePtr = 1)
  {
    Long offset = 0;
    if(ptr_ != 0)
    {
      if(isSpacePtr)
        offset = ((Space *)space)->convertToOffset((char *)ptr_);
      else
        offset = ((char *)space - (char *)ptr_);
      offset_ = offset;
    }
    else offset_ = 0;
    return offset;
  }

  inline Long pack(void *space, short isSpacePtr = 1)
  {
    if(ptr_ != 0) ptr_->drivePack(space, isSpacePtr);
    return packShallow(space, isSpacePtr);
  }

  inline Lng32 unpackShallow(void *base)
  {
    if(offset_ != 0)
    {
      ptr_ = (Type *)((char *)base - offset_);
    }
    return 0;
  }

  // This one is NOT an inline function to avoid putting an object of
  // type "Type" on the stack. See the definition further down in this file.
  char * getVTblPtr(Int16 classID);

  inline Lng32 unpack(void *base, void * reallocator)
  {
    if(ptr_ != 0)
    {
      unpackShallow(base);
      // get vtblPtr by calling a (non-inline) method to avoid having
      // an object of type "Type" on the stack (used to overflow the stack)
      char *vtblPtr = getVTblPtr(ptr_->getClassID());
      ptr_ = (Type *)(ptr_->driveUnpack(base,vtblPtr, reallocator));
      return ((ptr_ == (Type *)NULL) ? -1 : 0);
    }
    return 0;
  }
    
  // -------------------------------------------------------------------
  // The two methods below assume that ptr_ is the beginning address of
  // an array of Type objects with numEntries.
  // -------------------------------------------------------------------
  
    inline Long packArray(void *space, Lng32 numEntries, short isSpacePtr = 1)
  {
    if(ptr_ != 0)
    {
      for(Lng32 i = 1; i < numEntries; i++)
        ptr_[i].drivePack(space,isSpacePtr);
    }
    return pack(space,isSpacePtr);
  }

  inline Lng32 unpackArray(void *base, Lng32 numEntries,
				     void * reallocator)
  {
    if(unpack(base, reallocator)) return -1;
    if(ptr_ != 0)
    {
      char *vtblPtr = getVTblPtr(ptr_->getClassID());

      for(Lng32 i = 1; i < numEntries; i++)
      {
        if(ptr_[i].driveUnpack(base,vtblPtr, reallocator) == NULL)
          return -1;
      }
    }
    return 0;
  }

protected:

  // -------------------------------------------------------------------
  // private data members
  // -------------------------------------------------------------------
  union
  {
    Type                 *ptr_;
    Int64                 offset_;
  };

};  // END of class declaration for NAVersionedObjectPtrTempl ----------



template<class Type>
char * NAVersionedObjectPtrTempl<Type>::getVTblPtr(Int16 classID)
{
  Type obj;
  return obj.findVTblPtr(classID);
}

// ---------------------------------------------------------------------
// Class NAVersionedObjectPtrArrayTempl<PtrType>
// 
// This template class encapsulates a pointer with a 4 byte filler on a
// 32-bit platform. On a 64-bit platform, the filler is non-existent.
// Objects of this class are used in place of a real pointer to "Type"
// in all objects stored on disk so that those objects have the same
// images regardless of whether they have been generated on a 32-bit or
// 64-bit platform.
//
// Also, the class overloads a bunch of operators which are frequently
// used with real pointers in such a way that those operators can be
// used on an object of this class as if the object is in fact a real
// pointer.
//
// This template differs from other class templates for pointers defined
// in this file in the way that the use of this template assumes that
// the pointer points to an array of objects instantiated from another
// template NAVersionedObjectPtrTempl. Packing and unpacking are handled
// accordingly.
// ---------------------------------------------------------------------


template<class PtrType> class NAVersionedObjectPtrArrayTempl
{
public:

  // -------------------------------------------------------------------
  // public member functions
  // -------------------------------------------------------------------

  // -------------------------------------------------------------------
  // Constructors
  // -------------------------------------------------------------------
  NAVersionedObjectPtrArrayTempl(PtrType *ptr = (PtrType *)NULL)
    : ptr_(ptr)
  {
    // -----------------------------------------------------------------
    // This ensures that size of the class remains to be 64 bits. No
    // virtual functions should be added to this class because of this
    // requirement.
    // -----------------------------------------------------------------
    assert(sizeof(NAVersionedObjectPtrArrayTempl<PtrType>) == 8);
  }

  // -------------------------------------------------------------------
  // This constructor allocates the 64-bit pointer array and copies the
  // pointer array given (which may be a 32-bit pointer array).
  // -------------------------------------------------------------------
  NAVersionedObjectPtrArrayTempl(Space *space, 
                                            void **ptrArray,
                                            Int32 numEntries)
                 { allocateAndCopyPtrArray(space,ptrArray,numEntries); }

  // -------------------------------------------------------------------
  // Accessors and Mutators
  // -------------------------------------------------------------------
  inline PtrType *getPointer() const         { return ptr_; }
  inline PtrType *&pointer()                 { return ptr_; }
  inline operator PtrType *() const          { return ptr_; }
  inline short isNull() const{return(ptr_==(PtrType *)NULL);}
  inline PtrType & operator *() const       { return *ptr_; }

  // -------------------------------------------------------------------
  // delete operators
  // -------------------------------------------------------------------
  // inline void remove()          { deleteObjectReferenced(); }
  // inline void deleteObjectReferenced()       { delete ptr_; }

  // -------------------------------------------------------------------
  // Assignment operators - support assignment of the "real" pointer
  // -------------------------------------------------------------------
  
    inline NAVersionedObjectPtrArrayTempl<PtrType> &
      operator = (const void * const ptr)
                                { ptr_ = (PtrType *)ptr; return *this; }
  
    inline NAVersionedObjectPtrArrayTempl<PtrType> &
      operator = (const Int32 ptr)
                                { ptr_ = (PtrType *)((long)ptr); return *this; }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType> &
        operator = (const PtrType * const ptr)
                                { ptr_ = (PtrType *)ptr; return *this; }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType> &
        operator = (const NAVersionedObjectPtrArrayTempl<PtrType> & ptr)
                                      { ptr_ = ptr.ptr_; return *this; }

  // -------------------------------------------------------------------
  // Comparison operators - support comparisons with the "real" pointer.
  // -------------------------------------------------------------------
  
    inline Int32 operator == (const Int32 ptr) const
                                    { return (ptr_ == (PtrType *)ptr); }
  
    inline
      Int32 operator == (
        const NAVersionedObjectPtrArrayTempl<PtrType> & other) const
                                        { return (ptr_ == other.ptr_); }
  
    inline Int32 operator == (const PtrType * const ptr) const
                                               { return (ptr_ == ptr); }
  
    inline Int32 operator != (const Int32 ptr) const
                                    { return (ptr_ != (PtrType *)ptr); }
  
    inline
      Int32 operator != (
        const NAVersionedObjectPtrArrayTempl<PtrType> & other) const
                                        { return (ptr_ != other.ptr_); }
  
    inline Int32 operator != (const PtrType * const ptr) const
                                               { return (ptr_ != ptr); }
  
    inline Int32 operator ! () const { return (ptr_ == (PtrType *)NULL); }

  // -------------------------------------------------------------------
  // Arithmetic operators - support pointer arithmetic.
  // -------------------------------------------------------------------
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType>
        operator +(const Int32 n) const
           { return NAVersionedObjectPtrArrayTempl<PtrType>(ptr_ + n); }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType>
        operator +(const short n) const
           { return NAVersionedObjectPtrArrayTempl<PtrType>(ptr_ + n); }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType>
        operator +(const Long n) const
           { return NAVersionedObjectPtrArrayTempl<PtrType>(ptr_ + n); }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType>
        operator +(const UInt32 n) const
           { return NAVersionedObjectPtrArrayTempl<PtrType>(ptr_ + n); }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType>
        operator +(const unsigned short n) const
           { return NAVersionedObjectPtrArrayTempl<PtrType>(ptr_ + n); }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType>
        operator +(const ULong n) const
           { return NAVersionedObjectPtrArrayTempl<PtrType>(ptr_ + n); }

    inline
      NAVersionedObjectPtrArrayTempl<PtrType>
        operator -(const Int32 n) const
           { return NAVersionedObjectPtrArrayTempl<PtrType>(ptr_ - n); }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType>
        operator -(const short n) const
           { return NAVersionedObjectPtrArrayTempl<PtrType>(ptr_ - n); }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType>
        operator -(const Long n) const
           { return NAVersionedObjectPtrArrayTempl<PtrType>(ptr_ - n); }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType>
        operator -(const UInt32 n) const
           { return NAVersionedObjectPtrArrayTempl<PtrType>(ptr_ - n); }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType>
        operator-(const unsigned short n) const
           { return NAVersionedObjectPtrArrayTempl<PtrType>(ptr_ - n); }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType>
        operator -(const ULong n) const
           { return NAVersionedObjectPtrArrayTempl<PtrType>(ptr_ - n); }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType> & operator +=(const Int32 n)
                                            { ptr_ += n; return *this; }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType> &
        operator +=(const short n)          { ptr_ += n; return *this; }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType> &
        operator +=(const Long n)           { ptr_ += n; return *this; }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType> &
        operator +=(const UInt32 n)   { ptr_ += n; return *this; }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType> &
        operator +=(const unsigned short n) { ptr_ += n; return *this; }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType> &
        operator +=(const ULong n)  { ptr_ += n; return *this; }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType> & operator -=(const Int32 n)
                                            { ptr_ -= n; return *this; }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType> &
        operator -=(const short n)          { ptr_ -= n; return *this; }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType> &
        operator -=(const Long n)           { ptr_ -= n; return *this; }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType> &
        operator -=(const UInt32 n)   { ptr_ -= n; return *this; }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType> &
        operator -=(const unsigned short n) { ptr_ -= n; return *this; }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType> &
        operator -=(const ULong n)  { ptr_ -= n; return *this; }
  
    inline NAVersionedObjectPtrArrayTempl<PtrType> & operator ++()
                                               { ++ptr_; return *this; }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType> & operator ++(const Int32 n)
                                               { ptr_++; return *this; }
  
    inline NAVersionedObjectPtrArrayTempl<PtrType> & operator --()
                                               { --ptr_; return *this; }
  
    inline
      NAVersionedObjectPtrArrayTempl<PtrType> & operator --(const Int32 n)
                                               { ptr_--; return *this; }

  // -------------------------------------------------------------------
  // Subscript operator.
  // -------------------------------------------------------------------
  inline PtrType & operator [] (const Int32 i) const
                                                     { return ptr_[i]; }
  inline PtrType & operator [] (const short i) const
                                                     { return ptr_[i]; }

  inline PtrType & operator [] (const Long i) const
                                                     { return ptr_[i]; }

  inline PtrType & operator [] (const UInt32 i) const
                                                     { return ptr_[i]; }
  inline PtrType & operator [] (const unsigned short i) const
                                                     { return ptr_[i]; }

  inline PtrType & operator [] (const ULong i) const
                                                     { return ptr_[i]; }

  // -------------------------------------------------------------------
  // Functions subclasses should redefine despite they are non-virtual
  // because we want to keep size of this class 8 bytes.
  // -------------------------------------------------------------------
  inline PtrType *operator ->() const
                                          { assert(ptr_); return ptr_; }

  // -------------------------------------------------------------------
  // Support for allocating and copying the array of pointers.
  // -------------------------------------------------------------------
  inline void allocatePtrArray(Space *space,
                                          UInt32 numEntries)
  {
    if (numEntries != 0)
    {
      ptr_ = (PtrType *)
              space->allocateAlignedSpace(numEntries * sizeof(PtrType));
    }
    else ptr_ = (PtrType *)NULL;
  }

  inline void copyPtrArray(void **ptrArray,
                                      UInt32 numEntries)
  {
    for(UInt32 i=0; i<numEntries; i++) ptr_[i] = (PtrType *)(ptrArray[i]);
  }

  inline void allocateAndCopyPtrArray(Space *space,
                                                 void **ptrArray,
                                                 UInt32 numEntries)
  {
    allocatePtrArray(space,numEntries);
    copyPtrArray(ptrArray,numEntries);
  }

  // -------------------------------------------------------------------
  // Packing and Unpacking.
  // -------------------------------------------------------------------
  inline Long packShallow(void *space, short isSpacePtr = 1)
  {
    Long offset = 0;
    if(ptr_ != 0)
    {
      if(isSpacePtr)
        offset = ((Space *)space)->convertToOffset((char *)ptr_);
      else
        offset = ((char *)space - (char *)ptr_);
      offset_ = offset;
    }
    else offset_ = 0;
    return offset;
  }

  
    inline Long pack(void *space, Lng32 numEntries, short isSpacePtr = 1)
  {
    // Pack the pointer objects in the array.
    for(Lng32 i = 0; i < numEntries; i++) ptr_[i].pack(space,isSpacePtr);

    // Convert this pointer to an offset.
    return packShallow(space,isSpacePtr);
  }

  inline Lng32 unpackShallow(void *base)
  {
    if(offset_ != 0)
    {
      ptr_ = (PtrType *)((char *)base - offset_);
    }
    return 0;
  }

  inline Lng32 unpack(void *base, Lng32 numEntries, void * reallocator)
  {
    // Convert this pointer object from offset to a real pointer.
    if(unpackShallow(base)) return -1;

    // Unpack the pointer objects in the array.
    for(Lng32 i = 0; i < numEntries; i++)
    {
      if(ptr_[i].unpack(base, reallocator)) return -1;
    }
    return 0;
  }

protected:

  // -------------------------------------------------------------------
  // private data members
  // -------------------------------------------------------------------
  union
  {
    PtrType              *ptr_;
    Int64                 offset_;
  };

};  // END of class declaration for NAVersionedObjectPtrArrayTempl -----


// ---------------------------------------------------------------------
// Define pointer to an NAVersionedObject by using its template.
// ---------------------------------------------------------------------
typedef NAVersionedObjectPtrTempl<NAVersionedObject> NAVersionedObjectPtr;


// ---------------------------------------------------------------------
// Class NAVersionedObject
// ---------------------------------------------------------------------

class NAVersionedObject
{
public:

  // -------------------------------------------------------------------
  // public member functions
  // -------------------------------------------------------------------

  // -------------------------------------------------------------------
  // Constructors
  //
  // Note that imageSize_ and versionIDArray_ are only filled in at
  // packing time. These values are not useful on the source platform
  // where the objects are constructed. They are only useful at the
  // destination while the objects are unpacked. Before the objects are
  // packed, drivePack() will make sure their values are correctly set.
  // This arrangement avoids the dependence on the constructors of sub-
  // classes of NAVersionedObject on setting these values correctly.
  // -------------------------------------------------------------------
  NAVersionedObject(Int16 classID = -1);


  inline void init(Int16 classID = -1)
  {
    classID_ = classID;
    populateImageVersionIDArray();
    imageSize_ = getClassSize();
    clearFillers();
    str_cpy_all(eyeCatcher_,VOBJ_EYE_CATCHER,VOBJ_EYE_CATCHER_SIZE);
    initFlags();
    reallocatedAddress_ = (NAVersionedObjectPtr) NULL ;
  }

  // -------------------------------------------------------------------
  // Utility to toggle the endianness of all applicable members
  // -------------------------------------------------------------------
  inline void toggleEndiannessOfVersionHeader()
  {
    swapInt16(&classID_);
    swapInt16(&imageSize_);
  
    // -----------------------------------------------------------------
    // Theoretically, if the object has been packed, reallocatedAddress_
    // has been changed into an offset. Thus, we should also toggle its
    // endianness. In reality, however, reallocatedAddress_ is only set
    // during unpacking. It should always be NULL when the object is in
    // a "packed" state.
    // -----------------------------------------------------------------
    if (isPacked()) assert(reallocatedAddress_.isNull());
  }

  // -------------------------------------------------------------------
  // reallocateImage() provides the basic implementation for the virtual
  // function migrateToNewVersion(). It is called when the new version
  // object has a larger size than the older version object image we
  // have. A new object is allocated. The old image is overlay onto it.
  // The left-over space will be zero'ed. Finally, reallocatedAddress_
  // field in the older object is set to the address of the new object.
  // -------------------------------------------------------------------
  NAVersionedObject *reallocateImage(void * reallocator);

  // -------------------------------------------------------------------
  // Accessors and Mutators
  // -------------------------------------------------------------------
  inline Int16 getClassID() const        { return classID_; }
  inline void setClassID(Int16 classID) 
                                                 { classID_ = classID; }

  inline unsigned char * getImageVersionIDArray()
                                             { return versionIDArray_; }

  inline unsigned char getImageVersionID(unsigned short ix) const
  {
    assert(ix < VERSION_ID_ARRAY_SIZE);
    // add the following to prevent false alarm on "ix"
    // without considering the above assert
    // coverity[overrun_local]
    return versionIDArray_[ix];
  }

  void setImageVersionID(unsigned short ix,
                                           unsigned char versionID)
  {
    assert(ix < VERSION_ID_ARRAY_SIZE);
    // add the following to prevent false alarm on "ix"
    // without considering the above assert
    // coverity[overrun_local]
    versionIDArray_[ix] = versionID;
  }

  inline NAVersionedObjectPtr getReallocatedAddress() const
                                         { return reallocatedAddress_; }
  inline void setReallocatedAddress(
                                           NAVersionedObjectPtr address)
                                      { reallocatedAddress_ = address; }
  inline void setReallocatedAddress(
                                             NAVersionedObject *address)
                                      { reallocatedAddress_ = address; }

  inline Int16 getImageSize() const    { return imageSize_; }
  inline void setImageSize(Int16 size) { imageSize_ = size; }

  inline void setVTblPtr(char *ptr)
  {
    *((char **)this) = ptr;
  }

  inline NABoolean isPacked() const
                            { return (flags_ & VOBJ_PACKED) != 0; }
  inline void markAsPacked()
                                    { flags_ |= VOBJ_PACKED; }
  inline void markAsNotPacked()
                                   { flags_ &= ~VOBJ_PACKED; }

  inline NABoolean isBigEndian() const
                        { return (flags_ & VOBJ_BIG_ENDIAN) != 0; }
  inline void markAsBigEndian()
                                { flags_ |= VOBJ_BIG_ENDIAN; }
  inline void markAsLittleEndian()
                               { flags_ &= ~VOBJ_BIG_ENDIAN; }

  
    NABoolean isSpacePtr() { return (flags_ & IS_SPACE_PTR) != 0; }
  
    void setIsSpacePtr(NABoolean v) 
    { (v ? flags_ |= IS_SPACE_PTR : flags_ &= ~IS_SPACE_PTR); }

  // -------------------------------------------------------------------
  // Virtual functions subclasses could redefine
  // -------------------------------------------------------------------

  // -------------------------------------------------------------------
  // Subclass MUST redefine this method to return the version number of
  // the specific subclass. This number should be consistent with the
  // number supplied to the constructor when an object is constructed.
  // -------------------------------------------------------------------
  virtual unsigned char getClassVersionID() = 0;
  
  // -------------------------------------------------------------------
  // Subclass MUST redefine this method to set its class version ID in
  // the object's version ID array and then call the same function on
  // its base class. The index of the array to use depends on the depth
  // of the sub-class in the class hierarchy. For example, direct sub-
  // classes of NAVersionedObject should set versionIDArray_[0], a
  // direct subclass of those subclasses should set versionIDArray_[1]
  // and so on...
  // -------------------------------------------------------------------
  virtual void populateImageVersionIDArray() = 0;
  
  // -------------------------------------------------------------------
  // An immediate subclass (aka the Anchor class) MUST redefine this
  // method to return the virtual table function pointer of the subclass
  // under it with the class ID given.
  // -------------------------------------------------------------------
  virtual char *findVTblPtr(Int16 classID)
  {
    // -----------------------------------------------------------------
    // If subclass doesn't redefine this method, return the virtual
    // function table pointer of this object.
    // -----------------------------------------------------------------

    // classID should be set to -1 if reached here.
    if (classID != -1)
      return NULL;

    // assert(classID == -1);
    // on NT or Unix the vptr is stored in the first four bytes of the object
    return *((char **)this);
  }

  // -------------------------------------------------------------------
  // All subclasses MUST redefine this method to return the correct
  // object sizes.
  // -------------------------------------------------------------------
  virtual short getClassSize()
  { return (short)sizeof(NAVersionedObject); }

  // -------------------------------------------------------------------
  // All subclasses MUST redefine toggleEndianness() to toggle the
  // endianness of all of their members and call toggleEndianness() on
  // the base class. Note that toggling of the endianness of the Version
  // Header (members of NAVersionedObject) is handled separately by
  // NAVersionedObject::toggleEndiannessOfVersionHeader().
  // -------------------------------------------------------------------
  virtual void toggleEndianness()                          {}

  // -------------------------------------------------------------------
  // All subclasses could redefine convertToReference/LocalPlatform() to 
  // convert their members to the reference platform from the local
  // platform and vice versa. Typically, this only involves toggling the
  // endianness of some members.
  // -------------------------------------------------------------------
  virtual void convertToReferencePlatform();
  virtual void convertToLocalPlatform();

  // -------------------------------------------------------------------
  // Subclasses could redefine this method to provide a migration path
  // from an older object (versionID as in its versionIDArray_) to an
  // object of the current version by initializing the new members added 
  // due to the upgrade to their appropriate values so that the object
  // can be understood properly by the new executable.
  // -------------------------------------------------------------------
  virtual void initNewMembers()                            {}

  // -------------------------------------------------------------------
  // Subclasses MUST redefine pack() to drive the packing of objects
  // referenced by their members which are pointers and convert them to
  // offsets by calling NAVersionedObjectPtr::pack(). They should also
  // convert all their data members to the endianness of the reference
  // platform. Note that endianness of the Version Header is handled
  // separately in drivePack().
  // -------------------------------------------------------------------
  virtual Long pack(void *space)
    { 
      if (isSpacePtr())
	return ((Space *)space)->convertToOffset((char *)this); 
      else
	return (Long((char *)space - (char *)this));
    }


  // -------------------------------------------------------------------
  // Subclasses MUST redefine unpack() to handle the conversions of all
  // offsets back to pointers and the subsequent unpacking of objects
  // referenced by those pointers. This could be handled by calling
  // NAVersionedObjectPtr::unpack() on the pointers. They should also
  // convert all their data members to the endianness of the local
  // platform. Note that endianness of the Version Header is handled
  // separately in driveUnpack().
  // -------------------------------------------------------------------
  virtual Lng32 unpack(void *base, void * reallocator)               { return 0; }

  // -------------------------------------------------------------------
  // This is a utility for use by redefined migrateToNewVersion() at the
  // subclass level. Given the old class size and the new class size, it
  // expands the room for members of a particular subclass inside an
  // image of possibly another subclass down the derivation chain. It
  // assumes the image has been reallocated so that it is big enough to
  // make this expansion.
  // -------------------------------------------------------------------
  void makeRoomForNewVersion(Int16 oldSubClassSize,
                                        Int16 newSubClassSize);

  // -------------------------------------------------------------------
  // Subclasses could redefine migrateToNewVersion() when a new version
  // is introduced according to the following template:
  //
  // long SubClass::migrateToNewVersion(NAVersionedObject *&newImage)
  // {
  //   if (newImage == NULL)
  //   {
  //     newImage = ( getImageSize() == getClassSize() ?
  //                  this :
  //                  reallocateNewImage() );
  //   }
  //
  //   // Version not supported when migrating base class.
  //   if (BaseClass::migrateToNewVersion(newImage)) return -1;
  //
  //   // ?n is the current version of SubClass. ?cs1 is the class size
  //   // of version 1, ?cs2 is the class size of version 2, ..., etc.
  //   //
  //   const short classSizesArray[?n] = { ?cs1, ?cs2, ..., ?csn };
  //
  //   short newClassSize = SubClass::getClassSize(); // or ?csn
  //
  //   // ?SUBCLASS_LEVEL begins with 0 if SubClass is directly derived
  //   // from NAVersionedObject and increases down the derivation chain.
  //   //
  //   unsigned char version = getImageVersionID(?SUBCLASS_LEVEL);
  //   short oldClassSize = classSizesArray[version];
  //
  //   if (oldClassSize != newClassSize)
  //     makeRoomForNewVersion(oldClassSize,newClassSize);
  //
  //   // Implement migration of old members other than size difference OR
  //   // return -1 if it was decided that a particular version shouldn't
  //   // be supported anymore. Note that new members should be initialized
  //   // only later at initNewMembers().
  //   //
  //   switch (version)
  //   {
  //     // provides code to migrate image from version 1 to 2.
  //     case 1:
  //     // provides code to migrate image from version 2 to 3.
  //     case 2:
  //     // ... upto version ?(n-1) to ?n.
  //   };
  //
  //   return 0;
  // }
  //
  // This method is redefined by following a strategy similar to RelExpr::
  // copyTopNode() in the optimizer directory. Each subclass invokes the
  // same method on its base class and then handles the migration of its
  // own members. The object is reallocated if needed at the "real" sub-
  // class the object belongs.
  // -------------------------------------------------------------------
  virtual Lng32 migrateToNewVersion(NAVersionedObject *&newImage);

  // -------------------------------------------------------------------
  // Driver for Packing
  //
  // Should return a 64 bit integer on a 64 bit platform. Could be fixed
  // later when 64-bit platforms are really available since it doesn't
  // affect object layout.
  // -------------------------------------------------------------------
  Long drivePack(void *space, short isSpacePtr = 1);

  // -------------------------------------------------------------------
  // Driver for Unpacking
  //
  // In a nutshell, unpacking consists of the following major steps:
  //
  // 1. fix the endianness of the version header (members of this class)
  // 2. fix up the virtual table function pointer for the object
  // 3. migrate an object of a previous version to the current version
  // 4. fix the endianness of all other members in the subclasses
  // 5. convert pointers in this object from offsets back to addresses
  // 5. initiate unpacking for other objects referenced by this object
  // 6. initialize new members added in the new version
  //
  // -------------------------------------------------------------------
  NAVersionedObject *driveUnpack(void *base, char *vtblPtr, 
					    void * reallocator);
  NAVersionedObject *driveUnpack(void *base,
					    NAVersionedObject *ptrToAnchorClass,
					    void * reallocator);

private:

// ---------------------------------------------------------------------
// Bit patterns defined for use in *(char *)(&NAVersionedObject::flags_)
// ---------------------------------------------------------------------
  enum {VOBJ_PACKED = 0x0080, VOBJ_BIG_ENDIAN = 0x0040,
        IS_SPACE_PTR = 0x0020};

  // -------------------------------------------------------------------
  // private member functions
  // -------------------------------------------------------------------

  inline void clearFillers()
  {
  }

  inline void clearVersionIDArray()
  {
    memset(versionIDArray_, 0, VERSION_ID_ARRAY_SIZE);
  }

  inline void copyFlags(const NAVersionedObject & obj)
  {
    flags_ = obj.flags_;
  }

  inline void initFlags()
  {
    flags_ = 0;
    markAsNotPacked();
#ifndef NA_LITTLE_ENDIAN
    markAsBigEndian();
#else
    markAsLittleEndian();
#endif
  }

  inline void copyVersionIDArray(unsigned char *versionIDArray)
  {
    str_cpy_all((char *)versionIDArray_,(char *)versionIDArray,VERSION_ID_ARRAY_SIZE);
  }

  inline void copyVersionIDArray(const NAVersionedObject & obj)
  {
    str_cpy_all((char*)versionIDArray_, (char*)obj.versionIDArray_,
                 VERSION_ID_ARRAY_SIZE);
  }

  // -------------------------------------------------------------------
  // private data members
  // -------------------------------------------------------------------

  char                  eyeCatcher_[2];                         // 08-09
  Int16                 classID_;                               // 10-11
  Int16                 imageSize_;                             // 12-13
  Int16                 flags_;                                 // 14-15
  unsigned char         versionIDArray_[VERSION_ID_ARRAY_SIZE]; // 16-23

  NAVersionedObjectPtr  reallocatedAddress_;                    // 24-31

};  // END of class declaration for NAVersionedObject ------------------



#endif  // ---------------------------------------------------- EOF ----


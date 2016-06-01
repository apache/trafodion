// ***********************************************************************
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
// ***********************************************************************
// -*-C++-*-
// ***********************************************************************
//
// File:         SharedPtrCollections.h
// Description:  Provide classes that properly handle SharedPtr classes
//               within NAList and NAArray collections.  Some functions
//               must be overridden to properly handle the reference
//               counting mechanism in the SharedPtr class.
//
// Created:      04/04/07
// Language:     C++
// ***********************************************************************
#ifndef _SHAREDPTR_COLLECTIONS_H
#define _SHAREDPTR_COLLECTIONS_H

#include "SharedPtr.h"
#include "Collections.h"

// Define shared pointer list and array types in a simliar way
// to the current LIST and ARRAY.
#define SHPTR_LIST(Type)   NAShPtrList<Type>
#define SHPTR_ARRAY(Type)  NAShPtrArray<Type>

// ***********************************************************************
// NAShPtrList class
//  Description:  A specialization of the NAList (LIST) class .  This
//                class just overrides some of the behavior to work
//                correctly with shared pointers.
// ***********************************************************************
template <class T>
class NAShPtrList : public NAList<T> {
public:
  NAShPtrList(CollIndex initLen = 0) : NAList<T>(initLen) {}
  NAShPtrList(CollHeap * heap, CollIndex initLen = 0) 
        : NAList<T>(heap, initLen) {}
  NAShPtrList(const NAShPtrList<T> &other, CollHeap *heap=0)
        : NAList<T>(other, heap) {}

  // The destructor in the Collections base class calls deallocate(),
  // but will call the Collections deallocate() instead of the
  // one defined here.  It is necessary to call decrAllRefCount()
  // here to insure the shared pointers decrement their reference
  // counts.
  ~NAShPtrList() { decrAllRefCount(); }

  // Reduce the reference count on all members.
  void decrAllRefCount() {
    CollIndex maxSize = NACollection<T>::getSize();
    for (CollIndex i = 0; i < maxSize; i++) {
      if (NACollection<T>::getUsage(i) != UNUSED_COLL_ENTRY)
        this->usedEntry(i) = 0;
    }
  }

  // Clear the list of smart pointers (but don't deallocate memory).
  void clear() {
    decrAllRefCount();
    NAList<T>::clear();
  }

  // Deallocate the list of smart pointers.  This is not meant to be used
  // by external users of the NAShPtrList class.
  virtual void deallocate() {
    decrAllRefCount();
    NACollection<T>::deallocate();
  }

  // Decrement the reference count and remove a particular element
  // in the list
  NABoolean removeAt(const CollIndex index) {
    if (index >= NAList<T>::entries())
      return FALSE;

    (*this)[index] = 0;
    return NAList<T>::removeAt(index);
  }
};

// ***********************************************************************
// NAShPtrArray class
//  Description:  A specialization of the NAArray (ARRAY) class .  This
//                class just overrides some of the behavior to work
//                correctly with shared pointers.
// ***********************************************************************
template <class T>
class NAShPtrArray : public NAArray<T> {
public:
  NAShPtrArray(CollIndex initLen = 0) : NAArray<T>(initLen) {}
  NAShPtrArray(CollHeap * heap, CollIndex initLen = 0) 
        : NAArray<T>(heap, initLen) {}
  NAShPtrArray(const NAShPtrArray<T> &other, CollHeap *heap=0)
        : NAArray<T>(other, heap) {}

  // The destructor in the Collections base class calls deallocate(),
  // but will call the Collections deallocate() instead of the
  // one defined here.  It is necessary to call decrAllRefCount()
  // here to insure the shared pointers decrement their reference
  // counts.
  ~NAShPtrArray() { decrAllRefCount(); }

  // Reduce the reference count on all members.
  void decrAllRefCount() {
    CollIndex maxSize = NACollection<T>::getSize();
    for (CollIndex i = 0; i < maxSize; i++) {
      if (NAArray<T>::used(i))
        this->usedEntry(i) = 0;
    }
  }

  // Clear the array of smart pointers (but don't deallocate memory)
  void clear() {
    decrAllRefCount();
    NAArray<T>::clear();
  }

  // Deallocate the array of smart pointers.  This is not meant to be called
  // by users of the NAShPtrArray class.
  virtual void deallocate() {
    decrAllRefCount();
    NACollection<T>::deallocate();
  }

  // Remove an entry from the NAShPtrArray object at a particular index.
  NABoolean remove(CollIndex index) {
    if (NAArray<T>::used(index)) {
      this->usedEntry(index) = 0;
      NACollection<T>::remove(index);
      return TRUE;
    } else {
      return FALSE;
    }
  }
};

#endif // _SHAREDPTR_COLLECTIONS_H

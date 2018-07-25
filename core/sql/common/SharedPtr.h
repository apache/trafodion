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
// File:         SharedPtr.h
// Description:  A shared pointer implementation that works with the
//               NAMemory class and supports some of the programming
//               styles used by the SQLMX code. Standard shared_ptr
//               classes cannot be used for the compiler (or executor)
//               because of the memory management used by the NAMemory
//               class.  Standard implementations use new/delete and
//               this would cause memory leaks when the NAMemory heaps
//               are reinitialized or are destructed.
//
// Created:      04/04/07
// Language:     C++
// ***********************************************************************
#ifndef _SHAREDPTR_H
#define _SHAREDPTR_H

// ***********************************************************************
// USAGE NOTES:
//    The SharedPtr class should be used in most cases.  However, if a
//    regular pointer needs to be converted into a SharedPtr, then an
//    IntrusiveSharedPtr should be used.  An example is if you are in
//    a member function of a particular class, and you need to pass the
//    "this" pointer to something that requires a SharedPtr, then you
//    most likely need to be using an IntrusiveSharedPtr.
//
//    A SharedPtr allocates memory externally for a reference count, while
//    an IntrusiveSharedPtr places the reference count within the user
//    object itself.  This means that a heap must be passed to the
//    SharedPtr constructor, but does not need to be passed to the
//    IntrusiveSharedPtr constructor. IntrusiveSharedPtr user classes
//    will have a "INTRUSIVE_SHARED_PTR(class);" declaration included
//    in the public section of the class.  Failure to do this will result
//    in a compile-time error.
//
//    A good practice to follow is to specify a typedef for a new
//    SharedPtr.  Each place that allocates a user object will need some
//    syntax changes.
//
// EXAMPLE:
//    #include "SharedPtr.h"
//    #include "NAMemory.h"
//    #include "iostream"
//    class MyClass {
//      public:
//         MyClass(int a) : a_(a) {}
//         int getVal() { return a_; }
//      private:
//         int a_;
//    };
//    typedef SharedPtr<MyClass> MyClassSharedPtr;
//
//    int myFunction(CollHeap* heap)
//    {
//      // This allocates a MyClass object, which is pointed to by the
//      // "mcp" SharedPtr.  The internal reference count will be 1
//      // after "mcp" is constructed
//      MyClassSharedPtr mcp(new (heap) MyClass(5), heap);
//
//      // This shows that "mcp" can be used just like a regular pointer
//      if (mcp) {
//        std::cout << "Value of mcp " << mcp->getVal() << std::end;
//      }
//
//      // The "MyClass" object will be automatically destroyed once
//      // "mcp" goes out of scope.
//    }
// ***********************************************************************


#include "NAMemory.h"
#include "NAAssert.h"

// Classes and structs defined in this file.
struct nullDeleter;
template <class T> struct SharedRefCountBase;
template <class T> struct SharedRefCount;
template <class T, class D> struct SharedRefCountDel;
template <class T> struct IntrusiveSharedRefCount;
template <class T> class SharedPtr;
template <class T> class IntrusiveSharedPtr;

// Classes that must use an embedded shared count must declare 
// INTRUSIVE_SHARED_PTR in the public section of the class.
#define INTRUSIVE_SHARED_PTR(TYPE) IntrusiveSharedRefCount<TYPE> ISHP_

// ***********************************************************************
// NullDeleter struct
//  Description: This may be used when constructing a SharedPtr object
//               that references an object that should not be deleted.
//               For instance, objects constructed on the stack should
//               not be deleted.
// ***********************************************************************
struct NullDeleter {
  void operator() (void const *) const {}
};

// ***********************************************************************
// SharedRefCountBase struct
//  Description: This provides the base class that the shared reference
//               counting classes may use.
// ***********************************************************************
template <class T>
struct SharedRefCountBase {
  SharedRefCountBase() : useCount_(0), objectP_(0) {}

  SharedRefCountBase(T* t, Int32 use_count) :
    useCount_(use_count), objectP_(t) {}

  // Functions to increment and decrement the use count
  void incrUseCount() { ++useCount_; }
  void decrUseCount() { if (--useCount_ == 0) destroyObjects(); }

  // Function that properly destroys the object and reference count
  // objects.
  virtual void destroyObjects() = 0;

  Lng32 useCount_;     // Reference count of this object
  T *objectP_;        // Pointer to object
};

// ***********************************************************************
// SharedRefCount struct
//  Description: This struct provides shared reference counting that
//               is used by the SharedPtr class.
// ***********************************************************************
template <class T>
struct SharedRefCount : public SharedRefCountBase<T> {

  SharedRefCount(T* t, NAMemory* heap, Int32 use_count) :
         SharedRefCountBase<T>(t, use_count), heap_(heap) {}

  virtual void destroyObjects()
  {
    delete SharedRefCountBase<T>::objectP_;
    if (heap_)
      NADELETEBASIC(this, heap_);
    else
      ::operator delete((void*)this);
  }

private:
  NAMemory *heap_;         // Heap used to construct SharedRefCount.
};

// ***********************************************************************
// SharedRefCountDel class
//  Description: This provides a reference counted object with a
//       specialized delete function.  It has two main purposes:
//          1) Used in conjuntion with nullDeleter to prevent
//             objects on the stack from being deleted when used in
//             conjunction with the SharedPtr class.
//          2) Provide a special deleter class if some special
//             deletion is necessary.
// ***********************************************************************
template<class T, class D>
struct SharedRefCountDel : public SharedRefCountBase<T> {

  SharedRefCountDel(T *t, NAMemory *heap, Int32 use_count, D deleter) :
     SharedRefCountBase<T>(t, use_count), heap_(heap), del_(deleter) {}

  virtual void destroyObjects()
  {
    del_(SharedRefCountBase<T>::objectP_);
    if (heap_)
     // heap_->deallocateMemory(this);
      NADELETEBASIC(this, heap_);
    else
      ::operator delete((void*)this);
  }

private:
  NAMemory *heap_;     // Heap used to construct SharedRefCount.
  D del_;              // Deletion class
};

// ***********************************************************************
// IntrusiveSharedRefCount struct
//  Description: This provides the ability for a class to contain the
//               reference count instead of it being allocated externally.
//               This allows a SharedPtr object to be created correctly
//               from the "this" pointer.
//
//  Usage:       An object that uses this mechanism must add the following
//               to the public section of the object (where objectType is
//               the class name):
//
//                  CMP_INTRUSIVE_SHARED_PTR(objectType);
//
// ***********************************************************************
template<class T>
struct IntrusiveSharedRefCount : public SharedRefCountBase<T> {

  IntrusiveSharedRefCount() {}

  IntrusiveSharedRefCount(T *t, Int32 use_count) :
      SharedRefCountBase<T>(t, use_count) {}

  virtual void destroyObjects()
  {
    // Only the object pointer should be deleted for this class.
    delete SharedRefCountBase<T>::objectP_;
  }
};

// ***********************************************************************
// SharedPtr class
//  Description: This provides a reference counted smart pointer
//               implementation.  For SQL/MX, it is assumed that objects
//               that use this class will be derived from classes that
//               provide their own operator new and operator delete.
// ***********************************************************************
template<class T>
class SharedPtr {
public:
  // Constructors
  SharedPtr(): objectP_(0), refCount_(0) {}

  // Normal constructor
  template<class Y>
  SharedPtr(Y *t, NAMemory *heap) : objectP_(t) {
    if (heap)
      refCount_ = new (heap) SharedRefCount<Y> (t, heap, 1);
    else
      refCount_ = new SharedRefCount<Y> (t, heap, 1);
  }

  // Constructor with a special deleter object
  template<class Y, class D>
  SharedPtr(Y *t, NAMemory *heap, D deleter_class) : objectP_(t) {
    if (heap)
      refCount_ = new (heap) SharedRefCountDel<Y, D> (t, heap, 1, deleter_class);
    else
      refCount_ = new SharedRefCountDel<Y, D> (t, heap, 1, deleter_class);
  }

  // Constructor that allows construction with a NULL pointer
  SharedPtr(const long i) : refCount_(0), objectP_((T*)i) {}

  // Copy constructor
  SharedPtr(SharedPtr<T> const & r) :
     objectP_(r.objectP_),
     refCount_(r.refCount_)
  {
    if (refCount_)
      refCount_->incrUseCount();
  }

  // Destructor
  ~SharedPtr()
  {
    if (refCount_) {
#ifdef _DEBUG
      assert(objectP_ == refCount_->objectP_);
#endif
      refCount_->decrUseCount();
    }
  }

  // Assignment operator
  SharedPtr<T>& operator= (SharedPtr<T> const & r)
  {
    if (objectP_ != r.objectP_) {
      if (refCount_) {
#ifdef _DEBUG
        assert(objectP_ == refCount_->objectP_);
#endif
        refCount_->decrUseCount();
      }
      objectP_ = r.objectP_;
      refCount_ = r.refCount_;
      if (refCount_)
        refCount_->incrUseCount();
    }
    return *this;
  }

  // Allow assignment of pointer to NULL.
  SharedPtr<T>& operator= (const long i)
  {
    if (refCount_) {
#ifdef _DEBUG
      assert(objectP_ == refCount_->objectP_);
#endif
      refCount_->decrUseCount();
    }
    objectP_ = (T*)i;
    refCount_ = 0;
    return *this;
  }

  // Access operations
  T& operator* () const
  {
#ifdef _DEBUG
    if (refCount_)
      assert(objectP_ == refCount_->objectP_);
#endif
    return *objectP_;
  }

  T* operator->() const
  {
#ifdef _DEBUG
    if (refCount_)
      assert(objectP_ == refCount_->objectP_);
#endif
    return objectP_;
  }

  // Return a pointer to the object.
  T* get() const { return objectP_; }

  // Return the reference count
  Lng32 getUseCount() const { return (refCount_ ? refCount_->useCount_ : 0); }

  // Return whether this SharePtr is not null.
  operator bool () const { return objectP_ != 0; }

  // reset() may be used for special cases where it is possible to
  // tell that a SharedPtr is no longer valid.  The current SharedPtr
  // can then be reset to point to nothing.
  void reset() { objectP_ = 0; refCount_ = 0; }

protected:
  T* objectP_;                       // Pointer to contained object
  SharedRefCountBase<T>* refCount_;  // Pointer to reference count object
};

// Equality operator
template<class T, class O>
inline bool operator==(SharedPtr<T> const &a, SharedPtr<O> const &b)
{
  return a.get() == b.get();
}

// Inequality operator
template<class T, class O>
inline bool operator!=(SharedPtr<T> const &a, SharedPtr<O> const &b)
{
  return a.get() != b.get();
}

// ***********************************************************************
// IntrusiveSharedPtr class
//  Description: This is a specialized SharedPtr in which the object
//               contains the reference count instead of it being
//               allocated externally.  This allows for a shared pointer
//               to be allocated from an existing object pointer.
// ***********************************************************************
template<class T>
class IntrusiveSharedPtr : public SharedPtr<T> {
public:
  // Constructors
  IntrusiveSharedPtr() {}

  // Normal constructor
  template<class Y>
  IntrusiveSharedPtr(Y *t)
  {
    // Reconstruct the reference count from object
    SharedPtr<T>::objectP_ = (T*)t;
    SharedPtr<T>::refCount_ = (SharedRefCountBase<T>*)&t->ISHP_;
    SharedPtr<T>::refCount_->objectP_ = (T*)t;
    SharedPtr<T>::refCount_->useCount_++;
  }

  // Allow construction with a NULL
  IntrusiveSharedPtr(const long i) : SharedPtr<T>(i) {} 

  // Return a pointer to a shared pointer given an object with an intrusive
  // shared pointer.
  static IntrusiveSharedPtr<T> getIntrusiveSharedPtr(const T *t)
  {
    IntrusiveSharedPtr retPtr;

    // Copy information to the returned shared pointer.  The casts done
    // below are to get rid of "const" errors.
    retPtr.refCount_ = (SharedRefCountBase<T>*)&t->ISHP_;
    retPtr.objectP_ = (T*)t;
    retPtr.refCount_->objectP_ = (T*)t;
    retPtr.refCount_->useCount_++;
    return retPtr;
  }

  // Provide an assignment operator to prevent the compiler from writing one.
  IntrusiveSharedPtr<T>& operator= (const long i)
  {
    (void)SharedPtr<T>::operator=(i);
    return *this;
  }
};

// Equality operator (for comparing an intrusive pointer to a regular pointer)
template<class T>
inline bool operator==(IntrusiveSharedPtr<T> const &a, T const *b)
{
  return a.get() == b;
}

// Inequality operator (for comparing an intrusive pointer to a regular pointer)
template<class T>
inline bool operator!=(IntrusiveSharedPtr<T> const &a, T const *b)
{
  return a.get() != b;
}

#endif // _SHAREDPTR_H

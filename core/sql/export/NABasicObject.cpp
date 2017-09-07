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
* File:         NABasicObject.cpp (previously under /common)
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


#ifdef NA_STD_NAMESPACE
#include <iosfwd>
using namespace std;
#else
#include <iostream>
#endif
#include <stdlib.h>

#include "ComASSERT.h"
#include "NAMemory.h"
#include "HeapLog.h"


//#include <cextdecs (DEBUG)>

// -----------------------------------------------------------------------
// Methods for NABasicObject
// -----------------------------------------------------------------------

// Here we cannot initialize the heap pointer because that would overwrite
// the value from the "operator new" below.
// Stack or statically allocated objects will just have random garbage in
// this field but as long as noone tries to "delete" them (as noone should)
// that will not matter (except to the Purify tool which may report UMR errors).
NABasicObject::NABasicObject() {}

// Note that this just ignores the passed object, which is correct:
// we do *not* want to copy its heap pointer!
NABasicObject::NABasicObject(const NABasicObject&) {}

// Note that this just ignores the passed object, which is correct:
// we do *not* want to copy its heap pointer!
// Otherwise, a default operator= on a NABasicObject-derived class would 
// overwrite the heap pointer, a very bad thing.
NABasicObject& NABasicObject::operator=(const NABasicObject&) { return *this; }

void* NABasicObject::operator new(size_t t, CollHeap* h, NABoolean failureIsFatal) 
{
  if (t < sizeof(NABasicObject))
    t = sizeof(NABasicObject);			// always allocate some size.

  void* p;

   if (NOT_CHECK_NAHEAP(h))
    {
      // This is asserted only because the check in 'operator delete' below
      // depends on it -- but that check is rather half-baked anyway.
      // So this is ComDEBUG (no-op if Release build) rather than ComABORT.
      ComDEBUG(h != invalidHeapPtr());
      p = h->allocateMemory(t, failureIsFatal);
    }
  else
    {
#ifdef NA_LEAK_DETECTION
      get_caller_location();
#endif


      p = ::operator new(t);
#pragma nowarn(1506)   // warning elimination 
      //      HEAPLOG_ADD_ENTRY(p, t, NA_HEAP_BASIC, "NABasic Heap")
#pragma warn(1506)  // warning elimination 
    }

  if (p)
    ((NABasicObject*)p)->h_ = h;
  else
    {
      if (failureIsFatal)
        {
          ComABORT(p);
        }
    }

  return p;
}

void* NABasicObject::operator new(size_t t, void* loc, NAMemory* h) 
{
  if (loc != NULL)
  {
    ((NABasicObject*)loc)->h_ = h;
    return loc;
  }
  return 0;
}
void NABasicObject::operator delete(void* p)
{
  if (p)	// "delete NULL;" is legal in C++, obviously a no-op
    {
      CollHeap* h = ((NABasicObject*)p)->h_;

      // ComDEBUG(h != invalidHeapPtr());
      if (h == invalidHeapPtr())
        {
	  #ifndef NDEBUG
	    cerr << "**WARNING: " << __FILE__ << ": "
	         << "Ignoring attempt to delete pointer twice    "
	         << "(possible memory leak at " << p << "; need to run Purify)"
		 << endl;
	  #endif
	  return;
	}

      ((NABasicObject*)p)->h_ = invalidHeapPtr();
      if (NOT_CHECK_NAHEAP(h))
	h->deallocateMemory(p);
      else
	{
	  //	  HEAPLOG_DELETE_ENTRY(p, NA_HEAP_BASIC)
	  ::operator delete(p);
	}
    }
}


void NABasicObject::operator delete(void* p, NAMemory*h, NABoolean f)
{
   if (p)
   {
      if (NOT_CHECK_NAHEAP(h))
         h->deallocateMemory(p);
      else
	  ::operator delete(p);
   }
}

void NABasicObject::operator delete(void* p, void*, NAMemory*)
{
  // This shouldn't do anything.
}

void* NABasicObject::operator new[](size_t t, CollHeap* h, NABoolean failureIsFatal) 
{
  if (t < sizeof(NABasicObject))
    t = sizeof(NABasicObject);			// always allocate some size.

  void* p;

    if (NOT_CHECK_NAHEAP(h))
    {
      // This is asserted only because the check in 'operator delete' below
      // depends on it -- but that check is rather half-baked anyway.
      // So this is ComDEBUG (no-op if Release build) rather than ComABORT.
      ComDEBUG(h != invalidHeapPtr());
      p = h->allocateMemory(t, failureIsFatal);
    }
  else
    {

#ifdef NA_LEAK_DETECTION
    get_caller_location();
#endif
    


      p = ::operator new(t);
#pragma nowarn(1506)   // warning elimination 
      //      HEAPLOG_ADD_ENTRY(p, t, NA_HEAP_BASIC, "NABasic Heap")
#pragma warn(1506)  // warning elimination 
    }

  if (p)
    ((NABasicObject*)p)->h_ = h;
  else
    {
      if (failureIsFatal)
        {
          ComABORT(p);
        }
    }

  return p;
}

void NABasicObject::operator delete[](void* p)
{
#if 0
  if (p)	// "delete NULL;" is legal in C++, obviously a no-op
    {
      //NGG      CollHeap* h = ((NABasicObject*)p)->h_;
      NAHeap* h = (NAHeap *) ((NABasicObject*)p)->h_;

      // ComDEBUG(h != invalidHeapPtr());
      if (h == invalidHeapPtr())
        {
	  #ifndef NDEBUG
	    cerr << "**WARNING: " << __FILE__ << ": "
	         << "Ignoring attempt to delete pointer twice    "
	         << "(possible memory leak at " << p << "; need to run Purify)"
		 << endl;
	  #endif
	  return;
	}

      ((NABasicObject*)p)->h_ = invalidHeapPtr();
      if (h)
	h->deallocateMemory(p);
      else
	{
	  //	  HEAPLOG_DELETE_ENTRY(p, NA_HEAP_BASIC)
	  ::operator delete(p);
	}
    }
#endif
	  ::operator delete(p);

}

// This function will be called if the constructor throws an exception.
void NABasicObject::operator delete[](void* p,  NAMemory* h, NABoolean)
{
   if ( p ) {
      if (NOT_CHECK_NAHEAP(h))
        h->deallocateMemory(p);
     else
        ::operator delete(p);
   }
}

//
//void* operator new[](size_t, NAMemory* h, NABoolean failureIsFatal)
//{
//return NABasicObject::operator new(t, h, failureIsFatal);
//}

//void* NABasicObject::operator delete[](void* p /* , another arg; see C++ ARM*/)
//{
// We need to call multiple destructors here, one for each object in
// the array, and then deallocate ...
//   NABasicObject::operator delete(p);	/*won't work, no dtors called*/
//}

// Calls to this debugging method have been put into the getPtr methods in the
// optimizer; further bugproofing calls could be placed elsewhere.
//
// To debug, set a breakpoint at the "return -1", run the program, and see where
// you are when the invalid dereference occurs and what object type it is (was).
// You can then set a breakpoint in that object class's destructor to see when
// that particular instance is being destroyed and being marked invalid/free.
// (Although not actually being freed by deallocateMemory() yet -- which is why 
// Purify can't find this "FMR" error, and also why your code continues to run 
// okay FOR NOW; it may not on a future platform or future deallocation scheme.
// This finds latent bugs.)

#if !defined(NDEBUG)
  Int32 NABasicObject::checkInvalidObject(const void* const referencingObject)
  {
    if (this && maybeInvalidObject())
      {
	Lng32 stackVar = (char*)this - (char*)&stackVar;
	if (stackVar < 0) stackVar = -stackVar;

	cerr << "**ERROR: Object " << (void*)this;

	if (stackVar <= 8192)
	  cerr << " may be invalid (may be on stack, currently " 
	       << (void*)&stackVar << ")";
	else
	  cerr << " is invalid";

	if (referencingObject)
	  cerr << " (pointer from " << (void*)referencingObject << ")";
	
	cerr << ".**" << endl;

	return -1;
      }
    return 0;
  }
#endif

// -----------------------------------------------------------------------
// overloaded new operator for CollHeap*
// -----------------------------------------------------------------------

// The following new/delete operator should follow the rule of new 
// operator, i.e.
// in the case of out of space, it should
//  - call the new_handler if being set previously
//  - if still no space, throw the bad_alloc exception.
// Since now the new/delete are used by executor, which can not
// take any global variables, need to make sure set_new_handler does
// not use any globals before using it.
// The compiler does not support exception yet. Should be done when
// the compiler supports exception.
//
void* operator new (size_t t, CollHeap* h)
{

  if (NOT_CHECK_NAHEAP(h))
  {
    return h->allocateMemory(t);
  } else {

#ifdef NA_LEAK_DETECTION
      get_caller_location();
#endif


    return (::new char[t] );
  }

}

void* operator new[] (size_t t, CollHeap* h)
{
  //return ( (h) ? h->allocateMemory(t) : (::new char[t] ) );

  if (NOT_CHECK_NAHEAP(h))
  {
    return h->allocateMemory(t);
  } else {

#ifdef NA_LEAK_DETECTION
    get_caller_location();
#endif


    return (::new char[t] );
  }

}

void* operator new[] (size_t t, CollHeap* h, NABoolean failureIsFatal)
{
  if (NOT_CHECK_NAHEAP(h))
  {
    return h->allocateMemory(t, failureIsFatal);
  } else {

#ifdef NA_LEAK_DETECTION
    get_caller_location();
#endif


    return (::new char[t] );
  }

}


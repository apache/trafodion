/* -*-C++-*-
 *****************************************************************************
 *
 * File:         NAHeap.h
 * Description:  The memory management classes for Noah Ark sql.
 *
 * Created:      9/28/96
 * Language:     C++
 *
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
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#ifndef NAHEAP__H
#define NAHEAP__H

#include "ComSpace.h"

#ifdef NA_STD_NAMESPACE
#include <iosfwd>
using namespace std;
#else
class ostream;
#endif

// -----------------------------------------------------------------------
// This file contains the memory management classes :
//
// -class CollHeap - abstract base class. defined in CollHeap.h
//    allocateMemory, deallocateMemory methods defined.
//
// -class NASpace: public CollHeap
//    NASpace acts as a Space in ComSpace.h
//
// -class NAMFHeap: public CollHeap
//    NAMFHeap use malloc/free.
//
// -examples:
//   class YourHeap : public CollHeap
//     implement your allocateMemory and deallocateMemory methods.
//
//   class YourClass : NABasicObject
//
//   then, you can do
//   YourHeap* aHeap = new YourHeap;
//   YourClass* object = new (aHeap) YourClass(aHeap);
//     the memory  will be allocated through YourHeap
//   YourClass* sysObject = new YourClass;
//     the memory will be allocated through global new
//   delete object;
//   delete sysObject;
//     the memory allocated will be freed accordingly.
//
// - Some new operators and macro defines (for delete) for CollHeap*
//   memory management.
//   These are for classes/types that can NOT be derived from NABasicObject.
//   e.g. RW classes, basic types, .etc. Also for the new of array which
//   goes through the global new/delete always.
//     void * operator new(size_t size, CollHeap* h);
//     NADELETEBASIC
//     NADELETE
//     NADELETEARRAY
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// NASpace is a space that will use the Space for memory allocation.
// And it acts just like a space, i.e. the memory deallocated will not
// be reused.
// -----------------------------------------------------------------------

class NASpace : public CollHeap
{
public:

  NASpace(Space::SpaceType t=Space::SYSTEM_SPACE);

  virtual ~NASpace();

  virtual void * allocateMemory(size_t size, NABoolean failureIsFatal = TRUE);

  virtual void deallocateMemory(void*)
    {
#ifdef NAHEAP__DEBUG
      cout << "NASpace::deallocateMemory()\n";
#endif
    }

#if (defined(_DEBUG) || defined(NSK_MEMDEBUG))
  virtual void dump(ostream* outstream, Lng32 indent) {};
#else
  inline void dump(void* outstream, Lng32 indent) {}
#endif

private:
  NASpace(const NASpace&);
  NASpace& operator =(const NASpace&);

  Space s_;
}; // end of NASpace


#endif


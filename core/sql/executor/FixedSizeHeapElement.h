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
#ifndef FIXEDSIZEHEAPELEMENT_H
#define FIXEDSIZEHEAPELEMENT_H
/* -*-C++-*-
********************************************************************************
*
* File:         FixedSizeHeapElement.h
* Description:  Fixed size heap element
*
*
* Created:      12/19/96
* Language:     C++
*
*
********************************************************************************
*/

// -----------------------------------------------------------------------------

#include "ex_stdh.h"
#ifdef NA_MDAM_EXECUTOR_DEBUG
#include <iostream>
#endif /* NA_MDAM_EXECUTOR_DEBUG */

// *****************************************************************************
// FixedSizeHeapElement models an element on the free list managed by
// FixedSizeHeapManager.
// *****************************************************************************

// Forward declarations.

// End of forward declarations.

class FixedSizeHeapElement
{

public:

  // Constructor.
  FixedSizeHeapElement
    (FixedSizeHeapElement * nextElementPtr = 0)
    : nextElementPtr_(nextElementPtr) {}


  // Destructor.
  inline ~FixedSizeHeapElement() {}

  // Operator new.
  inline void * operator new(size_t, char * ptr);

  // Operator new. This should never be called.
  void * operator new(size_t)
  {
  ex_assert(0,"FixedSizeHeapElement::operator new(size_t) called.");
  return 0;
  }

  // Operator delete.  This should never be called.
  void operator delete(void *)
  {
  ex_assert(0,"FixedSizeHeapElement::operator delete(void *) called.");
  }


  // Get function for nextElementPtr_.
  inline FixedSizeHeapElement * getNextElementPtr() const;

  // Print the status of the element.
  #ifdef NA_MDAM_EXECUTOR_DEBUG
  inline void print(const char * header = "") const;
  #endif /* NA_MDAM_EXECUTOR_DEBUG */

  // Set function for nextElementPtr_.
  inline void setNextElementPtr
    (FixedSizeHeapElement * nextElementPtr);

private:

  // Forward pointer to form a linked list.
  FixedSizeHeapElement * nextElementPtr_;


}; // class FixedSizeHeapElement


// *****************************************************************************
// Inline member functions for class FixedSizeHeapElement
// *****************************************************************************


// Operator new.
inline void * FixedSizeHeapElement::operator new(size_t, char * ptr)
{
  return (void *)ptr;  // $$$$ is the cast necessary?
}

// Get function for nextElementPtr_.
inline FixedSizeHeapElement * FixedSizeHeapElement::getNextElementPtr() const
{
  return nextElementPtr_;
}


// Print the status of the element.
#ifdef NA_MDAM_EXECUTOR_DEBUG
inline void FixedSizeHeapElement::print(const char * header) const
{
  cout << header
       << "   this = " << (void *)this
       << ", nextElementPtr_ = " << (void *)nextElementPtr_ << endl;
}
#endif /* NA_MDAM_EXECUTOR_DEBUG */


// Set function for nextElementPtr_.
inline void FixedSizeHeapElement::setNextElementPtr
  (FixedSizeHeapElement * nextElementPtr)
{
  nextElementPtr_ = nextElementPtr;
}


#endif /* FIXEDSIZEHEAPELEMENT_H */

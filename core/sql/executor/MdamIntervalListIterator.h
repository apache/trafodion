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
#ifndef MDAMINTERVALLISTITERATOR_H
#define MDAMINTERVALLISTITERATOR_H
/* -*-C++-*-
********************************************************************************
*
* File:         MdamIntervalListIterator.h
* Description:  MDAM Interval List Iterator
*               
*               
* Created:      9/11/96
* Language:     C++
*
*
*
*
********************************************************************************
*/

// -----------------------------------------------------------------------------

// *****************************************************************************
// MdamIntervalListIterator : MDAM Interval List Iterator
//
// This class performs iteration over the members of an MDAM Interval list.
//
// There is no MdamIntervalListIterator.C file because all the member functions
// are inline.
// *****************************************************************************

class MdamIntervalListIterator
{

public:

  // Constructor.
  MdamIntervalListIterator
    (const MdamIntervalList & intervalListRef)
           : intervalListRef_(intervalListRef),
           intervalPtr_(intervalListRef.firstIntervalPtr_)
  { }

  // Destructor.
  ~MdamIntervalListIterator() {}

  // Iteration operator returns a pointer to the next interval on each call.
  // It is safe to use this operator even if the intervals are being deleted.
  inline MdamInterval * operator()();

  // This function resets the iterator to the beginning of the list.
  inline void init();


private:

  // Reference to the MdamIntervalList for which the iterator is used.
  const MdamIntervalList & intervalListRef_;

  // Pointer to the current MdamInterval.
  MdamInterval * intervalPtr_;

}; // class MdamIntervalListIterator


// *****************************************************************************
// Inline member functions for class MdamIntervalListIterator
// *****************************************************************************


// Iteration operator returns a pointer to the next interval on each call.
inline MdamInterval * MdamIntervalListIterator::operator()()
{
  MdamInterval * tempIntervalPtr = intervalPtr_;
  if (intervalPtr_ != 0)
    {
      intervalPtr_ = intervalPtr_->getNextMdamIntervalPtr();
    };
  return tempIntervalPtr;
}


// This function resets the iterator to the beginning of the list.
inline void MdamIntervalListIterator::init()
{
  intervalPtr_ = intervalListRef_.firstIntervalPtr_;
}


#endif /* MDAMINTERVALLISTITERATOR_H */

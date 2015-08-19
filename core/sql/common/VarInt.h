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
#ifndef INCLUDE_VARINT_H
#define INCLUDE_VARINT_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         VarInt.h
 * Description:  Array of unsigned integers with bit widths from 1-32
 * Created:      1/11/2013
 * Language:     C++
 *
 *
 *****************************************************************************
 */


#include "Platform.h"
#include "NABoolean.h"
#include "NAMemory.h"

ULng32 pack(char*& buffer, UInt16 x, NABoolean swapBytes);
ULng32 pack(char*& buffer, ULng32 x, NABoolean swapBytes);
ULng32 pack(char*& buffer, Int32 x, NABoolean swapBytes);
ULng32 pack(char*& buffer, UInt64 x, NABoolean swapBytes);
ULng32 pack(char*& buffer, ULng32* ptr, ULng32 x, NABoolean swapBytes);

ULng32 unpack(char*& buffer, UInt16& x);
ULng32 unpack(char*& buffer, ULng32& x);
ULng32 unpack(char*& buffer, Int32& x);
ULng32 unpack(char*& buffer, UInt64& x);
ULng32 unpack(char*& buffer, ULng32*& ptr, ULng32& x, NAHeap* heap);

//----------------------------------------------------------------------------------
// Array of unsigned integers with a bit width between 1 and 32.
//
// - Constructor specifies number of entries, bit width, and heap.
// - get method and indexing operator [] return the i-th element
// - put method sets i-th element, values > max are mapped to max
//   (returns TRUE if overflow happened)
// - add method adds a value to i-th element, returns TRUE if overflow happened
// - sub method subtracts a value from i-th element,
//    + returns TRUE if value is at max and does nothing
//    + returns TRUE if value underflows (and sets i-th item to 0)
//----------------------------------------------------------------------------------


class VarUIntArray
{
  static const ULng32 BitsPerWord = 32;
  static const ULng32 AllOnes = 0xFFFFFFFF;

public:
  // create an array with a given length of number of bits per entry (up to 32 allowed)
  // and initialize it to zeroes
  VarUIntArray(ULng32 numEntries,
               ULng32 numBits, NAHeap* heap);

  // copy constructor
  VarUIntArray(const VarUIntArray&, NAHeap* heap);

  // create an array with no space allocated
  VarUIntArray(NAHeap* heap);

  ~VarUIntArray() { NADELETEBASIC(counters_, heap_); }

  void clear();

  // get array entry at index ix
  ULng32 get(ULng32 ix) const { return (*this)[ix]; }
  ULng32 operator[](ULng32 ix) const;

  // overwrite array entry at index ix with a new value,
  // return TRUE if an overflow occurred
  NABoolean put(ULng32 ix, ULng32 val);

  // add to existing value, return TRUE if overflow occurred
  NABoolean add(ULng32 ix, ULng32 val, ULng32& result);

  // subtract from existing value, return TRUE if value
  // did overflow in the past.  
  NABoolean sub(ULng32 ix, ULng32 val, ULng32& minuend);

  // maximum value  (this value is used to indicate overflow)
  ULng32 getMaxVal() const { return maxVal_; }

  UInt32 entries() { return numEntries_; };
  UInt32 numBits() { return numBits_; };
  
  // estimate amount of memory needed to store x entries with b bits per entry
  // The estimation is expressed in bytes.
  static UInt64 estimateMemoryInBytes(ULng32 x, ULng32 b);

  // Modeled based on the following method
  // IpcMessageObjSize ComCondition::packObjIntoMessage(char* buffer,
  //                         NABoolean swapBytes)
  ULng32 packIntoBuffer(char*& buffer, NABoolean swapBytes);
  ULng32 unpackBuffer(char*& buffer);


private:

  ULng32 numEntries_;
  ULng32 numWords_;
  ULng32 numBits_;
  ULng32 maxVal_;
  ULng32 *counters_;
  NAHeap* heap_;
};


typedef VarUIntArray* VarUIntArrayPtr;

#endif

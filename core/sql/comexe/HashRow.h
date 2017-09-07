/* -*-C++-*-
****************************************************************************
*
* File:         HashRow.h
* Description:  
*
* Created:      5/6/98
* Language:     C++
*
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
*
****************************************************************************
*/

#ifndef COMHASHROW_H
#define COMHASHROW_H

#include "NABasicObject.h"
#include "BaseTypes.h"

/////////////////////////////////////////////////////
// The HashRow here describes only the row header.
// The expressions working on the rows know where to
// find the data
// The class is NOT derived from ExGod! HashRow is
// never allocated with new. Instead, it is just an
// "overlay" on an allocated buffer.
/////////////////////////////////////////////////////

#define MASK31 0x7FFFFFFF
class HashRow {
  friend class HashTableCursor;
  friend class HashTable;
  friend class HashBufferSerial;
public:
  inline HashRow() {};
  inline ~HashRow() {};
  void print(ULng32 rowlength);
  inline SimpleHashValue hashValue() const { return hashValue_ & MASK31; }

  // Return the raw hash value (no masking)
  inline SimpleHashValue hashValueRaw() const { return hashValue_; }

  inline void setHashValue(SimpleHashValue hv) { hashValue_ = hv & MASK31; }

  // Set the hash value to the raw hash value (no masking)
  inline void setHashValueRaw(SimpleHashValue hv) { hashValue_ = hv; }

  inline NABoolean bitSet() const { return ((hashValue_ & ~MASK31) != 0L) ; }
  inline void setBit(NABoolean val)
	 { if ( val ) hashValue_ |= ~MASK31 ; else hashValue_ &= MASK31 ;}
inline void setNext (HashRow *next) {next_ = next;}
inline HashRow *next() const {return next_;}

inline char *getData() const {
  return ((char *)this) + sizeof(HashRow);
}

inline UInt32 getRowLength() const {
  return *((UInt32 *)getData());
}

private:
inline void setRowLength(UInt32 rowLen) {
  *((UInt32 *)getData()) = rowLen;
}

#ifdef CHECK_EYE
  inline void setEye() {
    eye_ = 42424242;
  }
  inline void checkEye() {
    assert(eye_ == 42424242);
  }

  UInt32 eye_;
#else
  inline void setEye() {}
  inline void checkEye() {}
#endif

  SimpleHashValue hashValue_;
  HashRow * next_;
};

#endif

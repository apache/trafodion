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
* File:         HashBufferHeader.h
* Description:  
*
* Created:      5/6/98
* Language:     C++
*
*
*
*
****************************************************************************
*/

#ifndef HASH_BUFFER_HEADER_H
#define HASH_BUFFER_HEADER_H

/////////////////////////////////////////////////////////////////////////////
// the HashBufferHeader describes the first few bytes of a buffer written
// to a temporary file.
// The class is NOT derived from NABasicObject. HashBufferHeader is never
// allocated with new. Instead, it is just an "overlay" on an allocated
// buffer.
/////////////////////////////////////////////////////////////////////////////
class HashBufferHeader {
  friend class HashBuffer;
  friend class HashBufferSerial;
private:
  HashBufferHeader();
 ~HashBufferHeader() {};
  inline ULng32 getRowCount() const;
  inline void setRowCount(ULng32 rowCount);
  inline void incRowCount();

  ULng32 rowCount_;      // # rows in buffer (pointer to next free row)
  ULng32 bucketCount_;   // the buffer contains row from bucketCount_
                                // buckets. This is used in phase 3 on the
                                // hash join. If the cluster is a inner during
                                // this phase and there was a cluster split,
                                // not all rows in the buffer are chained
};

/////////////////////////////////////////////////////////////////////////////
// inline functions of HashBufferHeader
/////////////////////////////////////////////////////////////////////////////

inline ULng32 HashBufferHeader::getRowCount() const {
  return rowCount_;
};

inline void HashBufferHeader::setRowCount(ULng32 rowCount) {
  rowCount_ = rowCount;
};

inline void HashBufferHeader::incRowCount() {
  rowCount_++;
};

#endif


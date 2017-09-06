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
#ifndef SQL_BUFFER_SIZE_H
#define SQL_BUFFER_SIZE_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         sql_buffer_neededsize.h
 * Description:  Definition for static method SqlBufferNeededSize
 *               
 *               
 * Created:      7/10/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

//
// NOTE : Definition for SqlBufferNeededSize and SqlBufferGetTuppSize moved to this file as part of warning 
// elimination effort. If these static methods are defined in sql_buffer.h we get
// warning 262 (function defined but not referenced) when compiling various files such
// as ex_transaction.cpp. These functions are not used by all cpp files that include sql_buffer.h
// Therefore we moved SqlBufferNeededSize and SqlBufferGetTuppSize to this file and changed the cpp files that call 
// SqlBufferNeededSize or SqlBufferGetTuppSize to include this file (11/01/2003)
//

static inline ULng32 SqlBufferHeaderSize(SqlBufferHeader::BufferType bufType)
{
  // Return the buffer header size 
  Lng32 headerSize = 0;
  switch (bufType)
    {
    case SqlBufferHeader::DENSE_:
      headerSize = sizeof(SqlBufferDense);
      break;

    case SqlBufferHeader::OLT_:
      headerSize = sizeof(SqlBufferOlt);
      break;

    case SqlBufferHeader::NORMAL_:
      headerSize = sizeof(SqlBufferNormal);
      break;

    default:
      headerSize = sizeof(SqlBufferNormal);
      break;
    }

  headerSize = ROUND8(headerSize);

  return (headerSize);
}

static inline ULng32 SqlBufferGetTuppSize(
     Lng32 recordLength = 0,
     SqlBufferHeader::BufferType bufType = SqlBufferHeader::NORMAL_)
{
  Lng32 sizeofTuppDescriptor = 
    ((bufType == SqlBufferHeader::DENSE_) ? ROUND8(sizeof(TupleDescInfo))
     : sizeof(tupp_descriptor));
  
  return ROUND8(recordLength) + sizeofTuppDescriptor;
  
}

static inline ULng32 SqlBufferNeededSize(Lng32 numTuples = 0, 
				  Lng32 recordLength = 0,
				  SqlBufferHeader::BufferType bufType = SqlBufferHeader::NORMAL_)
{
  // Return the header size plus the size of any tuple descriptors
  // beyond the first (which is included in the header) plus the
  // size for the aligned data.
  Lng32 headerSize = SqlBufferHeaderSize (bufType);
  return (headerSize +
	  (numTuples * SqlBufferGetTuppSize(recordLength, bufType)));
}

#endif

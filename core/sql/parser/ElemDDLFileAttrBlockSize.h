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
#ifndef ELEMDDLFILEATTRBLOCKSIZE_H
#define ELEMDDLFILEATTRBLOCKSIZE_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLFileAttrBlockSize.h
 * Description:  class for File Block Size (parse node) elements in
 *               DDL statements
 *
 *               
 * Created:      4/21/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ElemDDLFileAttr.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLFileAttrBlockSize;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class ElemDDLFileAttrBlockSize
// -----------------------------------------------------------------------
class ElemDDLFileAttrBlockSize : public ElemDDLFileAttr
{

public:

  enum { DEFAULT_BLOCK_SIZE = 32768 };

  // default constructor
  ElemDDLFileAttrBlockSize(ULng32 blockSizeInBytes
                           = DEFAULT_BLOCK_SIZE);

  // virtual destructor
  virtual ~ElemDDLFileAttrBlockSize();

  // cast
  virtual ElemDDLFileAttrBlockSize * castToElemDDLFileAttrBlockSize();

  // accessors
  inline ULng32 getBlockSize() const;

  // member functions for tracing
  virtual const NAString getText() const;
  virtual const NAString displayLabel1() const;

    // method for building text
  virtual NAString getSyntax() const;



private:

  //
  // method(s)
  //

  NABoolean isLegalBlockSizeValue(ULng32 blockSize) const;

  //
  // data member(s)
  //

  ULng32 blockSizeInBytes_;

}; // class ElemDDLFileAttrBlockSize

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLFileAttrBlockSize
// -----------------------------------------------------------------------

inline ULng32
ElemDDLFileAttrBlockSize::getBlockSize() const
{
  return blockSizeInBytes_;
}

#endif // ELEMDDLFILEATTRBLOCKSIZE_H

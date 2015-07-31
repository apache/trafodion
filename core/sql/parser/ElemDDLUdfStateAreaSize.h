/* -*-C++-*- */
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
#ifndef ELEMDDLUDFSTATEAREASIZE_H
#define ELEMDDLUDFSTATEAREASIZE_H
/* -*-C++-*-
******************************************************************************
*
* File:         ElemDDLUdfStateAreaSize.h
* Description:  class for UDF State Area Size (parse node) elements in
*               DDL statements
*
*
* Created:      7/14/09
* Language:     C++
*
*
*
*
******************************************************************************
*/
#include "ElemDDLNode.h"

class ElemDDLUdfStateAreaSize : public ElemDDLNode
{

public:

  // default constructor
  ElemDDLUdfStateAreaSize(ComUInt32 sizeInBytes);

  // virtual destructor
  virtual ~ElemDDLUdfStateAreaSize(void);

  // cast
  virtual ElemDDLUdfStateAreaSize * castToElemDDLUdfStateAreaSize(void);

  // accessor
  inline const ComUInt32 getStateAreaSize(void) const
  {
    return stateAreaSize_;
  }

  //
  // methods for tracing
  //

  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;

private:

  ComUInt32 stateAreaSize_;

}; // class ElemDDLUdfStateAreaSize

#endif /* ELEMDDLUDFSTATEAREASIZE_H */

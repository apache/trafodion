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
#ifndef ELEMDDLUDFFINALCALL_H
#define ELEMDDLUDFFINALCALL_H
/* -*-C++-*-
******************************************************************************
*
* File:         ElemDDLUdfFinalCall.h
* Description:  class for UDF Final Call attribute (parse node) elements in
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

class ElemDDLUdfFinalCall : public ElemDDLNode
{

public:

  // constructor
  ElemDDLUdfFinalCall(NABoolean theFinalCall);

  // virtual destructor
  virtual ~ElemDDLUdfFinalCall(void);

  // cast
  virtual ElemDDLUdfFinalCall * castToElemDDLUdfFinalCall(void);

  // accessor
  inline const NABoolean getFinalCall(void) const
  {
    return finalCall_;
  }

  //
  // methods for tracing
  //

  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;

private:

  NABoolean finalCall_;

}; // class ElemDDLUdfFinalCall

#endif /* ELEMDDLUDFFINALCALL_H */

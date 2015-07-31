/* -*-C++-*- */
#ifndef ELEMDDLUDFOPTIMIZATIONHINT_H
#define ELEMDDLUDFOPTIMIZATIONHINT_H
/* -*-C++-*-
******************************************************************************
*
* File:         ElemDDLUdfOptimizationHint.h
* Description:  class for function optimization hint (parser node)
*               elements in DDL statements
*
*
* Created:      1/28/2010
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
******************************************************************************
*/

#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif
#include "SqlParserGlobals.h"

#include "ComSmallDefs.h"
#include "ElemDDLNode.h"
#include "Collections.h"

class ElemDDLUdfOptimizationHint : public ElemDDLNode
{

public:

  // default constructor
  ElemDDLUdfOptimizationHint(ComUdfOptimizationHintKind optimizationKind,
                             CollHeap * h = PARSERHEAP());

  // virtual destructor
  virtual ~ElemDDLUdfOptimizationHint();

  // cast
  virtual ElemDDLUdfOptimizationHint * castToElemDDLUdfOptimizationHint(void);

  //
  // accessors
  //

  inline ComUdfOptimizationHintKind getOptimizationKind(void) const
  {
    return udfOptimizationKind_;
  }

  inline ComSInt32 getCost(void) const
  {
    return cost_;
  }

  inline const NAList<ComSInt64> & getUniqueOutputValues(void) const
  {
    return uniqueOutputValues_;
  }

  //
  // mutators
  //

  inline void setOptimizationKind(ComUdfOptimizationHintKind newValue)
  {
    udfOptimizationKind_ = newValue;
  }

  inline void setCost(ComSInt32 newValue)
  {
    cost_ = newValue;
  }

  inline void setUniqueOutputValuesParseTree(ItemExprList *pList)
  {
    uniqueOutputValuesParseTree_ = pList;
  }

  inline NAList<ComSInt64> & getUniqueOutputValues(void)
  {
    return uniqueOutputValues_;
  }

  //
  // helpers
  //

  void synthesize(void);

  //
  // methods for tracing
  //

  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;

private:

  ComUdfOptimizationHintKind udfOptimizationKind_;
  ComSInt32 cost_;
  ItemExprList * uniqueOutputValuesParseTree_;
  NAList<ComSInt64> uniqueOutputValues_;

}; // class ElemDDLUdfOptimizationHint

#endif /* ELEMDDLUDFOPTIMIZATIONHINT_H */

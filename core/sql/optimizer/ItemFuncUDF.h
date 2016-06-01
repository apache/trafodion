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
#ifndef ITEMFUNCUDF_H
#define ITEMFUNCUDF_H

// This code should be moved into ItemFunc.h

#include "IntervalType.h"
#include "CharType.h"
#include "ItemExpr.h"
#include "NATable.h"
#include "exp_like.h"
#include "QRExprElement.h"
#include "RoutineDesc.h"

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class CharType;
class DisjunctArray;
class Generator;
class RangePartitioningFunction;
class IntegerList;
class NARoutine;
class ItemList;
class RoutineDesc;

// -----------------------------------------------------------------------
// User-defined functions (e. g. stored procedures), new functions may
// be defined at run time
// -----------------------------------------------------------------------
class UDFunction : public Function
{
  // ITM_USER_DEF_FUNCTION
public:
  UDFunction(const ComObjectName &functionName,
             const LIST(ItemExpr *) &children,
             CollHeap *h) :
       Function(ITM_USER_DEF_FUNCTION, children, h),
       functionName_(functionName), functionNamePos_(0),
       udfDesc_(0), hasSubquery_(FALSE), heap_(h)
       {

         // we do allow null input parameters
         allowsSQLnullArg() = TRUE;
       }

  // virtual destructor
  virtual ~UDFunction();

  // Function name position within command line.  This is used for rewriting the 
  // UDF name to fully qualified for MVS.
  const Int32 getNamePosOfUdfInQuery() const { return functionNamePos_; }
  void setNamePosOfUdfInQuery(Int32 pos) { functionNamePos_ = pos; }

  // Routine descriptor
  const RoutineDesc *getRoutineDesc() const { return udfDesc_; }
  void setRoutineDesc(RoutineDesc *rdesc) { udfDesc_ = rdesc; }

  // append an ascii-version of ItemExpr into cachewa.qryText_
  virtual void generateCacheKey(CacheWA& cwa) const;

  // is the UDFunction expression cacheable.
  // Only makes sense after it has been bound.
  virtual NABoolean isCacheableExpr(CacheWA& cwa);

  // is any literal in this expr safely coercible to its target type?
  virtual NABoolean isSafelyCoercible(CacheWA& cwa) const;

  // replace any constant parameters so that we can reuse the plan
  virtual ItemExpr* normalizeForCache(CacheWA& cwa, BindWA& bindWA);

  virtual ExprValueId & operator[] (Lng32 ix);
  virtual const ExprValueId & operator[] (Lng32 ix) const;
  // get the degree of this node (it depends on the type of the operator)
  virtual Int32 getArity() const;

  virtual HashValue topHash();
  virtual NABoolean duplicateMatch(const ItemExpr & other) const;
  virtual ItemExpr * copyTopNode(ItemExpr *derivedNode = NULL,
				 CollHeap* outHeap = 0);

  // Does semantic checks and binds the function.
  virtual ItemExpr * bindNode(BindWA * bindWA);

  virtual void transformNode(NormWA & normWARef,
                                    ExprValueId & locationOfPointerToMe,
                                    ExprGroupId & introduceSemiJoinHere,
                                    const ValueIdSet & externalInputs);

  virtual void transformToRelExpr(NormWA & normWARef,
                                 ExprValueId & locationOfPointerToMe,
                                 ExprGroupId & introduceSemiJoinHere,
                                 const ValueIdSet & externalInputs);

  virtual ItemExpr * normalizeNode(NormWA & normWARef);
 
  virtual ItemExpr *containsUDF();
  virtual NABoolean containsIsolatedUDFunction();

  virtual const NAType *synthesizeType();

  // method to do code generation
  virtual short codeGen(Generator*);

  // get a printable string that identifies the operator
  virtual const NAString getText() const;
  ComObjectName &getFunctionName() { return functionName_; }
  virtual void unparse(NAString &result,
		       PhaseEnum phase = OPTIMIZER_PHASE,
		       UnparseFormatEnum form = USER_FORMAT,
		       TableDesc * tabId = NULL) const;

  // output degree functions to handle multi-valued UDFs.
  // Returns the number of outputs and the ItemExpr for each.
  virtual Int32     getOutputDegree();
  virtual ItemExpr *getOutputItem(UInt32 i);

private:
  // Copy constructor and assignment operator not written and not allowed.
  UDFunction (const UDFunction &, CollHeap * h=0);
  UDFunction &operator=(UDFunction &other); 

  inline const NARoutine *getRoutine() 
  { if (udfDesc_) return udfDesc_->getNARoutine(); else return NULL; }
  inline const NARoutine *getAction()  
  { if (udfDesc_) return udfDesc_->getActionNARoutine(); else return NULL; }

  void setInOrOutParam (RoutineDesc *routine,
                        ItemExpr *argument,
                        Int32 position,
                        ComColumnDirection paramMode,
                        ItemExprList *inParams,
                        BindWA *bindWA);

  CollHeap     *heap_;
  Int32         functionNamePos_;
  ComObjectName functionName_;
  NAString      actionName_;  // Must be a string: MX Object names can't have '$' as first char.
  RoutineDesc  *udfDesc_;

  NABoolean     hasSubquery_;
  ValueIdSet    inputVars_;
  ValueIdSet    outputVars_;
};


#endif /* ITEMFUNCUDF_H */

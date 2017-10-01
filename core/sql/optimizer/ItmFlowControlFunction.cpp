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

#include "CharType.h"
#include "NumericType.h"
#include "ItemExpr.h"
#include "ItmFlowControlFunction.h"

//
// ItmDoWhile
//

// synthesizeType
//
const NAType *ItmDoWhileFunction::synthesizeType() {
  return &child(0)->castToItemExpr()->getValueId().getType();
};

// copyTopNode
//
ItemExpr *ItmDoWhileFunction::copyTopNode(ItemExpr *derivedNode, 
					 CollHeap *outHeap) {
  ItemExpr *result;

  if(derivedNode == NULL) {
    result = new(outHeap) ItmDoWhileFunction(NULL,NULL);
  }
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
};


//
// ItmBlockFunction
//

// synthesizeType
//
// Returns the type of the second argument.
//
const NAType *ItmBlockFunction::synthesizeType() {
  return &child(1)->castToItemExpr()->getValueId().getType();
};

// copyTopNode
//
ItemExpr *ItmBlockFunction::copyTopNode(ItemExpr *derivedNode, 
					 CollHeap *outHeap) {
  ItemExpr *result;

  if(derivedNode == NULL) {
    result = new(outHeap) ItmBlockFunction(NULL,NULL);
  }
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
};

//
// ItmWhile
//

// synthesizeType
//
const NAType *ItmWhileFunction::synthesizeType() {
  return &child(0)->castToItemExpr()->getValueId().getType();
};

// copyTopNode
//
ItemExpr *ItmWhileFunction::copyTopNode(ItemExpr *derivedNode, 
					 CollHeap *outHeap) {
  ItemExpr *result;

  if(derivedNode == NULL) {
    result = new(outHeap) ItmWhileFunction(NULL,NULL);
  }
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(derivedNode, outHeap);
};





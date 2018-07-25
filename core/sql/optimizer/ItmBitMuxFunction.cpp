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

// This class is obsolete in the sense 
// that it had been added long time ago during the data
// mining days (late 90s) but is not used anymore
#include "CharType.h"
#include "NumericType.h"
#include "ItemExpr.h"
#include "ItmBitMuxFunction.h"

// Destructor
//
ItmBitMuxFunction::~ItmBitMuxFunction() { ; };

// synthesizeType
//
const NAType *ItmBitMuxFunction::synthesizeType() {
  Int32 size = 0;
  for(Int32 i=0; i<getArity(); i++) {
    const NAType &type = child(i)->getValueId().getType();
    size += type.getTotalSize();
  }

  return new(CmpCommon::statementHeap()) SQLChar(CmpCommon::statementHeap(), size, FALSE);
};

// copyTopNode
//
ItemExpr *ItmBitMuxFunction::copyTopNode(ItemExpr *derivedNode, 
					 CollHeap *outHeap) {
  ItemExpr *result;

  if(derivedNode == NULL) {
    LIST(ItemExpr*) item(outHeap);
    result = new(outHeap) ItmBitMuxFunction(item);
  }
  else
    result = derivedNode;

  return BuiltinFunction::copyTopNode(result, outHeap);
};

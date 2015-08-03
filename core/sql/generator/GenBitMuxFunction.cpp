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

// Includes
//
#include "ItemExpr.h"
#include "ItmBitMuxFunction.h"
#include "Generator.h"
#include "GenExpGenerator.h"
#include "ExpBitMuxFunction.h"

ItemExpr *ItmBitMuxFunction::preCodeGen(Generator *generator) {
  if (nodeIsPreCodeGenned())
    return this;

  Lng32 nc = (Lng32)getArity();
  
  for (Lng32 index = 0; index < nc; index++)
    {

      // during key encode expr generation, no need to convert external
      // column types(like tandem floats) to their internal
      // equivalent(ieee floats). Avoid doing preCodeGen in these cases.
      //
      if (NOT child(index)->getValueId().getType().isExternalType()) {
        child(index) = child(index)->preCodeGen(generator);
        if (! child(index).getPtr())
          return NULL;
      }
    }

  markAsPreCodeGenned();

  return this;
}

short ItmBitMuxFunction::codeGen(Generator * generator) {
  Attributes ** attr;

  if (generator->getExpGenerator()->genItemExpr(this, &attr, 
						(1 + getArity()), -1) == 1)
    return 0;

  ex_clause * function_clause =
    new(generator->getSpace()) ExpBitMuxFunction(getOperatorType(),
						 1 + getArity(),
						 attr, 
						 generator->getSpace());
  
  generator->getExpGenerator()->linkClause(this, function_clause);

#ifdef _DEBUG
  Lng32 totalLength = 0;

  for(Int32 i = 0; i < getArity(); i++) {
    totalLength += function_clause->getOperand((short)(i+1))->getStorageLength();
  }

  GenAssert(totalLength == function_clause->getOperand(0)->getLength(),
            "Not enough storage allocated for bitmux");
#endif

  return 0;
}


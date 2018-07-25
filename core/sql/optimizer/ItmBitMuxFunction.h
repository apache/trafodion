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
#ifndef BitMuxFunction_h
#define BitMuxFunction_h

// This class is obsolete in the sense
// that it had been added long time ago during the data
// mining days (late 90s) but is not used anymore

// Includes
//
#include "ItemFunc.h"

// Forward External Declarations
//
class ItemExpr;
class BuiltinFunction;
class Generator;
class NAMemory;

// Forward Internal Declarations
//
class ItmBitMuxFunction;

class ItmBitMuxFunction : public BuiltinFunction {
public:
  ItmBitMuxFunction(const LIST(ItemExpr*) &children) 
    : BuiltinFunction(ITM_BITMUX, CmpCommon::statementHeap(), children) { };

  virtual ~ItmBitMuxFunction();

  const NAType *synthesizeType();
  ItemExpr *preCodeGen(Generator*);
  short codeGen(Generator*);
  ItemExpr *copyTopNode(ItemExpr *derivedNode =NULL,
			NAMemory *outHeap =0);
};

#endif


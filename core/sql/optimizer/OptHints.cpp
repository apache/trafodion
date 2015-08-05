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
/* -*-C++-*- */

#include "OptHints.h"

Hint::Hint(const NAString &indexName, NAMemory *h) 
  : indexes_(h,1), selectivity_(-1.0), cardinality_(-1.0)
{ indexes_.insert(indexName); }

Hint::Hint(double c, double s, NAMemory *h) 
  : indexes_(h,0), selectivity_(s), cardinality_(c)
{ }

Hint::Hint(double s, NAMemory *h) 
  : indexes_(h,0), selectivity_(s), cardinality_(-1.0)
{ }

Hint::Hint(NAMemory *h) 
  : indexes_(h,0), selectivity_(-1.0), cardinality_(-1.0)
{ }

Hint::Hint(const Hint &hint, NAMemory *h) 
  : indexes_(hint.indexes_, h), selectivity_(hint.selectivity_), 
    cardinality_(hint.cardinality_)
{}

NABoolean Hint::hasIndexHint(const NAString &xName)
{ return indexes_.contains(xName); }

Hint* Hint::addIndexHint(const NAString &indexName)
{ 
  indexes_.insert(indexName); 
  return this;
}

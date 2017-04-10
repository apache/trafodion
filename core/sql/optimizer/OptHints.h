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
#ifndef OPTHINTS_H
#define OPTHINTS_H
/* -*-C++-*- */

#include "CmpCommon.h"
#include "NAStringDef.h"

// -----------------------------------------------------------------------
// forward declarations
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class Hint;

// -----------------------------------------------------------------------
// Optimizer Hint
// -----------------------------------------------------------------------
class Hint : public NABasicObject
{
 public:
  // constructors
  Hint(const NAString &indexName, NAMemory *h=HEAP);
  Hint(double c, double s, NAMemory *h=HEAP);
  Hint(double s, NAMemory *h=HEAP);
  Hint(NAMemory *h=HEAP);

  // copy ctor
  Hint(const Hint &hint, NAMemory *h);

  // virtual destructor
  virtual ~Hint() {}

  // mutators
  Hint* addIndexHint(const NAString &indexName);

  Hint* setCardinality(double c) { cardinality_ = c; return this; }

  Hint* setSelectivity(double s) { selectivity_ = s; return this; }

  void replaceIndexHint(CollIndex x, const NAString &newIndexName)
                                    { indexes_[x] = newIndexName; }

  // accessors
  NABoolean hasIndexHint(const NAString &xName); 
  CollIndex indexCnt() const { return indexes_.entries(); }
  const NAString& operator[](CollIndex x) const { return indexes_[x]; }

  double getCardinality() const { return cardinality_; }
  NABoolean hasCardinality() const { return cardinality_ != -1.0; }

  double getSelectivity() const { return selectivity_; }
  NABoolean hasSelectivity() const { return selectivity_ != -1.0; }

protected:
  LIST(NAString) indexes_;     // ordered list of index hints
  double         selectivity_; // table's hinted selectivity or -1
  double         cardinality_; // table's hinted cardinality or -1
}; // Hint

class HbaseAccessOptions : public NABasicObject
{
 public:
  HbaseAccessOptions(Lng32 v, NAMemory *h=HEAP)
    : hbaseVersions_(v)
  {}

  Lng32 getHbaseVersions() { return hbaseVersions_; }
  void setHbaseVersions(Lng32 v){ hbaseVersions_ = v;}

  NABoolean isMaxVersions() { return (hbaseVersions_ == -1); }
  NABoolean isAllVersions() { return (hbaseVersions_ == -2); }
 private:
  // -1, get max versions allowed. 
  // -2, get all versions including ones marked for delete (RAW in hbase)
  //  N,  get max N versions.
  Lng32 hbaseVersions_;
};

// -----------------------------------------------------------------------

#endif /* OPTHINTS_H */

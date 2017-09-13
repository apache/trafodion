// **********************************************************************
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
// **********************************************************************
//
#ifndef RELPROBECACHE_H
#define RELPROBECACHE_H
// -----------------------------------------------------------------------
// ProbeCache is a physical operator inserted by NestedJoin between
// itself and its right child during preCodeGen.  Its characteristic
// outputs are the same as its child.  Its characteristic inputs are 
// the same as its child with the addition of an ITM_EXEC_COUNT 
// so that it can invalidate the probe cache between statement 
// executions.  The ProbeCache operator has no item expressions
// until codeGen, and these are just temporary, used to create 
// runtime ex_expr objects. 
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------

// The following are physical operators
class ProbeCache;

// -----------------------------------------------------------------------
// The ProbeCache physical operator
// -----------------------------------------------------------------------

class ProbeCache : public RelExpr
{

public:
          // Constructor
  ProbeCache(RelExpr  *child,
             ULng32 numCachedProbes,
             CollHeap *oHeap = CmpCommon::statementHeap())
             : RelExpr(REL_PROBE_CACHE,
                        child, 
                        NULL,  // no right child.
                        oHeap),
               numCachedProbes_(numCachedProbes),
               numInnerTuples_(0)       // set in the generator for now. 
  {}

  virtual NABoolean isLogical() const;
  virtual NABoolean isPhysical() const;

  virtual Int32 getArity() const;
  virtual RelExpr * copyTopNode(RelExpr *derivedNode = NULL,
				CollHeap* outHeap = 0);

  virtual RelExpr * preCodeGen(Generator *generator,
                               const ValueIdSet & externalInputs,
                               ValueIdSet &pulledNewInputs);

  virtual short codeGen(Generator *g);

  virtual CostScalar getEstimatedRunTimeMemoryUsage(NABoolean perNode, Lng32 *numStreams = NULL);
  virtual double getEstimatedRunTimeMemoryUsage(ComTdb * tdb);

  virtual const NAString getText() const;

  ExplainTuple *addSpecificExplainInfo(ExplainTupleMaster *explainTuple,
                                       ComTdb *tdb,
                                       Generator *generator);
private:
  // Currently, both numCachedProbes_ and numInnerTuples_  are set in the generator
  // but in the future, the optimizer may set these, or at least supply 
  // suggestions or initial settings.  That is why they are members of this
  // class.
  ULng32 numCachedProbes_;
  ULng32 numInnerTuples_;

};  // ProbeCache

#endif  /* RELPROBECACHE_H */


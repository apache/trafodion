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
#ifndef TRANSRULE_H
#define TRANSRULE_H
/* -*-C++-*-
******************************************************************************
*
* File:         TransRule.h
* Description:  Transformation rules
*
*
* Created:	9/14/94
* Language:     C++
*
*
*
*
******************************************************************************
*/


#include "Rule.h"
#include "Analyzer.h"

//------------------------------------------------------------------------
//forward declaration
//------------------------------------------------------------------------

class MultiJoin;
class LSRConfidence;

// -----------------------------------------------------------------------
// classes defined in this file
// -----------------------------------------------------------------------

class JoinCommutativityRule;
class JoinLeftShiftRule;
class IndexJoinRule1;
class IndexJoinRule2;
class OrOptimizationRule;
class TSJRule;
class RoutineJoinToTSJRule;
class JoinToTSJRule;
class TSJFlowRule;
class TSJUDRRule;

class FilterRule;
class FilterRule0;
class FilterRule1;
class FilterRule2;

class GroupByEliminationRule;
class GroupByOnJoinRule;
class PartialGroupByOnTSJRule;
class GroupBySplitRule;
class AggrDistinctEliminationRule;
class GroupByTernarySplitRule;
class ShortCutGroupByRule;
class CommonSubExprRule;

class SampleScanRule;
class JoinToBushyTreeRule;

class OnceGuidance;
class StopGuidance;
class FilterGuidance;

// LSRs declared in this file
class MJStarJoinIRule;
class MJStarJoinIIRule;

// Workareas for MJ Rules
class MJRulesWA;
class MJStarJoinIRuleWA;

// Non-LSR MJ rules
class MJExpandRule;
class MJEnumRule;
class MVQRRule;
class MVQRScanRule;
class GroupByMVQRRule;

class HbaseScanRule;


// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------

class ScanIndexInfo;
class Scan;

// -----------------------------------------------------------------------
// Function to add transformation rules to the rule set
// -----------------------------------------------------------------------
void CreateTransformationRules(RuleSet*);

// -----------------------------------------------------------------------
// transformation rule subclasses
// -----------------------------------------------------------------------

class MJExpandRule : public Rule
{
public:
  MJExpandRule (const char * name,
                RelExpr * pattern,
                RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  // copy ctor
  MJExpandRule (const MJExpandRule &) ; // not written

  virtual ~MJExpandRule() {}

  virtual NABoolean topMatch (RelExpr * expr,
			      Context * context);

  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context *context,
				   RuleSubstituteMemory * & memory);

  virtual Int32 promiseForOptimization(RelExpr * relExpr,
               Guidance * guidance,
               Context * context);

  virtual NABoolean isImplementationRule () const { return FALSE; }

};

// -----------------------------------------------------------------------
// use for MV query rewrite
// -----------------------------------------------------------------------
class MVQRRule : public Rule
{
public:
  MVQRRule (const char * name,
            RelExpr * pattern,
            RelExpr * substitute) :
     Rule(name,pattern,substitute) {}

  // copy ctor
  MVQRRule (const MVQRRule &) ; 

  virtual ~MVQRRule() {}

  virtual NABoolean topMatch (RelExpr * expr,
                              Context * context);

  virtual RelExpr * nextSubstitute(RelExpr * before,
                                   Context *context,
                                   RuleSubstituteMemory * & memory);

  virtual NABoolean isImplementationRule () const { return FALSE; }
};

class MVQRScanRule : public Rule
{
public:
  MVQRScanRule (const char * name,
                RelExpr * pattern,
                RelExpr * substitute) :
     Rule(name,pattern,substitute) {}

  // copy ctor
  MVQRScanRule (const MVQRScanRule &) ;

  virtual ~MVQRScanRule() {}

  virtual NABoolean topMatch (RelExpr * expr,
                              Context * context);

  virtual RelExpr * nextSubstitute(RelExpr * before,
                                   Context *context,
                                   RuleSubstituteMemory * & memory);

  virtual NABoolean isImplementationRule () const { return FALSE; }
};

class GroupByMVQRRule : public Rule
{
public:
  GroupByMVQRRule (const char * name,
            RelExpr * pattern,
            RelExpr * substitute) :
     Rule(name,pattern,substitute) {}

  // copy ctor
  GroupByMVQRRule (const GroupByMVQRRule &) ;

  virtual ~GroupByMVQRRule() {}

  virtual NABoolean topMatch (RelExpr * expr,
                              Context * context);

  virtual RelExpr * nextSubstitute(RelExpr * before,
                                   Context *context,
                                   RuleSubstituteMemory * & memory);

  virtual NABoolean isImplementationRule () const { return FALSE; }
};

class MJEnumRule : public Rule
{
public:
  MJEnumRule (const char * name,
              RelExpr * pattern,
              RelExpr * substitute) :
     Rule(name,pattern,substitute) {}

  // copy ctor
  MJEnumRule (const MJEnumRule &) ; // not written

  virtual ~MJEnumRule() {}

  virtual NABoolean topMatch (RelExpr * expr,
			      Context * context);

  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context *context,
				   RuleSubstituteMemory * & memory);

  virtual NABoolean isImplementationRule () const { return FALSE; }

};

enum StarJoinType { TYPE_I, TYPE_II };

class MJStarJoinIRuleWA : public NABasicObject
{
  friend class MJStarJoinIRule;

public:
  // listOfEdges_ is allocated from the statement heap. It is safe to
  // ignore this dstr is not code coveraged.
  ~MJStarJoinIRuleWA()
  {
    if(listOfEdges_)
      delete listOfEdges_;
  }

private:

  // construct is private since only the MJStarJoinRule should be able to
  // instantiate an object of this class
  MJStarJoinIRuleWA(CollHeap *outHeap = CmpCommon::statementHeap()):
  factTable_(NULL_CA_ID),
  listOfEdges_(NULL),
  matchedStarSchema_(FALSE),
  optimalFTLocation_(-1),
  heap_(outHeap)
  {
  }

  // the fact table
  CANodeId factTable_;

  // This is the set of tables are are joined before
  // the fact table is joined. Generally this should
  // be a union of the tables in the lists of sets below.
  CANodeIdSet nodesJoinedBeforeFactTable_;

  // list of fringes that are joined to the fact table via clustering
  // key prefix predicates
  NAList<CANodeIdSet> * listOfEdges_;

  // optimal position of FactTable in list of edges
  Int32 optimalFTLocation_;

  // Nodes that are not part of the fringes
  CANodeIdSet availableNodes_;

  // flag indicating if the Star Schema was matched
  // in the MJStarJoinRule topMatch
  NABoolean matchedStarSchema_;

  CollHeap *heap_;
};

// ------------------------------------------------------------------------
// Base class for the two star join LSRs
// 1. Star Join Type I
//    * This produces a plan with a nested join into the fact table
// 2. Star Join Type II
//    * This produces a plan with the fact table as the outer most
//      in a series of hash joins
// ------------------------------------------------------------------------

class MJStarJoinRules : public Rule
{
public:
  MJStarJoinRules(const char * name,
                RelExpr * pattern,
                RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  // copy ctor
  MJStarJoinRules (const MJExpandRule &) ; // not written

  virtual ~MJStarJoinRules() {}

  /*virtual NABoolean topMatch (RelExpr * expr,
			      Context * context);

  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context *context,
				   RuleSubstituteMemory * & memory);

  virtual NABoolean isImplementationRule ();*/

  virtual NABoolean isAStarJoinRule() const = 0;

protected:
  //extends an edge of the star
  CANodeIdSet extendEdge(CANodeId connectedTable,//in
                         CANodeIdSet& availableNodes,//in
                         UInt32 lookAhead);//in
};

// ------------------------------------------------------------------------
// The MultiJoin StarJoin Type I  Rule
// This is a Large-Scope Rule
// It tries to match the pattern for a classic star schema
// and if the shape matches it produces a plan with a nested
// join into the fact table
// If nested join into the fact table does not make sense
// the topMatch returns false and schedules the MJStarBDRule
// ------------------------------------------------------------------------

class MJStarJoinIRule : public MJStarJoinRules
{
public:
  MJStarJoinIRule (const char * name,
                RelExpr * pattern,
                RelExpr * substitute) :
       MJStarJoinRules(name,pattern,substitute) {}

  // copy ctor
  MJStarJoinIRule (const MJStarJoinIRule &) ; // not written

  virtual ~MJStarJoinIRule() {}

  virtual NABoolean topMatch (RelExpr * expr,
			      Context * context);

  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context *context,
				   RuleSubstituteMemory * & memory);

  virtual NABoolean isImplementationRule () const { return FALSE; }
  virtual NABoolean isAStarJoinRule() const {return TRUE; }

private:
  NABoolean topMatch_Old(RelExpr * expr,
                         Context * context);

  RelExpr * nextSubstitute_Old(RelExpr * before,
                               Context *context,
                               RuleSubstituteMemory * & memory);
  //helper methods
  //Sort JBBCs of the MultiJoin on which this rule is firing.
  //The sorting order is based on the cardinality of the JBBC after application
  //of local predicates on the prefix of the clustering key.
  void sortMJJBBCsByCardAfterLocalKeyPrefixPred(NAList<CANodeId> &sortedJBBCs,//out
                                                const MultiJoin * const mjoin);//in

  // return the fact table if one is found otherwise return a CANodeId of
  // NULL_CA_ID
  CANodeId findFactTable(const MultiJoin * const mjoin, //in
                         CostScalar & factTableCKPrefixCardinality, //out
                         CANodeId & biggestNode); //out

  // does the given multijoin resemble a star shape
  // e.g.
  //                t1
  //                 |
  //                 |
  //                 |
  //     t2---------t3-----------t4
  //                 |
  //                 |
  //                 |
  //                t5
  //
  // The method will return the center of the star, in this case
  // it should be t3.
  // Note this does not necessarily mean it is a star schema
  // since t3 might be a small table.
  // For the above schema to be a star schema the center should be
  // a large fact table
  // This method just looks and table connectivities and returns the
  // center of the shape, otherwise it returns NULL_CA_ID
  CANodeId isAStarShape(MultiJoin * mjoin);

  // This method give a fact table, matches a multi-join to a star pattern
  NABoolean isAStarPattern(MultiJoin * mjoin,//in
                           CANodeId factTable,//in
                           CostScalar factTableCKPrefixCardinality);//in

  // get a rough estimate of cost for doing a nested join on the fact table
  // number of probes = dataFlowFromEdge
  // Rows of fact table that will be scanned = factTableRowsToScan
  CostScalar computeCostForFactTable(CostScalar probes,
                                     CostScalar factTableRowsToScan,
                                     CANodeId   factTable,
                                     MultiJoin *mjoin);

  // sort the JBBCs based on cardinality after local predicates
  // the sorting order is descending i.e. largest -> smallest
  void sortMJJBBCs(NAList<CANodeId> &sortedJBBCs,
                   const MultiJoin * const mjoin);

};

// ------------------------------------------------------------------------
// The MultiJoin StarJoin  Rule
// This is a Large-Scope Rule
// ------------------------------------------------------------------------

class MJStarJoinIIRule : public MJStarJoinRules
{
public:
  MJStarJoinIIRule (const char * name,
                    RelExpr * pattern,
                    RelExpr * substitute) :
       MJStarJoinRules(name,pattern,substitute) {}

  // copy ctor
  MJStarJoinIIRule (const MJExpandRule &) ; // not written

  virtual ~MJStarJoinIIRule() {}

  virtual NABoolean topMatch (RelExpr * expr,
			      Context * context);

  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context *context,
				   RuleSubstituteMemory * & memory);

  virtual NABoolean isImplementationRule () const { return FALSE; }
  virtual NABoolean isAStarJoinRule() const {return TRUE; }

private:
  RelExpr * nextSubstitute_Old (RelExpr * before,
                                Context *context,
                                RuleSubstituteMemory * & memory);
};

class JoinCommutativityRule : public Rule
{
public:
  JoinCommutativityRule (const char * name,
                         RelExpr * pattern,
                         RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  // copy ctor
  JoinCommutativityRule (const JoinCommutativityRule &) ; // not written

  virtual ~JoinCommutativityRule() {}

  virtual NABoolean topMatch (RelExpr * expr,
			      Context * context);

  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context *context,
				   RuleSubstituteMemory * & memory);

};

class JoinLeftShiftRule : public Rule
{
public:
  JoinLeftShiftRule (const char * name,
                     RelExpr * pattern,
                     RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  // copy ctor
  JoinLeftShiftRule (const JoinLeftShiftRule &) ; // not written

  virtual ~JoinLeftShiftRule() {}

  virtual NABoolean topMatch (RelExpr * expr,
			      Context * context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context *context,
				   RuleSubstituteMemory * & memory);

  virtual Guidance * guidanceForExploringChild(Guidance *, Context *, Lng32);

  virtual Guidance * guidanceForExploringSubstitute(Guidance * guidance);

  virtual Guidance * guidanceForOptimizingSubstitute(Guidance * guidance,
						     Context * context);

};

class IndexJoinRule1 : public Rule
{
public:
  IndexJoinRule1 (const char * name,
                  RelExpr * pattern,
                  RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  // copy ctor
  IndexJoinRule1 (const IndexJoinRule1 &) ; // not written

  virtual ~IndexJoinRule1() {}
  virtual NABoolean topMatch (RelExpr * expr,
			      Context * context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context *context,
				   RuleSubstituteMemory * & memory);

  // non-virtual methods to do the work for both pass 1 and pass 2
  RelExpr * nextSubstituteForPass(RelExpr * before,
				  RuleSubstituteMemory * & memory,
				  Lng32 pass);
  RelExpr * makeSubstituteFromIndexInfo(Scan *bef,
					ScanIndexInfo *ixi);
};

class IndexJoinRule2 : public IndexJoinRule1
{
public:
  IndexJoinRule2 (const char * name,
                  RelExpr * pattern,
                  RelExpr * substitute) :
       IndexJoinRule1(name,pattern,substitute) {}

  // copy ctor
  IndexJoinRule2 (const IndexJoinRule2 &) ; // not written

  virtual ~IndexJoinRule2() {}
  virtual NABoolean topMatch (RelExpr * expr,
			      Context * context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context *context,
				   RuleSubstituteMemory * & memory);
};

class OrOptimizationRule : public Rule
{
public:
  OrOptimizationRule (const char * name,
		      RelExpr * pattern,
		      RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  // copy ctor
  OrOptimizationRule (const OrOptimizationRule &) ; // not written

  virtual ~OrOptimizationRule() {}
  virtual NABoolean topMatch (RelExpr * expr,
			      Context * context);

  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context *context,
				   RuleSubstituteMemory * & memory);

private:

  CostScalar rateIndexForColumn(Int32 colNumInIndex,
				Scan *s,
				IndexDesc *ixDesc,
				NABoolean indexOnly);
  RelExpr * makeSubstituteScan(Scan *s,
			       const ValueIdSet &disjuncts,
			       RelExpr *partialResult,
			       const ValueIdSet disjunctsProcessedSoFar,
			       const ValueIdList &origCharOutputList,
			       ValueIdList &resultCharOutputs);

};

class TSJRule : public Rule
{
public:
  TSJRule (const char * name,
           RelExpr * pattern,
           RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  // copy ctor
  TSJRule (const TSJRule &) ; // not written

  virtual ~TSJRule() {}

  virtual NABoolean topMatch (RelExpr * expr,
                              Context * context);

  virtual RelExpr * nextSubstitute(RelExpr * before,
                                   Context *context,
                                   RuleSubstituteMemory * & memory);
};

class RoutineJoinToTSJRule : public Rule
{
public:
  RoutineJoinToTSJRule(const char * name,
                RelExpr * pattern,
                RelExpr * substitute) : 
       Rule(name,pattern,substitute) {}

  // copy ctor
  RoutineJoinToTSJRule (const RoutineJoinToTSJRule &) ; // not written

  virtual ~RoutineJoinToTSJRule();
  virtual NABoolean topMatch (RelExpr * relExpr,
                              Context * context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
                                   Context * context,
                                   RuleSubstituteMemory * & memory);
  virtual Int32 promiseForOptimization(RelExpr * relExpr,
                                     Guidance * guidance,
                                     Context * context);

  virtual NABoolean canBePruned(RelExpr * expr) const;

};

class JoinToTSJRule : public Rule
{
public:
  JoinToTSJRule (const char * name,
                 RelExpr * pattern,
                 RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  // copy ctor
  JoinToTSJRule (const JoinToTSJRule &) ; // not written

  virtual ~JoinToTSJRule() {}

  virtual NABoolean topMatch (RelExpr * expr,
			      Context * context);

  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context *context,
				   RuleSubstituteMemory * & memory);
  virtual Int32 promiseForOptimization(RelExpr * relExpr,
				       Guidance * guidance,
				       Context * context);
  virtual NABoolean canBePruned(RelExpr * expr) const;
};

class TSJFlowRule : public Rule
{
public:
  TSJFlowRule (const char * name,
               RelExpr * pattern,
               RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  // copy ctor
  TSJFlowRule (const TSJFlowRule &) ; // not written

  virtual ~TSJFlowRule() {}

  virtual NABoolean topMatch (RelExpr * expr,
			      Context * context);

  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context *context,
				   RuleSubstituteMemory * & memory);
};

// Raj P - 10/2000
// Rule to support CALL <stored-proc>
class TSJUDRRule : public Rule
{
public:
  TSJUDRRule (const char * name,
               RelExpr * pattern,
               RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  virtual ~TSJUDRRule() {}

  virtual NABoolean topMatch (RelExpr * expr,
			      Context * context);

  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context *context,
				   RuleSubstituteMemory * & memory);

  virtual NABoolean isContextSensitive() const;

private:
  // default constructor, not written
  TSJUDRRule ();

  // copy ctor
  TSJUDRRule (const TSJUDRRule &) ; // not written

};

class FilterRule : public Rule
{
public:
  FilterRule(const char * name,
             RelExpr * pattern,
             RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  // copy ctor
  FilterRule (const FilterRule &) ; // not written

  virtual ~FilterRule() {}
  virtual Guidance * guidanceForExploringChild(Guidance * guidance,
					       Context * context,
					       Lng32 childIndex);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context *context,
				   RuleSubstituteMemory * & memory)=0;
  virtual Int32 promiseForOptimization(RelExpr * relExpr,
				       Guidance * guidance,
				       Context * context);
};

class FilterRule0 : public FilterRule
{
public:
  FilterRule0(const char * name,
              RelExpr * pattern,
              RelExpr * substitute) :
       FilterRule(name,pattern,substitute) {}

  // copy ctor
  FilterRule0 (const FilterRule0 &) ; // not written

  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context *context,
				   RuleSubstituteMemory * & memory);

};

class FilterRule1 : public FilterRule
{
public:
  FilterRule1(const char * name,
              RelExpr * pattern,
              RelExpr * substitute) :
       FilterRule(name,pattern,substitute) {}

  // copy ctor
  FilterRule1 (const FilterRule1 &) ; // not written

  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context *context,
				   RuleSubstituteMemory * & memory);

};

class FilterRule2 : public FilterRule
{
public:
  FilterRule2(const char * name,
              RelExpr * pattern,
              RelExpr * substitute) :
       FilterRule(name,pattern,substitute) {}

  // copy ctor
  FilterRule2 (const FilterRule2 &) ; // not written

  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context *context,
				   RuleSubstituteMemory * & memory);

};

class GroupByEliminationRule : public Rule
{
public:
  GroupByEliminationRule(const char * name,
                         RelExpr * pattern,
                         RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  // copy ctor
  GroupByEliminationRule (const GroupByEliminationRule &) ; // not written

  virtual ~GroupByEliminationRule();
  virtual NABoolean topMatch (RelExpr * expr,
			      Context * context);
  virtual Int32 promiseForOptimization(RelExpr * relExpr,
				       Guidance * guidance,
				       Context * context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context *context,
				   RuleSubstituteMemory * & memory);
};

class GroupByOnJoinRule : public Rule
{
public:
  GroupByOnJoinRule(const char * name,
                    RelExpr * pattern,
                    RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  // copy ctor
  GroupByOnJoinRule (const GroupByOnJoinRule &) ; // not written

  virtual ~GroupByOnJoinRule();

  virtual NABoolean topMatch (RelExpr * expr,
			      Context * context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
};

class PartialGroupByOnTSJRule : public Rule
{
public:
  PartialGroupByOnTSJRule(const char * name,
                          RelExpr * pattern,
                          RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  // copy ctor
  PartialGroupByOnTSJRule (const PartialGroupByOnTSJRule &) ; // not written

  virtual ~PartialGroupByOnTSJRule();

  virtual NABoolean topMatch (RelExpr * expr,
			      Context * context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
};

class GroupBySplitRule : public Rule
{
public:
  GroupBySplitRule(const char * name,
                   RelExpr * pattern,
                   RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  // copy ctor
  GroupBySplitRule (const GroupBySplitRule &) ; // not written

  virtual ~GroupBySplitRule();

  virtual NABoolean topMatch (RelExpr * expr,
			      Context * context);
  virtual Int32 promiseForOptimization(RelExpr * relExpr,
				       Guidance * guidance,
				       Context * context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
  virtual NABoolean canMatchPattern (const RelExpr * pattern) const;
};

class AggrDistinctEliminationRule : public Rule
{
public:
  AggrDistinctEliminationRule(const char * name,
                              RelExpr * pattern,
                              RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  // copy ctor
  AggrDistinctEliminationRule (const AggrDistinctEliminationRule &) ; // not written

  virtual ~AggrDistinctEliminationRule();
  virtual NABoolean topMatch (RelExpr * expr,
			      Context * context);
  virtual RelExpr * nextSubstitute (RelExpr * before,
		    	            Context * context,
			            RuleSubstituteMemory * & memory);
  virtual NABoolean canMatchPattern (const RelExpr * pattern) const;
};

class GroupByTernarySplitRule : public GroupBySplitRule
{
public:
  GroupByTernarySplitRule(const char * name,
                          RelExpr * pattern,
                          RelExpr * substitute) :
       GroupBySplitRule(name,pattern,substitute) {}

  // copy ctor
  GroupByTernarySplitRule (const GroupByTernarySplitRule &) ; // not written

  virtual ~GroupByTernarySplitRule();
  virtual NABoolean isContextSensitive() const;
  virtual NABoolean topMatch (RelExpr * expr,
			      Context * context);
  virtual NABoolean canMatchPattern (const RelExpr * pattern) const;
};

class ShortCutGroupByRule : public Rule
{
public:
  ShortCutGroupByRule (const char * name,
                      RelExpr * pattern,
                      RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  // copy ctor
  ShortCutGroupByRule (const ShortCutGroupByRule &) ; // not written

  virtual ~ShortCutGroupByRule();
  virtual NABoolean topMatch (RelExpr * relExpr,
			      Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
  virtual NABoolean canMatchPattern (const RelExpr * pattern) const;
};

class CommonSubExprRule : public Rule
{
public:
  CommonSubExprRule (const char * name,
                     RelExpr * pattern,
                     RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  // copy ctor
  CommonSubExprRule (const CommonSubExprRule &) ; // not written

  virtual ~CommonSubExprRule();
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context * context,
				   RuleSubstituteMemory * & memory);
  virtual NABoolean canMatchPattern (const RelExpr * pattern) const;
};

class SampleScanRule : public Rule
{
public:
  SampleScanRule(const char * name,
                 RelExpr * pattern,
                 RelExpr * substitute) :
       Rule(name,pattern,substitute) {}

  // copy ctor
  SampleScanRule (const SampleScanRule &) ; // not written

  virtual ~SampleScanRule() {}
  virtual NABoolean topMatch (RelExpr * expr,
			      Context * context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context *context,
				   RuleSubstituteMemory * & memory);

  virtual Int32 promiseForOptimization(RelExpr * relExpr,
				       Guidance * guidance,
				       Context * context);
};


//++MV,
class JoinToBushyTreeRule : public Rule
{
public:
  JoinToBushyTreeRule(const char * name,
			    RelExpr * pattern,
			    RelExpr * substitute) :
                                         Rule(name,pattern,substitute) {}
  virtual ~JoinToBushyTreeRule() {}

  virtual NABoolean topMatch (RelExpr * expr,
			      Context * context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
				   Context *context,
				   RuleSubstituteMemory * & memory);

};

//--MV

/*
class HbaseScanRule : public Rule
{
public:
  HbaseScanRule(const char * name,
                RelExpr * pattern,
                RelExpr * substitute) :
  Rule(name,pattern,substitute) {}

  // copy ctor
  HbaseScanRule (const HbaseScanRule &) ; // not written

  virtual ~HbaseScanRule();
  virtual NABoolean topMatch (RelExpr * relExpr,
                              Context *context);
  virtual RelExpr * nextSubstitute(RelExpr * before,
                                   Context * context,
                                   RuleSubstituteMemory * & memory);
};
*/


// -----------------------------------------------------------------------
// Work areas to share work done between MJ Rules (i.e. LSRs) and also
// between topMatch and nextSubstitute of an MJ Rule
// -----------------------------------------------------------------------

// Work area used by MJ Rules (i.e. LSRs), to store common information shared
// between the various LSRs. This helps avoids recomputation of this information
// for each seperate LSR
class MJRulesWA : public NABasicObject
{
  friend class MJStarJoinRule;
  friend class MJStarJoinIRule;
  friend class MJStarJoinIIRule;
  friend class MJStarBDRule;

public:
  ~MJRulesWA(){};
  MJRulesWA(JBBSubsetAnalysis * analysis);
  CANodeId computeCenterTable();

private:

  // information set by method MJRulesWA::getCenterTable()
  // begin
  // the center table in the connectivity graph
  NABoolean centerTableComputed_;
  CANodeId centerTable_;
  CostScalar centerTableRowsScanned_;
  CostScalar centerTableDataScanned_;
  CostScalar centerTableDataPerPartition_;
  CostScalar centerTablePartitions_;
  // number of tables connected to the center table
  UInt32 centerTableConnectivity_;
  // max number of tables connected to other tables,
  // i.e. table apart from the center table
  UInt32 maxDimensionConnectivity_;
  // end

  // information set by the MJRulesWA::findFactTable
  // begin
  // fact table used in the star join I and star join II rules
  // a fact table significantly larger (e.g. 5x) than all other
  // tables in the JBBSubset. But if the largest table in the
  // JBBSubset is not significantly larger then this is set to
  // NULL_CA_ID
  CANodeId factTable_;
  // this is the largest table in the JBBSubset.
  CANodeId largestTable_;
  // this is the largest node in the JBBSubset, it is not necessarily
  // a table, could be a subquery, e.g. group by.
  CANodeId largestNode_;
  // end

  // pointer to the JBBSubsetAnalysis to which this guy belongs
  JBBSubsetAnalysis * analysis_;
};

// -----------------------------------------------------------------------
// OnceGuidance: instruct not to apply a rule on an expression
// (usually used on rules like the commutativity rule that produce
// the original expression when applied twice)
// -----------------------------------------------------------------------
class OnceGuidance : public Guidance
{
public:

  // ctor
  OnceGuidance(NAUnsigned exceptRule, NAMemory * h);

  // copy ctor
  OnceGuidance (const OnceGuidance &) ; // not written

  virtual ~OnceGuidance();

  virtual const RuleSubset * applicableRules();

private:

  RuleSubset allButOne_; // all applicable rules minus "exceptRule"
};

// -----------------------------------------------------------------------
// StopGuidance: stop doing anything (don't apply any more rules)
// -----------------------------------------------------------------------
class StopGuidance : public Guidance
{
public:

  StopGuidance(NAMemory * h);

  // copy ctor
  StopGuidance (const StopGuidance &) ; // not written

  virtual ~StopGuidance();

  virtual const RuleSubset * applicableRules();

private:

  RuleSubset emptySet_; // an empty rule subset
};

// -----------------------------------------------------------------------
// FilterGuidance:
//   Only allow the filter rules to be applied - i.e.
// FilterRule0, FilterRule1, and FilterRule2. Used when exploring
// the child of a filter so that double filter nodes - i.e. a
// filter that is a child of a filter - are processed correctly.
//
// This is necessary because FilterRule1, which is the rule that
// would process a filter over a filter (since a filter is a unary
// non-leaf operator), will refuse to do anything if it's child is
// a filter. It assumes that the filter child will be merged with
// it's child when exploring.  But the filter rule used to issue
// "StopGuidance", above, so this would never happen during exploring.
// It also never happens during optimization, because there is no
// implementation rule for a filter, and rules are only issued
// for the immediate child of an operator during optimization if the
// parent was actually implemented. Prior to adding this guidance
// class, once one filter ended up on top of another filter,
// the "double filter" could never be eliminated. One solution
// would be to issue no guidance at all, so all rules could fire.
// This was not done because it was assumed that the reason
// "StopGuidance" was issued was because applying all the rules to
// the child of a filter was causing a performance problem,
// since all the rules would have to be applied on the child
// again after the filter was merged with the child. By
// issuing a guidance that only allows the filter rules to fire,
// additional work will only be performed if we are indeed processing
// a "double filter". These were never being processed before, so
// it is not possible that this will result in any new unnecessary work.
// -----------------------------------------------------------------------
class FilterGuidance : public Guidance
{
public:

  FilterGuidance(NAMemory * h);

  // copy ctor
  FilterGuidance (const StopGuidance &) ; // not written

  virtual ~FilterGuidance();

  virtual const RuleSubset * applicableRules();

private:

  RuleSubset filterRules_; // FilterRule0, FilterRule1, FilterRule2
};

#endif // TRANSRULE_H

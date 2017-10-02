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
#ifndef ANALYZER_H
#define ANALYZER_H
/* -*-C++-*-
 **************************************************************************
 *
 * File:         Analyzer.h
 * Description:  Query Analyzer classes and methods
 * Created:      08/17/2001
 * Language:     C++
 *
 *
 *
 **************************************************************************
 */

#include "GroupAttr.h"
#include "MVCandidates.h"
#include "RelRoutine.h"
#include "CmpStatement.h"

// Classes Defined in this file

class QueryAnalysis;
class GroupAnalysis;
class NodeAnalysis;
class JBBC;
class TableAnalysis;
class GBAnalysis;
class AccessPathAnalysis;
class TableConnectivityGraph;
class JBB;
class JBBWA;
class JBBItem;
class JBBSubset;
class JBBSubsetAnalysis;
class JBBCompItem;
class JBBOrderedItem;
class CANodeId;
class CANodeIdSet;
class PredAnalysis;
class ColAnalysis;
class CqsWA;
class QueryComplexityVector;

// Forward declarations

class AppliedStatMan;
class RelExpr;
class Join;
class Scan;
class GroupByAgg;
class MJRulesWA;
class MJStarJoinRuleWA;
class MJStarJoinIRuleWA;
class MJStarBDRuleWA;
class MultiJoin;
class MvQueryRewriteHandler;
class QRMVDescriptor;
class RoutineDesc;

// enum to indicate the legality of a JBBSubset
enum SUBSET_LEGALITY{
  NOT_KNOWN,
  LEGAL,
  ILLEGAL
};

// ------------------------------------------------------------------------
// The following method will be added to GroupAttributes and ValueId classes

NAString istring (ULng32 i);
NAString valueIdSetGetText(const ValueIdSet & set);

// -----------------------------------------------------------------------
// CANodeId: Most Exprs analyzed during the CA phase are given a unique
// CA Id which is used to simplify and speed up the set operations.
// Currently only JBBCs and base tables scan nodes are given a CANodeId.
// Also a group by on top of a JBB is given a unique grouping CANodeId.
// -----------------------------------------------------------------------
class CANodeId
{

  friend class CANodeIdSet; // to access id_

public:

  // Constructor, default constructor, and copy constructor
  CANodeId (CollIndex x = 0) : id_(x)
  {}

  // Destructor.
  ~CANodeId()
  {}

  // Automatic conversion operator to type CollIndex.
  inline operator CollIndex () const
  {
    return id_;
  }

  // Comparison operators
  inline NABoolean operator == (const CANodeId & other)
  {
    return id_ == other.id_;
  }

  inline NABoolean operator != (const CANodeId & other)
  {
    return id_ != other.id_;
  }


  // Get NodeAnalysis structure for this CANodeId. This method will
  // return null if there is no NodeAnalysis for this  CANodeId.
  // Method defined at bottom of this file
  inline NodeAnalysis * getNodeAnalysis();

  // returns the input to the JBB
  // assumes the CANodeId represents a JBBC
  EstLogPropSharedPtr getJBBInput();

  const NAString getText() const
  {
    return NAString("CANodeId: ") + istring(id_);
  }
  
  inline ValueIdSet getUsedTableCols();
  inline const UInt32 toUInt32() const { return id_; };
  
private:

  // an index into the CANodeArray
  CollIndex id_;

};

// Define a constant for an invalid CANodeId
const CANodeId NULL_CA_ID((CollIndex) 0);


// -----------------------------------------------------------------------
// CANodeIdSet : A collection of CANodeIds
// -----------------------------------------------------------------------

class CANodeIdSet : public SUBARRAY(NodeAnalysis *)
{
public:

  // Constuctor
  CANodeIdSet(CollHeap *outHeap = CmpCommon::statementHeap());

  // Copy constructor
  CANodeIdSet(const CANodeIdSet &other,
              CollHeap *outHeap = CmpCommon::statementHeap());

  CANodeIdSet(const CANodeId& id,
              CollHeap *outHeap = CmpCommon::statementHeap());

  // Destructor
  virtual ~CANodeIdSet()
  {}

  // Comparison operators
  inline NABoolean operator == (const CANodeIdSet &other) const
  {
    return SUBARRAY(NodeAnalysis *)::operator ==(other);
  }

  CANodeIdSet operator +(const CANodeIdSet &) const;

  inline NABoolean operator != (const CANodeIdSet &other) const
  {
    return NOT operator==(other);
  }

  // ---------------------------------------------------------------------
  // Iterator methods for a CANodeIdSet
  // use the iterators in a for loop like this (assuming you have a
  // CANodeIdSet S over which you want to iterate)
  // for (CANodeId x= S.init();  S.next(x); S.advance(x) )
  //    { /* x is the current element */ }
  // ---------------------------------------------------------------------
  CANodeId init() const
  {
    return CANodeId((CollIndex) 0);
  }

  NABoolean next(CANodeId & x) const
  {
    return nextUsed(x.id_);
  }

  void advance(CANodeId & x) const
  {
    x.id_++;
  }

  NABoolean containsThisId(CANodeId &x) const
  {
    CANodeId id;
    for (id=init(); next(id); advance(id))
    {
      if (id.operator==(x))
        return TRUE;
    }
    return FALSE;
  }

  // get first element (if empty return NULL_CA_ID)
  CANodeId getFirst() const;

  // compute the JBBSubset structure for this CANodeIdSet
  // Note that not every CANodeIdSet has a JBBSubset (for example
  // all JBBCs has to be of the same JBB). The method verify()
  // on JBBSubset can be used to verify this condition.
  JBBSubset * computeJBBSubset() const;

  // returns the input to the JBB
  // assumes the CANodeIdSet represents a JBBSubset
  EstLogPropSharedPtr getJBBInput() const;
  
  // This method also compute a JBBSubset but much faster than
  // computJBBSubset() because it assume the CANodeIdSet contains
  // only sibling JBBCs. Use only if that condition is true (which
  // is the case for most CANodeIdSet values generated by getJoinedJBBCs()
  // and similar methods on JBBC and JBBSubset).
  JBBSubset * jbbcsToJBBSubset() const;

  // check if this CANodeIdSet is legal i.e.
  // predecessor of each node is found within
  // this set
  NABoolean legal() const;

  // will adding this node to the current set result in a legal set
  // assumption is that current set by itself is legal
  NABoolean legalAddition(CANodeId newNode) const;

  ValueIdSet  getUsedCols() const;
  
  ValueIdSet getUsedTableCols();

  // Find the jbbc from the jbbsubset, that has minimum connections
  // If there are more than one jbbcs, with the same number of
  // connections, pick the first one.
  CANodeId getJBBCwithMinConnectionsToThisJBBSubset() const;

  CostScalar getMinChildEstRowCount() const;
  const NAString getText() const;

  void display() const;

  void print (FILE *f = stdout,
	      const char * prefix = DEFAULT_INDENT,
	      const char * suffix = "") const;

};

// -----------------------------------------------------------------------
// CANodeIdSetMap : Generic Hash Table Map keyed by CANodeIdSet
// and storing JBBSubsetAnalysis
// -----------------------------------------------------------------------

class CANodeIdSetMap : public HASHDICTIONARY(CANodeIdSet, JBBSubsetAnalysis)
{
public:

  static ULng32 Hasher(const CANodeIdSet & key);
  enum { Default_Size = 107 };

  // Constuctor
  CANodeIdSetMap(ULng32 init_size = Default_Size,
                 CollHeap *outHeap = CmpCommon::statementHeap());

  // Destructor
  virtual ~CANodeIdSetMap()
  {}

  inline JBBSubsetAnalysis* get(const CANodeIdSet & key) const
  {
    return getFirstValue(&key);
  }

  JBBSubsetAnalysis* getFirstValue(const CANodeIdSet* key) const;

private:

  Lng32     hits_;    // # of cache hits
  Lng32     misses_;  // # of cache misses

};

//-----------------------------------------------------------------------
// QueryAnalysis Class
// Single point of access for global query analysis results.
// Example of usage (get ColAnalysis for a certain column):
// QueryAnalysis* queryAnalysis = QueryAnalysis::Instance();
// ColAnalysis* col = queryAnalysis->getColAnalysis(colValueId);
//-----------------------------------------------------------------------
class QueryAnalysis : public NABasicObject
{

public:

  // construct a QueryAnalysis
  QueryAnalysis(CollHeap *outHeap = CmpCommon::statementHeap(),
                NABoolean analysis = TRUE);

  // destruct a QueryAnalysis
  virtual ~QueryAnalysis();

  // The Singleton instance
  inline static QueryAnalysis * Instance()
  {
    //return queryAnalysis_;
    return (CmpCommon::statement()) ? 
           CmpCommon::statement()->getQueryAnalysis() : NULL;
  }

  // List all JBBs in the query
  inline const ARRAY(JBB*) & getJBBs()
  {
    return jbbArray_;
  }

  // get all CANodes in the query tree that are JBBCs
  inline const CANodeIdSet & getJBBCs()
  {
    return allJBBCs_;
  }

  // get all CANodes in the query tree that are base tables
  // Method defined at bottom of this file
  inline const CANodeIdSet & getTables();

  // get all CANodes in the query tree that are group Bys at CA phase
  inline const CANodeIdSet & getGBs()
  {
    return allGBs_;
  }

  inline const CANodeIdSet & getLeftJoinJBBCs()
  {
    return leftJoinJBBCs_;
  }
  
  inline const CANodeIdSet & getSemiJoinJBBCs()
  {
    return semiJoinJBBCs_;
  }

  inline const CANodeIdSet & getRoutineJoinJBBCs()
  {
    return routineJoinJBBCs_;
  }
  
  inline const CANodeIdSet & getInnerNonSemiNonTSJJBBCs()
  {
    return innerNonSemiNonTSJJBBCs_;
  }

  // direct access to the ASM of the singleton
  inline static AppliedStatMan * ASM()
  {
    return Instance()->getASM();
  }

  // get the ASM of this object
  inline AppliedStatMan * getASM() const
  {
    return appStatMan_;
  }

  // direct access to the TableConnectivityGraph of the singleton
  inline static TableConnectivityGraph * TCG()
  {
    return Instance()->getTableConnectivityGraph();
  }

  // get the TableConnectivityGraph of this object
  inline TableConnectivityGraph * getTableConnectivityGraph()
  {
    return tCGraph_;
  }

  // get the query complexity vector
  inline QueryComplexityVector * getQueryComplexityVector()
  { 
    return queryComplexityVector_;
  }
  
  // get the set of all base columns in the query
  // Method defined at bottom of this file
  inline const ValueIdSet & getColumns();

  // Fast lookup (array lookup) for NodeAnalysis using CANodeId
  inline NodeAnalysis * getNodeAnalysis(CollIndex id)
  {
    if (nodeAnalysisArray_.used(id))
    return nodeAnalysisArray_[id];
    return NULL;
  }

  // Fast lookup (array lookup) for ColAnalysis using ValueId
  inline ColAnalysis * getColAnalysis(CollIndex id)
  {
    if (colAnalysisArray_.used(id))
    return colAnalysisArray_[id];
    return NULL;
  }

  // Fast lookup (array lookup) for ColAnalysis using ValueId
  inline CANodeIdSet * getProducingJBBCs(CollIndex id)
  {
    if (outputToJBBCsMap_.used(id))
    return outputToJBBCsMap_[id];
    return NULL;
  }

  // add an expression that is completely covered by the outputs of the jbbc
  void addProducingJBBC(ValueId expr, CANodeId jbbc);

  // Fast lookup (array lookup) for PredAnalysis using ValueId
  inline PredAnalysis * getPredAnalysis(CollIndex id)
  {
    return predAnalysisArray_[id];
  }

  // Create PredAnalysis for this valueId if none was created so far
  PredAnalysis* findPredAnalysis(ValueId x);

  // Lookup a JBB by its id
  inline JBB * getJBB(CollIndex id)
  {
    return jbbArray_[id];
  }

  // Lookup a JBBSubsetAnalysis by JBBSubset
  JBBSubsetAnalysis * findJBBSubsetAnalysis(const JBBSubset & subset);

  // get JBBC-Expr Map. Used for initial building of MultiJoin nodes
  // and any lookup for RelExpr based on JBBC id at analysis phase.
  //inline const JBBCExprGroupMap & getJBBCExprMap()
  //{
    //return jbbcExprMap_;
  //}

  // check if this RelExpr was a top expr of a JBB during analysis phase
  // This is a temp way to mark expressions for WPC rule.
  inline NABoolean isOriginalJBBTopExpr(RelExpr* expr)
  {
    return origJBBTopExprs_.contains(expr);
  }

  // returns true if current query has a mandatory XP
  NABoolean hasMandatoryXP();

  // Analyze the query tree
  RelExpr* analyzeThis(RelExpr* expr, NABoolean noMVQR = FALSE);

  // Cleanup if pilot analysis phase failed
  void cleanup(RelExpr* expr);

  // Do the CQS fixup work if applicable
  void cqsRewrite(RelExpr* expr);

  // Is Analysis ON?
  NABoolean isAnalysisON()
  {
    return analysisON_;
  }

  // Find a nodemap that was created for this degree of paralleism
  inline NodeMap * getNodeMap(CollIndex degreeOfParallelism)
  {
    if (nodeMapArray_.used(degreeOfParallelism))
      return nodeMapArray_[degreeOfParallelism];
    return NULL;
  }

  // Insert the current node map in the global node map array
  inline void setNodeMap(NodeMap* map, CollIndex degree)
  {
    nodeMapArray_.insertAt(degree, map);
  }

  // Did MultiJoin Rewrite take place
  NABoolean multiJoinsUsed()
  {
    return multiJoinsUsed_;
  }

  NABoolean isCompressedHistsViable()
  {
    return compressedHistsViable_;
  }

  void disableCompressedHistsViable()
  {
    compressedHistsViable_ = FALSE;
  }

  // Should we optimize for FirstN optimization goal
  NABoolean optimizeForFirstNRows()
  {
    return optimizeForFirstNRows_;
  }

  // Use the join order as specified in SQL query FROM clause
  NABoolean joinOrderByUser()
  {
    return joinOrderByUser_;
  }

  void setJoinOrderByUser()
  {
    joinOrderByUser_ = TRUE;
  }

  // WaveFix Begin
  NABoolean dontSurfTheWave()
  {
    return dontSurfTheWave_;
  }

  void setDontSurfTheWave(NABoolean value)
  {
    dontSurfTheWave_ = value;
  }
  // WaveFix End

  // recursively do primeGroupAnalysis on subtree expressions
  // will move this method to RelExpr
  void primeGroupAnalysisForSubtree(RelExpr* expr);

  // analyze dependencies between JBBCs
  void analyzeJBBCDependencies(RelExpr* expr);

  // recursively do clearGroupAnalysis on subtree expressions
  // will move this method to RelExpr
  void clearAnalysis(RelExpr* expr);

  // Analysis object creation methods
  NodeAnalysis* newNodeAnalysis(RelExpr* expr);
  JBB* newJBB(RelExpr* expr);
  JBBC* newJBBC(Join* parent, RelExpr* expr, NABoolean isFullOuterJoinOrTSJJBBC=FALSE);
  TableAnalysis* newTableAnalysis(RelExpr* tableExpr);
  GBAnalysis* newGBAnalysis(GroupByAgg* gb);
  RoutineAnalysis * newRoutineAnalysis(RelExpr* routineExpr);

  // Do ASM precomputation phase
  void initializeASM();

  // Called after ASM initialization to compute nodes' stats
  void computeStatsForNodes();

  // Initialize flags for global directives
  void initializeGlobalDirectives(RelExpr * root);

  // Last step in table/column connectivity analysis
  // (Table connectivity graph)
  void finishConnectivityAnalysis();

  void checkIfCompressedHistsViable();

  // Find JBB with most JBBCs
  JBB* getLargestJBB();
  ULng32 getSizeOfLargestJBB();

  // Handle all the MV query rewrite stuff.
  RelExpr* handleMvQueryRewrite(RelExpr* expr);

  /* Do not inspect monitor members and methods */
  // Compile Time Monitors
  inline TaskMonitor & pilotPhaseMonitor()
  {
    return pilotPhaseMon_;
  }
  inline TaskMonitor & jbbSetupMonitor()
  {
    return jbbSetupMon_;
  }
  inline TaskMonitor & tcgSetupMonitor()
  {
    return tcgSetupMon_;
  }
  inline TaskMonitor & synthLogPropMonitor()
  {
    return synthLogPropMon_;
  }
  inline TaskMonitor & asmPrecompMonitor()
  {
    return asmPrecompMon_;
  }
  inline TaskMonitor & queryGraphMonitor()
  {
    return queryGraphMon_;
  }
  inline TaskMonitor & compilerMonitor()
  {
    return compilerMon_;
  }
  inline TaskMonitor & parserMonitor()
  {
    return parserMon_;
  }
  inline TaskMonitor & compCleanupMonitor()
  {
    return compCleanupMon_;
  }

  inline TaskMonitor & tempMonitor()
  {
    return tempMon_;
  }

  // Different compilation phases
  enum CompilerPhaseEnum
  {
    PRE_BINDER,
    BINDER,
    NORMALIZER,
    ANALYZER,
    OPTIMIZER,
    PRECODE_GENERATOR,
    GENERATOR,
    POST_GENERATOR
  };

  inline CompilerPhaseEnum getCompilerPhase()
  {
    return compilerPhase_;
  }


  inline void setCompilerPhase(CompilerPhaseEnum phase)
  {
    compilerPhase_ = phase;
  }

  inline TableAnalysis * getLargestTable()
  {
    return largestTable_;
  }

  inline void setLargestTable(TableAnalysis * largestTable)
  {
    largestTable_ = largestTable;
  }

  inline const CANodeIdSet & getTablesJoinedToLargestTable()
  {
    return joinedToLargestTable_;
  }

  // the out heap
  inline CollHeap* outHeap()
  {
    return heap_;
  }

  MvQueryRewriteHandler* getMvQueryRewriteHandler() { return mvQueryRewriteHandler_; }

  inline void setMvCreation(NABoolean state)
  {
    isMvCreation_ = state;
  }

  inline NABoolean isMvCreation()
  {
    return isMvCreation_;
  }

  inline void setSkippedSomeJoins()
  {
    skippedSomeJoins_ = TRUE;
  }

  inline NABoolean skippedSomeJoins()
  {
    return skippedSomeJoins_;
  }

  inline Lng32 getHighestNumOfPartns()
  {
    return highestNumOfPartns_;
  }

  void setHistogramsToDisplay(RelRoot * root);
  
  void graphDisplay() const;

  const NAString getText() const;

  void display() const;

  void print (FILE *f = stdout,
	      const char * prefix = DEFAULT_INDENT,
	      const char * suffix = "") const;

  void showQueryStats(const char *qText, CollHeap * space, char * buf);

private:

  TableConnectivityGraph*      tCGraph_;
  ARRAY(JBB*)                  jbbArray_;
  LIST(RelExpr*)               origJBBTopExprs_;
  CANodeIdSet                  allJBBCs_;
  CANodeIdSet                  allGBs_;
  CANodeIdSet                  leftJoinJBBCs_;
  CANodeIdSet                  semiJoinJBBCs_;
  CANodeIdSet                  routineJoinJBBCs_;
  CANodeIdSet                  innerNonSemiNonTSJJBBCs_;
  
  ARRAY(NodeAnalysis*)         nodeAnalysisArray_;
  // consider reuse ValueDesc for these two
  // only problem will lose possibility of multiple query analysis.
  ValueIdSet                   allUsedCols_;
  ARRAY(ColAnalysis*)          colAnalysisArray_;
  ARRAY(CANodeIdSet*)          outputToJBBCsMap_;
  ValueIdSet                   allPreds_;
  ARRAY(PredAnalysis*)         predAnalysisArray_;
  // array for node map reuse
  ARRAY(NodeMap*)              nodeMapArray_;
  //
  CANodeIdSetMap               jbbSubsetMap_;
  //JBBCExprGroupMap             jbbcExprMap_;
  AppliedStatMan*              appStatMan_;
  NABoolean                    analysisON_; // is Analysis ON
  NABoolean                    multiJoinsUsed_; // did MJRewrtie occure
  ULng32                multiJoinThreshold_; // do MJRewrite only if threshold is reached

  // Begin Global directive flags
  NABoolean                    optimizeForFirstNRows_;
  NABoolean                    joinOrderByUser_;
  NABoolean                    dontSurfTheWave_;// WaveFix
  NABoolean                    compressedHistsViable_;

  // End Global directive flags

  /* largest table in query */
  TableAnalysis * largestTable_;

  // set of tables joined to the largest table
  // these are connected via a simple join
  // not a outer/semi join
  CANodeIdSet joinedToLargestTable_;

  // this computes the set above
  void computeTablesJoinedToLargestTable();

  // fixup the triggers in the tree
  NABoolean fixupTriggers(RelExpr * expr);

  //mandatoryXPflags
  NABoolean                    hasMandatoryXPComputed_;
  NABoolean                    hasMandatoryXP_;

  // highest number of partitions among all tables 
  Lng32                         highestNumOfPartns_;

  /* Do not inspect monitor members and methods */
  // Comile Time Monitors
  TaskMonitor                  pilotPhaseMon_;
  TaskMonitor                  jbbSetupMon_;
  TaskMonitor                  tcgSetupMon_;
  TaskMonitor                  synthLogPropMon_;
  TaskMonitor                  asmPrecompMon_;
  TaskMonitor                  queryGraphMon_;

  // These Globals and Monitors will be moved to CompGlobals
  TaskMonitor                  compilerMon_;
  TaskMonitor                  parserMon_;
  TaskMonitor                  compCleanupMon_;
  TaskMonitor                  tempMon_;
  CompilerPhaseEnum            compilerPhase_;

  // MV Query Rewrite
  // We declare a pointer instead of an object to avoid a dependency on
  // QueryRewriteHandler.h in this file, which would cause a lot of
  // recompilation when the rewrite code changes.
  MvQueryRewriteHandler*       mvQueryRewriteHandler_;
  NABoolean		       isMvCreation_;
  NABoolean                    skippedSomeJoins_;
  
  QueryComplexityVector * queryComplexityVector_;

  // used by "showstats for query" feature
  EstLogPropSharedPtr statsToDisplay_;
  ValueIdList          selectListCols_;

  CollHeap*                    heap_;
};

class QueryComplexityVector: public NABasicObject
{
  public:
  
  QueryComplexityVector(CollHeap *outHeap = CmpCommon::statementHeap()):
  n_(-1),
  n2_(-1),
  n3_(-1),
  n4_(-1),
  exhaustive_(-1),
  heap_(outHeap)
  {}
  
  double getNComplexity(){return n_;}
  double getN2Complexity(){return n2_;}
  double getN3Complexity(){return n3_;}
  double getN4Complexity(){return n4_;}
  double getExhaustiveComplexity(){return exhaustive_;}
  
  void setNComplexity(double n){ n_ = n; }
  void setN2Complexity(double n2){ n2_ = n2; }
  void setN3Complexity(double n3){ n3_ = n3; }
  void setN4Complexity(double n4){ n4_ = n4; }
  void setExhaustiveComplexity(double exhaustive){ exhaustive_ = exhaustive; }

  private:
  
  //linear
  double n_;
  // n^2
  double n2_;
  // n^3
  double n3_;
  // n^4
  double n4_;
  // exhaustive: n*2^(n-1)
  double exhaustive_;
  
  CollHeap*                heap_;
};

// ------------------------------------------------------------------------
// GroupAnalysis class hold analysis results that are useful to the
// whole group.
// ------------------------------------------------------------------------
class GroupAnalysis : public NABasicObject
{

  friend class GroupAttributes; // GroupAnalysis is owned by GroupAttribte

public:

  // construct a GroupAnalysis
  GroupAnalysis(GroupAttributes* groupAttr,
	            CollHeap *outHeap = CmpCommon::statementHeap()):
    groupAttr_(groupAttr),
    localJBBView_(NULL),
    caNode_(NULL),
    heap_(outHeap)
  {}

  // copy constructor.
  GroupAnalysis(const GroupAnalysis & other,
                CollHeap *outHeap = CmpCommon::statementHeap());

  // destruct a GroupAnalysis
  virtual ~GroupAnalysis();

  // clear analysis results in GroupAnalysis.
  // used for queryAnalysis cleanup
  void clear();

  //reconcile this GroupAnalysis object with the GroupAnalysis
  //object passed in
  //This is used in case of Group Merges
  void reconcile(GroupAnalysis * other);

  // Comparison between two groups
  // should move this to GroupAttributes ==
  NABoolean operator == (const GroupAnalysis & other);

  // get the GroupAttr associated (1-1) with this GroupAn
  inline GroupAttributes * getGroupAttr() const
  {
    return groupAttr_;
  }

  // get the CA NodeAnalysis if any
  inline NodeAnalysis * getCANodeAnalysis() const
  {
    return caNode_;
  }
  inline NodeAnalysis * getNodeAnalysis() const
  {
    return caNode_;
  }
  inline void setNodeAnalysis(NodeAnalysis* node)
  {
    caNode_ = node;
  }

  // The local JBB view of the JBBSubset
  inline const JBBSubset * getLocalJBBView()
  {
    return localJBBView_;
  }
  inline void setLocalJBBView(JBBSubset* jbbSubset)
  {
    localJBBView_ = jbbSubset;
  }

  // The parent JBB view of the JBBSubset. If this group is a
  // JBBC then a JBBSubset of this JBBC alone is returned. Otherwise
  // the localJBBView_ is returned
  const JBBSubset * getParentJBBView() const;

  // get the set of all base tables in this subtree
  inline const CANodeIdSet & getAllSubtreeTables() const
  {
    return allSubtreeTables_;
  }
  inline void setSubtreeTables(const CANodeIdSet & tables)
  {
    allSubtreeTables_ = tables;
  }

  // list of all promising indexes in this subtree
  // this will replace availableBtreeIndexes in GroupAttr
  const LIST(AccessPathAnalysis*) & getAllSubtreePromisingAccessPaths();

  // the out heap
  inline CollHeap* outHeap()
  {
    return heap_;
  }

  const NAString getText() const;

  void display() const;

  void print (FILE *f = stdout,
	      const char * prefix = DEFAULT_INDENT,
	      const char * suffix = "") const;

private:

  GroupAttributes*         groupAttr_;
  CANodeIdSet              allSubtreeTables_;
  JBBSubset*               localJBBView_; // xxx add parentView as member too
  NodeAnalysis*            caNode_;
  CollHeap*                heap_;
};

// ------------------------------------------------------------------------
// NodeAnalysis class hold analysis results collected for a relExpr
// during the Connectivity Analyzer phase. Each CA NodeAnalysis has
// a unique CANodeId
// ------------------------------------------------------------------------
class NodeAnalysis : public NABasicObject
{

  friend class QueryAnalysis;

public:

  // destruct the NodeAnalysis
  virtual ~NodeAnalysis()
  {
    // jbbc_, groupBy_, and table_ are deleted seperately
  }

  // get the CA Id for this node
  inline CANodeId getId()
  {
    return id_;
  }

  // get the TableAnalysis for this node. This returns NULL if this
  // node is not a table scan
  inline TableAnalysis * getTableAnalysis()
  {
    return table_;
  }

  // get the JBBC for this node. This returns NULL if this
  // node is not a JBBC
  inline JBBC * getJBBC()
  {
    return jbbc_;
  }

  // get the GBAnalysis for this node. This returns NULL if this
  // node is not a GroupBy (at the CA phase)
  // I am not yet sure if i want to put this under NodeAnalysis or
  // associate it to group by expressions only.
  inline GBAnalysis * getGBAnalysis()
  {
    return groupBy_;
  }

  // get the RoutineAnalysis for this node. This returns NULL if this
  // node is not a routine
  inline RoutineAnalysis * getRoutineAnalysis()
  {
    return routine_;
  }

  // This returns a copy of the original rel expression that the NodeAnalysis
  // was computed for.
  RelExpr * getOriginalExpr()
  {
    return originalExpr_;
  }

  // This returns a copy of the original modified expression after the
  // tree was modified.
  RelExpr * getModifiedExpr()
  {
    return modifiedExpr_;
  }

  void setModifiedExpr(RelExpr * expr)
  {
    modifiedExpr_ = expr;
  }

  NABoolean isExtraHub();
//  {
//    return isExtraHub_;
//  }

///////////////////////////////////

  // This method computes some basic stats about the table, and caches them
  void computeStats();

  EstLogPropSharedPtr getStats();

  CostScalar getCardinality();

  inline RowSize getRecordSize() const
  {
    return recordSize_;
  }

///////////////////////////////////

  //reconcile this NodeAnalysis object with the NodeAnalysis
  //object passed in
  //This is used in case of Group Merges
  void reconcile(NodeAnalysis * other);

  EstLogPropSharedPtr getJBBInput() const
  {
    return jbbInputLP_;
  };
  
  void setJBBInput(EstLogPropSharedPtr & inLP)
  {
    jbbInputLP_ = inLP;
  };
  
  const NAString getText() const;

  void display() const;

  void print (FILE *f = stdout,
	      const char * prefix = DEFAULT_INDENT,
	      const char * suffix = "") const;

private:

  // construct a NodeAnalysis
  NodeAnalysis(CollIndex id, RelExpr* expr,
               CollHeap *outHeap = CmpCommon::statementHeap())
  {
    heap_ = outHeap; // should be able to move most of these to initilization
    id_ = id;
    originalExpr_ = expr;
	// start with the original expression
	modifiedExpr_ = expr;
    jbbc_ = NULL;
    table_ = NULL;
    groupBy_ = NULL;
    routine_ = NULL;
    stats_ = NULL;
    recordSize_ = 0;
    isExtraHub_ = expr->isExtraHub();
    jbbInputLP_ = NULL;
  }

  NodeAnalysis()
  {
    // Will never be called. Use other constuctor(s).
  }

  inline void setJBBC(JBBC* jbbc)
  {
    jbbc_ = jbbc;
  }

  inline void setTableAnalysis(TableAnalysis* tab)
  {
    table_ = tab;
  }

  inline void setGBAnalysis(GBAnalysis* gb)
  {
    groupBy_ = gb;
  }

  inline void setRoutineAnalysis(RoutineAnalysis* routine)
  {
    routine_ = routine;
  }

  inline void setExtraHub(NABoolean eh)
  {
    isExtraHub_ = eh;
  }

  CANodeId                 id_;
  JBBC*                    jbbc_;
  TableAnalysis*           table_;
  GBAnalysis*              groupBy_;
  RoutineAnalysis*         routine_;
  RelExpr*                 originalExpr_;
  RelExpr*                 modifiedExpr_;

  EstLogPropSharedPtr      stats_;
  RowSize                  recordSize_;

  NABoolean		   isExtraHub_;
  EstLogPropSharedPtr       jbbInputLP_;

  CollHeap*                heap_;
};


// ------------------------------------------------------------------------
// TableAnalysis class hold analysis results for tables
// ------------------------------------------------------------------------
class TableAnalysis : public NABasicObject
{

  friend class QueryAnalysis;

public:

  enum coverageCriteria {
    INDEX,
    PART_KEY
  };

  // destruct a TableAnalysis
  virtual ~TableAnalysis()
  {
    // loop over access path analysis and delete them
  }

  // Get NodeAnalysis for this table
  inline NodeAnalysis * getNodeAnalysis() const
  {
    return caNodeAnalysis_;
  }

  // Return the TableDesc
  inline TableDesc * getTableDesc() const
  {
    return tableDesc_;
  }

  // List all access paths
  inline const LIST(AccessPathAnalysis*) & getAccessPaths() const
  {
    return accessPaths_;
  }

  // List index-only access paths
  inline const LIST(AccessPathAnalysis*) & getIndexOnlyAccessPaths() const
  {
    return indexOnlyAccessPaths_;
  }

  // Return AccessPathAnalysis for base table
  inline AccessPathAnalysis* getApaForBaseTable()
  {
    return apaForBaseTable_;
  }

  // Get the set of used Cols
  inline const ValueIdSet & getUsedCols() const
  {
    return usedCols_;
  }

  // Get all cols that equivalent to constant value (via a veg pred)
  inline const ValueIdSet & getConstCols() const
  {
    return constCols_;
  }

  // Get local predicates on this table
  inline const ValueIdSet & getLocalPreds() const
  {
    return localPreds_;
  }

  // Get all columns directly connected to a column in this table
  // (via a VEG predicates)
  inline const ValueIdSet & getConnectedCols() const
  {
    return connectedCols_;
  }

  // Get all tables that have columns directly connected
  // (via a VEG predicates) to a column in this table.
  inline const CANodeIdSet & getConnectedTables() const
  {
    return connectedTables_;
  }

  inline const ValueIdSet & getReferencingPreds() const
  {
    return referencingPreds_;
  }

  inline const ValueIdSet & getVegPreds() const
  {
    return vegPreds_;
  }

  // Add these to the list of equality predicates that did not become VEG
  // these exclude predicates with other columns in the same table i.e. 
  // t1.a = t1.b
  inline void addEqualityRelation(ValueId colId, 
                                  ValueId pred,
                                  ValueId equalityExpr,
                                  CANodeIdSet connectedJBBCs)
  {
    if(!equalityConnectingPreds_.contains(pred))
    {
      equalityConnectedColList_.insert(colId);
      equalityConnectingPredList_.insert(pred);
      equalityExpressionList_.insert(equalityExpr);
    }
    
    equalityConnectedCols_ += colId;
    equalityConnectingPreds_ += pred;
    equalityConnectedJBBCs_ += connectedJBBCs;
  }

  // return the equality expression for the provided equality connecting pred
  inline ValueId getEqualityExpression(ValueId pred) const
  {
    UInt32 index = equalityConnectingPredList_.index(pred);
    
    if(index != NULL_COLL_INDEX)
    {
      return equalityExpressionList_[index];
    }
    else{
      return NULL_VALUE_ID; 
    }
  }
  
  // return the Column for the provided equality connecting pred
  inline ValueId getEqualityConnectedCol(ValueId pred) const
  {
    UInt32 index = equalityConnectingPredList_.index(pred);
    
    if(index != NULL_COLL_INDEX)
    {
      return equalityConnectedColList_[index];
    }
    else{
      return NULL_VALUE_ID; 
    }
  }

  // return the JBBC(s) connected via the equalityConnectingPred passed in
  inline const CANodeIdSet * getEqualityConnectedJBBCs(ValueId pred)
  {
    UInt32 index = equalityConnectingPredList_.index(pred);
    
    if(index != NULL_COLL_INDEX)
    {
      ValueId equalityExpression = equalityExpressionList_[index];
      CANodeIdSet * connectedJBBCs =
        QueryAnalysis::Instance()->getProducingJBBCs(equalityExpression);
      return connectedJBBCs;
    }
    else{
      return NULL; 
    }    
  }
  
  inline const ValueIdSet & getEqualityConnectedCols() const
  {
    return equalityConnectedCols_;
  }

  inline const ValueIdSet & getEqualityConnectingPreds() const
  {
    return equalityConnectingPreds_;
  }

  inline const CANodeIdSet & getEqualityConnectedJBBCs() const
  {
    return equalityConnectedJBBCs_;
  }

  ValueIdSet getConnectingVegPreds(ColAnalysis & col) const;

  ValueIdSet getConnectingVegPreds(TableAnalysis & other) const;

  // List access paths that are promising for predicate lookup
  const LIST(AccessPathAnalysis*) & promisingAccessPathsForLookup();

  // This is the helper function for the interfaces below:
  // - indexOnly specifies the list of access paths to search.
  // - cc specifies the coverage (index or part key).
  // - vidSet is the provided set to check for coverage.
  // - exactMatch is only relevant for partitioning key coverage and
  //      specifies if exact match is needed with the provided set.
  LIST(AccessPathAnalysis*)
    getCoveringAccessPaths(NABoolean indexOnly,
                           coverageCriteria cc,
                           const ValueIdSet& vidSet,
                           NABoolean exactMatch);

  // List access paths where given set covers prefix of an index.
  //
  // If indexOnly flag is set, only return index-only access paths
  // covering prefix of an index. Otherwise, return all access
  // paths cevering prefix of an index.
  //
  // If exactMatch flag is set, only return access paths where all
  // the members of the provided set are covered i.e the provided set
  // is a subset of index columns. Otherwise, return access paths
  // where the provided set covers prefix of an index.
  LIST(AccessPathAnalysis*) accessPathsCoveringIndex(const ValueIdSet& vidSet,
                                                     NABoolean indexOnly,
                                                     NABoolean exactMatch);
  // List access paths where given set cover entire partitioning key.
  //
  // If indexOnly flag is set, only return index-only access paths
  // covering entire partitioning key. Otherwise, return access paths
  // cevering entire partitioning key.
  //
  // If exactMatch flag is set, only return access paths where the
  // provided set is exact match with the partitioning key. Otherwise,
  // return access paths where the partitioning key columns are a
  // prefix-subset of the provded set.
  LIST(AccessPathAnalysis*) accessPathsCoveringPartKey(const ValueIdSet& vidSet,
                                                       NABoolean indexOnly,
                                                       NABoolean exactMatch);


  /////////////////////////////////////////////////

  // Get all JBBCs that are connected to this set of columns and the join preds
  // This table must be JBBC
  CANodeIdSet getJBBCsConnectedToCols(const CANodeIdSet & jbbcs,
                                      const ValueIdSet & cols,
                                      ValueIdSet & joinPreds /*OUT*/,
                                      ValueIdSet & localPreds /*OUT*/,
                                      ValueIdSet * predCols = NULL /*OUT*/ );

  // Get the JBBCs that are connected to the maximum prefix size in the given column list
  // This table must be JBBC
  CANodeIdSet getJBBCsConnectedToPrefixOfList(const CANodeIdSet & jbbcs,
                                              const ValueIdList & cols,
                                              Lng32 & prefixSize /*OUT*/,
                                              ValueIdSet & joinPreds /*OUT*/,
                                              ValueIdSet & localPreds /*OUT*/);

  // get columns that are connected via join preds with the set being passed in
  ValueIdSet getColsConnectedViaEquiJoinPreds(const CANodeIdSet & jbbcs);

  // Compute the local predicates on this table that references any of these columns
  // of the table
  ValueIdSet getLocalPredsOnColumns(const ValueIdSet & cols,
                                    ValueIdSet * colsWithLocalPreds = NULL /*OUT*/);

  // Compute the local predicates on this table that references a prefix of this
  // column list. compute also the prefix size.
  ValueIdSet getLocalPredsOnPrefixOfList(const ValueIdList & cols,
                                         Lng32 & prefixSize /*OUT*/);

  // Get all my columns connected via this set of predicates
  ValueIdSet getMyConnectedCols(const ValueIdSet & preds);

  // Get all my columns referenced by this set of predicates
  ValueIdSet getMyReferencedCols(const ValueIdSet & preds);

  // This method computes some basic stats about the table, and caches them
  void computeTableStats();

  EstLogPropSharedPtr getStatsOfBaseTable();

  CostScalar getCardinalityOfBaseTable();

  CostScalar getMaxCardinalityOfBaseTable();

  inline EstLogPropSharedPtr getStatsAfterLocalPredsOnCKPrefix() const
  {
    return statsAfterLocalPredsOnCKPrefix_;
  }

  inline CostScalar getCardinalityAfterLocalPredsOnCKPrefix() const
  {
    return statsAfterLocalPredsOnCKPrefix_->getResultCardinality();
  }

  inline RowSize getRecordSizeOfBaseTable() const
  {
    return recordSizeOfBaseTable_;
  }

  // get base UEC of the given column set
  CostScalar getBaseUec(const ValueIdSet & columns);

  // get equality Join predicates connected to the given CANodeIDSet
  // jbbc should be a table
  ValueIdSet getColsConnectedViaVEGPreds(CANodeId jbbcs);

  // get equality factor for the given set of columns. Equality factor is
  // defined as base UEC of given column set dividied by the base row count
  // In case of multiple columns, if it would be multi-col UEC. If no stats
  // exist for these set of columns, equality factor is zero.
  CostScalar getEqualityVal(ValueIdSet joinCols);

  ///////// Inspectors ignore these methods /////////////

  // List indexes providing any key prefix covering for any of the
  // given local predicates. This is used to find indexes for lookup.
  LIST(AccessPathAnalysis*) findIndexesWithKeyPrefixCoveredByPreds(const ValueIdSet & preds);

  // List indexes where the given local predicates covers the entire key.
  LIST(AccessPathAnalysis*) findIndexesWithKeyCoveredByPreds(const ValueIdSet & preds);


  // List indexes where the given set of columns covers a prefix of index key.
  LIST(AccessPathAnalysis*) findIndexesWithKeyPrefixCoveredByCols(const ValueIdSet & cols);

  // List indexes where a prefix of the given list of columns covers a prefix of index key.
  LIST(AccessPathAnalysis*) findIndexesWithKeyPrefixCoveredByColList(const ValueIdList & cols);


  // List indexes where the given set of columns covers the entire index key.
  LIST(AccessPathAnalysis*) findIndexesWithKeyCoveredByCols(const ValueIdSet & cols);

  // List indexes where a prefix of the given list of columns covers the entire index key.
  LIST(AccessPathAnalysis*) findIndexesWithKeyCoveredByColList(const ValueIdList & cols);

  ////////  End of inspectors ignore ////////////////

  NABoolean hasMatchingHashPartitioning(TableAnalysis * other);

  NABoolean predsOnUnique(ValueIdSet& vidSet, NABoolean *hasPrefix=NULL);

  void setFactTableNJAccessCost(CostScalar factNJAccessCost)
  {
    factTableNJAccessCost_ = factNJAccessCost;
  }
  
  CostScalar getFactTableNJAccessCost() const
  {
    return factTableNJAccessCost_;
  }
  
  CostScalar computeDataAccessCostForTable(CostScalar probes,
                                           CostScalar tableRowsToScan) const;
  // finalized analysis
  void finishAnalysis();

  void checkIfCompressedHistsViable();

  // initialize access path analysis structures
  void initAccessPaths();
  // initialize index only access paths
  void initIndexOnlyAccessPaths();

  const NAString getText() const;

  void display() const;

  void print (FILE *f = stdout,
	      const char * prefix = DEFAULT_INDENT,
	      const char * suffix = "") const;

private:

  // construct a TableAnalysis. Called only by QueryAnalysis
  TableAnalysis(NodeAnalysis* nodeAnalysis,
                TableDesc* tableDesc,
                CollHeap *outHeap = CmpCommon::statementHeap()):
  caNodeAnalysis_(nodeAnalysis),
  tableDesc_(tableDesc),
  apaForBaseTable_(NULL),
  statsOfBaseTable_(NULL),
  recordSizeOfBaseTable_(0),
  statsAfterLocalPredsOnCKPrefix_(NULL),
  heap_(outHeap),
  accessPaths_(outHeap),
  indexOnlyAccessPaths_(outHeap),
  uniqueIndexes_(outHeap),
  factTableNJAccessCost_(-1)
  {
  }

  // construct a TableAnalysis
  TableAnalysis()
    :accessPaths_(STMTHEAP),
    indexOnlyAccessPaths_(STMTHEAP),
    uniqueIndexes_(STMTHEAP)
  {
    // should never be called
	// use a constructor that passes NodeAnalysis* and TableDesc*
  }

  // set the usedCols_. to be used by QueryAnalysis
  void setUsedCols(const ValueIdSet & usedCols)
  {
    usedCols_ = usedCols;
  }

  // set the localPreds_. to be used by QueryAnalysis
  void setLocalPreds(const ValueIdSet & localPreds)
  {
    localPreds_ = localPreds;
  }

  // Set AccessPathAnalysis for base table.
  inline void setApaForBaseTable(AccessPathAnalysis* apa)
  {
    apaForBaseTable_ = apa;
  }


  NodeAnalysis*                 caNodeAnalysis_;
  TableDesc*                    tableDesc_;
  AccessPathAnalysis*           apaForBaseTable_;
  ValueIdSet                    usedCols_;
  ValueIdSet                    constCols_;
  ValueIdSet                    localPreds_;
  ValueIdSet                    referencingPreds_;
  ValueIdSet                    vegPreds_;
  ValueIdSet                    equalityConnectedCols_;
  ValueIdSet                    equalityConnectingPreds_;
  CANodeIdSet                   equalityConnectedJBBCs_;
  ValueIdList                   equalityConnectedColList_;
  ValueIdList                   equalityConnectingPredList_;
  ValueIdList                   equalityExpressionList_;
  ValueIdSet                    connectedCols_;
  CANodeIdSet                   connectedTables_;
  LIST(AccessPathAnalysis*)     accessPaths_;
  LIST(AccessPathAnalysis*)     indexOnlyAccessPaths_;
  LIST(AccessPathAnalysis*)     uniqueIndexes_;
  EstLogPropSharedPtr           statsOfBaseTable_; // No predicates
  RowSize                       recordSizeOfBaseTable_;
  EstLogPropSharedPtr           statsAfterLocalPredsOnCKPrefix_; // No predicates

  // data access cost in case this is the fact table and is under nested join
  CostScalar                    factTableNJAccessCost_;
  
  CollHeap*                     heap_;
};


// ------------------------------------------------------------------------
// AccessPathAnalysis class hold analysis results for table indexes
// ------------------------------------------------------------------------
class AccessPathAnalysis : public NABasicObject
{

  friend class TableAnalysis;

public:

  // construct an AccessPathAnalysis
  AccessPathAnalysis(CollHeap *outHeap = CmpCommon::statementHeap())
  {}

  // destruct an AccessPathAnalysis
  virtual ~AccessPathAnalysis()
  {}

  // return the IndexDesc
  inline IndexDesc * getIndexDesc() const
  {
    return indexDesc_;
  }

  // Return TableAnalysis associated with this AccessPathAnalysis
  inline TableAnalysis* getTableAnalysis() const
  {
    return tableAnalysis_;
  }

  // find if index only or alternative index lookup.
  NABoolean isIndexOnly();

  // is this the Clustering Index (i.e. base table)?
  NABoolean isClustering() const;

  Int32 numIndexPrefixCovered(const ValueIdSet& vidSet, NABoolean exactMatch,
                            NABoolean useKeyCols=FALSE);
  NABoolean isPartKeyCovered(const ValueIdSet& vidSet, NABoolean exactMatch);

  // is the accessPath Key covered
  NABoolean keyCoveredByEqualityPreds(const ValueIdSet& vidSet,
                                      NABoolean *hasPrefix=NULL);

  const NAString getText() const;

  void print (FILE *f = stdout,
	      const char * prefix = DEFAULT_INDENT,
	      const char * suffix = "") const;

private:

  // construct a AccessPathAnalysis. Called only by TableAnalysis
  AccessPathAnalysis(IndexDesc* indexDesc,
                     TableAnalysis* tableAnalysis,
                     CollHeap *outHeap = CmpCommon::statementHeap());

  IndexDesc*                    indexDesc_;
  TableAnalysis*                tableAnalysis_;
  ValueIdSet                    accessPathColSet_;
  ValueIdList                   accessPathColList_;
  ValueIdList                   accessPathKeyCols_;
  UInt32                  accessPathKeyCount_;
  ValueIdList                   accessPathPartKeyCols_;
  CollHeap*                     heap_;
};


// ------------------------------------------------------------------------
// GBAnalysis class hold analysis results for group bys that are
// located at top of JBBs at the CA phase
// ------------------------------------------------------------------------
class GBAnalysis : public NABasicObject
{

  friend class QueryAnalysis;
  friend class JBB;

public:

  // destruct a GBAnalysis
  virtual ~GBAnalysis()
  {}

  // This returns a copy of the original GB expression that the NodeAnalysis
  // was computed for. Please use this method only if necessary. Our ultimate
  // goal is that this will eventually not be needed. But at this phase we will
  // keep it as its needed by some of the phase developments.
  inline GroupByAgg * getOriginalGBExpr()
  {
    return originalGBExpr_;
  }

  // R1 & R2
  // we can compute one from the other for now
  // later we should have more detailed info about independently pullables

  // R2: The jbbcs that can be pulled (together) above the GB
  const CANodeIdSet & getPullableJBBCs() const
  {
    return pullableJBBCs_;
  }

  // R1: The other sibling jbbcs that are not part of pullableJBBCs
  const CANodeIdSet & getRequiredJBBCs() const
  {
    return requiredJBBCs_;
  }

  // The groupingNodeId is different than id_ in NodeAnalysis.
  // This id does not stand alone but rather is used to represent
  // the GB in a JBBSubset
  inline CANodeId getGroupingNodeId() const
  {
    return groupingNodeId_;
  }

  // get and JBB
  inline JBB* getJBB()
  {
    return jbb_;
  }

  const NAString getText() const
  {
    return "GBAnalysis";
  }

private:

  // construct a GBAnalysis. Called only by QueryAnalysis
  GBAnalysis(CANodeId id,
             GroupByAgg* gbExpr,
             CollHeap *outHeap = CmpCommon::statementHeap()):
  groupingNodeId_(id),
  originalGBExpr_(gbExpr),
  jbb_(NULL),
  heap_(outHeap)
  {
  }

  // construct a GBAnalysis
  GBAnalysis()
  {
    // should never be called
  }

  // In the future when we allow more GBAnalysis objects to be created
  // during the optimization phase (due to GB split for example), we will
  // need to make this method public.
  inline void setJBB(JBB* jbb)
  {
    jbb_ = jbb;
  }

  CANodeIdSet       pullableJBBCs_;
  CANodeIdSet       requiredJBBCs_; // this could be computed as mainSubset-pullable
  GroupByAgg*       originalGBExpr_;
  JBB*              jbb_;
  CANodeId          groupingNodeId_;
  CollHeap*         heap_;
};

class RoutineAnalysis : public NABasicObject
{
public:
  // construct a RoutineAnalysis. Called only by QueryAnalysis
  RoutineAnalysis(NodeAnalysis* nodeAnalysis,
                RoutineDesc* routineDesc,
                CollHeap *outHeap = CmpCommon::statementHeap()):
  caNodeAnalysis_(nodeAnalysis),
  routineDesc_(routineDesc)
  {
  }
  
  // Get NodeAnalysis for this routine
  inline NodeAnalysis * getNodeAnalysis() const
  {
    return caNodeAnalysis_;
  }
  
  // Return the TableDesc
  inline RoutineDesc * getRoutineDesc() const
  {
    return routineDesc_;
  }
  

private:
  RoutineAnalysis(RoutineAnalysis & other){CMPASSERT(FALSE);}
  
  NodeAnalysis * caNodeAnalysis_;
  RoutineDesc * routineDesc_;
};

// ------------------------------------------------------------------------
// This class is an interface class.
// ------------------------------------------------------------------------
class JBBItem : public NABasicObject
{

public:

  // Get all JBBCs that are joined to this JBBItem and do not have
  // dependency relation with this JBBItem. For definition of join dependency
  // Refer to "Connectivity Analyzer PRD" document.
  virtual const CANodeIdSet & getJoinedJBBCs() const = 0;

  // Get all the join predicates associated with getJoinedJBBCs
  virtual const ValueIdSet & getJoinPreds() const = 0;

  // Get all JBBCs that are joined to this JBBItem and that are
  // dependent on this JBBItem. For definition of join dependency
  // Refer to "Connectivity Analyzer PRD" document.
  virtual const CANodeIdSet & getSuccessorJBBCs() const = 0;

  // Get all the join predicates associated with getSuccessorJBBCs
  virtual const ValueIdSet & getPredsWithSuccessors() const = 0;

  // Get all JBBCs that are joined to this JBBItem and that this
  // JBBItem is dependent on. For definition of join dependency
  // Refer to "Connectivity Analyzer PRD" document.
  virtual const CANodeIdSet & getPredecessorJBBCs() const = 0;

  // Get all the join predicates associated with getPredecessorJBBCs
  virtual const ValueIdSet & getPredsWithPredecessors() const = 0;

  // Get all JBBCs that are have no join predicate with this JBBItem.
  virtual CANodeIdSet getJBBCsThatXProductWithMe() const = 0;

  // Get all the join predicates (no dependency) between this JBBItem
  // and the other JBBItem
//  virtual ValueIdSet joinPredsWithOther(const JBBItem & other) const;

  // Get all dependent predicates from other JBBItem on this JBBItem.
  virtual ValueIdSet successorPredsOfOther(const JBBItem & other) const;

  // Get all dependent predicates from this JBBItem on other JBBItem.
  virtual ValueIdSet predecessorPredsOnOther(const JBBItem & other) const;

  // Get the set of JBBCs in this JBBItem
//  virtual const CANodeIdSet & getJBBCs() const;
};

// ------------------------------------------------------------------------
// This is the JBBC class
// ------------------------------------------------------------------------
class JBBC : public JBBItem
{

  friend class QueryAnalysis;
  friend class JBB;

public:

  // destruct a JBBC
  virtual ~JBBC()
  {}

  // Get all JBBCs that are joined to this JBBC and do not have
  // dependency relation with this JBBC. For definition of join dependency
  // Refer to "Connectivity Analyzer PRD" document.
  virtual const CANodeIdSet & getJoinedJBBCs() const
  {
    return joinedJBBCs_;
  }

  // Get all the join predicates associated with getJoinedJBBCs
  virtual const ValueIdSet & getJoinPreds() const
  {
    return joinPreds_;
  }

  CANodeIdSet getJBBCsConnectedViaKeyJoins();

  // Set the join preds to this JBBC.
  // This will also update predAnalysis of these preds of this JBBC
  void setJoinPreds(const ValueIdSet & joinPreds);

  void setLeftJoinFilterPreds(ValueIdSet & leftJoinFilterPreds)
  {
    leftJoinFilterPreds_ = leftJoinFilterPreds;
  }

  ValueIdSet getLeftJoinFilterPreds() const
  {
    return leftJoinFilterPreds_;
  }

  void setPredsWithDependencies(const ValueIdSet & predsWithDependencies,
                                const ValueIdSet & predsWithPredecessors);

  // Get all JBBCs that are joined to this JBBC and that are
  // dependent on this JBBC. For definition of join dependency
  // Refer to "Connectivity Analyzer PRD" document.
  virtual const CANodeIdSet & getSuccessorJBBCs() const
  {
    return successorJBBCs_;
  }

  // Get all the join predicates associated with getDependentJBBCs
  virtual const ValueIdSet & getPredsWithSuccessors() const
  {
    return predsWithSuccessors_;
  }

  // Get all JBBCs that are joined to this JBBC and that this
  // JBBC is dependent on. For definition of join dependency
  // Refer to "Connectivity Analyzer PRD" document.
  virtual const CANodeIdSet & getPredecessorJBBCs() const
  {
    return predecessorJBBCs_;
  }

  // Get all the join predicates associated with getPredecessorJBBCs
  virtual const ValueIdSet & getPredsWithPredecessors() const
  {
    return predsWithPredecessors_;
  }

  LIST(CANodeIdSet) * const getPredecessorJBBCsList() const
  {
    return predecessorJBBCsList_;
  }

  // Get constant predicates with predecessors, these are really
  // not predicates with predecessors, they are just constant
  // predicates that are evaluated after the join. An example
  // query is below:
  //
  //  SELECT Distinct 'J', T0.I3, t0.i3, T0.I3
  //  FROM d2 T0, d2 t1, d1 t2
  //  WHERE
  //    'kTrn' < ALL (
  //      SELECT 'a'
  //      FROM d1 t3
  //      WHERE
  //        NOT (
  //          ('DLU' BETWEEN ( T3.i1 ) AND ( T3.I3 ))
  //          OR
  //          ( T0.I2 IN (t1.I2 , 'gwm')))
  //      GROUP BY T3.I1
  //    )
  //  ORDER BY 1, 1 ;
  //
  // This query has the following constant predicate with predecessor
  //
  // ('kTrn' >= 'a')
  //
  // It is a join predicate for the semi-join but does not cause
  // dependencies between any two tables.
  virtual const ValueIdSet & getConstPredsWithPredecessors() const
  {
    return constPredsWithPredecessors_;
  }

  void setRoutineJoinFilterPreds(const ValueIdSet & routineJoinFilterPreds)
  {
    routineJoinFilterPreds_ = routineJoinFilterPreds;
  }

  ValueIdSet getRoutineJoinFilterPreds() const
  {
    return routineJoinFilterPreds_;
  }

  // inputs need from sibling JBBCs i.e. jbbcs part of the same JBB as this JBBC
  const ValueIdSet & getInputsRequiredFromSiblings() const
  {
    return inputsRequiredFromSiblings_; 
  }
  
  
  void setInputsRequiredFromSiblings(ValueIdSet & inputsRequiredFromSiblings)
  {
    inputsRequiredFromSiblings_ = inputsRequiredFromSiblings; 
  }

  // Get all JBBCs that need input from this jbbc
  virtual const CANodeIdSet & getJBBCsRequiringInputFromMe() const
  {
    return jbbcsRequiringInputFromMe_;
  }

  // Get all JBBCs that this jbbc needs input from
  virtual const CANodeIdSet & getJBBCsProvidingInput() const
  {
    return jbbcsProvidingInput_;
  }

  // Get all JBBCs that are have no join predicate with this JBBC.
  virtual CANodeIdSet getJBBCsThatXProductWithMe() const;

  // implementation for JBBItem method
//  virtual const CANodeIdSet & getJBBCs() const
//  {
//    return *(new (heap_) CANodeIdSet(getId(),heap_));
//  }

  ValueIdSet joinPredsWithOther(const JBBC & other) const;

  /* Do not inspect this method */
  // Returns a subset of getJoinedJBBCs that are joined with this JBBC
  // on this particular column
  CANodeIdSet getJoinedJBBCsOnThisColumn(const ValueId & col,
	                                     ValueIdSet& joiningPreds /*OUT*/) const;
  /* Do not inspect this method */
  // Returns a subset of getJoinedJBBCs that are joined with this JBBC
  // on these particular columns
  CANodeIdSet getJoinedJBBCsOnTheseColumns(const ValueIdSet & cols,
	                                       ValueIdSet & joiningPreds /*OUT*/) const;

  const CANodeIdSet & getSubtreeTables() const
  {
    return getNodeAnalysis()->getOriginalExpr()->
           getGroupAnalysis()->getAllSubtreeTables();
  }

  // Returns the original parent join expression during the CA phase
  // or a copy of it. This JBBC was a right child for the join expression
  // during the CA phase. The left most JBBC will have a fake inner join
  // with no preds.
  inline Join * getOriginalParentJoin() const
  {
    return origParentJoin_;
  }

  // Get NodeAnalysis for this JBBC
  inline NodeAnalysis * getNodeAnalysis() const
  {
    return caNodeAnalysis_;
  }

  // Get the CANodeId for this JBBC
  inline CANodeId getId() const
  {
    return getNodeAnalysis()->getId();
  }

  // Get the parent JBB of this JBBC
  inline JBB* getJBB() const
  {
    return jbb_;
  }

  const ValueIdList & nullInstantiatedOutput() const
  {
    return nullInstantiatedOutput_;
  }

  inline NABoolean isFullOuterJoinOrTSJJBBC() const
  {
    return isFullOuterJoinOrTSJJBBC_;
  }

  NABoolean isGuaranteedEqualizer();

  NABoolean hasNonExpandingJoin();

  inline NABoolean parentIsLeftJoin(){ return (parentJoinType_ == REL_LEFT_JOIN); }

  NABoolean isOneRowMax() const {return isOneRowMax_; }
  
  const NAString getText() const;

  void display() const;

  void print (FILE *f = stdout,
	      const char * prefix = DEFAULT_INDENT,
	      const char * suffix = "") const;

private:

  // construct a JBBC. Made private because only QueryAnalysis
  // can create new JBBCs.
  JBBC(NodeAnalysis* nodeAnalysis,
	   Join* parentJoin,
	   CollHeap* outHeap = CmpCommon::statementHeap()):
    caNodeAnalysis_(nodeAnalysis),
    origParentJoin_(parentJoin),
    jbb_(NULL),
    joinedJBBCs_(outHeap),
    successorJBBCs_(outHeap),
    predecessorJBBCs_(outHeap),
    predsWithPredecessorsList_(NULL),
    predecessorJBBCsList_(NULL),
    inputsRequiredFromSiblings_(NULL),
    isFullOuterJoinOrTSJJBBC_(FALSE),
    parentJoinType_(REL_JOIN),
    jbbcsJoinedViaTheirKeyIsSet_(FALSE),
    isGuaranteedEqualizer_(-1),
    hasNonExpandingJoin_(-1),
    heap_(outHeap)
  {
    if(parentJoin){
      if(parentJoin->isLeftJoin())
        nullInstantiatedOutput_ = parentJoin->nullInstantiatedOutput();

      parentJoinType_ = parentJoin->getOperatorType();
    }
    
    if(nodeAnalysis->getOriginalExpr()->getGroupAttr()->getMaxNumOfRows() <= 1)
      isOneRowMax_ = TRUE;
    else
      isOneRowMax_ = FALSE;
  }

  inline void setJBB(JBB* jbb)
  {
    jbb_ = jbb;
  }

  inline void setIsFullOuterJoinOrTSJJBBC()
  {
    isFullOuterJoinOrTSJJBBC_ = TRUE;
  }

  // Class members
  NodeAnalysis*    caNodeAnalysis_;
  JBB*             jbb_;
  ValueIdSet       joinPreds_;
  CANodeIdSet      joinedJBBCs_;
  CANodeIdSet      jbbcsJoinedViaTheirKey_;
  NABoolean        jbbcsJoinedViaTheirKeyIsSet_;
  ValueIdList      nullInstantiatedOutput_;
  ValueIdSet       leftJoinFilterPreds_;
  ValueIdSet       routineJoinFilterPreds_;
  ValueIdSet       inputsRequiredFromSiblings_;
  ValueIdSet       predsWithSuccessors_;
  CANodeIdSet      successorJBBCs_;
  ValueIdSet       predsWithPredecessors_;
  CANodeIdSet      predecessorJBBCs_;
  ValueIdList*     predsWithPredecessorsList_;
  LIST(CANodeIdSet) * predecessorJBBCsList_;
  ValueIdSet       constPredsWithPredecessors_;
  Join*            origParentJoin_;
  NABoolean        isFullOuterJoinOrTSJJBBC_;
  Int32     isGuaranteedEqualizer_;
  Int32     hasNonExpandingJoin_;
  NABoolean isOneRowMax_;
  OperatorTypeEnum parentJoinType_;
  CANodeIdSet      jbbcsRequiringInputFromMe_;
  CANodeIdSet      jbbcsProvidingInput_;
  CollHeap*         heap_;
};


// ------------------------------------------------------------------------
// Class CASortedList
// A helper class for JBBWA
// This class is used to cache various sorted lists of CANodeIds in the
// JBBWA and the JBBSubsetAnalysis. This is done so that we avoid re-sorting
// when the same list is needed again. Since the sorting involves some
// expensive ASM computations to apply predicates to histograms
// ------------------------------------------------------------------------
typedef NAList<CANodeId> CASortedList;

/* Inspection advice: inspect JBBSubset class before JBBSubsetAnalysis */
// ------------------------------------------------------------------------
// The JBBSubsetAnalysis class. This is where the operations for JBBSubset
// are performed. Because JBBSubset computations are relatively expensive
// and could be asked multiple times from different JBBSubset objects
// corresponding to the same logical JBBSubset, one JBBSubsetAnalysis object
// is used to compute and save such computations. All equivalent JBBSubsets
// points to the same JBBSubsetAnalysis.
// ------------------------------------------------------------------------
class JBBSubsetAnalysis : public NABasicObject
// consider deriving from JBBSubset
// disadvantage for that is sharing the setter methods
{
public:

  // construct a JBBSubsetAnalysis
  JBBSubsetAnalysis(const JBBSubset & subset,
                    CollHeap *outHeap = CmpCommon::statementHeap());

  // destruct a JBBSubsetAnalysis
  virtual ~JBBSubsetAnalysis()
  {}

  // equality operator
  inline NABoolean operator == (const JBBSubsetAnalysis &other) const
  {
    return (allMembers_ == other.allMembers_);
  }

  // Get all JBBCs that are joined to this JBBSubset and do not have
  // dependency relation with this JBBSubset. For definition of join dependency
  // Refer to "Connectivity Analyzer PRD" document.
  virtual const CANodeIdSet & getJoinedJBBCs() const
  {
    return joinedJBBCs_;
  }

  // Get all the join predicates associated with getJoinedJBBCs
  virtual const ValueIdSet & getJoinPreds() const
  {
    return joinPreds_;
  }

  // Get all JBBCs that are joined to this JBBSubset and that are
  // dependent on this JBBSubset. For definition of join dependency
  // Refer to "Connectivity Analyzer PRD" document.
  virtual const CANodeIdSet & getSuccessorJBBCs() const
  {
    return successorJBBCs_;
  }

  // Get all the join predicates associated with getSuccessorJBBCs
  virtual const ValueIdSet & getPredsWithSuccessors() const
  {
    return predsWithSuccessors_;
  }

  // Get all JBBCs that are joined to this JBBSubset and that this
  // JBBSubset is dependent on. For definition of join dependency
  // Refer to "Connectivity Analyzer PRD" document.
  virtual const CANodeIdSet & getPredecessorJBBCs() const
  {
    return predecessorJBBCs_;
  }

  // Get all the join predicates associated with getPredecessorJBBCs
  virtual const ValueIdSet & getPredsWithPredecessors() const
  {
    return predsWithPredecessors_;
  }

  // Get the join preds between my jbbcs
  virtual const ValueIdSet & getLocalJoinPreds() const
  {
    return localJoinPreds_;
  }

  // Get the inner join preds between my jbbcs
  virtual const ValueIdSet & getLocalInnerNonSemiJoinPreds() const
  {
    return localInnerNonSemiJoinPreds_;
  }

  // Get the dependent join preds between my jbbcs i.e. all
  // semi, anti-semi and outer-join preds
  virtual const ValueIdSet & getLocalDependentJoinPreds() const
  {
    return localDependentJoinPreds_;
  }

  // which JBBCs can I add to this subset while reserving its self-dpendency
  CANodeIdSet legalJBBCAdditions() const;

  // Get the set of JBBCs in this JBBSubset
  inline const CANodeIdSet & getJBBCs() const
  {
    return jbbcs_;
  }

  void setSubsetMJ(MultiJoin * subsetMJ);

  // Get the MJ representing this JBBSubset
  inline MultiJoin * getSubsetMJ() const
  {
    return subsetMJ_;
  }

  // Get the Grouping Id for the group by in this JBBSubset
  inline CANodeId getGB() const
  {
    return gb_;
  }

  // union of both getJBBCs() and getGB()
  inline CANodeIdSet getJBBCsAndGB() const
  {
	return allMembers_;
  }

  inline NABoolean allJoinsInnerNonSemi()
  {
    return allJoinsInnerNonSemi_;
  }

  NABoolean coversExpr(ValueId vid) const;

  NAList<CANodeIdSet*>* getConnectedSubgraphs(NABoolean followSuccessors = TRUE, 
                                              NABoolean pullInPredecessors = FALSE);

  CANodeIdSet getInputSubgraph(CANodeId node, NABoolean followSuccessors = TRUE) const;
  
  // initialize JBBSubsetAnalysis computable fields
  void init();

  // Compute the join preds between my jbbcs
  void computeLocalJoinPreds();

  CASortedList * getNodesSortedByLocalKeyPrefixPredsCard();

  CASortedList * getNodesSortedByLocalPredsCard();

  CANodeId getLargestNode();

  CANodeId getLargestIndependentNode();

  void consolidateFringes(CANodeId factTable,
                          NAList<CANodeIdSet> &leftDeepJoinSequence);

  // Insert an MV match (for query rewrite purposes) into the list of potential
  // replacements for the JBB subset. Preferred matches are inserted at the
  // front of the list, non-preferred matches go at the end.
  void addMVMatch(MVMatchPtr match, NABoolean isPreferred);

  MJRulesWA * getMJRulesWA();

  void setMJStarJoinRuleWA(MJStarJoinRuleWA * mjStarJoinRuleWA)
  {
    mjStarJoinRuleWA_ = mjStarJoinRuleWA;
  }

  inline MJStarJoinRuleWA * getMJStarJoinRuleWA()
  {
    return mjStarJoinRuleWA_;
  }

  void setMJStarJoinIRuleWA(MJStarJoinIRuleWA * mjStarJoinIRuleWA)
  {
    mjStarJoinIRuleWA_ = mjStarJoinIRuleWA;
  }

  inline MJStarJoinIRuleWA * getMJStarJoinIRuleWA()
  {
    return mjStarJoinIRuleWA_;
  }

  void setMJStarBDRuleWA(MJStarBDRuleWA * mjStarBDRuleWA)
  {
    mjStarBDRuleWA_ = mjStarBDRuleWA;
  }

  inline MJStarBDRuleWA * getMJStarBDRuleWA()
  {
    return mjStarBDRuleWA_;
  }

  const NAString getText() const
  {
    return "JBBSubsetAnalysis";
  }

  // find the fact table and the largest table for this JBBSubset
  // if a fact table is not found NULL_CA_ID is returned
  CANodeId findFactAndLargestTable(
    CostScalar & factTableCKPrefixCardinality,
    CANodeId & biggestTable);

  // find the fact table and the largest table for this childSet
  // if a fact table is not found NULL_CA_ID is returned
  CANodeId findFactTable(
    CANodeIdSet childSet,
    CostScalar & factTableCKPrefixCardinality,
    CANodeId & biggestTable);

  CANodeId computeCenterTable();

  CostScalar getCenterTableSize()
  {
    return centerTableDataScanned_;
  }

  CostScalar getCenterTableSizePerPartition()
  {
    return centerTableDataPerPartition_;
  }

  void setStarJoinTypeIFeasible()
  {
    starJoinTypeIFeasible_ = TRUE;
  }

  NABoolean starJoinTypeIFeasible()
  {
    return starJoinTypeIFeasible_;
  }

  void setFactTable(CANodeId factTable)
  {
    factTable_ = factTable;
  }

  CANodeId getFactTable() { return factTable_;}

  // This method given a fact table, tries to match this JBBSubset to a
  // Star Join pattern
  NABoolean isAStarPattern(CANodeId factTable,//in
                           CostScalar factTableCKPrefixCardinality);//in

  void analyzeInitialPlan(MultiJoin * mjoin);

  void computeRequiredResources(MultiJoin * mjoin,
                            RequiredResources & reqResouces,
                            EstLogPropSharedPtr & inLP);

  const NAList<CANodeIdSet> * const getInitialPlanLeftDeepJoinSequence()
  { return &leftDeepJoinSequence_; }

  CostScalar getCenterTableRowsScanned()
    { return centerTableRowsScanned_; }
  CostScalar getCenterTableDataScanned()
    { return centerTableDataScanned_; }
  CostScalar getCenterTableDataPerPartition()
    { return centerTableDataPerPartition_; }
  CostScalar getCenterTablePartitions()
    { return centerTablePartitions_; }
  UInt32 getCenterTableConnectivity()
  {  return centerTableConnectivity_; }
  UInt32 getMaxDimensionConnectivity()
    { return maxDimensionConnectivity_; }

  NAPtrList<MVMatchPtr> getMatchingMVs()
    { return matchingMVs_; };

  void analyzeInitialPlan();
  
  SUBSET_LEGALITY getLegality() const { return legality_;};
  void setLegality(SUBSET_LEGALITY legality) { legality_ = legality; };
  CASortedList * getSynthLogPropPath() const { return synthLogPropPath_;};
  void setSynthLogPropPath(CASortedList * path) { synthLogPropPath_ = path;};

private:

  JBBSubsetAnalysis(CollHeap *outHeap = CmpCommon::statementHeap())
    : leftDeepJoinSequence_(outHeap),
    matchingMVs_(outHeap)
  {
    // should never be called
  }

  CANodeIdSet extendEdge(CANodeId thisTable,//in
                         CANodeIdSet& availableNodes,
                         UInt32 lookAhead);//in\out

  // get a rough estimate of cost for doing a nested join on the fact table
  // number of probes = dataFlowFromEdge
  // Rows of fact table that will be scanned = factTableRowsToScan
  CostScalar computeDataAccessCostForTable(CostScalar probes,
                                           CostScalar tableRowsToScan,
                                           CANodeId   table);

  void arrangeTablesAfterFactForStarJoinTypeI();
  void arrangeTablesAfterFactForStarJoinTypeII();

  JBB*                jbb_;
  CANodeIdSet         jbbcs_;
  MultiJoin *         subsetMJ_;
  CANodeId            gb_;
  CANodeIdSet         allMembers_;
  ValueIdSet          joinPreds_;
  CANodeIdSet         joinedJBBCs_;
  ValueIdSet          predsWithSuccessors_;
  CANodeIdSet         successorJBBCs_;
  ValueIdSet          predsWithPredecessors_;
  CANodeIdSet         predecessorJBBCs_;
  ValueIdSet          localJoinPreds_;
  ValueIdSet          localInnerNonSemiJoinPreds_;
  ValueIdSet          localDependentJoinPreds_;

  // caching the known covered and not covered expressions
  ValueIdSet          knownCovered_;
  ValueIdSet          knownNotCovered_;

  // are all the JBBCs join via innerNonSemi joins
  NABoolean           allJoinsInnerNonSemi_;

  SUBSET_LEGALITY     legality_;
  
  // information set by method JBBSubsetAnalysis::getCenterTable()
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

  // information set by the JBBSubsetAnalysis::findFactAndLargestTable
  // begin
  // fact table used in the star join I and star join II rules
  // a fact table significantly larger (e.g. 5x) than all other
  // tables in the JBBSubset. But if the largest table in the
  // JBBSubset is not significantly larger then this is set to
  // NULL_CA_ID
  NABoolean           computedFactAndLargestTable_;
  CANodeId            factTable_;
  CANodeId            largestTable_;
  //end

  // information set by the JBBSubsetAnalysis::getLargestIndependentNode
  // begin
  // fact table used in the star join II rules
  // a fact table significantly larger (e.g. 5x) than all other
  // tables in the JBBSubset. But if the largest table in the
  // JBBSubset is not significantly larger then this is set to
  // NULL_CA_ID
  NABoolean           computedLargestIndependentNode_;
  CANodeId            largestIndependentNode_;
  //end

  // is StarJoin Type I feasible
  NABoolean starJoinTypeIFeasible_;

  // is the analysis of initial plan finished
  NABoolean analyzedInitialPlan_;

  // This is the set of tables that are joined before
  // the fact table is joined. Generally this should
  // be a union of the tables in the lists of sets below.
  CANodeIdSet nodesJoinedBeforeFactTable_;

  // list of fringes that are joined to the fact table via clustering
  // key prefix predicates
  NAList<CANodeIdSet> * listOfEdges_;

  // optimal position of FactTable in list of edges
  Int32 optimalFTLocation_;

  // lowest data access cost of doing nested join into FactTable
  CostScalar lowestFactNJCost_;

  // the cost for a nested join into fact
  // if all the edges are below the fact
  CostScalar costForMaxKeyFactNJ_;

  // Nodes that are not part of the fringes
  CANodeIdSet availableNodes_;

  // list of CANodeIdSets representing the left deep
  // join sequence for the initial plan
  NAList<CANodeIdSet> leftDeepJoinSequence_;

  NAList<CANodeIdSet*>*  connectedSubgraphs_;  // built on demand

  // List of MV matches that can be substituted for the JBB subset using query
  // rewrite.
  NAPtrList<MVMatchPtr> matchingMVs_;

  MJRulesWA * mjRulesWA_;  // Work area used to store common information
                           // shared across MJ Rules, this helps avoid
                           // recomputation of that information for each
                           // different MJ Rule

  MJStarJoinRuleWA * mjStarJoinRuleWA_;  // Work Area used by the MJStarJoinRule
                                         // this will be created and passed in by
                                         // the MJStarJoinRule

  MJStarJoinIRuleWA * mjStarJoinIRuleWA_;  // Work Area used by the MJStarJoinIRule
                                           // this will be created and passed in by
                                           // the MJStarJoinIRule

  MJStarBDRuleWA * mjStarBDRuleWA_;

  CASortedList * synthLogPropPath_; // The sequence of joined jbbcs used
                                    // in synthesis of logical properties
                                    // Set in MultiJoin::synthLogProp

  CollHeap*           heap_;

};

// ------------------------------------------------------------------------
// The JBBSubset class. Properties of set of JBBCs joined together
// with an optional GB
// ------------------------------------------------------------------------
class JBBSubset : public JBBItem
{

public:

  // construct a JBBSubset
  JBBSubset(CollHeap *outHeap = CmpCommon::statementHeap())
  {}

  // need copy constructor.

  // destruct a JBBSubset
  virtual ~JBBSubset()
  {}

  // Comparison operators
  inline NABoolean operator == (const JBBSubset &other) const
  {
    return ((jbbcs_ == other.jbbcs_) && (gb_ == other.gb_));
  }

  inline NABoolean operator != (const JBBSubset &other) const
  {
    return NOT operator==(other);
  }

  // Get all JBBCs that are joined to this JBBSubset and do not have
  // dependency relation with this JBBSubset. For definition of join dependency
  // Refer to "Connectivity Analyzer PRD" document.
  virtual const CANodeIdSet & getJoinedJBBCs() const
  {
    return getJBBSubsetAnalysis()->getJoinedJBBCs();
  }

  // Get all the join predicates associated with getJoinedJBBCs
  virtual const ValueIdSet & getJoinPreds() const
  {
    return getJBBSubsetAnalysis()->getJoinPreds();
  }

  // Get all JBBCs that are joined to this JBBSubset and that are
  // dependent on this JBBSubset. For definition of join dependency
  // Refer to "Connectivity Analyzer PRD" document.
  virtual const CANodeIdSet & getSuccessorJBBCs() const
  {
    return getJBBSubsetAnalysis()->getSuccessorJBBCs();
  }

  // Get all the join predicates associated with getSuccessorJBBCs
  virtual const ValueIdSet & getPredsWithSuccessors() const
  {
    return getJBBSubsetAnalysis()->getPredsWithSuccessors();
  }

  // Get all JBBCs that are joined to this JBBSubset and that this
  // JBBSubset is dependent on. For definition of join dependency
  // Refer to "Connectivity Analyzer PRD" document.
  virtual const CANodeIdSet & getPredecessorJBBCs() const
  {
    return getJBBSubsetAnalysis()->getPredecessorJBBCs();
  }

  // Get all the join predicates associated with getMyDependentJBBCs
  virtual const ValueIdSet & getPredsWithPredecessors() const
  {
    return getJBBSubsetAnalysis()->getPredsWithPredecessors();
  }

  // Get the join preds between my jbbcs
  virtual const ValueIdSet & getLocalJoinPreds() const
  {
    return getJBBSubsetAnalysis()->getLocalJoinPreds();
  }

  // Get the innerNonSemi join preds between my jbbcs
  virtual const ValueIdSet & getLocalInnerNonSemiJoinPreds() const
  {
    return getJBBSubsetAnalysis()->getLocalInnerNonSemiJoinPreds();
  }

  // Get the join preds between my jbbcs that cause dependencies
  // i.e. all outer, semi and anti semi join preds
  virtual const ValueIdSet & getLocalDependentJoinPreds() const
  {
    return getJBBSubsetAnalysis()->getLocalDependentJoinPreds();
  }

  // check if this JBBSubset is legal i.e.
  // predecessor of each node is found within
  // this set
  inline NABoolean legal() const { return jbbcs_.legal(); }

  // Get all JBBCs that are have no join predicate with this JBBSubset.
  virtual CANodeIdSet getJBBCsThatXProductWithMe() const;

  // which JBBCs can I add to this subset while reserving its self-dpendency
  inline CANodeIdSet legalJBBCAdditions() const
  {
    return getJBBSubsetAnalysis()->legalJBBCAdditions();
  }

  ValueIdSet joinPredsWithOther(const JBBSubset & other) const;

  // Get the JBB which this JBBSubset is a subset of
  JBB* getJBB() const
  {
    // Should not be asked for empty subset
    CMPASSERT(jbbcs_.entries() > 0);

    // All jbbcs_ are from same JBB
    JBBC* jbbc = jbbcs_.getFirst().getNodeAnalysis()->getJBBC();

    if (!jbbc)
       return NULL;

    return jbbc->getJBB();
  }

  // Get the set of JBBCs in this JBBSubset
  virtual const CANodeIdSet & getJBBCs() const
  {
    return jbbcs_;
  }

  // set JBBSubset MultiJoin
  inline void setSubsetMJ(MultiJoin * subsetMJ)
  {
    getJBBSubsetAnalysis()->setSubsetMJ(subsetMJ);
  };

  // get MultiJoin representing JBBSubset
  MultiJoin * getSubsetMJ() const
  {
    return getJBBSubsetAnalysis()->getSubsetMJ();
  };

  Join * getPreferredJoin();

  // Get the Grouping Id for the group by in this JBBSubset
  inline CANodeId getGB() const
  {
    return gb_;
  }

  // Add a JBBC to the set of JBBCs in this JBBSubset. At this point we
  // do not do integrity check while updating a JBBSubset. The method
  // verify can be used to check integrity after updating a JBBSubset
  // if user is unsure about result.
  inline void addJBBC(CANodeId jbbc)
  {
    jbbcs_.insert(jbbc);
    clearAnalysis();
  }

  // Same as JBBC but inserts multiple JBBCs at the same time.
  inline void addJBBCs(const CANodeIdSet & jbbcs)
  {
    jbbcs_.insert(jbbcs);
    clearAnalysis();
  }

  // Set the GB Member grouping Id
  inline void setGB(CANodeId gb)
  {
    gb_ = gb;
    clearAnalysis();
  }

  // Set the JBBCs
  inline void setJBBCs(const CANodeIdSet & jbbcs)
  {
    jbbcs_= jbbcs;
    clearAnalysis();
  }

  // Copy the JBBCs and GB from other JBBSubset.
  // This method copy the JBBSubset without copying its heap_ affiliation.
  void copySubsetMembers(const JBBSubset & other);

  // Check if this vid is covered by what this JBBSubset can provide.
  NABoolean coversExpr(ValueId vid) const
  {
    return getJBBSubsetAnalysis()->coversExpr(vid);
  }

  // Subtract the content (JBBCs and GB) of other JBBSubset from this
  // JBBSubset. This method side-effect this JBBSubset.
  void subtractSubset(const JBBSubset & other);

  // add the content (JBBCs and GB) of other JBBSubset to this
  // JBBSubset. This method side-effect this JBBSubset.
  void addSubset(const JBBSubset & other);

  // Does this JBBSubset has a mandatory cross product.
  // If so,=, then in any join order for this JBBSubset there will be
  // a cross product.
  // xxx: this function will be moved to JBBSubsetAnalysis
  NABoolean hasMandatoryXP() const;

  // Calculate number of disjunct subgraphs in this JBBSubset
  // xxx: this function will be moved to JBBSubsetAnalysis
  Lng32 numConnectedSubgraphs(NABoolean followSuccessors = TRUE,
                             NABoolean pullInPredecessors = FALSE) const;

  CANodeIdSet getJBBCsAndGB() const;

  CANodeIdSet getSubtreeTables() const;

  // verify integrity of the JBBSubset. That is
  // jbbcs_ contains only jbbcs from same JBB
  // gb_ belongs to a group by of the same JBB
  // xxx:Also verify that dependencies are respected when we
  // add outer and semi joins
  NABoolean verify();

  // Get the JBBSubsetAnalysis
  inline JBBSubsetAnalysis * getJBBSubsetAnalysis() const
  {
    if (!jbbSubsetAnalysis_)
      findJBBSubsetAnalysis();
    return jbbSubsetAnalysis_;
  }

  inline NABoolean allJoinsInnerNonSemi() const
  {
    return getJBBSubsetAnalysis()->allJoinsInnerNonSemi();
  }

  // get the minimum row count from all JBBCs in the JBBSubset after applying
  // local predicates to individual JBBCs

  CostScalar getMinimumJBBCRowCount() const;

  NABoolean isGuaranteedNonExpandingJoin(JBBC jbbc);

  // reset analysis. this is called everytime a change happen to this
  // JBBSubset.
  inline void clearAnalysis()
  {
    jbbSubsetAnalysis_ = NULL;
  }

  const NAString getText() const;

  void display() const;

  void print (FILE *f = stdout,
	      const char * prefix = DEFAULT_INDENT,
	      const char * suffix = "") const;

  SUBSET_LEGALITY getLegality() const {return getJBBSubsetAnalysis()->getLegality();};
  void setLegality(SUBSET_LEGALITY legality) { getJBBSubsetAnalysis()->setLegality(legality);};
  
  CASortedList * getSynthLogPropPath() {
    findJBBSubsetAnalysis();
    if(jbbSubsetAnalysis_) return jbbSubsetAnalysis_->getSynthLogPropPath();
    return NULL;
  }
  
  void setSynthLogPropPath(CASortedList * path) {
    findJBBSubsetAnalysis();
    if(jbbSubsetAnalysis_)
      jbbSubsetAnalysis_->setSynthLogPropPath(path);
    else
      CMPASSERT(FALSE);
  }
      
private:

  void findJBBSubsetAnalysis() const
  {
    mutate()->jbbSubsetAnalysis_ =
      QueryAnalysis::Instance()->findJBBSubsetAnalysis(*this);
    return;
  }

  // mutate method. use carefully and keep private (hush hush)
  // used only for methods that may change computable members
  inline JBBSubset* mutate() const
  {
    return (JBBSubset*) this;
  }

  CANodeIdSet         jbbcs_;
  CANodeId            gb_;
  JBBSubsetAnalysis*  jbbSubsetAnalysis_;

};

// ------------------------------------------------------------------------
// This class is a composite class of JBBItems
// It is ignored now as it serve no purpose during the first implementation
// phase. This is practically a SET of JBBItems.
// ------------------------------------------------------------------------
class JBBCompItem : public JBBItem
{

public:

  // construct a JBBCompItem
  JBBCompItem(CollHeap *outHeap = CmpCommon::statementHeap())
  {}

  // destruct a JBBCompItem
  virtual ~JBBCompItem()
  {}
};

// ------------------------------------------------------------------------
// This class is a composite class of JBBItems with order
// It is ignored now as it serve no purpose during the first implementation
// phase. This is practically a LIST of JBBItems.
// ------------------------------------------------------------------------
class JBBOrderedItem : public JBBCompItem
{

public:

  // construct a JBBOrderedItem
  JBBOrderedItem(CollHeap *outHeap = CmpCommon::statementHeap())
  {}

  // destruct a JBBOrderedItem
  virtual ~JBBOrderedItem()
  {}
};

// ------------------------------------------------------------------------
// JBBWA class: A work area for saving information related to a JBB
// ------------------------------------------------------------------------
class JBBWA : public NABasicObject
{

  //friend class JBB;

public:
  // construct a JBBWA
  JBBWA(JBB * parent, CollHeap *outHeap = CmpCommon::statementHeap())
  {
    // Initialize all the lists to NULL
    // Will compute each list on first demand
    byLocalKeyPrefixPredsCard_ = NULL;
    byLocalKeyPrefixPredsData_ = NULL;
    byLocalPredsCard_          = NULL;
    byLocalPredsData_          = NULL;
    byBaseCard_                = NULL;
    byBaseData_                = NULL;
    byNodeCard_                = NULL;
    byNodeData_                = NULL;
    byNodeOutputData_          = NULL;
    parentJBB_ = parent;
  }

  const CASortedList * getNodesSortedByLocalKeyPrefixPredsCard();
  const CASortedList * getNodesSortedByLocalKeyPrefixPredsData();
  const CASortedList * getNodesSortedByLocalPredsCard();
  const CASortedList * getNodesSortedByLocalPredsData();
  const CASortedList * getNodesSortedByBaseCard();
  const CASortedList * getNodesSortedByBaseData();
  const CASortedList * getNodesSortedByCard();
  const CASortedList * getNodesSortedByData();
  const CASortedList * getNodesSortedByOutputData();

private:
  // Provide a prototype function pointer for compatibility with various
  // compilers.  This was added for gcc compilation on Linux.
  typedef CostScalar (JBBWA::*shortMetricFunc)(CANodeId, EstLogPropSharedPtr&);

  // A generic method to sort given a method to compute the sort
  // metric for each node
  CASortedList* sort(shortMetricFunc getSortMetric);

  // Given a node, return it's cardinality after application of local preds
  // on the prefix of the clustering key. If logProps is passed in is NULL,
  // then ASM will be called to compute the logical properties.
  CostScalar getCardinalityAfterLocalKeyPrefixPreds(CANodeId node,
                                                    EstLogPropSharedPtr & logProps);

  // Given a node, return it's data-size (i.e. cardinality * rowsize)
  // after application of local preds on the prefix of the clustering key
  // If logProps is passed in is NULL, then ASM will be called to compute
  // the logical properties
  CostScalar getDataSizeAfterLocalKeyPrefixPreds(CANodeId node,
                                                 EstLogPropSharedPtr & logProps);

  // Given a node, return it's cardinality after application of local preds.
  // If logProps is passed in is NULL, then ASM will be called to compute
  // the logical properties
  CostScalar getCardinalityAfterLocalPreds(CANodeId node,
                                           EstLogPropSharedPtr & logProps);

  // Given a node, return it's data-size (i.e. cardinality * rowsize)
  // after application of local preds. If logProps is passed in is NULL,
  // then ASM will be called to compute the logical properties
  CostScalar getDataSizeAfterLocalPreds(CANodeId node,
                                        EstLogPropSharedPtr & logProps);

  // Given a node, return it's base cardinality
  CostScalar getBaseCardinality(CANodeId node,
                                EstLogPropSharedPtr & logProps);

  // Given a node, return it's size (i.e. rowcount * rowsize)
  CostScalar getBaseDataSize(CANodeId node,
                             EstLogPropSharedPtr & logProps);

  // Given a node, return it's cardinality
  CostScalar getNodeCardinality(CANodeId node,
                                EstLogPropSharedPtr & logProps);

  // Given a node, return it's size (i.e. rowcount * rowsize)
  CostScalar getNodeDataSize(CANodeId node,
                             EstLogPropSharedPtr & logProps);

  // Given a node, return it's output data size (i.e. rowcount * outputrowsize)
  CostScalar getNodeOutputDataSize(CANodeId node,
                             EstLogPropSharedPtr & logProps);

  // list of nodes sorted by cardinality after application of
  // local predicates on prefix of key
  CASortedList*                    byLocalKeyPrefixPredsCard_;

  // list of nodes sorted by data size (i.e. cardinality*rowsize)
  // after application of local predicates on prefix of key
  CASortedList*                    byLocalKeyPrefixPredsData_;

  // list of nodes sorted by cardinality after application of
  // all local predicates
  CASortedList*                    byLocalPredsCard_;

  // list of nodes sorted by data size (i.e. cardinality*rowsize)
  // after application of all local predicates
  CASortedList*                    byLocalPredsData_;

  // list of nodes sorted by base cardinality (i.e. number of rows
  // in the table)
  CASortedList*                    byBaseCard_;

  // list of nodes sorted by size of data in the table (i.e. number
  // of rows * rowsize)
  CASortedList*                    byBaseData_;

  // list of nodes sorted by cardinality (i.e. number of rows
  // returned by the node)
  // the difference between nodes and tables is that nodes includes
  // sub queries (e.g. group by) also
  CASortedList*                    byNodeCard_;

  // list of nodes sorted by size of data returned by the node (i.e.
  // number of rows * rowsize)
  CASortedList*                    byNodeData_;

  // list of nodes sorted by size of output data returned by the node (i.e.
  // number of rows * outputrowsize)
  CASortedList*                    byNodeOutputData_;
  
  // the JBB to which I belong
  JBB * parentJBB_;

  //heap on which the child objects should be created
  CollHeap*         heap_;

};

// ------------------------------------------------------------------------
// JBB class holds the info about the JBB
// ------------------------------------------------------------------------
class JBB : public NABasicObject
{

  friend class QueryAnalysis;

public:

  // destruct a JBB
  virtual ~JBB()
  {}

  // initialize and compute the JBB starting from the top join node
  void analyze(Join* topJoinExpr);

  // initialize and compute the JBB starting from the top GB node
  void analyze(GroupByAgg* topGBExpr);

  // initialize and compute the JBB starting from the top Scan node (MVQR only for now)
  void analyze(Scan* topScanExpr);

  // analyze the dependency relations between children
  // the dependency analysis is for dependencies due to:
  // * Left Joins
  // * Semi Joins
  void analyzeChildrenDependencies();

  // compute predicates that are not part of the
  // join predicate for a left join, rather these
  // are a filter on the left join node
  void computeLeftJoinFilterPreds();

  // compute subgraphs needed by non inner join children to avoid XP
  void computeChildrenSubGraphDependencies();
  
  // This include all the JBBCs and the GB of this JBB
  const JBBSubset & getMainJBBSubset() const
  {
    return mainJBBSubset_;
  }

  // return the set of all jbbcs in this jbb
  inline const CANodeIdSet getJBBCs() const
  {
    return getMainJBBSubset().getJBBCs();
  }

  // The GBAnalysis for the group by on the top of this JBB
  GBAnalysis * getGBAnalysis() const
  {
    return gbAnalysis_;
  }

  // Workspace for the JBB
  JBBWA * getJBBWA() const
  {
    return workArea_;
  }

  // Global Id for the JBB in the QueryAnalysis
  CollIndex getJBBId() const
  {
    return jbbId_;
  }

  // Get the char. inputs during Query Analysis time. These were
  // the inputs from the normalizer tree for the top expression
  // in the join backbone.
  inline const ValueIdSet & getNormInputs() const
  {
    return normInputs_;
  }

  // Get the char. outputs during Query Analysis time. These were
  // the outputs from the normalizer tree for the top expression
  // in the join backbone.
  inline const ValueIdSet & getNormOutputs() const
  {
    return normOutputs_;
  }

  // Does the JBB has a mandatory cross product?
  NABoolean hasMandatoryXP() const;

  void setInputEstLP(EstLogPropSharedPtr& inEstLP);

  EstLogPropSharedPtr getInputEstLP()
  {
    return inEstLP_;
  }

  const NAString graphDisplay(const QueryAnalysis* qa) const;

  const NAString getText() const;

  void display() const;

  void print (FILE *f = stdout,
	      const char * prefix = DEFAULT_INDENT,
	      const char * suffix = "") const;

private:

  // construct a JBB
  JBB(CollIndex id, CollHeap *outHeap = CmpCommon::statementHeap()):
    jbbId_(id),
    heap_(outHeap),
    inEstLP_((*GLOBAL_EMPTY_INPUT_LOGPROP))
  {
    workArea_ = new (heap_) JBBWA(this, heap_);
    gbAnalysis_ = 0;
  }

  JBB(CollHeap *outHeap = CmpCommon::statementHeap())
  {
    // should not be called
  }

  // Add this JBBC to the JBB.
  void addJBBC(CANodeId jbbc);
  void addJBBC(JBBC* jbbc);

  // Add this GroupBy to the JBB
  void setGB(CANodeId gb);
  void setGB(GBAnalysis* gb);

  // Set the norm char inputs and outputs
  void setNormInputs(const ValueIdSet & inputs);
  void setNormOutputs(const ValueIdSet & outputs);

  JBBWA*            workArea_;
  JBBSubset         mainJBBSubset_;
  GBAnalysis*       gbAnalysis_;
  ValueIdSet        normInputs_;
  ValueIdSet        normOutputs_;
  CollIndex         jbbId_;
  EstLogPropSharedPtr inEstLP_;
  CollHeap*         heap_;
};

// -----------------------------------------------------------------------
// The Connection Predicate class
// This class captures the columns and tables that a certain predicate
// is connecting. It can also capture interesting info about the predicate
// such as selectivity and usefulness for access paths.
// -----------------------------------------------------------------------
class PredAnalysis : public NABasicObject
{

public:

  // Construct a PredAnalysis
  PredAnalysis(ValueId predicate):
  predicate_(predicate)
  {}

  // Destructor
  virtual ~PredAnalysis()
  {}

  // Get the predicate ValueId
  inline ValueId getPredicate()
  {
    return predicate_;
  }

  // Get all columns directly connected by this predicate
  inline ValueIdSet & getReferencedCols()
  {
    return referencedCols_;
  }

  // Add this column to the list of Referenced col by me
  inline void addToReferencedCols(ValueId col)
  {
    referencedCols_ += col;
  }
  inline void addToReferencedCols(const ValueIdSet & cols)
  {
    referencedCols_ += cols;
  }

  // Get all columns directly connected by this VEG predicate
  inline ValueIdSet & getVegConnectedCols()
  {
    return vegConnectedCols_;
  }

  // Add this column to the list of vegConnected col by me
  inline void addToVegConnectedCols(ValueId col)
  {
    vegConnectedCols_ += col;
  }
  inline void addToVegConnectedCols(const ValueIdSet & cols)
  {
    vegConnectedCols_ += cols;
  }

  // Get all columns directly connected by this Equality Predicate
  inline ValueIdSet & getEqualityConnectedCols()
  {
    return equalityConnectedCols_;
  }

  // Add this column to the list of vegConnected col by me
  inline void addToEqualityConnectedCols(ValueId col)
  {
    equalityConnectedCols_ += col;
  }
  inline void addToEqualityConnectedCols(const ValueIdSet & cols)
  {
    equalityConnectedCols_ += cols;
  }

  // Get all tables directly connected by this predicate
  inline CANodeIdSet & getConnectedTables()
  {
    return connectedTables_;
  }

  // Get all JBBCs referenced by this predicate
  inline CANodeIdSet & getReferencedJBBCs()
  {
    return referencedJBBCs_;
  }

  // Add this JBBC to my set of referenced JBBCs
  inline void addToReferencedJBBCs(const CANodeId jbbc)
  {
    referencedJBBCs_ += jbbc;
  }

  const NAString getText() const
  {
    return "PredAnalysis";
  }

private:

  ValueId                  predicate_;
  ValueIdSet               referencedCols_;
  ValueIdSet               vegConnectedCols_;
  // Predicate remained equality '=' i.e. did not become a VEG
  // this excludes equality between same table columns.
  ValueIdSet               equalityConnectedCols_;
  CANodeIdSet              referencedTables_;
  CANodeIdSet              connectedTables_;
  CANodeIdSet              referencedJBBCs_;


};


// -----------------------------------------------------------------------
// The ValueAnalysis class
// -----------------------------------------------------------------------
class ValueAnalysis : public NABasicObject
{

public:

  // Construct a ValueAnalysis
  ValueAnalysis(ValueId value,
           CollHeap *outHeap = CmpCommon::statementHeap());

  // Destructor
  virtual ~ValueAnalysis()
  {}

  // Get the ValueId
  inline ValueId valueId()
  {
    return valueId_;
  }

  // Get all columns directly or indirectly referenced by this value
  inline const ValueIdSet & getReferencedCols()
  {
    return referencedCols_;
  }

  // Get all tables whose column(s) was directly or indirectly
  // referenced by this value
  inline const CANodeIdSet & getReferencedTables()
  {
    return referencedTables_;
  }

  inline void addToReferencedCols(ValueId c)
  {
    referencedCols_ += c;
  }

  const NAString getText() const
  {
    return "ValueAnalysis";
  }

private:

  ValueId                  valueId_;
  ValueIdSet               referencedCols_;
  CANodeIdSet              referencedTables_;
  ColAnalysis*             col_;  // NULL unless this a base column
  PredAnalysis*            pred_; // NULL unless this is a predicate

};

// -----------------------------------------------------------------------
// The Column connectivity Analysis class
// This class captures the columns and tables that a certain column
// is connected to.
// -----------------------------------------------------------------------

class ColAnalysis : public NABasicObject
{

  friend class QueryAnalysis;

public:

  // Destructor
  virtual ~ColAnalysis()
  {}

  // Get the Column ValueId
  inline ValueId getColumnId()
  {
    return column_;
  }

  // Get all columns directly connected to this column
  // (via VEG predicates)
  inline ValueIdSet & getConnectedCols()
  {
    return connectedCols_;
  }

  // Get all tables that have columns directly connected to this column
  // (via VEG predicates). My table execluded ofcourse.
  inline CANodeIdSet & getConnectedTables()
  {
    return connectedTables_;
  }

  // Get all sibling JBBCs that are joined to this table on this column
  // (via VEG predicates).
  CANodeIdSet getConnectedJBBCs();

  // Get VEG connecting Preds between me and this JBBC.
  ValueIdSet getConnectingPreds(JBBC* jbbc);

  // Get all sibling JBBCs that are joined to this table on this column
  // these include JBBCs joined via non_VEG preds like t1.a = t2.b + 7
  CANodeIdSet getAllConnectedJBBCs();

  // get all connecting Preds between me and this JBBC.
  ValueIdSet getAllConnectingPreds(JBBC* jbbc);

  // Return the TableAnalysis for my table
  inline TableAnalysis * getTableAnalysis()
  {
    return table_;
  }

  // Return the TableDesc for my table
  inline TableDesc * getTableDesc()
  {
    return table_->getTableDesc();
  }

  // Add these predicates to the list of Referencing predicates for me
  inline void addToReferencingPreds(ValueId pred)
  {
    referencingPreds_ += pred;
  }
  inline void addToReferencingPreds(const ValueIdSet & preds)
  {
    referencingPreds_ += preds;
  }
  inline const ValueIdSet & getReferencingPreds()
  {
    return referencingPreds_;
  }

  // Add these predicates to the list of connecting veg predicates for me
  inline void addToVegPreds(ValueId pred)
  {
    vegPreds_ += pred;
  }
  inline void addToVegPreds(const ValueIdSet & preds)
  {
    vegPreds_ += preds;
  }

  inline const ValueIdSet & getVegPreds()
  {
    return vegPreds_;
  }

  // Add these to the list of equality predicates that did not become VEG
  // these exclude predicates with other columns in the same table i.e. 
  // t1.a = t1.b
  inline void addEqualityRelation(ValueId colId, 
                                  ValueId pred,
                                  ValueId equalityExpr,
                                  CANodeIdSet connectedJBBCs)
  {
    equalityConnectingPreds_ += pred;
    equalityConnectedJBBCs_ += connectedJBBCs;
    if(table_)
      table_->addEqualityRelation(colId, pred, equalityExpr, connectedJBBCs);
  }
  
  inline const ValueIdSet & getEqualityConnectingPreds()
  {
    return equalityConnectingPreds_;
  }

  inline const CANodeIdSet & getEqualityConnectedJBBCs()
  {
    return equalityConnectedJBBCs_;
  }

  // Add these to the list of equality predicates against constants 
  // that did not become VEG i.e. 
  // t1.a = extract_month(10212009);
  inline void addToEqualityCoveringPreds(ValueId preds)
  {
    equalityCoveringPreds_ += preds;
  }
  
  inline const ValueIdSet & getEqualityCoveringPreds()
  {
    return equalityCoveringPreds_;
  }

  // attribute maxFreq_ is used to cache the cardinality of the most frequently
  // occuring value.

  inline CostScalar getMaxFreq()
  {
	return maxFreq_;
  }

  inline void setMaxFreq(CostScalar val)
  {
	maxFreq_ = val;
  }

  // finalUec_ is used to store the total UEC of the column, just before
  // the column is dropped from the list of histograms being passed to the
  // parent. Example, if the column has only local predicates on it, and is
  // not participating in the query anywhere. It will be dropped after the Scan
  // This attribute will contain the UEC of the column after applying local
  // predicates

  inline CostScalar getFinalUec()
  {
	return finalUec_;
  }

  inline void setFinalUec(CostScalar val)
  {
	finalUec_ = val;
  }

  inline CostScalar getFinalRC()
  {
	return finalRC_;
  }

  inline void setFinalRC(CostScalar val)
  {
	finalRC_ = val;
  }

  // find all local preds referencing this column
  const ValueIdSet getLocalPreds() const;

  // find veg preds connecting this column with other
  ValueIdSet getConnectingVegPreds(ValueId other);

  // FALSE does not mean column is not a constant. It means we did not verify
  // that it is a constant.
  NABoolean getConstValue(ItemExpr* & cv, NABoolean refAConstValue=TRUE) const;

  // finalized analysis
  void finishAnalysis();

  const NAString getText() const;

private:

  // Constructor a ColAnalysis. Called only by QueryAnalysis
  ColAnalysis(ValueId col,
              TableAnalysis* table):
    column_(col),
    table_(table)
  {
    maxFreq_ = csMinusOne;
    finalUec_ = csZero;
    finalRC_= csZero;
  }

  ColAnalysis()
  {
    // should never be called
  }

  ValueId                  column_;
  TableAnalysis*           table_;
  ValueIdSet               referencingPreds_;
  ValueIdSet               vegPreds_;
  // equality predicates that did not become VEG
  // these exclude predicates with other columns in the same table i.e. 
  // t1.a = t1.b
  ValueIdSet               equalityConnectingPreds_;
  CANodeIdSet              equalityConnectedJBBCs_;
  ValueIdSet               equalityCoveringPreds_;
  ValueIdSet               connectedCols_;
  CANodeIdSet              connectedTables_;
  CANodeIdSet              connectedJBBCs_;
  CostScalar               maxFreq_;
  CostScalar               finalUec_;
  CostScalar               finalRC_;
};

//---------------------------------------------------------
// TableConnectivityGraph Class
//---------------------------------------------------------
class TableConnectivityGraph : public NABasicObject
{

  friend class QueryAnalysis;

public:

  // Construct a TableConnectivityGraph
  TableConnectivityGraph(CollHeap *outHeap = CmpCommon::statementHeap())
  {}

  // Destuctor
  virtual ~TableConnectivityGraph()
  {}

  // get the set of all base columns in the query
  inline const ValueIdSet & getColumns()
  {
    return cols_;
  }

  // get the set of all base tables in the query
  inline const CANodeIdSet & getTables()
  {
    return tables_;
  }

  const NAString getText() const
  {
    return "TableConnectivityGraph";
  }

private:

  ValueIdSet               connections_;
  ValueIdSet               cols_;
  CANodeIdSet              tables_;
};

class TableCANodeIdPair : public NABasicObject
{
friend class TableCANodeIdPairLookupList;
friend class CqsWA;
private:
  TableCANodeIdPair() {visited_ = FALSE;}
  const TableDesc *tabId_;
  CANodeId Id_;
  NABoolean visited_; // how to deal with same table appearing twice?
private:
};

class TableCANodeIdPairLookupList : public LIST(TableCANodeIdPair *)
{
public:
  TableCANodeIdPairLookupList(CollHeap *h) : LIST(TableCANodeIdPair *)(h) {}

};

class CQSRelExprCANodeIdPair : public NABasicObject
{
  friend class CQSRelExprCANodeIdMap;
  friend class CqsWA;
private:
  RelExpr *forcedNode_;
  CANodeIdSet leftChildSet_;
  CANodeIdSet rightChildSet_;
  CollIndex parent_;

public:
  CANodeId populateReturnCANodeId(RelExpr *, CqsWA*);
  NABoolean operator == (CQSRelExprCANodeIdPair other)
         {return forcedNode_ == other.forcedNode_;}
};


class CQSRelExprCANodeIdMap : public HASHDICTIONARY(ULng32, CQSRelExprCANodeIdPair )
{
public:
  static ULng32 HashFn(const ULng32 & key);

  CQSRelExprCANodeIdMap(ULng32 init_size = 30,
                         CollHeap *outHeap = CmpCommon::statementHeap());

  CANodeIdSet  gatherNodeIdSetsForCQSTree(RelExpr*, CqsWA*);
  CQSRelExprCANodeIdPair * get(RelExpr *);
  void insertThisElement(RelExpr *, CQSRelExprCANodeIdPair *);

private:
    ULng32 *myKey_;

};

class CqsWA : public NABasicObject
{
private:

  TableCANodeIdPairLookupList *tableCANodeList_;
  CQSRelExprCANodeIdMap  *cqsCANodeIdMap_;

public:
  CqsWA();
  void setTableNodeIdPairList(TableCANodeIdPairLookupList * tc)
         { tableCANodeList_=tc;}

  TableCANodeIdPairLookupList * getTableCANodeList()
         { return tableCANodeList_;}

  CQSRelExprCANodeIdMap  * getcqsCANodeIdMap()
         { return cqsCANodeIdMap_;}

  void  gatherNodeIdSetsForCQSTree(RelExpr*);

  void gatherCANodeIDTableNamepairsForNormalizedTree( RelExpr *nqtExpr);
  void initialize(RelExpr *, RelExpr *);
  CANodeId findCANodeId(const NAString &);

  void getTableSets(RelExpr *, CANodeIdSet &, CANodeIdSet &);

  RelExpr *checkAndProcessGroupByJBBC(RelExpr *, CANodeIdSet &,
                                      CANodeIdSet &, RelExpr *);
  static NABoolean shouldContinue(RelExpr *, RelExpr *);
  NABoolean isIndexJoin(RelExpr *);

  NAString  makeItThreePartAnsiString(const NAString &);

  NABoolean reArrangementSuccessful_;
  void incrementNumberOfScanNodesinCQS()
     { numberOfScanNodesinCQS_++;}
private:
  UInt32 numberOfScanNodesinNQT_;
  UInt32 numberOfScanNodesinCQS_;
  static NABoolean containsCutOp(RelExpr *);
  NABoolean isMPTable(const NAString &);
  static NABoolean containsNotSupportedOperator(RelExpr *);

};

// Definition of inline methods

NodeAnalysis * CANodeId::getNodeAnalysis()
{
  return QueryAnalysis::Instance()->getNodeAnalysis(id_);
}

const CANodeIdSet & QueryAnalysis::getTables()
{
  return tCGraph_->getTables();
}

const ValueIdSet & QueryAnalysis::getColumns()
{
  return tCGraph_->getColumns();
}


#endif /* ANALYZER_H */

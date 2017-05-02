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
/* -*-C++-*-
******************************************************************************
*
* File:         Analyzer.cpp
* Description:  Query Analyzer classes and methods
*
* Created:      6/7/2002
* Language:     C++
*
*
*
*
******************************************************************************
*/

//#include <process.h>

#include "Analyzer.h"

#include "MultiJoin.h"
#include "AppliedStatMan.h"
#include "opt.h"
#include "RelExpr.h"
#include "TransRule.h"

//tmp includes for join and other relexpr methods
#include "RelJoin.h"
#include "RelScan.h"
#include "RelGrby.h"
#include "RelUpdate.h"
#include "RelRoutine.h"
#include "RelExeUtil.h"
#include "QRDescGenerator.h"
#include "QueryRewriteHandler.h"
#include "Globals.h"

void forceCQS1(RelExpr *, RelExpr *);

// LCOV_EXCL_START :rfi
// tmp methods
void xxx()
{
  CMPASSERT(FALSE);
}

void revisitLater()
{
  CMPASSERT(FALSE);
}
// LCOV_EXCL_STOP

// -----------------------------------------------------------------------
// Methods for class CANodeId
// -----------------------------------------------------------------------
EstLogPropSharedPtr CANodeId::getJBBInput()
{
  JBB * jbb = NULL;

  if(getNodeAnalysis()->getJBBC())
    jbb = getNodeAnalysis()->getJBBC()->getJBB();

  EstLogPropSharedPtr jbbInLP = (*GLOBAL_EMPTY_INPUT_LOGPROP);

  if(jbb)
    jbbInLP = jbb->getInputEstLP();
    
  return jbbInLP;
}

// -----------------------------------------------------------------------
// Methods for class CANodeIdSet
// -----------------------------------------------------------------------

CANodeIdSet::CANodeIdSet(CollHeap *outHeap) :
   SUBARRAY(NodeAnalysis *)(NULL, outHeap)
{}

CANodeIdSet::CANodeIdSet(const CANodeIdSet &other, CollHeap *outHeap) :
   SUBARRAY(NodeAnalysis *)(other, outHeap)
{}

CANodeIdSet::CANodeIdSet(const CANodeId& id, CollHeap *outHeap) :
   SUBARRAY(NodeAnalysis *)(NULL, outHeap)
{
  addElement(id.id_);
}

CANodeId CANodeIdSet::getFirst() const
{
  CANodeId id = init();
  if (NOT next(id))
    id = NULL_CA_ID;
  return id;
}

CANodeIdSet CANodeIdSet::operator +(const CANodeIdSet & other) const
{
  CANodeId id_;
  CANodeIdSet result(*this, CmpCommon::statementHeap());

  for (id_ = other.init();
       other.next(id_);
       other.advance(id_))
  {
    result.addElement(id_);
  }

  return result;
}

// Assumption: each member of this set represents a jbbc
//
// A set is legal if:
// * the predecessors required for each jbbc are present
//   in the set
// AND
// * there is atleast on jbbc connected via a InnerNonSemi
//   join
NABoolean CANodeIdSet::legal() const
{
  CANodeIdSet nodeSet(*this);
  CANodeId i;
  NABoolean hasInnerNonSemiNonTSJJoin = FALSE;
  
  JBBSubset * jbbSubset = jbbcsToJBBSubset();
  
  if(jbbSubset && (jbbSubset->getLegality() != NOT_KNOWN))
  {
    if(jbbSubset->getLegality() == LEGAL)
      return TRUE;
    
    return FALSE;
  }
  
  for (i = init(); next(i); advance(i))
  {
    Join * parentJoin =
      i.getNodeAnalysis()->getJBBC()->getOriginalParentJoin();

    // if a jbbc is connected via a InnerNonSemi join it is
    // legal. It can be legally present anywhere and does not
    // depend on the presence of a predecessor. Such guys are
    // citizens they don't need anyone to sponsor them to make
    // them legal :)
    if ((!parentJoin) ||
        (parentJoin->isInnerNonSemiNonTSJJoin()))
      // an interesting thing to note is that sometimes jbbcs
      // connected via a special join e.g. left join don't have
      // any join predicate. In such a case the jbbc would be
      // legal by itself if we only checked for predecessors.
      // But since such a jbbc can only be the right child of a
      // join we require that there is atleast one jbbc connected
      // via a InnerNonSemi join so that it can be the left child
      // of the left most join.
      hasInnerNonSemiNonTSJJoin = TRUE;
    else{
      // if a jbbc is connected via a special join (i.e. left,
      // semi, anti semi) then it needs some predecessors to
      // make it legal. He needs some predecessor to sponsor
      // him to make him a legal citizen. The legal addition
      // check below ensures that jbbc i is a legal citizen :)
      nodeSet -= i;
      if(!nodeSet.legalAddition(i))
      {
        if(jbbSubset)
          jbbSubset->setLegality(ILLEGAL);
        return FALSE;
      }
      nodeSet += i;
    }

  }

  if(hasInnerNonSemiNonTSJJoin)
  {
    if(jbbSubset)
      jbbSubset->setLegality(LEGAL);

    return TRUE;
  }

  if(jbbSubset)
    jbbSubset->setLegality(ILLEGAL);

  return FALSE;
}

NABoolean CANodeIdSet::legalAddition(CANodeId newNode) const
{
  JBBC* newNodeJBBC = newNode.getNodeAnalysis()->getJBBC();
  Join * newNodeParentJoin = newNodeJBBC->getOriginalParentJoin();

  // left most child in the original join ordering
  if(!newNodeParentJoin)
    return TRUE;
  // right child of an inner join
  else if (newNodeParentJoin->isInnerNonSemiNonTSJJoin())
    return TRUE;
  // right child of left, semi, anti-semi join, routine join
  else{
    CANodeIdSet predecessorJBBCs = newNodeJBBC->getPredecessorJBBCs();

	  // if the newNode is connect via a:
    // * left outer join
    // * semi join
    // * anti semi join
    // * routine join
    // Then make sure the dependencies are satisfied

    // return false since there are no JBBCs in 'this' set
    // therefore any dependency will not be satisfied
    if(!entries())
      return FALSE;

	  if(CmpCommon::getDefault(ASYMMETRIC_JOIN_TRANSFORMATION) == DF_MINIMUM)
	  {
      if(!contains(predecessorJBBCs))
        return FALSE;
	  }
    else{

      if(contains(predecessorJBBCs))
        return TRUE;

      if(predecessorJBBCs.entries() < 2)
	    {
        if(!contains(predecessorJBBCs))
          return FALSE;
      }
      else{

        JBBSubset * jbbSubset = jbbcsToJBBSubset();

        // predicates for which newNode is successor
        ValueIdSet dependencyPreds =
          jbbSubset->getPredsWithSuccessors();
        ValueIdSet newNodePredsWithPredecessors =
          newNodeJBBC->getPredsWithPredecessors();
        newNodePredsWithPredecessors -=
          newNodeJBBC->getConstPredsWithPredecessors();
        dependencyPreds.intersectSet(newNodePredsWithPredecessors);

        CANodeIdSet combinedSet;
        combinedSet += *this;
        combinedSet += newNode;
        JBBSubset * combinedJBBSubset = combinedSet.jbbcsToJBBSubset();

        if(newNodeParentJoin && newNodeParentJoin->isRoutineJoin())
        {
          ValueIdSet newNodeInputsRequiredFromSiblings =
            newNodeJBBC->getInputsRequiredFromSiblings();

          // check if input required from siblings is covered
          for (ValueId input= newNodeInputsRequiredFromSiblings.init();
              newNodeInputsRequiredFromSiblings.next(input);
              newNodeInputsRequiredFromSiblings.advance(input) )
          {
            NABoolean inputIsCovered = combinedJBBSubset->coversExpr(input);

            if(!inputIsCovered)
              return FALSE;
          }          
        }

        // if all predicates with predecessors for the new node
        // are not covered then this is not a legal addition
        if (dependencyPreds.entries() != newNodePredsWithPredecessors.entries())
          return FALSE;

        // if all predicates with predecessors for the new node
        // are covered, we need to do a coverage test. This is
        // needed because the predicate could be a 'hyper' join
        // pred i.e. a predicate that required presence of all
        // successors and not just one of them e.g. t3.a = t1.b + t2.c.
        // If t2 and t3 are successors then the predicate requires all
        // successors. In case like:
        //
        // select *
        // from
        //  t1
        //  join
        //  t2 on t1.a = t2.a
        //  left join
        //  t3 on t1.a = t3.a;
        //
        // In the case above sets {t1,t3} and {t2,t3} are legal
        // We can have anyone of t1 or t2 paired up with t3
        // Of course set {t3} would be illegal. In the hyper join
        // case the {t1, t3} and {t2, t3} are not legal rather
        // {t1, t2, t3} is legal of course {t1,t2} would be legal
        // This is because we need all three to evaluate the predicate
        // t3.a = t1.b + t2.c
        for (ValueId x= newNodePredsWithPredecessors.init();
            newNodePredsWithPredecessors.next(x);
            newNodePredsWithPredecessors.advance(x) )
        {
          NABoolean predIsCovered = combinedJBBSubset->coversExpr(x);

          if(!predIsCovered)
            return FALSE;
        }
      }
    }
  }
  return TRUE;
}

// Find the jbbc from the jbbsubset, that has minimum connections
// If there are more than one jbbcs, with the same number of
// connections, pick the first one.

CANodeId CANodeIdSet::getJBBCwithMinConnectionsToThisJBBSubset() const
{
  CollIndex noOfConn = MAX_COLL_INDEX;
  CANodeId childWithMinConnections = getFirst();

  CANodeIdSet jbbcsWithMinConnections(childWithMinConnections);
  CostScalar maxEqFactor = csZero;

  CANodeId id;

  if (entries() == 1)
    return childWithMinConnections;

  // For two way joins make sure to return a child that can be the
  // right side of a join. If one child is the right child of a
  // left, semi, anti-semi join then that child should be returned.
  // This is because that child cannot be the left child of a join
  if (entries() == 2){
    for(id = init(); next(id); advance(id))
    {
      JBBC* idJbbc = id.getNodeAnalysis()->getJBBC();
      Join * jbbcParent = idJbbc->getOriginalParentJoin();
      if (jbbcParent && (!jbbcParent->isInnerNonSemiNonTSJJoin()))
        childWithMinConnections = id;
    }
    return childWithMinConnections;
  }

  // traverse thru all the nodes to check if
  // this set has only one node connected via
  // a inner non semi join
  UInt32 numNodesConnectedViaInnerNonSemiJoin = 0;
  CANodeId nodeConnectedViaInnerNonSemiJoin;
  for(id = init(); next(id); advance(id))
  {
    JBBC* idJbbc = id.getNodeAnalysis()->getJBBC();
    Join * jbbcParent = idJbbc->getOriginalParentJoin();
    if((!jbbcParent) ||
       (jbbcParent->isInnerNonSemiNonTSJJoin()))
    {
      numNodesConnectedViaInnerNonSemiJoin++;
      nodeConnectedViaInnerNonSemiJoin = id;
    }
  }

  // traverse thru all connections passed
  // to determine the one with minimum connection
  for (id = init();
       next(id);
	   advance(id))
  {
    JBBC* idJbbc = id.getNodeAnalysis()->getJBBC();

    // dont pick the only node that can be a left child of a join
    if((numNodesConnectedViaInnerNonSemiJoin == 1) &&
       (id == nodeConnectedViaInnerNonSemiJoin ))
      continue;

    CANodeIdSet jbbcsJoinedToThisSet(*this);
	  jbbcsJoinedToThisSet.intersectSet(idJbbc->getJoinedJBBCs());

	  if (idJbbc->getSuccessorJBBCs().entries() != 0)
	  {
	    CANodeIdSet dependentJbbcs(*this);
	    dependentJbbcs.intersectSet(idJbbc->getSuccessorJBBCs());
	    if (dependentJbbcs.entries() != 0)
	      continue;
    }

	  CollIndex connectedJBBCsCount = jbbcsJoinedToThisSet.entries();

   // get the connections from this subset
   if ( connectedJBBCsCount < noOfConn)
   {
	  noOfConn = connectedJBBCsCount;
	  // flush the previously stored conflicitng JBBCs and start a new set
	  jbbcsWithMinConnections.clear();
	  jbbcsWithMinConnections.insert(id);
	  childWithMinConnections = id;
	}
	else
	{
	  // collect all conflicting nodes
	   if ( connectedJBBCsCount == noOfConn)
	  {
		noOfConn = connectedJBBCsCount;
		jbbcsWithMinConnections.insert(id);
	  }
	}
   } // for loop to pick the jbbc with min connections, and collect if there are
	 // more than one with min connections.

// LCOV_EXCL_START 
// debugging code
#ifdef _DEBUG
if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
   CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
{
  CURRCONTEXT_OPTDEBUG->stream() << endl
				  << "jbbcs with minimum connection "\
		          << jbbcsWithMinConnections.getText();
  CURRCONTEXT_OPTDEBUG->stream() << "Number of connections "\
                  << istring(Lng32(noOfConn))\
                  <<endl;
}
#endif
// LCOV_EXCL_STOP
  if (jbbcsWithMinConnections.entries() > 1)
  {

	CostScalar smallestTableSize = COSTSCALAR_MAX;
	CostScalar currentTableSize = csOne;

	// traverse thru all conflicting nodes (nodes with minimum connection)
	for (id = jbbcsWithMinConnections.init();
	   jbbcsWithMinConnections.next(id);
	   jbbcsWithMinConnections.advance(id))
	{
	  // if the node is not a table, we continue
	  TableAnalysis * currentTA = id.getNodeAnalysis()->getTableAnalysis();

	  if (!currentTA) continue;

	  // get all the nodes, this one is connected to
      CANodeIdSet jbbcsConnectedToId(*this);

	  jbbcsConnectedToId.subtractElement(id);

	  // out of the remaining nodes, traverse thru the ones that are
	  // connected to current id.

	  CANodeIdSet connectedNodes = currentTA->getConnectedTables();

	  jbbcsConnectedToId.intersectSet(connectedNodes);

	  for (CANodeId id2 = jbbcsConnectedToId.init();
		   jbbcsConnectedToId.next(id2);
		   jbbcsConnectedToId.advance(id2) )
		   {
			 // get all columns with equality predicates between this
			 // and the connecting node
			 ValueIdSet idCols = currentTA->getColsConnectedViaVEGPreds(id2);

			 // get the equality factor for this set of columns.
			 // Equality factor is defined as UEC / base row count.
			 CostScalar eqFactor = currentTA->getEqualityVal(idCols);

			 currentTableSize = currentTA->getCardinalityOfBaseTable();

			 if ((eqFactor > maxEqFactor) ||
				   ((!CURRSTMT_OPTDEFAULTS->isComplexMJQuery()) &&
            (eqFactor == maxEqFactor) &&
				    (currentTableSize < smallestTableSize)))
			 {
			   // save the node with max equality factor
			   maxEqFactor = eqFactor;
			   childWithMinConnections = id;
               smallestTableSize = currentTableSize;

			 }
#ifdef _DEBUG
if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
   CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
{
  CURRCONTEXT_OPTDEBUG->stream() << "Connections tried "\
                  << id.getText() \
				  << " with " << id2.getText() \
				  << endl;   // LCOV_EXCL_LINE :dpm
  CURRCONTEXT_OPTDEBUG->stream() << "Number of columns "\
                  << istring(Lng32(idCols.entries()))\
                  <<endl;   // LCOV_EXCL_LINE :dpm
  CURRCONTEXT_OPTDEBUG->stream() << "Equality factor "\
                  << eqFactor.value() \
                  << endl
				  << endl;   // LCOV_EXCL_LINE :dpm
}
#endif
		   } // for loop to traverse all two way connections

	} // For loop to traverse over all confliciting ID, computing their equality factors
  } // end if number of conflicting jbbcs > 1

#ifdef _DEBUG
if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
   CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
{
  CURRCONTEXT_OPTDEBUG->stream() << "CANodeId picked up "\
                  << childWithMinConnections.getText() \
				  << endl;   // LCOV_EXCL_LINE :dpm
  CURRCONTEXT_OPTDEBUG->stream() << "Max Equality factor "\
                  << maxEqFactor.value() \
                  << endl
				  << endl;   // LCOV_EXCL_LINE :dpm
}
#endif
  return childWithMinConnections;
}

// LCOV_EXCL_START :cnu
// get minimum estimated row count of the group from amongst the nodeSet
CostScalar CANodeIdSet::getMinChildEstRowCount() const
{
  CostScalar minEstCard = COSTSCALAR_MAX;
  CostScalar minCard = csOne;

  for (CANodeId nodeId = init();
       next(nodeId);
       advance(nodeId) )
    {
      minCard = (nodeId.getNodeAnalysis()->getOriginalExpr()->getGroupAttr()->getMinChildEstRowCount()).minCsOne();
      if (minCard < minEstCard)
	minEstCard = minCard;
    }
    return minEstCard;
}
// LCOV_EXCL_STOP


// compute the JBBSubset structure for this CANodeIdSet
// Note that not every CANodeIdSet has a JBBSubset (for example
// all JBBCs has to be of the same JBB). The method verify()
// on JBBSubset can be used to verify this condition.
JBBSubset * CANodeIdSet::computeJBBSubset() const
{
  QueryAnalysis* qa = QueryAnalysis::Instance();

  CANodeIdSet jbbcs(*this);
  jbbcs.intersectSet(qa->getJBBCs());

  JBBSubset* result = jbbcs.jbbcsToJBBSubset();

  CANodeIdSet gbs(*this);
  gbs.intersectSet(qa->getGBs());
  if (gbs.entries() > 0)
  {
    CMPASSERT(gbs.entries() == 1);
    CANodeId gb = gbs.getFirst();
    result->setGB(gb);
  }
  return result;
}

EstLogPropSharedPtr CANodeIdSet::getJBBInput() const
{
  JBB * jbb = NULL;

  CANodeId jbbcId = getFirst();

  if(jbbcId != NULL_CA_ID)
    return jbbcId.getJBBInput();

  return (*GLOBAL_EMPTY_INPUT_LOGPROP);
}

// This method also compute a JBBSubset but much faster than
// computJBBSubset() because it assume the CANodeIdSet contains
// only sibling JBBCs. Use only if that condition is true (which
// is the case for most CANodeIdSet values generated by getJoinedJBBCs()
// and similar methods on JBBC and JBBSubset).
JBBSubset * CANodeIdSet::jbbcsToJBBSubset() const
{
  JBBSubset* result = new (CmpCommon::statementHeap())
	                    JBBSubset(CmpCommon::statementHeap());
  result->setJBBCs(*this);
  return result;
}

// LCOV_EXCL_START :dpm
const NAString CANodeIdSet::getText() const
{
  NAString result("CANodeIdSet: {");
  for (CANodeId x= init();  next(x); advance(x) )
  {
    result += " " + istring(x) + " ";
  }
  result += "}\n";
  return result;
}

void CANodeIdSet::display() const
{
  print();
}

void CANodeIdSet::print (FILE *f,
	      const char * prefix,
	      const char * suffix) const
{
  fprintf (f, getText());
}
// LCOV_EXCL_STOP

// -----------------------------------------------------------------------
// Methods for class CANodeIdSetMap
// -----------------------------------------------------------------------

CANodeIdSetMap::CANodeIdSetMap(ULng32 init_size,
                               CollHeap *outHeap):
  HASHDICTIONARY(CANodeIdSet, JBBSubsetAnalysis) (&(CANodeIdSetMap::Hasher),
                                                  init_size,
                                                  TRUE, // uniqueness constraint
                                                  outHeap )
{
  hits_ = 0;
  misses_ = 0;
}

ULng32 CANodeIdSetMap::Hasher(const CANodeIdSet & key)
{
  return key.hash();
}

JBBSubsetAnalysis* CANodeIdSetMap::getFirstValue(const CANodeIdSet* key) const
{
  JBBSubsetAnalysis* result =
    HASHDICTIONARY(CANodeIdSet, JBBSubsetAnalysis)::getFirstValue(key);

  // Record hit and miss stat (may be debug code only)
  if (result)
    ((CANodeIdSetMap*)this)->hits_++; // cast to non-const
  else
    ((CANodeIdSetMap*)this)->misses_++;

  return result;
}


// -----------------------------------------------------------------------
// Methods for class QueryAnalysis
// -----------------------------------------------------------------------

// construct a QueryAnalysis
QueryAnalysis::QueryAnalysis(CollHeap *outHeap, NABoolean analysis):
  jbbSubsetMap_(107,outHeap),
  analysisON_(analysis),
  multiJoinsUsed_(FALSE),
  optimizeForFirstNRows_(FALSE),
  compressedHistsViable_(TRUE),
  heap_(outHeap),
  nodeAnalysisArray_(outHeap),
  jbbArray_(outHeap),
  allJBBCs_(outHeap),
  allGBs_(outHeap),
  origJBBTopExprs_(outHeap),
  colAnalysisArray_(outHeap),
  outputToJBBCsMap_(outHeap),
  predAnalysisArray_(outHeap),
  nodeMapArray_(outHeap),
  mvQueryRewriteHandler_(new (outHeap) MvQueryRewriteHandler(outHeap)),
  isMvCreation_(FALSE),
  skippedSomeJoins_(FALSE),
  dontSurfTheWave_(FALSE), // WaveFix
  largestTable_(NULL),
  joinedToLargestTable_(outHeap),
  compilerPhase_(PRE_BINDER),
  hasMandatoryXPComputed_(FALSE),
  queryComplexityVector_(NULL),
  highestNumOfPartns_(-1),
  statsToDisplay_(NULL)
{
  // create the table connectivity graph
  tCGraph_ = new (heap_) TableConnectivityGraph(heap_);

  // create a NULL entry for CANodeId = 0
  nodeAnalysisArray_.insertAt(0,NULL);

  // set the limit at which one JBB should reach in order to invoke MJ-Rewrite
  multiJoinThreshold_ =
    ActiveSchemaDB()->getDefaults().getAsLong(MULTI_JOIN_THRESHOLD);

  // set the property of JoinOrder By User. May get reset by hint later
  joinOrderByUser_ = (CmpCommon::getDefault(JOIN_ORDER_BY_USER) == DF_ON);

  // initialize ASM only if allowed
  if ((CmpCommon::getDefault(ASM_ALLOWED) == DF_ON) & analysisON_)
    appStatMan_ = new (heap_) AppliedStatMan(heap_);
  else
    appStatMan_ = NULL;
}

// destruct a QueryAnalysis
// LCOV_EXCL_START :dd
QueryAnalysis::~QueryAnalysis()
{
  delete tCGraph_;
  delete appStatMan_;
  delete mvQueryRewriteHandler_;
  // should loop over other collections and delete later
}
// LCOV_EXCL_STOP


// This is a temporary placement of initialization. Later it shall be
// moved to a common area where we can have all global initialization
// for the compiler
THREAD_P NABoolean CompCCAssert::useCCMPAssert_ = FALSE;

// -----------------------------------------------------------------------
// Analysis objects creation methods by QueryAnalysis
// -----------------------------------------------------------------------

// create a new NodeAnalysis for expr node and assign it new CANodeId
NodeAnalysis * QueryAnalysis::newNodeAnalysis(RelExpr* expr)
{
  CollIndex newId = nodeAnalysisArray_.entries();
  NodeAnalysis* result = new (heap_) NodeAnalysis(newId, expr, heap_);
  nodeAnalysisArray_.insertAt(newId, result);
  return result;
}

// create new JBB. The provided expr is the top node at the join
// backbone in the normalizer output. expr could be either a join
// or a group by on top of a join
JBB * QueryAnalysis::newJBB(RelExpr* expr)
{
  CollIndex newId = jbbArray_.entries();

  JBB* result = new (heap_) JBB(newId, heap_);
  result->setNormInputs(expr->getGroupAttr()->getCharacteristicInputs());
  result->setNormOutputs(expr->getGroupAttr()->getCharacteristicOutputs());

  jbbArray_.insertAt(newId, result);

  // if expr is not a join then its a GB, use its child instead
  if (expr->getOperator().match(REL_ANY_JOIN))
    origJBBTopExprs_.insert(expr);
  else
    origJBBTopExprs_.insert(expr->child(0));

  return result;
}

// create a new TableAnalysis structure for this tableExpr
// currently only Scan and TableValuedFunction can have
// TableAnalysis structure associated with them
TableAnalysis * QueryAnalysis::newTableAnalysis(RelExpr* tableExpr)
{
  // tableExpr could be of type Scan (most probably) or it could
  // be of type TableValuedFunction. I could put the TableDesc
  // extraction logic below in the pilotAnalysis implementation for
  // these RelExprs and pass the TableDesc to the constructor.
  // I decided however to do it this way in case we will need to
  // compute something else that is expression based (other than
  // TableDesc) inside TableAnalysis

  TableDesc* tableDesc = NULL;
  if (tableExpr->getOperator().match(REL_SCAN))
  {
    tableDesc = ((Scan*)tableExpr)->getTableDesc();
  } else if ((tableExpr->getOperator().match(REL_UNARY_INSERT)) ||
	  (tableExpr->getOperator().match(REL_LEAF_INSERT))) {

	tableDesc = ((Insert *)tableExpr)->getTableDesc();
  } else if ((tableExpr->getOperator().match(REL_UNARY_UPDATE)) ||
    (tableExpr->getOperator().match(REL_LEAF_UPDATE))) {

    tableDesc = ((Update *)tableExpr)->getTableDesc();
  } else if ((tableExpr->getOperator().match(REL_UNARY_DELETE)) ||
    (tableExpr->getOperator().match(REL_LEAF_DELETE))) {

    tableDesc = ((Delete *)tableExpr)->getTableDesc();
  }
  else if (tableExpr->getOperatorType() == REL_EXE_UTIL)
    {
      tableDesc = ((ExeUtilExpr*)tableExpr)->getVirtualTableDesc();
      if (tableDesc == NULL)
	return NULL;
    }
  else if ((tableExpr->getOperatorType() == REL_HBASE_ACCESS) ||
	   (tableExpr->getOperatorType() == REL_HBASE_COPROC_AGGR))
    {
      tableDesc = ((HbaseAccess*)tableExpr)->getTableDesc();
      if (tableDesc == NULL)
	return NULL;
    }
  else if (tableExpr->getOperatorType() == REL_HBASE_DELETE)
    {
      tableDesc = ((HbaseDelete*)tableExpr)->getTableDesc();
      if (tableDesc == NULL)
	return NULL;
    }
  else
  {
    tableDesc = ((TableValuedFunction*)tableExpr)->getTableDesc();
    // some TableValuedFunction returns NULL tableDesc
    if (tableDesc == NULL)
      return NULL;
  }

  const IndexDesc *tableIndexDesc = tableDesc->getClusteringIndex();
  // Record the highest no of partitions for a table among all of the tables 
  // in a query. This is used in opt.cpp to adjust max parallelism.
  if (CmpCommon::getDefault(COMP_BOOL_24) == DF_ON)
  {
    // Get the number of partitions for the table
    Lng32 tableNumOfPartns = tableIndexDesc->getNAFileSet()->getCountOfPartitions();

    // Check and save highest no of partitions
    if (highestNumOfPartns_ == -1)
      highestNumOfPartns_  = tableNumOfPartns;
    else
      if (tableNumOfPartns > highestNumOfPartns_)
        highestNumOfPartns_  = tableNumOfPartns;
  }    

  // find NodeAnalysis. create new one if NULL
  NodeAnalysis* nodeAnalysis = tableExpr->getGroupAnalysis()->getNodeAnalysis();

  if (!nodeAnalysis)
  {
    nodeAnalysis = newNodeAnalysis(tableExpr);
    tableExpr->getGroupAnalysis()->setNodeAnalysis(nodeAnalysis);
  }

  // create a new TableAnalysis and assign it to this Node
  TableAnalysis* tabAnalysis =
    new (heap_) TableAnalysis(nodeAnalysis, tableDesc, heap_);
  nodeAnalysis->setTableAnalysis(tabAnalysis);
  tableDesc->setTableAnalysis(tabAnalysis);

  // find the used cols in this table
  ValueIdSet usedCols;
  // append columns referenced in the local predicates
  ValueIdSet selectionPred = tableExpr->getSelectionPred();
  selectionPred.findAllReferencedBaseCols(usedCols);
  // append columns referenced in the scan required outputs
  tableExpr->getGroupAttr()->getCharacteristicOutputs()
    .findAllReferencedBaseCols(usedCols);
 // add table's clustering key for index joins
  ValueIdSet keyAsSet(tableIndexDesc->getIndexKey());
  keyAsSet.findAllReferencedBaseCols(usedCols) ;


  const ValueIdSet tableCols(tableDesc->getColumnList());

  if (((tableExpr->getOperator().match(REL_UNARY_UPDATE)) ||
        (tableExpr->getOperator().match(REL_LEAF_UPDATE)) ||
        (tableExpr->getOperator().match(REL_UNARY_DELETE)) ||
        (tableExpr->getOperator().match(REL_LEAF_DELETE))))
  {
    usedCols += tableCols;
  }

  // Remove columns that belong to other tables. Those arrive here because they
  // are equivalent to one of this table used columns (already in usedCols)
  usedCols.intersectSet(tableCols); // should i make tableCols a member of TA too?
  // set the usedCols_ field in TableAnalysis and append to allUsedCols_
  tabAnalysis->setUsedCols(usedCols);
  allUsedCols_ += usedCols;

  // initialize ColAnalysis objects for each usedCols (no need to do it for
  // columns that are not used)
  for (ValueId x= usedCols.init(); usedCols.next(x); usedCols.advance(x) )
  {
    ColAnalysis* colAn = new (heap_) ColAnalysis(x, tabAnalysis);
    colAnalysisArray_.insertAt(x, colAn);
  }

  // If column x in usedCols is the underlying column of a computed column, 
  // compute the column analysis for the computed column.
  ValueIdSet divColumns = tableDesc->getDivisioningColumns();

  for (ValueId x= divColumns.init(); divColumns.next(x); divColumns.advance(x) )
  {
     // Generate ColAnalysis for every DIV column, regardless of whether
     // it appears in a predicate or not. A leading DIV column that does
     // not appear in a prdicate can still be consiered for MDAM scan over
     // it. See keyless heuristics for NJ in JoinToTSJRule::topMatch() in
     // TransRule.cpp.
     ColAnalysis* colAn = new (heap_) ColAnalysis(x, tabAnalysis);
     colAnalysisArray_.insertAt(x, colAn);
  }

  // By the same argument as for the divisioning columns, include the salt column.
  ValueIdSet saltColumnSet = tableDesc->getSaltColumnAsSet();
  for (ValueId x= saltColumnSet.init(); saltColumnSet.next(x); saltColumnSet.advance(x) )
  {
     ColAnalysis* colAn = new (heap_) ColAnalysis(x, tabAnalysis);
     colAnalysisArray_.insertAt(x, colAn);
  }

  // initialize local preds. Add characteristics inputs as
  // used cols, to account for host variables. Host variables
  // come as characteristic inputs
  ValueIdSet scanCoverageExprs =
    tableExpr->getGroupAttr()->getCharacteristicInputs();
  scanCoverageExprs += usedCols;
  ValueIdSet localPreds(tableExpr->getSelectionPred());
  localPreds.removeUnCoveredExprs(scanCoverageExprs);
  tabAnalysis->setLocalPreds(localPreds);
  //local preds not final, will be finalized at finishAnalysis

  // initialize AccessPathAnalysis objects
  tabAnalysis->initAccessPaths();

  // set also the allSubtreeTables in groupAnalysis
  CANodeIdSet tables(nodeAnalysis->getId(), heap_);
  tableExpr->getGroupAnalysis()->setSubtreeTables(tables);

  // add this nodeId to the list of table nodeIds in tCGraph
  tCGraph_->tables_.insert(nodeAnalysis->getId());

  return tabAnalysis;
}

// create a new RoutineAnalysis structure for this routineExpr
// currently only scalar udfs can have RoutineAnalysis structure
// associated with them
RoutineAnalysis * QueryAnalysis::newRoutineAnalysis(RelExpr* routineExpr)
{

  RelRoutine * rExpr = (RelRoutine*) routineExpr;
  RoutineDesc * routineDesc = rExpr->getRoutineDesc();
  
  // find NodeAnalysis. create new one if NULL
  NodeAnalysis* nodeAnalysis = routineExpr->getGroupAnalysis()->getNodeAnalysis();

  if (!nodeAnalysis)
  {
    nodeAnalysis = newNodeAnalysis(routineExpr);
    routineExpr->getGroupAnalysis()->setNodeAnalysis(nodeAnalysis);
  }

  // create a new RoutineAnalysis and assign it to this Node
  RoutineAnalysis* routineAnalysis =
    new (heap_) RoutineAnalysis(nodeAnalysis, routineDesc, heap_);
  nodeAnalysis->setRoutineAnalysis(routineAnalysis);
  routineDesc->setRoutineAnalysis(routineAnalysis);
  
  return routineAnalysis;
}

void QueryAnalysis::addProducingJBBC(ValueId expr, CANodeId jbbc)
{
  CANodeIdSet * jbbcsProducingOutput = NULL;

  if (outputToJBBCsMap_.used(expr))
  {
    jbbcsProducingOutput = outputToJBBCsMap_[expr];
  }
  else{
    jbbcsProducingOutput = new (STMTHEAP) CANodeIdSet();
    outputToJBBCsMap_.insertAt(expr, jbbcsProducingOutput);
  }

  (*jbbcsProducingOutput) += jbbc;
}

// create a new JBBC for this expr (which is a join child
// of parent). Create a new NodeAnalysis if needed.
JBBC * QueryAnalysis::newJBBC(Join* parent, RelExpr* expr,
                              NABoolean isFullOuterJoinOrTSJJBBC)
{
  // find NodeAnalysis. create new one if NULL
  NodeAnalysis* nodeAnalysis = expr->getGroupAnalysis()->getNodeAnalysis();

  if (!nodeAnalysis)
  {
    nodeAnalysis = newNodeAnalysis(expr);
    expr->getGroupAnalysis()->setNodeAnalysis(nodeAnalysis);
  }

  JBBC* jbbc = new (heap_) JBBC(nodeAnalysis, parent, heap_);
  if(isFullOuterJoinOrTSJJBBC)
    jbbc->setIsFullOuterJoinOrTSJJBBC();

  if((expr->getOperator() == REL_SCAN) &&
     expr->isExtraHub())
    jbbc->isGuaranteedEqualizer_=1;

  if( parent &&
      parent->isRoutineJoin())
    jbbc->setRoutineJoinFilterPreds(parent->getSelectionPred());

  nodeAnalysis->setJBBC(jbbc);

  // add this nodeId to allJBBCs
  allJBBCs_.insert(nodeAnalysis->getId());

  if( parent )
  {
    if(parent->isInnerNonSemiNonTSJJoin())
      innerNonSemiNonTSJJBBCs_ += nodeAnalysis->getId();
    else if(parent->isRoutineJoin())
      routineJoinJBBCs_ += nodeAnalysis->getId();
    else if(parent->isLeftJoin())
      leftJoinJBBCs_ += nodeAnalysis->getId();
    else if( parent->isSemiJoin() || parent->isAntiSemiJoin())
      semiJoinJBBCs_ += nodeAnalysis->getId();
    else
      CMPASSERT(FALSE);
  }
  else
    innerNonSemiNonTSJJBBCs_ += nodeAnalysis->getId();

  ValueIdSet jbbcOutputs = expr->getGroupAttr()->getCharacteristicOutputs();

  for (ValueId jbbcOutput = jbbcOutputs.init();
                            jbbcOutputs.next(jbbcOutput);
                            jbbcOutputs.advance(jbbcOutput))
  {
    addProducingJBBC(jbbcOutput, nodeAnalysis->getId());
    CANodeIdSet * jbbcsProducingOutput = NULL;

    if (outputToJBBCsMap_.used(jbbcOutput))
    {
      jbbcsProducingOutput = outputToJBBCsMap_[jbbcOutput];
    }
    else{
// LCOV_EXCL_START :rfi
      jbbcsProducingOutput = new (STMTHEAP) CANodeIdSet();
      outputToJBBCsMap_.insertAt(jbbcOutput, jbbcsProducingOutput);
// LCOV_EXCL_STOP
    }

    (*jbbcsProducingOutput) += nodeAnalysis->getId();
  }


  return jbbc;
}

// create a new GBAnalysis
GBAnalysis * QueryAnalysis::newGBAnalysis(GroupByAgg* gb)
{
  // Note that calling gb->getGroupAnalysis()->getNodeAnalysis() would
  // give you a different NodeAnalysis object that the one we are associating
  // here with GBAnalysis. This is OK. This should be the case.
  NodeAnalysis* nodeAnalysis = newNodeAnalysis(gb);
  // This NodeAnalysis has no association with a Group. It is associated
  // with groupBy expression. This NodeAnalysis is actually a hidden one.
  // It is only there to give an extra unique id for the presence of this
  // group by in a jbbsubset.
  GBAnalysis* gbAnalysis = new (heap_) GBAnalysis(nodeAnalysis->getId(), gb, heap_);
  nodeAnalysis->setGBAnalysis(gbAnalysis);
  gb->setGBAnalysis(gbAnalysis);
  // add this nodeId to allGBs_
  allGBs_.insert(nodeAnalysis->getId());
  return gbAnalysis;
}

// Lookup a JBBSubsetAnalysis by CANodeIdSet. If none is available in cache,
// then create a new one and insert it in cache
JBBSubsetAnalysis * QueryAnalysis::findJBBSubsetAnalysis(const JBBSubset & subset)
{
  JBBSubsetAnalysis* result = jbbSubsetMap_.get(subset.getJBBCsAndGB());

  if (result)
  {
    return result;  // A hit
  }

  // A miss. Create a new JBBSubsetAnalysis and insert it
  result = new (heap_) JBBSubsetAnalysis(subset, heap_);

  CANodeIdSet * key = new (heap_) CANodeIdSet(subset.getJBBCsAndGB(), heap_);

  CANodeIdSet * insertion = jbbSubsetMap_.insert(key, result);

  // Should work
  CMPASSERT(insertion != NULL);

  return result;
}

// Find the PredAnalysis the predicate with ValueId x. If none is created
// so far, then create a new one and insert it in array
PredAnalysis* QueryAnalysis::findPredAnalysis(ValueId x)
{
  // if predAnalysis not yet created, then do so
  if (!allPreds_.contains(x))
  {
    PredAnalysis* predAn = new (CmpCommon::statementHeap()) PredAnalysis(x);
    predAnalysisArray_.insertAt(x, predAn);
    allPreds_ += x;
  }
  return predAnalysisArray_[x];
}

// -------------------------------------------------------------
// Method to initialize flags for global directives
// -------------------------------------------------------------
void QueryAnalysis::initializeGlobalDirectives(RelExpr * root)
{
  // Figure out if First N optimization is required

  RelExpr * firstNExpr = NULL;

  // The firstN info could be stamped on the root or inserted as a firstN node
  // as child of the root
  if (root->child(0)->getOperator() == REL_FIRST_N)
    firstNExpr = root->child(0);
  else if (root->getFirstNRows() >= 0)
    firstNExpr = root;

  if (firstNExpr)
  {
    // get the number of rows requested i.e. the FirstN rows
    // Note: its different methods for RelExpr and FirstN classes
    Int64 numRowsRequested;
    if (firstNExpr->getOperator() == REL_FIRST_N)
      numRowsRequested = ((FirstN*) firstNExpr)->getFirstNRows();
    else
      numRowsRequested = firstNExpr->getFirstNRows();

    RelExpr * childOfFirstN = firstNExpr->child(0);

    GroupAttributes * childOfFirstNGroupAttr = childOfFirstN->getGroupAttr();

    // get the number of rows estimated to be returned
    CostScalar numRowsProduced =
      childOfFirstNGroupAttr->getResultCardinalityForEmptyInput();

    if((numRowsRequested < 100) ||
       ((double)numRowsRequested < (0.01 * numRowsProduced.getValue())))
    {
      optimizeForFirstNRows_ = TRUE;
    }
    if (CmpCommon::getDefault(COMP_BOOL_78) != DF_ON)
    {
      optimizeForFirstNRows_ = FALSE;
    }
  }

  // WaveFix Begin
  if ((root->child(0)) &&
      (root->child(0)->getOperatorType() == REL_GROUPBY) &&
      (!((GroupByAgg *)(root->child(0).getPtr()))->groupExpr().entries()) &&
      (root->child(0)->child(0)->getOperatorType() == REL_SCAN) &&
	  (CmpCommon::getDefault(COMP_BOOL_169) == DF_OFF))
  {
    // check if table is a single partitioned table
    NABoolean isASinglePartitionedTable = FALSE;

    Scan * tableScan = (Scan *)root->child(0)->child(0).getPtr();

    TableAnalysis * tableAnalysis =
      (TableAnalysis *)tableScan->getTableDesc()->getTableAnalysis();

    const IndexDesc * tableIndexDesc =
      tableAnalysis
        ->getTableDesc()
          ->getClusteringIndex();

    const PartitioningFunction * tablePartFunc =
      tableIndexDesc->getPartitioningFunction();

    if ((!tablePartFunc)||
        (tablePartFunc->getCountOfPartitions()==1) || 
        (tablePartFunc->getPartitioningKey().
            doAllExprsEvaluateToConstant(FALSE,TRUE)))
    {
      isASinglePartitionedTable = TRUE;
    }

    // check if there are any count distincts
    NABoolean noCountDistincts = FALSE;
    GroupByAgg * groupByNode = (GroupByAgg *)(root->child(0).getPtr());

    ValueIdSet distinctColumns;              // columns (exprs) referenced
                                             // in non-redundant DISTINCT
                                             // aggregates
    CollIndex numNonMultiDistinctAggs = 0;

    const ValueIdSet &aggrs = groupByNode->aggregateExpr();
    for (ValueId x = aggrs.init(); aggrs.next(x); aggrs.advance(x))
    {
      Aggregate *agg = (Aggregate *) x.getItemExpr();

      CMPASSERT(x.getItemExpr()->isAnAggregate());

      if (agg->isDistinct())
      {
        ValueIdSet uniqueSet = groupByNode->groupExpr();
        uniqueSet += agg->getDistinctValueId();
        if (NOT groupByNode->child(0).getGroupAttr()->isUnique(uniqueSet)) {
          distinctColumns += agg->getDistinctValueId();
          if((agg->getDistinctValueId() != agg->child(0)) ||
             (agg->getArity() > 1)) {
            numNonMultiDistinctAggs++;
          }
        }
      }
    }

    if (!distinctColumns.entries())
      noCountDistincts = TRUE;

    // don't surf the wave if:
    // * the table is not a single partitioned table
    // And
    // * there are no count distincts
    // * it has no local predicates
    if ((!isASinglePartitionedTable) && 
        noCountDistincts &&
        tableAnalysis->getLocalPreds().entries() == 0)
      setDontSurfTheWave(TRUE);
  }
  // WaveFix End
}

NABoolean QueryAnalysis::fixupTriggers(RelExpr* expr)
{
  NABoolean treeFixed = FALSE;
  expr->calcAccessSets(CmpCommon::statementHeap());
  expr->fixupTriggers(treeFixed);
  expr->clearAllEstLogProp();

  if(treeFixed)
    expr->synthLogProp();

  return treeFixed;
}
// -------------------------------------------------------------
// The Main Analysis method
// Returns a pointer to the analyzed RelExpr tree, which may be
// a different tree than the one passed in as input. If noMVQR is
// TRUE (it is FALSE by default), MV query rewrite is suppressed
// regardless of the value of the MVQR_REWRITE_LEVEL CQD.
// -------------------------------------------------------------
RelExpr* QueryAnalysis::analyzeThis(RelExpr* expr, NABoolean noMVQR)
{
  NABoolean status = TRUE;
  NABoolean monitor = CURRSTMT_OPTDEFAULTS->compileTimeMonitor();
  NABoolean treeFixed = fixupTriggers(expr);
    
  // analyzedExpr is the pointer returned to the caller.
  RelExpr* analyzedExpr = expr;

  // First do pilot phase analysis
  if (monitor) pilotPhaseMon_.enter();
  status = expr->pilotAnalysis(this);
  if (monitor) pilotPhaseMon_.exit();

  // stop if incompatible node found
  if (!status || !analysisON_)
  {
    // clear analysis
    cleanup(expr);

    // setInitialMemory for optimizer here
    // doing this here because in cases
    // when we don't come here this is
    // called below, before the call to
    // RelExpr::analysizeInitialPlan
    // This ensures that we setInitialMemory
    // when analysis is ON of OFF.
    // This is the OFF case.
    CURRSTMT_OPTDEFAULTS->setInitialMemory();

    return analyzedExpr;
  }

  if (monitor) jbbSetupMon_.enter();
  expr->jbbAnalysis(this);
  primeGroupAnalysisForSubtree(expr);

  // analyze dependencies between JBBCs
  analyzeJBBCDependencies(expr);

  if (monitor) jbbSetupMon_.exit();

  // TCG work here
  if (monitor) tcgSetupMon_.enter();

  expr->predAnalysis(this);

  finishConnectivityAnalysis();
  
  checkIfCompressedHistsViable();
  
  if (monitor) tcgSetupMon_.exit();

  // finishSynthEstLogProp has nothing to do with Query Analysis, but it
  // needs to be done before multi-join transformation

  expr->finishSynthEstLogProp();

  ULng32 sizeOfLargestJBB = getSizeOfLargestJBB();

#ifndef NDEBUG
  RelExpr * analyzerInput = expr;
#endif

  // Do the multi join re-write now
  if ((sizeOfLargestJBB >= multiJoinThreshold_) AND
      !joinOrderByUser())
  {
#ifndef NDEBUG
    // get a copy of the RelExpr tree before it is converted to
    // multiJoin. This is needed below for the MultiJoinTester.
    // Want to invoke the MultiJoinTester after setting JBBInput.
    if (sizeOfLargestJBB && (CmpCommon::getDefault(COMP_BOOL_8) == DF_ON))
    {
      analyzerInput = expr->copyRelExprTree(CmpCommon::statementHeap());
    }
#endif

    expr->convertToMultiJoinSubtree(this);

    double n, n2, n3, n4;
    n= n2= n3= n4= 0;

    double queryMJComplexity =
      expr->calculateQueryMJComplexity(n,n2,n3,n4);
   
    // complexity should be atleast 1
    if (queryMJComplexity < 1)
      queryMJComplexity=n=n2=n3=n4=1;

    queryComplexityVector_ = new(CmpCommon::statementHeap())
      QueryComplexityVector(CmpCommon::statementHeap());
      
    queryComplexityVector_->setNComplexity(n);
    queryComplexityVector_->setN2Complexity(n2);
    queryComplexityVector_->setN3Complexity(n3);
    queryComplexityVector_->setN4Complexity(n4);
    queryComplexityVector_->setExhaustiveComplexity(queryMJComplexity);

    CURRSTMT_OPTDEFAULTS->setQueryMJComplexity(queryMJComplexity);

    expr->clearLogExprForSynthDuringAnalysis();
    expr->synthLogProp();

    multiJoinsUsed_ = TRUE;
  }

  expr->setJBBInput((*GLOBAL_EMPTY_INPUT_LOGPROP));

  if ((expr->getOperatorType() == REL_ROOT) AND
      (CmpCommon::context()->showQueryStats()))
    setHistogramsToDisplay((RelRoot*) expr);

#ifndef NDEBUG
  // TESTING
  if (sizeOfLargestJBB && (CmpCommon::getDefault(COMP_BOOL_8) == DF_ON))
  {
    MultiJoinTester::Test1(analyzerInput, expr);
    expr->clearLogExprForSynthDuringAnalysis();
    expr->synthLogProp();
    expr->setJBBInput((*GLOBAL_EMPTY_INPUT_LOGPROP));
  }
#endif

  if (expr->getOperatorType() == REL_ROOT)
  {
    ((RelRoot *)expr)->setHasMandatoryXP(hasMandatoryXP());
  }

  // ASM and QG work
  // This should also be disabled if cqsRewrite succeed
  if (appStatMan_ && CmpCommon::getDefault(ASM_PRECOMPUTE) == DF_ON)
  {
    if (monitor) asmPrecompMon_.enter();
    initializeASM();
    if (monitor) asmPrecompMon_.exit();
  }

  // Compute basic stats for the nodes
  computeStatsForNodes();

  // Compute the set of tables joined to the largest table
  if (CmpCommon::getDefault(COMP_BOOL_119) == DF_ON)
    computeTablesJoinedToLargestTable();

  // Now we do the CQS Fixup if applicable
  if (expr->getOperatorType() == REL_ROOT)
  {
     cqsRewrite(expr);
  }

  // Initialize the global directive flags
  // Note:
  //  this should be after synthLogProp since
  //  some flags will be set based on the statistics
  //  computations that are done in synthLogProp
  initializeGlobalDirectives(expr);

  // setInitialMemory for optimizer here
  // doing this because some LSR work is
  // done in analyzeInitialPlan
  CURRSTMT_OPTDEFAULTS->setInitialMemory();

  // this should be the last active call
  // i.e. last call that does anything to
  // the expression
  if (CmpCommon::getDefault(COMP_BOOL_115) == DF_OFF)
    expr->analyzeInitialPlan();

  // display (or dump to file) JBB connectivity graph if requested
  graphDisplay();

  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << getText() << endl;
  }

#define MVQR
#ifdef MVQR
if (!noMVQR && CmpCommon::getDefaultLong(MVQR_REWRITE_LEVEL) > 0)
  analyzedExpr = handleMvQueryRewrite(analyzedExpr);

  // if dontSurfTheWave() is set previously and there are any MVQR matches
  // then unset dont suf the wave as we do not want the privority of the
  // SortGrby plan to be increased that avoids checking the costing
  // between SortGrby plan and the matching MV plan.
  if ((dontSurfTheWave()) &&
      (expr->child(0)) &&
      (expr->child(0)->getOperatorType() == REL_GROUPBY) &&
      (((GroupByAgg *)(expr->child(0).getPtr()))->getJBBSubsetAnalysis() &&
       ((GroupByAgg *)(expr->child(0).getPtr()))->getJBBSubsetAnalysis()
             ->getMatchingMVs().entries()))
    setDontSurfTheWave(FALSE);
#endif

  return analyzedExpr;
}

void QueryAnalysis::setHistogramsToDisplay(RelRoot * root)
{
  CANodeIdSet nodeSetForHistEst;
  const char * nodeSetStr = ActiveSchemaDB()->getDefaults().getValue(HIST_ROOT_NODE);
  size_t nodeSetStrLen = strlen(nodeSetStr);
  NABoolean validNodeSet = TRUE;
  
  if (nodeSetStr && nodeSetStrLen)
  {
    char * nodeSetStrForTokenization = NULL;
    validNodeSet = FALSE;
    
    if (nodeSetStr)
    {
      if (nodeSetStrLen)
      {
        nodeSetStrForTokenization = new (CmpCommon::statementHeap()) char[nodeSetStrLen+1];
        strcpy(nodeSetStrForTokenization, nodeSetStr);
        char * token;
        token = strtok (nodeSetStrForTokenization," ,");

        while (token != NULL)
        {
          nodeSetForHistEst.insert(atoi(token));
          token = strtok (NULL, " ,");
        }
      }      
    }

    if(nodeSetForHistEst.entries())
    {
      if(nodeSetForHistEst.entries() == 1)
      {
        // can be:
        // 0. any jbbc
        // 1. scan
        // 2. group by
        // 3. full outer join
        // 4. tsj
        CANodeId node = nodeSetForHistEst.getFirst();
        if(node.getNodeAnalysis())
        {
          RelExpr * nodeExpr = node.getNodeAnalysis()->getModifiedExpr();
          if(node.getNodeAnalysis()->getJBBC() ||
             node.getNodeAnalysis()->getTableAnalysis() ||
             node.getNodeAnalysis()->getGBAnalysis() ||
             (nodeExpr->isAnyJoin() &&
               (((Join*) nodeExpr)->isFullOuterJoin() ||
                ((Join*) nodeExpr)->isTSJ())))
             validNodeSet = TRUE;
        }
      }
      else{
        validNodeSet = TRUE;
        
        // has to be a join
        // check if all nodes are jbbcs in the same jbb
        CollIndex jbbId = -1;
        for(CANodeId node = nodeSetForHistEst.init();
            nodeSetForHistEst.next(node);
            nodeSetForHistEst.advance(node))
        {
          // each node should be jbbc
          if (!node.getNodeAnalysis() || !node.getNodeAnalysis()->getJBBC())
          {
            validNodeSet = FALSE;
            break;
          }
          
          // all jbbcs should belong to the same JBB
          if((jbbId != -1) && (node.getNodeAnalysis()->getJBBC()->getJBB()->getJBBId()!=jbbId))
          {
            validNodeSet = FALSE;
            break;
          }

          jbbId = node.getNodeAnalysis()->getJBBC()->getJBB()->getJBBId();
        }
      
        if(validNodeSet)
          validNodeSet = nodeSetForHistEst.legal();
      }
    }    
  }
  
  if (validNodeSet)
  {
    EstLogPropSharedPtr inputLP = (*GLOBAL_EMPTY_INPUT_LOGPROP);
    
    if (nodeSetForHistEst.entries() == 0)
    {
      statsToDisplay_ = root->child(0)->getGroupAttr()->outputLogProp(inputLP);
    }
    else if (nodeSetForHistEst.entries() == 1)
    {
       inputLP = nodeSetForHistEst.getFirst().getNodeAnalysis()->getJBBInput();
       if(inputLP.get() == NULL) inputLP = (*GLOBAL_EMPTY_INPUT_LOGPROP);
       statsToDisplay_ =
        nodeSetForHistEst.getFirst().getNodeAnalysis()->getModifiedExpr()->getGroupAttr()->outputLogProp(inputLP);
    }
    else{        
      JBBSubset * jbbSubset = nodeSetForHistEst.jbbcsToJBBSubset();
      inputLP = jbbSubset->getJBB()->getInputEstLP();
      //create and prime multijoin
      //use multijoin to get EstLogProp
      MultiJoin* mj = new (CmpCommon::statementHeap())
        MultiJoin((*jbbSubset), CmpCommon::statementHeap());
      // Set the children based on the Query Analysis jbbcExprMap_
      mj->setChildrenFromOrigExprs(this);
      mj->setGroupAttr(new (CmpCommon::statementHeap()) GroupAttributes());
      mj->synthLogProp();
      statsToDisplay_ = mj->getGroupAttr()->outputLogProp(inputLP);
    }
    selectListCols_ = root->compExpr();
  }
}

// method to show a query's histograms
void QueryAnalysis::showQueryStats(const char *qText, CollHeap *c, char *buf)
{
  if (statsToDisplay_)
  {
    sprintf(buf, "Histograms for query %s", qText);
    statsToDisplay_->getColStats().showQueryStats(c, buf, selectListCols_);
  }
}

//----------------------------------------------------------------------------
RelExpr* QueryAnalysis::handleMvQueryRewrite(RelExpr* expr)
{
  NAString warningMessage = "";

  // We don't handle olap or sequence functions yet,
  if (expr->containsNode(REL_SEQUENCE))
  {
    warningMessage = "SEQUENCE is not supported.";
  }
  else if (isMvCreation())
  {
    // We are in a CREATE MV operation.
    // Need to create an MV descriptor, 
    mvQueryRewriteHandler_->createMvDescriptor(this, expr, warningMessage);
  }
  else
  {
    // Don't attempt rewrite of DDL statements, and don't warn about it.
    if (expr->getRETDesc()->getBindWA()->inDDL())
      return expr;

    // Do the work here.
    expr = mvQueryRewriteHandler_->handleMvQueryRewrite(this, expr, warningMessage);
  }

  // Anything to warn about?
  if (warningMessage != "")
  {
    QRLogger::log(CAT_SQL_COMP_QR_HANDLER, LL_WARN, warningMessage);
    mvQueryRewriteHandler_->getWarningMessage() = warningMessage;
  }

  return expr;
}  // handleMvQueryRewrite()

//----------------------------------------------------------------------------
void QueryAnalysis::analyzeJBBCDependencies(RelExpr* expr)
{
  // set the dependencies between the children of JBB
  ValueIdSet predsWithDependencies;

  // This will set predicates with dependencies
  expr->jbbJoinDependencyAnalysis(predsWithDependencies);

  // The loop below will set the dependent
  // JBBCs based on the predicates set above
  const ARRAY(JBB*) jBBs = getJBBs();
  for(UInt32 i = 0; i < jBBs.entries(); i++)
    jBBs[i]->analyzeChildrenDependencies();

  for(UInt32 i = 0; i < jBBs.entries(); i++)
    jBBs[i]->computeLeftJoinFilterPreds();
    
  //compute children subgraph dependencies
  for(UInt32 i = 0; i < jBBs.entries(); i++)
    jBBs[i]->computeChildrenSubGraphDependencies();
  
}

NABoolean QueryAnalysis::hasMandatoryXP()
{
// LCOV_EXCL_START :rfi
  if(hasMandatoryXPComputed_)
    return hasMandatoryXP_;
// LCOV_EXCL_STOP

  hasMandatoryXP_ = FALSE;

  const ARRAY(JBB*) jBBs = getJBBs();
  for(UInt32 i = 0; i < jBBs.entries(); i++)
    if(jBBs[i]->hasMandatoryXP())
      hasMandatoryXP_ = TRUE;

  hasMandatoryXPComputed_ = TRUE;

  return hasMandatoryXP_;
}

void QueryAnalysis::computeTablesJoinedToLargestTable()
{
  if(!largestTable_)
    return;

  ValueIdList largestTableCK =
    largestTable_
      ->getTableDesc()
        ->getClusteringIndex()
          ->getIndexKey();

  if (!largestTableCK.entries())
    return;

  // get a handle to the key prefix column
  ValueId keyColumn = largestTableCK[0];

  //get a handle to the column analysis for this column
  ColAnalysis * keyColAnalysis = keyColumn.colAnalysis();

  if (!keyColAnalysis)
    return;

  CANodeIdSet tablesConnectedViaKeyCol =
    keyColAnalysis->getConnectedTables();

  if (!tablesConnectedViaKeyCol.entries())
    return;

  CANodeId mostReducedTable;
  CostScalar highestRedFactor = -1;

  for(CANodeId currentTable = tablesConnectedViaKeyCol.init();
      tablesConnectedViaKeyCol.next(currentTable);
      tablesConnectedViaKeyCol.advance(currentTable))
  {
    if(!currentTable.getNodeAnalysis()->getTableAnalysis())
      continue;

    if(mostReducedTable==NULL_CA_ID)
    {
      mostReducedTable = currentTable;
      highestRedFactor =
        (mostReducedTable.
          getNodeAnalysis()->
            getTableAnalysis()->
              getCardinalityOfBaseTable())/
        (mostReducedTable.
          getNodeAnalysis()->
            getCardinality());
      continue;
    }

    CostScalar currentTableRedFactor =
      (currentTable.
        getNodeAnalysis()->
          getTableAnalysis()->
            getCardinalityOfBaseTable())/
      (currentTable.
        getNodeAnalysis()->
          getCardinality());

    if (currentTableRedFactor > highestRedFactor)
    {
      highestRedFactor = currentTableRedFactor;
      mostReducedTable = currentTable;
    }
  }

  if(mostReducedTable != NULL_CA_ID)
    joinedToLargestTable_.insert(mostReducedTable);

}

void QueryAnalysis::cleanup(RelExpr* expr)
{
  Lng32 sizeOfLargestJBB = getSizeOfLargestJBB();
  // set Analysis flag OFF
  analysisON_ = FALSE;
  // set MultiJoin Rewrite flag OFF
  multiJoinsUsed_ = FALSE;
  // disable ASM by setting appStatMan to NULL
  appStatMan_ = NULL;
  // clear analysis data stored in GroupAnalysis of tree's exressions
  clearAnalysis(expr);
  // do finishAnalysis before exiting Query Analysis phase.
  expr->finishSynthEstLogProp();
  return;
}

void QueryAnalysis::cqsRewrite(RelExpr* expr)
{
  RelRoot *root = (RelRoot *) expr;
  CURRENTSTMT->clearCqsWA();
  if (multiJoinsUsed_ AND // cqsRewrite expect multiJoins
      root->getReqdShape() AND
      NOT root->getReqdShape()->isCutOp())
  {
     RelExpr * cqsExpr = root->getReqdShape();
     root->forceCQS(cqsExpr);
     root->synthLogProp();
  }
  return;
}

void QueryAnalysis::clearAnalysis(RelExpr* expr)
{
  // do self
  expr->getGroupAttr()->clearGroupAnalysis();
  // do children
  Int32 arity = expr->getArity();
  for (Lng32 i = 0; i < arity; i++)
  {
    clearAnalysis(expr->child(i).getPtr());
  }

  return;
}

void QueryAnalysis::primeGroupAnalysisForSubtree(RelExpr* expr)
{
  // do children first
  Int32 arity = expr->getArity();
  for (Lng32 i = 0; i < arity; i++)
  {
    primeGroupAnalysisForSubtree(expr->child(i).getPtr());
  }
  // do self now
  expr->primeGroupAnalysis();
  return;
}

void QueryAnalysis::finishConnectivityAnalysis()
{
  CANodeIdSet tables = tCGraph_->tables_;
  CANodeId table;
  for (table = tables.init();
       tables.next(table);
       tables.advance(table))
  {
    table.getNodeAnalysis()->getTableAnalysis()->finishAnalysis();
  }
  return;
}

void QueryAnalysis::checkIfCompressedHistsViable()
{
  CANodeIdSet tables = tCGraph_->tables_;
  CANodeId table;
  for (table = tables.init();
       tables.next(table);
       tables.advance(table))
  {
    table.getNodeAnalysis()->getTableAnalysis()->checkIfCompressedHistsViable();
  }
  return;
}

// Find JBB with most JBBCs
JBB* QueryAnalysis::getLargestJBB()
{
  JBB* largestJBB = NULL;
  CollIndex sizeOfLargestJBB = 0;

  CollIndex i = 0;
  CollIndex remainingJBBs = jbbArray_.entries();
  for (i = 0; remainingJBBs > 0; i++)
  {
    if (jbbArray_.used(i))
    {
      if (sizeOfLargestJBB < jbbArray_[i]->getJBBCs().entries())
      {
        largestJBB = jbbArray_[i];
        sizeOfLargestJBB = largestJBB->getJBBCs().entries();
      }
      remainingJBBs--;
    }
  }
  return largestJBB;
}

ULng32 QueryAnalysis::getSizeOfLargestJBB()
{
  JBB* largestJBB = getLargestJBB();
  if (largestJBB)
    return largestJBB->getJBBCs().entries();
  else
    return 0;
}

// This method is called only after ASM initialization
// It computes some basic stats for nodes and tables and caches them.
void QueryAnalysis::computeStatsForNodes()
{
  CollIndex i = 0;
  CollIndex remainingNodes = nodeAnalysisArray_.entries();
  for (i = 0; remainingNodes > 0; i++)
  {
    remainingNodes--;
    CMPASSERT(nodeAnalysisArray_.used(i));
    if (nodeAnalysisArray_.used(i) && i != NULL_CA_ID) // NULL_CA_ID is zero
    {
      nodeAnalysisArray_[i]->computeStats();
    }
  }
}

// For the graph display by the dotty tool
void QueryAnalysis::graphDisplay() const
{
  // See if there is something to display first
  if(jbbArray_.entries() == 0)
    return;

  NAString fileName = "";

  const char* fname = ActiveSchemaDB()->getDefaults().
                        getValue(DISPLAY_DATA_FLOW_GRAPH);

  if(!(*fname) || strcasecmp(fname,"OFF") == 0)
  {
    // Display graph is OFF. bye bye
    return;
  }
// LCOV_EXCL_START :dpm
  if(*fname && strcasecmp(fname,"ON") == 0)
  {
    // The default file name is used here
    fileName = "C:\\ATT\\Graphviz\\graphs\\f.dot";
  }
  else
  {
    // Use name specified in CQD
    fileName = fname;
  }

  NAString result = "graph G {\n";

  ofstream fileout(fileName, ios::out);

  result += "fontsize=20 label=\"\\nQuery Connectivity Analysis\"\n";
  result += "edge [len=2 color=blue fontcolor=blue];\n";

  CollIndex i = 0;
  CollIndex remainingJBBs = jbbArray_.entries();
  for (i = 0; remainingJBBs > 0; i++)
  {
    if (jbbArray_.used(i))
    {
      result += jbbArray_[i]->graphDisplay(this) + "\n";
	  remainingJBBs--;
    }
  }
  result += "}\n";
  fileout<<result;

// LCOV_EXCL_STOP
  return;
}

// LCOV_EXCL_START :dpm
const NAString QueryAnalysis::getText() const
{
  NAString result(heap_);
  result += "-----------------------------------------------------\n";
  result += "QueryAnalysis:\n\n";

  CollIndex i = 0;
  result += "Have "+ istring(nodeAnalysisArray_.entries()-1) + " CA Nodes:\n\n";
  CollIndex remainingNodes = nodeAnalysisArray_.entries();
  for (i = 0; remainingNodes > 0; i++)
  {
    if (nodeAnalysisArray_.used(i))
    {
      if (i != NULL_CA_ID) // which is zero
        result += nodeAnalysisArray_[i]->getText() + "\n";
	  remainingNodes--;
    }
  }
  result += "Have "+ istring(allJBBCs_.entries()) + " JBBCs:\n";
  result += allJBBCs_.getText()+"\n";
  result += "Have "+ istring(jbbArray_.entries()) + " JBBs:\n";
  CollIndex remainingJBBs = jbbArray_.entries();
  for (i = 0; remainingJBBs > 0; i++)
  {
    if (jbbArray_.used(i))
    {
      result += jbbArray_[i]->getText() + "\n";
	  remainingJBBs--;
    }
  }
  result += "\n";
  result += "-----------------------------------------------------\n";
  return result;
}

void QueryAnalysis::display() const
{
  print();
}

void QueryAnalysis::print (FILE *f,
	      const char * prefix,
	      const char * suffix) const
{
  fprintf (f, getText());
}
// LCOV_EXCL_STOP

// -----------------------------------------------------------------------
// pilotAnalysis() and jbbAnalysis() and other RelExpr methods
// -----------------------------------------------------------------------

NABoolean RelExpr::pilotAnalysis(QueryAnalysis* qa)
{
  NABoolean status = TRUE;
  // do pilot analysis on this expr

  // xxx make this status=FALSE rather than return FALSE

  // Compound Statements are not supported now. xxx
  // This can be added, but no big gain expected since CS
  // statements are small usually.
  if (getOperator().match(REL_COMPOUND_STMT))
  {
    return FALSE;
  }

  // Streams causes problems now with the MultiJoin becuase
  // they requires TSJ and the MultiJoin rules do not respect
  // this requirement for now. This can be fixed.
  if (getGroupAttr()->isStream())
  {
    return FALSE;
  }

  // now pass it to children
  Int32 arity = getArity();

  for (Lng32 i = 0; i < arity; i++)
    status = status AND child(i)->pilotAnalysis(qa);

  return status;
}

NABoolean GroupByAgg::pilotAnalysis(QueryAnalysis* qa)
{
  // First we do the normal RelExpr pilotAnalysis work
  NABoolean status = RelExpr::pilotAnalysis(qa);
  // stop here if it fails
  if (!status)
    return FALSE;

  // Now we do special GB work
  RelExpr* child = this->child(0);
  NABoolean joinThatCanBePartOfJBB =
      (child->getOperator().match(REL_ANY_JOIN) && ((Join*)child)->canBePartOfJBB());

  NABoolean singleScanThatCanBePartOfJBB =
      ((child->getOperator().match(REL_SCAN)) && (CmpCommon::getDefaultLong(MVQR_REWRITE_LEVEL) > 0) &&
       !(child->getGroupAttr()->isUnique(groupExpr()))); // group by can be eliminated

  if (!joinThatCanBePartOfJBB && !singleScanThatCanBePartOfJBB)
  {
    // This is not on top of JBB. No more work needed.
	return status;
  }

  // Now that this GB is on top of a JBB, we need to create a GBAnalysis
  // and assigh a special CANodeId for it.

  if (singleScanThatCanBePartOfJBB)
  {
     qa->newJBBC(NULL, child);
  }

  qa->newGBAnalysis(this);

  return status;
}

NABoolean Scan::pilotAnalysis(QueryAnalysis* qa)
{
  // Describe operator is a problem because it is derived from
  // Scan but it has junk TableDesc. This check will be put later
  // in Describe::pilotAnalysis(), but this is a fast kludge xxx.
  if (getOperator().match(REL_DESCRIBE))
  {
    return FALSE;
  }

  if (!qa->newTableAnalysis(this))
    return FALSE; // LCOV_EXCL_LINE :rfi

  return TRUE;
}


NABoolean TableValuedFunction::pilotAnalysis(QueryAnalysis* qa)
{
  if (!qa->newTableAnalysis(this))
    return FALSE;

  return TRUE;
}

NABoolean RelRoutine::pilotAnalysis(QueryAnalysis* qa)
{
  NABoolean status = RelExpr::pilotAnalysis(qa);
  
  if(!status)
    return FALSE;
    
  if (!qa->newRoutineAnalysis(this))
    return FALSE;

  return TRUE;
}

NABoolean Join::pilotAnalysis(QueryAnalysis* qa)
{
  NABoolean status = TRUE;

  // set status to FALSE if this
  // join cannot be part of JBB
  if (isASpoilerJoin())
    status = FALSE;

  if ( qa->isCompressedHistsViable() &&
       (isTSJ() || isSemiJoin() || isOuterJoin() ) )
    qa->disableCompressedHistsViable();

  if(canBePartOfJBB())
  {
    // the right child is always a JBBC
    qa->newJBBC(this, child(1).getPtr());

    // the left child is a JBBC only if it is:
    // * not one of the following joins
    //   * inner
    //   * left join
    //   * semi
    //   * anti-semi
    // * full outer join becomes a JBBC
    // * TSJ becomes a JBBC but by default it is a spoiler
    if ((!child(0)->getOperator().match(REL_ANY_JOIN))||
        (!((Join *)child(0).getPtr())->canBePartOfJBB()))
    {
      NABoolean isFullOuterJoinOrTSJJBBC = FALSE;
      if (child(0)->getOperator().match(REL_ANY_JOIN))
        isFullOuterJoinOrTSJJBBC = TRUE;
      qa->newJBBC(NULL, child(0).getPtr(),isFullOuterJoinOrTSJJBBC);
    }
  }
  else
    qa->setSkippedSomeJoins();

  // now pass call to children
  for (Lng32 i = 0; i < 2; i++)
    status = status AND child(i)->pilotAnalysis(qa);

  return status;
}

NABoolean Insert::pilotAnalysis(QueryAnalysis* qa)
{

  NABoolean status = TRUE;

  // Ensure the column analysis is properly set up
  // for an embedded insert that contains predicates.
  if (getGroupAttr()->isEmbeddedInsert() || (getGroupAttr()->getCharacteristicOutputs().entries() > 0))
    {
      if (!qa->newTableAnalysis(this))
	return FALSE;
    }

  // now pass it to children
  Int32 arity = getArity();

  for (Lng32 i = 0; i < arity; i++)
    status = status AND child(i)->pilotAnalysis(qa);

  return status;
}

// this method will be used for update and delete (both are derived from
// generic update. Insert (also derived from generic update) has its own
// pilotAnalysis
NABoolean GenericUpdate::pilotAnalysis(QueryAnalysis* qa)
{

  NABoolean status = TRUE;

  // Update operator is a problem for TCG because it generates
  // multiple ValueIds for the same BaseColumn. We will delay
  // handling this spoiler for now. This check will be later put
  // in Update::pilotAnalysis(), but this is a fast kludge xxx.
  // Future fix for this whole Update issue is either to change
  // type of these duplicate base columns to something else. Or
  // to rely on base column list in analyzer to test.
  // Same thing for Delete operator.
  if (((CmpCommon::getDefault(COMP_BOOL_217) == DF_ON) &&
       ((getOperator().match(REL_UNARY_UPDATE)) OR
        (getOperator().match(REL_UNARY_DELETE)))) OR
      (getGroupAttr()->isEmbeddedUpdateOrDelete()))
  {
    return FALSE;
  }

  // Ensure the column analysis is properly set up
  if (((getOperator().match(REL_UNARY_UPDATE)) OR
       (getOperator().match(REL_UNARY_DELETE)) OR
       (getGroupAttr()->getCharacteristicOutputs().entries() > 0))
       &&
      (!qa->newTableAnalysis(this)))
    return FALSE;

  // now pass it to children
  Int32 arity = getArity();

  for (Lng32 i = 0; i < arity; i++)
    status = status AND child(i)->pilotAnalysis(qa);

  return status;
}

void RelExpr::jbbAnalysis(QueryAnalysis* qa)
{
  // recursively call the children
  Int32 arity = getArity();
  for (Lng32 i = 0; i < arity; i++)
  {
    child(i)->jbbAnalysis(qa);
  }
  return;
}

void GroupByAgg::jbbAnalysis(QueryAnalysis* qa)
{
  RelExpr* child = this->child(0);

  NABoolean joinThatCanBePartOfJBB =
      (child->getOperator().match(REL_ANY_JOIN) && ((Join*)child)->canBePartOfJBB());

  NABoolean singleScanThatCanBePartOfJBB =
      ((child->getOperator().match(REL_SCAN)) && (CmpCommon::getDefaultLong(MVQR_REWRITE_LEVEL) > 0) &&
       !(child->getGroupAttr()->isUnique(groupExpr()))); // group by can be eliminated

  if (!joinThatCanBePartOfJBB && !singleScanThatCanBePartOfJBB)
  {
    // not on top of JBB. do it like other RelExprs.
    RelExpr::jbbAnalysis(qa);
	return;
  }

  // This GB is on top of a JBB. create a JBB object.
  JBB* jbb = qa->newJBB(this);

  // analyse the JBB and children
  jbb->analyze(this);

  // recursively pass call to children exprs of the entire JBB
  RelExpr* expr = child;
  while (expr->getOperator().match(REL_ANY_JOIN) &&
         ((Join*)expr)->canBePartOfJBB())
  {
    expr->child(1)->jbbAnalysis(qa);
    expr = expr->child(0);
  }
  // left most JBBC
  expr->jbbAnalysis(qa);

  // Can do other GB Analysis stuff here like R1 and R2 computations.

  return;
}

void Join::jbbAnalysis(QueryAnalysis* qa)
{
  if(!canBePartOfJBB())
  {
    RelExpr::jbbAnalysis(qa);
    return;
  }

  // this is the top join in a JBB. create a JBB object for it
  JBB* jbb = qa->newJBB(this);

  // analyse the JBB and children
  jbb->analyze(this);

  // recursively pass call to children exprs of the entire JBB
  RelExpr* expr = this;
  while (expr->getOperator().match(REL_ANY_JOIN)&&
         ((Join*)expr)->canBePartOfJBB())
  {
    expr->child(1)->jbbAnalysis(qa);
    expr = expr->child(0);
  }
  // left most JBBC
  expr->jbbAnalysis(qa);

  return;
}

void RelExpr::jbbJoinDependencyAnalysis(ValueIdSet & predsWithDependencies)
{
  // call dependency analysis on children
  Int32 arity = getArity();

  for(Lng32 i = 0; i < arity; i++)
    child(i)->jbbJoinDependencyAnalysis(predsWithDependencies);
}

void Join::jbbJoinDependencyAnalysis(ValueIdSet & predsWithDependencies)
{
  if(canBePartOfJBB())
  {
    predsWithDependencies += getJoinPred();
  }

  RelExpr::jbbJoinDependencyAnalysis(predsWithDependencies);

  if(canBePartOfJBB())
  {
    ValueIdSet emptySet;

    JBBC * rightChildJBBC =
      child(1)->
        getGroupAnalysis()->
          getNodeAnalysis()->
            getJBBC();

    if(rightChildJBBC)
	  {
      if(!isInnerNonSemiNonTSJJoin())
        rightChildJBBC->setPredsWithDependencies(predsWithDependencies,
                                                 getJoinPred());
	    else
        rightChildJBBC->setPredsWithDependencies(predsWithDependencies,
                                                 emptySet);
	  }

    NodeAnalysis * leftChildNodeAnalysis =
      child(0)->
        getGroupAnalysis()->
          getNodeAnalysis();

    JBBC * leftChildJBBC = NULL;

    if (leftChildNodeAnalysis)
      leftChildJBBC =
        leftChildNodeAnalysis->getJBBC();

    if(leftChildJBBC){
      leftChildJBBC->setPredsWithDependencies(predsWithDependencies, emptySet);
    }
  }
}

void RelExpr::predAnalysis(QueryAnalysis* qa)
{
  // do pred analysis on this expr
  ValueIdSet exprSet = getSelectionPred();

  exprSet.columnAnalysis(qa);

  // now pass it to children
  Int32 arity = getArity();

  for (Lng32 i = 0; i < arity; i++)
    child(i)->predAnalysis(qa);

  return;
}


void Join::predAnalysis(QueryAnalysis* qa)
{
  // do pred analysis on this expr
  ValueIdSet exprSet = getSelectionPred();
  exprSet += joinPred();

  NABoolean predsOnSemiOrLeftJoin =
    !isInnerNonSemiJoin();
    
  exprSet.columnAnalysis(qa, predsOnSemiOrLeftJoin);

  // now pass it to children
  Int32 arity = getArity();

  for (Lng32 i = 0; i < arity; i++)
    child(i)->predAnalysis(qa);

  return;
}

// returns TRUE if join is a spoiler
NABoolean Join::isASpoilerJoin()
{
  if(isRoutineJoin() && (CmpCommon::getDefault(ROUTINE_JOINS_SPOIL_JBB) == DF_ON))
    return TRUE;
    
  // TSJ and right outer join are not allowed in the MultiJoin framework
  // Note isRightJoin returns TRUE for full outer joins also, hence
  // the second OR condition
  if(((isTSJ() && !isRoutineJoin()) && (CmpCommon::getDefault(TSJS_SPOIL_JBB) == DF_ON))||
     (!isFullOuterJoin() && isRightJoin()))
    return TRUE;

  // control if left joins are allowed allowed in MultiJoin framework
  if(isFullOuterJoin() &&
     (CmpCommon::getDefault(FULL_OUTER_JOINS_SPOIL_JBB) == DF_ON))
    return TRUE;

  // control if left joins are allowed allowed in MultiJoin framework
  if(isLeftJoin() &&
     (CmpCommon::getDefault(LEFT_JOINS_SPOIL_JBB) == DF_ON))
    return TRUE;

  // control if semi and anti-semijoins are allowed in MultiJoin framework
  if((isSemiJoin() || isAntiSemiJoin()) &&
     (CmpCommon::getDefault(SEMI_JOINS_SPOIL_JBB) == DF_ON))
    return TRUE;

  return FALSE;
}

// returns TRUE if join can be part of the JBB
NABoolean Join::canBePartOfJBB()
{
  // Non-RoutineJoin-TSJ and right-outer-join and full-outer-join cannot be part of JBB
  if((isTSJ() && !isRoutineJoin()) || isRightJoin() || isFullOuterJoin())
    return FALSE;

  return TRUE;
}

RelExpr* RelExpr::convertToMultiJoinSubtree(QueryAnalysis* qa)
{
  // pass it to children
  Int32 arity = getArity();

  for (Lng32 i = 0; i < arity; i++)
    child(i) = child(i)->convertToMultiJoinSubtree(qa);

  if (getGroupAnalysis()->getNodeAnalysis())
  getGroupAnalysis()->getNodeAnalysis()->setModifiedExpr(this);

  return this;
}

RelExpr* Join::convertToMultiJoinSubtree(QueryAnalysis* qa)
{
  if(!canBePartOfJBB())
  {
    return RelExpr::convertToMultiJoinSubtree(qa);
  }

  if (!getGroupAnalysis()->getLocalJBBView())
  {
    // in this case just use default RelExpr implementation
	return RelExpr::convertToMultiJoinSubtree(qa);
  }

  CollHeap* outHeap = qa->outHeap();

  MultiJoin* result = new (outHeap)
    MultiJoin(*(getGroupAnalysis()->getLocalJBBView()), outHeap);

  // Share my groupAttr with result
  result->setGroupAttr(getGroupAttr());

  // Set the children based on the Query Analysis jbbcExprMap_
  result->setChildrenFromOrigExprs(qa);

  // This call will pass the request to the MultiJoin children
  result->convertToMultiJoinSubtree(qa);

  return result;
}

EstLogPropSharedPtr RelExpr::setJBBInput(EstLogPropSharedPtr & inLP)
{
  // pass it to children
  Int32 arity = getArity();

  for (Lng32 i = 0; i < arity; i++)
    child(i)->setJBBInput(inLP);

  if (getGroupAttr() &&
      getGroupAttr()->getGroupAnalysis() &&
      getGroupAttr()->getGroupAnalysis()->getNodeAnalysis())
  {
    getGroupAttr()->getGroupAnalysis()->getNodeAnalysis()->setJBBInput(inLP);
  }

  return getGroupAttr()->outputLogProp(inLP);
}

EstLogPropSharedPtr Join::setJBBInput(EstLogPropSharedPtr & inLP)
{
  EstLogPropSharedPtr inputForRight = inLP;

  EstLogPropSharedPtr leftOutput =
    child(0)->setJBBInput(inLP);

  if(isTSJ())
  {
    // if this is a TSJ set the flag that indicates
    // that it is a mandatory TSJ, i.e. a TSJ that
    // could not be unnested
    setTSJAfterSQO();
    inputForRight = leftOutput;
  }

  child(1)->setJBBInput(inputForRight);

  if (getGroupAttr() &&
      getGroupAttr()->getGroupAnalysis())
  {
    if (getGroupAttr()->getGroupAnalysis()->getNodeAnalysis())
      getGroupAttr()->getGroupAnalysis()->getNodeAnalysis()->setJBBInput(inLP);
    else if (getGroupAttr()->getGroupAnalysis()->getLocalJBBView() &&
		     getGroupAttr()->getGroupAnalysis()->getLocalJBBView()->getJBBCs().entries() == 2)
      getGroupAttr()->getGroupAnalysis()->getLocalJBBView()->getJBB()->setInputEstLP(inLP);
  }

  return getGroupAttr()->outputLogProp(inLP);

}

EstLogPropSharedPtr MultiJoin::setJBBInput(EstLogPropSharedPtr & inLP)
{
  if((inLP != (*GLOBAL_EMPTY_INPUT_LOGPROP)) && (!inLP->getNodeSet()))
  {
    CANodeIdSet * inputNodeSet = new (CmpCommon::statementHeap()) CANodeIdSet();
    *inputNodeSet += NULL_CA_ID;
    inLP->setNodeSet(inputNodeSet);
  }

  inLP->setCacheableFlag(TRUE);
  jbbSubset_.getJBB()->setInputEstLP(inLP);

  // pass it to children
  Int32 arity = getArity();

  for (Lng32 i = 0; i < arity; i++)
    child(i)->setJBBInput(inLP);

  // first clear out the expression already synthesized
  getGroupAttr()->setLogExprForSynthesis(NULL);
  
  // Now do the synthLogProp. This is done here since stats for all the leaf
  // nodes have been computed. We can now correctly break down the MultiJoin
  // into a join tree join the jbbcs in the preferred order.
  synthLogProp();
  
  return QueryAnalysis::ASM()->getStatsForJBBSubset(jbbSubset_);
}

// LCOV_EXCL_START :cnu
RelExpr* RelExpr::expandMultiJoinSubtree()
{
  // pass it to children
  Int32 arity = getArity();

  for (Lng32 i = 0; i < arity; i++)
    child(i) = child(i)->expandMultiJoinSubtree();

  // Need to clear log properties since we are re-shaping the tree.
  getGroupAttr()->clearLogProperties();

  return this;
}
// LCOV_EXCL_STOP

// xxx move this to RelExpr.h and make inline
GroupAnalysis * RelExpr::getGroupAnalysis()
{
  return getGroupAttr()->getGroupAnalysis();
}

void RelExpr::primeGroupAnalysis()
{
  // Do no GroupAnalysis priming if there was no QueryAnalysis
  if (!QueryAnalysis::Instance()->isAnalysisON())
    return;

  GroupAnalysis* groupAnalysis = getGroupAnalysis();

  // The LocalJBBView is NULL unless a join or GB on top of join
  groupAnalysis->setLocalJBBView(NULL);

  // recursively call the children appending their subtreeTables
  CANodeIdSet allSubtreeTables;
  Int32 arity = getArity();
  for (Lng32 i = 0; i < arity; i++)
  {
    allSubtreeTables +=
      child(i).getPtr()->getGroupAnalysis()->getAllSubtreeTables();
  }
  // If this node has TableAnalysis itself, add it.
  if (groupAnalysis->getNodeAnalysis() AND
      groupAnalysis->getNodeAnalysis()->getTableAnalysis())
  {
    allSubtreeTables += groupAnalysis->getNodeAnalysis()->getId();
  }

  groupAnalysis->setSubtreeTables(allSubtreeTables);

  return;
}

void Join::primeGroupAnalysis()
{
  // Do no GroupAnalysis priming if there was no QueryAnalysis
  if (!QueryAnalysis::Instance()->isAnalysisON())
    return;

  GroupAnalysis* groupAnalysis = getGroupAnalysis();

  // recursively call the children appending their subtreeTables
  // and parentJBBViews
  JBBSubset* localView = NULL;
  if (canBePartOfJBB())
    localView = new (groupAnalysis->outHeap()) JBBSubset(groupAnalysis->outHeap());
  CANodeIdSet allSubtreeTables;
  Int32 arity = getArity();
  for (Lng32 i = 0; i < arity; i++)
  {
    GroupAnalysis* childAnalysis = child(i).getPtr()->getGroupAnalysis();

	// use children parentViews to build this join local view
    if (canBePartOfJBB())
    {
      const JBBSubset * childParentView =
        childAnalysis->getParentJBBView();
      if (childParentView && localView)
        localView->addSubset((*childParentView));
      else
        localView = NULL;
    }
    allSubtreeTables += childAnalysis->getAllSubtreeTables();
  }
  if(canBePartOfJBB())
    groupAnalysis->setLocalJBBView(localView);
  groupAnalysis->setSubtreeTables(allSubtreeTables);

  return;
}

void GroupByAgg::primeGroupAnalysis()
{
  // Do no GroupAnalysis priming if there was no QueryAnalysis
  if (!QueryAnalysis::Instance()->isAnalysisON())
    return;

  GroupAnalysis* groupAnalysis = getGroupAnalysis();
  GroupAnalysis* childAnalysis = child(0).getPtr()->getGroupAnalysis();

  // have same subtree tables as child
  groupAnalysis->setSubtreeTables(childAnalysis->getAllSubtreeTables());

  // The LocalJBBView is NULL unless
  // 1. This is a GB that is a GBMember of some JBB i.e it has a GBAnalysis.
  // 2. The child has a parentJBBView i.e. its either a JBBC or a JBBSubset.
  // 3. This GB is the GBMember of the same JBB that the parentJBBView belongs to.
  // This assure that any generated localJBBView would not mingle GB and JBBCs from
  // different JBBs. For such evil combination, we set localJBBView to NULL.

  if (getGBAnalysis() AND
      childAnalysis->getParentJBBView() AND
      childAnalysis->getParentJBBView()->getJBB()->getGBAnalysis() == getGBAnalysis())
  {
    JBBSubset* localView =
      new (groupAnalysis->outHeap()) JBBSubset(groupAnalysis->outHeap());
    localView->addSubset(*(childAnalysis->getParentJBBView()));
    localView->setGB(getGBAnalysis()->getGroupingNodeId());
    groupAnalysis->setLocalJBBView(localView);
  }
  else
  {
    groupAnalysis->setLocalJBBView(NULL);
  }
  // Warning: I should attach GBAnalysis to groupBy expr rather than
  // NodeAnalysis.
  // Warning: if a GB is pushed through multiple JBBs then we could
  // endup with a JBBSubset with GB and JBBCs from different JBBs.\
  // This can't happen in R2.0 but could happen after that. A solution
  // for this exist for after R2.0.

  return;
}

void Filter::primeGroupAnalysis()
{
  // Do no GroupAnalysis priming if there was no QueryAnalysis
  if (!QueryAnalysis::Instance()->isAnalysisON())
    return;

  // a filter node is transparent with respect to group analysis (so far)
  GroupAnalysis* groupAnalysis = getGroupAnalysis();
  GroupAnalysis* childAnalysis = child(0).getPtr()->getGroupAnalysis();

  // have same subtree tables as child
  groupAnalysis->setSubtreeTables(childAnalysis->getAllSubtreeTables());

  // have same localJBBView as child. Make deep copy (if not NULL of course)
  if (childAnalysis->getLocalJBBView())
  {
    JBBSubset* localView =
      new (groupAnalysis->outHeap()) JBBSubset(groupAnalysis->outHeap());
    localView->copySubsetMembers(*(childAnalysis->getLocalJBBView()));
    groupAnalysis->setLocalJBBView(localView);
  }
  else
  {
    groupAnalysis->setLocalJBBView(NULL);
  }

  // have same node analysis as child
  // Warning: Materialize could have a TableAnalysis this way! do
  // we want that? xxx
  groupAnalysis->setNodeAnalysis(childAnalysis->getNodeAnalysis());

  return;
}

// -----------------------------------------------------------------------
// Methods for class ValueIdSet
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// ValueIdSet::accumulateReferencingValues()
//
// This method accumulates those expressions that are members of the
// referencingSet and are referencing any value in ReferencedSet.
// -----------------------------------------------------------------------
void ValueIdSet::accumulateReferencingExpressions(const ValueIdSet & referencingSet,
                                  const ValueIdSet & referencedSet,
                                  NABoolean doNotDigInsideVegRefs,
                                  NABoolean doNotDigInsideInstNulls)
{
  for (ValueId referencingExpr = referencingSet.init();
	   referencingSet.next(referencingExpr);
	   referencingSet.advance(referencingExpr))
  {
    for (ValueId referencedExpr = referencedSet.init();
         referencedSet.next(referencedExpr);
         referencedSet.advance(referencedExpr))
	{
      if (referencingExpr.getItemExpr()
          ->referencesTheGivenValue(
                   referencedExpr,
                   doNotDigInsideVegRefs,
                   doNotDigInsideInstNulls))
      {
	    addElement(referencingExpr);
        break;
      }
	} // referencedExpr loop
  } // referencingExpr loop

} // ValueIdSet::accumulateReferencingExpressions()

// -----------------------------------------------------------------------
// ValueIdSet::removeNonReferencingExpressions()
//
// This method remove those expressions that are members of the
// referencingSet and not referencing any value in ReferencedSet.
// -----------------------------------------------------------------------
// LCOV_EXCL_START :cnu
void ValueIdSet::removeNonReferencingExpressions(const ValueIdSet & otherSet,
                                    NABoolean doNotDigInsideVegRefs,
                                    NABoolean doNotDigInsideInstNulls)
{
  for (ValueId myExpr = init(); next(myExpr); advance(myExpr))
  {
    NABoolean isReferencing = FALSE;
    for (ValueId otherExpr = otherSet.init();
         otherSet.next(otherExpr);
         otherSet.advance(otherExpr))
	{
      if (myExpr.getItemExpr()
          ->referencesTheGivenValue(
                   otherExpr,
                   doNotDigInsideVegRefs,
                   doNotDigInsideInstNulls))
      {
	    isReferencing = TRUE;
        break;
      }
	} // otherExpr loop
    if (NOT isReferencing)
      subtractElement(myExpr); // delete expression from set
  } // myExpr loop
}
// LCOV_EXCL_STOP

// -----------------------------------------------------------------------
// ValueIdSet::findAllReferencedBaseCols()
//
// This method find all base columns referenced directly or indirectly
// via this ValueIdSet. This includes digging into VEGs and recursively
// walking ItemExpr trees until the leaf nodes.
// -----------------------------------------------------------------------
void ValueIdSet::findAllReferencedBaseCols(ValueIdSet & result) const
{
  // Simply loop over all the expressions in the set
  for (ValueId x= init(); next(x); advance(x) )
  {
    x.getItemExpr()->findAll(ITM_BASECOLUMN, result, TRUE, TRUE);
  }
}

// -----------------------------------------------------------------------
// ValueIdSet::findAllReferencedIndexCols()
//
// This method find all index columns referenced directly or indirectly
// via this ValueIdSet. This includes digging into VEGs and recursively
// walking ItemExpr trees until the leaf nodes.
// -----------------------------------------------------------------------
void ValueIdSet::findAllReferencedIndexCols(ValueIdSet & result) const
{
  // Simply loop over all the expressions in the set
  for (ValueId x= init(); next(x); advance(x) )
  {
    x.getItemExpr()->findAll(ITM_INDEXCOLUMN, result, TRUE, TRUE);
  }
}


// -----------------------------------------------------------------------
// ValueIdSet::findAllEqualityCols()
//
// This method finds all eqaulity columns referenced directly or indirectly
// via this ValueIdSet.
// -----------------------------------------------------------------------
// LCOV_EXCL_START :cnu
void ValueIdSet::findAllEqualityCols(ValueIdSet & result) const
{
  // Iterate through all the expressions in the set
  for (ValueId x = init(); next(x); advance(x) )
  {
    x.getItemExpr()->findEqualityCols(result);
  }
}
// LCOV_EXCL_STOP

// -----------------------------------------------------------------------
// Perform columnAnalysis on this set
//
// The purpose of this method is to compute all the connection-by-reference
// and connection-by-VEG between columns. Connection-by-reference means
// simply that the two columns are referenced in the same predicate. The
// connection-by-VEG means that the two columns are in the same VEG.
//
// Another version of this method (which is commented) include also connection
// via range predicates and equal (non-VEG) predicates. 
// -----------------------------------------------------------------------
void ValueIdSet::columnAnalysis(QueryAnalysis* qa, NABoolean predOnSemiOrLeftJoin) const
{
  QueryAnalysis* queryAnalysis = QueryAnalysis::Instance();

  ValueIdSet cols;
  // Loop over all expressions in the set
  for (ValueId x= init(); next(x); advance(x) )
  {
    cols.clear();
    x.getItemExpr()->findAll(ITM_BASECOLUMN, cols, TRUE, TRUE);

    // This method will create PredAnalysis for this value if non has
    // been created so far.
    QueryAnalysis::Instance()->findPredAnalysis(x);

    for (ValueId c= cols.init(); cols.next(c); cols.advance(c) )
    {
      if (c.colAnalysis())
      {
	c.colAnalysis()->addToReferencingPreds(x);
	x.predAnalysis()->addToReferencedCols(c);
      }
    }

    // look for VEGPredicates and add them to their column's
    // vegPreds_ field
    ItemExpr* itemExpr = x.getItemExpr();
    OperatorTypeEnum op = itemExpr->getOperatorType();
    if (op ==  ITM_VEG_PREDICATE)
    {
      // I do not think we need to check for vegRef too. only vegPreds
      VEG* veg = ((VEGPredicate*) itemExpr)->getVEG();
      const ValueIdSet & values = veg->getAllValues();
      for (ValueId v = values.init(); values.next(v); values.advance(v))
      {
        if (v.getItemExpr()->getOperatorType() == ITM_BASECOLUMN)
        {
	  if (v.colAnalysis())
	  {
	    v.colAnalysis()->addToVegPreds(x);
	    x.predAnalysis()->addToVegConnectedCols(v);
	  }
        }
      }
    }
    else if((!predOnSemiOrLeftJoin) && (op == ITM_EQUAL))
    {
      ItemExpr * rhs = itemExpr->child(1);
      ItemExpr * lhs = itemExpr->child(0);
      
      ValueId rVid = rhs->getValueId();
      ValueId lVid = lhs->getValueId();
         
      ValueIdSet rightCols;
      ValueIdSet leftCols;
      
      NABoolean rightEvaluatesToConstant =
        rhs->doesExprEvaluateToConstant(FALSE, TRUE);
      NABoolean leftEvaluatesToConstant = 
        lhs->doesExprEvaluateToConstant(FALSE, TRUE);
        
      /*
      ValueIdSet rExprBaseCols;
      if(rhs)
        rhs->findAll(ITM_BASECOLUMN, rExprBaseCols, TRUE, TRUE);
      ValueIdSet lExprBaseCols;
      if(lhs)
        lhs->findAll(ITM_BASECOLUMN, lExprBaseCols, TRUE, TRUE);
      */

      if (rhs->getOperatorType() == ITM_BASECOLUMN)
      {
        rightCols += rhs->getValueId();
      }
      else if( rhs->getOperatorType() == ITM_VEG_REFERENCE )
      {
        ValueIdSet rVegMembers = ((VEGReference*)rhs)->getVEG()->getAllValues();
        
        for (ValueId rVegMember = rVegMembers.init();
                                  rVegMembers.next(rVegMember);
                                  rVegMembers.advance(rVegMember))
        {
          ItemExpr * rExpr = rVegMember.getItemExpr();
          if(rExpr && (rExpr->getOperatorType() == ITM_BASECOLUMN))
          {
            rightCols += rVegMember;
          }
        }
      }
        
      if (lhs->getOperatorType() == ITM_BASECOLUMN)
      {
        leftCols += lhs->getValueId();
      }
      else if( lhs->getOperatorType() == ITM_VEG_REFERENCE )
      {
        ValueIdSet lVegMembers = ((VEGReference*)lhs)->getVEG()->getAllValues();
        
        for (ValueId lVegMember = lVegMembers.init();
                                  lVegMembers.next(lVegMember);
                                  lVegMembers.advance(lVegMember))
        {
          ItemExpr * lExpr = lVegMember.getItemExpr();
          if(lExpr && (lExpr->getOperatorType() == ITM_BASECOLUMN))
          {
            leftCols += lVegMember;
          }
        }
      }

      // For this predicate to be useful for positionining the LHS, the RHS
      // should be independent of any other columns of the same table. i.e.
      // 1) RHS should evaluate to constan, OR
      // 2) RHS should  be produced by other JBBCs/tables.
      if(!leftEvaluatesToConstant)  // i.e. the left is not a constant
        if(leftCols.entries() && 
           rightEvaluatesToConstant)
        {
          for (ValueId lCol = leftCols.init();
                              leftCols.next(lCol);
                              leftCols.advance(lCol))
          {
            if (lCol.colAnalysis())
               lCol.colAnalysis()->addToEqualityCoveringPreds(x);
            //x.predAnalysis()->addToEqualityConnectedCols(lCol);
          }          
        }
        else{
          for (ValueId lCol = leftCols.init();
                              leftCols.next(lCol);
                              leftCols.advance(lCol))
          {
            JBBC * lTableJBBC = NULL;

            if (lCol.colAnalysis())
            {
               CANodeId lTable = lCol.colAnalysis()->
                                        getTableAnalysis()->
                                          getNodeAnalysis()->
                                            getId();
               lTableJBBC = lCol.colAnalysis()->
                                        getTableAnalysis()->
                                          getNodeAnalysis()->
                                            getJBBC();
            }

            CANodeIdSet lJoinedJBBCs;

            if(lTableJBBC)
              lJoinedJBBCs = lTableJBBC->getJoinedJBBCs();

            for(CANodeId joinedJBBC = lJoinedJBBCs.init();
                                      lJoinedJBBCs.next(joinedJBBC);
                                      lJoinedJBBCs.advance(joinedJBBC))
            {
              CANodeIdSet jbbcSubset;
              jbbcSubset += joinedJBBC;

              ValueIdSet joinPreds = joinedJBBC.
                                       getNodeAnalysis()->
                                         getJBBC()->
                                           getJoinPreds();
              if(joinPreds.contains(x) &&
                 jbbcSubset.jbbcsToJBBSubset()->coversExpr(rVid))
              {
                queryAnalysis->addProducingJBBC(rVid, joinedJBBC);
                lCol.colAnalysis()->addEqualityRelation(lVid, x, rVid, joinedJBBC);
                //x.predAnalysis()->addToEqualityConnectedCols(lCol);
              }
            }                                         
          }
        }
        
      // For this predicate to be useful for positionining the RHS, the LHS
      // should be independent of any other columns of the same table. i.e.
      // 1) LHS should evaluate to constant, OR
      // 2) LHS should be produced by other JBBCs/tables.
      if(!rightEvaluatesToConstant)
        if(rightCols.entries() && 
           leftEvaluatesToConstant)
        {
          for (ValueId rCol = rightCols.init();
                              rightCols.next(rCol);
                              rightCols.advance(rCol))
          {
            if (rCol.colAnalysis())
               rCol.colAnalysis()->addToEqualityCoveringPreds(x);
            //x.predAnalysis()->addToEqualityConnectedCols(rCol);
          }          
        }
        else{
          for (ValueId rCol = rightCols.init();
                              rightCols.next(rCol);
                              rightCols.advance(rCol))
          {
            JBBC * rTableJBBC = NULL;
            if (rCol.colAnalysis())
            {
               CANodeId rTable = rCol.colAnalysis()->
                                        getTableAnalysis()->
                                          getNodeAnalysis()->
                                            getId();
                                         
               rTableJBBC = rCol.colAnalysis()->
                                        getTableAnalysis()->
                                          getNodeAnalysis()->
                                            getJBBC();
            }

            CANodeIdSet rJoinedJBBCs;

            if(rTableJBBC)
              rJoinedJBBCs = rTableJBBC->getJoinedJBBCs();

            for(CANodeId joinedJBBC = rJoinedJBBCs.init();
                                      rJoinedJBBCs.next(joinedJBBC);
                                      rJoinedJBBCs.advance(joinedJBBC))
            {
              CANodeIdSet jbbcSubset;
              jbbcSubset += joinedJBBC;

              ValueIdSet joinPreds = joinedJBBC.
                                       getNodeAnalysis()->
                                         getJBBC()->
                                           getJoinPreds();
              if(joinPreds.contains(x) &&
                 jbbcSubset.jbbcsToJBBSubset()->coversExpr(lVid))
              {
                queryAnalysis->addProducingJBBC(lVid, joinedJBBC);
                rCol.colAnalysis()->addEqualityRelation(rVid, x, lVid, joinedJBBC);
                //x.predAnalysis()->addToEqualityConnectedCols(lCol);
              }
            }
          }          
        }
    }
    /* Inspectors ignore this commented block

    else if (op == ITM_LESS OR
	     op == ITM_LESS_EQ OR
	     op == ITM_GREATER OR
	     op == ITM_GREATER_EQ)
    {
      ItemExpr * lhs = itemExpr->child(0);
      ItemExpr * rhs = itemExpr->child(1);

      if (lhs->getOperatorType() == ITM_BASECOLUMN)
      {
        // For this query to be useful for positionining, the RHS should
        // be independent of any other columns of the same table. i.e.
        // 1) RHS should evaluate to constan, OR
        // 2) RHS should have no other column of the same table.
        v.colAnalysis()->addToVegPreds(x);
        x.predAnalysis()->addToVegConnectedCols(v);
      }

      ValueId column = lhs->getValueId();



      const ValueIdSet & values = veg->getAllValues();
      for (ValueId v = values.init(); values.next(v); values.advance(v))
      {
        if (v.getItemExpr()->getOperatorType() == ITM_BASECOLUMN)
        {
          v.colAnalysis()->addToVegPreds(x);
          x.predAnalysis()->addToVegConnectedCols(v);
        }
      }
    }

    else if (op == ITM_EQUAL)
    {
      ItemExpr * lhs = itemExpr->child(0);
      ItemExpr * rhs = itemExpr->child(1);

      if (lhs->getOperatorType() == ITM_BASECOLUMN)
      {
        // For this query to be useful for positionining, the RHS should
        // be independent of any other columns of the same table. i.e.
        // 1) RHS should evaluate to constan, OR
        // 2) RHS should have no other column of the same table.
        v.colAnalysis()->addToVegPreds(x);
        x.predAnalysis()->addToVegConnectedCols(v);
      }

      ValueId column = lhs->getValueId();



      const ValueIdSet & values = veg->getAllValues();
      for (ValueId v = values.init(); values.next(v); values.advance(v))
      {
        if (v.getItemExpr()->getOperatorType() == ITM_BASECOLUMN)
        {
          v.colAnalysis()->addToVegPreds(x);
          x.predAnalysis()->addToVegConnectedCols(v);
        }
      }
    }
    Inpspectors ignore end here
    */

  }

  return;
}

// -----------------------------------------------------------------------
// Methods for class GroupAnalysis
// -----------------------------------------------------------------------

// copy constructor.
GroupAnalysis::GroupAnalysis(const GroupAnalysis & other,
                             CollHeap *outHeap):
  groupAttr_(other.groupAttr_),
  allSubtreeTables_(other.allSubtreeTables_),
  caNode_(other.caNode_),
  heap_(outHeap)
{
  if (other.localJBBView_)
  {
    localJBBView_ = new (heap_) JBBSubset(heap_);
    localJBBView_->copySubsetMembers(*(other.localJBBView_));
  }
  else
  {
    localJBBView_ = NULL;
  }
}

// LCOV_EXCL_START :dd
GroupAnalysis::~GroupAnalysis()
{
  delete localJBBView_;
}
// LCOV_EXCL_STOP

// clear analysis results in GroupAnalysis.
// note: this should clear only analysis results and
// should not clear groupAttr_ or heap_
void GroupAnalysis::clear()
{
  localJBBView_ = NULL;
  caNode_ = NULL;
  allSubtreeTables_.clear();
}

// LCOV_EXCL_START :dpm
const NAString GroupAnalysis::getText() const
{
  NAString result("GroupAnalysis:\n");
  if (localJBBView_)
  {
    result += "LocalJBBView     :" + localJBBView_->getText() + "\n";
  }
  result += "SubtreeTables    : " + allSubtreeTables_.getText() + "\n";
  if (caNode_)
  {
    result += caNode_->getText() + "\n";
  }
  if (localJBBView_ &&
      localJBBView_->getJBB() &&
      localJBBView_->getJBB()->getInputEstLP() &&
      localJBBView_->getJBB()->getInputEstLP()->getNodeSet())
  {
    result += "JBBInput         : " + 
      localJBBView_->getJBB()->getInputEstLP()->getNodeSet()->getText() + "\n";
  }

  Int32 potential = groupAttr_->getPotential();

  if(potential < 0)
    result += "GroupPotential   : -" + istring(-1*groupAttr_->getPotential()) + "\n";
  else
    result += "GroupPotential   : " + istring(groupAttr_->getPotential()) + "\n";
  return result;
}

void GroupAnalysis::display() const
{
  print();
}

void GroupAnalysis::print (FILE *f,
	      const char * prefix,
	      const char * suffix) const
{
  fprintf (f, getText());
}

// Comparison between two groups
// should move this to GroupAttributes ==
NABoolean GroupAnalysis::operator == (const GroupAnalysis & other)
{
  if (this == &other)
    return TRUE;

  // two groups are the same if they have the same jbbParentView
  //and same char inputs.
  if (getGroupAttr()->getCharacteristicInputs() !=
        other.getGroupAttr()->getCharacteristicInputs())
    return FALSE;

  const JBBSubset * parentJBBView = getParentJBBView();
  if (parentJBBView == NULL OR
      parentJBBView != other.getParentJBBView())
    return FALSE;

  // the two groups are equivalent
  return TRUE;
}
// LCOV_EXCL_STOP

// The parent JBB view of the JBBSubset. If this group is a
// JBBC then a JBBSubset of this JBBC alone is returned. Otherwise
// the localJBBView_ is returned
const JBBSubset * GroupAnalysis::getParentJBBView() const
{
  if (getNodeAnalysis() &&
      getNodeAnalysis()->getJBBC())
  {
    JBBSubset* parentView = new (heap_) JBBSubset();
	parentView->addJBBC(getNodeAnalysis()->getId());
	return parentView;
  }
  else
    return localJBBView_;
}

void GroupAnalysis::reconcile(GroupAnalysis * other)
{
  // reconcile NodeAnalysis
  NodeAnalysis* othersNodeAnalysis = other->getNodeAnalysis();

  if (!getNodeAnalysis())
  {
    setNodeAnalysis(othersNodeAnalysis);
  }
  else
  {
    // I have a NodeAnalysis
    // Reconcile NodeAnalysis
    // In order to avoid inconsistencies between the ids used in
    // the JBBSubsets and the id of the JBBC resulting from the
    // reconcilation, we use the NodeAnalysis with the JBBC as
    // a base for the reconcilation.
    // xxx
    // Note that we may still have a similar problem with
    // allSubTreeTables (point to obsolete TableAnalysis ids),
    // we will need to fix this when allSubTreeTables starting
    // getting used (post R2.0).
    if(othersNodeAnalysis &&
       othersNodeAnalysis->getJBBC() &&
       !getNodeAnalysis()->getJBBC())
    {
      // reconcile othersNodeAnalysis with mine
      othersNodeAnalysis->reconcile(getNodeAnalysis());
      caNode_ = othersNodeAnalysis;
    }
    else{
      // reconcile NodeAnalysis with other
      getNodeAnalysis()->reconcile(othersNodeAnalysis);
    }
  }

  // reconcile localJBBView
  JBBSubset* othersLocalView = (JBBSubset*) other->getLocalJBBView();
  if (!getLocalJBBView() && othersLocalView)
  {
    setLocalJBBView(othersLocalView);
  }
  else if (getLocalJBBView() && !othersLocalView)
  {
    // keep yours in this case
  }
  else if (getLocalJBBView() && othersLocalView)
  {
    if (getLocalJBBView()->getJBB() == othersLocalView->getJBB())
    {
      // The assertion below may become invalid in the future if we implement
      // features like elimination of referential integrity joins.
      CMPASSERT(getLocalJBBView()->getJBBCs() == othersLocalView->getJBBCs());

      // GroupBy may be eliminated legaly via the GB elimination rule.
      // If any of the two groupAnalysis has no GB, make the result without GB.
      // First lets check for the following
      // if both have GBs they should be the same
      CMPASSERT((othersLocalView->getGB()==NULL_CA_ID) ||
                (getLocalJBBView()->getGB()==NULL_CA_ID) ||
                (getLocalJBBView()->getGB()==othersLocalView->getGB()));

      if (othersLocalView->getGB() == NULL_CA_ID)
      {
        ((JBBSubset*)(getLocalJBBView()))->setGB(NULL_CA_ID);
      }
    }
    else
    {
      // In some rare cases we may get two JBBSubsets from different JBBs
      // merging. This requires an expression (group) that is a JBBC for
      // for a parent JBB (parentJBBView) and a mainSubset for a child JBB
      // (localJBBView). Then an elimination for a GB that is a parent expr
      // for the aforementioned expr could result in a merge of the groups
      // of the two expressions. The top expr (GB) would have a localJBBView of
      // {JBBC;GB}, while the lower expr woulf have a localJBBView equal to
      // the mainSet of the lower JBB

      // In practice this should be the child expr groupAnalysis and other should
      // be that of the GB. But its safer not to assume that. So we try both ways.

      if (othersLocalView->getJBBCs().entries() == 1)
      {
	// other has 1 JBBC and this is mainJBBSubset
        CMPASSERT(*(getLocalJBBView()) ==
                    getLocalJBBView()->getJBB()->getMainJBBSubset());
        // We keep localJBBView as is
      }
      else
      {
	// other is a mainJBBSubset and this has 1 JBBC
        CMPASSERT(getLocalJBBView()->getJBBCs().entries() == 1 AND
                  *(othersLocalView) ==
                    othersLocalView->getJBB()->getMainJBBSubset());
        setLocalJBBView(othersLocalView); // take the values of the child group
      }
    }
  }

  // For now allSubtreeTables should be the same
  // this may change later if we enable table re-use and table elimination
  CCMPASSERT(getAllSubtreeTables() ==
  	      other->getAllSubtreeTables());
}

// list of all promising indexes in this subtree
// this will replace availableBtreeIndexes in GroupAttr
//const LIST(AccessPathAnalysis*) &
//  GroupAnalysis::getAllSubtreePromisingAccessPaths()
//{
//  xxx();
//}


// -----------------------------------------------------------------------
// Methods for class NodeAnalysis
// -----------------------------------------------------------------------

// LCOV_EXCL_START :cnu
EstLogPropSharedPtr NodeAnalysis::getStats()

{
  if (stats_ == NULL)
  {
	AppliedStatMan * appStatMan = QueryAnalysis::ASM();
	stats_ = appStatMan->getStatsForCANodeId(id_);
  }

  return stats_;
}
// LCOV_EXCL_STOP

CostScalar NodeAnalysis::getCardinality()
{
  if (stats_ == NULL)
  {
	AppliedStatMan * appStatMan = QueryAnalysis::ASM();
	stats_ = appStatMan->getStatsForCANodeId(id_);
  }

  return stats_->getResultCardinality();
}

// reconcile this NodeAnalysis object with the NodeAnalysis
// object passed in
// This is used in case of Group Merges
void NodeAnalysis::reconcile(NodeAnalysis * other)
{
  if (!other)
  {
    // the other guy is NULL
    // nothing to reconcile
    return;
  }

  if (getId() == other->getId())
    return;

  if(!getJBBC())
  {
    setJBBC(other->getJBBC());
  }
  else if (!other->getJBBC())
  {
    // keep yours
  }
  else
  {
    // both should be same JBBC
    CMPASSERT(getJBBC() == other->getJBBC());
  }

  if(!getTableAnalysis())
  {
    setTableAnalysis(other->getTableAnalysis());
  }
  else if (!other->getTableAnalysis())
  {
    // keep yours
  }
  else
  {
    // both should have the same table analysis
    CCMPASSERT(getTableAnalysis() == other->getTableAnalysis());
  }

  // NodeAnalysis with GBAnalysis are not reconcilable
  CMPASSERT((!getGBAnalysis()) && (!other->getGBAnalysis()));
}

// This method computes some basic stats about the node, and caches them
void NodeAnalysis::computeStats()
{
  //Get a handle to ASM
  AppliedStatMan * appStatMan = QueryAnalysis::ASM();
  stats_ = appStatMan->getStatsForCANodeId(id_);
  recordSize_ = getOriginalExpr()->getGroupAttr()->getRecordLength();
  // if table compute table specific stats
  if (getTableAnalysis())
  getTableAnalysis()->computeTableStats();
  return;
}

// LCOV_EXCL_START :dpm
const NAString NodeAnalysis::getText() const
{
  NAString result("NodeAnalysis # ");
  result += istring(id_) + ":\n";
  if (jbbc_)
  {
    result += "This node is a JBBC\n";
    result += jbbc_->getText();
  }
  if (table_)
  {
    result += "This node is a Base Table\n";
    result += table_->getText();
  }
  result += "\n";
  return result;
}

void NodeAnalysis::display() const
{
  print();
}

void NodeAnalysis::print (FILE *f,
	      const char * prefix,
	      const char * suffix) const
{
  fprintf (f, getText());
}
// LCOV_EXCL_STOP

NABoolean NodeAnalysis::isExtraHub()
{
//// Debugging stuff TBD!!!
//NAString name(table_->getTableDesc()->getNATable()->getTableName().getQualifiedNameAsString());
//if (!name.compareTo("CAT.TPCD.REGION"))
//return TRUE;

  return isExtraHub_;
}


// -----------------------------------------------------------------------
// Methods for class TableAnalysis
// -----------------------------------------------------------------------

// LCOV_EXCL_START :cnu & dpm
// List access paths that are promising for predicate lookup
const LIST(AccessPathAnalysis*) &
  TableAnalysis::promisingAccessPathsForLookup()
{
  //  xxx Not complete. Return all indexes for now.
  return accessPaths_;
}

// This is the helper function for the interfaces below:
// - indexOnly specifies the list of access paths to search.
// - cc specifies the coverage (index or part key).
// - vidSet is the provided set to check for coverage.
// - exactMatch is only relevant for partitioning key coverage and
//      specifies if exact match is needed with the provided set.
LIST(AccessPathAnalysis*)
TableAnalysis::getCoveringAccessPaths(NABoolean indexOnly,
                                      coverageCriteria cc,
                                      const ValueIdSet& vidSet,
                                      NABoolean exactMatch)
{
  LIST(AccessPathAnalysis*) accessPaths(STMTHEAP), retAccessPaths(STMTHEAP);

  if (indexOnly)
    // Search index-only access paths.
    accessPaths = indexOnlyAccessPaths_;
  else
    // Search all access paths.
    accessPaths = accessPaths_;

  // Iterate through access paths to locate required access paths
  for (CollIndex i=0; i<accessPaths.entries(); i++)
  {
    // Index coverage
    if (cc == INDEX)
    {
      if (accessPaths[i]->numIndexPrefixCovered(vidSet, exactMatch))
        // The index prefix is covered, so add the access path to the list.
        retAccessPaths.insert(accessPaths[i]);
    }
    // Partitioning key coverage
    if (cc == PART_KEY)
    {
      if (accessPaths[i]->isPartKeyCovered(vidSet, exactMatch))
        // The part key is covered, so add the access path to the list.
        retAccessPaths.insert(accessPaths[i]);
    }
  }

  // Return the list of access paths.
  return retAccessPaths;
}

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
LIST(AccessPathAnalysis*)
TableAnalysis::accessPathsCoveringIndex(const ValueIdSet& vidSet,
                                        NABoolean indexOnly,
                                        NABoolean exactMatch)
{
  return getCoveringAccessPaths(indexOnly, INDEX, vidSet, exactMatch);
}

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
LIST(AccessPathAnalysis*)
TableAnalysis::accessPathsCoveringPartKey(const ValueIdSet& vidSet,
                                          NABoolean indexOnly,
                                          NABoolean exactMatch)
{
  return getCoveringAccessPaths(indexOnly, PART_KEY, vidSet, exactMatch);
}

const NAString TableAnalysis::getText() const
{
  NAString result(">> TableAnalysis # ");
  result += istring(caNodeAnalysis_->getId()) + ":\n";
  result += tableDesc_->getNATable()->getTableName().getQualifiedNameAsString();
  result += "\n";
  result += "localPreds: " + valueIdSetGetText(localPreds_) + "\n";
#ifdef MVQR
  result += "referencingPreds: " + valueIdSetGetText(referencingPreds_) + "\n";
  result += "vegPreds: " + valueIdSetGetText(vegPreds_) + "\n";
#endif
  result += "constCols: " + valueIdSetGetText(constCols_) + "\n";
  result += "usedCols: " + valueIdSetGetText(usedCols_) + "\n";

  for (ValueId x = usedCols_.init(); usedCols_.next(x); usedCols_.advance(x))
  {
    result += ">>>> " + x.colAnalysis()->getText();
  }

  if (caNodeAnalysis_ && caNodeAnalysis_->getJBBC())
    {
      const CANodeIdSet &predecessors = caNodeAnalysis_->getJBBC()->getPredecessorJBBCs();
      
      if (predecessors.entries() > 0)
        {
          char buf[12];
          NABoolean first = TRUE;
          result += "predecessor JBBCs: {";
          for (CANodeId x=predecessors.init(); predecessors.next(x); predecessors.advance(x))
            {
              if (!first)
                result += ", ";
              first = FALSE;
              snprintf(buf, sizeof(buf), "%d", x.toUInt32());
              result += buf;
            }
          result += "}";
        }
    }

  result += "\nIndexOnly Access Paths: \n";
  for (CollIndex i=0; i<indexOnlyAccessPaths_.entries(); i++)
  {
    result += indexOnlyAccessPaths_[i]->getText() + "\n";
  }

  result += "\nAll Access Paths: \n";
  for (CollIndex i=0; i<accessPaths_.entries(); i++)
  {
    result += accessPaths_[i]->getText() + "\n";
  }

  return result;
}

void TableAnalysis::print (FILE *f,
	      const char * prefix,
	      const char * suffix) const
{
  fprintf (f, getText());
}
// LCOV_EXCL_STOP

void TableAnalysis::initAccessPaths()
{
  const LIST(IndexDesc *) &indexes = tableDesc_->getIndexes();
  for (CollIndex i = 0; i < indexes.entries(); i++)
  {
    AccessPathAnalysis * accessPath =
      new (heap_) AccessPathAnalysis(indexes[i], this, heap_);
    indexes[i]->setAccessPathAnalysis(accessPath);
    accessPaths_.insert(accessPath);

    // if index is Unique add to uniqueIndexes_
    // have to consider base table also
    if((indexes[i]->isUniqueIndex()) ||
       (indexes[i]->isClusteringIndex() &&
        (indexes[i]->getPrimaryTableDesc()->getPrimaryKeyColumns().entries()>0)))
      uniqueIndexes_.insert(accessPath);

    // Set AccessPathAnalysis for base table.
    if (indexes[i]->isClusteringIndex())
      setApaForBaseTable(accessPath);
  }
}

void TableAnalysis::initIndexOnlyAccessPaths()
{
  // Get the expression from node analysis.
  Scan *scanExpr = (Scan *)getNodeAnalysis()->getOriginalExpr();
  // Only do this if the operator is REL_SCAN.
  if (scanExpr->getOperatorType() != REL_SCAN)
    return;
  GroupAttributes *groupAttr = scanExpr->getGroupAttr();

  // All the value ids that are required by the scan.
  ValueIdSet requiredValueIds(groupAttr->getCharacteristicOutputs());
  // Add all the base cols that contribute to VEGPredicates.
  scanExpr->addBaseColsFromVEGPreds(requiredValueIds);
  // Add local predicates since they are also required.
  requiredValueIds += getLocalPreds();

  for (CollIndex i=0; i<accessPaths_.entries(); i++)
  {
    ValueIdSet referencedInputs;
    ValueIdSet coveredSubexpr;
    ValueIdSet unCoveredExpr;
    GroupAttributes indexOnlyGA;
    NABoolean indexOnlyScan;

    IndexDesc *index = accessPaths_[i]->getIndexDesc();
    // make group attributes for an index scan
    indexOnlyGA.addCharacteristicOutputs(index->getIndexColumns());
    indexOnlyGA.addCharacteristicOutputs(scanExpr->getExtraOutputColumns());

    // Check if the index covers all required values.
    // There is additional information that is returned but not used here.
    indexOnlyScan =
      requiredValueIds.isCovered(groupAttr->getCharacteristicInputs(),
                                 indexOnlyGA,
                                 referencedInputs,
                                 coveredSubexpr,
                                 unCoveredExpr);
    if (indexOnlyScan)
    {
      // If all the required value ids are covered, add this to the list
      // of index-only access paths.
      indexOnlyAccessPaths_.insert(accessPaths_[i]);
    }
  }
}

void TableAnalysis::finishAnalysis()
{
  ValueIdSet inputsToScan;

  inputsToScan = getNodeAnalysis()->
                  getOriginalExpr()->
                    getGroupAttr()->
                      getCharacteristicInputs();

  ValueIdSet scanInputs;

  for (ValueId inputVid = inputsToScan.init();
                          inputsToScan.next(inputVid);
                          inputsToScan.advance(inputVid) )
  {
    if(inputVid.getItemExpr()->getOperatorType() == ITM_VEG_REFERENCE)
    {
      const ItemExpr * vegRef = inputVid.getItemExpr();
      scanInputs +=
        ((VEGReference *)vegRef)->getVEG()->getAllValues();
    }
    else
      scanInputs += inputVid;
  }

  connectedCols_.clear();
  connectedTables_.clear();

  ColAnalysis* col = NULL;
  for (ValueId c = usedCols_.init();
       usedCols_.next(c);
       usedCols_.advance(c))
  {
    col = c.colAnalysis();
    col->finishAnalysis();
    connectedCols_ += col->getConnectedCols();
    connectedTables_ += col->getConnectedTables();
    referencingPreds_ += col->getReferencingPreds();
    vegPreds_ += col->getVegPreds();
  }
  // assert that connectedCols_ and connectedTables_ does
  // not contain self.

  // Another consistency check:
  // we can recompute connectedCols_ and connectedTables_
  // here based on the preds and assert if different that
  // above computation. In Debug code only.

  // Now finalize local preds and compute constCols_
  ValueIdSet localVegs = localPreds_;
  localVegs.intersectSet(vegPreds_);
  for (ValueId veg = localVegs.init();
       localVegs.next(veg);
       localVegs.advance(veg))
  {
    // a VegPred is considered local if it has a constant value
    // or if it equates 2 or more columns from this table

    // first lets find the columns from this table that are in this veg
    ValueIdSet myColsInVeg = veg.predAnalysis()->getVegConnectedCols();
    myColsInVeg.intersectSet(usedCols_);

    // Now let us see if this veg reference any const expression.
    // (I am thinking of caching the fact that veg is constant in
    // PredAnalysis in future).
    const ItemExpr* pred = veg.getItemExpr();
    const VEG * predVEG = ((VEGPredicate *)pred)->getVEG();
    ValueIdSet vegGroup;
    vegGroup += predVEG->getAllValues();

    if (vegGroup.referencesAConstExpr())
    {
      // Certainly a local pred.
      // also add tableColsInVeg to constCols.
      constCols_ += myColsInVeg;
      continue;
    }

    if (myColsInVeg.entries() > 1)
    {
      // do nothing, this is a local Pred since it is connecting
      // two or more of my columns. The fact that it may be connecting
      // to other tables as well is fine.
      continue;
    }

    if (vegGroup.intersectSet(scanInputs).entries())
      continue;

    // Fail both conditions. Remove from localPreds_.
    // This predicate can't be evaluated localy
    localPreds_ -= veg;
  }

  // Add a column to constCols_ if it has an equality predicate
  // against a constant that did not become a VEG e.g. c1 = extract_month(10212009);
  for (ValueId colId= usedCols_.init(); usedCols_.next(colId); usedCols_.advance(colId))
  {
    ColAnalysis* colAn = QueryAnalysis::Instance()->getColAnalysis(colId);
    if(colAn->getEqualityCoveringPreds().entries())
      constCols_+=colId;
  }

  // Initialize index only access paths.
  if(CmpCommon::getDefault(COMP_BOOL_23) == DF_ON)
    initIndexOnlyAccessPaths();

  return;
}

void TableAnalysis::checkIfCompressedHistsViable()
{
  QueryAnalysis *qa = QueryAnalysis::Instance();
  if(!qa || !qa->isCompressedHistsViable())
    return;

  NABoolean okToCompress = TRUE;

  for (ValueId c = usedCols_.init();
       usedCols_.next(c);
       usedCols_.advance(c))
  {
    if(!okToCompress || (CmpCommon::getDefault(COMP_BOOL_25) == DF_OFF))
      break;
    
    ColAnalysis * colAnalysis = c.colAnalysis();
     
      if(colAnalysis)
      {
        ValueIdSet localPreds = colAnalysis->getLocalPreds();	
        ValueIdSet vegPreds = colAnalysis->getVegPreds();	
        ValueIdSet refPreds = colAnalysis->getReferencingPreds();
        refPreds.subtractSet(localPreds);
        refPreds.subtractSet(vegPreds);

        for (ValueId col = refPreds.init();
                           refPreds.next(col);
                           refPreds.advance(col))
        {
          if(!okToCompress)
            break;

          ItemExpr *tempIE = col.getItemExpr();

          if(tempIE->getOperatorType() != ITM_OR)
            continue;

          ValueIdSet constExprs;
          tempIE->accumulateConstExprs(constExprs);

          for (ValueId constExprId = constExprs.init();
                       constExprs.next(constExprId);
                       constExprs.advance(constExprId))
          {
            OperatorTypeEnum op = constExprId.getItemExpr()->getOperatorType();
            if( (op != ITM_EQUAL)     &&
                (op != ITM_NOT_EQUAL) &&
                (op != ITM_LESS)      &&
                (op != ITM_LESS_EQ)   &&
                (op != ITM_GREATER)   &&
                (op != ITM_GREATER_EQ))
              continue;

            if(constExprId.getItemExpr()->referencesTheGivenValue(c))
            {
              okToCompress = FALSE;
              break;
            }
          }
        }
      }
  }
  if(!okToCompress)
    qa->disableCompressedHistsViable();
}

// LCOV_EXCL_START :cnu
ValueIdSet TableAnalysis::getConnectingVegPreds(ColAnalysis & col) const
{
  ValueIdSet result = vegPreds_;
  result.intersectSet(col.getVegPreds());
  return result;
}
// LCOV_EXCL_STOP

ValueIdSet TableAnalysis::getConnectingVegPreds(TableAnalysis & other) const
{
  ValueIdSet result = vegPreds_;
  result.intersectSet(other.getVegPreds());
  return result;
}

////////////////////////////////////////

// LCOV_EXCL_START :cnu
// Get all JBBCs that are connected to this set of columns and the join preds
// This table must be JBBC
CANodeIdSet TableAnalysis::getJBBCsConnectedToCols(const CANodeIdSet & jbbcs,
                                    const ValueIdSet & cols,
                                    ValueIdSet & joinPreds /*OUT*/,
                                    ValueIdSet & localPreds /*OUT*/,
                                    ValueIdSet * predCols /*OUT*/ /*could be either join or local pred*/)
{
  CANodeIdSet result;
  localPreds.clear();
  joinPreds.clear();
  if (predCols)
    predCols->clear();

  for (ValueId col = cols.init();
       cols.next(col);
       cols.advance(col))
  {
    ColAnalysis* colAn = col.colAnalysis();
    if (!colAn) continue; // no preds on this column

    const ValueIdSet & colLocalPreds = colAn->getLocalPreds();
    if (colLocalPreds.entries())
    {
      localPreds += colLocalPreds;
      if (predCols)
        *predCols += col;
    }

    for (CANodeId jbbc = jbbcs.init();
         jbbcs.next(jbbc);
         jbbcs.advance(jbbc))
    {
      //ValueIdSet currentJoinPreds = colAn->getConnectingPreds(jbbc.getNodeAnalysis()->getJBBC());
      ValueIdSet currentJoinPreds = colAn->getAllConnectingPreds(jbbc.getNodeAnalysis()->getJBBC());
      if (currentJoinPreds.entries() > 0)
      {
        result.insert(jbbc);
        joinPreds += currentJoinPreds;
        if (predCols)
          *predCols += col;
      }
    }
  }
  return result;
};
// LCOV_EXCL_STOP

// Get the JBBCs that are connected to the maximum prefix size in the given column list
// This table must be JBBC
CANodeIdSet TableAnalysis::getJBBCsConnectedToPrefixOfList(const CANodeIdSet & jbbcs,
                                            const ValueIdList & cols,
                                            Lng32 & prefixSize /*OUT*/,
                                            ValueIdSet & joinPreds /*OUT*/,
                                            ValueIdSet & localPreds /*OUT*/)
{
  CANodeIdSet result;
  prefixSize = 0;
  localPreds.clear();
  joinPreds.clear();

  for (CollIndex i = 0; i < cols.entries(); i++)
  {
    NABoolean continuePrefix = FALSE;
    ColAnalysis* colAn = cols[i].colAnalysis();
    if (!colAn) break; // no preds on this column

    // xxx should change this to positioning local preds post EAP
    const ValueIdSet & colLocalPreds = colAn->getLocalPreds();
    if (colLocalPreds.entries() &&
        constCols_.contains(colAn->getColumnId()))
    {
      localPreds += colLocalPreds;
      continuePrefix = TRUE;
    }

    for (CANodeId jbbc = jbbcs.init();
         jbbcs.next(jbbc);
         jbbcs.advance(jbbc))
    {
      //ValueIdSet currentJoinPreds = colAn->getConnectingPreds(jbbc.getNodeAnalysis()->getJBBC());
      ValueIdSet currentJoinPreds = colAn->getAllConnectingPreds(jbbc.getNodeAnalysis()->getJBBC());

     if (currentJoinPreds.entries() && result.legalAddition(jbbc))
      {
          result.insert(jbbc);
          joinPreds += currentJoinPreds;
          continuePrefix = TRUE;
      }
    }

    if (continuePrefix)
      prefixSize++;
    else
      break;
  }

  return result;

};

// Get the JBBCs that are connected to the maximum prefix size in the given column list
// This table must be JBBC
ValueIdSet TableAnalysis::getColsConnectedViaEquiJoinPreds(const CANodeIdSet & jbbcs)
{
  ValueIdSet result;
  QueryAnalysis* qa = QueryAnalysis::Instance();

  CANodeIdSet joinedNodes = jbbcs;
  CANodeIdSet mySelf(this->getNodeAnalysis()->getId());

  JBBSubset * joinedJBBCs = joinedNodes.jbbcsToJBBSubset();
  JBBSubset * mySubset = mySelf.jbbcsToJBBSubset();

  // get all join preds in this JBBSubset
  ValueIdSet equiJoinPreds = joinedJBBCs->joinPredsWithOther(*mySubset);
  
  // narrow down join preds to veg preds
  equiJoinPreds.intersectSet(getVegPreds());

  // table columns involved in current query
  ValueIdSet tableCols = getTableDesc()->getColumnList();

  // iterate over join preds, for each pred
  // get the column from this table
  for (ValueId joinPred = equiJoinPreds.init();
       equiJoinPreds.next(joinPred);
       equiJoinPreds.advance(joinPred))
  {
    PredAnalysis * predAnalysis = joinPred.predAnalysis();
    ValueIdSet predRefCols = predAnalysis->getReferencedCols();
    predRefCols.intersectSet(tableCols);
    result.insert(predRefCols);
  }

  // add columns connected via non-VEG equijoin predicates
  // e.g. t1.a in predicate t1.a = t2.b + 1
  for (ValueId column = usedCols_.init();
       usedCols_.next(column);
       usedCols_.advance(column))
  {
    ColAnalysis * ca = qa->getColAnalysis(column);
    CANodeIdSet equalityConnectedJBBCs;
    
    if(ca)
      equalityConnectedJBBCs = ca->getEqualityConnectedJBBCs();
    
    equalityConnectedJBBCs.intersectSet(jbbcs);
    
    if(equalityConnectedJBBCs.entries())
      result.insert(column);
  }

  return result;

};

// get equality Join predicates connected to the given CANodeID
// jbbc should be a table
ValueIdSet TableAnalysis::getColsConnectedViaVEGPreds(CANodeId jbbc)
{
  ValueIdSet equiJoinPreds;

  CANodeIdSet connectedNodes = getConnectedTables();

  if (!connectedNodes.contains(jbbc) )
	return equiJoinPreds;

  TableAnalysis * connctedTA = jbbc.getNodeAnalysis()->getTableAnalysis();
  if (!connctedTA )
	return equiJoinPreds;

  equiJoinPreds.insert(getConnectingVegPreds(*connctedTA));

  // narrow down join preds to equijoin preds
  equiJoinPreds.intersectSet(getVegPreds());

  return equiJoinPreds;
}

// get equality factor for the given set of columns. Equality factor is
// defined as base UEC of given column set dividied by the base row count
// In case of multiple columns, if it would be multi-col UEC. If no stats
// exist for these set of columns, equality factor is zero.
CostScalar TableAnalysis::getEqualityVal(ValueIdSet joinCols)
{
  CostScalar equalityFactor = csZero;

  // nothing to do for cross products
  if (joinCols.entries() == 0)
	return equalityFactor;

  CostScalar baseRC = getCardinalityOfBaseTable();

  CostScalar baseUec = getBaseUec(joinCols);

  if (baseUec == csMinusOne)
	return equalityFactor;

  equalityFactor = baseUec/baseRC;

  return equalityFactor;
}

  // UEC of the given column set
CostScalar TableAnalysis::getBaseUec(const ValueIdSet & columns)
{
  const MultiColumnUecList * MCUecOfCurJoinPred = \
	  getStatsOfBaseTable()->getColStats().getUecList();

  if (!MCUecOfCurJoinPred)
	return csMinusOne;

  ValueIdSet baseColSet;

  columns.findAllReferencedBaseCols(baseColSet);

  // get all join columns of this table
  baseColSet.intersectSet(getUsedCols());

  // it could be single or multi-col UEC depending on the number of columns passed
  CostScalar baseUec = MCUecOfCurJoinPred->lookup(baseColSet);

  if (baseUec <= 0)
  {
	TableDesc * tableDesc = getTableDesc();
        // The flag is passed to indicate if stats should be used to check for uniqueness
        // too. That is define unique if rowcount == uec
	if (baseColSet.doColumnsConstituteUniqueIndex(tableDesc, FALSE) )
	  baseUec = getCardinalityOfBaseTable();
  }

  return baseUec;
}


// LCOV_EXCL_START :cnu

// Compute the local predicates on this table that references any of these columns
// of the table
ValueIdSet TableAnalysis::getLocalPredsOnColumns(const ValueIdSet & cols,
                                  ValueIdSet * colsWithLocalPreds /*OUT*/)
{
  // one other way of doing this is to get the table local preds and then select from
  // it the preds that reference cols in this set.

  ValueIdSet result;
  if (colsWithLocalPreds)
    colsWithLocalPreds->clear();

  for (ValueId col = cols.init();
       cols.next(col);
       cols.advance(col))
  {
    ColAnalysis* colAn = col.colAnalysis();
    if (!colAn) continue; // no preds on this column

    const ValueIdSet & colLocalPreds = colAn->getLocalPreds();
    if (colLocalPreds.entries())
    {
      result += colLocalPreds;
      if (colsWithLocalPreds)
        *colsWithLocalPreds += col;
    }
  }

  // assert that result is subset of table local preds
  CMPASSERT(localPreds_.contains(result));

  return result;
};
// LCOV_EXCL_STOP

// Compute the local predicates on this table that references a prefix of this
// column list. compute also the prefix size.
ValueIdSet TableAnalysis::getLocalPredsOnPrefixOfList(const ValueIdList & cols,
                                       Lng32 & prefixSize /*OUT*/)
{
  // xxx warning: at this point we do not distinguish between local predicates
  // and local predicates good for positioning. For example a=c is local but not
  // good for key positioning. Moreover a=c will be in a local preds but it needs
  // both a and c to execute. i.e. its local for the table but not the col.
  // We need to introduce positioning local preds for columns. This will be done
  // in the future.

  ValueIdSet result;
  prefixSize = 0;

  for (CollIndex i = 0; i < cols.entries(); i++)
  {
    ColAnalysis* colAn = cols[i].colAnalysis();
    if (!colAn) break; // no preds on this column
    const ValueIdSet & colLocalPreds = colAn->getLocalPreds();
    if (colLocalPreds.entries())
    {
      result += colLocalPreds;
      prefixSize++;
    }
    else {

      if ( !cols[i].isSaltColumn() )
        break;
    }
  }

  return result;
};

// This method computes some basic stats about the table, and caches them
void TableAnalysis::computeTableStats()
{
  //Get a handle to ASM
  AppliedStatMan * appStatMan = QueryAnalysis::ASM();
  ValueIdSet emptySet;
  CANodeId id = getNodeAnalysis()->getId();

  statsOfBaseTable_ = appStatMan->getStatsForCANodeId(id,(*GLOBAL_EMPTY_INPUT_LOGPROP),&emptySet);

  statsAfterLocalPredsOnCKPrefix_ =
    appStatMan->getStatsForLocalPredsOnCKPOfJBBC(id); // should change ASM method name

  recordSizeOfBaseTable_ = tableDesc_->getNATable()->getRecordLength();

      QueryAnalysis* queryAnalysis = QueryAnalysis::Instance();
      TableAnalysis * largestTable = queryAnalysis->getLargestTable();
      if(!largestTable)
        queryAnalysis->setLargestTable(this);
      else{
        CostScalar largestTableCardinality =
          largestTable->getCardinalityAfterLocalPredsOnCKPrefix();
        CostScalar largestTableRecordSize =
          largestTable->getRecordSizeOfBaseTable();
        CostScalar largestTableSize =
          largestTableCardinality * largestTableRecordSize;
        CostScalar tableCardinality =
          getCardinalityAfterLocalPredsOnCKPrefix();
        CostScalar tableRecordSize =
          getRecordSizeOfBaseTable();
        CostScalar tableSize =
          tableCardinality * tableRecordSize;

        if (tableSize > largestTableSize)
          queryAnalysis->setLargestTable(this);
      }
  return;
}

EstLogPropSharedPtr TableAnalysis::getStatsOfBaseTable()
{
	if (statsOfBaseTable_ == NULL)
	{
        AppliedStatMan * appStatMan = QueryAnalysis::ASM();
        ValueIdSet emptySet;
        CANodeId id = getNodeAnalysis()->getId();

        statsOfBaseTable_ = appStatMan->getStatsForCANodeId(id,(*GLOBAL_EMPTY_INPUT_LOGPROP),&emptySet);
	}
	return statsOfBaseTable_;
}

CostScalar TableAnalysis::getCardinalityOfBaseTable()
{
    return getStatsOfBaseTable()->getResultCardinality();
}

// LCOV_EXCL_START :cnu
CostScalar TableAnalysis::getMaxCardinalityOfBaseTable()
{
    return getStatsOfBaseTable()->getMaxCardEst();
}

NABoolean TableAnalysis::hasMatchingHashPartitioning(TableAnalysis * other)
{
  // get the index desc for me
  const IndexDesc * myIndxDesc = getTableDesc()->getClusteringIndex();

  // get the index desc for the other table
  const IndexDesc * otherIndxDesc = other->getTableDesc()->getClusteringIndex();

  // get my partitioning function
  PartitioningFunction * myPartFunc = myIndxDesc->getNAFileSet()->getPartitioningFunction();

  // get other table partition function
  PartitioningFunction * otherPartFunc = otherIndxDesc->getNAFileSet()->getPartitioningFunction();

  // return false if the partitioning type does not match
  if( myPartFunc->getPartitioningFunctionType() !=
      otherPartFunc->getPartitioningFunctionType())
    return FALSE;

  // return false if not hash partitioned
  if ((!myPartFunc->isATableHashPartitioningFunction()))
    return FALSE;

  // get my number of partitions
  Int32 myPartNum =  myIndxDesc->
                   getNAFileSet()->
                   getCountOfPartitions();

  // get other number of partitions
  Int32 otherPartNum =  otherIndxDesc->
                      getNAFileSet()->
                      getCountOfPartitions();

  // return false if number of partitions don't match
  if (myPartNum != otherPartNum)
    return FALSE;

  // get my partitioning key
  ValueIdList myPartKey = ((TableHashPartitioningFunction *) myPartFunc)->getKeyColumnList();

  // get other partitioning key
  ValueIdList otherPartKey = ((TableHashPartitioningFunction *) otherPartFunc)->getKeyColumnList();

  // return false if number of partitioning key columns is different
  if (myPartKey.entries() != otherPartKey.entries())
    return FALSE;

  for (UInt32 i = 0; i < myPartKey.entries(); i++)
  {
    //
    ColAnalysis * myColAnalysis = myPartKey[i].colAnalysis();

    if(!myColAnalysis)
      return FALSE;

    if(!myColAnalysis->getConnectedCols().contains(otherPartKey[i]))
      return FALSE;
  }

  return TRUE;
};

NABoolean TableAnalysis::predsOnUnique(ValueIdSet& vidSet, 
                                       NABoolean *hasPrefix)
{
  if(!vidSet.entries())
    return FALSE;
  ValueIdSet preds = vidSet;
  preds += localPreds_;

  for (CollIndex i=0; i<uniqueIndexes_.entries(); i++)
  {
    if(uniqueIndexes_[i]->keyCoveredByEqualityPreds(preds, hasPrefix))
      return TRUE;
  }

  return FALSE;
}
// LCOV_EXCL_STOP

// get a rough estimate of cost for doing a nested join
// number of probes = dataFlowFromEdge
// Rows of fact table that will be scanned = factTableRowsToScan
// 0 probes = Hash Join i.e. no nested join
CostScalar TableAnalysis::computeDataAccessCostForTable(
                                CostScalar probes,
                                CostScalar tableRowsToScan) const
{
  //Get a handle to ASM
  AppliedStatMan * appStatMan = QueryAnalysis::ASM();
  
  CANodeId tableId = getNodeAnalysis()->getId();

  CostScalar costPerProbe ((ActiveSchemaDB()->getDefaults()).getAsDouble(COMP_FLOAT_0));
  CostScalar costPerUnitSize ((ActiveSchemaDB()->getDefaults()).getAsDouble(COMP_FLOAT_1));
  CostScalar costPerRowReturned ((ActiveSchemaDB()->getDefaults()).getAsDouble(COMP_FLOAT_2));

  //Record Size of the factTable
  CostScalar tableRecordSize = getTableDesc()->
                                 getNATable()->
                                   getRecordLength();

  CostScalar subsetSizeKB = (tableRowsToScan * tableRecordSize)/1024;

  CostScalar tableRowsReturned = appStatMan->
                                 getStatsForCANodeId(tableId)->
                                 getResultCardinality();

  RelExpr * tableScanExpr = getNodeAnalysis()->getOriginalExpr();

  CostScalar sizeOfRowReturnedFromTable =
    tableScanExpr->getGroupAttr()->getRecordLength();


  CostScalar costForTable = (costPerProbe * probes) +
                            (costPerUnitSize * subsetSizeKB) +
                            (costPerRowReturned *
                            tableRowsReturned *
                            sizeOfRowReturnedFromTable);

  return costForTable;
}

////////////////////////////////////////

// -----------------------------------------------------------------------
// Methods for class AccessPathAnalysis
// -----------------------------------------------------------------------

AccessPathAnalysis::AccessPathAnalysis
  (IndexDesc* indexDesc,
   TableAnalysis* tableAnalysis,
   CollHeap *outHeap):
  indexDesc_(indexDesc),
  tableAnalysis_(tableAnalysis),
  accessPathKeyCount_(0),
  heap_(outHeap)
  {
    if(indexDesc_)
    {
      // setup index cols list
      ValueIdList indexCols = indexDesc_->getIndexColumns();
      ValueIdList keyCols = indexDesc_->getIndexKey();

      for(UInt32 i =0;
          i < indexCols.entries();
          i++)
      {
        ItemExpr   *ie = indexCols[i].getItemExpr();
        accessPathColList_.insert(((IndexColumn *) ie)->getDefinition());
      }

      for(UInt32 i =0;
          i < keyCols.entries();
          i++)
      {
        ItemExpr   *ie = keyCols[i].getItemExpr();
        accessPathKeyCols_.insert(((IndexColumn *) ie)->getDefinition());
      }

      // the set of columns in the access path
      accessPathColSet_.insertList(accessPathColList_);

      // get the column count of the access path key
      accessPathKeyCount_ = indexDesc_->getIndexKey().entries();

      // setup part key cols list
      ValueIdList partKeyCols = indexDesc_->getPartitioningKey();

      for(UInt32 i =0;
          i < partKeyCols.entries();
          i++)
      {
        ItemExpr   *ie = partKeyCols[i].getItemExpr();
        accessPathPartKeyCols_.insert(((IndexColumn *) ie)->getDefinition());
      }
    }
  }

// LCOV_EXCL_START :cnu
NABoolean AccessPathAnalysis::isIndexOnly()
{
  // xxx May be I should cache the result of this method if turn out
  // to be frequently asked.
  const ValueIdSet & usedCols =
    indexDesc_->getPrimaryTableDesc()->getTableAnalysis()->getUsedCols();
  return accessPathColSet_.contains(usedCols);
}

Int32 AccessPathAnalysis::numIndexPrefixCovered(const ValueIdSet& vidSet,
                                              NABoolean exactMatch,
                                              NABoolean useKeyCols)
{
    // find all the equality columns
  ValueIdSet equalityCols;
  vidSet.findAllEqualityCols(equalityCols);

  // Intersect the equality columns with the set of used columns
  // to get the columns for only this table.
  const ValueIdSet& usedCols =
    indexDesc_->getPrimaryTableDesc()->getTableAnalysis()->getUsedCols();

  // get the use equality columns
  equalityCols.intersectSet((ValueIdSet&)usedCols);

  // Get the list of index columns and check if a prefix is
  // covered in the provided set.
  Int32 numCoveredCols = useKeyCols ? 
    accessPathKeyCols_.prefixCoveredInSet(equalityCols) :
    accessPathColList_.prefixCoveredInSet(equalityCols) ;

  if (numCoveredCols &&
      ((!exactMatch) || numCoveredCols == vidSet.entries()))
    return numCoveredCols;
  else
    return 0;
}

NABoolean AccessPathAnalysis::isPartKeyCovered(const ValueIdSet& vidSet,
                                               NABoolean exactMatch)
{
  // find all the equality columns
  ValueIdSet equalityCols;
  vidSet.findAllEqualityCols(equalityCols);

  // Intersect the equality columns with the set of used columns
  // to get the columns for only this table.
  const ValueIdSet& usedCols =
    indexDesc_->getPrimaryTableDesc()->getTableAnalysis()->getUsedCols();

  // get the used equality cols
  equalityCols.intersect((ValueIdSet&)usedCols);

  // check if part key cols are covered in the provided set.
  return accessPathPartKeyCols_.allMembersCoveredInSet(equalityCols, exactMatch);
}

NABoolean AccessPathAnalysis::keyCoveredByEqualityPreds
(const ValueIdSet& vidSet, NABoolean *hasPrefix)
{
  Int32 keyPrefixCovered = numIndexPrefixCovered(vidSet, FALSE, 
                                                 hasPrefix?TRUE:FALSE);
  if (hasPrefix)
    *hasPrefix = (keyPrefixCovered > 0);
  if(keyPrefixCovered == accessPathKeyCount_)
    return TRUE;
  return FALSE;
}

const NAString AccessPathAnalysis::getText() const
{
  NAString result("AccessPathAnalysis: \n");
  result += "Index:  " + indexDesc_->getExtIndexName() + "   ";
  result += "Index Columns: " + valueIdSetGetText(indexDesc_->getIndexColumns());

  return result;
}

void AccessPathAnalysis::print (FILE *f,
                                const char * prefix,
                                const char * suffix) const
{
  fprintf (f, getText());
}

// -----------------------------------------------------------------------
// Methods for class JBBItem
// -----------------------------------------------------------------------

// Get all dependent predicates from other JBBItem on this JBBItem.
ValueIdSet JBBItem::successorPredsOfOther(const JBBItem & other) const
{
  ValueIdSet result = getPredsWithSuccessors();
  result.intersectSet(other.getPredsWithPredecessors());
  return result;
}

// Get all dependent predicates from this JBBItem on other JBBItem.
ValueIdSet JBBItem::predecessorPredsOnOther(const JBBItem & other) const
{
  ValueIdSet result = getPredsWithPredecessors();
  result.intersectSet(other.getPredsWithSuccessors());
  return result;
}
// LCOV_EXCL_STOP

// -----------------------------------------------------------------------
// Methods for class JBBC
// -----------------------------------------------------------------------

// Set the join preds to this JBBC.
// This will also update predAnalysis of these preds of this JBBC
void JBBC::setJoinPreds(const ValueIdSet & joinPreds)
{
  joinPreds_ = joinPreds;

  for (ValueId pred = joinPreds_.init();
       joinPreds_.next(pred);
       joinPreds_.advance(pred))
  {
    // This method will create PredAnalysis for this value if none has
    // been created so far.
	PredAnalysis* predAnalysis =
      QueryAnalysis::Instance()->findPredAnalysis(pred);
    predAnalysis->addToReferencedJBBCs(getId());
  }
}

// LCOV_EXCL_START 
// Not used - we are using the JBBSubset version of this
ValueIdSet JBBC::joinPredsWithOther(const JBBC & other) const
{
  ValueIdSet predSet = getJoinPreds();
  predSet.intersectSet(other.getJoinPreds());

  JBBSubset combinedSubset;
  combinedSubset.addJBBC(getId());
  combinedSubset.addJBBC(other.getId());

  ValueIdSet result;

  for (ValueId pred = predSet.init();
       predSet.next(pred);
       predSet.advance(pred))
  {
    if (combinedSubset.coversExpr(pred))
      result += pred;
  }

  return result;
}
// LCOV_EXCL_STOP

void JBBC::setPredsWithDependencies(const ValueIdSet & predsWithDependencies,
                                    const ValueIdSet & predsWithPredecessors)
{
  // set predsWithPredecessors_
  predsWithPredecessors_ += predsWithPredecessors;
  predsWithPredecessorsList_ =
    new (CmpCommon::statementHeap()) ValueIdList(predsWithPredecessors_);
  predecessorJBBCsList_ =
    new (CmpCommon::statementHeap())
      LIST(CANodeIdSet)(CmpCommon::statementHeap(),
                        predsWithPredecessorsList_->entries());

  // set constPredsWithPredecessors_
  NABoolean strict = FALSE;

  for (ValueId predId= predsWithPredecessors_.init();
       predsWithPredecessors_.next(predId);
       predsWithPredecessors_.advance(predId) )
  {
    ItemExpr * predExpr = predId.getItemExpr();
    if(predExpr && predExpr->doesExprEvaluateToConstant(strict))
      constPredsWithPredecessors_ += predId;
  }

  // set predsWithSuccessors_
  // get characterisic outputs of JBBCs
  const GroupAttributes * jbbcGA = getNodeAnalysis()->
                                     getOriginalExpr()->
                                       getGroupAttr();

  ValueIdSet jbbcCharOutput = jbbcGA->getCharacteristicOutputs();
  jbbcCharOutput += (ValueIdSet) nullInstantiatedOutput();

  ValueIdSet potentialPredsWithSuccessors = predsWithDependencies;
  potentialPredsWithSuccessors -= predsWithPredecessors_;
  predsWithSuccessors_.
    accumulateReferencingExpressions(potentialPredsWithSuccessors,
                                     jbbcCharOutput);
}

// LCOV_EXCL_START :cnu
// Get all JBBCs that are have no join predicate with this JBBC.
CANodeIdSet JBBC::getJBBCsThatXProductWithMe() const
{
  CANodeIdSet result = jbb_->getMainJBBSubset().getJBBCs();
  result.subtractElement(getId());
  result.subtractSet(joinedJBBCs_);
  result.subtractSet(successorJBBCs_);
  result.subtractSet(predecessorJBBCs_);
  return result;
}
// LCOV_EXCL_STOP

CANodeIdSet JBBC::getJBBCsConnectedViaKeyJoins()
{
  if(jbbcsJoinedViaTheirKeyIsSet_)
    return jbbcsJoinedViaTheirKey_;

  CANodeIdSet joinedJBBCs = joinedJBBCs_;
  CANodeIdSet jbbcsJoinedViaTheirKey;

  for (CANodeId joinedJBBCNode = joinedJBBCs.init();
       joinedJBBCs.next(joinedJBBCNode);
       joinedJBBCs.advance(joinedJBBCNode) )
  {
    TableAnalysis * tAnalysis = joinedJBBCNode.
                                  getNodeAnalysis()->
                                    getTableAnalysis();

    if (!tAnalysis)
      continue;

    TableDesc * tDesc = tAnalysis->getTableDesc();
    ValueIdSet primaryKey = tDesc->getPrimaryKeyColumns();

    JBBC * joinedJBBC = joinedJBBCNode.
                          getNodeAnalysis()->
                            getJBBC();

    // JBBC is joined via Left or Semi Join
    if (joinedJBBC->getPredecessorJBBCs().entries())
      continue;

    ValueIdSet joinedCols =
      tAnalysis->getColsConnectedViaEquiJoinPreds(getId());

    if(joinedCols.contains(primaryKey))
      jbbcsJoinedViaTheirKey += joinedJBBCNode;

  }

  jbbcsJoinedViaTheirKey_ = jbbcsJoinedViaTheirKey;
  jbbcsJoinedViaTheirKeyIsSet_ = TRUE;

  return jbbcsJoinedViaTheirKey_;
}

// LCOV_EXCL_START :cnu
/* Do not inspect this method */
// Returns a subset of getJoinedJBBCs that are joined with this JBBC
// on this particular column
CANodeIdSet JBBC::getJoinedJBBCsOnThisColumn(const ValueId & col,
	                                         ValueIdSet& joiningPreds /*OUT*/) const
{
  xxx();
  return CANodeIdSet(heap_);
}

/* Do not inspect this method */
// Returns a subset of getJoinedJBBCs that are joined with this JBBC
// on these particular columns
CANodeIdSet JBBC::getJoinedJBBCsOnTheseColumns(const ValueIdSet & cols,
	                                           ValueIdSet & joiningPreds /*OUT*/) const
{
  xxx();
  return CANodeIdSet(heap_);
}
// LCOV_EXCL_STOP

NABoolean JBBC::isGuaranteedEqualizer()
{
  if(isGuaranteedEqualizer_ == -1)
  {
    isGuaranteedEqualizer_ = 0;
    if(parentIsLeftJoin() &&
       !getSuccessorJBBCs().entries() &&
       !getJoinedJBBCs().entries() &&
       (getPredecessorJBBCs().entries()==1))
    {
      ValueIdSet leftJoinPreds = getPredsWithPredecessors();
      ValueIdSet leftJoinedCols;
      leftJoinPreds.findAllReferencedBaseCols(leftJoinedCols);
      TableAnalysis * leftJoinedTA = getNodeAnalysis()->
                                          getTableAnalysis();

      if(leftJoinedTA)
      {
        TableDesc * leftJoinedTDesc = leftJoinedTA->getTableDesc();
        ValueIdSet primaryKeyCols = leftJoinedTDesc->getPrimaryKeyColumns();

        if(leftJoinedCols.contains(primaryKeyCols))
          isGuaranteedEqualizer_ = 1;
      }
    }
  }

  return isGuaranteedEqualizer_;
}

// LCOV_EXCL_START :cnu
NABoolean JBBC::hasNonExpandingJoin()
{

  if(hasNonExpandingJoin_ != -1)
    return hasNonExpandingJoin_;

  Join * jbbcParentJoin = getOriginalParentJoin();

  if(jbbcParentJoin &&
     (jbbcParentJoin->isSemiJoin() ||
      jbbcParentJoin->isAntiSemiJoin()))
    return TRUE;

  ValueIdSet joinPreds = getJoinPreds();

  joinPreds += getPredsWithPredecessors();

  NodeAnalysis * jbbcNodeAnalysis = getNodeAnalysis();

  RelExpr * jbbcExpr = jbbcNodeAnalysis->getOriginalExpr();
  GroupAttributes * jbbcGA = jbbcExpr->getGroupAttr();
  ValueIdSet equalityCols;

  for (ValueId x = joinPreds.init();
                   joinPreds.next(x);
                   joinPreds.advance(x) )
  {
    ItemExpr * ie = x.getItemExpr();
    OperatorType ot = ie->getOperator();
    if(ot == ITM_VEG_PREDICATE)
    {
      VEGPredicate * vegPred = (VEGPredicate *) ie;
      equalityCols.insert(vegPred->getVEG()->getVEGReference()->getValueId());
    }
  }

  // add predicates that did not become a VEG and are against a single JBBC
  // this should capture something like t1.a = t2.a + 1
  // this will miss something like t1.a = t2.a + t3.a
  if(jbbcNodeAnalysis->getTableAnalysis())
  {
    equalityCols += jbbcNodeAnalysis->getTableAnalysis()->getEqualityConnectedCols();
  }

  hasNonExpandingJoin_ = jbbcGA->isUnique(equalityCols);

  return hasNonExpandingJoin_;
}

const NAString JBBC::getText() const
{
  NAString result("JBBC # ");
  result += istring(caNodeAnalysis_->getId()) + ":\n";
  result += "Child of JBB#" + istring(jbb_->getJBBId()) + "\n";
  result += "joinedJBBCs_      : " + joinedJBBCs_.getText();
  result += "successorJBBCs_   : " + successorJBBCs_.getText();
  result += "predecessorJBBCs_ : " + predecessorJBBCs_.getText();
  if (origParentJoin_)
    result += "origParentJoin_   : " + origParentJoin_->getText();
  else
    result += "origParentJoin_   : NULL";
  result += "\n";
  return result;
}

void JBBC::display() const
{
  print();
}

void JBBC::print (FILE *f,
	      const char * prefix,
	      const char * suffix) const
{
  fprintf (f, getText());
}
// LCOV_EXCL_STOP

// -----------------------------------------------------------------------
// Methods for class JBBSubset
// -----------------------------------------------------------------------

// Get all the join predicates (no dependency) between this JBBItem
// and the other JBBSubset
ValueIdSet JBBSubset::joinPredsWithOther(const JBBSubset & other) const
{
  ValueIdSet predSet = getJoinPreds();
  predSet.intersectSet(other.getJoinPreds());

  JBBSubset combinedSubset;
  combinedSubset.addJBBCs(jbbcs_);
  combinedSubset.addJBBCs(other.getJBBCs());

  ValueIdSet result;

  for (ValueId pred = predSet.init();
       predSet.next(pred);
       predSet.advance(pred))
  {
    if (combinedSubset.coversExpr(pred))
      result += pred;
  }

  return result;
}

Join * JBBSubset::getPreferredJoin()
{
  if(jbbcs_.entries() < 2)
    return NULL;

  MultiJoin * subsetMJ = getSubsetMJ();

  if(!subsetMJ)
  {
    CANodeIdSet allJBBCs = getJBB()->getMainJBBSubset().getJBBCs();
    JBBSubset * mainJBBSubset = allJBBCs.jbbcsToJBBSubset();
    MultiJoin * jbbMJ = mainJBBSubset->getSubsetMJ();
    NABoolean reUseMJ = TRUE;
    subsetMJ = jbbMJ->createSubsetMultiJoin(*this, reUseMJ);
    subsetMJ->synthLogPropWithMJReuse();
  }

  Join * preferredJoin = subsetMJ->getPreferredJoin();
  
  if(!preferredJoin->isJoinFromMJSynthLogProp())
  {
    CCMPASSERT(FALSE);
    subsetMJ->synthLogPropWithMJReuse();
  }

  return subsetMJ->getPreferredJoin();
}

  // union of both getJBBCs() and getGB()
CANodeIdSet JBBSubset::getJBBCsAndGB() const
  {
    // I may endup caching this as member (depending on usage frequency)
    CANodeIdSet jbbcsAndGB = jbbcs_;
	if (gb_ != NULL_CA_ID)
	  jbbcsAndGB.insert(gb_);
	return jbbcsAndGB;
  }

// LCOV_EXCL_START :cnu
// Get all JBBCs that are have no join predicate with this JBBSubset.
CANodeIdSet JBBSubset::getJBBCsThatXProductWithMe() const
{
  CANodeIdSet result = getJBB()->getMainJBBSubset().getJBBCs();
  result.subtractSet(getJBBCs());
  result.subtractSet(getJoinedJBBCs());
  result.subtractSet(getSuccessorJBBCs());
  result.subtractSet(getPredecessorJBBCs());
  return result;
}
// LCOV_EXCL_STOP

// Copy the JBBCs and GB from other JBBSubset.
// This method copy the JBBSubset without copying its heap_ affiliation.
void JBBSubset::copySubsetMembers(const JBBSubset & other)
{
  jbbcs_= other.jbbcs_;
  gb_ = other.gb_;
  // instead of clearing Analysis, its a good idea to copy the pointer
  // jbbSubsetAnalysis_ as well. Its either null (so thats like clearing it)
  // or already computed which is even better.
  jbbSubsetAnalysis_ = other.jbbSubsetAnalysis_;
  // Of course we should NOT copy heap_ here.
}

// LCOV_EXCL_START :cnu
// Subtract the content (JBBCs and GB) of other JBBSubset from this
// JBBSubset. This method side-effect this JBBSubset.
void JBBSubset::subtractSubset(const JBBSubset & other)
{
  jbbcs_ -= other.getJBBCs();
  if (gb_ == other.getGB())
  {
    gb_ = NULL_CA_ID;
  }
  clearAnalysis();
}
// LCOV_EXCL_STOP

// add the content (JBBCs and GB) of other JBBSubset to this
// JBBSubset. This method side-effect this JBBSubset.
void JBBSubset::addSubset(const JBBSubset & other)
{
  jbbcs_ += other.getJBBCs();
  CANodeId othersGB = other.getGB();
  if (othersGB != NULL_CA_ID)
  {
    CMPASSERT((gb_ == NULL_CA_ID) || (gb_ == othersGB))
    gb_ = othersGB;
  }
  clearAnalysis();
}

// Does this JBBSubset has a mandatory cross product.
// If so,=, then in any join order for this JBBSubset there will be
// a cross product.
// xxx: this function will be moved to JBBSubsetAnalysis
// Note: this method will retire now since numConnectedSubgraphs()
// does its job.
NABoolean JBBSubset::hasMandatoryXP() const
{
  if (jbbcs_.entries() < 2)
    return FALSE;

  CANodeIdSet available(jbbcs_);
  CANodeIdSet frontNodes(available.getFirst());

  CANodeId currNode;
  while ((frontNodes.entries() != 0) && (available.entries() != 0))
  {
    currNode = frontNodes.getFirst();
    available -= currNode;
    CANodeIdSet newNodes(currNode.getNodeAnalysis()->getJBBC()->getJoinedJBBCs());
    // execlude nodes that are already tested and nodes that are outside
    // this JBBSubset jbbcs
    newNodes.intersectSet(available);
    available -= newNodes;
    // now add the newNodes to frontNodes (nodes to be tested) and
    // remove currNode from frontNodes;
    frontNodes += newNodes;
    frontNodes -= currNode;
  }
  if (available.entries() !=0)
    return TRUE;
  else
    return FALSE;
}

// Calculate number of disjunct subgraphs in this JBBSubset
// xxx: this function will be moved to JBBSubsetAnalysis
Lng32 JBBSubset::numConnectedSubgraphs(NABoolean followSuccessors, 
                                      NABoolean pullInPredecessors) const
{
  if (jbbcs_.entries() < 1)
    return 0;

  Lng32 numSubgraphs = 1;

  CANodeIdSet available(jbbcs_);
  CANodeId frontNode;

  // pick a node that does not have predecessors
  for(frontNode = available.init();
      available.next(frontNode);
      available.advance(frontNode))
  {
    JBBC * frontNodeJBBC = frontNode.getNodeAnalysis()->getJBBC();
    CANodeIdSet frontNodePredecessors =
      frontNodeJBBC->getPredecessorJBBCs();
    if(!frontNodePredecessors.entries())
      break;
  }

  CANodeIdSet frontNodes(frontNode);

  CANodeId currNode;
  while (available.entries() != 0)
  {
    currNode = frontNodes.getFirst();
    available -= currNode;
    CANodeIdSet newNodes(currNode.getNodeAnalysis()->getJBBC()->getJoinedJBBCs());

    if(followSuccessors)
      newNodes += currNode.getNodeAnalysis()->getJBBC()->getSuccessorJBBCs();

    if(pullInPredecessors)
      newNodes += currNode.getNodeAnalysis()->getJBBC()->getPredecessorJBBCs();

    // execlude nodes that are already tested and nodes that are outside
    // this JBBSubset jbbcs
    newNodes.intersectSet(available);
    available -= newNodes;
    // now add the newNodes to frontNodes (nodes to be tested) and
    // remove currNode from frontNodes;
    frontNodes += newNodes;
    frontNodes -= currNode;
    // Check to see if we finished all nodes in current subgraph
    // In that case start a new subgraph, unless this is the last
    // subgraph i.e available set is empty.
    if ((frontNodes.entries() == 0) && (available.entries() > 0))
    {
      frontNodes += available.getFirst();
      numSubgraphs++;
    }
  }

  return numSubgraphs;
}


NAList<CANodeIdSet*>* JBBSubsetAnalysis::getConnectedSubgraphs(NABoolean followSuccessors,
                                                               NABoolean pullInPredecessors)
{
  if (connectedSubgraphs_ && followSuccessors && (!pullInPredecessors))
    return connectedSubgraphs_;

  NAList<CANodeIdSet*>* connectedSubgraphs = new (heap_) NAList<CANodeIdSet*>(heap_);

  //long numSubgraphs = 1;
  CANodeIdSet available(jbbcs_);

  CANodeId frontNode;

  // pick a node that does not have predecessors
  for(frontNode = available.init();
      available.next(frontNode);
      available.advance(frontNode))
  {
    JBBC * frontNodeJBBC = frontNode.getNodeAnalysis()->getJBBC();
    CANodeIdSet frontNodePredecessors =
      frontNodeJBBC->getPredecessorJBBCs();
    if(!frontNodePredecessors.entries())
      break;
  }

  CANodeIdSet frontNodes(frontNode);

  CANodeId currNode;
  CANodeIdSet currSubgraph;

  while (available.entries() != 0)
  {
    currNode = frontNodes.getFirst();
    currSubgraph += currNode;

    available -= currNode;
    CANodeIdSet newNodes(currNode.getNodeAnalysis()->getJBBC()->getJoinedJBBCs());

    if(followSuccessors)
      newNodes += currNode.getNodeAnalysis()->getJBBC()->getSuccessorJBBCs();

    if(pullInPredecessors)
      newNodes += currNode.getNodeAnalysis()->getJBBC()->getPredecessorJBBCs();
      
    // execlude nodes that are already tested and nodes that are outside
    // this JBBSubset jbbcs
    newNodes.intersectSet(available);
    available -= newNodes;
    // add the new nodes to the subgraph
    currSubgraph += newNodes;
    // now add the newNodes to frontNodes (nodes to be tested) and
    // remove currNode from frontNodes;
    frontNodes += newNodes;
    frontNodes -= currNode;
    // Check to see if we finished all nodes in current subgraph
    // In that case start a new subgraph, unless this is the last
    // subgraph i.e available set is empty.
    if ((frontNodes.entries() == 0) && (available.entries() > 0))
    {
      frontNodes += available.getFirst();
      connectedSubgraphs->insert(new (heap_) CANodeIdSet(currSubgraph, heap_));
      //numSubgraphs++;
      currSubgraph.clear();
    }
  }

  connectedSubgraphs->insert(new (heap_) CANodeIdSet(currSubgraph, heap_));

  if (followSuccessors)
    connectedSubgraphs_ = connectedSubgraphs;
  
  return connectedSubgraphs;
}


// LCOV_EXCL_START :cnu
CANodeIdSet JBBSubsetAnalysis::getInputSubgraph(CANodeId node, NABoolean followSuccessors) const
{
  
  CANodeIdSet inputSubgraph;

  //long numSubgraphs = 1;
  CANodeIdSet available(jbbcs_);
  
  available -= node;

  CANodeIdSet frontNodes(node.getNodeAnalysis()->getJBBC()->getPredecessorJBBCs());
  frontNodes.intersectSet(available);

  CANodeId currNode;
  CANodeIdSet currSubgraph;

  while (available.entries() != 0)
  {
    currNode = frontNodes.getFirst();
    inputSubgraph += currNode;

    available -= currNode;
    CANodeIdSet newNodes(currNode.getNodeAnalysis()->getJBBC()->getJoinedJBBCs());

    if(followSuccessors)
      newNodes += currNode.getNodeAnalysis()->getJBBC()->getSuccessorJBBCs();
    newNodes += currNode.getNodeAnalysis()->getJBBC()->getPredecessorJBBCs();
    
    // execlude nodes that are already tested and nodes that are outside
    // this JBBSubset jbbcs
    newNodes.intersectSet(available);
    available -= newNodes;
    // add the new nodes to the subgraph
    inputSubgraph += newNodes;
    // now add the newNodes to frontNodes (nodes to be tested) and
    // remove currNode from frontNodes;
    frontNodes += newNodes;
    frontNodes -= currNode;
    // Check to see if we finished all nodes in current subgraph
    // In that case start a new subgraph, unless this is the last
    // subgraph i.e available set is empty.
    if (frontNodes.entries() == 0)
      break;
  }

  return inputSubgraph;
}
// LCOV_EXCL_STOP
  
CANodeIdSet JBBSubset::getSubtreeTables() const
{
  CANodeIdSet result;

  for (CANodeId x= jbbcs_.init();  jbbcs_.next(x); jbbcs_.advance(x) )
  {
    result += x.getNodeAnalysis()->getJBBC()->getSubtreeTables();
  }

  return result;
}

ValueIdSet CANodeIdSet::getUsedCols() const
{
  ValueIdSet usedCols;
  usedCols.clear();

  for (CANodeId table = init();
				next(table);
				advance(table) )
  {
	usedCols += table.getNodeAnalysis()->getTableAnalysis()->getUsedCols();
  }
  return usedCols;
}

ValueIdSet CANodeId::getUsedTableCols()
{
  ValueIdSet usedCols;
  usedCols.clear();

  if((!getNodeAnalysis()) ||
     (!(getNodeAnalysis()->getTableAnalysis())))
    return usedCols;
    
  return getNodeAnalysis()->getTableAnalysis()->getUsedCols();
}

ValueIdSet CANodeIdSet::getUsedTableCols()
{
  ValueIdSet usedCols;
  usedCols.clear();

  for (CANodeId table = init();
        next(table);
        advance(table) )
  {
  usedCols += table.getUsedTableCols();
  }
  return usedCols;
}

// ------------------------------------------------------------------------
// get the minimum row count from all JBBCs in the JBBSubset after applying
// local predicates to individual JBBCs
// ------------------------------------------------------------------------

CostScalar JBBSubset::getMinimumJBBCRowCount() const
{

  CostScalar minimumRowCount = COSTSCALAR_MAX;

  //Get a handle to ASM
  AppliedStatMan * appStatMan = QueryAnalysis::ASM();

  for (CANodeId x= jbbcs_.init();  jbbcs_.next(x); jbbcs_.advance(x) )
  {
    CostScalar cardx = appStatMan->getStatsForCANodeId(x)->getResultCardinality();
    if (cardx < minimumRowCount)
      minimumRowCount = cardx;
  }

  return minimumRowCount;

} // JBBSubset::getMinimumJBBCRowCount

NABoolean JBBSubset::isGuaranteedNonExpandingJoin(JBBC jbbc)
{
  CANodeIdSet thisSetJBBCs = getJBBCs();

  if(!thisSetJBBCs.entries())
    return TRUE;

  Join * jbbcParentJoin = jbbc.getOriginalParentJoin();

  CANodeIdSet successorJBBCs = getSuccessorJBBCs();

  if(successorJBBCs.contains(jbbc.getId()) &&
     jbbcParentJoin &&
     (jbbcParentJoin->isSemiJoin() ||
      jbbcParentJoin->isAntiSemiJoin()))
    return TRUE;

  CANodeIdSet jbbcsThatCanComeBefore =
    jbbc.getJoinedJBBCs();
  jbbcsThatCanComeBefore +=
    jbbc.getPredecessorJBBCs();

  jbbcsThatCanComeBefore.intersectSet(thisSetJBBCs);

  if(!jbbcsThatCanComeBefore.entries())
    return FALSE;

  JBBSubset * jbbcSubset =
    ((CANodeIdSet)jbbc.getId()).jbbcsToJBBSubset();

  JBBSubset * connectedJBBCs =
    jbbcsThatCanComeBefore.jbbcsToJBBSubset();

  ValueIdSet connectingPreds =
    connectedJBBCs->joinPredsWithOther(*jbbcSubset);

  ValueIdSet jbbcPredsWithPredecessors =
    jbbc.getPredsWithPredecessors();

  jbbcPredsWithPredecessors.intersectSet(connectedJBBCs->getPredsWithSuccessors());

  connectingPreds += jbbcPredsWithPredecessors;

  NodeAnalysis * jbbcNodeAnalysis = jbbc.getNodeAnalysis();

  RelExpr * jbbcExpr = jbbcNodeAnalysis->getOriginalExpr();
  TableAnalysis * jbbcTable = jbbcNodeAnalysis->getTableAnalysis();
  ValueIdSet equalityConnectingPreds;
  if(jbbcTable)
    equalityConnectingPreds = jbbcTable->getEqualityConnectingPreds();
  
  GroupAttributes * jbbcGA = jbbcExpr->getGroupAttr();
  ValueIdSet ColVegRefSet;

  for (ValueId x = connectingPreds.init();
                   connectingPreds.next(x);
                   connectingPreds.advance(x) )
  {
    ItemExpr * ie = x.getItemExpr();
    OperatorType ot = ie->getOperator();
    if(ot == ITM_VEG_PREDICATE)
    {
      VEGPredicate * vegPred = (VEGPredicate *) ie;
      ColVegRefSet.insert(vegPred->getVEG()->getVEGReference()->getValueId());
    }
    
    if(equalityConnectingPreds.contains(x))
      ColVegRefSet.insert(jbbcTable->getEqualityConnectedCol(x));
  }

  return jbbcGA->isUnique(ColVegRefSet);
}

// LCOV_EXCL_START :cnu
// verify integrity of the JBBSubset. That is
// jbbcs_ contains only jbbcs from same JBB
// gb_ belongs to a group by of the same JBB
NABoolean JBBSubset::verify()
{
  xxx(); // never called for now
  return FALSE;
}

const NAString JBBSubset::getText() const
{
  NAString result("JBBSubset : {");

  for (CANodeId x= jbbcs_.init();  jbbcs_.next(x); jbbcs_.advance(x) )
  {
    result += " " + istring(x) + " ";
  }

  if (gb_ != NULL_CA_ID)
    result += "; " + istring(gb_);

  result += "}\n";

  return result;
}

void JBBSubset::display() const
{
  print();
}

void JBBSubset::print (FILE *f,
	      const char * prefix,
	      const char * suffix) const
{
  fprintf (f, getText());
}
// LCOV_EXCL_STOP

// -----------------------------------------------------------------------
// Methods for class JBBSubsetAnalysis
// -----------------------------------------------------------------------

// construct a JBBSubsetAnalysis
JBBSubsetAnalysis::JBBSubsetAnalysis(const JBBSubset & subset,
                                     CollHeap *outHeap)
  :leftDeepJoinSequence_(CmpCommon::statementHeap(),
                         subset.getJBBCs().entries()),
   matchingMVs_(CmpCommon::statementHeap())
{
  heap_ = outHeap;
  jbb_ = subset.getJBB();
  jbbcs_ = subset.getJBBCs();
  subsetMJ_ = NULL;
  gb_ = subset.getGB();
  allMembers_ = subset.getJBBCsAndGB();
  allJoinsInnerNonSemi_ = TRUE;
  connectedSubgraphs_ = NULL; // on demand only
  mjRulesWA_ = NULL;
  mjStarJoinIRuleWA_ = NULL; // this will be created and passed in by
                             // MJStarJoinIRule

  legality_ = NOT_KNOWN;
  
  computedFactAndLargestTable_ = FALSE;
  factTable_ = NULL_CA_ID;
  largestTable_ = NULL_CA_ID;

  computedLargestIndependentNode_ = FALSE;
  largestIndependentNode_ = NULL_CA_ID;

  starJoinTypeIFeasible_ =  FALSE;
  analyzedInitialPlan_ = FALSE;

  centerTableComputed_ = FALSE;
  centerTable_ = NULL_CA_ID;
  centerTableRowsScanned_ = -1;
  centerTableDataScanned_ = -1;
  centerTableDataPerPartition_ = -1;
  centerTablePartitions_ = -1;
  centerTableConnectivity_ = 0;
  maxDimensionConnectivity_ = 0;

  listOfEdges_ = NULL;
  optimalFTLocation_ = -1;
  lowestFactNJCost_ = csZero;
  costForMaxKeyFactNJ_ = csZero;
  synthLogPropPath_ = NULL;

  // Compute other fields
  init();
}

// LCOV_EXCL_START :cnu
// which JBBCs can I add to this subset while reserving its self-dpendency
CANodeIdSet JBBSubsetAnalysis::legalJBBCAdditions() const
{
  // for now, all other JBBCs are legal additions
  // when we include dependency joins, we need to use
  // dependency graph for this.
  CANodeIdSet result = jbb_->getMainJBBSubset().getJBBCs();
  result.subtractSet(jbbcs_);
  return result;
}
// LCOV_EXCL_STOP

// initialize JBBSubsetAnalysis computable fields
void JBBSubsetAnalysis::init()
{
  joinedJBBCs_.clear();
  successorJBBCs_.clear();
  predecessorJBBCs_.clear();
  ValueIdSet tmpJoinPreds;
  ValueIdSet tmpSuccessorPreds;
  ValueIdSet tmpPredecessorPreds;

  CANodeId i;

  for (i = jbbcs_.init();  jbbcs_.next(i); jbbcs_.advance(i) )
  {
    JBBC* jbbc = i.getNodeAnalysis()->getJBBC();
    joinedJBBCs_ += jbbc->getJoinedJBBCs();
    successorJBBCs_ += jbbc->getSuccessorJBBCs();
    predecessorJBBCs_ += jbbc->getPredecessorJBBCs();
    tmpJoinPreds += jbbc->getJoinPreds();
    tmpSuccessorPreds += jbbc->getPredsWithSuccessors();
    tmpPredecessorPreds += jbbc->getPredsWithPredecessors();
    Join * parentJoin = jbbc->getOriginalParentJoin();
    if(parentJoin && !(parentJoin->isInnerNonSemiNonTSJJoin()))
      allJoinsInnerNonSemi_ = FALSE;
  }

  // take subset members out
  joinedJBBCs_ -= jbbcs_;
  successorJBBCs_ -= jbbcs_;
  predecessorJBBCs_ -= jbbcs_;

  joinPreds_.clear();
  predsWithSuccessors_.clear();
  predsWithPredecessors_.clear();

  // now loop over joinedJBBCs_ etc to compute joinPreds_ etc
  for (i = joinedJBBCs_.init();
       joinedJBBCs_.next(i);
       joinedJBBCs_.advance(i) )
  {
    JBBC* jbbc = i.getNodeAnalysis()->getJBBC();
    ValueIdSet currentPreds = jbbc->getJoinPreds();
    // include joinPreds from current JBBC that are
    // joinPreds for some JBBC in this JBBSubset
    joinPreds_ += currentPreds.intersectSet(tmpJoinPreds);
  }

  for (i = successorJBBCs_.init();
       successorJBBCs_.next(i);
       successorJBBCs_.advance(i) )
  {
    JBBC* jbbc = i.getNodeAnalysis()->getJBBC();
    ValueIdSet currentPreds = jbbc->getPredsWithPredecessors();
    // include myDependentPreds from current JBBC that are
    // dependentPreds for some JBBC in this JBBSubset
    predsWithSuccessors_ += currentPreds.intersectSet(tmpSuccessorPreds);
  }

  for (i = predecessorJBBCs_.init();
       predecessorJBBCs_.next(i);
       predecessorJBBCs_.advance(i) )
  {
    JBBC* jbbc = i.getNodeAnalysis()->getJBBC();
    ValueIdSet currentPreds = jbbc->getPredsWithSuccessors();
    // include dependentPreds from current JBBC that are
    // myDependentPreds for some JBBC in this JBBSubset
    predsWithPredecessors_ += currentPreds.intersectSet(tmpPredecessorPreds);
  }

  if(jbbcs_.entries() == 1)
  {
     synthLogPropPath_ =
       new (CmpCommon::statementHeap()) 
       CASortedList(CmpCommon::statementHeap(), jbbcs_.entries());
     
     (*synthLogPropPath_).insert(jbbcs_.getFirst());
  }
  
  // xxx consider making this on demand only
  computeLocalJoinPreds();
}

// compute the join preds between these subset members
void JBBSubsetAnalysis::computeLocalJoinPreds()
{
  ValueIdSet accumulatedJoinPreds;
  ValueIdSet currentJoinPreds;
  ValueIdSet newLocalJoinPreds;
  ValueIdSet multipleReferenceJoinPreds;
  ValueIdSet allSuccessorPreds;
  ValueIdSet allPredecessorPreds;
  ValueIdSet allConstPredsWithPredecessors;
  ValueIdSet allLeftJoinFilterPreds;

  // First we try to compute multipleReferenceJoinPreds which are join preds
  // that are referencing more than 1 JBBC is this subset.
  CANodeId i;

  for (i = jbbcs_.init();  jbbcs_.next(i); jbbcs_.advance(i) )
  {
    JBBC* jbbc = i.getNodeAnalysis()->getJBBC();
    currentJoinPreds = jbbc->getJoinPreds();
    newLocalJoinPreds = currentJoinPreds;
    multipleReferenceJoinPreds += newLocalJoinPreds.intersectSet(accumulatedJoinPreds);
    accumulatedJoinPreds += currentJoinPreds;
    allSuccessorPreds += jbbc->getPredsWithSuccessors();
    allPredecessorPreds += jbbc->getPredsWithPredecessors();
    allConstPredsWithPredecessors += jbbc->getConstPredsWithPredecessors();
    allLeftJoinFilterPreds += jbbc->getLeftJoinFilterPreds();
  }

  // update multipleReferencePreds to exclude nonInnerJoinPreds
  // this is done since for nonInnerJoinPreds the VEG in the VEGPred
  // is only valid on the right side of the join
  multipleReferenceJoinPreds -= allSuccessorPreds;
  multipleReferenceJoinPreds -= allPredecessorPreds;
  multipleReferenceJoinPreds -= allConstPredsWithPredecessors;
  multipleReferenceJoinPreds -= allLeftJoinFilterPreds;
  
  // It is now guaranteed that each pred in multipleReferenceJoinPreds references
  // at least two JBBCs. This is necessary but not suffecient condition since it
  // is possible that the pred needs other values not supplied by any JBBC in
  // this JBBSubset. We filter those using the coverExpr(pred) test.

  localInnerNonSemiJoinPreds_.clear();

  for (ValueId pred = multipleReferenceJoinPreds.init();
       multipleReferenceJoinPreds.next(pred);
       multipleReferenceJoinPreds.advance(pred))
  {
    // if pred is a VEG then add it to the cache
    if ((pred.getItemExpr()->getOperatorType() == ITM_VEG_PREDICATE)||
        coversExpr(pred))
    {
      knownCovered_+=pred;
      localInnerNonSemiJoinPreds_ += pred;
    }
  }

  // localInnerNonSemiJoinPreds_ is now computed

  // consider predicates with dependencies
  localDependentJoinPreds_.clear();

  ValueIdSet allDependentPreds;
  allDependentPreds +=
    allPredecessorPreds.intersectSet(allSuccessorPreds);
  allDependentPreds +=
    allConstPredsWithPredecessors;
  allDependentPreds +=
    allLeftJoinFilterPreds;

  for (ValueId pred = allDependentPreds.init();
       allDependentPreds.next(pred);
       allDependentPreds.advance(pred))
  {
    // if pred is a VEG then add it to the cache
    if (//(pred.getItemExpr()->getOperatorType() == ITM_VEG_PREDICATE)||
        coversExpr(pred))
    {
      knownCovered_+=pred;
      localDependentJoinPreds_ += pred;
    }
  }

  // localDependentJoinPreds_ is now computed

  localJoinPreds_.clear();
  localJoinPreds_ += localInnerNonSemiJoinPreds_;
  localJoinPreds_ += localDependentJoinPreds_;

  // localJoinPreds_ is now computed
}

NABoolean JBBSubsetAnalysis::coversExpr(ValueId vid) const
{
  // first check the known covered and not covered caches
  if(knownCovered_.contains(vid))
    return TRUE;

  if(knownNotCovered_.contains(vid))
    return FALSE;

  // have to do this since we don't want to change the signature to remove
  // the const. The two data members that can be potentially modified are
  // there to cache the result, since a given vid should always return the
  // same result
  ValueIdSet * knownCovered = const_cast<ValueIdSet*> (&knownCovered_);
  ValueIdSet * knownNotCovered = const_cast<ValueIdSet*> (&knownNotCovered_);

  // if this Expr is already one of my local join predicates then
  // it is covered already. xxx add this to dependent local preds
  // in the future.
  // Note: Every localJoinPred is covered by the JBBSubset but *not*
  // every covered pred is part of the localJoinPred. For example
  // a covered subexpression which is evaluated at the JBBC level.
  if (getLocalJoinPreds().contains(vid))
  {
    (*knownCovered)+=vid;
    return TRUE;
  }

  NABoolean isAVEGPred = FALSE;
  ValueId vegRefId;
  
  if(vid.getItemExpr()->getOperatorType() == ITM_VEG_PREDICATE)
  {
    isAVEGPred = TRUE;
    vegRefId = ((VEGPredicate*)(vid.getItemExpr()))->getVEG()->getVEGReference()->getValueId();
  }
  
  // Otherwise we do the actual check for covering. We uses the
  // isCovered() method on the ItemExpr for that. Use the combined
  // JBBCs outputs for output and the JBB normInputs for inputs.
  ValueIdSet maximumOutput;
  for (CANodeId x= jbbcs_.init();  jbbcs_.next(x); jbbcs_.advance(x) )
  {
    JBBC * jbbc = x.getNodeAnalysis()->getJBBC();
    Join * parentJoin = jbbc->getOriginalParentJoin();
    
    
    if(!parentJoin || (parentJoin->isInnerNonSemiJoin() ||
        jbbc->getPredsWithPredecessors().contains(vid)))
    {
      ValueIdSet childOutputs = 
       x.getNodeAnalysis()->getOriginalExpr()->getGroupAttr()->getCharacteristicOutputs();
       
      if(jbbc->getPredsWithPredecessors().contains(vid) && isAVEGPred)
      {
        if(childOutputs.contains(vegRefId))
          childOutputs -= vegRefId;
      }
      
      maximumOutput += childOutputs;
    }

	// have to add the null instantiated values if the JBBC is
	// connected via a left outer join. If a JBBC is connected
	// via a left outer join then it has nullInstantiatedOutput
	// otherwise nullInstantiatedOutput is empty. This is because
	// the nullInstantiated output is not part of the JBBC's group
	// characteristic output, rather it is the output of the left
	// join.
    ValueIdSet nullInstantiatedOutputs(x.getNodeAnalysis()->
		                               getJBBC()->
									   nullInstantiatedOutput());
    maximumOutput += nullInstantiatedOutputs;
  }

  if(maximumOutput.contains(vid))
  {
    (*knownCovered)+=vid;
    return TRUE;
  }

  ValueIdSet inputs = jbb_->getNormInputs();

  ValueIdSet dummyNewExternalInputs;
  ValueIdSet dummyReferencedInputs;
  ValueIdSet dummyCoveredSubExpr;
  ValueIdSet dummyUnCoveredExpr;
  GroupAttributes* dummyGA =
    new (CmpCommon::statementHeap()) GroupAttributes();

  dummyGA->setCharacteristicOutputs(maximumOutput);
  dummyGA->setCharacteristicInputs(inputs);

  if (vid.getItemExpr()->isCovered
              (dummyNewExternalInputs,
		       *dummyGA,
		       dummyReferencedInputs,
		       dummyCoveredSubExpr,
		       dummyUnCoveredExpr))
  {
     (*knownCovered)+=vid;
     return TRUE;
  }

  (*knownNotCovered)+=vid;
  return FALSE;
}

void JBBSubsetAnalysis::setSubsetMJ(MultiJoin * subsetMJ)
{
  if(!jbbcs_.entries())
    subsetMJ_ = NULL;

  CMPASSERT(subsetMJ);

  CANodeIdSet mjJBBCs = subsetMJ->getJBBSubset().getJBBCs();

  if(jbbcs_ == mjJBBCs)
    subsetMJ_ = subsetMJ;
}

// LCOV_EXCL_START
// Used to be used by LargeScopeRules old topMatch method of MJStarJoinIRule,
// not used anymore this code is OFF by default
MJRulesWA * JBBSubsetAnalysis::getMJRulesWA()
{
  if(!mjRulesWA_)
    mjRulesWA_ = new (CmpCommon::statementHeap()) MJRulesWA(this);
  return mjRulesWA_;
}

CANodeId MJRulesWA::computeCenterTable()
{
  if(centerTableComputed_)
  {
    return centerTable_;
  }

  //get CANodeIdSet representing this MultiJoin
  CANodeIdSet childSet = analysis_->getJBBCs();

  //Get a handle to ASM
  AppliedStatMan * appStatMan = QueryAnalysis::ASM();

  //n is the number of jbbcs (i.e. tables) in this multijoin
  UInt32 n = childSet.entries();

  CANodeId center = NULL_CA_ID;

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "There are "<<n<<" jbbcs"<<endl;
  }
#endif //_DEBUG

  // require atleast 3 tables
  if (n < 3)
  {
    centerTableComputed_ = TRUE;
    centerTable_ = NULL_CA_ID;
    centerTableConnectivity_ = 0;
    maxDimensionConnectivity_ = 0;
    return centerTable_;
  }

  // variable used in iteration
  CANodeId currentTable;

  // get a set of single row tables
  CANodeIdSet singleRowTables;

  // go over all the tables in the query
  // and create a set of tables that are
  // returning just one row
  for(currentTable = childSet.init();
      childSet.next(currentTable);
      childSet.advance(currentTable))
  {
    // get number of tables in this JBBC
    Int32 numSubtreeTables = currentTable.getNodeAnalysis()
                                      ->getJBBC()
                                      ->getSubtreeTables().entries();
    CostScalar jbbcCardinality = appStatMan->
                                 getStatsForCANodeId(currentTable)->
                                 getResultCardinality();
    //currentTable.getNodeAnalysis()->getCardinality();

    if((jbbcCardinality == 1) &&
       (numSubtreeTables < 2))
       singleRowTables.insert(currentTable);
  }

  n = n - singleRowTables.entries();

  if( n < 3)
  {
    centerTableComputed_ = TRUE;
    centerTable_ = NULL_CA_ID;
    centerTableConnectivity_ = 0;
    maxDimensionConnectivity_ = 0;
    return centerTable_;
  }

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "SingleRow JBBCs are "<<singleRowTables.getText()<<endl;
  }
#endif //_DEBUG

  // minimum connectivity for center
  UInt32 minimumCenterConnectivity = n-1;

  // maximum connectivity for dimensions
  UInt32 maximumDimensionConnectivity = 1;

  // connectivity of the center / fact table
  UInt32 centerConnectivity = 0;

  // gets the maximum number of tables connected
  // connected to a dimension table
  UInt32 maxDimConnectivity = 0;

  // figure out the parameters for matching the connectivity pattern
  switch (n)
  {
    case 3:
    case 4:
      // for n == 4 or n==3
      minimumCenterConnectivity = n-1;
      maximumDimensionConnectivity = 1;
      break;
    case 5:
      // for n == 5
      minimumCenterConnectivity = n-2;
      maximumDimensionConnectivity = 2;
      break;
    default:
      // for n >= 6
      minimumCenterConnectivity = n-3;
      // Changed COMP_INT_0 to COMP_INT_20 to avoid conflict with Nice context changes, with this
      // Nice Context feature is OFF by default. (06/07/06)
      maximumDimensionConnectivity = (ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_20);
      break;
  }

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "Pattern match parameters are: "<<endl;
    CURRCONTEXT_OPTDEBUG->stream() << "Min center connectivity: "<<minimumCenterConnectivity<<endl;
    CURRCONTEXT_OPTDEBUG->stream() << "Max dimension connectivity: "<<maximumDimensionConnectivity<<endl;
    CURRCONTEXT_OPTDEBUG->stream() << endl;
  }
#endif //_DEBUG


  // get the fact table name if a fact table has been forced
  const char* factTableName = ActiveSchemaDB()->getDefaults().
                                getValue(COMP_STRING_1);

  // variable that tells us that the
  // table we want to be the fact table
  // is found in the multijoin
  Int32 centerTableForced = 0;

  // The loop below tries to match the connectivity pattern
  // iterate over all the jbbcs (i.e. tables)
  // in this multijoin
  // In each iteration
  // Figure out:
  // * if this table is the center/fact table
  // * if this table is a dimension table
  for(currentTable = childSet.init();
      childSet.next(currentTable);
      childSet.advance(currentTable))
  {
    // if factTableName is NONE (default value),
    // then we don't want to force a particular
    // table to be the fact table
    if(stricmp(factTableName,"NONE") != 0)
    {
      //if this is not a table move on to the next node
      if(!currentTable.getNodeAnalysis()->getTableAnalysis())
      {
        // compare to currentTable's name
        // if comparison is successful we get a zero
        // therefore reverse the result
        centerTableForced = !(currentTable.getNodeAnalysis()->
                                           getTableAnalysis()->
                                           getTableDesc()->
                                           getCorrNameObj().\
                                           getCorrNameAsString().\
                                           compareTo(factTableName,
                                                     NAString::ignoreCase));
        // we found a the table that we wanted
        // force as the fact table
        if (centerTableForced)
        {
          center = currentTable;
          centerConnectivity = center.getNodeAnalysis()->
                                      getJBBC()->
                                      getJoinedJBBCs().
                                      entries();
        }
      }
    }

    //all JBBCs connect to current table
    CANodeIdSet connectedChildren = currentTable.getNodeAnalysis()->getJBBC()->getJoinedJBBCs();

    // get number of tables connected to current jbbc
    UInt32 numConnectedChildren = connectedChildren.entries();

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "JBBC: "<<currentTable.getText()<<endl;
    CURRCONTEXT_OPTDEBUG->stream() << "connected jbbcs: "<<connectedChildren.getText()<<endl;
    CURRCONTEXT_OPTDEBUG->stream() << endl;
  }
#endif //_DEBUG

    // if current jbbc is:
    // * connected to at least minimumCenterConnectivity jbbcs
    // * has a connectivity greater than the connectivity
    //   of the current center/fact table
    // then set this jbbc to be the center/fact table
    if ((numConnectedChildren >= minimumCenterConnectivity) &&
        (centerConnectivity < numConnectedChildren) &&
        (currentTable.getNodeAnalysis()->getTableAnalysis()) &&
        (!centerTableForced))
    {
      // see we are going have found a new center
      // the previous center should be a dimension
      // make sure it is not connected to more than
      // the allowed number of connected tables for
      // dimension tables.
      // If the previous center is connected to a
      // greater number of tables than allowed for
      // a dimension table then fail pattern match
      if (centerConnectivity > maximumDimensionConnectivity)
      {
	centerTableComputed_ = TRUE;
        centerTable_ = NULL_CA_ID;
        centerTableConnectivity_ = 0;
        maxDimensionConnectivity_ = 0;
        return centerTable_;
      }

      center = currentTable;
      centerConnectivity = numConnectedChildren;
    }
    else{

      // this table is potentially a dimension table
      // make sure it is not connected to more than
      // the allowed number of tables

      // Ignore tables that return 1 row when counting
      // the connected tables
      UInt32 numNonSingleRowConnectedChildren =
        connectedChildren.subtractSet(singleRowTables).entries();

      // if the number of connected children is greater than
      // the maximum connectivity of any dimension table upto
      // this point, then update the maximum connectivity
      if (numNonSingleRowConnectedChildren > maxDimConnectivity)
        maxDimConnectivity = numNonSingleRowConnectedChildren;

      // if this table is connected to more than the maximum
      // number of tables allowed for a dimension table
      // then fail the pattern match
      if ( numNonSingleRowConnectedChildren > maximumDimensionConnectivity)
      {
	centerTableComputed_ = TRUE;
        centerTable_ = NULL_CA_ID;
        centerTableConnectivity_ = 0;
        maxDimensionConnectivity_ = 0;
        return centerTable_;
      }
    }
  }

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    if(center != NULL_CA_ID)
    {
      CURRCONTEXT_OPTDEBUG->stream() << "Center Table is: "<<center.getText()<<endl;
      CURRCONTEXT_OPTDEBUG->stream() << "Center connectivity is: "<<centerConnectivity<<endl;
      CURRCONTEXT_OPTDEBUG->stream() << "Max dimension connectivity is: "<<maxDimConnectivity<<endl;
      CURRCONTEXT_OPTDEBUG->stream() << endl;
    }
    else{
      CURRCONTEXT_OPTDEBUG->stream() << "No center table found "<<center.getText()<<endl;
    }
  }
#endif //_DEBUG

  // we did not find a center table
  if (center == NULL_CA_ID)
  {
    centerTableComputed_ = TRUE;
    centerTable_ = NULL_CA_ID;
    centerTableConnectivity_ = 0;
    maxDimensionConnectivity_ = 0;
    return centerTable_;
  }

  // get rows to be scanned for center table
  CostScalar centerTableRowsToBeScanned =
    appStatMan->getStatsForLocalPredsOnCKPOfJBBC(center)->
                getResultCardinality();

  // get record size for center table
  CostScalar centerTableRecordSize =
    center.getNodeAnalysis()->
           getTableAnalysis()->
           getTableDesc()->
           getNATable()->
           getRecordLength();

  // compute amount of data to be scanned from center table
  // amount is in KB
  CostScalar centerTableDataToBeScanned =
    (centerTableRowsToBeScanned * centerTableRecordSize) / 1024;

  // get number of partitions for the center table
  CostScalar numCenterTablePartitions =
    center.getNodeAnalysis()->
           getTableAnalysis()->
           getTableDesc()->
           getClusteringIndex()->
           getNAFileSet()->
           getCountOfPartitions();

  CostScalar sizePerPartition = centerTableDataToBeScanned / numCenterTablePartitions;

  centerTableComputed_ = TRUE;
  centerTable_ = center;
  centerTableConnectivity_ = centerConnectivity;
  maxDimensionConnectivity_ = maxDimConnectivity;
  centerTableRowsScanned_ = centerTableRowsToBeScanned;
  centerTableDataScanned_ = centerTableDataToBeScanned;
  centerTablePartitions_ = numCenterTablePartitions;
  centerTableDataPerPartition_ = sizePerPartition;


#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "Center Table Rows Scanned: "\
                    <<centerTableRowsScanned_.getValue()<<endl;
    CURRCONTEXT_OPTDEBUG->stream() << "Center Table Record Size: "\
                    <<centerTableRecordSize.getValue()<<endl;
    CURRCONTEXT_OPTDEBUG->stream() << "Center Table Data Scanned: "\
                    <<centerTableDataScanned_.getValue()<<endl;
    CURRCONTEXT_OPTDEBUG->stream() << "Center Table Num partitions: "\
                    <<centerTablePartitions_.getValue()<<endl;
    CURRCONTEXT_OPTDEBUG->stream() << "Center Table size per partition: "\
                    <<centerTableDataPerPartition_.getValue()<<endl;
  }
#endif //_DEBUG

  return centerTable_;
}
// LCOV_EXCL_STOP

CASortedList * JBBSubsetAnalysis::getNodesSortedByLocalPredsCard()
{
  // get CANodeIdSet representing this MultiJoin
  CANodeIdSet childSet = getJBBCs();

  JBBWA * jbbWA = jbb_->getJBBWA();

  const CASortedList * sortedListOfNodes =
                                   jbbWA->
                                     getNodesSortedByLocalPredsCard();

  CASortedList * result = new (CmpCommon::statementHeap())
    CASortedList(CmpCommon::statementHeap(), childSet.entries());

  for( UInt32 i = 0;
       i < sortedListOfNodes->entries();
       i++)
  {
    if(childSet.contains((*sortedListOfNodes)[i]))
      (*result).insert((*sortedListOfNodes)[i]);
  }

  return result;
}

// LCOV_EXCL_START :cnu
CASortedList * JBBSubsetAnalysis::getNodesSortedByLocalKeyPrefixPredsCard()
{
  // get CANodeIdSet representing this MultiJoin
  CANodeIdSet childSet = getJBBCs();

  JBBWA * jbbWA = jbb_->getJBBWA();

  const CASortedList * sortedListOfNodes =
                                   jbbWA->
                                     getNodesSortedByLocalKeyPrefixPredsCard();

  CASortedList * result = new (CmpCommon::statementHeap())
    CASortedList(CmpCommon::statementHeap(), childSet.entries());

  for( UInt32 i = 0;
       i < sortedListOfNodes->entries();
       i++)
  {
    if(childSet.contains((*sortedListOfNodes)[i]))
      (*result).insert((*sortedListOfNodes)[i]);
  }

  return result;
}

CANodeId JBBSubsetAnalysis::getLargestNode()
{
  // get CANodeIdSet representing this JBBSubset
  CANodeIdSet childSet = getJBBCs();

  JBBWA * jbbWA = jbb_->getJBBWA();

  const CASortedList * sortedListOfNodes = jbbWA->getNodesSortedByOutputData();

  for( UInt32 i = 0;
       i < sortedListOfNodes->entries();
       i++)
  {
    if(childSet.contains((*sortedListOfNodes)[i]))
      return (*sortedListOfNodes)[i];
  }

  return NULL_CA_ID;
}
// LCOV_EXCL_STOP

CANodeId JBBSubsetAnalysis::getLargestIndependentNode()
{
  if(computedLargestIndependentNode_)
    return largestIndependentNode_;

  // get CANodeIdSet representing this JBBSubset
  CANodeIdSet childSet = getJBBCs();

  JBBWA * jbbWA = jbb_->getJBBWA();

  const CASortedList * sortedListOfNodes = jbbWA->getNodesSortedByOutputData();

  for( UInt32 i = 0;
       i < sortedListOfNodes->entries();
       i++)
  {
    if(childSet.contains((*sortedListOfNodes)[i]))
    {
      CANodeId nodeId = (*sortedListOfNodes)[i];
      JBBC * nodeJBBC = nodeId.getNodeAnalysis()->getJBBC();
      Join * parentJoin = nodeJBBC->getOriginalParentJoin();
      if ((!parentJoin) ||
          (parentJoin->isInnerNonSemiNonTSJJoin()))
      {
        largestIndependentNode_ = nodeId;
        computedLargestIndependentNode_ = TRUE;
        return nodeId;
      }
      /*
      if(!(nodeJBBC->getPredecessorJBBCs().entries()))
        return nodeId;
      */
    }
  }

  largestIndependentNode_ = NULL_CA_ID;
  computedLargestIndependentNode_ = TRUE;
  return NULL_CA_ID;
}

void JBBSubsetAnalysis::analyzeInitialPlan(MultiJoin * mjoin)
{
  CMPASSERT(mjoin);

  // get the subset analysis for this MultiJoin
  JBBSubsetAnalysis * subsetAnalysis = mjoin->getJBBSubset().getJBBSubsetAnalysis();

  // makesure it is the same as 'this'
  CMPASSERT(this==subsetAnalysis);

  analyzeInitialPlan();

}

void JBBSubsetAnalysis::analyzeInitialPlan()
{
  if(analyzedInitialPlan_)
    return;

  CANodeId factTable = NULL_CA_ID;
  CANodeId largestTable = NULL_CA_ID;
  CostScalar factTableCKPrefixCardinality;

  factTable = findFactAndLargestTable(factTableCKPrefixCardinality,
                                      largestTable);

  // if we did not find a fact table then look for the center table
  if (factTable == NULL_CA_ID)
  {

    CANodeId centerTable = computeCenterTable();
    CostScalar centerTableSize = getCenterTableSize();//centerTableDataScanned_;
    CostScalar centerTableSizePerPartition =
      getCenterTableSizePerPartition();//centerTableDataPerPartition_;
    CostScalar centerTablePartitions = getCenterTablePartitions();//centerTablePartitions_;

    if(centerTable != NULL_CA_ID)
    {
      JBBC * centerTableJBBC = centerTable.getNodeAnalysis()->getJBBC();

      if(!(centerTableJBBC->getPredecessorJBBCs().entries()))
      {
        if(centerTable == largestTable)
          factTable = centerTable;
        else
        {
          if((centerTableSizePerPartition >
              ((ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_16)*1024))||
             (centerTablePartitions >
              (ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_17)))
          {
            factTable = centerTable;
          }
        }
      }
    }
  }

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "FactTable: " << factTable.getText() << endl;
    CURRCONTEXT_OPTDEBUG->stream() << endl;
  }
#endif

  // -----------------------------------------------------
  // Begin: Find if there is a full outer join or TSJ node
  // -----------------------------------------------------

  //get CANodeIdSet representing this MultiJoin
  CANodeIdSet childSet = getJBBCs();
  NABoolean hasFullOuterJoinOrTSJ = FALSE;

  CANodeId currentNode = NULL_CA_ID;
  for(currentNode=childSet.init();
      childSet.next(currentNode);
      childSet.advance(currentNode))
  {
    JBBC * currentJBBC = currentNode.getNodeAnalysis()->getJBBC();
    if (currentJBBC->isFullOuterJoinOrTSJJBBC() &&
       !(currentJBBC->getOriginalParentJoin()))
    {
      hasFullOuterJoinOrTSJ = TRUE;
    }
  }

  // ---------------------------------------------
  // End: Find if there is a full outer join node
  // ---------------------------------------------

  // check to see if this subset matches a star pattern
  // this means that the fact table has good key prefix
  // access via some dimension tables
  // If there is a good key prefix coverage this method
  // will find the set of dimension tables that should
  // be used to probe the fact table.
  if ((!hasFullOuterJoinOrTSJ) &&
      (factTable != NULL_CA_ID ) &&
      isAStarPattern(factTable, factTableCKPrefixCardinality))
  {
// LCOV_EXCL_START 
// for debugging only
#ifdef _DEBUG
    if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
         CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
    {
      CURRCONTEXT_OPTDEBUG->stream() << "JBBSubsetAnalysis StarJoinTypeI feasible" <<endl;
    }
#endif //_DEBUG
// LCOV_EXCL_STOP
    factTable_ = factTable;
    setStarJoinTypeIFeasible();
    arrangeTablesAfterFactForStarJoinTypeI();
  }
  else{
#ifdef _DEBUG
    if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
         CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
    {
      CURRCONTEXT_OPTDEBUG->stream() << "JBBSubsetAnalysis StarJoinTypeI not feasible" <<endl;
    }
#endif //_DEBUG
    arrangeTablesAfterFactForStarJoinTypeII();
  }

  // recursively call analyzeInitialPlan on any children
  // that are JBBSubsets i.e. MultiJoins
  UInt32 numJoinChildren = leftDeepJoinSequence_.entries();

  for (UInt32 i = 0; i < numJoinChildren; i++)
  {
    CANodeIdSet joinChild = leftDeepJoinSequence_[i];

    if (joinChild.entries() > 1)
    {
      JBBSubset * childSet = joinChild.jbbcsToJBBSubset();
      childSet->
        getJBBSubsetAnalysis()->
          analyzeInitialPlan();
    }
  }

  analyzedInitialPlan_ = TRUE;

}

// compute the resources required for this subset
void JBBSubsetAnalysis::computeRequiredResources(
  MultiJoin * mjoin,
  RequiredResources & reqResources,
  EstLogPropSharedPtr & inLP)

{
  // makesure initial plan has been analyzed
  CMPASSERT(analyzedInitialPlan_);

  CostScalar A = csOne;
  CostScalar B (getDefaultAsDouble(WORK_UNIT_ESP_DATA_COPY_COST));

  //Get a handle to ASM
  AppliedStatMan * appStatMan = QueryAnalysis::ASM();

  UInt32 numJoinChildren = leftDeepJoinSequence_.entries();

  CANodeIdSet joinOuterChildren = getJBBCs();

  CostScalar joinMaxCard = csZero;
  CostScalar maxCard = csZero;

  // iterate over all children except the last child
  UInt32 i = 0;
  for (i = 0; i < (numJoinChildren-1); i++)
  {
    // get join's max cardinality
    joinMaxCard = appStatMan->
                 getStatsForCANodeIdSet(joinOuterChildren)-> getMaxCardEst();

    CostScalar memoryResourcesRequired  = csZero;
    CostScalar cpuResourcesRequired = csZero;

    CANodeIdSet joinChild = leftDeepJoinSequence_[i];
    // get all the nodes on the left of the join
    joinOuterChildren -= joinChild;

    CostScalar innerChildCardinality = csZero;
    CostScalar innerChildRecordSize = csZero;
    CostScalar innerChildMaxCardinality = csZero;
    CostScalar outerChildCardinality = csZero;
    CostScalar outerChildRecordSize = csZero;
    CostScalar outerChildMaxCardinality = csZero;

    CostScalar joinChildDataAccessCost = csZero;

    // if child is a MultiJoin
    if (joinChild.entries() > 1)
    {
      JBBSubset * innerChildSet = joinChild.jbbcsToJBBSubset();
      MultiJoin * innerChildMJ = mjoin->createSubsetMultiJoin(*innerChildSet);
      JBBSubsetAnalysis * innerChildAnalysis = innerChildSet->getJBBSubsetAnalysis();
      innerChildAnalysis->computeRequiredResources(innerChildMJ,reqResources, inLP);
      innerChildCardinality =  appStatMan->
                                 getStatsForCANodeIdSet(joinChild)->
                                   getResultCardinality();

      innerChildMaxCardinality =  appStatMan->
                                 getStatsForCANodeIdSet(joinChild)->
                                   getMaxCardEst();

      innerChildRecordSize = innerChildMJ->getGroupAttr()->getCharacteristicOutputs().getRowLength();

    }
    // child is a single node
    else if (!joinChild.contains(factTable_) || !starJoinTypeIFeasible())
    {
      CANodeId innerChildNode = joinChild.getFirst();
      innerChildCardinality = appStatMan->
                                getStatsForCANodeId(innerChildNode)->
                                  getResultCardinality();

      innerChildMaxCardinality = appStatMan->
                                getStatsForCANodeId(innerChildNode)->
                                  getMaxCardEst();

      RelExpr * innerChildExpr = innerChildNode.getNodeAnalysis()->getOriginalExpr();
      innerChildRecordSize = innerChildExpr->getGroupAttr()->getCharacteristicOutputs().getRowLength();
    }

    if (joinOuterChildren.entries() > 1)
    {
      JBBSubset * outerChildSet = joinOuterChildren.jbbcsToJBBSubset();
      MultiJoin * outerChildMJ = mjoin->createSubsetMultiJoin(*outerChildSet);
      JBBSubsetAnalysis * outerChildAnalysis = outerChildSet->getJBBSubsetAnalysis();
      outerChildCardinality =  appStatMan->
                                 getStatsForCANodeIdSet(joinOuterChildren)->
                                   getResultCardinality();

      outerChildMaxCardinality =  appStatMan->
                                 getStatsForCANodeIdSet(joinOuterChildren)->
                                   getMaxCardEst();

      outerChildRecordSize = outerChildMJ->getGroupAttr()->getCharacteristicOutputs().getRowLength();
    }
    else{
      CANodeId outerChildNode = joinOuterChildren.getFirst();
      outerChildCardinality = appStatMan->
                                getStatsForCANodeId(outerChildNode)->
                                  getResultCardinality();

      outerChildMaxCardinality = appStatMan->
                                getStatsForCANodeId(outerChildNode)->
                                  getMaxCardEst();

      RelExpr * outerChildExpr = outerChildNode.getNodeAnalysis()->getOriginalExpr();
      outerChildRecordSize = outerChildExpr->getGroupAttr()->getCharacteristicOutputs().getRowLength();
    }

    if (maxCard < joinMaxCard)
      maxCard = joinMaxCard;

    if (maxCard < innerChildMaxCardinality)
      maxCard = innerChildMaxCardinality;

    if (maxCard < outerChildMaxCardinality)
      maxCard = outerChildMaxCardinality;

    memoryResourcesRequired  += (innerChildCardinality * innerChildRecordSize);

    cpuResourcesRequired +=
      (A * innerChildCardinality) +
      (B * innerChildCardinality * innerChildRecordSize );

    cpuResourcesRequired +=
      (A * outerChildCardinality) +
      (B * outerChildCardinality * outerChildRecordSize );

    reqResources.accumulate(memoryResourcesRequired, cpuResourcesRequired, csZero, maxCard);
  }

  CANodeIdSet outerMostChild = leftDeepJoinSequence_[numJoinChildren-1];
  if (outerMostChild.entries() > 1)
  {
    JBBSubset * childSet = outerMostChild.jbbcsToJBBSubset();
    MultiJoin * childMJ = mjoin->createSubsetMultiJoin(*childSet);
    JBBSubsetAnalysis * childAnalysis = childSet->getJBBSubsetAnalysis();
    childAnalysis->computeRequiredResources(childMJ,reqResources, inLP);
  }

}

CANodeId JBBSubsetAnalysis::findFactTable(
  CANodeIdSet childSet,
  CostScalar & factTableCKPrefixCardinality,
  CANodeId & biggestTable)
{
  // The fact table should be the table that has significantly more data read
  // from it as compared to all the other tables in a multi-join. The data read
  // is measured by the number of rows read by a scan of the table multiplied
  // by the record size. The number of rows read by a  scan is not the number
  // of rows returned by the scan, rather all the rows read from the table based
  // on clustering key prefix predicates.
  // Hence anytime the word cardinality is used in this method, it refers to the
  // cardinality of a table after application of local predicates on the prefix
  // of the clustering key

  //Get a handle to ASM
  AppliedStatMan * appStatMan = QueryAnalysis::ASM();

  CANodeId factTable = NULL_CA_ID;

  const char* factTableName = ActiveSchemaDB()->getDefaults().
                                getValue(COMP_STRING_1);

  // if the user is trying to force a certain table to be considered
  // the fact table
  if(stricmp(factTableName,"NONE") != 0)
  {
    //try to match the factTable Name to the table names of the JBBCs

    CANodeId currentTable = NULL_CA_ID;
    for(currentTable=childSet.init();
        childSet.next(currentTable);
        childSet.advance(currentTable))
    {
      //if this is not a table move on to the next node
      if(!currentTable.getNodeAnalysis()->getTableAnalysis())
        continue;

      JBBC * tableJBBC = currentTable.getNodeAnalysis()->getJBBC();

      // fact table cannot depend on any other table
      if(tableJBBC->getPredecessorJBBCs().entries())
        continue;

      //compare to currentTable's name
      Int32 comparison_result = currentTable.getNodeAnalysis()->
                                           getTableAnalysis()->
                                           getTableDesc()->
                                           getCorrNameObj().\
                                           getCorrNameAsString().\
                                           compareTo(factTableName,
                                                     NAString::ignoreCase);
      if(comparison_result == 0)
      {
        factTable = currentTable;

        // get fact table cardinality after application
        // of local key prefix preds
        factTableCKPrefixCardinality =
        appStatMan->
          getStatsForLocalPredsOnCKPOfJBBC(factTable)->
            getResultCardinality();

#ifdef _DEBUG
        if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
             CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
        {
// LCOV_EXCL_START :dpm
          CURRCONTEXT_OPTDEBUG->stream() << "Picked the fact Table specified by user" << endl;
          CURRCONTEXT_OPTDEBUG->stream() << "fact Table: " << factTable.getText() << endl;
          CURRCONTEXT_OPTDEBUG->stream() << "fact Table  num rows scanned: ";
          CURRCONTEXT_OPTDEBUG->stream() << istring(Lng32(factTableCKPrefixCardinality.value()))\
                          << endl;
// LCOV_EXCL_STOP
        }
#endif
        break;
      }

    }
  }

  // the largest table (i.e. greatest data to be read) after application of local
  // predicates on prefix of clustering key
  CANodeId largestTable = NULL_CA_ID;
  CostScalar largestTableTotalDataVol;
  CostScalar largestTableCardinality;

#ifdef _DEBUG
  // these are for debugging
  CostScalar largestTableActualRecordSize(0);
  CostScalar largestTableEffectiveRecordSize(0);
#endif //_DEBUG

  // second largest table after application of local predicates on prefix of
  // clustering key
  CANodeId secondLargestTable = NULL_CA_ID;
  CostScalar secondLargestTableTotalDataVol;
  CostScalar secondLargestTableCardinality;

#ifdef _DEBUG
  // these are for debugging
  CostScalar secondLargestTableActualRecordSize(0);
  CostScalar secondLargestTableEffectiveRecordSize(0);
#endif //_DEBUG

  UInt32 recordSizeCap =
    (ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_4);

  CostScalar maxRecordSize(recordSizeCap);

  CostScalar sumOfTotalDataVol(csZero);
  CostScalar numTablesInCurrentMultiJoin(csZero);

  UInt32 numChildren = childSet.entries();

  CostScalar PAtoScanRatio =
    (ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_45);

  // Flag indicating if we got the following info for independent tables i.e.
  // tables that are join via inner-non-semi joins
  // * numTablesInCurrentMultiJoin
  // * sumOfTotalDataVol
  NABoolean gotTableStats = FALSE;

  //The following loop performs the sorting of the JBBCs in
  //this multi-join based on the cardinality after application
  //of local predicates on clustering key prefix
  for(UInt32 i = 0; i < numChildren; i++)
  {
    //To start with, pick an arbitrary child as largest
    CANodeId largestChild = childSet.init();
    childSet.next(largestChild);

    if(!largestChild.getNodeAnalysis()->getTableAnalysis())
    {
      childSet.remove(largestChild);
      continue;
    }

    JBBC * largestChildJBBC = largestChild.getNodeAnalysis()->getJBBC();

    Join * largestChildParentJoin = largestChildJBBC->getOriginalParentJoin();

    // fact table cannot depend on any other table
    if (largestChildParentJoin &&
        !(largestChildParentJoin->isInnerNonSemiNonTSJJoin()))
    {
      childSet.remove(largestChild);
      continue;
    }

    //get cardinality
    EstLogPropSharedPtr estLogProps = appStatMan->getStatsForLocalPredsOnCKPOfJBBC(largestChild);

    // cardinality of largest child
    CostScalar largestChildCardinality = estLogProps->getResultCardinality();

    // record size of largest child
    CostScalar largestChildRecordSize = largestChild.getNodeAnalysis()->
                                                     getTableAnalysis()->
                                                     getTableDesc()->
                                                     getNATable()->
                                                     getRecordLength();

    estLogProps = appStatMan->getStatsForCANodeId(largestChild);
    CostScalar largestChildOutputCard = estLogProps->getResultCardinality();
    RowSize largestChildOutputRowSize = largestChild.getNodeAnalysis()->
                                    getOriginalExpr()->
                                      getGroupAnalysis()->
                                        getGroupAttr()->
                                          getRecordLength();

#ifdef _DEBUG
    // This is for debugging
    CostScalar largestChildActualRecordSize = largestChildRecordSize;
#endif //_DEBUG

    if(largestChildRecordSize > maxRecordSize)
      largestChildRecordSize = maxRecordSize;

#ifdef _DEBUG
    // This is for debugging
    CostScalar largestChildEffectiveRecordSize = largestChildRecordSize;
#endif //_DEBUG

    CostScalar largestChildSubsetSize = largestChildCardinality *
                                        largestChildRecordSize;

    CostScalar largestChildDataRetrievedSize = largestChildOutputCard *
                                        largestChildOutputRowSize;

    CostScalar largestChildTotalDataVol = largestChildSubsetSize +
      PAtoScanRatio * largestChildDataRetrievedSize / 100.0;

    for( CANodeId child = largestChild; childSet.next(child); childSet.advance(child))
    {
      //don't execute the following code if child is not a table
      if(!child.getNodeAnalysis()->getTableAnalysis())
        continue;

     JBBC * childJBBC = child.getNodeAnalysis()->getJBBC();

     Join * childParentJoin = childJBBC->getOriginalParentJoin();

     // fact table cannot depend on any other table
     if (childParentJoin &&
         !(childParentJoin->isInnerNonSemiNonTSJJoin()))
       continue;

     /*
     // fact table cannot depend on any other table
     if(childJBBC->getPredecessorJBBCs().entries())
       continue;
     */

      // Current child is a table increment the count
      if(!gotTableStats)
        numTablesInCurrentMultiJoin++;

      //get cardinality of child after application of local preds on
      //clustering key prefix
      estLogProps = appStatMan->getStatsForLocalPredsOnCKPOfJBBC(child);
      CostScalar childCardinality = estLogProps->getResultCardinality();

      //Record Size of the factTable
      CostScalar childRecordSize = child.getNodeAnalysis()->
                                         getTableAnalysis()->
                                         getTableDesc()->
                                         getNATable()->
                                         getRecordLength();

#ifdef _DEBUG
      CostScalar childActualRecordSize = childRecordSize;
#endif //_DEBUG

      if(childRecordSize > maxRecordSize)
        childRecordSize = maxRecordSize;

#ifdef _DEBUG
      CostScalar childEffectiveRecordSize = childRecordSize;
#endif //_DEBUG

      CostScalar childSubsetSize = childCardinality * childRecordSize;
      
      estLogProps = appStatMan->getStatsForCANodeId(child);
      CostScalar childOutputCard = estLogProps->getResultCardinality();
      RowSize childOutputRowSize = child.getNodeAnalysis()->
                               getOriginalExpr()->
                                 getGroupAnalysis()->
                                   getGroupAttr()->
                                     getRecordLength();
      CostScalar childDataRetrievedSize = childOutputCard * childOutputRowSize;
      CostScalar childTotalDataVol = childSubsetSize +
        PAtoScanRatio * childDataRetrievedSize / 100.0;

      if(!gotTableStats)
        sumOfTotalDataVol += childTotalDataVol;

      //if this child's TotalDataVol is larger than the largest child's
      //TotalDataVol, set this child to be the largest
      if( childTotalDataVol >= largestChildTotalDataVol)
      {
        largestChild = child;
        largestChildCardinality = childCardinality;
        largestChildTotalDataVol=childTotalDataVol;

#ifdef _DEBUG
        // These are for debugging
        largestChildActualRecordSize = childActualRecordSize;
        largestChildEffectiveRecordSize = childEffectiveRecordSize;
#endif //_DEBUG
      }
    }

    // we got the table stats (not histograms but the stats mentioned above)
    gotTableStats = TRUE;

    //remove the largest child from the child set
    childSet.remove(largestChild);

    if ((largestTable != NULL_CA_ID) &&
        (largestChild.getNodeAnalysis()->getTableAnalysis()))
    {
      secondLargestTable = largestChild;
      secondLargestTableCardinality = largestChildCardinality;
      secondLargestTableTotalDataVol = largestChildTotalDataVol;

#ifdef _DEBUG
      // These are for debugging
      secondLargestTableActualRecordSize = largestChildActualRecordSize;
      secondLargestTableEffectiveRecordSize = largestChildEffectiveRecordSize;
#endif //_DEBUG
      break;
    }

    // get the fact Table
    if ((largestTable == NULL_CA_ID) &&
        (largestChild.getNodeAnalysis()->getTableAnalysis()))
    {
      largestTable = largestChild;
      largestTableCardinality = largestChildCardinality;
      largestTableTotalDataVol = largestChildTotalDataVol;

#ifdef _DEBUG
      //These are for debugging
      largestTableActualRecordSize = largestChildActualRecordSize;
      largestTableEffectiveRecordSize = largestChildEffectiveRecordSize;
#endif //_DEBUG
    }
  }

  biggestTable = largestTable;

  if ((secondLargestTable == NULL_CA_ID) &&
      (largestTable != NULL_CA_ID))
  {
    // get CANodeIdSet representing this JBBSubset
    CANodeIdSet childSet = getJBBCs();
    
    JBBWA * jbbWA = jbb_->getJBBWA();
    
    const CASortedList * sortedListOfNodes = jbbWA->getNodesSortedByLocalKeyPrefixPredsCard();
    
    for( UInt32 i = 0;
         i < sortedListOfNodes->entries();
         i++)
    {
      if(childSet.contains((*sortedListOfNodes)[i]))
      {
        if((*sortedListOfNodes)[i] == largestTable)
        {
          factTableCKPrefixCardinality = largestTableCardinality;
          factTable = largestTable;
        }
        break;
      }
    }    

    return factTable;
  }

  CostScalar averageSizeOfAllTablesExceptLargest(csZero);

  sumOfTotalDataVol-=largestTableTotalDataVol;
  numTablesInCurrentMultiJoin-=1;

  if(numTablesInCurrentMultiJoin.isGreaterThanZero())
    averageSizeOfAllTablesExceptLargest=
      sumOfTotalDataVol / numTablesInCurrentMultiJoin;

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
// LCOV_EXCL_START :dpm
    CURRCONTEXT_OPTDEBUG->stream() << "Largest Table: " << largestTable.getText() << endl;
    CURRCONTEXT_OPTDEBUG->stream() << "Largest Table  num rows scanned: ";
    CURRCONTEXT_OPTDEBUG->stream() << istring(Lng32(largestTableCardinality.value()))\
                    << endl;
    CURRCONTEXT_OPTDEBUG->stream() << "Largest Table actual rowsize: ";
    CURRCONTEXT_OPTDEBUG->stream() << istring(Lng32(largestTableActualRecordSize.value()))\
                    << endl;
    CURRCONTEXT_OPTDEBUG->stream() << "Largest Table effective rowsize: ";
    CURRCONTEXT_OPTDEBUG->stream() << istring(Lng32(largestTableEffectiveRecordSize.value()))\
                    << endl;
    CURRCONTEXT_OPTDEBUG->stream() << "Largest Table data scanned: ";
    CURRCONTEXT_OPTDEBUG->stream() << istring(Lng32(largestTableTotalDataVol.value()))\
                    << endl;
    CURRCONTEXT_OPTDEBUG->stream() << "Average size of all tables apart from largest: ";
    CURRCONTEXT_OPTDEBUG->stream() << istring(Lng32(averageSizeOfAllTablesExceptLargest.value()))\
                    << endl;
    CURRCONTEXT_OPTDEBUG->stream() << "Second Largest Table is: " << secondLargestTable.getText() <<endl;
    CURRCONTEXT_OPTDEBUG->stream() << "Second Largest Table num rows scanned: ";
    CURRCONTEXT_OPTDEBUG->stream() << istring(Lng32(secondLargestTableCardinality.value()))\
                    << endl;
    CURRCONTEXT_OPTDEBUG->stream() << "Second Largest Table actual rowsize: ";
    CURRCONTEXT_OPTDEBUG->stream() << istring(Lng32(secondLargestTableActualRecordSize.value()))\
                    << endl;
    CURRCONTEXT_OPTDEBUG->stream() << "Second Largest Table effective rowsize: ";
    CURRCONTEXT_OPTDEBUG->stream() << istring(Lng32(secondLargestTableEffectiveRecordSize.value()))\
                    << endl;
    CURRCONTEXT_OPTDEBUG->stream() << "Second Largest Table data scanned: ";
    CURRCONTEXT_OPTDEBUG->stream() << istring(Lng32(secondLargestTableTotalDataVol.value()))\
                    << endl;
    CURRCONTEXT_OPTDEBUG->stream() << endl;
// LCOV_EXCL_STOP
  }
#endif //_DEBUG

  UInt32 factTableFactor =
    (ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_2);

  //check if the largest table can be considered a fact table
  if (((largestTableTotalDataVol/secondLargestTableTotalDataVol) >= factTableFactor) &&
      (factTable == NULL_CA_ID))
  {
    factTableCKPrefixCardinality = largestTableCardinality;
    factTable = largestTable;
  }

  //if we want to force a star join plan
  //This has to be done otherwise the star join pattern is
  //not considered a match without a fact table
  if (((CmpCommon::getDefault(DIMENSIONAL_QUERY_OPTIMIZATION) == DF_ON) ||
       (CmpCommon::getDefault(COMP_BOOL_13) == DF_ON)) &&
      (factTable == NULL_CA_ID))
    factTable = largestTable;

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "FactTable chosen: " << factTable.getText() << endl;
  }
#endif //_DEBUG

  return factTable;

};//JBBSubsetAnalysis::findFactTable

CANodeId JBBSubsetAnalysis::findFactAndLargestTable(
  CostScalar & factTableCKPrefixCardinality,
  CANodeId & biggestTable)
{

  if (computedFactAndLargestTable_)
  {
    biggestTable = largestTable_;
    return factTable_;
  }

  //get CANodeIdSet representing this MultiJoin
  CANodeIdSet childSet = getJBBCs();

  CANodeId factTable = findFactTable
    ( childSet, factTableCKPrefixCardinality, biggestTable );

  largestTable_ = biggestTable;
  factTable_ = factTable;

  computedFactAndLargestTable_ = TRUE;

  return factTable;

};//JBBSubsetAnalysis::findFactAndLargestTable

CANodeId JBBSubsetAnalysis::computeCenterTable()
{
  if(centerTableComputed_)
  {
    return centerTable_;
  }

  //get CANodeIdSet representing this MultiJoin
  CANodeIdSet childSet = getJBBCs();

  //Get a handle to ASM
  AppliedStatMan * appStatMan = QueryAnalysis::ASM();

  //n is the number of jbbcs (i.e. tables) in this multijoin
  UInt32 n = childSet.entries();

  CANodeId center = NULL_CA_ID;

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "There are "<<n<<" jbbcs"<<endl;
  }
#endif //_DEBUG

  // require atleast 3 tables
  if (n < 3)
  {
    centerTableComputed_ = TRUE;
    centerTable_ = NULL_CA_ID;
    centerTableConnectivity_ = 0;
    maxDimensionConnectivity_ = 0;
    return centerTable_;
  }

  // variable used in iteration
  CANodeId currentTable;

  // get a set of single row tables
  CANodeIdSet singleRowTables;

  // go over all the tables in the query
  // and create a set of tables that are
  // returning just one row
  for(currentTable = childSet.init();
      childSet.next(currentTable);
      childSet.advance(currentTable))
  {
    // get number of tables in this JBBC
    Int32 numSubtreeTables = currentTable.getNodeAnalysis()
                                      ->getJBBC()
                                      ->getSubtreeTables().entries();
    CostScalar jbbcCardinality = appStatMan->
                                 getStatsForCANodeId(currentTable)->
                                 getResultCardinality();
    //currentTable.getNodeAnalysis()->getCardinality();

    if((jbbcCardinality == 1) &&
       (numSubtreeTables < 2))
       singleRowTables.insert(currentTable);
  }

  n = n - singleRowTables.entries();

  if( n < 3)
  {
    centerTableComputed_ = TRUE;
    centerTable_ = NULL_CA_ID;
    centerTableConnectivity_ = 0;
    maxDimensionConnectivity_ = 0;
    return centerTable_;
  }

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "SingleRow JBBCs are "<<singleRowTables.getText()<<endl;
  }
#endif //_DEBUG

  // minimum connectivity for center
  UInt32 minimumCenterConnectivity = n-1;

  // maximum connectivity for dimensions
  UInt32 maximumDimensionConnectivity = 1;

  // connectivity of the center / fact table
  UInt32 centerConnectivity = 0;

  // gets the maximum number of tables connected
  // connected to a dimension table
  UInt32 maxDimConnectivity = 0;

  // figure out the parameters for matching the connectivity pattern
  switch (n)
  {
    case 3:
    case 4:
      // for n == 4 or n==3
      minimumCenterConnectivity = n-1;
      maximumDimensionConnectivity = 1;
      break;
    case 5:
      // for n == 5
      minimumCenterConnectivity = n-2;
      maximumDimensionConnectivity = 2;
      break;
    default:
      // for n >= 6
      minimumCenterConnectivity = n-3;
      // Changed COMP_INT_0 to COMP_INT_20 to avoid conflict with Nice context changes, with this
      // Nice Context feature is OFF by default. (06/07/06)
      maximumDimensionConnectivity = (ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_20);
      break;
  }

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
// LCOV_EXCL_START :dpm
    CURRCONTEXT_OPTDEBUG->stream() << "Pattern match parameters are: "<<endl;
    CURRCONTEXT_OPTDEBUG->stream() << "Min center connectivity: "<<minimumCenterConnectivity<<endl;
    CURRCONTEXT_OPTDEBUG->stream() << "Max dimension connectivity: "<<maximumDimensionConnectivity<<endl;
    CURRCONTEXT_OPTDEBUG->stream() << endl;
// LCOV_EXCL_STOP
  }
#endif //_DEBUG


  // get the fact table name if a fact table has been forced
  const char* factTableName = ActiveSchemaDB()->getDefaults().
                                getValue(COMP_STRING_1);

  // variable that tells us that the
  // table we want to be the fact table
  // is found in the multijoin
  Int32 centerTableForced = 0;

  // The loop below tries to match the connectivity pattern
  // iterate over all the jbbcs (i.e. tables)
  // in this multijoin
  // In each iteration
  // Figure out:
  // * if this table is the center/fact table
  // * if this table is a dimension table
  for(currentTable = childSet.init();
      childSet.next(currentTable);
      childSet.advance(currentTable))
  {
    // if factTableName is NONE (default value),
    // then we don't want to force a particular
    // table to be the fact table
    if(stricmp(factTableName,"NONE") != 0)
    {
      //if this is not a table move on to the next node
      if(!currentTable.getNodeAnalysis()->getTableAnalysis())
      {
        // compare to currentTable's name
        // if comparison is successful we get a zero
        // therefore reverse the result
        centerTableForced = !(currentTable.getNodeAnalysis()->
                                           getTableAnalysis()->
                                           getTableDesc()->
                                           getCorrNameObj().\
                                           getCorrNameAsString().\
                                           compareTo(factTableName,
                                                     NAString::ignoreCase));
        // we found a the table that we wanted
        // force as the fact table
        if (centerTableForced)
        {
          center = currentTable;
          centerConnectivity = center.getNodeAnalysis()->
                                      getJBBC()->
                                      getJoinedJBBCs().
                                      entries();
        }
      }
    }

    //all JBBCs connect to current table
    CANodeIdSet connectedChildren = currentTable.getNodeAnalysis()->getJBBC()->getJoinedJBBCs();

    // get number of tables connected to current jbbc
    UInt32 numConnectedChildren = connectedChildren.entries();

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
// LCOV_EXCL_START :dpm
    CURRCONTEXT_OPTDEBUG->stream() << "JBBC: "<<currentTable.getText()<<endl;
    CURRCONTEXT_OPTDEBUG->stream() << "connected jbbcs: "<<connectedChildren.getText()<<endl;
    CURRCONTEXT_OPTDEBUG->stream() << endl;
// LCOV_EXCL_STOP
  }
#endif //_DEBUG

    // if current jbbc is:
    // * connected to at least minimumCenterConnectivity jbbcs
    // * has a connectivity greater than the connectivity
    //   of the current center/fact table
    // then set this jbbc to be the center/fact table
    if ((numConnectedChildren >= minimumCenterConnectivity) &&
        (centerConnectivity < numConnectedChildren) &&
        (currentTable.getNodeAnalysis()->getTableAnalysis()) &&
        (!centerTableForced))
    {
      // see we are going have found a new center
      // the previous center should be a dimension
      // make sure it is not connected to more than
      // the allowed number of connected tables for
      // dimension tables.
      // If the previous center is connected to a
      // greater number of tables than allowed for
      // a dimension table then fail pattern match
      if (centerConnectivity > maximumDimensionConnectivity)
      {
  centerTableComputed_ = TRUE;
        centerTable_ = NULL_CA_ID;
        centerTableConnectivity_ = 0;
        maxDimensionConnectivity_ = 0;
        return centerTable_;
      }

      center = currentTable;
      centerConnectivity = numConnectedChildren;
    }
    else{

      // this table is potentially a dimension table
      // make sure it is not connected to more than
      // the allowed number of tables

      // Ignore tables that return 1 row when counting
      // the connected tables
      UInt32 numNonSingleRowConnectedChildren =
        connectedChildren.subtractSet(singleRowTables).entries();

      // if the number of connected children is greater than
      // the maximum connectivity of any dimension table upto
      // this point, then update the maximum connectivity
      if (numNonSingleRowConnectedChildren > maxDimConnectivity)
        maxDimConnectivity = numNonSingleRowConnectedChildren;

      // if this table is connected to more than the maximum
      // number of tables allowed for a dimension table
      // then fail the pattern match
      if ( numNonSingleRowConnectedChildren > maximumDimensionConnectivity)
      {
  centerTableComputed_ = TRUE;
        centerTable_ = NULL_CA_ID;
        centerTableConnectivity_ = 0;
        maxDimensionConnectivity_ = 0;
        return centerTable_;
      }
    }
  }

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
// LCOV_EXCL_START :dpm
    if(center != NULL_CA_ID)
    {
      CURRCONTEXT_OPTDEBUG->stream() << "Center Table is: "<<center.getText()<<endl;
      CURRCONTEXT_OPTDEBUG->stream() << "Center connectivity is: "<<centerConnectivity<<endl;
      CURRCONTEXT_OPTDEBUG->stream() << "Max dimension connectivity is: "<<maxDimConnectivity<<endl;
      CURRCONTEXT_OPTDEBUG->stream() << endl;
    }
    else{
      CURRCONTEXT_OPTDEBUG->stream() << "No center table found "<<center.getText()<<endl;
    }
// LCOV_EXCL_STOP
  }
#endif //_DEBUG

  // we did not find a center table
  if (center == NULL_CA_ID)
  {
    centerTableComputed_ = TRUE;
    centerTable_ = NULL_CA_ID;
    centerTableConnectivity_ = 0;
    maxDimensionConnectivity_ = 0;
    return centerTable_;
  }

  // get rows to be scanned for center table
  CostScalar centerTableRowsToBeScanned =
    appStatMan->getStatsForLocalPredsOnCKPOfJBBC(center)->
                getResultCardinality();

  // get record size for center table
  CostScalar centerTableRecordSize =
    center.getNodeAnalysis()->
           getTableAnalysis()->
           getTableDesc()->
           getNATable()->
           getRecordLength();

  // compute amount of data to be scanned from center table
  // amount is in KB
  CostScalar centerTableDataToBeScanned =
    (centerTableRowsToBeScanned * centerTableRecordSize) / 1024;

  // get number of partitions for the center table
  CostScalar numCenterTablePartitions =
    center.getNodeAnalysis()->
           getTableAnalysis()->
           getTableDesc()->
           getClusteringIndex()->
           getNAFileSet()->
           getCountOfPartitions();

  CostScalar sizePerPartition = centerTableDataToBeScanned / numCenterTablePartitions;

  centerTableComputed_ = TRUE;
  centerTable_ = center;
  centerTableConnectivity_ = centerConnectivity;
  maxDimensionConnectivity_ = maxDimConnectivity;
  centerTableRowsScanned_ = centerTableRowsToBeScanned;
  centerTableDataScanned_ = centerTableDataToBeScanned;
  centerTablePartitions_ = numCenterTablePartitions;
  centerTableDataPerPartition_ = sizePerPartition;


#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
// LCOV_EXCL_START :dpm
    CURRCONTEXT_OPTDEBUG->stream() << "Center Table Rows Scanned: "\
                    <<centerTableRowsScanned_.getValue()<<endl;
    CURRCONTEXT_OPTDEBUG->stream() << "Center Table Record Size: "\
                    <<centerTableRecordSize.getValue()<<endl;
    CURRCONTEXT_OPTDEBUG->stream() << "Center Table Data Scanned: "\
                    <<centerTableDataScanned_.getValue()<<endl;
    CURRCONTEXT_OPTDEBUG->stream() << "Center Table Num partitions: "\
                    <<centerTablePartitions_.getValue()<<endl;
    CURRCONTEXT_OPTDEBUG->stream() << "Center Table size per partition: "\
                    <<centerTableDataPerPartition_.getValue()<<endl;
// LCOV_EXCL_STOP
  }
#endif //_DEBUG

  return centerTable_;
} //JBBSubsetAnalysis::computeCenterTable

// This method, given a fact table, matches a multi-join to a star pattern
NABoolean JBBSubsetAnalysis::isAStarPattern(CANodeId factTable,
                                            CostScalar factTableCKPrefixCardinality)
{

  if (CmpCommon::getDefault(COMP_BOOL_88) == DF_ON)
    return FALSE;

  if(CmpCommon::getDefault(NESTED_JOINS) == DF_OFF)
    return FALSE;

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "JBBSubsetAnalysis_isAStarPattern_Begin" <<endl;
  }
#endif //_DEBUG

  //get CANodeIdSet representing this MultiJoin
  CANodeIdSet childSet = getJBBCs();

  //Get a handle to ASM
  AppliedStatMan * appStatMan = QueryAnalysis::ASM();

  //all nodes in this multi-join except the fact table
  CANodeIdSet nodesExcludingFactTable = childSet;
  nodesExcludingFactTable.remove(factTable);

  //temp vars used for iteration
  ULng32 i=0;
  CANodeId currentNode;
  CANodeIdSet currentNodeSet;
  CostScalar currentCardinality;

  // get the index_desc for the facttable primary index
  const IndexDesc * factTableIndexDesc =
    factTable.getNodeAnalysis()
      ->getTableAnalysis()
        ->getTableDesc()
          ->getClusteringIndex();

  const PartitioningFunction * factTablePartFunc =
    factTableIndexDesc
      ->getPartitioningFunction();

  ValueIdSet factTablePartKey;

  Int32 factTableNumPartitions = 1;

  NABoolean factIsHashPartitioned = FALSE;

  if (factTablePartFunc)
  {
    factTablePartKey =
      factTablePartFunc->getPartitioningKey();

    factTableNumPartitions =
      factTablePartFunc->getCountOfPartitions();

    factIsHashPartitioned =
      factTablePartFunc->isATableHashPartitioningFunction();

  }

  //for further processing we'll need the clustering key of the fact table
  ValueIdList factTableCK =
    factTableIndexDesc->getIndexKey();


  // Variables used during iteration
  ValueId keyColumn;
  CANodeIdSet connectedTables;
  CANodeIdSet tablesConnectedToColumn;
  CANodeIdSet tablesConnectedViaThisColumn;
  Lng32 numPrefixColsCoveredFromFactTableCK = 0;

  // this variable is used to  get the list of
  // fact table clustering key prefix columns
  // that are covered by join or local preds
  ValueIdList coveredFactTableCKPrefix;

  // temp vars to pass into getJBBCsConnectedToPrefixOfList()
  ValueIdSet dummyForJoinPreds;
  ValueIdSet dummyForLocalPreds;

  // get:
  // 1. Tables connected to Fact Table via join predicates on clustering key prefix
  // 2. Fact Table clustering key prefix columns that have a predicate on them
  // 3. Number of fact table clustering key prefix columns covered
  connectedTables = factTable.getNodeAnalysis()->
                              getTableAnalysis()->
                              getJBBCsConnectedToPrefixOfList(
                      nodesExcludingFactTable,
                                        factTableCK,
                                        numPrefixColsCoveredFromFactTableCK,
                                        dummyForJoinPreds,
                                              dummyForLocalPreds);

  // if a prefix of the fact table clustering key is not covered
  // by a join predicate, then return FALSE
  if((!connectedTables.entries()))
  {
#ifdef _DEBUG
    if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
         CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
    {
// LCOV_EXCL_START :dpm
      CURRCONTEXT_OPTDEBUG->stream() << "Pattern Not Matched, there is no join predicate on prefix of clustering key" <<endl;
      CURRCONTEXT_OPTDEBUG->stream() << "MJStarJoinIRule_isAStarPattern_End" <<endl;
// LCOV_EXCL_STOP
    }
#endif //_DEBUG
    // return FALSE, indicating this rule is not a good match for the current
    // query
    return FALSE;
  }


  // this set will be used to keep track of tables
  // that are not part of the edges of the star
  // schema
  CANodeIdSet availableNodes(childSet);

  // remove fact table from available nodes
  availableNodes-=factTable;

  // remove tables directly connected to the fact table
  // via join predicates on fact table clustering key
  // prefix
  availableNodes-=connectedTables;

  // set of tables representing the edges of the star from tables connected to the
  // prefix of the fact table clustering key
  // start by building an edge from the tables connected to the first prefix column
  // then add to that the edge from the tables connected to the next prefix column
  // and so on....
  // once an edge is built it will be inserted in a list of edges (variable
  // listOfEdges) The list as evident from the comments above will be ordered by
  // the position of the fact table clustering key column through which the edge
  // is connected.
  CANodeIdSet edge;

  // variable used while iterating in the loops below
  CANodeIdSet currentEdge;

  // Set of all tables in edges that should be joined before fact table
  // i.e. rows coming out of this set of tables will be nested joined
  // into fact table
  // If this set is empty then the star pattern was not matched
  CANodeIdSet optimalEdgeSet;

  // Cardinality of the set of tables in an edge
  CostScalar dataFlowFromEdge;

  // Cardinality of the fact table after application of clustering key
  // prefix columns after joining with the tables in the set 'edge'.
  CostScalar factTableRowsToScan;

  // Rows returned from the factTable
  CostScalar dataFlowFromFactTable;

  // cost of accessing the fact Table as outer most table
  CostScalar factTableHashJoinCost;

  // compute the cost of accessing the fact Table as the outer most table
  factTableHashJoinCost = computeDataAccessCostForTable(csZero,
                                                        factTableCKPrefixCardinality,
                                                        factTable);

  // variable will be used while iterating through the loops below
  // cost of nested join of fact table with tables from edge
  CostScalar factTableCost;

  // variable use to keep track of the lowest cost of a nested join
  // on the fact table
  //CostScalar lowestFactTableCost(csZero);
  CostScalar lowestCombinedCost(csZero);

  // This determines how much cheaper a fact table nested should be
  // vs hash join / full scan of the fact table, for a plan with a
  // nested join on the fact table to be considered feasible.
  // Currently the default value is 10, this can be adjusted after
  // further testing.
  // Remember we are trying to find if a type-I plan is feasible
  // If not we'll try a type-II plan (i.e. a plan with a hash
  // join on the fact table)
  UInt32 factTableCostFactor =
    (ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_3);

  // list of edges, where each edge starts from a column connected
  // to a key prefix of the fact table. The list is in the order
  // of the columns in the key of the fact table
  NAList<CANodeIdSet> * listOfEdges =
    new (CmpCommon::statementHeap()) NAList<CANodeIdSet> (CmpCommon::statementHeap(),
                                                          connectedTables.entries());


#ifdef _DEBUG
      if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
           CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
      {
// LCOV_EXCL_START :dpm
        CURRCONTEXT_OPTDEBUG->stream() << "Cost estimate of fact table access as outer most table: "\
                        << istring(Lng32(factTableHashJoinCost.value()))\
                        <<endl;
        CURRCONTEXT_OPTDEBUG->stream() << "Star Join will make sense if fact table nested join access is "\
                        << istring(Lng32(factTableCostFactor))\
                        << " times cheaper" << endl;
// LCOV_EXCL_STOP
      }
#endif //_DEBUG

  //Doing a nested join on the fact table will only make sense if accessing
  //fact table as outer most table is factTableCostFactor times as expensive
  //as doing a nested join
  factTableHashJoinCost = factTableHashJoinCost / factTableCostFactor;

  //set of connected tables that have already been used to build edges
  CANodeIdSet usedConnectedTables;

  UInt32 lookAhead = (ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_18);

  // in the loop below we iterate over the columns of the fact table clustering
  // key prefix that are covered by either local or join predicates.
  // We iterate in order of the column's position in the clustering key.
  // In each iteration we try to see if the column has a join pred then
  // start an edge from the table connected via the column.
  // This is done so that we get a list of edges in an order which would
  // allow for ordered probes into the nested join on the fact table
  // e.g. consider the following connectivity graph
  // fact table cluster key is : a,b,c in that order
  //
  //
  //          DIM2
  //           |
  //           b
  //           |
  //   DIM1-a-FACT-c-DIM3
  //
  //
  // Then we want to get the edges in order starting from the outer most
  // i.e. DIM1, DIM2, DIM3
  //
  // Since we are attempting to get a plan like
  //
  //                   NJ
  //                  /  \
  //                /      \
  //              NJ      FACT
  //             /  \
  //           /      \
  //         NJ       DIM3
  //        /  \
  //      /      \
  //    DIM1     DIM2

  CANodeIdSet prevEdgeAndFact;
  prevEdgeAndFact += factTable;

  //iterate over the covered prefix of the clustering key
  for (i = 0 ; i < (ULng32) numPrefixColsCoveredFromFactTableCK; i++)
  {
    // Get the covered prefix on the fact table clustering key
    // covered prefix is constituted by columns that have either
    // a local or a join predicate on them.

    // get a handle to the key column
    keyColumn = factTableCK[i];

    //get a handle to the column analysis for this column
    ColAnalysis * keyColAnalysis = keyColumn.colAnalysis();

    // check if this column has join predicates on it
    if(!keyColAnalysis->getAllConnectedJBBCs().entries())
    {
      // if this does not have a join predicate on it
      // then it's covered by a local pred. Add it to
      // the list of key prefix columns covered.
      // It could also be because of a veg this column
      // does not have a join rather a local pred e.g.
      //
      // select count(*)
      // from
      //   fact, dim
      // where
      //   fact.a = dim.a and
      //   dim.a = 10;
      //
      // In this case 'fact.a = 10' is implied via a veg
      // and the join between fact and dim is essentially
      // a cross product after application of local preds
      coveredFactTableCKPrefix.insert(keyColumn);
      continue;
    }

    // Get tables from this multi-join (i.e. the multi-join on which we are
    // doing the top match) that are connected to the fact table via this
    // column

    // This will get all tables connected via this column
    tablesConnectedViaThisColumn = keyColAnalysis->getAllConnectedJBBCs();

    // This will narrow down the set of tables from all connected
    // tables to connected tables in this multi-join
    tablesConnectedViaThisColumn.intersectSet(connectedTables);

    if(!tablesConnectedViaThisColumn.entries())
    {
      if(usedConnectedTables.entries())
	break;
      else
	return FALSE;
    }

    //tablesConnectedViaThisColumn should have atleast one entry
    CCMPASSERT(tablesConnectedViaThisColumn.entries());

    /*
    if(!tablesConnectedViaThisColumn.entries())
    {
      if(listOfEdges->entries())
        break;
      else
        return FALSE;
    }
    */

    // insert this column into list of key prefix columns covered
    coveredFactTableCKPrefix.insert(keyColumn);

    //get the number of tables connected via this columns
    UInt32 numTablesConnectedViaThisColumn =
      tablesConnectedViaThisColumn.entries();

    // Remove tables that have already been considered in previous iterations
    tablesConnectedViaThisColumn.subtractSet(usedConnectedTables);

    // get connected tables that have not been considered
    // i.e. we have not built an edge starting from them.
    UInt32 connectedTablesNotConsidered =
      tablesConnectedViaThisColumn.entries();

    // there are no connected tables to consider
    if(!connectedTablesNotConsidered)
      continue;

    // if tables connected via this column is less than the number of
    // connected tables not considered
    if(numTablesConnectedViaThisColumn > connectedTablesNotConsidered)
    {
      // This would happen in the following scenario
      // consider fact table key is a, b, c
      // consider column connectivity as below
      //
      //           a     b    c
      //           |     |    |
      //          DIM1 DIM1   |
      //           |          |
      //          DIM2       DIM2
      //
      // Notice here when we come to b, we would have already covered b if
      // we started an edge from DIM1 while iterating for column 'a'. Therefore
      // b is already covered and we can skip over to column c.
      //
      // This is a special case but I was worried about the case where all dimension
      // table might joined to the fact table via a partitioning key column in addition
      // to what ever other part of the fact table key they join to.
      // This was apparently an MP practice to try to colocate joins in the same partitions
      //
      // In such a case it would be preferrable to get an ordering of DIM1,DIM2 rather
      // than DIM2, DIM1 for the probes going into the fact table nested join

      // This column is already covered, move to the next one
      continue;
    }

    // the next table to consider for starting an edge
    CANodeId tableToConsider = NULL_CA_ID;
    CostScalar dataFlowFromTableToConsider = -1;
    CostScalar dataFlowAfterIncludingTableToConsider = -1;

    if (tablesConnectedViaThisColumn.entries()==1)
    {

      // get the jbbc in this set
      tableToConsider = tablesConnectedViaThisColumn.getFirst();

    }
    else{

      Lng32 prefixCoveredByTableToConsider = 0;
      CostScalar tableToConsiderFactTableCost = 0;
      CANodeIdSet coveredTables;

      // Find the table that gives the max key coverage. Consider the example
      // below:
      // consider fact table key is a, b, c
      // consider column connectivity as below
      //
      //           a     b    c
      //           |     |    |
      //          DIM1 DIM1   |
      //           |          |
      //          DIM2       DIM2
      //
      // When iterating for column 'a' we would like to start an edge
      // from table DIM1 rather than DIM2, since 'a' will cover a greater
      // key prefix.

      for(CANodeId currentTable = tablesConnectedViaThisColumn.init();
          tablesConnectedViaThisColumn.next(currentTable);
          tablesConnectedViaThisColumn.advance(currentTable))
      {
        if (!usedConnectedTables.legalAddition(currentTable))
          continue;

        Lng32 coveredPrefix = 0;
        CostScalar dataFlowFromCurrentTable =
          appStatMan->
            getStatsForCANodeId(currentTable)->
              getResultCardinality();

        // add currentTable to list of tables that we
        // have already determined cover key prefix
        // In each iteration:
        // 1. Add the current table to this list
        // 2. determine the key prefix covered
        // 3. Remove current table from this list
        // Eventually the table that provides the
        // greatest key prefix will be considered
        usedConnectedTables.insert(currentTable);

        // find the key prefix covered
        coveredTables = factTable.getNodeAnalysis()->
                                  getTableAnalysis()->
                                  getJBBCsConnectedToPrefixOfList(
                                    usedConnectedTables,
                                    factTableCK,
                                    coveredPrefix,
                                    dummyForJoinPreds,
                                    dummyForLocalPreds);

        //build the edge from the current table
        CANodeIdSet currentTableEdge =
          extendEdge(currentTable, availableNodes,lookAhead);
          
        //all tables before fact
        CANodeIdSet jbbcsBeforeFact = currentTableEdge;
        jbbcsBeforeFact += edge;
        
        CostScalar dataFlowAfterIncludingCurrentTable =
          appStatMan->
            getStatsForCANodeIdSet(jbbcsBeforeFact)->
              getResultCardinality();

        if((factIsHashPartitioned) &&
           (CmpCommon::getDefault(COMP_BOOL_150) == DF_OFF))
        {
          ValueIdSet factTableColumnsJoined = factTable.getNodeAnalysis()
                                                ->getTableAnalysis()
                                                  ->getColsConnectedViaEquiJoinPreds(jbbcsBeforeFact);

          CollIndex numPartKeyCols = factTablePartKey.entries();
          CollIndex numJoinedCols = factTableColumnsJoined.entries();
          CollIndex numPartKeyColsCovered = 0;

          for (ValueId VId = factTablePartKey.init();
                             factTablePartKey.next(VId);
                             factTablePartKey.advance(VId) )
          {
            ItemExpr * ce;
            ValueIdSet vIdSet(VId);
            if(vIdSet.referencesAConstExpr(&ce))
            {
              numPartKeyColsCovered++;
              continue;
            };
            
            ValueIdSet tempJoinedCols = factTableColumnsJoined;
            tempJoinedCols.removeCoveredVIdSet(VId);
            if(tempJoinedCols.entries() < numJoinedCols)
              numPartKeyColsCovered++;
            else
              break;
          }

          if(numPartKeyColsCovered < numPartKeyCols)
          {
            dataFlowAfterIncludingCurrentTable = 
              dataFlowAfterIncludingCurrentTable * factTableNumPartitions;
          }
        }

        // figure out the number of fact table rows that'll be scanned for this
        // nested join
        factTableRowsToScan = appStatMan->
                                getStatsForJoinPredsOnCKOfJBBC(jbbcsBeforeFact, factTable)->
                                  getResultCardinality();

        // figure out the rows coming out of the fact table after this nested join
        // using joinJBBChildren(prevEdgeAndFact, currentEdge) instead of
        // using joinJBBChildren(edge, factTable), in order to drive ASM using
        // joins, 'edge' could have cross products and will miss some cardinality
        // adjustments because of join with a potential cross product.
        dataFlowFromFactTable = appStatMan->
                                  joinJBBChildren(prevEdgeAndFact, currentTableEdge)->
                                    getResultCardinality();


        // now cost the nested join
        factTableCost = computeDataAccessCostForTable(dataFlowAfterIncludingCurrentTable,
                                                      factTableRowsToScan,
                                                      factTable);
        
        //put back nodes that were made unavailable
        currentTableEdge -= currentTable;
        availableNodes += currentTableEdge;

        if ( (CmpCommon::getDefault(COMP_BOOL_14) == DF_ON))
        {
        // if covered prefix is the greater than the
        // max prefix we have till now, then consider
        // this table to start an edge from.
        if((tableToConsider == NULL_CA_ID) ||
             (prefixCoveredByTableToConsider < coveredPrefix) ||
             ((prefixCoveredByTableToConsider) &&
              (prefixCoveredByTableToConsider == coveredPrefix) &&
            (dataFlowFromTableToConsider > dataFlowFromCurrentTable)))
        {
          tableToConsider = currentTable;
            tableToConsiderFactTableCost = factTableCost;
            prefixCoveredByTableToConsider = coveredPrefix;
            dataFlowFromTableToConsider = dataFlowFromCurrentTable;
            dataFlowAfterIncludingTableToConsider = dataFlowAfterIncludingCurrentTable;
          }
        }
        else{
          if ((tableToConsiderFactTableCost == 0) ||
              (factTableCost < tableToConsiderFactTableCost))
          {
            tableToConsider = currentTable;
            tableToConsiderFactTableCost = factTableCost;
            prefixCoveredByTableToConsider = coveredPrefix;
            dataFlowFromTableToConsider = dataFlowFromCurrentTable;
            dataFlowAfterIncludingTableToConsider = dataFlowAfterIncludingCurrentTable;            
          }
        }

        // remove currentTable from list of tables that we
        // have already determined cover key prefix
        usedConnectedTables.remove(currentTable);

      }

    }

    // finally we have determined the table that provides for
    // the greatest key prefix on the fact table, add it to
    // the list of connected tables for which we had built edges
    usedConnectedTables.insert(tableToConsider);

    //build the edge
    currentEdge = extendEdge(tableToConsider, availableNodes,lookAhead);

    (*listOfEdges).insert(currentEdge);

    edge += currentEdge;
#ifdef _DEBUG
    if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
         CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
    {
// LCOV_EXCL_START :dpm
      CURRCONTEXT_OPTDEBUG->stream() << "Built an edge starting from table "\
                      << tableToConsider.getText()<< endl;
      CURRCONTEXT_OPTDEBUG->stream() << "The edge is " \
                      << currentEdge.getText()<<endl;
// LCOV_EXCL_STOP
    }
#endif

    // figure out the rows coming out of the fact table after this nested join
    // using joinJBBChildren(prevEdgeAndFact, currentEdge) instead of
    // using joinJBBChildren(edge, factTable), in order to drive ASM using
    // joins, 'edge' could have cross products and will miss some cardinality
    // adjustments because of join with a potential cross product.
    dataFlowFromFactTable = appStatMan->
                              joinJBBChildren(prevEdgeAndFact, currentEdge)->
                                getResultCardinality();

    prevEdgeAndFact += currentEdge;
  }

  // variable used in iteration
  CANodeId connectedTable = NULL_CA_ID;

  // if there are still some tables left that were on the key columns
  // but were not considered earlier because we where optimizing key
  // coverage
  if(usedConnectedTables.entries() != connectedTables.entries())
  {
    // build an edge from all the tables in the unusedConnectedTables set
    // since they still have predicates on fact table clustering key prefix
    // columns and could provide reduction.
    CANodeIdSet unusedConnectedTables(connectedTables);
    unusedConnectedTables.subtractSet(usedConnectedTables);

    for(connectedTable = unusedConnectedTables.init();
                         unusedConnectedTables.next(connectedTable);
                         unusedConnectedTables.advance(connectedTable))
    {

      //build the edge
      currentEdge = extendEdge(connectedTable, availableNodes, lookAhead);

      (*listOfEdges).insert(currentEdge);

       edge += currentEdge;
#ifdef _DEBUG
      if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
           CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
      {
// LCOV_EXCL_START :dpm
        CURRCONTEXT_OPTDEBUG->stream() << "Built an edge starting from table "\
                        << connectedTable.getText()<< endl;
        CURRCONTEXT_OPTDEBUG->stream() << "The edge is " \
                        << currentEdge.getText()<<endl;
// LCOV_EXCL_STOP
      }
#endif
    }
  }


  // We have the list of edges joined to the fact table in the order
  // of the fact table clustering key columns.
  // Find the optimal location for the fact table, i.e. we'll try to
  // see which tables should be join below the fact table in a left
  // deep join sequence.
  //
  // e.g. consider the following connectivity graph
  // fact table cluster key is : a,b,c in that order
  //
  //
  //          DIM2
  //           |
  //           b
  //           |
  //   DIM1-a-FACT-c-DIM3
  //
  //
  // Then the list of edges will be DIM1, DIM2, DIM3
  //
  // In the following loop we'll estimate the cost of
  // doing a nested join on the fact table after
  // 1. DIM1,
  // 2. DIM1, DIM2
  // 3. DIM1, DIM2, DIM3
  //
  // Then we compare the optimal cost of doing a nested join
  // on the fact table, with the cost of just doing a hash
  // join on the fact table which basically involves scanning the
  // whole fact table.

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "Starting to evaluate optimal location of Fact Table "<< endl;

  }
#endif

  //cost of fact table nested join after
  //maximum possible key prefix coverage
  CostScalar costForMaxKeyCoverage=0;

  prevEdgeAndFact.clear();
  edge.clear();

  CostScalar costOfDimProbes(csZero);
  double probeHashTableUnitCost =
    (double) (ActiveSchemaDB()->getDefaults()).getAsDouble
    (MULTI_JOIN_PROBE_HASH_TABLE);

  // iterate over list of edges
  for ( i = 0; i < (*listOfEdges).entries(); i++)
  {

    // get the current edge from the list of edges
    currentEdge = (*listOfEdges)[i];

    //the edge that was used in the last iteration
    prevEdgeAndFact = edge;
    prevEdgeAndFact += factTable;

    // add it to the cumulative edge, i.e. list of
    // all tables we are considering to be below the
    // fact table
    edge += currentEdge;

    // get the number of rows coming out of all the tables
    // that'll be below the fact tables.
    // This is the number of probes going into the fact table
    // for this nested join
    dataFlowFromEdge = appStatMan->
                         getStatsForCANodeIdSet(edge)->
                           getResultCardinality();

    if((factIsHashPartitioned) &&
       (CmpCommon::getDefault(COMP_BOOL_150) == DF_OFF))
    {
      ValueIdSet factTableColumnsJoined = factTable.getNodeAnalysis()
                                            ->getTableAnalysis()
                                              ->getColsConnectedViaEquiJoinPreds(edge);

      CollIndex numPartKeyCols = factTablePartKey.entries();
      CollIndex numJoinedCols = factTableColumnsJoined.entries();
      CollIndex numPartKeyColsCovered = 0;

      for (ValueId VId = factTablePartKey.init();
	   factTablePartKey.next(VId);
	   factTablePartKey.advance(VId) )
      {
        ItemExpr * ce;
        ValueIdSet vIdSet(VId);
        if(vIdSet.referencesAConstExpr(&ce))
        {
          numPartKeyColsCovered++;
          continue;
        };
 
        ValueIdSet tempJoinedCols = factTableColumnsJoined;
        tempJoinedCols.removeCoveredVIdSet(VId);
        if(tempJoinedCols.entries() < numJoinedCols)
          numPartKeyColsCovered++;
        else
          break;
      }

      if(numPartKeyColsCovered < numPartKeyCols)
      {
        dataFlowFromEdge = dataFlowFromEdge * factTableNumPartitions;
      }

    }

    // figure out the number of fact table rows that'll be scanned for this
    // nested join
    factTableRowsToScan = appStatMan->
                            getStatsForJoinPredsOnCKOfJBBC(edge, factTable)->
                              getResultCardinality();

    // figure out the rows coming out of the fact table after this nested join
    // using joinJBBChildren(prevEdgeAndFact, currentEdge) instead of
    // using joinJBBChildren(edge, factTable), in order to drive ASM using
    // joins, 'edge' could have cross products and will miss some cardinality
    // adjustments because of join with a potential cross product.
    dataFlowFromFactTable = appStatMan->
                              joinJBBChildren(prevEdgeAndFact, currentEdge)->
                                getResultCardinality();


    // now cost the nested join
    factTableCost = computeDataAccessCostForTable(dataFlowFromEdge,
                                                  factTableRowsToScan,
                                                  factTable);

    // capture the cost of the nested join when all edges are join below the
    // fact table.
    // This will be the cost for max key coverage after the last iteration.
    costForMaxKeyCoverage = factTableCost;

#ifdef _DEBUG
    if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
         CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
    {
// LCOV_EXCL_START :dpm
      CURRCONTEXT_OPTDEBUG->stream() << "FactTable after edge " \
                      << currentEdge.getText()<<endl;
      CURRCONTEXT_OPTDEBUG->stream() << "The cummulative edge is "\
                      << edge.getText()<<endl;
      CURRCONTEXT_OPTDEBUG->stream() << "Probes into fact table: "\
                      << istring(Lng32(dataFlowFromEdge.value()))\
                      <<endl;
      CURRCONTEXT_OPTDEBUG->stream() << "Fact Table Rows to scan: "\
                      << istring(Lng32(factTableRowsToScan.value()))\
                      <<endl;
      CURRCONTEXT_OPTDEBUG->stream() << "Rows coming out of fact table: "\
                      << istring(Lng32(dataFlowFromFactTable.value()))\
                      <<endl;
      CURRCONTEXT_OPTDEBUG->stream() << "Our cost estimate of fact table nested join: "\
                      << istring(Lng32(factTableCost.value()))\
                      <<endl;
// LCOV_EXCL_STOP
    }
#endif //_DEBUG

    // if this is the first iteration
    // or current nested join cost is cheaper
    // than lowest nested join cost
    CostScalar combinedCost = costOfDimProbes + factTableCost;
    if ((NOT lowestCombinedCost.isGreaterThanZero()) ||
        (combinedCost < lowestCombinedCost))
    {
      lowestCombinedCost = combinedCost;

      if (lowestCombinedCost <= factTableHashJoinCost)
      {
        optimalEdgeSet = edge;

        // set the set Of tables to be joined before the fact table,
        // this will be used during nextSubstitute for this rule.
        nodesJoinedBeforeFactTable_ =
        optimalEdgeSet;

        // set the optimal location of the fact table in the
        // list of edges, this will be used by the nextSubstitute()
        // method
        optimalFTLocation_ = (Int32) i+1;
        lowestFactNJCost_ = lowestCombinedCost;
      }
    }
#if 0
    if ((NOT lowestFactTableCost.isGreaterThanZero()) ||
        (factTableCost < lowestFactTableCost))
    {
      lowestFactTableCost = factTableCost;

      if (lowestFactTableCost <= factTableHashJoinCost)
      {
        optimalEdgeSet = edge;

        // set the set Of tables to be joined before the fact table,
        // this will be used during nextSubstitute for this rule.
        nodesJoinedBeforeFactTable_ =
        optimalEdgeSet;

        // set the optimal location of the fact table in the
        // list of edges, this will be used by the nextSubstitute()
        // method
        optimalFTLocation_ = (Int32) i+1;
        lowestFactNJCost_ = lowestFactTableCost;
      }
    }
#endif
    costOfDimProbes += dataFlowFromEdge * probeHashTableUnitCost;
  }

  // note the cost for a nested join into fact
  // if all the edges are below the fact
  costForMaxKeyFactNJ_ = costForMaxKeyCoverage;

  //This variable determines if we want all edges of the star
  //that join on the key prefix on the Fact Table, to be joined
  //below i.e. before the fact table.
  NABoolean forceMaxKeyCoverageOnFT = FALSE;

  // COMP_BOOL_12 is used to force a plan with max key coverage on
  // the fact table, i.e. join all edges covering key prefix column
  // below the fact table to get max key coverage for nested join
  // on fact table.

  // COMP_BOOL_11 is similar to COMP_BOOL_12. It differs to COMP_BOOL_12
  // in that it does not force a max key coverage plan if the cost of
  // the nested join is higher than the cost of the hash join on the
  // fact table.

  // COMP_BOOL_13 basically means to force a star join rule plan.

  //if we want to force max key prefix coverage on fact table
  //this should only be allowed if fact table nested join is
  //cheaper than doing a full scan of the fact table (i.e. the
  //hash join scenario)
  //If COMP_BOOL_12 is 'ON' then we'll just force a plan with
  //max key prefix coverage on fact table
  if ((costForMaxKeyCoverage < factTableHashJoinCost) &&
      (CmpCommon::getDefault(COMP_BOOL_11) == DF_ON))
    forceMaxKeyCoverageOnFT = TRUE;

  //If COMP_BOOL_12 is 'ON' then we'll just force a plan with
  //max key prefix coverage on fact table
  if (CmpCommon::getDefault(COMP_BOOL_12) == DF_ON)
    forceMaxKeyCoverageOnFT = TRUE;


  //COMP_BOOL_13 means force star join rule !!!
  //therefore if starjoin rule was not found feasible
  //just force max key coverage i.e. force all the
  //edges joining on FT key prefix columns to be below
  //fact table
  if (((!optimalEdgeSet.entries()) &&
       ( (CmpCommon::getDefault(DIMENSIONAL_QUERY_OPTIMIZATION) == DF_ON) ||
         (CmpCommon::getDefault(COMP_BOOL_13) == DF_ON) )) &&
      (CmpCommon::getDefault(COMP_BOOL_77) != DF_ON))
    forceMaxKeyCoverageOnFT = TRUE;


  // if we are forcing max key prefix coverage for the
  // fact table nested join.
  if(forceMaxKeyCoverageOnFT)
  {
    edge.clear();

    // iterate over list of edges
    for ( i = 0; i < (*listOfEdges).entries(); i++)
    {

      currentEdge = (*listOfEdges)[i];

      edge += currentEdge;
    }

    optimalEdgeSet = edge;

    // set the set Of tables to be joined before the fact table,
    // this will be used during nextSubstitute for this rule.
    nodesJoinedBeforeFactTable_ = optimalEdgeSet;

    optimalFTLocation_ = (Int32) (*listOfEdges).entries();
  }

  // from the set of tables in all the edges that join to the key prefix
  // take out the set of tables that compose the fringes that should be joined
  // before the fact table
  edge.subtractSet(optimalEdgeSet);

  // add the remaining tables to the available nodes set
  // these tables will be joined above/after the fact table
  // in the left deep join sequence
  availableNodes.addSet(edge);

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
// LCOV_EXCL_START :dpm
      CURRCONTEXT_OPTDEBUG->stream() << "The tables below fact table"\
                      << optimalEdgeSet.getText()<<endl;
      CURRCONTEXT_OPTDEBUG->stream() << "The tables above fact table "\
                      << availableNodes.getText()<<endl;
// LCOV_EXCL_STOP

  }
#endif //_DEBUG


  // nodes to be joined after Fact Table
  availableNodes_ = availableNodes;

  // save list of edges in the work area it'll be used when by nextSubstitute()
  listOfEdges_ = listOfEdges;

  // if optimalEdgeSet is empty it could mean
  // 1. there were not join predicates on fact table clustering key prefix
  // 2. there were join predicates on fact table clustering key prefix but
  //    nested join on fact table clustering is just too expensive.
  // In either case, a type-I nested join plan is not a good idea
  if (!optimalEdgeSet.entries())
  {

#ifdef _DEBUG
    if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
         CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
    {
// LCOV_EXCL_START :dpm
      CURRCONTEXT_OPTDEBUG->stream() << "Pattern Not Matched, there is no significant reduction on fact table" <<endl;
      CURRCONTEXT_OPTDEBUG->stream() << "JBBSubsetAnalysis_isAStarPattern_End" <<endl;
// LCOV_EXCL_STOP
    }
#endif //_DEBUG
    // return FALSE, indicating this rule is not a good match for the current
    // query
    return FALSE;
  }

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
// LCOV_EXCL_START :dpm
    CURRCONTEXT_OPTDEBUG->stream() << "Pattern Matched" <<endl;
    CURRCONTEXT_OPTDEBUG->stream() << "JBBSubsetAnalysis_isAStarPattern_End" <<endl;
// LCOV_EXCL_STOP
  }
#endif //_DEBUG

  CostScalar factNJAccessCost = lowestFactNJCost_;
  
  if (CmpCommon::getDefault(COMP_BOOL_12) == DF_ON)
    factNJAccessCost = costForMaxKeyFactNJ_;

  factTable.getNodeAnalysis()->
    getTableAnalysis()->
      setFactTableNJAccessCost(factNJAccessCost);
      
  return TRUE;
}

// get a rough estimate of cost for doing a nested join on the fact table
// number of probes = dataFlowFromEdge
// Rows of fact table that will be scanned = factTableRowsToScan
CostScalar JBBSubsetAnalysis::computeDataAccessCostForTable(
                                CostScalar probes,
                                CostScalar tableRowsToScan,
                                CANodeId   table)
{
  //Get a handle to ASM
  AppliedStatMan * appStatMan = QueryAnalysis::ASM();

  CostScalar costPerProbe ((ActiveSchemaDB()->getDefaults()).getAsDouble(COMP_FLOAT_0));
  CostScalar costPerUnitSize ((ActiveSchemaDB()->getDefaults()).getAsDouble(COMP_FLOAT_1));
  CostScalar costPerRowReturned ((ActiveSchemaDB()->getDefaults()).getAsDouble(COMP_FLOAT_2));

  //Record Size of the factTable
  CostScalar tableRecordSize = table.getNodeAnalysis()->
                                     getTableAnalysis()->
                                     getTableDesc()->
                                     getNATable()->
                                     getRecordLength();

  CostScalar subsetSizeKB = (tableRowsToScan * tableRecordSize)/1024;

  CostScalar tableRowsReturned = appStatMan->
                                 getStatsForCANodeId(table)->
                                 getResultCardinality();

  RelExpr * tableScanExpr = table.getNodeAnalysis()->getOriginalExpr();

  CostScalar sizeOfRowReturnedFromTable =
    tableScanExpr->getGroupAttr()->getRecordLength();


  CostScalar costForTable = (costPerProbe * probes) +
                            (costPerUnitSize * subsetSizeKB) +
                            (costPerRowReturned *
                            tableRowsReturned *
                            sizeOfRowReturnedFromTable);

  return costForTable;
}

//extends an edge of the star
CANodeIdSet JBBSubsetAnalysis::extendEdge(CANodeId thisTable,//in
                                          CANodeIdSet& availableNodes,
                                          UInt32 lookAhead)//in\out
{
  // CANodeIdSet representing thisTable
  CANodeIdSet thisTableSet(thisTable);

  //Get a handle to ASM
  AppliedStatMan * appStatMan = QueryAnalysis::ASM();

  // return value
  CANodeIdSet edge;

  //remove this table from the availableNodes
  //if it there
  if(availableNodes.contains(thisTable))
    availableNodes.remove(thisTable);

  //extend the edge to include thisTable
  edge += thisTable;

  if(!availableNodes.entries())
    return edge;

  if(lookAhead)
    lookAhead -=1;

  //if(!lookAhead)
  //  return edge;

  // Tables connected to thisTable that are still available
  CANodeIdSet connectedTables;

  // get all connected tables that are still available
  // i.e. not already part of some edge.
  connectedTables = thisTable.getNodeAnalysis()
                      ->getJBBC()->getJoinedJBBCs();
  connectedTables.intersectSet(availableNodes);

  CANodeIdSet connectedTablesBackup = connectedTables;

  UInt32 numConnectedTables = connectedTables.entries();

  // get after local predicate cardinality of thisTable
  CostScalar thisTableCardinality =  appStatMan->
                                       getStatsForCANodeId(thisTable)->
                                         getResultCardinality();

  //temp var to capture the set representing edge extension
  CANodeIdSet edgeExtension;

  //temp var to get cardinality of edge as it is being built
  CostScalar currentEdgeCardinality;

  //variables used for iteration
  UInt32 i;
  CANodeId connectedTable;
  CANodeIdSet connectedTableSet;

  const CostScalar EXTENSION_ALLOWANCE(1.05); // We allow 5% increase

  // sort connected Tables by cardinality of join with thisTable
  for (i = 0;i < numConnectedTables; i++)
  {
    CostScalar lowestCardinality = 0;

    // A table from the set of connectedTables that produces the lowest
    // cardinality when joined to thisTable
    CANodeId smallestRemainingTable = NULL_CA_ID;

    for(connectedTable = connectedTables.init();
                         connectedTables.next(connectedTable);
                         connectedTables.advance(connectedTable))
    {

      if(!edge.legalAddition(connectedTable))
        continue;

      //get a CANodeIdSet only containing the connectedTable to estimate
      //join of connectedTable with thisTable.
      connectedTableSet = connectedTable;

      //get cardinality for join of thisTable and connectedTable
      CostScalar joinCardinality =
        appStatMan->joinJBBChildren(thisTableSet, connectedTableSet)
          ->getResultCardinality();

      if(smallestRemainingTable == NULL_CA_ID)
      {
        lowestCardinality = joinCardinality;
        smallestRemainingTable = connectedTable;
        continue;
      }

      if(joinCardinality < lowestCardinality)
      {
        lowestCardinality = joinCardinality;
        smallestRemainingTable = connectedTable;
      }
    }

    //add the table that produces the lowest cardinality
    //after joining with thisTable
    //connectedTablesSorted.insert(smallestRemainingTable);

    if(smallestRemainingTable == NULL_CA_ID)
      continue;

    JBBC * smallestRemainingTableJBBC =
      smallestRemainingTable.getNodeAnalysis()->getJBBC();

    JBBSubset * thisTableJBBSubset =
      thisTableSet.jbbcsToJBBSubset();

    NABoolean isGuaranteedNonExpandingJoin =
      thisTableJBBSubset->
        isGuaranteedNonExpandingJoin(*smallestRemainingTableJBBC);

    // if the smallestRemaingTable
    if((lowestCardinality <= (EXTENSION_ALLOWANCE * thisTableCardinality))&&
       (isGuaranteedNonExpandingJoin || 
        lookAhead || 
        smallestRemainingTableJBBC->isOneRowMax()))
    {

      //sortedListOfConnectedReducingTables.insert(connectedTable);
      //only extend to a table that is still available
      if(availableNodes.contains(smallestRemainingTable))
      {

        edgeExtension = extendEdge(smallestRemainingTable,
                                   availableNodes,
                                   lookAhead);

        // get cardinality for join of edge and edgeExtension
        // I don't really need to do this, but doing this to
        // better drive the ASM cache
        currentEdgeCardinality =
          appStatMan->joinJBBChildren(edge, edgeExtension)
            ->getResultCardinality();

        edge += edgeExtension;
      }
    }

    //remove it from connectedTables
    connectedTables.remove(smallestRemainingTable);
  }

  return edge;
}
//JBBSubsetAnalysis::extendEdge

void JBBSubsetAnalysis::arrangeTablesAfterFactForStarJoinTypeI()
{

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "JBBSubsetAnalysis_arrangeTablesAfterFactForStarJoinTypeI_Begin"\
    <<endl;

  }
#endif //_DEBUG

  //Get a handle to ASM
  AppliedStatMan * appStatMan = QueryAnalysis::ASM();

  // create the plan
  // By this time we should have already done the following:
  // 1. figured out the fact table - FT
  // 2. matched the star schema pattern
  // 3. computed the list of edges to be joined before the fact table - BFT
  // 4. the list of tables that should be join after the fact table - AFT
  //
  // The BFT will be joined in the order the edges are found in
  // mjStarJoinIRuleWA->listOfEdges
  //
  // Following is how the join tree should look like
  //
  //               Join
  //               /  \
  //              /  AFT
  //             /
  //         Nested_Join
  //          /     \
  //        BFT     FT
  //
  // In this method we need to figure out the ordering of the tables in the AFT
  // and then produce a plan

  CANodeId factTable = factTable_;

  // begin - try to figure out the ordering of the tables in AFT

  // get the set of tables (BFT + FT).
  // we already know the join ordering for this set
  CANodeIdSet factTableSet = nodesJoinedBeforeFactTable_;
  factTableSet.insert(factTable);

  // get the set of table (AFT).
  // we don't know the join ordering of these tables except the fact that
  // they should be join after the fact table
  CANodeIdSet availableNodes = availableNodes_;

#ifdef _DEBUG
    //print the right child of the current join
    if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
    {
// LCOV_EXCL_START :dpm

      CURRCONTEXT_OPTDEBUG->stream() << "Table to be joined after fact table "\
                      << availableNodes.getText() << endl;
      CURRCONTEXT_OPTDEBUG->stream() << "Fact table "\
                      << factTable.getText() << endl;
      CURRCONTEXT_OPTDEBUG->stream() << "Table to be joined before fact table "\
                      << nodesJoinedBeforeFactTable_.getText() << endl;
// LCOV_EXCL_STOP
    }
#endif

  // this list should contain the available nodes in the order
  // they should be joined
  NAList<CANodeIdSet> availableNodesOrdered(CmpCommon::statementHeap(),
                                            availableNodes.entries());

  while (availableNodes.entries())
  {
    // Node that produces the fewest rows when joined to the factTableSet
    CANodeId smallestNode = NULL_CA_ID;

    CostScalar lowestReduction = 0;

    for( CANodeId availableNode = availableNodes.init();
         availableNodes.next(availableNode);
         availableNodes.advance(availableNode))
    {
      JBBC * availableNodeJBBC =
        availableNode.getNodeAnalysis()->getJBBC();

      if (!factTableSet.legalAddition(availableNode))
        continue;

      //get cardinality of joining factTableset to availableNode
      //to nonAvailableNodes
      CostScalar joinReduction =
        appStatMan->
          computeJoinReduction(factTableSet, availableNode);

      if ((CmpCommon::getDefault(COMP_BOOL_163) == DF_OFF) &&
          !(availableNodeJBBC->getOriginalParentJoin() &&
            availableNodeJBBC->getOriginalParentJoin()->isRoutineJoin()))
      {
        JBBSubset * factTableSetJBBSubset = factTableSet.jbbcsToJBBSubset();
        CANodeIdSet availableNodeSet;
        availableNodeSet += availableNode;
        JBBSubset * availableNodeJBBSubset = availableNodeSet.jbbcsToJBBSubset();
        ValueIdSet joinPreds = factTableSetJBBSubset->joinPredsWithOther(*availableNodeJBBSubset);
        joinPreds += availableNodeJBBSubset->getPredsWithPredecessors();
        if(joinPreds.entries() == 0)
        {
          joinReduction += 1000000;
        }
      }

      if((smallestNode == NULL_CA_ID) ||
         (joinReduction <= lowestReduction))
      {
        lowestReduction = joinReduction;
        smallestNode = availableNode;
      }

    }

    CANodeIdSet smallestAndAttachedNodes = smallestNode;

    Join * smallestNodeParentJoin = smallestNode.getNodeAnalysis()->getJBBC()
                                                     ->getOriginalParentJoin();
    
    NABoolean smallestNodeParentIsInnerNonSemiJoin =
      (!smallestNodeParentJoin) || smallestNodeParentJoin->isInnerNonSemiJoin();
      
    if ((CmpCommon::getDefault(COMP_BOOL_49) == DF_OFF) &&
        smallestNodeParentIsInnerNonSemiJoin)
      // Add nodes connected to smallestNode by join on their key.
      smallestAndAttachedNodes += smallestNode.getNodeAnalysis()->
                                        getJBBC()->getJBBCsConnectedViaKeyJoins();

    smallestAndAttachedNodes.intersectSet(availableNodes);

    // remove the smallest node from the available nodes
    availableNodes.remove(smallestAndAttachedNodes);

    // add the smallest node to ordered list of available nodes
    availableNodesOrdered.insert(smallestAndAttachedNodes);
    // update the factTableSet for the next iteration
    factTableSet.insert(smallestAndAttachedNodes);
  }

  // end - try to figure out the ordering of the tables in AFT

  // begin - construct left deep join sequence

  // The list CANodeIdSets representing the join
  // sequence produced by this rule
  // Element 0: is inner most / top most element in left deep join sequence
  // Element 1: is left most/outer most element in left deep join sequence
  UInt32 i = 0;

  // first insert tables from AFT
  for(i = availableNodesOrdered.entries(); i > 0; i--)
  {
    leftDeepJoinSequence_.insert(availableNodesOrdered[(i-1)]);
  }

  // insert the fact table
  leftDeepJoinSequence_.insert(factTable);

  NAList<CANodeIdSet> * listOfEdges = listOfEdges_;

  // insert the tables from the BFT
  // tables in the BFT are organized as a list of edges
  for(i = optimalFTLocation_; i > 0; i--)
  {
    leftDeepJoinSequence_.insert((*listOfEdges)[(i-1)]);
  }

  // Consolidate the fringes
  consolidateFringes(factTable, leftDeepJoinSequence_);

  // end - construct left deep join sequence

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "JBBSubsetAnalysis_arrangeTablesAfterFactForStarJoinTypeI_End" <<endl;
  }
#endif //_DEBUG
}

void JBBSubsetAnalysis::arrangeTablesAfterFactForStarJoinTypeII()
{

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream()
      << "JBBSubsetAnalysis_arrangeTablesAfterFactForStarJoinTypeII_Begin"
      << endl;
  }
#endif //_DEBUG


  //Get a handle to ASM
  AppliedStatMan * appStatMan = QueryAnalysis::ASM();

  RelExpr * result = NULL;

  CANodeId factTable;
  if (CmpCommon::getDefault(COMP_BOOL_1) == DF_OFF)
  {
    factTable = getLargestIndependentNode();
  }
  else
  {
    factTable = factTable_;
  }

  // -----------------------------------------------------
  // Begin: Find if there is a full outer join or TSJ node
  // -----------------------------------------------------

  //get CANodeIdSet representing this MultiJoin
  CANodeIdSet childSet = getJBBCs();
  CANodeId fullOuterJoinOrTSJNode = NULL_CA_ID;

  CANodeId currentNode = NULL_CA_ID;
  for(currentNode=childSet.init();
      childSet.next(currentNode);
      childSet.advance(currentNode))
  {
    JBBC * currentJBBC = currentNode.getNodeAnalysis()->getJBBC();
    if (currentJBBC->isFullOuterJoinOrTSJJBBC() && 
        !(currentJBBC->getOriginalParentJoin()))
    {
      fullOuterJoinOrTSJNode = currentNode;
    }
  }

  // -----------------------------------------------------------------
  // End: Find if there is a outerjoin node with large num of tables
  // -----------------------------------------------------------------

  if(fullOuterJoinOrTSJNode != NULL_CA_ID)
    factTable = fullOuterJoinOrTSJNode;

  if(factTable == NULL_CA_ID)
    factTable = getLargestIndependentNode();

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "Using factTable as outermost "\
                    << factTable.getText()<< endl;
  }
#endif //_DEBUG

  CANodeIdSet factTableSet;
  factTableSet.insert(factTable);

  // Get list of JBBCs in this Multi Join, the list should be sorted by the
  // after local pred cardinality of the JBBC
  NAList<CANodeId> * sortedJBBCs = getNodesSortedByLocalPredsCard();

  // compute list of fringes

  //for tracking, which of the JBBCs in the multijoin are still available
  CANodeIdSet availableNodes = getJBBCs();
  availableNodes.remove(factTable);

  // get tables directly connected to Fact Table
  CANodeIdSet connectedTables = factTable.getNodeAnalysis()->getJBBC()
                                                           ->getJoinedJBBCs();
  connectedTables += factTable.getNodeAnalysis()->getJBBC()
                                                ->getJBBCsRequiringInputFromMe();
  
  // remove connected tables from availableNodes
  // this is because we want to make sure only
  // one directly connected table per fringer
  availableNodes -= connectedTables;

  //List of fringes
  NAList<CANodeIdSet> listOfFringes(CmpCommon::statementHeap(),30);

  CANodeIdSet currentEdge;

  UInt32 lookAhead = (ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_18);

  for(UInt32 j=(*sortedJBBCs).entries(); j > 0; j--)
  {
    CANodeId jbbc = (*sortedJBBCs)[(j-1)];

    if (connectedTables.contains(jbbc))
    {
      NABoolean isLegalAddition = factTableSet.legalAddition(jbbc);

      if(!isLegalAddition)
      {
        availableNodes += jbbc;
        continue;
      }

      Join * jbbcParentJoin =
        jbbc.getNodeAnalysis()->getJBBC()->getOriginalParentJoin();

      currentEdge = jbbc;
      CANodeIdSet attachedNodes;

      JBBC * nodeJBBC = jbbc.getNodeAnalysis()->getJBBC();

      // build a fringe if jbbc not connect via routine join
      if((!jbbcParentJoin) ||
         (jbbcParentJoin->isInnerNonSemiNonTSJJoin()))
      {
      currentEdge = extendEdge(jbbc, availableNodes,lookAhead);

      // attachedNodes are nodes connected
      // to jbbc by join on their key
        attachedNodes =
        jbbc.getNodeAnalysis()->
          getJBBC()->
            getJBBCsConnectedViaKeyJoins();

      attachedNodes.intersectSet(availableNodes);
      }

      if (CmpCommon::getDefault(COMP_BOOL_49) == DF_OFF)
      {
        currentEdge += attachedNodes;
        availableNodes -= attachedNodes;
      }

      listOfFringes.insert(currentEdge);
#ifdef _DEBUG
      if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
           CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
      {
        CURRCONTEXT_OPTDEBUG->stream() << "Built a fringe starting from table "\
                        << jbbc.getText()<< endl;
        CURRCONTEXT_OPTDEBUG->stream() << "The fringe is " \
                        << currentEdge.getText()<<endl;
      }
#endif
    }
    else{
      continue;
    }
  }

#ifdef _DEBUG
  if ( CmpCommon::getDefault( NSK_DBG ) == DF_ON  &&
       CmpCommon::getDefault( NSK_DBG_MJRULES_TRACKING ) == DF_ON )
  {
    CURRCONTEXT_OPTDEBUG->stream() << "Following tables are not part of any fringe, " \
                    << "put each of them as a fringe by itself: \n" \
                    << availableNodes.getText()<< endl;
  }
#endif

  // create fringes out of each one of the remaining tables
  // i.e. tables that have not been included as part of any
  // fringe
  for (CANodeId availableNode = availableNodes.init();
       availableNodes.next(availableNode);
       availableNodes.advance(availableNode))
  {
    listOfFringes.insert(availableNode);
  }

  // compute join order
  NAList<CANodeIdSet> orderedListOfFringes(STMTHEAP);
  orderedListOfFringes.insert(factTableSet);


  while (listOfFringes.entries())
  {
    // fringe that produces lowest cardinality when joined to the factTableSet
    CANodeIdSet smallestFringe;

    CostScalar lowestReduction = 0;

    for (UInt32 i = 0;
         i < listOfFringes.entries();
         i ++)
    {
      CANodeIdSet currentFringe = listOfFringes[i];

      NABoolean currentFringeIsARoutine = FALSE;
      
      if(currentFringe.entries() == 1)
      {
        CANodeId fringeNode =
          currentFringe.getFirst();

        if(!factTableSet.legalAddition(fringeNode))
          continue;
          
        JBBC * currentFringeJBBC = fringeNode.getNodeAnalysis()->getJBBC();

        if (currentFringeJBBC->getOriginalParentJoin() &&
            currentFringeJBBC->getOriginalParentJoin()->isRoutineJoin())
          currentFringeIsARoutine = TRUE;
      }

      // get cardinality fo join factTableSet to currentFringe
      CostScalar joinReduction =
        appStatMan->
          computeJoinReduction(factTableSet, currentFringe);

      // xxx kkk
      if ((CmpCommon::getDefault(COMP_BOOL_163) == DF_OFF) &&
          !currentFringeIsARoutine)
      {
        JBBSubset * factTableSetJBBSubset = factTableSet.jbbcsToJBBSubset();
        JBBSubset * currentFringeJBBSubset = currentFringe.jbbcsToJBBSubset();
        ValueIdSet joinPreds = factTableSetJBBSubset->joinPredsWithOther(*currentFringeJBBSubset);
        joinPreds += currentFringeJBBSubset->getPredsWithPredecessors();
        if(joinPreds.entries() == 0)
        {
          joinReduction += 1000000;
        }
      }

      if((!smallestFringe.entries()) ||
         (joinReduction <= lowestReduction))
      {
        lowestReduction = joinReduction;
        smallestFringe = currentFringe;
      }
    }

    listOfFringes.remove(smallestFringe);
    orderedListOfFringes.insert(smallestFringe);
    factTableSet.insert(smallestFringe);
  }

  // join order is not set, the left deep join order going
  // from the outer most to the inner most is:
  // Fact Table, orderedListOfFringes[0], orderedListOfFringest[1] ....., orderedListOfFringes[n]

  // construct the left deep join sequence going from the inner most -> outer most
  //

  // The list CANodeIdSets representing the join
  // sequence produced by this rule
  // Element 0: is inner most / top most element in left deep join sequence
  // Element N: is left most/outer most element in left deep join sequence
  for(UInt32 k = orderedListOfFringes.entries(); k > 0; k--)
  {
    leftDeepJoinSequence_.insert(orderedListOfFringes[(k-1)]);
  }

  // Consolidate the fringes
  consolidateFringes(factTable, leftDeepJoinSequence_);

}

void JBBSubsetAnalysis::consolidateFringes(CANodeId factTable,
                                           NAList<CANodeIdSet> &leftDeepJoinSequence)
{
  if(leftDeepJoinSequence.entries() < 3)
    return;

  //Get a handle to ASM
  AppliedStatMan * appStatMan = QueryAnalysis::ASM();

  CANodeIdSet factTableSet;

  UInt32 i= 0;

  // go through the join sequence till the fact table is encountered
  for(i = leftDeepJoinSequence.entries()-1;
      !factTableSet.contains(factTable);
      i--)
  {
    factTableSet.insert(leftDeepJoinSequence[i]);

    if (i == 0)
      break;
  }

  CostScalar r1 = 0;

  CostScalar significanceFactor =
    (ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_21);

  UInt32 numRowsLimit =
    (ActiveSchemaDB()->getDefaults()).getAsLong(COMP_INT_14);

  while (i > 0)
  {
    r1 =
      appStatMan->
        joinJBBChildren(factTableSet,leftDeepJoinSequence[i])->
          getResultCardinality();

    UInt32 j = i-1;

    if (leftDeepJoinSequence[i].entries() == 1)
    {
      JBBC * iJBBC =
        leftDeepJoinSequence[i].getFirst().
          getNodeAnalysis()->getJBBC();

      Join * iParentJoin =
        iJBBC->getOriginalParentJoin();

      if(iParentJoin &&
         !(iParentJoin->isInnerNonSemiNonTSJJoin()))
      {
        factTableSet.insert(leftDeepJoinSequence[i]);
        i--;
        continue;
      }
    }

    if (leftDeepJoinSequence[j].entries() == 1)
    {
      JBBC * jJBBC =
        leftDeepJoinSequence[j].getFirst().
          getNodeAnalysis()->getJBBC();

      Join * jParentJoin =
        jJBBC->getOriginalParentJoin();

      if(jParentJoin &&
         !(jParentJoin->isInnerNonSemiNonTSJJoin()))
      {
        factTableSet.insert(leftDeepJoinSequence[i]);
        i--;
        continue;
      }
    }

    CostScalar cardD1D2 = 0;

    if ( CmpCommon::getDefault( COMP_BOOL_117 ) == DF_OFF )
      cardD1D2 =
        appStatMan->
          joinJBBChildren(leftDeepJoinSequence[i],leftDeepJoinSequence[j])->
            getResultCardinality();
    else
    {
      cardD1D2 =
      appStatMan->
        getStatsForCANodeIdSet(leftDeepJoinSequence[i])->
          getResultCardinality();

      cardD1D2 *=
      appStatMan->
        getStatsForCANodeIdSet(leftDeepJoinSequence[j])->
          getResultCardinality();
    }

    // multiplying by 1000000 to save on COMP_FLOATs
    if (((r1 * significanceFactor) > (cardD1D2 * 1000000)) &&
        !(cardD1D2 > numRowsLimit))
    {
      leftDeepJoinSequence[i].insert(leftDeepJoinSequence[j]);
      leftDeepJoinSequence.removeAt(j);
      i--;
    }
    else
    {
      factTableSet.insert(leftDeepJoinSequence[i]);
      i--;
    }
  }
}

void JBBSubsetAnalysis::addMVMatch(MVMatchPtr match, NABoolean isPreferred)
{
  if (isPreferred)
    matchingMVs_.insertAt(0, match);
  else
    matchingMVs_.insert(match);
}

// -----------------------------------------------------------------------
// Methods for class JBBWA
// -----------------------------------------------------------------------

const CASortedList * JBBWA::getNodesSortedByLocalPredsCard()
{
  if(!byLocalPredsCard_)
    byLocalPredsCard_ = sort(&JBBWA::getCardinalityAfterLocalPreds);

  return byLocalPredsCard_;
}

const CASortedList * JBBWA::getNodesSortedByLocalKeyPrefixPredsCard()
{
  if(!byLocalKeyPrefixPredsCard_)
    byLocalKeyPrefixPredsCard_ = sort(&JBBWA::getCardinalityAfterLocalKeyPrefixPreds);

  return byLocalKeyPrefixPredsCard_;
}

// LCOV_EXCL_START :cnu
const CASortedList * JBBWA::getNodesSortedByCard()
{
  if(!byNodeCard_)
    byNodeCard_ = sort(&JBBWA::getNodeCardinality);

  return byNodeCard_;
}

const CASortedList * JBBWA::getNodesSortedByData()
{
  if(!byNodeData_)
    byNodeData_ = sort(&JBBWA::getNodeDataSize);

  return byNodeData_;
}
// LCOV_EXCL_STOP

const CASortedList * JBBWA::getNodesSortedByOutputData()
{
  if(!byNodeOutputData_)
    byNodeOutputData_ = sort(&JBBWA::getNodeOutputDataSize);

  return byNodeOutputData_;
}

// Given a node, return it's cardinality after application of local preds
// on the prefix of the clustering key. If logProps is passed in is NULL,
// then ASM will be called to compute the logical properties.
CostScalar
JBBWA::getCardinalityAfterLocalKeyPrefixPreds(CANodeId node,
                                              EstLogPropSharedPtr & logProps)
{
  if (!logProps)
  {
    //Get a handle to ASM
    AppliedStatMan * appStatMan = QueryAnalysis::ASM();

    //get cardinality
    logProps = appStatMan->getStatsForLocalPredsOnCKPOfJBBC(node);
  }

  //return the cardinality
  return logProps->getResultCardinality();
}

// LCOV_EXCL_START :cnu
// Given a node, return it's data-size (i.e. cardinality * rowsize)
// after application of local preds on the prefix of the clustering key
// If logProps is passed in is NULL, then ASM will be called to compute
// the logical properties
CostScalar
JBBWA::getDataSizeAfterLocalKeyPrefixPreds(CANodeId node,
                                           EstLogPropSharedPtr & logProps)
{
  if (!logProps)
  {
    //Get a handle to ASM
    AppliedStatMan * appStatMan = QueryAnalysis::ASM();

    //get cardinality
    logProps = appStatMan->getStatsForLocalPredsOnCKPOfJBBC(node);
  }

  //record i.e. row size for node
  CostScalar nodeRecordSize = -1;

  if(node.getNodeAnalysis()->getTableAnalysis())
    nodeRecordSize = node.getNodeAnalysis()->
                          getTableAnalysis()->
                          getTableDesc()->
                          getNATable()->
                          getRecordLength();

  //return the datasize, datasize will be < 0 if this is not a table
  return (logProps->getResultCardinality() * nodeRecordSize);

}
// LCOV_EXCL_STOP

// Given a node, return it's cardinality after application of local preds.
// If logProps is passed in is NULL, then ASM will be called to compute
// the logical properties
CostScalar
JBBWA::getCardinalityAfterLocalPreds(CANodeId node,
                                     EstLogPropSharedPtr & logProps)
{
  if (!logProps)
  {
    //Get a handle to ASM
    AppliedStatMan * appStatMan = QueryAnalysis::ASM();

    //get cardinality
    logProps = appStatMan->getStatsForCANodeId(node);
  }

  //return the cardinality
  return logProps->getResultCardinality();
}

// LCOV_EXCL_START :cnu
// Given a node, return it's data-size (i.e. cardinality * rowsize)
// after application of local preds. If logProps is passed in is NULL,
// then ASM will be called to compute the logical properties
CostScalar
JBBWA::getDataSizeAfterLocalPreds(CANodeId node,
                                  EstLogPropSharedPtr & logProps)
{
  if (!logProps)
  {
    //Get a handle to ASM
    AppliedStatMan * appStatMan = QueryAnalysis::ASM();

    //get cardinality
    logProps = appStatMan->getStatsForCANodeId(node);
  }

  //record i.e. row size for node
  CostScalar nodeRecordSize = -1;

  if(node.getNodeAnalysis()->getTableAnalysis())
    nodeRecordSize = node.getNodeAnalysis()->
                          getTableAnalysis()->
                          getTableDesc()->
                          getNATable()->
                          getRecordLength();

  //return the datasize, datasize will be < 0 if this is not a table
  return (logProps->getResultCardinality() * nodeRecordSize);
}

// Given a node, return it's base cardinality
CostScalar JBBWA::getBaseCardinality(CANodeId node,
                                     EstLogPropSharedPtr& logProps)
{
  if (!logProps)
  {
    //Get a handle to ASM
    AppliedStatMan * appStatMan = QueryAnalysis::ASM();

    //empty set, i.e. no preds are to be applied
    ValueIdSet emptySet;

    //get cardinality
    logProps = appStatMan->getStatsForCANodeId(node,
                                               (*GLOBAL_EMPTY_INPUT_LOGPROP),
                                               &emptySet);
  }

  //return the cardinality
  return logProps->getResultCardinality();

}

// Given a node, return it's size (i.e. rowcount * rowsize)
CostScalar JBBWA::getBaseDataSize(CANodeId node,
                                  EstLogPropSharedPtr & logProps)
{
  if (!logProps)
  {
    //Get a handle to ASM
    AppliedStatMan * appStatMan = QueryAnalysis::ASM();

    //empty set, i.e. no preds are to be applied
    ValueIdSet emptySet;

    //get cardinality
    logProps = appStatMan->getStatsForCANodeId(node,
                                               (*GLOBAL_EMPTY_INPUT_LOGPROP),
                                               &emptySet);
  }

  //record i.e. row size for node
  CostScalar nodeRecordSize = -1;

  if(node.getNodeAnalysis()->getTableAnalysis())
    nodeRecordSize = node.getNodeAnalysis()->
                          getTableAnalysis()->
                          getTableDesc()->
                          getNATable()->
                          getRecordLength();

  //return the datasize, datasize will be < 0 if this is not a table
  return (logProps->getResultCardinality() * nodeRecordSize);
}

// Given a node, return it's cardinality
CostScalar JBBWA::getNodeCardinality(CANodeId node,
                                     EstLogPropSharedPtr & logProps)
{
  if (!logProps)
  {
    //Get a handle to ASM
    AppliedStatMan * appStatMan = QueryAnalysis::ASM();

    //get cardinality
    logProps = appStatMan->getStatsForCANodeId(node);
  }

  //return the cardinality
  return logProps->getResultCardinality();

}

// Given a node, return it's size (i.e. rowcount * rowsize)
CostScalar JBBWA::getNodeDataSize(CANodeId node,
                                  EstLogPropSharedPtr & logProps)
{
  if (!logProps)
  {
    //Get a handle to ASM
    AppliedStatMan * appStatMan = QueryAnalysis::ASM();

    //get cardinality
    logProps = appStatMan->getStatsForCANodeId(node);
  }

  //record i.e. row size for node
  CostScalar nodeRecordSize = node.getNodeAnalysis()->getRecordSize();

  //return the datasize, datasize will be < 0 if this is not a table
  return (logProps->getResultCardinality() * nodeRecordSize);
}
// LCOV_EXCL_STOP

// Given a node, return it's output data size (i.e. rowcount * outputrowsize)
CostScalar JBBWA::getNodeOutputDataSize(CANodeId node,
                                  EstLogPropSharedPtr & logProps)
{
  if (!logProps)
  {
    //Get a handle to ASM
    AppliedStatMan * appStatMan = QueryAnalysis::ASM();

    //get cardinality
    logProps = appStatMan->getStatsForCANodeId(node);
  }

  //record i.e. output row size for node
  CostScalar nodeOutputRecordSize = node.getNodeAnalysis()->
                                      getOriginalExpr()->
                                        getGroupAnalysis()->
                                          getGroupAttr()->
                                            getRecordLength();

  //return the datasize, datasize will be < 0 if this is not a table
  return (logProps->getResultCardinality() * nodeOutputRecordSize);
}

// A generic method to sort given a method to compute the sort
// metric for each node
CASortedList* JBBWA::sort(JBBWA::shortMetricFunc getSortMetric)
{
  CANodeIdSet nodesToSort = parentJBB_->getJBBCs();
  CASortedList * result = new (CmpCommon::statementHeap())
    CASortedList(CmpCommon::statementHeap(), nodesToSort.entries());

  while(nodesToSort.entries())
  {


    CANodeId largestNode = nodesToSort.init();
    // coverity cid 819 flags this a CHECKED_RETURN anomaly. Ignore it.
    // coverity[unchecked_value]
    nodesToSort.next(largestNode);
    EstLogPropSharedPtr estLogProps;
    CostScalar largestNodeSortMetric = (*this.*getSortMetric)(largestNode,
                                                              estLogProps);

    for( CANodeId node = largestNode;
         nodesToSort.next(node);
         nodesToSort.advance(node))
    {
      estLogProps = NULL;
      CostScalar nodeSortMetric = (*this.*getSortMetric)(node,
                                                         estLogProps);

      if(nodeSortMetric > largestNodeSortMetric)
      {
	      largestNode = node;
	      largestNodeSortMetric = nodeSortMetric;
      }

    }

    result->insert(largestNode);
    nodesToSort.remove(largestNode);
  }

  return result;
}

// -----------------------------------------------------------------------
// Methods for class JBB
// -----------------------------------------------------------------------

// Add this JBBC to the JBB.
void JBB::addJBBC(CANodeId jbbc)
{
  mainJBBSubset_.addJBBC(jbbc);
  // jbbc id must belong to JBBC node (no need to check for null)
  jbbc.getNodeAnalysis()->getJBBC()->setJBB(this);
}

void JBB::addJBBC(JBBC* jbbc)
{
  mainJBBSubset_.addJBBC(jbbc->getId());
  jbbc->setJBB(this);
}

// LCOV_EXCL_START 
// Currently not used
// Set the GB in the JBB.
void JBB::setGB(CANodeId gb)
{
  mainJBBSubset_.setGB(gb);
  gbAnalysis_ = gb.getNodeAnalysis()->getGBAnalysis();
  // gb id must belong to GBAnalysis node (no need to check for null)
  gbAnalysis_->setJBB(this);
}
// LCOV_EXCL_STOP

// Set the GB in the JBB using GBAnalysis
void JBB::setGB(GBAnalysis* gb)
{
  mainJBBSubset_.setGB(gb->getGroupingNodeId());
  gbAnalysis_ = gb;
  gb->setJBB(this);
}

// Set the norm char inputs and outputs
void JBB::setNormInputs(const ValueIdSet & inputs)
{
  normInputs_ = inputs;
}
void JBB::setNormOutputs(const ValueIdSet & outputs)
{
  normOutputs_ = outputs;
}

void JBB::setInputEstLP(EstLogPropSharedPtr& inEstLP)
{
  inEstLP_ = inEstLP;
}

// JBB Analysis method on Group By
// Capture the GB info and pass the call to child join
void JBB::analyze(GroupByAgg* topGBExpr)
{
  // do GB Analysis stuff here.
  GBAnalysis* gb = topGBExpr->getGBAnalysis();
  gb->setJBB(this);
  setGB(gb);

  // analyse the joins below the GB
  RelExpr* topExpr = (RelExpr*)(topGBExpr->child(0));

  if (topExpr->getOperator().match(REL_ANY_JOIN))
    analyze((Join*)topExpr);
  else if (topExpr->getOperator().match(REL_SCAN))
    analyze((Scan*)topExpr);


  // One potential place to do the R1 and R2 computation
  // for GBAnalysis

  return;
}

// Main JBB Analysis method for Single JBB with GroupBy (MVQR only for now)
void JBB::analyze(Scan* topScanExpr)
{
  RelExpr* expr = topScanExpr;

  JBBC* jbbc = NULL;

  // A CANode must have been assigned to this child in pilot analysis
  CMPASSERT(expr->getGroupAnalysis() &&
            expr->getGroupAnalysis()->getNodeAnalysis());
  jbbc = expr->getGroupAnalysis()->getNodeAnalysis()->getJBBC();
  addJBBC(jbbc);
}

// Main JBB Analysis method
void JBB::analyze(Join* topJoinExpr)
{
  RelExpr* expr = topJoinExpr;

  ValueIdSet joinPreds;  // accumulation of join selectionPreds

  JBBC* jbbc = NULL;

// change start

  while (expr->getOperator().match(REL_ANY_JOIN)&&
         ((Join*)expr)->canBePartOfJBB())
  {
    joinPreds += expr->getSelectionPred();
    expr = expr->child(0);
  }

  ValueIdSet freeRadicalPreds = joinPreds;
  expr = topJoinExpr;
  ValueIdSet childCharOutputs;
  while (expr->getOperator().match(REL_ANY_JOIN)&&
         ((Join*)expr)->canBePartOfJBB())
  {
    // A CANode must have been assigned to this child in pilot analysis
    CMPASSERT(expr->child(1)->getGroupAnalysis() &&
              expr->child(1)->getGroupAnalysis()->getNodeAnalysis());
	  jbbc = expr->child(1)->getGroupAnalysis()->getNodeAnalysis()->getJBBC();
	  addJBBC(jbbc);

    ValueIdSet jbbcJoinPreds;
	  childCharOutputs = expr->child(1)->getGroupAttr()->getCharacteristicOutputs();
  
    // Soln. 10-100203-7927. If we have NullInstantiate(VEG{that contains a constant})
    // then that veg is not included in child charOutputs. The following IF block
    // is to include such VEGs so that we can figure out the correct join linkages.
    Join * parentJoin = jbbc->getOriginalParentJoin() ;

    if (parentJoin && parentJoin->getOperator().match(REL_ANY_LEFT_JOIN))
      childCharOutputs += (ValueIdSet) parentJoin->nullInstantiatedOutput();

    jbbcJoinPreds.accumulateReferencingExpressions(joinPreds, childCharOutputs);
	  jbbc->setJoinPreds(jbbcJoinPreds);

    freeRadicalPreds -= jbbcJoinPreds;
    expr = expr->child(0);
  }

  // do the most left child here
  // A CANode must have been assigned to this child in pilot analysis
  CMPASSERT(expr->getGroupAnalysis() &&
            expr->getGroupAnalysis()->getNodeAnalysis());
  jbbc = expr->getGroupAnalysis()->getNodeAnalysis()->getJBBC();
  addJBBC(jbbc);

  ValueIdSet jbbcJoinPreds;
  childCharOutputs = expr->getGroupAttr()->getCharacteristicOutputs();
  jbbcJoinPreds.accumulateReferencingExpressions(joinPreds, childCharOutputs);
  jbbc->setJoinPreds(jbbcJoinPreds);
  freeRadicalPreds -= jbbcJoinPreds;

  expr = topJoinExpr;

  while (expr->getOperator().match(REL_ANY_JOIN)&&
         ((Join*)expr)->canBePartOfJBB())
  {
    if(expr->getOperatorType() == REL_LEFT_JOIN)
    {
      ValueIdSet freePredsToAddBack = freeRadicalPreds;
      freePredsToAddBack.intersectSet(expr->getSelectionPred());
      jbbc = expr->child(1)->getGroupAnalysis()->getNodeAnalysis()->getJBBC();
      ValueIdSet jbbcJoinPreds = jbbc->getJoinPreds();
      jbbcJoinPreds += freePredsToAddBack;
      jbbc->setJoinPreds(jbbcJoinPreds);
    }
    expr = expr->child(0);
  }

// change end

/*
old one
  while (expr->getOperator().match(REL_ANY_JOIN))
  {
    ValueIdSet childCharOutputs =
      expr->child(1)->getGroupAttr()->getCharacteristicOutputs();

    ValueIdSet otherChildCharOutputs =
      expr->child(0)->getGroupAttr()->getCharacteristicOutputs();

    ValueIdSet selectionPreds = expr->getSelectionPred();

	jbbc = expr->child(1)->getGroupAnalysis()->getNodeAnalysis()->getJBBC();

	addJBBC(jbbc);

    // Add any selectionPreds or a (joinPreds that reference child's
    // char output) to joinPreds_ or nonLocalOrPreds_(future) of the JBBC
    // Warning: we are not doing the OR case yet 
    ValueIdSet jbbcJoinPreds = selectionPreds;
    jbbcJoinPreds.accumulateReferencingExpressions(joinPreds, childCharOutputs);

	jbbc->setJoinPreds(jbbcJoinPreds);

    joinPreds.removeNonReferencingExpressions(otherChildCharOutputs);

    joinPreds.accumulateReferencingExpressions(selectionPreds,
                                              otherChildCharOutputs);

    expr = expr->child(0);
  }

  // do the most left child here
  jbbc = expr->getGroupAnalysis()->getNodeAnalysis()->getJBBC();
  addJBBC(jbbc);

  jbbc->setJoinPreds(joinPreds);
*/

  // now that all JBBCs are added and their predicates are set
  // compute the other fields in the JBBC
  const CANodeIdSet & jbbcs = getJBBCs();
  CANodeId i,j;
  for (i = jbbcs.init(); jbbcs.next(i); jbbcs.advance(i))
  {
    JBBC* jbbci = i.getNodeAnalysis()->getJBBC();
    for (j=i+1; jbbcs.next(j); jbbcs.advance(j))
    {
      JBBC* jbbcj = j.getNodeAnalysis()->getJBBC();
      ValueIdSet commonPreds = jbbci->getJoinPreds();
      commonPreds.intersectSet(jbbcj->getJoinPreds());
      if (commonPreds.entries() > 0)
      {
        jbbci->joinedJBBCs_.insert(j);
        jbbcj->joinedJBBCs_.insert(i);
      }
    }
  }
}

void JBB::analyzeChildrenDependencies()
{
  const CANodeIdSet & jbbcs = getJBBCs();

  CANodeId i,j;
  for (i = jbbcs.init(); jbbcs.next(i); jbbcs.advance(i))
  {
    JBBC* jbbci = i.getNodeAnalysis()->getJBBC();
    RelExpr * const expri = i.getNodeAnalysis()->getOriginalExpr();
    GroupAttributes * const attri = expri->getGroupAttr();
    ValueIdSet charOutputsi = attri->getCharacteristicOutputs();
    
    // set inputs required from sibling JBBCs
    ValueIdSet inputsRequiredFromSiblings;
    
    if(jbbci->getOriginalParentJoin() &&
       jbbci->getOriginalParentJoin()->isRoutineJoin())
    {
      inputsRequiredFromSiblings = attri->getCharacteristicInputs();
      inputsRequiredFromSiblings -= getNormInputs();
      jbbci->setInputsRequiredFromSiblings(inputsRequiredFromSiblings);
    }
    
    for (j= jbbcs.init(); jbbcs.next(j); jbbcs.advance(j))
    {
      if(i == j)
        continue;

      JBBC* jbbcj = j.getNodeAnalysis()->getJBBC();
      RelExpr * const exprj = j.getNodeAnalysis()->getOriginalExpr();
      GroupAttributes * const attrj = exprj->getGroupAttr();
      Join * parentJoinj = jbbcj->getOriginalParentJoin();

      // find if j depends on i
      ValueIdSet predsWithDependencies = jbbci->getPredsWithSuccessors();
      predsWithDependencies.intersectSet(jbbcj->getPredsWithPredecessors());
      if (predsWithDependencies.entries() > 0)
      {
        jbbci->successorJBBCs_.insert(j);
        jbbcj->predecessorJBBCs_.insert(i);
      }
      
      if(inputsRequiredFromSiblings.entries())
      {
        ValueIdSet charOutputsj = attrj->getCharacteristicOutputs();
        if(charOutputsj.intersectSet(inputsRequiredFromSiblings).entries())
        {
          jbbci->predecessorJBBCs_.insert(j);
          jbbci->jbbcsProvidingInput_.insert(j);
          jbbcj->successorJBBCs_.insert(i);
          jbbcj->jbbcsRequiringInputFromMe_.insert(i);
        }
      }
    }
  }
}

void JBB::computeLeftJoinFilterPreds()
{
  const CANodeIdSet & jbbcs = getJBBCs();

  CANodeId i;
  for (i = jbbcs.init(); jbbcs.next(i); jbbcs.advance(i))
  {
    JBBC* jbbci = i.getNodeAnalysis()->getJBBC();

    if(!jbbci->parentIsLeftJoin())
      continue;

    CANodeIdSet jbbciIdSet;
    jbbciIdSet.insert(i);
    JBBSubset * jbbciSubset = jbbciIdSet.jbbcsToJBBSubset();

    CANodeIdSet othersIdSet(getJBBCs());
    othersIdSet -= i;
    JBBSubset * othersJBBSubset = othersIdSet.jbbcsToJBBSubset();

    ValueIdSet predsWithOthers =
      jbbciSubset->joinPredsWithOther(*othersJBBSubset);

    ValueIdSet leftJoinFilterPreds = jbbci->getJoinPreds();
    leftJoinFilterPreds -= predsWithOthers;

    // compute predicates that are covered by the JBBC's output
    ValueIdSet predsCoveredByJBBC;

    for(ValueId joinPred = predsWithOthers.init(); 
                           predsWithOthers.next(joinPred); 
                           predsWithOthers.advance(joinPred))
    {
      CANodeIdSet jbbcNodeSet;
      jbbcNodeSet.insert(i);
      JBBSubset * jbbcSubset = jbbcNodeSet.jbbcsToJBBSubset();
      if(jbbcSubset->coversExpr(joinPred))
        predsCoveredByJBBC += joinPred;
    }
    
    leftJoinFilterPreds += predsCoveredByJBBC;
    
    // set the left join filter preds
    jbbci->setLeftJoinFilterPreds(leftJoinFilterPreds);
  }
}

void JBB::computeChildrenSubGraphDependencies()
{
  /*
  CANodeIdSet jbbcs = getJBBCs();
  
  //SubGraphs produced by excluding nodes that are not innerNonSemiNonTSJ
  CANodeIdSet subGraph = jbbcs;
  subGraph.intersectSet(QueryAnalysis::Instance()->getInnerNonSemiNonTSJJBBCs());
  CANodeIdSet jbbcsNotInSubGraph = jbbcs;
  jbbcsNotInSubGraph -= subGraph;
  NAList<CANodeIdSet*>* innerJoinedJBBCsConnectedSubGraphs =
    subGraph.jbbcsToJBBSubset()->getJBBSubsetAnalysis()->getConnectedSubgraphs();
  int numInnerJoinedJBBCsConnectedSubGraphs = 
    innerJoinedJBBCsConnectedSubGraphs->entries();
  
  // SubGraphs produced by just following inner joins, i.e. not traversing
  // successor/predecessor relationships (i.e. left, semi, routine joins)
  NABoolean followSuccessors = FALSE;
  NAList<CANodeIdSet*>* innerJoinConnectedSubGraphs =
    jbbcs.jbbcsToJBBSubset()->getJBBSubsetAnalysis()->getConnectedSubgraphs(followSuccessors);
  int numInnerJoinConnectedSubGraphs =
    innerJoinConnectedSubGraphs->entries();
  */
}

NABoolean JBB::hasMandatoryXP() const
{
  NAList<CANodeIdSet*>* connectedSubgraphs =
    getMainJBBSubset().getJBBSubsetAnalysis()->getConnectedSubgraphs();

  if(connectedSubgraphs)
    return (connectedSubgraphs->entries() > 1);
  else
    return FALSE;
}

// LCOV_EXCL_START :dpm
const NAString JBB::graphDisplay(const QueryAnalysis* qa) const
{
  NAString result = "";

  CANodeIdSet jbbcs = mainJBBSubset_.getJBBCs();
  CANodeIdSet usedJbbcs_;

  AppliedStatMan * appStatMan = qa->getASM();

  for (CANodeId x= jbbcs.init();  jbbcs.next(x); jbbcs.advance(x) )
  {
    CANodeIdSet cons = x.getNodeAnalysis()->getJBBC()->getJoinedJBBCs();
    cons -= usedJbbcs_;

    NAString name;
    if (x.getNodeAnalysis()->getTableAnalysis())
    {
      name = x.getNodeAnalysis()->getTableAnalysis()->getTableDesc()
		->getNATable()->getTableName().getObjectName();
    }
    else
    {
      name = "JBBC:#" + istring(x);
    }

    ValueIdSet emptySet;

    CostScalar cardx0 = 0;
    CostScalar cardx1 = 0;
    TableAnalysis* tableAn = x.getNodeAnalysis()->getTableAnalysis();

    if (tableAn)
    {
      cardx0 = tableAn->getCardinalityOfBaseTable();
      cardx1 = tableAn->getCardinalityAfterLocalPredsOnCKPrefix();
    }

    CostScalar cardx2 = x.getNodeAnalysis()->getCardinality();

    ULng32 intx0 = (Int32) cardx0.value();
    ULng32 intx1 = (Int32) cardx1.value();
    ULng32 intx2 = (Int32) cardx2.value();

    result += istring(x) + " [label =\"" + name + ":\\n" + istring(intx0)
              + "\\n" + istring(intx1) + "\\n" + istring(intx2) + "\"];\n";

    for (CANodeId y= cons.init();  cons.next(y); cons.advance(y))
    {
      CostScalar cardy = appStatMan->getStatsForCANodeId(y)->getResultCardinality();
      CANodeIdSet xy = CANodeIdSet(x);
      xy += y;
      CANodeIdSet xSet = CANodeIdSet(x);
      CANodeIdSet ySet = CANodeIdSet(y);

      CostScalar cardxy = appStatMan->joinJBBChildren(xSet, ySet)\
                              ->getResultCardinality();

      CostScalar xcardy = appStatMan->getStatsForJoinPredsOnCKOfJBBC(xSet,y)\
                              ->getResultCardinality();

      CostScalar ycardx = appStatMan->getStatsForJoinPredsOnCKOfJBBC(ySet,x)\
                              ->getResultCardinality();

      ULng32 ixy = (Int32) cardxy.value();
      ULng32 ixjy = (Int32) xcardy.value();
      ULng32 iyjx = (Int32) ycardx.value();

      result += istring(x) + " -- " + istring(y) + " [label =\""
                + istring(ixjy) + " , " + istring(ixy) + " , " + istring(iyjx) + "\"];\n";
    }

	usedJbbcs_ += x;
  }

  return result;
}

const NAString JBB::getText() const
{
  NAString result("JBB # ");
  result += istring(jbbId_) + " :\n";
  result += "mainJBBSubset_ :" + mainJBBSubset_.getText();
  CANodeIdSet jbbcs = mainJBBSubset_.getJBBCs();

  for (CANodeId x= jbbcs.init();  jbbcs.next(x); jbbcs.advance(x) )
  {
    result += x.getNodeAnalysis()->getJBBC()->getText();
  }

  // add group by analysis here
  if (gbAnalysis_) {
    result += "\nGBAnalysis: \n";
    result += gbAnalysis_->getText();
    result += "\n";

    GroupByAgg *gba = gbAnalysis_->getOriginalGBExpr();
    ValueIdSet gbvis = gba->groupExpr();
    result += "GBItem Expression:\n";
    for (ValueId gbvid = gbvis.init(); gbvis.next(gbvid); gbvis.advance(gbvid)) {
      ItemExpr *gbie = gbvid.getItemExpr();
      gbie->unparse(result, OPTIMIZER_PHASE, MVINFO_FORMAT);
      result += "\n";
    }
  }

  return result;
}

void JBB::display() const
{
  print();
}

void JBB::print (FILE *f,
	      const char * prefix,
	      const char * suffix) const
{
  fprintf (f, getText());
}


// -----------------------------------------------------------------------
// Methods for class ColAnalysis
// -----------------------------------------------------------------------

const NAString ColAnalysis::getText() const
{
  NAString result("ColAnalysis # ");
  result += istring(column_) + ":\n";

  Int32 i = (Int32)((CollIndex) column_); // valueid as an integer
  NAString unparsed(CmpCommon::statementHeap());
  column_.getItemExpr()->unparse(unparsed); // expression as ascii string
  result += unparsed + "\n";
  result += "vegPreds: " + valueIdSetGetText(vegPreds_) + "\n";
  result += "referencingPreds: " + valueIdSetGetText(referencingPreds_) + "\n";

  return result;
}
// LCOV_EXCL_STOP

void ColAnalysis::finishAnalysis()
{
  connectedCols_.clear();

  for (ValueId pred = vegPreds_.init();
       vegPreds_.next(pred);
       vegPreds_.advance(pred))
  {
    connectedCols_ += pred.predAnalysis()->getVegConnectedCols();
  }
  // remove self and cols from same table
  connectedCols_ -= table_->getUsedCols();

  // Now compute connectedTables_;
  connectedTables_.clear();
  for (ValueId c= connectedCols_.init();
       connectedCols_.next(c);
       connectedCols_.advance(c) )
  {
    connectedTables_ += c.colAnalysis()->
        getTableAnalysis()->getNodeAnalysis()->getId();
  }
  // assert self is not there

  return;
}

// LCOV_EXCL_START 
// Currently not used
ValueIdSet ColAnalysis::getConnectingVegPreds(ValueId other)
{
  ValueIdSet result = vegPreds_;
  result.intersectSet(other.colAnalysis()->getVegPreds());
  return result;
}
// LCOV_EXCL_STOP

// find all local preds referencing this column
// I am considering caching this localPred xxx
const ValueIdSet ColAnalysis::getLocalPreds() const
{
  ValueIdSet result(table_->getLocalPreds());
  result.intersectSet(referencingPreds_);
  return result;
}

// LCOV_EXCL_START 
// This code was being called from LargeScopeRule.cpp method
// MJStarJoinIRule::isAStarPattern but that code is no longer used
// Get all sibling JBBCs that are joined to this table on this column
// (via VEG predicates).
CANodeIdSet ColAnalysis::getConnectedJBBCs()
{
  // Must be JBBC
  JBBC* myJBBC = table_->getNodeAnalysis()->getJBBC();
  CMPASSERT(myJBBC);

  CANodeIdSet jbbcs(myJBBC->getJoinedJBBCs());
  CANodeIdSet myJoinedJBBCs;

  CANodeId i;
  for (i = jbbcs.init(); jbbcs.next(i); jbbcs.advance(i))
  {
    ValueIdSet commonPreds = i.getNodeAnalysis()->getJBBC()->getJoinPreds();
    // xxx for now we only consider veg preds
    // In the future we may include range and non-Veg-equality as well (still not decided)
    commonPreds.intersectSet(vegPreds_);
    if (commonPreds.entries() > 0)
    {
      myJoinedJBBCs.insert(i);
    }
  }

  return myJoinedJBBCs;
}
// LCOV_EXCL_STOP

// Get all sibling JBBCs that are joined to this table on this column
// these include JBBCs joined via non_VEG preds like t1.a = t2.b + 7
CANodeIdSet ColAnalysis::getAllConnectedJBBCs()
{
  // Must be JBBC
  JBBC* myJBBC = table_->getNodeAnalysis()->getJBBC();
  CMPASSERT(myJBBC);

  CANodeIdSet jbbcs(myJBBC->getJoinedJBBCs());
  CANodeIdSet myJoinedJBBCs;

  CANodeId i;
  for (i = jbbcs.init(); jbbcs.next(i); jbbcs.advance(i))
  {
    if(equalityConnectedJBBCs_.contains(i))
    {
      myJoinedJBBCs.insert(i);
      continue;
    }

    ValueIdSet commonPreds = i.getNodeAnalysis()->getJBBC()->getJoinPreds();
    commonPreds.intersectSet(vegPreds_);
    if (commonPreds.entries() > 0)
    {
      myJoinedJBBCs.insert(i);
    }
  }

  return myJoinedJBBCs;
}

// LCOV_EXCL_START 
// This is no longer used. Seems to have been replaced by getAllConnectingPreds
// Get connecting Veg Preds between me and this JBBC.
ValueIdSet ColAnalysis::getConnectingPreds(JBBC* jbbc)
{
  ValueIdSet commonPreds = jbbc->getJoinPreds();
  // xxx for now we only consider veg preds
  // In the future we may include range and non-Veg-equality as well (still not decided)
  commonPreds.intersectSet(vegPreds_);
  return commonPreds;
}
// LCOV_EXCL_STOP

// Get connecting Veg Preds between me and this JBBC.
ValueIdSet ColAnalysis::getAllConnectingPreds(JBBC* jbbc)
{
  ValueIdSet commonPreds = jbbc->getJoinPreds();
  // xxx for now we only consider veg preds
  // In the future we may include range and non-Veg-equality as well (still not decided)
  ValueIdSet connectingPreds = vegPreds_;
  if(equalityConnectedJBBCs_.contains(jbbc->getId()))
  {
    for (ValueId pred = equalityConnectingPreds_.init();
                        equalityConnectingPreds_.next(pred);
                        equalityConnectingPreds_.advance(pred))
    {
      const CANodeIdSet * jbbcsConnectedViaPred =
        table_->getEqualityConnectedJBBCs(pred);
      if(jbbcsConnectedViaPred &&
         jbbcsConnectedViaPred->contains(jbbc->getId()))
        connectingPreds += pred;
    }
  }
  commonPreds.intersectSet(connectingPreds);
  return commonPreds;
}

// ****************************************************************

// LCOV_EXCL_START :dpm
// this will be changed to a getText method for ValueIdSet
NAString valueIdSetGetText(const ValueIdSet & set)
{
  NAString result("ValueIdSet {");
  for (ValueId x = set.init(); set.next(x); set.advance(x))
  {
    Int32 i = (Int32)((CollIndex) x); // valueid as an integer
    NAString unparsed(CmpCommon::statementHeap());
    if (x.getItemExpr())
    {
      x.getItemExpr()->unparse(unparsed); // expression as ascii string
      result += unparsed;
      result += " ";
    }
  }
  result += "}";
  return result;
}
// LCOV_EXCL_STOP

// FALSE does not mean column is not a constant. It means we did not verify
// that it is a constant.
NABoolean ColAnalysis::getConstValue(ItemExpr* & cv, 
                                     NABoolean refAConstValue) const
{
  ValueIdSet localPreds(getLocalPreds());

  // iterate over ValueIdSet to find a VEGPred
  for( ValueId vid = localPreds.init();
       localPreds.next(vid);
       localPreds.advance(vid))
  {
    const ItemExpr * ie = vid.getItemExpr();

    if (ie->getOperatorType() == ITM_VEG_PREDICATE)
    {
      VEG *veg = ((VEGPredicate *)ie)->getVEG();

      ValueIdSet vegMembers = veg->getAllValues();

      // See if the VEG group contains a constant.
      // NOTE:  A VEG group should not contain more than 1 constant; but if
      //        that should happen, referencesAConstValue will return the first
      //        constant found.
      if (refAConstValue)
        {
          if (vegMembers.referencesAConstValue(&cv))
            {
              return TRUE;
            }
        }
      else
        {
          if (vegMembers.referencesAConstExpr(&cv))
            {
              return TRUE;
            }
        }
    }
  }
  return FALSE;
}

NAString istring (ULng32 i)
{
  char buffer[20];
  str_itoa(i, buffer);
  return NAString(buffer);
}

JBBSubsetAnalysis* GroupByAgg::getJBBSubsetAnalysis()
{
  JBBSubsetAnalysis* jbbSubsetAnalysis = NULL;

  GBAnalysis* gb = NULL;
  JBB * jbb = NULL;

  if ( (gb  = getGBAnalysis()) &&
       (jbb = gb->getJBB())
     )
  {
     jbbSubsetAnalysis = jbb->getMainJBBSubset().getJBBSubsetAnalysis();
  }

  return jbbSubsetAnalysis;
}

JBBSubsetAnalysis* Scan::getJBBSubsetAnalysis()
{
  JBBSubsetAnalysis* jbbSubsetAnalysis = NULL;

  GroupAttributes *groupAttr = NULL;
  GroupAnalysis *groupAnal =  NULL;
  JBBSubset *jbbSubset =  NULL;
  JBBC * myJBBC = NULL;

  if ((groupAttr = getGroupAttr()) &&
      (groupAnal =  groupAttr->getGroupAnalysis()) &&
      (groupAnal->getAllSubtreeTables().entries()) &&
      (jbbSubset =  groupAnal->getAllSubtreeTables().jbbcsToJBBSubset()) &&
      (jbbSubset->getJBB()))
  {
     jbbSubsetAnalysis =  jbbSubset->getJBBSubsetAnalysis();
  }

  return jbbSubsetAnalysis;
}

// ****************************************************************



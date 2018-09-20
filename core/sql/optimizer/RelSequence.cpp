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
* File:         RelSequence.cpp
* Description:  All the methods of RelSequence() and PhysSequence().
*
* Created:      6/27/97
* Language:     C++
*
*
******************************************************************************
*/
#include "AllItemExpr.h"
#include "ItemSample.h"
#include "AllRelExpr.h"
#include "RelSequence.h"
#include "SchemaDB.h"
#include "GroupAttr.h"
#include "BindWA.h"
#include "NormWA.h"
#include "Cost.h"
#include "CostMethod.h"
#include "opt.h"
#include "Globals.h"

// -----------------------------------------------------------------------
// This file contains most of the methods for the class RelSequence
// This is a departure from the way other RelExpr's are organized.
//

// The RelSequence constructor.
// Inputs:
//
//  child: The child node of this node. presumably a Scan node.
//
RelSequence::RelSequence(RelExpr *child,
                         ItemExpr *requiredOrder,
                         CollHeap *oHeap)
  : RelExpr(REL_SEQUENCE, child, NULL, oHeap),
    requiredOrderTree_(requiredOrder),
    partitionBy_(NULL),
    cancelExprTree_(NULL),
    hasOlapFunctions_(FALSE),
    hasTDFunctions_(FALSE),
    cachedNumHistoryRows_(0),
    cachedHistoryRowLength_(0),
    cachedUnboundedFollowing_(FALSE),
    cachedMinFollowingRows_(0),
    historyInfoCached_(FALSE)
{
  setNonCacheable();
}

RelSequence::RelSequence(RelExpr *child,
                         ItemExpr *partitionBy,
                         ItemExpr *orderBy,
                         CollHeap *oHeap)
  : RelExpr(REL_SEQUENCE, child, NULL, oHeap),
    requiredOrderTree_(orderBy),
    partitionBy_(partitionBy),
    cancelExprTree_(NULL),
    hasOlapFunctions_(FALSE),
    hasTDFunctions_(FALSE),
    cachedNumHistoryRows_(0),
    cachedHistoryRowLength_(0),
    cachedUnboundedFollowing_(FALSE),
    cachedMinFollowingRows_(0),
    historyInfoCached_(FALSE)


{
  setNonCacheable();
}


// RelSequence::~RelSequence() -----------------------------------------------
// The destructor
//
RelSequence::~RelSequence()
{
}

ItemExpr *
RelSequence::removeRequiredOrderTree()
{
  ItemExpr *requiredOrderTree = requiredOrderTree_;

  requiredOrderTree_ = NULL;

  return requiredOrderTree;
}

//++MV
void RelSequence::addRequiredOrderTree(ItemExpr *orderExpr)
{
  ExprValueId c = requiredOrderTree_;

  ItemExprTreeAsList(&c, ITM_ITEM_LIST).insert(orderExpr);
  requiredOrderTree_ = c.getPtr();
}
//--MV 

// RelSequence::topHash() --------------------------------------------------
// Compute a hash value for a chain of derived RelExpr nodes.
// Used by the Cascade engine as a quick way to determine if
// two nodes are identical.
// Can produce false positives (nodes appear to be identical),
// but should not produce false negatives (nodes are definitely different)
//
// Inputs: none (other than 'this')
//
// Outputs: A HashValue of this node and all nodes in the
// derivation chain below (towards the base class) this node.
//
HashValue RelSequence::topHash()
{
  // Compute a hash value of the derivation chain below this node.
  //
  HashValue result = RelExpr::topHash();

  result ^= requiredOrder();
  result ^= partition();
  result ^= sequenceFunctions();
  result ^= sequencedColumns();
  result ^= cancelExpr();

  return result;
}

// RelSequence::duplicateMatch()
// A more thorough method to compare two RelExpr nodes.
// Used by the Cascades engine when the topHash() of two
// nodes returns the same hash values.
//
// Inputs: other - a reference to another node of the same type.
//
// Outputs: NABoolean - TRUE if this node is 'identical' to the
//          'other' node. FALSE otherwise.
//
// In order to match, this node must match all the way down the
// derivation chain to the RelExpr class.
//
// For the RelSequence node, the only relevant data members which
// need to be compared are partition_, requiredOrder_ and sequenceFunctions_
//
NABoolean
RelSequence::duplicateMatch(const RelExpr & other) const
{
  // Compare this node with 'other' down the derivation chain.
  //
  if (!RelExpr::duplicateMatch(other))
    return FALSE;

  // Cast the RelExpr to a RelSequence node. (This must be a RelSequence node)
  //
  const RelSequence &o = (RelSequence &) other;

  // If the required order keys are not the same 
  // then the nodes are not identical
  //
  if (!(requiredOrder() == o.requiredOrder()))
    return FALSE;

  if (!(partition() == o.partition()))
    return FALSE;

  // If the sequence functions are not the same 
  // then the nodes are not identical
  //
  if (!(sequenceFunctions() == o.sequenceFunctions()))
    return FALSE;

  if (!(sequencedColumns() == o.sequencedColumns()))
    return FALSE;

  if (!(cancelExpr() == o.cancelExpr()))
    return FALSE;

  return TRUE;
} // RelSequence::duplicateMatch

// RelSequence::copyTopNode ----------------------------------------------
// Copy a chain of derived nodes (Calls RelExpr::copyTopNode).
// Needs to copy all relevant fields.
// Used by the Cascades engine.
//
// Inputs: derivedNode - If Non-NULL this should point to a node
//         which is derived from this node.  If NULL, then this
//         node is the top of the derivation chain and a node must
//         be constructed.
//
// Outputs: RelExpr * - A Copy of this node.
//
// If the 'derivedNode is non-NULL, then this method is being called
// from a copyTopNode method on a class derived from this one. If it
// is NULL, then this is the top of the derivation chain and an RelSequence
// node must be constructed.
// 
// In either case, the relevant data members must be copied to 'derivedNode'
// and 'derivedNode' is passed to the copyTopNode method of the class
// below this one in the derivation chain (RelExpr::copyTopNode() in this
// case).
// 
RelExpr *
RelSequence::copyTopNode(RelExpr *derivedNode, CollHeap *outHeap)
{
  RelSequence *result;

  if (derivedNode == NULL)
    // This is the top of the derivation chain
    // Create an empty RelSequence node.
    //
    result = new (outHeap) RelSequence();
  else
    // A node has already been constructed as a derived class.
    //
    result = (RelSequence *) derivedNode;

  // Copy the relavant fields.

  result->requiredOrder() = requiredOrder();
  result->partition() = partition();
  result->sequenceFunctions() = sequenceFunctions();
  result->sequencedColumns() = sequencedColumns();
  result->cancelExpr() = cancelExpr();

  result->checkPartitionChangeExpr_  = checkPartitionChangeExpr_;
  result->hasOlapFunctions_ = hasOlapFunctions_;
  result->hasTDFunctions_= hasTDFunctions_;
  result->movePartIdsExpr_ = movePartIdsExpr_;

  //need to review the commented code below
  //result->requiredOrderTree_ = requiredOrderTree_->copyTree(outHeap)->castToItemExpr();
  if (partitionBy_)
    result->partitionBy_ = partitionBy_->copyTree(outHeap)->castToItemExpr();
  //result->cancelExprTree_ = cancelExprTree_->copyTree(outHeap)->castToItemExpr();
  // Copy any data members from the classes lower in the derivation chain.
  //
  return RelExpr::copyTopNode(result, outHeap);
} // RelSequence::copyTopNode


// RelSequence::addLocalExpr() -----------------------------------------------
// Insert into a list of expressions all the expressions of this node and
// all nodes below this node in the derivation chain. Insert into a list of
// names, all the names of the expressions of this node and all nodes below
// this node in the derivation chain. This method is used by the GUI tool
// and by the Explain Function to have a common method to get all the
// expressions associated with a node.
//
// Inputs/Outputs: xlist - a list of expressions.
//                 llist - a list of names of expressions.
//
// The xlist contains a list of all the expressions associated with this
// node. The llist contains the names of these expressions. (This lists
// must be kept in the same order).
// RelSequence::addLocalExpr potentially adds the requiredOrder_ expression
// ("required_order") and the sequenceFunctions_ ("sequence_functions")
//
// It then calls RelExpr::addLocalExpr() which will add any RelExpr
// expressions to the list.
//
void RelSequence::addLocalExpr(LIST(ExprNode *) &xlist,
                               LIST(NAString) &llist) const
{

  if (requiredOrderTree_) {
    xlist.insert(requiredOrderTree_);
    llist.insert("required_order");
  } else if (requiredOrder().entries() > 0) {
    xlist.insert(requiredOrder().rebuildExprTree(ITM_ITEM_LIST));
    llist.insert("required_order");
  }

  if (partitionBy_) {
    xlist.insert(partitionBy_);
    llist.insert("partition_by");
  } else if (partition().entries() > 0) {
    xlist.insert(partition().rebuildExprTree(ITM_ITEM_LIST));
    llist.insert("partition_by");
  }


  if (sequenceFunctions().entries() > 0) {
    xlist.insert(sequenceFunctions().rebuildExprTree(ITM_ITEM_LIST));
    llist.insert("read_functions");
  }

  if (returnSeqFunctions().entries() > 0) {
    xlist.insert(returnSeqFunctions().rebuildExprTree(ITM_ITEM_LIST));
    llist.insert("return_functions");
  }
  if (sequencedColumns().entries() > 0) {
    xlist.insert(sequencedColumns().rebuildExprTree(ITM_ITEM_LIST));
    llist.insert("sequenced_columns");
  }
  if (movePartIdsExpr().entries() > 0) {
    xlist.insert(movePartIdsExpr().rebuildExprTree(ITM_ITEM_LIST));
    llist.insert("move_partition");
  }

  if (cancelExpr().entries() > 0) {
    xlist.insert(cancelExpr().rebuildExprTree(ITM_ITEM_LIST));
    llist.insert("cancel_expression");
  }

  
  if (checkPartitionChangeExpr_.entries() > 0) 
  {
    xlist.insert(checkPartitionChangeExpr_.rebuildExprTree(ITM_ITEM_LIST));
    llist.insert("olap_partition_change");
  }

  RelExpr::addLocalExpr(xlist,llist);
} // RelSequence::addLocalExpr

// RelSequence::getPotentialOutputValues() ---------------------------------
// Construct a Set of the potential outputs of this node.
//
// Inputs: none (other than 'this')
//
// Outputs: outputValues - a ValueIdSet representing the potential outputs
//          of this node.
//
// The potential outputs for the RelSequence node are the sequenced columns
// generated by the RelSequence node. 
//
void
RelSequence::getPotentialOutputValues(ValueIdSet & outputValues) const
{
  // Make sure the ValueIdSet is empty.
  //
  outputValues.clear();

  outputValues += sequencedColumns();

  // Add the values generated by the RelSequence node.
  //
  outputValues += sequenceFunctions();

  // Add the cancel expression.
  //
  outputValues.insertList(cancelExpr());

} // RelSequence::getPotentialOutputValues()


// The destructor
//
PhysSequence::~PhysSequence()
{
}

// PhysSequence::copyTopNode ----------------------------------------------
// Copy a chain of derived nodes (Calls RelSequence::copyTopNode).
// Needs to copy all relevant fields.
// Used by the Cascades engine.
//
// Inputs: derivedNode - If Non-NULL this should point to a node
//         which is derived from this node.  If NULL, then this
//         node is the top of the derivation chain and a node must
//         be constructed.
//
// Outputs: RelExpr * - A Copy of this node.
//
// If the 'derivedNode is non-NULL, then this method is being called
// from a copyTopNode method on a class derived from this one. If it
// is NULL, then this is the top of the derivation chain and a RelSequence
// node must be constructed.
// 
// In either case, the relevant data members must be copied to 'derivedNode'
// and 'derivedNode' is passed to the copyTopNode method of the class
// below this one in the derivation chain (RelSequence::copyTopNode() in this
// case).
// 
RelExpr *
PhysSequence::copyTopNode(RelExpr *derivedNode, CollHeap *outHeap)
{
  PhysSequence *result;

  if (derivedNode == NULL)
    // This is the top of the derivation chain
    // Generate an empty PhysSequence node.
    //
    result = new (outHeap) PhysSequence();
  else
    // A node has already been constructed as a derived class.
    //
    result = (PhysSequence *) derivedNode;

  // PhysSequence has no data members.

  // Copy any data members from the classes lower in the derivation chain.
  //
  return RelSequence::copyTopNode(result, outHeap);
} // PhysSequence::copyTopNode

// RelSequence::transformNode() -------------------------------------------
// Unconditional query transformations such as the transformation of
// a subquery to a semijoin are implemented by the virtual function
// transformNode(). The aim of such transformations is to bring the
// query tree to a canonical form. transformNode() also ensures
// that the "required" (or characteristic) input values are "minimal"
// and the "required" (or characteristic) outputs values are
// "maximal" for each operator.
//
// transformNode() is an overloaded name, which is used for a set
// of methods that implement the transformation phase of query
// normalization.
//
// We use the term query tree for a tree of relational operators,
// each of which can contain none or more scalar expression trees.
// The transformations performed by transformNode() brings scalar
// expressions into a canonical form. The effect of most such
// transformations is local to the scalar expression tree.
// However, the transformation of a subquery requires a semijoin
// to be performed between the relational operator that contains
// the subquery and the query tree for the subquery. The effect
// of such a subquery transformation is therefore visible not
// only in the scalar expression tree but also in the relational
// expression tree.
//
// Parameters:
//
// NormWA & normWARef
//    IN : a pointer to the normalizer work area
//
// ExprGroupId & locationOfPointerToMe
//    IN : a reference to the location that contains a pointer to
//         the RelExpr that is currently being processed.
//
void RelSequence::transformNode(NormWA &normWARef,
                                ExprGroupId &locationOfPointerToMe)
{
  CMPASSERT( this == locationOfPointerToMe );

  // If this node has already been transformed, we are done.
  //
  if (nodeIsTransformed())
    return;
  
  // Make sure that it is only transformed once.
  //
  markAsTransformed();

  // transformNode takes up a bound tree and turns into a transformed
  // tree. For a RelExpr that means the following.
  //    + expressions are transformed. If the expressions contain
  //        subqueries then new RelExpr are created for them and
  //        they are usually added above (as an ancestor) of the node
  //        that contained them.
  //    + predicates are pulled up from the children and their
  //        required inputs are modified
  //    + the required inputs of the node the node itself are changed
  //        from being a sufficient set to being a sufficient minimal
  //        set.
  //
  // Transform the child.
  // Pull up their transformed predicates
  // recompute their required inputs.
  //
  child(0)->transformNode(normWARef, child(0)); 

  if(requiredOrder().
     transformNode(normWARef,
                   child(0),
                   getGroupAttr()->getCharacteristicInputs())) {

    // The requiredOrder list apparently had some subqueries that had
    // not been processed before (is this possible?). Normalize the
    // new tree that has become our child.
    //
    child(0)->transformNode(normWARef, child(0));
  }
  
  if(partition().
     transformNode(normWARef,
                   child(0),
                   getGroupAttr()->getCharacteristicInputs())) {

    // The partition list apparently had some subqueries that had
    // not been processed before (is this possible?). Normalize the
    // new tree that has become our child.
    //
    child(0)->transformNode(normWARef, child(0));
  }
  
  if (CmpCommon::getDefault(COMP_BOOL_201) == DF_ON){
    normWARef.resetSeqFunctionsCache();
  }


  if(sequenceFunctions().
     transformNode(normWARef,
                   child(0),
                   getGroupAttr()->getCharacteristicInputs())) {

    // The sequenceFunctions apparently had some subqueries that had not
    // been processed before (is this possible?). Normalize the new
    // tree that has become our child.
    //
    child(0)->transformNode(normWARef, child(0));
  }
/*
  Note: This code resulted in an internal error when TD RANK code was added. 
  if (CmpCommon::getDefault(COMP_BOOL_200) == DF_ON){
    
    normWARef.resetAllSeqFunctions();
    for(ValueId seqId = sequenceFunctions().init(); sequenceFunctions().next(seqId); 
        sequenceFunctions().advance(seqId) ){
      ItemExpr * ie;
      ie = seqId.getItemExpr();
      normWARef.optimizeSeqFunctions( ie, NULL, 0 );
    }
    
  }*/



  if(sequencedColumns().
     transformNode(normWARef,
                   child(0),
                   getGroupAttr()->getCharacteristicInputs())) {

    // The sequencedColumns apparently had some subqueries that had not
    // been processed before (is this possible?). Normalize the new
    // tree that has become our child.
    //
    child(0)->transformNode(normWARef, child(0));
  }

  if(cancelExpr().
     transformNode(normWARef,
                   child(0),
                   getGroupAttr()->getCharacteristicInputs())) {

    // The cancelExpr apparently had some subqueries that had not
    // been processed before (is this possible?). Normalize the new
    // tree that has become our child.
    //
    child(0)->transformNode(normWARef, child(0));
  }

  // Pull up the predicates and recompute the required inputs
  // of whoever my children are now.
  //
  pullUpPreds();

  // transform the selection predicates
  //
  transformSelectPred(normWARef, locationOfPointerToMe);

} // RelSequence::transformNode()

// RelSequence::rewriteNode() ---------------------------------------------
// rewriteNode() is the virtual function that computes
// the transitive closure for "=" predicates and rewrites value
// expressions.
//
// Parameters:
//
// NormWA & normWARef
//    IN : a pointer to the normalizer work area
//
void RelSequence::rewriteNode(NormWA & normWARef)
{
  RelExpr::rewriteNode(normWARef);

  if(requiredOrder().normalizeNode(normWARef)) {
  }

  if(partition().normalizeNode(normWARef)) {
  }

  if(sequenceFunctions().normalizeNode(normWARef)) {
  }

  if(sequencedColumns().normalizeNode(normWARef)) {
  }

  if(cancelExpr().normalizeNode(normWARef)) {
  }

} // RelSequence::rewriteNode()

RelExpr * RelSequence::normalizeNode(NormWA & normWARef)
{
  RelExpr *result = RelExpr::normalizeNode(normWARef);

  // See RelRoot::normalizeNode(), which has a code segment for a case
  // 10-010321-1842 (details of the case are now lost to history).
  // Since the RelSequence has an order by expression similar to that
  // of the RelRoot, we have to apply an equivalent fix here, so that
  // an OVER(ORDER BY x) will work, even if the expression x refers
  // to something like a parameter or to current_timestamp.
  // For this case we need to
  // enforce that Sort operator can sort on this expression by keeping
  // parameter ?p in RelRoot child's group requiredInput.
  // NOTE. This solution will force the Sort operator to be done
  // directly below the RelSequence node.
  if (requiredOrder_.entries() > 0)
  {
    ValueIdSet orderBySet(requiredOrder_), 
               coveredOrderBySet,
               inputsNeededForOrderBy;

    GroupAttributes * childGAPtr = child(0).getPtr()->getGroupAttr();

    childGAPtr->coverTest(orderBySet,
                          getGroupAttr()->getCharacteristicInputs(),
                          coveredOrderBySet,
                          inputsNeededForOrderBy);

    childGAPtr->addCharacteristicInputs(inputsNeededForOrderBy);
  }

  return result;
}


// RelSequence::pullUpPreds() --------------------------------------------
// is redefined to disallow the pullup of most predicates from the
// operator's child.  RelSequence can not pull up any predicates from
// its child since this will change the semantics of the sequence
// functions.  The exception is that predicates which consist entirely
// of outer references (are covered by the inputs) can (and must) be
// pulled up.  The reason that these predicates can be pulled up is
// that these types of predicates will either be TRUE for all rows and
// therefore have no affect on the result, or will be FALSE for all
// rows and the sequence will not produce any rows (so when the pred
// is pulled up, the request will not even get to sequence). The
// reason that these predicates must be pulled up is because if these
// preds become VEGPreds, they will not be handled properly if they
// cannot be pulled up and pushed to their source (see Genesis case:
// 10-070111-6157)
//
void
RelSequence::pullUpPreds()
{
  // ---------------------------------------------------------------------
  // Pull up predicates from the child.
  // move them to my selection predicates
  // ---------------------------------------------------------------------

  // Only predicates that reference only input values can be pulled
  // up.
  ValueIdSet availableInputs = getGroupAttr()->getCharacteristicInputs();

  ValueIdSet predicatesToPullUp;
  ValueIdSet notUsed;
  ValueIdSet predicatesThatStay;

  GroupAttributes emptyGA;

  // Find the set of predicates on the child which are covered by the inputs
  // These predicates can be pulled up above the Sequence operator
  //
  emptyGA.coverTest(child(0)->selectionPred(),
                    availableInputs,
                    predicatesToPullUp,
                    notUsed,
                    &predicatesThatStay);

  // If there are some predicates that can be pulled, then remove them
  // from the child and add them to my selection predicates
  //
  if (NOT predicatesToPullUp.isEmpty())
    {
      selectionPred() += predicatesToPullUp;
      child(0)->selectionPred() -= predicatesToPullUp;
    }



  // ---------------------------------------------------------------------
  // WARNING: One rule that this procedure must follow is
  // that recomputeOuterReferences() must be called on the children even
  // if no predicates are pulled up from them. This is to correct
  // the outer references that are added to a right child of a
  // semi or outer join when processing subqueries in the ON clause.
  // ---------------------------------------------------------------------
  child(0)->recomputeOuterReferences();


} // RelSequence::pullUpPreds()


// RelSequence::recomputeOuterReferences() --------------------------------
// This method is used by the normalizer for recomputing the
// outer references (external dataflow input values) that are
// still referenced by each operator in the subquery tree
// after the predicate pull up is complete.
//
// Side Effects: sets the characteristicInputs of the groupAttr.
//
void RelSequence::recomputeOuterReferences()
{
  // This is virtual method on RelExpr.  When this is called it is
  // assumed that the children have already been transformed.  The
  // required inputs of the child are therefore already minimal and
  // sufficient.  It is also assumed that the RelExpr itself has been
  // bound.  That implies that the group attributes have already been
  // allocated and the required inputs is a sufficient (but not
  // necessarilly minimum) set of external values needed to evaluate
  // all expressions in this subtree.
  // 
  // Delete all those input values that are no longer referenced on
  // this operator because the predicates that reference them have 
  // been pulled up.
  //
  ValueIdSet outerRefs = getGroupAttr()->getCharacteristicInputs(); 

  // The set of valueIds need by this node.
  //
  ValueIdSet allMyExpr(getSelectionPred());

  allMyExpr.insertList(requiredOrder());
  allMyExpr.insertList(partition());

  allMyExpr += sequenceFunctions();

  allMyExpr += sequencedColumns();

  allMyExpr.insertList(cancelExpr());
  
  // Remove from outerRefs those valueIds that are not needed
  // by all my expressions
  //
  allMyExpr.weedOutUnreferenced(outerRefs);

  // Add to outerRefs those that my children need.
  //
  outerRefs += child(0).getPtr()->getGroupAttr()->getCharacteristicInputs();

  // set my Character Inputs to this new minimal set.
  //
  getGroupAttr()->setCharacteristicInputs(outerRefs);
} // RelSequence::recomputeOuterReferences()  


// RelSequence::synthEstLogProp() ------------------------------------------
// synthesize estimated logical properties given a specific set of
// input log. properties.
//
// Parameters:
//
// EstLogPropSharedPtr inputEstLogProp
//    IN : A set of input logical properties used to estimate the logical
//         properities of this node.
//
void RelSequence::synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp)
{
  if (getGroupAttr()->isPropSynthesized(inputEstLogProp) == TRUE) 
    return;

  // Get the estimated logical properties of the child. To be used
  // to estimate the logical properties of this node.
  //
  EstLogPropSharedPtr childEstProp  = child(0).outputLogProp(inputEstLogProp);

  EstLogPropSharedPtr myEstProps =
    synthEstLogPropForUnaryLeafOp(inputEstLogProp,
                                  childEstProp->getColStats(),
                                  childEstProp->getResultCardinality());

  // Set the logical properties of this node.
  //
  getGroupAttr()->addInputOutputLogProp(inputEstLogProp, myEstProps);
  
} // RelSequence::synthEstLogProp


// RelSequence::synthLogProp ----------------------------------------------
// synthesize logical properties
//
void
RelSequence::synthLogProp(NormWA * normWAPtr)
{
  // check to see whether properties are already synthesized.
  //
  if (getGroupAttr()->existsLogExprForSynthesis()) 
    return;

  // Synthesize log. properties for me and my children.
  //
  RelExpr::synthLogProp(normWAPtr);

  ValueIdSet nonRIConstraints;
  for (ValueId x= child(0).getGroupAttr()->getConstraints().init();
       child(0).getGroupAttr()->getConstraints().next(x);
       child(0).getGroupAttr()->getConstraints().advance(x) )
    {
      if ((x.getItemExpr()->getOperatorType() != ITM_COMP_REF_OPT_CONSTRAINT) &&
	  (x.getItemExpr()->getOperatorType() != ITM_REF_OPT_CONSTRAINT))
	  nonRIConstraints += x;
    }
  getGroupAttr()->addConstraints(nonRIConstraints);
  getGroupAttr()->addSuitableRefOptConstraints
  (child(0).getGroupAttr()->getConstraints());

} // RelSequence::synthLogProp()


// RelSequence::costMethod()
// Obtain a pointer to a CostMethod object providing access
// to the cost estimation functions for nodes of this type.
CostMethod*
PhysSequence::costMethod() const
{
  static THREAD_P CostMethodRelSequence *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap())  CostMethodRelSequence();
  return m;

} // PhysSequence::costMethod()


ValueIdList
RelSequence::mapSortKey(const ValueIdList &sortKey) const
{

  ValueIdMap seqColsMap;

  for(ValueId sequenceCol = sequencedColumns().init();
      sequencedColumns().next(sequenceCol);
      sequencedColumns().advance(sequenceCol)) {

    CMPASSERT(sequenceCol.getItemExpr()->getOperatorType() == ITM_NOTCOVERED);

      seqColsMap.addMapEntry(sequenceCol, 
                             sequenceCol.getItemExpr()->child(0)->getValueId());
    }
  
  ValueIdList newSortKey;
  
  seqColsMap.mapValueIdListUp(newSortKey, sortKey);
  
  return newSortKey;
}

PhysicalProperty *
PhysSequence::synthPhysicalProperty(const Context *context,
                                    const Lng32   pn,
                                    PlanWorkSpace *pws)
{

  // Call the default implementation
  // (RelExpr::synthPhysicalProperty()) to synthesize the properties
  // on the number of cpus.
  // 
  PhysicalProperty* sppTemp = RelExpr::synthPhysicalProperty(context,pn,pws);

  const PhysicalProperty * const sppOfChild =
    context->getPhysicalPropertyOfSolutionForChild(0);

  // ---------------------------------------------------------------------
  // Replace the sort key in my spp.
  // ---------------------------------------------------------------------
  PhysicalProperty* sequencePP =
    new (CmpCommon::statementHeap())
    PhysicalProperty(*sppOfChild,
                     mapSortKey(sppOfChild->getSortKey()),
                     sppOfChild->getSortOrderType(),
                     sppOfChild->getDp2SortOrderPartFunc());

  sequencePP->setCurrentCountOfCPUs(sppTemp->getCurrentCountOfCPUs());
  delete sppTemp;

  return sequencePP;
} //  RelSequence::synthPhysicalProperty() 


void
RelSequence::pushdownCoveredExpr(const ValueIdSet & outputExpr,
                                 const ValueIdSet & newExternalInputs,
                                 ValueIdSet & predicatesOnParent,
				 const ValueIdSet * setOfValuesReqdByParent,
                                 Lng32 childIndex
		                 )
{

  ValueIdSet exprOnParent;
  if(setOfValuesReqdByParent)
    exprOnParent = *setOfValuesReqdByParent;
  exprOnParent += sequenceFunctions();
  exprOnParent.insertList(requiredOrder());
  exprOnParent.insertList(partition());
  ValueIdSet outputSet(outputExpr);

  // Prune from the sequencedColumns() ValueIdSet, those expressions
  // that are not needed above (in setOfValuesReqdByParent) or by
  // the selectionPred.
  //
  ValueId refVal;
  for(ValueId seqCol = sequencedColumns().init(); 
      sequencedColumns().next(seqCol);
      sequencedColumns().advance(seqCol)) {

    if(!exprOnParent.referencesTheGivenValue(seqCol, refVal) &&
       !selectionPred().referencesTheGivenValue(seqCol, refVal) &&
       !outputSet.referencesTheGivenValue(seqCol, refVal)) {
      sequencedColumns() -= seqCol;
    }
  }
  
  exprOnParent += sequencedColumns();

  RelExpr::pushdownCoveredExpr(outputSet,
                               newExternalInputs,
                               predicatesOnParent,
			       &exprOnParent,
                               childIndex
                               );

  // predicatesOnParent has not been changed.
  //

} // RelSequence::pushdownCoveredExpr

Context* RelSequence::createContextForAChild(Context* myContext,
                                             PlanWorkSpace* pws,
                                             Lng32& childIndex)
{
  // ---------------------------------------------------------------------
  // If one Context has been generated for each child, return NULL 
  // to signal completion.
  // ---------------------------------------------------------------------
  if (pws->getCountOfChildContexts() == getArity())
    return NULL;

  childIndex = 0;

  Lng32 planNumber = 0;
  const ReqdPhysicalProperty* rppForMe = myContext->getReqdPhysicalProperty();
  PartitioningRequirement* partReqForMe = 
    rppForMe->getPartitioningRequirement();

  // If a partitioning requirement exists and it requires broadcast
  // replication, then return NULL now. Only an exchange operator
  // can satisfy a broadcast replication partitioning requirement.
  if ((partReqForMe != NULL) AND
      partReqForMe->isRequirementReplicateViaBroadcast())
    return NULL;

  RequirementGenerator rg(child(0),rppForMe);

  // ---------------------------------------------------------------------
  // Add the order requirements needed for this RelSequence node
  // ---------------------------------------------------------------------

  // Remove any sort order requirement from parent.
  //
  rg.removeSortKey();
  rg.removeArrangement();
  rg.removeSortOrderTypeReq();

  ValueIdList sortKey;

  // Note for Bob W.
  // Look into whether requiredOrder() can be used the same way for OLAP
  // and TD Rank so this special case can be eliminated.  
  if( !getHasTDFunctions() )
  {
    sortKey.insert(partition());
  }  

  sortKey.insert(requiredOrder());

  // Shouldn't/Can't add a sort order type requirement
  // if we are in DP2
  if (rppForMe->executeInDP2())
    rg.addSortKey(sortKey,NO_SOT);
  else
    rg.addSortKey(sortKey,ESP_SOT);

  // Cannot execute in DP2
  //
  rg.addLocationRequirement(EXECUTE_IN_MASTER_OR_ESP);

  Lng32 childNumPartsRequirement = ANY_NUMBER_OF_PARTITIONS;
  float childNumPartsAllowedDeviation = 0.0;
  NABoolean numOfESPsForced = FALSE;

  if(okToAttemptESPParallelism(myContext,
                               pws,
                               childNumPartsRequirement,
                               childNumPartsAllowedDeviation,
                               numOfESPsForced) AND
     (partition().entries() > 0)) {

    rg.addPartitioningKey(partition());

    if (NOT numOfESPsForced)
      rg.makeNumOfPartsFeasible(childNumPartsRequirement,
                                &childNumPartsAllowedDeviation);
    rg.addNumOfPartitions(childNumPartsRequirement,
                          childNumPartsAllowedDeviation);
  } else {
    
    // Cannot execute in parallel
    //
    rg.addNumOfPartitions(1);
  }


  // ---------------------------------------------------------------------
  // Done adding all the requirements together, now see whether it worked
  // and give up if it is not possible to satisfy them
  // ---------------------------------------------------------------------
  if (NOT rg.checkFeasibility())
    return NULL;

  // ---------------------------------------------------------------------
  // Compute the cost limit to be applied to the child.
  // ---------------------------------------------------------------------
  CostLimit* costLimit = computeCostLimit(myContext, pws);
  
  // ---------------------------------------------------------------------
  // Get a Context for optimizing the child.
  // Search for an existing Context in the CascadesGroup to which the
  // child belongs that requires the same properties as those in 
  // rppForChild. Reuse it, if found. Otherwise, create a new Context
  // that contains rppForChild as the required physical properties..
  // ---------------------------------------------------------------------
  Context* result = shareContext(
         childIndex,
	 rg.produceRequirement(),
         myContext->getInputPhysicalProperty(),
	 costLimit, 
	 myContext,
	 myContext->getInputLogProp());

  // ---------------------------------------------------------------------
  // Store the Context for the child in the PlanWorkSpace.
  // ---------------------------------------------------------------------
  pws->storeChildContext(childIndex, planNumber, result);

  return result;
  
} // RelSequence::createContextForAChild()

// Helper routine for reporting errors with sequence functions. Called
// by RelRoot::bindNode().
//
static NABoolean checkUnresolvedSequenceFunctions(BindWA *bindWA)
{
  ValueIdSet &seqs = 
    bindWA->getCurrentScope()->getUnresolvedSequenceFunctions();

  if (seqs.isEmpty()) 
    return FALSE;			// no error

  NAString unparsed(CmpCommon::statementHeap());
  for (ValueId vid = seqs.init(); seqs.next(vid); seqs.advance(vid)) {

    ItemExpr *ie = vid.getItemExpr();

    unparsed += ", ";
    ie->unparse(unparsed, DEFAULT_PHASE, USER_FORMAT_DELUXE);
  }
  unparsed.remove(0,2);					// remove initial ", "

  // 4109 Sequence functions placed incorrectly.
  *CmpCommon::diags() << DgSqlCode(-4109) << DgString0(unparsed);
  bindWA->setErrStatus();
  return TRUE;
} // checkUnresolvedSequenceFunctions()

// RelSequence::bindNode - Bind the RelSequence node.
//
RelExpr *RelSequence::bindNode(BindWA *bindWA)
{
  
  // If this node has already been bound, we are done.
  //
  if (nodeIsBound())
    return this;

  CMPASSERT(!bindWA->getCurrentScope()->getSequenceNode());

  bindWA->getCurrentScope()->getSequenceNode()= this;

  // Add sequence functions found in parent node(s)
  //
  sequenceFunctions() += 
    bindWA->getCurrentScope()->getUnresolvedSequenceFunctions();

  bindWA->getCurrentScope()->getUnresolvedSequenceFunctions().clear();
  bindWA->getCurrentScope()->getAllSequenceFunctions().clear();
  
  // Bind the child nodes.
  //
  bindChildren(bindWA);
  if (bindWA->errStatus())
    return this;

  ItemExpr * partitionChange =NULL;
  ValueIdList change;
  if(partitionBy_) {
    partitionChange = partitionBy_->copyTree(bindWA->getCurrentScope()->collHeap());    
    bindWA->getCurrentScope()->context()->inOrderBy() = TRUE;
    partitionBy_->convertToValueIdList(partition(),
                                       bindWA,
                                       ITM_ITEM_LIST);
    bindWA->getCurrentScope()->context()->inOrderBy() = FALSE;
    if(bindWA->errStatus())
      return this;

    partitionBy_ = NULL;
  }

  ItemExpr *requiredOrderTree = removeRequiredOrderTree();
  
  if(requiredOrderTree) {
    bindWA->getCurrentScope()->context()->inOrderBy() = TRUE;
    requiredOrderTree->convertToValueIdList(requiredOrder(),
                                            bindWA,
                                            ITM_ITEM_LIST);
    bindWA->getCurrentScope()->context()->inOrderBy() = FALSE;

/*
    //
    // Next, we enforce the restriction that no CZECH collation
    // is involved in any of the SEQUENCE BY columns/expressions.
    //
    for (CollIndex i = 0; i < requiredOrder().entries(); i++)
    {
        ValueId ValId        = requiredOrder()[i];
        NABuiltInTypeEnum TQ = ValId.getType().getTypeQualifier();

        if ( TQ == NA_CHARACTER_TYPE)
        {
            const CharType& ColumnCT = (const CharType&)ValId.getType();
            if ( ColumnCT.getCollation() == CharInfo::CZECH_COLLATION )
            {
                *CmpCommon::diags() << DgSqlCode(-4380)
                    << DgString0(ColumnCT.getCollationName());
                bindWA->setErrStatus();
            }
        }
    }
*/

    if(bindWA->errStatus())
      return this;
  }
  
  
  

  // if unresolved sequence functions have been found in the children
  // of the Sequence node or in the requiredOrderTree, that would mean
  // that we are referencing sequence functions before the sequence
  // operation is performed If this is the case, the following method
  // will issue the error.
  //
  if (checkUnresolvedSequenceFunctions(bindWA))
    return this;

  // Construct the RETDesc for this node.
  //
  RETDesc *resultTable = new(bindWA->wHeap()) RETDesc(bindWA);
  
  // Add the columns from the child to the RETDesc.
  // 
  const RETDesc &childTable = *child(0)->getRETDesc();

  if(childTable.isGrouped())
    resultTable->setGroupedFlag();
  
  const ColumnDescList *sysColList = childTable.getSystemColumnList();
  const ColumnDescList *colList = childTable.getColumnList();
  
  CollIndex i = 0;
  for(i = 0; i < sysColList->entries(); i++) 
  {
    ValueId columnValueId = sysColList->at(i)->getValueId();
    ItemExpr *newColumn = new (bindWA->wHeap()) 
      NotCovered (columnValueId.getItemExpr());
    newColumn->synthTypeAndValueId();
    
    resultTable->addColumn(bindWA,
      sysColList->at(i)->getColRefNameObj(),
      newColumn->getValueId(),
      SYSTEM_COLUMN,
      sysColList->at(i)->getHeading());

    if(sysColList->at(i)->isGrouped()) {
      ColumnNameMap *cnm =
        resultTable->findColumn(sysColList->at(i)->getColRefNameObj());
      cnm->getColumnDesc()->setGroupedFlag();
    }

    sequencedColumns() += newColumn->getValueId();
  }
  ValueIdMap ncToOldMap;
  for(i = 0; i < colList->entries(); i++) 
  {
    ValueId columnValueId = colList->at(i)->getValueId();
    ItemExpr *newColumn = new (bindWA->wHeap()) 
      NotCovered (columnValueId.getItemExpr());
    newColumn->synthTypeAndValueId();
    newColumn->setOrigOpType(columnValueId.getItemExpr()->origOpType());
    
    resultTable->addColumn(bindWA,
      colList->at(i)->getColRefNameObj(),
      newColumn->getValueId(),
      USER_COLUMN,
      colList->at(i)->getHeading());
    ncToOldMap.addMapEntry(newColumn->getValueId(),columnValueId);
    if(colList->at(i)->isGrouped()) {
      ColumnNameMap *cnm =
        resultTable->findColumn(colList->at(i)->getColRefNameObj());
      cnm->getColumnDesc()->setGroupedFlag();
    }

    sequencedColumns() += newColumn->getValueId();
  }
  
  // Set the return descriptor
  //
  setRETDesc(resultTable);
  bindWA->getCurrentScope()->setRETDesc(resultTable);

  //bindWA->getCurrentScope()->getSequenceNode() = this;
  if (!bindWA->getCurrentScope()->context()->inUpsertXform())
    {
      if(partitionChange) {
        partitionChange->convertToValueIdList(partitionChange_, bindWA, ITM_ITEM_LIST);
      }
    
    if(bindWA->errStatus())
      return this;
  
    partitionChange = NULL;

   /// bindWA->getCurrentScope()->setTDPartitionChange(change);
  }
  // -- MV
  // Bind the cancel expression.
  //
  if (cancelExprTree_)
  {
    cancelExprTree_->convertToValueIdList(cancelExpr(), bindWA, ITM_ITEM_LIST);
    cancelExprTree_ = NULL;
    if (bindWA->errStatus()) {
      delete resultTable;
      return NULL;
    }
  }

  //
  // bind the qualify predicates and attach the resulting value id set
  // to the node (as a selection predicate on the sequence node)
  //
  ItemExpr *qualifyPred = removeSelPredTree();
  if (qualifyPred) {
	  bindWA->getCurrentScope()->context()->inQualifyClause() = TRUE;
	  qualifyPred->convertToValueIdSet(selectionPred(), bindWA, ITM_AND);
	  bindWA->getCurrentScope()->context()->inQualifyClause() = FALSE;
	  if (bindWA->errStatus()) return this;
  }


  bindWA->getCurrentScope()->getSequenceNode() = NULL;

  //
  // Bind the base class.
  //
  RelExpr *boundExpr = bindSelf(bindWA);
  if (bindWA->errStatus())
    return boundExpr;

  // Register this node in current scope.  Later (in
  // RelRoot::bindNode()), the unresolved sequence functions will be
  // attached to this registered node.
  //
  //CMPASSERT(!bindWA->getCurrentScope()->getSequenceNode());

  bindWA->getCurrentScope()->getSequenceNode() = boundExpr;
  // save the ncToOldmap in the current scope. It will be used in Insert::bindnode for a special case.
  bindWA->getCurrentScope()->setNCToOldMap( ncToOldMap);
  return boundExpr;

} // RelSequence::bindNode()

NABoolean PhysSequence::isBigMemoryOperator(const Context* context,  
                                            const Lng32 planNumber)
{
  const double memoryLimitPerCPU = CURRSTMT_OPTDEFAULTS->getMemoryLimitPerCPU();

  // The Sequence operator allocates the history buffer in memory.
  //

  const ReqdPhysicalProperty* rppForMe = context->getReqdPhysicalProperty();
  // Start off assuming that the sort will use all available CPUs.
  Lng32 cpuCount = rppForMe->getCountOfAvailableCPUs();
  PartitioningRequirement* partReq = rppForMe->getPartitioningRequirement();
  const PhysicalProperty* spp = context->getPlan()->getPhysicalProperty();
  Lng32 numOfStreams;  

  // If the physical properties are available, then this means we 
  // are on the way back up the tree. Get the actual level of 
  // parallelism from the spp to determine if the number of cpus we 
  // are using are less than the maximum number available.
  if (spp != NULL)
  {
    PartitioningFunction* partFunc = spp->getPartitioningFunction();
    numOfStreams = partFunc->getCountOfPartitions();
    if (numOfStreams < cpuCount)
      cpuCount = numOfStreams;
  }
  else
  if ((partReq != NULL) AND
      (partReq->getCountOfPartitions() != ANY_NUMBER_OF_PARTITIONS))
  {
    // If there is a partitioning requirement, then this may limit
    // the number of CPUs that can be used.
    numOfStreams = partReq->getCountOfPartitions();
    if (numOfStreams < cpuCount)
      cpuCount = numOfStreams;
  }

  EstLogPropSharedPtr inLogProp = context->getInputLogProp();

  const double probeCount =
    MIN_ONE(inLogProp->getResultCardinality().value());

  CMPASSERT ( numHistoryRows() > 0 &&
	      getEstHistoryRowLength() > 0);

  const double historyBufferRowCount = MIN_ONE(numHistoryRows());

  const double rowsPerCpu = MIN_ONE(historyBufferRowCount / cpuCount);
  const double rowsPerCpuPerProbe = MIN_ONE(rowsPerCpu / probeCount);

  const Lng32 bufferWidth = estHistoryRowLength_;
  const double bufferSizePerCpu = (rowsPerCpuPerProbe / 1024. * bufferWidth);

  return (bufferSizePerCpu >= memoryLimitPerCPU);

} // PhysSequence::isBigMemoryOperator


// addUnResolvedSeqFunctions - Add unresolved sequence functions to
// the RelSequence operator found when binding the select list.  Also,
// if OLAP, bind the partition by and order by clauses and add to
// RelSequence.
//
void RelSequence::addUnResolvedSeqFunctions(ValueIdSet &unresolvedSeqFuncs,
                                            BindWA *bindWA)
{ 
  // Add the unresolved sequence functions to this RelSequence
  //
  sequenceFunctions_ += unresolvedSeqFuncs; 

  if(sequenceFunctions_.entries()) {

    if (CmpCommon::getDefault(OLAP_CAN_INVERSE_ORDER) == DF_ON)
    {
      NABoolean mustInv = FALSE;
      NABoolean canInv = TRUE;

      Lng32 precedingMax = 0;
      Lng32 followingMax = 0;

      for(ValueId sfId = sequenceFunctions_.init();
          sequenceFunctions_.next(sfId); sequenceFunctions_.advance(sfId)) 
        {
          ItmSequenceFunction * sf = (ItmSequenceFunction *)(sfId.getItemExpr());
          if(!sf->isOLAP()) 
          {
            canInv = FALSE;
            continue;
          }
          ItmSeqOlapFunction *olapF = (ItmSeqOlapFunction *)sf;
          precedingMax = MAXOF(precedingMax, -(olapF->getframeStart()));
          followingMax = MAXOF(followingMax, olapF->getframeEnd());
          
          if(!olapF->canInverseOLAPOrder()) {
            canInv = FALSE;
            continue;
          }
          if(olapF->mustInverseOLAPOrder()) {
            mustInv = TRUE;
          }
        }

      NABoolean shouldInv = (canInv && (followingMax > precedingMax));

      if((mustInv && canInv) || shouldInv) {
        for(ValueId sfId = sequenceFunctions_.init();
            sequenceFunctions_.next(sfId); sequenceFunctions_.advance(sfId)) 
          {
            ItmSeqOlapFunction * olapF = (ItmSeqOlapFunction *)(sfId.getItemExpr());
            olapF->inverseOLAPOrder(bindWA->wHeap());
          }
      } else if(mustInv && !canInv) {
        *CmpCommon::diags() << DgSqlCode(-4349);
        bindWA->setErrStatus();
        return;
      }
    }


    // Get the first sequence function
    //
    ValueId sFunc = sequenceFunctions_.init();
    sequenceFunctions_.next(sFunc);

    ItemExpr *ie = sFunc.getItemExpr();
    CMPASSERT(ie->isASequenceFunction());
    ItmSequenceFunction *seqFunc = (ItmSequenceFunction *)ie;

    // If it is OLAP, the bind the partition by and order by clauses.
    // and attach them to this RelSequence operator.
    //
    if( getHasOlapFunctions() || getHasTDFunctions() ) {

      // only for OLAP.
      // for TD, the partition by is already bound
      if( getHasOlapFunctions() )
      {
        CMPASSERT(partition().entries() == 0);
      }
      CMPASSERT(requiredOrder().entries() == 0);

      ItemExpr * prevSF = NULL;
      for(ValueId sfId = sequenceFunctions_.init();
        sequenceFunctions_.next(sfId); sequenceFunctions_.advance(sfId)) 
      {
        ItemExpr * ie = sfId.getItemExpr();
        CMPASSERT(ie->isASequenceFunction());
        ItmSequenceFunction *sf = (ItmSequenceFunction *)ie;               
        
        if ( 
            (getHasTDFunctions() && !sf->isTDFunction())
             ||
             (getHasOlapFunctions() && !sf->isOLAP())
            )
        {   //Using rank function and sequence functions together 
            //  in the same query scope is not supported.
            *CmpCommon::diags() << DgSqlCode(-4367);
            bindWA->setErrStatus();
            return;
        }
        if ((!getHasOlapFunctions()) && prevSF)
          if (!sf->hasBaseEquivalence(prevSF))
          { //All rank functions need to have the same expression as argument
            *CmpCommon::diags() << DgSqlCode(-4361);
            bindWA->setErrStatus();
            return;
          }
        prevSF = sf;
      }

      // Bind the order by and partition by using the child's
      // RETDesc.
      //
      BindScope *currScope = bindWA->getCurrentScope();

      RETDesc *myRETDesc = getRETDesc();
      RETDesc *currentRETDesc = currScope->getRETDesc();

      setRETDesc(child(0)->getRETDesc());
      currScope->setRETDesc(child(0)->getRETDesc());

      if (getHasOlapFunctions()) {

        ItemExpr *partitionBy = seqFunc->getOlapPartitionBy();

        if(partitionBy) {
          bindWA->getCurrentScope()->context()->inOlapPartitionBy() = TRUE;
          partitionBy->convertToValueIdList(partition(),
                                            bindWA,
                                            ITM_ITEM_LIST);
          bindWA->getCurrentScope()->context()->inOlapPartitionBy() = FALSE;
          if(bindWA->errStatus())
            return;
        }
      }

      ItemExpr *orderBy = seqFunc->getOlapOrderBy();
      if(orderBy) {        
        
        if( seqFunc->isTDFunction() ) { 
          // TBD: Try to do requiredOrder()=partition() somewhere else to
          // make it consistent with OLAP
          requiredOrder()=partition();
          orderBy = orderBy->changeDefaultOrderToDesc();
        }           
        // if for OLAP and there's a double inverse, remove them.
        else if( seqFunc->isOLAP()  ) {
          NABoolean inv = FALSE;
          orderBy = orderBy->removeInverseFromExprTree(inv,TRUE);          
        }


        bindWA->getCurrentScope()->context()->inOrderBy() = TRUE;
        bindWA->getCurrentScope()->context()->inOlapOrderBy() = TRUE;
        orderBy->convertToValueIdList(requiredOrder(),
                                      bindWA,
                                      ITM_ITEM_LIST);
        bindWA->getCurrentScope()->context()->inOrderBy() = FALSE;
        bindWA->getCurrentScope()->context()->inOlapOrderBy() = FALSE;
        if(bindWA->errStatus())
          return;        
        
        // NULLs are in a different order for TD
        if( seqFunc->isTDFunction() ) {

          ValueIdList newOrder;
          ValueId valId;
          for (CollIndex i=0; i<requiredOrder().entries(); i++)
          {
            valId = requiredOrder()[i];
            NAType * type = valId.getType().newCopy();

            if (type->supportsSQLnull())
            {              
              ItemExpr * ie =NULL; 

              if (valId.getItemExpr()->getOperatorType() == ITM_INVERSE)
              {
                 ie = valId.getItemExpr()->removeInverseOrder();
              }
              else
              {
                ie = valId.getItemExpr();
              }
              ItemExpr * constZero = new (bindWA->wHeap()) ConstValue(0);
              ItemExpr * constOne = new (bindWA->wHeap()) ConstValue(1);
              ItemExpr *isnull = new HEAP UnLogic (ITM_IS_NULL, ie);
              ItemExpr *ifthenelse = new (bindWA->wHeap()) IfThenElse(isnull, constZero, constOne);              
              ItemExpr * newExpr =  new (bindWA->wHeap()) Case(NULL, ifthenelse);
              newExpr = new (bindWA->wHeap()) 
                          Cast(newExpr, 
                               ((ConstValue *) constZero)->getType()->newCopy());

              if (valId.getItemExpr()->getOperatorType() == ITM_INVERSE)
              {
                 newExpr = new  (bindWA->wHeap()) InverseOrder( newExpr);
              }
              newExpr->synthTypeAndValueId(TRUE);
              newOrder.insert(newExpr->getValueId());
              newOrder.insert(valId);
            }
            else
            {
              newOrder.insert(valId);
            }            
          }  
          requiredOrder()= newOrder;          
        }
      }

      // Return the RETDescs to their proper values.
      //
      setRETDesc(myRETDesc);
      currScope->setRETDesc(currentRETDesc);
    }    
  }
}

void PhysSequence::setNumHistoryRows(Lng32 numHistoryRows)
{
  numHistoryRows_ = numHistoryRows;
};

void PhysSequence::setUnboundedFollowing(NABoolean v)
{
  unboundedFollowing_ = v;
};

NABoolean PhysSequence::getUnboundedFollowing() const
{
  return unboundedFollowing_;
};
void PhysSequence::setMinFollowingRows(Lng32 v)
{
  minFollowingRows_ = v;
};

void PhysSequence::computeAndSetMinFollowingRows(Lng32 v)
{
  if ( v > minFollowingRows_ )
     minFollowingRows_ = v;
};

Lng32 PhysSequence::getMinFollowingRows() const
{
  return minFollowingRows_;
};


void PhysSequence::estimateHistoryRowLength(const ValueIdSet &sequenceFunctions,
					    const ValueIdSet &outputFromChild,
					    ValueIdSet &histIds,
					    Lng32 &estimatedLength)
{
  ValueIdSet children;

  for(ValueId valId = sequenceFunctions.init();
      sequenceFunctions.next(valId);
      sequenceFunctions.advance(valId)) 
  {
    if(valId.getItemExpr()->isASequenceFunction()) 
    {
      ItemExpr *itmExpr = valId.getItemExpr();

      switch(itmExpr->getOperatorType())
      {
        case ITM_OLAP_LEAD:
        case ITM_OLAP_LAG:
        case ITM_OFFSET:
        case ITM_ROWS_SINCE:
        case ITM_THIS:
        case ITM_NOT_THIS:
          updateEstHistoryRowLength(histIds,
                                    itmExpr->child(0)->getValueId(),
                                    estimatedLength);
	  //estimatedLength +=  itmExpr->child(0)->getValueId().getType().getTotalSize();
          break;
        case ITM_RUNNING_SUM:
        case ITM_RUNNING_COUNT:
        case ITM_RUNNING_MIN:
        case ITM_RUNNING_MAX:
        case ITM_LAST_NOT_NULL:
          updateEstHistoryRowLength(histIds,
                                    itmExpr->getValueId(),
                                    estimatedLength);
          break;
        case ITM_OLAP_SUM:
        case ITM_OLAP_COUNT:
        case ITM_OLAP_RANK:
        case ITM_OLAP_DRANK:
	  {
	    ItemExpr * tmpExpr = 
	      ((ItmSeqOlapFunction *)itmExpr)->transformOlapFunction(
						  CmpCommon::statementHeap());
	    tmpExpr->synthTypeAndValueId(TRUE);
	    CMPASSERT (valId != tmpExpr->getValueId());
	    children += tmpExpr->getValueId();
	  }
          break;
        case ITM_OLAP_MIN:
        case ITM_OLAP_MAX:
	  {
	    if (((ItmSeqOlapFunction *)itmExpr)->getframeStart() < 0 && 
	        ((ItmSeqOlapFunction *)itmExpr)->getframeEnd() >0)
	    {
	      ItemExpr * tmpExpr = 
		((ItmSeqOlapFunction *)itmExpr)->transformOlapFunction(
						    CmpCommon::statementHeap());
	      tmpExpr->synthTypeAndValueId(TRUE);
	      CMPASSERT (valId != tmpExpr->getValueId());
	      children += tmpExpr->getValueId();
	    }
	    else
	    {
              updateEstHistoryRowLength(histIds,
                                    itmExpr->getValueId(),
                                    estimatedLength);

              updateEstHistoryRowLength(histIds,
                                    itmExpr->child(0)->getValueId(),
                                    estimatedLength);
	      //estimatedLength += itmExpr->getValueId().getType().getTotalSize(); //
	      //estimatedLength += itmExpr->child(0)->getValueId().getType().getTotalSize(); //
	    }
	  }
	  break;
        case ITM_MOVING_MIN:
        case ITM_MOVING_MAX:
	  {
            updateEstHistoryRowLength(histIds,
                                    itmExpr->getValueId(),
                                    estimatedLength);

            updateEstHistoryRowLength(histIds,
                                    itmExpr->child(0)->getValueId(),
                                    estimatedLength);
	  //estimatedLength += itmExpr->getValueId().getType().getTotalSize(); //
	  //estimatedLength += itmExpr->child(0)->getValueId().getType().getTotalSize(); //
	  }
          break;
        case ITM_RUNNING_CHANGE:
          if (itmExpr->child(0)->getOperatorType() == ITM_ITEM_LIST)
          {
            ExprValueId treePtr = itmExpr->child(0);
            ItemExprTreeAsList changeValues(&treePtr,
                                            ITM_ITEM_LIST,
                                            RIGHT_LINEAR_TREE);
            CollIndex nc = changeValues.entries();
            for (CollIndex i = nc; i > 0; i--)
            {
              updateEstHistoryRowLength(histIds,
                                     changeValues[i-1]->getValueId(),
                                     estimatedLength);
            }
          }
          else
          {
            updateEstHistoryRowLength(histIds,
                                    itmExpr->child(0)->getValueId(),
                                    estimatedLength);
	      // estimatedLength +=  itmExpr->child(0)->getValueId().getType().getTotalSize();
          }
          updateEstHistoryRowLength(histIds,
                                    itmExpr->getValueId(),
                                    estimatedLength);
          //estimatedLength +=  itmExpr->getValueId().getType().getTotalSize();
          break;

        default:
          CMPASSERT(0);
      }
    }
    // Gather all the children, and if not empty, recurse down to the
    // next level of the tree.
    //
    for(Lng32 i = 0; i < valId.getItemExpr()->getArity(); i++) 
    {
      if (!outputFromChild.contains(valId.getItemExpr()->child(i)->getValueId()))
      {
        children += valId.getItemExpr()->child(i)->getValueId();
      }
    }
  }
  
  if (!children.isEmpty())
  {
    estimateHistoryRowLength(children,
			     outputFromChild,
                             histIds,
			     estimatedLength);
  }

}

void PhysSequence::retrieveCachedHistoryInfo(RelSequence * cacheRelSeq) 
{
  ValueIdSet outputFromChild = child(0).getGroupAttr()->getCharacteristicOutputs();
  

  if (!cacheRelSeq->getHistoryInfoCached())
  {
    Lng32 unableToCalculate = 0;
    Lng32 estimatedRowLength = 0;
    Lng32 numHistoryRows = 0;
    NABoolean unboundedFollowing = 0;
    Lng32 minFollowing = 0;
    ValueIdSet histIds;
    
    estimateHistoryRowLength( sequenceFunctions(),
			      outputFromChild,
                              histIds,
			      estimatedRowLength);
    if (sequencedColumns().entries()>0)
    {
      estimatedRowLength += sequencedColumns().getRowLength();
    }

    if (partition().entries()>0)
    {
      estimatedRowLength += partition().getRowLength();
    }

    estimatedRowLength += sequenceFunctions().getRowLength();

    cacheRelSeq->setCachedHistoryRowLength(estimatedRowLength);

    computeHistoryRows(sequenceFunctions(), 
		       numHistoryRows, 
		       unableToCalculate, 
		       unboundedFollowing, 
		       minFollowing,
		       outputFromChild);

    if(unableToCalculate || numHistoryRows ==0)
    {
      numHistoryRows = getDefault(DEF_MAX_HISTORY_ROWS);
    }

    cacheRelSeq->setCachedNumHistoryRows( numHistoryRows);
    cacheRelSeq->setCachedUnboundedFollowing(unboundedFollowing);
    cacheRelSeq->setCachedMinFollowingRows(minFollowing);
    cacheRelSeq->setHistoryInfoCached(TRUE);
  }
   CMPASSERT(cacheRelSeq->getCachedHistoryRowLength() != 0 &&
             cacheRelSeq->getCachedNumHistoryRows() != 0 &&
             cacheRelSeq->getHistoryInfoCached());

  estHistoryRowLength_ = cacheRelSeq->getCachedHistoryRowLength();
  numHistoryRows_ = cacheRelSeq->getCachedNumHistoryRows();
  unboundedFollowing_ = cacheRelSeq->getCachedUnboundedFollowing();
  minFollowingRows_ = cacheRelSeq->getCachedMinFollowingRows();

}







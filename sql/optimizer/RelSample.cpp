/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
/* -*-C++-*-
******************************************************************************
*
* File:         RelSample.cpp
* Description:  All the methods of RelSample() and PhysSample().
*
* Created:      9/24/98
* Language:     C++
*
*
******************************************************************************
*/
#include "AllItemExpr.h"
#include "ItemSample.h"
#include "AllRelExpr.h"
#include "RelSample.h"
#include "SchemaDB.h"
#include "GroupAttr.h"
#include "BindWA.h"
#include "NormWA.h"
#include "Cost.h"
#include "CostMethod.h"
#include "opt.h"
#include "Globals.h"

// -----------------------------------------------------------------------
// This file contains all the methods for the class RelSample.
// This is a departure from the way other RelExpr's are organized.
//


///////////////////////////////////////////////////////////////////////////
//      
//            RelSample Class
//
//
///////////////////////////////////////////////////////////////////////////

RelSample::RelSample(RelExpr *child, 
                     SampleTypeEnum sampleType, 
                     ItemExpr *balanceExpr,
                     ItemExpr *requiredOrder,
                     CollHeap *oHeap)
  : RelExpr(REL_SAMPLE, child, NULL, oHeap), 
    balanceExprTree_(balanceExpr),
    sampleType_(sampleType),
    sampleScanSucceeded_(FALSE),
    requiredOrderTree_(requiredOrder)
{
  setNonCacheable();
  if (balanceExprTree_ != NULL) 
  {
    ((ItmBalance *)balanceExprTree_)->propagateSampleType(sampleType);
    ((ItmBalance *)balanceExprTree_)->rearrangeChildren();
  }
}

RelSample::~RelSample()
{
}

ItemExpr *
RelSample::removeRequiredOrderTree()
{
  ItemExpr *requiredOrderTree = requiredOrderTree_;

  requiredOrderTree_ = NULL;

  return requiredOrderTree;
}


Float32 RelSample::getSamplePercent() const
{
  // Call ONLY if: RANDOM or CLUSTER sampling and relative size
  // and NOT stratified sampling
  if (sampleType() != RANDOM &&
      sampleType() != CLUSTER)
    return -1.0;

  ItmBalance * balExp = NULL;

  if (balanceExprTree_ != NULL)
    balExp = (ItmBalance *)balanceExprTree_;
  else
  {
    if (NOT balanceExpr().isEmpty())
    {
      ValueId exprId;
      balanceExpr().getFirst(exprId);
      balExp = (ItmBalance *)(exprId.getItemExpr());
    }
  }

  if (balExp != NULL) 
  {
    if ((balExp->isAbsolute() == TRUE) OR 
        (balExp->getNextBalance() != NULL))
      return -1.0;
    
    double size = balExp->getSampleConstValue();
    size = size / 100; // Size specified as percent
    return (float)size;
  }
  return -1.0;
}

Lng32 RelSample::getClusterSize() const
{
  // Call ONLY if: CLUSTER sampling
  // and NOT stratified sampling
  if (sampleType() != CLUSTER)
    return -1;

  ItmBalance * balExp = NULL;

  if (balanceExprTree_ != NULL)
    balExp = (ItmBalance *)balanceExprTree_;
  else
  {
    if (NOT balanceExpr().isEmpty())
    {
      ValueId exprId;
      balanceExpr().getFirst(exprId);
      balExp = (ItmBalance *)(exprId.getItemExpr());
    }
  }

  if (balExp != NULL) 
  {
    if (balExp->getNextBalance() != NULL)
      return -1;

    double size = balExp->getClusterConstValue();
    return (Lng32)size;
  }
  return -1;
}

NABoolean RelSample::isSimpleRandomRelative() const
{
  // True only if: RANDOM sampling and relative size
  // and NOT stratified sampling and NOT oversampling
  if (getSamplePercent() == -1.0)
    return FALSE;
  else
    return TRUE;
}

// RelSample::topHash() --------------------------------------------------
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
HashValue RelSample::topHash()
{
  // Compute a hash value of the derivation chain below this node.
  //
  HashValue result = RelExpr::topHash();
  
  result ^= sampleType();
  result ^= balanceExpr();
  result ^= sampledColumns();

  result ^= requiredOrder();

  return result;
}

// RelSample::duplicateMatch()
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
NABoolean
RelSample::duplicateMatch(const RelExpr & other) const
{
  // Compare this node with 'other' down the derivation chain.
  //
  if (!RelExpr::duplicateMatch(other))
    return FALSE;
  
  // Cast the RelExpr to a RelSample node.
  //
  RelSample &o = (RelSample &) other;
  
  // If the sampling type and sample size expressions are same then the 
  // nodes are identical
  //
  if (!(sampleType() == o.sampleType()))
    return FALSE;
  
  if(!(balanceExpr() == o.balanceExpr()))
    return FALSE;
  
  // If the required order keys are not the same 
  // then the nodes are not identical
  //
  if (!(requiredOrder() == o.requiredOrder()))
    return FALSE;

  return TRUE;
}

// RelSample::copyTopNode ----------------------------------------------
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
// is NULL, then this is the top of the derivation chain and an UnPackRows
// node must be constructed.
// 
// In either case, the relevant data members must be copied to 'derivedNode'
// and 'derivedNode' is passed to the copyTopNode method of the class
// below this one in the derivation chain (RelExpr::copyTopNode() in this
// case).
// 
RelExpr *
RelSample::copyTopNode(RelExpr *derivedNode, CollHeap *outHeap)
{
  RelSample *result;
  
  if (derivedNode == NULL)
    // This is the top of the derivation chain
    result = new (outHeap) RelSample(child(0), 
                                     sampleType(), 
                                     balanceExprTree()
                                     );
  else
    // A node has already been constructed as a derived class.
    result = (RelSample *) derivedNode;
  
  // Copy the relavant fields.
  
  result->sampleType_ = sampleType();

  result->sampleScanSucceeded_ = sampleScanSucceeded();
  
  if (balanceExprTree() != NULL)
    result->balanceExprTree() = balanceExprTree()->copyTree(outHeap)->castToItemExpr();
  
  result->balanceExpr() = balanceExpr();
  
  result->sampledColumns() = sampledColumns();

  result->requiredOrder() = requiredOrder();
  
  // Copy any data members from the classes lower in the derivation chain.
  //
  return RelExpr::copyTopNode(result, outHeap);
}


// RelSample::addLocalExpr() -----------------------------------------------
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
// RelSample::addLocalExpr potentially adds the balance expression
// ("balance_expression").
//
// It then calls RelExpr::addLocalExpr() which will add any RelExpr
// expressions to the list.
//
void RelSample::addLocalExpr(LIST(ExprNode *) &xlist,
                             LIST(NAString) &llist) const
{
  if (sampledColumns().entries() > 0)
  {
    xlist.insert(sampledColumns().rebuildExprTree(ITM_ITEM_LIST));
    llist.insert("sampled_columns");
  }
  
  if (balanceExprTree() || balanceExpr().entries() > 0)
  {
    if (balanceExprTree_) 
      xlist.insert(balanceExprTree_);
    else 
      xlist.insert(balanceExpr().rebuildExprTree(ITM_ITEM_LIST));
    llist.insert("balance_expression");
  }
  
  if (requiredOrderTree_) {
    xlist.insert(requiredOrderTree_);
    llist.insert("required_order");
  } else if (requiredOrder().entries() > 0) {
    xlist.insert(requiredOrder().rebuildExprTree(ITM_ITEM_LIST));
    llist.insert("required_order");
  }

  RelExpr::addLocalExpr(xlist,llist);
}

// RelSample::getPotentialOutputValues() ---------------------------------
// Construct a Set of the potential outputs of this node.
//
// Inputs: none (other than 'this')
//
// Outputs: outputValues - a ValueIdSet representing the potential outputs
//          of this node.
//
// The potential outputs for the RelSample node are the new columns
// generated by the RelSample node. The new columns generated by RelSample
// node are "Sampled" versions of all the potential outputs of its child. 
//
void 
RelSample::getPotentialOutputValues(ValueIdSet & outputValues) const
{
  outputValues += sampledColumns();
} 

// RelSample::pushdownCoveredExpr() ------------------------------------
//
// In order to compute the Group Attributes for a relational operator
// an analysis of all the scalar expressions associated with it is
// performed. The purpose of this analysis is to identify the sources
// of the values that each expression requires. As a result of this
// analysis values are categorized as external dataflow inputs or
// those that can be produced completely by a certain child of the
// relational operator.
//
// This method is invoked on each relational operator. It causes
// a) the pushdown of predicates and
// b) the recomputation of the Group Attributes of each child.
//    The recomputation is required either because the child is
//    assigned new predicates or is expected to compute some of the
//    expressions that are required by its parent.
//
// For the sample operator, only the balance expression contains references 
// to any outputs produced by the child. These expressions refer to the unsampled
// columns and therefore can be pushed down. However, the predicatesOnParent if
// they exist refer to the sampled columns and therefore they will not be pushed 
// down. 
// ---------------------------------------------------------------------

void 
RelSample::pushdownCoveredExpr(const ValueIdSet &outputExpr,
                               const ValueIdSet &newExternalInputs,
                               ValueIdSet &predicatesOnParent,
			       const ValueIdSet *setOfValuesReqdByParent,
                               Lng32 childIndex
                              )
{
  ValueIdSet exprOnParent;
  if (setOfValuesReqdByParent)
    exprOnParent = *setOfValuesReqdByParent;
  exprOnParent += outputExpr;
  ValueId refVal;
  ValueIdSet outputSet;
  
  // Prune from the sampledColumns() ValueIdSet, those expressions
  // that are not needed above (in setOfValuesReqdByParent) or by
  // the selectionPred.
  //
  for(ValueId sampleCol = sampledColumns().init(); sampledColumns().next(sampleCol);
  sampledColumns().advance(sampleCol)) {
    if(!exprOnParent.referencesTheGivenValue(sampleCol, refVal) &&
      !selectionPred().referencesTheGivenValue(sampleCol, refVal)) {
      sampledColumns() -= sampleCol;
    }
  }
  
  // Remove all expressions from exprOnParent.  They
  // can't be pushed down!
  //
  exprOnParent.clear();
  
  // Add all the values required for the Sample expressions
  // to the values required by the parent.  These expression
  // can't be pushed down either, but attempting to push them
  // down causes the child node to provide the values needed.
  //
  outputSet += sampledColumns();

  exprOnParent += balanceExpr();
  
  exprOnParent.insertList(requiredOrder());

  RelExpr::pushdownCoveredExpr(outputSet,
                               newExternalInputs,
                               predicatesOnParent,
			       &exprOnParent,
                               childIndex);
  
} // RelSample::pushdownCoveredExpr

Context* RelSample::createContextForAChild(Context* myContext,
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
  // Add the order requirements needed for this RelSample node
  // ---------------------------------------------------------------------

  // Remove any sort order requirement from parent.
  //
  rg.removeSortKey();
  rg.removeArrangement();
  rg.removeSortOrderTypeReq();

  // Shouldn't/Can't add a sort order type requirement
  // if we are in DP2
  if (rppForMe->executeInDP2())
    rg.addSortKey(requiredOrder(),NO_SOT);
  else
    rg.addSortKey(requiredOrder(),ESP_SOT);

  // Can not execute absolute sampling in parallel
  //
  if(!isSimpleRandomRelative())
    rg.addNumOfPartitions(1);

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
  Context* result = shareContext(childIndex,
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
  
} // RelSample::createContextForAChild()

// RelSample::removeBalanceExprTree() -------------------------------------
// Return the sizeExprTree_ ItemExpr tree and set to NULL,
//
// Inputs: none (Other than 'this')
//
// Outputs: ItemExpr * - the value of sizeExprTree_
//
// Side Effects: Sets the value of sizeExprTree_ to NULL.
//
// Called by RelSample::bindNode(). The value of sizeExprTree_ is not
// needed after the binder.
//
ItemExpr *
RelSample::removeBalanceExprTree()
{
  ItemExpr *result = balanceExprTree();
  balanceExprTree_ = (ItemExpr *)NULL;
  return result;
}

// RelSample::transformNode() -------------------------------------------
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
void RelSample::transformNode(NormWA &normWARef,
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
  
  //Sample node does not pull up the predicates and so the equality 
  //predicates on below this node are not true above this node,
  //so create a new VEGRegion when transfroming the child 
  normWARef.allocateAndSetVEGRegion(IMPORT_AND_EXPORT,this);

  // transformNode takes up a bound tree and turns into a transformed
  // tree. For a RelExpr that means the following.
  //    + expressions are transformed. If the expressions contain
  //        subqueries then new RelExpr are created for them and
  //        they are usually added above (as an ancestor) of the node
  //        that contained them.
  //    + predicates are pulled up from the children and their
  //        required inputs are modified
  //    + the required inputs of the node itself are changed from
  //        being a sufficient set to being a sufficient minimal set.
  //       
  // Transform the child.
  // Pull up their transformed predicates
  // recompute their required inputs.
  //
  child(0)->transformNode(normWARef, child(0)); 
  
  if(balanceExpr().transformNode(normWARef,
    child(0),
    getGroupAttr()->getCharacteristicInputs())) 
  {
    // -----------------------------------------------------------------
    // Transform my new child.
    // -----------------------------------------------------------------
    child(0)->transformNode(normWARef, child(0));
  }
  
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

  normWARef.restoreOriginalVEGRegion();

  // Pull up the predicates and recompute the required inputs
  // of whoever my children are now.
  //
  pullUpPreds();
  

  // transform the selection predicates
  //
  transformSelectPred(normWARef, locationOfPointerToMe);
  
} // RelSample::transformNode()

// RelSample::rewriteNode() ---------------------------------------------
// rewriteNode() is the virtual function that computes
// the transitive closure for "=" predicates and rewrites value
// expressions.
//
// Parameters:
//
// NormWA & normWARef
//    IN : a pointer to the normalizer work area
//
void RelSample::rewriteNode(NormWA & normWARef)
{

  // locate the VEGRegion that was created for this node during transformation
  normWARef.locateAndSetVEGRegion(this);

  child(0)->rewriteNode(normWARef); 

  balanceExpr().normalizeNode(normWARef);

  requiredOrder().normalizeNode(normWARef);
  
  normWARef.restoreOriginalVEGRegion();
  
  selectionPred().normalizeNode(normWARef);

  // rewrite expression in the group attributes
  getGroupAttr()->normalizeInputsAndOutputs(normWARef);

} // RelSample::rewriteNode()


RelExpr * RelSample::normalizeNode(NormWA & normWARef)
{
  if (nodeIsNormalized())
    return this;
  markAsNormalized();

  pushdownCoveredExpr(getGroupAttr()->getCharacteristicOutputs(),
                      getGroupAttr()->getCharacteristicInputs(),
                      selectionPred());

  // locate the VEGRegion that was created for this node during transformation
  normWARef.locateAndSetVEGRegion(this);

  child(0) = child(0)->normalizeNode(normWARef);

  normWARef.restoreOriginalVEGRegion();

  fixEssentialCharacteristicOutputs();

  return this;
}


// RelSample::pullUpPreds() --------------------------------------------
// is redefined to disallow the pullup of predicates
// from the operator's child. The outputs of the sample operator
// are "sampled" versions of the outputs of its child. 
//
void RelSample::pullUpPreds()
{
  // ---------------------------------------------------------------------
  // Simply don't pull up child's selection predicates. Still need to tell
  // child to recompute its outer references due to the warning below.
  // ---------------------------------------------------------------------
  child(0)->recomputeOuterReferences();
  
  // ---------------------------------------------------------------------
  // WARNING: One rule that this procedure must follow is
  // that recomputeOuterReferences() must be called on the children even
  // if no predicates are pulled up from them. This is to correct
  // the outer references that are added to a right child of a
  // semi or outer join when processing subqueries in the ON clause.
  // ---------------------------------------------------------------------
}

// RelSample::recomputeOuterReferences() --------------------------------
// This method is used by the normalizer for recomputing the
// outer references (external dataflow input values) that are
// still referenced by each operator in the subquery tree
// after the predicate pull up is complete.
//
// Side Effects: sets the characteristicInputs of the groupAttr.
//
void RelSample::recomputeOuterReferences()
{
  // This is virtual method on RelExpr.
  // When this is called it is assumed that the children have already
  // been transformed.
  // The required inputs of the child are therefore already minimal
  // and sufficient.
  // It is also assumed that the RelExpr itself has been bound.
  // That implies that the group attributes have already been allocated
  // and the required inputs is a sufficient (but not necessarilly minimum)
  // set of external values needed to evaluate all expressions in this subtree.
  // 
  // Delete all those input values that are no longer referenced on
  // this operator because the predicates that reference them have 
  // been pulled up.
  //
  ValueIdSet outerRefs = getGroupAttr()->getCharacteristicInputs(); 
  
  // The set of valueIds need by this node.
  //
  ValueIdSet allMyExpr(getSelectionPred());
  
  allMyExpr += balanceExpr();

  allMyExpr.insertList(requiredOrder());
  
  
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
} // RelSample::recomputeOuterReferences()  


CostScalar RelSample::computeResultSize(const CostScalar &childCardinality)
{

  // CostScalar resultSize;
  
  // Compute the result size based on the size expression. 
  // For now, set it to the child cardinality
  
  CMPASSERT(balanceExpr().entries() == 1);

  ValueId balanceRoot;

  balanceExpr().getFirst(balanceRoot);

  ItmBalance *balanceExpr = (ItmBalance *)balanceRoot.getItemExpr();

  CostScalar resultSize = balanceExpr->computeResultSize(childCardinality);
  
  return resultSize;
}


// RelSample::synthEstLogProp() ------------------------------------------
// synthesize estimated logical properties given a specific set of
// input log. properties.
//
// Parameters:
//
// EstLogPropSharedPtr inputEstLogProp
//    IN : A set of input logical properties used to estimate the logical
//         properities of this node.
//
void RelSample::synthEstLogProp(const EstLogPropSharedPtr& inputEstLogProp)
{
  if (getGroupAttr()->isPropSynthesized(inputEstLogProp) == TRUE) 
    return;
  
  // Get the estimated logical properties of the child. To be used
  // to estimate the logical properties of this node.
  //
  EstLogPropSharedPtr childEstProp  = child(0).outputLogProp(inputEstLogProp);
  const ColStatDescList &childColStats = childEstProp->getColStats();

  CostScalar rowCount = 
    computeResultSize(childEstProp->getResultCardinality());

  for(CollIndex i = 0; i < childColStats.entries(); i++) {
    ColStatDescSharedPtr columnStatDesc = childColStats[i];
    CostScalar oldCount = columnStatDesc->getColStats()->getRowcount();

    if (oldCount != rowCount)
      columnStatDesc->synchronizeStats(oldCount, rowCount);
  }

  EstLogPropSharedPtr myEstProps =
    synthEstLogPropForUnaryLeafOp(inputEstLogProp,
                                  childColStats,
                                  rowCount);

  // Set the logical properties of this node.
  //
  getGroupAttr()->addInputOutputLogProp(inputEstLogProp, myEstProps);
  
} // RelSample::synthEstLogProp


// RelSample::synthLogProp ----------------------------------------------
// synthesize logical properties
//
void
RelSample::synthLogProp(NormWA * normWAPtr)
{
  // check to see whether properties are already synthesized.
  if (getGroupAttr()->existsLogExprForSynthesis()) 
    return;
  
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

} // RelSample::synthLogProp()



// RelSample::bindNode - Bind the RelSample node.
// This node is generated by the parser when it encounters a SAMPLE
// clause. This node has two item expressions:
// 
// sizeExprTree(): This expression contains either a simple size 
// expression containing a size, type and normal/oversampling field 
// or a tree of IfThenElse nodes each with a predicate on a column 
// and a simple size expression followed by an optional else part. 
//
// skipPeriodTree(): This is a simple size expression with no reference 
// to any inputs or outputs of this node or its child. The reason to make
// this into an expression is only to reuse the SampleSize ItemExpr. 
// However, this "expression" is really not an expression and hence need not
// treated as one (e.g., there is no need to bind, normalize, etc. Similarly
// no need to check for outer references, etc). 
//
RelExpr *RelSample::bindNode(BindWA *bindWA)
{
  
  // If this node has already been bound, we are done.
  //
  if (nodeIsBound())
    return this;
  
  // Bind the child nodes.
  //
  bindChildren(bindWA);
  if (bindWA->errStatus())
    return this;
  
  // If this is a random sample on an HBase table, push the sampling down into
  // the Scan node and remove the Sample node from the tree. For HBase, we
  // unconditionally perform sampling via a row filter on the HBase side.
  //
  RelExpr* myChild = child(0);
  if (myChild->getOperatorType() == REL_SCAN &&
      (static_cast<Scan*>(myChild))->isHbaseTable() &&
      myChild->selectionPred().entries() == 0 &&
      isSimpleRandomRelative())
    {
      (static_cast<Scan*>(myChild))->samplePercent(getSamplePercent());
      return myChild;
    }

  ItemExpr *requiredOrderTree = removeRequiredOrderTree();
  
  if(requiredOrderTree) {
    bindWA->getCurrentScope()->context()->inOrderBy() = TRUE;
    requiredOrderTree->convertToValueIdList(requiredOrder(),
                                            bindWA,
                                            ITM_ITEM_LIST);
    bindWA->getCurrentScope()->context()->inOrderBy() = FALSE;
    if(bindWA->errStatus())
      return this;
  }

  // Bind the balanceExprTree. This expression may contain a tree 
  // of Balance expressions.
  //
  ItemExpr *boundBalanceExpr = removeBalanceExprTree()->bindNode(bindWA);

  if (bindWA->errStatus())
    return this;

  if (boundBalanceExpr != NULL) {
    balanceExpr().insert(boundBalanceExpr->getValueId());

    if(((ItmBalance *)boundBalanceExpr)->checkErrors()) {
      bindWA->setErrStatus();
      return this;
    }
  }
  
  // Generate the selection predicate from the balance expression only
  // if there is a balance expression tree and there is no else clause. 
  //
  NABoolean hasReturnTrue = FALSE;
  ItmBalance * nextBalanceNode = (ItmBalance *)boundBalanceExpr;
  
  while ((nextBalanceNode != NULL) AND (hasReturnTrue == FALSE))
  {
    if (nextBalanceNode->getPredicate()->getOperatorType() == ITM_RETURN_TRUE)
      hasReturnTrue = TRUE;
    nextBalanceNode = (ItmBalance *)nextBalanceNode->getNextBalance();
  }
  
  if (hasReturnTrue == FALSE)
  {
    nextBalanceNode = (ItmBalance *)boundBalanceExpr;
    ItemExpr *pred;
    ItemExpr *orexpr = NULL;
    
    while (nextBalanceNode != NULL)
    {  
      pred = nextBalanceNode->getPredicate();
      
      if (orexpr != NULL)
        orexpr = new (CmpCommon::statementHeap()) BiLogic(ITM_OR, orexpr, pred);
      else
        orexpr = pred;

      nextBalanceNode = (ItmBalance *)nextBalanceNode->getNextBalance();
    }    
    // Now synthesize type 
    if (orexpr)
      orexpr->synthTypeAndValueId(TRUE);  // redrive Type Synthesis
    
    addSelPredTree(orexpr);
  }

  // Construct the RETDesc for this node.
  //
  RETDesc *resultTable = new(bindWA->wHeap()) RETDesc(bindWA);
  
  // Add the columns from the child to the RETDesc.
  // 
  const RETDesc &childTable = *child(0)->getRETDesc();
  
  const ColumnDescList *sysColList = childTable.getSystemColumnList();
  
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
    sampledColumns() += newColumn->getValueId();
  }
  
  for(i = 0; i < childTable.getDegree(); i++) 
  {
    ValueId columnValueId = childTable.getValueId(i);
    ItemExpr *newColumn = new (bindWA->wHeap()) 
      NotCovered (columnValueId.getItemExpr());
    newColumn->synthTypeAndValueId();
    
    resultTable->addColumn(bindWA,
      childTable.getColRefNameObj(i),
      newColumn->getValueId(),
      USER_COLUMN,
      childTable.getHeading(i));
    sampledColumns() += newColumn->getValueId();
  }
  
  // Set the return descriptor
  //
  setRETDesc(resultTable);
  bindWA->getCurrentScope()->setRETDesc(resultTable);
  
  //
  // Bind the base class.
  //
  return bindSelf(bindWA);
} // RelSample::bindNode()

// -----------------------------------------------------------------------
// RelSample::semanticQueryOptimizeNode()
// This instance of the SQO virtual method is the same as the base class 
// implementation except that it also keeps track of which
// VEGRegion we are currently in.
// -----------------------------------------------------------------------
RelExpr * RelSample::semanticQueryOptimizeNode(NormWA & normWARef)
{
  if (nodeIsSemanticQueryOptimized())
    return this;
  markAsSemanticQueryOptimized() ;

  normWARef.locateAndSetVEGRegion(this);
  // ---------------------------------------------------------------------
  // UnNest the child.
  // ---------------------------------------------------------------------
  child(0) = child(0)->semanticQueryOptimizeNode(normWARef);

  normWARef.restoreOriginalVEGRegion();

  return this;

} // RelSample::semanticQueryOptimizeNode()


/////////////////////////////////////////////////////////////////////////////////////
//
// Methods of the PhysSample 
//
/////////////////////////////////////////////////////////////////////////////////////

PhysSample::~PhysSample()
{
};


RelExpr *
PhysSample::copyTopNode(RelExpr *derivedNode, CollHeap *oHeap)
{
  PhysSample *result;
  
  if (derivedNode == NULL)
    result = new (oHeap) PhysSample();
  else
    result = (PhysSample *)derivedNode;
  
  return RelSample::copyTopNode(result, oHeap);
  
}


// PhysSample::costMethod()
// Obtain a pointer to a CostMethod object providing access
// to the cost estimation functions for nodes of this type.
CostMethod*
PhysSample::costMethod() const
{
  static THREAD_P CostMethodSample *m = NULL;
  if (m == NULL)
    m = new (GetCliGlobals()->exCollHeap())  CostMethodSample();
  return m;
} // PhysSample::costMethod()


ValueIdList
RelSample::mapSortKey(const ValueIdList &sortKey) const
{

  ValueIdMap sampColsMap;

  for(ValueId sampleCol = sampledColumns().init();
      sampledColumns().next(sampleCol);
      sampledColumns().advance(sampleCol)) {

    CMPASSERT(sampleCol.getItemExpr()->getOperatorType() == ITM_NOTCOVERED);

    sampColsMap.addMapEntry(sampleCol, 
                            sampleCol.getItemExpr()->child(0)->getValueId());
  }

  ValueIdList newSortKey;
  
  sampColsMap.mapValueIdListUp(newSortKey, sortKey);
  
  return newSortKey;
}

PhysicalProperty *
PhysSample::synthPhysicalProperty(const Context *context, const Lng32 pn)
{

  const PhysicalProperty * const sppOfChild =
    context->getPhysicalPropertyOfSolutionForChild(0);

  // for now, simply propagate the physical property
  PhysicalProperty *samplePP = new(CmpCommon::statementHeap()) 
    PhysicalProperty(*sppOfChild,
                     mapSortKey(sppOfChild->getSortKey()),
                     sppOfChild->getSortOrderType(),
                     sppOfChild->getDp2SortOrderPartFunc());
  
  return samplePP;
} //  PhysSample::synthPhysicalProperty() 


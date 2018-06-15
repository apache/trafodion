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
* File:         NormRelExpr.C
* Description:  Relational expressions (both physical and logical operators)
*               Methods related to the normalizer
*
* Created:      5/17/94
* Language:     C++
*
*
*
*
******************************************************************************
*/


#include "Debug.h"
#include "Sqlcomp.h"
#include "GroupAttr.h"
#include "opt.h"
#include "NormWA.h"
#include "AllRelExpr.h"
#include "AllItemExpr.h"
#include "ValueDesc.h"
#include "Triggers.h"
#include "Cost.h"
#include "CostMethod.h"
#include "opt.h"
#include "RelGrby.h"
#include "ItemFunc.h"
#include "ControlDB.h"
#include "Analyzer.h"
#include "MultiJoin.h"
#include "CompException.h"
#include "ExpPCodeOptimizations.h"
#include <math.h>

#include "OptRange.h"
#include "ItemOther.h"
#include "ItemExpr.h"
#include "QRDescGenerator.h"
#include "HBaseClient_JNI.h"
#include "HiveClient_JNI.h"

#ifndef TRANSFORM_DEBUG_DECL		// artifact of NSK's OptAll.cpp ...
#define TRANSFORM_DEBUG_DECL
DBGDECLDBG( dbg; )
DBGDECL( static NAString unp; )
#endif

//----------------------------------------------------------------------
// static helper function: this is used by Join::transformNode() to decide
// to put the two vids in a VEG; currently this is checked for TSJs.
//----------------------------------------------------------------------

static NABoolean doTwoVidsReferToSameColumn(ValueId &vid, ValueId &vid1)
{

  NAColumn *col = ((IndexColumn *) vid.getItemExpr())->getNAColumn();
  NAColumn *col1 = ((IndexColumn *) vid1.getItemExpr())->getNAColumn();

  if (col == NULL || col1 == NULL ) return FALSE;
 
  if (col->getColName() != col1->getColName()) return FALSE; 
                     
  if (col->getTableName(TRUE) == col1->getTableName(TRUE))
    {
      if (col->getTableName() == NULL) 
          return FALSE;
        else
          return TRUE;
    }

  return FALSE;

} // doTwoVidsReferToSameColumn()



static void tryToConvertFullOuterJoin(Join *fullOuterJoin, NormWA &normWARef)
{
  NABoolean leftChildMerged, rightChildMerged = FALSE;

  // Check if the left child's VEGRegion is merged
  leftChildMerged = normWARef.locateVEGRegionAndCheckIfMerged(fullOuterJoin, 0 /* left child*/ );

  // Check if the left child's VEGRegion is merged
  rightChildMerged = normWARef.locateVEGRegionAndCheckIfMerged(fullOuterJoin, 1/* right child*/ );

  // should not get here since we disable FOJ to inner in Join::bindNode() and
  // should be removed when we support FOJ to inner
  CMPASSERT(!leftChildMerged && !rightChildMerged);

  if (leftChildMerged && !rightChildMerged)
    fullOuterJoin->setOperatorType(REL_LEFT_JOIN);

  if (rightChildMerged && !leftChildMerged)
    fullOuterJoin->setOperatorType(REL_RIGHT_JOIN);

  if (leftChildMerged && rightChildMerged)
    fullOuterJoin->setOperatorType(REL_JOIN);  // inner Join

  switch (fullOuterJoin->getOperatorType())
    {
    case REL_LEFT_JOIN:
      {
	// This means the left child region (subtreeId = 0) is merged
	// with the parent.
	// Now merge the right child region (subtreeId = 1)
	// and the join predicate region (subtreeId = 2)
	VEGRegion * rightChildVEGRegion = 
	  normWARef.locateVEGRegion(fullOuterJoin, 1/* right child*/ );
	
	VEGRegion * joinPredVEGRegion = 
	  normWARef.locateVEGRegion(fullOuterJoin, 2/* join predicate */ );

	rightChildVEGRegion->mergeVEGRegion(joinPredVEGRegion);
	
	VEGRegion *parentVEGRegion = rightChildVEGRegion->getParentVEGRegion();
	CMPASSERT(parentVEGRegion);  // MUST have a parent
	parentVEGRegion->fixupZonesAfterFullToLeftConversion();
  
	// We don't need to null instantiate the left rows anymore
	fullOuterJoin->nullInstantiatedForRightJoinOutput().clear();
      }
      break; 

    case REL_RIGHT_JOIN:
      break; // Hema TBD - need to flip

    case REL_JOIN:  //Inner Join
      {
	// TBD - Hema. Need some sort of assert that
	// all the three regions (SubtreeId 0,1 & 2) are merged.
	
	fullOuterJoin->selectionPred() += fullOuterJoin->joinPred();
	fullOuterJoin->joinPred().clear();
      }
      break;

    case REL_FULL_JOIN:
      break; // do nothing. has not been transformed.

    default:
      ABORT("Internal error: tryToConvertFullOuterJoin()");
      break;
    }
}



// ***********************************************************************
// $$$$ RelExpr
// member functions for class RelExpr
// ***********************************************************************

// -----------------------------------------------------------------------
// RelExpr::transformNode()
// -----------------------------------------------------------------------
void RelExpr::transformNode(NormWA &   normWARef,
                            ExprGroupId & locationOfPointerToMe)
{
  CMPASSERT( locationOfPointerToMe.getPtr() == this );

  if (nodeIsTransformed())
    return;
  markAsTransformed();

  // ---------------------------------------------------------------------
  // tranformNode takes up a bound tree and turns into a transformed
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
  // ---------------------------------------------------------------------
  Int32 arity = getArity();

  // ---------------------------------------------------------------------
  // Transform each child.
  // Pull up their transformed predicates
  // recompute their required inputs.
  // ---------------------------------------------------------------------
  for (Int32 i = 0; i < arity; i++)
    {
      // ---------------------------------------------------------------------
      // Make values available to child
      // ---------------------------------------------------------------------
      child(i)->getGroupAttr()->addCharacteristicInputs
                          (getGroupAttr()->getCharacteristicInputs());

      child(i)->transformNode(normWARef, child(i)); 
      // My child has now been transformed.
      // A new semiJoin may now be my direct descendant and my original
      // child a descendant of it.
      // In either case my child has now been transformed.
    };

  // Pull up the predicates and recompute the required inputs
  // of whoever my children are now.
  pullUpPreds();

  // transform the selection predicates
  transformSelectPred(normWARef, locationOfPointerToMe);

} // RelExpr::transformNode()

// QSTUFF
// ***********************************************************************
// $$$$ RelExpr
// member functions for class RelExpr
// ***********************************************************************

// -----------------------------------------------------------------------
// RelExpr::checkReadWriteConflicts()
// -----------------------------------------------------------------------
RelExpr::rwErrorStatus RelExpr::checkReadWriteConflicts(NormWA & normWARef)
{
  rwErrorStatus rc;

  Int32 arity = getArity();
  
  for (Int32 i = 0; i < arity; i++)
    {
      if ( (rc = child(i)->checkReadWriteConflicts(normWARef) ) != RWOKAY)
          return rc;
    }

  return RWOKAY;
} // RelExpr::checkReadWriteConflicts()

// Build the MapValueIds node to sit on top of a GroupBy.
// This function is used by the SemanticQueryOptimization phase 
// to insert a MapValueIds node on top of a GroupBy and a LeftJoin
// when a Join is converted to a LeftJoin during unnesting 
// Also update the outputs of the MapValueIds node accordingly.
//------------------------------------------------------------------------------

MapValueIds * GroupByAgg::buildMapValueIdNode(ValueIdMap *map)
{
  CollHeap* stmtHeap = CmpCommon::statementHeap() ;

  // Create the MapValueIds node with the mapping.
  MapValueIds *mapNode = new (stmtHeap) MapValueIds(this, *map);

  // The inputs are same as for the child
  mapNode->getGroupAttr()->addCharacteristicInputs(
                               getGroupAttr()->getCharacteristicInputs());

  mapNode->primeGroupAttributes();
  return mapNode;
} // GroupByAgg::buildMapValueIdNode()  

// -----------------------------------------------------------------------
// RelRoot::checkReadWriteConflicts()
// -----------------------------------------------------------------------
RelExpr::rwErrorStatus RelRoot::checkReadWriteConflicts(NormWA & normWARef)
{
  
  // checking is only done in the presence of embedded deletes and updates
  if (!(getGroupAttr()->isEmbeddedUpdateOrDelete()) && isTrueRoot())
    return RWOKAY;
  
  rwErrorStatus rc;

  Int32 arity = getArity();
  
  for (Int32 i = 0; i < arity; i++) 
    {
      if ( (rc = child(i)->checkReadWriteConflicts(normWARef) ) != RWOKAY)
          return rc;
    }

  return RWOKAY;
} // RelRoot::checkReadWriteConflicts()

// -----------------------------------------------------------------------
// RelScan::checkReadWriteConflicts()
// -----------------------------------------------------------------------
RelExpr::rwErrorStatus Scan::checkReadWriteConflicts(NormWA & normWARef)
{
  
  NAString fileName ( 
    getTableDesc()->getNATable()->
     getClusteringIndex()->getFileSetName().getQualifiedNameAsString(),
    CmpCommon::statementHeap());
  
  CollIndex i = 0;
  for (i=0; i < normWARef.getWriteList().entries(); i++)
    if (strcmp(normWARef.getWriteList()[i], fileName) == 0) {
      *CmpCommon::diags() << DgSqlCode(-4152)   
        << DgTableName(getTableDesc()->getNATable()->getTableName().getQualifiedNameAsAnsiString()); 
      return RWERROR;
    }
    
  for (i=0; i < normWARef.getReadList().entries(); i++)
    if (strcmp(normWARef.getReadList()[i], fileName) == 0) {
      return RWOKAY;
    }
    
  normWARef.getReadList().insert(fileName);
  return RWOKAY;
      
} // Scan::checkReadWriteConflicts()

// -----------------------------------------------------------------------
// GenericUpdate::checkReadWriteConflicts()
// -----------------------------------------------------------------------
RelExpr::rwErrorStatus GenericUpdate::checkReadWriteConflicts(NormWA & normWARef)
{
  
  // ---------------------------------------------------------------------
  // This routine checks whether the same table is both read from and 
  // updated in the same query. This is done after transformation and binding
  // to ensure that all inlining of operations already happened and removal
  // as well as removal of structural nodes. 
  // ---------------------------------------------------------------------
  
  NAString fileName( 
    getTableDesc()->getNATable()->
     getClusteringIndex()->getFileSetName().getQualifiedNameAsString(),
    CmpCommon::statementHeap());
  
  CollIndex i = 0;
  for ( i=0; i < normWARef.getReadList().entries(); i++)
    if (strcmp(normWARef.getReadList()[i], fileName) == 0) {
      *CmpCommon::diags() << DgSqlCode(-4152)   
        << DgTableName(getTableDesc()->getNATable()->getTableName().getQualifiedNameAsAnsiString()); 
      return RWERROR;
    }
    
  for ( i=0; i < normWARef.getWriteList().entries(); i++)
    if (strcmp(normWARef.getWriteList()[i], fileName) == 0) {
      return RWOKAY;
    }
      
  normWARef.getWriteList().insert(fileName);
  return RWOKAY;
      
} // GenericUpdate::checkReadWriteConflicts()

// QSTUFF

// -----------------------------------------------------------------------
// Could/should be a ValueIdSet:: method.
// NEED A WAY TO GUARANTEE THAT THIS SET REPRESENTS AN *AND*ed LOGICAL SET,
// not an ITM_ITEM_LIST or other backboned set.
// This guarantee is true for the two RelExpr::transformSelectPred() callers
// in this file.
// -----------------------------------------------------------------------
static void applyTruthTable(ValueIdSet & vs)
{
  // If this ValueIdSet is an ANDed set of value-items, i1 AND i2 AND ..., then:
  //    Remove any item which is a simple TRUE:
  //            il..AND TRUE AND..ir    => il AND ir
  //    If any item is a simple FALSE, ignore all other items:
  //            il..AND FALSE AND..ir   => FALSE

  for (ValueId vid = vs.init(); vs.next(vid); vs.advance(vid))
    {
      OperatorTypeEnum op = vid.getItemExpr()->getOperatorType();
      if (op == ITM_RETURN_TRUE)
        vs -= vid;
      else if (op == ITM_RETURN_FALSE)
        {
          vs.clear();
          vs += vid;
          break;
        }
    }
}

// Breadth First Traversal to print the transformed and source tree.
Int32 printTree(ItemExpr *ptrToTree,ItemExpr *parent, Int32 depth, Int32 l1)
{
  if (ptrToTree != NULL) 
  {
    Int32 left, right;

    if (depth == 0) 
    {
      if(l1 == 0)
	cout << "root ";
      if(l1 == 1)
	cout << "left child ";
      if(l1 == 2)
	cout << "right child ";
      cout << " ValueId: " << ptrToTree->getValueId() << "  Value:" << ptrToTree->getText() << " Parent ValueId: " << parent->getValueId() << " Parent Value:" << parent->getText() << endl;
      return 1;
    }
    left = printTree(ptrToTree->child(0),ptrToTree, depth - 1, 1);
    right = printTree(ptrToTree->child(1),ptrToTree, depth - 1, 2);
    return left || right;
  }
  return 0;
}

static ItemExpr* transformUnSupportedNotEqualTo(CollHeap *heap, ItemExpr* itemNotEqual)
{
  if(itemNotEqual->getOperatorType() == ITM_NOT_EQUAL)
  {
    ItemExpr* newLeftNode = new (heap) BiRelat(ITM_LESS,itemNotEqual->child(0),itemNotEqual->child(1));
    ItemExpr* newRightNode = new (heap) BiRelat(ITM_GREATER,itemNotEqual->child(0),itemNotEqual->child(1));
    ItemExpr* result = new (heap) BiLogic(ITM_OR,newLeftNode,newRightNode);
    result->synthTypeAndValueId();
    return result;
  }
  return NULL;
}

// shrinkNewTree(argv[]): 
// This function in turn calls union() and intersection() function on RangeSpec object, through 
// the wrapper object RangeSpecRef->getRangeObject() to club the values together.
// getRangeObject() call gives RangeSpec object. 	    
// This gets called while performing  
// (1) an "OR" operation between an  RangeSpecRef ItemExpression
//     (operator type =ITM_RANGE_SPEC_FUNC) and an OR'ed Set (operator type =ITM_OR)
// (2) an "AND" operation between an  RangeSpecRef ItemExpression
//     (operator type =ITM_RANGE_SPEC_FUNC) and an OR'ed Set (operator type =ITM_AND)
// (3) an "AND/OR" operation between an  RangeSpecRef ItemExpression
//     (operator type =ITM_RANGE_SPEC_FUNC) and an RangeSpecRef ItemExpression (operator type =ITM_RANGE_SPEC_FUNC)
//			           
// Simple usage example:  where a = 10 or b = 20 or a=30;
//                
// Step 1:
//                            Or'ed Set = (RangeSpecRef(a=10),RangeSpecRef(b=20))
//                            RangeSpecRef = (a=30)
// Step 2:
//                            Or'ed Set = (RangeSpecRef(a=10,a=30),RangeSpecRef(b=20))
// Step 3:                    returns true;
//                           
//
// argv[] ={  OperatorTypeEnum op, ItemExpr *ptrToNewTree, RangeSpecRef* xref}
// argv[0] -> needed since union and insersection on a RangeSpec object is determined by this parameter: 
//         -> { ITM_OR, ITM_AND } 
// argv[1] ->  { OR'ed set, AND'ed Set} where OR'ed set = { RangeSpecRef(a), RangeSpecRef(b), ..} 
//             OR'ed set and AND'ed set can only exist if it is more than one column, the name "set" represents that
// argv[2] ->  xref = Xternal,  which is the RangeSpecRef object needs to be merged
// argv[3] ->  normWARef, the Normalizer work area, passed on to getRangeItemExpr()
//             so the range ItemExpr can be normalized.
// shrinkNewTree() returns   (true/false) -> Boolean instead of void is needed to optimize the traversal of the Tree.
NABoolean Scan::shrinkNewTree(OperatorTypeEnum op, ItemExpr *ptrToNewTree, 
                              RangeSpecRef* xref, NormWA& normWARef)
{
 NABoolean status = false;
 // Need to work on the leaf's of ptrToNewTree tree, 
 // which is of RangeSpecRef(ITM_RANGE_SPEC_FUNC) type object
 if (ptrToNewTree->getOperatorType() == ITM_RANGE_SPEC_FUNC) 
 {
   RangeSpecRef* rangeIE = (RangeSpecRef *)ptrToNewTree;
   OptNormRangeSpec* destObj = rangeIE->getRangeObject();
   CMPASSERT(destObj != NULL);
   if (op == ITM_OR) 
   {
     if (destObj->getRangeExpr()->getValueId() == 
       xref->getRangeObject()->getRangeExpr()->getValueId())
     {
       destObj->unionRange(xref->getRangeObject());
       rangeIE->setChild(1, const_cast<ItemExpr*>
                               (destObj->getRangeItemExpr(&normWARef)));
       status = true;
     }
   }
   else if( op == ITM_AND) 
   {
     if (destObj->getRangeExpr()->getValueId() == 
       xref->getRangeObject()->getRangeExpr()->getValueId())
     {
       destObj->intersectRange(xref->getRangeObject());
       rangeIE->setChild(1, const_cast<ItemExpr*>
                               (destObj->getRangeItemExpr(&normWARef)));
       status = true;
     }
   }
 }
 else 
 {
   // Internal and's for ROOT as or which can't be converted and vice versa.
   //  for the above tree, op = ITM_OR which is the root
   //  whenever we first hit an "AND'ed set" for the OR root 
   //  we don't traverse the AND'ed set formed.
   //  since Or'ed set and And'ed set are disjoint.
   //  For a tree like the following, we dont traverse the cut(s) while 
   //  merging the RangeSpecRef with New ItemExpression:
   //              Or
   //              / \
   //             /   RangeSpecRef(a(4))
   //             or'ed set
   //           /   \/
   //          /   / \
   //         /   /  and'ed set
   //        /   cut     /   \
   //               	 RangeSpecRef RangeSpecRef
   //	     /          (b,BiRel(=1)) (c,BiRel(=3))
   //      /
   //   RangeSpecRef
   //	 (leftchild=a,
   //  	 rightchild=Reconstructed ItemExpression ( for values{1,2}) i.e.   or
   //              \    cut(we don't traverse)                                /\
   //               \ /                                                      /  \
   //               / \                                                    a=1 a=2
   //              /   \
   //   	         and'ed set
   //                 /  \
   //                /    \
   // RangeSpecRef(a,BiRel(=4))RangeSpecRef(b,BiRel(=6))
   if (op == ptrToNewTree->getOperatorType() )
   { 
     // Traverse Left of OR'ed set or AND'ed set
     status = shrinkNewTree(op,ptrToNewTree->child(0),xref,normWARef);
     // Optimization: if(!status)
     // No need to traverse the right child of the tree, 
     // since already found the match.
     if(!status) 
       status = shrinkNewTree(op,ptrToNewTree->child(1),xref,normWARef);
   }
 }
 return status;
}

#define AVR_STATE0 0
#define AVR_STATE1 1
#define AVR_STATE2 2

ItemExpr * Scan::applyAssociativityAndCommutativity(
					      QRDescGenerator *descGenerator,
					      CollHeap *heap, 
					      ItemExpr *origPtrToOldTree, 
					      NormWA& normWARef,
					      NABoolean& transformationStatus)
{
  if( CmpCommon::getDefault(SUBSTRING_TRANSFORMATION) != DF_OFF )
    return origPtrToOldTree;

  ItemExpr *         newLeftNode  = NULL ;
  ItemExpr *         newRightNode = NULL ;
  ItemExpr *         newNode      = NULL ;
  ItemExpr *         ptrToOldTree = NULL ;

  //
  // applyAssociativityAndCommutativity() used to be called recursively not just
  // for all the items in an expression but for all the items in the node
  // tree for an entire query. Consequently, we must eliminate the recursive
  // calls to applyAssociativityAndCommutativity() by keeping the
  // information needed by each "recursive" level in the HEAP and using
  // a "while" loop to look at each node in the tree in the same order as
  // the old recursive technique would have done.
  // The information needed by each "recursive" level is basically just
  // * a pointer to what node (ItemExpr *) to look at next,
  // * a "state" value that tells us where we are in the
  //   applyAssociativityAndCommutativity() code for the ItemExpr node
  //   that we are currently working on, and
  // * a pointer to the new left node (from the "recursive" call on child(0))
  //   which we need to have available *after* recursing down the child(1) tree.
  //   NOTE: We don't have to keep the ptr to the new right node in a similar
  //   fashion because the code does not assign to 'newRightNode' until *after*
  //   all recursion is finished.
  //
  ARRAY( ItemExpr * ) IEarray(heap, 10) ; //Initially 10 elements (no particular reason to choose 10)
  ARRAY( Int16 )      state(heap, 10)   ; //These ARRAYs will grow automatically as needed.)
  ARRAY( ItemExpr *) leftNodeArray(heap, 10);

  Int32   currIdx = 0;
  IEarray.insertAt( currIdx, origPtrToOldTree );
  state.insertAt(   currIdx, AVR_STATE0 );

 // if(ptrToOldTree->getOperatorType() == ITM_NOT_EQUAL)
   // ptrToOldTree = transformUnSupportedNotEqualTo(heap,ptrToOldTree);

  while ( currIdx >= 0 )
  {
     ptrToOldTree = IEarray[currIdx] ;

     // Convert the expression to a rangespec immediately under any of the following
     // conditions:
     //   1) The expression is a leaf predicate (not an AND or OR).
     //   2) The expressions is rooted by an OR node that is derived from an in-list.
     //      This is guaranteed to be an OR backbone of conditions on the same
     //      column/expr, and can be handled by createRangeSpec() without the overhead
     //      of recursing through applyAssociativityAndCommutativity(), which incurs
     //      a massive usage of memory for a large in-list. See bug #3248.
     //   3) The expression has already undergone rangespec conversion.
     if((ptrToOldTree->getOperatorType() != ITM_AND &&
         ptrToOldTree->getOperatorType() != ITM_OR)
           ||
        (ptrToOldTree->getOperatorType() == ITM_OR &&
         static_cast<BiLogic*>(ptrToOldTree)->createdFromINlist())
           ||
         ptrToOldTree->isRangespecItemExpr())
     {
       OptNormRangeSpec* range = static_cast<OptNormRangeSpec*>(
                                   OptRangeSpec::createRangeSpec(descGenerator,
                                                                 ptrToOldTree,
                                                                 heap,
                                                                 TRUE));

       // Transforms all Birel ItemExpression into RangeSpecRef ItemExpression
       if( range != NULL)
       {
         RangeSpecRef *refrange = new (heap)
                                  RangeSpecRef(ITM_RANGE_SPEC_FUNC,
                                               range,
                                               range->getRangeExpr(),
                                               range->getRangeItemExpr(&normWARef));
         transformationStatus = TRUE;
         // Ensure that base column value ids are replaced by vegrefs (Bugzilla 2808).
         refrange->getReplacementExpr()->normalizeNode(normWARef);
         newNode = refrange ;
       }
       else
         newNode = ptrToOldTree ;
     }
     else
     {
       // Recurse through for ITM_AND/ITM_OR
       // depth first traversal
       if ( state[currIdx] == AVR_STATE0 )
       {
          state.insertAt( currIdx, AVR_STATE1 ) ;
          currIdx++ ;                               //"Recurse" down to child 0
          state.insertAt(   currIdx, AVR_STATE0 ) ; // and start that child's state at 0
          IEarray.insertAt( currIdx, ptrToOldTree->child(0) ) ;
          continue ;
       }
       else if ( state[currIdx] == AVR_STATE1 )
       {
          leftNodeArray.insertAt( currIdx, newNode ); //Save the "return value" from recursion

          state.insertAt( currIdx, AVR_STATE2 ) ;
          currIdx++ ;                               //"Recurse" down to child 1
          state.insertAt(   currIdx, AVR_STATE0 ) ; // and start that child's state at 0
          IEarray.insertAt( currIdx, ptrToOldTree->child(1) ) ;
          continue ;
       }
       else
       {
          newLeftNode    = leftNodeArray[currIdx] ;   //Restore 'newLeftNode'
          state.insertAt( currIdx, AVR_STATE0 ) ;     //Mark us as done with this IE

          newRightNode   = newNode ; // Set newRightNode = "return value" from recursion
       }

       // case OR:
       if ((newLeftNode->getOperatorType() == ITM_RANGE_SPEC_FUNC) &&
	   (newRightNode->getOperatorType() == ITM_RANGE_SPEC_FUNC))
       {
         // where a = 10 or b =20
         // where a = 10 or a =20
         if(shrinkNewTree(ptrToOldTree->getOperatorType(),
                          newLeftNode, (RangeSpecRef *)newRightNode, normWARef))
         {
	   newNode = (ItemExpr *)newLeftNode;
         }
         else
         {
	   // where a = 10 or b =20
	   newNode = new (heap) BiLogic(ptrToOldTree->getOperatorType(),
				       (RangeSpecRef *)newLeftNode,
				       (RangeSpecRef *)newRightNode);
         }
       }
       else if((newLeftNode->getOperatorType() == ptrToOldTree->getOperatorType())
	       && (newRightNode->getOperatorType() == ITM_RANGE_SPEC_FUNC))
       {
         // where a = 10 or b =20  or a =30
         // ored set = ((a=10),(b=20))
         // we are merging anded set with rangespec (a=30)
         // if shrinkNewTree() returns true then intervals are already merged in shrinkNewTree(),
         // since matching columns are
         // found in the ored set.
         // else we add the rangespec into ored set.
         if(!shrinkNewTree(ptrToOldTree->getOperatorType(),
                           newLeftNode,(RangeSpecRef *)newRightNode,normWARef))
         {
	   newNode = new (heap) BiLogic(ptrToOldTree->getOperatorType(),
				        newLeftNode,
				        (RangeSpecRef *)newRightNode);
         }
         else
	   newNode = (ItemExpr *)newLeftNode;
       }
       // This condition is redundant, not able to formulate any query for this
       // we can't generate tree like
       //                 Or
       //                / \
       //            OrSet OrSet
       else if((newLeftNode->getOperatorType() ==
		       ptrToOldTree->getOperatorType()) &&
	      (newRightNode->getOperatorType() ==
		       ptrToOldTree->getOperatorType()))
       {
         newNode = new (heap) BiLogic(ptrToOldTree->getOperatorType(),
				      newLeftNode,newRightNode);
       }
       else if ((newLeftNode->getOperatorType() == ITM_RANGE_SPEC_FUNC)
	       && (newRightNode->getOperatorType() == ptrToOldTree->getOperatorType()))
       {
         // where a = 10 or b =20  or a =30
         // ored set = ((a=10),(b=20))
         // we are merging anded set with rangespec (a=30)
         // if shrinkNewTree() returns true then intervals are already merged in shrinkNewTree(),
         // since matching columns are
         // found in the ored set.
         // else we add the rangespec into ored set.

         if(!shrinkNewTree(ptrToOldTree->getOperatorType(),
                           newRightNode,(RangeSpecRef *)newLeftNode,normWARef))
         {
	   newNode = new (heap) BiLogic(ptrToOldTree->getOperatorType(),
				         (RangeSpecRef *)newLeftNode,
				         newRightNode);
         }
         else
	   newNode = (ItemExpr *)newRightNode;
       }
       else
       {
         newNode = new (heap) BiLogic(ptrToOldTree->getOperatorType(),
				      newLeftNode,newRightNode);
       }

       // If user had specified selectivity for original predicate,
       // then apply the same to the new predicate as well.
       if(ptrToOldTree->isSelectivitySetUsingHint())
       {
         if(newNode->getOperatorType() == ITM_RANGE_SPEC_FUNC)
         {
           newNode->child(1)->setSelectivitySetUsingHint();
           newNode->child(1)->setSelectivityFactor(ptrToOldTree->getSelectivityFactor());
         }
         else
         {
           newNode->setSelectivitySetUsingHint();
           newNode->setSelectivityFactor(ptrToOldTree->getSelectivityFactor());
         }
       }

       CMPASSERT(newNode != NULL);
     }
     if ( state[currIdx] == AVR_STATE0 )  // if done with current ItemExpr
       currIdx-- ;                        // then return to parent
  }
  return newNode;
}
// -----------------------------------------------------------------------
// RelExpr::transformSelectPred()
// Do the common steps in processing selection predicates
// -----------------------------------------------------------------------
void RelExpr::transformSelectPred(NormWA &   normWARef,
                                  ExprGroupId & locationOfPointerToMe)
{
  // ---------------------------------------------------------------------
  // This is a common procedure for relExprs to process the subquery
  // or Isolated UDFunction predicates in its select list. It sets up the 
  // required inputs and outputs of the RelExpr before adding new Join
  // nodes above it. The subquery/UDFunction transformation logic needs the
  // required inputs of the node to properly reflect the final
  // (transformed) inputs and outputs.
  // ---------------------------------------------------------------------
  ValueIdSet subqueryOrIsolatedUDFunctionPredicates;
  const NABoolean movePredicates = TRUE;
  const NABoolean postJoinPredicates = TRUE;
  
  // ---------------------------------------------------------------------
  // Compute the potential inputs and outputs for the node before
  // transforming the selectionPred so that the characteristic outputs
  // are correct (they are needed by the subquery transformation).
  // ---------------------------------------------------------------------
  primeGroupAttributes();
 
  // remove the subquery predicates from the select list
  selectionPred().removeSubqueryOrIsolatedUDFunctionPredicates (
                                  subqueryOrIsolatedUDFunctionPredicates);

  // ---------------------------------------------------------------------
  // Save the original inputs to use when the subquery predicates get
  // transformed.
  // ---------------------------------------------------------------------
  ValueIdSet externalInputs = getGroupAttr()->getCharacteristicInputs();

  // ---------------------------------------------------------------------
  // Transform the remaining selection predicates.
  // ---------------------------------------------------------------------
  if (selectionPred().transformNode(normWARef, locationOfPointerToMe,
                                    externalInputs, movePredicates,
                                    postJoinPredicates))
    {
      // -----------------------------------------------------------------
      // No subqueries should have been left here.
      // -----------------------------------------------------------------
      CMPASSERT(0);
    }
  applyTruthTable(selectionPred());

  // ---------------------------------------------------------------------
  // Transform the subquery predicates.
  // ---------------------------------------------------------------------
  // semiJoin's that are added should be added between me and my parent.
  if (subqueryOrIsolatedUDFunctionPredicates.transformNode(normWARef, 
                                       locationOfPointerToMe, 
                                       externalInputs, movePredicates,
                                       postJoinPredicates))
    {
      // -----------------------------------------------------------------
      // The transformed subquery predicate requires values that are
      // produced by the semiJoin above me.
      // The transform predicate was moved there.
      // -----------------------------------------------------------------
    }
  applyTruthTable(subqueryOrIsolatedUDFunctionPredicates);

  // ---------------------------------------------------------------------
  // Add the transform subquery predicates back to the selection predicates.
  // Some subquery predicates transform into regular predicates
  // e.g. EXISTS (SELECT MAX(t.a) FROM t) ==> TRUE
  // ---------------------------------------------------------------------
  selectionPred() += subqueryOrIsolatedUDFunctionPredicates;

  // ---------------------------------------------------------------------
  // If I am no longer the direct descendant of my parent then transform
  // the usurper. During its transformation it may get a taste of its
  // own medicine and stop becoming the direct descendant of my parent.
  // ---------------------------------------------------------------------
  if (locationOfPointerToMe != (const RelExpr *)this)
    {
      locationOfPointerToMe->transformNode(normWARef,
                                           locationOfPointerToMe);

  // ---------------------------------------------------------------------
  // If this whole subquery is under an OR or inside a complicated expr,
  // this flag has been set while the expr is transformed. This is done
  // so that when the new join introduced and its right child are being
  // transformed, we won't incorrectly use the selection predicates in
  // the subquery to convert left join elsewhere into inner join. This
  // has been achieved now, so resetting the flag.
  // ---------------------------------------------------------------------
      if (normWARef.subqUnderExprTree())
        normWARef.restoreSubqUnderExprTreeFlag();

      
      // We are on our way back from a number of transformNode()s.
      // Let's just make sure that the final usurper got transformed
      CMPASSERT( locationOfPointerToMe->nodeIsTransformed());
    }

    // If there is a selection predicate, we check to see if there are
    // constant expressions in it, and we compute them, i.e. this is
    // Constant Folding
    ValueIdList foldPreds;
    foldPreds = getSelectionPred();
    if (!foldPreds.isEmpty()) {
      NABoolean allTrue = foldPreds.constantFolding();
      if (!foldPreds.isEmpty()) {
        CMPASSERT(selPredTree() == NULL);
	if (allTrue) foldPreds.clear();
        setSelectionPredicates(foldPreds);
      }
    }

} // RelExpr::transformSelectPred()

// -----------------------------------------------------------------------
// RelExpr::pullUpPreds()
// Most operators transmit predicates to their parents as-is.
// -----------------------------------------------------------------------
void RelExpr::pullUpPreds()
{
  // ---------------------------------------------------------------------
  // This method is called on a RelExpr so that it can gather the
  // predicates of its immediate children unto itself.
  // It is a virtual function.
  // PullUpPreds gets from all the children the predicates they
  // can surrender is adds them to the local selectionPred()
  // ---------------------------------------------------------------------
  Int32 arity = getArity();
  for (Int32 i = 0; i < arity; i++)
    {
        selectionPred() += child(i)->getSelectionPred();
        child(i)->selectionPred().clear();
        child(i)->recomputeOuterReferences();
    };

  // ---------------------------------------------------------------------
  // WARNING: One rule that this procedure must follow is
  // that recomputeOuterReferences() must be called on the children even
  // if no predicates are pulled up from them. This is to correct
  // the outer references that are added to a right child of a
  // semi or outer join when processing subqueries in the ON clause.
  // ---------------------------------------------------------------------
  
} // RelExpr::pullUpPreds()

// -----------------------------------------------------------------------
// RelExpr::recomputeOuterReferences()
// -----------------------------------------------------------------------
void RelExpr::recomputeOuterReferences()
{
  // ---------------------------------------------------------------------
  // This is virtual method on RelExpr.
  // When this is called it is assumed that the children have already
  // been transformed.
  // The required inputs of the child are therefore already minimal
  // and sufficient.
  // It is also assumed that the RelExpr itself has been bound.
  // That implies that the group attributes have already been allocated
  // and the required inputs is a sufficient (but not necessarilly minimum)
  // set of external values needed to evaluate all expressions in this subtree.
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // Delete all those input values that are no longer referenced on
  // this operator because the predicates that reference them have 
  // been pulled up.
  // ---------------------------------------------------------------------
  ValueIdSet outerRefs = getGroupAttr()->getCharacteristicInputs(); 

  // Remove from outerRefs those valueIds that are not needed
  // by my selection predicate
  selectionPred().weedOutUnreferenced(outerRefs);

  // Add to outerRefs those that my children need.
  Int32 arity = getArity();
  for (Int32 i = 0; i < arity; i++)
    {
      outerRefs += child(i).getPtr()->getGroupAttr()->getCharacteristicInputs();
    }

  // set my Character Inputs to this new minimal set.
  getGroupAttr()->setCharacteristicInputs(outerRefs);
} // RelExpr::recomputeOuterReferences()  

// -----------------------------------------------------------------------
// RelExpr::rewriteNode()
// -----------------------------------------------------------------------
void RelExpr::rewriteNode(NormWA & normWARef)
{
  // ---------------------------------------------------------------------
  // Rewrite the expressions of each child.
  // ---------------------------------------------------------------------
  Int32 nc = getArity();
  for (Int32 i = 0; i < nc; i++)
    child(i)->rewriteNode(normWARef);
  // ---------------------------------------------------------------------
  // Rewrite the expressions in the selection preidcates.
  // ---------------------------------------------------------------------
  if (selectionPred().normalizeNode(normWARef))
    {
    }

  // ++MV
  if (getUniqueColumns().normalizeNode(normWARef))
    {
    }
  // --MV

  // ---------------------------------------------------------------------
  // Rewrite the expressions in the Group Attributes.
  // ---------------------------------------------------------------------
        getGroupAttr()->normalizeInputsAndOutputs(normWARef);

} // RelExpr::rewriteNode()

// -----------------------------------------------------------------------
// RelExpr::normalizeNode()
// -----------------------------------------------------------------------
RelExpr * RelExpr::normalizeNode(NormWA & normWARef)
{
  if (nodeIsNormalized())
    return this;
  markAsNormalized();

  Int32 arity = getArity();
  // --------------------------------------------------------------------
  // Check which expressions can be evaluated by my child.
  // Modify the Group Attributes of those children who
  // ---------------- inherit some of
  // these expressions.
  // ---------------------------------------------------------------------
  
  pushdownCoveredExpr(getGroupAttr()->getCharacteristicOutputs(),
		      getGroupAttr()->getCharacteristicInputs(),
		      selectionPred());

  // ---------------------------------------------------------------------
  // Transform each child.
  // ---------------------------------------------------------------------
  for (Int32 i = 0; i < arity; i++)
    child(i) = child(i)->normalizeNode(normWARef);

  // The essential char. outputs of my child can be fully computed only
  // when the essential char. outputs of my grandchildren are fully computed
  // This is because one of the rules for computing essential char. outputs
  // is : An ouput that is essential in my child will stay essential in me.
  // This rule can be enforced in the bottom-up part of the tree walk, while
  // pushDownCoveredExpr which computes outputs is performed in the top-down
  // part. Therefore we need to call this method here to set up the essential
  // outputs correctly. PushDownCoveredExpr also has this logic, in phases
  // beyond the normalizer this method need not be called again, its only that
  // for the first time the logic in pushDownCoveredExpr is not sufficient as
  // the grandchildren don't have any essential outputs yet.
  fixEssentialCharacteristicOutputs();

  return this;
} // RelExpr::normalizeNode()

// -----------------------------------------------------------------------
// RelExpr::semanticQueryOptimizeNode()
// -----------------------------------------------------------------------
RelExpr * RelExpr::semanticQueryOptimizeNode(NormWA & normWARef)
{ 
  Int32 arity = getArity();
  
  // ---------------------------------------------------------------------
  // SemanticQueryOptimize each child.
  // ---------------------------------------------------------------------
  for (Int32 i = 0; i < arity; i++)
    child(i) = child(i)->semanticQueryOptimizeNode(normWARef);


  return this;

} // RelExpr::semanticQueryOptimizeNode()


// -----------------------------------------------------------------------
// RelExpr::getMoreOutputsIfPossible()
// This method is recursive. It is capable of making a tree walk down from 
// the RelExpr pointed to by "this" and promoting the outputs
// of the children of each node so that the "this" node has all columns
// required to produce the valueids in the parameter outputsNeeded. 
// If all members of outputsNeeded cannot be produced due to the presence 
// of some operator that does not allow outputs from children to flow through 
// (like groupby or sequence) then this method returns FALSE.
// Currently, this method is used towards the end of unnesting a tsj node.
// As part of the unnesting process, the left child of the join is 
// required to produce additional columns which have been identified 
// as a unique set for the left sub-tree. Sometimes it is possible that
// the children of the Join's left child are not producing one or more
// members of this unique set.
// -----------------------------------------------------------------------
NABoolean RelExpr::getMoreOutputsIfPossible(ValueIdSet& outputsNeeded)
{
  // no additional outputs are needed.
  if (outputsNeeded.isEmpty())
    return TRUE;

  Int32 i, nc ;
  ValueIdSet tempSet, potentialOutputsFromChildren, newOutputsNeeded ;
  ValueIdSet emptySet,coveredExprs, coveredSubExprs;
  GroupAttributes     fakeGA;
  NABoolean gotOutputsNeeded = FALSE ;

  // in the top down part of the tree-walk check if the children of
  // current node can produce the required outputs, if so the tree
  // walk need not proceed any futher.
  nc = getArity();
  for(i = 0; i < nc ; i++)
  {
    child(i).getPtr()->getPotentialOutputValuesAsVEGs(tempSet);
    potentialOutputsFromChildren += tempSet ;
    fakeGA.addCharacteristicInputs(child(i).getGroupAttr()->getCharacteristicInputs());
  }
  fakeGA.addCharacteristicOutputs(potentialOutputsFromChildren);
  fakeGA.coverTest(outputsNeeded,             
		   emptySet,    // additional inputs not provided
		   coveredExprs, 
		   emptySet,  // additional inputs not provided
		   &coveredSubExprs,
		   &newOutputsNeeded);

  if (NOT newOutputsNeeded.isEmpty())
  {
    // children of current node could not produce all needed outputs
    // proceed further down the tree, looking for needed outputs.
    for(i = 0; i < nc ; i++)
    {
      if (NOT gotOutputsNeeded)
	gotOutputsNeeded = child(i).getPtr()->
		      getMoreOutputsIfPossible(newOutputsNeeded) ;
    }
  }

    // In the bottom-up part of the tree walk, add to the outputs of 
    // of children whatever values will cover any part of outPutsNeeded.
    // If ouputsNeeded cannot be entirely satisfied, return FALSE.

    // check what the children are capable of producing now that their
    // outputs have been possibly increased, If children still cannot 
    // produce all all outputs needed (ever after the recursive 
    // call returned TRUE), then that means that the child is an operator
    // like SEQUENCE that does not allow outputs to flow through. Return
    // FALSE in this case.
    potentialOutputsFromChildren.clear();
    newOutputsNeeded.clear();
    for(i = 0; i < nc ; i++)
    {
      child(i).getPtr()->getPotentialOutputValuesAsVEGs(tempSet);
      potentialOutputsFromChildren += tempSet ;
    }
    fakeGA.addCharacteristicOutputs(potentialOutputsFromChildren);
    fakeGA.coverTest(outputsNeeded,             
		     emptySet,    // additional inputs not provided
		     coveredExprs, 
		     emptySet,  // additional inputs not provided
		     &coveredSubExprs,
		     &newOutputsNeeded);
    
    // increase outputs for children if all is well.
    ValueIdSet outputsToAdd, maxOutputs ;
    for(i = 0; i < nc ; i++)
    {
      outputsToAdd.clear();
      maxOutputs.clear();
      child(i).getPtr()->getPotentialOutputValuesAsVEGs(maxOutputs);
      outputsToAdd.accumulateReferencedValues( 
      maxOutputs,
      outputsNeeded);
      child(i)->getGroupAttr()->addCharacteristicOutputs(outputsToAdd);
      if (getOperatorType() == REL_MAP_VALUEIDS) 
      {
	((MapValueIds *)this)->addSameMapEntries(outputsToAdd);
      }
     // child(i).getGroupAttr()->computeCharacteristicIO
//	                                            (emptySet,  // no additional inputs
  //                                                   outputsNeeded);
    }
    if (NOT newOutputsNeeded.isEmpty()) 
    {
      outputsNeeded = newOutputsNeeded ;
      return FALSE ;
    }
    else
    {
      outputsNeeded.clear();
      return TRUE ;
    }
}
// RelExpr::getMoreOutputsIfPossible()


// ***********************************************************************
// $$$$ Join 
// member functions for class Join
// ***********************************************************************

// -----------------------------------------------------------------------
// Join::transformNode()
// -----------------------------------------------------------------------
void Join::transformNode(NormWA & normWARef,
                         ExprGroupId & locationOfPointerToMe)
{
  CMPASSERT( this == locationOfPointerToMe );

  if (nodeIsTransformed())
    return;
  markAsTransformed();

  // ---------------------------------------------------------------------
  // Rewrite a Right Join as a Left Join
  // ---------------------------------------------------------------------
  if (getOperatorType() == REL_RIGHT_JOIN)
    {
      setOperatorType(REL_LEFT_JOIN);
      // switch the left and right subtrees
      RelExpr * temp = child(0).getPtr();
      child(0) = child(1).getPtr();
      child(1) = temp;
    }

  if (isInnerNonSemiJoin())
    {
      // -----------------------------------------------------------------
      // If not a SEMI or OUTER join then move the predicates in joinPred_
      // to the selection predicates.
      // -----------------------------------------------------------------
      selectionPred() += joinPred_;
      joinPred_.clear();
    }

  // before triggers need special handling for subqueries
  if (child(0)->getOperatorType() == REL_BEFORE_TRIGGER)
  {
	  normWARef.setInBeforeTrigger(TRUE);
  }
  // Make values available to the childs
  ValueIdSet availableValues = getGroupAttr()->getCharacteristicInputs();
  child(1)->getGroupAttr()->addCharacteristicInputs(availableValues);
  if (isTSJForMergeUpsert())
  {
    ValueIdSet subqVids;
    for (ValueId vid = availableValues.init();
	 availableValues.next(vid); availableValues.advance(vid)) {
      if (vid.getItemExpr()->getOperatorType() == ITM_ROW_SUBQUERY)
	subqVids.insert(vid);
    }
    availableValues -= subqVids;
    //remove subqueries
    
  }
    child(0)->getGroupAttr()->addCharacteristicInputs(availableValues);


  // ---------------------------------------------------------------------
  // Allocate a new VegRegion for the left subtree for
  // full outer join. 
  // This is need so as to convert the Full Outer to 
  // (a) Left Join - if there is a selection predicate on a column that
  //                  suffers from null-instantiation and that column is
  //                  part of the join column and that column is covered by
  //                  left subtree.
  // (b) Right Join - if there is a selection predicate on a column that
  //                  suffers from null-instantiation and that column is
  //                  part of the join column and that column is covered by
  //                  right subtree.
  // (c) Inner Join - if both (a) and (b) is true. That is there is a predicate
  //                  that satisfies (a) and there is predicate that 
  //                  statisfies (b).
  // ---------------------------------------------------------------------
  if (isFullOuterJoin())
    normWARef.allocateAndSetVEGRegion(IMPORT_AND_EXPORT,
				      this, // owner
				      0     // first child
				      );
  
  
  // ---------------------------------------------------------------------
  // Transform the left child.
  // Put any semijoins between the child and myself
  // ---------------------------------------------------------------------
  child(0)->transformNode(normWARef, child(0));

  // Return to my own VEGRegion.
  if (isFullOuterJoin())
    normWARef.restoreOriginalVEGRegion();
  
  // ---------------------------------------------------------------------
  // Initialize a new VEGRegion when entering the right subtree of a 
  // Left Join. The new VEGRegion should be capable of importing
  // any outer references and exporting any value that suffers null
  // instantiation.
  // We don't really need the right subtree to be in a different VEGRegion
  // from the left subtree since it cannnot reference any values produced
  // from there anyway. But the on clause needs to be in a different VEGRegion
  // and since the on clause may introduce semiJoin's in the right subtree
  // it is more convenient to put both the subtree and the ON clause in the
  // same VEGRegion.
  // ---------------------------------------------------------------------
  if (isLeftJoin() OR isFullOuterJoin() OR isAntiSemiJoin())
    {
      // Create a new VEGRegion for the right child for full outer Join.
      if (isFullOuterJoin())
	normWARef.allocateAndSetVEGRegion(IMPORT_AND_EXPORT,
					  this, // owner
					  1     //second child
					  );
      else
	normWARef.allocateAndSetVEGRegion(IMPORT_AND_EXPORT,
					  this);  // default to first child.
    }


  // ---------------------------------------------------------------------
  // Transform the right child.
  // Put any semijoins between the child and myself
  // ---------------------------------------------------------------------
  child(1)->transformNode(normWARef, child(1)); 


  if (isFullOuterJoin())
    normWARef.restoreOriginalVEGRegion();

  // done transforming before triggers subqueries
  if (normWARef.isInBeforeTrigger())
  { 
     normWARef.setInBeforeTrigger(FALSE);
  }

  // ---------------------------------------------------------------------
  // If there is a joinPred transform them. Put any new Joins between this
  // join and my current transformed child. The predicates may reference
  // values from the left child.
  // ---------------------------------------------------------------------

  // Create a new VEGRegion for the Full Outer Region.
  // The Join Predicate will reside in this VEGRegion.
  // The selection predicate will remain in the parent's
  // VEGRegion.
  if (isFullOuterJoin())
    normWARef.allocateAndSetVEGRegion(IMPORT_ONLY,
				      this, // owner
				      2     // third child
				      );
  

  // TBD - Hema.
  // Disallow subqueries in Join Predicate in Full Outer Join.


  const NABoolean movePredicates = TRUE;

  ValueIdSet externalInputs(getGroupAttr()->getCharacteristicInputs());
  externalInputs += child(0)->getGroupAttr()->getCharacteristicOutputs();

  if (joinPred().transformNode(normWARef,child(1),
                               externalInputs,movePredicates ))
    {
      // Transform the new right child
      child(1)->transformNode(normWARef, child(1));

      // -----------------------------------------------------------------
      // The transformed subquery predicate required values that are
      // produced by the semiJoin who now is my right child
      // The transformed predicates was moved there and the required
      // inputs for my child are now correct (sufficient and minimal).
      // -----------------------------------------------------------------

      // Check to see if we need to turn this into a TSJ.
      ValueIdSet neededInputs;
      neededInputs = child(1).getPtr()->getGroupAttr()->getCharacteristicInputs();
      neededInputs -= getGroupAttr()->getCharacteristicInputs();

      ValueIdSet crossReferences;
      crossReferences = child(0)->getGroupAttr()->getCharacteristicOutputs();

      // --------------------------------------------------------------------
      // At this point of transformation, the vid's in the different parts
      // of the query tree might be inconsistent due to replacement expr
      // being set. This will be corrected during normalization. Here, we
      // need to explicitly compared the vid's of the replacement expr's.
      // --------------------------------------------------------------------
      ValueIdSet neededInputs2; 
      ValueId vid;
      for (vid = neededInputs.init();
           neededInputs.next(vid);
           neededInputs.advance(vid))
             neededInputs2.insert(vid.getItemExpr()->
                                    getReplacementExpr()->getValueId());

      ValueIdSet crossReferences2;
      for (vid = crossReferences.init();
           crossReferences.next(vid);
           crossReferences.advance(vid))
             crossReferences2.insert(vid.getItemExpr()->
                                       getReplacementExpr()->getValueId());

      crossReferences2.intersectSet(neededInputs2);

      // If the right child needs values from the left child, turn this
      // Join into a TSJ
      if(NOT crossReferences2.isEmpty() && NOT isTSJ())
        {
          convertToTsj();

          // After we transform the right child and we pullup predicates
          // we may turn back to a non TSJ if we were able to pull-up
          // all those predicates that needed values from the left child.
        }

      // Verify that we can produce every value the right child needs
      neededInputs2 -= crossReferences2;
      // neededInputs is now what the right child needs and is
      // neither an input to this join nor an output of the left child
      CMPASSERT(neededInputs2.isEmpty());
    }

  // ---------------------------------------------------------------------
  // Restore the original VEGRegion.
  // ---------------------------------------------------------------------
  if (isLeftJoin() OR isFullOuterJoin() OR isAntiSemiJoin())
    normWARef.restoreOriginalVEGRegion();

#if 0
  // ---------------------------------------------------------------------
  // Try to create "singleton" VEG with an null-inst value to emulate
  // what's happening with base columns and index columns. This might not
  // really be necessary and is therefore commented out for now pending
  // a more detailed study.
  // ---------------------------------------------------------------------
  // Go through null-instantiated outputs and add columns to VEG.
  if (isLeftJoin())
  {
    for (CollIndex x = 0; x < nullInstantiatedOutput().entries(); x++)
    {
      ValueId vid = nullInstantiatedOutput().at(x);
      normWARef.addVEG(vid,vid);
    }
  }
#endif
         
  DBGSETDBG( "TRANSFORM_DEBUG" )
  DBGIF(
    unp = "";
    selectionPred().unparse(unp);
    cerr << "Join selpred: " << unp << endl;
  )

  // Pull up the predicates and recompute the required inputs
  // of whoever my children are now.
  pullUpPreds();

  DBGIF(
    unp = "";
    selectionPred().unparse(unp);
    cerr << "Join selpred: " << unp << endl;
  )

  if (CmpCommon::getDefault(NOT_IN_OPTIMIZATION) == DF_ON)
  {
    if (isAntiSemiJoin())
    {
      // if there are NotIn(A,B) predicate try transforming it to A=B if possible
      ValueIdSet origSet;
      ValueIdSet newSet;
      rewriteNotInPredicate(origSet, newSet);
    
      if (newSet.entries()>0)
      {
	normWARef.locateAndSetVEGRegion(this);

        newSet.transformNode(normWARef,child(1),
                               externalInputs,movePredicates );
	
	normWARef.restoreOriginalVEGRegion();
	
	joinPred() -= origSet;
	joinPred() += newSet;
      }

    }

  }
   
  // ---------------------------------------------------------------------
  // Convert a tsj to a join when a value that is produced by the left
  // subtree is not referenced in the right subtree.
  // If the tsj right child contain triggers - don't convert to join.
  //   Triggers need to be activated for each left row even if they don't 
  //   reference the left subtree data.
  // 
  // For RoutineJoins/Udfs we also want to convert it to a join if the UDF
  // does not need any inputs from the left and if the routine is
  // deterministic.
  // ---------------------------------------------------------------------
  if (isTSJ())
    {
      /*--- old code (see comment below why it's replaced)
      ValueIdSet outerRefs =
           child(0)->getGroupAttr()->getCharacteristicOutputs();
      outerRefs.intersectSet
          (child(1)->getGroupAttr()->getCharacteristicInputs());
      ---*/

      // Check to see if we need to turn this into a TSJ.
      ValueIdSet neededInputs;
      neededInputs = child(1).getPtr()->getGroupAttr()->getCharacteristicInputs();
      // is this ok? Our set of char. inputs may not yet be minimal,
      // and could contain char. outputs from the left child.
      neededInputs -= getGroupAttr()->getCharacteristicInputs();

      ValueIdSet crossReferences;
      crossReferences = child(0)->getGroupAttr()->getCharacteristicOutputs();

      // --------------------------------------------------------------------
      // At this point of transformation, the vid's in the different parts
      // of the query tree might be inconsistent due to replacement expr
      // being set. This will be corrected during normalization. Here, we
      // need to explicitly compared the vid's of the replacement expr's.
      // --------------------------------------------------------------------
      ValueIdSet neededInputs2;
      ValueId vid;
      for (vid = neededInputs.init();
           neededInputs.next(vid);
           neededInputs.advance(vid))
             neededInputs2.insert(vid.getItemExpr()->
                                    getReplacementExpr()->getValueId());

      ValueIdSet crossReferences2;
      for (vid = crossReferences.init();
           crossReferences.next(vid);
           crossReferences.advance(vid))
             crossReferences2.insert(vid.getItemExpr()->
                                       getReplacementExpr()->getValueId());

      crossReferences2.intersectSet(neededInputs2);

      // The above logic looks at intersection of ValueIdSet of outputs
      // of child(0) and inputs of child(1) to decide if a join is a TSJ.
      // Sometimes, an expression (such as a base column of child(0))
      // may have two distinct valueids depending on where it is appearing;
      // i.e., it may have v1 as valueid on left side (child(0)) and 
      // v2 on right side
      // (child(1)). In these cases, the simple intersection test is not 
      // complete. So, we look at physical index columns and names
      // to deduce that the two valueids are infact same or not.

      if (crossReferences2.isEmpty())
         {
         ValueIdSet eis; // for storing equivalent index set
         for (vid = crossReferences.init();
                  crossReferences.next(vid);
                  crossReferences.advance(vid))
            {
             if (vid.getItemExpr()->getOperatorType() != ITM_BASECOLUMN)
                continue;

             eis += ((BaseColumn *)vid.getItemExpr())->getEIC();
            } // for populate eis set

         ValueId vid1;
         ValueIdSet eis1; // for storing equivalent index set

         for (vid1 = neededInputs.init();
                 neededInputs.next(vid1);
                 neededInputs.advance(vid1))
            {
             if (vid1.getItemExpr()->getOperatorType() != ITM_BASECOLUMN)
                 continue;
           
             eis1+= ((BaseColumn *)vid1.getItemExpr())->getEIC();
            } // populate eis1 set

         // now compare physical fileset name and column names, to see
         // if any two columns from sets eis and eis1 are same
          
         ValueIdSet rightChildInputs = 
             child(1).getPtr()->getGroupAttr()->getCharacteristicInputs();

         for (vid = eis.init();
                eis.next(vid);
                eis.advance(vid))
             {
             if (vid.getItemExpr()->getOperatorType() != ITM_INDEXCOLUMN)
                 continue;

             for (vid1 = eis1.init();
                eis1.next(vid1);
                eis1.advance(vid1))
                {
                if (vid1.getItemExpr()->getOperatorType() != ITM_INDEXCOLUMN)
                   continue;

                 if ( doTwoVidsReferToSameColumn(vid, vid1) )
                    {
                    normWARef.addVEG(
                          ((IndexColumn *)vid.getItemExpr())->getDefinition(), 
                          ((IndexColumn *)vid1.getItemExpr())->getDefinition()
                        );

                    //-------------------------------------------------------
                    // Genesis Case: 10-000626-1151:
                    // This is a TSJ: if right child is asking for a valueid 
                    // that is in VEG as that of left child is producing, then
                    // right child may ask for what left child is producing
                    //--------------------------------------------------------

                    rightChildInputs -= 
                        ((IndexColumn *)vid1.getItemExpr())->getDefinition();

                    rightChildInputs +=
                          ((IndexColumn *)vid.getItemExpr())->getDefinition();
                         
                    crossReferences2.insert(vid);
                    }
                } // inner for 

             } // outer for
         
         child(1).getPtr()->getGroupAttr()->
                 setCharacteristicInputs(rightChildInputs);
         } // isEmpty()

         if (crossReferences2.isEmpty() && 
             !isTSJForWrite()           &&
             !getInliningInfo().isDrivingPipelinedActions() &&
             !getInliningInfo().isDrivingTempInsert() && // Triggers -
             !(isRoutineJoin() &&
               child(1).getGroupAttr()->getHasNonDeterministicUDRs()))
         {
           // Remember we used to be a RoutineJoin. This is used to determine
           // what type of contexts for partitioning we will try in OptPhysRel.
           if (isRoutineJoin())
             setDerivedFromRoutineJoin();
           convertToNotTsj();
         }
    
      else
         {
           // We have a TSJ that will be changed to Nested join 
           // safe to change NotIn here to non equi-predicate form (NE)
           // at this point only the case on single column NotIn can reach here
           // and the either the outer or inner column or both is nullable
           // and may have null values
           resolveSingleColNotInPredicate();
         }
    }
    transformSelectPred(normWARef, locationOfPointerToMe);

} // Join::transformNode()

// -----------------------------------------------------------------------
// Join::pullUpPreds()
// -----------------------------------------------------------------------
void Join::pullUpPreds()
{

  // We don't pull up predicateds for Full Outer Join (FOJ).
  // That is because we don't try to push them down during
  // normalization.
  // Just recomputeOuterReferences() on both children.
  if (getOperatorType() == REL_FULL_JOIN) 
    {
      child(0)->recomputeOuterReferences();
      child(1)->recomputeOuterReferences();
      return;
    }
  

  // ---------------------------------------------------------------------
  // Pull up predicates from each child.
  // Accumulate the predicates from each of my child subtrees.
  // ---------------------------------------------------------------------
  // Pull up the predicates from the left child
  // ---------------------------------------------------------------------
  selectionPred() += child(0)->getSelectionPred();
  child(0)->selectionPred().clear();
  child(0)->recomputeOuterReferences();

  // ---------------------------------------------------------------------
  // If outer/semi join then predicates from the right child go to
  // joinPred otherwise they go to the selectionPred.
  // ---------------------------------------------------------------------
  if (isInnerNonSemiJoin() || getOperatorType() == REL_TSJ_FLOW)
    {
       selectionPred() += child(1)->getSelectionPred();
    }
  else
    {
       joinPred() += child(1)->getSelectionPred();
    }

  child(1)->selectionPred().clear();
  child(1)->recomputeOuterReferences();

  //----------------------------------------------------------------------
  // if am a SemiJoin and any of my joinPred is covered by my inputs
  // and my first child output then move that to the selectionPed.
  //----------------------------------------------------------------------
  ValueIdSet predicatesToMove, boringSet, predicatesThatStay;
  if (isSemiJoin()) // anti-joins, left-joins shouldn't do this!
    {
      getGroupAttr()->coverTest(joinPred(),
                            child(0)->getGroupAttr()->getCharacteristicOutputs(),
                            predicatesToMove,
                            boringSet,
                            &predicatesThatStay);

      if (NOT predicatesToMove.isEmpty())
        {
          joinPred() -= predicatesToMove;
          selectionPred() += predicatesToMove;
        }
    }

} // Join::pullUpPreds()

// -----------------------------------------------------------------------
// Join::recomputeOuterReferences()
// -----------------------------------------------------------------------
void Join::recomputeOuterReferences()
{
  // ---------------------------------------------------------------------
  // Delete all those input values that are no longer referenced on
  // this operator because the predicates that reference them have 
  // been pulled up.
  // ---------------------------------------------------------------------
  if (NOT getGroupAttr()->getCharacteristicInputs().isEmpty())
    {
      ValueIdSet outerRefs = getGroupAttr()->getCharacteristicInputs();
 
      // Weed out those expressions not needed by my selectionPred and joinPred
      ValueIdSet exprSet = getSelectionPred();
      exprSet += joinPred();

      exprSet.insertList(nullInstantiatedOutput());
      exprSet.insertList(nullInstantiatedForRightJoinOutput());

      exprSet.weedOutUnreferenced(outerRefs);

      // Add back those expressiones needed by my left child
      outerRefs += child(0).getPtr()->getGroupAttr()->getCharacteristicInputs();

      // If it is a TSJ don't add the outputs of the left child to
      // the needed inputs.
      exprSet = child(1).getPtr()->getGroupAttr()->getCharacteristicInputs();
      if (isTSJForMergeUpsert()) 
      {
	ValueIdSet exprSet2; 
	ValueId vid;
	for (vid = exprSet.init();
	     exprSet.next(vid);
	     exprSet.advance(vid))
	  exprSet2.insert(vid.getItemExpr()->
			  getReplacementExpr()->getValueId());
	exprSet2.removeCoveredExprs(child(0).getPtr()->getGroupAttr()->getCharacteristicOutputs());
	outerRefs += exprSet2;
      }
      else 
      {
	if (isTSJ())
        {
          exprSet.removeCoveredExprs(child(0).getPtr()->getGroupAttr()->getCharacteristicOutputs());
        }
	outerRefs += exprSet;
      }

      getGroupAttr()->setCharacteristicInputs(outerRefs);
    }
} // Join::recomputeOuterReferences()  

// ----------------------------------------------------------------------
// Fix genesis case 10-061010-8731, solution 10-061010-9689 RFE in which
// queries like 
//   select ... from t1 
//   where t1.c not in (select t2.c from t2 where t2.c is not null ...)
//     and t1.c is not null ...
// is compiled into a horribly inefficient but correct plan like
//   nested_anti_semi_join(pa(t1), pa(t2))
// which generates a cross product of t1 with t2 and applies the predicate
//   not((t1.c <> t2.c) is true)
// A joke is that the Bank of America query reported in the case would have
// taken 5 years to run. A much better plan would be
//   hash_anti_semi_join(pa(t1), pa(t2))
// which generates a join of t1 with t2 and applies the predicate
//   t1.c = t2.c
// Using this plan, the Bank of America query completes in under 2 minutes.
// ----------------------------------------------------------------------
void Join::tryToRewriteJoinPredicate(NormWA & normWARef)
{
  // applies only to anti_semi_joins
  if (!isAntiSemiJoin()) {
    return; 
  }

  // look for "not((t1.c <> t2.c) is true)"
  for (ValueId exprId = joinPred().init(); 
       joinPred().next(exprId);
       joinPred().advance(exprId)) {
    ItemExpr *iePtr = exprId.getItemExpr();
    if (iePtr->getOperatorType() == ITM_NOT) {
    ItemExpr *grandkid, *kid = iePtr->child(0);
      if (kid && kid->getOperatorType() == ITM_IS_TRUE &&
        (grandkid=kid->child(0)) != NULL && 
        grandkid->getOperatorType() == ITM_NOT_EQUAL) {
      // look for conditions that can guarantee opds' non-nullability, eg,
      // look for "t1.c is not null && t2.c is not null" 
      ValueIdSet preds = joinPred(); // start with join predicate
      preds -= exprId; // remove "not((t1.c <> t2.c) is true)" from set
      preds += selectionPred(); // add any selection predicate
      if (preds.isNotNullable(grandkid->child(0)) &&
          preds.isNotNullable(grandkid->child(1))) {
#ifndef NDEBUG
        FILE *logF = NULL;
        NABoolean logRewrites = 
          CmpCommon::getDefault(COMP_BOOL_138) == DF_OFF &&
          CmpCommon::getDefault(COMP_BOOL_137) == DF_ON;
        if (logRewrites && 
            (logF = fopen("rewriteJoinPredicateLog.txt", "a")) != NULL) {
          preds.print(logF, "", "ASJ predicates:");
          iePtr->print(logF);
        }
#endif
        // both operands are guaranteed to be non-null. replace 
        // "not((t1.c<>t2.c) is true) && t1.c is not null && t2.c is not null"
        // with "t1.c=t2.c".
        ItemExpr *eqpred = new(normWARef.wHeap())
          BiRelat(ITM_EQUAL, grandkid->child(0).getPtr(),
                  grandkid->child(1).getPtr());
        ((BiRelat *)eqpred)->
          specialMultiValuePredicateTransformation() = TRUE;
        exprId.replaceItemExpr(eqpred);
        eqpred->synthTypeAndValueId(TRUE);
#ifndef NDEBUG
        if (logRewrites && logF) {
          exprId.getItemExpr()->print(logF);
          fclose(logF);
        }
#endif
        }
      }
    }
  }
}

// -----------------------------------------------------------------------
// Join::rewriteNode()
// -----------------------------------------------------------------------
void Join::rewriteNode(NormWA & normWARef)
{
  NABoolean isALeftJoin = isLeftJoin();
  NABoolean isAFullOuterJoin = isFullOuterJoin();
  NABoolean isASemiJoin = isSemiJoin();

  // ---------------------------------------------------------------------
  // Check if this is a Left Join.
  // ---------------------------------------------------------------------
  if (isALeftJoin && !isAFullOuterJoin)
    {
      if (canConvertLeftJoinToInnerJoin(normWARef))
        {
          // -------------------------------------------------------------
          // Convert the operator so that it is no longer an outer join.
          // -------------------------------------------------------------
          convertToNotOuterJoin();
          isALeftJoin = FALSE;      // no longer a LEFT JOIN
          // -------------------------------------------------------------
          // Combine all the predicates together.
          // -------------------------------------------------------------
          if (isASemiJoin || isAntiSemiJoin()) 
            {
              CMPASSERT (FALSE) ; // left joins can't be semi!
              joinPred() += getSelectionPred();
              selectionPred().clear();
            }
          else
            {
              selectionPred() += joinPred();
              joinPred().clear();
            }
        }
    }

  // Check if it's a full outer join
  if (isAFullOuterJoin)
    {
      tryToConvertFullOuterJoin(this, normWARef);
      // that means Full Outer Join has been converted.
      if (getOperatorType() != REL_FULL_JOIN) 
	isAFullOuterJoin = FALSE; // no longer a FULL OUTER
    }
  
  
  // try to rewrite join predicate from 
  //   not((t1.c <> t2.c) is true) and t1.c is not null and t2.c is not null
  // to
  //   t1.c = t2.c and t1.c is not null and t2.c is not null
  // -----------------------------------------------------------
  // When NOT_IN_OPTIMIZATION is ON we don't need to call 
  // tryToRewriteJoinPredicate method anymore. 
  // We may need to remove this call and the method in the future
  if (CmpCommon::getDefault(COMP_BOOL_138) == DF_OFF && 
      CmpCommon::getDefault(NOT_IN_OPTIMIZATION) == DF_OFF) {
    tryToRewriteJoinPredicate(normWARef);
  }
  
  // ---------------------------------------------------------------------
  // Rewrite the expressions of the left child.
  // ---------------------------------------------------------------------
  if (isAFullOuterJoin)
    normWARef.locateAndSetVEGRegion(this, 0 /* first child */);


  child(0)->rewriteNode(normWARef);

  // -----------------------------------------------------------------
  // Normalize the values that will be subject to null-instantiation
  // with values in the Child(0) region.
  // -----------------------------------------------------------------
  normalizeNullInstantiatedForRightJoinOutput(normWARef); 

  // -----------------------------------------------------------------
  // Restore the original VEGRegion.
  // -----------------------------------------------------------------
  if(isAFullOuterJoin)
    normWARef.restoreOriginalVEGRegion();

  // ---------------------------------------------------------------------
  // Rewrite the expressions of the right child.
  // ---------------------------------------------------------------------
  if (isALeftJoin OR isAFullOuterJoin OR isAntiSemiJoin())
    {
      // -----------------------------------------------------------------
      // Locate and set the VEGRegion for the ON clause.
      // This is done in order to rewrite "=" predicates in terms of 
      // the VEGs that are valid within its VEGRegion.
      // -----------------------------------------------------------------
      if (isAFullOuterJoin)
        normWARef.locateAndSetVEGRegion(this, 1 /* second child */);
      else
        normWARef.locateAndSetVEGRegion(this);

      child(1)->rewriteNode(normWARef);


      // -----------------------------------------------------------------
      // Normalize the values that will be subject to null-instantiation
      // with values in the Child(1) region.
      // -----------------------------------------------------------------
      normalizeNullInstantiatedOutput(normWARef);

      // -----------------------------------------------------------------
      // Restore the original VEGRegion.
      // -----------------------------------------------------------------
      if (isAFullOuterJoin)
        normWARef.restoreOriginalVEGRegion();
      
      // -----------------------------------------------------------------
      // Rewrite expressions in the ON clause predicate.
      // -----------------------------------------------------------------
      if (isAFullOuterJoin)
	normWARef.locateAndSetVEGRegion(this, 2 /* third child */);
      
      normWARef.setInJoinPredicate(TRUE) ;      
      if (joinPred().normalizeNode(normWARef))
                {
                }
      normWARef.setInJoinPredicate(FALSE) ;
      // -----------------------------------------------------------------
      // Restore the original VEGRegion.
      // -----------------------------------------------------------------
      normWARef.restoreOriginalVEGRegion();
    }                         // normalize the ON clause of the LEFT  Join
  else
    {                         // normalize the ON clause of the INNER Join
      child(1)->rewriteNode(normWARef);
      // -----------------------------------------------------------------
      // Rewrite expressions in the ON clause predicate.
      // -----------------------------------------------------------------

     if (joinPred().normalizeNode(normWARef))
        {
        }
    }                         // normalize the ON clause of the INNER Join
  // ---------------------------------------------------------------------
  // Rewrite expressions in the WHERE clause predicate tree.
  // ---------------------------------------------------------------------

  if (selectionPred().normalizeNode(normWARef))
    {
    }

  // ---------------------------------------------------------------------
  // Rewrite the ValueIdMap between the select and the update part so
  // it has VEGReferences init (note that we avoided VEGies that span
  // both the select and the update part, this is (probably?) one
  // reason why we only normalized one half of the keys preds above.
  // ---------------------------------------------------------------------
  if( getInliningInfo().isDrivingMvLogInsert()
      &&
      NULL != updateSelectValueIdMap_ )
  {
    updateSelectValueIdMap_->normalizeNode(normWARef);

    // If a VID in the bottom is of the form ValueIdUnion(x,x),
    // then replace it with x. This is necessary to push down UPDATEs
    // with MV attached tables into DP2.

    const ValueIdList& originalBottomValues =
            updateSelectValueIdMap_->getBottomValues();

    ValueIdList newBottomValues(originalBottomValues);

    for(CollIndex i = 0; i < originalBottomValues.entries(); i++) {
        ItemExpr* x  = originalBottomValues[i].getItemExpr();
        if (x && x->getOperatorType() == ITM_VALUEIDUNION &&
            ((ValueIdUnion*)x) -> getLeftSource() ==
            ((ValueIdUnion*)x) -> getRightSource()
           )
        {
          newBottomValues[i] = ((ValueIdUnion*)x)->getRightSource();
        } else
          newBottomValues[i] = originalBottomValues[i];
     }

     updateSelectValueIdMap_ = new (CmpCommon::statementHeap())
         ValueIdMap(updateSelectValueIdMap_->getTopValues(), newBottomValues);
  }

  // ---------------------------------------------------------------------
  // Rewrite expressions in the Group Attributes.
  // ---------------------------------------------------------------------
  
  if (isALeftJoin)
    normWARef.saveLeftJoinChildVEGRegion(this,0);
   
  ((ValueIdSet &)getGroupAttr()->getCharacteristicInputs()).normalizeNode(normWARef);

  if (isALeftJoin)
    normWARef.resetLeftJoinChildVEGRegion();

  ((ValueIdSet &)getGroupAttr()->getCharacteristicOutputs()).normalizeNode(normWARef);

// getGroupAttr()->normalizeInputsAndOutputs(normWARef);

} // Join::rewriteNode()

// -----------------------------------------------------------------------
// Join::canConvertLeftJoinToInnerJoin()
// Currently handles LEFT JOIN only. 
// -----------------------------------------------------------------------
NABoolean Join::canConvertLeftJoinToInnerJoin(NormWA & normWARef) 
{
  return normWARef.locateVEGRegionAndCheckIfMerged(this);
} // Join::canConvertLeftJoinToInnerJoin()

// -----------------------------------------------------------------------
// Join::normalizeNullInstantiatedOutput()
//
// A method for normalizing the operands of an InstantiateNull operator
// that appears in the nullInstantiatedOutput(). A special method is 
// necessary to prevent an InstantiateNull that appears in this list
// from being replaced with a VEGReference for the VEG to which it 
// belongs.
// -----------------------------------------------------------------------
void Join::normalizeNullInstantiatedOutput(NormWA & normWARef)
{
  ItemExpr * instNull;
  
  for (CollIndex index = 0; 
       index < nullInstantiatedOutput().entries(); index++)
    {
      instNull = nullInstantiatedOutput()[index].getItemExpr();
      CMPASSERT(instNull->getOperatorType() == ITM_INSTANTIATE_NULL);
      // Replace the existing child of the InstantiateNull with 
      // its normalized form.
      instNull->child(0) =  instNull->child(0)->normalizeNode(normWARef);
    } // endfor
 
  //++MV
  // Used for translating the required sort key to the right 
  // child sort key and backwards
  BuildRightChildMapForLeftJoin();
  //--MV

} // Join::normalizeNullInstantiatedOutput()

// -----------------------------------------------------------------------
// Join::normalizeNullInstantiatedForRightJoinOutput()
//
// A method for normalizing the operands of an InstantiateNull operator
// that appears in the nullInstantiatedForRightJoinOutput(). A 
// special method is necessary to prevent an InstantiateNull that 
// appears in this list from being replaced with a VEGReference for the 
// VEG to which it belongs.
// -----------------------------------------------------------------------
void Join::normalizeNullInstantiatedForRightJoinOutput(NormWA & normWARef)
{
  ItemExpr * instNull;
  
  for (CollIndex index = 0; 
       index < nullInstantiatedForRightJoinOutput().entries(); index++)
    {
      instNull = nullInstantiatedForRightJoinOutput()[index].getItemExpr();
      CMPASSERT(instNull->getOperatorType() == ITM_INSTANTIATE_NULL);
      // Replace the existing child of the InstantiateNull with 
      // its normalized form.
      instNull->child(0) =  instNull->child(0)->normalizeNode(normWARef);
    } // endfor
 
  //++MV
  // Used for translating the required sort key to the right 
  // child sort key and backwards
  BuildLeftChildMapForRightJoin();
  //--MV

} // Join::nullInstantiatedForRightJoinOutput()


// -----------------------------------------------------------------------
// Join::leftLinearizeJoinTree()
//
// A left-linear tree of Inner Joins is one in which no Inner Join 
// has another Inner Join as its right child. This method implements 
// a transformation rule that produces a left-linear tree of Inner Joins.
// It replaces, if possible, T1 IJ (T2 IJ T3) with a left-linear sequence 
// T1 IJ T2 IJ T3.
// 
// The figure below assumes that subtree L2 and subtree R2 do not have
// and Inner Join as the topmost node.
// Pattern before the transformation:
// 
//            Inner Join #1 : p1
//           /             \
// subtree L1 : p2          Inner Join #2 : p5 
//                         /             \
//               subtree L2 : p3          subtree R2 : p4
//
// NOTE: p1,p2,p3,p4,p5 are predicates
//  
// Left linear tree produced by this transformation:
//          
//                          Inner Join #2 : p1 & p5 (we attempt to push down these predicates, so they may end up in the children) 
//                         /              \
//            Inner Join #1                subtree R2 : p4
//           /             \
// subtree L1  : p2        subtree L2 : p3
//       
// -----------------------------------------------------------------------
Join * Join::leftLinearizeJoinTree(NormWA & normWARef,
                                   TransformationType transformationType)
{
  // Don't do this transformation if the user said they want the 
  // join order to be completely determined by the order the 
  // tables are specified in the query.
  if (CURRSTMT_OPTDEFAULTS->joinOrderByUser())
    return this;

  // Condition for applying this rule: 
  // I and my right child must both be an Inner Join.
  if ( (getOperatorType() != REL_JOIN) OR
       ( (child(1)->getOperatorType() != REL_JOIN) AND 
         (child(1)->getOperatorType() != REL_ROUTINE_JOIN) AND 
         (child(1)->getOperatorType() != REL_LEFT_JOIN) ) )
    return this;

  // R1 is my current right child
  Join * R1 = (Join *)(child(1).getPtr()); 

  // Left linearize R1
  R1->leftLinearizeJoinTree(normWARef, transformationType); 

  // Assign the left child of R1 to become my new right child
  child(1) = R1->child(0);

  // If we pulled anything up or R1 has a join predicate, we need to 
  // run recursive pushdown at the RelRoot to make sure we don't end up
  // with predicates on unions and TSJs. This will happen at the end 
  // of the SQO phase so we don't do any unnecessary tree walks.
  
  if ((!selectionPred().isEmpty() || !R1->joinPred().isEmpty() ) &&
      (R1->child(0)->getOperatorType() == REL_UNION || 
       R1->child(1)->getOperatorType() == REL_UNION))
    normWARef.setRequiresRecursivePushdown(TRUE);

  // Pull up predicates so that VEGPredicates and predicates that contain
  // VEGReferences can potentially be distributed more extensively amongst
  // my subtrees. 

  R1->selectionPred() +=  getSelectionPred();
  selectionPred().clear();
  
  // R1 inherits all the values that I received as inputs and should
  // produce all the values that I was producing as output.
  R1->getGroupAttr()->setCharacteristicInputs
                         (getGroupAttr()->getCharacteristicInputs());
  R1->getGroupAttr()->setCharacteristicOutputs
                         (getGroupAttr()->getCharacteristicOutputs());
  
  // Recompute my own Inputs and Outputs.
  primeGroupAttributes();
 
  // Temporarily set the left child after the rotation so that I could push
  // predicate down to it before starting left linearization again (due to
  // the new right child).
  //
  R1->child(0) =  this;

  ValueIdSet availableInputs; 
  availableInputs = R1->getGroupAttr()->getCharacteristicInputs();

  // If this method is being called during subquery unnesting
  // then logical properies need to be resynthesized and 
  // pushdown rules are slightly different
  if (transformationType == UNNESTING)
  {
    ValueIdSet outerReferences ;
    
    availableInputs.getOuterReferences(outerReferences);
    availableInputs -= outerReferences ;

    ValueIdSet nonPredExpr;
    if (R1->getOperatorType() == REL_ROUTINE_JOIN)
      nonPredExpr += R1->child(1)->getGroupAttr()->getCharacteristicInputs() ;

    R1->pushdownCoveredExprSQO(R1->getGroupAttr()->getCharacteristicOutputs(),
                               availableInputs, 
                               R1->selectionPred(),
                               nonPredExpr,
                               TRUE, // keepPredsNotCoveredByLeftChild
                               TRUE);  // keepPredsNotCoveredByRightChild

    R1->getGroupAttr()->clearLogProperties();
    getGroupAttr()->clearLogProperties();
    R1->synthLogProp();
  }
  else if (transformationType == SEMI_JOIN_TO_INNER_JOIN)
  {
    R1->pushdownCoveredExpr(R1->getGroupAttr()->getCharacteristicOutputs(),
                            availableInputs, 
                            R1->selectionPred());

    R1->getGroupAttr()->clearLogProperties();
    getGroupAttr()->clearLogProperties();
    R1->synthLogProp();
  }
  else 
  {
    // Pushdown predicates that were pulled up
    R1->pushdownCoveredExpr(R1->getGroupAttr()->getCharacteristicOutputs(),
                            availableInputs, 
                            R1->selectionPred());
  }

  // I must left-linearize myself once again because I have acquired a new
  // right child.
  //
  R1->child(0) = leftLinearizeJoinTree(normWARef, transformationType);
  return R1;  // the tree was indeed left linearized
  
} // Join::leftLinearizeJoinTree()

// -----------------------------------------------------------------------
// Join::normalizeNode()
// -----------------------------------------------------------------------
RelExpr * Join::normalizeNode(NormWA & normWARef)
{
  if (nodeIsNormalized())
    return this;
  markAsNormalized();
  NABoolean isATSJ = isTSJ();
  RelExpr * normalizedExpr = this;     // default return value

  //--------------------------------------------------------------------------------
  // Create filternode on top of grandchild of a subquery TSJ to prevent pushdown
  // of predicates. This is needed if the correlated subquery will be unnested.
  //--------------------------------------------------------------------------------

  if (candidateForSubqueryUnnest() &&  
	  (child(1)->getOperatorType() == REL_GROUPBY)) 
  {
    createAFilterGrandChildIfNeeded(normWARef);
  }


  // -----------------------------------------------------------------
  // Perform predicate pushdown.
  // -----------------------------------------------------------------
  
  pushdownCoveredExpr(getGroupAttr()->getCharacteristicOutputs(),
                                          getGroupAttr()->getCharacteristicInputs(),
                                          selectionPred());

  
  if (CmpCommon::getDefault(NOT_IN_OUTER_OPTIMIZATION) == DF_ON)
  {
    //rewrite notin predicate
    rewriteNotInPredicate();
  }

  // -----------------------------------------------------------------
  // Normalize the left subtrees. Store pointers to the
  // roots of the subtrees after normalization.
  // -----------------------------------------------------------------
  
  if (isFullOuterJoin())
    normWARef.locateAndSetVEGRegion(this, 0 /* first child */);

  child(0) = child(0)->normalizeNode(normWARef);

  if (isFullOuterJoin())
    normWARef.restoreOriginalVEGRegion();

  // -----------------------------------------------------------------
  // Normalize the right subtree in the proper VEGRegion
  // -----------------------------------------------------------------
  if (isLeftJoin() OR isAntiSemiJoin() OR isFullOuterJoin())
    {
      // -------------------------------------------------------------
      // Locate and set the VEGRegion for the right subtree.
      // -------------------------------------------------------------
      
      if (isFullOuterJoin())
	normWARef.locateAndSetVEGRegion(this, 1 /* second child */);
      else 
	normWARef.locateAndSetVEGRegion(this);

      child(1) = child(1)->normalizeNode(normWARef);

      normWARef.restoreOriginalVEGRegion();
    }
  else
    {
      child(1) = child(1)->normalizeNode(normWARef);
    }

  fixEssentialCharacteristicOutputs();

  // -----------------------------------------------------------------
  // Transform a bushy tree of inner joins or a subtree in which
  // a left join is the right child of an inner join into a 
  // left associative linear sequence of join. Note that TSJs are
  // not transformed.
  // -----------------------------------------------------------------
  normalizedExpr = leftLinearizeJoinTree(normWARef);

  // ---------------------------------------------------------------------
  // Convert a tsj to a join if the tsj is not for a write operation
  // and if a value that is produced by the left subtree is not
  // referenced in the right subtree, 
  // ---------------------------------------------------------------------
  if (isATSJ AND NOT isTSJForWrite() AND
      NOT child(1)->getGroupAttr()->
            getCharacteristicInputs().referencesOneValueFromTheSet
               (child(0)->getGroupAttr()->getCharacteristicOutputs())
      && !getInliningInfo().isDrivingPipelinedActions() 
      && !getInliningInfo().isDrivingTempInsert() // Triggers -
      && !(isRoutineJoin() &&
           child(1).getGroupAttr()->getHasNonDeterministicUDRs())
     )
  {
    // Remember we used to be a RoutineJoin. This is used to determine
    // what type of contexts for partitioning we will try in OptPhysRel.
    if (isRoutineJoin())
      setDerivedFromRoutineJoin();

    convertToNotTsj();
    // ---------------------------------------------------------------
    // Transform a bushy tree of inner joins or a subtree in which
    // a left join is the right child of an inner join into a 
    // left associative linear sequence of join.
    // ---------------------------------------------------------------
    normalizedExpr = leftLinearizeJoinTree(normWARef);
  }

  normWARef.setExtraHubVertex(normalizedExpr);
  return normalizedExpr;
  
} // Join::normalizeNode()

//--------------------------------------------------------------------------
// Join::createAFilterGrandChildIfNecessary()
// This filter node is created (if necessary) after transform but before 
// normalization. Therefore inputs are minimal but outputs are maximal. Any
// predicates with outerreferences will be as high up in the tree as possible.
//---------------------------------------------------------------------------
void Join::createAFilterGrandChildIfNeeded(NormWA & normWARef) 
{
  // caller has already verified that child(1) is a groupby
  CMPASSERT(child(1)->getOperatorType() == REL_GROUPBY) ;

  Filter *predFilterNode = NULL;
  NABoolean doNotUnnest = FALSE ;
  GroupByAgg * gbyNode = (GroupByAgg *) child(1)->castToRelExpr();
  RelExpr * oldRightGrandChild = child(1)->child(0)->castToRelExpr();
  NABoolean candidateForLeftJoin = candidateForSubqueryLeftJoinConversion();
  NABoolean nestedAggInSubQ = FALSE;
  GroupByAgg * subQGby = NULL ;

  if (oldRightGrandChild->getOperator().match(REL_GROUPBY))
  {
    subQGby = (GroupByAgg *) oldRightGrandChild ;
    oldRightGrandChild = oldRightGrandChild->child(0)->castToRelExpr();
    nestedAggInSubQ = TRUE ;
  }
  if (oldRightGrandChild->getOperator().match(REL_ANY_SEMIJOIN) ||   
      oldRightGrandChild->getOperator().match(REL_ANY_ANTI_SEMIJOIN) ||
      oldRightGrandChild->getOperator().match(REL_GROUPBY))
  {
    // we do not want to unnest queries that have a semijoin or a group by
    // as a child of the groupby.
    doNotUnnest = TRUE;
    if (CmpCommon::getDefault(SUBQUERY_UNNESTING) == DF_DEBUG)
    {
      *CmpCommon::diags() << DgSqlCode(2997)
                          << DgString1("Subquery was not unnested. Reason: Right grandchild of TSJ is a semijoin or has more than one group by");
    }
  }
  // -----------------------------------------------------------------------
  // Check to see if we have any Outer References in our selection predicate
  // If we do we want to create a Filter Node on top of ourselves to hold
  // the Outer Reference predicate.
  // ------------------------------------------------------------------------
   ValueIdSet outerReferences, nonLocalPreds;
   gbyNode->getGroupAttr()->getCharacteristicInputs().
                               getOuterReferences(outerReferences);

  // We found that for left joins, we don't want to pull up correlated 
  // predicates from the selection predicate if there is also correlated 
  // predicates in the join preidcate. This is a fix for solution 
  // 10-090206-8977. 
  if ( (doNotUnnest == FALSE) && 
        oldRightGrandChild->getOperator().match(REL_ANY_LEFT_JOIN))
  {
    Join *myself = (Join *) oldRightGrandChild;

    if (((Join *) oldRightGrandChild)->joinPred().
              getReferencedPredicates(outerReferences, nonLocalPreds))
    {
      doNotUnnest = TRUE;
      if (CmpCommon::getDefault(SUBQUERY_UNNESTING) == DF_DEBUG)
      {
        *CmpCommon::diags() << DgSqlCode(2997)
                            << DgString1("Subquery was not unnested. Reason: Filter child is leftJoin with outerreferences in joinPred ");
      }
    }
  }
  if (doNotUnnest == FALSE)
  {
    nonLocalPreds.clear();
    oldRightGrandChild->selectionPred().getReferencedPredicates
      (outerReferences, nonLocalPreds) ;
    if (nestedAggInSubQ)
      subQGby->selectionPred().getReferencedPredicates
	(outerReferences, nonLocalPreds);

    if (!nonLocalPreds.isEmpty())
    {
      // Right grandchild selection pred has outer references

      // Like the case for the joinpredicates above, we need to 
      // make sure we don't create a filter with aggregates in it.
      // The problem we run into is if the groupBy that produced the
      // aggregate gets moved above the join.
      if (candidateForLeftJoin || 
          oldRightGrandChild->getOperator().match(REL_ANY_LEFT_JOIN))
      {
        for ( ValueId filterVid = nonLocalPreds.init(); 
                          nonLocalPreds.next(filterVid) ;
                          nonLocalPreds.advance(filterVid)) 
        {
          // Check to see if the filter predicates contains any
          // aggregates, if so do not create filter
          if (filterVid.getItemExpr()->containsAnAggregate())
          {
            doNotUnnest = TRUE;
            if (CmpCommon::getDefault(SUBQUERY_UNNESTING) == DF_DEBUG)
            {
              *CmpCommon::diags() << DgSqlCode(2997)   
                                  << DgString1("Subquery was not unnested. Reason: Filter preds would have contained aggregates "); 
            }
          }
        }
      }

      if ((doNotUnnest == FALSE) && candidateForLeftJoin && 
          (CmpCommon::getDefault(SUBQUERY_UNNESTING_P2) != DF_INTERNAL) &&
          ((normWARef.getLeftJoinConversionCount() >= 2)||nestedAggInSubQ))
      {
        doNotUnnest = TRUE;
        // For phase 2 we only unnest 2 subqueries 
        // containing NonNullRejecting Predicates. Later we will ensure
	// that these 2 subqueries are not nested.

        if (CmpCommon::getDefault(SUBQUERY_UNNESTING) == DF_DEBUG) 
        {
          if (!nestedAggInSubQ)
            *CmpCommon::diags() << DgSqlCode(2997)
              << DgString1("Skipping unnesting of Subquery due to NonNullRejecting Predicates in more than two subqueries");
          else
             *CmpCommon::diags() << DgSqlCode(2997)
              << DgString1("Skipping unnesting of Subquery since we have both NonNullRejecting predicate and nested aggregate in subquery.");
        }

      }

      // create the filter node
      if (doNotUnnest == FALSE)
      {
        predFilterNode = new (CmpCommon::statementHeap()) 
                                    Filter(oldRightGrandChild);
        predFilterNode->selectionPred() += nonLocalPreds;
        oldRightGrandChild->selectionPred() -= nonLocalPreds;
	if (nestedAggInSubQ)
	{
	  subQGby->selectionPred() -= nonLocalPreds;
	  predFilterNode->getGroupAttr()->setCharacteristicInputs
            (subQGby->getGroupAttr()->getCharacteristicInputs());
	  subQGby->recomputeOuterReferences();
	}
	else
	{
	  predFilterNode->getGroupAttr()->setCharacteristicInputs
            (oldRightGrandChild->getGroupAttr()->getCharacteristicInputs());
	}
  
        oldRightGrandChild->recomputeOuterReferences();

        // If the nodes below us require the same outer references as inputs 
        // as before we don't want to do the unnesting
        if (oldRightGrandChild->getGroupAttr()->getCharacteristicInputs() ==
            predFilterNode->getGroupAttr()->getCharacteristicInputs())
        {
          // disassociate the oldGrandChild from the Filter
          predFilterNode->child(0) = (RelExpr *) NULL;

          // put the predicate back.
          oldRightGrandChild->selectionPred() += nonLocalPreds;

          // remember that we decided not to unnest.
          doNotUnnest = TRUE;

          if (CmpCommon::getDefault(SUBQUERY_UNNESTING) == DF_DEBUG)
              *CmpCommon::diags() << DgSqlCode(2997)
                << DgString1("Skipping unnesting of Subquery due to subtree below filter requires same outer references as filter");

        } 
        else 
        {

          // Recompute inputs/outputs
          oldRightGrandChild->primeGroupAttributes();
          predFilterNode->primeGroupAttributes();
          
          if (candidateForLeftJoin)
            normWARef.incrementLeftJoinConversionCount();
  
          if (nestedAggInSubQ)
	    gbyNode->child(0)->child(0) = predFilterNode;
	  else
	    gbyNode->child(0) = predFilterNode;
        }
      }
    }
    else 
    {
      // right grandchild has no outer refs in selection pred.
      // Look in the groupby node now

      if (gbyNode->selectionPred().getReferencedPredicates
                                    (outerReferences, nonLocalPreds) ||
          gbyNode->aggregateExpr().getReferencedPredicates
                                    (outerReferences, nonLocalPreds))
      {
        // we know group expr is empty as this is a scalar grby.
        // do nothing as we have something to unnest (i.e. do not set the doNotUnnest flag)
        // unless we need Phase2 and we have already matrked one level.
        if (candidateForLeftJoin && 
            (CmpCommon::getDefault(SUBQUERY_UNNESTING_P2) != DF_INTERNAL) &&
            (normWARef.getLeftJoinConversionCount() >= 1))
        {
          doNotUnnest = TRUE;
          // For phase 2 we only unnest 1 level of subqueries 
          // containing NonNullRejecting Predicates
          if (CmpCommon::getDefault(SUBQUERY_UNNESTING) == DF_DEBUG)
              *CmpCommon::diags() << DgSqlCode(2997)
                << DgString1("Skipping unnesting of Subquery due to NonNullRejecting Predicates in more than one subquery");
        }
      }
      else
      {
        // no outer ref in grandchild's selection pred and in grby's (child) selection pred or
        // aggregate expr.
        doNotUnnest = TRUE;
        if (CmpCommon::getDefault(SUBQUERY_UNNESTING) == DF_DEBUG)
          *CmpCommon::diags() << DgSqlCode(2997)
                              << DgString1("Subquery was not unnested. Reason: No Correlation found");
      }
    }
  }

  if (doNotUnnest)
  {
    setCandidateForSubqueryUnnest(FALSE);
    normWARef.decrementCorrelatedSubqCount();
  }
  return ;
}
// Join::createAFilterGrandChildIfNecessary()

 /* --------------------------------------------------------------------------
 Join::eliminateRedundantJoin()
 -----------------------------------------------------------------------------

 Performs one of the following transformations, if this node is suitably marked

  1) If predicates have been marked for removal
  Join {selection_pred : p1,p2,p3,...pn} ----> Join {selection_pred : p3,...pn}
  where p1 and p2 are equi join predicates that are known to be true due to a 
  foreign_key-unique_key relationship

  2) If the children of the join are marked for removal
         
	parent
          |                  parent
	Join                   |
	/   \       ------>    X
       X    Y   
    
    where the node Y has been marked for elimination by the synthLogPhase. Note that
    instead of node Y, node X may also be marked for elimination and a similar
    transformation is performed in that case too.

  3) If its a left join and has been markedForElimination by the normalize phase
      then

        parent
          |                  parent
      LeftJoin                 |
	/   \       ------>    X
       X    Y   

   Note that in this case, it is only possible to eliminate the right child. */

RelExpr* Join::eliminateRedundantJoin(NormWA &normWARef)
{
  if (getOperatorType() == REL_JOIN)
  {	
    RelExpr *result = NULL;
    GroupAttributes *ga = NULL;

    selectionPred() -= getPredicatesToBeRemoved();
    equiJoinPredicates_ -= getPredicatesToBeRemoved();
    clearPredicatesToBeRemoved();


    if ((child(1).getPtr())->markedForElimination())
    {
      result = child(0);
      ga = child(1)->getGroupAttr();
    }
    else if ((child(0).getPtr())->markedForElimination())
    {
      result = child(1);
      ga = child(0)->getGroupAttr();
    }

    if (result)
    {
      CMPASSERT(selectionPred().isEmpty() && joinPred().isEmpty());

      NABoolean found = FALSE;
      TableDesc *tabDesc = NULL;

      const ValueIdSet &constraints = ga->getConstraints();

      for (ValueId id = constraints.init();
              constraints.next(id) && NOT found;
              constraints.advance(id) )
      {
        if (id.getItemExpr()->getOperatorType() == ITM_COMP_REF_OPT_CONSTRAINT)
        {
          ComplementaryRefOptConstraint * compRIConstraint =
            (ComplementaryRefOptConstraint *) id.getItemExpr();
          if (compRIConstraint->getIsMatchedForElimination())
          {
            tabDesc = compRIConstraint->getTableDesc();
            found = TRUE;
          }
        }
      }
      
      CMPASSERT(found);

      const ValueIdList &allCols = tabDesc->getColumnList();
      for (CollIndex i = 0; i < allCols.entries(); i++)
      {
        ItemExpr *ie = allCols[i].getItemExpr();

        CMPASSERT(ie->getOperatorType() == ITM_BASECOLUMN)

        const ValueIdSet &eic = ((BaseColumn *)ie)->getEIC();

        normWARef.deleteVEGMember(((BaseColumn *)ie)->getValueId());

        for (ValueId eqVid = eic.init(); eic.next(eqVid); eic.advance(eqVid))
          normWARef.deleteVEGMember(eqVid);
      }
      
      return result;
    }

  }
  else if (markedForElimination() && (getOperatorType() == REL_LEFT_JOIN))
  {
    TableDescList tableDescs(CmpCommon::statementHeap());

    child(1)->getAllTableDescs(tableDescs);
    
    CMPASSERT(tableDescs.entries() != 0);

    normWARef.locateAndSetVEGRegion(this);

    for (CollIndex j = 0; j < tableDescs.entries(); j++) 
    {
      const ValueIdList &allCols = tableDescs[j]->getColumnList();
      for (CollIndex i = 0; i < allCols.entries(); i++)
      {
        ItemExpr *ie = allCols[i].getItemExpr();

        CMPASSERT(ie->getOperatorType() == ITM_BASECOLUMN)

        const ValueIdSet &eic = ((BaseColumn *)ie)->getEIC();

        normWARef.deleteVEGMember(((BaseColumn *)ie)->getValueId());

        for (ValueId eqVid = eic.init(); eic.next(eqVid); eic.advance(eqVid))
          normWARef.deleteVEGMember(eqVid);
      }
    }

    normWARef.restoreOriginalVEGRegion();

    return child(0) ; // outer joins
  }
  return this;
} // Join::eliminateRedundantJoin()

void RelExpr::getAllTableDescs(TableDescList &tableDescs)
{
  Int32 arity = getArity();
  if (arity == 0)
  {
    switch (getOperatorType())
    {
      case REL_SCAN:
        tableDescs.insert(((Scan *)this)->getTableDesc());
        break;
      case REL_STORED_PROC:
        tableDescs.insert(((TableValuedFunction *)this)->getTableDesc());
        break;
      default:
        break;
    }
  }
  else
  {
    for (Int32 i = 0; i < arity; i++) 
    {
      child(i)->getAllTableDescs(tableDescs);
    }
  }
}

/*-------------------------------------------------------------------------
Join::transformSemiJoin() 
---------------------------------------------------------------------------

This method transforms a semi join to an inner join.

a) In the simplest case, which is enabled by default the right child is
unique in the joining column and the semi join can be simply translated
into a join. An example query is 

	select t1.a
	from t1
	where t1.b in (select t2.a
		       from t2) ;
Here t2.a is a unique key of table t2.

The following transformation is made
         Semi Join {pred : t1.b = t2.a}          Join {pred : t1.b = t2.a} 
        /         \                   ------->  /    \
       /           \                           /      \
 Scan t1        Scan t2                   Scan t1     Scan t2
                                                

						
b) If the right child is not unique in the joining column then 
we transform the semijoin into an inner join followed by a groupby
as the join's right child. This transformation is enabled by default
only if the right side is an IN list or if the groupby's reduction 
ratio is greater than 5.0, otherwise a CQD has to be used.

Examples:

select t1.a
from t1
where t1.b in (1,2,3,4,...,101) ;


  Semi Join {pred : t1.b = InList.col}  Join {pred : t1.b = InList.col}
 /         \                   ------->  /    \
/           \                           /      \
Scan t1   TupleList                 Scan t1   GroupBy {group cols: InList.col}
                                                  |
                                                  |
                                                TupleList

select t1.a
from t1
where t1.b in (select t2.c from t2 where whatever) ;


  Semi Join {pred : t1.b = t2.c }       Join {pred : t1.b = t2.c}
 /         \                   ------->  /    \
/           \                           /      \
Scan t1   Scan t2                   Scan t1   GroupBy {group cols: t2.c}
                                                  |
                                                  |
                                                Scan t2

*/

RelExpr* Join::transformSemiJoin(NormWA& normWARef) 
{
  // SQO is called in a loop sometimes. 
  // We do not wish to apply this transformation more than once.
      setCandidateForSemiJoinTransform(FALSE);

  // If there are no equijoins or if there is some correlation,  
  // this transformation cannot be applied.
      if ((getOperatorType() == REL_SEMITSJ) ||
	  getEquiJoinPredicates().isEmpty())
      {
	return this ;
      }

  // apply the transformation described in item a) above
      ValueIdSet equiJoinCols1  = getEquiJoinExprFromChild1();
      if ((NOT equiJoinCols1.isEmpty()) && 
	   child(1)->getGroupAttr()->isUnique(equiJoinCols1))
	{
	  RelExpr * linearizedExpr = this ;
	  // in this case no additional groupBy is necessary, 
	  // simply changing semijoin --> join 
	  // will suffice.
	  setOperatorType(REL_JOIN) ; 
	  // move prds from joinPred to selection pred.
	  selectionPred() += joinPred();
	  joinPred().clear() ;
	  linearizedExpr = leftLinearizeJoinTree(normWARef, 
                                                 SEMI_JOIN_TO_INNER_JOIN);
	  return linearizedExpr ;
	}

 /* Apply the transformation described in item b) above.
   The transformation below is done if there are no non-equijoin preds either 
  and the inner side has no base tables (i.e. is an IN LIST) OR if the groupby
  is expected to provide a reduction > SEMIJOIN_TO_INNERJOIN_REDUCTION_RATIO
  (default is 5.0) OR the inner row count is small OR if we have used a CQD to 
  turn this transformation on. Some rationale: A data reduction might reduce
  the amount of data for the inner table of a hash join (or it might not!
  hash-semi-join sometimes does duplicate elimination itself, but not always).
  Converting to a join allows the join to be commuted; if the number of rows
  is small, nested join might be profitably chosen in that case. */

      ValueIdSet preds ;
      preds += joinPred();
      preds += selectionPred();
      preds -= getEquiJoinPredicates() ;

      EstLogPropSharedPtr innerEstLogProp = child(1)->getGroupAttr()->outputLogProp((*GLOBAL_EMPTY_INPUT_LOGPROP));
      CostScalar innerRowCount = innerEstLogProp->getResultCardinality(); 
      CostScalar innerUec = innerEstLogProp->getAggregateUec(equiJoinCols1);
      NABoolean haveSignificantReduction = FALSE;
      CostScalar reductionThreshold = 
        ((ActiveSchemaDB()->getDefaults()).getAsDouble(SEMIJOIN_TO_INNERJOIN_REDUCTION_RATIO));
      NABoolean noInnerStats = innerEstLogProp->getColStats().containsAtLeastOneFake();
      // have a valid value of uec, have something other than default 
      // cardinality and satisfy reduction requirement.
      if ((innerUec > 0) && (!noInnerStats) && 
          (innerRowCount/innerUec > reductionThreshold))
        haveSignificantReduction = TRUE;
      CostScalar innerAllowance =
        ((ActiveSchemaDB()->getDefaults()).getAsDouble(SEMIJOIN_TO_INNERJOIN_INNER_ALLOWANCE));
      NABoolean haveSmallInner = FALSE;
      if ((innerRowCount < innerAllowance) && (!noInnerStats))
        haveSmallInner = TRUE;

      if (preds.isEmpty() && 
	  ((child(1)->getGroupAttr()->getNumBaseTables() == 0) ||
           haveSignificantReduction ||
           haveSmallInner ||
	    (CmpCommon::getDefault(SEMIJOIN_TO_INNERJOIN_TRANSFORMATION) == DF_ON)))
  {                     
    CollHeap *stmtHeap = CmpCommon::statementHeap() ;
	
	setOperatorType(REL_JOIN) ;  
	// we need a group by below the transformed join
	GroupByAgg *newGrby = new (stmtHeap) GroupByAgg(
				child(1)->castToRelExpr()) ;
	newGrby->setGroupAttr(new (stmtHeap) 
	  GroupAttributes(*(child(1)->getGroupAttr())));
	// must reset numJoinedTables_; we might be copying GroupAttributes from a join
	newGrby->getGroupAttr()->resetNumJoinedTables(1);
	newGrby->getGroupAttr()->clearLogProperties();
	  
	newGrby->setGroupExpr(equiJoinCols1);
	child(1) = newGrby ;
	newGrby->synthLogProp(&normWARef); 

        // move preds from joinPred to selection pred.
	selectionPred() += joinPred();
	joinPred().clear() ;

	 //synthesize logical props for the new nodes.
	return this ;
      }
      return this ;  // semijoin has non-equijoin predicates or this
                     // transformation is OFF
} // Join::transformSemiJoin() 

// -----------------------------------------------------------------------
// copyNode()
// This method creates a copy of the original RelExpr.
// Sideffects: no change to the old Node.
//             newNode will have new a new groupAttrib structure allocated
//             and initialized with the information from the old one.
//             Similarly the newNode will initialize its RETDesc to the
//             same as the oldNode.
// -----------------------------------------------------------------------
static RelExpr * copyNode(RelExpr* oldNode, CollHeap* heap)
{
  RelExpr* newNode = oldNode->copyTopNode(NULL, heap);
  newNode->setGroupAttr(new (heap) 
    GroupAttributes(*(oldNode->getGroupAttr())));
  newNode->setRETDesc(oldNode->getRETDesc());
  newNode->getGroupAttr()->setLogExprForSynthesis(newNode);
  return newNode;
} // copyNode()

// -----------------------------------------------------------------------
// copyNodeAndSetChildren()
// This method creates a copy of the original RelExpr and also initializes
// the copy's children to be identical to that of the original.
// Sideffects: no change to the old Node.
//             see sideffects of copyNode.
//             newNode will have its children initialized to the same
//             as that of the original node.
// -----------------------------------------------------------------------
static RelExpr * copyNodeAndSetChildren(RelExpr* oldNode, CollHeap* heap)
{
  RelExpr* newNode = copyNode(oldNode,heap);
  for(Int32 i = 0; i < oldNode->getArity(); i++)
  {
    newNode->child(i) = oldNode->child(i) ;
  }
  return newNode;
} // copyNodeAndSetChildren()

// -----------------------------------------------------------------------
// Join::pullUpPredsWithAggrs()
//
// For certain PullUpGroupBy and all MoveUpGroupby transformations 
// a GroupBy node moves over a Join node (i.e. the GroupBy which 
// used to be a child of the Join, now becomes its parent). For
// such a tree transformation to work, any predicate in the Join
// that references aggregates in the GroupBy must now be moved into
// the GroupBy node. This method performs this task.
//
// 
// Sideffects: Will move join selection predicates that contains 
//             aggregates into the grbyNode.
//
// Returns: TRUE:   if we can move the aggregates from the join's selection
//                  predicate to the groupBy's selection predicate.
//          FALSE:  If the join contains join predicates and the join 
//                  predicates contains aggregates expressions from the groupBy.
// -----------------------------------------------------------------------
NABoolean Join::pullUpPredsWithAggrs(GroupByAgg* grbyNode, MapValueIds * mapNode)
{

  // We need to check Left Joins too, but we cannot pull any predicates
  // up from the join preds, so if we find aggregates in the join preds
  // indicate a failure so we do not put the groupBy on top of the join.
  if (NOT joinPred().isEmpty()) 
  {
    ValueIdSet predicatesThatNeedsToBePulled;
    if (joinPred().getReferencedPredicates
        (grbyNode->aggregateExpr(), predicatesThatNeedsToBePulled))
    {

        // Skip such this subquery .
       if (CmpCommon::getDefault(SUBQUERY_UNNESTING) == DF_DEBUG)
       {
          *CmpCommon::diags() << DgSqlCode(2997)   
               << DgString1("Subquery was not unnested. Reason: Join has aggregates in its predicates."); 
       }
      
       return FALSE;
    }
  }

  if (NOT selectionPred().isEmpty())
  {

    if (mapNode == NULL) 
    {
       ValueIdSet predicatesToPullUp;
       if (selectionPred().getReferencedPredicates
           (grbyNode->aggregateExpr(), predicatesToPullUp))
         {
            selectionPred() -= predicatesToPullUp ;
            grbyNode->selectionPred() += predicatesToPullUp ;
         }
    }
    else
    {
       ValueIdMap *copyOfMap = new (CmpCommon::statementHeap()) 
                                ValueIdMap(mapNode->getMap());

       for (ValueId vid = selectionPred().init();
                          selectionPred().next(vid);
                          selectionPred().advance(vid))
       {
          ValueId bottomMapId;

          copyOfMap->rewriteValueIdDown(vid, bottomMapId);


          // Only if our outputs will actually be different, do we want to
          // create a map.
          if ( vid != bottomMapId ) 
          {
             ValueId ignoreVid;
             ValueIdSet mapPullUpPred( bottomMapId);
             ValueIdSet mapPredicatesToPullUp;

             if (mapPullUpPred.getReferencedPredicates
                 (grbyNode->aggregateExpr(), mapPredicatesToPullUp))
               {
                  
                  selectionPred() -=  vid;
                  grbyNode->selectionPred() += mapPredicatesToPullUp ;
                  if (getGroupAttr()->getCharacteristicOutputs().
                     referencesTheGivenValue(vid, ignoreVid, FALSE,FALSE))
                  {
                     // Need to add a Map Entry in the MapNode
                     mapNode->addMapEntry(vid, bottomMapId);
                  }
               }
          }
       }
       
    }
  }
  return TRUE;
}  // Join::pullUpPredsWithAggrs()

// -----------------------------------------------------------------------
// GroupByAgg::computeGroupExpr()
//
// The group expression for a pulledUp or movedUp GroupBy node
// is computed from a seed ValueIdSet and a superSet ValueIdSet.
// The seed valueId set is that starting set of values that need
// to be in the groupExpr. For the pullUpGroupBy transformation this
// is the set of uniqueCols from the left child. The superSet is then 
// used to add more values to the groupExpr. Items in the output or
// having clause that are referenced by items in the superset are also
// added to the groupExpr.
//
// Sideffects: Changes the groupBy's group expression
// -----------------------------------------------------------------------
void GroupByAgg::computeGroupExpr(const ValueIdSet& seed, 
                                  ValueIdSet& superSet,
                                  NormWA& normWARef)
{
  ValueIdSet duplicates;
  ValueIdSet reqGrpValues = seed ;
  reqGrpValues += leftUniqueExpr() ;
  reqGrpValues.accumulateReferencedValues(
  superSet, selectionPred());
  reqGrpValues.accumulateReferencedValues( 
  superSet, getGroupAttr()->getCharacteristicOutputs());

  // Need to make sure we retain original groupExpr() for 
  // cases where we have a semijoin->GroupBy->Filter
  // In this case the group Expression will not be empty 
  // initially like it is for ScalarAggs, and so we have to
  // make sure we keep it. However due to moveUpGroupByTransformation
  // we have to make sure we remove duplicates..
  duplicates = reqGrpValues.intersect(groupExpr());
  if (duplicates.isEmpty())
     reqGrpValues += groupExpr();
  else
     reqGrpValues -= duplicates;
  addGroupExpr(reqGrpValues);
  groupExpr().normalizeNode(normWARef) ;
}  // GroupByAgg::computeGroupExpr()


/*-----------------------------------------------------------------------
Join::pullUpGroupByTransformation()

// The PullUpGroupBy transformation  is one of the two main transformations
// aplied while unnesting a subquery. For a single level subquery this is 
// the only transformation required for subquery unnesting. 
// X and Y denote arbitatry RelExprs.
// The TSJ has to be one introduced while flattening out a subquery
// in the Transform phase. Under some circumstance the TSJ can be transformed
// into a Join by the time it gets to this method. The Filter node is 
// introduced during Normalization to prevent pushdown of predicates with 
// outerReferences
// 
//      TSJ                            GroupBy {pred3}(grouping cols:  
//     /   \                              |    cluster_key of X (leftUniqueCols)+ 
//    /     \                             |    other necessary columns of X)
//   X      ScalarAgg {pred3}   -->      Join {pred2}
//             |                        /    \
//             |                       /      \
//          Filter {pred2}            X        Y {pred1}
//             |
//             |
//             Y {pred1}
//
//The same tree as above but in terms of the local variables used in the code below
//
//      this                            newGrby {pred3}(grouping cols:  
//     /   \                              |    cluster_key of newLeftChild (leftUniqueCols)+ 
//    /     \                             |    other necessary columns of newLeftChild)
//oldLeft   oldGB {pred3}
//Child                   -->          newJoin {pred2}
//             |                        /    \
//             |                       /      \
//          Filter {pred2}            newLeft  newRight {pred1}
//             |                      Child    Child
//             |
//     oldGBGrandChild {pred1}
//
//
// Expects:   RelExpr tree as seen above to the left.
// Sideffects: if successful, returns a new groupBy with the 
//             a copy of join as the child. The original tree has not changed.
//             The predicates in the new groupBy and the new Join will have
//             changed according to the comments above.
//
// If there is an explicit groupby in the subquery the transformation above is extended as
//      TSJ                            GroupBy {pred3,agg2(agg1)}(grouping cols:  
//     /   \                              |    cluster_key of X (leftUniqueCols)+ 
//    /     \                             |    other necessary columns of X)
//   X      ScalarAgg {pred3}   -->   SubQ_GroupBy {agg1} (grouping cols: g1 + 
//             | {agg2(agg1)}             |    cluster_key of X (leftUniqueCols) +   
//             |                          |    other necessary columns of X) 
//          SubQ_GroupBy {agg1}        newJoin {pred2}
//             | {grouping cols: g1}    /    \
//             |                       /      \
//          Filter {pred2}        newLeft  newRight {pred1}
//             |                  Child    Child
//             |
//             Y {pred1}
//
// If there is an explicy groupby in the subquery then the flag nestedAggInSubQ is set.

------------------------------------------------------------------------------*/
GroupByAgg* Join::pullUpGroupByTransformation(NormWA& normWARef)
{
  CollHeap *stmtHeap = CmpCommon::statementHeap() ;

   RelExpr *oldGB = child(1)->castToRelExpr();
  // note that typically child of oldGB is actually a Filter node, here 
  // oldGBgrandchild is the child of oldGB before the Filter was added.
  RelExpr *oldGBgrandchild ;
  NABoolean nestedAggInSubQ = FALSE;
  
  if ((oldGB->child(0)->getOperatorType() == REL_GROUPBY) &&
    (oldGB->child(0)->child(0)->getOperatorType() == REL_FILTER))
  {
    oldGBgrandchild = oldGB->child(0)->child(0)->child(0)->castToRelExpr();
    nestedAggInSubQ = TRUE;
  }
  else if (oldGB->child(0)->getOperatorType() == REL_FILTER)
    oldGBgrandchild = oldGB->child(0)->child(0)->castToRelExpr();
  else
    oldGBgrandchild = oldGB->child(0)->castToRelExpr();

  RelExpr *filterParent = nestedAggInSubQ ? 
    oldGB->child(0)->castToRelExpr() : oldGB;

  RelExpr *oldLeftChild = child(0)->castToRelExpr();

  // Determine a set of unique columns for the left sub-tree.

  // Note: Scans and joins synthesize uniqueness constraints even for
  // columns that are not in the characteristic outputs. Other
  // operators such as groupby or union don't. We make use of these
  // extra uniqeness constraints here. Any needed columns not yet
  // added to the characteristic outputs will be added later, in
  // method getMoreOutputsIfPossible().

  ValueIdSet leftUniqueCols ;
  if (NOT (child(0)->getGroupAttr()->findUniqueCols(leftUniqueCols)))
  {
    // Could not find a set of unique cols.
    // If the left sub-tree contains a UNION/TRANSPOSE/SEQUENCE or SAMPLE
    // then we will fail to unnest the subquery for this reason.
    filterParent->eliminateFilterChild();
    // left child does not have a unique constraint
    // cannot unnest this subquery
    if (CmpCommon::getDefault(SUBQUERY_UNNESTING) == DF_DEBUG)
      *CmpCommon::diags() << DgSqlCode(2997)   
	<< DgString1("Subquery was not unnested. Reason: Left child does not have a unique constraint"); 

    // Things to consider (referring to the picture above): If the all of the 
    // following are true:
    //   * {pred2} has only equals/VEG predicates of the form X.col = Y.col
    //   * {aggr} does not have any outer references
    //   * {pred3} does not have any outer references
    //
    // then we could do an alternative transformation, not yet implemented:
    //
    //      TSJ                                   Join {pred2: X.a=Y.b, ...}
    //     /   \                                  /   \
    //    /     \                                /     \
    //   X      ScalarAgg {pred3}      -->      X      grby {Y.b, ...} {pred3}
    //             |      {aggr}                         \  {aggr}
    //             |                                      \
    //          Filter {pred2: X.a=Y.b, ...}               Y {pred1}
    //             |
    //             |
    //             Y {pred1}
    //
    // Pros: - The groupby is already at a place where it will likely
    //         end up in the optimal plan
    // Cons: - We don't get a nice join backbone with all base tables
    //
    // Cases where we could attempt this transformation:
    //   - We fail to find a unique key for X (i.e. we reach here)
    //   - pred2 has a very high selectivity, making newJoin (in the picture
    //     at the top of this method) similar to a cartesian product

    return NULL ;
  }

  // if subquery needs left joins some additional checks are done here to
  // see if pull up groupby transformation can be done while preserving 
  // semantic correctness. No changes for left joins or preserving nulls
  // is done in this method.
  if (candidateForSubqueryLeftJoinConversion())
  {
    if (NOT selectionPred().isEmpty())
    { 
      // Selection predicates in a Join that needs to be converted to a Left Join
      // can be tricky, particularly if they contain aggregates.
      // We skip such a subquery for now.
      if (CmpCommon::getDefault(SUBQUERY_UNNESTING) == DF_DEBUG)
      {
         *CmpCommon::diags() << DgSqlCode(2997)   
                << DgString1("Subquery was not unnested. Reason: Join with selectionPreds cannot be converted to LeftJoin."); 
      }
      filterParent->eliminateFilterChild();
      return NULL ;
    }
  }

  // make copies of GroupBy, Join, Joins left and right children before 
  // making any changes. All changes will be made on the copied nodes.
  // If for some reason unnesting cannot be completed, the original node
  // is returned.

  // copy the left child of Join
  RelExpr * newLeftChild = copyNodeAndSetChildren(oldLeftChild, stmtHeap);

      // copy the right child of Join
  RelExpr * newRightChild = copyNodeAndSetChildren(oldGBgrandchild, stmtHeap);

      // copy the Join
  Join * newJoin = (Join *) copyNode(this, stmtHeap);
  newJoin->getGroupAttr()->clearLogProperties(); //logical prop. must be resynthesized

      // New GroupBy is a copy of the old Scalar Aggregate
  GroupByAgg *newGrby = (GroupByAgg *) copyNode(oldGB, stmtHeap);
      newGrby->setRETDesc(getRETDesc()); 
  newGrby->getGroupAttr()->clearLogProperties(); //logical prop. must be resynthesized
  GroupByAgg *newSubQGrby = NULL;
  if (nestedAggInSubQ)
  {
    newSubQGrby = (GroupByAgg *) copyNode(oldGB->child(0)->castToRelExpr(), 
					  stmtHeap);
    newSubQGrby->getGroupAttr()->clearLogProperties();  
  }
      
  // For multi-level subqueries it is possible that this Join is 
  // not a TSJ, but still contains outer references. This happens 
  // when right child does not need any values from the left child, 
  // but it does need values from a parent subquery. If the 
  // selection predicate (or join predicate)
  // of this Join needs any aggregate outputs
  // from the its old groupBy child, then those predicates need
  // to move up to the new parent GroupBy node.
  NABoolean safeToPullUpGrby;
  safeToPullUpGrby = newJoin->pullUpPredsWithAggrs(newGrby);
  
  if (NOT safeToPullUpGrby )
  { 
      // The join contains aggregates
      // Skip such this subquery . 
     filterParent->eliminateFilterChild();
     return NULL ;
  }

  if (nestedAggInSubQ)
  {
    safeToPullUpGrby = newJoin->pullUpPredsWithAggrs(newSubQGrby); 
    if (NOT safeToPullUpGrby )
    {  
      filterParent->eliminateFilterChild();
      return NULL ;
    }
    // inputs of newSubQGroupBy are same as the old
    // TSJ/Join that we are replacing. Outputs are join's + aggregates
    newSubQGrby->getGroupAttr()->addCharacteristicOutputs
      (getGroupAttr()->getCharacteristicOutputs());
    newSubQGrby->getGroupAttr()->setCharacteristicInputs
      (getGroupAttr()->getCharacteristicInputs());
    newSubQGrby->child(0) = newJoin ;
  }
      
  // inputs and outputs of new GroupBy are same as the old
  // TSJ/Join that we are replacing
  newGrby->getGroupAttr()->setCharacteristicOutputs
    (getGroupAttr()->getCharacteristicOutputs());
  newGrby->getGroupAttr()->setCharacteristicInputs
    (getGroupAttr()->getCharacteristicInputs());

  if (nestedAggInSubQ)
    newGrby->child(0) = newSubQGrby;
  else
    newGrby->child(0) = newJoin ;


  // set the grouping cols for new GroupBy
  // grouping cols for new GroupBy are
  // unique cols of X + 
  // cols of X that are needed to evaluate its selection pred. +
  // cols of X that part of the characteristic outputs
  newGrby->setLeftUniqueExpr(leftUniqueCols);
  ValueIdSet oldLeftChildOutputs
    (oldLeftChild->getGroupAttr()->getCharacteristicOutputs());
  newGrby->computeGroupExpr(
		leftUniqueCols, 
		oldLeftChildOutputs, 
		normWARef
			   );
  if (nestedAggInSubQ) 
  {
    newSubQGrby->getGroupAttr()->
      addCharacteristicOutputs(newGrby->groupExpr());
    newSubQGrby->computeGroupExpr(newGrby->groupExpr(), 
				  oldLeftChildOutputs, 
				  normWARef); 
  }
  


  // The newGrby cannot be a scalar groupby under any circumstance
  // So if the group expression is empty, add a constant to the 
  // list of the grouping columns, so that this groupby is not scalar
  // i.e. does not produce a NULL value for empty groups.
  if (newGrby->groupExpr().isEmpty()) 
  {
    ItemExpr *tf = new (stmtHeap) ConstValue(0);
    tf->synthTypeAndValueId(TRUE);
    newGrby->groupExpr() += tf->getValueId();
  } 

  // connect newJoin to newX and newY
  newJoin->child(0) = newLeftChild ;
  newJoin->child(1) = newRightChild ;

  newJoin->setOperatorType(REL_JOIN) ;

  // pull up predicates in filter to newJoin
  // do not change the filter itself in case we 
  // decide to not unnest. 
  if (oldGB->child(0)->getOperatorType() == REL_FILTER)
    newJoin->selectionPred() += oldGB->child(0)->castToRelExpr()->selectionPred();
  else if (nestedAggInSubQ && 
	   oldGB->child(0)->child(0)->getOperatorType() == REL_FILTER)
    newJoin->selectionPred() += 
      oldGB->child(0)->child(0)->castToRelExpr()->selectionPred();  

  // If the new GroupBy contains any outer references (i.e. requiredInputs
  // that are not provided by the user) then mark it as needing
  // the MoveUpGroupBy transformation.
  ValueIdSet outerReferences;
  newGrby->getGroupAttr()->getCharacteristicInputs().
					getOuterReferences(outerReferences);
  if (NOT(outerReferences.isEmpty()))
  {
    if (!nestedAggInSubQ)
      newGrby->setRequiresMoveUp(TRUE) ;
    else
    {
       filterParent->eliminateFilterChild();
       if (CmpCommon::getDefault(SUBQUERY_UNNESTING) == DF_DEBUG)
         *CmpCommon::diags() << DgSqlCode(2997)   
                             << DgString1("Subquery was not unnested. Reason: More than 1 level of nested subquery and nested aggregate are both present");
       return NULL;
    }
  }
 
  return newGrby ;
}   // Join::pullUpGroupByTransformation()

/*-----------------------------------------------------------------------
GroupByAgg::nullPreservingTransformation()

// The Null preserving transformation is applied to the output of the 
// PullUpGroupBy transformation, if the subquery has null preserving 
// predicates. According to the the Dayal-Murali algorithm such subqueries
// require a Left Join instead of a Join. The effect of this transformation
// is shown below
//
// 
//     GroupBy {pred3}                    MapValueId {topMap:original outputs of GroupBy
//        |                                    |       bottomMap:new NullInstantiated outputs of GroupBy}
//        |                                    |
//       Join {SP:pred2}       ---------->  GroupBy {pred3, aggregateExpr and groupExpr
//      /    \                                 |     expressed in terms of nullInstantiated output of LeftJoin}
//     /      \                                |      
//    X        Y {pred1}                    LeftJoin{JP:pred2}
//                                          /       \
//                                         /         \
//                                        X          Y{pred1}
//
// The MapValueId node shown here is present only if the GroupBy has outputs
// from the right side of the Join. The aggregateExpr in the transformed GroupBy 
// has new aggregates if the original aggregate contains count or oneTrue
// aggregates.
//
// This method is split in two halfs. This one that does the LeftJoin 
// conversion and error checking, and nullPreserveMyExprs() that
// does the aggregate rewriting and nullInstantiation.
//
// Expects:    Child of groupBy to be a Join.
// Sideffects: If successfull will convert the join child into a LeftJoin
//             with its output from the joins Right child nullInstantiated
//             and the groupBy's aggregates rewritten in terms of the 
//             nullInstantiated outputs of the LeftJoin and in the case
//             of the OneTrue aggregate, rewrittien as a count(0).
//           
//             Another sideffect is that the LeftJoin now will own the
//             the VEGregion of the old groupBy. Thus we have now 
//             changed the original query tree. We remember this in the SqoWA
//             (part of the NormWA) so that we can reassign the VEGregion back 
//             if we have to give up on unnesting this subquery further down 
//             the road.  
------------------------------------------------------------------------------*/
RelExpr* GroupByAgg::nullPreservingTransformation(GroupByAgg* oldGB, 
                                               NormWA& normWARef) 
{
  GroupByAgg * newGrby = this;
  Join * newJoin = (Join*) child(0)->castToRelExpr();
  RelExpr * newRightChild = newJoin->child(1)->castToRelExpr();
  
  // oldGBgrandchild is going to be the child of the filter from the
  // original tree. We use that references because we know that its
  // outputs are correct and consistent at this point.
  RelExpr *oldGBgrandchild;
  if (oldGB->child(0)->getOperatorType() == REL_FILTER)
    oldGBgrandchild = oldGB->child(0)->child(0)->castToRelExpr();
  else
    oldGBgrandchild = oldGB->child(0)->castToRelExpr();

  // two checks are performed below to see if this subquery can be unnested
  // using left joins. If one of the following is true we do not unnest
  // (a) Filter preds without reference to anything from the inner side
  //       - This typically only happens in multilevel queries
  //         where the filter predicates are correlated on the outer tables
  //         but do not refer to anything on the inner. What we have observed
  //         with these types of queries is that the predicate ends up being
  //         pulled up, then pushed down again, but when it gets pushed down,
  //         it will end up on the left hand side instead of the right - where
  //         it came from.
  //
  // (b) Aggregate Expr and outputs of newGrby contains oneTrue 
  //       - we do not want to unnest these as we have no way of 
  //         fixing up the oneTrue replaced by (count(1)>0) predicate
  //         upwards as the groupby can only output what is part of its
  //         aggregate or grouping expression. We could solve this by adding
  //         the expression to the group expression, but at the moment there
  //         isn't enought time to adequately test the semantic effects of 
  //         such a change. This should be looked at for phase 3. 

  ValueIdSet emptySet, coveredSubs, newOutput;
  const ValueIdSet& filterPreds = newJoin->selectionPred();
  
  // If the filter preds do not reference anything from the inner side
  // we do not unnest this subquery with left joins.

  if ( (NOT filterPreds.isEmpty()) && 
        filterPreds.referencesOneValueFromTheSet
        (oldGBgrandchild->getGroupAttr()->getCharacteristicOutputs()) == FALSE ) 

  {
    // We are not referencing anything from the inner side...
    // Predicate does not reference the Inner Table
    if (CmpCommon::getDefault(SUBQUERY_UNNESTING) == DF_DEBUG)
    {
      *CmpCommon::diags() << DgSqlCode(2997)   
          << DgString1("Subquery was not unnested. Reason: Join predicate has no references to inner table."); 
    }
    oldGB->eliminateFilterChild();
    return NULL ;
  } 

  // Check to see if the newGrby contains a oneTrue
  // which is used to represent an EXIST.
  // if we find one we want to replace it with a
  // count(1)>0 predicate , and count(1) will then replace the
  // oneTrue as the aggregate.
  // This is done so to get a nullinstantiated version of
  // the constant, thus preserving the semantic of the 
  // query. The actual transformation of this happens in 
  // nullPreserveMyExprs(), but we need to do some checking before we get 
  // that far.
  ValueId ignoreReturnedVid;
  ValueId oneTrueVid;

  // change newJoin into a left join and move preds into the join predicate
  // we have already guaranteed that all selection preds in the newJoin are
  // from the filter node and they do not contain any aggregates.
  
  CMPASSERT((newJoin->getOperatorType() == REL_JOIN) || 
            (newJoin->getOperatorType() == REL_TSJ));

  newJoin->setOperatorType(REL_LEFT_JOIN) ;
  newJoin->joinPred() = newJoin->selectionPred();
  newJoin->selectionPred().clear(); 

  // Want the left join to take over the VEG region from
  // the old scalar-agg and use that to the 
  // right child region (subtreeId = 1)
  VEGRegion* oldGBRegion = normWARef.locateVEGRegion(oldGB,0);
  CMPASSERT (oldGBRegion != NULL);

  normWARef.getSqoWA()->insertChangedRelExpr(oldGB, newJoin, 
                        SQO_REASSIGNED_VREGION, oldGBRegion->getRegionId(), 0, 1);

  normWARef.reassignVEGRegion(oldGB, 0, newJoin, 1);


  return( nullPreserveMyExprs(normWARef));
                          
}   // GroupByAgg::nullPreservingTransformation()

/*-----------------------------------------------------------------------
GroupByAgg::nullPresereMyExprs()

// This method takes care of nullInstantiate any of the outputs from
// the right child of the Left join.
//
// It also rewrites the GroupBy's expressions in terms of the newly 
// nullinstantiated values.
//
// It then creates a MapValueId on top of the groupBy if the groupBy outputs
// any of those values that got NullInstantiated. This faciliatates translating
// the NullInstantiated values back to their original form before we introduced
// the LeftJoin. The top part of the map has the original ValueIds in it, so 
// that we do not need to rewrite the tree above us.
//
// A before and after picture is shown below:
//
// 
//     GroupBy {pred3}                    MapValueId {topMap:original outputs of GroupBy
//        |                                    |       bottomMap:new NullInstantiated outputs of GroupBy}
//        |                                    |
//       Join {SP:pred2}       ---------->  GroupBy {pred3, aggregateExpr and groupExpr
//      /    \                                 |     expressed in terms of nullInstantiated output of LeftJoin}
//     /      \                                |      
//    X        Y {pred1}                    LeftJoin{JP:pred2}
//                                          /       \
//                                         /         \
//                                        X          Y{pred1}
//
// The MapValueId node shown here is present only if the GroupBy has outputs
// from the right side of the Join. The aggregateExpr in the transformed GroupBy 
// has new aggregates if the original aggregate contains count or oneTrue
// aggregates.
//
// Note: It is assumed that this function is called after the GroupBy has moved
// on top of the LeftJoin!
//
// Expects: A LeftJoin as the groupBy's child
// 
// Sideffects: 1) NullInstantiates outputs of the LeftJoin steming from the
//                the LeftJoins right child.
//           
//             2) count(*) and count(keyCol) has already been translated into
//                a count(1). Add the 1 to the LeftJoins output and 
//                nullInstantiate it. For a count(col), col is already part
//                of the LeftJoin's output. Then change the opType of the 
//                count() to be of type ITM_COUNT_NONULL to take care of the
//                count bug. Since we now changed the the itemExpr that is 
//                common to the old relExpr tree, we need to remember this 
//                in the SqoWA(member of NormWA) so we can undo it if we 
//                need to give up.
//                Note we can have several counts in here..
//
//             3) EXIST is translated earlier in the compiler to a ITM_ONE_TRUE.
//                Replace the ONETRUE with a count(0) similarly to what we did
//                in 2) above. Again we need to remember this change as we 
//                are changing an itemExpr common to the old tree.
//                There will be only ONE special aggregate like ONE_TRUE.
//
//             4) The selection predicate, aggregate expression, 
//                grouping expression,leftUnique expressions and output 
//                expression are rewritten in terms of the nullInstantiated 
//                outputs from the LeftJoin.
//
//             5) if the rewritten outputs of the groupBy contains any
//                nullInstantiated values from the LeftJoin, we need to 
//                insert a mapValueId node on top of the groupBy to translate
//                between the old and the new groupBys. The tree above us
//                expects the old ones.
------------------------------------------------------------------------------*/
RelExpr* GroupByAgg::nullPreserveMyExprs( NormWA& normWARef)
{
  GroupByAgg * newGrby = this;
  ValueId oneTrueVid;
  ValueId anyTrueVid;
  Join * newJoin = (Join*) child(0)->castToRelExpr();
  RelExpr * joinRightChild = newJoin->child(1)->castToRelExpr();
  CollHeap *stmtHeap = CmpCommon::statementHeap() ;

  // For safety, in case someone calls this method out of context.
  if (newJoin->getOperatorType() != REL_LEFT_JOIN) 
     return this;
  
  // Get the outputs of the Left Joins right child
  // we need to add a constant to it to get it nullinstantiated
  // if we have a count or a oneTrue aggregate in the newGrby
  ValueIdSet currentOutputs = joinRightChild->getGroupAttr()->getCharacteristicOutputs();

  // Handle the count case
  if (newGrby->aggregateExpr().containsCount()) 
  {
    // count(*) gets translated earlier in the compiler
    // to count(1).
    // If we have a count(*) situation, add the constant
    // to the join's right child's output so that it can 
    // get nullInstantiated
    // Doing so takes care of the infamous count() bug.
    // We also need to make sure the count operator is of type
    // ITM_COUNT_NONULL.

    // count(col) works as is as long as we make sure that
    // the count operator is of type ITM_COUNT_NONULL.
    // In the case where col is non Nullable count(col) also
    // gets translated into a count(1)

    // Need to nullInstantiate any outPuts from the right side..

    for ( ValueId vid = newGrby->aggregateExpr().init(); 
                        newGrby->aggregateExpr().next(vid);
                        newGrby->aggregateExpr().advance(vid)) 
    {
      if ((vid.getItemExpr()->origOpType() == ITM_COUNT_STAR__ORIGINALLY) ||
          (vid.getItemExpr()->origOpType() == ITM_COUNT))
      {     
         // Found a count(*) or a count(col)
         // a count(*) is represented as a count(1) 
         // Make sure we add the constant as a fake output
         // of the leftJoin so that it will be nullInstantiated.
         // In the case of count(col), col is already an output from the
         // leftJoin.
         // 

         // Add the const used in count(*) expression to the joins 
         // output so it can be nullinstantiated.
         if (vid.getItemExpr()->child(0)->getOperatorType() == ITM_CONSTANT)
         {
           currentOutputs += vid.getItemExpr()->child(0)->getValueId();

         }
         normWARef.getSqoWA()->insertChangedItemExpr(vid, SQO_NEWOPTYPE, vid.getItemExpr(),
                                   vid.getItemExpr()->getOperatorType());
         // unconditionally change the COUNT to COUNT_NONNULL
         // This constant will be nullinstantiated below
         vid.getItemExpr()->setOperatorType(ITM_COUNT_NONULL);
      }
    }
  }

  if ( aggregateExpr().containsOneTrue(oneTrueVid) ) 
  {

    ItemExpr *constVal = new (stmtHeap) SystemLiteral(1);

    // replace the OneTrue aggreate in the newGrby selection 
    // predicate with the manufactured count(1) > 0 in
    // the groupby selection predicate. 

    // Also replace the OneTrue aggregate with the
    // count(1) aggregate in the newGrby's aggregate.

    // create the new count(1) aggregate.
    Aggregate * dummyAgg = new (stmtHeap)
                           Aggregate(ITM_COUNT_NONULL, constVal);

    // Create the count(1)>0 predicate.
    BiRelat *fakeCountPred = new (stmtHeap)
                             BiRelat(ITM_GREATER,
                             dummyAgg,
                             new (stmtHeap) SystemLiteral(0));

    fakeCountPred->synthTypeAndValueId();

    // Need to nullInstantiate any outPuts from the right side..
    // Add the fake column to the output of the join's right child.
    // By having the LeftJoin output the fake constant we can tell
    // if the row gets nullInstantiated. If the fake constant comes
    // back as NULL, we have a nullInstantiated row!

    // This can only happen after we do the synthTypeAndValueId() above...
    // if this line needs to move above, then you have to call 
    // synthTypeAndValueId on the constVal...
    currentOutputs += constVal->getValueId();

    // Retain the old itemExpr so that we can restore it 
    // if we bail from unnesting..
    normWARef.getSqoWA()->insertChangedItemExpr(oneTrueVid, SQO_REPLACED, 
                                   oneTrueVid.getItemExpr());

    // By using replaceItemExpr, we immediately 
    // fix up the newGrby's selection predicate if it 
    // contained the oneTrue.
    oneTrueVid.replaceItemExpr(fakeCountPred);

    // Fix up the aggregate.
    newGrby->aggregateExpr() -= oneTrueVid;
    newGrby->aggregateExpr() += dummyAgg->getValueId();

  }

  if (newGrby->aggregateExpr().containsAnyTrue(anyTrueVid)) 
  {

    // For the cases where the groupBy's selection predicate
    // contains a AnyTrue(), we need to add in an additional
    // check to also allow nullInstantiated rows to pass or to
    // transform its result to be of equivalent value to that of 
    // the aggregate in its nested form.

    // For example
    // 
    // The following query:
    //  SELECT A  FROM T1  WHERE  B   =  ALL
    //    (SELECT   T2.D      FROM T2 WHERE T2.D = T1.B)  OR     EXISTS
    //    (SELECT  T3.F  FROM T3  WHERE   T3.H > T1. B  AND  T3.H  < T1.A)
    //    order by 1;
    //
    //         Root                             Root
    //          |                                |
    //         Tsj                 ->           Tsj
    //         /   \                           /   \
    //       Tsj   ScalAgg2                 MapVid  ScalAgg2
    //      /   \        \                    /          \ 
    //    T1    ScalAgg  T3                 GroupBy      T3
    //             \                          |
    //             T2                       LeftJoin
    //                                       /    \
    //                                      T1    T2
    //
    // In this example, ScalAgg in the nested case produces the following
    // AnyTrue() aggregate: AnyTrue(T2.D <> T1.B), which is an input to ScalAgg2
    // If we have a row in T1, where T1.B is NULL, the nested ScalAgg will
    // get a NO_DATA from the scan of T2 for that row, which means an empty
    // group, in which case ANY_TRUE will evaluate to FALSE. 
    //
    // In the unnested case, the same row from T1 will produce a NullInstantiated
    // row when joined with T2 due to the leftJoin, thus the group passed up
    // to the GroupBy (which for the unnested case also produces the same
    // AnyTrue() aggregate), contains 1 row, and the AnyTrue() aggregate will
    // evaluate to UNKNOWN due to the NULL value for T1.B and the NULL value
    // for T2.D.
    //
    // To solve this problem for the unnested case, we add a fake constant
    // to the output of the LeftJoin, and augment the anyTrue predicate to be 
    // AnyTrue(T2.D <> T1.B AND NOT IsNull(fakeConst)).
    //
    //

    // We have a similar problem when the groupBy contains a Not AnyTrue()
    // selection predicate as a result of a translation of an ALL expression to
    // a NOT ANY.

    // Create the fake constant
    ItemExpr *constVal = new (stmtHeap) SystemLiteral(1);
    constVal->synthTypeAndValueId();

    // Need to add the fake constant to the group Expression
    // since the expression we are ANDing in is not part of the 
    // aggregate expression .
    newGrby->groupExpr() += constVal->getValueId(); 

    // Create the IS NULL predicate
    UnLogic *newIsNullPred = new (stmtHeap) 
                                 UnLogic(ITM_IS_NULL, constVal);
 
    newIsNullPred->synthTypeAndValueId();

    ItemExpr *anyTrueExpr = anyTrueVid.getItemExpr()->child(0);

    // Create the Not IS NULL predicate
    UnLogic *newNotPred = new (stmtHeap) UnLogic(ITM_NOT, newIsNullPred);
    newNotPred->synthTypeAndValueId();

    // AND it with the existing AnyTrue predicate..
    BiLogic *newPred = new (stmtHeap) BiLogic(ITM_AND,
                                                anyTrueExpr,
                                                newNotPred);

    newPred->synthTypeAndValueId(TRUE);

    // Remember what we changed, so it can be restored if we need to back out..
    normWARef.getSqoWA()->insertChangedItemExpr(
                                            anyTrueVid, 
                                            SQO_NEWCHILD, 
                                            anyTrueExpr, 
                                            0);


    // assign the new predicate to the AnyTrue node.
    anyTrueVid.getItemExpr()->child(0) = newPred;

    // Need to nullInstantiate any outputs from the right side..
    // Add the fake column to the output of the left join.
    // By having the LeftJoin output the fake constant we can tell
    // if the row gets nullInstantiated. If the fake constant comes
    // back as NULL, we have a nullInstantiated row!

    // We facilitate this by adding the constant to the LeftJoins
    // nullInstantiatedOutput list. 


    currentOutputs += constVal->getValueId();

  }

  // NullInstantiate the output from the newJoin's right child.
  ValueIdList &nullOutputList = newJoin->nullInstantiatedOutput();
  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context());

  for (ValueId exprId = currentOutputs.init(); 
               currentOutputs.next(exprId); 
               currentOutputs.advance(exprId)) 
  {
    ValueId nullId = exprId.nullInstantiate(&bindWA,TRUE);
    nullOutputList.insert(nullId);
  }

  newJoin->normalizeNullInstantiatedOutput(normWARef);

  ValueIdSet aggExprRewritten, selPredsRewritten;
  ValueIdSet leftUniqueExprRewritten, grpExprRewritten;
  const ValueIdSet &selPreds = newGrby->getSelectionPred();
  const ValueIdSet &grpExpr = newGrby->groupExpr();
  const ValueIdSet &aggExpr = newGrby->aggregateExpr();
  const ValueIdSet &leftUniqueExpr = newGrby->leftUniqueExpr();
  // Create a copy of the newJoins map so that our rewrites
  // do not have unwanted sideffects where the join might 
  // put any of the newGrby aggregates in its output.
  ValueIdMap *rightChildMap = new (stmtHeap) 
                                ValueIdMap(newJoin->rightChildMapForLeftJoin());
  // This is kind of counter intuitive.
  // We the top part of the map contains the nullInstantiated
  // predicates, which we want to use in the groupBy's 
  // aggregate expression, thus the use of rewriteValueIdSetUp,
  // and the reversal of the arguments...
  rightChildMap->rewriteValueIdSetUp(aggExprRewritten,aggExpr);
  // Now we need to look for count(1) (same as count(*) 
  // and substitute that with the nullinstantiation of the
  // const column.
  newGrby->aggregateExpr() = aggExprRewritten;

  // Remap the selection predicate as well.
  rightChildMap->rewriteValueIdSetUp(selPredsRewritten,selPreds);
  newGrby->setSelectionPredicates(selPredsRewritten);

  // Remap the group expression as well.
  rightChildMap->rewriteValueIdSetUp(grpExprRewritten,grpExpr);
  newGrby->groupExpr() = grpExprRewritten;

  // Remap the leftUnique expression as well. Needed for when we move
  // above a Left Join
  rightChildMap->rewriteValueIdSetUp(leftUniqueExprRewritten,leftUniqueExpr);
  newGrby->leftUniqueExpr() = leftUniqueExprRewritten;

  newGrby->getGroupAttr()->normalizeInputsAndOutputs(normWARef);

  // Create a MapValueID Node on top of the 
  // GroupBy that map between the old join 
  // output used above and the new GroupBy output

  NABoolean mapNeeded = FALSE;
  ValueIdSet rewrittenGbyOutputs;
  ValueIdSet gbyOutputs = newGrby->getGroupAttr()->getCharacteristicOutputs();
  ValueIdMap *map = new (stmtHeap) ValueIdMap;


  // cannot use the rewriteValueIdSetUp routine for the outputs,
  // as we need to construct a map for the outputs and the ValueIdSets
  // used in rewriteValueIdSetUp() do not retain the order of the Vids
  // using a ValueIdList doesn't help either as the order is reversed,
  // and it seemed like a bad idea to rely on that to never change.
  for (ValueId topMapId = gbyOutputs.init(); 
      gbyOutputs.next(topMapId); gbyOutputs.advance(topMapId))
  {
    ValueId bottomMapId;

    rightChildMap->rewriteValueIdUp(bottomMapId,topMapId);

    // Only if our outputs will actually be different, do we want to
    // create a map.
    // One would think that it should be ok to add elemets to the map
    // that have the same value in both the upper and lower part, 
    // but that ends up producing incorrect output.... so we only 
    // add elements that are
    // different.
    if ( topMapId != bottomMapId ) 
    {
      mapNeeded = TRUE;
    }
      // Add a new mapping entry for the MapValeIds node.
    map->addMapEntry(topMapId, bottomMapId);
    rewrittenGbyOutputs += bottomMapId;
  }
  newGrby->getGroupAttr()->setCharacteristicOutputs(rewrittenGbyOutputs);

    // For phase 3 we need to remember that we created a map so 
    // If we are moving a GroupBy on top of a LeftJoin (that we already
    // have converted), we don't want to create an additional map. This
    // since the first map already maps any of the output from this left
    // join.
  if ( mapNeeded )
  {
    MapValueIds * newMap = newGrby->buildMapValueIdNode(map);
    return newMap;
  }
  return newGrby;
}   // GroupByAgg::nullPreserveMyExprs()

/*-----------------------------------------------------------------------------
// Join::moveUpGroupByTransformation()
// MoveUp GroupBy transformation. Relevant only for subqueries with
// two or more levels of nesting. For a two level subquery, at this stage
// the new tree looks like 
// MovedUpGroupByTail(newJoin(X2,moveUpGroupBy(Y2))).
// If the selection pred. of moveUpGroupBy and/or Y2 contain outer references
// those predicates will have to be pulled up so that newJoin does not have
// to be a TSJ. The first step in this process is to apply the MoveUpGroupBy
// transformation which will change the new tree to
// MovedUpGroupByTail(moveUpGroupBy(newJoin(X2,Y2))).

//     movedUpGrbyTail(newGrby)                    movedUpGrbyTail(newGrby)     
//             |                                           |
//             |                                           |
//           newJoin                                   moveUpGroupBy
//           /     \                                       |
//          /       \                 ------>              |
//         X2      moveUpGroupBy                        newJoin         
//                     |                                /     \
//                     |                               /       \
//                     Y2                             X2       Y2

// If Y2 is GroupBy that has been marked for moveUp (which can happen if we have
// more than 2 levels of nesting), then it the next iteration through the while
// loop below the old Y2 will become the new moveUpGrby and the old moveUpGroupBy 
// will become the new movedUpGrpupByTail.
// If the query has N levels of nesting, we may have to move N-1 GroupBy
// nodes over the newly introduced Join.

// If subquery unnesting has introduced Left Joins and MapValueId nodes
// through the NullPreservingTransformation, then the moveUpGroupTransformation
// is slightly different from the figure shown above. If MapValueId nodes are
// present the transformation will be as shown below. Note that newJoin can be
// a regular Join or a LeftJoin. Note that In phase 2 we allow only atmost one 
// LeftJoin to be introduced by subquery unnesting per query, thus there can 
// be at most one MapValueId node introduced by subquery unnesting. 
// The transformation shown below will occur at most once per query. 
// In phase 3 this restriction will go away as we will then be able to unnest
// multiple subqueries that requires a Left Join.

//     movedUpGrbyTail(topGrby)                    movedUpGrbyTail(topGrby)     
//             |                                           |
//             |                                           |
//           newJoin                                   moveUpMap
//           /     \                                       |
//          /       \                 ------>              |
//         X2      moveUpMap                          moveUpGroupBy        
//                     |                                   |
//                     |                                   |
//                 moveUpGroupBy                        newJoin
//                     |                                 /    \
//                     |                                /      \
//                     Y2                              X2       Y2
//
//
// Expects: child(1) to be a GroupBy with the moveUp flag set, or a mapValueId
//          with a GroupBy child that has the moveUp flag set.
//
// Sideffects: If successfull will return a pointer to a groupBy that 
//             is a copy of the groupBy marked for moveUp which now
//             has this join as its child. This new groupby will also
//             have its grouping expression altered, as well as its inputs
//             and outputs. The groupBy's selection predicate will also
//             contain any of the join's selection predicates that contained
//             an aggregate from the original groupBy.

-------------------------------------------------------------------------------*/
GroupByAgg* Join::moveUpGroupByTransformation(GroupByAgg* topGrby, 
                                              NormWA & normWARef)
{
  GroupByAgg *moveUpGrby;
  ValueIdSet emptySet ;
  GroupByAgg *movedUpGrbyTail = topGrby;
  RelExpr * joinRightChild = child(1)->castToRelExpr();
  CollHeap *stmtHeap = CmpCommon::statementHeap() ;

  MapValueIds *moveUpMap = NULL;
  while ((joinRightChild->getOperatorType() == REL_GROUPBY) &&
        (((GroupByAgg*) joinRightChild)->requiresMoveUp()) || 
          ((joinRightChild->getOperatorType() == REL_MAP_VALUEIDS) &&
          ((( (RelExpr*) joinRightChild->child(0))->getOperatorType() == REL_GROUPBY) &&
            (((GroupByAgg*) joinRightChild->child(0)->castToRelExpr())->requiresMoveUp())))) 
  {

    if (isLeftJoin())
    {
          // We do not want to pull the groupBy above the left join
          // as it will restrict tuples from the left side of the left 
          // join to flow up. This needs to be revisited for phase 3 
          // when we transform to a Left Leaning tree when we have 
          // LeftJoins. 

       if (CmpCommon::getDefault(SUBQUERY_UNNESTING) == DF_DEBUG)
           *CmpCommon::diags() << DgSqlCode(2997)   
                               << DgString1("Subquery was not unnested. Reason: Can not move groupBy above LeftJoin."); 
       return NULL;
    }

    if (joinRightChild->getOperatorType() == REL_MAP_VALUEIDS) 
      {
          moveUpMap = (MapValueIds*) copyNodeAndSetChildren(
                                                 joinRightChild, stmtHeap);
          moveUpGrby = (GroupByAgg*) copyNodeAndSetChildren
                                     (
                                      joinRightChild->child(0)->castToRelExpr(),
                                      stmtHeap
                                     ) ;
          moveUpMap->child(0) = moveUpGrby;
          moveUpMap->getGroupAttr()->clearLogProperties();
      }
      else
      {
          moveUpMap = NULL;
          moveUpGrby = (GroupByAgg*) copyNodeAndSetChildren(
                                                 joinRightChild,stmtHeap) ;
      }



    moveUpGrby->getGroupAttr()->clearLogProperties();

    joinRightChild = copyNodeAndSetChildren
                      (
                         moveUpGrby->child(0)->castToRelExpr(),
                         stmtHeap
                      );
    
    //Join may have predicates that reference aggregates in moveUpGrby
    // If so pull these preds into moveUpGrby
    NABoolean safeToMoveGrby;
    safeToMoveGrby = pullUpPredsWithAggrs(moveUpGrby, moveUpMap);

    if (NOT safeToMoveGrby )
    { 
      // The join contains aggregates, skip this subquery.
       return NULL ;
    }
    child(1) = joinRightChild ;
    moveUpGrby->child(0) = this ;

    if (moveUpMap != NULL) 
      movedUpGrbyTail->child(0) = moveUpMap  ;
    else 
      movedUpGrbyTail->child(0) = moveUpGrby  ;

    // set up inputs and outputs of moveUpGroupBy taking into account
    // its new location in the query tree.
    moveUpGrby->addGroupExpr(movedUpGrbyTail->groupExpr());
    moveUpGrby->getGroupAttr()->setCharacteristicInputs(emptySet);
    moveUpGrby->primeGroupAttributes();
    moveUpGrby->getGroupAttr()->normalizeInputsAndOutputs(normWARef);

    //  Need to make sure we nullInstantiate anything that we need from
    //  the right hand side, as we may have just moved over a Left Join.
    //  This function is a no-op if our child is not a LeftJoin. 

    RelExpr * result = moveUpGrby->nullPreserveMyExprs(normWARef);

    if (result == NULL)
       return NULL;


    if (moveUpMap)
    {
      moveUpMap->getGroupAttr()->setCharacteristicInputs(emptySet);
      moveUpMap->primeGroupAttributes();
      moveUpMap->getGroupAttr()->normalizeInputsAndOutputs(normWARef);
      moveUpMap->pushdownCoveredExpr(
                    moveUpMap->getGroupAttr()->getCharacteristicOutputs(),
                    moveUpMap->getGroupAttr()->getCharacteristicInputs(),
                    emptySet);
    }
    
    movedUpGrbyTail->pushdownCoveredExpr(
      movedUpGrbyTail->getGroupAttr()->getCharacteristicOutputs(),
      movedUpGrbyTail->getGroupAttr()->getCharacteristicInputs(),
      movedUpGrbyTail->selectionPred()
                                        );


    // Sometimes the moveUpMap ends up being empty after being moved
    // on top of a Join. Eliminate it if we don't need it, otherwise 
    // it will impede output flow.

    if ( moveUpMap != NULL && 
         moveUpMap->getGroupAttr()->getCharacteristicOutputs().isEmpty()) 
    {
       movedUpGrbyTail->child(0) = moveUpMap->child(0); 
       // set up inputs and outputs of moveUpGroupBy taking into account
       // its new location in the query tree.
       moveUpGrby->addGroupExpr(movedUpGrbyTail->groupExpr());
       moveUpGrby->getGroupAttr()->setCharacteristicInputs(emptySet);
       moveUpGrby->primeGroupAttributes();
       moveUpGrby->getGroupAttr()->normalizeInputsAndOutputs(normWARef);

       // Repush

       movedUpGrbyTail->pushdownCoveredExpr(
          movedUpGrbyTail->getGroupAttr()->getCharacteristicOutputs(),
          movedUpGrbyTail->getGroupAttr()->getCharacteristicInputs(),
          movedUpGrbyTail->selectionPred()
                                        );
    }
    

    // does moveUpGroupBy still have outer references? If NO then it need
    // not "move over" any more Join nodes.
    ValueIdSet outerReferences;
    moveUpGrby->getGroupAttr()->getCharacteristicInputs().
                                           getOuterReferences(outerReferences);
    if (outerReferences.isEmpty())
      moveUpGrby->setRequiresMoveUp(FALSE) ;


    // moveUpGroupBy will have grouping cols of movedUpGroupByTail +
    // left unique cols that were computed for it previously +
    // cols needed to provide its outputs +
    // cols needed to compute its selection pred.
    // the superSet (second param) for this call is the current
    // value for the groupExpr. In other words the aim of the call below
    // is to see if the groupExpr for the moveUpGrby can be reduced 
    // from the setting that was done a few lines earlier.
    moveUpGrby->computeGroupExpr(movedUpGrbyTail->groupExpr(),
                                 moveUpGrby->groupExpr(), normWARef);

    // movedUpGrbyTail is set to moveUpGroupBy in case there are more 
    // GroupBys that need to be moved up.
    movedUpGrbyTail = moveUpGrby  ;
  }
  // end of MoveUpGroupBy transformation
  // note that if the specific pattern of GroupBy and Joins shown here
  // is not present then this transformation will not be applied and 
  // right subtree of the new Join will contain outer references. This
  // will cause unnesting this TSJ/Join to fail below, and we will revert
  // to the orginal nested tree for this subquery level.
  return movedUpGrbyTail;
}  // Join::moveUpGroupByTransformation()

/*----------------------------------------------------------------------------
// GroupByAgg::subqueryUnnestFinalize()
// set up inputs/outputs of the new Join, its children 
// and the newJoin's parent GroupBy.
// move selection predicates to the appropriate nodes.
// return FALSE if any outer references remain or if
// sufficient outputs cannot be produced to compute left side's 
// unique columns
//
// Expects: Child(0) to be a Join or a subQ groupby.
// Sideffects: recomputed inputs and outputs of the join child's children
//             recomputed inputs and outputs of the join.
//             pushes any of the groupBy's predicates down that can go down.
//             pushes any of the join's predicates down that can go down.

-------------------------------------------------------------------------------*/
NABoolean GroupByAgg::subqueryUnnestFinalize(ValueIdSet& newGrbyGroupExpr, 
					     NormWA& normWARef)
{
  Join * newJoin = NULL ;
  if (child(0)->getOperatorType() == REL_GROUPBY)
    newJoin = (Join*) child(0)->child(0)->castToRelExpr();
  else
    newJoin = (Join*) child(0)->castToRelExpr();
  RelExpr * newLeftChild  = newJoin->child(0)->castToRelExpr();
  RelExpr * newRightChild = newJoin->child(1)->castToRelExpr();


  newLeftChild->primeGroupAttributes();
  newRightChild->primeGroupAttributes();
  newLeftChild->getGroupAttr()->normalizeInputsAndOutputs(normWARef);
  newRightChild->getGroupAttr()->normalizeInputsAndOutputs(normWARef);
  ValueIdSet nonLocalPreds,valuesReqdByParent,availableInputs,outerReferences;

  // availableInputs is the requiredInputs of the newJoin minus
  // any outer references. These outer references are not really
  // available as our intention is to apply this transformation 
  // at succcesive levels and unnest all subqueries.
  availableInputs = newJoin->getGroupAttr()->getCharacteristicInputs();
  availableInputs += newRightChild->getGroupAttr()->getCharacteristicInputs();
  availableInputs.getOuterReferences(outerReferences);
  availableInputs -= outerReferences ;
  for (Int32 i = 0; i < 2; i++) {
    // --------------------------------------------------------------------
    // Check to see if we have any Outer References in our child's selection 
    // predicate
    // If we do we want to pull it up .
    // ---------------------------------------------------------------------
    if (newJoin->child(i)->selectionPred().getReferencedPredicates
                                            (outerReferences, nonLocalPreds))
    {
      if ((i == 1)&&newJoin->isLeftJoin())
         newJoin->joinPred() += nonLocalPreds ;
      else
         newJoin->selectionPred() += nonLocalPreds ;
      newJoin->child(i)->selectionPred() -= nonLocalPreds ;
      newJoin->child(i)->recomputeOuterReferences();
      nonLocalPreds.clear();
    }
  }

  //computing Join's inputs/outputs
  newJoin->primeGroupAttributes();
  newJoin->getGroupAttr()->normalizeInputsAndOutputs(normWARef);

  //push down any of the groupBy's predicates that we can.
  pushdownCoveredExpr( getGroupAttr()->getCharacteristicOutputs(),
                       getGroupAttr()->getCharacteristicInputs(),
                       selectionPred() );

  // Rules for pushdown from Join during this transformation are different 
  // in two ways from the usual. 
  // 1) If left child does not cover any part of a 
  // VEGPred it will still be retained in the Join, so that it can be pulled
  // further up the query tree as we apply this transformation at other levels
  // In the usual rules, the VEGPred will be pushed down to the right child
  // without being retained at the Join

  ValueIdSet emptySet;
  valuesReqdByParent = newJoin->getGroupAttr()->getCharacteristicOutputs() ;
  newJoin->pushdownCoveredExprSQO(valuesReqdByParent,
			      availableInputs, 
			      newJoin->selectionPred(),
			      emptySet,
			      TRUE, // keepPredsNotCoveredByChild0
			      TRUE); // keepPredsNotCoveredByChild1

  // check if right child still contains outer references. If so
  // this subquery level cannoy be unnested. Give up and return the
  // old TSJ. Note that other subquery levels may still be 
  // successfully unnested.
  outerReferences.clear();
  newRightChild->getGroupAttr()->getCharacteristicInputs().
					getOuterReferences(outerReferences);
  if (NOT(outerReferences.isEmpty()))
  {
    // right child still has outer references
    // cannot unnest this subquery
    if (CmpCommon::getDefault(SUBQUERY_UNNESTING) == DF_DEBUG)
      *CmpCommon::diags() << DgSqlCode(2997)   
	<< DgString1("Subquery was not unnested. Reason: Right child has outer references that cannot be removed by current unnesting."); 
    
    return FALSE ;
  }

  // Is the Join producing all the outputs needed for the new goruping
  // columns of the GroupBy? If not make a tree walk down the left subtree,
  // increase outputs as needed at various child levels so that this Join
  // can produce the needed values. If we fail unnesting is not possible
  // at this level.
  ValueIdSet additionalOutputsNeeded = newGrbyGroupExpr;
  additionalOutputsNeeded -= newJoin->getGroupAttr()->getCharacteristicOutputs();
  ValueIdSet savedOutputsNeeded = additionalOutputsNeeded ;
  if (newJoin->getMoreOutputsIfPossible(additionalOutputsNeeded))
  {
    newJoin->getGroupAttr()->addCharacteristicOutputs(savedOutputsNeeded);
  }
  else
  {
    // left sub-tree cannot produce additional columns required to group
    // by the left unique cols. Cannot unnest this subquery.
    // Can occur if left-subtree contains UNION, TRANSPOSE, SEQUENCE or SAMPLE
    if (CmpCommon::getDefault(SUBQUERY_UNNESTING) == DF_DEBUG)
      *CmpCommon::diags() << DgSqlCode(2997)   
	<< DgString1("Subquery was not unnested. Reason: Left subtree cannot produce output values required for grouping."); 
    return FALSE ;
  }
  return TRUE;
}   // GroupByAgg::subqueryUnnestFinalize()

/*----------------------------------------------------------------------------
// Join::applyInnerKeyedAccessHeuristic()
//
// Checks to see if the join predicate is on a key column of the inner table
// and the key column is the leading key column.
//
// Expects: a child chain like this:
//            GroupBy->Filter->Scan
//
// Sideffects: Doesn't change anything.
-------------------------------------------------------------------------------*/

NABoolean Join::applyInnerKeyedAccessHeuristic(const GroupByAgg* newGrby, 
					       NormWA & normWARef)
{
  RelExpr *oldGBgrandchild;
  // note that the child of oldGB is actually a Filter node, here 
  // oldGBgrandchild is the child of oldGB before the Filter was added.
  if (child(1)->child(0)->getOperatorType() == REL_FILTER)
    oldGBgrandchild = child(1)->child(0)->child(0)->castToRelExpr();
  else
    oldGBgrandchild = child(1)->child(0)->castToRelExpr();

	// Apply inner table keyed scan heuristic. This heuristic turns off subquery
	// unnesting for this tsj if the join predicate is on a key column of the inner table.
	// The heuristic applies only if the key column is the leading key column 
	// of a base table or an index. No consideration is made for the selectivity
	// of the index. This heuristic applies only if
	// 1. comp_bool_168 is OFF
	// 2. Inner side of tsj is a scan (not another subquery)
	// 3. There is only one level of nesting or this is tree subquery
        // 4. The number of tables below this join is LEQ COMP_INT_46 (default value is 10)
	// If there are multiple levels of nesting the benefit of this heuristic is 
	// doubtful as unnesting the lowest level will allow higher levels to be unnested.
	if((CmpCommon::getDefault(COMP_BOOL_168) == DF_OFF) &&
	   (oldGBgrandchild->getOperatorType() == REL_SCAN) &&
	   ((normWARef.getCorrelatedSubqCount() == 1) || 
	(NOT (((GroupByAgg*)newGrby)->requiresMoveUp()))) &&
        (getGroupAttr()->getNumJoinedTables() <= 
	    ActiveSchemaDB()->getDefaults().getAsLong(COMP_INT_46)))
	{
          RelExpr *oldGB = child(1)->castToRelExpr();

	  const TableDesc     * tableDesc = 
	    ((Scan *)oldGBgrandchild)->getTableDesc();
	  const LIST(IndexDesc *) & ixlist = tableDesc->getIndexes();
	  ValueIdSet preds = oldGB->child(0)->castToRelExpr()->selectionPred();
	  ValueIdSet leadingKeyCols ;
	  for (CollIndex j = 0; j < ixlist.entries(); j++)
	  {
	    // get only the leading key column from every access path.
	    ValueId vid = ixlist[j]->getOrderOfKeyValues()[0];
	    ItemExpr *colIE = vid.getItemExpr();
	    if (colIE->getOperatorType() == ITM_VEG_REFERENCE)
	    {
	      // get the valueid of the VEG
	      leadingKeyCols +=
		   ((VEGReference *)colIE)->getVEG()->getValueId();
	    }
	  }
	  for (ValueId x = preds.init();
	       preds.next(x);
	       preds.advance(x))
	  {
	    ItemExpr *ie = x.getItemExpr();
	    if (ie->getOperatorType() == ITM_VEG_PREDICATE)
	    {
	      ValueId id = ((VEGPredicate *)ie)->getVEG()->getValueId();
	      if (leadingKeyCols.contains(id))
	      {
		child(1)->eliminateFilterChild();
		if (CmpCommon::getDefault(SUBQUERY_UNNESTING) == DF_DEBUG)
		  *CmpCommon::diags() << DgSqlCode(2997)   
		    << DgString1("Subquery was not unnested. Reason: Join predicate is on a leading key column of inner table."); 
	  return TRUE ;
	    }
	  }
	}
      }
  return FALSE;
}   // Join::applyInnerKeyedAccessHeuristic()

// -----------------------------------------------------------------------
// Join::semanticQueryOptimizeNode().  This method facilitate the entry 
// point for semantic Query Optimization. It will attempt the following types
// of optimization:
//
// a) join elimination
// b) transform semi joins to inner joins 
// c) subqueries unnesting 
//
// For the time being, only one of these transformations happens on a 
// single query. Phase 3 may look at allowing multiple transformation
// on the same query. 
//
// The prework for checking if a particular transformation is possible 
// occured in the transformer, where we set flags to indicate what kind of
// transformation a particular query is a candidate for. The other thing that
// may happen in the transformer, if we decide a query is a candidate for SQO,
// is that we may create a filter node to hold predicates with outer references.
// One of the main functions of the filter is to prevent pushdown of predicates
// with outer references.
//
// JOIN ELIMINATION:
// For join elimination we apply the following rules:
//  1) If predicates have been marked for removal
//     Join {selection_pred: p1,p2,p3,...pn} --> Join {selection_pred: p3,...pn}
//     where p1 and p2 are equi join predicates that are known to be true due 
//     to a foreign_key-unique_key relationship
//
//  2) If the children of the join are marked for removal
//         
//	parent
//        |                  parent
//	Join                   |
//	/   \       ------>    X
//     X    Y   
//    
//    where the node Y has been marked for elimination by the synthLogPhase. 
//
//  3) If its a left join and has been markedForElimination by the normalize 
//     phase then
//
//      parent
//        |                    parent
//      LeftJoin                 |
//	/   \       ------>      X
//     X    Y   
//
// SEMI-JOIN TRANSFORMATION
//
//   a) If the right child is unique in the joining column and the semi join 
//      can be simply translated into a join. 
//      
//      An example query is 
//      
//      select t1.a
//      from t1
//      where t1.b in (select t2.a
//      		from t2) ; 
//      Here t2.a is a unique key of table t2.
//      
//      The following transformation is made
//        Semi Join {pred : t1.b = t2.a}  ------>  Join {pred : t1.b = t2.a} 
//
//   b) If the right child is not unique in the joining column then 
//      we transform the semijoin into an inner join followed by a groupby
//      as the join's right child. This transformation is enabled by default
//      only if the right side is an IN list, otherwise a CQD has to be used.
//
//                                          groupby  (X.key)
//          SemiJoin                           |
//           /    \        ------>           Join
//          X      Y                         /  \
//                                          X    Y
//
// SUBQUERY UNNESTING 
// The subquery unnesting consist of two main transformations:
// pullUpGroupBy and moveUpGroupBy transformation
// which are based on Dayal and Muralikrishna's algorithm (see below).
//
// a) pullUpGroupBy transformation:
//
// For a single level subquery this is the only transformation required for 
// subquery unnesting. 
// 
//      TSJ                             GroupBy 
//     /   \                              |    
//   X      ScalarAgg           -->      Join  (pred)
//             |                         /   \
//          Filter (pred)               X     Y 
//             |
//             Y 
//
// For a multilevel query this may happen several times. 
// Under certain circumstances, in a multilevel subquery, we may also need 
// to apply the moveupGroupBy transformation.
//
// b) moveUpGroupBy transformation:
//
// When the pullUpGroupBy transformation has to be applied more than once 
// on a query tree (for multi-level subqueries), then it is possible that 
// that a groupBy below still contains outer references. For example with
// a two level query, this is what the tree will look like after applying
// the pullUpGroupBy transformation twice:
//
//      TSJ2                               GroupBy2
//     /   \               pullUpGroupBy      |    
//   X      ScalarAgg2    transformation    Join2 
//             |             (2 times)      /   \
//          Filter2         ---------->    X     GroupBy1 
//             |                                   \
//             TSJ1                                Join1
//            /   \                                /  \
//           Y    ScalarAgg1                      Y    Z
//                   \
//                 Filter1
//                     \
//                      Z
//
// If the selection pred. of GroubBy1 and/or Join1 contain outer
// references after the transformation, those predicates will have to
// be pulled up so that Join2 does not have to be a TSJ. See the comment
// in Join::moveUpGroupByTransformation() for how the right side
// of the picture above gets transformed further.
// 
// One additional complication occurs when we need to convert any of the
// TSJs into a LeftJoin. This conversion occurs during either or both
// the pullUpGroupBy or moveUpGroupBy transformation. If we require a LeftJoin
// we manipulate the predicates and null-instantiated outputs of the LeftJoin
// that is from the right subtree in order to preserve correctness. Refer
// to the infamous count bug!
// For more details, please refer to 
// M. Muralikrishna, "Improved Unnesting Algorithms for Join Aggregate SQL Queries", 
// Proc. VLDB Conf., pp. 91-102 (1992)
// -----------------------------------------------------------------------
RelExpr * Join::semanticQueryOptimizeNode(NormWA & normWARef)
{
  // ---------------------------------------------------------------------
  // SemanticQueryOptimize each child.
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // SemanticQueryOptimize the left and right subtrees. Store pointers to 
  // the roots of the subtrees after SQO.
  // ---------------------------------------------------------------------

  if (isFullOuterJoin())
    normWARef.locateAndSetVEGRegion(this, 0 /* first child */);

  child(0) = child(0)->semanticQueryOptimizeNode(normWARef);

  if(isFullOuterJoin())
    normWARef.restoreOriginalVEGRegion();

  if (ownsVEGRegions())
    {
      // -------------------------------------------------------------
      // Locate and set the VEGRegion for the right subtree.
      // -------------------------------------------------------------
      if (isFullOuterJoin())
         normWARef.locateAndSetVEGRegion(this, 1 /* second child */);
      else
         normWARef.locateAndSetVEGRegion(this);
      child(1) = child(1)->semanticQueryOptimizeNode(normWARef);
      normWARef.restoreOriginalVEGRegion();
    }
  else
    {
      child(1) = child(1)->semanticQueryOptimizeNode(normWARef);
    }

  // In the bottom up phase of the SQO tree walk
  // check if there are 
  // a) joins to be eliminated or
  // b) semi joins to transform to inner joins or
  // c) any subqueries to unnest 

// a) Join Elimination
/*---------------------------------------------------------------------------------------*/
 
  RelExpr* reducedExpr = eliminateRedundantJoin(normWARef);
  if (reducedExpr != this)
    return reducedExpr;

// b) SemiJoin Transformation
/*---------------------------------------------------------------------------------------*/
  
  if (candidateForSemiJoinTransform())  // we have a semi join that could be transformed to 
                      // an inner join + group by.
  {
    reducedExpr = transformSemiJoin(normWARef);
    if (reducedExpr != this)
      return reducedExpr;
  }

// c) Subquery Unnesting
/*---------------------------------------------------------------------------------------*/

  if (candidateForSubqueryUnnest())
  {
    // SQO phase is called in a loop for join elimination
    // we do not want to attempt unnesting on the same node twice.
    setCandidateForSubqueryUnnest(FALSE);

    // Outer references are kept in a filter node, if there are no outer 
    // references then unnesting is not needed.
    // For subqueries that are not correlated the Filter node will be absent
    // as the method createAFilterParentIfNecessary() would not have created
    // a Filter node at this point in the query tree. Therefore non-correlated
    // subqueries will not enter the loop.
    // If comp_bool_221 is on we will unnest even if there is no filter node.
    if ((CmpCommon::getDefault(COMP_BOOL_221) == DF_OFF) &&
        ((child(1)->getArity() != 1) ||
	 !(child(1)->castToRelExpr()->hasFilterChild())))
    {
        if (CmpCommon::getDefault(SUBQUERY_UNNESTING) == DF_DEBUG)
          *CmpCommon::diags() << DgSqlCode(2997)
                              << DgString1("Subquery was not unnested. Reason: No Correlation found");
      return this ; // do nothing, no correlated subquery
    }


    // Increment the subquery id counter in SqoWA so that we can
    // destinguish between things we change for this subquery over another..
    normWARef.getSqoWA()->incrementSubQId();

    // Main body of subquery unnesting. The PullUpGroupBy transformation and 
    // the MoveUpGroupBy transformation are applied here.

    // The PullUpGroupByTransformation
    GroupByAgg* newGrby = pullUpGroupByTransformation(normWARef);
    if (newGrby == NULL)
      return this;

    // If inner table is accessed on a leading column of any access path
    // then do not unnest this subquery. We perform this check after 
    // the pullUpGroupByTransformation() since we want to cover the
    // the case of a Tree Query and we need to know if the newGrby requires
    // a moveUpGroupBy transformation.
    if(applyInnerKeyedAccessHeuristic((const GroupByAgg*)newGrby,normWARef))
      return this;
      
    MapValueIds* newMap = NULL ;

    if (candidateForSubqueryLeftJoinConversion())
    {
      RelExpr* result = newGrby->nullPreservingTransformation(
                                     (GroupByAgg*)child(1)->castToRelExpr(),
                                      normWARef);
      if (result == NULL)
      {
         normWARef.getSqoWA()->undoChanges(normWARef);
         return this;
      }
      if (result->getOperatorType() == REL_MAP_VALUEIDS)
         newMap = (MapValueIds*) result;
    }

    // Apply MoveUp GroupBy transformation. Relevant only for subqueries with
    // two or more levels of nesting. If moveUpGroupBy is not needed
    // movedUpGroupByTail will be set to newGrby.
    RelExpr* gbChild = newGrby->child(0)->castToRelExpr();
    Join * newJoin = NULL;
    GroupByAgg * newJoinParent = newGrby;
    if (gbChild->getOperatorType() == REL_GROUPBY) 
    {
      newJoin = (Join*) gbChild->child(0)->castToRelExpr();
      newJoinParent = (GroupByAgg*) gbChild;
    }
    else
      newJoin = (Join*) gbChild;

    GroupByAgg* movedUpGrbyTail = 
      newJoin->moveUpGroupByTransformation(newJoinParent, normWARef);

    NABoolean hasNoErrors;
    if (movedUpGrbyTail != NULL) 
    {
       hasNoErrors = movedUpGrbyTail->subqueryUnnestFinalize(
                                                    newGrby->groupExpr(),
                                                    normWARef);
    }
    if ((movedUpGrbyTail == NULL) || (NOT hasNoErrors))
    {  
      normWARef.getSqoWA()->undoChanges(normWARef);
      child(1)->eliminateFilterChild();
      return this ;
    }

    // this subquery level has been successfully unnested. Left linearize the 
    // join backbone. Comp_int_11 can be used to not left linearize  as we
    // go further up the tree. This is not advised as the the analyzer expects 
    // the tree to be left linear in many situations. This control is kept
    // as it provides the possibility to see some interesting plans.
    if ((ActiveSchemaDB()->getDefaults().getAsLong(COMP_INT_11) < 0) ||
        (newJoin->child(1)->getGroupAttr()->getNumJoinedTables() <= 
         ActiveSchemaDB()->getDefaults().getAsLong(COMP_INT_11)))
    {
      newJoin = newJoin->leftLinearizeJoinTree(normWARef, 
                                               UNNESTING); // Unnesting
      movedUpGrbyTail->child(0) = newJoin  ;
    }

      //synthesize logical props for the new nodes.
    if (newMap == NULL)
    {
      newGrby->synthLogProp(&normWARef); 
      return newGrby ;
    }
    else
    {
      newMap->synthLogProp(&normWARef); 
      return newMap ;
    }
  } 
  else
  {

    // this subquery was not unnested, but we could have other transformations
    // that would render the tree no longer left linearized
    if ((ActiveSchemaDB()->getDefaults().getAsLong(COMP_INT_11) < 0) ||
        (child(1)->getGroupAttr()->getNumJoinedTables() <= 
         ActiveSchemaDB()->getDefaults().getAsLong(COMP_INT_11)))
    {
      return leftLinearizeJoinTree(normWARef, SEMI_JOIN_TO_INNER_JOIN); // 
    }
  }
/*---------------------------------------------------------------------------------------*/
  return this;
} // Join::semanticQueryOptimizeNode()

NABoolean Join::prepareMeForCSESharing(
     const ValueIdSet &outputsToAdd,
     const ValueIdSet &predicatesToRemove,
     const ValueIdSet &commonPredicatesToAdd,
     const ValueIdSet &inputsToRemove,
     ValueIdSet &valuesForVEGRewrite,
     ValueIdSet &keyColumns,
     CSEInfo *info)
{
  if (isTSJForWrite() ||
      isTSJForUndo() ||
      isTSJForMerge() ||
      getIsForTrafLoadPrep())
    return FALSE;

  // The caller of this methods added "commonPredicatesToAdd" to
  // predicates_ (the generic selection predicates stored in the
  // RelExpr). That works for both inner and non-inner joins.  The
  // only thing we have left to do is to recompute the equi-join
  // predicates.
  findEquiJoinPredicates();

  return TRUE;
}

// ***********************************************************************
// $$$$ Union
// member functions for class Union
// ***********************************************************************

// -----------------------------------------------------------------------
// Union::transformNode()
// -----------------------------------------------------------------------
void Union::transformNode(NormWA & normWARef,
                          ExprGroupId & locationOfPointerToMe)
{
  CMPASSERT( this == locationOfPointerToMe );

  if (nodeIsTransformed())
    return;
  markAsTransformed();

  //++Triggers -
  if (getBlockedUnion())
	  normWARef.setInBlockedUnionCount();

  // ---------------------------------------------------------------------
  // Compartmentalize the VEGRegions between the left and the right
  // child so that their VEGs get installed in different VEGRegions,
  // where they rightfully belong. It prevents unenforcable "=" 
  // relationships from being deduced transitively.
  // The VEGRegion for each child of the union is only allowed to 
  // import outer references. It cannot "export" any "=" relationships.
  // ---------------------------------------------------------------------

  // Allocate a new VEGRegion within the scope of my own VEGRegion
  // for my left child.
  normWARef.allocateAndSetVEGRegion(IMPORT_ONLY,this,0);

  // Make values available to left child
  child(0)->getGroupAttr()->addCharacteristicInputs
	                    (getGroupAttr()->getCharacteristicInputs());

  // Transform the left child.
  child(0)->transformNode(normWARef, child(0)); 

  // Return to my own VEGRegion.
  normWARef.restoreOriginalVEGRegion();
  
  // Allocate another new VEGRegion within the scope of my own VEGRegion
  // for my right child.
  normWARef.allocateAndSetVEGRegion(IMPORT_ONLY,this,1);
  
  // Make values available to right child
  child(1)->getGroupAttr()->addCharacteristicInputs
	                    (getGroupAttr()->getCharacteristicInputs());

  // Transform the right child.
  child(1)->transformNode(normWARef, child(1)); 

  // Return to my own VEGRegion.
  normWARef.restoreOriginalVEGRegion();

  // No need to transform colMapExprList because the source and the target
  // expressions will be transformed by their own operators.

  // Pull up the predicates and recompute the required inputs
  // of whoever my children are now.
  pullUpPreds();

  // transform the selection predicates
  transformSelectPred(normWARef, locationOfPointerToMe);

  // this Union will be removed during optimization. So send the outputs
  // of its left child to the parent. That is what will happen when this
  // node is later removed.
  if ((getIsTemporary()) && 
      (getGroupAttr()->getCharacteristicOutputs().entries() == 0))
  {
    GroupAttributes * childGAPtr = child(0).getPtr()->getGroupAttr();
    getGroupAttr()->setCharacteristicOutputs(childGAPtr->getCharacteristicOutputs());
  }

  //++Triggers -
  if (getBlockedUnion())
	  normWARef.restoreInBlockedUnionCount();

} // Union::transformNode()

// -----------------------------------------------------------------------
// Union::pullUpPreds()
// -----------------------------------------------------------------------
void Union::pullUpPreds()
{

  // For a predicate to be pulled up from the children it has to
  // be part of both children. The only predicates we can detect
  // as being part of both child are those that only use correlated
  // references. and are identical (i.e. use the same value id's
  // for expressions under the tree).
  //
  // Other predicates will require a more sophisticated pattern matching.
  // 
  ValueIdSet commonPredicates(child(0)->getSelectionPred());
  commonPredicates.intersectSet(child(1)->getSelectionPred());

  selectionPred() += commonPredicates;
  child(0)->selectionPred() -= commonPredicates;
  child(1)->selectionPred() -= commonPredicates;
  child(0)->recomputeOuterReferences();
  child(1)->recomputeOuterReferences();
} // Union::pullUpPreds()

// -----------------------------------------------------------------------
// Union::recomputeOuterReferences()
// -----------------------------------------------------------------------
void Union::recomputeOuterReferences()
{
  // ---------------------------------------------------------------------
  // Delete all those input values that are no longer referenced on
  // this operator because the predicates that reference them have 
  // been pulled up.
  // ---------------------------------------------------------------------
  if (NOT getGroupAttr()->getCharacteristicInputs().isEmpty())
    {
      ValueIdSet outerRefs = getGroupAttr()->getCharacteristicInputs();
 
      // Weed out those expressions not needed by my selection predicates
      // and by my left and right children as input values.
      ValueIdSet exprSet = getSelectionPred();
      exprSet += child(0)->getGroupAttr()->getCharacteristicInputs();
      exprSet += child(1)->getGroupAttr()->getCharacteristicInputs();

      // Add conditional expression for conditional union.
      exprSet.insertList(condExpr());

      exprSet.insertList(alternateRightChildOrderExpr()); //++MV
      
      // Add the output expressions of each child that are cached in the 
      // UnionMap. If a child references an external input value in its 
      // output, i.e., the select list of the SELECT, but that external 
      // input is not referenced elsewhere in the query, then no record
      // of such a reference exists but in the UnionMap. This is so
      // because the RelRoot for the subselect under the Union, which
      // contained the select list is eliminated by transformNode().
      // When the RelRoot is eliminated, its characteristic inputs
      // are added to its children. However, Union::pullUpPreds()
      // calls recomputeOuterReferences on each child. The latter call 
      // wipes out all such external input values that are not referenced
      // elsewhere in the query. In order to ensure that such external
      // input values flow down to the Union, add them to exprSet. 
      exprSet.insertList(getLeftMap().getBottomValues());
      exprSet.insertList(getRightMap().getBottomValues());
      exprSet.weedOutUnreferenced(outerRefs);

      getGroupAttr()->setCharacteristicInputs(outerRefs);
    }
} // Union::recomputeOuterReferences()  

// -----------------------------------------------------------------------
// UnionMap::normalizeSpecificChild()
// -----------------------------------------------------------------------
void UnionMap::normalizeSpecificChild(NormWA & normWARef, Lng32 childIndex)
{
  // Normalize the maps constructed for the union, replacing
  // valueIds with VegRef's where appropriate.
  ValueIdUnion * viduPtr;  
  for (CollIndex index = 0; index < colMapTable_.entries(); index++)
    {
      viduPtr = ((ValueIdUnion *)(colMapTable_[index].getItemExpr()));
      CMPASSERT(viduPtr->getOperatorType() == ITM_VALUEIDUNION);
      viduPtr->normalizeSpecificChild(normWARef, childIndex);
    }
  
  switch (childIndex)
    {
    case 0:
      leftColMap_.normalizeNode(normWARef);
      break;
    case 1:
      rightColMap_.normalizeNode(normWARef);
      break;
    default:
      CMPASSERT(childIndex < 2);
      break;
    }
} // UnionMap::normalizeSpecificChild()

// -----------------------------------------------------------------------
// Union::rewriteNode()
// -----------------------------------------------------------------------
void Union::rewriteNode(NormWA & normWARef)
{
  // Locate the VEGRegion that I had allocated for my left child.
  normWARef.locateAndSetVEGRegion(this,0);
  
  // Normalize expressions contributed by child(0)
  child(0)->rewriteNode(normWARef);

  // Normalize expressions contributed by child(0)
  getUnionMap()->normalizeSpecificChild(normWARef, 0); 

  
  normWARef.restoreOriginalVEGRegion();

  // Locate the VEGRegion that I had allocated for my right child.
  normWARef.locateAndSetVEGRegion(this,1);
  
  // Normalize expressions contributed by child(1)
  child(1)->rewriteNode(normWARef);

  // Normalize expressions contributed by child(1)
  getUnionMap()->normalizeSpecificChild(normWARef, 1); 
  
  // ++MV
  // The alternate right child order expression should be normalized in 
  // the region of the right child
  if (alternateRightChildOrderExpr().normalizeNode(normWARef))
    {
    }
  // --MV

  normWARef.restoreOriginalVEGRegion();
  
  // Normalize the predicates.
  if (selectionPred().normalizeNode(normWARef))
    {
    }

  if (condExpr().normalizeNode(normWARef))
    {
    }

  // for embedded statements, when a blocked union is introduced by triggers
  // that will later be removed use the veg region of the left child before
  // normalizing outputs.
  if (getIsTemporary())
   normWARef.locateAndSetVEGRegion(this,0);

  // Rewrite my own Group Attributes
  getGroupAttr()->normalizeInputsAndOutputs(normWARef);
  
} // Union::rewriteNode()

// -----------------------------------------------------------------------
// Union::normalizeNode()
// -----------------------------------------------------------------------
RelExpr * Union::normalizeNode(NormWA & normWARef)
{
  if (nodeIsNormalized())
    return this;
  markAsNormalized();
  // ---------------------------------------------------------------------
  // Check which expressions can be evaluated by the children of the union.
  // Modify the Group Attributes of those children who inherit some of
  // these expressions.
  // ---------------------------------------------------------------------
  pushdownCoveredExpr(getGroupAttr()->getCharacteristicOutputs(),
                      getGroupAttr()->getCharacteristicInputs(),
                      selectionPred()
                     );

  // ---------------------------------------------------------------------
  // Normalize the left and right subtrees. Store pointers to the
  // roots of the subtrees after normalization.
  // ---------------------------------------------------------------------
  // Locate the VEGRegion that I had allocated for my left child.
  normWARef.locateAndSetVEGRegion(this, 0);


  child(0) = child(0)->normalizeNode(normWARef);

  normWARef.restoreOriginalVEGRegion();

  // Locate the VEGRegion that I had allocated for my left child.
  normWARef.locateAndSetVEGRegion(this, 1);

  child(1) = child(1)->normalizeNode(normWARef);
  
  normWARef.restoreOriginalVEGRegion();
  
  fixEssentialCharacteristicOutputs();
  normWARef.setExtraHubVertex(this);

  return this; // return a -> to self
} // Union::normalizeNode()

// -----------------------------------------------------------------------
// Union::semanticQueryOptimizeNode()
// This instance of the SQO virtual method is the same as the base class 
// implementation except that it also keeps track of which
// VEGRegion we are currently in.
// -----------------------------------------------------------------------
RelExpr * Union::semanticQueryOptimizeNode(NormWA & normWARef)
{
  // ---------------------------------------------------------------------
  // SemanticQueryOptimize the left and right subtrees. Store pointers to 
  // the roots of the subtrees after SQO.
  // ---------------------------------------------------------------------
  // Locate the VEGRegion that I had allocated for my left child.
  normWARef.locateAndSetVEGRegion(this, 0);


  child(0) = child(0)->semanticQueryOptimizeNode(normWARef);

  normWARef.restoreOriginalVEGRegion();

  // Locate the VEGRegion that I had allocated for my left child.
  normWARef.locateAndSetVEGRegion(this, 1);

  child(1) = child(1)->semanticQueryOptimizeNode(normWARef);
  
  normWARef.restoreOriginalVEGRegion();

  return this;

} // Union::semanticQueryOptimizeNode()

NABoolean Union::prepareTreeForCSESharing(
     const ValueIdSet &outputsToAdd,
     const ValueIdSet &predicatesToRemove,
     const ValueIdSet &commonPredicatesToAdd,
     const ValueIdSet &inputsToRemove,
     ValueIdSet &valuesForVEGRewrite,
     ValueIdSet &keyColumns,
     CSEInfo *info)
{
  NABoolean result = TRUE;

  // we only support UNION nodes without local predicates, which
  // should be all cases, since there should not be any predicates on
  // a UNION
  if (getSelectionPred().entries() > 0)
    {
      info->getConsumer(0)->emitCSEDiagnostics(
           "Selection predicates on union node not supported");
      return FALSE;
    }

  // recursively call this for the children
  for (CollIndex i=0; i<2 && result; i++)
    {
      ValueIdSet locOutputsToAdd(outputsToAdd);
      ValueIdSet childOutputsToAdd;
      ValueIdSet childPredsToRemove;
      ValueIdSet childPredsToAdd;
      ValueIdMap *map = (i==0 ? &getLeftMap() : &getRightMap());
      ValueIdSet availableValues(map->getTopValues());
      ValueIdSet dummyValuesForVEGRewrite;
      ValueIdSet mappedKeyColumns;
      ValueIdSet childKeyColumns;

      // if there are outputs to add, we can only do that for
      // outputs that already exist in the ValueIdMap
      availableValues += getGroupAttr()->getCharacteristicInputs();
      if (locOutputsToAdd.removeUnCoveredExprs(availableValues))
        {
          info->getConsumer(0)->emitCSEDiagnostics(
               "Not able to add output values unknown to union operator");
          result = FALSE;
        }

      map->rewriteValueIdSetDown(outputsToAdd, childOutputsToAdd);
      map->rewriteValueIdSetDown(predicatesToRemove, childPredsToRemove);
      map->rewriteValueIdSetDown(commonPredicatesToAdd, childPredsToAdd);
      
      result = child(i)->prepareTreeForCSESharing(
           childOutputsToAdd,
           childPredsToRemove,
           childPredsToAdd,
           inputsToRemove,
           dummyValuesForVEGRewrite,
           childKeyColumns,
           info);

      map->mapValueIdSetUp(mappedKeyColumns, childKeyColumns);
      // include only those that actually got mapped
      mappedKeyColumns -= childKeyColumns;
      keyColumns += mappedKeyColumns;
    }

  if (result)
    {
      NABoolean dummy;
      CollIndex nu = unionMap_->leftColMap_.getBottomValues().entries();

      getGroupAttr()->addCharacteristicOutputs(outputsToAdd);
      getGroupAttr()->removeCharacteristicInputs(inputsToRemove);

      // add columns that are a constant in at least one of the
      // UNION's children to the key columns. Such columns can be used
      // to eliminate entire legs of the union and therefore act like
      // key or partition key columns.
      for (CollIndex u=0; u<nu; u++)
        {
          if (unionMap_->leftColMap_.getBottomValues()[u].getItemExpr()->
                castToConstValue(dummy) ||
              unionMap_->rightColMap_.getBottomValues()[u].getItemExpr()->
                castToConstValue(dummy))
            keyColumns += unionMap_->colMapTable_[u];
        }
    }

  // there is no need to call prepareMeForCSESharing() here

  return result;
}

// ***********************************************************************
// $$$$ GroupByAgg
// member functions for class GroupByAgg
// ***********************************************************************
void GroupByAgg::transformNode(NormWA & normWARef,
                               ExprGroupId &locationOfPointerToMe)
{
  CMPASSERT( this == locationOfPointerToMe );

  if (nodeIsTransformed())
    return;
  markAsTransformed();

  NABoolean needsNewVEGRegion = FALSE;
  
  // ---------------------------------------------------------------------
  // Each scalar aggregate allocates a VEGRegion for "=" predicates that 
  // do not reject null values.
  // It can only import a VEG from another VEGRegion in which an "outer 
  // reference" is involved. 
  //    select empname
  //    from employee
  //	where not exists
  //	  (select branchnum
  //	   from branch
  //	   where ( employee.branchnum =  branch.branchnum)
  //	   and (branchnum = 1)
  //	   group by branchnum
  //	  );
  // It is legal to deduce that employee.branchnum = 1 within the subquery 
  // but not in the main query. 
  // ---------------------------------------------------------------------
  if (groupExpr().isEmpty())
    {
      // -----------------------------------------------------------------
      // Fix to "BR0198" (Genesis 10-000303-8476).
      // If there's no grouping expression and no aggregation expression,
      // then aggregate over a constant, i.e.,
      // make one single group (zero or one "row") of the entire table.
      // 	  See Ansi 7.8 SR 1 + GR 1 (HAVING clause).
      // 	  See /regress/fullstack/test002 cases.
      // By adding a constant to the grouping expression we are treating 
      // this as a nonScalar grby.
      // -----------------------------------------------------------------
      if (aggregateExpr().isEmpty())
	{
	  ItemExpr *tf = new(normWARef.wHeap()) ConstValue(0);
	  tf->synthTypeAndValueId(TRUE);
	  groupExpr() += tf->getValueId();
	}
      else if (NOT containsNullRejectingPredicates())
	{
	  needsNewVEGRegion = TRUE;
	  normWARef.allocateAndSetVEGRegion(IMPORT_ONLY,this);
	} 
    }

  // ---------------------------------------------------------------------
  // Transform child. Pull up its transformed predicates
  // recompute their required inputs.
  // ---------------------------------------------------------------------
  child(0)->transformNode(normWARef, child(0));

  // My child has now been transformed.
  // A new semiJoin may now be my direct descendant and my original
  // child a descendant of it.
  // In either case my child has now been transformed.

  // ---------------------------------------------------------------------
  // A Group By clause can only contain column references.
  // An aggregate function cannot contain a subquery according to SQL2.
  // However, the group by list and aggregate functions could be columns
  // from a derived table and may therefor contain subselects an all
  // sorts of nasty things. So we allow anything here.
  //
  // Subqueries in the group by list and aggregate functions should
  // introduce semijoins below the groupby and subqueries in the
  // having clause above the groupby
  //
  // Order of work should be
  //    process group by
  //    process aggregate expressions
  //    pull up predicates
  //    process having clause
  // ---------------------------------------------------------------------
  if (groupExpr().transformNode(normWARef, child(0),
                                getGroupAttr()->getCharacteristicInputs(),
                                FALSE /* Move predicates */ ) )
    {
      // The group by list apparently had some subqueries that had not been
      // processed before (scracth, scratch..). Normalize the new
      // tree that has become our child.
      child(0)->transformNode(normWARef, child(0));
  
    }


  if (aggregateExpr().transformNode(normWARef, child(0),
                                    getGroupAttr()->getCharacteristicInputs(),
                                    FALSE /* Move predicates */ ) )
    {
      // The aggregate was on a subquery that had not been
      // processed before (scracth, scratch..). Normalize the new
      // tree that has become our child.
      child(0)->transformNode(normWARef, child(0));
  
    }


  // Pull up the predicates into my having clause and recompute the
  // required inputs of whoever my children are now.
  pullUpPreds();

  if (needsNewVEGRegion)
    {
      // Restore the original VEGRegion.
      normWARef.restoreOriginalVEGRegion();
    }
  
  // transform the selection predicates
  normWARef.setInHavingClause(TRUE) ;
  transformSelectPred(normWARef, locationOfPointerToMe);
  normWARef.setInHavingClause(FALSE) ;

} // GroupByAgg::transformNode()

// -----------------------------------------------------------------------
// GroupByAgg::pullUpPreds()
// -----------------------------------------------------------------------
void GroupByAgg::pullUpPreds()
{
  // ---------------------------------------------------------------------
  // Pull up predicates from the child.
  // move them to my having clause
  // ---------------------------------------------------------------------

  // Make inputs available to child
  child(0)->getGroupAttr()->addCharacteristicInputs(getGroupAttr()->getCharacteristicInputs());

  // Parts of the rules for this virtual method is that recomputOuterRefs()
  // should be called on the child even if no predicates are pulled up
  // from it.
  child(0)->recomputeOuterReferences();

  // If this is a scalar groupby that can produce NULL values then predicates
  // cannot be moved up.
  if (groupExpr().isEmpty() && NOT containsNullRejectingPredicates())
    return;

  if (child(0)->getSelectionPred().isEmpty())
    return;

  // Only predicates that reference group by columns or
  // other input values can be pulled up.

  // We are going to prime group attributes ahead of time here so that
  // we can call coverTest() from here.
  ValueIdSet saveExternalInputs = getGroupAttr()->getCharacteristicInputs();
  primeGroupAttributes();
  ValueIdSet predicatesToPullUp, boringSet, predicatesThatStay;

  getGroupAttr()->coverTest(child(0)->selectionPred(),
                            saveExternalInputs,   // Like passing empty
                            predicatesToPullUp,
                            boringSet,
                            &predicatesThatStay);

  if (NOT predicatesToPullUp.isEmpty())
    {
      selectionPred() += predicatesToPullUp;
      child(0)->selectionPred() -= predicatesToPullUp;
      child(0)->recomputeOuterReferences();
    }

  getGroupAttr()->setCharacteristicInputs(saveExternalInputs);

} // GroupByAgg::pullUpPreds()

// -----------------------------------------------------------------------
// GroupByAgg::recomputeOuterReferences()
// -----------------------------------------------------------------------
void GroupByAgg::recomputeOuterReferences()
{
  // ---------------------------------------------------------------------
  // Delete all those input values that are no longer referenced on
  // this operator because the predicates that reference them have 
  // been pulled up.
  // ---------------------------------------------------------------------
  ValueIdSet outerRefs = getGroupAttr()->getCharacteristicInputs(); 

  ValueIdSet allMyExpr(getSelectionPred());
  allMyExpr += groupExpr();
  allMyExpr += aggregateExpr();

  allMyExpr.weedOutUnreferenced(outerRefs);

  outerRefs += child(0).getPtr()->getGroupAttr()->getCharacteristicInputs();
  getGroupAttr()->setCharacteristicInputs(outerRefs);
} // GroupByAgg::recomputeOuterReferences()  

// -----------------------------------------------------------------------
// GroupbyAgg::rewriteNode()
// -----------------------------------------------------------------------
void GroupByAgg::rewriteNode(NormWA & normWARef)
{
  // ---------------------------------------------------------------------
  // Each scalar aggregate allocates a VEGRegion for "=" predicates that 
  // do not reject null values.
  // It can only import a VEG from another VEGRegion in which an "outer 
  // reference" is involved. 
  //    select empname
  //    from employee
  //	where not exists
  //	  (select branchnum
  //	   from branch
  //	   where ( employee.branchnum =  branch.branchnum)
  //	   and (branchnum = 1)
  //	   group by branchnum
  //	  );
  // It is legal to deduce that employee.branchnum = 1 within the subquery 
  // but not in the main query. 
  // ---------------------------------------------------------------------
  NABoolean needsNewVEGRegion = FALSE;

   if (groupExpr().isEmpty() && (NOT containsNullRejectingPredicates()))
    {
      needsNewVEGRegion = TRUE;
      normWARef.locateAndSetVEGRegion(this);
    } 
  // ---------------------------------------------------------------------
  // Rewrite the expressions of the child.
  // ---------------------------------------------------------------------
  child(0)->rewriteNode(normWARef);
  // ---------------------------------------------------------------------
  // Rewrite the expressions that are grouping expressions
  // ---------------------------------------------------------------------
  if (groupExpr().normalizeNode(normWARef))
    {
    }
  // ---------------------------------------------------------------------
  // Rewrite the expressions that are rollup grouping expressions
  // ---------------------------------------------------------------------
  if (rollupGroupExprList().normalizeNode(normWARef))
    {
    }
 
  normalizeExtraOrderExpr(normWARef);

  // ---------------------------------------------------------------------
  // Rewrite the expressions that are aggregate expressions
  // ---------------------------------------------------------------------
  if (aggregateExpr().normalizeNode(normWARef))
    {
    }
  // 10-050616-8826 -BEGIN
  // If transformation has not happened then its a possiblity that
  // the "TYPE" of the ItemExpr can change. For Example case when
  // we transform outer joins to inner joins.
  if(NOT aggregateExpr().isEmpty())
  {
      ValueIdSet postExpr = aggregateExpr();
      for(ValueId exprId = postExpr.init(); postExpr.next(exprId); postExpr.advance(exprId))
      {
         const NAType &type1 = exprId.getType();
	 const NAType &type2 = exprId.getItemExpr()->child(0).getValueId().getType();
	 if( NOT(type1 == type2) )
	  {
             exprId.getItemExpr()->synthTypeAndValueId(TRUE);
          }
      }
  }
  // 10-050616-8826 -END

  // ---------------------------------------------------------------------
  // If we're enforcing an ITM_ONE_ROW on (x,y), then we can produce not
  // merely the ITM_ONE_ROW, but also x and y, so add them to our outputs.
  // For example, if the aggregate is, say,
  //    ITM_ONE_ROW(VEGRef_10(T.A,ixT.A), VEGRef_15(T.B,ixT.B))
  //    { example query: select * from S where (select A,B from T) < (100,200) }
  // then add value ids 10 and 11 to our characteristic outputs.
  // ---------------------------------------------------------------------
  ValueIdSet moreOutputs;
  getPotentialOutputValues(moreOutputs);
  getGroupAttr()->addCharacteristicOutputs(moreOutputs);
  // ---------------------------------------------------------------------
  // Restore the VEGRegion of my parent.
  // ---------------------------------------------------------------------
  if (needsNewVEGRegion)
    normWARef.restoreOriginalVEGRegion();
  // ---------------------------------------------------------------------
  // Rewrite the expressions in the HAVING clause predicate.
  // ---------------------------------------------------------------------
  if (selectionPred().normalizeNode(normWARef))
    {
    }
  // ---------------------------------------------------------------------
  // Rewrite my own Group Attributes
  // ---------------------------------------------------------------------
  getGroupAttr()->normalizeInputsAndOutputs(normWARef);
} // GroupbyAgg::rewriteNode()

// -----------------------------------------------------------------------
// GroupbyAgg::normalizeNode()
// -----------------------------------------------------------------------
RelExpr * GroupByAgg::normalizeNode(NormWA & normWARef)
{
  if (nodeIsNormalized())
    return this;
  markAsNormalized();
  
  // ---------------------------------------------------------------------
  // Each scalar aggregate allocates a VEGRegion for "=" predicates that 
  // do not reject null values.
  // It can only import a VEG from another VEGRegion in which an "outer 
  // reference" is involved. 
  //    select empname
  //    from employee
  //	where not exists
  //	  (select branchnum
  //	   from branch
  //	   where ( employee.branchnum =  branch.branchnum)
  //	   and (branchnum = 1)
  //	   group by branchnum
  //	  );
  // It is legal to deduce that employee.branchnum = 1 within the subquery 
  // but not in the main query. 
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // If one of my HAVING preds is a truth-test that always evaluates to TRUE,
  // remove it; in particular, remove IS_NOT_UNKNOWN(IS_NOT_NULL(myAggrExpr))
  // (doubtless created by dissectOutSubqueries in NormItemExpr.cpp)
  // as redundant, the aggregation already being enforced by this GroupByAgg.
  // ---------------------------------------------------------------------
  DBGSETDBG( "TRANSFORM_DEBUG" )
  DBGIF(
    unp = "";
    unp += "sel:";
    selectionPred().unparse(unp);
    unp += "\nagg:";
    aggregateExpr().unparse(unp);
  )

  ItemExpr *bottomOfTest;
  ValueIdSet &agg = aggregateExpr();
  ValueIdSet &sel = selectionPred();
  for (ValueId svid = sel.init(); sel.next(svid); sel.advance(svid))
    {
      bottomOfTest = UnLogicMayBeAnEliminableTruthTest(svid.getItemExpr(),TRUE);
      if (bottomOfTest)
        if (bottomOfTest->isAnAggregate())
          for (ValueId avid = agg.init(); agg.next(avid); agg.advance(avid))
            if (bottomOfTest == avid.getItemExpr())
              {
                DBGIF(
                  cerr << unp << endl;
                  cerr << "Eliminating aggr "<< svid << endl;
                )
                sel.subtractElement(svid);      // svid, not avid!
              }
        else
          {
            DBGIF(
              cerr << unp << endl;
              cerr << "Eliminating having-pred " << svid << endl;
            )
            sel.subtractElement(svid);
          }
    }

  // ---------------------------------------------------------------------
  // Check which expressions can be evaluated by my child.
  // Modify the Group Attributes of those children who inherit some of
  // these expressions.
  // Check if any of the HAVING clause predicates can be pushed down
  // (only when a Group By list is given).
  // ---------------------------------------------------------------------
  
  // if this is a rollup groupby, then do not pushdown having pred to
  // child node. If pushdown is done, then it might incorrectly process rows that
  // are generated during rollup groupby processing.
  // For ex:
  //  insert into t values (1);
  //  select a from t group by rollup(a) having a is not null;
  //  If 'having' pred is pushdown to scan node as a where pred, 
  //  then SortGroupBy will return all rollup groups generated 
  //  and represented as null. They will not be filtered out which
  //  they would if having pred is applied after rollup group materialization.
  //  Maybe later we can optimize so this pushdown is done if possible,
  //  for ex, if there are no 'is null/not null' having preds on grouping cols.
  if (NOT isRollup())
    {
      pushdownCoveredExpr(getGroupAttr()->getCharacteristicOutputs(),
                          getGroupAttr()->getCharacteristicInputs(),
                          selectionPred()
                          );
    }

  NABoolean needsNewVEGRegion = FALSE;

  if (groupExpr().isEmpty() && (NOT containsNullRejectingPredicates()))
  {
    needsNewVEGRegion = TRUE;
    normWARef.locateAndSetVEGRegion(this);
  } 
  // ---------------------------------------------------------------------
  // Normalize the child.
  // ---------------------------------------------------------------------
  child(0) = child(0)->normalizeNode(normWARef);

  if (needsNewVEGRegion)
    normWARef.restoreOriginalVEGRegion();

  fixEssentialCharacteristicOutputs();

  if (CmpCommon::getDefault(CASCADED_GROUPBY_TRANSFORMATION) != DF_OFF)
  {
    checkForCascadedGroupBy(normWARef);
  }
  
  return this; // return a -> to self
} // GroupbyAgg::normalizeNode()

// -----------------------------------------------------------------------
// GroupByAgg::semanticQueryOptimizeNode()
// This instance of the SQO virtual method is the same as the base class 
// implementation except that it also keeps track of which
// VEGRegion we are currently in.
// -----------------------------------------------------------------------
RelExpr * GroupByAgg::semanticQueryOptimizeNode(NormWA & normWARef)
{
  NABoolean needsNewVEGRegion = FALSE;
  if (groupExpr().isEmpty() && (NOT containsNullRejectingPredicates()))
    {
      needsNewVEGRegion = TRUE;
      normWARef.locateAndSetVEGRegion(this);
    } 

  // ---------------------------------------------------------------------
  // UnNest the child.
  // ---------------------------------------------------------------------
  child(0) = child(0)->semanticQueryOptimizeNode(normWARef);

  if (needsNewVEGRegion)
    normWARef.restoreOriginalVEGRegion();

  eliminateCascadedGroupBy(normWARef);

  return this;

} // GroupByAgg::semanticQueryOptimizeNode()


//  This method checks if we can merge multiple group by nodes that are next
//  to each other into a single group by and then marks the group by node
//  that can be eliminated so it could eliminated during the SQO phase.
//  Following are the conditions under which a bottom GB node can be eliminated
//  1) If the grouping columns of the top group by node are a subset of the
//     grouping columns of the bottom group by node.
//  2) If all the aggreate expressions of the top group by can be rewritten to
//     use the bottom values in such a way it does not change the output.
//     For now this method would handle the following aggregate expressions
//     to be rolled up.
//     SUM(SUM(a))    => SUM(a) 
//     SUM( COUNT(a)) => COUNT(a) 
//     SUM( COUNT(*)) => COUNT(*) 
//     MIN( MIN(a)) => MIN(a) 
//     MAX( MAX(a)) => MAX(a)

void GroupByAgg::checkForCascadedGroupBy(NormWA & normWARef)
{
  if (child(0)->getOperatorType() == REL_GROUPBY)
  {
    GroupByAgg *childGB = (GroupByAgg*)(child(0)->castToRelExpr());

    if ( childGB->groupExpr().contains(groupExpr())  &&
         childGB->selectionPred().isEmpty() )
    {
      NABoolean allExprsCanBeRolledup = TRUE;

      for (ValueId x = aggregateExpr().init();
           aggregateExpr().next(x)  &&
           allExprsCanBeRolledup;
           aggregateExpr().advance(x))
      {
        CMPASSERT(x.getItemExpr()->isAnAggregate());
        Aggregate *aggrExpr = (Aggregate *) x.getItemExpr();

        if (!aggrExpr->isDistinct() &&
            aggrExpr->child(0)->isAnAggregate())
        {
          Aggregate *childAggrExpr = (Aggregate *) aggrExpr->child(0)->castToItemExpr();
         
          if (!childAggrExpr->isDistinct())
          {
            switch (aggrExpr->getOperatorType())
            {
              case ITM_SUM:
                if (aggrExpr->child(0)->getOperatorType() != ITM_SUM  &&
                    aggrExpr->child(0)->getOperatorType() != ITM_COUNT)
                  allExprsCanBeRolledup = FALSE;
                break;
              case ITM_MIN:
                if (aggrExpr->child(0)->getOperatorType() != ITM_MIN)
                  allExprsCanBeRolledup = FALSE;
                break;
              case ITM_MAX:
                if (aggrExpr->child(0)->getOperatorType() != ITM_MAX)
                  allExprsCanBeRolledup = FALSE;
                break;
              case ITM_COUNT_NONULL:
                if (!normWARef.compilingMVDescriptor())
                {
                  allExprsCanBeRolledup = FALSE;
                }
                else
                  aggrExprsToBeDeleted() += x;       
                break;
              default:
                allExprsCanBeRolledup = FALSE;
                break;
            }
          }
          else
            allExprsCanBeRolledup = FALSE;
        }
        else
        {
          if (normWARef.compilingMVDescriptor() &&
              (aggrExpr->getOperatorType() == ITM_COUNT &&
                  aggrExpr->child(0)->getOperatorType() == ITM_CONSTANT))
              aggrExprsToBeDeleted() += x;       
          else 
            allExprsCanBeRolledup = FALSE;
        }
      }

      if (allExprsCanBeRolledup)
      {
        childGB->setIsMarkedForElimination(TRUE);
        normWARef.setContainsGroupBysToBeEliminated(TRUE);
      }
      else
       aggrExprsToBeDeleted().clear();
    }
  }
}


void GroupByAgg::eliminateCascadedGroupBy(NormWA & normWARef)
{
  if (child(0)->getOperatorType() == REL_GROUPBY)
  {
    GroupByAgg *childGB = (GroupByAgg*)(child(0)->castToRelExpr());
    short value = 1;
          
    if (childGB->isMarkedForElimination())
    {
      for (ValueId y = aggrExprsToBeDeleted().init();
           aggrExprsToBeDeleted().next(y);
           aggrExprsToBeDeleted().advance(y))
      {
        ItemExpr *constValue = new (CmpCommon::statementHeap()) 
                    SystemLiteral(&(y.getType()), &value, sizeof(short));

        y.replaceItemExpr(constValue);
        constValue->synthTypeAndValueId();
      }

      aggregateExpr() -= aggrExprsToBeDeleted();

      aggrExprsToBeDeleted().clear();

      for (ValueId x = aggregateExpr().init();
           aggregateExpr().next(x);
           aggregateExpr().advance(x))
      {
        CMPASSERT(x.getItemExpr()->isAnAggregate());

        Aggregate *aggrExpr = (Aggregate *) x.getItemExpr();

        CMPASSERT(aggrExpr->child(0)->isAnAggregate())

        if (aggrExpr->getOperatorType() == ITM_SUM &&
            aggrExpr->child(0)->getOperatorType() == ITM_COUNT)
        {
          aggrExpr->setOperatorType(ITM_COUNT);

          // Need to update the type as well
          const NAType &origSumType = x.getType();
          const NAType &origCountType = aggrExpr->child(0)->getValueId().getType();

          // here we change the type of the old SUM(), now new COUNT() to that
          // of the original count. This to prevent numeric overflow error.
          // See solution: 10-100514-0329.

          x.changeType(&origCountType);

          // Ideally we should put in a cast node to cast the new count
          // type back to the original sum type to maintain the properties
          // of the original valueId, but since groupBys only outputs
          // valueIds from the aggregateExpr or groupExpr we can't do this
          // here. Cast is not an aggregate function, so it cannot go in the
          // aggregate expression, and if we group by it we will change the
          // the meaning of the groupby.
          // so for now we assume we will be ok since Numeric(19) and largeInt
          // are roughly eqivalent.
          // ItemExpr * castNode = 
          //              new(newNormWA.wHeap()) Cast((x.getItemExpr(),
          //                                           &(origSumType));
        }



        aggrExpr->child(0) = aggrExpr->child(0)->child(0);
      
      }

      child(0) = child(0)->child(0);
    }
  }
}

NABoolean GroupByAgg::prepareMeForCSESharing(
     const ValueIdSet &outputsToAdd,
     const ValueIdSet &predicatesToRemove,
     const ValueIdSet &commonPredicatesToAdd,
     const ValueIdSet &inputsToRemove,
     ValueIdSet &valuesForVEGRewrite,
     ValueIdSet &keyColumns,
     CSEInfo *info)
{
  // The caller of this method took care of most adjustments to
  // make. The main thing the groupby node needs to do is to add any
  // outputs that are required to its characteristic outputs.

  ValueIdSet myAvailableValues(groupExpr_);
  ValueIdSet referencedValues;
  ValueIdSet myOutputsToAdd;
  ValueIdSet unCoveredExpr;

  myAvailableValues += aggregateExpr_;
  valuesForVEGRewrite += aggregateExpr_;

  // The caller may be asking for expressions on columns, maybe
  // even an expression involving grouping columns and aggregates
  // and multiple tables, therefore use the isCovered method to
  // determine those subexpressions that we can produce here.
  NABoolean allCovered =
    outputsToAdd.isCovered(myAvailableValues,
                           *(getGroupAttr()),
                           referencedValues,
                           myOutputsToAdd,
                           unCoveredExpr);

  if (allCovered)
    myOutputsToAdd = outputsToAdd;

  getGroupAttr()->addCharacteristicOutputs(myOutputsToAdd);

  return TRUE;
}


// ***********************************************************************
// $$$$ Scan
// member functions for class Scan
// ***********************************************************************
// -----------------------------------------------------------------------
// Scan::transformNode()
// -----------------------------------------------------------------------
void Scan::transformNode(NormWA & normWARef,
                         ExprGroupId & locationOfPointerToMe)
{
  CMPASSERT( this == locationOfPointerToMe );

  if (nodeIsTransformed())
    return;
  markAsTransformed();

  // ---------------------------------------------------------------------
  // Transform the entire column list of the base table to pick up
  // equivalences of base table columns and index columns
  // ---------------------------------------------------------------------
  const ValueIdList &allCols = getTableDesc()->getColumnList();
  ItemExpr *oldPtr;
  ExprValueId newPtr;

  for (CollIndex i = 0; i < allCols.entries(); i++)
    {
      oldPtr = allCols[i].getItemExpr();
      newPtr = oldPtr;
      oldPtr->transformNode(normWARef, newPtr, locationOfPointerToMe,
                            getGroupAttr()->getCharacteristicInputs());
      // the column list shouldn't be changed by the transformation
      CMPASSERT(oldPtr == newPtr.getPtr());
      // ---------------------------------------------------------------------
      // Create a VEG with all equivalent index columns and equivalent columns
      // ---------------------------------------------------------------------
      if (oldPtr->getOperatorType() == ITM_BASECOLUMN)
        {
	  const ValueIdSet &eic = ((BaseColumn *)oldPtr)->getEIC();
	  for (ValueId eqVid = eic.init(); eic.next(eqVid); eic.advance(eqVid))
	    {
	      normWARef.addVEG(((BaseColumn *)oldPtr)->getValueId(),eqVid);
	    }

          //check if this is an clustering key column
          NABoolean isClusteringKeyColumn = FALSE;
          ValueIdList ckColumns = getTableDesc()->getClusteringIndex()
                                                         ->getIndexKey();

          for (CollIndex j=0; j < ckColumns.entries(); j++)
          {
            if (allCols[i].getNAColumn()->getPosition() == 
                      ckColumns[j].getNAColumn()->getPosition())
            {
              isClusteringKeyColumn = TRUE;
              break;
            }
          }

          // If it is a nullable clustering key column and there are indexes
          // then set the special nulls flag to TRUE so that during an index
          // join the equality predicate between the clustering key
          // of the base and the index does reutrn NULL equals NULL
          // as TRUE and so finds the base table row in the index table.
          if ( isClusteringKeyColumn &&
               allCols[i].getType().supportsSQLnull() &&
               eic.entries() > 0 ) 
          {
            ItemExpr * vegrefPtr = normWARef.getVEGReference(allCols[i]);
            if (vegrefPtr)
              ((VEGReference *)vegrefPtr)->getVEG()->setSpecialNulls(TRUE);
          }
        }
      else
	CMPASSERT(oldPtr->getOperatorType() == ITM_BASECOLUMN); 
    }

  // transform the selection predicates
  transformSelectPred(normWARef, locationOfPointerToMe);

} // Scan::transformNode()

// -----------------------------------------------------------------------
// Scan::rewriteNode()
// -----------------------------------------------------------------------
void Scan::rewriteNode(NormWA & normWARef)
{
  const ValueIdList &allCols = getTableDesc()->getColumnList();
  ItemExpr *newPtr = NULL;

  // ---------------------------------------------------------------------
  // walk through all the columns of the table, normalizing them
  // and adding the result into the ColumnVEGList of the table descriptor
  // ---------------------------------------------------------------------
  CollIndex i = 0;
  for (i = 0; i < allCols.entries(); i++)
    {
      // ---------------------------------------------------------------------
      // Create a VEG with all equivalent index columns
      // ---------------------------------------------------------------------
      newPtr = allCols[i].getItemExpr()->normalizeNode(normWARef);
      getTableDesc()->addToColumnVEGList(newPtr->getValueId());
    }
  // -------------------------------------------------------------------------
  // Normalize the indexes.
  // -------------------------------------------------------------------------
  for (i = 0;
       i < (Int32)getTableDesc()->getIndexes().entries();
       i++)
    {
      IndexDesc *idesc = getTableDesc()->getIndexes()[i];
      ValueIdList indexOrder(idesc->getOrderOfKeyValues());

      // ---------------------------------------------------------------------
      // Normalize the asc/desc order of the index.
      // ---------------------------------------------------------------------
      indexOrder.normalizeNode(normWARef);
      idesc->setOrderOfKeyValues(indexOrder);

      // ---------------------------------------------------------------------
      // Normalize the partitioning keys in the partitioning function.
      // ---------------------------------------------------------------------
      if (idesc->isPartitioned())
        idesc->getPartitioningFunction()->normalizePartitioningKeys(normWARef);
    }

  // -------------------------------------------------------------------------
  // Normalize the Vertical Partitions.
  // -------------------------------------------------------------------------
  for (i = 0;
       i < (Int32)getTableDesc()->getVerticalPartitions().entries();
       i++)
    {
      IndexDesc *idesc = getTableDesc()->getVerticalPartitions()[i];
      ValueIdList indexOrder(idesc->getOrderOfKeyValues());

      // ---------------------------------------------------------------------
      // Normalize the asc/desc order of the index.
      // ---------------------------------------------------------------------
      indexOrder.normalizeNode(normWARef);
      idesc->setOrderOfKeyValues(indexOrder);

      // ---------------------------------------------------------------------
      // Normalize the partitioning keys in the partitioning function.
      // ---------------------------------------------------------------------
      // Vertically partitioned tables always have a partitioning
      // function, even if there is only one horizontal partition.
      //
      idesc->getPartitioningFunction()->normalizePartitioningKeys(normWARef);
    }

  // QSTUFF
  // we need to normalize the potential outputs here to avoid problems 
  // during code generation
  potentialOutputs_.normalizeNode(normWARef);
  // QSTUFF

  // ---------------------------------------------------------------------
  // Rewrite the expressions in the selection predicates and
  // in the Group Attributes.
  // ---------------------------------------------------------------------
  RelExpr::rewriteNode(normWARef);
  
} // Scan::rewriteNode()

// -----------------------------------------------------------------------
// Scan::recomputeOuterReferences()
// -----------------------------------------------------------------------
// void Scan::recomputeOuterReferences()
//
// No virtual method needed
//
// Scan::recomputeOuterReferences()  

// -----------------------------------------------------------------------
// Scan::normalizeNode()
// -----------------------------------------------------------------------
RelExpr * Scan::normalizeNode
  ( NormWA & normWARef )
{
  if (nodeIsNormalized())
    return this;

  RelExpr::normalizeNode(normWARef);

 if(CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) != DF_OFF &&
    !normWARef.inMVQueryRewrite())
 {
    ValueIdSet vs,vs1;
    ValueId exprId;
    ItemExpr *inputItemExprTree = NULL;
    ValueIdList selectionPredList(selectionPred()); 
    inputItemExprTree = selectionPredList.rebuildExprTree(ITM_AND,FALSE,FALSE);
    CollHeap *heap = normWARef.wHeap();
    QRDescGenerator* descGenerator = new (heap) QRDescGenerator(false, heap);
    if (CmpCommon::getDefault(MVQR_LOG_QUERY_DESCRIPTORS) == DF_DUMP_MV)
      // Used for generating MV descriptors for queries in workload analysis mode.
      descGenerator->setDumpMvMode();
    // Desc generator needs equality sets or mvqr won't set range bitmap
    // correctly for equijoin operands with additional range predicates.
    descGenerator->createEqualitySets(selectionPred());
    ItemExpr *result = NULL;
    ItemExpr *ie = NULL ;
    if( inputItemExprTree != NULL )
    {
      NABoolean transStatus = FALSE;
      result = applyAssociativityAndCommutativity(descGenerator,heap,
						  inputItemExprTree, normWARef,
						  transStatus);
      if(transStatus)
      {
	// result->synthTypeAndValueId(); // You can not remove it, it causes regression (however Bob told in Code review) for case core/test029
	// delete from T29xv2 where j like 'f%';		-- ok ->ValueId of AND node is not available.

	result->convertToValueIdSet(vs, NULL, ITM_AND);
	if(CmpCommon::getDefault(RANGESPEC_TRANSFORMATION) == DF_MINIMUM )
	{
	  for (exprId = vs.init(); vs.next(exprId); vs.advance(exprId))
	  {
	    ie = exprId.getItemExpr()->removeRangeSpecItems(&normWARef);
	    if (ie->getOperatorType() == ITM_AND)
	    {
	      OperatorTypeEnum op = ie->child(0)->getOperatorType();
	      if ( (op == ITM_GREATER_EQ) ||(op == ITM_GREATER) ||
		  (op == ITM_LESS) ||(op == ITM_LESS_EQ))
	      {
		if(!((BiRelat*)ie->child(0).getPtr())->derivativeOfLike())
		{
		  vs1.insert(ie->child(0)->getValueId());
		  vs1.insert(ie->child(1)->getValueId());
		  continue ;
		}
	      }
	    }
	    vs1.insert(ie->getValueId());
	  }
	  vs.clear();
	  vs += vs1 ;
	}

	//doNotReplaceAnItemExpressionForLikePredicates(result,vs,result);
	vs.normalizeNode(normWARef);
	setSelectionPredicates(vs);


	// For testing purpose:
	//  ValueIdList selectionPredList1(vs); 
	//  ItemExpr * inputItemExprTree0 = selectionPredList1.rebuildExprTree(ITM_AND,FALSE,FALSE);
	//  oldTree = revertBackToOldTree(heap,inputItemExprTree0);
	//  oldTree->convertToValueIdSet(leafs, NULL, ITM_AND);
	//  doNotReplaceAnItemExpression(oldTree,leafs,oldTree);
      }
    }
  }
  
  // the following block of code can transform an OR predicate into
  //    semijoin(Scan, TupleList)
  // where the Scan is this scan node.
  // The transformation is in general guarded by tight heuristics
  // so that OR preds can be evaluated using a hash table (code in generator)
  // selection preds of a scan node can be affected by this code block.
  ValueIdSet & preds = selectionPred();
  ValueId exprId;
  ItemExprList valuesListIE(normWARef.wHeap());
  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context());
  ExprGroupId newJoin = this;
  ItemExpr *retItemExpr = NULL;
  ValueId colVid;
  Lng32 numParams;
  if (normWARef.getMergeUpdDelCount() == 0)
  for (exprId = preds.init(); preds.next(exprId); preds.advance(exprId))
  {
    if (exprId.getItemExpr()->canTransformToSemiJoin(valuesListIE, 
                         getTableDesc(), numParams, colVid, normWARef.wHeap()))
    {
      // it is an OR pred. that meets the basic correctness conditions
      
      if (!passSemiJoinHeuristicCheck(exprId, valuesListIE.entries(), numParams, colVid))
      {
        continue;  // did not satisfy heuristics
      }
    
      TupleList * tl = new(normWARef.wHeap()) 
        TupleList(valuesListIE.convertToItemExpr(RIGHT_LINEAR_TREE));
      tl->setCreatedForInList(TRUE);
      RelRoot * rr = new (normWARef.wHeap()) RelRoot(tl);
      retItemExpr = new (normWARef.wHeap()) 
        QuantifiedComp(ITM_EQUAL_ANY, colVid.getItemExpr(), rr, FALSE);
      ((QuantifiedComp*)retItemExpr)->setCreatedFromINlist(TRUE);
      retItemExpr->bindNode(&bindWA);
      if(bindWA.errStatus())
      {	  
        CmpCommon::diags()->clear();
	bindWA.resetErrStatus();
        continue ;
      }
      ExprValueId nePtr(retItemExpr);  

      retItemExpr->transformNode(normWARef, nePtr, 
                  newJoin, getGroupAttr()->getCharacteristicInputs());
     if(!(newJoin->getOperator().match(REL_SEMITSJ)))
       continue ;
     // is an OR pred that passed the heuristics check
     preds.remove(exprId);
    }
  }

  // we have changed the tree and introduced at least one semijoin.
  if ((RelExpr *)newJoin != this)
  {
    ((RelExpr *)newJoin)->getGroupAttr()->setCharacteristicOutputs
    (getGroupAttr()->getCharacteristicOutputs());
    ((RelExpr *)newJoin)->getGroupAttr()->setCharacteristicInputs
    (getGroupAttr()->getCharacteristicInputs());

    primeGroupAttributes();
    getGroupAttr()->normalizeInputsAndOutputs(normWARef);

    ExprGroupId eg(newJoin);
    newJoin->transformNode(normWARef,eg);
    newJoin = newJoin->normalizeNode(normWARef);
  }
  
  TableDesc * tableDesc = getTableDesc();
  // Make sure we rewrite the computedColumn Expressions
  const ValueIdList &allCols = tableDesc->getColumnList();
  ItemExpr *iePtr;
  CollIndex i = 0;
  for (i = 0; i < allCols.entries(); i++)
  {
    iePtr = allCols[i].getItemExpr();
    if (((BaseColumn *) iePtr)->getNAColumn()->isComputedColumn())
    {
      BaseColumn *bc = ((BaseColumn *) iePtr);
      ItemExpr *ccExpr = bc->getComputedColumnExpr().getItemExpr();

      ccExpr = ccExpr->normalizeNode(normWARef);
      bc->setComputedColumnExpr(ccExpr->getValueId());
    }
  }

  SelectivityHint * selHint = tableDesc->selectivityHint();

  if (selHint)
  {
    selHint->setLocalPreds(getSelectionPredicates());
  }

  CardinalityHint * cardHint = tableDesc->cardinalityHint();

  if (cardHint)
  {
    cardHint->setLocalPreds(getSelectionPredicates());
  }
  return ((RelExpr *)newJoin);
} // Scan::normalizeNode()

NABoolean Scan::prepareMeForCSESharing(
     const ValueIdSet &outputsToAdd,
     const ValueIdSet &predicatesToRemove,
     const ValueIdSet &commonPredicatesToAdd,
     const ValueIdSet &inputsToRemove,
     ValueIdSet &valuesForVEGRewrite,
     ValueIdSet &keyColumns,
     CSEInfo *info)
{
  // The caller of this method took care of most adjustments to
  // make. The main thing the scan node needs to do is to add any
  // outputs that are required to its characteristic outputs.

  ValueIdSet myColSet(getTableDesc()->getColumnVEGList());
  ValueIdSet referencedCols;
  ValueIdSet myOutputsToAdd;
  ValueIdSet unCoveredExpr;

  // The caller may be asking for expressions on columns, maybe
  // even an expression involving multiple tables, therefore use
  // the isCovered method to determine those subexpressions that we
  // can produce here.
  outputsToAdd.isCovered(myColSet,
                         *(getGroupAttr()),
                         referencedCols,
                         myOutputsToAdd,
                         unCoveredExpr);

  getGroupAttr()->addCharacteristicOutputs(myOutputsToAdd);
  valuesForVEGRewrite.insertList(getTableDesc()->getColumnList());

  keyColumns.insertList(getTableDesc()->getClusteringIndex()->getIndexKey());

  return TRUE;
}

/* This method applies a long list of heuristics to determine whether
 its better to use a semijoin to evaluate the OR pred or if we should
 wait till the generator and use the hash table implementation
OR_PRED_TO_SEMIJOIN = 0 ==> semijoin trans is turned OFF
OR_PRED_TO_SEMIJOIN = <val1>==> semijoin trans kicks in if 
   a. hash table transformation does not apply for some reason and
   b. number of literals in OR pred > <val1>
default is 25.

OR_PRED_TO_JUMPTABLE = 0 ==> hash table trans is turned OFF in generator
OR_PRED_TO_JUMPTABLE = <val2> ==> hash table implemenation shuts OFF for in lists 
larger than this size. default value is 5,000

OR_PRED_TO_SEMIJOIN_TABLE_MIN_SIZE : The key column heuristic applies only if table
has more rows than this setting. Default is 10000. The key column heuristic says that 
semi join transformation can give a good plan only if number of rows read by probes coming
in less than small fraction of a big table.

OR_PRED_TO_SEMIJOIN_PROBES_MAX_RATIO : Relevant only to the key column heuristic. 
This default specifies the ratio specified in the previous comment. 
The default value is 0.10. Currently join preds on key columns and multiple IN 
lists on key columns are not handled well by the key col heuristic.

The other heuristic checked here relates to the partioning key. If the in list size 
is less than half the number of partitions and the partitioning key is covered by 
equality preds then we figue that it is better to do the semijoin transformation and 
open only a few partitions. Opening a few partitions and sending on avg. one probe to each
one (total number of probes is guaranteed to be less than half the number of partitions)
is better than opening all the partitions and scanning the entire table once.

The first argument vid is the giant OR predicate that we already know meets all
logical criteria for transformation to semijoin.
*/
NABoolean Scan::passSemiJoinHeuristicCheck(ValueId vid, Lng32 numValues, 
                                     Lng32 numParams, ValueId colVid) const
{
  Lng32 orPredToSemiJoin = 
    ActiveSchemaDB()->getDefaults().getAsLong(OR_PRED_TO_SEMIJOIN);
  Lng32 orPredToJumpTable = 
    ActiveSchemaDB()->getDefaults().getAsLong(OR_PRED_TO_JUMPTABLE);
  Lng32 orPredToSemiJoinTableMinSize = 
    ActiveSchemaDB()->getDefaults().getAsLong(OR_PRED_TO_SEMIJOIN_TABLE_MIN_SIZE);
  float orPredToSemiJoinMaxRatio ;
  ActiveSchemaDB()->getDefaults().getFloat(OR_PRED_TO_SEMIJOIN_PROBES_MAX_RATIO, 
                                            orPredToSemiJoinMaxRatio);

  if (orPredToSemiJoin == 0)  // feature is turned OFF
    return FALSE;

  // if pcode is not available then the hash table implentation does not
  // apply. Be more aggressive with semijoin trans.
   DefaultToken pcodeOptLevel = CmpCommon::getDefault(PCODE_OPT_LEVEL);
   NABoolean unSupportedType = FALSE;
   NABoolean noPCodeSupport = FALSE;
   UInt32 optFlags = (UInt32)CmpCommon::getDefaultLong(PCODE_OPT_FLAGS);
   if (((optFlags & PCodeCfg::INDIRECT_BRANCH) == 0) ||
       (pcodeOptLevel == DF_OFF) || (pcodeOptLevel == DF_MINIMUM))
   {
       noPCodeSupport = TRUE;
   }
   if (colVid.getType().getTypeQualifier() == NA_NUMERIC_TYPE)
   {
    const NumericType &ntype = (NumericType &)colVid.getType() ;
    if (ntype.isBigNum() || ntype.isDecimal() || (ntype.getScale() > 0))
      unSupportedType = TRUE;
   }
   if (numValues > orPredToSemiJoin)  // num of in list values still has to be
   {   // greater than OR_PRED_TO_SEMIJOIN
      if ( noPCodeSupport ||
      (orPredToJumpTable == 0) || (orPredToJumpTable < numValues)|| // hash table imp. is OFF or In list VERY large
      (numParams > orPredToSemiJoin) || // params not supported hash table imp.
      unSupportedType )
        return TRUE;
   }

  NABoolean isBigTable = FALSE;
  CostScalar totalRowCount = getTableDesc()->getTableColStats()[0]->getColStats()->getRowcount();
  if (totalRowCount > orPredToSemiJoinTableMinSize)
    isBigTable = TRUE;

  // We do cycle through all indexes of the base table though 
  // there is no guarantee that the index we base our decision upon here
  // will be chosen by the optimizer.
  const LIST(IndexDesc *) & ixlist = getTableDesc()->getIndexes();
  for (CollIndex ix =0; ix < ixlist.entries(); ix++)
  {
    IndexDesc* idesc = ixlist[ix];
    ValueIdList keyCols, partKeyCols;
    getTableDesc()->getEquivVEGCols(idesc->getIndexKey(), keyCols);
    getTableDesc()->getEquivVEGCols(idesc->getPartitioningKey(), partKeyCols);
    CollIndex keyColIndex = keyCols.index(colVid);
    CollIndex partKeyColIndex = partKeyCols.index(colVid);

    if (partKeyColIndex != NULL_COLL_INDEX) // 'a' is a partitioning key column
    {
      NABoolean applyPartKeyHeuristic = FALSE;
      if ((numValues <  0.5*(idesc->getNAFileSet()->getCountOfPartitions())) &&
	  isBigTable && (numValues > orPredToSemiJoin))
      {
        // number of clauses in IN List is less than half the number of partitions
        applyPartKeyHeuristic = TRUE;
      }
      for (CollIndex i =0; 
          (applyPartKeyHeuristic && (i < partKeyCols.entries())); i++)
      {
        if (i == partKeyColIndex)
          continue ;
        if (!partKeyCols[i].getItemExpr()->doesExprEvaluateToConstant(FALSE,TRUE)) // equality preds on all part key columns, except 'a'
        {
          applyPartKeyHeuristic = FALSE;
        }
      }
      if (applyPartKeyHeuristic)
        return TRUE;
    }

    ItemExpr* ie;
    if ((keyColIndex != NULL_COLL_INDEX)&& isBigTable)  // 'a' is a key column of this index
    {
      NABoolean fullKeyConstant = TRUE;
      NABoolean keyConstantUptoCol = TRUE;
      for (CollIndex i =0; i < keyCols.entries(); i++)
      {
        if (i == keyColIndex)
          continue ;
        ie = keyCols[i].getItemExpr();

        if (!(ie->doesExprEvaluateToConstant(FALSE,TRUE))) // equality preds on all key columns
        {
	  if (i < keyColIndex)
	  {
	    fullKeyConstant = FALSE;
	    keyConstantUptoCol = FALSE;
	  }
	  else
	  {
	    fullKeyConstant = FALSE;
	  }
	  break;
        }
      }
      if (fullKeyConstant)
        return TRUE;
      if (keyConstantUptoCol && (numValues > orPredToSemiJoin))
	return TRUE;

      // the following block separates out the key predicates from the selection
      // preds of this scan. Then we estimated the number of rows that will
      // reult after applying these key predicates. Only local predicates are
      // considered. OR preds on key columns, join preds on key columns, etc.
      // are not included in this computation. Hopefully these pred types can also be 
      // considered eventually. Code is mostly a copy of AppliedStatMan::getStatsForCANodeId()

      ValueIdSet nonKeyPredicates (getSelectionPred());
      ValueIdSet externalInputs = getGroupAttr()->getCharacteristicInputs();
      ValueIdSet nonKeyColumnSet;
      idesc->getNonKeyColumnSet(nonKeyColumnSet);
      SearchKey * skey = new(CmpCommon::statementHeap())
        SearchKey (idesc->getIndexKey(),
               idesc->getOrderOfKeyValues(),
               externalInputs, TRUE,
               nonKeyPredicates,
               nonKeyColumnSet,
               idesc);

    const CorrName& name = getTableDesc()->getNATable()->getTableName();
    Scan *scanExpr = new STMTHEAP Scan(name, getTableDesc(), REL_SCAN, STMTHEAP);
    scanExpr->setBaseCardinality((Cardinality)totalRowCount.getValue()) ;

    GroupAttributes * gaExpr = new STMTHEAP GroupAttributes();
    scanExpr->setSelectionPredicates(skey->keyPredicates());
    gaExpr->setCharacteristicOutputs(getGroupAttr()->getCharacteristicOutputs());
    scanExpr->setGroupAttr(gaExpr);
    gaExpr->setLogExprForSynthesis(scanExpr);

    EstLogPropSharedPtr outputEstLogProp = scanExpr->getGroupAttr()->outputLogProp((*GLOBAL_EMPTY_INPUT_LOGPROP));
    CostScalar keyPredRowCount = outputEstLogProp->getResultCardinality() ;  
    delete skey;
    delete scanExpr; // gaExpr is deleted here too
     
    if (( keyPredRowCount < ((CostScalar)orPredToSemiJoinMaxRatio)*totalRowCount)&&
         (numValues > orPredToSemiJoin))
        return TRUE; // ratio of rows chosen by keypreds is less than specified 
    // by the default OR_PRED_TO_SEMIJOIN_PROBES_MAX_RATIO
    }  // end of isBigTable IF block
  } // end of loop over all index paths

  // part key and key column heuristic did not apply
  return FALSE;
}



// ***********************************************************************
// $$$$ Tuple
// methods for class Tuple
// ***********************************************************************

// ***********************************************************************
// $$$$ GenericUpdate
// member functions for class GenericUpdate
// *********************************************************************** 
void GenericUpdate::transformNode(NormWA & normWARef,
                                  ExprGroupId & locationOfPointerToMe)
{
  CMPASSERT( this == locationOfPointerToMe );

  if (nodeIsTransformed())
    return;
  markAsTransformed();

  // ---------------------------------------------------------------------
  // Transform the child,
  // unless it's a leaf op introduced by Binder Index Maintenance.
  // ---------------------------------------------------------------------
  ValueId val_id;
  if (child(0)) {
    // Make values available to child    
    child(0)->getGroupAttr()->addCharacteristicInputs
                        (getGroupAttr()->getCharacteristicInputs());
    child(0)->transformNode(normWARef, child(0)); 
  } else
    CMPASSERT(getOperator().match(REL_ANY_LEAF_GEN_UPDATE));
  
  // only if update and scan on the same table,
  // i.e. no temp tables are involved 
  if (((getOperatorType() == REL_UNARY_UPDATE ||
        getOperatorType() == REL_UNARY_DELETE))){
    
    if(child(0)->getOperatorType() == REL_SCAN) {
      Scan * scanNode = (Scan *)(child(0)->castToRelExpr());
      const NATable *scanTable = scanNode->getTableDesc()->getNATable();
      if(scanTable->getSpecialType() != ExtendedQualName::TRIGTEMP_TABLE){
	ValueIdList topValueIds = oldToNewMap().getTopValues();
	ValueIdList bottomValueIds = oldToNewMap().getBottomValues();
    
	for (CollIndex v = 0; v < topValueIds.entries();v++){
	  normWARef.addVEG(topValueIds[v],bottomValueIds[v]);
	}
      }
    }
  }


  // ---------------------------------------------------------------------
  // Transform the computable expressions associated with me.
  // If a subquery appears in the compute list, then let the subquery
  // transformation cause a semijoin to be performed between the 
  // child of the GenericUpdate and the GenericUpdate.
  // ---------------------------------------------------------------------
  NABoolean origInGenericUpdateAssignFlag(normWARef.inGenericUpdateAssign());

  normWARef.setInGenericUpdateAssign(TRUE);
  if (newRecExpr().transformNode(normWARef, child(0),
                                 getGroupAttr()->getCharacteristicInputs(),
                                 FALSE /* Move predicates */) )
    {
      normWARef.setInGenericUpdateAssign(origInGenericUpdateAssignFlag) ;
      // -----------------------------------------------------------------
      // Transform my new child.
      // -----------------------------------------------------------------
      child(0)->transformNode(normWARef, child(0));
    }

  normWARef.setInGenericUpdateAssign(origInGenericUpdateAssignFlag) ;

  normWARef.setInGenericUpdateAssign(TRUE);
  // QSTUFF
  if (newRecBeforeExpr().transformNode(normWARef, child(0),
                                 getGroupAttr()->getCharacteristicInputs(),
                                 FALSE /* Move predicates */) )
    {
      normWARef.setInGenericUpdateAssign(origInGenericUpdateAssignFlag) ;
      // -----------------------------------------------------------------
      // Transform my new child.
      // -----------------------------------------------------------------
      child(0)->transformNode(normWARef, child(0));
    }
  normWARef.setInGenericUpdateAssign(origInGenericUpdateAssignFlag) ;
  // QSTUFF

  if (isMerge())
    {
      normWARef.setInGenericUpdateAssign(TRUE) ;
      if (mergeInsertRecExpr().transformNode(normWARef, child(0),
					getGroupAttr()->getCharacteristicInputs(),
					FALSE /* Move predicates */) )
	{
          normWARef.setInGenericUpdateAssign(origInGenericUpdateAssignFlag) ;
	  // -----------------------------------------------------------------
	  // Transform my new child.
	  // -----------------------------------------------------------------
	  child(0)->transformNode(normWARef, child(0));
	}
      normWARef.setInGenericUpdateAssign(origInGenericUpdateAssignFlag) ;

      // remember previous "are we in mergeUpdateWhere?" flag 
      NABoolean origInMergeUpdWhere(normWARef.inMergeUpdWhere());
      normWARef.setInMergeUpdWhere(TRUE); // we're in a mergeUpdateWhere
      if (mergeUpdatePred().transformNode
          (normWARef, child(0),	getGroupAttr()->getCharacteristicInputs(),
           FALSE /* Move predicates */) )
	{ // restore previous "are we in mergeUpdateWhere?" flag
          normWARef.setInMergeUpdWhere(origInMergeUpdWhere) ;
	  // Transform my new child.
	  child(0)->transformNode(normWARef, child(0));
	}
      // restore previous "are we in mergeUpdateWhere?" flag
      normWARef.setInMergeUpdWhere(origInMergeUpdWhere) ;
    }

  ValueId exprId;
  for (exprId = newRecExpr().init(); newRecExpr().next(exprId); newRecExpr().advance(exprId))
  {
    ItemExpr *thisIE = exprId.getItemExpr();
    thisIE = thisIE->removeOneRowAggregate( thisIE, normWARef );
  }

  // QSTUFF
    for (exprId = newRecBeforeExpr().init(); newRecBeforeExpr().next(exprId); newRecBeforeExpr().advance(exprId))
  {
    ItemExpr *thisIE = exprId.getItemExpr();
    thisIE = thisIE->removeOneRowAggregate( thisIE, normWARef );
  }
  // QSTUFF

  for (exprId = mergeInsertRecExpr().init(); mergeInsertRecExpr().next(exprId); mergeInsertRecExpr().advance(exprId))
    {
      ItemExpr *thisIE = exprId.getItemExpr();
      thisIE = thisIE->removeOneRowAggregate( thisIE, normWARef );
    }

  for (exprId = mergeUpdatePred().init(); 
       mergeUpdatePred().next(exprId); 
       mergeUpdatePred().advance(exprId))
    {
      ItemExpr *thisIE = exprId.getItemExpr();
      thisIE = thisIE->removeOneRowAggregate( thisIE, normWARef );
    }

  // ---------------------------------------------------------------------
  // For key expressions only normalize the right hand side of the =
  // left side should have been a different valueId from the one below
  // ---------------------------------------------------------------------
  ValueIdList keyList = beginKeyPred();
  if (keyList.entries() > 0)
    {
      for (CollIndex i = 0; i < keyList.entries(); i++)
        {
          ItemExpr * eqPtr = ((keyList[i]).getValueDesc())->getItemExpr();
          (*eqPtr)[1]->transformNode(normWARef, eqPtr->child(1), child(0),
                                     getGroupAttr()->getCharacteristicInputs());
          (*eqPtr)[0]->markAsTransformed();
          eqPtr->markAsTransformed();
        }
    }    

  // ---------------------------------------------------------------------
  // ---------------------------------------------------------------------
  beginKeyPred().transformNode(normWARef, child(0),
                               getGroupAttr()->getCharacteristicInputs());



  // ---------------------------------------------------------------------
  // Transform the check constraint expressions.
  // Indicate that we are processing a complex scalar expression to
  // suppress the performance of transitive closure.
  // ---------------------------------------------------------------------
  normWARef.setComplexScalarExprFlag();
  normWARef.setInConstraintsFlag();

  checkConstraints().transformNode(normWARef, child(0),
                                   getGroupAttr()->getCharacteristicInputs());

  normWARef.restoreComplexScalarExprFlag();
  normWARef.restoreInConstraintsFlag();

  // There should be no select predicates here,
  // except if it's an embedded insert.

  if (!getGroupAttr()->isEmbeddedInsert())
    {
      CMPASSERT(selectionPred().isEmpty() 
		// QSTUFF
		OR getGroupAttr()->isGenericUpdateRoot()
		// QSTUFF
		
		); 
    }
  // fix CR: message bytes increase with rowsets (CR 10-010720-4032) 
  if ( child(0) )
   child(0)->recomputeOuterReferences();
  
  // QSTUFF

  if (!selectionPred().isEmpty()){
    transformSelectPred(normWARef, locationOfPointerToMe);
  }
  // QSTUFF

  // ---------------------------------------------------------------------
  // Transform the entire column list of the base table to pick up
  // equivalences of base table columns and index columns
  // ---------------------------------------------------------------------
  const ValueIdList &allCols = getTableDesc()->getColumnList();
  ItemExpr *oldPtr;
  ExprValueId newPtr;
  ValueId eqVid;
  
  CollIndex i = 0;
  for (i = 0; i < allCols.entries(); i++) {
    oldPtr = allCols[i].getItemExpr();
    newPtr = oldPtr;
    oldPtr->transformNode(normWARef, newPtr, locationOfPointerToMe,
                          getGroupAttr()->getCharacteristicInputs());
    // the column list shouldn't be changed by the transformation
    CMPASSERT(oldPtr == newPtr.getPtr()); 
    // ---------------------------------------------------------------------
    // Create a VEG with all equivalent index columns
    // ---------------------------------------------------------------------
    if (oldPtr->getOperatorType() == ITM_BASECOLUMN) {
      const ValueIdSet &eic = ((BaseColumn *)oldPtr)->getEIC();
      for (eqVid = eic.init(); eic.next(eqVid); eic.advance(eqVid)) {
	// for trigger temp tables
	if (updatedTableName_.getSpecialType() == ExtendedQualName::TRIGTEMP_TABLE &&
	    getOperatorType() == REL_LEAF_INSERT) {
	  normWARef.addVEGInOuterRegion(((BaseColumn *)oldPtr)->getValueId(),eqVid);
	}
	// no trigger temp tables
	else {
	  normWARef.addVEG(((BaseColumn *)oldPtr)->getValueId(),eqVid);
	}
      }
    }
    else {
      CMPASSERT(oldPtr->getOperatorType() == ITM_BASECOLUMN);
    }
  }

  // ---------------------------------------------------------------------
  // Prime the Group Attributes for the GenericUpdate.
  // ---------------------------------------------------------------------
  primeGroupAttributes();

} // GenericUpdate::transformNode()

// -----------------------------------------------------------------------
// GenericUpdate::rewriteNode()
// -----------------------------------------------------------------------
void GenericUpdate::rewriteNode(NormWA & normWARef)
{

  // QSTUFF
  const ValueIdList &allCols = getTableDesc()->getColumnList();
  ItemExpr *newPtr = NULL;

  // ---------------------------------------------------------------------
  // walk through all the columns of the table, normalizing them
  // and adding the result into the ColumnVEGList of the table descriptor
  // ---------------------------------------------------------------------
  CollIndex j = 0;
  for (j = 0; j < allCols.entries(); j++)
    {
      // ---------------------------------------------------------------------
      // Create a VEG with all equivalent index columns
      // ---------------------------------------------------------------------
      newPtr = allCols[j].getItemExpr()->normalizeNode(normWARef);
      getTableDesc()->addToColumnVEGList(newPtr->getValueId());
    }
  // -------------------------------------------------------------------------
  // Normalize the indexes.
  // -------------------------------------------------------------------------
  for (j = 0;
       j < (Int32)getTableDesc()->getIndexes().entries();
       j++)
    {
      IndexDesc *idesc = getTableDesc()->getIndexes()[j];
      ValueIdList indexOrder(idesc->getOrderOfKeyValues());

      // ---------------------------------------------------------------------
      // Normalize the asc/desc order of the index.
      // ---------------------------------------------------------------------
      indexOrder.normalizeNode(normWARef);
      idesc->setOrderOfKeyValues(indexOrder);

      // ---------------------------------------------------------------------
      // Normalize the partitioning keys in the partitioning function.
      // ---------------------------------------------------------------------
      if (idesc->isPartitioned())
        idesc->getPartitioningFunction()->normalizePartitioningKeys(normWARef);
    }

  // we need to normalize the potential outputs here to avoid problems 
  // during code generation
  potentialOutputs_.normalizeNode(normWARef);
  // QSTUFF

  precondition_.normalizeNode(normWARef);

  // these are no longer used in the following phases,
  // so remove them instead of rewriting them
  exprsInDerivedClasses_.clear();
  // ---------------------------------------------------------------------
  // Rewrite the expressions in the selection predicates and
  // in the Group Attributes.
  // ---------------------------------------------------------------------
  RelExpr::rewriteNode(normWARef);

  // ---------------------------------------------------------------------
  // Rewrite values in the newrec expressions.
  // ---------------------------------------------------------------------
  if (newRecExpr().normalizeNode(normWARef))
    {
    }
  // QSTUFF
  if (newRecBeforeExpr().normalizeNode(normWARef))
    {
    }
  // QSTUFF
  if (executorPred().normalizeNode(normWARef))
    {
    }

  if (isMerge())
    {
      if (mergeInsertRecExpr().normalizeNode(normWARef))
	{
	}
      if (mergeUpdatePred().normalizeNode(normWARef))
	{
	}
    }

  // ---------------------------------------------------------------------
  // Rewrite expressions in the order by list, if this is an insert.
  // ---------------------------------------------------------------------
  if (getOperatorType() == REL_UNARY_INSERT)
  {
    Insert * ins
      = (Insert *)(this->castToRelExpr());

    if (ins->reqdOrder().normalizeNode(normWARef))
    {
    }
  }

  /*
  // QSTUFF 
  // this has been moved up before rewriting the index item expressions
  // ---------------------------------------------------------------------
  // walk through all the columns of the table, normalizing them
  // and adding the result into the ColumnVEGList of the table descriptor
  // ---------------------------------------------------------------------
  const ValueIdList &allCols = getTableDesc()->getColumnList();
  ItemExpr *newPtr = NULL;
  for (CollIndex i = 0; i < allCols.entries(); i++)
    {
      // ---------------------------------------------------------------------
      // Create a VEG with all equivalent index columns
      // ---------------------------------------------------------------------
      newPtr = allCols[i].getItemExpr()->normalizeNode(normWARef);
      getTableDesc()->addToColumnVEGList(newPtr->getValueId());
    } 
  // QSTUFF
  */

  // ---------------------------------------------------------------------
  // Rewrite values in the key expressions.
  // ---------------------------------------------------------------------
  // For key expressions only normalize the right hand side of the =
  // left side should have been a different valueId from the one below
  ValueIdList keyList = beginKeyPred();
  if (keyList.entries() > 0)
    {
      for (CollIndex i = 0; i < keyList.entries(); i++)
        {
          ItemExpr * eqPtr = ((keyList[i]).getValueDesc())->getItemExpr();
          ItemExpr * right_side = (*eqPtr)[1]->normalizeNode(normWARef);
          eqPtr->child(1) = right_side;
        }
    }    

  // ---------------------------------------------------------------------
  // Rewrite the ValueIdMap between the select and the update part so
  // it has VEGReferences init (note that we avoided VEGies that span
  // both the select and the update part, this is (probably?) one
  // reason why we only normalized one half of the keys preds above.
  // ---------------------------------------------------------------------
  updateToSelectMap_.normalizeNode(normWARef);

  // ---------------------------------------------------------------------
  // Rewrite values in the check constraint expressions.
  // Indicate that we are processing a complex scalar expression to
  // suppress the performance of transitive closure.
  // ---------------------------------------------------------------------
  normWARef.setComplexScalarExprFlag();
  normWARef.setInConstraintsFlag();

  if (checkConstraints().normalizeNode(normWARef))
    {
    }

  normWARef.restoreComplexScalarExprFlag();
  normWARef.restoreInConstraintsFlag();

  // ---------------------------------------------------------------------
  // Rewrite the expressions in the TriggerBindInfo object which is part
  // of the inlining info.
  // ---------------------------------------------------------------------

  if (getInliningInfo().getTriggerBindInfo())
  {
    getInliningInfo().getTriggerBindInfo()->normalizeMembers(normWARef);
  }
} // GenericUpdate::rewriteNode()

// -----------------------------------------------------------------------
// GenericUpdate::recomputeOuterReferences()
// -----------------------------------------------------------------------
void GenericUpdate::recomputeOuterReferences()
{
  // Should replace with appropriate virtual methods

  // Solution 10-040114-2405 start 
  // Our transformation for input rowsets always involves a unpack and a 
  // flow operator. Hence we shouldnt be accessing any input rowset 
  // directly. Remove it's reference from the required inputs
    
  ValueIdSet outerRefs = getGroupAttr()->getCharacteristicInputs();
  ValueId vid;
  ValueIdSet inRowsets;
  ItemExpr *ie ;
  for (vid = outerRefs.init(); outerRefs.next(vid); outerRefs.advance(vid))
    {
      ie = vid.getItemExpr();
      if (ie->getOperatorType() != ITM_CONSTANT) {
	if ((vid.getType().getTypeQualifier() == NA_ROWSET_TYPE) ||
	    (( ie->getOperatorType() == ITM_DYN_PARAM) &&
	    (((DynamicParam *) ie)->getRowsetSize() != 0)))
	      inRowsets +=vid;
      }
    }
  // Remove input rowset references 
  outerRefs -=inRowsets;
  // Solution 10-040114-2405 end
  


  if ((getOperatorType() != REL_UNARY_INSERT) && (getOperatorType() != REL_LEAF_INSERT) &&
      (getOperatorType() != REL_UNARY_DELETE) && (getOperatorType() != REL_LEAF_DELETE) && 
      (getOperatorType() != REL_UNARY_UPDATE) && (getOperatorType() != REL_LEAF_UPDATE)) {
     getGroupAttr()->setCharacteristicInputs(outerRefs);
     return;
  }

  // ---------------------------------------------------------------------
  // Delete all those input values that are no longer referenced on
  // this operator because the predicates that reference them have 
  // been pulled up.
  // ---------------------------------------------------------------------

  ValueIdSet allMyExpr(newRecExpr());
  allMyExpr += newRecBeforeExpr();
  allMyExpr += executorPred();
  allMyExpr += usedColumns();
  allMyExpr += getSelectionPred();
  allMyExpr += exprsInDerivedClasses_;

  ValueIdSet beginKeyPredSet(beginKeyPred());
  allMyExpr += beginKeyPredSet;
  if (isMerge())
    {
      allMyExpr += mergeInsertRecExpr();
      allMyExpr += mergeUpdatePred();
    }

  allMyExpr.weedOutUnreferenced(outerRefs);

  // Add references needed by children, if any
  Int32 arity = getArity();
  for (Int32 i = 0; i < arity; i++)
    {
      outerRefs += child(i).getPtr()->getGroupAttr()->getCharacteristicInputs();
    }

  getGroupAttr()->setCharacteristicInputs(outerRefs);

} // GenericUpdate::recomputeOuterReferences

// -----------------------------------------------------------------------
// GenericUpdate::normalizeNode
// -----------------------------------------------------------------------
RelExpr * GenericUpdate::normalizeNode(NormWA & normWARef)
{
  if (nodeIsNormalized())
    return this;

  if (isMerge()) 
    normWARef.incrementMergeUpdDelCount();
  // Call the super class to do the normalization work.
  RelExpr *normalizedThis = RelExpr::normalizeNode(normWARef);

  if ((getOperator().match(REL_ANY_GEN_UPDATE) ||  // general update cases
       getOperator().match(REL_UNARY_INSERT)       // update of a key column
      ) 
      &&
      (getInliningInfo().hasTriggers()       ||    // driving trigger temp table insert
       getInliningInfo().isMVLoggingInlined()      // driving MV IUD log insert 
      )
     )
  {
     Lng32 actualMessageSize = getGroupAttr()->getCharacteristicOutputs().getRowLength();

     // 2 headers: one for record header and the other for the message header
     Lng32 maxMessageSize = (ActiveSchemaDB()->getDefaults().getAsULong(LOCAL_MESSAGE_BUFFER_SIZE) * 1024) -
                           (2*(ActiveSchemaDB()->getDefaults().getAsULong(DP2_MESSAGE_HEADER_SIZE_BYTES)));

     // check row size against max executor message buffer size
     if (actualMessageSize >= maxMessageSize)
     {
       Lng32 tableRecordLength = getTableDesc()->getNATable()->getRecordLength();
       NAString tableName = getTableDesc()->getNATable()->getTableName().getQualifiedNameAsAnsiString();

       *CmpCommon::diags() << DgSqlCode(-12070)
                           << DgString0(tableName)
                           << DgInt0(tableRecordLength)
                           << DgInt1((Lng32)maxMessageSize/2);
       return this;
     }
  }

  /// YYY
   if (getOperator().match(REL_ANY_UNARY_GEN_UPDATE))
   {
      Scan * scan = getLeftmostScanNode();
      if (scan && scan->requiresHalloweenForUpdateUsingIndexScan())
        setAvoidHalloween(TRUE);
   }

  if (producedMergeIUDIndicator_ != NULL_VALUE_ID)
    {
      ValueId dummy;
      if (NOT getGroupAttr()->getCharacteristicOutputs().referencesTheGivenValue(
               producedMergeIUDIndicator_,
               dummy))
        // nobody asked for the merge IUD indicator, therefore remove
        // it, (e.g. simple table without index maintenance)
        producedMergeIUDIndicator_ = NULL_VALUE_ID;
    }

  return normalizedThis;
}

// -----------------------------------------------------------------------
// Insert::normalizeNode()
// The purpuse of this method is to eliminate the Tuple node of an
// INSERT-VALUES statement. After normalization is done, the Tuple node
// becomes redundant, since all the information is inside the Insert node
// anyway.
// -----------------------------------------------------------------------
RelExpr * Insert::normalizeNode(NormWA & normWARef)
{
  if (nodeIsNormalized())
    return this;

  // Call the super class to do the normalization work.
  RelExpr *normalizedThis = GenericUpdate::normalizeNode(normWARef);

  // If this already is a LeafInsert node - the work is done.
  if (normalizedThis->getOperatorType() == REL_LEAF_INSERT)
    return normalizedThis;

  // If there is an ORDER BY + a [first n], copy the ORDER BY ValueIds
  // down to the FirstN node so we order the rows before taking the first n.
  // If it is ORDER BY + [any n] we don't do this, as it is sufficient
  // and more efficient to sort the rows after taking just n of them.
  // Note: We do this at normalize time instead of bind time because if
  // there are complex expressions in the ORDER BY, the binder will get
  // different ValueIds for the non-leaf nodes which screws up coverage
  // tests. Doing it here the ValueIds have already been uniquely computed.
  if ((reqdOrder().entries() > 0) && 
      (child(0)->getOperatorType() == REL_FIRST_N))
    {
      FirstN * firstn = (FirstN *)child(0)->castToRelExpr();
      if (firstn->isFirstN())  // that is, [first n], not [any n] or [last n]
        firstn->reqdOrder().insert(reqdOrder());
    }

  // If the child is not a Tuple node - nothing to do here.
  CMPASSERT(normalizedThis->getArity() > 0);
  if (normalizedThis->child(0)->getOperatorType() != REL_TUPLE)
    return normalizedThis;

  if (normalizedThis->child(0)->getSelectionPred().isEmpty())
  {
     // Now get rid of the Tuple node and start a new (although shortlived)
     // life as a LeafInsert node. The optimizer will next transform it
     // to a DP2Insert node.
     normalizedThis->child(0) = (RelExpr *)NULL;
     normalizedThis->setOperatorType(REL_LEAF_INSERT);
  }
  // else this is the case of an insert node to an ON STATEMENT MV
  // an insert to a statement MV is inlined with an update to an
  // ON STATEMENT MV source table

  return normalizedThis;
}

// ***********************************************************************
// $$$$ RelRoot
// member functions for class RelRoot
// ***********************************************************************

// -----------------------------------------------------------------------
// ***NOTE*** These methods must be called AFTER the transformation phase
//            or they will not return the correct answer.
//
// A sql statement cursor is updatable if all of the following are true:
//  -- it is a SELECT statement
//  -- there is only one underlying table, and no subquery references that tbl
//  -- there are no aggregates present
//  -- neither GROUP BY, DISTINCT, nor ORDER BY is specified
//  -- all view columns must be column references
//  -- no column reference can occur more than once
//  -- The underlying table is not a materialized view
//
// A view is updatable similarly, except that
//  -- ORDER BY *is* allowed (if it's allowed in a view at all)
//
// See Ansi 6.3 and 7.9 SR 12, and references to "read-only table".
// -----------------------------------------------------------------------
NABoolean RelRoot::isUpdatableBasic(NABoolean isView, 
                                    NABoolean &isInsertable) const
{
  CMPASSERT(nodeIsBound() && nodeIsTransformed());
    
  // ## Must ensure this still works when we have updatable Stored Procedures
  // QSTUFF
  Scan *scan;
  GenericUpdate *gu = 0;
  // QSTUFF

  // QSTUFF
 
  if (child(0)->getGroupAttr()->isEmbeddedUpdateOrDelete() && 
    child(0)->getGroupAttr()->isGenericUpdateRoot()){
    
    gu = (GenericUpdate *) child(0)->castToRelExpr();
    
    if (gu->getOperator().match(REL_ANY_UNARY_GEN_UPDATE))
      scan = (Scan *)(child(0)->castToRelExpr())->getLeftmostScanNode();
    else
      return FALSE;
  }       
  else
  // QSTUFF

    {
	scan = (Scan *)child(0)->castToRelExpr();
    }

  if (scan->getOperatorType() != REL_SCAN)
    return FALSE;

  if (scan->accessOptions().accessType() == TransMode::READ_UNCOMMITTED_ACCESS_)    // "read-only table"
    return FALSE;

  TransMode::IsolationLevel il;
  if ((NOT isView) ||
      (CmpCommon::getDefault(ISOLATION_LEVEL_FOR_UPDATES) == DF_NONE))
    ActiveSchemaDB()->getDefaults().getIsolationLevel
      (il);
  else
    ActiveSchemaDB()->getDefaults().getIsolationLevel
      (il,
       CmpCommon::getDefault(ISOLATION_LEVEL_FOR_UPDATES));
  if (scan->accessOptions().accessType() == TransMode::ACCESS_TYPE_NOT_SPECIFIED_ &&
      il == TransMode::READ_UNCOMMITTED_)
    return FALSE;

  NATable *naTable = ActiveSchemaDB()->getNATableDB()->get(&
    scan->getTableDesc()->getNATable()->getExtendedQualName());
  CMPASSERT(naTable);

  if (naTable->getReferenceCount() > 1)
    // QSTUFF
    if (getGroupAttr()->isEmbeddedUpdateOrDelete()){
      if (naTable->getReferenceCount() > 2)
        return FALSE;
    }
    else
      // QSTUFF
      // A subquery references the scan tbl
      return FALSE;
                  
  if (naTable->isAnMV())
	return FALSE;  // A materialized view is not updatable. -- MV

  if (naTable->isPartitionNameSpecified())
	return FALSE; // If the PARTITION clause is specified in the view's query text
  // then the view is not updatable. 
  // Check option can only check predicates in the 
  // where clause, the partition clause is like an extra predicate, in that it 
  // restricts the statements action to a single partition. But this extra predicate
  // cannot be enforced by our current check option mechanism.
  // Similarly if the PARTITION clause is specified 
  // in the query specification a cursor declaration then the cursor is not updatable.

  ValueIdSet selectCols;
  if (isView)
    {
      for (CollIndex i = 0; i < compExpr().entries(); i++)
        {
          ValueId idcol = compExpr()[i];
          const NAColumn *nacol = idcol.getNAColumn(TRUE/*okIfNotColumn*/);

          if (!nacol)                   // not a column reference
            return FALSE;

          // QSTUFF
          // in case of an embedded update within a view there may be an old and
          // a new column pointing to the same base table. We have to detect that
          // and prevent those views to be updatable.
          if (getGroupAttr()->isEmbeddedUpdateOrDelete())
            {
              CMPASSERT(gu);
              for (CollIndex j = 0; 
                   j < gu->getTableDesc()->getColumnList().entries(); 
                   j++)
                {
                  if ( gu->getTableDesc()->
                    getColumnList()[j].getItemExpr()->getValueId() == idcol)
                    {
                      idcol = scan->getTableDesc()->
                        getColumnList()[j].getItemExpr()->getValueId();
                    }
                }
            }
          // QSTUFF
          
          if (selectCols.contains(idcol))       // colref appears multiple times
            return FALSE;                       // (cf. errors 4017, 4022 in Binder)
          selectCols += idcol;

          // A system column is ok as long as user doesn't actually UPDATE or INSERT
          // it (by definition, the system supplies a default when not explicitly
          // named in INSERT)
          //    if (nacol->isSystemColumn() &&  // cf. error 4013 in Binder
          //        nacol->getDefaultValue() == NULL)
          //          isInsertable = FALSE;
        }
    }

  // All columns not selected in the view must have a default value
  // for the view to be "insertable" (Tandem notion, not Ansi;
  // see SCMPBIDD for SQL/MP definition).
  // We don't care what default info a system column has;
  // by definition a system column is always filled in (defaulted).
  // Cf. error 4024 in Binder.
  if (isView)
    {
      const ValueIdList &allCols = scan->getTableDesc()->getColumnList();
      for (CollIndex i = 0; i < allCols.entries(); i++)
        {
          const ValueId idcol = allCols[i];
          const NAColumn *nacol = idcol.getNAColumn();
          if (!selectCols.contains(idcol) &&
              !nacol->getDefaultValue() &&
              !nacol->isSystemColumn())
            {
              isInsertable = FALSE;
              break;
            }
        } // for allCols
    } // isView

  return TRUE;
}

NABoolean RelRoot::isUpdatableCursor()          // this is NOT const
{
  NABoolean junk;
  if (!isUpdatableBasic(FALSE, junk)) return FALSE;

  // Ansi 13.1 SR 5a -- no updatability clause specified, but ORDER BY was.
  if (!updatableSelect_)
      if (reqdOrder().entries()) return FALSE;  // ORDER BY col-list
      // ##When INSENSITIVE and SCROLL are supported, this rule also applies 

  // The following mods to the updatable-column list are only done if
  // we have to (for efficiency).

  if (!updateCol().entries() || reqdOrder().entries())  {
    // "FOR UPDATE;"  w/o col-list -- 
    // is equivalent to "FOR UPDATE OF all-cols", per Ansi 13.1 SR 5b + 13.
    //
    ValueIdSet upd(updateCol());
    if (!upd.entries()) {
      const ColumnDescList &cols = 
              *getScanNode()->getRETDesc()->getColumnList();
      for (CollIndex i = 0; i < cols.entries(); i++)  {
        const ValueId idcol = cols[i]->getValueId();
        const NAColumn *nacol = idcol.getNAColumn(TRUE/*okIfNotColumn*/);
        if (nacol)
          upd += idcol;
      }
    }

    // Genesis 10-990201-0094.  Ansi 17.18 SR 5.
    // Remove any ORDER BY cols from the FOR UPDATE OF cols,
    // then we let cli/Statement handle it (error CLI_INVALID_UPDATE_COLUMN).
    // ## We really should enhance StaticCompiler to catch these
    // ## syntax errors (and also CLI_NON_UPDATABLE_SELECT_CURSOR)
    // ## at compile-time not run-time.
    // ## This would require intersecting an updateWhereCurrentOf's
    // ##       newRecExpr's target columns' NAColumns (or full-col-names)
    // ## with its
    // ##       cursor's updateCols' NAColumns (or full-col-names),
    // ## via some extra lookup in cursor PLTs in StmtDeclStatCurs process().
    upd -= ValueIdSet(reqdOrder());

    updateCol() = ValueIdList(upd);             // this is NOT const
  }

  if (!updateCol().entries()) return FALSE;

  return TRUE;
}

NABoolean RelRoot::isUpdatableView(NABoolean &isInsertable) const
{
  isInsertable = TRUE;

  if (!isUpdatableBasic(TRUE, isInsertable))
    {
      isInsertable = FALSE;
      return FALSE;
    }

  return TRUE;
}

void RelRoot::transformNode(NormWA & normWARef,
                            ExprGroupId & locationOfPointerToMe)
{
  CMPASSERT( this == locationOfPointerToMe );

  if (nodeIsTransformed())
    return;
  markAsTransformed();

  // QSTUFF
  // in case of embedded updates or deletes we have to prevent outer
  // predicates being pushed into subtrees being generated by a generic
  // update. We achieve that by recording whether a node is at the root
  // of a GenericUpdate subtree and preventing predicates being pushed
  // beyond that node. By contruction we know that those nodes are either
  // anti semi-joins or unary updates.
  // we prevent predicates from being pushed down by forcing them not to be
  // covered by the coverTest method which in turn causes them not to be
  // pushed down by pushdowncoveredExpressions. 
  // This works fine execpt for equality predicates (x.x = 10) which are usually
  // veggyfied by the transformation pass. Since the compiler assumes that all those
  // predicates have been pushed down to the leaves it will just forget about
  // them at code generation time. To prevent that we disable generation of 
  // veggies for constant equality termsand rely on pushdown covered 
  // expression to do the right thing...which seems to work just fine.
  
  if (getGroupAttr()->isEmbeddedUpdateOrDelete())
    normWARef.setInEmbeddedUpdateOrDelete(TRUE);
  // QSTUFF
  
  // Embedded insert has the same equality predicate pushdown problems
  // as embedded updates or deletes.  Set the flag to prevent the pushdown.
  
  if (getGroupAttr()->isEmbeddedInsert())
    normWARef.setInEmbeddedInsert(TRUE);   

  // ---------------------------------------------------------------------
  // Make a working copy of the NormWA for each (sub)query tree.
  // ---------------------------------------------------------------------
  NormWA newNormWA(normWARef);

  // ---------------------------------------------------------------------
  // Each Subquery represents its own region for the construction of
  // VEGPredicates.
  // ---------------------------------------------------------------------
  if (isTrueRoot())
    newNormWA.allocateAndSetVEGRegion(IMPORT_AND_EXPORT,this);
  else
    newNormWA.clearStateInformation(); // each subquery tree has its own state
  
  // RelRoots predicates in the selectPred() are to be evaluated
  // above the context of the RelRoot. Predicates from the child of
  // the RelRoot are to stay there.
  //
  // Predicates are found at this stage in a RelRoot were pushed
  // down by the parent to be transformed here. The parent was either
  // a Rename node with or I'm the right child of a Semi or Outer Join.
  //

  // ---------------------------------------------------------------------
  // Make values available to child
  // ---------------------------------------------------------------------
  child(0)->getGroupAttr()->addCharacteristicInputs
	                    (getGroupAttr()->getCharacteristicInputs());

  // ---------------------------------------------------------------------
  // Transform the child
  // ---------------------------------------------------------------------
  child(0)->transformNode(newNormWA, child(0)); 

  if ((isTrueRoot()) &&
      (child(0)) &&
      ((child(0)->getOperatorType() == REL_SORT_LOGICAL) ||
       ((child(0)->getOperatorType() == REL_FIRST_N) &&
	((child(0)->child(0)) &&
	 (child(0)->child(0)->getOperatorType() == REL_SORT_LOGICAL)))))
    {
      SortLogical * sl = NULL;
      if (child(0)->getOperatorType() == REL_SORT_LOGICAL)
	sl = (SortLogical*)child(0)->castToRelExpr();
      else 
	sl = (SortLogical*)child(0)->child(0)->castToRelExpr();
      if (NOT hasOrderBy())
	{
	  // move order by sort key from SortLogical child to me.
	  reqdOrder() = sl->getSortKey();
	}
    }

  // ---------------------------------------------------------------------
  // Transform the computable expressions associated with me.
  // If a subquery appears in the compute list, then let the subquery
  // transformation cause a semijoin to be performed between the 
  // child of the RelRoot and the subquery.
  // ---------------------------------------------------------------------
  newNormWA.setInSelectList() ;
  if (compExpr().transformNode(newNormWA, child(0),
                               getGroupAttr()->getCharacteristicInputs()))
    {
      // -----------------------------------------------------------------
      // Transform my new child.
      // -----------------------------------------------------------------
      child(0)->transformNode(newNormWA, child(0));
    }
  newNormWA.restoreInSelectList() ;


  // ---------------------------------------------------------------------
  // Definitely no subqueries in the host variables, dynamic parameters
  // and constant values.
  // ---------------------------------------------------------------------
  if (inputVars().transformNode(newNormWA, child(0),
                                getGroupAttr()->getCharacteristicInputs()))
    {
      ABORT("Internal error in RelRoot::transformNode - subquery in inputVars");
    }
  // ---------------------------------------------------------------------
  // Definitely no subqueries in the order by list, at least until SQL MCLLXIV!
  // ---------------------------------------------------------------------
  if (reqdOrder().transformNode(newNormWA, child(0),
                                getGroupAttr()->getCharacteristicInputs()))
    {
      ABORT("Internal error in RelRoot::transformNode - subquery in reqdOrder");
    }

  pullUpPreds();

  // transform the selection predicates
  transformSelectPred(newNormWA, locationOfPointerToMe);

  // We are currently assuming that no subqueries have been introduced above me;
  // any new subquery parent would just silently be ignored!
  CMPASSERT( this == locationOfPointerToMe );    // Genesis 10-970828-6025

  normWARef.setCorrelatedSubqCount(newNormWA.getCorrelatedSubqCount()); 
  normWARef.setContainsSemiJoinsToBeTransformed
    (newNormWA.containsSemiJoinsToBeTransformed());

  if (isTrueRoot())
    {
      // -----------------------------------------------------------------
      // Sometimes a Left Join can be transformed to an Inner Join if
      // there are binary comparison predicates that can filter out 
      // null augmented rows. In such a case, the VEGRegion created
      // the Left Join needs to be merged into its parent VEGRegion. 
      // -----------------------------------------------------------------
      normWARef.processVEGRegions();
      // Restore the original VEGRegion.
      newNormWA.restoreOriginalVEGRegion();

      // if updatability of the cursor was not disabled explicitly
      // by specifying a READ ONLY clause, then check to see if
      // the cursor really is updatable. Retrieve child's pkeys,
      // if the cursor is updatable.
      if (updatableSelect() == TRUE 
          // QSTUFF
          &&
          // we allow simple views containing embedded deletes 
          // to be updated...but that does not translate into an
          // updatable cusor
          ! child(0)->getGroupAttr()->isGenericUpdateRoot()
          // QSTUFF
          )
        { 
          if (isUpdatableCursor())
            {
	      updatableSelect() = TRUE;
	      
	      // add child's clustering key columns to pkeyList.
	      // Convert nodes are added to convert the key value to
	      // the actual key type at runtime. The key value id gets 
	      // replaced by a veg ref, so it is important that we 'remember'
	      // what the correct key type is and then convert to that type.
	      // This list is used to generate expression to compute a row
	      // of primary key values that will be returned to CLI so it
	      // could be passed in to an UPDATE...WHERE CURRENT OF... query.
 
	      // if child is a FirstN node, skip it.
	      Scan * scan = NULL;
	      if ((child(0)->castToRelExpr()->getOperatorType() == REL_FIRST_N) &&
		  (child(0)->child(0)))
		scan = (Scan *)child(0)->child(0)->castToRelExpr();
	      else
		scan = (Scan *)child(0)->castToRelExpr();
	      
	      const ValueIdList * keyList =
		&(scan->getTableDesc()->getClusteringIndex()->getIndexKey());
	      CollIndex i = 0;
	      for (i = 0; i < keyList->entries(); i++)
		{
		  ItemExpr * castNode = 
		    new(newNormWA.wHeap()) Cast((*keyList)[i].getItemExpr(),
						&((*keyList)[i].getType()));
		  
		  castNode->synthTypeAndValueId();
		  
		  pkeyList().insert(castNode->getValueId());
		}
	      
	      ValueIdList nonKeyColList;
	      scan->getTableDesc()->getClusteringIndex()->getNonKeyColumnList(nonKeyColList);
	      for (i = 0; i < nonKeyColList.entries(); i++)
		{
		  ItemExpr * castNode = 
		    new(newNormWA.wHeap()) Cast(nonKeyColList[i].getItemExpr(),
						&(nonKeyColList[i].getType()));
		  
		  castNode->synthTypeAndValueId();
		  
		  pkeyList().insert(castNode->getValueId());
		}
	    } // updatable cursor select
          else // nonupdatable cursor
            {
              updatableSelect() = FALSE;
              if (updateColTree_)
                {
                  // cursor has FOR UPDATE OF clause that can't be honored.
                  *CmpCommon::diags() << DgSqlCode(-4118);
                  locationOfPointerToMe = (RelExpr*)NULL;
                }
            }
        }
      else
        updatableSelect() = FALSE;
      
      if ((child(0)->castToRelExpr()->getOperatorType() == REL_FIRST_N) )
      {
         FirstN* firstn = (FirstN *)child(0)->castToRelExpr();
         if(firstn->reqdOrderInSubquery().entries() >0 )
         {
           reqdOrder().insert( firstn->reqdOrderInSubquery());
         }
      }
    }
  else
    {
      // -----------------------------------------------------------------
      // Modify the Group Attributes of my child so that it receives all
      // the input values that I receive.
      // Assign my selection predicates to the child.
      // -----------------------------------------------------------------

      child(0)->getGroupAttr()->addCharacteristicInputs
	(getGroupAttr()->getCharacteristicInputs());
      child(0)->selectionPred() += getSelectionPred();
      
      // -- Triggers
      child(0)->getInliningInfo().merge(&getInliningInfo());
    
      locationOfPointerToMe = child(0); // my parent now -> my child
      child(0)->setFirstNRows(getFirstNRows());

      //keep the order if there is FIRSTN
      if (child(0)->getOperatorType()==REL_FIRST_N)
      {
         FirstN* firstn = (FirstN *)child(0)->castToRelExpr();
         firstn->reqdOrderInSubquery().insert(reqdOrder()) ;
      }

      deleteInstance();                 // Goodbye!
      
      
    } // eliminate intermediate RelRoots
  
} // RelRoot::transformNode()

// -----------------------------------------------------------------------
// RelRoot::pullUpPreds()
// -----------------------------------------------------------------------
void RelRoot::pullUpPreds()
{
  // A RelRoot never pulls up predicates from its children.
  child(0)->recomputeOuterReferences();
} // RelRoot::pullUpPreds()

// -----------------------------------------------------------------------
// RelRoot::recomputeOuterReferences()
// -----------------------------------------------------------------------
void RelRoot::recomputeOuterReferences()
{
  // ---------------------------------------------------------------------
  // Delete all those input values that are no longer referenced on
  // this operator because the predicates that reference them have 
  // been pulled up.
  // ---------------------------------------------------------------------
  if (NOT getGroupAttr()->getCharacteristicInputs().isEmpty())
  {
    ValueIdSet leafValues, emptySet;
    GroupAttributes emptyGA;
    child(0)->getGroupAttr()->getCharacteristicInputs().
      getLeafValuesForCoverTest(leafValues, emptyGA, emptySet);
    CMPASSERT((getGroupAttr()->getCharacteristicInputs().contains
               (child(0)->getGroupAttr()->getCharacteristicInputs())) || 
              (getGroupAttr()->getCharacteristicInputs().contains (leafValues))); 
    ValueIdSet outerRefs = getGroupAttr()->getCharacteristicInputs(); 
    
    // Remove from outerRefs those valueIds that are not needed
    // by my selection predicate or by my computed expression list.
    // Need to add the orderby list since it is not a subset of the
    // computed expression list.
    ValueIdSet allMyExpr(getSelectionPred());
    allMyExpr.insertList(compExpr());
    allMyExpr.insertList(reqdOrder());
    
    allMyExpr.weedOutUnreferenced(outerRefs);
    
    // Add to outerRefs those that my child need.
    outerRefs += child(0).getPtr()->getGroupAttr()->getCharacteristicInputs();
    
    // set my Character Inputs to this new minimal set.
    getGroupAttr()->setCharacteristicInputs(outerRefs);
  }
} // RelRoot::recomputeOuterReferences()  

// -----------------------------------------------------------------------
// RelRoot::rewriteNode()
// -----------------------------------------------------------------------
void RelRoot::rewriteNode(NormWA & normWARef)
{
  CMPASSERT(isTrueRoot());


  
  // ---------------------------------------------------------------------
  // Save the original external inputs. The original values have to be
  // made available by someone and that someone is the top root.
  // --------------------------------------------------------------------
  ValueIdSet externalInputs(getGroupAttr()->getCharacteristicInputs());
  // ---------------------------------------------------------------------
  // Rewrite the value expressions using the VEG expressions that are
  // created when the transitive closure of "=" predicates was computed.
  // Transform a Left Join to an Inner Join, whenever possible. 
  // ---------------------------------------------------------------------
  RelExpr::rewriteNode(normWARef);
  // ---------------------------------------------------------------------
  // Add the original external inputs to the characteristic inputs.
  // --------------------------------------------------------------------
  getGroupAttr()->addCharacteristicInputs(externalInputs);
  // ---------------------------------------------------------------------
  // Rewrite expressions in the computable expressions.
  // ---------------------------------------------------------------------
  if (compExpr().normalizeNode(normWARef))
    {
    }
  // ---------------------------------------------------------------------
  // Rewrite expressions in the sort key list.
  // ---------------------------------------------------------------------
  if (reqdOrder().normalizeNode(normWARef))
    {
    }
  // ---------------------------------------------------------------------
  // Rewrite expressions in the pkey list.
  // ---------------------------------------------------------------------
  if ((updatableSelect() == TRUE) &&
      (pkeyList().normalizeNode(normWARef)))
    {
    }

} // RelRoot::rewriteNode()

// -----------------------------------------------------------------------
// RelRoot::normalizeNode()
// -----------------------------------------------------------------------
RelExpr * RelRoot::normalizeNode(NormWA & normWARef)
{
  if (nodeIsNormalized())
    return this;
  markAsNormalized();

  CMPASSERT(isTrueRoot());
  
  // ---------------------------------------------------------------------
  // Locate the VEGRegion for the root.
  // ---------------------------------------------------------------------
  normWARef.locateAndSetVEGRegion(this);
  // ---------------------------------------------------------------------
  // Rewrite value expressions in the query tree using the VEG notation.
  // Convert Left Joins to Inner Joins, if possible.
  // Note that this is an extra walk through the query tree and is 
  // hidden in between the tranformNode() and normalizeNode() phases. 
  // Its purpose is to perform a top-down, left-to-right tree walk in 
  // the transformed tree and initiate the rewrite on its way up.
  // This will cause all of the values that are generated at the leaves
  // to be normalized, i.e, rewritten in terms of the VEG notation,
  // before expressions that reference them further up in the tree
  // are normalized.
  // ---------------------------------------------------------------------
  rewriteNode(normWARef);
  // ---------------------------------------------------------------------
  // Check which expressions can be evaluated by my child.
  // Modify the Group Attributes of those children who inherit some of
  // these expressions.
  // ---------------------------------------------------------------------

  pushdownCoveredExpr(getGroupAttr()->getCharacteristicOutputs(),
                      getGroupAttr()->getCharacteristicInputs(),
                      selectionPred()
                     );

  ValueIdList orderByList = reqdOrder();
  ValueIdSet myCharInput = getGroupAttr()->getCharacteristicInputs();
  // This was added to fix the problem exposed by the case 10-010321-1842 
  // Compiler failed to create a plan when query had sort order req. by
  // column number which is expression containing dynamic parameter and
  // covered by another column in RelRoot requiredOutput like
  // SELECT a,a/(?p) FROM t ORDER BY 2; For this case we need to
  // enforce that Sort operator can sort on this expression by keeping
  // parameter ?p in RelRoot child's group requiredInput. Previously,
  // expression got removed from this group requiredOutput, the only
  // reference to ?p was removed, and as a result ?p was not kept in
  // this group requiredInput.
  // NOTE. This solution will force the Sort operator to be done
  // directly below the Root node.
  if (orderByList.entries() > 0)
  {
    ValueIdSet orderBySet(orderByList), 
               coveredOrderBySet,
	       inputsNeededForOrderBy,
               coveredOrderBySubExpr,
               uncoveredOrderByExpr;

    GroupAttributes * childGAPtr = child(0).getPtr()->getGroupAttr();

    childGAPtr->coverTest(orderBySet,
  			  myCharInput,
  			  coveredOrderBySet,
  			  inputsNeededForOrderBy,
  			  &coveredOrderBySubExpr);

    childGAPtr->addCharacteristicInputs(inputsNeededForOrderBy);
  }

  // ---------------------------------------------------------------------
  // If there is an ORDER BY + a [first n], copy the ORDER BY ValueIds
  // down to the FirstN node so we order the rows before taking the first n.
  // If it is ORDER BY + [any n] we don't do this, as it is sufficient
  // and more efficient to sort the rows after taking just n of them.
  // Note: We do this at normalize time instead of bind time because if
  // there are complex expressions in the ORDER BY, the binder will get
  // different ValueIds for the non-leaf nodes which screws up coverage
  // tests. Doing it here the ValueIds have already been uniquely computed.
  // ---------------------------------------------------------------------
  if ((reqdOrder().entries() > 0) && 
      (child(0)->getOperatorType() == REL_FIRST_N))
    {
      FirstN * firstn = (FirstN *)child(0)->castToRelExpr();
      if (firstn->isFirstN())  // that is, [first n], not [any n] or [last n]
        firstn->reqdOrder().insert(reqdOrder());
    }

  // ---------------------------------------------------------------------
  // Normalize the child.
  // ---------------------------------------------------------------------
  child(0) = child(0)->normalizeNode(normWARef);

  // ---------------------------------------------------------------------
  // Restore the region before returning
  // ---------------------------------------------------------------------
  normWARef.restoreOriginalVEGRegion();

  fixEssentialCharacteristicOutputs();

  if (NOT normWARef.getExtraHubVertex())
    normWARef.setExtraHubVertex(this);

  // ---------------------------------------------------------------------
  // Synthesize logical properties
  // ---------------------------------------------------------------------
  synthLogProp(&normWARef);
  normWARef.setMergeUpdDelCount(0);

  // check for any errors occured during normalization
  if (CmpCommon::diags()->mainSQLCODE() < 0)
    return NULL;
  else
    return this;

} // RelRoot::normalizeNode()

// -----------------------------------------------------------------------
// RelRoot::semanticQueryOptimizeNode()
// -----------------------------------------------------------------------
RelExpr * RelRoot::semanticQueryOptimizeNode(NormWA & normWARef)
{
  if (nodeIsSemanticQueryOptimized())
    return this;
  markAsSemanticQueryOptimized() ;

  // sematicQueryOptimize(SQO) is undertaken only if 
  // (a) there are subqueries that can be unnested OR
  // (b) semijoins that can be transformed to inner joins OR
  // (c) joins that can be eliminated.
  // (d) joins tha can be extra hub
  if (normWARef.requiresSemanticQueryOptimization() )
  {
    // make a copy of the current query tree. If there is an exception
    // during the SQO phase we can proceed with the copied.
    // SQO can provide impoved performance but is not needed for
    // correctness.
    RelExpr *copyTree = child(0)->
                         copyRelExprTree(CmpCommon::statementHeap());
    Lng32 numSQOPasses = 0;
    Lng32 multiPassJoinElimLimit = 
      ActiveSchemaDB()->getDefaults().getAsLong(MULTI_PASS_JOIN_ELIM_LIMIT);
    try 
    {
      while ((numSQOPasses == 0) ||
	      (((numSQOPasses < multiPassJoinElimLimit) || 
	      (multiPassJoinElimLimit < 0)) &&
	      (normWARef.containsJoinsToBeEliminated() ||
               normWARef.checkForExtraHubTables())))
      {
	normWARef.locateAndSetVEGRegion(this);

        normWARef.setCheckForExtraHubTables(FALSE);

	// ---------------------------------------------------------------------
	// Semantic Query Optimize the child.
	// ---------------------------------------------------------------------
	child(0) = child(0)->semanticQueryOptimizeNode(normWARef);

        child(0) = inlineTempTablesForCSEs(normWARef);

	normWARef.restoreOriginalVEGRegion();

        normWARef.setExtraHubVertex(NULL);

	normWARef.setContainsJoinsToBeEliminated(FALSE);
	recursivePushDownCoveredExpr(&normWARef);
	numSQOPasses++ ;
      }

    }
  
    catch(AssertException & e)
    {

      // Undo any common expression changes done during Unnesting so that
      // we can start over.
      normWARef.getSqoWA()->undoChanges(normWARef);

      *CmpCommon::diags() << DgSqlCode(2078)
			<< DgString0(e.getCondition())
			<< DgString1(e.getFileName())
			<< DgInt0((Lng32)e.getLineNum());
      
      child(0) = copyTree ;
      if (normWARef.requiresRecursivePushdown())
      {
        recursivePushDownCoveredExpr(&normWARef, 
                                     FALSE // no need to do any synthLogProp
                                    );
      }
    }
  }
  else if (normWARef.requiresRecursivePushdown())
  {
    recursivePushDownCoveredExpr(&normWARef, 
                                 FALSE // no need to do any synthLogProp
                                );
  }

  // for debugging
  if (normWARef.getCommonSubExprRefCount() > 0 &&
      CmpCommon::getDefault(CSE_PRINT_DEBUG_INFO) == DF_ON)
    CommonSubExprRef::displayAll();

  return this;

} // RelRoot::semanticQueryOptimizeNode()

RelExpr * RelRoot::inlineTempTablesForCSEs(NormWA & normWARef)
{
  RelExpr *result = NULL;
  const LIST(CSEInfo *) * cses = CmpCommon::statement()->getCSEInfoList();

  if (cses && cses->entries() > 0)
    {
      // If this query tree has any common subexpressions that need
      // to be materialized as temp tables, then insert these
      // materialization steps (called CTi below) between the root
      // and its child node, Q, like this:
      //
      //     Root                                    Root
      //       |                                       |
      //       Q                                  MapValueIds
      //                                               |
      //                                          BlockedUnion
      //                                           /        \
      //                                        Union        Q
      //                                        /   \
      //                                      ...    CTn
      //                                      /
      //                                   Union
      //                                   /   \
      //                                 CT1   CT2
      //
      // The common subexpressions may depend on each other, so make
      // sure to create them in the right order and to use blocked
      // union instead of a regular union if there are such
      // dependencies.
      NABitVector toDoVec;   // still to be done
      NABitVector readyVec;  // ready, all predecessors are done
      NABitVector doneVec;   // already done

      // first, figure out all the CSEs that we have to process
      for (CollIndex i=0; i<cses->entries(); i++)
        if (cses->at(i)->getInsertIntoTemp() != NULL)
          toDoVec += i;

      // Loop over the to-do list, finding new entries for which we
      // already processed all of their predecessors. In this context,
      // the children are the predecessors, since we have to build the
      // graph bottom-up. In other words, find a topological reverse
      // order of the lexical graph of the CSEs.
      while (toDoVec.entries() > 0)
        {
          RelExpr *thisLevelOfInserts = NULL;

          for (CollIndex c=0; toDoVec.nextUsed(c); c++)
            {
              CSEInfo *info = cses->at(c);
              // predecessor (child) CSEs that have to be computed before we
              // can attempt to compute this one
              const LIST(CountedCSEInfo) &predecessors(info->getChildCSEs());
              NABoolean isReady = TRUE;

              for (CollIndex p=0; p<predecessors.entries(); p++)
                {
                  Int32 cseId = predecessors[p].getInfo()->getCSEId();

                  CMPASSERT(cses->at(cseId)->getCSEId() == cseId);
                  if (!doneVec.contains(cseId) &&
                      cses->at(cseId)->getInsertIntoTemp() != NULL)
                    // a predecessor CSE for which we have to
                    // materialize a temp table has not yet
                    // been processed - can't do this one
                    isReady = FALSE;
                }

              if (isReady)
                {
                  // no predecessors or all predecessors have been
                  // done
                  readyVec += c;
                }
            }

          // At this point we will have one or more CSEs in readyVec.
          // All of their predecessors (if any) have already been
          // processed.  Now make a Union backbone to process all the
          // CSEs in readyVec in parallel.

          // If we find nothing, we may have circular dependencies,
          // and this is not allowed
          // (recursive queries will have to be handled separately)
          CMPASSERT(readyVec.entries() > 0);

          for (CollIndex r=0; readyVec.nextUsed(r); r++)
            {
              CSEInfo *info = cses->at(r);
              
              if (thisLevelOfInserts == NULL)
                thisLevelOfInserts = info->getInsertIntoTemp();
              else
                {
                  thisLevelOfInserts = CommonSubExprRef::makeUnion(
                       thisLevelOfInserts,
                       info->getInsertIntoTemp(),
                       FALSE);
                }
            } // loop over ready list

          if (result == NULL)
            result = thisLevelOfInserts;
          else
            result = CommonSubExprRef::makeUnion(
                 result,
                 thisLevelOfInserts,
                 TRUE);

          toDoVec -= readyVec;
          doneVec += readyVec;
          readyVec.clear();
        } // while loop over to-do-list
    } // CSEs exist for this statement

  if (result)
    {
      const ValueIdSet &childOutputs(
           child(0).getGroupAttr()->getCharacteristicOutputs());
      ValueIdList outputValueList;
      ValueIdList unionValueList;

      // make a final blocked union between the inlined
      // insert statements and the actual query
      Union *topUnion = CommonSubExprRef::makeUnion(
           result,
           child(0),
           TRUE);

      // This top-level union has a right child that produces the
      // desired outputs. The left child produces fake dummy ValueIds,
      // it doesn't produce any rows. Since the root expects the right
      // child's ValueIds, we put a MapValueIds on top that maps the
      // values back to what they were in the right child.
      for (ValueId o=childOutputs.init();
           childOutputs.next(o);
           childOutputs.advance(o))
        {
          ItemExpr *leftFake = new(CmpCommon::statementHeap())
            NATypeToItem(o.getType().newCopy(CmpCommon::statementHeap()));

          leftFake->synthTypeAndValueId();

	  ValueIdUnion *vidUnion = new(CmpCommon::statementHeap())
	    ValueIdUnion(leftFake->getValueId(),
                         o,
                         NULL_VALUE_ID,
			 topUnion->getUnionFlags());
	  vidUnion->synthTypeAndValueId();
	  topUnion->addValueIdUnion(vidUnion->getValueId(),
                                    CmpCommon::statementHeap());
          outputValueList.insert(o);
          unionValueList.insert(vidUnion->getValueId());
          topUnion->getGroupAttr()->addCharacteristicOutput(
               vidUnion->getValueId());
        }

      result = new(CmpCommon::statementHeap())
        MapValueIds(topUnion,
                    ValueIdMap(outputValueList, unionValueList),
                    CmpCommon::statementHeap());

      result->setGroupAttr(new (CmpCommon::statementHeap()) GroupAttributes());
      result->getGroupAttr()->addCharacteristicInputs(
           topUnion->getGroupAttr()->getCharacteristicInputs());
      result->getGroupAttr()->setCharacteristicOutputs(childOutputs);

      result->synthLogProp(&normWARef);
    }
  else
    // no change, return child pointer
    result = child(0);

  return result;
}

// -----------------------------------------------------------------------
// Filter::normalizeNode()
// -----------------------------------------------------------------------
RelExpr * Filter::normalizeNode(NormWA & normWARef)
{
  if (nodeIsNormalized())
    return this;
  markAsNormalized();

  ValueIdSet outerReferences, nonLocalPreds; 
  ValueIdSet predsToPushDown, valuesReqdByParent, availableInputs;

  // differs from the base class implementation in that 
  // predicates with outer references are not pushed down to child but
  // are retained in this Filter node.
  availableInputs = getGroupAttr()->getCharacteristicInputs();
  availableInputs.getOuterReferences(outerReferences);
  availableInputs -= outerReferences ;

  predsToPushDown = selectionPred() ;
   if (selectionPred().getReferencedPredicates(outerReferences, nonLocalPreds))
   {
      predsToPushDown  -= nonLocalPreds;
      computeValuesReqdForPredicates(nonLocalPreds,
				     valuesReqdByParent) ;
   }

  pushdownCoveredExpr(getGroupAttr()->getCharacteristicOutputs(),
                      availableInputs, 
		      predsToPushDown,
		      &valuesReqdByParent);
  
  CMPASSERT( predsToPushDown.isEmpty() );
  
  child(0) = child(0)->normalizeNode(normWARef);

  fixEssentialCharacteristicOutputs();

  return this;
} // Filter::normalizeNode()

// -----------------------------------------------------------------------
// SortLogical::normalizeNode()
// -----------------------------------------------------------------------
RelExpr * SortLogical::normalizeNode(NormWA & normWARef)
{
  if (nodeIsNormalized())
    return this;

  RelExpr::normalizeNode(normWARef);

  // eliminate me, I am no longer needed.
  return child(0);
} // SortLogical::normalizeNode()

NABoolean RelExpr::hasFilterChild()
 {
   if (getArity() == 1 && child(0)->getOperatorType() == REL_FILTER)
     return TRUE;
   else if (getArity() == 1  && child(0)->getArity() == 1 && 
	    child(0)->child(0)->getOperatorType() == REL_FILTER)
     return TRUE;
   else
     return FALSE;
 }
// If subquery unnesting fails for some reason at a particular level
// then the Filter node at that level can be elimated by pushing
// its selection predicate to its child. This is not strictly necessary
// as the optimizer has Rules to eliminate Filter nodes. But we do so
// since it helps with cardinality estimation after the SQO phase.
// The Filter nodes selection predicates are only pushed down to its child,
// aand not any further down the query tree.
void RelExpr::eliminateFilterChild()
{

  if(child(0) && 
    child(0)->getOperatorType() == REL_FILTER)
  {

    RelExpr* filterNode = child(0).getPtr() ;
    filterNode->pushdownCoveredExpr(
  		      filterNode->getGroupAttr()->getCharacteristicOutputs(),
  		      filterNode->getGroupAttr()->getCharacteristicInputs(),
  		      filterNode->selectionPred());

    if (filterNode->selectionPred().isEmpty())
      child(0) = filterNode->child(0) ;
    else
    {
       // Pushdown failed to push the predicate for some reason.
       // add it by hand and call pushdown again with an empty predicate 
       // to recompute the IO.

       filterNode->child(0)->selectionPred() += filterNode->selectionPred();
       filterNode->selectionPred().clear();
       filterNode->pushdownCoveredExpr(
  		      filterNode->getGroupAttr()->getCharacteristicOutputs(),
  		      filterNode->getGroupAttr()->getCharacteristicInputs(),
  		      filterNode->selectionPred());
      child(0) = filterNode->child(0) ;
    }
  }

  return ;
}

// called at the end of SQO phase tio guarantee that all outputs 
// minimal. Prior to this call the SQO phase can have outputs that
// are not minimal for threse three reasons
// (a) unnesting for a subquery failed and predicates from Filter were 
// pushed down only to its child
// (b) unnesting for a subquery failed because outputs from left child 
// tree could not be prmoted sufficiently. getMoreOutputsIfPossible() can
// leave some nodes with more than the minimal set of outputs in this case.
// (c) The pullUpGroupBy transformation calls pushDownCoveredExpr only 
// upto the children of the join being transformed and not all the way down.
void RelExpr::recursivePushDownCoveredExpr(NormWA * normWAPtr,
                                           NABoolean doSynthLogProp)
{
  Int32 arity = getArity();
  // --------------------------------------------------------------------
  // Check which expressions can be evaluated by my child.
  // Modify the Group Attributes of those children who
  // inherit some of these expressions.
  // ---------------------------------------------------------------------
  
  if (getOperator().match(REL_ANY_JOIN))
  {
    if ((NOT normWAPtr->getExtraHubVertex()) && !isExtraHub())
      normWAPtr->setExtraHubVertex(this);
 
  }

  pushdownCoveredExpr(getGroupAttr()->getCharacteristicOutputs(),
                      getGroupAttr()->getCharacteristicInputs(),
                      selectionPred());

  if (getOperator().match(REL_ANY_JOIN) && doSynthLogProp)
  {
        // Make sure equiJoinPredicates_ gets updated
        // in case pushdownCovereExpr() changed any of the joins
        // predicates.
     synthLogProp();  

  }

  // ---------------------------------------------------------------------
  // pushDown expressions from children
  // ---------------------------------------------------------------------
  for (Int32 i = 0; i < arity; i++)
     child(i)->recursivePushDownCoveredExpr(normWAPtr);

  if (doSynthLogProp)
    processCompRefOptConstraints(normWAPtr);

  return;
}

// base class implementation does nothing
void RelExpr::processCompRefOptConstraints(NormWA * normWAPtr)
{
}

NABoolean RelExpr::prepareTreeForCSESharing(
     const ValueIdSet &outputsToAdd,
     const ValueIdSet &predicatesToRemove,
     const ValueIdSet &newPredicatesToAdd,
     const ValueIdSet &inputsToRemove,
     ValueIdSet &valuesForVEGRewrite,
     ValueIdSet &keyColumns,
     CSEInfo *info)
{
  NABoolean result = TRUE;
  CollIndex nc = getArity();
  ValueIdSet newLocalPredicates(newPredicatesToAdd);
  ValueIdSet newVEGPreds;

  newLocalPredicates.findAllOpType(ITM_VEG_PREDICATE, newVEGPreds);

  // recursively call this for the children
  for (CollIndex i=0; i<nc && result; i++)
    {
      ValueIdSet childPredsToRemove(predicatesToRemove);
      ValueIdSet childPredsToAdd(newPredicatesToAdd);
      ValueIdSet childAvailValues(outputsToAdd);

      childAvailValues += child(i).getGroupAttr()->getCharacteristicOutputs();
      childAvailValues += child(i).getGroupAttr()->getCharacteristicInputs();

      childPredsToRemove.removeUnCoveredExprs(childAvailValues);
      childPredsToAdd.removeUnCoveredExprs(childAvailValues);

      result = child(i)->prepareTreeForCSESharing(
           outputsToAdd,
           childPredsToRemove,
           childPredsToAdd,
           inputsToRemove,
           valuesForVEGRewrite,
           keyColumns,
           info);

      // if the child already had or has added any of the requested
      // outputs, then add them to our own char. outputs
      ValueIdSet childAddedOutputs(
           child(i).getGroupAttr()->getCharacteristicOutputs());

      childAddedOutputs.intersectSet(outputsToAdd);
      getGroupAttr()->addCharacteristicOutputs(childAddedOutputs);

      // Todo: CSE: consider using recursivePushDownCoveredExpr
      // instead of pushing these new predicates in this method
      newVEGPreds.intersectSet(childPredsToAdd);
      newLocalPredicates -= childPredsToAdd;
    }

  if (result)
    {
      // Remove the predicates from our selection predicates.
      // Note that prepareMeForCSESharing() is supposed to remove
      // these predicates from all other places in the node.
      predicates_ -= predicatesToRemove;

      // Todo: CSE: need to remove predicates that are "similar" to
      // the ones requested, e.g. same columns and constants, but
      // an "=" operator with a different ValudId?

      // add any predicates that aren't covered by one of the children
      // and also add VEGPredicates that are covered by both of the
      // children

      newLocalPredicates += newVEGPreds;
      predicates_ += newLocalPredicates;

      // Remove the char. inputs the caller asked to remove.
      // At this time we are not doing additional checks to
      // ensure these inputs aren't referenced anymore in
      // our node. We rely on the caller to ensure that
      // these extra inputs are only needed by the predicates
      // that we removed.
      getGroupAttr()->removeCharacteristicInputs(inputsToRemove);
    }

  // Call a virtual method on this node to give it a chance to
  // remove the predicates from any other places where they might be
  // storing them, and to add any outputs it produces locally. Also
  // give it a chance to say "no" to the whole idea of pulling out
  // predicates and changing char. inputs and outputs (the default
  // behavior).
  if (result)
    result = prepareMeForCSESharing(outputsToAdd,
                                    predicatesToRemove,
                                    newLocalPredicates,
                                    inputsToRemove,
                                    valuesForVEGRewrite,
                                    keyColumns,
                                    info);

  return result;
}

// Note that the caller of this method is responsible for adding those
// new outputs to the group attributes that come from the children and
// for removing the requested inputs. The caller also removes
// "predicatesToRemove" from the selection predicates. This method
// only needs to do the following:

// - Add any new outputs to the char. outputs that are generated
//   directly by this node (not by its children)
// - Add "newPredicatesToAdd" to any other places where predicates
//   are needed, remove then from the selection predicates if they
//   should be stored elsewhere
// - Remove "predicatesToRemove" from this node
//   (not from the children, that is done by the caller)
// - Make sure that "inputsToRemove" isn't referenced anywhere else
//   in this node
NABoolean RelExpr::prepareMeForCSESharing(
     const ValueIdSet &outputsToAdd,
     const ValueIdSet &predicatesToRemove,
     const ValueIdSet &newPredicatesToAdd,
     const ValueIdSet &inputsToRemove,
     ValueIdSet &valuesForVEGRewrite,
     ValueIdSet &keyColumns,
     CSEInfo *info)
{
  // A class derived from RelExpr must explicitly define
  // this method to support being part of a shared CSE

  char buf[100];

  snprintf(buf, sizeof(buf), "Operator %s not supported",
           getText().data());

  info->getConsumer(0)->emitCSEDiagnostics(buf);

  return FALSE;
}

void Join::processCompRefOptConstraints(NormWA * normWAPtr)
{
  if (CmpCommon::getDefault(ELIMINATE_REDUNDANT_JOINS) != DF_OFF)
  {
    GroupAttributes &myGA = *getGroupAttr();
    GroupAttributes &leftGA = *child(0).getGroupAttr();
    GroupAttributes &rightGA = *child(1).getGroupAttr();

    const ValueIdSet &leftConstraints = leftGA.getConstraints();
    const ValueIdSet &rightConstraints = rightGA.getConstraints();

    if (normWAPtr && isInnerNonSemiJoin())
      matchRIConstraint(leftGA,rightGA, normWAPtr) ;

    // Full Outer Join has a join pred that affect the rows that flow from the left
    if (NOT isFullOuterJoin())
      myGA.addSuitableCompRefOptConstraints(leftConstraints,	
					    getSelectionPredicates(), this);
    // only non semi inner join rely solely on selection pred to control rows from
    // thr right. Other joins use a join pred also. 
    if (isInnerNonSemiJoin())
      myGA.addSuitableCompRefOptConstraints(rightConstraints,
					  getSelectionPredicates(), this); 
  }
}


void GroupByAgg::processCompRefOptConstraints(NormWA * normWAPtr)
{
  if (CmpCommon::getDefault(ELIMINATE_REDUNDANT_JOINS) != DF_OFF)
  {
     getGroupAttr()->addSuitableCompRefOptConstraints
      (child(0).getGroupAttr()->getConstraints(),getSelectionPredicates(), this);
  }
}

void Filter::processCompRefOptConstraints(NormWA * normWAPtr)
{
  if (CmpCommon::getDefault(ELIMINATE_REDUNDANT_JOINS) != DF_OFF)
  {
    GroupAttributes &myGA = *getGroupAttr();
    myGA.addSuitableCompRefOptConstraints
    (child(0).getGroupAttr()->getConstraints(),getSelectionPredicates(), this);
  }
}

// ***********************************************************************
// $$$$ Rename
// member functions for class Rename, 
// used by sub-classes: RenameTable and RenameReference
// ***********************************************************************
void Rename::transformNode(NormWA &   normWARef,
                                ExprGroupId & locationOfPointerToMe)
{
  CMPASSERT( this == locationOfPointerToMe );

  // The rename table node has outlived its usefulness; remove from the tree.
  locationOfPointerToMe = child(0);

  // Move the predicates down to my child, OR my grandchild if child is a root.
  // Move the characteristic inputs down to my child, AND my grandchild if
  // child is a root node (the root must have at least as many inputs as any
  // of its children -- see assertion in RelRoot::recomputeOuterRefs).
  //
  // This moving past the root to the grandchild seems like it should be
  // unnecessary, that RelRoot::transformNode would do this for us anyway.
  // The problem is if this RenameTable is in the topmost (outermost) scope
  // and that scope has a predicate containing a subquery -- e.g.
  //    select * from (select a from t1) x where a>(select b from t2);
  // -- without this "grandchild fix" the semijoin introduced by the subquery
  // was being placed above the topmost root (!) and that entire subq pred
  // was being lost.  This was Genesis case 10-970828-6025.

  RelExpr *descendant = child(0);
  descendant->getGroupAttr()->addCharacteristicInputs   // child
                               (getGroupAttr()->getCharacteristicInputs());
  if (descendant->getOperatorType() == REL_ROOT)
    descendant = descendant->child(0);                        // grandchild
  descendant->selectionPred() += getSelectionPred();  // child or grandchild
  descendant->getGroupAttr()->addCharacteristicInputs // child or grandchild
                               (getGroupAttr()->getCharacteristicInputs());

  // transform my child
  locationOfPointerToMe->transformNode(normWARef, locationOfPointerToMe);

  // -- Triggers
  locationOfPointerToMe->getInliningInfo().merge(&getInliningInfo());

  // Verify that my child or whoever replaced it is now transformed
  CMPASSERT( locationOfPointerToMe->nodeIsTransformed());

} // Rename::transformNode()

//////////////////////////////////////////////////////////////////////////////
// The purpose of this method is to fix the inputs of the tentative branch
// of the before triggers tree. After binding, the inputs for the temp insert
// side are the expressions that represent the NEW values. These expressions
// should not be inputs, but rather calculated in the temp insert node itself
// using as inputs just basic columns. This method calculates the correct 
// inputs based on the inputs from above (inputs of the TSJ node), and the 
// values generated below the tentativeGU node (the OLD values). 
// When this is done, we call the transformNode() method of the superclass.
//////////////////////////////////////////////////////////////////////////////
void BeforeTrigger::transformNode(NormWA & normWARef, 
								  ExprGroupId & locationOfPointerToMe)
{
	// Call the inherited method to do the vanishing trick.
	Rename::transformNode(normWARef, locationOfPointerToMe);

	if (parentTSJ_ != NULL)  // Is this the top most BeforeTrigger node?
	{
		// Find the interesting nodes we need.
		RelExpr *tsjNode    = parentTSJ_;
		RelExpr *rootNode   = tsjNode->child(1);
		RelExpr *tempInsert = rootNode->child(0);
		RelExpr *tupleNode  = tempInsert->child(0);

		// locationOfPointerToMe now points to the node below the tentative
		// node and before triggers. It's outputs are the values generated
		// by the subtree below the original GU (including sub-queries).
		const ValueIdSet& generatedValues =
			locationOfPointerToMe->getGroupAttr()->getCharacteristicOutputs();

		// The inputs of the TSJ node are the values needed from above:
		// transition variables and the executeId value.
		const ValueIdSet& externalInputs =
			tsjNode->getGroupAttr()->getCharacteristicInputs();

		// Together they are the set of basic input values the temp Insert 
		// node needs to evaluate the NEW expressions.
		ValueIdSet minInputs;
		minInputs.insert(generatedValues);
		minInputs.insert(externalInputs);

		// The root node has the max required inputs with all the expressions.
		ValueIdSet maxInputs(rootNode->getGroupAttr()->getCharacteristicInputs());
		// Leave only the inputs required to evaluate the expressions.
// problem is it also weeds out subqueries...
//		maxInputs.weedOutUnreferenced(minInputs);

		// Set the minimum inputs in all the nodes of the temp insert subtree.
		rootNode  ->getGroupAttr()->setCharacteristicInputs(minInputs);
		tempInsert->getGroupAttr()->setCharacteristicInputs(minInputs);
		tupleNode ->getGroupAttr()->setCharacteristicInputs(minInputs);
	}
}

// ***********************************************************************
// $$$$ RelRoutine
// member functions for class RelRoutine
//
// The other intermediate classes derived from RelRoutine does not need 
// their own recomputeOuterReferences() at this point
// That would be classes like 
// 	TableValuedFunction
// 	BuiltinTableValuedFunction
//	IsolatedNonTableUDR
// for example
// ***********************************************************************

// -----------------------------------------------------------------------
// RelRoutine::transformNode()
// -----------------------------------------------------------------------

void RelRoutine::transformNode(NormWA &normWARef,
                               ExprGroupId & locationOfPointerToMe)
{
  if (nodeIsTransformed())
    return;


  // ---------------------------------------------------------------------
  // Transform the computable expressions associated with me.
  // If a subquery appears in the compute list, then let the subquery
  // transformation cause a join to be performed between the 
  // node were we found the reference to the UDF on the left
  // and the UDF on the right.
  //
  // Note that we procInputParamsVids and procInputAllParamsVids may now
  // be divergent since we don't transform the the procAllParamsVids
  // So we really should not use procAllParamsVids any more!
  // ---------------------------------------------------------------------
  if (getProcInputParamsVids().transformNode(normWARef, locationOfPointerToMe,
                                 getGroupAttr()->getCharacteristicInputs()))
    {
      // -----------------------------------------------------------------
      // Transform my new child.
      // -----------------------------------------------------------------
      locationOfPointerToMe->transformNode(normWARef, locationOfPointerToMe);
    }

  // Make sure all the normal stuff is taken care of.
  // We need to do this before we transforms the inputs so that
  // we deal with the Tuple Child CallSp inserts below for subqueries 
  // as an input.  
  //
  // A call with a subquery as an input parameter gets transfered to something
  // like this:
  //
  // At bind time the RelExpr tree for a Call with a subquery as an input 
  // looks like this: 
  //
  //    CallSP
  //        \ 
  //       Tuple(Subq)
  //
  // After transform it looks like this:
  // 
  //    CallSP
  //       \
  //       Join
  //       /   \
  //   Tuple   GrbyAgg
  //               \
  //               Scan
  //
  // UDFs will not have the Tuple child and its subqueries
  // and UDF as inputs was transformed when we transformed the UDFunction
  // ItemExpr earlier.

  // transform the selection predicates

  transformSelectPred(normWARef, locationOfPointerToMe);

  primeGroupAttributes();

  markAsTransformed();	
}

// -----------------------------------------------------------------------
// RelRoutine::recomputeOuterReferences()
// -----------------------------------------------------------------------
void RelRoutine::recomputeOuterReferences()
{
  // ---------------------------------------------------------------------
  // Delete all those input values that are not referenced
  // by the input parameters.
  // ---------------------------------------------------------------------
  if (NOT getGroupAttr()->getCharacteristicInputs().isEmpty())
    {
      ValueIdSet outerRefs = getGroupAttr()->getCharacteristicInputs();
 
      // Weed out those inputs not needed by my parameters or
      // by my predicates
      GroupAttributes emptyGA;
      ValueIdSet leafExprSet, emptySet;
      ValueIdSet exprSet(getProcInputParamsVids());

      exprSet.getLeafValuesForCoverTest(leafExprSet, emptyGA, emptySet); 
      leafExprSet += getSelectionPred();

      leafExprSet.weedOutUnreferenced(outerRefs);

      getGroupAttr()->setCharacteristicInputs(outerRefs);
    }
}


// -----------------------------------------------------------------------
// RelRoutine::rewriteNode()
// -----------------------------------------------------------------------
void RelRoutine::rewriteNode(NormWA &normWARef)
{
  // ---------------------------------------------------------------------
  // Make sure to rewrite all of our parameter inputs and predicates.
  // ---------------------------------------------------------------------
  selectionPred().normalizeNode(normWARef);
  getProcInputParamsVids().normalizeNode(normWARef);
  getProcOutputParamsVids().normalizeNode(normWARef);
  getProcAllParamsVids().normalizeNode(normWARef);

  // if a CallSP had a subquery  or UDFs as an input parameter it gets attached
  // as child(0) at bind time, so we need to rewrite it too. This
  // child gets moved by the optimizer - UdrToTSJFlow rule.

  // If IsolatedScalarUDFs,  on the other hand, contains subqueries or UDFs in 
  // its input parameters, we transform those the normal way at transform
  // time.

  if (child(0) != NULL) 
     child(0)->rewriteNode(normWARef);

  // ---------------------------------------------------------------------
  // Rewrite my own Group Attributes
  // ---------------------------------------------------------------------
  getGroupAttr()->normalizeInputsAndOutputs(normWARef);
}

// ***********************************************************************
// $$$$ Tuple
// member functions for class Tuple
// ***********************************************************************
void Tuple::transformNode(NormWA & normWARef,
                               ExprGroupId &locationOfPointerToMe)
{
  CMPASSERT( this == locationOfPointerToMe );

  if (nodeIsTransformed())
    return;
  //markAsTransformed();	NO!  We call RelExpr::transformNode() below!

  ValueIdSet subqueryOrIsolatedUDFunctionPredicates;

  // remove the subquery or Isolated UDFunction predicates from the 
  // tupleExpr() list
  tupleExpr().removeSubqueryOrIsolatedUDFunctionPredicates(
                            subqueryOrIsolatedUDFunctionPredicates);

  // -- Triggers
  getGroupAttr()->setCharacteristicOutputs(tupleExpr());

  // ---------------------------------------------------------------------
  // Save the original inputs to use when the subquery predicates get
  // transformed.
  // ---------------------------------------------------------------------
  ValueIdSet externalInputs = getGroupAttr()->getCharacteristicInputs();

  // Let RelExpr:: do the work
  RelExpr::transformNode(normWARef, locationOfPointerToMe);

  // ---------------------------------------------------------------------
  // Transform the subqueries or Isolated UDFunctions in the tupleExpr() list
  // ---------------------------------------------------------------------
  // semiJoin's that are added should be added directly below my
  // original parent
  if (subqueryOrIsolatedUDFunctionPredicates.transformNode(normWARef, 
                                       locationOfPointerToMe,
                                       externalInputs))
    {
      locationOfPointerToMe->transformNode(normWARef,
                                           locationOfPointerToMe);
      // We are on our way back from a number of transformNode()s.
      // Let's just make sure that the final usurper got transformed
      CMPASSERT( locationOfPointerToMe->nodeIsTransformed());
    }

} // Tuple::transformNode()

// -----------------------------------------------------------------------
// Tuple::recomputeOuterReferences()
// -----------------------------------------------------------------------
void Tuple::recomputeOuterReferences()
{
  // ---------------------------------------------------------------------
  // Delete all those input values that are no longer referenced on
  // this operator because the predicates that reference them have 
  // been pulled up.
  // ---------------------------------------------------------------------
  ValueIdSet outerRefs = getGroupAttr()->getCharacteristicInputs(); 

  ValueIdSet allMyExpr(getSelectionPred());
  allMyExpr.insertList(tupleExpr());

  allMyExpr.weedOutUnreferenced(outerRefs);

  getGroupAttr()->setCharacteristicInputs(outerRefs);
} // Tuple::recomputeOuterReferences()  

// -----------------------------------------------------------------------
// Tuple::rewriteNode()
// -----------------------------------------------------------------------
void Tuple::rewriteNode(NormWA & normWARef)
{
  // ---------------------------------------------------------------------
  // Rewrite the tuple expressions
  // ---------------------------------------------------------------------
  if (tupleExpr().normalizeNode(normWARef))
    {
    }
  // ---------------------------------------------------------------------
  // Rewrite the selection expressions
  // ---------------------------------------------------------------------
  if (selectionPred().normalizeNode(normWARef))
    {
    }
  // ---------------------------------------------------------------------
  // Rewrite my own Group Attributes
  // ---------------------------------------------------------------------
  getGroupAttr()->normalizeInputsAndOutputs(normWARef);
} // Tuple::rewriteNode()

// -----------------------------------------------------------------------
// Tuple::normalizeNode()
// -----------------------------------------------------------------------
RelExpr * Tuple::normalizeNode(NormWA & normWARef)
{
  // -- Triggers
  // If predicates should not be pushed down here, delete them.
  if (rejectPredicates() && !selectionPred().isEmpty())
    selectionPred().clear();
  
  // Let RelExpr:: do the work
  return RelExpr::normalizeNode(normWARef);
}

// ***********************************************************************
// member functions for class TupleList
// ***********************************************************************
void TupleList::transformNode(NormWA & normWARef,
                               ExprGroupId &locationOfPointerToMe)
{
  Tuple::transformNode(normWARef, locationOfPointerToMe);
} // TupleList::transformNode()

// -----------------------------------------------------------------------
// TupleList::recomputeOuterReferences()
// -----------------------------------------------------------------------
void TupleList::recomputeOuterReferences()
{
  ValueIdSet outerRefs = getGroupAttr()->getCharacteristicInputs();

  ValueIdSet allMyExpr(getSelectionPred());
  ValueIdSet refExpr, emptySet;
  GroupAttributes emptyGA;
  allMyExpr.insertList(tupleExpr());

  tupleExprTree()->getLeafValuesForCoverTest(refExpr, emptyGA, emptySet);
  allMyExpr += refExpr;

  allMyExpr.weedOutUnreferenced(outerRefs);

  getGroupAttr()->setCharacteristicInputs(outerRefs);

} // TupleList::recomputeOuterReferences()  

// -----------------------------------------------------------------------
// TupleList::rewriteNode()
// -----------------------------------------------------------------------
void TupleList::rewriteNode(NormWA & normWARef)
{
  Tuple::rewriteNode(normWARef);
} // TupleList::rewriteNode()

// ***********************************************************************
// Member functions for class Transpose
// ***********************************************************************

// Transpose::transformNode() -------------------------------------------
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
// This implementation is basically the same as the RelExpr:transformNode,
// but here we need to tranform each member of each ValueIdUnion of
// transUnionVals().
//
void Transpose::transformNode(NormWA &normWARef,
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

  // The child has now been transformed.
  // A new semiJoin may now be my direct descendant and my original
  // child a descendant of it.
  // In either case my child has now been transformed.

  RelExpr *origChild = child(0); // My original child

  // Transform each expression of each ValueIdUnion.
  // (Do not transform the ValueIdIUnion, but each of its members)
  // The keyCol ValueIdUnion does not need to be transformed,
  // so the loop index could start at 1.
  //
  for(CollIndex v = 0; v < transUnionVectorSize(); v++) {
    ValueIdList &valIdList = transUnionVector()[v];

    for(CollIndex i = 0; i < valIdList.entries(); i++) {
      ValueIdUnion *valIdu = ((ValueIdUnion *)valIdList[i].
                              getValueDesc()->getItemExpr());

      CollIndex numEntries = valIdu->entries();

      for(CollIndex j = 0; j < numEntries; j++) {

        // original expression before transformation.
        //
        ItemExpr * iePtr = valIdu->getSource(j).getItemExpr(); 

        // The transformed expression.
        //
        ExprValueId nePtr(iePtr);                   

        // Transform the Item Expression.
        iePtr->transformNode(normWARef,
                             nePtr, 
                             child(0), 
                             getGroupAttr()->getCharacteristicInputs());
                           
        // If the original expression was transformed, update the entry
        // in the ValueIdUnion
        //
        if (nePtr != (const ItemExpr *)iePtr) {
          valIdu->setSource(j, nePtr->getValueId());
        }
      }
    }
  }

  if(origChild != child(0)) {
    // The transpose expressions were on a subquery that had not been
    // processed before. Normalize the new tree that has become 
    // our child.
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

} // Transpose::transformNode()

// Transpose::rewriteNode() ---------------------------------------------
// rewriteNode() is the virtual function that computes
// the transitive closure for "=" predicates and rewrites value
// expressions.
//
// Parameters:
//
// NormWA & normWARef
//    IN : a pointer to the normalizer work area
//
// This implementation is basically the same as RelExpr::rewriteNode()
// but here we need to normalize each member of each ValueIdUnion of
// transUnionVals().
//
void Transpose::rewriteNode(NormWA & normWARef)
{
  // Rewrite the expressions of the child node.
  //
  child(0)->rewriteNode(normWARef);

  // normalize each member of each ValueIdUnion of transUnionVals().
  // (may be able to get away without normalizing the first (key Values)
  // ValueIdUnion. If this is so, the index could start at 1).
  //
  for(CollIndex v = 0; v < transUnionVectorSize(); v++) {
    ValueIdList &valIdList = transUnionVector()[v];

    for(CollIndex i = 0; i < valIdList.entries(); i++) {
      ValueIdUnion *valIdu = ((ValueIdUnion *)valIdList[i].
                              getValueDesc()->getItemExpr());
      
      CollIndex numEntries = valIdu->entries();
    
      // Normalize each expression.  This may generate new
      // ValueIds for the members of the ValueIdUnion.
      //
      for(CollIndex j = 0; j < numEntries; j++) {
        valIdu->normalizeSpecificChild(normWARef, j);
      }
    }
  }

  // Rewrite the expressions in the selection preidcates.
  //
  if (selectionPred().normalizeNode(normWARef))
    {
    }

  // ++MV
  if (getUniqueColumns().normalizeNode(normWARef))
    {
    }
  // --MV

  // Rewrite the expressions in the Group Attributes.
  //
  getGroupAttr()->normalizeInputsAndOutputs(normWARef);
} // Transpose::rewriteNode()

// Transpose::recomputeOuterReferences() --------------------------------
// This method is used by the normalizer for recomputing the
// outer references (external dataflow input values) that are
// still referenced by each operator in the subquery tree
// after the predicate pull up is complete.
//
// Side Effects: sets the characteristicInputs of the groupAttr.
//
void Transpose::recomputeOuterReferences()
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

  // Add the valueIds of each member of each ValueIdUnion of transUnionVals().
  //
  for(CollIndex v = 0; v < transUnionVectorSize(); v++) {
    ValueIdList &valIdList = transUnionVector()[v];

    for(CollIndex i = 0; i < valIdList.entries(); i++) {
      ValueIdUnion *valIdu = ((ValueIdUnion *)valIdList[i].
                              getValueDesc()->getItemExpr());
      
      CollIndex numEntries = valIdu->entries();
      
      for(CollIndex j = 0; j < numEntries; j++) {
        
        // Add the valueIds of each member.
        //
        allMyExpr += valIdu->getSource(j);
      }
    }
  }

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
} // Transpose::recomputeOuterReferences()  

// ***********************************************************************
// Member functions for class Pack
// ***********************************************************************

// -----------------------------------------------------------------------
// Pack::pullUpPreds() is refined to disallow the pullup of predicates
// from the operator's child which may be made up of non-packed columns.
// The pack node packs all the columns it receives from its child and
// predicates evaluated by child couldn't be evaluated here on the packed
// columns any more.
// -----------------------------------------------------------------------
void Pack::pullUpPreds()
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

// -----------------------------------------------------------------------
// Pack::recomputeOuterReferences() adds the packing factor to be the
// additional outer references needed by the Pack node.
// -----------------------------------------------------------------------
void Pack::recomputeOuterReferences()
{
  // Original set of outer references.
  ValueIdSet outerRefs = getGroupAttr()->getCharacteristicInputs();

  // The set of valueIds need by the Pack operator.
  ValueIdSet allMyExpr(getSelectionPred());
  allMyExpr += packingFactor();
  allMyExpr.insertList(packingExpr());
  allMyExpr.insertList(requiredOrder());

  // Remove from outerRefs those valueIds that are not needed by allMyExpr.
  allMyExpr.weedOutUnreferenced(outerRefs);

  // Add to outerRefs those that my children need.
  outerRefs += child(0).getPtr()->getGroupAttr()->getCharacteristicInputs();

  // Set my characteristic inputs to this new minimal set.
  getGroupAttr()->setCharacteristicInputs(outerRefs);
}

// -----------------------------------------------------------------------
// Pack::tranformNode() tranforms the packing expression which might has
// a subquery in it.
// -----------------------------------------------------------------------
void Pack::transformNode(NormWA & normWARef,
                         ExprGroupId & locationOfPointerToMe)
{
  CMPASSERT(this == locationOfPointerToMe);

  if(nodeIsTransformed()) return;
  markAsTransformed();

  // Make inputs available to child
  child(0)->getGroupAttr()->addCharacteristicInputs
	                    (getGroupAttr()->getCharacteristicInputs());

  // ---------------------------------------------------------------------
  // Transform the child
  // ---------------------------------------------------------------------
  child(0)->transformNode(normWARef,child(0));

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

  // ---------------------------------------------------------------------
  // Transform the computable expressions associated with me.
  // If a subquery appears in the compute list, then let the subquery
  // transformation cause a semijoin to be performed between Pack and its
  // child.
  // ---------------------------------------------------------------------
  if(packingExpr_.transformNode(normWARef,
                                child(0),
                                getGroupAttr()->getCharacteristicInputs()))
  {
    // -------------------------------------------------------------------
    // Transform my new child.
    // -------------------------------------------------------------------
    child(0)->transformNode(normWARef,child(0));
  }

  // Pull up the predicates and recompute the required inputs
  // of whoever my children are now.
  pullUpPreds();

  // transform the selection predicates
  transformSelectPred(normWARef,locationOfPointerToMe);
}

// -----------------------------------------------------------------------
// Pack::rewriteNode() needs to rewrite the packing expressions as well
// as the selPreds and the inputs/outputs.
// -----------------------------------------------------------------------
void Pack::rewriteNode(NormWA& normWA)
{
  // First rewrite the child node.
  child(0)->rewriteNode(normWA);

  // Rewrite the Pack node's own expressions and its inputs/outputs.
  packingFactor().normalizeNode(normWA);
  packingExpr().normalizeNode(normWA);
  selectionPred().normalizeNode(normWA);
  requiredOrder().normalizeNode(normWA);

  getGroupAttr()->normalizeInputsAndOutputs(normWA);
}

// ***********************************************************************
// $$$$ CommonSubExprRef
// member functions for class CommonSubExprRef
// ***********************************************************************

void CommonSubExprRef::transformNode(NormWA & normWARef,
                                     ExprGroupId & locationOfPointerToMe)
{
  CMPASSERT( locationOfPointerToMe.getPtr() == this );

  if (nodeIsTransformed())
    return;
  markAsTransformed();

  // set lexicalRefNumFromParent_ for expanded refs, now that
  // we can be sure the lexical ref has been bound
  if (isAnExpansionOf_)
    lexicalRefNumFromParent_ = isAnExpansionOf_->lexicalRefNumFromParent_;

  // Allocate a new VEG region for the child, to prevent VEGies that
  // cross the potentially common part and the rest of the query tree.
  //normWARef.allocateAndSetVEGRegion(EXPORT_ONLY, this);
  child(0)->getGroupAttr()->addCharacteristicInputs(
       getGroupAttr()->getCharacteristicInputs());
  child(0)->transformNode(normWARef, child(0)); 
  pullUpPreds();
  transformSelectPred(normWARef, locationOfPointerToMe);
  //normWARef.restoreOriginalVEGRegion();
}

void CommonSubExprRef::pullUpPreds()
{
  // To preserve the commonality of common subexpressions, we
  // don't allow to pull predicates out of them.

  // so do nothing here, preventing predicate pull-up

  // alternatively, we could do the pull-up and record the
  // pulled-up predicates here
  // RelExpr::pullUpPreds();
  // pulledPredicates_ += selectionPred();
}

void CommonSubExprRef::pushdownCoveredExpr(
     const ValueIdSet & outputExpr,
     const ValueIdSet & newExternalInputs,
     ValueIdSet & predicatesOnParent,
     const ValueIdSet * setOfValuesReqdByParent,
     Lng32 childIndex)
{
  // Remember the predicates we pushed down, since other consumers of
  // this CSE may not have pushed the equivalent
  // predicates. Therefore, if we want to materialize a common
  // subexpressions, any predicates that were pushed down and are not
  // common to all the consumers must be pulled back out before we can
  // share a common query tree.
  ValueIdSet predsPushedThisTime(predicatesOnParent);

  if (pushedPredicates_.isEmpty())
    // this is also the time to record the original set of inputs
    // for this node, before predicate pushdown can alter the inputs
    commonInputs_ = getGroupAttr()->getCharacteristicInputs();

  RelExpr::pushdownCoveredExpr(outputExpr,
                               newExternalInputs,
                               predicatesOnParent,
                               setOfValuesReqdByParent,
                               childIndex);

  predsPushedThisTime -= predicatesOnParent;
  pushedPredicates_   += predsPushedThisTime;
}

void CommonSubExprRef::rewriteNode(NormWA & normWARef)
{
  RelExpr::rewriteNode(normWARef);

  nonVEGColumns_ = columnList_;
  columnList_.normalizeNode(normWARef);
  commonInputs_.normalizeNode(normWARef);

  normWARef.incrementCommonSubExprRefCount();
}

RelExpr * CommonSubExprRef::semanticQueryOptimizeNode(NormWA & normWARef)
{
  RelExpr *result = this;
  NABoolean ok = TRUE;
  CSEInfo *info = CmpCommon::statement()->getCSEInfo(internalName_);

  // do the analysis top-down
  analyzeAndPrepareForSharing(*info);

  RelExpr::semanticQueryOptimizeNode(normWARef);

  switch (info->getAnalysisOutcome(id_))
    {
    case CSEInfo::EXPAND:
      // Not able to share the CSE, expand the CSE by eliminating
      // this node and putting its child tree in its place. In this
      // case, analyzeAndPrepareForSharing() left the tree unchanged.
      result = child(0).getPtr();
      break;

    case CSEInfo::CREATE_TEMP:
      determineTempTableType(*info);
      if (createTempTable(*info))
        {
          RelExpr *ins = createInsertIntoTemp(*info, normWARef);

          if (ins)
            info->setInsertIntoTemp(ins);
          else
            result = NULL;
        }
      else
        result = NULL;

      if (!result)
        break;
      // fall through to the next case

    case CSEInfo::TEMP:
      // We are able to share this CSE between multiple consumers.
      // Replace this node with a scan on the temp table that
      // holds the CSE results.
      result = createTempScan(*info, normWARef);
      break;

    case CSEInfo::ERROR:
      // diags should be set
      CMPASSERT(CmpCommon::diags()->mainSQLCODE() < 0);
      break;

    default:
      CMPASSERT(0);
    }

  if (result == NULL)
    emitCSEDiagnostics("Error in creating temp table or temp table insert",
                       TRUE);

  return result;
}

NABoolean CommonSubExprRef::prepareMeForCSESharing(
     const ValueIdSet &outputsToAdd,
     const ValueIdSet &predicatesToRemove,
     const ValueIdSet &commonPredicatesToAdd,
     const ValueIdSet &inputsToRemove,
     ValueIdSet &valuesForVEGRewrite,
     ValueIdSet &keyColumns,
     CSEInfo *info)
{
  // the caller of this method already took care of the adjustments to
  // make, just make sure that all predicates could be pushed down to
  // the child

  if (!getSelectionPred().isEmpty())
    {
      // this should not happen
      emitCSEDiagnostics("Unable to push common predicates into child tree");
      return FALSE;
    }
  return TRUE;
}

CSEInfo::CSEAnalysisOutcome CommonSubExprRef::analyzeAndPrepareForSharing(CSEInfo &info)
{
  // do a few simple shortcuts first

  // Make sure this consumer is in the main list of consumers. Note
  // that the analysis is done top-down and that currently the only
  // two places where we make copies of the tree are in
  // RelRoot::semanticQueryOptimizeNode() and in this method. The copy
  // made in the root is only used when we bypass SQO completely.
  // Although we may sometimes look at unused copies during the CSE
  // analysis phase, this guarantees (for now) that the analyzing
  // consumer always is and stays in the list of consumers. If we ever
  // make additional copies of the tree we may need to reconsider this
  // logic.
  if (info.getConsumer(id_) != this)
    {
      info.replaceConsumerWithAnAlternative(this);
      DCMPASSERT(info.getConsumer(id_) == this);
    }

  // If another consumer has already done the analysis, return its result.
  // Note: Right now, all the consumers do the same, in the future, we could
  // expand some and share others.
  if (info.getIdOfAnalyzingConsumer() >= 0)
    return info.getAnalysisOutcome(id_);

  // mark me as the analyzing consumer
  info.setIdOfAnalyzingConsumer(id_);

  if (CmpCommon::getDefault(CSE_USE_TEMP) == DF_OFF)
    {
      emitCSEDiagnostics("Forced with CQD CSE_USE_TEMP CQD 'off'");
      info.setAnalysisOutcome(CSEInfo::EXPAND);
      return CSEInfo::EXPAND;
    }

  CSEInfo::CSEAnalysisOutcome result = CSEInfo::UNKNOWN_ANALYSIS;
  NABoolean canShare = TRUE;
  NABitVector neededColumnsBitmap;
  ValueIdList tempTableColumns;
  const ValueIdSet &charOutputs(getGroupAttr()->getCharacteristicOutputs());
  CollIndex numConsumers = info.getNumConsumers();
  RelExpr *copyOfChildTree = NULL;

  // A laundry list of changes to undo the effects of normalization,
  // specifically of pushing predicates down and of minimizing the
  // outputs. Also, a list of new common selection predicates to add.
  ValueIdSet outputsToAdd;
  ValueIdSet predicatesToRemove(pushedPredicates_);
  ValueIdSet newPredicatesToAdd;
  ValueIdSet commonPredicates(pushedPredicates_);
  ValueIdSet inputsToRemove(child(0).getGroupAttr()->getCharacteristicInputs());
  ValueIdSet *nonCommonPredicatesArray =
    new(CmpCommon::statementHeap()) ValueIdSet[numConsumers];
  ValueIdMap *myColsToConsumerMaps =
    new(CmpCommon::statementHeap()) ValueIdMap[numConsumers];
  ItemExpr *nonCommonPredicatesORed = NULL;
  int numORedPreds = 0;
  NABoolean singleLexicalRefWithTempedAncestors =
    (info.getNumLexicalRefs() == 1);
  Int32 numPreliminaryRefs = 0;
  ValueIdSet childTreeKeyColumns;

  // ------------------------------------------------------------------
  // CSE Analysis phase
  // ------------------------------------------------------------------

  // loop over the consumers of the CSE to negotiate a common set
  // of columns to retrieve and a common set of predicates that can
  // remain pushed down
  for (CollIndex c=0; c<numConsumers && canShare; c++)
    {
      CommonSubExprRef *consumer = info.getConsumer(c);

      const ValueIdList &cCols(consumer->columnList_);
      ValueIdSet availableValues(cCols);
      ValueIdSet requiredValues(
           consumer->getGroupAttr()->getCharacteristicOutputs());
      const ValueIdSet &cPreds(consumer->pushedPredicates_);
      ValueIdSet mappedPreds;
      ValueId dummy;
      CSEInfo *infoToCheck = &info;
      CommonSubExprRef *childToCheck = consumer;
      NABoolean ancestorIsTemped = FALSE;

      // look for a chain of only lexical ancestors of which one is
      // materialized in a temp table
      while (!ancestorIsTemped &&
             infoToCheck->getNumLexicalRefs() == 1 &&
             childToCheck &&
             childToCheck->parentRefId_ >= 0)
        {
          // look at the ancestor and what it is planning to do
          infoToCheck = CmpCommon::statement()->getCSEInfoById(
               childToCheck->parentCSEId_);
          CMPASSERT(infoToCheck);
          CommonSubExprRef *parent =
            infoToCheck->getConsumer(childToCheck->parentRefId_);
          CSEInfo::CSEAnalysisOutcome parentOutcome =
            infoToCheck->getAnalysisOutcome(parent->getId());

          if (parentOutcome == CSEInfo::CREATE_TEMP ||
              parentOutcome == CSEInfo::TEMP)
            ancestorIsTemped = TRUE;

          childToCheck = parent;
        }

      if (!ancestorIsTemped)
        singleLexicalRefWithTempedAncestors = FALSE;

      requiredValues += cPreds;
      availableValues +=
        consumer->getGroupAttr()->getCharacteristicInputs();

      // Do a sanity check whether we can produce the required
      // values (outputs and predicates) from the available values
      // (tables of the original subexpression, to be a temp table).
      // If not, one reason could be that we copied an expression
      // and now have different ValueIds. This could be improved.
      if (requiredValues.removeUnCoveredExprs(availableValues))
        {
          emitCSEDiagnostics(
               "Characteristic outputs not covered by common subexpression");
          canShare = FALSE;
        }

      // Check the required values of this consumer and add all of
      // them (by position number of the original list) to the bit
      // vector of required columns. Note that we might be able to
      // optimize this somewhat for expressions.
      for (CollIndex i=0; i<cCols.entries(); i++)
        if (requiredValues.referencesTheGivenValue(cCols[i],
                                                   dummy,
                                                   TRUE,
                                                   TRUE))
          neededColumnsBitmap += i;

      if (!cPreds.isEmpty())
        if (consumer->id_ == id_)
          {
            // Assert for now that we are still seeing the same node,
            // not a copy.  If this fails, think about whether making
            // a copy might cause issues here, e.g. because some of
            // the information has diverged.
            DCMPASSERT(consumer == this);

            // consumer is the same as "this"
            mappedPreds = cPreds;
          }
        else
          {
            // another consumer, likely to use different ValueIds

            // a ValueIdMap that maps my columns (top) to those of the
            // other consumer (bottom)
            ValueIdSet vegRefsWithDifferingConsts;
            ValueIdSet vegRefsWithDifferingInputs;

            myColsToConsumerMaps[c] = ValueIdMap(columnList_, cCols);

            // make sure we can also map VEGPreds for any VEGRefs in the map
            myColsToConsumerMaps[c].augmentForVEG(
                 TRUE,  // add VEGPreds for existing VEGRefs
                 FALSE, // no need to add more VEGRefs
                 TRUE,  // only do this if constants match
                 // only do this if the VEGies refer to
                 // the same outputs
                 &(getGroupAttr()->getCharacteristicInputs()),
                 &(consumer->getGroupAttr()->getCharacteristicInputs()),
                 &vegRefsWithDifferingConsts,
                 &vegRefsWithDifferingInputs);

            // for now, don't work on trees that have VEGies with differing
            // constants or inputs
            if (vegRefsWithDifferingConsts.entries() > 0)
              {
                info.addVEGRefsWithDifferingConstants(vegRefsWithDifferingConsts);
                emitCSEDiagnostics(
                     "Encountered VEGs with different constants in different consumers");
                canShare = FALSE;
              }

            if (vegRefsWithDifferingInputs.entries() > 0)
              {
                info.addVEGRefsWithDifferingInputs(vegRefsWithDifferingInputs);
                emitCSEDiagnostics("Encountered VEGs with different characteristic inputs");
                canShare = FALSE;
              }

            // Check the inputs, all of the consumers must have the same inputs
            // (parameters). We could see differences if query caching decides
            // to parameterize the copies of the CTEs differently.
            if (consumer->commonInputs_ != commonInputs_)
              {
                emitCSEDiagnostics(
                     "Differing inputs in CTE references, try CQD QUERY_CACHE '0'");
                canShare = FALSE;
              }

            // rewrite the predicates on the consumer in terms of my
            // own ValueIds
            myColsToConsumerMaps[c].rewriteValueIdSetUp(mappedPreds, cPreds);

            commonPredicates.findCommonSubexpressions(mappedPreds, FALSE);

          }
      // Save the mapped preds for later.
      // Note: These are not final yet, until we have found
      // common predicates among all the consumers.
      nonCommonPredicatesArray[c] = mappedPreds;
    }

  if (singleLexicalRefWithTempedAncestors)
    {
      // if all the parent refs are materialized and each one is a
      // copy of a single lexical ref, then that means that we will
      // evaluate this CSE only once, therefore no need to materialize
      // it
      emitCSEDiagnostics(
           "expression is only evaluated once because parent is materialized");
      canShare = FALSE;
    }

  // translate the bit vector of required columns into a set of values
  // that are required (by other consumers) but are not produced by my
  // child tree
  makeValueIdListFromBitVector(tempTableColumns,
                               columnList_,
                               neededColumnsBitmap);
  outputsToAdd.insertList(tempTableColumns);
  info.setNeededColumns(neededColumnsBitmap);
  predicatesToRemove -= commonPredicates;
  info.setCommonPredicates(commonPredicates);

  if (canShare && info.getNeededColumns().entries() == 0)
    {
      // Temp table has no columns, looks like all we care about is
      // the number of rows returned. This is not yet supported. We
      // could make a table with a dummy column.
      emitCSEDiagnostics("Temp table with no columns is not yet supported");
      canShare = FALSE;
    }

  // Make an ORed predicate of all those non-common predicates of the
  // consumers, to be applied on the common subexpression when creating
  // the temp table. Also determine non-common predicates to be applied
  // when scanning the temp table.
  for (CollIndex n=0; n<numConsumers && canShare; n++)
    {
      // Now that we have the definitive set of common predicates,
      // we can get the "uncommon" predicates, i.e. those that
      // have to be evaluated on the individual scans of the temp
      // tables. What we can do, however, is to OR these "uncommon"
      // predicates and apply that OR predicate when building the
      // temp table.

      // repeat step from above, but this time remove the common
      // preds from the array of non-common ones
      commonPredicates.findCommonSubexpressions(nonCommonPredicatesArray[n],
                                                TRUE);

      if (nonCommonPredicatesArray[n].entries() > 0)
        {
          if (numORedPreds == n)
            {
              // build the ORed predicate
              ItemExpr *uncommonPreds =
                nonCommonPredicatesArray[n].rebuildExprTree();

              if (nonCommonPredicatesORed)
                nonCommonPredicatesORed =
                  new(CmpCommon::statementHeap()) BiLogic(
                       ITM_OR,
                       nonCommonPredicatesORed,
                       uncommonPreds);
              else
                nonCommonPredicatesORed = uncommonPreds;

              numORedPreds++;
            }
          
          // rewrite the non-common predicates in terms of the consumer
          // (the ValueIdMap should in many cases already have the
          // correct translation)
          myColsToConsumerMaps[n].rewriteValueIdSetDown(
               nonCommonPredicatesArray[n],
               info.getConsumer(n)->nonSharedPredicates_);
        }
    }

  // adding the ORed non-common predicates makes sense only if all
  // consumers have some such predicate. If at least one consumer
  // doesn't, that's equivalent to a TRUE predicate, and TRUE OR x is
  // always TRUE.
  if (numORedPreds == numConsumers)
    {
      nonCommonPredicatesORed->synthTypeAndValueId();

      newPredicatesToAdd += nonCommonPredicatesORed->getValueId();
      info.addCommonPredicates(newPredicatesToAdd);
    }


  // ------------------------------------------------------------------
  // Preparation phase
  // ------------------------------------------------------------------

  if (canShare)
    {
      // make a copy of the child tree, so we can revert back to the
      // original tree if things don't work out
      copyOfChildTree = child(0)->copyRelExprTree(CmpCommon::statementHeap());

      outputsToAdd -= child(0).getGroupAttr()->getCharacteristicOutputs();
      inputsToRemove -= commonInputs_;

      canShare = copyOfChildTree->prepareTreeForCSESharing(
           outputsToAdd,
           predicatesToRemove,
           newPredicatesToAdd,
           inputsToRemove,
           nonVEGColumns_,
           childTreeKeyColumns,
           &info);

      if (!canShare)
        emitCSEDiagnostics("Failed to prepare child tree for materialization");
      else if (!copyOfChildTree->getGroupAttr()->getCharacteristicOutputs().contains(
                    outputsToAdd))
        {
          // we failed to produce the requested additional outputs
          emitCSEDiagnostics("Failed to produce all the required output columns");
          canShare = FALSE;
        }
      else
        {
          // remember est. log. props of the child, those will be transplanted
          // into the temp scan later
          cseEstLogProps_ =
            copyOfChildTree->getGroupAttr()->outputLogProp(
                 (*GLOBAL_EMPTY_INPUT_LOGPROP));
          // Get a preliminary bearing on how many times we are going
          // to evaluate this CSE if it isn't shared. Note that this
          // looks at the parent CSE's analysis outcome, and not all
          // of these parents may be analyzed yet, so this may be an
          // overestimate.
          numPreliminaryRefs = info.getTotalNumRefs();

          for (CollIndex k=0; k<tempTableColumns.entries(); k++)
            if (childTreeKeyColumns.contains(tempTableColumns[k]))
              info.addCSEKeyColumn(k);
        }
    }

  if (canShare &&
      CmpCommon::getDefault(CSE_USE_TEMP) != DF_ON)
    {
      // When CSE_USE_TEMP is set to SYSTEM, make a heuristic decision

      // calculate some metrics for the temp table, based on row length,
      // cardinality (or max. cardinality) and number of times it is used
      Lng32 tempTableRowLength = tempTableColumns.getRowLength();
      CostScalar cseTempTableSize = cseEstLogProps_->getResultCardinality() *
        tempTableRowLength / numPreliminaryRefs;
      CostScalar cseTempTableMaxSize = cseEstLogProps_->getMaxCardEst() *
        tempTableRowLength / numPreliminaryRefs;
      double maxTableSize =
        ActiveSchemaDB()->getDefaults().getAsDouble(CSE_TEMP_TABLE_MAX_SIZE);
      double maxTableSizeBasedOnMaxCard =
        ActiveSchemaDB()->getDefaults().getAsDouble(CSE_TEMP_TABLE_MAX_MAX_SIZE);

      // cumulative number of key columns referenced in consumers
      Int32 totalKeyColPreds = 0;

      // key cols that are referenced by a predicate in all consumers
      ValueIdSet commonKeyCols(childTreeKeyColumns);

      // check the total size of the temp table, divided by the number
      // of times it is used
      if (maxTableSize > 0 && cseTempTableSize > maxTableSize)
        {
          char buf[200];

          snprintf(buf, sizeof(buf),
                   "Temp table size %e exceeds limit %e",
                   cseTempTableSize.getValue(),
                   maxTableSize);
          emitCSEDiagnostics(buf);
          canShare = FALSE;
        }
      else if (maxTableSizeBasedOnMaxCard > 0 &&
               cseTempTableMaxSize > maxTableSizeBasedOnMaxCard)
        {
          char buf[200];

          snprintf(buf, sizeof(buf),
                   "Temp table size %e (based on max card) exceeds limit %e",
                   cseTempTableMaxSize.getValue(),
                   maxTableSizeBasedOnMaxCard);
          emitCSEDiagnostics(buf);
          canShare = FALSE;
        }

      // determine which "key" columns are referenced by non-common
      // predicates
      for (CollIndex ncp=0; ncp<numConsumers; ncp++)
        {
          const ValueIdSet &nonCommonPreds(nonCommonPredicatesArray[ncp]);
          ValueIdSet tempRefCols;

          tempRefCols.accumulateReferencedValues(childTreeKeyColumns,
                                                 nonCommonPreds);
          totalKeyColPreds += tempRefCols.entries();
          nonCommonPreds.weedOutUnreferenced(commonKeyCols);
        }

      // decide against materialization if the average number of "key"
      // columns referenced in each consumer is greater than
      // CSE_PCT_KEY_COL_PRED_CONTROL percent
      if (totalKeyColPreds >
          (numConsumers * childTreeKeyColumns.entries() *
           ActiveSchemaDB()->getDefaults().getAsDouble(CSE_PCT_KEY_COL_PRED_CONTROL) / 100.0))
        {
          char buf[200];

          snprintf(buf, sizeof(buf),
                   "Number of potential key predicates in consumers (%d) exceeds limit %f",
                   totalKeyColPreds,
                   (numConsumers * childTreeKeyColumns.entries() *
                    ActiveSchemaDB()->getDefaults().getAsDouble(CSE_PCT_KEY_COL_PRED_CONTROL) / 100.0));
          
          emitCSEDiagnostics(buf);
          canShare = FALSE;
        }

      // decide against materialization if the number of key columns
      // referenced by every consumer is > CSE_COMMON_KEY_PRED_CONTROL
      if (commonKeyCols.entries() >
          ActiveSchemaDB()->getDefaults().getAsLong(CSE_COMMON_KEY_PRED_CONTROL))
        {
          char buf[200];
          snprintf(buf, sizeof(buf),
                   "All consumers have a predicate on %d common key columns, limit is %d",
                   commonKeyCols.entries(),
                   ActiveSchemaDB()->getDefaults().getAsLong(CSE_COMMON_KEY_PRED_CONTROL));
          emitCSEDiagnostics(buf);
          canShare = FALSE;
        }
    }

  if (canShare)
    {
      result = CSEInfo::CREATE_TEMP;
      child(0) = copyOfChildTree;
    }
  else if (result == CSEInfo::UNKNOWN_ANALYSIS)
    result = CSEInfo::EXPAND;

  info.setAnalysisOutcome(result);

  return result;
}

void CommonSubExprRef::determineTempTableType(CSEInfo &info)
{
  NABoolean createHiveTable =
    (CmpCommon::getDefault(CSE_HIVE_TEMP_TABLE) == DF_ON);

  if (createHiveTable)
    info.setTempTableType(CSEInfo::HIVE_TEMP_TABLE);
  else
    info.setTempTableType(CSEInfo::VOLATILE_TEMP_TABLE);
}

NABoolean CommonSubExprRef::createTempTable(CSEInfo &info)
{
  int result = TRUE;
  const int maxCSENameLen = 12;
  NAString tempTableName(COM_CSE_TABLE_PREFIX);
  NAString tempTableSchema;
  NAString tempTableCatalog;
  CSEInfo::CSETempTableType tempTableType = info.getTempTableType();
  char buf[32];
  NAString tempTableDDL;
  ValueIdList cols;
  NAString cseNamePrefix(internalName_.data(),
                         MINOF(internalName_.length(),16));

  // Note: Errors at this stage of the process may be recoverable, so
  // we emit only warning diagnostics and just return FALSE if the
  // temp table cannot be created


  // Step 1: Create temp table name
  // ------------------------------

  // we create a name of this form:
  // where
  //   ppp... is a prefix of the CTE name or an internal name
  //          (just to make it easier to identify, not really needed,
  //           we only use letters, digits, underscores)
  //   iii... is the SQL session id
  //          (Hive tables only, to keep different sessions apart)
  //   sss    is the statement number in this session
  //   ccc    is the CSE number in this statement

  // Overall name length is 256, and both HDFS directory and file name
  // can be quite long, so don't allow long user names as well. Note
  // that the user name is just here to improve readability by humans,
  // it's not needed for uniqueness.
  if (cseNamePrefix.length() > maxCSENameLen)
    cseNamePrefix.remove(maxCSENameLen);

  cseNamePrefix.toUpper();
  
  for (int p=0; p<cseNamePrefix.length(); p++)
    {
      char c = cseNamePrefix[p];

      if (!(c >= '0' && c <= '9' ||
            c >= 'A' && c <= 'Z' ||
            c == '_'))
        cseNamePrefix.replace(p,1,"_");
    }

  tempTableName += cseNamePrefix;
  if (tempTableType == CSEInfo::HIVE_TEMP_TABLE)
    {
      tempTableName += "_";
      tempTableName +=
        CmpCommon::context()->sqlSession()->getSessionId();
    }
  snprintf(buf, sizeof(buf), "_S%u_%d",
           CmpCommon::context()->getStatementNum(),
           info.getCSEId());
  tempTableName += buf;

  if (tempTableType == CSEInfo::HIVE_TEMP_TABLE)
    {
      tempTableSchema  = HIVE_SYSTEM_SCHEMA;
      tempTableCatalog = HIVE_SYSTEM_CATALOG;
    }

  info.setTempTableName(QualifiedName(tempTableName,
                                      tempTableSchema,
                                      tempTableCatalog));

  // Step 2: Create the DDL for the temp table
  // -----------------------------------------
       
  tempTableDDL += "CREATE ";
  if (tempTableType == CSEInfo::VOLATILE_TEMP_TABLE)
    tempTableDDL += "VOLATILE ";
  tempTableDDL += "TABLE ";
  if (tempTableType == CSEInfo::HIVE_TEMP_TABLE &&
      tempTableSchema == HIVE_SYSTEM_SCHEMA ||
      tempTableType == CSEInfo::VOLATILE_TEMP_TABLE)
    {
      // Hive table in default schema or volatile table,
      // juse a one-part name
      tempTableDDL += tempTableName;
    }
  else if (tempTableType == CSEInfo::HIVE_TEMP_TABLE)
    {
      // Hive table in a different schema, use a 2 part name
      // (not yet supported)
      tempTableDDL += tempTableSchema;
      tempTableDDL += '.';
      tempTableDDL += tempTableName;
    }
  else
    {
      // use a regular 3-part name
      // (not yet supported)
      tempTableDDL +=
        info.getTempTableName().
          getQualifiedNameAsAnsiString();
    }

  tempTableDDL += "(\n";

  makeValueIdListFromBitVector(cols, columnList_, info.getNeededColumns());

  for (CollIndex c=0; c<cols.entries(); c++)
    {
      char colName[10];
      NAString colType;

      snprintf(colName, sizeof(colName),"  C%05d ", c);
      tempTableDDL +=  colName;
      if (tempTableType == CSEInfo::HIVE_TEMP_TABLE)
        cols[c].getType().getMyTypeAsHiveText(&colType);
      else
        cols[c].getType().getMyTypeAsText(&colType);

      if (colType == "unknown")
        {
          char buf[100];

          colType = "";
          cols[c].getType().getMyTypeAsText(&colType);
          snprintf(buf, sizeof(buf),
                   "Unsupported data type for Hive temp table: %s",
                   colType.data());
          emitCSEDiagnostics(buf, FALSE);
          result = FALSE;
        }

      tempTableDDL += colType;
      if (c+1 < cols.entries())
        tempTableDDL += ",\n";
      else
        tempTableDDL += ")";
    }

  if (result)
    info.setTempTableDDL(tempTableDDL);

  // Step 3: Create the temp table
  // -----------------------------

  if (result)
    if (tempTableType == CSEInfo::HIVE_TEMP_TABLE)
      {
        int m = CmpCommon::diags()->mark();
        if (HiveClient_JNI::executeHiveSQL(tempTableDDL) != HVC_OK)
          {
            if (CmpCommon::statement()->recompiling() ||
                CmpCommon::statement()->getNumOfCompilationRetries() > 0)
              // ignore temp table creation errors if we are
              // recompiling, the temp table may have been
              // created in a previous compilation attempt
              // (if not, we will run into other errors later)
              CmpCommon::diags()->rewind(m);
            else
              {
                result = FALSE;
                // we will fall back to a previous tree and try to
                // recover, make sure there are no errors from our
                // failed attempt in the diags area
                CmpCommon::diags()->negateAllErrors();
                emitCSEDiagnostics(
                     "Error in creating Hive temp table");
              }
          }
      }
    else
      {
        // Todo: CSE: create volatile table
        emitCSEDiagnostics("Volatile temp tables not yet supported");
        result = FALSE;
      }

  // Step 4: Get the NATable for the temp table
  // ------------------------------------------

  if (result)
    {
      BindWA bindWA(ActiveSchemaDB(), CmpCommon::context());
      CorrName cn(info.getTempTableName());
      NATable *tempNATable =
        ActiveSchemaDB()->getNATableDB()->get(cn,
                                              &bindWA,
                                              NULL);

      if (!tempNATable)
        emitCSEDiagnostics("Unable to read metadata for temporary table");
      else
        info.setTempNATable(tempNATable);
    }

  return result;
}

RelExpr * CommonSubExprRef::createInsertIntoTemp(CSEInfo &info, NormWA & normWARef)
{
  RelExpr *result = NULL;
  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context());
  CorrName cn(info.getTempTableName());

  if (!info.getTempNATable())
    // an earlier failure
    return NULL;

  TableDesc *tableDesc =
    bindWA.createTableDesc(info.getTempNATable(),
                           cn,
                           FALSE);
  ValueIdList srcValueList;

  if (info.getTempTableType() == CSEInfo::HIVE_TEMP_TABLE)
    {
      // Create this tree:
      //
      //     BlockedUnion
      //      /        \
      //  Truncate  FastExtract temp
      //    temp         |
      //                cse
      //
      // In this tree "cse" is the child of this node and "temp" is
      // the name of the Hive table. The tree is equivalent to what
      // would be generated by an SQL statement
      // "insert overwite table <temp> <cse>".

      result = FastExtract::makeFastExtractTree(
         tableDesc,
         child(0).getPtr(),
         TRUE,   // overwrite the table
         FALSE,  // called outside the binder
         TRUE,   // this is a table for a common subexpression
         &bindWA);

      CMPASSERT(result->getOperatorType() == REL_UNION &&
                result->child(1)->getOperatorType() == REL_FAST_EXTRACT);
      RelExpr *fe = result->child(1);

      makeValueIdListFromBitVector(srcValueList, columnList_, info.getNeededColumns());
      CMPASSERT(fe->getOperatorType() == REL_FAST_EXTRACT);
      static_cast<FastExtract *>(fe)->setSelectList(srcValueList);
      fe->setGroupAttr(new (CmpCommon::statementHeap()) GroupAttributes());
      fe->getGroupAttr()->addCharacteristicInputs(
           fe->child(0).getGroupAttr()->getCharacteristicInputs());
      result->child(0)->setGroupAttr(
           new (CmpCommon::statementHeap()) GroupAttributes());
      result->setGroupAttr(new (CmpCommon::statementHeap()) GroupAttributes());
      result->getGroupAttr()->addCharacteristicInputs(
           fe->getGroupAttr()->getCharacteristicInputs());

    }
  else
    {
      emitCSEDiagnostics(
           "Unsupported temp table type in createInsertIntoTemp()",
           TRUE);
    }

  info.setInsertIntoTemp(result);

  return result;
}

RelExpr * CommonSubExprRef::createTempScan(CSEInfo &info, NormWA & normWARef) // 
{
  // check for earlier errors
  if (!info.getInsertIntoTemp())
    return NULL;

  MapValueIds *result = NULL;
  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context());
  CorrName cn(info.getTempTableName(),
              CmpCommon::statementHeap(),
              internalName_);
  TableDesc *tableDesc =
    bindWA.createTableDesc(info.getTempNATable(),
                           cn,
                           FALSE,
                           getHint());

  Scan *scan =
    new(CmpCommon::statementHeap()) Scan(cn, tableDesc);

  // Run the new scan through bind and normalization phases, like the
  // rest of the nodes have
  ExprGroupId x(scan);

  scan->bindSelf(&bindWA);
  normWARef.allocateAndSetVEGRegion(IMPORT_ONLY, scan);
  scan->transformNode(normWARef, x);
  CMPASSERT(x.getPtr() == scan);
  scan->rewriteNode(normWARef);
  scan->normalizeNode(normWARef);
  scan->synthLogProp(&normWARef);
  normWARef.restoreOriginalVEGRegion();

  scan->setCommonSubExpr(this);

  // At this point we have a scan node on the temp table, with a new
  // TableDesc that has new ValueIds. Make a map from the new ids to
  // my own.
  ValueIdList myOutputs;
  ValueIdList tempTableOutputList;
  ValueIdList tempTableVEGOutputList;
  ValueIdSet tempTableOutputs;
  ValueIdSet tempTablePreds;

  makeValueIdListFromBitVector(myOutputs, columnList_, info.getNeededColumns());
  tableDesc->getUserColumnList(tempTableOutputList);
  tableDesc->getEquivVEGCols(tempTableOutputList, tempTableVEGOutputList);
  CMPASSERT(myOutputs.entries() == tempTableVEGOutputList.entries());

  ValueIdMap outToTempMap(myOutputs, tempTableVEGOutputList);

  result = new(CmpCommon::statementHeap()) MapValueIds(scan,
                                                       outToTempMap,
                                                       CmpCommon::statementHeap());

  result->setCSERef(this);
  result->addValuesForVEGRewrite(nonVEGColumns_);
  outToTempMap.rewriteValueIdSetDown(getGroupAttr()->getCharacteristicOutputs(),
                                     tempTableOutputs);
  // Todo: CSE: the rewrite below doesn't work with VEGPreds, and the
  // augment method also isn't sufficient
  outToTempMap.rewriteValueIdSetDown(nonSharedPredicates_, tempTablePreds);
  scan->getGroupAttr()->setCharacteristicInputs(
       getGroupAttr()->getCharacteristicInputs());
  scan->getGroupAttr()->setCharacteristicOutputs(tempTableOutputs);
  scan->setSelectionPredicates(tempTablePreds);

  result->setGroupAttr(getGroupAttr());

  return result;
}

void CommonSubExprRef::emitCSEDiagnostics(const char *message, NABoolean forceError)
{
  // Normally this does nothing.
  // With CQD CSE_DEBUG_WARNINGS ON, it emits diagnostics about the reason(s) why
  //      we don't share some common subexpressions.
  // With forceError set to TRUE, it generates an internal error that causes the
  //      query to fail. This should be avoided as best as possible, since expanding
  //      the CSEs should have given us a successful plan.

  if (CmpCommon::getDefault(CSE_DEBUG_WARNINGS) == DF_ON || forceError)
    {
      *CmpCommon::diags() << DgSqlCode(5001)
                          << DgString0(internalName_.data())
                          << DgString1(message);
      if (forceError)
        // throw an exception that forces the normalizer to skip the
        // SQO phase and to revert to the original tree
        AssertException(message, __FILE__, __LINE__).throwException();
    }
}

// -----------------------------------------------------------------------
// IsolatedNonTableUDR::transformNode()
// -----------------------------------------------------------------------
void IsolatedNonTableUDR::transformNode(NormWA & normWARef,
                                        ExprGroupId & locationOfPointerToMe)
{
  CMPASSERT( this == locationOfPointerToMe );

  if (nodeIsTransformed())
    return;


  // If we are a CallSP, the binder put the subquery or UDF in a Tuple 
  // node as child(0). Need to transform the child before we do the rest 
  // of the Node to allow the Tuple::transformNode() to remove the ValueId 
  // of the Subquery or UDF from its tupleExpr.  Otherwise we end up with 
  // an illegal transformation.
  // 
  // This would not be needed if CallSP worked like the other nodes.
  // Consider fixing so the binder doesn't create the tuple, but allow
  // the normal transformation like we do here do its magic.
  //
  // The other thing that is different for CallSP is that if it has a 
  // subquery or UDF in its inputs, it is not a leafNode until after
  // we do the final transformation in TransRule.
  //
  // There we transform something like this:
  //
  //           CallSP                     Join
  //             |                        /   \
  //            Join           ===>    Join   CallSp
  //           /   \                   /  \
  //        Values T1                Values T1
  //
  if (child(0) != NULL)
  {
    child(0)->transformNode (normWARef, child(0));
    // The RelRoutine:: transformNode () will transform the new child.
  }

  // Let the RelRoutine::transformNode() do the work.
  RelRoutine::transformNode (normWARef, locationOfPointerToMe);
  

  // The needeedValueIds is left over from the old CallSp class hierarchy
  // It is believed that the inputParamsVids() should suffice.
  // Will optimize this later.
  getNeededValueIds() = getProcInputParamsVids();
  // ---------------------------------------------------------------------
  // Prime the Group Attributes 
  // ---------------------------------------------------------------------
  primeGroupAttributes();

  markAsTransformed();
} // IsolatedNonTableUDR::transformNode()


// -----------------------------------------------------------------------
// IsolatedNonTableUDR::rewriteNode()
// -----------------------------------------------------------------------
void IsolatedNonTableUDR::rewriteNode(NormWA &normWARef)
{
  // ---------------------------------------------------------------------
  // Make sure to rewrite all of our parameter inputs and predicates.
  // ---------------------------------------------------------------------
  getNeededValueIds().normalizeNode(normWARef);
  RelRoutine::rewriteNode(normWARef);
}

//**********************************
// Constructor for class CqsWA
//***********************************
CqsWA::CqsWA():
  tableCANodeList_(new (CmpCommon::statementHeap())
                   TableCANodeIdPairLookupList(
                       CmpCommon::statementHeap())
                 ),
  cqsCANodeIdMap_(new (CmpCommon::statementHeap()) 
                  CQSRelExprCANodeIdMap(30,
                       CmpCommon::statementHeap())
                ),
  reArrangementSuccessful_(FALSE),
  numberOfScanNodesinNQT_(0),
  numberOfScanNodesinCQS_(0) 
{}

//************************************************************************
// This method collects CANodeIds from each scan node of the Normalized
// tree
//************************************************************************
void CqsWA::gatherCANodeIDTableNamepairsForNormalizedTree( RelExpr *nqtExpr)
                                   
{
  // leaf
  if (nqtExpr->getArity() == 0)
  {
     if (nqtExpr->getOperatorType() == REL_SCAN)
       {
        Scan *scan = (Scan *) nqtExpr;
        TableCANodeIdPair *tableIdPair = new (CmpCommon::statementHeap())
                                           TableCANodeIdPair();
        tableIdPair->Id_ = scan->getGroupAttr()->getGroupAnalysis()->
                                  getNodeAnalysis()->getId();
        tableIdPair->tabId_ = scan->getTableDesc();
        getTableCANodeList()->insert(tableIdPair);
        
       }
  }
  else
  {
    Int32 i =0;
    for (; i<nqtExpr->getArity();i++)
    {
      gatherCANodeIDTableNamepairsForNormalizedTree(nqtExpr->child(i));
    }
  }
  
} // gatherCANodeIDTableNamepairsForNormalizedTree()

//**************************************************************************
// This method delegates responsibility of gathering CANodeIdSets (TableSets)
// for the CQS tree to class CQSRelExprNodeMap
//***************************************************************************
void  CqsWA::gatherNodeIdSetsForCQSTree(RelExpr *cqsExpr)
                                
{
  CANodeIdSet set_= getcqsCANodeIdMap()->gatherNodeIdSetsForCQSTree(cqsExpr, this);
}

//*************************************************************************
// This method collects CANodeId values for all tables in the CQS tree.
//*************************************************************************

CANodeIdSet CQSRelExprCANodeIdMap::gatherNodeIdSetsForCQSTree(RelExpr* cqsExpr,
                                                               CqsWA *cwa)
{ 
  Int32 arity = cqsExpr->getArity();

  if ((arity == 0) && (cqsExpr->getOperatorType() == REL_FORCE_ANY_SCAN))
  {
     CQSRelExprCANodeIdPair * relExprNodeId = new (CmpCommon::statementHeap())
                                CQSRelExprCANodeIdPair();
     cwa->incrementNumberOfScanNodesinCQS();
     ScanForceWildCard *forcedScan = (ScanForceWildCard *) cqsExpr;

     CANodeId Id_ = relExprNodeId->populateReturnCANodeId(forcedScan, cwa);
     insertThisElement(forcedScan, relExprNodeId);

     CANodeIdSet caNodeset(Id_);
     return caNodeset;
     
  }
  else if (arity > 0)
  {
    CQSRelExprCANodeIdPair * relExprNodeId =
      new (CmpCommon::statementHeap()) CQSRelExprCANodeIdPair();

    if (cwa->isIndexJoin(cqsExpr))
    {
      
      relExprNodeId->leftChildSet_ = CANodeIdSet();
       
      relExprNodeId->rightChildSet_=gatherNodeIdSetsForCQSTree(
                                     cqsExpr->child(1), cwa);
      
    }
    else
    {

      relExprNodeId->leftChildSet_=gatherNodeIdSetsForCQSTree(
                                  cqsExpr->child(0), cwa);

      if (arity == 1)
        relExprNodeId->rightChildSet_=CANodeIdSet();
      else 
    // arity is 2
        relExprNodeId->rightChildSet_=gatherNodeIdSetsForCQSTree(
                                     cqsExpr->child(1), cwa);
    }

    relExprNodeId->forcedNode_ = cqsExpr;
    
    insertThisElement(cqsExpr,relExprNodeId); 
    return relExprNodeId->leftChildSet_+
           relExprNodeId->rightChildSet_; 
    
  }
  else
  {
    // leaves other than scan such as Tuple...
    // how do we treat derived tables? (values(1) as t(a))
    return CANodeIdSet();
  }
} // gatherNodeIdSetsForCQSTree()

//************************************************************************
// Given a Table Name or Index Name, this finds the corresponding CANodeId
// For MP tables: the table name needs to be like \node.$vol.subvol.tablename
// Otherwise, we assume that it is an MX table....

//************************************************************************
CANodeId  CqsWA::findCANodeId(const NAString &tableName)
{
  TableCANodeIdPairLookupList *tcpairList = getTableCANodeList();

  TableCANodeIdPair *tcpair;

  // if tableName is of form cat.sch.t ok
  // otherwise get it to that form by appending current catalog and
  // schema name as required.
  // how about MP tables???????????? TBD.....

  // for MX tables only or ANSI notation...

  NAString tableNameAppend(CmpCommon::statementHeap());
  for (CollIndex i=0; i < tcpairList->entries(); i++
      )
  {
    tcpair = tcpairList->at(i);

    if (tcpair->tabId_->getCorrNameObj().getCorrNameAsString() != "")
    {
       // if correlation name is set, do not append the default catalog
       //  and schema name
       tableNameAppend = tableName;
    }
    else if (isMPTable(tableName))
    {
       tableNameAppend = tableName;
    }
    else
    { 
      tableNameAppend = makeItThreePartAnsiString(tableName);
    }
    
    if ((tcpair->tabId_->getCorrNameObj().getQualifiedNameAsString()
         == tableNameAppend) ||
        (tcpair->tabId_->getCorrNameObj().getCorrNameAsString()
         == tableNameAppend))
 
    { 
      if (tcpair->visited_)
        AssertException("", __FILE__, __LINE__).throwException();

      tcpair->visited_ = TRUE; 
      return tcpair->Id_;
  
    }
    else
    {
      // check indexes
     IndexDesc *indexDesc;
     const LIST(IndexDesc *) &indexList = tcpair->tabId_->getIndexes();

     for (CollIndex j=0; j < indexList.entries(); j++)
     {
        indexDesc = indexList.at(j);
        if (tableNameAppend == indexDesc->getNAFileSet()->getExtFileSetName())
        {
          if (tcpair->visited_)
            AssertException("", __FILE__, __LINE__).throwException();

          tcpair->visited_ = TRUE;
          return tcpair->Id_;
        }
     }
      
    }
  } // for

  //if you are here, invoke error handling
  AssertException("", __FILE__, __LINE__).throwException();
  return CANodeId();  // keep VisualC++ happy

} // CqsWA::findCANodeId()

//*************************************************************
// This method collects CANodeId values for each table
// It then traverses the CQS tree and collects Tablesets for each 
// node. Both left and right child table sets are kept at each node
// Tablesets are sets of CANodeId values...
//**************************************************************
void CqsWA::initialize(RelExpr *nqtExpr, RelExpr *cqsExpr)
{
  gatherCANodeIDTableNamepairsForNormalizedTree(nqtExpr);
  numberOfScanNodesinNQT_ = getTableCANodeList()->entries();

  gatherNodeIdSetsForCQSTree(cqsExpr);

  if (numberOfScanNodesinNQT_ > numberOfScanNodesinCQS_)
  {
    AssertException("", __FILE__, __LINE__).throwException();
  }
  
}


//*****************************************************************
// For a given scan node, this collects the CANodeId
//*****************************************************************
CANodeId CQSRelExprCANodeIdPair::populateReturnCANodeId(RelExpr *scan,
                                                        CqsWA *cwa)
{
  ScanForceWildCard *forcedScan = (ScanForceWildCard *) scan;
  
  NAString tableName(forcedScan->getExposedName(),
                   CmpCommon::statementHeap());
  NAString indexName( forcedScan->getIndexName(),CmpCommon::statementHeap());
  CANodeId Id_;

  if (tableName != "")
  {
    Id_ = cwa->findCANodeId(tableName);
  }
  else if (indexName != "")
  {
    Id_ = cwa->findCANodeId(indexName);
  }
  //else
   // error & give-up

  forcedNode_ = forcedScan;
  leftChildSet_ = CANodeIdSet();
  rightChildSet_ = CANodeIdSet();

  return Id_;
} // CQSRelExprCANodeIdPair::populateReturnCANodeId()

//*********************************************************************
// Constructor for the map: maps CQS Relational expression pointer with
// CANodeId Sets of left subtree and right subtree
//*********************************************************************
CQSRelExprCANodeIdMap::CQSRelExprCANodeIdMap(ULng32 init_size,
                                               CollHeap *outHeap):

   HASHDICTIONARY(ULng32, CQSRelExprCANodeIdPair) 
          ( &(CQSRelExprCANodeIdMap::HashFn),
            init_size,
            TRUE, // uniqueness
            outHeap)
{}

//*******************************************************************
// A hash function required by Hashdictionary 
//*******************************************************************
ULng32 CQSRelExprCANodeIdMap::HashFn(const ULng32 &key)
{
  return key;
}

//**************************************************************
// Given the RelExpr pointer, this method gives the table subsets
//
//**************************************************************
CQSRelExprCANodeIdPair * CQSRelExprCANodeIdMap::get(RelExpr *key )
{

  ULng32 *myKey_ = (ULng32 *)new (CmpCommon::statementHeap()) Long;
  *(Long *)myKey_ =  (const Long) (key);

  CQSRelExprCANodeIdPair *result = 
    HASHDICTIONARY(ULng32, CQSRelExprCANodeIdPair)::getFirstValue(myKey_);

  return result;
}

//****************************************************************
//
//****************************************************************
void CQSRelExprCANodeIdMap::insertThisElement(RelExpr * expr,
                                              CQSRelExprCANodeIdPair *cqsNodeId)
{
   ULng32 * myKey_ = (ULng32 *)new (CmpCommon::statementHeap())
                            Long;

   *(Long *)myKey_ = (Long) (expr); 
   insert(myKey_, cqsNodeId);
   
}

//*********************************************************************
// pointer "this" is group by CQS expression. If the corresponding normalized
// relExpr is a group by, we process that expression; it could be a JBBC or
// not. If not we ignore it. This is because a group by may correspond to
// several groupby expressions at the end of optimization
//*********************************************************************
RelExpr *GroupByAgg::generateMatchingExpr(CANodeIdSet &lChildSet,
                     CANodeIdSet &rChildSet,
                     RelExpr *relExpr)
{
  if (relExpr->getOperator().match(REL_ANY_GROUP))
  {
    return CURRSTMT_CQSWA->checkAndProcessGroupByJBBC(relExpr,
                                                      lChildSet,
                                                      rChildSet,
                                                      this);
  }
  else
  {
    return RelExpr::generateMatchingExpr(lChildSet, rChildSet, relExpr);
  }
} // GroupByAgg::generateMatchingExpr()

//************************************************************
// The default Implementation simply calls the routine on the 
// child expression
//************************************************************
RelExpr *RelExpr::generateMatchingExpr(CANodeIdSet &lChildSet,
                                       CANodeIdSet &rChildSet,
                                       RelExpr *relExpr)
{
  CANodeIdSet leftTableSet, rightTableSet;

  // throw an exception if the arity is not one
  if (getArity() != 1)
    AssertException("", __FILE__, __LINE__).throwException();

  RelExpr *wcChild = child(0);
  CURRSTMT_CQSWA->getTableSets(wcChild, leftTableSet, rightTableSet);

  return (child(0)->generateMatchingExpr(leftTableSet, 
                                         rightTableSet, 
                                         relExpr));

} // RelExpr::generateMatchingExpr()


//*****************************************************************
// This recursive procedure traverses Join back bone of CQS tree and
// generates the logical relational expression using the normalized
// expression tree, relExpr.
//
//*****************************************************************
RelExpr *JoinForceWildCard::generateMatchingExpr(CANodeIdSet &lChildSet,
                                                 CANodeIdSet &rChildSet,
                                                 RelExpr *relExpr)
{
  Join *j = NULL;
  // check if the argument relExpr is a join; give an error if not ?

  if ( relExpr->getOperator().match(REL_ANY_JOIN) ||
       relExpr->getOperatorType() == REL_MULTI_JOIN)
  {
    j = (Join *)relExpr->generateLogicalExpr(lChildSet, rChildSet);
  }
  else
  {
    // index join? 
    if (
        (relExpr->getOperator().match(REL_SCAN)) &&
        (lChildSet.isEmpty() && rChildSet.entries() == 1)
       )
      return relExpr;
   
    if (relExpr->getOperator().match(REL_ANY_GROUP))
      {
        return CURRSTMT_CQSWA->checkAndProcessGroupByJBBC(relExpr,
                                                 lChildSet,
                                                 rChildSet,
                                                 this );
      }

    // throw an exception, otherwise
    AssertException("", __FILE__, __LINE__).throwException();
  }

  if (j != NULL)
  {
    RelExpr *lChild = j->child(0);
    RelExpr *rChild = j->child(1); 
    RelExpr *jwc = this;
    CANodeIdSet leftTableSet, rightTableSet;

    RelExpr *wcLeftChild = jwc->child(0);
    RelExpr *wcRightChild = jwc->child(1);

    CURRSTMT_CQSWA->getTableSets(wcLeftChild, leftTableSet, rightTableSet);
    j->child(0) = wcLeftChild->generateMatchingExpr(leftTableSet,
                                                    rightTableSet,
                                                    lChild);

    CURRSTMT_CQSWA->getTableSets(wcRightChild, leftTableSet, rightTableSet);

    j->child(1) = wcRightChild->generateMatchingExpr(leftTableSet,
                                                    rightTableSet,
                                                    rChild
                                                    );

    j->pushdownCoveredExpr
      (j->getGroupAttr()->getCharacteristicOutputs(),
       j->getGroupAttr()->getCharacteristicInputs(),
       j->selectionPred());

    return j; 
  }  

  return NULL;
} //JoinForceWildCard::generateMatchingExpr()

//*******************************************************************
// check if a join's child is a GB and if it is a JBBC handle it
// appropriately
//*******************************************************************
RelExpr * CqsWA::checkAndProcessGroupByJBBC( RelExpr *relExpr,
                                             CANodeIdSet &lChildSet,
                                             CANodeIdSet &rChildSet,
                                             RelExpr *cqsExpr)
{

  if (!relExpr->getOperator().match(REL_ANY_GROUP))
  {
    return relExpr;
  }

  // check if this GroupBy is a JBBC...
  GBAnalysis *pGBAnalysis = NULL;
  NodeAnalysis *nodeAnalysis = relExpr->getGroupAttr()->getGroupAnalysis()->
                                 getNodeAnalysis();

  GroupByAgg * gb = (GroupByAgg *) relExpr;
  pGBAnalysis = gb->getGBAnalysis();

  if (pGBAnalysis)
  {
    CANodeId id = nodeAnalysis->getId();
    
    // you may not need this check
    if (! QueryAnalysis::Instance()->getJBBCs().containsThisId(id))
      AssertException("", __FILE__, __LINE__).throwException();
    
      // get the child of GroupBy and re-arrange the join tree from
      // TableSets....
      RelExpr *childExpr = relExpr->child(0);

      RelExpr *wcChild;
      if (cqsExpr->getOperator().match(REL_ANY_GROUP))
      {
         wcChild = cqsExpr->child(0);
         getTableSets(wcChild,lChildSet, rChildSet);
      }
      else
         wcChild = cqsExpr;

      relExpr->child(0) = wcChild->generateMatchingExpr
                                     (lChildSet,rChildSet,childExpr);

      relExpr->primeGroupAttributes();
      relExpr->pushdownCoveredExpr
        (relExpr->getGroupAttr()->getCharacteristicOutputs(),
         relExpr->getGroupAttr()->getCharacteristicInputs(),
         relExpr->selectionPred());

      relExpr->synthLogProp();
      return relExpr;

  }
  else
  {
    // not a JBBC
    return relExpr;
    // handle the case right child of a join GB<-Scan  ?
  }

  // error
   return NULL; // keep VisualC++ happy

}// CqsWA::checkAndProcessGroupByJBBC()

//*************************************************************************
// Given an expression from CQS tree as input, this method returns TableSets
// of its two children
//**************************************************************************
void CqsWA::getTableSets(RelExpr * cqsExpr,
                         CANodeIdSet & leftSet, 
                         CANodeIdSet &rightSet)
{
  CQSRelExprCANodeIdPair *NodeIdSets = getcqsCANodeIdMap()->get(cqsExpr);
  leftSet = NodeIdSets->leftChildSet_;
  rightSet = NodeIdSets->rightChildSet_;
} // CqsWA::getTableSets()

//************************************************************************
// We essentially ignore the exchange wild card: simply pass the control to
// it's child
//************************************************************************
RelExpr *ExchangeForceWildCard::generateMatchingExpr(CANodeIdSet &lChildSet,
                                                 CANodeIdSet &rChildSet,
                                                 RelExpr *relExpr)
{
  // check the rChildSet is empty

  if (! rChildSet.isEmpty())
    AssertException("", __FILE__, __LINE__).throwException();

  CANodeIdSet leftTableSet, rightTableSet;
  RelExpr *wcChild = child(0);
  CURRSTMT_CQSWA->getTableSets(wcChild, leftTableSet, rightTableSet); 
  
  return child(0)->generateMatchingExpr(leftTableSet, 
                                        rightTableSet,
                                        relExpr);
} // ExchangeForceWildCard::generateMatchingExpr()


//***********************************************************************
// return the relExpr as it is. we do error checking
//***********************************************************************
RelExpr *ScanForceWildCard::generateMatchingExpr(CANodeIdSet &lChildSet,
                                                 CANodeIdSet &rChildSet,
                                                 RelExpr *relExpr)
{
   // check lChildSet and rChildSet are empty
   // check relExpr is a Scan
  if (  relExpr->getOperator().match(REL_SCAN) &&
        lChildSet.isEmpty() &&
        rChildSet.isEmpty() 
     )
    return relExpr;
  else
  {
    AssertException("", __FILE__, __LINE__).throwException();
    return NULL;  // keep VisualC++ happy
  }
} // ScanForceWildCard::generateMatchingExpr()

RelExpr * RelExpr::generateLogicalExpr(CANodeIdSet &lChildSet, 
                                       CANodeIdSet &rChildSet)
{
  AssertException("", __FILE__, __LINE__).throwException();
    return NULL;  // keep VisualC++ happy
}

//**************************************************************
// Split the join backbone along the requested child backbones
// returns a  join node, if such a split is possible
// throws an exception otherwise.
//**************************************************************
RelExpr * MultiJoin::generateLogicalExpr (CANodeIdSet &lChildSet,
                                          CANodeIdSet &rChildSet)
{
  
  Join *j = splitByTables(lChildSet, rChildSet);

  // if the split is not possible, throw an exception 
  if (j == NULL)
    AssertException("", __FILE__, __LINE__).throwException();
 
  j->child(0)->synthLogProp();
  j->child(1)->synthLogProp(); 
  j->synthLogProp();
  return j;
}

RelExpr * GroupByAgg::generateLogicalExpr(CANodeIdSet &lChildSet,
                                           CANodeIdSet &rChildSet)
{

  AssertException("", __FILE__, __LINE__).throwException(); 
    return NULL;  // keep VisualC++ happy
}

NABoolean RelRoot::forceCQS(RelExpr *cqsExpr)
{  
  RelExpr *nqtExpr = this;
  if (CmpCommon::getDefault(FORCE_BUSHY_CQS) != DF_ON)
    return FALSE;
  

  // make a copy of nqtExpr.  In case we encounter exceptions and unable 
  // to proceed, we give back the saved expression for further processing
  // Take care of transitively called CmpAsserts.:w

  RelExpr *rootExpr = nqtExpr;
  RelExpr *nqtCopyExpr = nqtExpr->child(0)->
                         copyRelExprTree(CmpCommon::statementHeap());
  try 
  {

    // do not bother with this if this query is simple: single table query etc.
    // no updates, compound statements etc, describe, union
    // if CQS relexpr contains CutOp do not continue

    if (CqsWA::shouldContinue(nqtExpr, cqsExpr))
    { 
       RelExpr *parentExpr = nqtExpr;
       nqtExpr = nqtExpr->child(0);
 
       RelExpr *topJoin= nqtExpr;

       while (! topJoin->getOperator().match(REL_ANY_JOIN) &&
              ! (topJoin->getOperatorType() == REL_MULTI_JOIN))
       {
         if (topJoin->getOperator().match(REL_ANY_LEAF_OP))
         {
           AssertException("", __FILE__, __LINE__).throwException(); 
         }
         // we look for the top most join
         parentExpr = topJoin;
         topJoin = topJoin->child(0);
         if (topJoin == NULL)
           {
            AssertException("", __FILE__, __LINE__).throwException();
           }
       } // while no join is found
 
       CURRENTSTMT->initCqsWA();
       CURRSTMT_CQSWA->initialize(nqtExpr, cqsExpr);
       CANodeIdSet leftTableSet, rightTableSet;
  
       CURRSTMT_CQSWA->getTableSets(cqsExpr,leftTableSet,rightTableSet); 
       RelExpr *childExpr = cqsExpr->generateMatchingExpr(leftTableSet,
                                                            rightTableSet, 
                                                            topJoin); 

       parentExpr->child(0) = childExpr;

       ValueIdList orderByList = reqdOrder();
       ValueIdSet valuesNeeded = parentExpr->getGroupAttr()->
                           getCharacteristicOutputs();

       // now add orderByList, if any, to expected outputs of Root's child
       // this is needed so that child can synthesize and keep the sortkey.
       // see the related code in RelRoot::normalizeNode() and
       // PhysicalProperty::enforceCoverageByGroupAttributes(). The latter
       // resets sortKey if it is not part of child's output. This needs to be
       // investigated at a latter time.
       valuesNeeded.insertList(orderByList);

       parentExpr->pushdownCoveredExpr
        (valuesNeeded,
         parentExpr->getGroupAttr()->getCharacteristicInputs(),
         parentExpr->selectionPred());

       parentExpr->synthLogProp();
       
       CURRSTMT_CQSWA->reArrangementSuccessful_ = TRUE;
       return TRUE;
       } // shouldContinue?
       return FALSE;
    }
  
  catch(...)
  {
    // decide on what message to give...
    rootExpr->child(0)=nqtCopyExpr;

    // reset any thing else?
    CURRENTSTMT->clearCqsWA();
  }
 
  return FALSE;
}

NABoolean CqsWA::shouldContinue(RelExpr *nqtExpr, RelExpr *cqsExpr)
{

  // check if the Normalized expression contains any unsupported operators.
  if (CqsWA::containsNotSupportedOperator(nqtExpr))
     return FALSE;
 
  // check if the CQS expression contains any cuts.. 
  if (CqsWA::containsCutOp(cqsExpr))
    return FALSE; 
  return TRUE;
  
} // CqsWA::shouldContinue()

NABoolean CqsWA::containsNotSupportedOperator(RelExpr *nqtExpr)
{
  if (nqtExpr->getOperatorType() == REL_COMPOUND_STMT ||
      nqtExpr->getOperator().match(REL_ANY_GEN_UPDATE) ||
      nqtExpr->getOperatorType() == REL_UNION ||
      nqtExpr->getOperatorType() == REL_DESCRIBE ||
      nqtExpr->getOperatorType() == REL_TUPLE_LIST ||
      nqtExpr->getOperatorType() == REL_TUPLE ||
      nqtExpr->getOperatorType() == REL_DDL) 

     return TRUE;

  for (Int32 i=0; i < nqtExpr->getArity(); i++)
  {
    if (containsNotSupportedOperator(nqtExpr->child(i)))
      return TRUE; 
  }
  return FALSE;
} //CqsWA::containsNotSupportedOperator()

NABoolean CqsWA::containsCutOp(RelExpr *cqsExpr)
{
  if (cqsExpr->isCutOp())
    return TRUE;
 
  for (Int32 i = 0; i < cqsExpr->getArity(); i++)
    {
    if (containsCutOp(cqsExpr->child(i)))
      return TRUE;
    }
  return FALSE;  
} // CqsWA::containsCutOp()

//***************************************************************
// if tablename is not of form cat.sch.t, make it so
//
//***************************************************************
NAString CqsWA::makeItThreePartAnsiString(const NAString & tableName)
{
  NAString tableNameAppend(CmpCommon::statementHeap()); 
  size_t catlen, schlen;
  catlen = tableName.first('.');
  schlen = tableName.last('.');
  SchemaName s = CmpCommon::context()->schemaDB_->getDefaultSchema();
  size_t len = tableName.length();
  if ((catlen > len) && (schlen > len ))
  {
    // append current catalog and schema names...
    tableNameAppend += s.getCatalogName();
    tableNameAppend += '.';
    tableNameAppend += s.getSchemaName();
    tableNameAppend += '.';
    tableNameAppend += tableName;
  }
  else if ((catlen > len) && (schlen < len ))
  {
    // append catalog name
    tableNameAppend += s.getCatalogName();
    tableNameAppend += '.';
    tableNameAppend += tableName;
   
  }
  else
  {
    tableNameAppend = tableName;
  }

  return tableNameAppend;
} // CqsWA::makeItThreePartAnsiString()

//***********************************************************
// check if the CQS Relational expression is an index join
//***********************************************************
NABoolean CqsWA::isIndexJoin(RelExpr *cqsExpr)
{
  if (cqsExpr->getArity() == 1) return FALSE;

  if (cqsExpr->getOperator().match(REL_FORCE_JOIN) ||
      cqsExpr->getOperator().match(REL_FORCE_NESTED_JOIN) ||
      cqsExpr->getOperator().match(REL_FORCE_HASH_JOIN) ||
      cqsExpr->getOperator().match(REL_FORCE_MERGE_JOIN)
     )
  {
     JoinForceWildCard *jwc= (JoinForceWildCard *)cqsExpr;
     if (jwc->getPlan() == JoinForceWildCard::FORCED_INDEXJOIN)
       return TRUE;

     return FALSE;
  }
  else
    return FALSE;
} // CqsWA::isIndexJoin()

//***************************************************************
// is tableName an MP table: does it look like $vol.subvol.tname?
//***************************************************************
NABoolean CqsWA::isMPTable(const NAString &tableName)
{
  size_t volumeLength, schemaLength, nameLength;
  nameLength = tableName.length();

  volumeLength = tableName.first('$');
  if (volumeLength < nameLength)
  {
    schemaLength=tableName.last('.');
    if ( schemaLength < nameLength &&
         volumeLength < schemaLength)
    {
      return TRUE;
    }
    else
    {
      AssertException("", __FILE__, __LINE__).throwException();
    return FALSE;  // keep VisualC++ happy
    }
  }
  else
  { 
    return FALSE;
  }
} // CqsWA::isMPTable()

 

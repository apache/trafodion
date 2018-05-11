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
* File:         ItemExprList.C
* Description:  Trees of ItemExpr object seen as lists and vice versa
*               
* Created:      7/7/95
* Language:     C++
*
*
******************************************************************************
*/

#include "Sqlcomp.h"
#include "ItemOther.h"
#include "ItemLog.h"
#include "ItemSubq.h"
#include "ItemFunc.h"
#include "ItemFuncUDF.h"


// -----------------------------------------------------------------------
// methods for class ItemExprList
// -----------------------------------------------------------------------

void ItemExprList::insertTree(ItemExpr *tree,
			      OperatorTypeEnum backBoneType,
			      NABoolean flattenSBQ, NABoolean flattenUDF)
{
  if (tree->getOperatorType() == backBoneType) 
    {
      for (Int32 i = 0; i < tree->getArity(); i++)
        {
          // Check for NULL list for right linear trees. That is, arity may be 
          // two, but second child is NULL.
          ItemExpr *child = tree->child(i);
          if (child)
            insertTree(tree->child(i), backBoneType, flattenSBQ, flattenUDF);
        }
    } 
  else if (tree->getOperatorType() == ITM_ONE_ROW)
  {
    Aggregate *agr = (Aggregate *)tree;

    if (agr->isOneRowTransformed_)
    {
      for (Int32 i = 0; i < tree->getArity(); i++)
        insertTree(tree->child(i), backBoneType, flattenSBQ, flattenUDF); 
    }
    else
    {
      // do nothing, postpone this processing until OneRow transformation
      // is done
    }
  }
  else if ((flattenSBQ AND tree->isASubquery()) OR 
           (flattenUDF AND 
           (tree->getOperatorType() == ITM_USER_DEF_FUNCTION)) AND
           (NOT tree->nodeIsTransformed()))
          // Added the extra check for transformation above to avoid any issues
          // where we might flatten a subquery/MVF a second time around while 
          // we deal with ValueIdProxies. 
          // The ValueIdProxy->needToTransformChild()
          // flag should be sufficient, but it never hurts to be safe.
    {
      ValueIdList cols;
      NABoolean haveRDesc(FALSE);

      if (tree->isASubquery())
      {
        // flatten the subquery select list
        RETDesc *retDesc = ((Subquery*)tree)->getSubquery()->getRETDesc();
        if (retDesc)
        {
          retDesc->getColumnList()->getValueIdList(cols);
          if (cols.entries() > 1)
          {
            haveRDesc = TRUE;
          }
        }
 
      }
      else if (tree->getOperatorType() == ITM_USER_DEF_FUNCTION)
      {
        // flatten the UDF by adding the additional outputs to the tree 
        const RoutineDesc *rDesc = ((UDFunction *)tree)->getRoutineDesc();
        if (rDesc && rDesc->getOutputColumnList().entries() > 1)
        {
          cols = rDesc->getOutputColumnList();
          haveRDesc = TRUE;
        }
      }

      if (haveRDesc == TRUE)
      {
        for (CollIndex i = 0; i < cols.entries(); i++)

        {
           ValueId  proxyId;

           proxyId = cols[i];

           // We create a ValueIdProxy for each element in the subquery's
           // select list or for each output parameter of a MVF. The first
           // one of these will be marked to be transformed. This allows
           // us to get the correct degree of statements containing MVFs or
           // subquery with degree > 1 at bind time. 
           ValueIdProxy *proxyOutput = 
                 new (CmpCommon::statementHeap())
                      ValueIdProxy( tree->getValueId(), 
                                    proxyId,
                                    i);


           proxyOutput->synthTypeAndValueId();

           // Make sure we transform the subquery or MVF 
           if (i == 0 ) proxyOutput->setTransformChild(TRUE);

           insert(proxyOutput);
        }

      }
      else
        insert(tree); // we are processing a valueId of a UDFunction 
                      // or subquery before we have bound it. Just insert 
                      // its valueId and we'll have to deal with it later..
    }
  else
    insert(tree);
}


//----------------------------------------------------------------------------
ItemExpr * 
ItemExprList::convertToItemExpr(ItemExprShapeEnum treeShape) const
{
  const ItemExprList & list = *this;
  
  if( ! (list.entries() > 0 ) )
  {
    return NULL;
  }

  ItemExpr * result = list[0];

  for (CollIndex i(1) ; i < list.entries() ; i++)
  {
    ItemExpr * pLeftSun = NULL;
    ItemExpr * pRightSun = NULL;
  
    switch(treeShape)
    {
      case LEFT_LINEAR_TREE:
	pLeftSun = result;
	pRightSun = list[i];
	break;

      case RIGHT_LINEAR_TREE:
	pLeftSun = list[i]; 
	pRightSun = result;
	break;

      case BUSHY_TREE:
      default:
	CMPASSERT(FALSE);
	break;
    };
  
    result = new(heap_) ItemList(pLeftSun, pRightSun);
  } // for  

  return result;
} // convertToItemExpr


void ItemExprList::unparse(NAString &result, PhaseEnum phase,
                           UnparseFormatEnum form, TableDesc* tabId) const
{
  NABoolean firstTime = TRUE;

  for (CollIndex j = 0; j < entries(); j++)
    {
      NAString unparsed;

      if (at(j) != NULL) {
        at(j)->unparse(unparsed, phase, form, tabId); 
        
        if ( !firstTime ) 
           result += ",";
        else
           firstTime = FALSE;

        result += unparsed;
      }
    }
}


//----------------------------------------------------------------------------
void ItemExprList::print(FILE* ofd, const char* indent, const char* title) const
{
  if (entries() > 0)
    {
#pragma nowarn(1506)   // warning elimination
      BUMP_INDENT(indent);
#pragma warn(1506)  // warning elimination

      NAString unparsed;

      unparse(unparsed, DEFAULT_PHASE, EXPLAIN_FORMAT);
      fprintf(ofd, "%s%s%s\n", NEW_INDENT, title, unparsed.data());
    }
} // ItemExprList::print()

// To be called from the debugger.
void ItemExprList::display() const
{
 ItemExprList::print();
} // ItemExprList::display()

// -----------------------------------------------------------------------
// methods for class ItemExprTreeAsList
// -----------------------------------------------------------------------

// make a new object that allows viewing a given tree as a list
ItemExprTreeAsList::ItemExprTreeAsList(ExprValueId *treePtr,
				       OperatorTypeEnum op,
				       ItemExprShapeEnum shape)
{
  treePtr_  = treePtr;
  operator_ = op;
  shape_    = shape;

  if (shape == BUSHY_TREE)
    ABORT("can't handle bushy trees for now");
}

// return number of entries
Lng32 ItemExprTreeAsList::entries() const
{
  ItemExpr *aNode = *treePtr_;
  Lng32 result = 0;

  while (aNode != NULL)
    {
      result++;
      if (aNode->getOperatorType() == operator_ AND
	  aNode->getArity() >= 2)
	{
	  if (shape_ == LEFT_LINEAR_TREE)
	    aNode = aNode->child(0);
	  else
	    if (shape_ == RIGHT_LINEAR_TREE)
	      aNode = aNode->child(1);
	    else
	      ABORT("can't do other than right-linear trees");
	}
      else
	aNode = NULL;
    }
  return result;
}

// insert a new entry.
// The new entry is created at the end of the current tree.
void ItemExprTreeAsList::insert(ItemExpr *treeToInsert)
{
  if (treePtr_->getPtr() == NULL)
    {
      *treePtr_ = treeToInsert;
    }
  else
    {
      if (shape_ == RIGHT_LINEAR_TREE)
	{
	  switch (operator_)
	    {
	    case ITM_AND:
	      *treePtr_ = new(CmpCommon::statementHeap()) 
		BiLogic(operator_,
			treeToInsert,
			treePtr_->getPtr());
	      break;

	    case ITM_ITEM_LIST:
	      {
		ItemExpr *aNode = treePtr_->getPtr();
		ItemExpr *pNode = treePtr_->getPtr();
		if (aNode->getOperatorType() != operator_ AND
		    aNode->getArity() < 2)
		  {
		    // case of current number of entries equal to 1.
		    *treePtr_ = new(CmpCommon::statementHeap())
		      ItemList(treePtr_->getPtr(),
			       treeToInsert);
		  }
		else
		  {
		    // current number of entries > 1
		    while (aNode != NULL)
		      {
			if (aNode->getOperatorType() == operator_ AND
			    aNode->getArity() >= 2)
			  {
			    if (shape_ == RIGHT_LINEAR_TREE)
			      {
				pNode = aNode;
				aNode = aNode->child(1);
			      }
			    else
			      ABORT("can't do other than right-linear trees");
			  }
			else
			  aNode = NULL;
		      }
		    
		    pNode->child(1) = new(CmpCommon::statementHeap())
		      ItemList(pNode->child(1),
			       treeToInsert);
		    
		  }
	      }
	      
	      break;

	    default:
	      ABORT("Can't do backbones with this node type");
	    }
	}
      else
	ABORT("can only insert into right-linear trees");
    }
}


// insert a new entry to the top of the ItemExprTree.
// as of R1.5 this method is called only for ODBC initiated dynamic
// rowset queries. This method is supported only for RIGHT_LINEAR_TREEs
// with operator = ITM_ITEM_LIST. 
// Calling this method to an ItemExpr* called newItem will result
// in the following tree, where old1, old2, old3 are previously inserted 
// ItemExpr* with old1 being inserted first, then old2 and then old3.

  //
  //        ITEM_LIST
  //        /   \
  //   newItem   ITEM_LIST
  //	       /   \
  //	    old1    ITEM_LIST
  //                 /   \
  //              old2   old3
  //	                
  
void ItemExprTreeAsList::insertAtTop(ItemExpr *treeToInsert)
{
  if (!((shape_ == RIGHT_LINEAR_TREE) && (operator_ == ITM_ITEM_LIST)))
     ABORT("can only do right-linear trees with a backbone type of ITEM_LIST ");

  if (treePtr_->getPtr() == NULL)
    {
      *treePtr_ = treeToInsert;
    }
  else
    {		
      *treePtr_ = new(CmpCommon::statementHeap())
		      ItemList(treeToInsert, treePtr_->getPtr());			       
    }
}

// remove an element that is given by its value
ItemExpr * ItemExprTreeAsList::remove(ItemExpr *treeToRemove)
{
  ItemExpr *andNodePtr  = treePtr_->getPtr();
  ItemExpr *predecessor = NULL;
  NABoolean found       = FALSE;

  // assume the predicate is represented in right-linear
  // form:
  //
  //         op
  //        /   \
  //	   A    op
  //	       /   \
  //	      B     ...
  //
  // and search for the <op> node that is directly above the
  // predicate that we want to remove.

  if (shape_ != RIGHT_LINEAR_TREE)
    ABORT("delete is supported for right linear trees only");

  // is the node to delete the only node?
  if (treeToRemove == andNodePtr)
    {
      found = TRUE;
      delete andNodePtr;
      *treePtr_ = NULL;
    }
  else
    // traverse the backbone looking for "treeToRemove"
    while (andNodePtr != NULL AND
	   andNodePtr->getOperatorType() == operator_ AND
	   andNodePtr->getArity() == 2 AND
	   NOT found)
      {
	// did we find the right node?
	if (andNodePtr->child(0).getPtr() == treeToRemove)
	  {
	    found = TRUE;
	    
	    // take "andNode" out of the original tree
	    if (predecessor != NULL)
	      {
		predecessor->child(1) = andNodePtr->child(1);
	      }
	    else
	      *treePtr_ = andNodePtr->child(1);
	    
	    // set all children of "andNode" to NULL, then delete it
	    andNodePtr->child(0) = NULL;
	    andNodePtr->child(1) = NULL;
	    delete andNodePtr;
	    andNodePtr = NULL;
	  }
	else
	  {
	    predecessor = andNodePtr;
	    andNodePtr = andNodePtr->child(1);
	  }
      }
  
  if (found)
    return treeToRemove;
  else
    return NULL;

}

// check whether an element is in the collection
NABoolean ItemExprTreeAsList::contains(const ItemExpr *treeToCheck)
{
  
  ItemExpr *aNodePtr = *treePtr_;

  while (aNodePtr != NULL)
    {
      if (aNodePtr == treeToCheck)
	return TRUE;

      if (aNodePtr->getOperatorType() == operator_ AND
	  aNodePtr->getArity() >= 2)
	{
	  if (shape_ == RIGHT_LINEAR_TREE)
	    aNodePtr = aNodePtr->child(1);
	  else
	    ABORT("can't do other than right-linear trees");
	}
      else
	aNodePtr = NULL;
    }
  return FALSE;
}

// index access (both reference and value)
ItemExpr * ItemExprTreeAsList::operator [] (CollIndex i)
{
  
  //     think of three different cases:
  // 
  //     a) i is out of range (< 0 or >= #entries)
  // 
  //     b) the node we are looking for is neither the only nor the last
  //        node in the backbone
  // 
  //     c) we are looking for the last element in the backbone (which
  //        may be the only element)
  // 

  ItemExpr *aNodePtr = *treePtr_;
  Int32 j = (Int32) i; // j may become negative, i may be unsigned

  if (j < 0)
    return NULL; // case a

  if (aNodePtr->getOperatorType() != operator_ AND
      j == 0)
    return aNodePtr; // case b

  while (aNodePtr != NULL AND j >= 0)
    {
      if (aNodePtr->getOperatorType() == operator_ AND
	  aNodePtr->getArity() >= 2)
	{
	  if (shape_ == LEFT_LINEAR_TREE)
	    {
	      if (j == 0)
		aNodePtr = aNodePtr->child(1); // case b
	      else
		aNodePtr = aNodePtr->child(0);
	    }
	  else if (shape_ == RIGHT_LINEAR_TREE)
	    {
	      if (j == 0)
		aNodePtr = aNodePtr->child(0); // case b
	      else
		aNodePtr = aNodePtr->child(1);
	    }
	  else
	    ABORT("can't do bushy trees");
	}
      j--;
    }

  // if we are looking for the only element, the while loop
  // is not executed at all

  return aNodePtr;

}

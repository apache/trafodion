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
#ifndef ITEMEXPRLIST_H
#define ITEMEXPRLIST_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         <file>
 * Description:  
 *               
 *               
 * Created:      //95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "BaseTypes.h"
#include "Collections.h"
#include "NABasicObject.h"

// -----------------------------------------------------------------------
// forward declarations
// -----------------------------------------------------------------------

class ExprValueId;
class TableDesc;

enum ItemExprShapeEnum { LEFT_LINEAR_TREE,
			 RIGHT_LINEAR_TREE,
			 BUSHY_TREE
		       };



// -----------------------------------------------------------------------
// a list of item expressions (takes a tree of item exprs as constructor
// argument)
// -----------------------------------------------------------------------
class ItemExprList : public LIST(ItemExpr *)
{

public:

  // Constructor
  ItemExprList(CollHeap* h/*=0*/)
    : LIST(ItemExpr *)(h), heap_(h) {}
  
  // Constructor
  ItemExprList(Lng32 numberOfElements, CollHeap* h) 
    : LIST(ItemExpr *)(h, numberOfElements), heap_(h) {}
  
  ItemExprList(ItemExpr *tree, CollHeap* h,
	       OperatorTypeEnum backBoneType = ITM_ITEM_LIST,
	       NABoolean flattenSubqueries = TRUE,
	       NABoolean flattenUDFs = TRUE)
    : LIST(ItemExpr *)(h), heap_(h)
  {
    insertTree(tree, backBoneType, flattenSubqueries, flattenUDFs);
  }

  // Insert an ItemExpr tree
  void insertTree(ItemExpr *tree,
                  OperatorTypeEnum backBoneType = ITM_ITEM_LIST,
		  NABoolean flattenSubqueries = TRUE,
		  NABoolean flattenUDFs = TRUE);

  void addMember(ItemExpr* x) { insert(x); };

  // convert the list to an ItemExpr *. if there is more then one node in the 
  // list, the result will be an ItemList that can be either a 
  // LEFT_LINEAR_TREE or a RIGHT_LINEAR_TREE
  // Note: does not support BUSHY_TREE. 
  ItemExpr * 
  convertToItemExpr(ItemExprShapeEnum treeShape = LEFT_LINEAR_TREE) const;

  // create a comma separated string list in argument 'result'
  virtual void unparse(NAString &result,
                       PhaseEnum phase = OPTIMIZER_PHASE,
                       UnparseFormatEnum form = USER_FORMAT,
                       TableDesc * tabId = NULL) const;

  // ---------------------------------------------------------------------
  // Print 
  // ---------------------------------------------------------------------
  void display() const;

  void print( FILE* ofd = stdout,
	      const char* indent = DEFAULT_INDENT,
              const char* title = "ItemExprList") const;

  
private:

  CollHeap * heap_;

}; // ItemExprList

// -----------------------------------------------------------------------
// Pseudo collection class to treat a tree as a list of items. The list
// is realized as a "backbone" of nodes with arity 2 (like AND, comma,
// join, union, ...) and the nodes hanging off the backbone (the elements
// of the list).
// -----------------------------------------------------------------------

class ItemExprTreeAsList : public NABasicObject
{

public:

  // make a new object that allows viewing a given tree
  // as a list
  ItemExprTreeAsList(ExprValueId *treePtr,
		     OperatorTypeEnum op,
		     ItemExprShapeEnum shape = RIGHT_LINEAR_TREE);

  // return number of entries
  Lng32 entries() const;

  // insert a new entry to bottom of list
  void insert(ItemExpr *treeToInsert);

  // insert a new entry to top of list
  void insertAtTop(ItemExpr *treeToInsert);

  // remove an element that is given by its value
  ItemExpr * remove(ItemExpr *treeToRemove);

  // check whether an element is in the tree
  NABoolean contains(const ItemExpr *treeToCheck);

  // index access
  ItemExpr * operator [] (CollIndex i);

private:

  ExprValueId *treePtr_;
  OperatorTypeEnum operator_;
  ItemExprShapeEnum shape_;

}; // ItemExprTreeAsList

#endif /* ITEMEXPRLIST_H */


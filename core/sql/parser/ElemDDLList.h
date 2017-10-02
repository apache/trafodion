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
#ifndef ELEMDDLLIST_H
#define ELEMDDLLIST_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLList.h
 * Description:  generic class for lists of elements in DDL statements --
 *               Note that each list is represented by a left linear
 *               tree (also called left skewed binary tree).  For
 *               example:
 *                                          o
 *                                         / \
 *                                        o  x3
 *                                       / \
 *                                      o  x2
 *                                     / \
 *                                    x0 x1
 *
 *               where o represents the parse nodes instantiated from class
 *               ElemDDLList (or one of its derived classes) and x<digit>
 *               represents the parse nodes in the list.  In this example,
 *               the list has four (4) elements x0, x1, x2, and x3, with
 *               x0 being the first element and x3 being the last element
 *               in the list.
 *
 *               Note that (unlike the names of several other classes; for
 *               example, TableDescList) the suffix List in ElemDDLList is
 *               not related to the singular link list template defined in
 *               module Collections.  I hate to overload the suffix List,
 *               but I could not think of any better name.
 *
 *               
 * Created:      3/29/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ElemDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLList;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// A list of elements in DDL statement.
// -----------------------------------------------------------------------
class ElemDDLList : public ElemDDLNode
{

public:

  // constructors
  ElemDDLList(ElemDDLNode * commaExpr, ElemDDLNode * otherExpr);
  ElemDDLList(const enum OperatorTypeEnum operType,
              ElemDDLNode * commaExpr,
              ElemDDLNode * otherExpr);

  // virtual destructor
  virtual ~ElemDDLList();

  // cast
  virtual ElemDDLList * castToElemDDLList();

  //
  // accessors
  //

  virtual Int32 getArity() const;

        // gets the degree of this node (the number of child
        // parse node).

  virtual ExprNode * getChild(Lng32 index);

  virtual CollIndex entries() const;

        // treats this node as the root node of a left linear tree.
        // Return the number of entries in the list represented by
        // this left linear tree (the number of the leaf nodes
        // in the tree).
  
  virtual ElemDDLNode * operator[](CollIndex index);

        // Returns the pointer pointing to an element in the list
        // represented by this left linear tree.  The parameter
        // index specifies the position of the element within the
        // list.  Note that the first element in the list is at
        // position 0.  The algorithm in this method is very
        // inefficient.  If the list is long and you have a need
        // to traverse all elements in the list, please do not use
        // this [] operator.  Please use the method traverseList
        // instead.

  virtual void traverseList(ElemDDLNode * pOtherNode,
                            void (*visitNode)(ElemDDLNode *,
                                              CollIndex,
                                              ElemDDLNode *));

        // traverseList() uses a pretty efficient algorithm to
        // traverse the leaf nodes of the left linear tree.
        // This method should be used whenever the user would like
        // to visit all leaf nodes in the tree.  The caller needs
        // to pass a function pointer to this method.  This function
        // pointer will be invoked for each leaf node.  The method
        // will pass to the invoked function pointer the index of
        // the visited leaf node (the index of the element in the
        // list) and the pointer pointing to the leaf node.

  inline ElemDDLList * getParentListNode() const;

  // mutators
  virtual void setChild(Lng32 index, ExprNode * pElemDDLNode);
  inline void setParentListNode(ElemDDLList * pParentListNode);

  // method for tracing
  virtual const NAString getText() const;

  // method for building text
  NAString getSyntax(NAString separator) const;




private:

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------

  void initializeDataMembers(ElemDDLNode * commaExpr,
                             ElemDDLNode * otherExpr);

  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------

  // pointer to parent node if the parent node is 
  // a left linear tree node (of the same kind);
  // set to NULL otherwise.  The method traverseList
  // uses this field so it can traverse the left
  // linear tree efficiently.
  
  ElemDDLList * pParentListNode_;

  // pointers to child parse nodes

  enum { INDEX_ELEM_DDL_LIST_CHILD = 0,
         INDEX_ELEM_DDL_NODE_CHILD,
         MAX_ELEM_DDL_LIST_ARITY };

  ElemDDLNode * children_[MAX_ELEM_DDL_LIST_ARITY];

}; // class ElemDDLList

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLList
// -----------------------------------------------------------------------

//
// accessor
//

inline ElemDDLList *
ElemDDLList::getParentListNode() const
{
  return pParentListNode_;
}

//
// mutator
//

inline void
ElemDDLList::setParentListNode(ElemDDLList * pParentListNode)
{
  pParentListNode_ = pParentListNode;
}

#endif // ELEMDDLLIST_H

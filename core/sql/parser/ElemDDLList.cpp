/* -*-C++-*-
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
 *****************************************************************************
 *
 * File:         ElemDDLList.C
 * Description:  methods for classes representing lists (left-skewed
 *               binary trees)
 *
 * Created:      9/19/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#include "AllElemDDLList.h"
#include "ComASSERT.h"
#include "ComOperators.h"
#include "ElemDDLPartition.h"

//----------------------------------------------------------------------------
// methods for class ElemDDLList
//----------------------------------------------------------------------------

// constructors

ElemDDLList::ElemDDLList(ElemDDLNode * commaExpr, ElemDDLNode * otherExpr)
: ElemDDLNode(ELM_ELEM_LIST)
{
  initializeDataMembers(commaExpr, otherExpr);
}

ElemDDLList::ElemDDLList(const enum OperatorTypeEnum operType,
                         ElemDDLNode * commaExpr,
                         ElemDDLNode * otherExpr)
: ElemDDLNode(operType)
{
  initializeDataMembers(commaExpr, otherExpr);
}

// virtual destructor
ElemDDLList::~ElemDDLList()
{
  // delete all children
  for (Int32 i = 0; i < MAX_ELEM_DDL_LIST_ARITY; i++)
  {
    delete children_[i];
  }
}

// cast virtual function
ElemDDLList *
ElemDDLList::castToElemDDLList()
{
  return this;
}

//
// accessors
//

// get the degree of this node
Int32
ElemDDLList::getArity() const
{
  return MAX_ELEM_DDL_LIST_ARITY;
}

ExprNode *
ElemDDLList::getChild(Lng32 index)
{ 
  ComASSERT(index >= 0 AND index < getArity());
  return children_[index];
}

// returns number of entries in the list represented
// by a left linear tree (also called a left skewed
// binary tree).  Treat this node as the root node
// of a left linear tree.
//
//  1 entry     2 entries      > 2 entries
//                [op]             [op]
//    [0]         /  \             /  \
//              [0]  [1]         [op] [2]
//                               /  \
//                             [0]  [1]
//
//  op represents ElemDDLList node
//
//  The case of 1 entry is not handled by this class.
//  This case is handled by the class ElemDDLNode.
//
CollIndex  
ElemDDLList::entries() const
{
  CollIndex count = 0;
  ElemDDLNode * pElemDDLNode = (ElemDDLNode *)this;

  while (pElemDDLNode NEQ NULL)
  {
    count++;
    if (pElemDDLNode->getOperatorType() EQU getOperatorType() AND
        pElemDDLNode->getArity() >= 2)
    {
      pElemDDLNode = pElemDDLNode->getChild(0)->castToElemDDLNode();
    }
    else
    {
      pElemDDLNode = NULL;
    }
  }
  return count;
}

// index access (both reference and value) to the
// list represented by this left linear tree
//
//  1 entry     2 entries      > 2 entries
//                [op]             [op]
//    [0]         /  \             /  \
//              [0]  [1]         [op] [2]
//                               /  \
//                             [0]  [1]
//
//  [op] represents ElemDDLList node
//  [0], [1], [2] represent the leaf nodes.  The
//  number between the square brackets represents
//  the index of a leaf node in the list represented
//  by the left linear tree.
//
//  The case of 1 entry is not handled by this class.
//  This case is handled by the class ElemDDLNode.
//
//  The algorithem in this method is very inefficient.
//  If the list is long and you would like to visit
//  all leaf nodes in the left linear tree, please use
//  the method traverseList instead.
//
ElemDDLNode *
ElemDDLList::operator[](CollIndex index)
{
  CollIndex count;
  ElemDDLNode * pElemDDLNode = this;
  
  if (index >= entries())
  {
    return NULL;
  }

  if (index EQU 0)
  {
    for (count = 1; count < entries(); count ++)
    {
      pElemDDLNode = pElemDDLNode->getChild(0)->castToElemDDLNode();
    }
    ComASSERT(pElemDDLNode->getOperatorType() NEQ getOperatorType());
    return pElemDDLNode;
  }

  count = entries() - index;
  while (pElemDDLNode NEQ NULL AND count > 0)
  {
    if (pElemDDLNode->getOperatorType() EQU getOperatorType() AND
        pElemDDLNode->getArity() >= 2)
    {
      if (count EQU 1)
        pElemDDLNode = pElemDDLNode->getChild(1)->castToElemDDLNode();
      else
        pElemDDLNode = pElemDDLNode->getChild(0)->castToElemDDLNode();
    }
    count--;
  }
  ComASSERT(pElemDDLNode->getOperatorType() NEQ getOperatorType());
  return pElemDDLNode;
}

//
// mutators
//

void
ElemDDLList::initializeDataMembers(ElemDDLNode * commaExpr,
                                   ElemDDLNode * otherExpr)
{
  pParentListNode_ = NULL;

  setChild(INDEX_ELEM_DDL_LIST_CHILD, commaExpr);
  setChild(INDEX_ELEM_DDL_NODE_CHILD, otherExpr);

  // Besides pointing to a list node (a non-leaf node in
  // the left linear tree), commaExpr may also point to
  // a leaf node (which is the first element in the list)

  ComASSERT(commaExpr NEQ NULL);

  // Link the child list node (pointed by the pointer commaExpr with
  // the parent list node (this node).  The method traverseList will
  // use this link (in upward direction) to traverse the leaf nodes
  // of the left linear tree more efficiently.
  //
  // Only link the child list node to this node if both parent node
  // (this node) and child node represent the same kind of list node
  // (both nodes are instantiated from the same class; for example,
  // class ElemDDLPartitionList).  Note that class ElemDDLList and
  // class ElemDDLPartitionList represent two different kinds of list
  // nodes (even though class ElemDDLPartitionList is derived from
  // class ElemDDLList).

  ElemDDLList * pChildListNode = commaExpr->castToElemDDLList();
  if (pChildListNode NEQ NULL AND
      getOperatorType() EQU pChildListNode->getOperatorType())
    pChildListNode->setParentListNode(this);
}

void
ElemDDLList::setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index >= 0 AND index < getArity());
  if (pChildNode NEQ NULL)
  {
    ComASSERT(pChildNode->castToElemDDLNode() NEQ NULL);
    children_[index] = pChildNode->castToElemDDLNode();
  }
  else
  {
    children_[index] = NULL;
  }
}

// Traverses the leaf nodes of the left linear tree.
// For each leaf node, invokes the function pointer
// visitNode passed by the caller.  Passes to the
// function pointer visitNode the index of the leaf
// node and the pointer to the left node.  Please
// read the comment header section in the definition
// of operator[] method to find out how the index
// value is determined.
//
// Note that this method visits the leaf nodes
// sequentially, with the first element in the list
// being visited first.
//
// The parameter pOtherNode contains a pointer pointing
// to the parse node that contains the left linear
// tree to be traversed.  This parameter will be
// pass to the function pointer visitNode during
// the invocation of visitNode so the code in 
// visitNode can update the contents of the parse
// node pointed by pOtherNode.
//
// traverseList also passed the index of and the
// pointer to the (currently visited) element to
// visitNode.  These two parameters contain
// information is used by visitNode to update the
// contents of the parse node pointed by pOtherNode.
//
/*virtual*/ void
ElemDDLList::traverseList(ElemDDLNode * pOtherNode,
                          void (*visitNode)(ElemDDLNode * pOtherNode,
                                            CollIndex indexOfLeafNode,
                                            ElemDDLNode * pLeafNode))
{
  OperatorTypeEnum operatorType = getOperatorType();
  ElemDDLList * pElemDDLList = this;
  CollIndex count = 0;
  ElemDDLNode * pElemDDLNode = (ElemDDLNode *)this;

  // go to the first element of the list

  while (pElemDDLNode->getOperatorType() EQU operatorType)
  {
    count++;
    ComASSERT(pElemDDLNode->castToElemDDLList() NEQ NULL);
    pElemDDLList = pElemDDLNode->castToElemDDLList();
    // getChild(0) returns the pointer to the left sub-tree
    ComASSERT(pElemDDLNode->getChild(0) NEQ NULL);
    pElemDDLNode = pElemDDLNode->getChild(0)->castToElemDDLNode();
  }

  // count now contains the number of the list (non-leaf) nodes
  // in the left linear tree.  Note that the number of leaf nodes
  // is equal to the number of list nodes plus one.

  count++;

  // count now contains the number of elements in the list
  // pElemDDLNode now points to the first element in the list
  // pElemDDLList now points to the list node that is the 
  //   parent node of the leaf node representing the first
  //   element in the list
  
  ComASSERT(pElemDDLNode NEQ NULL);

  // visit the first element in the list
  (*visitNode)(pOtherNode, 0, pElemDDLNode);
  
  // visit the remaining elements in the list sequentially

  ComASSERT(count > 1);

  for (CollIndex index = 1; index < count; index ++)
  {
    // getChild(1) returns the pointer to the right sub-tree
    ComASSERT(pElemDDLList NEQ NULL AND
              pElemDDLList->getOperatorType() EQU operatorType AND
              pElemDDLList->getChild(1) NEQ NULL);
    (*visitNode)(pOtherNode,
                 index,
                 pElemDDLList->getChild(1)->castToElemDDLNode());
    pElemDDLList = pElemDDLList->getParentListNode();
  }
  ComASSERT(pElemDDLList EQU NULL OR
            pElemDDLList->getOperatorType() EQU operatorType);
}

// methods for tracing

const NAString
ElemDDLList::getText() const
{
  return "ElemDDLList";
}


// method for building text
NAString ElemDDLList::getSyntax(NAString separator) const
{
  NAString syntax;

  ElemDDLList * ncThis = (ElemDDLList*)this;

  syntax = (*ncThis)[0]->getSyntax();
  
  // notice we start from i = 1
  for (CollIndex i = 1 ; i < entries() ; i++)
  {
    syntax += separator;
    syntax += (*ncThis)[i]->getSyntax();
  }

  return syntax; 
}

// -----------------------------------------------------------------------
// methods for class ElemDDLColNameList
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLColNameList::~ElemDDLColNameList()
{
}

// cast virtual function
ElemDDLColNameList *
ElemDDLColNameList::castToElemDDLColNameList()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLColNameList::getText() const
{
  return "ElemDDLColNameList";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLColRefList
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLColRefList::~ElemDDLColRefList()
{
}

// cast virtual function
ElemDDLColRefList *
ElemDDLColRefList::castToElemDDLColRefList()
{
  return this;
}

// methods for tracing

const NAString
ElemDDLColRefList::getText() const
{
  return "ElemDDLColRefList";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLConstraintNameList
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLConstraintNameList::~ElemDDLConstraintNameList()
{
}

// cast
ElemDDLConstraintNameList *
ElemDDLConstraintNameList::castToElemDDLConstraintNameList()
{
  return this;
}

// methods for tracing

const NAString
ElemDDLConstraintNameList::getText() const
{
  return "ElemDDLConstraintNameList";
}

// -----------------------------------------------------------------------
// methods for class ElemDDLFileAttrList
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLFileAttrList::~ElemDDLFileAttrList()
{
}

// cast virtual function
ElemDDLFileAttrList *
ElemDDLFileAttrList::castToElemDDLFileAttrList()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLFileAttrList::getText() const
{
  return "ElemDDLFileAttrList";
}

// method for building text
// virtual 
NAString ElemDDLFileAttrList::getSyntax() const
{
  return ElemDDLList::getSyntax(", ");
}

// -----------------------------------------------------------------------
// methods for class ElemDDLPartnAttrList
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPartnAttrList::~ElemDDLPartnAttrList()
{
}

// cast virtual function
ElemDDLPartnAttrList *
ElemDDLPartnAttrList::castToElemDDLPartnAttrList()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPartnAttrList::getText() const
{
  return "ElemDDLPartnAttrList";
}

// method for building text
// virtual
NAString ElemDDLPartnAttrList::getSyntax() const
{
  return ElemDDLList::getSyntax(", ");
}
// -----------------------------------------------------------------------
// methods for class ElemDDLKeyValueList
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLKeyValueList::~ElemDDLKeyValueList()
{
}

// cast
ElemDDLKeyValueList *
ElemDDLKeyValueList::castToElemDDLKeyValueList()
{
  return this;
}

// methods for tracing

const NAString
ElemDDLKeyValueList::getText() const
{
  return "ElemDDLKeyValueList";
}


// method for building text
// virtual 
NAString ElemDDLKeyValueList::getSyntax() const
{

  return ElemDDLList::getSyntax(", ");

} // getSyntax


// -----------------------------------------------------------------------
// methods for class ElemDDLOptionList
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLOptionList::~ElemDDLOptionList()
{
}

// cast
ElemDDLOptionList *
ElemDDLOptionList::castToElemDDLOptionList()
{
  return this;
}

// methods for tracing

const NAString
ElemDDLOptionList::getText() const
{
  return "ElemDDLOptionList";
}


// method for building text
// virtual 
NAString ElemDDLOptionList::getSyntax() const
{
  return ElemDDLList::getSyntax(" ");
} // getSyntax()


// -----------------------------------------------------------------------
// methods for class ElemDDLPartitionList
// -----------------------------------------------------------------------

// virtual destructor
ElemDDLPartitionList::~ElemDDLPartitionList()
{
}

// cast virtual function
ElemDDLPartitionList *
ElemDDLPartitionList::castToElemDDLPartitionList()
{
  return this;
}

//
// methods for tracing
//

const NAString
ElemDDLPartitionList::getText() const
{
  return "ElemDDLPartitionList";
}


// method for building text
// virtual 
NAString ElemDDLPartitionList::getSyntax() const
{
  return ElemDDLList::getSyntax(", ");
} // getSyntax()


//
// End of File
//

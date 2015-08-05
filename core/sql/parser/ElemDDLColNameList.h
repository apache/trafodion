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
#ifndef ELEMDDLCOLNAMELIST_H
#define ELEMDDLCOLNAMELIST_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLColNameList.h
 * Description:  class for lists of Column Name elements in DDL statements
 *
 * Created:      10/16/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ElemDDLList.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLColNameList;
class ElemDDLColNameListNode;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class ElemDDLColNameList
// -----------------------------------------------------------------------
class ElemDDLColNameList : public ElemDDLList
{

public:

  // constructor
  ElemDDLColNameList(ElemDDLNode * commaExpr, ElemDDLNode * otherExpr)
  : ElemDDLList(ELM_COL_NAME_LIST, commaExpr, otherExpr)
  { }

  // virtual destructor
  virtual ~ElemDDLColNameList();

  // cast
  virtual ElemDDLColNameList * castToElemDDLColNameList();

  // methods for tracing
  virtual const NAString getText() const;


private:

}; // class ElemDDLColNameList

// -----------------------------------------------------------------------
// definition of class ElemDDLColNameListNode
// -----------------------------------------------------------------------
//
// This class allows a list node to be an array of other list nodes,
// i.e., allows a 2-dimensional list.
//

class ElemDDLColNameListNode : public ElemDDLNode
{

public:

  // constructor
  ElemDDLColNameListNode(ElemDDLNode * theList)
  : ElemDDLNode(ELM_COL_NAME_LIST_NODE)
  {
  columnNameList_ = theList;
  }

  // virtual destructor
  virtual ~ElemDDLColNameListNode();

  // cast
  virtual ElemDDLColNameListNode * castToElemDDLColNameListNode();

  // methods for tracing
  virtual const NAString getText() const;

  inline ElemDDLNode *getColumnNameList ();
private:

  ElemDDLNode *columnNameList_;
}; // class ElemDDLColNameListNode

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLColNameListNode
// -----------------------------------------------------------------------

inline ElemDDLNode *ElemDDLColNameListNode::getColumnNameList ()
  {
    return columnNameList_;
  }


#endif // ELEMDDLCOLNAMELIST_H

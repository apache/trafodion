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
#ifndef ELEMDDLPARTNATTRLIST_H
#define ELEMDDLPARTNATTRLIST_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLPartnAttrList.h
 * Description:  class for lists of partition attribute elements specified in
 *               DDL statements.  Since class ElemDDLPartnAttrList is
 *               derived from the base class ElemDDLList; therefore, the
 *               class ElemDDLPartnAttrList also represents a left linear
 *               tree.
 *
 *               Please note that class ElemDDLPartnAttrList is not derived
 *               from class ElemDDLFileAttr which represents file attribute
 *               elements.
 *
 *               
 * Created:      3/25/2003
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
class ElemDDLPartnAttrList;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class ElemDDLPartnAttrList
// -----------------------------------------------------------------------
class ElemDDLPartnAttrList : public ElemDDLList
{

public:

  // constructor
  ElemDDLPartnAttrList(ElemDDLNode * commaExpr,
                             ElemDDLNode * otherExpr)
  : ElemDDLList(ELM_PARTN_ATTR_LIST, commaExpr, otherExpr)
  { }

  // virtual destructor
  virtual ~ElemDDLPartnAttrList();

  // cast
  virtual ElemDDLPartnAttrList * castToElemDDLPartnAttrList();

  // methods for tracing
  virtual const NAString getText() const;

  // method for building text
  virtual NAString getSyntax() const;



private:

}; // class ElemDDLPartnAttrList

#endif // ELEMDDLPARTNATTRLIST_H

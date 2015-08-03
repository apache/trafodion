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
#ifndef ELEMDDLFILEATTRLIST_H
#define ELEMDDLFILEATTRLIST_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLFileAttrList.h
 * Description:  class for lists of file attribute elements specified in
 *               DDL statements.  Since class ElemDDLFileAttrList is
 *               derived from the base class ElemDDLList; therefore, the
 *               class ElemDDLFileAttrList also represents a left linear
 *               tree.
 *
 *               Please note that class ElemDDLFileAttrList is not derived
 *               from class ElemDDLFileAttr which represents file attribute
 *               elements.
 *
 *               
 * Created:      9/27/95
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
class ElemDDLFileAttrList;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.

// -----------------------------------------------------------------------
// definition of class ElemDDLFileAttrList
// -----------------------------------------------------------------------
class ElemDDLFileAttrList : public ElemDDLList
{

public:

  // constructor
  ElemDDLFileAttrList(ElemDDLNode * commaExpr,
                             ElemDDLNode * otherExpr)
  : ElemDDLList(ELM_FILE_ATTR_LIST, commaExpr, otherExpr)
  { }

  // virtual destructor
  virtual ~ElemDDLFileAttrList();

  // cast
  virtual ElemDDLFileAttrList * castToElemDDLFileAttrList();

  // methods for tracing
  virtual const NAString getText() const;

  // method for building text
  virtual NAString getSyntax() const;



private:

}; // class ElemDDLFileAttrList


#endif // ELEMDDLFILEATTRLIST_H

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
#ifndef ITEMNATYPE_H
#define ITEMNATYPE_H
/* -*-C++-*-
******************************************************************************
*
* File:         ItemNAType.h
* Description:  Functions and Aggregate item expressions
*		Aggregates to be used in groupby rel. operators.
*               Functions (and aggregates?) may be built-ins or user-defined.
* Created:      11/04/94
* Language:     C++
*
*
*
*
******************************************************************************
*/

// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
#include "NAType.h"

class NATypeToItem;

// -----------------------------------------------------------------------
// This class represents a node which "returns" an Item Expression
// and which has a single argument of an NAType.  The binder performs
// the magical conversion.  An instance of this class is used by the parser
// to represent the data_type portion of a cast expression. 
// -----------------------------------------------------------------------
class NATypeToItem : public ItemExpr
{
public:
  NATypeToItem( NAType *arg0) : ItemExpr(ITM_NATYPE,NULL) 
   {
      natype_pointer = arg0;
   }
  
  // virtual destructor
// warning elimination (removed "inline")
  virtual ~NATypeToItem() { delete natype_pointer; }

  // a virtual function for type propagating the node
  virtual const NAType * synthesizeType();

  virtual  ItemExpr* copyTopNode(ItemExpr* = NULL, CollHeap *oheap = 0);

  // get the degree of this node (it is a unary op).
  virtual Int32 getArity() const;

  const NAType* pushDownType(NAType& newType,
                       enum NABuiltInTypeEnum defaultQualifier);
  
  // get a printable string that identifies the operator
  virtual const NAString getText() const;

private:
  NAType   *natype_pointer;
};
#endif

/* -*-C++-*- */
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
#ifndef ELEMDDLUDFSPECIALATTRIBUTES_H
#define ELEMDDLUDFSPECIALATTRIBUTES_H
/* -*-C++-*-
******************************************************************************
*
* File:         ElemDDLUdfSpecialAttributes.h
* Description:  class for UDF Special Attributes Text (parse node)
*               elements in DDL statements.
*
*
* Created:      2/2/2010
* Language:     C++
*
*
*
*
******************************************************************************
*/

#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif
#include "SqlParserGlobals.h"

#include "ComSmallDefs.h"
#include "ElemDDLNode.h"

class ElemDDLUdfSpecialAttributes : public ElemDDLNode
{

public:

  // constructor
  ElemDDLUdfSpecialAttributes(const NAString &theSpecialAttributesText,
                              CollHeap * h = PARSERHEAP());

  // virtual destructor
  virtual ~ElemDDLUdfSpecialAttributes(void);

  // cast
  virtual ElemDDLUdfSpecialAttributes * castToElemDDLUdfSpecialAttributes(void);

  // accessor
  inline const ComString & getSpecialAttributesText(void) const
  {
    return specialAttributesText_;
  }

  //
  // methods for tracing
  //

  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;

private:

  ComString specialAttributesText_;

}; // class ElemDDLUdfSpecialAttributes

#endif /* ELEMDDLUDFSPECIALATTRIBUTES_H */

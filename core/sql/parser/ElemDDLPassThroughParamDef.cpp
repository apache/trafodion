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
/* -*-C++-*-
******************************************************************************
*
* File:         ElemDDLPassThroughParam.cpp
* Description:  methods for classes relating to the definition of a (static)
*               pass through input to be included to the actual parameter
*               list during a routine invocation.
*
*
* Created:      7/14/09
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "ComASSERT.h"
#include "ComDiags.h"
#include "ElemDDLPassThroughParamDef.h"


// -----------------------------------------------------------------------
// methods for class ElemDDLPassThroughParamDef
// -----------------------------------------------------------------------

// constructors
ElemDDLPassThroughParamDef::ElemDDLPassThroughParamDef(CollHeap * heap) // default is PARSERHEAP()
  : ElemDDLNode(ELM_PASS_THROUGH_PARAM_DEF_ELEM)
  , passThroughParamDefKind_(eADD_PASS_THROUGH_INPUT)
  , paramIndex_(0)                   // currently not used
  , isParamIndexSpec_(FALSE)
  , paramDirection_(COM_INPUT_PARAM) // currently not used
  , paramName_("", heap)             // currently not used
  , passThroughValueExpr_(NULL)
  , fileOssPathName_("", heap)
  , fileContentFormat_(eFILE_CONTENT_FORMAT_BINARY)
  , passThroughInputType_(COM_ROUTINE_PASS_THROUGH_INPUT_BINARY_TYPE)
{
}  // ElemDDLPassThroughParamDef()

ElemDDLPassThroughParamDef::ElemDDLPassThroughParamDef
  ( ItemExpr * passThroughValueExpr
  , CollHeap * heap                             // default is PARSERHEAP()
  )
  : ElemDDLNode(ELM_PASS_THROUGH_PARAM_DEF_ELEM)
  , passThroughParamDefKind_(eADD_PASS_THROUGH_INPUT)
  , paramIndex_(0)
  , isParamIndexSpec_(FALSE)
  , paramDirection_(COM_INPUT_PARAM)
  , paramName_("", heap)
  , passThroughValueExpr_(passThroughValueExpr) // shallow copy
  , fileOssPathName_("", heap)
  , fileContentFormat_(eFILE_CONTENT_FORMAT_BINARY)
  , passThroughInputType_(COM_ROUTINE_PASS_THROUGH_INPUT_BINARY_TYPE)
{
}  // ElemDDLPassThroughParamDef()

ElemDDLPassThroughParamDef::ElemDDLPassThroughParamDef(const NAString & fileOssPathName,
                                                       CollHeap * heap) // default is PARSERHEAP()
  : ElemDDLNode(ELM_PASS_THROUGH_PARAM_DEF_ELEM)
  , passThroughParamDefKind_(eADD_PASS_THROUGH_INPUT)
  , paramIndex_(0)
  , isParamIndexSpec_(FALSE)
  , paramDirection_(COM_INPUT_PARAM)
  , paramName_("", heap)
  , passThroughValueExpr_(NULL)
  , fileOssPathName_(fileOssPathName, heap) // deep copy
  , fileContentFormat_(eFILE_CONTENT_FORMAT_BINARY)
  , passThroughInputType_(COM_ROUTINE_PASS_THROUGH_INPUT_BINARY_TYPE)
{
}  // ElemDDLPassThroughParamDef()

// virtual destructor
ElemDDLPassThroughParamDef::~ElemDDLPassThroughParamDef()
{
}

// cast
ElemDDLPassThroughParamDef *
ElemDDLPassThroughParamDef::castToElemDDLPassThroughParamDef()
{
  return this;
}

//
// accessors
//

// get the degree of this node
int
ElemDDLPassThroughParamDef::getArity() const
{
  return 0;
}

ExprNode *
ElemDDLPassThroughParamDef::getChild(long index)
{
  ComASSERT(FALSE);  // No child node exists -- should not call this.
  return this;       // Just to keep the compiler happy.
}

//
// mutators
//


//
// methods for tracing
//

const NAString
ElemDDLPassThroughParamDef::displayLabel1() const
{
  if (NOT getParamName().isNull())
    return NAString("Param name: ") + getParamName();
  else if (isParamIndexSpecified())
    return (NAString("Pass-through input position: ") +
            LongToNAString(static_cast<long>(getParamPosition())));
  else
    return "Neither param name nor pass-through input Position specified";
}

const NAString
ElemDDLPassThroughParamDef::displayLabel2() const
{
  if (NOT getPassThroughValueExpr()->getText().isNull())
  {
    NAString displayText("Pass Through value: ");
    displayText += getPassThroughValueExpr()->getText();
    if (getPassThroughInputType() EQU COM_ROUTINE_PASS_THROUGH_INPUT_TEXT_TYPE)
      displayText += " TEXT";
    else
      displayText += " BINARY";

    return displayText;
  }
  else
    return "Pass-through input value not specified";
}

const NAString
ElemDDLPassThroughParamDef::getText() const
{
  return "ElemDDLPassThroughParamDef";
}

//
// End of File
//

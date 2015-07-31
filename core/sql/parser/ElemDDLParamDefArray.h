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
//
**********************************************************************/
#ifndef ELEMDDLPARAMDEFARRAY_H
#define ELEMDDLPARAMDEFARRAY_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLParamDefArray.h
 * Description:  class for an array of pointers pointing to instances of
 *               class ElemDDLParamDef
 *
 *
 * Created:      10/12/1999
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "Collections.h"
#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif
#include "SqlParserGlobals.h"
#include "ElemDDLParamDef.h"
#include "ElemDDLPassThroughParamDef.h"

// -----------------------------------------------------------------------
// Definition of class ElemDDLParamDefArray
// -----------------------------------------------------------------------
class ElemDDLParamDefArray : public LIST(ElemDDLParamDef *)
{

public:

  // constructor
  ElemDDLParamDefArray(CollHeap *heap = PARSERHEAP())
    : LIST(ElemDDLParamDef *)(heap)
  {
  }

  // virtual destructor
  virtual ~ElemDDLParamDefArray()
  {
  }

private:

}; // class ElemDDLParamDefArray

// -----------------------------------------------------------------------
// Definition of class ElemDDLPassThroughParamDefArray
// -----------------------------------------------------------------------
class ElemDDLPassThroughParamDefArray : public LIST(ElemDDLPassThroughParamDef *)
{

public:

  // constructor
  ElemDDLPassThroughParamDefArray(CollHeap *heap = PARSERHEAP())
    : LIST(ElemDDLPassThroughParamDef *)(heap)
  {
  }

  // virtual destructor
  virtual ~ElemDDLPassThroughParamDefArray()
  {
  }

private:

}; // class ElemDDLPassThroughParamDefArray

#endif /* ELEMDDLPARAMDEFARRAY_H */

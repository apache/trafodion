/* -*-C++-*- */
#ifndef ELEMDDLUDFVERSIONTAG_H
#define ELEMDDLUDFVERSIONTAG_H
/* -*-C++-*-
******************************************************************************
*
* File:         ElemDDLUdfVersionTag.h
* Description:  class for function version tag (parser node)
*               elements in DDL statements
*
*
* Created:      1/28/2010
* Language:     C++
*
*
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

class ElemDDLUdfVersionTag : public ElemDDLNode
{

public:

  // default constructor
  ElemDDLUdfVersionTag(const NAString &theVersionTag,
                       CollHeap * h = PARSERHEAP());

  // virtual destructor
  virtual ~ElemDDLUdfVersionTag();

  // cast
  virtual ElemDDLUdfVersionTag * castToElemDDLUdfVersionTag(void);

  // accessor
  inline const NAString &getVersionTag(void) const
  {
    return versionTag_;
  }

  //
  // methods for tracing
  //

  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;

private:

  NAString versionTag_;

}; // class ElemDDLUdfVersionTag

#endif /* ELEMDDLUDFVERSIONTAG_H */

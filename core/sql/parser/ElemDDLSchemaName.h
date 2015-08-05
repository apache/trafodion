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
#ifndef ELEMDDLSCHEMANAME_H
#define ELEMDDLSCHEMANAME_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLSchemaName.h
 * Description:  Temporary parse node to contain schema name and
 *               authorization identifier
 *
 * Created:      4/3/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ObjectNames.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLSchemaName;

// -----------------------------------------------------------------------
// ElemDDLSchemaName
//
// A temporary parse node to contain a schema name and an optional
// authorization identifier.
// -----------------------------------------------------------------------
class ElemDDLSchemaName : public ElemDDLNode
{

public:

  // default constructor
  ElemDDLSchemaName(const SchemaName & aSchemaName,
                    const NAString & anAuthorizationID = "",
                    CollHeap * h=PARSERHEAP())
    : ElemDDLNode(ELM_SCHEMA_NAME_ELEM),
      schemaName_(aSchemaName, h),
      authorizationID_(anAuthorizationID, h)  {}

  // copy ctor
  ElemDDLSchemaName (const ElemDDLSchemaName & orig, CollHeap * h=PARSERHEAP()) ; // not written

  // virtual destructor
  virtual ~ElemDDLSchemaName();

  // cast
  virtual ElemDDLSchemaName * castToElemDDLSchemaName();

  // accessors

  const NAString &
  getAuthorizationID() const
  {
    return authorizationID_;
  }

  const SchemaName &
  getSchemaName() const
  {
    return schemaName_;
  }

private:

  SchemaName schemaName_;
  NAString authorizationID_;

}; // class ElemDDLSchemaName

#endif /* ELEMDDLSCHEMANAME_H */



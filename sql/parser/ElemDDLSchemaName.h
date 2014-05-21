/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1995-2014 Hewlett-Packard Development Company, L.P.
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
                    const NAString & anAuthorizationId = "",
                    CollHeap * h=PARSERHEAP())
    : ElemDDLNode(ELM_SCHEMA_NAME_ELEM),
      schemaName_(aSchemaName, h),
      authorizationId_(anAuthorizationId, h),
      allowLocationReuse_(FALSE)
  {}

  // copy ctor
  ElemDDLSchemaName (const ElemDDLSchemaName & orig, CollHeap * h=PARSERHEAP()) ; // not written

  // virtual destructor
  virtual ~ElemDDLSchemaName();

  // cast
  virtual ElemDDLSchemaName * castToElemDDLSchemaName();

  // accessors

  const NAString &
  getAuthorizationId() const
  {
    return authorizationId_;
  }

  void
  setSubvolumeName(const NAString &subvolName)
  {
    subvolumeName_ = subvolName;
  }

  void
  setLocationReuse(NABoolean reuse)
  {
    allowLocationReuse_ = reuse;
  }

  const NAString &
  getSubvolumeName() const
  {
    return subvolumeName_;
  }

  const SchemaName &
  getSchemaName() const
  {
    return schemaName_;
  }

  NABoolean getLocationReuse() const { return allowLocationReuse_; }

private:

  SchemaName schemaName_;
  NAString authorizationId_;
  NAString subvolumeName_;
  NABoolean allowLocationReuse_;

}; // class ElemDDLSchemaName

#endif /* ELEMDDLSCHEMANAME_H */



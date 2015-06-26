/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2014 Hewlett-Packard Development Company, L.P.
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
#ifndef ELEMDDLAUTHSCHEMA_H
#define ELEMDDLAUTHSCHEMA_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLAuthSchema.h
 * Description:  Temporary parse node to contain schema name and
 *               schema class
 *
 * Created:      12/10/2014
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif
#include "SqlParserGlobals.h"
#include "ElemDDLSchemaName.h"
#include "ObjectNames.h"
#include "ComSmallDefs.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLAuthSchema;

// -----------------------------------------------------------------------
// ElemDDLAuthSchema
//
// A temporary parse node to contain a schema name and an optional
// authorization identifier.
// -----------------------------------------------------------------------
class ElemDDLAuthSchema : public ElemDDLNode
{

public:

  // default constructor
  ElemDDLAuthSchema(const SchemaName & aSchemaName,
                    const ComSchemaClass schemaClass,
                    CollHeap * h=PARSERHEAP())
    : ElemDDLNode(ELM_AUTH_SCHEMA_ELEM),
      schemaName_(aSchemaName, h),
      schemaClass_(schemaClass),
      isSchemaNameSpecified_(TRUE)  {}
      
  ElemDDLAuthSchema(const ComSchemaClass schemaClass,
                    CollHeap * h=PARSERHEAP())
    : ElemDDLNode(ELM_AUTH_SCHEMA_ELEM),
      schemaClass_(schemaClass),
      isSchemaNameSpecified_(FALSE)  {}

  // copy ctor
  ElemDDLAuthSchema (const ElemDDLAuthSchema & orig, CollHeap * h=PARSERHEAP()) ; // not written

  // virtual destructor
  virtual ~ElemDDLAuthSchema();

  // cast
  virtual ElemDDLAuthSchema * castToElemDDLAuthSchema();

  // accessors

  const ComSchemaClass
  getSchemaClass() const
  {
    return schemaClass_;
  }

  const SchemaName &
  getSchemaName() const
  {
    return schemaName_;
  }

  NABoolean
  isSchemaNameSpecified() const
  {
    return isSchemaNameSpecified_;
  }

private:

  SchemaName schemaName_;
  ComSchemaClass schemaClass_;
  NABoolean isSchemaNameSpecified_;

}; // class ElemDDLAuthSchema

#endif /* ELEMDDLAUTHSCHEMA_H */



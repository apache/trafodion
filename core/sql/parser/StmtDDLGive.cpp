/* -*-C++-*-
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
****************************************************************************
 *
 * File:         StmtDDLGive.C
 * Description:  Methods for classes representing DDL Give Statements
 *
 *               Also contains definitions of non-inline methods of
 *               classes relating to view usages.
 *
 * Created:      7/13/2006
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

#define   SQLPARSERGLOBALS_FLAGS        // must precede all #include's

#include <stdlib.h>
#ifndef NDEBUG
#include <iostream>
#endif
#include "AllElemDDLPartition.h"
#include "AllElemDDLParam.h"
#include "AllElemDDLUdr.h"
#include "AllStmtDDLGive.h"
#include "BaseTypes.h"
#include "ComDiags.h"
#include "ComOperators.h"

#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif

#include "SqlParserGlobals.h"   // must be last #include


// -----------------------------------------------------------------------
// methods for class StmtDDLGiveCatalog
// -----------------------------------------------------------------------

//
// constructor
//
StmtDDLGiveCatalog::StmtDDLGiveCatalog(const NAString & aCatalogName,
                                       const NAString & aUserID)
  : StmtDDLNode(DDL_GIVE_CATALOG),
    catalogName_(aCatalogName, PARSERHEAP()),
    userID_(aUserID, PARSERHEAP())
{


} // StmtDDLGiveCatalog::StmtDDLGiveCatalog()

//
// virtual destructor
//
StmtDDLGiveCatalog::~StmtDDLGiveCatalog()
{
  // delete all children
  for (Int32 i = 0; i < getArity(); i++)
  {
    delete getChild(i);
  }
}

//
// cast
//
StmtDDLGiveCatalog *
StmtDDLGiveCatalog::castToStmtDDLGiveCatalog()
{
  return this;
}

//
// accessors
//

Int32
StmtDDLGiveCatalog::getArity() const
{
  return MAX_STMT_DDL_GIVE_CATALOG_ARITY;
}

ExprNode *
StmtDDLGiveCatalog::getChild(Lng32 index)
{
  ComASSERT(index EQU INDEX_GIVE_CATALOG_ATTRIBUTE_LIST);
  return attributeList_;
}

//
// mutators
//


//
// Get the information in the parse node pointed by parameter
// pAttrNode.  Update the corresponding data members (in this
// class) accordingly.  Also check for duplicate clauses.
//
void
StmtDDLGiveCatalog::setAttribute(ElemDDLNode * pAttrNode)
{
  ComASSERT(pAttrNode NEQ NULL);

  {
    NAAbort("StmtDDLGiveCatalog.C", __LINE__, "internal logic error");
  }
}


void
StmtDDLGiveCatalog::setChild(Lng32 index, ExprNode * pChildNode)
{
  ComASSERT(index EQU INDEX_GIVE_CATALOG_ATTRIBUTE_LIST);
  if (pChildNode NEQ NULL)
  {
    attributeList_ = pChildNode->castToElemDDLNode();
  }
  else
  {
    attributeList_ = NULL;
  }
}


//
// methods for tracing
//

const NAString
StmtDDLGiveCatalog::displayLabel1() const
{
  return NAString("Catalog name: ") + getCatalogName();
}

const NAString
StmtDDLGiveCatalog::displayLabel2() const
{
  if (NOT getUserID().isNull())
  {
    return NAString("User ID: ") + getUserID();
  }
  else
  {
    return NAString("User ID not specified.");
  }
}


const NAString
StmtDDLGiveCatalog::getText() const
{
  return "StmtDDLGiveCatalog";
}



// -----------------------------------------------------------------------
// methods for class StmtDDLGiveAll
// -----------------------------------------------------------------------

//
// constructor
//
StmtDDLGiveAll::StmtDDLGiveAll(
   const NAString & fromAuthID,
   const NAString & toAuthID)
  : StmtDDLNode(DDL_GIVE_ALL),
    fromAuthID_(fromAuthID, PARSERHEAP()),
    toAuthID_(toAuthID, PARSERHEAP())
{

} // StmtDDLGiveAll::StmtDDLGiveAll()

//
// virtual destructor
//
StmtDDLGiveAll::~StmtDDLGiveAll()
{
}

//
// cast
//
StmtDDLGiveAll *
StmtDDLGiveAll::castToStmtDDLGiveAll()
{
  return this;
}


//
// methods for tracing
//

const NAString
StmtDDLGiveAll::displayLabel1() const
{
  return NAString("From authID: ") + getFromID();
}

const NAString
StmtDDLGiveAll::displayLabel2() const
{
  return NAString("To authID: ") + getToID();
}


const NAString
StmtDDLGiveAll::getText() const
{
  return "StmtDDLGiveAll";
}



// -----------------------------------------------------------------------
// methods for class StmtDDLGiveObject
// -----------------------------------------------------------------------

//
// constructor
//
StmtDDLGiveObject::StmtDDLGiveObject(
   ComObjectType objectType,
   const QualifiedName & qualifiedName,
   const NAString & aUserID)
  : StmtDDLNode(DDL_GIVE_OBJECT),
    objectType_(objectType),
    objectName_(PARSERHEAP()),
    objectQualName_(qualifiedName, PARSERHEAP()),
    userID_(aUserID, PARSERHEAP())
{

  objectName_ = objectQualName_.getQualifiedNameAsAnsiString();

} // StmtDDLGiveObject::StmtDDLGiveObject()

//
// virtual destructor
//
StmtDDLGiveObject::~StmtDDLGiveObject()
{
}

//
// cast
//
StmtDDLGiveObject *
StmtDDLGiveObject::castToStmtDDLGiveObject()
{
  return this;
}


//
// methods for tracing
//

const NAString
StmtDDLGiveObject::displayLabel1() const
{
  return NAString("Object name: ") + getObjectName();
}

const NAString
StmtDDLGiveObject::displayLabel2() const
{
  if (NOT getUserID().isNull())
  {
    return NAString("User ID: ") + getUserID();
  }
  else
  {
    return NAString("User ID not specified.");
  }
}


const NAString
StmtDDLGiveObject::getText() const
{
  return "StmtDDLGiveObject";
}















// -----------------------------------------------------------------------
// methods for class StmtDDLGiveSchema
// -----------------------------------------------------------------------

//
// constructor
//
StmtDDLGiveSchema::StmtDDLGiveSchema(
   const SchemaName & schemaName,
   const NAString & authID,
   ComDropBehavior dropBehavior)
  : StmtDDLNode(DDL_GIVE_SCHEMA),
    authID_(authID, PARSERHEAP()),
    dropBehavior_(dropBehavior) 
{

    catalogName_ = schemaName.getCatalogNameAsAnsiString();
    schemaName_ = schemaName.getSchemaName();
    
} // StmtDDLGiveSchema::StmtDDLGiveSchema()

//
// virtual destructor
//
StmtDDLGiveSchema::~StmtDDLGiveSchema()
{
}

//
// cast
//
StmtDDLGiveSchema *
StmtDDLGiveSchema::castToStmtDDLGiveSchema()
{
  return this;
}


//
// methods for tracing
//

const NAString
StmtDDLGiveSchema::displayLabel1() const
{
  return NAString("Schema name: ") + getCatalogName() + getSchemaName();
}

const NAString
StmtDDLGiveSchema::displayLabel2() const
{
  if (NOT getAuthID().isNull())
  {
    return NAString("Auth ID: ") + getAuthID();
  }
  else
  {
    return NAString("Auth ID not specified.");
  }
}


const NAString
StmtDDLGiveSchema::getText() const
{
  return "StmtDDLGiveSchema";
}



//
// End of File
//


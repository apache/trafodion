/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1995-2015 Hewlett-Packard Development Company, L.P.
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
#ifndef STMTDDLCREATESCHEMA_H
#define STMTDDLCREATESCHEMA_H

/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLCreateSchema.h
 * Description:  Create Schema Statement (parse node)
 *
 *
 * Created:      3/9/95
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#include "ElemDDLSchemaName.h"
#include "StmtDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLCreateSchema;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None.


// -----------------------------------------------------------------------
// Create Schema statement
// -----------------------------------------------------------------------
class StmtDDLCreateSchema : public StmtDDLNode
{
public:

  // initialize constructor
  StmtDDLCreateSchema(const ElemDDLSchemaName & aSchemaName,
                      ComSchemaClass schemaClass,
                      CharType* pCharType,
                      CollHeap    * heap = PARSERHEAP());

  // virtual destructor
  virtual ~StmtDDLCreateSchema();

  // cast
  virtual StmtDDLCreateSchema * castToStmtDDLCreateSchema();

  //
  // accessors
  //

  inline const NAString & getAuthorizationID() const;
  inline ComSchemaClass getSchemaClass() const;

  inline const NAString & getSchemaName() const;
  inline const SchemaName & getSchemaNameAsQualifiedName() const;
  inline       SchemaName & getSchemaNameAsQualifiedName();

  NABoolean createIfNotExists() { return createIfNotExists_; }
  void setCreateIfNotExists(NABoolean v) { createIfNotExists_ = v; }

  //
  // other public methods
  //

  // method for processing
  ExprNode * bindNode(BindWA *bindWAPtr);

  // method for collecting information
  void synthesize();

  // methods for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString displayLabel2() const;
  virtual const NAString getText() const;

  const CharType* getCharType() const;

private:

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------

  //
  // please do not use the following methods
  //
  
  StmtDDLCreateSchema();                                         // DO NOT USE
  StmtDDLCreateSchema(const StmtDDLCreateSchema &);              // DO NOT USE
  StmtDDLCreateSchema & operator=(const StmtDDLCreateSchema &);  // DO NOT USE

  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------
  
  NAString schemaName_;
  NAString authorizationID_;
  SchemaName schemaQualName_;
  CharType *pCharType_;
  ComSchemaClass schemaClass_;
  NABoolean createIfNotExists_;

}; // class StmtDDLCreateSchema

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLCreateSchema
// -----------------------------------------------------------------------

//
// accessors
//
inline SchemaName &
StmtDDLCreateSchema::getSchemaNameAsQualifiedName() 
{
  return schemaQualName_;
}

inline const SchemaName &
StmtDDLCreateSchema::getSchemaNameAsQualifiedName() const
{
  return schemaQualName_;
}

inline const NAString &
StmtDDLCreateSchema::getAuthorizationID() const
{
  return authorizationID_;
}

inline ComSchemaClass
StmtDDLCreateSchema::getSchemaClass() const
{
  return schemaClass_;
}

inline const NAString &
StmtDDLCreateSchema::getSchemaName() const
{
  return schemaName_;
}

inline const CharType*
StmtDDLCreateSchema::getCharType() const
{
  return pCharType_;
}

#endif // STMTDDLCREATESCHEMA_H

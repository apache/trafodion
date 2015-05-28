#ifndef STMTDDLDROPSCHEMA_H
#define STMTDDLDROPSCHEMA_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         StmtDDLDropSchema.h
 * Description:  class for parse node representing Drop Schema statements
 *
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
 *
 *
 *****************************************************************************
 */


#include "ComSmallDefs.h"
#include "StmtDDLNode.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class StmtDDLDropSchema;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
// None

// -----------------------------------------------------------------------
// Create Catalog statement
// -----------------------------------------------------------------------
class StmtDDLDropSchema : public StmtDDLNode
{

public:

  // constructor
  StmtDDLDropSchema(const ElemDDLSchemaName & aSchemaNameParseNode,
                    ComDropBehavior dropBehavior,
                    ComBoolean cleanupMode,
                    ComBoolean dropObjectsOnly);

  // copy ctor
  StmtDDLDropSchema (const StmtDDLDropSchema & orig) ;

  // virtual destructor
  virtual ~StmtDDLDropSchema();

  // cast
  virtual StmtDDLDropSchema * castToStmtDDLDropSchema();

  // accessors
  inline ComDropBehavior getDropBehavior() const;
  inline ComBoolean getCleanupMode() const;
  inline const NAString & getSchemaName() const;
  inline const SchemaName & getSchemaNameAsQualifiedName() const;
  inline       SchemaName & getSchemaNameAsQualifiedName();

  const NABoolean dropIfExists() const { return dropIfExists_; }
  
  void setDropIfExists(NABoolean v) { dropIfExists_ = v; }
  ComBoolean dropObjectsOnly() { return dropObjectsOnly_; }

  // for binding
  ExprNode * bindNode(BindWA *bindWAPtr);

  // for tracing
  virtual const NAString displayLabel1() const;
  virtual const NAString displayLabel2() const;
  virtual const NAString getText() const;


private:

  NAString schemaName_;
  SchemaName schemaQualName_;
  ComDropBehavior dropBehavior_;
  ComBoolean cleanupMode_;
  NABoolean dropIfExists_;

  // if TRUE, then only drop objects in the schema. Do not drop the schema.
  ComBoolean dropObjectsOnly_;

}; // class StmtDDLDropSchema

// -----------------------------------------------------------------------
// definitions of inline methods for class StmtDDLDropSchema
// -----------------------------------------------------------------------

//
// accessors
//

inline SchemaName &
StmtDDLDropSchema::getSchemaNameAsQualifiedName() 
{
  return schemaQualName_;
}

inline const SchemaName &
StmtDDLDropSchema::getSchemaNameAsQualifiedName() const
{
  return schemaQualName_;
}

inline ComDropBehavior
StmtDDLDropSchema::getDropBehavior() const
{
  return dropBehavior_;
}

inline ComBoolean
StmtDDLDropSchema::getCleanupMode() const
{
  return cleanupMode_;
}

inline const NAString &
StmtDDLDropSchema::getSchemaName() const
{
  return schemaName_;
}

#endif // STMTDDLDROPSCHEMA_H

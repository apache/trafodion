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
#include "StmtDDLAddConstraintCheckArray.h"
#include "StmtDDLAddConstraintRIArray.h"
#include "StmtDDLAddConstraintUniqueArray.h"
#include "StmtDDLCreateIndexArray.h"
#include "StmtDDLCreateTableArray.h"
#include "StmtDDLCreateViewArray.h"
#include "StmtDDLCreateTriggerArray.h"
#include "StmtDDLGrantArray.h"

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

  //
  // The following global friend function is defined in
  // StmtDDLCreate.C.  For information on how this function
  // is used, please look at the comments in header file
  // StmtDDLCreateTable.h that describes similar global
  // functions.
  //
  friend void StmtDDLCreateSchema_visitSchemaElement(
       ElemDDLNode * pCreateSchema,
       CollIndex index,
       ElemDDLNode * pElement);

public:

  // initialize constructor
  StmtDDLCreateSchema(const ElemDDLSchemaName & aSchemaName,
                      CharType* pCharType,
                      ElemDDLNode * pSchemaElemList = NULL,
                      CollHeap    * heap = PARSERHEAP());

  // virtual destructor
  virtual ~StmtDDLCreateSchema();

  // cast
  virtual StmtDDLCreateSchema * castToStmtDDLCreateSchema();

  //
  // accessors
  //

  inline       StmtDDLAddConstraintCheckArray & getAddConstraintCheckArray();
  inline const StmtDDLAddConstraintCheckArray & getAddConstraintCheckArray()
                                                                       const;
  inline       StmtDDLAddConstraintRIArray & getAddConstraintRIArray();
  inline const StmtDDLAddConstraintRIArray & getAddConstraintRIArray() const;

  inline       StmtDDLAddConstraintUniqueArray & getAddConstraintUniqueArray();
  inline const StmtDDLAddConstraintUniqueArray & getAddConstraintUniqueArray()
                                                 const;

  virtual Int32 getArity() const;

  inline const NAString &getSubvolumeName() const;
  inline NABoolean getLocationReuse() const { return allowLocationReuse_; };

  inline const NAString & getAuthorizationId() const;

  virtual ExprNode * getChild(Lng32 index);

  inline       StmtDDLCreateIndexArray & getCreateIndexArray();
  inline const StmtDDLCreateIndexArray & getCreateIndexArray() const;

  inline       StmtDDLCreateTableArray & getCreateTableArray();
  inline const StmtDDLCreateTableArray & getCreateTableArray() const;

  inline       StmtDDLCreateViewArray  & getCreateViewArray();
  inline const StmtDDLCreateViewArray  & getCreateViewArray() const;

  inline       StmtDDLCreateTriggerArray  & getCreateTriggerArray();
  inline const StmtDDLCreateTriggerArray  & getCreateTriggerArray() const;

  inline       StmtDDLGrantArray & getGrantArray();
  inline const StmtDDLGrantArray & getGrantArray() const;

  inline const NAString & getSchemaName() const;
  inline const SchemaName & getSchemaNameAsQualifiedName() const;
  inline       SchemaName & getSchemaNameAsQualifiedName();

  inline const ComBoolean isCompoundCreateSchema() const;
  //
  // mutators
  //

  virtual void setChild(Lng32 index, ExprNode * newNode);

  //
  // other public methods
  //

  // method for processing
  ExprNode * bindNode(BindWA *bindWAPtr);

  // method for collecting information
  void synthesize();

        // collects information in the parse sub-tree and
        // copy/move them to the current parse node.

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
  NAString authorizationId_;
  NAString subvolumeName_; 
  SchemaName schemaQualName_;
  NABoolean allowLocationReuse_;
  StmtDDLCreateIndexArray    createIndexArray_;
  StmtDDLCreateTableArray    createTableArray_;
  StmtDDLCreateViewArray     createViewArray_;
  StmtDDLCreateTriggerArray  createTriggerArray_;
  StmtDDLGrantArray          grantArray_;
  ComBoolean                 isCompoundCreateSchema_;
  CharType                   *pCharType_;

  //
  // Each element of the following arrays is a pointer
  // pointing to a StmtDDLAddConstraintCheck or
  // StmtDDLAddConstraintRI parse node in the parser
  // sub-tree containing the schema element definitions.
  //
  // Note that the array addConstraintUniqueArray_ does
  // not include the Primary Key constraint.
  //
  StmtDDLAddConstraintCheckArray  addConstraintCheckArray_;
  StmtDDLAddConstraintRIArray     addConstraintRIArray_;
  StmtDDLAddConstraintUniqueArray addConstraintUniqueArray_;

  // pointers to child parse nodes

  enum { INDEX_SCHEMA_ELEMENT_LIST = 0,
         MAX_STMT_DDL_CREATE_SCHEMA_ARITY };

  ElemDDLNode * children_[MAX_STMT_DDL_CREATE_SCHEMA_ARITY];
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

inline StmtDDLAddConstraintCheckArray &
StmtDDLCreateSchema::getAddConstraintCheckArray()
{
  return addConstraintCheckArray_;
}

inline const StmtDDLAddConstraintCheckArray &
StmtDDLCreateSchema::getAddConstraintCheckArray() const
{
  return addConstraintCheckArray_;
}

inline StmtDDLAddConstraintRIArray &
StmtDDLCreateSchema::getAddConstraintRIArray()
{
  return addConstraintRIArray_;
}

inline const StmtDDLAddConstraintRIArray &
StmtDDLCreateSchema::getAddConstraintRIArray() const
{
  return addConstraintRIArray_;
}

inline StmtDDLAddConstraintUniqueArray &
StmtDDLCreateSchema::getAddConstraintUniqueArray()
{
  return addConstraintUniqueArray_;
}

inline const StmtDDLAddConstraintUniqueArray &
StmtDDLCreateSchema::getAddConstraintUniqueArray() const
{
  return addConstraintUniqueArray_;
}

inline const NAString &
StmtDDLCreateSchema::getAuthorizationId() const
{
  return authorizationId_;
}

inline const NAString &
StmtDDLCreateSchema::getSubvolumeName() const
{
  return subvolumeName_;
}

inline const StmtDDLCreateIndexArray &
StmtDDLCreateSchema::getCreateIndexArray() const
{
  return createIndexArray_;
}

inline StmtDDLCreateIndexArray &
StmtDDLCreateSchema::getCreateIndexArray()
{
  return createIndexArray_;
}

inline const StmtDDLCreateTableArray &
StmtDDLCreateSchema::getCreateTableArray() const
{
  return createTableArray_;
}

inline StmtDDLCreateTableArray &
StmtDDLCreateSchema::getCreateTableArray()
{
  return createTableArray_;
}

inline const StmtDDLCreateViewArray &
StmtDDLCreateSchema::getCreateViewArray() const
{
  return createViewArray_;
}

inline StmtDDLCreateViewArray &
StmtDDLCreateSchema::getCreateViewArray()
{
  return createViewArray_;
}

inline const StmtDDLCreateTriggerArray &
StmtDDLCreateSchema::getCreateTriggerArray() const
{
  return createTriggerArray_;
}

inline StmtDDLCreateTriggerArray &
StmtDDLCreateSchema::getCreateTriggerArray()
{
  return createTriggerArray_;
}

inline const StmtDDLGrantArray &
StmtDDLCreateSchema::getGrantArray() const
{
  return grantArray_;
}

inline StmtDDLGrantArray &
StmtDDLCreateSchema::getGrantArray()
{
  return grantArray_;
}

inline const NAString &
StmtDDLCreateSchema::getSchemaName() const
{
  return schemaName_;
}

inline const ComBoolean
StmtDDLCreateSchema::isCompoundCreateSchema() const
{
  return isCompoundCreateSchema_;
}

inline const CharType*
StmtDDLCreateSchema::getCharType() const
{
  return pCharType_;
}

#endif // STMTDDLCREATESCHEMA_H

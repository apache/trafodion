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
#ifndef ELEMDDLCOLDEF_H
#define ELEMDDLCOLDEF_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         ElemDDLColDef.h
 * Description:  class for Column Definition elements in DDL statements
 *               
 *               
 * Created:      3/29/95
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

#include "ElemDDLNode.h"
#include "ElemDDLColRefArray.h"
#include "ElemDDLConstraintArray.h"
#include "ElemDDLConstraintNotNull.h"
#include "ElemDDLSGOptions.h"
#include "ExpLOBenums.h"

// -----------------------------------------------------------------------
// contents of this file
// -----------------------------------------------------------------------
class ElemDDLColDef;
class ElemProxyColDef;

// -----------------------------------------------------------------------
// forward references
// -----------------------------------------------------------------------
class NAType;
class ItemExpr;

// -----------------------------------------------------------------------
// Column Definition elements in DDL statements.
// -----------------------------------------------------------------------
class ElemDDLColDef : public ElemDDLNode
{

public:

  enum defaultClauseStatusType { DEFAULT_CLAUSE_NOT_SPEC = 0,
                                 NO_DEFAULT_CLAUSE_SPEC = 1,
                                 DEFAULT_CLAUSE_SPEC = 2};
  enum { INDEX_ELEM_DDL_COL_ATTR_LIST = 0,
         MAX_ELEM_DDL_COL_DEF_ARITY = 1};

  // default constructor
  ElemDDLColDef(
       const NAString * columnFamily,
       const NAString * columnName,
       NAType * pColumnDataType,
       ElemDDLNode * pColAttrList = NULL,
       CollHeap * heap = PARSERHEAP());

  // copy ctor
  ElemDDLColDef (const ElemDDLColDef & orig, CollHeap * h=0) ; // not written

  // virtual destructor
  virtual ~ElemDDLColDef();

  // cast
  virtual ElemDDLColDef * castToElemDDLColDef();

  //
  // accessors
  //

  virtual Int32 getArity() const;
  virtual ExprNode * getChild(Lng32 index);

  inline NAType * getColumnDataType() const;
  inline const NAString & getColumnFamily() const;
  inline const NAString & getColumnName() const;

  inline const ElemDDLConstraintArray & getConstraintArray() const;

        // Note that columnConstraintArray_ does not include either
        // Not Null or Primary Key column constraint definition.

  inline ElemDDLConstraintArray & getConstraintArray();
  inline ElemDDLConstraintNotNull * getConstraintNotNull() const;
  inline ElemDDLConstraintPK * getConstraintPK() const;

  void setDefaultClauseStatus(defaultClauseStatusType d)
  { defaultClauseStatus_ = d; }

  void setDefaultAttribute(ElemDDLNode * pColDefaultNode);

  inline const defaultClauseStatusType getDefaultClauseStatus() const;

        // Currently, only three cases are available:
        // 1. Neither DEFAULT clause nor NO DEFAULT clause) appears.
        // 2. The NO DEFAULT clause appears.
        // 3. The DEFAULT clause appears.
  
  inline ItemExpr * getDefaultValueExpr() const;
  inline Int32 getErrorCode() const;
  
        // returns the default value specified in the DEFAULT
        // clause; returns the NULL pointer value if NO DEFAULT
        // clause appears or the DEFAULT clause does not appear.
  
  inline const NAString & getComputedDefaultExpr() const;
  inline const NAString & getDefaultExprString() const;
  inline const NAString & getHeading() const;
  inline const NABoolean getIsConstraintNotNullSpecified() const;
  inline const NABoolean getIsConstraintPKSpecified() const;
  inline const NABoolean getIsHeadingSpecified() const;
  inline const ComColumnClass getColumnClass() const;

  inline const ElemDDLColRefArray & getPrimaryKeyColRefArray() const;
  inline ElemDDLColRefArray & getPrimaryKeyColRefArray();

        // get list of column names specified in PRIMARY KEY clause

  inline const NABoolean isHeadingSpecified() const;

        // same as getIsHeadingSpecified()

  inline const NABoolean isNotNullConstraintSpecified() const;

        // same as getIsConstraintNotNullSpecified()

  inline const NABoolean isNotNullNondroppable() const;

        // returns TRUE if the NOT NULL NONDROPPABLE constraint
        // appears; returns FALSE otherwise.

  inline const NABoolean getIsLoggableSpecified() const;

  inline const NABoolean getIsLoggable() const;

  inline const NABoolean isPrimaryKeyConstraintSpecified() const;
  
        // same as getIsConstraintPKSpecified()
  
  inline const ComColumnDirection getDirection(void) const;

  inline ElemDDLSGOptions * getSGOptions() const;

  inline NAString * getSGLocation() const;
  
  inline NABoolean isDivisionColumn() const;
  inline ComSInt32 getDivisionColumnSequenceNumber() const;

  //
  // mutators
  //
  
  virtual void setChild(Lng32 index, ExprNode * pChildNode);

  void setDefaultValueExpr(ItemExpr *pNewDefValNode);
  inline void setHeading(const NAString &heading);
  inline void setIsHeadingSpecified(const NABoolean isHeadingSpec);
  inline void setColumnClass(ComColumnClass columnClass);
  inline void setDirection(const ComColumnDirection direction);

  void setColumnAttribute(ElemDDLNode * pColumnAttribute);

  NAString getColDefAsText() const;

  void setSGOptions(ElemDDLSGOptions * pSGOptions);

  inline void setDivisionColumnFlag(NABoolean bVal);
  inline void setDivisionColumnSequenceNumber(ComSInt32 divColSeqNum);
  inline void setNotNullNondroppableFlag(NABoolean bVal) { isNotNullNondroppable_ = bVal; }
  inline void setNotNullSpecifiedFlag(NABoolean bVal) { isNotNullSpec_ = bVal; }
  inline void setNotNullConstraint(ElemDDLConstraintNotNull * pParseNode) { pConstraintNotNull_ = pParseNode; }

  inline LobsStorage getLobStorage() { return lobStorage_; }
  inline void setLobStorage(LobsStorage s) { lobStorage_ = s; }

  NABoolean isSerializedSpecified() { return isSeabaseSerializedSpec_; }
  inline NABoolean isSeabaseSerialized() { return seabaseSerialized_; }

  NABoolean isColDefaultSpecified() { return isColDefaultSpec_; }

  //
  // methods for tracing
  //
  
  virtual const NAString displayLabel1() const;
  virtual const NAString displayLabel2() const;
  virtual NATraceList getDetailInfo() const;
  virtual const NAString getText() const;


private:

  // ---------------------------------------------------------------------
  // private methods
  // ---------------------------------------------------------------------

  // ---------------------------------------------------------------------
  // private data members
  // ---------------------------------------------------------------------

  NAString columnFamily_;
  NAString columnName_;
  NAType * columnDataType_;

  // DEFAULT
  //
  //   Don't need to use isDefaultSpec_ to check for duplicate
  //   because the grammar disallows duplicate DEFAULT clause.
  //
  defaultClauseStatusType defaultClauseStatus_;
  NABoolean  isNewAdjustedDefaultConstValueNode_;
  ItemExpr * pDefault_; // points to a ConstValue parse node
  NAString   computedDefaultExpr_;
  NAString   defaultExprString_;
  ElemDDLSGOptions * pSGOptions_;
  NAString * pSGLocation_;

  // HEADING
  //
  //   isHeadingSpec_ is used by the parser to check for duplicate
  //   HEADING (either HEADING or NO HEADING) clause.
  //
  //   If one of the following conditions is true:
  //   1. NO HEADING clause is specified.
  //   2. Neither HEADING nor NO HEADING clause is specified.
  //   Then data member heading_ contains an empty string.
  //
  NABoolean isHeadingSpec_;
  NAString heading_;

  // Column class (set to user column by default, this is
  // only changed by internal users)
  ComColumnClass columnClass_;

  // NOT NULL
  //
  //   isNotNullSpec_ is used by the parser to check for
  //   duplicate NOT NULL clause
  //
  NABoolean isNotNullSpec_;
  NABoolean isNotNullNondroppable_;
  ElemDDLConstraintNotNull * pConstraintNotNull_;


  // LOGGABLE

  NABoolean isLoggableSpec_;
  NABoolean isLoggable_;

  // PRIMARY KEY
  //
  NABoolean isPrimaryKeySpec_;
  ElemDDLConstraintPK * pConstraintPK_;
  ElemDDLColRefArray primaryKeyColRefArray_;

  // To provide faster access, define the following array of pointers
  // pointing to the parse nodes derived from class ElemDDLConstraint.
  // These parse nodes are part of the Column Attributes parse sub-tree.
  // Note thate the array does contain neither Not Null nor Primary Key
  // column constraint definition.
  //
  ElemDDLConstraintArray columnConstraintArray_;

  // Data members relate to a internally generated division column.
  NABoolean isDivisionColumn_;
  ComSInt32 divisionColumnSeqNum_;

  // pointers to child parse nodes
  //
  //   Column Attributes list includes column constraint definitions
  //   and column heading specification (HEADING clause).
  //
  ElemDDLNode * children_[MAX_ELEM_DDL_COL_DEF_ARITY];

  ComColumnDirection direction_;  // IN / OUT / INOUT

  // attributes for LOB storage
  NABoolean isLobAttrsSpec_;
  LobsStorage lobStorage_;

  NABoolean isSeabaseSerializedSpec_;
  NABoolean seabaseSerialized_;

  NABoolean isColDefaultSpec_;

  Int32 errCode_;
}; // class ElemDDLColDef

// -----------------------------------------------------------------------
// Column definition elements in stored procedure proxy statements
// -----------------------------------------------------------------------
class ElemProxyColDef : public ElemDDLColDef
{
public:

  ElemProxyColDef(QualifiedName *tableName,
                  NAString &colName,
                  NAType *type,
                  ElemDDLNode *colAttrs,
                  CollHeap *heap);

  ~ElemProxyColDef();

  const QualifiedName *getTableName() const { return tableName_; }

  ElemProxyColDef *castToElemProxyColDef() { return this; }

private:

  QualifiedName *tableName_;

  // Do not implement default constructors or an assignment operator
  ElemProxyColDef();
  ElemProxyColDef(const ElemProxyColDef &);
  ElemProxyColDef &operator=(const ElemProxyColDef &);

}; // class ElemProxyColDef

// -----------------------------------------------------------------------
// definitions of inline methods for class ElemDDLColDef
// -----------------------------------------------------------------------

inline NAType *
ElemDDLColDef::getColumnDataType() const
{
  return columnDataType_;
}

inline const NAString &
ElemDDLColDef::getColumnFamily() const
{
  return columnFamily_;
}

inline const NAString &
ElemDDLColDef::getColumnName() const
{
  return columnName_;
}

//
// Note that columnConstraintArray_ does not include either
// Not Null or Primary Key column constraint definition.
//
inline const ElemDDLConstraintArray &
ElemDDLColDef::getConstraintArray() const
{
  return columnConstraintArray_;
}

inline ElemDDLConstraintArray &
ElemDDLColDef::getConstraintArray()
{
  return columnConstraintArray_;
}

inline ElemDDLConstraintNotNull *
ElemDDLColDef::getConstraintNotNull() const
{
  return pConstraintNotNull_;
}

inline ElemDDLConstraintPK *
ElemDDLColDef::getConstraintPK() const
{
  return pConstraintPK_;
}

inline const ElemDDLColDef::defaultClauseStatusType
ElemDDLColDef::getDefaultClauseStatus() const
{
  return defaultClauseStatus_;
}
  
inline ItemExpr *
ElemDDLColDef::getDefaultValueExpr() const
{
  return pDefault_;
}

inline Int32
ElemDDLColDef::getErrorCode() const
{
  return errCode_;
}

inline const NAString &
ElemDDLColDef::getComputedDefaultExpr() const
{
  return computedDefaultExpr_;
}

inline const NAString &
ElemDDLColDef::getDefaultExprString() const
{
  return defaultExprString_;
}

inline const NAString &
ElemDDLColDef::getHeading() const
{
  return heading_;
}

inline const NABoolean
ElemDDLColDef::getIsConstraintNotNullSpecified() const
{
  return isNotNullSpec_;
}

inline const NABoolean
ElemDDLColDef::getIsConstraintPKSpecified() const
{
  return isPrimaryKeySpec_;
}

inline const NABoolean
ElemDDLColDef::getIsHeadingSpecified() const
{
  return isHeadingSpec_;
}

inline const ComColumnClass
ElemDDLColDef::getColumnClass() const
{
  return columnClass_;
}

inline ElemDDLSGOptions *
ElemDDLColDef::getSGOptions() const
{
  return pSGOptions_;
}

inline NAString *
ElemDDLColDef::getSGLocation() const
{
  return pSGLocation_;
}

//
// get list of column names specified in PRIMARY KEY clause
//
inline const ElemDDLColRefArray &
ElemDDLColDef::getPrimaryKeyColRefArray() const
{
  return primaryKeyColRefArray_;
}

inline ElemDDLColRefArray &
ElemDDLColDef::getPrimaryKeyColRefArray()
{
  return primaryKeyColRefArray_;
}

inline const NABoolean
ElemDDLColDef::isHeadingSpecified() const
{
  return getIsHeadingSpecified();
}

inline const NABoolean
ElemDDLColDef::isNotNullConstraintSpecified() const
{
  return getIsConstraintNotNullSpecified();
}

inline const NABoolean
ElemDDLColDef::isNotNullNondroppable() const
{
  return isNotNullNondroppable_;
}


inline const NABoolean 
ElemDDLColDef::getIsLoggableSpecified() const
{
  return isLoggableSpec_;
}

inline const NABoolean 
ElemDDLColDef::getIsLoggable() const
{
  return isLoggable_;
}


inline const NABoolean
ElemDDLColDef::isPrimaryKeyConstraintSpecified() const
{
  return getIsConstraintPKSpecified();
}

inline const ComColumnDirection
ElemDDLColDef::getDirection() const
{
  return direction_;
}

inline NABoolean ElemDDLColDef::isDivisionColumn() const
{
  return isDivisionColumn_;
}

inline ComSInt32 ElemDDLColDef::getDivisionColumnSequenceNumber() const
{
  return divisionColumnSeqNum_;
}

//
// mutators
//

inline void
ElemDDLColDef::setHeading(const NAString &heading)
{
  heading_ = heading;
}

inline void
ElemDDLColDef::setIsHeadingSpecified(const NABoolean isHeadingSpec)
{
  isHeadingSpec_ = isHeadingSpec;
}

inline void
ElemDDLColDef::setColumnClass(ComColumnClass columnClass)
{
  columnClass_ = columnClass;
}

inline void 
ElemDDLColDef::setDirection(const ComColumnDirection direction)
{
  direction_ = direction;
}

inline void ElemDDLColDef::setDivisionColumnFlag(NABoolean bVal)
{
  isDivisionColumn_ = bVal;
}

inline void ElemDDLColDef::setDivisionColumnSequenceNumber(ComSInt32 divColSeqNum)
{
  divisionColumnSeqNum_ = divColSeqNum;
}

#endif // ELEMDDLCOLDEF_H

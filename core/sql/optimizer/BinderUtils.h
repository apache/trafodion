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
#ifndef BINDER_UTILS_H
#define BINDER_UTILS_H

/* -*-C++-*-
******************************************************************************
*
* File:         BinderUtils.h
* Description:  Binder utility functions that are generic and can be reused.
*
* Created:      09/22/2000
* Language:     C++
* Status:       $State: Exp $
*
*
******************************************************************************
*/


class ColRefName;
class ItemExpr;
class QualifiedName;
class IntegerList;
class NATable;
class RenameCol;
class CorrName;
class BindWA;
class ColReference;

#include "NABoolean.h"
#include "OperTypeEnum.h"

#include "Collections.h"
#include "CmpCommon.h"
#include "ItemExprList.h"

// specialFlags parameter for buildClusteringIndexVector()
enum ciSpecialFlags 
  { SP_NONE		      = 0x0000,
    SP_ALL_COLUMNS	      = 0x0001,   // Not only the CI columns.
    SP_USE_NULL		      = 0x0002,   // Return Null for that column
    SP_USE_LAST_NOT_NULL      = 0x0004,   // Use LAST_NOT_NULL sequence function.
    SP_USE_OFFSET	      = 0x0008,   // Use OFFSET(1) sequence function.
    SP_SYSKEY_AS_USER	      = 0x0010,   // Don't force SYSKEY as a system column.
    SP_USE_AT_SYSKEY	      = 0x0020,   // Use @SYSKEY instead of SYSKEY,
    SP_SKIP_EPOCH	      = 0x0040,	  // Skip @EPOCH column  
    SP_RENAME_AT_SYSKEY       = 0x0080,   // Rename @SYSKEY to SYSKEY
    SP_SKIP_IUD_CONTROL_COLS  = 0x0100    // Skip the IUD log control columns
  };

typedef LIST(const NAString *) ConstStringList;

//----------------------------------------------------------------------------
class BinderUtils
{
public:
  
  // get the ColRefName from a item that is either ITM_REFERENCE or 
  // ITM_RENAME_COL.
  static ColRefName getColRefName(ItemExpr* pItemExpr);


  // set the LIST to an itemExpr of ItemList tree
  static ItemExpr * 
  setItemExprFromList(const LIST(ItemExpr* )&  list, 
		      CollHeap                *heap,
		      ItemExprShapeEnum        treeShape = LEFT_LINEAR_TREE);

  // Build a dirction vector for the clustering index vector of the table.
  static IntegerList *
  buildClusteringIndexDirectionVector(const NATable *naTable,
				      CollHeap      *heap);


  // if we have renames on top of renames - this will return the ItemExpr 
  // below them. Example :in case we have a MAX(A_NAME) as max_a and the 
  // A_NAME is actually lets say a heading in the base table - we need to
  // peel of all the Renames
  static const ItemExpr *peelOffAllRenames(const ItemExpr *pColExpr);

  // make the names into column Items. the Item type can be ITM_REFERENCE or 
  // ITM_RENAME_COL. If the table name is null there will be no table name
  // in the ColRefName of the column.
  static void appendToExprList(ItemExprList&            toAddto, 
			       const ConstStringList&   columnNames,
			       CollHeap                *heap,
			       OperatorTypeEnum         itemType,
			       const CorrName&          tableName);
  
  static ItemExpr * getNamesListAsItemExpr(
				   const ConstStringList& nameList,
				   CollHeap * heap,
				   OperatorTypeEnum itemType,
				   const CorrName& tableName);

  static ItemExpr *buildPredOnCol(OperatorTypeEnum opType,
				  const NAString& colName,
				  Int32 constVal,
				  CollHeap *heap);

  static ItemExpr *buildClusteringIndexVector(const NATable  *naTable, 
					      CollHeap       *heap,
					      const CorrName *nameOverride = NULL,
					      Lng32            specialFlags = 0,
					      IntegerList    *directionVector = NULL,
					      const NAString *prefixColName = NULL,
					      const NAString *prefixRenameColName = NULL);

private:
  // Called by buildClusteringIndexVector().
  static ColReference *buildExpressionForSyskey(const CorrName *tableNameCorr,
					    CollHeap       *heap,
					    Lng32            specialFlags,
					    const NAString *prefixColName);


}; // class BinderUtils

//----------------------------------------------------------------------------
class IntegerList : public LIST(Lng32)
{
public:
  IntegerList(CollHeap* h=CmpCommon::statementHeap()) 
  : LIST(Lng32)(h) 
  {}

  // Copy Ctor.
  IntegerList(const IntegerList &other, CollHeap* h=CmpCommon::statementHeap()) 
  : LIST(Lng32)(other, h) 
  {}

  virtual ~IntegerList() {};
};

//----------------------------------------------------------------------------
class QualNamePtrList : public LIST(QualifiedName *)
{
public:
  QualNamePtrList(CollHeap* h=CmpCommon::statementHeap()) 
  : LIST(QualifiedName *)(h) 
  {}

  virtual ~QualNamePtrList() {}

  NABoolean containsName(const QualifiedName& name) const;
};

#endif // BINDER_UTILS_H

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
/* -*-C++-*-
******************************************************************************
*
* File:         BinderUtils.cpp
* Description:  all kind of general functionalities.
*
* Created:      09/22/2000
* Language:     C++
* Status:       $State: Exp $
*
*
******************************************************************************
*/


#include "BinderUtils.h"

#include "AllItemExpr.h"
#include "ItemOther.h"
#include "ItemColRef.h"
#include "NATable.h"
#include "ComMvDefs.h"
#include "BindWA.h"

//----------------------------------------------------------------------------
// extract the name from a reference or a rename
ColRefName BinderUtils::getColRefName(ItemExpr* pItemExpr)
{
  if(ITM_REFERENCE == pItemExpr->getOperatorType())
  {
    return ((ColReference*)pItemExpr)->getColRefNameObj();
  }

  if(ITM_RENAME_COL == pItemExpr->getOperatorType())
  {
    return *(ColRefName*)(((RenameCol*)pItemExpr)->getNewColRefName());
  }

  CMPASSERT(FALSE);

  return ColRefName();
}

//----------------------------------------------------------------------------
// set the LIST to an ItemExpr or ItemList tree
ItemExpr * BinderUtils::setItemExprFromList(const LIST(ItemExpr* ) & list, 
				      CollHeap *heap,
				      ItemExprShapeEnum treeShape)
{
  if( ! (list.entries() > 0 ) )
  {
    return NULL;
  }

  ItemExpr * result = list[0];

  for (CollIndex i(1) ; i < list.entries() ; i++)
  {
    ItemExpr * pLeftSun = NULL;
    ItemExpr * pRightSun = NULL;
  
    switch(treeShape)
    {
    case LEFT_LINEAR_TREE:
      {
      
	pLeftSun = result;
	pRightSun = list[i];
  
      }
      break;
    case RIGHT_LINEAR_TREE:
      {
	pLeftSun = list[i]; 
	pRightSun = result;
      }
      break;
    case BUSHY_TREE:
    default:
      CMPASSERT(FALSE);
      break;
    };
  
    result = new(heap)ItemList(pLeftSun, pRightSun);
  } // for  
  

  return result;
}


//----------------------------------------------------------------------------
// Build a direction vector for the columns of the clustering index of the
// base table. The direction vector is a list of integers: 1 for ascending 
// columns, and -1 for descending columns.

IntegerList *
BinderUtils::buildClusteringIndexDirectionVector(const NATable *naTable,
						 CollHeap      *heap)
{
  ExtendedQualName::SpecialTableType tableType = 
    naTable->getExtendedQualName().getSpecialType();
  CMPASSERT(tableType != ExtendedQualName::RANGE_LOG_TABLE);

  IntegerList *result = new(heap) IntegerList(heap);
  const NAColumnArray &indexColumns = 
    naTable->getClusteringIndex()->getIndexKeyColumns();

  CollIndex firstCol = 0;
  CollIndex lastCol  = indexColumns.entries()-1;
  // If the NATable is of the IUD log, skip the first and last columns.
  if (tableType == ExtendedQualName::IUD_LOG_TABLE ||
      tableType == ExtendedQualName::GHOST_IUD_LOG_TABLE)
  {
    // Skip the first CI column: @EPOCH.
    CMPASSERT(indexColumns[0]->getColName() == COMMV_EPOCH_COL);
    firstCol++;

    // Skip the last column of the IUD log: the log SYSKEY.
    CMPASSERT(indexColumns[lastCol]->getColName() == "SYSKEY");
    lastCol--;
  }

  for (CollIndex i=firstCol; i<=lastCol; i++)
  {
    if (indexColumns[i]->getClusteringKeyOrdering() == DESCENDING)
      result->insert(-1);
    else
      result->insert(1);
  }

  return result;
} // BinderUtils::buildClusteringIndexDirectionVector


//----------------------------------------------------------------------------
// if we have renames on top of renames - this will return the ItemExpr below
// them. Example :in case we have a MAX(A_NAME) as max_a and the A_NAME is 
// actually lets say a heading in the base table - we need to peel of all the 
// Renames
const ItemExpr *BinderUtils::peelOffAllRenames(const ItemExpr *pColExpr)
{
  const ItemExpr *pResult = pColExpr;
  while (ITM_RENAME_COL == pResult->getOperatorType())
  {
    pResult = pResult->child(0);
  }

  return pResult;    
} // MavBuilder::peelOfAllRenames

//----------------------------------------------------------------------------
// make the names into column Items.
void BinderUtils::appendToExprList(ItemExprList&            toAddto, 
				   const ConstStringList&   columnNames,
				   CollHeap                *heap,
				   OperatorTypeEnum         itemType,
				   const CorrName&          tableName) 
{
  for (CollIndex i(0) ; i < columnNames.entries() ; i++)
  {
    ColRefName *pColRefName = new(heap) 
      ColRefName(*(columnNames[i]), tableName, heap);
    ItemExpr *pInsertValue = NULL;

    switch(itemType)
    {
      case ITM_REFERENCE:
      {
	ColReference * pColRef = new(heap) ColReference(pColRefName);
	pInsertValue = pColRef;
      }
      break;

      case ITM_RENAME_COL:
      {
	RenameCol * pRenameCol = new(heap) RenameCol(NULL, pColRefName);
	pInsertValue = pRenameCol;
      }
      break;

      default:
	CMPASSERT(FALSE);
    }
    toAddto.insert(pInsertValue);
  }
} // BinderUtils::addToListAllColNamesAsItemExpr

//----------------------------------------------------------------------------
ItemExpr * BinderUtils::getNamesListAsItemExpr(
					   const ConstStringList & nameList,
					   CollHeap * heap,
					   OperatorTypeEnum itemType,
					   const CorrName& tableName)
{

  ItemExprList resultList(heap);

  BinderUtils::appendToExprList(resultList,
				nameList,
				heap,
				itemType,
				tableName);

  return BinderUtils::setItemExprFromList(resultList, heap);
} // BinderUtils::getNamesListAsItemExpr


//----------------------------------------------------------------------------
ItemExpr *BinderUtils::buildPredOnCol(OperatorTypeEnum opType,
				      const NAString& colName,
				      Int32 constVal,
				      CollHeap *heap)
{
  return new(heap) 
    BiRelat(opType,  
	    new(heap) ColReference(new(heap) 
			ColRefName(colName, heap)),
	    new(heap) ConstValue(constVal));
}

//----------------------------------------------------------------------------
ColReference *BinderUtils::buildExpressionForSyskey(const CorrName *tableNameCorr,
						CollHeap       *heap,
						Lng32            specialFlags,
						const NAString *prefixColName)
{
  NABoolean forceSystemColumn = 
    ( (specialFlags & SP_ALL_COLUMNS) &&
     !(specialFlags & SP_SYSKEY_AS_USER));
 
  char syskeyName[] = "SYSKEY";
  NAString colName;

  if (prefixColName)
  {
    colName = *prefixColName + syskeyName;
  }
  else
  {
    colName = (specialFlags & SP_USE_AT_SYSKEY) ? 
              COMMV_BASE_SYSKEY_COL             : 
              syskeyName;
  }

  ColReference *colRef = new(heap) 
    ColReference(new(heap) 
      ColRefName(colName, 
                 *tableNameCorr,
		 heap));

  if (forceSystemColumn)
    colRef->setTargetColumnClass(SYSTEM_COLUMN);

  return colRef;
}  // buildExpressionForSyskey()

//----------------------------------------------------------------------------
// Build a list of ColReferences to the columns of the base table's 
// clustering index columns. 
ItemExpr *BinderUtils::buildClusteringIndexVector(const NATable  *naTable, 
						  CollHeap       *heap,
						  const CorrName *nameOverride,
						  Lng32            specialFlags,
						  IntegerList    *directionVector,
						  const NAString *prefixColName,
						  const NAString *prefixRenameColName)
{
  ItemExpr *result = NULL;
  ColRefName *colNameRef=NULL;

  const NAColumnArray &indexColumns = 
    (specialFlags & SP_ALL_COLUMNS) ?
    naTable->getNAColumnArray()      :
    naTable->getClusteringIndex()->getIndexKeyColumns();

  const CorrName *tableNameCorr = NULL;
  if (nameOverride != NULL)
  {
    tableNameCorr = nameOverride; 
  }
  else
  {
    const QualifiedName& qualName = naTable->getTableName();
    NAString schemaName(qualName.getSchemaName());
    NAString catalogName(qualName.getCatalogName());
    NAString tableName(naTable->getTableName().getObjectName());
    tableNameCorr = new(heap) CorrName(tableName, heap, schemaName, catalogName);
  }

  for (CollIndex i=0; i<indexColumns.entries(); i++)
  {
    NAString colName = indexColumns[i]->getColName();

    if (prefixColName != NULL)
	colName = *prefixColName + colName;

    ItemExpr *colExpr = NULL;    

    // If this is the SYSKEY or @SYSKEY column and appropriate flags are set
    if ((colName == "SYSKEY" 
         ||
          (COMMV_SYSKEY_COL == colName && (specialFlags & SP_USE_AT_SYSKEY)))
        && 
        !(specialFlags & SP_USE_NULL))
    {
      ColReference *colRef = NULL;
      colExpr = colRef = buildExpressionForSyskey(tableNameCorr, heap, specialFlags, prefixColName);

      if (specialFlags & SP_RENAME_AT_SYSKEY)
      {
	// Rename @SYSKEY to SYSKEY
	colNameRef = new(heap) ColRefName("SYSKEY", *tableNameCorr, heap);
      }
      else
      {
        colNameRef = &(colRef->getColRefNameObj());
      }
    }
    else
    {
      if ( (specialFlags & SP_SKIP_EPOCH) && (COMMV_EPOCH_COL == colName))
      {
        continue;
      }

      if( (specialFlags & SP_SKIP_IUD_CONTROL_COLS) 
           && 
          ( COMMV_EPOCH_COL == colName
            ||
            COMMV_OPTYPE_COL == colName
            ||
            COMMV_IGNORE_COL == colName
            ||
            COMMV_BITMAP_COL == colName
            ||
            COMMV_RANGE_SIZE_COL == colName
            ||         
            COMMV_TS_COL == colName )           
         )
      {
	continue;
      }

      colNameRef = new(heap) ColRefName(colName, *tableNameCorr, heap);

      if (specialFlags & SP_USE_NULL)
      {
	colExpr =  new(heap) RenameCol(new(heap) ConstValue(), colNameRef);
      }
      else
      {
	colExpr = new(heap) ColReference(colNameRef);
      }
    }

    if (specialFlags & SP_USE_OFFSET)
      colExpr = new(heap) ItmSeqOffset( colExpr, 1);

    if (specialFlags & SP_USE_LAST_NOT_NULL)
      colExpr = new(heap) ItmSeqRunningFunction (ITM_LAST_NOT_NULL, colExpr);

    if (directionVector!=NULL && (*directionVector)[i]==-1 && (!(specialFlags & SP_USE_NULL)) )
      colExpr = new(heap) InverseOrder(colExpr);

    if (prefixRenameColName != NULL)
    {
      colName = *prefixRenameColName + colName;

      colNameRef = new(heap) ColRefName(colName, *tableNameCorr, heap);
    }
    
    colExpr = new(heap) RenameCol(colExpr, colNameRef);

    if (result == NULL)
      result = colExpr;
    else
      result = new(heap) ItemList(result, colExpr);
  }

  if (nameOverride == NULL)
    delete tableNameCorr;

  return result;
}  // buildClusteringIndexVector()


//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
//  QualNamePtrList
//----------------------------------------------------------------------------
//----------------------------------------------------------------------------


//----------------------------------------------------------------------------
// Does this list of names contain 'name'?
NABoolean QualNamePtrList::containsName(const QualifiedName& name) const
{
  for (CollIndex i=0; i<entries(); i++)
  {
    if (*at(i) == name)
      return TRUE;
  }
  return FALSE;
}



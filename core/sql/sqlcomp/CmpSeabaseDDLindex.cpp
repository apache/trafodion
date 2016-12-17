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
 *****************************************************************************
 *
 * File:         CmpSeabaseDDLindex.cpp
 * Description:  Implements ddl operations for Seabase indexes.
 *
 *
 * Created:     6/30/2013
 * Language:     C++
 *
 *
 *****************************************************************************
 */

#define   SQLPARSERGLOBALS_FLAGS	// must precede all #include's
#define   SQLPARSERGLOBALS_NADEFAULTS

#include "ComObjectName.h"

#include "StmtDDLCreateIndex.h"
#include "StmtDDLPopulateIndex.h"
#include "StmtDDLDropIndex.h"
#include "StmtDDLAlterTableEnableIndex.h"
#include "StmtDDLAlterTableDisableIndex.h"
#include "StmtDDLAlterIndexHBaseOptions.h"

#include "CmpDDLCatErrorCodes.h"
#include "ElemDDLHbaseOptions.h"

#include "SchemaDB.h"
#include "CmpSeabaseDDL.h"
#include "CmpDescribe.h"

#include "ExpHbaseInterface.h"

#include "ExExeUtilCli.h"
#include "Generator.h"

#include "ComCextdecs.h"
#include "ComUser.h"

#include "NumericType.h"

#include "PrivMgrCommands.h"

short 
CmpSeabaseDDL::createIndexColAndKeyInfoArrays(
     ElemDDLColRefArray &indexColRefArray,
     NABoolean isUnique,
     NABoolean hasSyskey,
     NABoolean alignedFormat,
     NAString &defaultColFam,
     const NAColumnArray &baseTableNAColArray,
     const NAColumnArray &baseTableKeyArr,
     Lng32 &keyColCount,
     Lng32 &nonKeyColCount,
     Lng32 &totalColCount,
     ComTdbVirtTableColumnInfo * &colInfoArray,
     ComTdbVirtTableKeyInfo * &keyInfoArray,
     NAList<NAString> &selColList,
     Lng32 &keyLength,
     NAMemory * heap)
{
  Lng32 retcode = 0;
  keyLength = 0;

  keyColCount = indexColRefArray.entries();
  nonKeyColCount = 0;

  Lng32 baseTableKeyCount = baseTableKeyArr.entries();

  

  if (isUnique)
    nonKeyColCount = baseTableKeyCount;
  else
    keyColCount += baseTableKeyCount;

  totalColCount = keyColCount + nonKeyColCount;

  colInfoArray = new(heap) ComTdbVirtTableColumnInfo[totalColCount];

  keyInfoArray = new(heap) ComTdbVirtTableKeyInfo[totalColCount];

  CollIndex i = 0;
  NABoolean syskeyOnly = TRUE;
  NABoolean syskeySpecified = FALSE;
  NABoolean incorrectSyskeyPos = FALSE;
  for ( i = 0; i < indexColRefArray.entries(); i++ )
    {
      ElemDDLColRef *nodeKeyCol = indexColRefArray[i];
      
     const NAColumn *tableCol =
       baseTableNAColArray.getColumn(nodeKeyCol->getColumnName());
     if (tableCol == NULL)
       {
	 *CmpCommon::diags() << DgSqlCode(-1009) //CAT_COLUMN_DOES_NOT_EXIST_ERR
			     << DgColumnName(ToAnsiIdentifier(nodeKeyCol->getColumnName()));
	 
	 return -1;
       }

     if (strcmp(nodeKeyCol->getColumnName(), "SYSKEY") != 0)
       syskeyOnly = FALSE;
     else
       {
	 syskeySpecified = TRUE;
	 if (i < (indexColRefArray.entries() - 1))
	   incorrectSyskeyPos = TRUE;
       }

     // update column info for the index
     char * col_name = new(heap) char[strlen(nodeKeyCol->getColumnName()) + 2 + 1];
     strcpy(col_name, nodeKeyCol->getColumnName());
     strcat(col_name, "@");
     if (baseTableKeyArr.getColumn(col_name))
       {
	 strcat(col_name, "@");
       }
     colInfoArray[i].colName = col_name;

     selColList.insert(nodeKeyCol->getColumnName());

     colInfoArray[i].colNumber = i; 

     colInfoArray[i].columnClass = COM_USER_COLUMN;

     const NAType * naType = tableCol->getType();
     if ((naType->getFSDatatype() == REC_BLOB) || (naType->getFSDatatype() == REC_CLOB))
     {
      *CmpCommon::diags() << DgSqlCode(-CAT_LOB_COL_CANNOT_BE_INDEX_OR_KEY)
                              << DgColumnName(col_name);
      processReturn();
      return -1;
     }
     Lng32 precision = 0;
     Lng32 scale = 0;
     Lng32 dtStart = 0; 
     Lng32 dtEnd = 0;
     Lng32 upshifted = 0;
     NAString charsetStr;
     SQLCHARSET_CODE charset = SQLCHARSETCODE_UNKNOWN;
     CharInfo::Collation collationSequence = CharInfo::DefaultCollation;
     ULng32 colFlags = 0;
     retcode = getTypeInfo(naType, FALSE, FALSE,
			   colInfoArray[i].datatype, colInfoArray[i].length, 
			   precision, scale, dtStart, dtEnd, upshifted, 
			   colInfoArray[i].nullable,
			   charsetStr, collationSequence, colFlags);
      if (retcode)
	{
	  if  (collationSequence != CharInfo::DefaultCollation)
	    {
	      // collation not supported
	      *CmpCommon::diags() << DgSqlCode(-4069)
				  << DgColumnName(ToAnsiIdentifier(col_name))
				  << DgString0(CharInfo::getCollationName(collationSequence));
	    }

	  return -1;
	}

     colInfoArray[i].precision = precision;
     colInfoArray[i].scale = scale;
     colInfoArray[i].dtStart = dtStart;
     colInfoArray[i].dtEnd = dtEnd;
     colInfoArray[i].upshifted = upshifted;
     colInfoArray[i].charset = 
       (SQLCHARSET_CODE)CharInfo::getCharSetEnum(charsetStr.data());
     
     colInfoArray[i].defaultClass = COM_NO_DEFAULT;
     colInfoArray[i].defVal = NULL;

     colInfoArray[i].colHeading = NULL;
     if (tableCol->getHeading())
       {
	 char * h = (char*)tableCol->getHeading();
	 Lng32 hlen = strlen(h);
	 char * head_val = new(heap) char[hlen+1];
	 strcpy(head_val, h); 
	 colInfoArray[i].colHeading = head_val;
       }

     colInfoArray[i].hbaseColFam = 
       new(heap) char[strlen(defaultColFam.data()) +1];
     strcpy((char*)colInfoArray[i].hbaseColFam, defaultColFam.data());

      char idxNumStr[40];
      idxNumStr[0] = '@';
      str_itoa(i+1, &idxNumStr[1]);

      colInfoArray[i].hbaseColQual =
	new(heap) char[strlen(idxNumStr) + 1];
      strcpy((char*)colInfoArray[i].hbaseColQual, idxNumStr);

      colInfoArray[i].hbaseColFlags = tableCol->getHbaseColFlags();
      strcpy(colInfoArray[i].paramDirection, COM_UNKNOWN_PARAM_DIRECTION_LIT);
      colInfoArray[i].isOptional = FALSE;
 
      keyLength += naType->getEncodedKeyLength();
      // update key info
      keyInfoArray[i].colName = col_name; 
      keyInfoArray[i].keySeqNum = i+1;
      keyInfoArray[i].tableColNum = tableCol->getPosition();
      keyInfoArray[i].ordering = 
        (nodeKeyCol->getColumnOrdering() == COM_ASCENDING_ORDER ? 0 : 1);

     keyInfoArray[i].nonKeyCol = 0;

     keyInfoArray[i].hbaseColFam = new(heap) char[strlen(defaultColFam.data()) + 1];
     strcpy((char*)keyInfoArray[i].hbaseColFam, defaultColFam.data());
     
     char qualNumStr[40];
     str_sprintf(qualNumStr, "@%d", keyInfoArray[i].keySeqNum);
     
     keyInfoArray[i].hbaseColQual = new(CTXTHEAP) char[strlen(qualNumStr)+1];
     strcpy((char*)keyInfoArray[i].hbaseColQual, qualNumStr);
    }
  
  if ((syskeyOnly) &&
      (hasSyskey))
    {
      *CmpCommon::diags() << DgSqlCode(-1112);

      return -1;
    }

  if ((syskeySpecified && incorrectSyskeyPos) &&
      (hasSyskey))
    {
      *CmpCommon::diags() << DgSqlCode(-1089);

      return -1;
    }

  // add base table primary key info
  CollIndex j = 0;
  NABoolean duplicateColFound = FALSE;
  while (i < totalColCount)
    {
      const NAColumn * keyCol = baseTableKeyArr[j];
      
      // If an index is being created on a subset of the base table's key
      // columns, then those columns have already been added in the loop above
      // We will skip them here, so that the index does not have the same 
      // column twice.
      duplicateColFound = FALSE;
      for (int k = 0; 
           (k < indexColRefArray.entries() && !duplicateColFound); k++)
        {
          if (keyInfoArray[k].tableColNum == keyCol->getPosition()) 
            {
              duplicateColFound = TRUE;
              totalColCount-- ;
              if (isUnique)
                nonKeyColCount-- ;
              else
                keyColCount-- ;
              j++;   
            }         
        }
      if (duplicateColFound)
        continue ; // do not add this col here since it has already been added
          

      // update column info for the index
      char * col_name = new(heap) char[strlen(keyCol->getColName().data()) + 2 + 1];
      strcpy(col_name, keyCol->getColName().data());
      colInfoArray[i].colName = col_name;

      colInfoArray[i].colNumber = i; 
      colInfoArray[i].columnClass = COM_USER_COLUMN;

      selColList.insert(keyCol->getColName());

      const NAType * naType = keyCol->getType();
      
      Lng32 precision = 0;
      Lng32 scale = 0;
      Lng32 dtStart = 0; 
      Lng32 dtEnd = 0;
      Lng32 upshifted = 0;
      NAString charsetStr;
      SQLCHARSET_CODE charset = SQLCHARSETCODE_UNKNOWN;
      CharInfo::Collation collationSequence = CharInfo::DefaultCollation;
      ULng32 colFlags = 0;
      
      retcode = getTypeInfo(naType, alignedFormat, FALSE,
			    colInfoArray[i].datatype, colInfoArray[i].length, 
			    precision, scale, dtStart, dtEnd, upshifted, 
			    colInfoArray[i].nullable,
			    charsetStr, collationSequence, colFlags);
      if (retcode)
	{
	  if  (collationSequence != CharInfo::DefaultCollation)
	    {
	      // collation not supported
	      *CmpCommon::diags() << DgSqlCode(-4069)
				  << DgColumnName(ToAnsiIdentifier(col_name))
				  << DgString0(CharInfo::getCollationName(collationSequence));
	    }

	  return -1;
	}

      colInfoArray[i].precision = precision;
      colInfoArray[i].scale = scale;
      colInfoArray[i].dtStart = dtStart;
      colInfoArray[i].dtEnd = dtEnd;
      colInfoArray[i].upshifted = upshifted;
      colInfoArray[i].charset = 
        (SQLCHARSET_CODE)CharInfo::getCharSetEnum(charsetStr.data());;
      colInfoArray[i].defaultClass = COM_NO_DEFAULT;
      colInfoArray[i].defVal = NULL;
      colInfoArray[i].colHeading = NULL;
      if (keyCol->getHeading())
	{
	  char * h = (char*)keyCol->getHeading();
	  Lng32 hlen = strlen(h);
	  char * head_val = new(heap) char[hlen+1];
	  strcpy(head_val, h); 
	  colInfoArray[i].colHeading = head_val;
	}
      
     colInfoArray[i].hbaseColFam = 
       new(heap) char[strlen(defaultColFam.data()) +1];
     strcpy((char*)colInfoArray[i].hbaseColFam, defaultColFam.data());
     
      char idxNumStr[40];
      idxNumStr[0] = '@';
      str_itoa(i+1, &idxNumStr[1]);

      colInfoArray[i].hbaseColQual =
	new(heap) char[strlen(idxNumStr) + 1];
      strcpy((char*)colInfoArray[i].hbaseColQual, idxNumStr);

      colInfoArray[i].hbaseColFlags = keyCol->getHbaseColFlags();
      strcpy(colInfoArray[i].paramDirection, COM_UNKNOWN_PARAM_DIRECTION_LIT);
      colInfoArray[i].isOptional = FALSE;
 
      // add base table keys for non-unique index
      keyInfoArray[i].colName = col_name;
      keyInfoArray[i].keySeqNum = i+1;
      keyInfoArray[i].tableColNum = keyCol->getPosition();
      keyInfoArray[i].ordering = 
	(keyCol->getClusteringKeyOrdering() == ASCENDING ? 0 : 1);
      
      if (isUnique)
	keyInfoArray[i].nonKeyCol = 1;
      else {
	keyInfoArray[i].nonKeyCol = 0;
        keyLength += naType->getEncodedKeyLength();
      }

      keyInfoArray[i].hbaseColFam = new(heap) char[strlen(defaultColFam.data()) + 1];
      strcpy((char*)keyInfoArray[i].hbaseColFam, defaultColFam);
      
      char qualNumStr[40];
      str_sprintf(qualNumStr, "@%d", keyInfoArray[i].keySeqNum);
      
      keyInfoArray[i].hbaseColQual = new(CTXTHEAP) char[strlen(qualNumStr)+1];
      strcpy((char*)keyInfoArray[i].hbaseColQual, qualNumStr);
      
      j++;
      i++;
    }

  return 0;
}

void CmpSeabaseDDL::createSeabaseIndex( StmtDDLCreateIndex * createIndexNode,
				       NAString &currCatName, NAString &currSchName)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  ComObjectName tableName(createIndexNode->getTableName());
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);
  NAString btCatalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  NAString btSchemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  NAString btObjectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  NAString extTableName = tableName.getExternalName(TRUE);
  NAString extTableNameForHbase = 
    btCatalogNamePart + "." + btSchemaNamePart + "." + btObjectNamePart;
  NAString tabName = (NAString&)createIndexNode->getTableName();

  NABoolean schNameSpecified = 
    (NOT createIndexNode->getOrigTableNameAsQualifiedName().getSchemaName().isNull());

  ComObjectName indexName(createIndexNode->getIndexName());
  indexName.applyDefaults(btCatalogNamePart, btSchemaNamePart); 

  NAString catalogNamePart = indexName.getCatalogNamePartAsAnsiString();
  NAString schemaNamePart = indexName.getSchemaNamePartAsAnsiString(TRUE);
  NAString objectNamePart = indexName.getObjectNamePartAsAnsiString(TRUE);
  NAString extIndexName = indexName.getExternalName(TRUE);
  NAString extNameForHbase = catalogNamePart + "." + schemaNamePart + "." + objectNamePart;
  NABoolean alignedFormatNotAllowed = FALSE; 
  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    return;

  if (((isSeabaseReservedSchema(indexName)) ||
       (ComIsTrafodionExternalSchemaName(schemaNamePart))) &&
      (!Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
    {
      *CmpCommon::diags() << DgSqlCode(-1118)
			  << DgTableName(extIndexName);
      deallocEHI(ehi); 
      return;
    }

  ExeCliInterface cliInterface(STMTHEAP, NULL, NULL, 
       CmpCommon::context()->sqlSession()->getParentQid());
  NABoolean isVolatileTable = FALSE;
  ComObjectName volTabName ;

  if ((NOT createIndexNode->isVolatile()) &&
       (CmpCommon::context()->sqlSession()->volatileSchemaInUse()))
     {
       QualifiedName *qn =
         CmpCommon::context()->sqlSession()->updateVolatileQualifiedName(
              createIndexNode->getOrigTableNameAsQualifiedName().getObjectName());

       if (qn == NULL)
         {
	   *CmpCommon::diags()
	     << DgSqlCode(-CAT_UNABLE_TO_DROP_OBJECT)
	     << DgTableName(createIndexNode->getOrigTableNameAsQualifiedName().
			    getQualifiedNameAsAnsiString(TRUE));

	   processReturn();

	   return;
         }

       volTabName = qn->getQualifiedNameAsAnsiString() ;
	volTabName.applyDefaults(currCatAnsiName, currSchAnsiName);

	NAString vtCatNamePart = volTabName.getCatalogNamePartAsAnsiString();
	NAString vtSchNamePart = volTabName.getSchemaNamePartAsAnsiString(TRUE);
	NAString vtObjNamePart = volTabName.getObjectNamePartAsAnsiString(TRUE);
	
	retcode = existsInSeabaseMDTable(&cliInterface, 
					 vtCatNamePart, vtSchNamePart, vtObjNamePart,
					 COM_BASE_TABLE_OBJECT);

	if (retcode < 0)
	  {
	    processReturn();

	    return;
	  }

       if (retcode == 1)
          {
            // table found in volatile schema
            // Validate volatile table name.
            if (CmpCommon::context()->sqlSession()->
                validateVolatileQualifiedName
                (createIndexNode->getOrigTableNameAsQualifiedName()))
              {
                // Valid volatile table. Create index on it.
                extTableName = volTabName.getExternalName(TRUE);
                isVolatileTable = TRUE;

		btCatalogNamePart = vtCatNamePart;
		btSchemaNamePart = vtSchNamePart;
              }
            else
              {
                // volatile table found but the name is not a valid
                // volatile name. Look for the input name in the regular
                // schema.
                // But first clear the diags area.
                CmpCommon::diags()->clear();
              }
          }
	else
	  {
	    CmpCommon::diags()->clear();
	  }

     }

  retcode = lookForTableInMD(&cliInterface, 
                             btCatalogNamePart, btSchemaNamePart, btObjectNamePart,
                             schNameSpecified, FALSE,
                             tableName, tabName, extTableName);
  if (retcode < 0)
    {
      processReturn();

      return;
    }

  ActiveSchemaDB()->getNATableDB()->useCache();

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  CorrName cn(tableName.getObjectNamePart().getInternalName(),
	      STMTHEAP,
	      btSchemaNamePart,
	      btCatalogNamePart);

  NATable *naTable = bindWA.getNATableInternal(cn); 

  if (naTable == NULL || bindWA.errStatus())
    {
      CmpCommon::diags()->clear();

      if (createIndexNode->isVolatile())
	*CmpCommon::diags()
	  << DgSqlCode(-4082)
	  << DgTableName(tableName.getObjectNamePart().getInternalName());
      else
	{
	  *CmpCommon::diags()
	    << DgSqlCode(-4082)
	    << DgTableName(cn.getExposedNameAsAnsiString());
	}

      deallocEHI(ehi); 

      processReturn();

      return;
    }

  Int64 btObjUID = naTable->objectUid().castToInt64();

  if (naTable->isHbaseMapTable())
    {
      // not supported
      *CmpCommon::diags() << DgSqlCode(-3242)
                          << DgString0("Cannot create index on an HBase mapped table.");

      deallocEHI(ehi); 
      processReturn();
      
      return;
    }

  NAString &indexColFam = naTable->defaultColFam();
  NAString trafColFam;
  if (indexColFam != SEABASE_DEFAULT_COL_FAMILY)
    {
      CollIndex idx = naTable->allColFams().index(indexColFam);
      genTrafColFam(idx, trafColFam);
      alignedFormatNotAllowed = TRUE;
    }
  else
    trafColFam = indexColFam;

  // Verify that current user has authority to create an index
  // The user must own the base table or have the ALTER_TABLE privilege or
  // have the CREATE_INDEX privilege
  if (!isDDLOperationAuthorized(SQLOperation::ALTER_TABLE,
                                naTable->getOwner(),naTable->getSchemaOwner()) &&
      !isDDLOperationAuthorized(SQLOperation::CREATE_INDEX,
                                naTable->getOwner(),naTable->getSchemaOwner()))
  {
     *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);

      processReturn();

      return;
    }

  if (naTable->getObjectType() != COM_BASE_TABLE_OBJECT)
    {
      *CmpCommon::diags()
	<< DgSqlCode(-1127)
	<< DgTableName (extTableName);

      processReturn();

      return;
    }

  // can only create a volatile index on a volatile table
  if ((NOT naTable->isVolatileTable()) &&
      (createIndexNode->isVolatile()))
    {
      *CmpCommon::diags()
	<< DgSqlCode(-CAT_VOLATILE_OPERATION_ON_REGULAR_OBJECT)
	<< DgTableName (createIndexNode->getTableName());

      processReturn();

      return;
    }
 
  if ((createIndexNode->isNoPopulateOptionSpecified()) &&
      (createIndexNode->isVolatile() || naTable->isVolatileTable()))
    {
      *CmpCommon::diags()
	<< DgSqlCode(-CAT_NO_POPULATE_VOLATILE_INDEX)
	<< DgString0(extIndexName)
	<< DgTableName(extTableName);

      processReturn();

      return;
    }
  ParDDLFileAttrsCreateIndex &fileAttribs =
    createIndexNode->getFileAttributes();

  NABoolean alignedFormat = FALSE;
  if (fileAttribs.isRowFormatSpecified() == TRUE)
    {
      if (fileAttribs.getRowFormat() == ElemDDLFileAttrRowFormat::eALIGNED)
        {
          if (alignedFormatNotAllowed)
          {
             *CmpCommon::diags() << DgSqlCode(-4223)
                                 << DgString0("Column Family specification on columns of an aligned format index is");
             processReturn();
          }
          alignedFormat = TRUE;
        }
    }
  else if(CmpCommon::getDefault(TRAF_INDEX_ALIGNED_ROW_FORMAT) == DF_ON)
    {
      if ( NOT isSeabaseReservedSchema(tableName))
        alignedFormat = TRUE;
    }
  else
  if (naTable->isSQLMXAlignedTable())
    alignedFormat = TRUE;

  if (alignedFormatNotAllowed)
     alignedFormat = FALSE;

  if ((naTable->hasSecondaryIndexes()) &&
      (NOT createIndexNode->isVolatile()))
    {
      const NAFileSetList &indexList = naTable->getIndexList();
      for (Int32 i = 0; i < indexList.entries(); i++)
	{
	  const NAFileSet * naf = indexList[i];
	  if (naf->getKeytag() == 0)
	    continue;
	  
	  if (naf->getExtFileSetName() == extIndexName)
	    {
	      *CmpCommon::diags() << DgSqlCode(-1390)
				  << DgString0(extIndexName);
	      deallocEHI(ehi); 
	      
	      processReturn();
	      
	      return;
	    }
	} // for
    } // if

  if ((naTable->isVolatileTable()) &&
      (CmpCommon::context()->sqlSession()->volatileSchemaInUse()) &&
      (NOT createIndexNode->isVolatile()))
    {
      // create volatile index
      QualifiedName *qn =
	CmpCommon::context()->sqlSession()->
	updateVolatileQualifiedName(objectNamePart);
      
      catalogNamePart = qn->getCatalogName();
      schemaNamePart = qn->getSchemaName();
      extIndexName = qn->getQualifiedNameAsAnsiString();
      extNameForHbase = catalogNamePart + "." + schemaNamePart + "." + objectNamePart;
    }
  
  retcode = existsInSeabaseMDTable
    (&cliInterface, 
     catalogNamePart, schemaNamePart, objectNamePart,
     COM_INDEX_OBJECT, 
     FALSE/*valid or invalid object*/);
  if (retcode < 0)
    {
      deallocEHI(ehi); 

      processReturn();

      return;
    }

  if (retcode == 0) // doesn't exist
    {
      retcode = existsInSeabaseMDTable
        (&cliInterface, 
         catalogNamePart, schemaNamePart, objectNamePart,
         COM_UNKNOWN_OBJECT/*check for any object with this name*/, 
         TRUE/*valid object*/);
      if (retcode < 0)
        {
          deallocEHI(ehi); 
          
          processReturn();
          
          return;
        }
    }

  if (retcode == 1) // already exists
    {
      if (1) //NOT createIndexNode->createIfNotExists())
	{
	  if (createIndexNode->isVolatile())
	    *CmpCommon::diags() << DgSqlCode(-1390)
				<< DgString0(objectNamePart);
	  else
	    *CmpCommon::diags() << DgSqlCode(-1390)
				<< DgString0(extIndexName);
	}

      deallocEHI(ehi); 

      processReturn();

      return;
    }

  HbaseStr hbaseIndex;
  hbaseIndex.val = (char*)extNameForHbase.data();
  hbaseIndex.len = extNameForHbase.length();

  const NAColumnArray & naColArray = naTable->getNAColumnArray();
  ElemDDLColRefArray & indexColRefArray = createIndexNode->getColRefArray();
  const NAFileSet * nafs = naTable->getClusteringIndex();
  const NAColumnArray &baseTableKeyArr = nafs->getIndexKeyColumns();
  Int32 numSplits = 0;
  CollIndex numPrefixColumns = 0;

  if (createIndexNode->getSaltOptions() && 
      createIndexNode->getSaltOptions()->getLikeTable())
  {
    if (createIndexNode->isUniqueSpecified())
    {
      // TBD: allow unique indexes on a superset of the SALT BY columns
      *CmpCommon::diags() << DgSqlCode(-CAT_INVALID_SALTED_UNIQUE_IDX)
                           << DgString0(extIndexName);
       deallocEHI(ehi);
       processReturn();
       return;
    }
    // verify base table is salted
    if (naTable->hasSaltedColumn())
    {
      createIndexNode->getSaltOptions()->setNumPartns(nafs->numSaltPartns());
      NAString saltColName;
      for (CollIndex c=0; c<baseTableKeyArr.entries(); c++)
        if (baseTableKeyArr[c]->isSaltColumn())
          {
            saltColName = baseTableKeyArr[c]->getColName();
            break;
          }

      ElemDDLColRef * saltColRef = new (STMTHEAP) ElemDDLColRef(
                                        saltColName /*column_name*/,
                                        COM_UNKNOWN_ORDER /*default val*/,
                                        STMTHEAP);
      //SALT column will be the first column in the index
      indexColRefArray.insertAt(numPrefixColumns, saltColRef);
      numPrefixColumns++;
      numSplits = nafs->numSaltPartns() - 1;
    }
    else
    {
       // give a warning that table is not salted
       *CmpCommon::diags() << DgSqlCode(CAT_INVALID_SALT_LIKE_CLAUSE)
                           << DgString0(extTableName)
                           << DgString1(extIndexName);
    }
  }

  if (createIndexNode->getDivisionType() == ElemDDLDivisionClause::DIVISION_LIKE_TABLE)
    {
      if (createIndexNode->isUniqueSpecified())
        {
          // TBD: Allow unique indexes on a superset of the division by columns
          *CmpCommon::diags() << DgSqlCode(-1402)
                              << DgTableName(extIndexName)
                              << DgString0(extTableName);
          deallocEHI(ehi);
          processReturn();
          return;
        }

      int numDivisioningColumns = 0;

      for (CollIndex c=0; c<baseTableKeyArr.entries(); c++)
        if (baseTableKeyArr[c]->isDivisioningColumn())
          {
            ElemDDLColRef * divColRef = new (STMTHEAP) ElemDDLColRef(
               baseTableKeyArr[c]->getColName(),
               COM_UNKNOWN_ORDER /*default val*/,
               STMTHEAP);
          // divisioning columns go after the salt but before any user columns
          indexColRefArray.insertAt(numPrefixColumns, divColRef);
          numPrefixColumns++;
          numDivisioningColumns++;
        }

      if (numDivisioningColumns == 0)
        {
          // give a warning that table is not divisioned
          *CmpCommon::diags() << DgSqlCode(4248)
                              << DgString0(extTableName)
                              << DgString1(extIndexName);
        }
    }

  Lng32 keyColCount = 0;
  Lng32 nonKeyColCount = 0;
  Lng32 totalColCount = 0;
  Lng32 keyLength = 0;

  ComTdbVirtTableColumnInfo * colInfoArray = NULL;
  ComTdbVirtTableKeyInfo * keyInfoArray = NULL;

  NAList<NAString> selColList(STMTHEAP);

  if (createIndexColAndKeyInfoArrays(indexColRefArray,
				     createIndexNode->isUniqueSpecified(),
				     naTable->getClusteringIndex()->hasSyskey(),
                                     alignedFormat,
                                     trafColFam,
				     naColArray,
				     baseTableKeyArr,
				     keyColCount,
				     nonKeyColCount,
				     totalColCount,
				     colInfoArray,
				     keyInfoArray,
				     selColList,
                                     keyLength,
				     STMTHEAP))
    {
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  char ** encodedKeysBuffer = NULL;
  if (numSplits > 0) {

    TrafDesc * colDescs = 
      convertVirtTableColumnInfoArrayToDescStructs(&tableName,
                                                   colInfoArray,
                                                   totalColCount) ;
    TrafDesc * keyDescs = 
      convertVirtTableKeyInfoArrayToDescStructs(keyInfoArray,
                                                colInfoArray,
                                                keyColCount) ;

    if (createEncodedKeysBuffer(encodedKeysBuffer/*out*/,
                                numSplits/*out*/,
                                colDescs, keyDescs,
                                nafs->numSaltPartns(), numSplits, NULL,
                                keyColCount, keyLength, TRUE))
      {
        deallocEHI(ehi);
        processReturn();
        return;
      }
  }

  ComTdbVirtTableTableInfo * tableInfo = new(STMTHEAP) ComTdbVirtTableTableInfo[1];
  tableInfo->tableName = NULL,
  tableInfo->createTime = 0;
  tableInfo->redefTime = 0;
  tableInfo->objUID = 0;
  tableInfo->objOwnerID = naTable->getOwner(); 
  tableInfo->schemaOwnerID = naTable->getSchemaOwner();

  if (NOT createIndexNode->isNoPopulateOptionSpecified())
    // if index is to be populated during create index, then initially create it as an
    // unaudited index. That would avoid any transactional inserts.
    // After the index has been populated, make it audited.
    tableInfo->isAudited = 0;
  else
    tableInfo->isAudited = (nafs->isAudited() ? 1 : 0);

  tableInfo->validDef = 0;
  tableInfo->hbaseCreateOptions = NULL;
  tableInfo->numSaltPartns = (numSplits > 0 ? numSplits+1 : 0);
  tableInfo->rowFormat = (alignedFormat ? COM_ALIGNED_FORMAT_TYPE : COM_HBASE_FORMAT_TYPE);

  ComTdbVirtTableIndexInfo * ii = new(STMTHEAP) ComTdbVirtTableIndexInfo();
  ii->baseTableName = (char*)extTableName.data();
  ii->indexName = (char*)extIndexName.data();
  ii->keytag = 1;
  ii->isUnique = createIndexNode->isUniqueSpecified() ? 1 : 0;
  ii->isExplicit = 1;
  ii->keyColCount = keyColCount;
  ii->nonKeyColCount = nonKeyColCount;
  ii->keyInfoArray = NULL; //keyInfoArray;

  NAList<HbaseCreateOption*> hbaseCreateOptions(STMTHEAP);
  NAString hco;

  if (alignedFormat)
    {
      hco += "ROW_FORMAT=>ALIGNED ";
    }

  short retVal = setupHbaseOptions(createIndexNode->getHbaseOptionsClause(), 
                                   numSplits, extIndexName,
                                   hbaseCreateOptions, hco);
  if (retVal)
  {
    deallocEHI(ehi); 
    processReturn();
    return;
  }
  tableInfo->hbaseCreateOptions = (hco.isNull() ? NULL : hco.data());

  NABoolean xnWasStartedHere = FALSE;
  Int64 objUID = -1;
  if (beginXnIfNotInProgress(&cliInterface, xnWasStartedHere))
    {
      goto label_error;
    }

  if (updateSeabaseMDTable(&cliInterface, 
			 catalogNamePart, schemaNamePart, objectNamePart,
			 COM_INDEX_OBJECT,
			 "N",
			 tableInfo,
			 totalColCount,
			 colInfoArray,
			 totalColCount,
			 keyInfoArray,
			 1, // numIndex
                         ii,
                         objUID))
    {
      goto label_error;
    }


  if (createHbaseTable(ehi, &hbaseIndex, trafColFam.data(),
                       &hbaseCreateOptions,
                       numSplits, keyLength, 
                       encodedKeysBuffer,
                       FALSE, createIndexNode->ddlXns()) == -1)
    {
      goto label_error_drop_index;
    }

  endXnIfStartedHere(&cliInterface, xnWasStartedHere, 0);

  if (NOT createIndexNode->isNoPopulateOptionSpecified())
    {
      NABoolean useLoad = 
        (CmpCommon::getDefault(TRAF_LOAD_USE_FOR_INDEXES) == DF_ON);

      NABoolean indexOpt = 
        (CmpCommon::getDefault(TRAF_INDEX_CREATE_OPT) == DF_ON);

      if (indexOpt)
        {
          // validate that table is empty.
          // If table is empty, no need to load data into the index.
          HbaseStr tblName;
          tblName.val = (char*)extTableNameForHbase.data();
          tblName.len = extNameForHbase.length();
          retcode = ehi->isEmpty(tblName);
          if (retcode < 0)
            {
              goto label_error;
            }
          
          if (retcode == 0) // not empty
            indexOpt = FALSE;
        }

      if (NOT indexOpt)
        {
          // populate index
          if (populateSeabaseIndexFromTable(&cliInterface,
                                            createIndexNode->isUniqueSpecified(),
                                            extIndexName, 
                                            isVolatileTable ? volTabName : tableName, 
                                            selColList,
                                            useLoad))
            { 
              goto label_error_drop_index;
            }
        }

      if (beginXnIfNotInProgress(&cliInterface, xnWasStartedHere))
      {
         goto label_error_drop_index;
      }

      if (updateObjectAuditAttr(&cliInterface, 
			       catalogNamePart, schemaNamePart, objectNamePart,
				TRUE, COM_INDEX_OBJECT_LIT))
	{
          goto label_error_drop_index;
	}

      if (updateObjectValidDef(&cliInterface, 
			       catalogNamePart, schemaNamePart, objectNamePart,
			       COM_INDEX_OBJECT_LIT,
			       "Y"))
	{
          goto label_error_drop_index;
	}
    }

  if (updateObjectRedefTime(&cliInterface,
                            btCatalogNamePart, btSchemaNamePart, btObjectNamePart,
                            COM_BASE_TABLE_OBJECT_LIT, -1, btObjUID))
    {
      goto label_error_drop_index;
    }

  deallocEHI(ehi);
  endXnIfStartedHere(&cliInterface, xnWasStartedHere, 0);

  if (!Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
    {
      ActiveSchemaDB()->getNATableDB()->removeNATable
        (cn,
         ComQiScope::REMOVE_FROM_ALL_USERS, 
         COM_BASE_TABLE_OBJECT,
         createIndexNode->ddlXns(), FALSE);
    }

  return;

 label_error:
  endXnIfStartedHere(&cliInterface, xnWasStartedHere, -1);
  deallocEHI(ehi);
  return;

 label_error_drop_index:
  endXnIfStartedHere(&cliInterface, xnWasStartedHere, -1);
  cleanupObjectAfterError(cliInterface, 
                          catalogNamePart, schemaNamePart, objectNamePart,
                          COM_INDEX_OBJECT,
                          FALSE);
  deallocEHI(ehi);
  return;
}

short CmpSeabaseDDL::populateSeabaseIndexFromTable(
					  ExeCliInterface * cliInterface,
					  NABoolean isUnique,
					  const NAString &indexName, 
                                          const ComObjectName &tableName,
					  NAList<NAString> &selColList,
					  NABoolean useLoad)
{
  Lng32 cliRC = 0;
  Lng32 saltRC = 0;

  NABoolean useHiveSrc = FALSE;
  NAString saltText;
  NAString hiveSrc = CmpCommon::getDefaultString(USE_HIVE_SOURCE);
  if (! hiveSrc.isNull())
    {
      for (CollIndex i = 0; i < selColList.entries(); i++)
      {
      NAString &colName = selColList[i];
      if (colName == "SYSKEY")
        break;
      if (i == selColList.entries() -1)
        useHiveSrc = TRUE;
     }
      if (useHiveSrc)
      {
        saltRC = getSaltText(cliInterface, 
                             tableName.getCatalogNamePartAsAnsiString().data(),
                             tableName.getSchemaNamePartAsAnsiString().data(),
                             tableName.getObjectNamePartAsAnsiString().data(),
                             COM_BASE_TABLE_OBJECT_LIT,
                             saltText);
        if (saltRC < 0)
          useHiveSrc = FALSE;
      }
    }

  NAString query = 
    (isUnique ? "insert with no rollback " : "upsert using load ");
  if (useLoad)
  {
    // index table only option is used internally and is used to populate 
    //the index table
    query = " Load with no output, no recovery, Index table only ";
  }
  query += "into table(index_table ";
  query += indexName;
  query += " ) select ";
  for (CollIndex i = 0; i < selColList.entries(); i++)
    {
      NAString &colName = selColList[i];
      if ((colName == "_SALT_") && useHiveSrc)
        query += saltText;
      else
      {
        query += "\"";
        query += colName;
        query += "\"";
      }

      if (i < selColList.entries() - 1)
	query += ",";
    }

  query += " from ";
  if (useHiveSrc)
    {
      query += "HIVE.HIVE.";
      query += tableName.getObjectNamePartAsAnsiString(); 
      query += hiveSrc; //will not work for delim tab names
    }
  else
    query += tableName.getExternalName(TRUE);

  query += " for read uncommitted access; ";

  UInt32 savedCliParserFlags = 0;
  SQL_EXEC_GetParserFlagsForExSqlComp_Internal(savedCliParserFlags);

  cliRC = cliInterface->holdAndSetCQD("ALLOW_DML_ON_NONAUDITED_TABLE", "ON");
  if (cliRC < 0)
    {
      return -1;
    }

  cliRC = cliInterface->holdAndSetCQD("attempt_esp_parallelism", "ON");
  if (cliRC < 0)
    {
      return -1;
    }

  cliRC = cliInterface->holdAndSetCQD("hide_indexes", "all");
  if (cliRC < 0)
    {
      return -1;
    }

  if (useLoad)
  {
    cliRC = cliInterface->holdAndSetCQD("TRAF_LOAD_FORCE_CIF", "OFF");
    if (cliRC < 0)
      {
        return -1;
      }
  }
  SQL_EXEC_SetParserFlagsForExSqlComp_Internal(ALLOW_SPECIALTABLETYPE);
  SQL_EXEC_SetParserFlagsForExSqlComp_Internal(ALLOW_VOLATILE_SCHEMA_IN_TABLE_NAME);
  cliRC = cliInterface->executeImmediate(query);

  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

      if ((isUnique) && (CmpCommon::diags()->mainSQLCODE() == -EXE_DUPLICATE_ENTIRE_RECORD))
        {
          *CmpCommon::diags() << DgSqlCode(-CAT_UNIQUE_INDEX_LOAD_FAILED_WITH_DUPLICATE_ROWS)
                              << DgTableName(indexName);
         }
      else
        {
          *CmpCommon::diags() << DgSqlCode(-CAT_CLI_LOAD_INDEX)
                              << DgTableName(indexName);
        }
    }

  //  SQL_EXEC_ResetParserFlagsForExSqlComp_Internal(ALLOW_SPECIALTABLETYPE);
  //  SQL_EXEC_ResetParserFlagsForExSqlComp_Internal(ALLOW_VOLATILE_SCHEMA_IN_TABLE_NAME);
  SQL_EXEC_AssignParserFlagsForExSqlComp_Internal(savedCliParserFlags);

  cliInterface->restoreCQD("hide_indexes");
  cliInterface->restoreCQD("allow_dml_on_nonaudited_table");
  cliInterface->restoreCQD("attempt_esp_parallelism");
  if (useLoad)
    cliInterface->restoreCQD("TRAF_LOAD_FORCE_CIF");

  if (cliRC < 0)
    return -1;

  return 0;
}

void CmpSeabaseDDL::populateSeabaseIndex(
					 StmtDDLPopulateIndex * populateIndexNode,
					 NAString &currCatName, NAString &currSchName)
{
  Lng32 retcode = 0;
  
  ComObjectName tableName(populateIndexNode->getTableName());
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);
  const NAString btCatalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString btSchemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);

  ComObjectName indexName(populateIndexNode->getIndexName());
  indexName.applyDefaults(btCatalogNamePart, btSchemaNamePart); 

  const NAString catalogNamePart = indexName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = indexName.getSchemaNamePartAsAnsiString(TRUE);
  NAString objectNamePart = indexName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extIndexName = indexName.getExternalName(TRUE);
  const NAString extNameForHbase = 
    catalogNamePart + "." + schemaNamePart + "." + objectNamePart;

  if (isSeabaseReservedSchema(indexName))
    {
      *CmpCommon::diags() << DgSqlCode(-1118)
			  << DgTableName(extTableName);
      return;
    }

  ExeCliInterface cliInterface(STMTHEAP, NULL, NULL, 
       CmpCommon::context()->sqlSession()->getParentQid());

  //  If an index was created with NO POPULATE option, then
  // it will be marked as invalid in metadata. The regular table descriptor will
  // not have information about invalid indexes as we dont want those to
  // be used in queries.
  // Create a table descriptor that contains information on both valid and
  // invalid indexes. Pass that to getNATable method which will use this
  // table desc to create the NATable struct.
  TrafDesc * tableDesc = 
    getSeabaseTableDesc(
		      tableName.getCatalogNamePart().getInternalName(),
		      tableName.getSchemaNamePart().getInternalName(),
		      tableName.getObjectNamePart().getInternalName(),
		      COM_BASE_TABLE_OBJECT,
		      TRUE /*return info on valid and invalid indexes */);

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), 
		FALSE/*inDDL*/);
  CorrName cn(tableName.getObjectNamePart().getInternalName(),
	      STMTHEAP,
	      tableName.getSchemaNamePart().getInternalName(),
	      tableName.getCatalogNamePart().getInternalName());
  
  NATable *naTable = bindWA.getNATable(cn, TRUE, tableDesc); 
  if (naTable == NULL || bindWA.errStatus())
    {
      *CmpCommon::diags()
	<< DgSqlCode(-4082)
	<< DgTableName(cn.getExposedNameAsAnsiString());

      processReturn();
    
      return;
    }

  // Verify that current user has authority to populate the index
  // User must be DB__ROOT or have privileges
  PrivMgrUserPrivs *privs = naTable->getPrivInfo();
  if (isAuthorizationEnabled() && privs == NULL)
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_PRIVS);
  
       processReturn();

       return;
    }

  // Requester must have SELECT and INSERT privileges
  if (!ComUser::isRootUserID() && 
      isAuthorizationEnabled() &&
      !Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
    {
      NABoolean hasPriv = TRUE;
      if ( !privs->hasSelectPriv() )
        {
           hasPriv = FALSE;
           *CmpCommon::diags() << DgSqlCode( -4481 )
                               << DgString0( "SELECT" )
                               << DgString1( extTableName.data());
        }

      if ( !privs->hasInsertPriv() )
        {   
           hasPriv = FALSE;
           *CmpCommon::diags() << DgSqlCode( -4481 )
                               << DgString0( "INSERT" )
                               << DgString1( extTableName.data());
        }   
      if (!hasPriv)
      {
         processReturn();

         return;
      }
    }

  const NAFileSetList &indexList = naTable->getIndexList();
  NABoolean xnWasStartedHere = FALSE;
  for (Int32 i = 0; i < indexList.entries(); i++)
    {
      const NAFileSet * naf = indexList[i];
      if (naf->getKeytag() == 0)
	continue;
      
      const QualifiedName &qn = naf->getFileSetName();
      const NAString& nafIndexName =
	qn.getQualifiedNameAsAnsiString(TRUE);
      
      if ((populateIndexNode->populateAll()) || 
          (populateIndexNode->populateAllUnique() && naf->uniqueIndex()) ||
	  (extIndexName == nafIndexName))
      {
        if (populateIndexNode->populateAll() || 
            populateIndexNode->populateAllUnique())
          {
            objectNamePart= qn.getObjectName().data();
          }

	  // check if nafIndexName is a valid index. Is so, it has already been
	  // populated. Skip it.
	  NABoolean isValid =
	    existsInSeabaseMDTable(
				 &cliInterface,
				 qn.getCatalogName().data(), 
				 qn.getSchemaName().data(), 
				 qn.getObjectName().data());
	  if (isValid)
	    continue;

	  NAList<NAString> selColList(STMTHEAP);

	  for (Lng32 ii = 0; ii < naf->getAllColumns().entries(); ii++)
	    {
	      NAColumn * nac = naf->getAllColumns()[ii];
	      
	      const NAString &colName = nac->getColName();

	      selColList.insert(colName);
	    }

	  // make the index unaudited during population to avoid transactional overhead.
	  if (updateObjectAuditAttr(&cliInterface, 
				    catalogNamePart, schemaNamePart, 
                                    objectNamePart,
				    FALSE, COM_INDEX_OBJECT_LIT))
	    {
	      processReturn();
	      
              goto label_return;
	    }

	  NABoolean useLoad = (CmpCommon::getDefault(TRAF_LOAD_USE_FOR_INDEXES) == DF_ON);
	  if (populateSeabaseIndexFromTable(&cliInterface,
					    naf->uniqueIndex(),
					    nafIndexName, tableName, selColList,
	                                    useLoad))
	  {
	    processReturn();
            goto purgedata_return;
	  }
      
          if (updateObjectAuditAttr(&cliInterface, 
				catalogNamePart, schemaNamePart, objectNamePart,
				TRUE, COM_INDEX_OBJECT_LIT))
	  {
	     processReturn();
             goto purgedata_return;
	  }
          if (beginXnIfNotInProgress(&cliInterface, xnWasStartedHere))
          {
	     processReturn();
             goto purgedata_return;
          }
          if (updateObjectValidDef(&cliInterface, 
			       catalogNamePart, schemaNamePart, objectNamePart,
			       COM_INDEX_OBJECT_LIT,
			       "Y"))
	  {
	     processReturn();
             goto purgedata_return;
	  }
          endXnIfStartedHere(&cliInterface, xnWasStartedHere, 0);
      }
    } // for

 label_return:
  processReturn();
  return;
 purgedata_return:
  endXnIfStartedHere(&cliInterface, xnWasStartedHere, -1);
  updateObjectAuditAttr(&cliInterface, 
                 catalogNamePart, schemaNamePart, objectNamePart,
                 TRUE, COM_INDEX_OBJECT_LIT);
  NABoolean dontForceCleanup = FALSE;
  purgedataObjectAfterError(cliInterface,
                               catalogNamePart, schemaNamePart, objectNamePart, COM_INDEX_OBJECT, dontForceCleanup);
  processReturn();
  return;
}

void CmpSeabaseDDL::dropSeabaseIndex(
				 StmtDDLDropIndex * dropIndexNode,
				 NAString &currCatName, NAString &currSchName)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  NAString idxName = dropIndexNode->getIndexName();

  ComObjectName indexName(idxName);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  indexName.applyDefaults(currCatAnsiName, currSchAnsiName);

  NAString catalogNamePart = indexName.getCatalogNamePartAsAnsiString();
  NAString schemaNamePart = indexName.getSchemaNamePartAsAnsiString(TRUE);
  NAString objectNamePart = indexName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extIndexName = indexName.getExternalName(TRUE);

  ExeCliInterface cliInterface(STMTHEAP, NULL, NULL, 
       CmpCommon::context()->sqlSession()->getParentQid());

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();
      return;
    }

  NABoolean isVolatile = FALSE;

  if ((dropIndexNode->isVolatile()) &&
      (CmpCommon::context()->sqlSession()->volatileSchemaInUse()))
    isVolatile = TRUE;
  
  if ((NOT dropIndexNode->isVolatile()) &&
      (CmpCommon::context()->sqlSession()->volatileSchemaInUse()))
    {
      QualifiedName *qn =
	CmpCommon::context()->sqlSession()->
	updateVolatileQualifiedName(objectNamePart);
      
      if (qn == NULL)
	{
	  *CmpCommon::diags()
	    << DgSqlCode(-CAT_UNABLE_TO_DROP_OBJECT)
	    << DgTableName(objectNamePart);

	  processReturn();

	  deallocEHI(ehi); 

	  return;
	}
      
        ComObjectName volTabName (qn->getQualifiedNameAsAnsiString());
	volTabName.applyDefaults(currCatAnsiName, currSchAnsiName);

	NAString vtCatNamePart = volTabName.getCatalogNamePartAsAnsiString();
	NAString vtSchNamePart = volTabName.getSchemaNamePartAsAnsiString(TRUE);
	NAString vtObjNamePart = volTabName.getObjectNamePartAsAnsiString(TRUE);

	retcode = existsInSeabaseMDTable(&cliInterface, 
					 vtCatNamePart, vtSchNamePart, vtObjNamePart,
					 COM_INDEX_OBJECT);

	if (retcode < 0)
	  {
	    processReturn();

	    deallocEHI(ehi); 
	    
	    return;
	  }

        if (retcode == 1)
          {
            // table found in volatile schema
            // Validate volatile table name.
            if (CmpCommon::context()->sqlSession()->
                validateVolatileQualifiedName
                (dropIndexNode->getOrigIndexNameAsQualifiedName()))
              {
                // Valid volatile table. Drop it.
                idxName = volTabName.getExternalName(TRUE);

		catalogNamePart = vtCatNamePart;
		schemaNamePart = vtSchNamePart;
		objectNamePart = vtObjNamePart;

		isVolatile = TRUE;
              }
            else
              {
                // volatile table found but the name is not a valid
                // volatile name. Look for the input name in the regular
                // schema.
                // But first clear the diags area.
                CmpCommon::diags()->clear();
              }
          }
	else
	  {
	    CmpCommon::diags()->clear();
	  }
      }

  retcode = existsInSeabaseMDTable(&cliInterface, 
                                   catalogNamePart, schemaNamePart, objectNamePart,
                                   COM_INDEX_OBJECT, 
                                   (Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL) 
                                    ? FALSE : TRUE),
                                   TRUE, TRUE);
  if (retcode < 0)
    {
      deallocEHI(ehi); 

      processReturn();

      return;
    }

  if (retcode == 0) // does not exist
    {
      if (1) //NOT dropIndexNode->dropIfExists())
	{
	  if (isVolatile)
	    *CmpCommon::diags() << DgSqlCode(-CAT_OBJECT_DOES_NOT_EXIST_IN_TRAFODION)
				<< DgString0(objectNamePart);
	  else
	    *CmpCommon::diags() << DgSqlCode(-CAT_OBJECT_DOES_NOT_EXIST_IN_TRAFODION)
				<< DgString0(extIndexName);
	}

      processReturn();

      deallocEHI(ehi); 

      return;
    }

  // if 'no check' option is specified, then dont check for dependent constraints. 
  // if cascade option is specified, then drop dependent constraints on this index.
  // 

  // get base table name and base table uid
  NAString btCatName;
  NAString btSchName;
  NAString btObjName;
  Int64 btUID;
  Int32 btObjOwner = 0;
  Int32 btSchemaOwner = 0;
  if (getBaseTable(&cliInterface,
		   catalogNamePart, schemaNamePart, objectNamePart,
		   btCatName, btSchName, btObjName, btUID, btObjOwner, btSchemaOwner))
    {
      processReturn();
      
      deallocEHI(ehi); 
      
      return;
    }
  
  // Verify that current user has authority to drop the index
  if ((!isDDLOperationAuthorized(SQLOperation::DROP_INDEX, btObjOwner, btSchemaOwner)) &&
      (!isDDLOperationAuthorized(SQLOperation::ALTER_TABLE, btObjOwner, btSchemaOwner)))
  {
     *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);

     processReturn();

     return;
  }

  if (dropIndexNode->getDropBehavior() != COM_NO_CHECK_DROP_BEHAVIOR)    
    {
      // get index UID
      Int64 indexUID = getObjectUID(&cliInterface,
				    catalogNamePart.data(), 
				    schemaNamePart.data(), 
				    objectNamePart.data(),
				    COM_INDEX_OBJECT_LIT);
      if (indexUID < 0)
	{
	  processReturn();
	  
	  deallocEHI(ehi); 
	  
	  return;
	}

      NAString constrCatName;
      NAString constrSchName;
      NAString constrObjName;
      Int64 constrUID = getConstraintOnIndex(&cliInterface,
					     btUID, indexUID, COM_UNIQUE_CONSTRAINT_LIT,
					     constrCatName, constrSchName, constrObjName);
      if (constrUID > 0)
	{
	  // constraint exists
	  if (dropIndexNode->getDropBehavior() != COM_CASCADE_DROP_BEHAVIOR)    
	    {
	      *CmpCommon::diags() << DgSqlCode(-CAT_DEPENDENT_CONSTRAINT_EXISTS);
	      
	      processReturn();
	      
	      deallocEHI(ehi); 
	      
	      return;
	    }  

	  // drop the constraint
	  char buf[4000];
	  str_sprintf(buf, "alter table \"%s\".\"%s\".\"%s\" drop constraint %s.%s.%s no check",
		      btCatName.data(), btSchName.data(), btObjName.data(),
		      constrCatName.data(), constrSchName.data(), constrObjName.data());

	  cliRC = cliInterface.executeImmediate(buf);
	  
	  if (cliRC < 0)
	    {
	      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

	      processReturn();
	      
	      deallocEHI(ehi); 
	      
	      return;
	    }
	}

      constrUID = getConstraintOnIndex(&cliInterface,
				       btUID, indexUID, COM_FOREIGN_KEY_CONSTRAINT_LIT,
				       constrCatName, constrSchName, constrObjName);
      if (constrUID > 0)
	{
	  // constraint exists
	  if (dropIndexNode->getDropBehavior() != COM_CASCADE_DROP_BEHAVIOR)    
	    {
	      *CmpCommon::diags() << DgSqlCode(-CAT_DEPENDENT_CONSTRAINT_EXISTS);
	      
	      processReturn();
	      
	      deallocEHI(ehi); 
	      
	      return;
	    }  

	  // drop the constraint
	  char buf[4000];
	  str_sprintf(buf, "alter table \"%s\".\"%s\".\"%s\" drop constraint %s.%s.%s no check",
		      btCatName.data(), btSchName.data(), btObjName.data(),
		      constrCatName.data(), constrSchName.data(), constrObjName.data());

	  cliRC = cliInterface.executeImmediate(buf);
	  
	  if (cliRC < 0)
	    {
	      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

	      processReturn();
	      
	      deallocEHI(ehi); 
	      
	      return;
	    }
	}
     }

  if (dropSeabaseObject(ehi, idxName,
			currCatName, currSchName, COM_INDEX_OBJECT,
                        dropIndexNode->ddlXns()))
    {
      processReturn();

      deallocEHI(ehi); 

      return;
    }

  if (updateObjectRedefTime(&cliInterface,
                            btCatName, btSchName, btObjName,
                            COM_BASE_TABLE_OBJECT_LIT, -1, btUID))
    {
      processReturn();

      deallocEHI(ehi);

      return;
    }

  // remove NATable for the base table of this index
  CorrName cn(btObjName, STMTHEAP, btSchName, btCatName);
  ActiveSchemaDB()->getNATableDB()->removeNATable
    (cn,
     ComQiScope::REMOVE_FROM_ALL_USERS, COM_BASE_TABLE_OBJECT,
     dropIndexNode->ddlXns(), FALSE);

  // remove NATable for this index in its real form as well as in its index_table
  // standalone format
  CorrName cni(objectNamePart, STMTHEAP, schemaNamePart, catalogNamePart);
  ActiveSchemaDB()->getNATableDB()->removeNATable
    (cni,
     ComQiScope::REMOVE_FROM_ALL_USERS, COM_INDEX_OBJECT,
     dropIndexNode->ddlXns(), FALSE);
  cni.setSpecialType(ExtendedQualName::INDEX_TABLE);
  ActiveSchemaDB()->getNATableDB()->removeNATable
    (cni,
     ComQiScope::REMOVE_MINE_ONLY, COM_INDEX_OBJECT,
     dropIndexNode->ddlXns(), FALSE);
  
  //  processReturn();

  deallocEHI(ehi); 

  return;
}

void CmpSeabaseDDL::alterSeabaseTableDisableOrEnableIndex(
				     ExprNode * ddlNode,
				     NAString &currCatName, NAString &currSchName)
{
  Lng32 retcode = 0;

  const StmtDDLAlterTableDisableIndex * alterDisableIndexNode =
    ddlNode->castToStmtDDLNode()->castToStmtDDLAlterTableDisableIndex();

  const StmtDDLAlterTableEnableIndex * alterEnableIndexNode =
    ddlNode->castToStmtDDLNode()->castToStmtDDLAlterTableEnableIndex();

  NABoolean isDisable = 
    (ddlNode->getOperatorType() == DDL_ALTER_TABLE_DISABLE_INDEX);

  const QualifiedName &btName = 
    (isDisable ? alterDisableIndexNode->getTableNameAsQualifiedName() :
     alterEnableIndexNode->getTableNameAsQualifiedName());

  const NAString &btCatName = btName.getCatalogName();
  const NAString &btSchName = btName.getSchemaName();
  const NAString &btObjName = btName.getObjectName();
  NAString extBTname = btCatName + "." + btSchName + "." + btObjName;

  const NAString &idxName = 
    (isDisable ? alterDisableIndexNode->getIndexName() : alterEnableIndexNode->getIndexName());

  ComObjectName indexName(idxName);
  ComAnsiNamePart currCatAnsiName(NOT btCatName.isNull() ? btCatName : currCatName);
  ComAnsiNamePart currSchAnsiName(NOT btSchName.isNull() ? btSchName : currSchName);
  indexName.applyDefaults(currCatAnsiName, currSchAnsiName);

  const NAString catalogNamePart = indexName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = indexName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = indexName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extIndexName = indexName.getExternalName(TRUE);

  ExeCliInterface cliInterface(STMTHEAP, NULL, NULL, 
       CmpCommon::context()->sqlSession()->getParentQid());

  retcode = existsInSeabaseMDTable(&cliInterface, 
                                   btCatName, btSchName, btObjName,
                                   COM_BASE_TABLE_OBJECT, FALSE,
                                   TRUE, TRUE);
  if (retcode < 0)
    {
      processReturn();

      return;
    }

  if (retcode == 0) // does not exist
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_OBJECT_DOES_NOT_EXIST_IN_TRAFODION)
			  << DgString0(extBTname);

      processReturn();

      return;
    }

  retcode = existsInSeabaseMDTable(&cliInterface, 
                                   catalogNamePart, schemaNamePart, objectNamePart,
                                   COM_INDEX_OBJECT, FALSE, TRUE, TRUE);
  if (retcode < 0)
    {
      processReturn();

      return;
    }

  if (retcode == 0) // does not exist
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_OBJECT_DOES_NOT_EXIST_IN_TRAFODION)
			  << DgString0(extIndexName);

      processReturn();

      return;
    }

  Int64 btUID;
  Int32 btObjOwner = 0;
  Int32 btSchemaOwner = 0;
  Int64 btObjectFlags = 0;
  if ((btUID = getObjectInfo(&cliInterface,
                             btCatName, btSchName, btObjName, 
                             COM_BASE_TABLE_OBJECT,
                             btObjOwner, btSchemaOwner, btObjectFlags, btUID)) < 0)
    {
      processReturn();

      return;
    }

  // Verify that current user has authority to drop the index
  if ((!isDDLOperationAuthorized(SQLOperation::DROP_INDEX, btObjOwner, btSchemaOwner)) &&
      (!isDDLOperationAuthorized(SQLOperation::ALTER_TABLE, btObjOwner, btSchemaOwner)))
  {
     *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);

     processReturn();

     return;
  }

  if (updateObjectValidDef(&cliInterface, 
			   catalogNamePart, schemaNamePart, objectNamePart,
			   COM_INDEX_OBJECT_LIT,
			   (isDisable ? "N" : "Y")))
    {
      processReturn();

      return;
    }

  if (updateObjectRedefTime(&cliInterface,
                            btCatName, btSchName, btObjName,
                            COM_BASE_TABLE_OBJECT_LIT, -1, btUID))
    {
      processReturn();

      return;
    }

  // remove NATable for the base table of this index
  CorrName cn(btObjName, STMTHEAP, btSchName, btCatName);
  ActiveSchemaDB()->getNATableDB()->removeNATable
    (cn,
     ComQiScope::REMOVE_FROM_ALL_USERS, COM_BASE_TABLE_OBJECT,
     ddlNode->castToStmtDDLNode()->ddlXns(), FALSE);
  // Also, remove index.
  CorrName cni(objectNamePart, STMTHEAP, schemaNamePart, catalogNamePart);
  ActiveSchemaDB()->getNATableDB()->removeNATable
    (cni,
     ComQiScope::REMOVE_FROM_ALL_USERS, COM_INDEX_OBJECT,
     ddlNode->castToStmtDDLNode()->ddlXns(), FALSE);

  //  processReturn();

  return;
}

short CmpSeabaseDDL::alterSeabaseTableDisableOrEnableIndex(
                                             const char * catName,
                                             const char * schName,
                                             const char * idxName,
                                             const char * tabName,
                                             NABoolean isDisable)
{
  char buf[4000];
 Lng32 cliRC = 0;


  ExeCliInterface cliInterface(STMTHEAP, NULL, NULL, 
       CmpCommon::context()->sqlSession()->getParentQid());

  sprintf (buf, " ALTER TABLE \"%s\".\"%s\".\"%s\"  %s INDEX \"%s\" ;", catName, schName, tabName,
      isDisable ? "DISABLE" : "ENABLE",idxName);

  cliRC = cliInterface.executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  return 0;

}
void CmpSeabaseDDL::alterSeabaseTableDisableOrEnableAllIndexes(
                                             ExprNode * ddlNode,
                                             NAString &currCatName,
                                             NAString &currSchName,
                                             NAString &tabName,
                                             NABoolean allUniquesOnly)
{
  Lng32 cliRC = 0;
  char buf[4000];

  NABoolean isDisable =
    (ddlNode->getOperatorType() == DDL_ALTER_TABLE_DISABLE_INDEX);

  ComObjectName tableName(tabName);

  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);

  ExeCliInterface cliInterface(STMTHEAP, NULL, NULL, 
       CmpCommon::context()->sqlSession()->getParentQid());

  // Fix for launchpad bug 1381621
  Lng32 retcode = existsInSeabaseMDTable(&cliInterface,
                                         catalogNamePart, schemaNamePart, objectNamePart,
                                         COM_BASE_TABLE_OBJECT, TRUE, TRUE, TRUE);
  if (retcode < 0)
    {
      processReturn();
      return;
    }

  if (retcode == 0) // does not exist
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_OBJECT_DOES_NOT_EXIST_IN_TRAFODION) 
                          << DgString0(tableName.getExternalName(TRUE));
      processReturn();
      return;
    }

  str_sprintf(buf,
              " select catalog_name,schema_name,object_name from  %s.\"%s\".%s  " \
              " where object_uid in ( select i.index_uid from "         \
              " %s.\"%s\".%s i "                                        \
              " join    %s.\"%s\".%s  o2 on i.base_table_uid=o2.object_uid " \
              " where  o2.catalog_name= '%s' AND o2.schema_name='%s' AND o2.Object_Name='%s' " \
              " %s "                                                    \
                " and object_type='IX' ; ",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_INDEXES,
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              catalogNamePart.data(),schemaNamePart.data(), objectNamePart.data(),  //table name in this case
              allUniquesOnly ? " AND is_unique = 1 )" : ")" );

  Queue * indexes = NULL;
  cliRC = cliInterface.fetchAllRows(indexes, buf, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return ;
    }

  if (indexes)
  {
    //table has no index -- return
    if (indexes->numEntries() == 0)
      return;

    char * catName = NULL;
    char * schName = NULL;
    indexes->position();
    for (int ii = 0; ii < indexes->numEntries(); ii++)
    {
      OutputInfo * idx = (OutputInfo*) indexes->getNext();

      catName = idx->get(0);
      schName = idx->get(1);
      char * idxName = idx->get(2);

      if (alterSeabaseTableDisableOrEnableIndex ( catName, schName, idxName, objectNamePart, isDisable))
        return;
    }
    CorrName cn( objectNamePart, STMTHEAP, NAString(schName), NAString(catName));
    ActiveSchemaDB()->getNATableDB()->removeNATable
      (cn, 
       ComQiScope::REMOVE_FROM_ALL_USERS, COM_BASE_TABLE_OBJECT,
       ddlNode->castToStmtDDLNode()->ddlXns(), FALSE);
  }

  return ;
}

void CmpSeabaseDDL::alterSeabaseIndexHBaseOptions(
                                       StmtDDLAlterIndexHBaseOptions * hbaseOptionsNode,
                                       NAString &currCatName, NAString &currSchName)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  NAString idxName = hbaseOptionsNode->getIndexName();

  ComObjectName indexName(idxName);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  indexName.applyDefaults(currCatAnsiName, currSchAnsiName);

  NAString catalogNamePart = indexName.getCatalogNamePartAsAnsiString();
  NAString schemaNamePart = indexName.getSchemaNamePartAsAnsiString(TRUE);
  NAString objectNamePart = indexName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extIndexName = indexName.getExternalName(TRUE);
  NAString extNameForHbase = catalogNamePart + "." + schemaNamePart + "." + objectNamePart;

  ExeCliInterface cliInterface(STMTHEAP, NULL, NULL, 
  CmpCommon::context()->sqlSession()->getParentQid());
  
  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();
      return;
    }

  // Disallow this ALTER on system metadata schema objects

  if ((isSeabaseReservedSchema(indexName)) &&
      (!Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_ALTER_NOT_ALLOWED_IN_SMD)
                          << DgTableName(extIndexName);
      deallocEHI(ehi); 
      processReturn();
      return;
    }
  
  // Make sure this object exists

  retcode = existsInSeabaseMDTable(&cliInterface, 
                                   catalogNamePart, schemaNamePart, objectNamePart,
                                   COM_INDEX_OBJECT,
                                   (Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL) 
                                    ? FALSE : TRUE),
                                   TRUE, TRUE);
  if (retcode < 0)  // some error occurred
    {
      processReturn();
      deallocEHI(ehi);
      return;
    }
  else if (retcode == 0)
    {
       CmpCommon::diags()->clear();
      
      *CmpCommon::diags() << DgSqlCode(-CAT_OBJECT_DOES_NOT_EXIST_IN_TRAFODION)
                          << DgString0(extIndexName);

      deallocEHI(ehi); 
      processReturn();     
      return;
    }

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);

  // Get base table name and base table uid

  NAString btCatName;
  NAString btSchName;
  NAString btObjName;
  Int64 btUID;
  Int32 btObjOwner = 0;
  Int32 btSchemaOwner = 0;
  if (getBaseTable(&cliInterface,
		   catalogNamePart, schemaNamePart, objectNamePart,
		   btCatName, btSchName, btObjName, btUID, btObjOwner, btSchemaOwner))
    {
      processReturn();      
      deallocEHI(ehi);      
      return;
    }
  
  CorrName cn(btObjName,
              STMTHEAP,
              btSchName,
              btCatName);
  
  NATable *naTable = bindWA.getNATable(cn); 
  if (naTable == NULL || bindWA.errStatus())
    {
      // shouldn't happen, actually, since getBaseTable above succeeded

      CmpCommon::diags()->clear();     
      *CmpCommon::diags() << DgSqlCode(-CAT_OBJECT_DOES_NOT_EXIST_IN_TRAFODION)
                          << DgString0(extIndexName);
      deallocEHI(ehi);  
      processReturn();     
      return;
    }
 
  // Make sure user has the privilege to perform the ALTER
  
  if (!isDDLOperationAuthorized(SQLOperation::ALTER_TABLE, btObjOwner, btSchemaOwner))
  {
     *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
     deallocEHI(ehi); 
     processReturn();
     return;
  }

  CmpCommon::diags()->clear();

  // Get the object UID so we can update the metadata

  Int64 objUID = getObjectUID(&cliInterface,
                              catalogNamePart.data(), schemaNamePart.data(), 
                              objectNamePart.data(),
                              COM_INDEX_OBJECT_LIT);
  if (objUID < 0)
    {
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  // update HBase options in the metadata

  ElemDDLHbaseOptions * edhbo = hbaseOptionsNode->getHBaseOptions();
  short result = updateHbaseOptionsInMetadata(&cliInterface,objUID,edhbo);
  
  if (result < 0)
    {
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  // tell HBase to change the options

  NAList<NAString> nal(STMTHEAP);
  nal.insert(naTable->defaultColFam());
  HbaseStr hbaseTable;
  hbaseTable.val = (char*)extNameForHbase.data();
  hbaseTable.len = extNameForHbase.length();
  result = alterHbaseTable(ehi,
                           &hbaseTable,
                           nal,
                           &(edhbo->getHbaseOptions()),
                           hbaseOptionsNode->ddlXns());
  if (result < 0)
    {
      deallocEHI(ehi);
      processReturn();
      return;
    }

  if (updateObjectRedefTime(&cliInterface,
                            btCatName, btSchName, btObjName,
                            COM_BASE_TABLE_OBJECT_LIT, -1, btUID))
    {
      deallocEHI(ehi);
      processReturn();

      return;
    }

  // invalidate cached NATable info on this table for all users

  ActiveSchemaDB()->getNATableDB()->removeNATable
    (cn,
     ComQiScope::REMOVE_FROM_ALL_USERS, COM_BASE_TABLE_OBJECT,
     hbaseOptionsNode->ddlXns(), FALSE);

  deallocEHI(ehi); 

  return ;
}

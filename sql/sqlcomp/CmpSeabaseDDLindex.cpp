/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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

#include "CmpDDLCatErrorCodes.h"

#include "SchemaDB.h"
#include "CmpSeabaseDDL.h"
#include "CmpDescribe.h"

#include "ExpHbaseInterface.h"

#include "ExExeUtilCli.h"
#include "Generator.h"
#include "desc.h"

#include "ComCextdecs.h"

#include "NumericType.h"

short CmpSeabaseDDL::createIndexColAndKeyInfoArrays(
						    ElemDDLColRefArray &indexColRefArray,
						    NABoolean isUnique,
						    NABoolean hasSyskey,
						    const NAColumnArray &baseTableNAColArray,
						    const NAColumnArray &baseTableKeyArr,
						    Lng32 &keyColCount,
						    Lng32 &nonKeyColCount,
						    Lng32 &totalColCount,
						    ComTdbVirtTableColumnInfo * &colInfoArray,
						    ComTdbVirtTableKeyInfo * &keyInfoArray,
						    NAList<NAString> &selColList,
						    NAMemory * heap)
{
  Lng32 retcode = 0;

  keyColCount = indexColRefArray.entries();
  nonKeyColCount = 0;

  Lng32 baseTableKeyCount = baseTableKeyArr.entries();

  if (isUnique)
    nonKeyColCount = baseTableKeyCount;
  else
    keyColCount += baseTableKeyCount;

  totalColCount = keyColCount + nonKeyColCount;

  colInfoArray = (ComTdbVirtTableColumnInfo*)
    new(heap) char[totalColCount *  sizeof(ComTdbVirtTableColumnInfo)];

  keyInfoArray = (ComTdbVirtTableKeyInfo*)
    new(heap) char[totalColCount * sizeof(ComTdbVirtTableKeyInfo)];

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

     strcpy(colInfoArray[i].columnClass, COM_USER_COLUMN_LIT);

     const NAType * naType = tableCol->getType();

     Lng32 precision = 0;
     Lng32 scale = 0;
     Lng32 dtStart = 0; 
     Lng32 dtEnd = 0;
     Lng32 upshifted = 0;
     NAString charsetStr;
     SQLCHARSET_CODE charset = SQLCHARSETCODE_UNKNOWN;
     CharInfo::Collation collationSequence = CharInfo::DefaultCollation;
     ULng32 colFlags = 0;
     retcode = getTypeInfo(naType, FALSE,
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
	new(heap) char[strlen(SEABASE_DEFAULT_COL_FAMILY) +1];
     strcpy((char*)colInfoArray[i].hbaseColFam, SEABASE_DEFAULT_COL_FAMILY);

      char idxNumStr[40];
      idxNumStr[0] = '@';
      str_itoa(i+1, &idxNumStr[1]);

      colInfoArray[i].hbaseColQual =
	new(heap) char[strlen(idxNumStr) + 1];
      strcpy((char*)colInfoArray[i].hbaseColQual, idxNumStr);

      colInfoArray[i].hbaseColFlags = tableCol->getHbaseColFlags();
      strcpy(colInfoArray[i].paramDirection, COM_UNKNOWN_PARAM_DIRECTION_LIT);
      colInfoArray[i].isOptional = FALSE;
 
     // update key info
     keyInfoArray[i].colName = col_name; 
     keyInfoArray[i].keySeqNum = i+1;
     keyInfoArray[i].tableColNum = tableCol->getPosition();
     keyInfoArray[i].ordering = 
       (nodeKeyCol->getColumnOrdering() == COM_ASCENDING_ORDER ? 0 : 1);

     keyInfoArray[i].nonKeyCol = 0;

     keyInfoArray[i].hbaseColFam = new(heap) char[strlen(SEABASE_DEFAULT_COL_FAMILY) + 1];
     strcpy((char*)keyInfoArray[i].hbaseColFam, SEABASE_DEFAULT_COL_FAMILY);
     
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
  while (i < totalColCount)
    {
      const NAColumn * keyCol = baseTableKeyArr[j];

      // update column info for the index
      char * col_name = new(heap) char[strlen(keyCol->getColName().data()) + 2 + 1];
      strcpy(col_name, keyCol->getColName().data());
      colInfoArray[i].colName = col_name;

      colInfoArray[i].colNumber = i; 
      strcpy(colInfoArray[i].columnClass, COM_USER_COLUMN_LIT);

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
      
      retcode = getTypeInfo(naType, FALSE,
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
      colInfoArray[i].charset = charset;
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
       new(heap) char[strlen(SEABASE_DEFAULT_COL_FAMILY) +1];
     strcpy((char*)colInfoArray[i].hbaseColFam, SEABASE_DEFAULT_COL_FAMILY);
     
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
      else
	keyInfoArray[i].nonKeyCol = 0;

      keyInfoArray[i].hbaseColFam = new(heap) char[strlen(SEABASE_DEFAULT_COL_FAMILY) + 1];
      strcpy((char*)keyInfoArray[i].hbaseColFam, SEABASE_DEFAULT_COL_FAMILY);
      
      char qualNumStr[40];
      str_sprintf(qualNumStr, "@%d", keyInfoArray[i].keySeqNum);
      
      keyInfoArray[i].hbaseColQual = new(CTXTHEAP) char[strlen(qualNumStr)+1];
      strcpy((char*)keyInfoArray[i].hbaseColQual, qualNumStr);
      
      j++;
      i++;
    }

  return 0;
}

void CmpSeabaseDDL::createSeabaseIndex(
				       StmtDDLCreateIndex * createIndexNode,
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
  NAString extTableName = tableName.getExternalName(TRUE);

  ComObjectName indexName(createIndexNode->getIndexName());
  indexName.applyDefaults(btCatalogNamePart, btSchemaNamePart); 

  NAString catalogNamePart = indexName.getCatalogNamePartAsAnsiString();
  NAString schemaNamePart = indexName.getSchemaNamePartAsAnsiString(TRUE);
  NAString objectNamePart = indexName.getObjectNamePartAsAnsiString(TRUE);
  NAString extIndexName = indexName.getExternalName(TRUE);
  NAString extNameForHbase = catalogNamePart + "." + schemaNamePart + "." + objectNamePart;

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    return;

  if ((isSeabaseReservedSchema(indexName)) &&
      (!Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
    {
      *CmpCommon::diags() << DgSqlCode(-1118)
			  << DgTableName(extIndexName);
      deallocEHI(ehi); 
      return;
    }

  ExeCliInterface cliInterface(STMTHEAP);

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

        ComObjectName volTabName (qn->getQualifiedNameAsAnsiString());
	volTabName.applyDefaults(currCatAnsiName, currSchAnsiName);

	NAString vtCatNamePart = volTabName.getCatalogNamePartAsAnsiString();
	NAString vtSchNamePart = volTabName.getSchemaNamePartAsAnsiString(TRUE);
	NAString vtObjNamePart = volTabName.getObjectNamePartAsAnsiString(TRUE);
	
	retcode = existsInSeabaseMDTable(&cliInterface, 
					 vtCatNamePart, vtSchNamePart, vtObjNamePart,
					 COM_BASE_TABLE_OBJECT_LIT);

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

  ActiveSchemaDB()->getNATableDB()->useCache();

  // save the current parserflags setting
  ULng32 savedParserFlags = Get_SqlParser_Flags (0xFFFFFFFF);
  Set_SqlParser_Flags(ALLOW_VOLATILE_SCHEMA_IN_TABLE_NAME);

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  CorrName cn(tableName.getObjectNamePart().getInternalName(),
	      STMTHEAP,
	      btSchemaNamePart,
	      btCatalogNamePart);

  NATable *naTable = bindWA.getNATable(cn); 

  // Restore parser flags settings to what they originally were
  Set_SqlParser_Flags (savedParserFlags);

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
  
  retcode = existsInSeabaseMDTable(&cliInterface, 
				   catalogNamePart, schemaNamePart, objectNamePart, NULL);
  if (retcode < 0)
    {
      deallocEHI(ehi); 

      processReturn();

      return;
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

  Lng32 keyColCount = 0;
  Lng32 nonKeyColCount = 0;
  Lng32 totalColCount = 0;

  ComTdbVirtTableColumnInfo * colInfoArray = NULL;
  ComTdbVirtTableKeyInfo * keyInfoArray = NULL;

  NAList<NAString> selColList;

  if (createIndexColAndKeyInfoArrays(indexColRefArray,
				     createIndexNode->isUniqueSpecified(),
				     naTable->getClusteringIndex()->hasSyskey(),
				     naColArray,
				     baseTableKeyArr,
				     keyColCount,
				     nonKeyColCount,
				     totalColCount,
				     colInfoArray,
				     keyInfoArray,
				     selColList,
				     STMTHEAP))
    {
      deallocEHI(ehi); 
      
      processReturn();
      
      return;
    }

  ComTdbVirtTableTableInfo tableInfo;
  tableInfo.tableName = NULL,
  tableInfo.createTime = 0;
  tableInfo.redefTime = 0;
  tableInfo.objUID = 0;
  tableInfo.objOwnerID = naTable->getOwner(); 

  if (NOT createIndexNode->isNoPopulateOptionSpecified())
    // if index is to be populated during create index, then initially create it as an
    // unaudited index. That would avoid any transactional inserts.
    // After the index has been populated, make it audited.
    tableInfo.isAudited = 0;
  else
    tableInfo.isAudited = (nafs->isAudited() ? 1 : 0);

  tableInfo.validDef = 0;
  tableInfo.hbaseCreateOptions = NULL;

  ComTdbVirtTableIndexInfo ii;
  ii.baseTableName = (char*)extTableName.data();
  ii.indexName = (char*)extIndexName.data();
  ii.keytag = 1;
  ii.isUnique = createIndexNode->isUniqueSpecified() ? 1 : 0;
  ii.isExplicit = 1;
  ii.keyColCount = keyColCount;
  ii.nonKeyColCount = nonKeyColCount;
  ii.keyInfoArray = NULL; //keyInfoArray;
  
  Int64 objUID = -1;
  if (updateSeabaseMDTable(&cliInterface, 
			 catalogNamePart, schemaNamePart, objectNamePart,
			 COM_INDEX_OBJECT_LIT,
			 "N",
			 &tableInfo,
			 totalColCount,
			 colInfoArray,
			 totalColCount,
			 keyInfoArray,
			 1, // numIndex
                         &ii,
                         tableInfo.objOwnerID,
                         objUID))
    {
      deallocEHI(ehi); 

      processReturn();

      return;
    }

  if (createHbaseTable(ehi, &hbaseIndex, SEABASE_DEFAULT_COL_FAMILY, NULL, NULL) == -1)
    {
      deallocEHI(ehi); 

      processReturn();

      return;
    }

  if (NOT createIndexNode->isNoPopulateOptionSpecified())
    {
      NABoolean useLoad = (CmpCommon::getDefault(TRAF_LOAD_USE_FOR_INDEXES) == DF_ON);
    // populate index
      if (populateSeabaseIndexFromTable(&cliInterface,
					createIndexNode->isUniqueSpecified(),
					extIndexName, extTableName, selColList,
                                        useLoad))
	{
	  if (dropSeabaseObject(ehi, createIndexNode->getIndexName(),
				currCatName, currSchName, COM_INDEX_OBJECT_LIT))
	    {
	      processReturn();
	      
	      return;
	    }

	  deallocEHI(ehi); 

	  processReturn();

	  return;
	}
      
      if (updateObjectAuditAttr(&cliInterface, 
			       catalogNamePart, schemaNamePart, objectNamePart,
				TRUE, COM_INDEX_OBJECT_LIT))
	{
	  deallocEHI(ehi); 

	  processReturn();

	  return;
	}

      if (updateObjectValidDef(&cliInterface, 
			       catalogNamePart, schemaNamePart, objectNamePart,
			       COM_INDEX_OBJECT_LIT,
			       "Y"))
	{
	  deallocEHI(ehi); 

	  processReturn();

	  return;
	}
    }

  deallocEHI(ehi);

  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);

  //  processReturn();
  
  return;
}

short CmpSeabaseDDL::populateSeabaseIndexFromTable(
					  ExeCliInterface * cliInterface,
					  NABoolean isUnique,
					  const NAString &indexName, const NAString &tableName,
					  NAList<NAString> &selColList,
					  NABoolean useLoad)
{
  Lng32 cliRC = 0;
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

      query += "\"";
      query += colName;
      query += "\"";

      if (i < selColList.entries() - 1)
	query += ",";
    }

  query += " from ";
  query += tableName;
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

  SQL_EXEC_SetParserFlagsForExSqlComp_Internal(ALLOW_SPECIALTABLETYPE);
  SQL_EXEC_SetParserFlagsForExSqlComp_Internal(ALLOW_VOLATILE_SCHEMA_IN_TABLE_NAME);
  cliRC = cliInterface->executeImmediate(query);

  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
    }

  //  SQL_EXEC_ResetParserFlagsForExSqlComp_Internal(ALLOW_SPECIALTABLETYPE);
  //  SQL_EXEC_ResetParserFlagsForExSqlComp_Internal(ALLOW_VOLATILE_SCHEMA_IN_TABLE_NAME);
  SQL_EXEC_AssignParserFlagsForExSqlComp_Internal(savedCliParserFlags);

  cliInterface->restoreCQD("hide_indexes");
  cliInterface->restoreCQD("allow_dml_on_nonaudited_table");
  cliInterface->restoreCQD("attempt_esp_parallelism");

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

  ExeCliInterface cliInterface(STMTHEAP);

  //  If an index was created with NO POPULATE option, then
  // it will be marked as invalid in metadata. The regular table descriptor will
  // not have information about invalid indexes as we dont want those to
  // be used in queries.
  // Create a table descriptor that contains information on both valid and
  // invalid indexes. Pass that to getNATable method which will use this
  // table desc to create the NATable struct.
  desc_struct * tableDesc = 
    getSeabaseTableDesc(
		      tableName.getCatalogNamePart().getInternalName(),
		      tableName.getSchemaNamePart().getInternalName(),
		      tableName.getObjectNamePart().getInternalName(),
		      "BT",
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

  const NAFileSetList &indexList = naTable->getIndexList();
  for (Int32 i = 0; i < indexList.entries(); i++)
    {
      const NAFileSet * naf = indexList[i];
      if (naf->getKeytag() == 0)
	continue;
      
      const QualifiedName &qn = naf->getFileSetName();
      const NAString& nafIndexName =
	qn.getQualifiedNameAsAnsiString(TRUE);
      
      if ((populateIndexNode->populateAll()) ||
	  (extIndexName == nafIndexName))
	{
          if (populateIndexNode->populateAll())
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
				 qn.getObjectName().data(),
				 NULL,
				 TRUE);
	  if (isValid)
	    continue;


	  NAList<NAString> selColList;

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
	      
	      return;
	    }
	  NABoolean useLoad = (CmpCommon::getDefault(TRAF_LOAD_USE_FOR_INDEXES) == DF_ON);
	  if (populateSeabaseIndexFromTable(&cliInterface,
					    naf->uniqueIndex(),
					    nafIndexName, extTableName, selColList,
	                                    useLoad))
	    {
	      // need to purgedata seabase index. TBD.
	      
	      //	      if (cmpDropSeabaseObject(createIndexNode->getIndexName(),
	      //				     currCatName, currSchName, COM_INDEX_OBJECT_LIT))
	      //			return;

	      processReturn();

	      return;
	    }
	}
      
      if (updateObjectAuditAttr(&cliInterface, 
				catalogNamePart, schemaNamePart, objectNamePart,
				TRUE, COM_INDEX_OBJECT_LIT))
	{
	  processReturn();
	  
	  return;
	}
      
      if (updateObjectValidDef(&cliInterface, 
			       catalogNamePart, schemaNamePart, objectNamePart,
			       COM_INDEX_OBJECT_LIT,
			       "Y"))
	{
	  processReturn();

	  return;
	}
    } // for

  //  processReturn();
      
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

  ExeCliInterface cliInterface(STMTHEAP);

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
					 COM_INDEX_OBJECT_LIT);

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
				 COM_INDEX_OBJECT_LIT);
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
	    *CmpCommon::diags() << DgSqlCode(-1389)
				<< DgString0(objectNamePart);
	  else
	    *CmpCommon::diags() << DgSqlCode(-1389)
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
  if (getBaseTable(&cliInterface,
		   catalogNamePart, schemaNamePart, objectNamePart,
		   btCatName, btSchName, btObjName, btUID))
    {
      processReturn();
      
      deallocEHI(ehi); 
      
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
			currCatName, currSchName, COM_INDEX_OBJECT_LIT))
    {
      processReturn();

      deallocEHI(ehi); 

      return;
    }

  // remove NATable for the base table of this index
  CorrName cn(btObjName, STMTHEAP, btSchName, btCatName);
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);

  // remove NATable for this index in its real form as well as in its index_table
  // standalone format
  CorrName cni(objectNamePart, STMTHEAP, schemaNamePart, catalogNamePart);
  ActiveSchemaDB()->getNATableDB()->removeNATable(cni);
  cni.setSpecialType(ExtendedQualName::INDEX_TABLE);
  ActiveSchemaDB()->getNATableDB()->removeNATable(cni);
  
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

  const NAString &idxName = 
    (isDisable ? alterDisableIndexNode->getIndexName() : alterEnableIndexNode->getIndexName());

  ComObjectName indexName(idxName);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  indexName.applyDefaults(currCatAnsiName, currSchAnsiName);

  const NAString catalogNamePart = indexName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = indexName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = indexName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extIndexName = indexName.getExternalName(TRUE);

  ExeCliInterface cliInterface(STMTHEAP);

  retcode = existsInSeabaseMDTable(&cliInterface, 
				 catalogNamePart, schemaNamePart, objectNamePart,
				 COM_INDEX_OBJECT_LIT);
  if (retcode < 0)
    {
      processReturn();

      return;
    }

  if (retcode == 0) // does not exist
    {
      *CmpCommon::diags() << DgSqlCode(-1389)
			  << DgString0(extIndexName);

      processReturn();

      return;
    }

  // get base table name
  NAString btCatName;
  NAString btSchName;
  NAString btObjName;
  Int64 btUID;
  if (getBaseTable(&cliInterface,
		   catalogNamePart, schemaNamePart, objectNamePart,
		   btCatName, btSchName, btObjName, btUID))
    {
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

  // remove NATable for the base table of this index
  CorrName cn(btObjName, STMTHEAP, btSchName, btCatName);
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);

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


  ExeCliInterface cliInterface(STMTHEAP);

  sprintf (buf, " ALTER TABLE \"%s\".\"%s\".\"%s\"  %s INDEX %s ;", catName, schName, tabName,
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
                                             NAString &tabName)
{
  Lng32 cliRC = 0;
  char buf[4000];

  NABoolean isDisable =
    (ddlNode->getOperatorType() == DDL_ALTER_TABLE_DISABLE_INDEX);

  ComObjectName tableName(tabName);

  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);

  CMPASSERT (catalogNamePart == currCatName);
  CMPASSERT (schemaNamePart == currSchName);

  ExeCliInterface cliInterface(STMTHEAP);

  str_sprintf(buf,
      " select catalog_name,schema_name,object_name from  %s.\"%s\".%s  " \
      " where object_uid in ( select i.index_uid from " \
      " %s.\"%s\".%s i " \
      " join    %s.\"%s\".%s  o2 on i.base_table_uid=o2.object_uid " \
      " where  o2.catalog_name= '%s' AND o2.schema_name='%s' AND o2.Object_Name='%s') " \
      " and object_type='IX' ; ",
      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_INDEXES,
      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
      catalogNamePart.data(),schemaNamePart.data(), objectNamePart.data()  //table name in this case
      );

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
    for (int idx = 0; idx < indexes->numEntries(); idx++)
    {
      OutputInfo * idx = (OutputInfo*) indexes->getNext();

      catName = idx->get(0);
      schName = idx->get(1);
      char * idxName = idx->get(2);

      if (alterSeabaseTableDisableOrEnableIndex ( catName, schName, idxName, objectNamePart, isDisable))
        return;
    }
    CorrName cn( objectNamePart, STMTHEAP, NAString(schName), NAString(catName));
    ActiveSchemaDB()->getNATableDB()->removeNATable(cn);
  }

  return ;
}

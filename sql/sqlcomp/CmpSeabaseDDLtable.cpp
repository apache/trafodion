/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2014 Hewlett-Packard Development Company, L.P.
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
 * File:         CmpSeabaseDDLtable.cpp
 * Description:  Implements ddl operations for Seabase tables.
 *
 *
 * Created:     6/30/2013
 * Language:     C++
 *
 *
 *****************************************************************************
 */


#include "CmpSeabaseDDLincludes.h"
#include "ElemDDLColDefault.h"
#include "NumericType.h"
#include "ComUser.h"
#include "keycolumns.h"
#include "ElemDDLColRef.h"
#include "ElemDDLColName.h"

#include "CmpDDLCatErrorCodes.h"
#include "Globals.h"
#include "CmpMain.h"

// defined in CmpDescribe.cpp
extern short CmpDescribeSeabaseTable ( 
			     const CorrName  &dtName,
			     short type, // 1, invoke. 2, showddl. 3, createLike
			     char* &outbuf,
			     ULng32 &outbuflen,
			     CollHeap *heap,
			     const char * pkeyStr = NULL,
			     NABoolean withPartns = FALSE,
			     NABoolean noTrailingSemi = FALSE);

static void convertVirtTableColumnInfoToDescStruct( 
     const ComTdbVirtTableColumnInfo * colInfo,
     const ComObjectName * objectName,
     desc_struct * column_desc)
{
  column_desc->body.columns_desc.tablename = 
    convertNAString(objectName->getExternalName(), STMTHEAP);
  
  char * col_name = new(STMTHEAP) char[strlen(colInfo->colName) + 1];
  strcpy(col_name, colInfo->colName);
  column_desc->body.columns_desc.colname = col_name;
  column_desc->body.columns_desc.colnumber = colInfo->colNumber;
  column_desc->body.columns_desc.datatype  = colInfo->datatype;
  column_desc->body.columns_desc.length    = colInfo->length;
  if (!(DFS2REC::isInterval(colInfo->datatype) || DFS2REC::isDateTime(colInfo->datatype)))
    column_desc->body.columns_desc.scale     = colInfo->scale;
  else
    column_desc->body.columns_desc.scale = 0;
  column_desc->body.columns_desc.precision = colInfo->precision;
  column_desc->body.columns_desc.datetimestart = (rec_datetime_field) colInfo->dtStart;
  column_desc->body.columns_desc.datetimeend = (rec_datetime_field) colInfo->dtEnd;
  if (DFS2REC::isDateTime(colInfo->datatype))
    column_desc->body.columns_desc.datetimefractprec = colInfo->scale;
  else
    column_desc->body.columns_desc.datetimefractprec = 0;
  if (DFS2REC::isInterval(colInfo->datatype))
    column_desc->body.columns_desc.intervalleadingprec = colInfo->precision;
  else
    column_desc->body.columns_desc.intervalleadingprec = 0 ;
  column_desc->body.columns_desc.null_flag = colInfo->nullable;
  column_desc->body.columns_desc.upshift   = colInfo->upshifted;
  column_desc->body.columns_desc.character_set = (CharInfo::CharSet) colInfo->charset;
  column_desc->body.columns_desc.colclass = colInfo->columnClass[0];
  column_desc->body.columns_desc.defaultClass = colInfo->defaultClass;
  
  
  column_desc->body.columns_desc.pictureText =
    (char *)STMTHEAP->allocateMemory(340);
  NAType::convertTypeToText(column_desc->body.columns_desc.pictureText,	 //OUT
			    column_desc->body.columns_desc.datatype,
			    column_desc->body.columns_desc.length,
			    column_desc->body.columns_desc.precision,
			    column_desc->body.columns_desc.scale,
			    column_desc->body.columns_desc.datetimestart,
			    column_desc->body.columns_desc.datetimeend,
			    column_desc->body.columns_desc.datetimefractprec,
			    column_desc->body.columns_desc.intervalleadingprec,
			    column_desc->body.columns_desc.upshift,
			    column_desc->body.columns_desc.caseinsensitive,
			    (CharInfo::CharSet)column_desc->body.columns_desc.character_set,
                            (CharInfo::Collation) 1, // default collation
                            NULL, // displayDataType
                            0); // displayCaseSpecific
  

  column_desc->body.columns_desc.offset    = -1; // not present in colInfo
  column_desc->body.columns_desc.caseinsensitive = (short)FALSE; // not present in colInfo
  column_desc->body.columns_desc.encoding_charset = (CharInfo::CharSet) column_desc->body.columns_desc.character_set ; // not present in colInfo so we go with the column's charset here. 
  column_desc->body.columns_desc.collation_sequence = (CharInfo::Collation)1; // not present in colInfo, so we go with default collation here (used in buildEncodeTree for some error handling)
  column_desc->body.columns_desc.uec     = (Cardinality)0; // not present in colInfo
  column_desc->body.columns_desc.highval = 0; // not present in colInfo
  column_desc->body.columns_desc.lowval  = 0; // not present in colInfo
  column_desc->body.columns_desc.defaultvalue = NULL ; // not present in colInfo
  column_desc->body.columns_desc.stored_on_disk = 0 ; // not present in colInfo
  column_desc->body.columns_desc.computed_column_text = NULL; // not present in colInfo
}

static desc_struct * convertVirtTableColumnInfoArrayToDescStructs(
     const ComObjectName * objectName,
     const ComTdbVirtTableColumnInfo * colInfoArray,
     Lng32 numCols)
{
  desc_struct * prev_column_desc  = NULL;
  desc_struct * first_column_desc = NULL;
  for (Int32 i = 0; i < numCols; i++)
  {
    const ComTdbVirtTableColumnInfo* colInfo = &(colInfoArray[i]);

    // readtabledef_allocate_desc() requires that HEAP (STMTHEAP) 
    // be used for operator new herein
    desc_struct * column_desc = readtabledef_allocate_desc(DESC_COLUMNS_TYPE);
    if (prev_column_desc != NULL)
      prev_column_desc->header.next = column_desc;
    else
      first_column_desc = column_desc;      
    
    prev_column_desc = column_desc;
    convertVirtTableColumnInfoToDescStruct(colInfo, objectName, column_desc);
  }

  return first_column_desc;
}

static desc_struct * convertVirtTableKeyInfoArrayToDescStructs(
     const ComTdbVirtTableKeyInfo *keyInfoArray,
     const ComTdbVirtTableColumnInfo *colInfoArray,
     Lng32 numKeys)
{
  desc_struct * prev_key_desc  = NULL;
  desc_struct * first_key_desc = NULL;
  for (Int32 i = 0; i < numKeys; i++)
    {
      const ComTdbVirtTableColumnInfo * colInfo = &(colInfoArray[keyInfoArray[i].tableColNum]);
      desc_struct * key_desc = readtabledef_allocate_desc(DESC_KEYS_TYPE);
      if (prev_key_desc != NULL)
	prev_key_desc->header.next = key_desc;
      else
       first_key_desc = key_desc;      
      
      prev_key_desc = key_desc;
      
      key_desc->body.keys_desc.tablecolnumber = keyInfoArray[i].tableColNum;
      key_desc->body.keys_desc.keyseqnumber = i;
      key_desc->body.keys_desc.ordering = keyInfoArray[i].ordering;
    }

  return first_key_desc;
}



void CmpSeabaseDDL::createSeabaseTableLike(
					   StmtDDLCreateTable * createTableNode,
					   NAString &currCatName, NAString &currSchName)
{
  Lng32 retcode = 0;

  ComObjectName tgtTableName(createTableNode->getTableName(), COM_TABLE_NAME);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tgtTableName.applyDefaults(currCatAnsiName, currSchAnsiName);
  const NAString extTgtTableName = tgtTableName.getExternalName(TRUE);
  
  ComObjectName srcTableName(createTableNode->getLikeSourceTableName(), COM_TABLE_NAME);
  srcTableName.applyDefaults(currCatAnsiName, currSchAnsiName);
  
  CorrName cn(srcTableName.getObjectNamePart().getInternalName(),
	      STMTHEAP,
	      srcTableName.getSchemaNamePart().getInternalName(),
	      srcTableName.getCatalogNamePart().getInternalName());

  ElemDDLColRefArray &keyArray = 
    (createTableNode->getIsConstraintPKSpecified() ?
     createTableNode->getPrimaryKeyColRefArray() :
     (createTableNode->getStoreOption() == COM_KEY_COLUMN_LIST_STORE_OPTION ?
      createTableNode->getKeyColumnArray() :
      createTableNode->getPrimaryKeyColRefArray()));

  NAString keyClause;
  if ((keyArray.entries() > 0) &&
      ((createTableNode->getIsConstraintPKSpecified()) ||
       (createTableNode->getStoreOption() == COM_KEY_COLUMN_LIST_STORE_OPTION)))
    {
      if (createTableNode->getIsConstraintPKSpecified())
	keyClause = " primary key ( ";
      else if (createTableNode->getStoreOption() == COM_KEY_COLUMN_LIST_STORE_OPTION)
	keyClause = " store by ( ";
      
      for (CollIndex i = 0; i < keyArray.entries(); i++) 
	{
	  const NAString &colName = keyArray[i]->getColumnName();
	  
	  keyClause += colName;

	  if (i < (keyArray.entries() - 1))
	    keyClause += ", ";
	}

      keyClause += ")";
    }

  ParDDLLikeOptsCreateTable &likeOptions = createTableNode->getLikeOptions();
  
  char * buf = NULL;
  ULng32 buflen = 0;
  retcode = CmpDescribeSeabaseTable(cn, 3/*createlike*/, buf, buflen, STMTHEAP,
				    NULL, likeOptions.getIsWithHorizontalPartitions(), TRUE);
  if (retcode)
    return;

  NAString query = "create table ";
  query += extTgtTableName;
  query += " ";

  NABoolean done = FALSE;
  Lng32 curPos = 0;
  while (NOT done)
    {
      short len = *(short*)&buf[curPos];
      NAString frag(&buf[curPos+sizeof(short)],
		    len - ((buf[curPos+len-1]== '\n') ? 1 : 0));

      query += frag;
      curPos += ((((len+sizeof(short))-1)/8)+1)*8;

      if (curPos >= buflen)
	done = TRUE;
    }

  if (NOT keyClause.isNull())
    {
      // add the keyClause
      query += keyClause;
    }

  ExeCliInterface cliInterface(STMTHEAP);

  Lng32 cliRC = 0;

  cliRC = cliInterface.executeImmediate((char*)query.data());
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return;
    }
  
  return;
}

short CmpSeabaseDDL::genPKeyName(StmtDDLAddConstraintPK *addPKNode,
				 const char * catName,
				 const char * schName,
				 const char * objName,
				 NAString &pkeyName)
{
  
  ComObjectName tableName( (addPKNode ? addPKNode->getTableName() : " "), COM_TABLE_NAME);

  ElemDDLConstraintPK *constraintNode = 
    (addPKNode ? (addPKNode->getConstraint())->castToElemDDLConstraintPK() : NULL);

  ComString specifiedConstraint;
  ComString constraintName;
  if( !constraintNode || (constraintNode->getConstraintName().isNull()))
    {
      specifiedConstraint.append( catName); 
      specifiedConstraint.append(".");
      specifiedConstraint.append("\"");
      specifiedConstraint.append( schName); 
      specifiedConstraint.append("\"");
      specifiedConstraint.append(".");

      ComString oName = "\"";
      oName += objName; 
      oName += "\"";
      Lng32 status = ToInternalIdentifier ( oName // in/out - from external- to internal-format
					    , TRUE  // in - NABoolean upCaseInternalNameIfRegularIdent
					    , TRUE  // in - NABoolean acceptCircumflex
					    );
      ComDeriveRandomInternalName ( ComGetNameInterfaceCharSet()
				    , /*internalFormat*/oName          // in  - const ComString &
				    , /*internalFormat*/constraintName // out - ComString &
				    , STMTHEAP
				    );

      // Generate a delimited identifier if objectName was delimited
      constraintName/*InExternalFormat*/ = ToAnsiIdentifier (constraintName/*InInternalFormat*/);

      specifiedConstraint.append(constraintName);
    }
  else
    {
      specifiedConstraint = constraintNode->
        getConstraintNameAsQualifiedName().getQualifiedNameAsAnsiString();
    }

  pkeyName = specifiedConstraint;

  return 0;
}

short CmpSeabaseDDL::updatePKeyInfo(
				    StmtDDLAddConstraintPK *addPKNode,
				    const char * catName,
				    const char * schName,
				    const char * objName,
				    Lng32 numKeys,
				    Int64 * outPkeyUID,
				    Int64 * outTableUID,
				    const ComTdbVirtTableKeyInfo * keyInfoArray,
				    ExeCliInterface *cliInterface)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  char buf[4000];

  // update primary key constraint info
  NAString pkeyStr;
  if (genPKeyName(addPKNode, catName, schName, objName, pkeyStr))
    {
      return -1;
    }

  Int64 createTime = NA_JulianTimestamp();

  ComUID comUID;
  comUID.make_UID();
  Int64 pkeyUID = comUID.get_value();
  if (outPkeyUID)
    *outPkeyUID = pkeyUID;

  ComObjectName pkeyName(pkeyStr);
  const NAString catalogNamePart = pkeyName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = pkeyName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = pkeyName.getObjectNamePartAsAnsiString(TRUE);
  NAString quotedSchName;
  ToQuotedString(quotedSchName, NAString(schemaNamePart), FALSE);
  NAString quotedObjName;
  ToQuotedString(quotedObjName, NAString(objectNamePart), FALSE);

  str_sprintf(buf, "insert into %s.\"%s\".%s values ('%s', '%s', '%s', '%s', %Ld, %Ld, %Ld, '%s', %d )",
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
	      catalogNamePart.data(), quotedSchName.data(), quotedObjName.data(),
	      COM_PRIMARY_KEY_CONSTRAINT_OBJECT_LIT,
	      pkeyUID,
	      createTime, 
	      createTime,
	      " ",
              SUPER_USER);
  cliRC = cliInterface->executeImmediate(buf);
  
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

      *CmpCommon::diags() << DgSqlCode(-1423)
			  << DgString0(SEABASE_OBJECTS);

      return -1;
    }

  Int64 tableUID = 
    getObjectUID(cliInterface,
		 catName, schName, objName,
		 COM_BASE_TABLE_OBJECT_LIT);

  if (outTableUID)
    *outTableUID = tableUID;

  Int64 indexUID = 0;
  str_sprintf(buf, "insert into %s.\"%s\".%s values (%Ld, %Ld, '%s', %d, %Ld )",
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLE_CONSTRAINTS,
	      tableUID, pkeyUID,
	      COM_PRIMARY_KEY_CONSTRAINT_LIT,
	      numKeys,
	      indexUID);
  cliRC = cliInterface->executeImmediate(buf);
  
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

      *CmpCommon::diags() << DgSqlCode(-1423)
			  << DgString0(SEABASE_TABLE_CONSTRAINTS);

      return -1;
    }

  if (keyInfoArray)
    {
      for (Lng32 i = 0; i < numKeys; i++)
	{
	  str_sprintf(buf, "insert into %s.\"%s\".%s values (%Ld, '%s', %d, %d, %d, %d)",
		      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_KEYS,
		      pkeyUID,
		      keyInfoArray[i].colName,
		      i+1,
		      keyInfoArray[i].tableColNum,
		      0,
		      0);
	  
	  cliRC = cliInterface->executeImmediate(buf);
	  if (cliRC < 0)
	    {
	      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
	      
	      *CmpCommon::diags() << DgSqlCode(-1423)
				  << DgString0(SEABASE_KEYS);

	      return -1;
	    }
	}
    }

  return 0;
}

short CmpSeabaseDDL::constraintErrorChecks(
					   StmtDDLAddConstraint *addConstrNode,
					   NATable * naTable,
					   ComConstraintType ct,
					   NAList<NAString> &keyColList)
{

  const NAString &addConstrName = addConstrNode->
    getConstraintNameAsQualifiedName().getQualifiedNameAsAnsiString();

  NABoolean foundConstr = FALSE;
  if (ct == COM_CHECK_CONSTRAINT)
    {
      const CheckConstraintList &checkList = naTable->getCheckConstraints();
      
      for (Int32 i = 0; i < checkList.entries(); i++)
	{
	  CheckConstraint *checkConstr = (CheckConstraint*)checkList[i];
	  
	  const NAString &tableConstrName = 
	    checkConstr->getConstraintName().getQualifiedNameAsAnsiString();
	  
	  if (addConstrName == tableConstrName)
	    {
	      foundConstr = TRUE;
	    }
	} // for
  
     if (foundConstr)
	{
	  *CmpCommon::diags()
	    << DgSqlCode(-1043)
	    << DgConstraintName(addConstrName);
	  
	  processReturn();
	  
	  return -1;
	}
 
    }
  else if ((ct == COM_UNIQUE_CONSTRAINT) || 
	   (ct == COM_FOREIGN_KEY_CONSTRAINT) ||
	   (ct == COM_PRIMARY_KEY_CONSTRAINT))
    {
      const AbstractRIConstraintList &ariList = 
	((ct == COM_UNIQUE_CONSTRAINT) ? naTable->getUniqueConstraints() : 
	 naTable->getRefConstraints());
      for (Int32 i = 0; i < ariList.entries(); i++)
	{
	  AbstractRIConstraint *ariConstr = (AbstractRIConstraint*)ariList[i];
	  
	  const NAString &tableConstrName = 
	    ariConstr->getConstraintName().getQualifiedNameAsAnsiString();
	  
	  if (addConstrName == tableConstrName)
	    {
	      foundConstr = TRUE;
	    }
	} // for
      
      if (foundConstr)
	{
	  *CmpCommon::diags()
	    << DgSqlCode(-1043)
	    << DgConstraintName(addConstrName);
	  
	  processReturn();
	  
	  return -1;
	}
      
      for (CollIndex ndx = 0; ndx < keyColList.entries(); ndx++)
	{
	  const ComString intColName = keyColList[ndx];
	  if (intColName EQU "SYSKEY")
	    {
	      const NAString &tabName = addConstrNode->getTableName();
	      
	      *CmpCommon::diags() << 
		DgSqlCode(((ct == COM_UNIQUE_CONSTRAINT) ? -CAT_SYSKEY_COL_NOT_ALLOWED_IN_UNIQUE_CNSTRNT :
			   -CAT_SYSKEY_COL_NOT_ALLOWED_IN_RI_CNSTRNT))
				  << DgColumnName( ToAnsiIdentifier( intColName ))
				  << DgTableName(tabName);
	      
	      processReturn();
	      
	      return -1;
	    }
	}
      
      const NAColumnArray & naColArray = naTable->getNAColumnArray();
      
      // Now process each column defined in the parseColList to see if
      // it exists in the table column list and it isn't a duplicate.
      NABitVector seenIt; 
      NAString keyColNameStr;
      for (CollIndex i = 0; i < keyColList.entries(); i++)
	{
	  NAColumn * nac = naColArray.getColumn(keyColList[i]);
	  if (! nac)
	    {
	      *CmpCommon::diags() << DgSqlCode(-1009)
				  << DgColumnName( ToAnsiIdentifier(keyColList[i]));
	      return -1;
	    }
	  
	  Lng32 colNumber = nac->getPosition();
	  
	  // If the column has already been found, return error
	  if( seenIt.contains(colNumber)) 
	    {
	      *CmpCommon::diags() << DgSqlCode(-CAT_REDUNDANT_COLUMN_REF_PK)
				  << DgColumnName( ToAnsiIdentifier(keyColList[i]));
	      return -1;
	    }
	  
	  seenIt.setBit(colNumber);
	}

      if (ct == COM_UNIQUE_CONSTRAINT)      
	{
	  // Compare the column list from parse tree to the unique and primary
	  // key constraints already defined for the table.  The new unique 
	  // constraint list must be distinct.  The order of the existing constraint
	  // does not have to be in the same order as the new constraint.
	  //
	  NABoolean matchFound = FALSE;
	  
	  if (naTable->getCorrespondingConstraint(keyColList,
						  TRUE, // unique constraint
						  NULL))
	    matchFound = TRUE;
	  
	  if (matchFound)
	    {
	      *CmpCommon::diags() << DgSqlCode(-CAT_DUPLICATE_UNIQUE_CONSTRAINT_ON_SAME_COL);
	      
	      return -1;
	    }
	}
    }

  return 0;
}
			
short CmpSeabaseDDL::genUniqueName(StmtDDLAddConstraint *addUniqueNode,
				 NAString &uniqueName)
{
  
  ComObjectName tableName( addUniqueNode->getTableName(), COM_TABLE_NAME);

  ElemDDLConstraint *constraintNode = 
    (addUniqueNode->getConstraint())->castToElemDDLConstraint();

  ComString specifiedConstraint;
  ComString constraintName;
  if( constraintNode->getConstraintName().isNull() )
    {
      specifiedConstraint.append( tableName.getCatalogNamePartAsAnsiString() );
      specifiedConstraint.append(".");
      specifiedConstraint.append( tableName.getSchemaNamePartAsAnsiString() );
      specifiedConstraint.append(".");

      ComString oName = tableName.getObjectNamePartAsAnsiString() ; 
      Lng32 status = ToInternalIdentifier ( oName // in/out - from external- to internal-format
					    , TRUE  // in - NABoolean upCaseInternalNameIfRegularIdent
					    , TRUE  // in - NABoolean acceptCircumflex
					    );
      ComDeriveRandomInternalName ( ComGetNameInterfaceCharSet()
				    , /*internalFormat*/oName          // in  - const ComString &
				    , /*internalFormat*/constraintName // out - ComString &
				    , STMTHEAP
				    );

      // Generate a delimited identifier if objectName was delimited
      constraintName/*InExternalFormat*/ = ToAnsiIdentifier (constraintName/*InInternalFormat*/);

      specifiedConstraint.append(constraintName);
    }
  else
    {
      specifiedConstraint = constraintNode->
        getConstraintNameAsQualifiedName().getQualifiedNameAsAnsiString();
    }

  uniqueName = specifiedConstraint;

  return 0;
}

short CmpSeabaseDDL::updateConstraintMD(
					NAList<NAString> &keyColList,
					NAList<NAString> &keyColOrderList,
					NAString &uniqueStr,
					Int64 tableUID,
					Int64 constrUID,
					NATable * naTable,
					ComConstraintType ct,
					ExeCliInterface *cliInterface)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  char buf[4000];

  const NAColumnArray & naColArray = naTable->getNAColumnArray();

  Int64 createTime = NA_JulianTimestamp();

  ComObjectName uniqueName(uniqueStr);
  const NAString catalogNamePart = uniqueName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = uniqueName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = uniqueName.getObjectNamePartAsAnsiString(TRUE);
  NAString quotedSchName;
  ToQuotedString(quotedSchName, NAString(schemaNamePart), FALSE);
  NAString quotedObjName;
  ToQuotedString(quotedObjName, NAString(objectNamePart), FALSE);

  str_sprintf(buf, "insert into %s.\"%s\".%s values ('%s', '%s', '%s', '%s', %Ld, %Ld, %Ld, '%s', %d )",
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
	      catalogNamePart.data(), quotedSchName.data(), quotedObjName.data(),
	      ((ct == COM_UNIQUE_CONSTRAINT) ? COM_UNIQUE_CONSTRAINT_OBJECT_LIT :
	       ((ct == COM_FOREIGN_KEY_CONSTRAINT) ? COM_REFERENTIAL_CONSTRAINT_OBJECT_LIT : COM_CHECK_CONSTRAINT_OBJECT_LIT)),
	      constrUID,
	      createTime, 
	      createTime,
	      " ",
              SUPER_USER);
  cliRC = cliInterface->executeImmediate(buf);
  
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  Int64 indexUID = 0;
  str_sprintf(buf, "insert into %s.\"%s\".%s values (%Ld, %Ld, '%s', %d, %Ld )",
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLE_CONSTRAINTS,
	      tableUID, constrUID,
	      ((ct == COM_UNIQUE_CONSTRAINT) ? COM_UNIQUE_CONSTRAINT_LIT :
	       ((ct == COM_FOREIGN_KEY_CONSTRAINT) ? COM_FOREIGN_KEY_CONSTRAINT_LIT : COM_CHECK_CONSTRAINT_LIT)),
	      keyColList.entries(),
	      indexUID);
  cliRC = cliInterface->executeImmediate(buf);
  
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  for (Lng32 i = 0; i < keyColList.entries(); i++)
    {
      NAColumn * nac = naColArray.getColumn(keyColList[i]);
      Lng32 colNumber = nac->getPosition();

      str_sprintf(buf, "insert into %s.\"%s\".%s values (%Ld, '%s', %d, %d, %d, %d)",
		  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_KEYS,
		  constrUID,
		  keyColList[i].data(),
		  i+1,
		  colNumber,
		  (keyColOrderList[i] == "DESC" ? 1 : 0),
		  0);
      
      cliRC = cliInterface->executeImmediate(buf);
      if (cliRC < 0)
	{
	  cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

	  return -1;
	}
    }

  return 0;
}

short CmpSeabaseDDL::updateRIConstraintMD(
					  Int64 ringConstrUID,
					  Int64 refdConstrUID,
					  ExeCliInterface *cliInterface)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  char buf[4000];

  str_sprintf(buf, "insert into %s.\"%s\".%s values (%Ld, %Ld, '%s', '%s', '%s' )",
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_REF_CONSTRAINTS,
	      ringConstrUID, refdConstrUID,
	      COM_FULL_MATCH_OPTION_LIT,
	      COM_RESTRICT_UPDATE_RULE_LIT,
	      COM_RESTRICT_DELETE_RULE_LIT);
  cliRC = cliInterface->executeImmediate(buf);
  
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  str_sprintf(buf, "insert into %s.\"%s\".%s values (%Ld, %Ld)",
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_UNIQUE_REF_CONSTR_USAGE,
	      refdConstrUID, ringConstrUID);
  cliRC = cliInterface->executeImmediate(buf);
  
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  return 0;
}

short CmpSeabaseDDL::updateIndexInfo(
				     NAList<NAString> &ringKeyColList,
				     NAList<NAString> &ringKeyColOrderList,
				     NAList<NAString> &refdKeyColList,
				     NAString &uniqueStr,
				     Int64 constrUID,
				     const char * catName,
				     const char * schName,
				     const char * objName,
				     NATable * naTable,
				     NABoolean isUnique, // TRUE: uniq constr. FALSE: ref constr.
				     NABoolean noPopulate,
				     ExeCliInterface *cliInterface)
{
  // Now we need to determine if an index has to be created for
  // the unique or ref constraint.  
  NABoolean createIndex = TRUE;

  NAString existingIndexName;
  if (naTable->getCorrespondingIndex(ringKeyColList, 
				     TRUE, // explicit index only
				     isUnique, //TRUE, look for unique index.
				     TRUE, //isUnique, //TRUE, look for primary key.
				     (NOT isUnique), // TRUE, look for any index or pkey
				     &existingIndexName))
    createIndex = FALSE;
  
  ComObjectName indexName(createIndex ? uniqueStr : existingIndexName);
  const NAString catalogNamePart = indexName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = indexName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = indexName.getObjectNamePartAsAnsiString(TRUE);
  NAString quotedSchName;
  ToQuotedString(quotedSchName, NAString(schemaNamePart), FALSE);
  NAString quotedObjName;
  ToQuotedString(quotedObjName, NAString(objectNamePart), FALSE);

  char buf[5000];
  Lng32 cliRC;

  Int64 tableUID = naTable->objectUid().castToInt64();
  
  if (createIndex)
    {
      NAString keyColNameStr;
      for (CollIndex i = 0; i < ringKeyColList.entries(); i++)
	{
	  keyColNameStr += "\"";
	  keyColNameStr += ringKeyColList[i];
	  keyColNameStr += "\" ";
	  keyColNameStr += ringKeyColOrderList[i];
	  if (i+1 < ringKeyColList.entries())
	    keyColNameStr += ", ";
	}

      char noPopStr[100];
      if (noPopulate)
	strcpy(noPopStr, " no populate ");
      else
	strcpy(noPopStr, " ");

      if (isUnique)
	str_sprintf(buf, "create unique index \"%s\" on \"%s\".\"%s\".\"%s\" ( %s ) %s",
		    quotedObjName.data(),
		    catName, schName, objName,
		    keyColNameStr.data(),
		    noPopStr);
      else
	str_sprintf(buf, "create index \"%s\" on \"%s\".\"%s\".\"%s\" ( %s ) %s",
		    quotedObjName.data(),
		    catName, schName, objName,
		    keyColNameStr.data(),
		    noPopStr);
	
      cliRC = cliInterface->executeImmediate(buf);
      
      if (cliRC < 0)
	{
	  cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
	  return -1;
	}

      // update indexes table and mark this index as an implicit index.
      str_sprintf(buf, "update %s.\"%s\".%s set is_explicit = 0 where base_table_uid = %Ld and index_uid = (select object_uid from %s.\"%s\".%s where catalog_name = '%s' and schema_name = '%s' and object_name = '%s' and object_type = 'IX') ",
		  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_INDEXES,
		  tableUID,
		  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
		  catName, schemaNamePart.data(), objectNamePart.data());
     cliRC = cliInterface->executeImmediate(buf);
      if (cliRC < 0)
	{
	  cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

	  return -1;
	}

      if (noPopulate)
	{
	  if (updateObjectValidDef(cliInterface, 
				   catalogNamePart, schemaNamePart, objectNamePart,
				   COM_INDEX_OBJECT_LIT,
				   "Y"))
	    {
	      return -1;
	    }
	}
    }

  // update table_constraints table with the uid of this index.
  Int64 indexUID = 
    getObjectUID(cliInterface,
		 catName, schemaNamePart, objectNamePart,
		 COM_INDEX_OBJECT_LIT);
  if (indexUID < 0)
    {
      // primary key. Clear diags area since getObjectUID sets up diags entry.
      CmpCommon::diags()->clear();
    }

  str_sprintf(buf, "update %s.\"%s\".%s set index_uid = %Ld where table_uid = %Ld and constraint_uid = %Ld and constraint_type = '%s'",
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLE_CONSTRAINTS,
	      indexUID,
	      tableUID, constrUID,
	      (isUnique ? COM_UNIQUE_CONSTRAINT_LIT : COM_FOREIGN_KEY_CONSTRAINT_LIT));
  cliRC = cliInterface->executeImmediate(buf);
  
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  return 0;
}

void CmpSeabaseDDL::createSeabaseTable(
				       StmtDDLCreateTable * createTableNode,
				       NAString &currCatName, NAString &currSchName)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  ComObjectName tableName(createTableNode->getTableName());
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);
  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);
  const NAString extNameForHbase = catalogNamePart + "." + schemaNamePart + "." + objectNamePart;
  
  ElemDDLColDefArray &colArray = createTableNode->getColDefArray();
  ElemDDLColRefArray &keyArray = 
    (createTableNode->getIsConstraintPKSpecified() ?
     createTableNode->getPrimaryKeyColRefArray() :
     (createTableNode->getStoreOption() == COM_KEY_COLUMN_LIST_STORE_OPTION ?
      createTableNode->getKeyColumnArray() :
      createTableNode->getPrimaryKeyColRefArray()));

  NABoolean trustedCaller = FALSE;
  if ((isSeabaseReservedSchema(tableName)) &&
      (Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
    trustedCaller = TRUE;

  // Verify that the current user has authority to perform operation
  ComUserVerifyObj verifyAuth(tableName, ComUserVerifyObj::OBJ_OBJ_TYPE);
  if (!verifyAuth.isAuthorized(ComUser::CREATE_TABLE, trustedCaller))
  {
    *CmpCommon::diags() << DgSqlCode(-1017);
    return;
  }

  Int32 objOwner = verifyAuth.getEffectiveUser(ComUser::CREATE_TABLE);
  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();

      return;
    }

  if ((isSeabaseReservedSchema(tableName)) &&
      (!Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_CREATE_TABLE_NOT_ALLOWED_IN_SMD)
			  << DgTableName(extTableName);
      deallocEHI(ehi); 

      processReturn();

      return;
    }

  ExeCliInterface cliInterface(STMTHEAP);

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
      if (NOT createTableNode->createIfNotExists())
	{
	  if (createTableNode->isVolatile())
	    *CmpCommon::diags() << DgSqlCode(-1390)
				<< DgString0(objectNamePart);
	  else
	    *CmpCommon::diags() << DgSqlCode(-1390)
				<< DgString0(extTableName);
	}

      deallocEHI(ehi); 

      processReturn();

      return;
    }

  if (createTableNode->getIsLikeOptionSpecified())
    {
      createSeabaseTableLike(createTableNode, currCatName, currSchName);

      processReturn();

      return;
    }

  // check if SYSKEY is specified as a column name.
  NABoolean explicitSyskeySpecified = FALSE;
  for (Lng32 i = 0; i < colArray.entries(); i++)
    {
      if (colArray[i]->getColumnName() == "SYSKEY")
	explicitSyskeySpecified = TRUE;
    }

  NABoolean implicitPK = FALSE;

  NAString syskeyColName("SYSKEY");
  SQLLargeInt * syskeyType = new(STMTHEAP) SQLLargeInt(TRUE, FALSE, STMTHEAP);
  ElemDDLColDef syskeyColDef(syskeyColName, syskeyType, NULL, NULL,
			     STMTHEAP);
  ElemDDLColRef edcr("SYSKEY", COM_ASCENDING_ORDER);
  CollIndex numSysCols = 0;
  if (((createTableNode->getStoreOption() == COM_KEY_COLUMN_LIST_STORE_OPTION) &&
       (NOT createTableNode->getIsConstraintPKSpecified())) ||
      (keyArray.entries() == 0))
    {
      colArray.insertAt(0, &syskeyColDef);

      keyArray.insert(&edcr);

      implicitPK = TRUE;
      numSysCols++;
    }

  if ((implicitPK) && (explicitSyskeySpecified))
    {
      *CmpCommon::diags() << DgSqlCode(-1080)
				<< DgColumnName("SYSKEY");

      deallocEHI(ehi); 

      processReturn();

      return;
    }

  int numSplits = 0;

  Lng32 numSaltPartnsFromCQD = 
    CmpCommon::getDefaultNumeric(TRAF_NUM_OF_SALT_PARTNS);
  
  if ((createTableNode->getSaltOptions()) ||
      ((numSaltPartnsFromCQD > 0) &&
       (NOT (implicitPK || explicitSyskeySpecified))))
    {
      // add a system column SALT INTEGER NOT NULL with a computed
      // default value HASH2PARTFUNC(<salting cols> FOR <num salt partitions>)
      ElemDDLSaltOptionsClause * saltOptions = createTableNode->getSaltOptions();
      ElemDDLColRefArray *saltArray = createTableNode->getSaltColRefArray();
      NAString saltExprText("HASH2PARTFUNC(");
      NABoolean firstSaltCol = TRUE;
      char numSaltPartnsStr[20];
      
      if (saltArray == NULL || saltArray->entries() == 0)
        {
          // if no salting columns are specified, use all key columns
          saltArray = &keyArray;
        }
      else
        {
          // Validate that salting columns refer to real key columns
          for (CollIndex s=0; s<saltArray->entries(); s++)
            if (keyArray.getColumnIndex((*saltArray)[s]->getColumnName()) < 0)
              {
                *CmpCommon::diags() << DgSqlCode(-1195) 
                                    << DgColumnName((*saltArray)[s]->getColumnName());
                deallocEHI(ehi); 
                processReturn();
                return;
              }
        }

      for (CollIndex i=0; i<saltArray->entries(); i++)
        {
          const NAString &colName = (*saltArray)[i]->getColumnName();
          ComAnsiNamePart cnp(colName, ComAnsiNamePart::INTERNAL_FORMAT);
          CollIndex      colIx    = colArray.getColumnIndex(colName);
          NAType         *colType = colArray[colIx]->getColumnDataType();
          NAString       typeText;
          short          rc       = colType->getMyTypeAsText(&typeText, FALSE);

          // don't include SYSKEY in the list of salt columns
          if (colName != "SYSKEY")
            {
              if (firstSaltCol)
                firstSaltCol = FALSE;
              else
                saltExprText += ",";
              saltExprText += "CAST(";
              saltExprText += cnp.getExternalName();
              saltExprText += " AS ";
              saltExprText += typeText;
              if (!colType->supportsSQLnull())
                saltExprText += " NOT NULL";
              saltExprText += ")";
              if (colType->getTypeQualifier() == NA_NUMERIC_TYPE &&
                  !(((NumericType *) colType)->isExact()))
                {
                  *CmpCommon::diags() << DgSqlCode(-1120);
                  deallocEHI(ehi); 
                  processReturn();
                  return;
                }
            }
          else if (saltArray != &keyArray || saltArray->entries() == 1)
            {
              // SYSKEY was explicitly specified in salt column or is the only column,
              // this is an error
              *CmpCommon::diags() << DgSqlCode(-1195) 
                                  << DgColumnName((*saltArray)[i]->getColumnName());
              deallocEHI(ehi); 
              processReturn();
              return;
            }
        }

      Lng32 numSaltPartns =
	(saltOptions ? saltOptions->getNumPartitions() : numSaltPartnsFromCQD);
      saltExprText += " FOR ";
      sprintf(numSaltPartnsStr,"%d", numSaltPartns);
      saltExprText += numSaltPartnsStr;
      saltExprText += ")";
      if (numSaltPartns <= 1 || numSaltPartns > 1024)
        {
          // number of salt partitions is out of bounds
          *CmpCommon::diags() << DgSqlCode(-1196) 
                              << DgInt0(2)
                              << DgInt1(1024);
          deallocEHI(ehi); 
          processReturn();
          return;
        }
      if (saltExprText.length() > 500)
        {
          // the salt expression will not fit into the metadata column
          // "_MD_".COLUMNS.DEFAULT_VALUE CHAR(512)
          // number of salt partitions is out of bounds
          *CmpCommon::diags() << DgSqlCode(-1197);
          deallocEHI(ehi); 
          processReturn();
          return;
        }

      NAString saltColName(ElemDDLSaltOptionsClause::getSaltSysColName());
      SQLInt * saltType = new(STMTHEAP) SQLInt(FALSE, FALSE, STMTHEAP);
      ElemDDLColDefault *saltDef = 
        new(STMTHEAP) ElemDDLColDefault(
             ElemDDLColDefault::COL_COMPUTED_DEFAULT);
      saltDef->setComputedDefaultExpr(saltExprText);
      ElemDDLColDef * saltColDef =
        new(STMTHEAP) ElemDDLColDef(saltColName, saltType, saltDef, NULL,
                                    STMTHEAP);

      ElemDDLColRef * edcrs = 
        new(STMTHEAP) ElemDDLColRef(saltColName, COM_ASCENDING_ORDER);

      // add this new salt column before user columns but after SYSKEY
      // and also as key column 0
      colArray.insertAt(numSysCols, saltColDef);
      keyArray.insertAt(0, edcrs);
      numSysCols++;
      numSplits = numSaltPartns - 1;
    }

  Int32 keyLength = 0;
  for(CollIndex i = 0; i < keyArray.entries(); i++) 
    {
      const NAString &colName = keyArray[i]->getColumnName();
      Lng32      colIx    = colArray.getColumnIndex(colName);
      if (colIx < 0)
	{
	  *CmpCommon::diags() << DgSqlCode(-1009)
			      << DgColumnName(colName);

	  deallocEHI(ehi); 
	  
	  processReturn();
	  
	  return;
	}

      NAType *colType = colArray[colIx]->getColumnDataType();
      keyLength += colType->getEncodedKeyLength();
    }

  // create table in seabase
  Lng32 numCols = colArray.entries();
  Lng32 numKeys = keyArray.entries();
  
  ComTdbVirtTableColumnInfo * colInfoArray = (ComTdbVirtTableColumnInfo*)
    new(STMTHEAP) char[numCols * sizeof(ComTdbVirtTableColumnInfo)];

  ComTdbVirtTableKeyInfo * keyInfoArray = (ComTdbVirtTableKeyInfo*)
    new(STMTHEAP) char[numKeys * sizeof(ComTdbVirtTableKeyInfo)];

  if (buildColInfoArray(&colArray, colInfoArray, implicitPK, numSysCols))
    {
      processReturn();
      
      return;
    }

  // allow nullable clustering key or unique constraints based on the
  // CQD settings. If a volatile table is being created and cqd
  // VOLATILE_TABLE_FIND_SUITABLE_KEY is ON, then allow it.
  // If ALLOW_NULLABLE_UNIQUE_KEY_CONSTRAINT is set, then allow it.
  NABoolean allowNullableUniqueConstr = FALSE;
  if (((CmpCommon::getDefault(VOLATILE_TABLE_FIND_SUITABLE_KEY) != DF_OFF) &&
       (createTableNode->isVolatile())) ||
      (CmpCommon::getDefault(ALLOW_NULLABLE_UNIQUE_KEY_CONSTRAINT) == DF_ON))
    allowNullableUniqueConstr = TRUE;

  if (buildKeyInfoArray(&colArray, &keyArray, colInfoArray, keyInfoArray, allowNullableUniqueConstr))
    {
      processReturn();
      
      return;
    }

  char ** encodedKeysBuffer = NULL;
  if (numSplits > 0) {

    desc_struct * colDescs = 
      convertVirtTableColumnInfoArrayToDescStructs(&tableName,
                                                   colInfoArray,
                                                   numCols) ;
    desc_struct * keyDescs = 
      convertVirtTableKeyInfoArrayToDescStructs(keyInfoArray,
                                                colInfoArray,
                                                numKeys) ;

    if (createEncodedKeysBuffer(encodedKeysBuffer,
				colDescs, keyDescs, numSplits, numKeys, keyLength))
      {
	processReturn();
	
	return;
      }
  }

  ParDDLFileAttrsCreateTable &fileAttribs =
    createTableNode->getFileAttributes();

  ComTdbVirtTableTableInfo tableInfo;
  tableInfo.tableName = NULL;
  tableInfo.createTime = 0;
  tableInfo.redefTime = 0;
  tableInfo.objUID = 0;
  tableInfo.isAudited = (fileAttribs.getIsAudit() ? 1 : 0);
  tableInfo.validDef = 1;
  tableInfo.hbaseCreateOptions = NULL;
  tableInfo.objOwner = objOwner;
  tableInfo.numSaltPartns = (numSplits > 0 ? numSplits+1 : 0);

  NAText hbaseOptionsStr;
  NAList<HbaseCreateOption*> hbaseCreateOptions;
  NABoolean maxFileSizeOptionSpecified = FALSE;
  NABoolean splitPolicyOptionSpecified = FALSE;
  const char *maxFileSizeOptionString = "MAX_FILESIZE";
  const char *splitPolicyOptionString = "SPLIT_POLICY";

  Lng32 numHbaseOptions = 0;
  if (createTableNode->getHbaseOptionsClause())
    {
      for (CollIndex i = 0; i < createTableNode->getHbaseOptionsClause()->getHbaseOptions().entries(); i++)
	{
	  HbaseCreateOption * hbaseOption =
            createTableNode->getHbaseOptionsClause()->getHbaseOptions()[i];

          hbaseCreateOptions.insert(hbaseOption);

          if (hbaseOption->key() == maxFileSizeOptionString)
            maxFileSizeOptionSpecified = TRUE;
          else if (hbaseOption->key() == splitPolicyOptionString)
            splitPolicyOptionSpecified = TRUE;

	  hbaseOptionsStr += hbaseOption->key();
	  hbaseOptionsStr += "=''";
	  hbaseOptionsStr += hbaseOption->val();
	  hbaseOptionsStr += "''";

	  hbaseOptionsStr += "|";
	}

      numHbaseOptions += createTableNode->getHbaseOptionsClause()->getHbaseOptions().entries();
    }

  if (numSplits > 0 /* i.e. a salted table */)
    {
      // set table-specific region split policy and max file
      // size, controllable by CQDs, but only if they are not
      // already set explicitly in the DDL.
      // Save these options in metadata if they are specified by user through
      // explicit create option or through a cqd.
      double maxFileSize = 
        CmpCommon::getDefaultNumeric(HBASE_SALTED_TABLE_MAX_FILE_SIZE);
      NABoolean usePerTableSplitPolicy = 
        (CmpCommon::getDefault(HBASE_SALTED_TABLE_SET_SPLIT_POLICY) == DF_ON);
      HbaseCreateOption * hbaseOption = NULL;

      if (maxFileSize > 0 && !maxFileSizeOptionSpecified)
         {
          char fileSizeOption[100];
          Int64 maxFileSizeInt;

          if (maxFileSize < LLONG_MAX)
            maxFileSizeInt = maxFileSize;
          else
            maxFileSizeInt = LLONG_MAX;
          
          snprintf(fileSizeOption,100,"%ld", maxFileSizeInt);
          hbaseOption = new(STMTHEAP) HbaseCreateOption("MAX_FILESIZE", fileSizeOption);
          hbaseCreateOptions.insert(hbaseOption);

         if (ActiveSchemaDB()->getDefaults().userDefault(HBASE_SALTED_TABLE_MAX_FILE_SIZE) == TRUE)
           {
             numHbaseOptions += 1;
             snprintf(fileSizeOption,100,"MAX_FILESIZE=''%ld''|", maxFileSizeInt);
             hbaseOptionsStr += fileSizeOption;
           }
        }

      if (usePerTableSplitPolicy && !splitPolicyOptionSpecified)
        {
          const char *saltedTableSplitPolicy =
            "org.apache.hadoop.hbase.regionserver.ConstantSizeRegionSplitPolicy";
          hbaseOption = new(STMTHEAP) HbaseCreateOption(
               "SPLIT_POLICY", saltedTableSplitPolicy);
          hbaseCreateOptions.insert(hbaseOption);

          if (ActiveSchemaDB()->getDefaults().userDefault(HBASE_SALTED_TABLE_SET_SPLIT_POLICY) == TRUE)
            {
              numHbaseOptions += 1;
              hbaseOptionsStr += "SPLIT_POLICY=''";
              hbaseOptionsStr += saltedTableSplitPolicy;
              hbaseOptionsStr += "''|";
            }
        }  
    }
  
  /////////////////////////////////////////////////////////////////////
  // update HBASE_CREATE_OPTIONS field in metadata TABLES table.
  // Format of data stored in this field, if applicable.
  //    HBASE_OPTIONS=>numOptions(4bytes)option='val'| ...
  //    NUM_SPLITS=>4_bytes(int)
  ///////////////////////////////////////////////////////////////////////
  NAString hco;
  //  if ((createTableNode->getHbaseOptionsClause()) &&
  if  (hbaseOptionsStr.size() > 0)
    {
      hco += "HBASE_OPTIONS=>";

      char hbaseOptionsNumCharStr[5];
      sprintf(hbaseOptionsNumCharStr, "%04d", numHbaseOptions);
      hco += hbaseOptionsNumCharStr;

      hco += hbaseOptionsStr.data();
      
      hco += " "; // separator
    }
  
  if (numSplits > 0)
    {
      char splitNumCharStr[5];
      sprintf(splitNumCharStr, "%04d", numSplits+1);
 
      hco += "NUM_SALT_PARTNS=>";
      hco+=splitNumCharStr;
		 
      hco += " "; // separator
    }

  tableInfo.hbaseCreateOptions = (hco.isNull() ? NULL : hco.data());
     
  Int64 objUID = -1;
  if (Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
    {
      const char* v = ActiveSchemaDB()->getDefaults().
	getValue(TRAF_CREATE_TABLE_WITH_UID);
      if ((v) and (strlen(v) > 0))
	{
	  objUID = str_atoi(v, strlen(v));
	}
    }

  if (updateSeabaseMDTable(&cliInterface, 
			   catalogNamePart, schemaNamePart, objectNamePart,
			   COM_BASE_TABLE_OBJECT_LIT,
			   "Y",
			   &tableInfo,
			   numCols,
			   colInfoArray,
			   numKeys,
			   keyInfoArray,
			   0, NULL,
			   objUID))
    {
      *CmpCommon::diags()
	<< DgSqlCode(-1029)
	<< DgTableName(extTableName);

      deallocEHI(ehi); 
      processReturn();
      return;
    }

  if (createTableNode->getAddConstraintPK())
    {
      if (updatePKeyInfo(createTableNode->getAddConstraintPK(), 
			 catalogNamePart, schemaNamePart, objectNamePart,
			 keyArray.entries(),
			 NULL,
			 NULL,
			 keyInfoArray,
			 &cliInterface))
	{
	  return;
	}
    }

  HbaseStr hbaseTable;
  hbaseTable.val = (char*)extNameForHbase.data();
  hbaseTable.len = extNameForHbase.length();
  if (createHbaseTable(ehi, &hbaseTable, SEABASE_DEFAULT_COL_FAMILY, NULL, NULL,
		       &hbaseCreateOptions, numSplits, keyLength, 
                       encodedKeysBuffer) == -1)
    {
      deallocEHI(ehi); 

      processReturn();

      return;
    }


  CorrName cn(objectNamePart, STMTHEAP, schemaNamePart, catalogNamePart);
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);
  
  processReturn();

  return;
}

void CmpSeabaseDDL::addConstraints(
				   ComObjectName &tableName,
				   ComAnsiNamePart &currCatAnsiName,
				   ComAnsiNamePart &currSchAnsiName,
				   StmtDDLAddConstraintPK * pkConstr,
  				   StmtDDLAddConstraintUniqueArray &uniqueConstrArr,
				   StmtDDLAddConstraintRIArray &riConstrArr,
				   StmtDDLAddConstraintCheckArray &checkConstrArr)
{
  Lng32 cliRC = 0;

  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);

  ExeCliInterface cliInterface(STMTHEAP);

  char buf[5000];

  if (pkConstr)
    {
      StmtDDLAddConstraintUnique *uniqConstr = pkConstr;
      
      NAString uniqueName;
      genUniqueName(uniqConstr, uniqueName);
      
      ComObjectName constrName(uniqueName);
      constrName.applyDefaults(currCatAnsiName, currSchAnsiName);
      const NAString constrCatalogNamePart = constrName.getCatalogNamePartAsAnsiString();
      const NAString constrSchemaNamePart = constrName.getSchemaNamePartAsAnsiString(TRUE);
      const NAString constrObjectNamePart = constrName.getObjectNamePartAsAnsiString(TRUE);
      
      ElemDDLConstraintUnique *constraintNode = 
	( uniqConstr->getConstraint() )->castToElemDDLConstraintUnique();
      ElemDDLColRefArray &keyColumnArray = constraintNode->getKeyColumnArray();
      
      NAString keyColNameStr;
      for (CollIndex i = 0; i < keyColumnArray.entries(); i++)
	{
	  keyColNameStr += "\"";
	  keyColNameStr += keyColumnArray[i]->getColumnName();
	  keyColNameStr += "\"";

	  if (keyColumnArray[i]->getColumnOrdering() == COM_DESCENDING_ORDER)
	    keyColNameStr += "DESC";
	  else
	    keyColNameStr += "ASC";
 
	  if (i+1 < keyColumnArray.entries())
	    keyColNameStr += ", ";
	}
      
      str_sprintf(buf, "alter table \"%s\".\"%s\".\"%s\" add constraint \"%s\".\"%s\".\"%s\" unique (%s)",
		  catalogNamePart.data(), schemaNamePart.data(), objectNamePart.data(),
		  constrCatalogNamePart.data(), constrSchemaNamePart.data(), constrObjectNamePart.data(),
		  keyColNameStr.data());
      
      cliRC = cliInterface.executeImmediate(buf);
      if (cliRC < 0)
	{
	  cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
	  
	  processReturn();
	  
	  goto label_return;
	}
    }
  
  if (uniqueConstrArr.entries() > 0)
    {
      for (Lng32 i = 0; i < uniqueConstrArr.entries(); i++)
	{
	  StmtDDLAddConstraintUnique *uniqConstr = 
	    uniqueConstrArr[i];

	  NAString uniqueName;
	  genUniqueName(uniqConstr, uniqueName);

	  ComObjectName constrName(uniqueName);
	  constrName.applyDefaults(currCatAnsiName, currSchAnsiName);
	  const NAString constrCatalogNamePart = constrName.getCatalogNamePartAsAnsiString();
	  const NAString constrSchemaNamePart = constrName.getSchemaNamePartAsAnsiString(TRUE);
	  const NAString constrObjectNamePart = constrName.getObjectNamePartAsAnsiString(TRUE);

	  ElemDDLConstraintUnique *constraintNode = 
	    ( uniqConstr->getConstraint() )->castToElemDDLConstraintUnique();
	  ElemDDLColRefArray &keyColumnArray = constraintNode->getKeyColumnArray();
 
	  NAString keyColNameStr;
	  for (CollIndex i = 0; i < keyColumnArray.entries(); i++)
	    {
	      keyColNameStr += "\"";
	      keyColNameStr += keyColumnArray[i]->getColumnName();
	      keyColNameStr += "\"";

	      if (keyColumnArray[i]->getColumnOrdering() == COM_DESCENDING_ORDER)
		keyColNameStr += "DESC";
	      else
		keyColNameStr += "ASC";

	      if (i+1 < keyColumnArray.entries())
		keyColNameStr += ", ";
	    }

	  str_sprintf(buf, "alter table \"%s\".\"%s\".\"%s\" add constraint \"%s\".\"%s\".\"%s\" unique (%s)",
		      catalogNamePart.data(), schemaNamePart.data(), objectNamePart.data(),
		      constrCatalogNamePart.data(), constrSchemaNamePart.data(), constrObjectNamePart.data(),
		      keyColNameStr.data());
		      
	  cliRC = cliInterface.executeImmediate(buf);
	  if (cliRC < 0)
	    {
	      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
	      
	      processReturn();
	      
	      goto label_return;
	    }

	} // for
    } // if

  if (riConstrArr.entries() > 0)
    {
      for (Lng32 i = 0; i < riConstrArr.entries(); i++)
	{
	  StmtDDLAddConstraintRI *refConstr = riConstrArr[i];

	  ComObjectName refdTableName(refConstr->getReferencedTableName(), COM_TABLE_NAME);
	  refdTableName.applyDefaults(currCatAnsiName, currSchAnsiName);
	  const NAString refdCatNamePart = refdTableName.getCatalogNamePartAsAnsiString();
	  const NAString refdSchNamePart = refdTableName.getSchemaNamePartAsAnsiString(TRUE);
	  const NAString refdObjNamePart = refdTableName.getObjectNamePartAsAnsiString(TRUE);

	  NAString uniqueName;
	  genUniqueName(refConstr, uniqueName);

	  ComObjectName constrName(uniqueName);
	  constrName.applyDefaults(currCatAnsiName, currSchAnsiName);
	  const NAString constrCatalogNamePart = constrName.getCatalogNamePartAsAnsiString();
	  const NAString constrSchemaNamePart = constrName.getSchemaNamePartAsAnsiString(TRUE);
	  const NAString constrObjectNamePart = constrName.getObjectNamePartAsAnsiString(TRUE);

	  ElemDDLConstraintRI *constraintNode = 
	    ( refConstr->getConstraint() )->castToElemDDLConstraintRI();
	  ElemDDLColNameArray &ringColumnArray = constraintNode->getReferencingColumns();
 
	  NAString ringColNameStr;
	  for (CollIndex i = 0; i < ringColumnArray.entries(); i++)
	    {
	      ringColNameStr += "\"";
	      ringColNameStr += ringColumnArray[i]->getColumnName();
	      ringColNameStr += "\"";
	      if (i+1 < ringColumnArray.entries())
		ringColNameStr += ", ";
	    }

	  ElemDDLColNameArray &refdColumnArray = constraintNode->getReferencedColumns();
 
	  NAString refdColNameStr;
	  if (refdColumnArray.entries() > 0)
	    refdColNameStr = "(";
	  for (CollIndex i = 0; i < refdColumnArray.entries(); i++)
	    {
	      refdColNameStr += "\"";
	      refdColNameStr += refdColumnArray[i]->getColumnName();
	      refdColNameStr += "\"";	      
	      if (i+1 < refdColumnArray.entries())
		refdColNameStr += ", ";
	    }
	  if (refdColumnArray.entries() > 0)
	    refdColNameStr += ")";

	  str_sprintf(buf, "alter table \"%s\".\"%s\".\"%s\" add constraint \"%s\".\"%s\".\"%s\" foreign key (%s) references \"%s\".\"%s\".\"%s\" %s",
		      catalogNamePart.data(), schemaNamePart.data(), objectNamePart.data(),
		      constrCatalogNamePart.data(), constrSchemaNamePart.data(), constrObjectNamePart.data(),
		      ringColNameStr.data(),
		      refdCatNamePart.data(), refdSchNamePart.data(), refdObjNamePart.data(),
		      (refdColumnArray.entries() > 0 ? refdColNameStr.data() : " "));
		      
	  cliRC = cliInterface.executeImmediate(buf);
	  if (cliRC < 0)
	    {
	      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
	      
	      processReturn();
	      
	    }

	  CorrName cn2(refdObjNamePart.data(),
		       STMTHEAP,
		       refdSchNamePart.data(),
		       refdCatNamePart.data());

	  // remove natable for the table being referenced
	  ActiveSchemaDB()->getNATableDB()->removeNATable(cn2);

	  if (cliRC < 0)
	    goto label_return;
	} // for
    } // if

  if (checkConstrArr.entries() > 0)
    {
      for (Lng32 i = 0; i < checkConstrArr.entries(); i++)
	{
	  StmtDDLAddConstraintCheck *checkConstr = checkConstrArr[i];

	  NAString uniqueName;
	  genUniqueName(checkConstr, uniqueName);

	  ComObjectName constrName(uniqueName);
	  constrName.applyDefaults(currCatAnsiName, currSchAnsiName);
	  const NAString constrCatalogNamePart = constrName.getCatalogNamePartAsAnsiString();
	  const NAString constrSchemaNamePart = constrName.getSchemaNamePartAsAnsiString(TRUE);
	  const NAString constrObjectNamePart = constrName.getObjectNamePartAsAnsiString(TRUE);

	  NAString constrText;
	  getCheckConstraintText(checkConstr, constrText);
	  str_sprintf(buf, "alter table \"%s\".\"%s\".\"%s\" add constraint \"%s\".\"%s\".\"%s\" check %s",
		      catalogNamePart.data(), schemaNamePart.data(), objectNamePart.data(),
		      constrCatalogNamePart.data(), constrSchemaNamePart.data(), constrObjectNamePart.data(),
		      constrText.data()
		      );
		      
	  cliRC = cliInterface.executeImmediate(buf);
	  if (cliRC < 0)
	    {
	      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
	      
	      processReturn();
	      
	      goto label_return;
	    }
	}
    }

 label_return:
  // remove NATable cache entries for this table
  CorrName cn(objectNamePart.data(),
	      STMTHEAP,
	      schemaNamePart.data(),
	      catalogNamePart.data());
  
  // remove NATable for this table
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);

  return;
}

void CmpSeabaseDDL::createSeabaseTableCompound(
				       StmtDDLCreateTable * createTableNode,
				       NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;
  ExeCliInterface cliInterface(STMTHEAP);

  ComObjectName tableName(createTableNode->getTableName());
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);
  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);

  if ((createTableNode->isVolatile()) &&
      ((createTableNode->getAddConstraintUniqueArray().entries() > 0) ||
       (createTableNode->getAddConstraintRIArray().entries() > 0) ||
       (createTableNode->getAddConstraintCheckArray().entries() > 0)))
    {
      *CmpCommon::diags() << DgSqlCode(-1283);
      
      processReturn();
      
      goto label_error;
    }

  cliRC = beginXn(&cliInterface);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return;
    }

  createSeabaseTable(createTableNode, currCatName, currSchName);
  if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_))
    {
      rollbackXn(&cliInterface);

      return;
    }

  cliRC = commitXn(&cliInterface);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      
      goto label_error;
    }

  cliRC = cliInterface.holdAndSetCQD("TRAF_NO_CONSTR_VALIDATION", "ON");
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      
      processReturn();
      
      goto label_error;
    }

  cliRC = beginXn(&cliInterface);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return;
    }

  addConstraints(tableName, currCatAnsiName, currSchAnsiName,
		 NULL,
		 createTableNode->getAddConstraintUniqueArray(),
		 createTableNode->getAddConstraintRIArray(),
		 createTableNode->getAddConstraintCheckArray());		     
  if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_))
    {
      if (cliInterface.statusXn() == 0) // xn in progress
	{
	  rollbackXn(&cliInterface);
	}

      *CmpCommon::diags() << DgSqlCode(-1029)
			  << DgTableName(extTableName);
      
      processReturn();
      
      goto label_error;
    }

  cliRC = commitXn(&cliInterface);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      
      goto label_error;
    }

  cliRC = cliInterface.restoreCQD("traf_no_constr_validation");

  return;

 label_error:
  cliRC = cliInterface.restoreCQD("traf_no_constr_validation");

  if (NOT createTableNode->isVolatile())
    {
      char buf [1000];

      cliRC = cliInterface.holdAndSetCQD("TRAF_RELOAD_NATABLE_CACHE", "ON");
      if (cliRC < 0)
	{
	  cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
	  
	  processReturn();
	  
	  return;
	}

      str_sprintf(buf, "drop table if exists \"%s\".\"%s\".\"%s\" cascade",
		  catalogNamePart.data(), schemaNamePart.data(), objectNamePart.data());
      
      cliRC = cliInterface.executeImmediate(buf);

      if (cliRC < 0)
	{
	  cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

	  cliRC = cliInterface.restoreCQD("TRAF_RELOAD_NATABLE_CACHE");
	  
	  processReturn();
	  
	  return;
	}

      cliRC = cliInterface.restoreCQD("TRAF_RELOAD_NATABLE_CACHE");
    }
}

void CmpSeabaseDDL::dropSeabaseTable(
				     StmtDDLDropTable                  * dropTableNode,
				     NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  NAString tabName = (NAString&)dropTableNode->getTableName();

  ComObjectName tableName(tabName, COM_TABLE_NAME);
  ComObjectName volTabName; 
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);

  NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);

  ExeCliInterface cliInterface(STMTHEAP);

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();
      
      return;
    }

  if ((isSeabaseReservedSchema(tableName)) &&
      (!Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_USER_CANNOT_DROP_SMD_TABLE)
			  << DgTableName(extTableName);
      deallocEHI(ehi); 

      processReturn();

      return;
    }

  NABoolean isVolatile = FALSE;

  if ((dropTableNode->isVolatile()) &&
      (CmpCommon::context()->sqlSession()->volatileSchemaInUse()))
    {
      volTabName = tableName;
      isVolatile = TRUE;
    }
  
  if ((NOT dropTableNode->isVolatile()) &&
      (CmpCommon::context()->sqlSession()->volatileSchemaInUse()))
    {

      // updateVolatileQualifiedName qualifies the object name with a 
      // volatile catalog and schema name (if a volatile schema exists)
      QualifiedName *qn =
	CmpCommon::context()->sqlSession()->
	updateVolatileQualifiedName
	(dropTableNode->getOrigTableNameAsQualifiedName().getObjectName());
      
      // don't believe it is possible to get a null pointer returned
      if (qn == NULL)
	{
	  *CmpCommon::diags()
	    << DgSqlCode(-CAT_UNABLE_TO_DROP_OBJECT)
	    << DgTableName(dropTableNode->getOrigTableNameAsQualifiedName().
			   getQualifiedNameAsAnsiString(TRUE));

	  processReturn();

	  return;
	}
      
        volTabName = qn->getQualifiedNameAsAnsiString();
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
                (dropTableNode->getOrigTableNameAsQualifiedName()))
              {
                // Valid volatile table. Drop it.
                tabName = volTabName.getExternalName(TRUE);

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
				   COM_BASE_TABLE_OBJECT_LIT);
  if (retcode < 0)
    {
      processReturn();

      return;
    }

  if (retcode == 0) // does not exist
    {
      if (NOT dropTableNode->dropIfExists())
        {
          CmpCommon::diags()->clear();

          if (isVolatile)
            *CmpCommon::diags() << DgSqlCode(-1389)
                                << DgString0(objectNamePart);
          else
            *CmpCommon::diags() << DgSqlCode(-1389)
                                << DgString0(extTableName);
        }

      processReturn();

      return;
    }

  // Check to see if the user has the authority to drop the table
  ComObjectName verifyName;
  if (isVolatile)
     verifyName = volTabName;
  else
     verifyName = tableName;

  NABoolean trustedCaller = FALSE;
  if ((isSeabaseReservedSchema(tableName)) &&
      (Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
    trustedCaller = TRUE;

  ComUserVerifyObj verifyAuth(verifyName, ComUserVerifyObj::OBJ_OBJ_TYPE);
  if (!verifyAuth.isAuthorized(ComUser::DROP_TABLE, trustedCaller))
  {
    *CmpCommon::diags() << DgSqlCode(-1017);

    processReturn ();

    return;
  }

  if (CmpCommon::getDefault(TRAF_RELOAD_NATABLE_CACHE) == DF_OFF)
    ActiveSchemaDB()->getNATableDB()->useCache();

 // save the current parserflags setting
  ULng32 savedParserFlags = Get_SqlParser_Flags (0xFFFFFFFF);
  Set_SqlParser_Flags(ALLOW_VOLATILE_SCHEMA_IN_TABLE_NAME);

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);

  CorrName cn(objectNamePart,
	      STMTHEAP,
	      schemaNamePart,
	      catalogNamePart);

  NATable *naTable = bindWA.getNATable(cn); 

  // Restore parser flags settings to what they originally were
  Set_SqlParser_Flags (savedParserFlags);

  if (naTable == NULL || bindWA.errStatus())
    {
      CmpCommon::diags()->clear();

      if (NOT dropTableNode->dropIfExists())
	{
	  CmpCommon::diags()->clear();
	  
	  if (isVolatile)
	    *CmpCommon::diags() << DgSqlCode(-1389)
				<< DgString0(objectNamePart);
	  else
	    *CmpCommon::diags() << DgSqlCode(-1389)
				<< DgString0(extTableName);
	}
      
      processReturn();

      return;
    }

  if ((dropTableNode->isVolatile()) && 
      (NOT CmpCommon::context()->sqlSession()->isValidVolatileSchemaName(schemaNamePart)))
    {
      *CmpCommon::diags() << DgSqlCode(-1279);

      processReturn();

      return;
    }

  Int64 objUID = getObjectUID(&cliInterface,
			      catalogNamePart.data(), schemaNamePart.data(), 
			      objectNamePart.data(),
			      COM_BASE_TABLE_OBJECT_LIT);
  if (objUID < 0)
    {

      processReturn();

      return;
    }

  Queue * usingViewsQueue = NULL;
  if (dropTableNode->getDropBehavior() == COM_RESTRICT_DROP_BEHAVIOR)
    {
      NAString usingObjName;
      cliRC = getUsingObject(&cliInterface, objUID, usingObjName);
      if (cliRC < 0)
	{
	  processReturn();
	  
	  return;
	}

      if (cliRC != 100) // found an object
	{
	  *CmpCommon::diags() << DgSqlCode(-CAT_DEPENDENT_VIEW_EXISTS)
			      << DgTableName(usingObjName);

	  processReturn();

	  return;
	}
    }
  else if (dropTableNode->getDropBehavior() == COM_CASCADE_DROP_BEHAVIOR)
    {
      cliRC = getUsingViews(&cliInterface, extTableName, TRUE, usingViewsQueue);
      if (cliRC < 0)
	{
	  processReturn();
	  
	  return;
	}
    }

  // return error is cascade is not specified and a referential constraint exists on
  // any of the unique constraints.
  const AbstractRIConstraintList &uniqueList = naTable->getUniqueConstraints();
  
  if (dropTableNode->getDropBehavior() == COM_RESTRICT_DROP_BEHAVIOR)
    {
      for (Int32 i = 0; i < uniqueList.entries(); i++)
	{
	  AbstractRIConstraint *ariConstr = uniqueList[i];
	  
	  if (ariConstr->getOperatorType() != ITM_UNIQUE_CONSTRAINT)
	    continue;
	  
	  UniqueConstraint * uniqConstr = (UniqueConstraint*)ariConstr;

	  if (uniqConstr->hasRefConstraintsReferencingMe())
	    {
	      const ComplementaryRIConstraint * rc = uniqConstr->getRefConstraintReferencingMe(0);
	      
	      const NAString &constrName = 
		(rc ? rc->getConstraintName().getObjectName() : " ");
	      *CmpCommon::diags() << DgSqlCode(-1059)
				  << DgConstraintName(constrName);
	      
	      processReturn();
	      
	      return;
	    }
	}
    }

  char query[4000];

  if (usingViewsQueue)
    {
      usingViewsQueue->position();
      for (int idx = 0; idx < usingViewsQueue->numEntries(); idx++)
	{
	  OutputInfo * vi = (OutputInfo*)usingViewsQueue->getNext(); 
	  
	  char * viewName = vi->get(0);
	  
	  if (dropSeabaseObject(ehi, viewName,
				currCatName, currSchName, COM_VIEW_OBJECT_LIT))
	    {
	      processReturn();
	      
	      return;
	    }
	}
    }

  // drop all referential constraints referencing me.
  for (Int32 i = 0; i < uniqueList.entries(); i++)
    {
      AbstractRIConstraint *ariConstr = uniqueList[i];
      
      if (ariConstr->getOperatorType() != ITM_UNIQUE_CONSTRAINT)
	continue;

      UniqueConstraint * uniqConstr = (UniqueConstraint*)ariConstr;

     // We will only reach here is cascade option is specified.
      // drop all constraints referencing me.
      if (uniqConstr->hasRefConstraintsReferencingMe())
	{
	  cliRC = cliInterface.holdAndSetCQD("TRAF_RELOAD_NATABLE_CACHE", "ON");
	  if (cliRC < 0)
	    {
	      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
	      
	      processReturn();
	      
	      return;
	    }

	  for (Lng32 j = 0; j < uniqConstr->getNumRefConstraintsReferencingMe(); j++)
	    {
	      const ComplementaryRIConstraint * rc = 
		uniqConstr->getRefConstraintReferencingMe(j);

	      str_sprintf(query, "alter table \"%s\".\"%s\".\"%s\" drop constraint \"%s\".\"%s\".\"%s\"",
			  rc->getTableName().getCatalogName().data(),
			  rc->getTableName().getSchemaName().data(),
			  rc->getTableName().getObjectName().data(),
			  rc->getConstraintName().getCatalogName().data(),
			  rc->getConstraintName().getSchemaName().data(),
			  rc->getConstraintName().getObjectName().data());

	      cliRC = cliInterface.executeImmediate(query);

	      if (cliRC < 0)
		{
		  cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

		  cliRC = cliInterface.restoreCQD("TRAF_RELOAD_NATABLE_CACHE");
		  
		  processReturn();
		  
		  return;
		}
	      
	    } // for
	  
	  cliRC = cliInterface.restoreCQD("TRAF_RELOAD_NATABLE_CACHE");
	  
	} // if
    } // for

  // drop all unique constraints from metadata
  for (Int32 i = 0; i < uniqueList.entries(); i++)
    {
      AbstractRIConstraint *ariConstr = uniqueList[i];
      
      if (ariConstr->getOperatorType() != ITM_UNIQUE_CONSTRAINT)
	continue;

      UniqueConstraint * uniqConstr = (UniqueConstraint*)ariConstr;

      const NAString& constrCatName = 
	uniqConstr->getConstraintName().getCatalogName();

      const NAString& constrSchName = 
	uniqConstr->getConstraintName().getSchemaName();
      
      const NAString& constrObjName = 
	uniqConstr->getConstraintName().getObjectName();
      
      Int64 constrUID = getObjectUID(&cliInterface,
				     constrCatName.data(), constrSchName.data(), constrObjName.data(),
				     (uniqConstr->isPrimaryKeyConstraint() ?
				      COM_PRIMARY_KEY_CONSTRAINT_OBJECT_LIT :
				      COM_UNIQUE_CONSTRAINT_OBJECT_LIT));		 
      if (constrUID < 0)
	{
	  processReturn();
	  
	  return;
	}

      if (deleteConstraintInfoFromSeabaseMDTables(&cliInterface,
						  naTable->objectUid().castToInt64(),
						  0,
						  constrUID,
						  0,
						  constrCatName,
						  constrSchName,
						  constrObjName,
						  (uniqConstr->isPrimaryKeyConstraint() ?
						   COM_PRIMARY_KEY_CONSTRAINT_OBJECT_LIT :
						   COM_UNIQUE_CONSTRAINT_OBJECT_LIT)))
	{
	  processReturn();
	  
	  return;
	}
     }

  // drop all referential constraints from metadata
  const AbstractRIConstraintList &refList = naTable->getRefConstraints();
  
  for (Int32 i = 0; i < refList.entries(); i++)
    {
      AbstractRIConstraint *ariConstr = refList[i];
      
      if (ariConstr->getOperatorType() != ITM_REF_CONSTRAINT)
	continue;

      RefConstraint * refConstr = (RefConstraint*)ariConstr;

      const NAString& constrCatName = 
	refConstr->getConstraintName().getCatalogName();

      const NAString& constrSchName = 
	refConstr->getConstraintName().getSchemaName();
      
      const NAString& constrObjName = 
	refConstr->getConstraintName().getObjectName();
      
      Int64 constrUID = getObjectUID(&cliInterface,
				     constrCatName.data(), constrSchName.data(), constrObjName.data(),
				     COM_REFERENTIAL_CONSTRAINT_OBJECT_LIT);		 
      if (constrUID < 0)
	{
	  processReturn();
	  
	  return;
	}

      NATable *otherNaTable = NULL;
      
      //      CorrName otherCN(refConstr->getOtherTableName());
      CorrName otherCN(refConstr->getUniqueConstraintReferencedByMe().getTableName());

      otherNaTable = bindWA.getNATable(otherCN);
      if (otherNaTable == NULL || bindWA.errStatus())
	{
	  deallocEHI(ehi); 
	  
	  processReturn();
	  
	  return;
	}

      AbstractRIConstraint * otherConstr = 
	refConstr->findConstraint(&bindWA, refConstr->getUniqueConstraintReferencedByMe());

      const NAString& otherSchName = 
	otherConstr->getConstraintName().getSchemaName();
      
      const NAString& otherConstrName = 
	otherConstr->getConstraintName().getObjectName();
      
      Int64 otherConstrUID = getObjectUID(&cliInterface,
					  constrCatName.data(), otherSchName.data(), otherConstrName.data(),
					  COM_UNIQUE_CONSTRAINT_OBJECT_LIT );
      if (otherConstrUID < 0)
	{
	  CmpCommon::diags()->clear();
	  otherConstrUID = getObjectUID(&cliInterface,
					constrCatName.data(), otherSchName.data(), otherConstrName.data(),
					COM_PRIMARY_KEY_CONSTRAINT_OBJECT_LIT );
	  if (otherConstrUID < 0)
	    {
	      processReturn();
	      
	      return;
	    }
	}
      
      if (deleteConstraintInfoFromSeabaseMDTables(&cliInterface,
						  naTable->objectUid().castToInt64(),
						  otherNaTable->objectUid().castToInt64(),
						  constrUID,
						  otherConstrUID,
						  constrCatName,
						  constrSchName,
						  constrObjName,
						  COM_REFERENTIAL_CONSTRAINT_OBJECT_LIT))
	{
	  processReturn();
	  
	  return;
	}
      
    }

  // drop all check constraints from metadata if 'no check' is not specified.
  if (NOT (dropTableNode->getDropBehavior() == COM_NO_CHECK_DROP_BEHAVIOR))
    {
      const CheckConstraintList &checkList = naTable->getCheckConstraints();
      for (Int32 i = 0; i < checkList.entries(); i++)
	{
	  CheckConstraint *checkConstr = checkList[i];
	  
	  const NAString& constrCatName = 
	    checkConstr->getConstraintName().getCatalogName();
	  
	  const NAString& constrSchName = 
	    checkConstr->getConstraintName().getSchemaName();
	  
	  const NAString& constrObjName = 
	    checkConstr->getConstraintName().getObjectName();
	  
	  Int64 constrUID = getObjectUID(&cliInterface,
					 constrCatName.data(), constrSchName.data(), constrObjName.data(),
					 COM_CHECK_CONSTRAINT_OBJECT_LIT);
	  if (constrUID < 0)
	    {
	      processReturn();
	      
	      return;
	    }
	  
	  if (deleteConstraintInfoFromSeabaseMDTables(&cliInterface,
						      naTable->objectUid().castToInt64(),
						      0,
						      constrUID,
						      0,
						      constrCatName,
						      constrSchName,
						      constrObjName,
						      COM_CHECK_CONSTRAINT_OBJECT_LIT))
	    {
	      processReturn();
	      
	      return;
	    }
	}
    }

  const NAFileSetList &indexList = naTable->getIndexList();

  // first drop all index objects from metadata.
  for (Int32 i = 0; i < indexList.entries(); i++)
    {
      const NAFileSet * naf = indexList[i];
      if (naf->getKeytag() == 0)
	continue;

      const QualifiedName &qn = naf->getFileSetName();
      NAString ansiName = qn.getQualifiedNameAsAnsiString();
      
      if (dropSeabaseObject(ehi, ansiName,
			    currCatName, currSchName, COM_INDEX_OBJECT_LIT, TRUE, FALSE))
	{
	  processReturn();
	  
	  return;
	}
    } // for

  // drop SB_HISTOGRAMS and SB_HISTOGRAM_INTERVALS entries, if any
  // if the table that we are dropping itself is not a SB_HISTOGRAMS or SB_HISTOGRAM_INTERVALS table
  if (objectNamePart != "SB_HISTOGRAMS" && 
      objectNamePart != "SB_HISTOGRAM_INTERVALS")
  {
    if (dropSeabaseStats(&cliInterface,
                         catalogNamePart.data(),
                         schemaNamePart.data(),
                         objUID))
    {
      processReturn();
      return;
    }
  }

  // if metadata drop succeeds, drop indexes from hbase.
  for (Int32 i = 0; i < indexList.entries(); i++)
    {
      const NAFileSet * naf = indexList[i];
      if (naf->getKeytag() == 0)
	continue;

      const QualifiedName &qn = naf->getFileSetName();
      NAString ansiName = qn.getQualifiedNameAsAnsiString();

      if (dropSeabaseObject(ehi, ansiName,
			    currCatName, currSchName, COM_INDEX_OBJECT_LIT, FALSE, TRUE))
	{
	  processReturn();
	  
	  return;
	}

      CorrName cni(qn);
      ActiveSchemaDB()->getNATableDB()->removeNATable(cni);

	cni.setSpecialType(ExtendedQualName::INDEX_TABLE);
	ActiveSchemaDB()->getNATableDB()->removeNATable(cni);
    } // for

  if (dropSeabaseObject(ehi, tabName,
			currCatName, currSchName, COM_BASE_TABLE_OBJECT_LIT))
    {
      processReturn();
      
      return;
    }

  processReturn();

  CorrName cn2(objectNamePart, STMTHEAP, schemaNamePart, catalogNamePart);
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn2);
  
  for (Int32 i = 0; i < refList.entries(); i++)
    {
      AbstractRIConstraint *ariConstr = refList[i];
      
      if (ariConstr->getOperatorType() != ITM_REF_CONSTRAINT)
	continue;
      
      RefConstraint * refConstr = (RefConstraint*)ariConstr;
      CorrName otherCN(refConstr->getUniqueConstraintReferencedByMe().getTableName());
      
      ActiveSchemaDB()->getNATableDB()->removeNATable(otherCN);
    }

  for (Int32 i = 0; i < uniqueList.entries(); i++)
    {
      UniqueConstraint * uniqConstr = (UniqueConstraint*)uniqueList[i];

      // We will only reach here is cascade option is specified.
      // drop all constraints referencing me.
      if (uniqConstr->hasRefConstraintsReferencingMe())
	{
	  for (Lng32 j = 0; j < uniqConstr->getNumRefConstraintsReferencingMe(); j++)
	    {
	      const ComplementaryRIConstraint * rc = 
		uniqConstr->getRefConstraintReferencingMe(j);

	      // remove this ref constr entry from natable cache
	      CorrName cnr(rc->getTableName().getObjectName().data(), STMTHEAP, 
			   rc->getTableName().getSchemaName().data(),
			   rc->getTableName().getCatalogName().data());
	      ActiveSchemaDB()->getNATableDB()->removeNATable(cnr);
	    } // for
	  
	} // if
    } // for
}

void CmpSeabaseDDL::renameSeabaseTable(
				       StmtDDLAlterTableRename * renameTableNode,
				       NAString &currCatName, NAString &currSchName)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  ComObjectName tableName(renameTableNode->getTableName());
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);
  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);
  const NAString extNameForHbase = catalogNamePart + "." + schemaNamePart + "." + objectNamePart;

  ComObjectName newTableName(renameTableNode->getNewNameAsAnsiString());
  //  newTableName.applyDefaults(currCatAnsiName, currSchAnsiName);
  newTableName.applyDefaults(catalogNamePart, schemaNamePart);
  const NAString newObjectNamePart = newTableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString newExtTableName = newTableName.getExternalName(TRUE);
  const NAString newExtNameForHbase = catalogNamePart + "." + schemaNamePart + "." + newObjectNamePart;

  ExeCliInterface cliInterface(STMTHEAP);
  
  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();

      return;
    }

  if (isSeabaseReservedSchema(tableName))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_CREATE_TABLE_NOT_ALLOWED_IN_SMD)
			  << DgTableName(extTableName);
      deallocEHI(ehi); 

      processReturn();

      return;
    }
  
  if (CmpCommon::context()->sqlSession()->volatileSchemaInUse())
    {
      QualifiedName *qn =
	CmpCommon::context()->sqlSession()->
	updateVolatileQualifiedName
	(renameTableNode->getTableNameAsQualifiedName().getObjectName());
      
      if (qn == NULL)
	{
	  *CmpCommon::diags()
	    << DgSqlCode(-1427);
	  
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
	  // table found in volatile schema. cannot rename it.
	  *CmpCommon::diags()
	    << DgSqlCode(-1427)
	    << DgString0("Reason: Operation not allowed on volatile tables.");
	  
	  processReturn();
	  return;
	}
    }
  
  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  
  CorrName cn(objectNamePart,
	      STMTHEAP,
	      schemaNamePart,
	      catalogNamePart);
  
  NATable *naTable = bindWA.getNATable(cn); 
  if (naTable == NULL || bindWA.errStatus())
    {
      CmpCommon::diags()->clear();
      
      *CmpCommon::diags() << DgSqlCode(-1389)
			  << DgString0(extTableName);
  
      processReturn();
      
      return;
    }
 
 CorrName newcn(newObjectNamePart,
	      STMTHEAP,
	      schemaNamePart,
	      catalogNamePart);
  
  NATable *newNaTable = bindWA.getNATable(newcn); 
  if (naTable != NULL && (NOT bindWA.errStatus()))
    {
      *CmpCommon::diags() << DgSqlCode(-1390)
			  << DgString0(newExtTableName);
      
      processReturn();
      
      return;
    }

  CmpCommon::diags()->clear();
  
  // cannot rename a view
  if (naTable->getViewText())
    {
      *CmpCommon::diags()
	<< DgSqlCode(-1427)
	<< DgString0("Reason: Operation not allowed on a view.");
      
      processReturn();
      
      return;
    }

  // cannot rename if views are using this table
  Queue * usingViewsQueue = NULL;
  cliRC = getUsingViews(&cliInterface, extTableName, TRUE, usingViewsQueue);
  if (cliRC < 0)
    {
      processReturn();
      
      return;
    }
  
  if (usingViewsQueue->numEntries() > 0)
    {
      *CmpCommon::diags() << DgSqlCode(-1427)
			  << DgString0("Reason: Dependent views exist.");
      
      processReturn();
      return;
    }

  Int64 objUID = getObjectUID(&cliInterface,
			      catalogNamePart.data(), schemaNamePart.data(), 
			      objectNamePart.data(),
			      COM_BASE_TABLE_OBJECT_LIT);
  if (objUID < 0)
    {

      processReturn();

      return;
    }

  cliRC = updateObjectName(&cliInterface,
			   objUID,
			   catalogNamePart.data(), schemaNamePart.data(),
			   newObjectNamePart.data());
  if (cliRC < 0)
    {
      processReturn();
      
      return;
    }

  // rename the underlying hbase object
  HbaseStr hbaseTable;
  hbaseTable.val = (char*)extNameForHbase.data();
  hbaseTable.len = extNameForHbase.length();
  
  HbaseStr newHbaseTable;
  newHbaseTable.val = (char*)newExtNameForHbase.data();
  newHbaseTable.len = newExtNameForHbase.length();

  retcode = ehi->copy(hbaseTable, newHbaseTable);
  if (retcode < 0)
    {
      *CmpCommon::diags() << DgSqlCode(-8448)
			  << DgString0((char*)"ExpHbaseInterface::copy()")
			  << DgString1(getHbaseErrStr(-retcode))
			  << DgInt0(-retcode)
			  << DgString2((char*)GetCliGlobals()->getJniErrorStr().data());
      
      deallocEHI(ehi); 
      
      processReturn();
      
      return;
    }

  retcode = dropHbaseTable(ehi, &hbaseTable);
  if (retcode < 0)
    {
      return;
    }

  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);
  ActiveSchemaDB()->getNATableDB()->removeNATable(newcn);

  return;
}

void CmpSeabaseDDL::alterSeabaseTableAddColumn(
					       StmtDDLAlterTableAddColumn * alterAddColNode,
					       NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  const NAString &tabName = alterAddColNode->getTableName();

  ComObjectName tableName(tabName, COM_TABLE_NAME);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);

  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);

  ExeCliInterface cliInterface(STMTHEAP);

  if ((isSeabaseReservedSchema(tableName)) &&
      (!Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_CANNOT_ALTER_DEFINITION_METADATA_SCHEMA);

      return;
    }

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();
      
      return;
    }

  ActiveSchemaDB()->getNATableDB()->useCache();

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  CorrName cn(tableName.getObjectNamePart().getInternalName(),
	      STMTHEAP,
	      tableName.getSchemaNamePart().getInternalName(),
	      tableName.getCatalogNamePart().getInternalName());

  NATable *naTable = bindWA.getNATable(cn); 
  if (naTable == NULL || bindWA.errStatus())
    {
      CmpCommon::diags()->clear();

      *CmpCommon::diags()
	<< DgSqlCode(-4082)
	<< DgTableName(cn.getExposedNameAsAnsiString());

      processReturn();

      return;
    }

  const NAColumnArray &nacolArr = naTable->getNAColumnArray();

  ElemDDLColDefArray ColDefArray = alterAddColNode->getColDefArray();
  ElemDDLColDef *pColDef = ColDefArray[0];

  // Do not allow to using a NOT NULL constraint without a default
  // clause.  Do not allow DEFAULT NULL together with NOT NULL.
  if (pColDef->getIsConstraintNotNullSpecified())
    {
      if (pColDef->getDefaultClauseStatus() != ElemDDLColDef::DEFAULT_CLAUSE_SPEC)
	{
	  *CmpCommon::diags() << DgSqlCode(-CAT_DEFAULT_REQUIRED);

	  processReturn();

	  return;
	}
      ConstValue *pDefVal = (ConstValue *)pColDef->getDefaultValueExpr();
      
      if ((pDefVal->origOpType() != ITM_CURRENT_USER) &&
	  (pDefVal->origOpType() != ITM_CURRENT_TIMESTAMP) &&
	  (pDefVal->origOpType() != ITM_CAST)) 
	{
	  if (pDefVal->isNull()) 
	    {
	      *CmpCommon::diags() << DgSqlCode(-CAT_CANNOT_BE_DEFAULT_NULL_AND_NOT_NULL);

	      processReturn();

	      return;
	    }
	}
    }
  
  //Do not allow NO DEFAULT
  if (pColDef->getDefaultClauseStatus() == ElemDDLColDef::NO_DEFAULT_CLAUSE_SPEC)
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_DEFAULT_REQUIRED);

      processReturn();

      return;
    }

  char query[4000];

  NAString colName;
  Lng32 datatype, length, precision, scale, dt_start, dt_end, nullable, upshifted;
  ComColumnDefaultClass defaultClass;
  NAString charset, defVal;
  NAString colClass;
  NAString heading;
  ULng32 colFlags;
  LobsStorage lobStorage;
  if (getColInfo(pColDef,
		 colName, 
		 datatype, length, precision, scale, dt_start, dt_end, upshifted, nullable,
		 charset, defaultClass, defVal, heading, lobStorage, colFlags))
    {
      processReturn();
      
      return;
    }

  const NAColumn * nacol = nacolArr.getColumn(colName);
  if (nacol)
    {
      // column exists. Error or return, depending on 'if not exists' option.
      if (NOT alterAddColNode->addIfNotExists())
	{
	  *CmpCommon::diags() << DgSqlCode(-CAT_DUPLICATE_COLUMNS)
			      << DgColumnName(colName);
	}
      
      processReturn();

      return;
    }

  char * col_name = new(STMTHEAP) char[colName.length() + 1];
  strcpy(col_name, (char*)colName.data());

  char * def_val = NULL;
  if (defVal.length() > 0)
    {
      def_val = new(STMTHEAP) char[defVal.length() + 1];
      str_cpy_all(def_val, (char*)defVal.data(), defVal.length());
      def_val[defVal.length()] = 0;
    }

  str_sprintf(query, "insert into %s.\"%s\".%s values (%Ld, '%s', %d, '%s', %d, %d, %d, %d, %d, %d, '%s', %d, %d, '%s', %d, %s'%s', '%s', '%s', '%d', '%s', '%s' )",
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_COLUMNS,
	      naTable->objectUid().castToInt64(), 
	      col_name,
	      naTable->getColumnCount(), 
	      COM_ADDED_USER_COLUMN_LIT,
	      datatype,
	      length,
	      precision,
	      scale,
	      dt_start,
	      dt_end,
	      (upshifted ? "Y" : "N"),
	      colFlags, 
	      nullable,
	      (char*)charset.data(),
	      (Lng32)defaultClass,
	      def_val ? "_UCS2 X": " ",
	      def_val ? def_val : " ",
	      (heading.isNull() ? "" : heading.data()),
	      SEABASE_DEFAULT_COL_FAMILY,
	      naTable->getColumnCount()+1,
              COM_UNKNOWN_PARAM_DIRECTION_LIT,
              "N"
              );
  
  cliRC = cliInterface.executeImmediate(query);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

      processReturn();

      return;
    }

  //  CorrName cn(objectNamePart, STMTHEAP, schemaNamePart, catalogNamePart);
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);

  if ((alterAddColNode->getAddConstraintPK()) OR
      (alterAddColNode->getAddConstraintCheckArray().entries() NEQ 0) OR
      (alterAddColNode->getAddConstraintUniqueArray().entries() NEQ 0) OR
      (alterAddColNode->getAddConstraintRIArray().entries() NEQ 0))
    {
      cliRC = cliInterface.holdAndSetCQD("TRAF_RELOAD_NATABLE_CACHE", "ON");
      if (cliRC < 0)
	{
	  cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
	  
	  processReturn();
	  
	  goto label_return;
	}

      addConstraints(tableName, currCatAnsiName, currSchAnsiName,
		     alterAddColNode->getAddConstraintPK(),
		     alterAddColNode->getAddConstraintUniqueArray(),
		     alterAddColNode->getAddConstraintRIArray(),
		     alterAddColNode->getAddConstraintCheckArray());		     

      cliRC = cliInterface.restoreCQD("TRAF_RELOAD_NATABLE_CACHE");

      if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_))
	return;
    }

 label_return:
  processReturn();

  return;
}

void CmpSeabaseDDL::alterSeabaseTableDropColumn(
						StmtDDLAlterTableDropColumn * alterDropColNode,
						NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  const NAString &tabName = alterDropColNode->getTableName();

  ComObjectName tableName(tabName, COM_TABLE_NAME);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);

  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);
  const NAString extNameForHbase = catalogNamePart + "." + schemaNamePart + "." + objectNamePart;

  ExeCliInterface cliInterface(STMTHEAP);

  if ((isSeabaseReservedSchema(tableName)) &&
      (!Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_CANNOT_ALTER_DEFINITION_METADATA_SCHEMA);

      return;
    }

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();
      
      return;
    }

  ActiveSchemaDB()->getNATableDB()->useCache();

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  CorrName cn(tableName.getObjectNamePart().getInternalName(),
	      STMTHEAP,
	      tableName.getSchemaNamePart().getInternalName(),
	      tableName.getCatalogNamePart().getInternalName());

  NATable *naTable = bindWA.getNATable(cn); 
  if (naTable == NULL || bindWA.errStatus())
    {
      *CmpCommon::diags()
	<< DgSqlCode(-4082)
	<< DgTableName(cn.getExposedNameAsAnsiString());
    
      processReturn();

      return;
    }

  const NAColumnArray &nacolArr = naTable->getNAColumnArray();
  const NAString &colName = alterDropColNode->getColName();

  const NAColumn * nacol = nacolArr.getColumn(colName);
  if (! nacol)
    {
      // column doesnt exist. Error or return, depending on 'if exists' option.
      if (NOT alterDropColNode->dropIfExists())
	{
	  *CmpCommon::diags() << DgSqlCode(-CAT_COLUMN_DOES_NOT_EXIST_ERROR)
			      << DgColumnName(colName);
	}

      processReturn();

      return;
    }

  const NAFileSet * naFS = naTable->getClusteringIndex();
  const NAColumnArray &naKeyColArr = naFS->getIndexKeyColumns();
  if (naKeyColArr.getColumn(colName))
    {
      // key column cannot be dropped
      *CmpCommon::diags() << DgSqlCode(-1420)
			  << DgColumnName(colName);

      processReturn();

      return;
    }

  if (naTable->hasSecondaryIndexes())
    {
      const NAFileSetList &naFsList = naTable->getIndexList();

      for (Lng32 i = 0; i < naFsList.entries(); i++)
	{
	  naFS = naFsList[i];

	  const NAColumnArray &naIndexColArr = naFS->getAllColumns();
	  if (naIndexColArr.getColumn(colName))
	    {
	      // secondary index column cannot be dropped
	      *CmpCommon::diags() << DgSqlCode(-1421)
				  << DgColumnName(colName)
				  << DgTableName(naFS->getExtFileSetName());

	      processReturn();

	      return;
	    }
	} // for
    } // secondary indexes present

  if ((naTable->getClusteringIndex()->hasSyskey()) &&
      (nacolArr.entries() == 2))
    {
      // this table has one SYSKEY column and one other column.
      // Dropping that column will leave the table with no user column.
      // Return an error.
      *CmpCommon::diags() << DgSqlCode(-1424)
			  << DgColumnName(colName);
    }

  Int64 objUID = naTable->objectUid().castToInt64();

  if (isSeabaseMD(tableName))
    {
      // objectUID for a metadata table is not available in NATable struct
      // since the definition for a MD table is hardcoded.
      // get objectUID for metadata tables from the OBJECTS table.
      objUID =
	getObjectUID(&cliInterface,
		     catalogNamePart.data(), schemaNamePart.data(), objectNamePart.data(),
		     COM_BASE_TABLE_OBJECT_LIT);
    }

  Lng32 colNumber = nacol->getPosition();

  char buf[4000];
  str_sprintf(buf, "delete from %s.\"%s\".%s where object_uid = %Ld and column_number = %d",
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_COLUMNS,
	      objUID,
	      colNumber);

  cliRC = cliInterface.executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

      processReturn();

      return;
    }

  str_sprintf(buf, "update %s.\"%s\".%s set column_number = column_number - 1 where object_uid = %Ld and column_number >= %d",
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_COLUMNS,
	      objUID,
	      colNumber);

  cliRC = cliInterface.executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

      processReturn();

      return;
    }

  str_sprintf(buf, "update %s.\"%s\".%s set column_number = column_number - 1 where object_uid = %Ld and column_number >= %d",
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_KEYS,
	      objUID,
	      colNumber);

  cliRC = cliInterface.executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

      processReturn();

      return;
    }

  // remove column from all rows of the base table
  HbaseStr hbaseTable;
  hbaseTable.val = (char*)extNameForHbase.data();
  hbaseTable.len = extNameForHbase.length();

  //  Text column("cf1:");

  //  Text column(SEABASE_DEFAULT_COL_FAMILY);
  //  column.append(":");
  //  column.append(colName);

  Text column(nacol->getHbaseColFam());
  column.append(":");
  //  column.append(nacol->getHbaseColQual());

  char * colQualPtr = (char*)nacol->getHbaseColQual().data();
  Lng32 colQualLen = nacol->getHbaseColQual().length();
  Int64 colQval = str_atoi(colQualPtr, colQualLen);
  if (colQval <= UCHAR_MAX)
    {
      unsigned char c = (unsigned char)colQval;
      column.append((char*)&c, 1);
    }
  else if (colQval <= USHRT_MAX)
    {
      unsigned short s = (unsigned short)colQval;
      column.append((char*)&s, 2);
    }
  else if (colQval <= ULONG_MAX)
    {
      Lng32 l = (Lng32)colQval;
      column.append((char*)&l, 4);
    }
  else
    column.append((char*)&colQval, 8);
  
  retcode = ehi->deleteColumns(hbaseTable, column);
  if (retcode < 0)
    {
      *CmpCommon::diags() << DgSqlCode(-8448)
			  << DgString0((char*)"ExpHbaseInterface::deleteColumns()")
			  << DgString1(getHbaseErrStr(-retcode))
			  << DgInt0(-retcode)
			  << DgString2((char*)GetCliGlobals()->getJniErrorStr().data());
      
      deallocEHI(ehi); 

      processReturn();

      return;
    }

  deallocEHI(ehi); 

  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);

  processReturn();

  return;
}

void CmpSeabaseDDL::alterSeabaseTableAddPKeyConstraint(
						       StmtDDLAddConstraint * alterAddConstraint,
						       NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  const NAString &tabName = alterAddConstraint->getTableName();

  ComObjectName tableName(tabName, COM_TABLE_NAME);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);

  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);
  const NAString extNameForHbase = catalogNamePart + "." + schemaNamePart + "." + objectNamePart;

  ExeCliInterface cliInterface(STMTHEAP);

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();
      
      return;
    }

  if (CmpCommon::getDefault(TRAF_RELOAD_NATABLE_CACHE) == DF_OFF)
    ActiveSchemaDB()->getNATableDB()->useCache();

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  CorrName cn(tableName.getObjectNamePart().getInternalName(),
	      STMTHEAP,
	      tableName.getSchemaNamePart().getInternalName(),
	      tableName.getCatalogNamePart().getInternalName());

  NATable *naTable = bindWA.getNATable(cn); 
  if (naTable == NULL || bindWA.errStatus())
    {
      *CmpCommon::diags()
	<< DgSqlCode(-4082)
	<< DgTableName(cn.getExposedNameAsAnsiString());

      deallocEHI(ehi); 

      processReturn();
      
      return;
    }

  ElemDDLColRefArray &keyColumnArray = alterAddConstraint->getConstraint()->castToElemDDLConstraintPK()->getKeyColumnArray();

  NAList<NAString> keyColList(HEAP, keyColumnArray.entries());
  NAString pkeyStr("(");
  for (Int32 j = 0; j < keyColumnArray.entries(); j++)
    {
      const NAString &colName = keyColumnArray[j]->getColumnName();
      keyColList.insert(colName);

      pkeyStr += colName;
      if (j < (keyColumnArray.entries() - 1))
	pkeyStr += ", ";
    }
  pkeyStr += ")";

  if (constraintErrorChecks(alterAddConstraint->castToStmtDDLAddConstraintUnique(),
			    naTable,
			    COM_UNIQUE_CONSTRAINT, //TRUE, 
			    keyColList))
    {
      return;
    }

 // update unique key constraint info
  NAString uniqueStr;
  if (genUniqueName(alterAddConstraint, uniqueStr))
    {
      return;
    }

  // if table doesnt have a user defined primary key, is empty and doesn't have any 
  // dependent objects (index, views, triggers, RI, etc), then drop it and recreate it with 
  // this new primary key.
  char query[2000];
  str_sprintf(query, "select [any 1] cast(1 as int not null) from \"%s\".\"%s\".\"%s\" for read committed access",
	      catalogNamePart.data(), schemaNamePart.data(), objectNamePart.data());
  Lng32 len = 0;
  Lng32 rowCount = 0;
  cliRC = cliInterface.executeImmediate(query, (char*)&rowCount, &len, NULL);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return;
    }
  
  // if the table is not empty, or there are dependent objects/constraints,
  //  or the table already has  a pkey/store by, then create a unique constraint.
  NABoolean isStoreBy = FALSE;
  Int32 nonSystemKeyCols = 0;
  if (naTable->getClusteringIndex())
    {
      NAFileSet * naf = naTable->getClusteringIndex();
      for (Lng32 i = 0; i < naf->getIndexKeyColumns().entries(); i++)
	{
	  NAColumn * nac = naf->getIndexKeyColumns()[i];
   
	  if (NOT nac->isSystemColumn())
	    nonSystemKeyCols++;
	  else if (nac->isSyskeyColumn())
	    isStoreBy = TRUE;
	} // for

      if (nonSystemKeyCols == 0)
	isStoreBy = FALSE;
    } // if
  
  if ((rowCount > 0) || // not empty
      (naTable->hasSecondaryIndexes()) || // user indexes
      (NOT naTable->getClusteringIndex()->hasSyskey()) || // user defined pkey
      (isStoreBy) ||     // user defined store by
      (naTable->getUniqueConstraints().entries() > 0) || // unique constraints
      (naTable->getRefConstraints().entries() > 0) || // ref constraints
      (naTable->getCheckConstraints().entries() > 0))
    {
      // cannot create clustered primary key constraint.
      // create a unique constraint instead.
      return alterSeabaseTableAddUniqueConstraint(alterAddConstraint,
						  currCatName, currSchName);
    }

  Int64 tableUID = 
    getObjectUID(&cliInterface,
		 catalogNamePart.data(), schemaNamePart.data(), objectNamePart.data(),
		 COM_BASE_TABLE_OBJECT_LIT);
   
  // empty table. Drop and recreate it with the new primary key.
  char * buf = NULL;
  ULng32 buflen = 0;
  retcode = CmpDescribeSeabaseTable(cn, 3/*createlike*/, buf, buflen, STMTHEAP,
				    pkeyStr.data(), TRUE);
  if (retcode)
    return;
  
  NAString cliQuery;
  // drop this table.
  cliQuery = "drop table ";
  cliQuery += extTableName;
  cliQuery += " no check;";
  cliRC = cliInterface.executeImmediate((char*)cliQuery.data());
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return;
    }

  char cqdbuf[200];
  str_sprintf(cqdbuf, "cqd traf_create_table_with_uid '%Ld';",
	      tableUID);
  cliRC = cliInterface.executeImmediate(cqdbuf);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return;
    }

  // and recreate it with the new primary key.
  cliQuery = "create table ";
  cliQuery += extTableName;
  cliQuery += " ";

  NABoolean done = FALSE;
  Lng32 curPos = 0;
  while (NOT done)
    {
      short len = *(short*)&buf[curPos];
      NAString frag(&buf[curPos+sizeof(short)],
		    len - ((buf[curPos+len-1]== '\n') ? 1 : 0));

      cliQuery += frag;
      curPos += ((((len+sizeof(short))-1)/8)+1)*8;

      if (curPos >= buflen)
	done = TRUE;
    }

  cliRC = cliInterface.executeImmediate((char*)cliQuery.data());

  str_sprintf(cqdbuf, "cqd traf_create_table_with_uid '';");
  cliInterface.executeImmediate(cqdbuf);

  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return;
    }

  // remove NATable for this table
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);

  return;
}

void CmpSeabaseDDL::alterSeabaseTableAddUniqueConstraint(
						StmtDDLAddConstraint * alterAddConstraint,
						NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  const NAString &tabName = alterAddConstraint->getTableName();

  ComObjectName tableName(tabName, COM_TABLE_NAME);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);

  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);
  const NAString extNameForHbase = catalogNamePart + "." + schemaNamePart + "." + objectNamePart;

  ExeCliInterface cliInterface(STMTHEAP);

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();
      
      return;
    }

  if (CmpCommon::getDefault(TRAF_RELOAD_NATABLE_CACHE) == DF_OFF)
    ActiveSchemaDB()->getNATableDB()->useCache();

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  CorrName cn(tableName.getObjectNamePart().getInternalName(),
	      STMTHEAP,
	      tableName.getSchemaNamePart().getInternalName(),
	      tableName.getCatalogNamePart().getInternalName());

  NATable *naTable = bindWA.getNATable(cn); 
  if (naTable == NULL || bindWA.errStatus())
    {
      *CmpCommon::diags()
	<< DgSqlCode(-4082)
	<< DgTableName(cn.getExposedNameAsAnsiString());

      deallocEHI(ehi); 

      processReturn();
      
      return;
    }

  ElemDDLColRefArray &keyColumnArray = alterAddConstraint->getConstraint()->castToElemDDLConstraintUnique()->getKeyColumnArray();

  NAList<NAString> keyColList(HEAP, keyColumnArray.entries());
  NAList<NAString> keyColOrderList(HEAP, keyColumnArray.entries());
  for (Int32 j = 0; j < keyColumnArray.entries(); j++)
    {
      const NAString &colName = keyColumnArray[j]->getColumnName();
      keyColList.insert(colName);

      if (keyColumnArray[j]->getColumnOrdering() == COM_DESCENDING_ORDER)
	keyColOrderList.insert("DESC");
      else
	keyColOrderList.insert("ASC");
    }

  if (constraintErrorChecks(alterAddConstraint->castToStmtDDLAddConstraintUnique(),
			    naTable,
			    COM_UNIQUE_CONSTRAINT, 
			    keyColList))
    {
      return;
    }

 // update unique key constraint info
  NAString uniqueStr;
  if (genUniqueName(alterAddConstraint, uniqueStr))
    {
      return;
    }
  
  Int64 tableUID = 
    getObjectUID(&cliInterface,
		 catalogNamePart.data(), schemaNamePart.data(), objectNamePart.data(),
		 COM_BASE_TABLE_OBJECT_LIT);

  ComUID comUID;
  comUID.make_UID();
  Int64 uniqueUID = comUID.get_value();

  if (updateConstraintMD(keyColList, keyColOrderList, uniqueStr, tableUID, uniqueUID, 
			 naTable, COM_UNIQUE_CONSTRAINT, &cliInterface))
    {
      *CmpCommon::diags()
	<< DgSqlCode(-1029)
	<< DgTableName(uniqueStr);

      return;
    }

  NAList<NAString> emptyKeyColList;
  if (updateIndexInfo(keyColList,
		      keyColOrderList,
		      emptyKeyColList,
		      uniqueStr,
		      uniqueUID,
		      catalogNamePart, schemaNamePart, objectNamePart,
		      naTable,
		      TRUE,
		      (CmpCommon::getDefault(TRAF_NO_CONSTR_VALIDATION) == DF_ON),
		      &cliInterface))
    {
      *CmpCommon::diags()
	<< DgSqlCode(-1029)
	<< DgTableName(uniqueStr);

      return;
    }

  // remove NATable for this table
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);

  return;
}

// returns 1 if referenced table refdTable has a dependency on the 
// original referencing table origRingTable.
// return 0, if it does not.
// return -1, if error.
short CmpSeabaseDDL::isCircularDependent(CorrName &ringTable,
					 CorrName &refdTable,
					 CorrName &origRingTable,
					 BindWA *bindWA)
{
  // get natable for the referenced table.
  NATable *naTable = bindWA->getNATable(refdTable); 
  if (naTable == NULL || bindWA->errStatus())
    {
      *CmpCommon::diags() << DgSqlCode(-1389)
			  << DgString0(naTable->getTableName().getQualifiedNameAsString());
      
      processReturn();
      
      return -1;
    }
  
  // find all the tables the refdTable depends on.
  const AbstractRIConstraintList &refList = naTable->getRefConstraints();
  
  for (Int32 i = 0; i < refList.entries(); i++)
    {
      AbstractRIConstraint *ariConstr = refList[i];
      
      if (ariConstr->getOperatorType() != ITM_REF_CONSTRAINT)
	continue;

      RefConstraint * refConstr = (RefConstraint*)ariConstr;

      //      CorrName cn(refConstr->getDefiningTableName());
      CorrName cn(refConstr->getUniqueConstraintReferencedByMe().getTableName());
      if (cn == origRingTable)
	{
	  return 1; // dependency exists
	}
      short rc = isCircularDependent(cn, cn, 
				     origRingTable, bindWA);
      if (rc)
	return rc;

    } // for

  return 0;
}

void CmpSeabaseDDL::alterSeabaseTableAddRIConstraint(
						StmtDDLAddConstraint * alterAddConstraint,
						NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  const NAString &tabName = alterAddConstraint->getTableName();

  ComObjectName referencingTableName(tabName, COM_TABLE_NAME);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  referencingTableName.applyDefaults(currCatAnsiName, currSchAnsiName);

  const NAString catalogNamePart = referencingTableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = referencingTableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = referencingTableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = referencingTableName.getExternalName(TRUE);
  const NAString extNameForHbase = catalogNamePart + "." + schemaNamePart + "." + objectNamePart;

  ExeCliInterface cliInterface(STMTHEAP);

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();
      
      return;
    }

  if (CmpCommon::getDefault(TRAF_RELOAD_NATABLE_CACHE) == DF_OFF)
    ActiveSchemaDB()->getNATableDB()->useCache();

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  CorrName cn(referencingTableName.getObjectNamePart().getInternalName(),
	      STMTHEAP,
	      referencingTableName.getSchemaNamePart().getInternalName(),
	      referencingTableName.getCatalogNamePart().getInternalName());

  NATable *ringNaTable = bindWA.getNATable(cn); 
  if (ringNaTable == NULL || bindWA.errStatus())
    {
      *CmpCommon::diags()
	<< DgSqlCode(-4082)
	<< DgTableName(cn.getExposedNameAsAnsiString());

      deallocEHI(ehi); 

      processReturn();
      
      return;
    }

  const ElemDDLConstraintRI *constraintNode = 
    alterAddConstraint->getConstraint()->castToElemDDLConstraintRI();
  ComObjectName referencedTableName( constraintNode->getReferencedTableName()
				     , COM_TABLE_NAME);
  referencedTableName.applyDefaults(currCatAnsiName, currSchAnsiName);

  CorrName cn2(referencedTableName.getObjectNamePart().getInternalName(),
	      STMTHEAP,
	      referencedTableName.getSchemaNamePart().getInternalName(),
	      referencedTableName.getCatalogNamePart().getInternalName());

  NATable *refdNaTable = bindWA.getNATable(cn2); 
  if (refdNaTable == NULL || bindWA.errStatus())
    {
      *CmpCommon::diags()
	<< DgSqlCode(-4082)
	<< DgTableName(cn2.getExposedNameAsAnsiString());

      deallocEHI(ehi); 

      processReturn();
      
      return;
    }

  // If the referenced and referencing tables are the same, 
  // reject the request.  At this time, we do not allow self
  // referencing constraints.
  if (referencingTableName == referencedTableName)
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_SELF_REFERENCING_CONSTRAINT);

      processReturn();
      
      return;
    }

  ElemDDLColNameArray &ringCols = alterAddConstraint->getConstraint()->castToElemDDLConstraintRI()->getReferencingColumns();

  NAList<NAString> ringKeyColList(HEAP, ringCols.entries());
  NAList<NAString> ringKeyColOrderList(HEAP, ringCols.entries());
  NAString ringColListForValidation;
  NAString ringNullList;
  for (Int32 j = 0; j < ringCols.entries(); j++)
    {
      const NAString &colName = ringCols[j]->getColumnName();
      ringKeyColList.insert(colName);
      ringKeyColOrderList.insert("ASC");

      ringColListForValidation += "\"";
      ringColListForValidation += colName;
      ringColListForValidation += "\"";
      if (j < (ringCols.entries() - 1))
	ringColListForValidation += ", ";

      ringNullList += "or ";
      ringNullList += "\"";
      ringNullList += colName;
      ringNullList += "\"";
      ringNullList += " is null ";
    }

  if (constraintErrorChecks(alterAddConstraint->castToStmtDDLAddConstraintRI(),
			    ringNaTable,
			    COM_FOREIGN_KEY_CONSTRAINT, //FALSE, // referencing constr
			    ringKeyColList))
    {
      return;
    }

 const NAString &addConstrName = alterAddConstraint->
    getConstraintNameAsQualifiedName().getQualifiedNameAsAnsiString();

  // Compare the referenced column list against the primary and unique
  // constraint defined for the referenced table.  The referenced 
  // column list must match one of these constraints.  Note that if there
  // was no referenced column list specified, the primary key is used
  // and a match has automatically been found.

  NABoolean matchFound = FALSE;

  const ElemDDLColNameArray &referencedColNode = 
    constraintNode->getReferencedColumns();

  NAList<NAString> refdKeyColList(HEAP, referencedColNode.entries());
  NAString refdColListForValidation;
  for (Int32 j = 0; j < referencedColNode.entries(); j++)
    {
      const NAString &colName = referencedColNode[j]->getColumnName();
      refdKeyColList.insert(colName);

      refdColListForValidation += "\"";
      refdColListForValidation += colName;
      refdColListForValidation += "\"";
      if (j < (referencedColNode.entries() - 1))
	refdColListForValidation += ", ";
    }

  if (referencedColNode.entries() == 0)
    {
      NAFileSet * naf = refdNaTable->getClusteringIndex();
      for (Lng32 i = 0; i < naf->getIndexKeyColumns().entries(); i++)
	{
	  NAColumn * nac = naf->getIndexKeyColumns()[i];

	  const NAString &colName = nac->getColName();
	  refdKeyColList.insert(colName);

	  refdColListForValidation += "\"";
	  refdColListForValidation += nac->getColName();
	  refdColListForValidation += "\"";
	  if (i < (naf->getIndexKeyColumns().entries() - 1))
	    refdColListForValidation += ", ";
	}
    }

  if (ringKeyColList.entries() != refdKeyColList.entries())
    {
      *CmpCommon::diags()
	<< DgSqlCode(-1046)
	<< DgConstraintName(addConstrName);
      
      processReturn();
      
      return;
    }

  const NAColumnArray &ringNACarr = ringNaTable->getNAColumnArray();
  const NAColumnArray &refdNACarr = refdNaTable->getNAColumnArray();
  for (Int32 i = 0; i < ringKeyColList.entries(); i++)
    {
      const NAString &ringColName = ringKeyColList[i];
      const NAString &refdColName = refdKeyColList[i];

      const NAColumn * ringNAC = ringNACarr.getColumn(ringColName);
      const NAColumn * refdNAC = refdNACarr.getColumn(refdColName);

      //      if (NOT(*ringNAC->getType() == *refdNAC->getType()))
      if (NOT (ringNAC->getType()->equalIgnoreNull(*refdNAC->getType())))
	{
	  *CmpCommon::diags()
	    << DgSqlCode(-1046)
	    << DgConstraintName(addConstrName);
      
	  processReturn();
	  
	  return;
	}
    }

  // method getCorrespondingConstraint expects an empty input list if there are no
  // user specified columns. Clear the refdKeyColList before calling it.
  if (referencedColNode.entries() == 0)
    {
      refdKeyColList.clear();
    }

  NAString constrName;
  NABoolean isPkey = FALSE;
  if (refdNaTable->getCorrespondingConstraint(refdKeyColList,
					      TRUE, // unique constraint
					      &constrName,
					      &isPkey))
    matchFound = TRUE;
  
  if (NOT matchFound)
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_REFERENCED_CONSTRAINT_DOES_NOT_EXIST)
			  << DgConstraintName(addConstrName);
      
      return;
    }

  // check for circular RI dependencies.
  // check if referenced table cn2 refers back to the referencing table cn.
  retcode = isCircularDependent(cn, cn2, cn, &bindWA);
  if (retcode == 1) // dependency exists
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_RI_CIRCULAR_DEPENDENCY)
      			  << DgConstraintName(addConstrName)
			  << DgTableName(cn.getExposedNameAsAnsiString());
      
      return;
    }
  else if (retcode < 0)
    {
      // error. Diags area has been populated
      return; 
    }

  if (CmpCommon::getDefault(TRAF_NO_CONSTR_VALIDATION) == DF_OFF)
    {
      // validate data for RI constraint.
      // generate a "select" statement to validate the constraint.  For example:
      // SELECT count(*) FROM T1 
      //   WHERE NOT ((T1C1,T1C2) IN (SELECT T2C1,T2C2 FROM T2))
      //   OR T1C1 IS NULL OR T1C2 IS NULL;
      // This statement returns > 0 if there exist data violating the constraint.
      
      char * validQry =
	new(STMTHEAP) char[ringColListForValidation.length() +
			   refdColListForValidation.length() +
			   ringNullList.length() +
			   2000];
      str_sprintf(validQry, "select count(*) from \"%s\".\"%s\".\"%s\" where not ((%s) in (select %s from \"%s\".\"%s\".\"%s\")) %s;",
		  referencingTableName.getCatalogNamePart().getInternalName().data(),
		  referencingTableName.getSchemaNamePart().getInternalName().data(),
		  referencingTableName.getObjectNamePart().getInternalName().data(),
		  ringColListForValidation.data(),
		  refdColListForValidation.data(),
		  referencedTableName.getCatalogNamePart().getInternalName().data(),
		  referencedTableName.getSchemaNamePart().getInternalName().data(),
		  referencedTableName.getObjectNamePart().getInternalName().data(),
		  ringNullList.data());

      Lng32 len = 0;
      Int64 rowCount = 0;
      cliRC = cliInterface.executeImmediate(validQry, (char*)&rowCount, &len, NULL);
      if (cliRC < 0)
	{
	  cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
	  return;
	}
      
      if (rowCount > 0)
	{
	  *CmpCommon::diags() << DgSqlCode(-1143)
			      << DgConstraintName(addConstrName)
			      << DgTableName(referencingTableName.getObjectNamePart().getInternalName().data())
			      << DgString0(referencedTableName.getObjectNamePart().getInternalName().data()) 
			      << DgString1("TBD");
	  
	  return;
	}
    }

  ComObjectName refdConstrName(constrName);
  refdConstrName.applyDefaults(currCatAnsiName, currSchAnsiName);

  const NAString refdCatName = refdConstrName.getCatalogNamePartAsAnsiString();
  const NAString refdSchName = refdConstrName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString refdObjName = refdConstrName.getObjectNamePartAsAnsiString(TRUE);

 Int64 refdConstrUID = 
    getObjectUID(&cliInterface,
		 refdCatName.data(), refdSchName.data(), refdObjName.data(),
		 (isPkey ?  COM_PRIMARY_KEY_CONSTRAINT_OBJECT_LIT :
		  COM_UNIQUE_CONSTRAINT_OBJECT_LIT));

  NAString uniqueStr;
  if (genUniqueName(alterAddConstraint, uniqueStr))
    {
      return;
    }
  
  Int64 tableUID = 
    getObjectUID(&cliInterface,
		 catalogNamePart.data(), schemaNamePart.data(), objectNamePart.data(),
		 COM_BASE_TABLE_OBJECT_LIT);

  ComUID comUID;
  comUID.make_UID();
  Int64 ringConstrUID = comUID.get_value();

  if (updateConstraintMD(ringKeyColList, ringKeyColOrderList, uniqueStr, tableUID, ringConstrUID, 
			 ringNaTable, COM_FOREIGN_KEY_CONSTRAINT, &cliInterface))
    {
      *CmpCommon::diags()
	<< DgSqlCode(-1029)
	<< DgTableName(uniqueStr);

      return;
    }

  if (updateRIConstraintMD(ringConstrUID, refdConstrUID,
			   &cliInterface))
    {
      *CmpCommon::diags()
	<< DgSqlCode(-1029)
	<< DgTableName(uniqueStr);

      return;
    }

  if (updateIndexInfo(ringKeyColList,
		      ringKeyColOrderList,
		      refdKeyColList,
		      uniqueStr,
		      ringConstrUID,
		      catalogNamePart, schemaNamePart, objectNamePart,
		      ringNaTable,
		      FALSE,
		      (CmpCommon::getDefault(TRAF_NO_CONSTR_VALIDATION) == DF_ON),
		      &cliInterface))
    {
      *CmpCommon::diags()
	<< DgSqlCode(-1029)
	<< DgTableName(uniqueStr);

      return;
    }

  // remove NATable for this table
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);

  // remove natable for the table being referenced
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn2);

  return;
}

short CmpSeabaseDDL::getCheckConstraintText(StmtDDLAddConstraintCheck *addCheckNode,
					    NAString &qualifiedText)
{
  //  ComString             qualifiedText;
  const ParNameLocList &nameLocList = addCheckNode->getNameLocList();
  const ParNameLoc     *pNameLoc    = NULL;
  const char           *pInputStr   = nameLocList.getInputStringPtr();

  StringPos inputStrPos = addCheckNode->getStartPosition();
  //  CharInfo::CharSet mapCS = (CharInfo::CharSet) SqlParser_ISO_MAPPING;

  for (size_t x = 0; x < nameLocList.entries(); x++)
  {
    pNameLoc = &nameLocList[x];
    const NAString &nameExpanded = pNameLoc->getExpandedName(FALSE/*no assert*/);
    size_t nameAsIs = 0;
    size_t nameLenInBytes = 0;

    //
    // When the character set of the input string is a variable-length/width
    // multi-byte characters set, the value returned by getNameLength()
    // may not be numerically equal to the number of bytes in the original
    // input string that we need to skip.  So, we get the character
    // conversion routines to tell us how many bytes we need to skip.
    //
    enum cnv_charset eCnvCS = convertCharsetEnum(nameLocList.getInputStringCharSet());

    const char *str_to_test = (const char *) &pInputStr[pNameLoc->getNamePosition()];
    const int max_bytes2cnv = addCheckNode->getEndPosition()
      - pNameLoc->getNamePosition() + 1;
    const char *tmp_out_bufr = new (STMTHEAP) char[max_bytes2cnv * 4 + 10 /* Ensure big enough! */ ];
    char * p1stUnstranslatedChar = NULL;
    
    int cnvErrStatus = LocaleToUTF16(
				     cnv_version1          // in  - const enum cnv_version version
				     , str_to_test           // in  - const char *in_bufr
				     , max_bytes2cnv         // in  - const int in_len
				     , tmp_out_bufr          // out - const char *out_bufr
				     , max_bytes2cnv * 4     // in  - const int out_len
				     , eCnvCS                // in  - enum cnv_charset charset
				     , p1stUnstranslatedChar // out - char * & first_untranslated_char
				     , NULL                  // out - unsigned int *output_data_len_p
				     , 0                     // in  - const int cnv_flags
				     , (int)FALSE            // in  - const int addNullAtEnd_flag
				     , NULL                  // out - unsigned int * translated_char_cnt_p
				     , pNameLoc->getNameLength() // in - unsigned int max_chars_to_convert
				     );
    // NOTE: No errors should be possible -- string has been converted before.
    
    NADELETEBASIC (tmp_out_bufr, STMTHEAP);
    nameLenInBytes = p1stUnstranslatedChar - str_to_test;
    
    // If name not expanded, then use the original name as is
    if (nameExpanded.isNull())
      nameAsIs = nameLenInBytes;
    
    // Copy from (last position in) input string up to current name
    qualifiedText += ComString(&pInputStr[inputStrPos],
                               pNameLoc->getNamePosition() - inputStrPos +
                               nameAsIs);
    
    if (NOT nameAsIs) // original name to be replaced with expanded
      {
	size_t namePos = pNameLoc->getNamePosition();
      size_t nameLen = pNameLoc->getNameLength();
      // Solution 10-080506-3000
      // For description and explanation of the fix, please read the
      // comments in method CatExecCreateView::buildViewText() in
      // module CatExecCreateView.cpp
      // Example: CREATE TABLE T ("c1" INT NOT NULL PRIMARY KEY,
      //          C2 INT CHECK (C2 BETWEEN 0 AND"c1")) NO PARTITION;
      if ( pInputStr[namePos] EQU '"'
           AND nameExpanded.data()[0] NEQ '"'
           AND namePos > 1
           AND ( pInputStr[namePos - 1] EQU '_' OR
                 isAlNumIsoMapCS((unsigned char)pInputStr[namePos - 1]) )
         )
      {
        // insert a blank separator to avoid syntax error
        // WITHOUT FIX - Example:
        // ... ALTER TABLE CAT.SCH.T ADD CONSTRAINT CAT.SCH.T_788388997_8627
        // CHECK (CAT.SCH.T.C2 BETWEEN 0 ANDCAT.SCH.T."c1") DROPPABLE ;
        // ...                           ^^^^^^
        qualifiedText += " "; // the FIX
        // WITH FIX - Example:
        // ... ALTER TABLE CAT.SCH.T ADD CONSTRAINT CAT.SCH.T_788388997_8627
        // CHECK (CAT.SCH.T.C2 BETWEEN 0 AND CAT.SCH.T."c1") DROPPABLE ;
        // ...                             ^^^
      }

      qualifiedText += nameExpanded;

      // Problem reported in solution 10-080506-3000
      // Example: CREATE TABLE T (C1 INT NOT NULL PRIMARY KEY,
      //          C2 INT CHECK ("C2"IN(1,2,3))) NO PARTITION;
      if ( pInputStr[namePos + nameLen - 1] EQU '"'
           AND nameExpanded.data()[nameExpanded.length() - 1] NEQ '"'
           AND pInputStr[namePos + nameLen] NEQ '\0'
           AND ( pInputStr[namePos + nameLen] EQU '_' OR
                 isAlNumIsoMapCS((unsigned char)pInputStr[namePos + nameLen]) )
         )
      {
        // insert a blank separator to avoid syntax error
        // WITHOUT FIX - Example:
        // ... ALTER TABLE CAT.SCH.T ADD CONSTRAINT CAT.SCH.T_654532688_9627
        // CHECK (CAT.SCH.T.C2IN (1, 2, 3)) DROPPABLE ;
        // ...              ^^^^
        qualifiedText += " "; // the FIX
        // WITH FIX - Example:
        // ... ALTER TABLE CAT.SCH.T ADD CONSTRAINT CAT.SCH.T_654532688_9627
        // CHECK (CAT.SCH.T.C2 IN (1, 2, 3)) DROPPABLE ;
        // ...               ^^^
      }
    } // if (NOT nameAsIs)

    inputStrPos = pNameLoc->getNamePosition() + nameLenInBytes;

  } // for

  //  CAT_ASSERT(addCheckNode->getEndPosition() >= inputStrPos);
  qualifiedText += ComString(&pInputStr[inputStrPos],
			     addCheckNode->getEndPosition() - inputStrPos + 1);
  
  PrettifySqlText(qualifiedText, NULL);

  //                  CharType::getCharSetAsPrefix(SqlParser_NATIONAL_CHARSET));
  
  return 0;
}

short CmpSeabaseDDL::getTextFromMD(
				   ExeCliInterface * cliInterface,
				   Int64 constrUID,
				   NAString &constrText)
{
  Lng32 cliRC;

  char query[1000];

  str_sprintf(query, "select text from %s.\"%s\".%s where object_uid = %Ld for read committed access order by seq_num",
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TEXT,
	      constrUID);
  
  Queue * constrTextQueue = NULL;
  cliRC = cliInterface->fetchAllRows(constrTextQueue, query, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

      processReturn();

      return -1;
    }
  
  // glue text together
  for (Lng32 idx = 0; idx < constrTextQueue->numEntries(); idx++)
    {
      OutputInfo * vi = (OutputInfo*)constrTextQueue->getNext(); 
    
      char * text = (char*)vi->get(0);
   
      constrText += text;
    }

  return 0;
}

void CmpSeabaseDDL::alterSeabaseTableAddCheckConstraint(
						StmtDDLAddConstraint * alterAddConstraint,
						NAString &currCatName, NAString &currSchName)
{
  StmtDDLAddConstraintCheck *alterAddCheckNode = alterAddConstraint
    ->castToStmtDDLAddConstraintCheck();

  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  const NAString &tabName = alterAddConstraint->getTableName();

  ComObjectName tableName(tabName, COM_TABLE_NAME);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);

  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);
  const NAString extNameForHbase = catalogNamePart + "." + schemaNamePart + "." + objectNamePart;

  ExeCliInterface cliInterface(STMTHEAP);

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();
      
      return;
    }

  if (CmpCommon::getDefault(TRAF_RELOAD_NATABLE_CACHE) == DF_OFF)
    ActiveSchemaDB()->getNATableDB()->useCache();

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  CorrName cn(tableName.getObjectNamePart().getInternalName(),
	      STMTHEAP,
	      tableName.getSchemaNamePart().getInternalName(),
	      tableName.getCatalogNamePart().getInternalName());

  NATable *naTable = bindWA.getNATable(cn); 
  if (naTable == NULL || bindWA.errStatus())
    {
      *CmpCommon::diags()
	<< DgSqlCode(-4082)
	<< DgTableName(cn.getExposedNameAsAnsiString());

      deallocEHI(ehi); 

      processReturn();
      
      return;
    }

  const ParCheckConstraintColUsageList &colList = 
    alterAddCheckNode->getColumnUsageList();
  for (CollIndex cols = 0; cols < colList.entries(); cols++)
    {
      const ParCheckConstraintColUsage &ckColUsg = colList[cols];
      const ComString &colName = ckColUsg.getColumnName();
      if ((colName EQU "SYSKEY") &&
	  (naTable->getClusteringIndex()->hasSyskey()))
	{
	  *CmpCommon::diags() << DgSqlCode(-CAT_SYSKEY_COL_NOT_ALLOWED_IN_CK_CNSTRNT)
			      << DgColumnName( "SYSKEY")
			      << DgTableName(extTableName);
	  
	  processReturn();
	  
	  deallocEHI(ehi); 

	  return;
	}
    }

  NAList<NAString> keyColList;
  if (constraintErrorChecks(alterAddConstraint->castToStmtDDLAddConstraintCheck(),
			    naTable,
			    COM_CHECK_CONSTRAINT, 
			    keyColList))
    {
      return;
    }

  // update check constraint info
  NAString uniqueStr;
  if (genUniqueName(alterAddConstraint, uniqueStr))
    {
      return;
    }
  
  // get check text
  NAString origCheckConstrText;
  if (getCheckConstraintText(alterAddCheckNode, origCheckConstrText))
    {
      return;
    }

  NAString checkConstrText;
  ToQuotedString(checkConstrText, origCheckConstrText, FALSE);

  if (CmpCommon::getDefault(TRAF_NO_CONSTR_VALIDATION) == DF_OFF)
    {
      // validate data for check constraint.
      // generate a "select" statement to validate the constraint.  For example:
      // SELECT count(*) FROM T1 where not checkConstrText;
      // This statement returns > 0 if there exist data violating the constraint.
      char * validQry = new(STMTHEAP) char[checkConstrText.length() + 2000];
      
      str_sprintf(validQry, "select count(*) from \"%s\".\"%s\".\"%s\" where not %s",
		  tableName.getCatalogNamePart().getInternalName().data(),
		  tableName.getSchemaNamePart().getInternalName().data(),
		  tableName.getObjectNamePart().getInternalName().data(),
		  origCheckConstrText.data());
      
      Lng32 len = 0;
      Int64 rowCount = 0;
      cliRC = cliInterface.executeImmediate(validQry, (char*)&rowCount, &len, NULL);
      if (cliRC < 0)
	{
	  cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
	  return;
	}
      
      if (rowCount > 0)
	{
	  *CmpCommon::diags() << DgSqlCode(-1083)
			      << DgConstraintName(uniqueStr);
	  return;
	}
    }
      
  Int64 tableUID = 
    getObjectUID(&cliInterface,
		 catalogNamePart.data(), schemaNamePart.data(), objectNamePart.data(),
		 COM_BASE_TABLE_OBJECT_LIT);

  ComUID comUID;
  comUID.make_UID();
  Int64 checkUID = comUID.get_value();

  NAList<NAString> emptyList;
  if (updateConstraintMD(keyColList, emptyList, uniqueStr, tableUID, checkUID, 
			 naTable, COM_CHECK_CONSTRAINT, &cliInterface))
    {
      *CmpCommon::diags()
	<< DgSqlCode(-1029)
	<< DgTableName(uniqueStr);
      
      return;
    }

  if (updateTextTable(&cliInterface, checkUID, checkConstrText))
    {
      processReturn();
      return;
    }

  // remove NATable for this table
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);

  return;
}


void CmpSeabaseDDL::alterSeabaseTableDropConstraint(
						StmtDDLDropConstraint * alterDropConstraint,
						NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  const NAString &tabName = alterDropConstraint->getTableName();

  ComObjectName tableName(tabName, COM_TABLE_NAME);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);

  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);
  const NAString extNameForHbase = catalogNamePart + "." + schemaNamePart + "." + objectNamePart;

  ExeCliInterface cliInterface(STMTHEAP);

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();
      
      return;
    }

  ActiveSchemaDB()->getNATableDB()->useCache();

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  CorrName cn(tableName.getObjectNamePart().getInternalName(),
	      STMTHEAP,
	      tableName.getSchemaNamePart().getInternalName(),
	      tableName.getCatalogNamePart().getInternalName());

  NATable *naTable = bindWA.getNATable(cn); 
  if (naTable == NULL || bindWA.errStatus())
    {
      *CmpCommon::diags()
	<< DgSqlCode(-4082)
	<< DgTableName(cn.getExposedNameAsAnsiString());

      deallocEHI(ehi); 

      processReturn();
      
      return;
    }

  const NAString &dropConstrName = alterDropConstraint->
    getConstraintNameAsQualifiedName().getQualifiedNameAsAnsiString();

  NABoolean isUniqConstr = FALSE;
  NABoolean isRefConstr = FALSE;
  NABoolean isPkeyConstr = FALSE;
  NABoolean isCheckConstr = FALSE;
  AbstractRIConstraint *ariConstr = NULL;
  const AbstractRIConstraintList &ariList = naTable->getUniqueConstraints();
  for (Int32 i = 0; ((NOT isUniqConstr) && (i < ariList.entries())); i++)
    {
      ariConstr = ariList[i];
      UniqueConstraint * uniqueConstr = (UniqueConstraint*)ariList[i];
      
      const NAString &tableConstrName = 
	uniqueConstr->getConstraintName().getQualifiedNameAsAnsiString();

      if (dropConstrName == tableConstrName)
	{
	  if (uniqueConstr->isPrimaryKeyConstraint())
	    isPkeyConstr = TRUE;

	  isUniqConstr = TRUE;
          
          if (uniqueConstr->hasRefConstraintsReferencingMe())
          {
             *CmpCommon::diags()
             << DgSqlCode(-1050);

             deallocEHI(ehi);
             processReturn();
             return;
          }
	}
    } // for

  if (NOT isUniqConstr)
    {
      const AbstractRIConstraintList &ariList2 = naTable->getRefConstraints();
      for (Int32 i = 0; ((NOT isRefConstr) && (i < ariList2.entries())); i++)
	{
	  ariConstr = ariList2[i];
	  
	  const NAString &tableConstrName = 
	    ariConstr->getConstraintName().getQualifiedNameAsAnsiString();
	  
	  if (dropConstrName == tableConstrName)
	    {
	      isRefConstr = TRUE;
	    }
	} // for
    }

  if ((NOT isUniqConstr) && (NOT isRefConstr))
    {
      const CheckConstraintList &checkList = naTable->getCheckConstraints();
      for (Int32 i = 0; i < checkList.entries(); i++)
	{
	  CheckConstraint *cc = checkList[i];
	
	  const NAString &tableConstrName = 
	    cc->getConstraintName().getQualifiedNameAsAnsiString();
	  
	  if (dropConstrName == tableConstrName)
	    {
	      isCheckConstr = TRUE;
	    }
	}
    }

  if ((NOT isUniqConstr) && (NOT isRefConstr) && (NOT isCheckConstr))
    {
      *CmpCommon::diags()
	<< DgSqlCode(-1005)
	<< DgConstraintName(dropConstrName);
      
      deallocEHI(ehi); 
      
      processReturn();
      
      return;
    }

  NAString indexName;

  // find the index that corresponds to this constraint
  if ((isUniqConstr || isRefConstr) && (NOT isPkeyConstr))
    {
      NAList<NAString> keyColList(HEAP, ariConstr->keyColumns().entries());
      NAList<NAString> keyColOrderList(HEAP, ariConstr->keyColumns().entries());
      for (Lng32 j = 0; j < ariConstr->keyColumns().entries(); j++)
	{
	  const NAString &colName = ariConstr->keyColumns()[j]->getColName();
	  keyColList.insert(colName);

	  if (ariConstr->keyColumns()[j]->getClusteringKeyOrdering() == DESCENDING)
	    keyColOrderList.insert("DESC");
	  else
	    keyColOrderList.insert("ASC");
	}
      
      if (NOT naTable->getCorrespondingIndex(keyColList,
					     FALSE, // implicit index 
					     isUniqConstr,  // look for unique index
					     TRUE, // look for pkey as well. //FALSE, // dont look for pkey
					     isRefConstr, // look for any index if ref constr
					     &indexName))
	{
	  *CmpCommon::diags()
	    << DgSqlCode(-1005)
	    << DgConstraintName("");
	  
	  deallocEHI(ehi); 
	  
	  processReturn();
	  
	  return;
	}
    }

  NATable *otherNaTable = NULL;
  Int64 otherConstrUID = 0;
  //  if (NOT isUniqConstr)
  if (isRefConstr)
    {
      RefConstraint * refConstr = (RefConstraint*)ariConstr;
      
      CorrName otherCN(refConstr->getUniqueConstraintReferencedByMe().getTableName());
      otherNaTable = bindWA.getNATable(otherCN);
      if (otherNaTable == NULL || bindWA.errStatus())
	{
	  deallocEHI(ehi); 
	  
	  processReturn();
	  
	  return;
	}

      AbstractRIConstraint * otherConstr = 
	refConstr->findConstraint(&bindWA, refConstr->getUniqueConstraintReferencedByMe());

      const NAString& otherCatName = 
	otherConstr->getConstraintName().getCatalogName();
      
      const NAString& otherSchName = 
	otherConstr->getConstraintName().getSchemaName();
      
      const NAString& otherConstrName = 
	otherConstr->getConstraintName().getObjectName();
      
      otherConstrUID = getObjectUID(&cliInterface,
				    otherCatName.data(), otherSchName.data(), otherConstrName.data(),
				    COM_UNIQUE_CONSTRAINT_OBJECT_LIT );
      if (otherConstrUID < 0)
	{
	  CmpCommon::diags()->clear();
	  otherConstrUID = getObjectUID(&cliInterface,
					otherCatName.data(), otherSchName.data(), otherConstrName.data(),
					COM_PRIMARY_KEY_CONSTRAINT_OBJECT_LIT );
	  if (otherConstrUID < 0)
	    {
	      processReturn();
	      
	      return;
	    }
	}
    }

  const NAFileSetList &indexes = naTable->getIndexList();
  NAFileSet * constrIndex = NULL;
  for (Int32 i = 0; i < indexes.entries(); i++)
    {
      if ((NOT indexName.isNull()) && (indexName == indexes[i]->getExtFileSetName()))
	constrIndex = indexes[i];
    }

  const NAString &constrCatName = alterDropConstraint->
    getConstraintNameAsQualifiedName().getCatalogName();
  const NAString &constrSchName = alterDropConstraint->
    getConstraintNameAsQualifiedName().getSchemaName();
  const NAString &constrObjName = alterDropConstraint->
    getConstraintNameAsQualifiedName().getObjectName();

  Int64 constrUID = getObjectUID(&cliInterface,
				 constrCatName.data(), constrSchName.data(), constrObjName.data(),
				 (isPkeyConstr ? COM_PRIMARY_KEY_CONSTRAINT_OBJECT_LIT :
				  (isUniqConstr ? COM_UNIQUE_CONSTRAINT_OBJECT_LIT :
				   (isRefConstr ? COM_REFERENTIAL_CONSTRAINT_OBJECT_LIT :
				    COM_CHECK_CONSTRAINT_OBJECT_LIT))));
  if (constrUID < 0)
    {
      processReturn();

      return;
    }

  if (deleteConstraintInfoFromSeabaseMDTables(&cliInterface,
					      naTable->objectUid().castToInt64(),
					      (otherNaTable ? otherNaTable->objectUid().castToInt64() : 0),
					      constrUID,
					      otherConstrUID, 
					      constrCatName,
					      constrSchName,
					      constrObjName,
					      (isPkeyConstr ? COM_PRIMARY_KEY_CONSTRAINT_OBJECT_LIT :
					       (isUniqConstr ? COM_UNIQUE_CONSTRAINT_OBJECT_LIT :
						(isRefConstr ? COM_REFERENTIAL_CONSTRAINT_OBJECT_LIT :
						 COM_CHECK_CONSTRAINT_OBJECT_LIT)))))
    {
      processReturn();
      
      return;
    }
					      
  // if the index corresponding to this constraint is an implicit index and 'no check'
  // option is not specified, drop it.
  if (((constrIndex) && ((NOT constrIndex->isCreatedExplicitly()) &&
			 (constrIndex->getKeytag() != 0))) &&
      (alterDropConstraint->getDropBehavior() != COM_NO_CHECK_DROP_BEHAVIOR))
    {
      char buf[4000];

      str_sprintf(buf, "drop index \"%s\".\"%s\".\"%s\" no check",
		  constrCatName.data(), constrSchName.data(), constrObjName.data());

      cliRC = cliInterface.executeImmediate(buf);
      
      if (cliRC < 0)
	{
	  cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
	  return;
	}
      
    }

  // remove NATable for this table
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);

  return;
}

static NABoolean dropOneTable(ExeCliInterface &cliInterface,
			      char * catName, char * schName, char * objName)
{
  Lng32 cliRC = 0;
  char buf [1000];
  
  cliRC = cliInterface.holdAndSetCQD("TRAF_RELOAD_NATABLE_CACHE", "ON");
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      
      return FALSE;
    }

  NABoolean someObjectsCouldNotBeDropped = FALSE;
  str_sprintf(buf, "drop table \"%s\".\"%s\".\"%s\" cascade",
	      catName, schName, objName);
  cliRC = cliInterface.executeImmediate(buf);
  
  if ((cliRC < 0) &&
      (cliRC != -1389))
    {
      someObjectsCouldNotBeDropped = TRUE;
    }
  
  // remove NATable for this table
  CorrName cn(catName, STMTHEAP, schName, objName);
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);
  
  cliRC = cliInterface.restoreCQD("TRAF_RELOAD_NATABLE_CACHE");

  return someObjectsCouldNotBeDropped;
}

void CmpSeabaseDDL::dropSeabaseSchema(
				      StmtDDLDropSchema                  * dropSchemaNode,
				      NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC = 0;

  ComSchemaName schemaName (dropSchemaNode->getSchemaName());
  NAString catName = schemaName.getCatalogNamePartAsAnsiString();
  ComAnsiNamePart schNameAsComAnsi = schemaName.getSchemaNamePart();
  NAString schName = schNameAsComAnsi.getInternalName();
  
  ExeCliInterface cliInterface(STMTHEAP);

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    return;

  ComObjectName objName(catName, schName, NAString("dummy"), 
                        COM_TABLE_NAME, TRUE);
  if (isSeabaseReservedSchema(objName))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_USER_CANNOT_DROP_SMD_TABLE)
			  << DgTableName(schName);

      deallocEHI(ehi);

      processReturn();
 
      return;
    }
  NABoolean isVolatile = 
    (memcmp(schName.data(), "VOLATILE_SCHEMA", strlen("VOLATILE_SCHEMA")) == 0);

  if ((isVolatile) &&
      (NOT dropSchemaNode->isVolatile()))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_RESERVED_METADATA_SCHEMA_NAME)
			  << DgTableName(schName);

      deallocEHI(ehi);

      processReturn();
 
      return;
    }

  char query[4000];

  str_sprintf(query, "select trim(object_name), trim(object_type) from %s.\"%s\".%s where catalog_name = '%s' and schema_name = '%s' and (object_type = '%s' or object_type = '%s' or object_type = '%s') for read committed access ",
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
	      (char*)catName.data(), (char*)schName.data(), 
	      COM_BASE_TABLE_OBJECT_LIT, COM_INDEX_OBJECT_LIT, COM_VIEW_OBJECT_LIT);
  
  Queue * objectsQueue = NULL;
  cliRC = cliInterface.fetchAllRows(objectsQueue, query, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

     processReturn();
 
      return;
    }

  objectsQueue->position();
  if ((dropSchemaNode->getDropBehavior() == COM_RESTRICT_DROP_BEHAVIOR) &&
      (objectsQueue->numEntries() > 0))
    {
      OutputInfo * oi = (OutputInfo*)objectsQueue->getCurr(); 
      
      *CmpCommon::diags() << DgSqlCode(-1028)
			  << DgTableName(oi->get(0));
 
      processReturn();
      
      return;
    }

  NABoolean someObjectsCouldNotBeDropped = FALSE;

  // drop views 
  objectsQueue->position();
  for (int idx = 0; idx < objectsQueue->numEntries(); idx++)
    {
      OutputInfo * vi = (OutputInfo*)objectsQueue->getNext(); 

      char * objName = vi->get(0);
      NAString objType = vi->get(1);
  
      if (objType == COM_VIEW_OBJECT_LIT)
	{
	  char buf [1000];
	  
	  str_sprintf(buf, "drop view \"%s\".\"%s\".\"%s\" cascade",
		      (char*)catName.data(), (char*)schName.data(), objName);
	  
	  cliRC = cliInterface.executeImmediate(buf);
	  if ((cliRC < 0) &&
	      (cliRC != -1389))
	    {
	      someObjectsCouldNotBeDropped = TRUE;
 	    }
	} // if
    } // for

  // drop tables 
  NABoolean histExists = FALSE;
  objectsQueue->position();
  for (int idx = 0; idx < objectsQueue->numEntries(); idx++)
    {
      OutputInfo * vi = (OutputInfo*)objectsQueue->getNext(); 

      NAString objName = vi->get(0);
      NAString objType = vi->get(1);

      // drop user objects first
      if (objType == COM_BASE_TABLE_OBJECT_LIT)
	{
	  if (NOT ((objName == HBASE_HIST_NAME) ||
		   (objName == HBASE_HISTINT_NAME)))
	    {
	      if (dropOneTable(cliInterface, 
			       (char*)catName.data(), (char*)schName.data(), (char*)objName.data()))
		someObjectsCouldNotBeDropped = TRUE;
	    }
	  else
	    histExists = TRUE;
	} // if
    } // for

  // drop indexes
  str_sprintf(query, "select trim(object_name), trim(object_type) from %s.\"%s\".%s where catalog_name = '%s' and schema_name = '%s' and object_type = '%s' for read committed access ",
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
	      (char*)catName.data(), (char*)schName.data(), 
	      COM_INDEX_OBJECT_LIT);
  
  cliRC = cliInterface.fetchAllRows(objectsQueue, query, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

      processReturn();
      
      return;
    }

  objectsQueue->position();
  for (int idx = 0; idx < objectsQueue->numEntries(); idx++)
    {
      OutputInfo * vi = (OutputInfo*)objectsQueue->getNext(); 

      char * objName = vi->get(0);
      NAString objType = vi->get(1);
  
      if (objType == COM_INDEX_OBJECT_LIT)
	{
	  char buf [1000];

	  str_sprintf(buf, "drop index \"%s\".\"%s\".\"%s\" cascade",
		      (char*)catName.data(), (char*)schName.data(), objName);
	  cliRC = cliInterface.executeImmediate(buf);

	  if ((cliRC < 0) &&
	      (cliRC != -1389))
	    {
	      someObjectsCouldNotBeDropped = TRUE;
	    }
	} // if
    } // for

  // now drop histogram objects
  if (histExists)
    {
      if (dropOneTable(cliInterface, 
		       (char*)catName.data(), (char*)schName.data(), (char*)HBASE_HISTINT_NAME))
	someObjectsCouldNotBeDropped = TRUE;
      
      if (dropOneTable(cliInterface, 
		       (char*)catName.data(), (char*)schName.data(), (char*)HBASE_HIST_NAME))
	someObjectsCouldNotBeDropped = TRUE;
    }

  processReturn();

  if (someObjectsCouldNotBeDropped)
    {
      CmpCommon::diags()->clear();
      
      *CmpCommon::diags() << DgSqlCode(-1069)
			  << DgSchemaName(catName + "." + schName);
    }
}

void CmpSeabaseDDL::seabaseGrantRevoke(
				      StmtDDLNode                  * stmtDDLNode,
				      NABoolean isGrant,
				      NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  StmtDDLGrant * grantNode = NULL;
  StmtDDLRevoke * revokeNode = NULL;
  NAString tabName;
  
  if (isGrant)
    {
      grantNode = stmtDDLNode->castToStmtDDLGrant();
      tabName = grantNode->getTableName();
    }
  else
    {
      revokeNode = stmtDDLNode->castToStmtDDLRevoke();
      tabName = revokeNode->getTableName();
    }

  ComObjectName tableName(tabName, COM_TABLE_NAME);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);

  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);
  const NAString extNameForHbase = catalogNamePart + "." + schemaNamePart + "." + objectNamePart;

  ExeCliInterface cliInterface(STMTHEAP);

  if (isSeabaseReservedSchema(tableName))
    {
      *CmpCommon::diags() << DgSqlCode(-1118)
			  << DgTableName(extTableName);

      return;
    }

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();
      
      return;
    }

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  CorrName cn(tableName.getObjectNamePart().getInternalName(),
	      STMTHEAP,
	      tableName.getSchemaNamePart().getInternalName(),
	      tableName.getCatalogNamePart().getInternalName());

  NATable *naTable = bindWA.getNATable(cn); 
  if (naTable == NULL || bindWA.errStatus())
    {
      *CmpCommon::diags()
	<< DgSqlCode(-4082)
	<< DgTableName(cn.getExposedNameAsAnsiString());

      deallocEHI(ehi); 

      processReturn();
      
      return;
    }

  ElemDDLGranteeArray & pGranteeArray = 
    (isGrant ? grantNode->getGranteeArray() : revokeNode->getGranteeArray());

  ElemDDLPrivActArray & pPrivActsArray =
    (isGrant ? grantNode->getPrivilegeActionArray() :
     revokeNode->getPrivilegeActionArray());

  NABoolean   allPrivs =
    (isGrant ? grantNode->isAllPrivilegesSpecified() : 
     revokeNode->isAllPrivilegesSpecified());


  TextVec userPermissions;
  if (allPrivs)
    {
      userPermissions.push_back("READ");
      userPermissions.push_back("WRITE");
      userPermissions.push_back("CREATE");
    }
  else
    {
      for (Lng32 i = 0; i < pPrivActsArray.entries(); i++)
	{
	  switch (pPrivActsArray[i]->getOperatorType() )
	    {
	    case ELM_PRIV_ACT_SELECT_ELEM:
	      {
		userPermissions.push_back("READ");
		break;
	      }
	      
	    case ELM_PRIV_ACT_INSERT_ELEM:
	    case ELM_PRIV_ACT_DELETE_ELEM:
	    case ELM_PRIV_ACT_UPDATE_ELEM:   
	      {
		userPermissions.push_back("WRITE");
		break;
	      }
	      
	    case ELM_PRIV_ACT_CREATE_ELEM:
	      {
		userPermissions.push_back("CREATE");
		break;
	      }
	      
	    default:
	      {
		NAString privType = "UNKNOWN";
		
		*CmpCommon::diags() << DgSqlCode(-CAT_INVALID_PRIV_FOR_OBJECT)
				    <<  DgString0(privType)
				    << DgString1(extTableName);
		
		deallocEHI(ehi); 
		
		processReturn();

		return;
	      }
	    }  // end switch
	} // for
    }

  for (CollIndex j = 0; j < pGranteeArray.entries(); j++)
    {
      NAString authName(pGranteeArray[j]->getAuthorizationIdentifier());

      if (isGrant)
	retcode = ehi->grant(authName.data(), extNameForHbase.data(), userPermissions);
      else
	retcode = ehi->revoke(authName.data(), extNameForHbase.data(), userPermissions);
	
      if (retcode < 0)
	{
	  *CmpCommon::diags() << DgSqlCode(-8448)
			      << (isGrant ? DgString0((char*)"ExpHbaseInterface::grant()") :
				  DgString0((char*)"ExpHbaseInterface::revoke()"))
			      << DgString1(getHbaseErrStr(-retcode))
			      << DgInt0(-retcode)
			      << DgString2((char*)GetCliGlobals()->getJniErrorStr().data());
	
	  deallocEHI(ehi); 
    
	  processReturn();

	  return;
	}
    }
  
  retcode = ehi->close();
  if (retcode < 0)
    {
      *CmpCommon::diags() << DgSqlCode(-8448)
			  << DgString0((char*)"ExpHbaseInterface::close()")
			  << DgString1(getHbaseErrStr(-retcode))
			  << DgInt0(-retcode)
			  << DgString2((char*)GetCliGlobals()->getJniErrorStr().data());
      
      deallocEHI(ehi); 
  
      processReturn();

      return;
    }

  deallocEHI(ehi); 
  
  processReturn();
 
  return;
}

void CmpSeabaseDDL::createNativeHbaseTable(
				       StmtDDLCreateHbaseTable * createTableNode,
				       NAString &currCatName, NAString &currSchName)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  ComObjectName tableName(createTableNode->getTableName());
  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  
  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();

      return;
    }

  const char * cf1 = NULL;
  const char * cf2 = NULL;
  const char * cf3 = NULL;
  for (Lng32 i = 0; i < createTableNode->csl()->entries(); i++)
    {
      const NAString * nas = (NAString*)(*createTableNode->csl())[i];

      switch (i)
	{
	case 0: cf1 = nas->data(); break;
	case 1: cf2 = nas->data(); break;
	case 2: cf3 = nas->data(); break;
	default: break;
	}

    }

  NAText hbaseOptionsStr;
  NAList<HbaseCreateOption*> * hbaseCreateOptions = NULL;
  if (createTableNode->getHbaseOptionsClause())
    {
      hbaseCreateOptions = 
	&createTableNode->getHbaseOptionsClause()->getHbaseOptions();
      
      for (CollIndex i = 0; i < hbaseCreateOptions->entries(); i++)
	{
	  HbaseCreateOption * hbaseOption = (*hbaseCreateOptions)[i];

	  hbaseOptionsStr += hbaseOption->key();
	  hbaseOptionsStr += " = ''";
	  hbaseOptionsStr += hbaseOption->val();
	  hbaseOptionsStr += "'' ";
	}
    }

  HbaseStr hbaseTable;
  hbaseTable.val = (char*)objectNamePart.data();
  hbaseTable.len = objectNamePart.length();
  if (createHbaseTable(ehi, &hbaseTable, cf1, cf2, cf3,
		       hbaseCreateOptions, 0, 0, NULL, FALSE) == -1)
    {
      deallocEHI(ehi); 

      processReturn();

      return;
    }

}

void CmpSeabaseDDL::dropNativeHbaseTable(
				       StmtDDLDropHbaseTable * dropTableNode,
				       NAString &currCatName, NAString &currSchName)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  ComObjectName tableName(dropTableNode->getTableName());
  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  
  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();

      return;
    }

  HbaseStr hbaseTable;
  hbaseTable.val = (char*)objectNamePart.data();
  hbaseTable.len = objectNamePart.length();
  retcode = dropHbaseTable(ehi, &hbaseTable);
  if (retcode < 0)
    {
      deallocEHI(ehi); 
      
      processReturn();
      
      return;
    }
  
}

desc_struct * CmpSeabaseDDL::getSeabaseMDTableDesc(const NAString &catName, 
						  const NAString &schName, 
						   const NAString &objName,
						   const char * objType)
{
  desc_struct * tableDesc = NULL;
  NAString schNameL = "\"";
  schNameL += schName;
  schNameL += "\"";

  ComObjectName coName(catName, schNameL, objName);
  NAString extTableName = coName.getExternalName(TRUE);

  Lng32 colInfoSize;
  const ComTdbVirtTableColumnInfo * colInfo;
  Lng32 keyInfoSize;
  const ComTdbVirtTableKeyInfo * keyInfo;

  Lng32 indexInfoSize = 0;
  const ComTdbVirtTableIndexInfo * indexInfo = NULL;
  if (NOT CmpSeabaseMDupgrade::getMDtableInfo(objName,
					      colInfoSize, colInfo,
					      keyInfoSize, keyInfo,
					      indexInfoSize, indexInfo,
					      objType))
    return NULL;

  tableDesc =
    Generator::createVirtualTableDesc
    ((char*)extTableName.data(),
     colInfoSize,
     (ComTdbVirtTableColumnInfo*)colInfo,
     keyInfoSize,
     (ComTdbVirtTableKeyInfo*)keyInfo,
     0, NULL,
     indexInfoSize, 
     (ComTdbVirtTableIndexInfo *)indexInfo);

  return tableDesc;

}

desc_struct * CmpSeabaseDDL::getSeabaseHistTableDesc(const NAString &catName, 
						     const NAString &schName, 
						     const NAString &objName)
{
  desc_struct * tableDesc = NULL;
  NAString schNameL = "\"";
  schNameL += schName;
  schNameL += "\"";

  ComObjectName coName(catName, schNameL, objName);
  NAString extTableName = coName.getExternalName(TRUE);

  Lng32 numCols = 0;
  ComTdbVirtTableColumnInfo * colInfo = NULL;
  Lng32 numKeys;
  ComTdbVirtTableKeyInfo * keyInfo;
  ComTdbVirtTableIndexInfo * indexInfo;

  Parser parser(CmpCommon::context());

  ComTdbVirtTableConstraintInfo * constrInfo = (ComTdbVirtTableConstraintInfo*)
	new(STMTHEAP) char[sizeof(ComTdbVirtTableConstraintInfo)];

  NAString constrName;
  if (objName == HBASE_HIST_NAME)
    {
      if (processDDLandCreateDescs(parser,
				   seabaseHistogramsDDL, sizeof(seabaseHistogramsDDL),
				   FALSE,
				   0, NULL, 0, NULL,
				   numCols, colInfo,
				   numKeys, keyInfo,
				   indexInfo))
	return NULL;

      constrName = HBASE_HIST_PK;
    }
  else if (objName == HBASE_HISTINT_NAME)
    {
      if (processDDLandCreateDescs(parser,
				   seabaseHistogramIntervalsDDL, sizeof(seabaseHistogramIntervalsDDL),
				   FALSE,
				   0, NULL, 0, NULL,
				   numCols, colInfo,
				   numKeys, keyInfo,
				   indexInfo))
	return NULL;
      
      constrName = HBASE_HISTINT_PK;
    }
  else
    return NULL;
  
  ComObjectName coConstrName(catName, schName, constrName);
  NAString * extConstrName = 
    new(STMTHEAP) NAString(coConstrName.getExternalName(TRUE));
  
  constrInfo->baseTableName = (char*)extTableName.data();
  constrInfo->constrName = (char*)extConstrName->data();
  constrInfo->constrType = 3; // pkey_constr

  constrInfo->colCount = numKeys;
  constrInfo->keyInfoArray = keyInfo;

  constrInfo->numRingConstr = 0;
  constrInfo->ringConstrArray = NULL;
  constrInfo->numRefdConstr = 0;
  constrInfo->refdConstrArray = NULL;
  
  constrInfo->checkConstrLen = 0;
  constrInfo->checkConstrText = NULL;

  tableDesc =
    Generator::createVirtualTableDesc
    ((char*)extTableName.data(),
     numCols,
     colInfo,
     numKeys,
     keyInfo,
     1, constrInfo,
     0, NULL);

  return tableDesc;
}

Lng32 CmpSeabaseDDL::getSeabaseColumnInfo(ExeCliInterface *cliInterface,
                                   Int64 objUID,
                                   char *direction,
                                   NABoolean *isTableSalted,
                                   Lng32 *numCols,
                                   ComTdbVirtTableColumnInfo **outColInfoArray)
{
  char query[3000];
  Lng32 cliRC;

  Queue * tableColInfo = NULL;
  str_sprintf(query, "select column_name, column_number, column_class, "
    "fs_data_type, column_size, column_precision, column_scale, "
    "datetime_start_field, datetime_end_field, trim(is_upshifted), column_flags, "
    "nullable, trim(character_set), default_class, default_value, "
    "trim(column_heading), hbase_col_family, hbase_col_qualifier, direction, "
    "is_optional  from %s.\"%s\".%s "
    "where object_uid = %Ld and direction in (%s)"
    "order by 2 for read committed access",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_COLUMNS,
              objUID,
              direction);
  cliRC = cliInterface->fetchAllRows(tableColInfo, query, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
  {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
  }

  *numCols = tableColInfo->numEntries();
  ComTdbVirtTableColumnInfo *colInfoArray = (ComTdbVirtTableColumnInfo*)
    new(STMTHEAP) char[*numCols * sizeof(ComTdbVirtTableColumnInfo)];
  NABoolean tableIsSalted = FALSE;
  tableColInfo->position();
  for (Lng32 idx = 0; idx < *numCols; idx++)
  {
      OutputInfo * oi = (OutputInfo*)tableColInfo->getNext(); 
      ComTdbVirtTableColumnInfo &colInfo = colInfoArray[idx];

      char * data = NULL;
      Lng32 len = 0;

      // get the column name
      oi->get(0, data, len);
      colInfo.colName = new(STMTHEAP) char[len + 1];
      strcpy((char*)colInfo.colName, data);
      
      colInfo.colNumber = *(Lng32*)oi->get(1);
  
      strcpy(colInfo.columnClass, (char*)oi->get(2));

      colInfo.datatype = *(Lng32*)oi->get(3);
      
      colInfo.length = *(Lng32*)oi->get(4);

      colInfo.precision = *(Lng32*)oi->get(5);

      colInfo.scale = *(Lng32*)oi->get(6);

      colInfo.dtStart = *(Lng32 *)oi->get(7);

      colInfo.dtEnd = *(Lng32 *)oi->get(8);

      if (strcmp((char*)oi->get(9), "Y") == 0)
	colInfo.upshifted = -1;
      else
	colInfo.upshifted = 0;

      colInfo.hbaseColFlags = *(ULng32 *)oi->get(10);

      colInfo.nullable = *(Lng32 *)oi->get(11);

      colInfo.charset = 
	(SQLCHARSET_CODE)CharInfo::getCharSetEnum((char*)oi->get(12));

      colInfo.defaultClass = (ComColumnDefaultClass)*(Lng32 *)oi->get(13);

      NAString tempDefVal;
      data = NULL;
      if (colInfo.defaultClass == COM_USER_DEFINED_DEFAULT ||
          colInfo.defaultClass == COM_ALWAYS_COMPUTE_COMPUTED_COLUMN_DEFAULT)
	{
	  oi->get(14, data, len);
          if (colInfo.defaultClass == COM_ALWAYS_COMPUTE_COMPUTED_COLUMN_DEFAULT)
            tableIsSalted = TRUE;
	}
      else if (colInfo.defaultClass == COM_NULL_DEFAULT)
	{
	  NAWString  nullConsW(WIDE_("NULL"));
	  tempDefVal = NAString(NAW_TO_NASTRING(nullConsW));
	}
      else if (colInfo.defaultClass == COM_USER_FUNCTION_DEFAULT)
	{
	  NAWString  userFuncW(WIDE_("USER"));
	  tempDefVal = NAString(NAW_TO_NASTRING(userFuncW));
	}
      else if (colInfo.defaultClass == COM_CURRENT_DEFAULT)
	{
	  NAWString  timeStmpW(WIDE_("CURRENT_TIMESTAMP"));
	  tempDefVal = NAString(NAW_TO_NASTRING(timeStmpW));
	}

      if (! tempDefVal.isNull())
	{
	  data = (char*)tempDefVal.data();
	  len = tempDefVal.length();
	}

      if (colInfo.defaultClass != COM_NO_DEFAULT)
	{
	  colInfo.defVal = new(STMTHEAP) char[len + 2];
	  str_cpy_all((char*)colInfo.defVal, data, len);
	  char * c = (char*)colInfo.defVal;
	  c[len] = 0;
	  c[len+1] = 0;
	}
      else
	colInfo.defVal = NULL;

      oi->get(15, data, len);
      if (len > 0)
	{
	  colInfo.colHeading = new(STMTHEAP) char[len + 1];
	  strcpy((char*)colInfo.colHeading, data);
	}
      else
	colInfo.colHeading = NULL;

      oi->get(16, data, len);
      colInfo.hbaseColFam = new(STMTHEAP) char[len + 1];
      strcpy((char*)colInfo.hbaseColFam, data);

      oi->get(17, data, len);
      colInfo.hbaseColQual = new(STMTHEAP) char[len + 1];
      strcpy((char*)colInfo.hbaseColQual, data);

      strcpy(colInfo.paramDirection, (char*)oi->get(18));
      if (*((char*)oi->get(19)) == 'Y')
         colInfo.isOptional = 1;
      else
         colInfo.isOptional = 0;
   }
   if (isTableSalted != NULL)
      *isTableSalted = tableIsSalted;
   *outColInfoArray = colInfoArray;
   return *numCols;
}

desc_struct * CmpSeabaseDDL::getSeabaseSequenceDesc(const NAString &catName, 
						    const NAString &schName, 
						    const NAString &seqName)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  desc_struct * tableDesc = NULL;

  NAString schNameL = "\"";
  schNameL += schName;
  schNameL += "\"";

  ComObjectName coName(catName, schNameL, seqName);
  NAString extSeqName = coName.getExternalName(TRUE);

  ExeCliInterface cliInterface(STMTHEAP);

  Int32 objectOwner =  0 ;
  Int64 seqUID = -1;
  seqUID = getObjectUIDandOwner(&cliInterface,
				catName.data(), schName.data(), seqName.data(),
				(char*)COM_SEQUENCE_GENERATOR_OBJECT_LIT, NULL, 
				&objectOwner, TRUE/*report error*/);
  if (seqUID == -1)
    return NULL;

 char buf[4000];

 str_sprintf(buf, "select fs_data_type, start_value, increment, max_value, min_value, cycle_option, cache_size, next_value, seq_type from %s.\"%s\".%s  where seq_uid = %Ld",
	     getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_SEQ_GEN,
	     seqUID);

  Queue * seqQueue = NULL;
  cliRC = cliInterface.fetchAllRows(seqQueue, buf, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return NULL;
    }
 
  if ((seqQueue->numEntries() == 0) ||
      (seqQueue->numEntries() > 1))
    {
      *CmpCommon::diags() << DgSqlCode(-4082)
			  << DgTableName(extSeqName);
      
      return NULL;
    }

  ComTdbVirtTableSequenceInfo *seqInfo =
     (ComTdbVirtTableSequenceInfo *)new (STMTHEAP) 
             ComTdbVirtTableSequenceInfo;

  seqQueue->position();
  OutputInfo * vi = (OutputInfo*)seqQueue->getNext(); 
 
  seqInfo->datatype = *(Lng32*)vi->get(0);
  seqInfo->startValue = *(Int64*)vi->get(1);
  seqInfo->increment = *(Int64*)vi->get(2);
  seqInfo->maxValue = *(Int64*)vi->get(3);
  seqInfo->minValue = *(Int64*)vi->get(4);
  seqInfo->cycleOption = (memcmp(vi->get(5), "Y", 1) == 0 ? 1 : 0);
  seqInfo->cache  = *(Int64*)vi->get(6);
  seqInfo->nextValue  = *(Int64*)vi->get(7);
  seqInfo->seqType = (memcmp(vi->get(8), "E", 1) == 0 ? COM_EXTERNAL_SG : COM_INTERNAL_SG);
  seqInfo->seqUID = seqUID;

  ComTdbVirtTableTableInfo tableInfo;
  tableInfo.tableName = extSeqName.data();
  tableInfo.createTime = 0;
  tableInfo.redefTime = 0;
  tableInfo.objUID = seqUID;
  tableInfo.isAudited = 0;
  tableInfo.validDef = 1;
  tableInfo.objOwner = objectOwner;
  tableInfo.hbaseCreateOptions = NULL;

  tableDesc =
    Generator::createVirtualTableDesc
    ((char*)extSeqName.data(),
     0, NULL, // colInfo
     0, NULL, // keyInfo
     0, NULL,
     0, NULL, //indexInfo
     0, NULL, // viewInfo
     &tableInfo,
     seqInfo);

  return tableDesc;
}

desc_struct * CmpSeabaseDDL::getSeabaseUserTableDesc(const NAString &catName, 
						     const NAString &schName, 
						     const NAString &objName,
						     const char * objType,
						     NABoolean includeInvalidDefs)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  char query[4000];
  ExeCliInterface cliInterface(STMTHEAP);

  desc_struct * tableDesc = NULL;

  Int32 objectOwner =  0 ;
  Int64 objUID      = -1 ;

  //
  // For performance reasons, whenever possible, we want to issue only one
  // "select" to the OBJECTS metadata table to determine both the existence
  // of the specified table and the objUID for the table.  Since it is more
  // likely that a user query refers to tables (directly or indirectly) that
  // are already in existence, this optimization can save the cost of the
  // existence check for all such user objects.  In the less likely case that
  // an object does not exist we must drop back and re-issue the metadata
  // query for the existence check in order to ensure we get the proper error
  // reported.
  //
  if ( objType ) // Must have objType
  {
    objUID = getObjectUIDandOwner(&cliInterface,
			      catName.data(), schName.data(), objName.data(),
				      objType, NULL, &objectOwner, FALSE /*no error now */);
  }
  // If we didn't call getObjectUIDandOwner() above OR if it gave an error, then:
  if ( objUID < 0 )
  {
    cliRC = existsInSeabaseMDTable(&cliInterface, 
				 catName.data(), schName.data(), objName.data(),
				 NULL,
				 (NAString(objType) != COM_INDEX_OBJECT_LIT ? TRUE : FALSE));
    if (cliRC < 0)
      {
        processReturn();

        return NULL;
      }

    if (cliRC == 0) // doesn't exist
      {
        processReturn();

        return NULL;
      }

    objUID = getObjectUIDandOwner(&cliInterface,
			      catName.data(), schName.data(), objName.data(),
				      objType, NULL, &objectOwner);
  }
  if (objUID < 0)
  {
     if ((!objType) || (objType && (strcmp(objType, "BT") != 0)))
     {
        processReturn();
	return NULL;
     }
     else
     {
       // object type passed in was for a table. Could not find it but.
       // this could be a view. Look for that.
       CmpCommon::diags()->clear();
       objUID = getObjectUIDandOwner(&cliInterface,
				     catName.data(), schName.data(), objName.data(), "VI", NULL, 
				     &objectOwner);
       if (objUID < 0)
	 {
          processReturn();
          return NULL;
       }
    }
  }

  str_sprintf(query, "select is_audited, hbase_create_options from %s.\"%s\".%s where table_uid = %Ld for read committed access",
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLES,
	      objUID);

  Queue * tableAttrQueue = NULL;
  cliRC = cliInterface.fetchAllRows(tableAttrQueue, query, 0, FALSE, FALSE, TRUE);

  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

      processReturn();

      return NULL;
    }

  NABoolean isAudited = TRUE;
  char * hbaseCreateOptions = NULL;
  if (cliRC == 0) // read some rows
    {
      if (tableAttrQueue->entries() != 1) // only one row should be returned
	{
	  processReturn();
	  return NULL;                     
	}
      
      tableAttrQueue->position();
      OutputInfo * vi = (OutputInfo*)tableAttrQueue->getNext();
      
      char * audit = vi->get(0);
      isAudited =  (memcmp(audit, "Y", 1) == 0);
      
      hbaseCreateOptions = vi->get(1);
    }

  Lng32 numCols;
  ComTdbVirtTableColumnInfo * colInfoArray;

  NABoolean tableIsSalted = FALSE;
  char direction[20];
  str_sprintf(direction, "'%s'", COM_UNKNOWN_PARAM_DIRECTION_LIT);

  if (getSeabaseColumnInfo(&cliInterface,
                          objUID,
                          (char *)direction,
                          &tableIsSalted,
                          &numCols,
                          &colInfoArray) <= 0)
  {
     processReturn();
     return NULL;                     
  } 

  Lng32 numSaltPartns = 0;
  if ((tableIsSalted) &&
      (hbaseCreateOptions))
    {
      // get num salt partns from hbaseCreateOptions.
      // It is stored as:  NUM_SALT_PARTNS=>NNNN
      char * saltStr = strstr(hbaseCreateOptions, "NUM_SALT_PARTNS=>");
      if (saltStr)
	{
	  char  numSaltPartnsCharStr[5];
	  char * startNumSaltPartns = saltStr + strlen("NUM_SALT_PARTNS=>");
	  memcpy(numSaltPartnsCharStr, startNumSaltPartns, 4);
	  numSaltPartnsCharStr[4] = 0;

	  numSaltPartns = str_atoi(numSaltPartnsCharStr, 4);
	}
    }

  if (strcmp(objType, COM_INDEX_OBJECT_LIT) == 0)
    {
      str_sprintf(query, "select k.column_name, c.column_number, k.keyseq_number, ordering, cast(0 as int not null)  from %s.\"%s\".%s k, %s.\"%s\".%s c where k.column_name = c.column_name and k.object_uid = c.object_uid and k.object_uid = %Ld and k.nonkeycol = 0 for read committed access order by keyseq_number",
		  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_KEYS,
		  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_COLUMNS,
		  objUID);
    }
  else
    {
      str_sprintf(query, "select column_name, column_number, keyseq_number, ordering, cast(0 as int not null)  from %s.\"%s\".%s where object_uid = %Ld and nonkeycol = 0 for read committed access order by keyseq_number",
		  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_KEYS,
		  objUID);
    }

  Queue * tableKeyInfo = NULL;
  cliRC = cliInterface.fetchAllRows(tableKeyInfo, query, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

      processReturn();

      return NULL;
    }

  ComTdbVirtTableKeyInfo * keyInfoArray = NULL;
  
  if (tableKeyInfo->numEntries() > 0)
    {
      keyInfoArray = (ComTdbVirtTableKeyInfo*)
	new(STMTHEAP) char[tableKeyInfo->numEntries() * sizeof(ComTdbVirtTableKeyInfo)];
    }

  tableKeyInfo->position();
  for (int idx = 0; idx < tableKeyInfo->numEntries(); idx++)
    {
      OutputInfo * vi = (OutputInfo*)tableKeyInfo->getNext(); 

      populateKeyInfo(keyInfoArray[idx], vi);
    }

  str_sprintf(query, "select O.catalog_name, O.schema_name, O.object_name, I.keytag, I.is_unique, I.is_explicit, I.key_colcount, I.nonkey_colcount from %s.\"%s\".%s I, %s.\"%s\".%s O where I.base_table_uid = %Ld and I.index_uid = O.object_uid %s for read committed access ",
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_INDEXES,
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
	      objUID,
	      (includeInvalidDefs ? " " : " and O.valid_def = 'Y' "));

  Queue * indexInfoQueue = NULL;
  cliRC = cliInterface.fetchAllRows(indexInfoQueue, query, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

      processReturn();

      return NULL;
    }

  ComTdbVirtTableIndexInfo * indexInfoArray = NULL;
  if (indexInfoQueue->numEntries() > 0)
    {
      indexInfoArray = (ComTdbVirtTableIndexInfo*)
	new(STMTHEAP) char[indexInfoQueue->numEntries() * sizeof(ComTdbVirtTableIndexInfo)];
    }

  NAString qCatName = "\"";
  qCatName += catName;
  qCatName += "\"";
  NAString qSchName = "\"";
  qSchName += schName;
  qSchName += "\"";
  NAString qObjName = "\"";
  qObjName += objName;
  qObjName += "\"";

  ComObjectName coName(qCatName, qSchName, qObjName);
  NAString * extTableName =
    new(STMTHEAP) NAString(coName.getExternalName(TRUE));

  indexInfoQueue->position();
  for (int idx = 0; idx < indexInfoQueue->numEntries(); idx++)
    {
      OutputInfo * vi = (OutputInfo*)indexInfoQueue->getNext(); 
      
      char * idxCatName = (char*)vi->get(0);
      char * idxSchName = (char*)vi->get(1);
      char * idxObjName = (char*)vi->get(2);
      Lng32 keyTag = *(Lng32*)vi->get(3);
      Lng32 isUnique = *(Lng32*)vi->get(4);
      Lng32 isExplicit = *(Lng32*)vi->get(5);
      Lng32 keyColCount = *(Lng32*)vi->get(6);
      Lng32 nonKeyColCount = *(Lng32*)vi->get(7);

      Int64 idxUID = getObjectUID(&cliInterface,
				  idxCatName, idxSchName, idxObjName,
				  COM_INDEX_OBJECT_LIT);
      if (idxUID < 0)
	{

	  processReturn();

	  return NULL;
	}

      indexInfoArray[idx].baseTableName = (char*)extTableName->data();

      NAString qIdxCatName = "\"";
      qIdxCatName += idxCatName;
      qIdxCatName += "\"";
      NAString qIdxSchName = "\"";
      qIdxSchName += idxSchName;
      qIdxSchName += "\"";
      NAString qIdxObjName = "\"";
      qIdxObjName += idxObjName;
      qIdxObjName += "\"";

      ComObjectName coIdxName(qIdxCatName, qIdxSchName, qIdxObjName);
      
      NAString * extIndexName = 
	new(STMTHEAP) NAString(coIdxName.getExternalName(TRUE));

      indexInfoArray[idx].indexName = (char*)extIndexName->data();
      indexInfoArray[idx].keytag = keyTag;
      indexInfoArray[idx].isUnique = isUnique;
      indexInfoArray[idx].isExplicit = isExplicit;
      indexInfoArray[idx].keyColCount = keyColCount;
      indexInfoArray[idx].nonKeyColCount = nonKeyColCount;

      Queue * keyInfoQueue = NULL;
      str_sprintf(query, "select column_name, column_number, keyseq_number, ordering, nonkeycol  from %s.\"%s\".%s where object_uid = %Ld for read committed access order by keyseq_number",
		  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_KEYS,
		  idxUID);
      cliRC = cliInterface.initializeInfoList(keyInfoQueue, TRUE);
      if (cliRC < 0)
	{
	  cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

	  processReturn();

	  return NULL;
	}
      
      cliRC = cliInterface.fetchAllRows(keyInfoQueue, query);
      if (cliRC < 0)
	{
	  cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

	  processReturn();

	  return NULL;
	}

      if (keyInfoQueue->numEntries() == 0)
	{
	  *CmpCommon::diags() << DgSqlCode(-4400);

	  processReturn();

	  return NULL;
	}

      ComTdbVirtTableKeyInfo * keyInfoArray = (ComTdbVirtTableKeyInfo*)
	new(STMTHEAP) char[keyColCount * sizeof(ComTdbVirtTableKeyInfo)];

      ComTdbVirtTableKeyInfo * nonKeyInfoArray = NULL;
      if (nonKeyColCount > 0)
	{
	  nonKeyInfoArray = (ComTdbVirtTableKeyInfo*)
	    new(STMTHEAP) char[nonKeyColCount * sizeof(ComTdbVirtTableKeyInfo)];
	}

      keyInfoQueue->position();
      Lng32 jk = 0;
      Lng32 jnk = 0;
      for (Lng32 j = 0; j < keyInfoQueue->numEntries(); j++)
	{
	  OutputInfo * vi = (OutputInfo*)keyInfoQueue->getNext(); 
	  
	  Lng32 nonKeyCol = *(Lng32*)vi->get(4);
	  if (nonKeyCol == 0)
	    {
	      populateKeyInfo(keyInfoArray[jk], vi, TRUE);
	      jk++;
	    }
	  else
	    {
	      if (nonKeyInfoArray)
		{
		  populateKeyInfo(nonKeyInfoArray[jnk], vi, TRUE);
		  jnk++;
		}
	    }
	}

      indexInfoArray[idx].keyInfoArray = keyInfoArray;

      indexInfoArray[idx].nonKeyInfoArray = nonKeyInfoArray;
    } // for

  // get constraint info
  str_sprintf(query, "select O.object_name, C.constraint_type, C.col_count, C.constraint_uid from %s.\"%s\".%s O, %s.\"%s\".%s C where O.catalog_name = '%s' and O.schema_name = '%s' and C.table_uid = %Ld and O.object_uid = C.constraint_uid ",
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLE_CONSTRAINTS,
	      catName.data(), schName.data(), 
	      objUID);
	      
  Queue * constrInfoQueue = NULL;
  cliRC = cliInterface.fetchAllRows(constrInfoQueue, query, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

      processReturn();

      return NULL;
    }

  ComTdbVirtTableConstraintInfo * constrInfoArray = NULL;
  if (constrInfoQueue->numEntries() > 0)
    {
      constrInfoArray = (ComTdbVirtTableConstraintInfo*)
	new(STMTHEAP) char[constrInfoQueue->numEntries() * sizeof(ComTdbVirtTableConstraintInfo)];
    }

  NAString tableCatName = "\"";
  tableCatName += catName;
  tableCatName += "\"";
  NAString tableSchName = "\"";
  tableSchName += schName;
  tableSchName += "\"";
  NAString tableObjName = "\"";
  tableObjName += objName;
  tableObjName += "\"";

  ComObjectName coTableName(tableCatName, tableSchName, tableObjName);
  extTableName =
    new(STMTHEAP) NAString(coTableName.getExternalName(TRUE));

  constrInfoQueue->position();
  for (int idx = 0; idx < constrInfoQueue->numEntries(); idx++)
    {
      OutputInfo * vi = (OutputInfo*)constrInfoQueue->getNext(); 
      
      char * constrName = (char*)vi->get(0);
      char * constrType = (char*)vi->get(1);
      Lng32 colCount = *(Lng32*)vi->get(2);
      Int64 constrUID = *(Int64*)vi->get(3);

      constrInfoArray[idx].baseTableName = (char*)extTableName->data();

      NAString cnNas = "\"";
      cnNas += constrName;
      cnNas += "\"";
      ComObjectName coConstrName(tableCatName, tableSchName, cnNas);
      NAString * extConstrName = 
	new(STMTHEAP) NAString(coConstrName.getExternalName(TRUE));

      constrInfoArray[idx].constrName = (char*)extConstrName->data();
      constrInfoArray[idx].colCount = colCount;

      if (strcmp(constrType, COM_UNIQUE_CONSTRAINT_LIT) == 0)
	constrInfoArray[idx].constrType = 0; // unique_constr
      else if (strcmp(constrType, COM_FOREIGN_KEY_CONSTRAINT_LIT) == 0)
	constrInfoArray[idx].constrType = 1; // ref_constr
     else if (strcmp(constrType, COM_CHECK_CONSTRAINT_LIT) == 0)
	constrInfoArray[idx].constrType = 2; // check_constr
     else if (strcmp(constrType, COM_PRIMARY_KEY_CONSTRAINT_LIT) == 0)
	constrInfoArray[idx].constrType = 3; // pkey_constr

      Queue * keyInfoQueue = NULL;
      str_sprintf(query, "select column_name, column_number, keyseq_number, ordering , cast(0 as int not null) from %s.\"%s\".%s where object_uid = %Ld for read committed access order by keyseq_number",
		  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_KEYS,
		  constrUID);
      cliRC = cliInterface.initializeInfoList(keyInfoQueue, TRUE);
      if (cliRC < 0)
	{
	  cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

	  processReturn();

	  return NULL;
	}
      
      cliRC = cliInterface.fetchAllRows(keyInfoQueue, query);
      if (cliRC < 0)
	{
	  cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

	  processReturn();

	  return NULL;
	}
      
      ComTdbVirtTableKeyInfo * keyInfoArray = NULL;
      if (colCount > 0)
	{
	  keyInfoArray = (ComTdbVirtTableKeyInfo*)
	    new(STMTHEAP) char[colCount * sizeof(ComTdbVirtTableKeyInfo)];
	  
	  keyInfoQueue->position();
	  Lng32 jk = 0;
	  for (Lng32 j = 0; j < keyInfoQueue->numEntries(); j++)
	    {
	      OutputInfo * vi = (OutputInfo*)keyInfoQueue->getNext(); 
	      
	      populateKeyInfo(keyInfoArray[jk], vi, TRUE);
	      jk++;
	    }
	}

      constrInfoArray[idx].keyInfoArray = keyInfoArray;
      constrInfoArray[idx].numRingConstr = 0;
      constrInfoArray[idx].ringConstrArray = NULL;
      constrInfoArray[idx].numRefdConstr = 0;
      constrInfoArray[idx].refdConstrArray = NULL;

      constrInfoArray[idx].checkConstrLen = 0;
      constrInfoArray[idx].checkConstrText = NULL;

      // attach all the referencing constraints
      if ((strcmp(constrType, COM_UNIQUE_CONSTRAINT_LIT) == 0) ||
	  (strcmp(constrType, COM_PRIMARY_KEY_CONSTRAINT_LIT) == 0))
	{
	  str_sprintf(query, "select trim(O.catalog_name || '.' || '\"' || O.schema_name || '\"' || '.' || '\"' || O.object_name || '\"' ) constr_name, trim(O2.catalog_name || '.' || '\"' || O2.schema_name || '\"' || '.' || '\"' || O2.object_name || '\"' ) table_name from %s.\"%s\".%s U, %s.\"%s\".%s O, %s.\"%s\".%s O2, %s.\"%s\".%s T where  O.object_uid = U.foreign_constraint_uid and O2.object_uid = T.table_uid and T.constraint_uid = U.foreign_constraint_uid and U.unique_constraint_uid = %Ld",
		      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_UNIQUE_REF_CONSTR_USAGE,
		      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
		      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
		      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLE_CONSTRAINTS,
		      constrUID
		      );

	  Queue * ringInfoQueue = NULL;
	  cliRC = cliInterface.fetchAllRows(ringInfoQueue, query, 0, FALSE, FALSE, TRUE);
	  if (cliRC < 0)
	    {
	      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
	      
	      processReturn();
	      
	      return NULL;
	    }

	  ComTdbVirtTableRefConstraints * ringInfoArray = NULL;
	  if (ringInfoQueue->numEntries() > 0)
	    {
	      ringInfoArray = (ComTdbVirtTableRefConstraints*)
		new(STMTHEAP) char[ringInfoQueue->numEntries() * sizeof(ComTdbVirtTableRefConstraints)];
	    }
	  
	  ringInfoQueue->position();
	  for (Lng32 i = 0; i < ringInfoQueue->numEntries(); i++)
	    {
	      OutputInfo * vi = (OutputInfo*)ringInfoQueue->getNext(); 
	      
	      ringInfoArray[i].constrName = (char*)vi->get(0);
	      ringInfoArray[i].baseTableName = (char*)vi->get(1);
	    }

	  constrInfoArray[idx].numRingConstr = ringInfoQueue->numEntries();
	  constrInfoArray[idx].ringConstrArray = ringInfoArray;
	}

      // attach all the referencing constraints
      if (strcmp(constrType, COM_FOREIGN_KEY_CONSTRAINT_LIT) == 0)
	{
	  str_sprintf(query, "select trim(O.catalog_name || '.' || '\"' || O.schema_name || '\"' || '.' || '\"' || O.object_name || '\"' ) constr_name, trim(O2.catalog_name || '.' || '\"' || O2.schema_name || '\"' || '.' || '\"' || O2.object_name || '\"' ) table_name from %s.\"%s\".%s R, %s.\"%s\".%s O, %s.\"%s\".%s O2, %s.\"%s\".%s T where  O.object_uid = R.unique_constraint_uid and O2.object_uid = T.table_uid and T.constraint_uid = R.unique_constraint_uid and R.ref_constraint_uid = %Ld",
		      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_REF_CONSTRAINTS,
		      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
		      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
		      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLE_CONSTRAINTS,
		      constrUID
		      );

	  Queue * refdInfoQueue = NULL;
	  cliRC = cliInterface.fetchAllRows(refdInfoQueue, query, 0, FALSE, FALSE, TRUE);
	  if (cliRC < 0)
	    {
	      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
	      
	      processReturn();
	      
	      return NULL;
	    }

	  ComTdbVirtTableRefConstraints * refdInfoArray = NULL;
	  if (refdInfoQueue->numEntries() > 0)
	    {
	      refdInfoArray = (ComTdbVirtTableRefConstraints*)
		new(STMTHEAP) char[refdInfoQueue->numEntries() * sizeof(ComTdbVirtTableRefConstraints)];
	    }
	  
	  refdInfoQueue->position();
	  for (Lng32 i = 0; i < refdInfoQueue->numEntries(); i++)
	    {
	      OutputInfo * vi = (OutputInfo*)refdInfoQueue->getNext(); 
	      
	      refdInfoArray[i].constrName = (char*)vi->get(0);
	      refdInfoArray[i].baseTableName = (char*)vi->get(1);
	    }

	  constrInfoArray[idx].numRefdConstr = refdInfoQueue->numEntries();
	  constrInfoArray[idx].refdConstrArray = refdInfoArray;

	}

     if (strcmp(constrType, COM_CHECK_CONSTRAINT_LIT) == 0)
       {
	 NAString constrText;
	 if (getTextFromMD(&cliInterface, constrUID, constrText))
	   {
	      processReturn();
	      
	      return NULL;
	   }

	 char * ct = new(STMTHEAP) char[constrText.length()+1];
	 memcpy(ct, constrText.data(), constrText.length());
	 ct[constrText.length()] = 0;

	 constrInfoArray[idx].checkConstrLen = constrText.length();
	 constrInfoArray[idx].checkConstrText = ct;
       }
    } // for

  str_sprintf(query, "select check_option, is_updatable, is_insertable from %s.\"%s\".%s where view_uid = %Ld for read committed access ",
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_VIEWS,
	      objUID);
  
  Queue * viewInfoQueue = NULL;
  cliRC = cliInterface.fetchAllRows(viewInfoQueue, query, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

      processReturn();

      return NULL;
    }

  ComTdbVirtTableViewInfo * viewInfoArray = NULL;
  if (viewInfoQueue->numEntries() > 0)
    {
      // must have only one entry
      if (viewInfoQueue->numEntries() > 1)
	{
	  processReturn();
	  
	  return NULL;
 	}

      viewInfoArray = (ComTdbVirtTableViewInfo*)
	new(STMTHEAP) char[sizeof(ComTdbVirtTableViewInfo)];

      viewInfoQueue->position();
      
      OutputInfo * vi = (OutputInfo*)viewInfoQueue->getNext(); 
      
      char * checkOption = (char*)vi->get(0);
      Lng32 isUpdatable = *(Lng32*)vi->get(1);
      Lng32 isInsertable = *(Lng32*)vi->get(2);
      
      viewInfoArray[0].viewName = (char*)extTableName->data();
      
       if (NAString(checkOption) != COM_NONE_CHECK_OPTION_LIT)
	{
	  viewInfoArray[0].viewCheckText = new(STMTHEAP) char[strlen(checkOption) + 1];
	  strcpy(viewInfoArray[0].viewCheckText, checkOption);
	}
      else
	viewInfoArray[0].viewCheckText = NULL;
      viewInfoArray[0].isUpdatable = isUpdatable;
      viewInfoArray[0].isInsertable = isInsertable;

      // get view text from TEXT table
      NAString viewText;
      if (getTextFromMD(&cliInterface, objUID, viewText))
	{
	  processReturn();
	  
	  return NULL;
	}

      viewInfoArray[0].viewText = new(STMTHEAP) char[viewText.length() + 1];
      strcpy(viewInfoArray[0].viewText, viewText.data());
    }

  ComTdbVirtTableTableInfo tableInfo;
  tableInfo.tableName = extTableName->data();
  tableInfo.createTime = 0;
  tableInfo.redefTime = 0;
  tableInfo.objUID = objUID;
  tableInfo.isAudited = (isAudited ? -1 : 0);
  tableInfo.validDef = 1;
  tableInfo.objOwner = objectOwner;
  tableInfo.numSaltPartns = numSaltPartns;
  tableInfo.hbaseCreateOptions = hbaseCreateOptions;

  tableDesc =
    Generator::createVirtualTableDesc
    (
     extTableName->data(), //objName,
     numCols,
     colInfoArray,
     tableKeyInfo->numEntries(), //keyIndex,
     keyInfoArray,
     constrInfoQueue->numEntries(),
     constrInfoArray,
     indexInfoQueue->numEntries(),
     indexInfoArray,
     viewInfoQueue->numEntries(),
     viewInfoArray,
     &tableInfo);

 // reset the SMD table flag
  tableDesc->body.table_desc.issystemtablecode = 0;

  if ( tableDesc ) {

       // request the default
      ExpHbaseInterface* ehi =CmpSeabaseDDL::allocEHI();
      ByteArrayList* bal = ehi->getRegionInfo(extTableName->data());

      // Set the header.nodetype to either HASH2 or RANGE based on whether
      // the table is salted or not.  
      if (tableIsSalted && CmpCommon::getDefault(HBASE_HASH2_PARTITIONING) == DF_ON) 
        ((table_desc_struct*)tableDesc)->hbase_regionkey_desc = 
          assembleRegionDescs(bal, DESC_HBASE_HASH2_REGION_TYPE);
      else
       if ( CmpCommon::getDefault(HBASE_RANGE_PARTITIONING) == DF_ON ) 
         ((table_desc_struct*)tableDesc)->hbase_regionkey_desc = 
            assembleRegionDescs(bal, DESC_HBASE_RANGE_REGION_TYPE);


      delete bal;
      CmpSeabaseDDL::deallocEHI(ehi);
  }


  if (! tableDesc)
    processReturn();
  
  return tableDesc;
}

desc_struct * CmpSeabaseDDL::getSeabaseTableDesc(const NAString &catName, 
						     const NAString &schName, 
						     const NAString &objName,
						     const char * objType,
						     NABoolean includeInvalidDefs)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  desc_struct *tDesc = NULL;
  if (isSeabaseMD(catName, schName, objName))
    {
      tDesc = getSeabaseMDTableDesc(catName, schName, objName, objType);
      
      // Could not find this metadata object in the static predefined structs.
      // It could be a metadata view or other objects created in MD schema.
      // Look for it as a regular object.
    }
  else if ((objName == HBASE_HIST_NAME) ||
	   (objName == HBASE_HISTINT_NAME))
    {
      NAString tabName = catName;
      tabName += ".";
      tabName += schName;
      tabName += ".";
      tabName += objName;
      if (existsInHbase(tabName))
	{
	  tDesc = getSeabaseHistTableDesc(catName, schName, objName);
	}
      return tDesc;
    }
  
  if (! tDesc)
    {
      if (CmpCommon::context()->isUninitializedSeabase())
	{
	  if (CmpCommon::context()->uninitializedSeabaseErrNum() == -1398)
	    *CmpCommon::diags() << DgSqlCode(CmpCommon::context()->uninitializedSeabaseErrNum())
				<< DgInt0(CmpCommon::context()->hbaseErrNum())
				<< DgString0(CmpCommon::context()->hbaseErrStr());
	  else
	    *CmpCommon::diags() << DgSqlCode(CmpCommon::context()->uninitializedSeabaseErrNum());
	}
      else
	{
          // use metadata compiler
          NABoolean switched = FALSE;
          CmpContext* currContext = CmpCommon::context();
          if (IdentifyMyself::GetMyName() == I_AM_EMBEDDED_SQL_COMPILER)
            if (SQL_EXEC_SWITCH_TO_COMPILER_TYPE(
                                     CmpContextInfo::CMPCONTEXT_TYPE_META))
              {
                // failed to switch/create metadata CmpContext
                // continue using current compiler
              }
             else
              switched = TRUE;

          if (sendAllControlsAndFlags(currContext))
            return NULL;

	  if ((objType) && (strcmp(objType, COM_SEQUENCE_GENERATOR_OBJECT_LIT) == 0))
	    tDesc = getSeabaseSequenceDesc(catName, schName, objName);
	  else
	    tDesc = getSeabaseUserTableDesc(catName, schName, objName, 
					    objType, includeInvalidDefs);

          // save existing diags info
          ComDiagsArea * tempDiags = ComDiagsArea::allocate(heap_);

          tempDiags->mergeAfter(*CmpCommon::diags());

          restoreAllControlsAndFlags();

          // ignore new (?) and restore old diags
          CmpCommon::diags()->clear();
          CmpCommon::diags()->mergeAfter(*tempDiags);
          tempDiags->clear();
          tempDiags->deAllocate();

          // switch back the internal commpiler, ignore error for now
          if (switched == TRUE)
            SQL_EXEC_SWITCH_BACK_COMPILER();

	}
    }

  return tDesc;
}

//
// Produce a list of desc_struct objects. In each object, the body_struct
// field points at hbase_region_desc. The order of the keyinfo, obtained from
// org.apache.hadoop.hbase.client.HTable.getEndKey(), is preserved.
//
// Allocate space from STMTHEAP, per the call of this function
// in CmpSeabaseDDL::getSeabaseTableDesc() and the
// Generator::createVirtualTableDesc() call make before this one that
// uses STMTPHEAP througout.
//
desc_struct* CmpSeabaseDDL::assembleRegionDescs(ByteArrayList* bal, desc_nodetype format)
{
   if ( !bal )
     return NULL;

   desc_struct *result = NULL;

   Int32 entries = bal->getSize();

   Int32 len = 0;
   char* buf = NULL;

   for (Int32 i=entries-1; i>=0; i-- ) {

      // call JNI interface
      len = bal->getEntrySize(i);
   
   
      if ( len > 0 ) {
   
         buf = new (STMTHEAP) char[len];
         Int32 datalen;
  
         if ( !bal->getEntry(i, buf, len, datalen) || datalen != len ) {
            return NULL;
         }
      }

      desc_struct* wrapper = NULL;
      wrapper = new (STMTHEAP) desc_struct();
      wrapper->header.OSV = 0; // TBD
      wrapper->header.OFV = 0; // TBD

      wrapper->header.nodetype = format;

      wrapper->body.hbase_region_desc.beginKey = NULL;
      wrapper->body.hbase_region_desc.beginKeyLen = 0;
      wrapper->body.hbase_region_desc.endKey = buf;
      wrapper->body.hbase_region_desc.endKeyLen = len;

      wrapper->header.next = result;
      result = wrapper;
   }

   return result;
}

// a wrapper method to getSeabaseRoutineDescInternal so 
// CmpContext context switching can take place. 
// getSeabaseRoutineDescInternal prepares and executes
// several queries on metadata tables
desc_struct *CmpSeabaseDDL::getSeabaseRoutineDesc(const NAString &catName,
                                      const NAString &schName,
                                      const NAString &objName)
{
   desc_struct *result = NULL;
   NABoolean switched = FALSE;

   // use metadata compiler
   if (IdentifyMyself::GetMyName() == I_AM_EMBEDDED_SQL_COMPILER)
     if (SQL_EXEC_SWITCH_TO_COMPILER_TYPE(
                              CmpContextInfo::CMPCONTEXT_TYPE_META))
       {
          // failed to switch/create metadata CmpContext
          // continue using current compiler 
       }
      else
       switched = TRUE;

   result = getSeabaseRoutineDescInternal(catName, schName, objName);

   // switch back the internal commpiler, ignore error for now
   if (switched == TRUE)
     SQL_EXEC_SWITCH_BACK_COMPILER();

   return result;
}


desc_struct *CmpSeabaseDDL::getSeabaseRoutineDescInternal(const NAString &catName,
                                      const NAString &schName,
                                      const NAString &objName)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  desc_struct *result;
  char query[4000];
  char buf[4000];

  ExeCliInterface cliInterface(STMTHEAP);

  Int64 objectUid = getObjectUID(&cliInterface, 
                                 catName.data(), schName.data(),
                                 objName.data(),
                                 (char *)COM_UDR_NAME_LIT);
  if (objectUid == -1)
     return NULL;

  str_sprintf(buf, "select udr_type, language_type, deterministic_bool,"
  " sql_access, call_on_null, isolate_bool, param_style,"
  " transaction_attributes, max_results, state_area_size, external_name,"
  " parallelism, user_version, external_security, execution_mode,"
  " library_filename, version, signature from %s.\"%s\".%s a, %s.\"%s\".%s b"
  " where a.udr_uid = %Ld and a.library_uid = b.library_uid "
  " for read committed access",
       getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_ROUTINES,
       getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_LIBRARIES, objectUid);


  cliRC = cliInterface.fetchRowsPrologue(buf, TRUE/*no exec*/);
  if (cliRC < 0)
  {
     cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
     return NULL;
  }

  cliRC = cliInterface.clearExecFetchClose(NULL, 0);
  if (cliRC < 0)
  {
    cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
     return NULL;
  }
  if (cliRC == 100) // did not find the row
  {
     *CmpCommon::diags() << DgSqlCode(-1389)
                        << DgString0(objName);
     return NULL;
  }

  char * ptr = NULL;
  Lng32 len = 0;

  ComTdbVirtTableRoutineInfo *routineInfo =
     (ComTdbVirtTableRoutineInfo *)new (STMTHEAP) 
             ComTdbVirtTableRoutineInfo;

  routineInfo->routine_name = objName.data();
  cliInterface.getPtrAndLen(1, ptr, len);
  str_cpy_all(routineInfo->UDR_type, ptr, len);
  routineInfo->UDR_type[len] =  '\0';
  cliInterface.getPtrAndLen(2, ptr, len);
  str_cpy_all(routineInfo->language_type, ptr, len);
  routineInfo->language_type[len] = '\0';
  cliInterface.getPtrAndLen(3, ptr, len);
  if (*ptr == 'Y')
     routineInfo->deterministic = 1;
  else
     routineInfo->deterministic = 0;
  cliInterface.getPtrAndLen(4, ptr, len);
  str_cpy_all(routineInfo->sql_access, ptr, len);
  routineInfo->sql_access[len] = '\0';
  cliInterface.getPtrAndLen(5, ptr, len);
  if (*ptr == 'Y')
     routineInfo->call_on_null = 1;
  else
     routineInfo->call_on_null = 0;
  cliInterface.getPtrAndLen(6, ptr, len);
  if (*ptr == 'Y')
     routineInfo->isolate = 1;
  else
     routineInfo->isolate = 0;
  cliInterface.getPtrAndLen(7, ptr, len);
  str_cpy_all(routineInfo->param_style, ptr, len);
  routineInfo->param_style[len] = '\0';
  cliInterface.getPtrAndLen(8, ptr, len);
  str_cpy_all(routineInfo->transaction_attributes, ptr, len);
  routineInfo->transaction_attributes[len] = '\0';
  cliInterface.getPtrAndLen(9, ptr, len);
  routineInfo->max_results = *(Int32 *)ptr;
  cliInterface.getPtrAndLen(10, ptr, len);
  routineInfo->state_area_size = *(Int32 *)ptr;
  cliInterface.getPtrAndLen(11, ptr, len);
  routineInfo->external_name = new (STMTHEAP) char[len+1];    
  str_cpy_and_null((char *)routineInfo->external_name, ptr, len, '\0', ' ', TRUE);
  cliInterface.getPtrAndLen(12, ptr, len);
  str_cpy_all(routineInfo->parallelism, ptr, len);
  routineInfo->parallelism[len] = '\0';
  cliInterface.getPtrAndLen(13, ptr, len);
  str_cpy_all(routineInfo->user_version, ptr, len);
  routineInfo->user_version[len] = '\0';
  cliInterface.getPtrAndLen(14, ptr, len);
  str_cpy_all(routineInfo->external_security, ptr, len);
  routineInfo->external_security[len] = '\0';
  cliInterface.getPtrAndLen(15, ptr, len);
  str_cpy_all(routineInfo->execution_mode, ptr, len);
  routineInfo->execution_mode[len] = '\0';
  cliInterface.getPtrAndLen(16, ptr, len);
  routineInfo->library_filename = new (STMTHEAP) char[len+1];    
  str_cpy_and_null((char *)routineInfo->library_filename, ptr, len, '\0', ' ', TRUE);
  cliInterface.getPtrAndLen(17, ptr, len);
  routineInfo->library_version = *(Int32 *)ptr;
  cliInterface.getPtrAndLen(18, ptr, len);
  routineInfo->signature = new (STMTHEAP) char[len+1];    
  str_cpy_and_null((char *)routineInfo->signature, ptr, len, '\0', ' ', TRUE);
  ComTdbVirtTableColumnInfo *paramsArray;
  Lng32 numParams;
  char direction[50];
  str_sprintf(direction, "'%s', '%s', '%s'", 
                      COM_INPUT_PARAM_LIT, COM_OUTPUT_PARAM_LIT,
                      COM_INOUT_PARAM_LIT);
  // Params
  if (getSeabaseColumnInfo(&cliInterface,
                          objectUid,
                          (char *)direction,
                          NULL,
                          &numParams,
                          &paramsArray) < 0)
  {
     processReturn();
     return NULL;
  } 
  
  desc_struct *routine_desc = NULL;
  routine_desc = Generator::createVirtualRoutineDesc(
            objName.data(),
            routineInfo,
            numParams,
            paramsArray);

  if (routine_desc == NULL)
     processReturn();
  return routine_desc;
}

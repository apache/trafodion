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
 * File:         CmpSeabaseDDLcommon.cpp
 * Description:  Implements common methods and operations for SQL/hbase tables.
 *
 *
 * Created:     6/30/2013
 * Language:     C++
 *
 *
 *****************************************************************************
 */

#include "CmpSeabaseDDLincludes.h"
#include "RelExeUtil.h"
#include "ControlDB.h"
#include "NumericType.h"
#include "CmpDDLCatErrorCodes.h"
#include "ValueDesc.h"
#include "Globals.h"
#include "CmpSeabaseDDLauth.h"
#include "NAUserId.h"
#include "StmtDDLCreateView.h"
#include "StmtDDLAlterTableDisableIndex.h"
#include "StmtDDLAlterTableEnableIndex.h"
#include "StmtDDLCreateComponentPrivilege.h"
#include "StmtDDLDropComponentPrivilege.h"
#include "StmtDDLGrantComponentPrivilege.h"
#include "StmtDDLRevokeComponentPrivilege.h"
#include "StmtDDLRegisterComponent.h"
#include "StmtDDLCreateRole.h"
#include "StmtDDLRoleGrant.h"
#include "PrivMgrCommands.h"
#include "PrivMgrComponentPrivileges.h"
#include "ComUser.h"

static __thread MDDescsInfo * trafMDDescsInfo_ = NULL;

static void createSeabaseComponentOperation(
   const std::string & systemCatalog,
   StmtDDLCreateComponentPrivilege *pParseNode);
   
static void dropSeabaseComponentOperation(
   const std::string & systemCatalog,
   StmtDDLDropComponentPrivilege *pParseNode);
   
static void grantSeabaseComponentPrivilege(
   const std::string & systemCatalog,
   StmtDDLGrantComponentPrivilege *pParseNode);
   
static void revokeSeabaseComponentPrivilege(
   const std::string & systemCatalog,
   StmtDDLRevokeComponentPrivilege *pParseNode);
   
static void grantRevokeSeabaseRole(
   const std::string & systemCatalog,
   StmtDDLRoleGrant *pParseNode);   
   
static bool hasValue(
   std::vector<int32_t> container,
   int32_t value);   

#include "EncodedKeyValue.h"
#include "SCMVersHelp.h"

// get software major and minor versions from -D defs defined in sqlcomp/Makefile.
// These defs pick up values from export vars defined in sqf/sqenvcom.sh.
#define SOFTWARE_MAJOR_VERSION TRAF_SOFTWARE_VERS_MAJOR
#define SOFTWARE_MINOR_VERSION TRAF_SOFTWARE_VERS_MINOR
#define SOFTWARE_UPDATE_VERSION TRAF_SOFTWARE_VERS_UPDATE
#define HBASE_OPTIONS_MAX_LENGTH 6000

CmpSeabaseDDL::CmpSeabaseDDL(NAHeap *heap, NABoolean syscatInit)
{
  savedCmpParserFlags_ = 0;
  savedCliParserFlags_ = 0;
  authorizationEnabled_ = -1;
  heap_ = heap;

  if ((syscatInit) && (ActiveSchemaDB()))
    {
      const char* sysCat = ActiveSchemaDB()->getDefaults().getValue(SEABASE_CATALOG);
      
      seabaseSysCat_ = sysCat;
    }
}

NABoolean CmpSeabaseDDL::getMDtableInfo(const NAString &objName,
                                        Lng32 &colInfoSize,
                                        const ComTdbVirtTableColumnInfo* &colInfo,
                                        Lng32 &keyInfoSize,
                                        const ComTdbVirtTableKeyInfo * &keyInfo,
                                        Lng32 &indexInfoSize,
                                        const ComTdbVirtTableIndexInfo* &indexInfo,
                                        const char * objType)
{
  indexInfoSize = 0;
  indexInfo = NULL;

  for (Int32 i = 0; i < sizeof(allMDtablesInfo)/sizeof(MDTableInfo); i++)
    {
      const MDTableInfo &mdti = allMDtablesInfo[i];
      
      MDDescsInfo &mddi = CmpCommon::context()->getTrafMDDescsInfo()[i];

      if (mdti.newName && (objName == mdti.newName))
        {
          if (strcmp(objType, COM_BASE_TABLE_OBJECT_LIT) == 0)
            {
              colInfoSize = mddi.numNewCols;
              colInfo = mddi.newColInfo;
              keyInfoSize = mddi.numNewKeys;
              keyInfo = mddi.newKeyInfo;

              indexInfoSize = mddi.numIndexes;
              indexInfo = mddi.indexInfo;

              // this is an index. It cannot selected as a base table objects.
              if (mdti.isIndex)
                return FALSE;
            }
          else if (strcmp(objType, COM_INDEX_OBJECT_LIT) == 0)
            {
              colInfoSize = mddi.numNewCols;
              colInfo = mddi.newColInfo;
              keyInfoSize = mddi.numNewKeys;
              keyInfo = mddi.newKeyInfo;

            }
          else
            return FALSE;

          return TRUE;
        }
      else  if (mdti.oldName && (objName == mdti.oldName))
        {
          if ((mddi.numOldCols > 0) && (mddi.oldColInfo))
            {
              colInfoSize = mddi.numOldCols;
              colInfo = mddi.oldColInfo;
            }
          else
            {
              colInfoSize = mddi.numNewCols;
              colInfo = mddi.newColInfo;
            }

          if ((mddi.numOldKeys > 0) && (mddi.oldKeyInfo))
            {
              keyInfoSize = mddi.numOldKeys;
              keyInfo = mddi.oldKeyInfo;
            }
          else
            {
              keyInfoSize = mddi.numNewKeys;
              keyInfo = mddi.newKeyInfo;
            }

          return TRUE;
        }
        
    } // for

  return FALSE;
}

short CmpSeabaseDDL::convertColAndKeyInfoArrays(
                                                Lng32 btNumCols, // IN
                                                ComTdbVirtTableColumnInfo* btColInfoArray, // IN
                                                Lng32 btNumKeys, // IN
                                                ComTdbVirtTableKeyInfo* btKeyInfoArray, // IN
                                                NAColumnArray *naColArray,
                                                NAColumnArray *naKeyArr)
{
  for (Lng32 i = 0; i < btNumCols; i++)
    {
      ComTdbVirtTableColumnInfo &ci = btColInfoArray[i];

      columns_desc_struct column_desc;
      column_desc.datatype = ci.datatype;
      column_desc.length = ci.length;
      column_desc.precision = ci.precision;
      column_desc.scale = ci.scale;
      column_desc.null_flag = ci.nullable;
      column_desc.character_set/*CharInfo::CharSet*/ 
        = (CharInfo::CharSet)ci.charset;
      column_desc.upshift = ci.upshifted;
      column_desc.caseinsensitive = 0;
      column_desc.collation_sequence = CharInfo::DefaultCollation;
      column_desc.encoding_charset = (CharInfo::CharSet)ci.charset;

      column_desc.datetimestart = (rec_datetime_field)ci.dtStart;
      column_desc.datetimeend = (rec_datetime_field)ci.dtEnd;
      column_desc.datetimefractprec = ci.scale;

      column_desc.defaultClass = ci.defaultClass;

      NAType *type;
      NAColumn::createNAType(&column_desc, NULL, type, STMTHEAP);

      NAColumn * nac = new(STMTHEAP) NAColumn(ci.colName, i, type, STMTHEAP);
      naColArray->insert(nac);

      for (Lng32 ii = 0; ii < btNumKeys; ii++)
        {
          ComTdbVirtTableKeyInfo &ki = btKeyInfoArray[ii];
          if (strcmp(ci.colName, ki.colName) == 0)
            {
              naKeyArr->insert(nac);
            }
        } // for

    } // for

  return 0;
}

static short resetCQD(NABoolean cqdWasSet, short retval)
{
  if (cqdWasSet)
    {
      NAString value("ON");
      ActiveSchemaDB()->getDefaults().validateAndInsert(
                                                        "hbase_serialization", value, FALSE);
    }

  return retval;
}

short CmpSeabaseDDL::processDDLandCreateDescs(
                                              Parser &parser,
                                              const QString *ddl,
                                              Lng32 sizeOfddl,

                                              NABoolean isIndexTable,

                                              Lng32 btNumCols, // IN
                                              ComTdbVirtTableColumnInfo* btColInfoArray, // IN
                                              Lng32 btNumKeys, // IN
                                              ComTdbVirtTableKeyInfo* btKeyInfoArray, // IN

                                              Lng32 &numCols, // OUT
                                              ComTdbVirtTableColumnInfo* &colInfoArray, // OUT
                                              Lng32 &numKeys, // OUT
                                              ComTdbVirtTableKeyInfo* &keyInfoArray, // OUT

                                              ComTdbVirtTableIndexInfo* &indexInfo) // OUT
{
  numCols = 0;
  numKeys = 0;
  colInfoArray = NULL;
  keyInfoArray = NULL;

  indexInfo = NULL;

  ExprNode * exprNode = NULL;
  const QString * qs = NULL;
  Int32 sizeOfqs = 0;
  
  qs = ddl;
  sizeOfqs = sizeOfddl; 
  
  Int32 qryArraySize = sizeOfqs / sizeof(QString);
  char * gluedQuery;
  Lng32 gluedQuerySize;
  glueQueryFragments(qryArraySize,  qs,
                     gluedQuery, gluedQuerySize);

  NABoolean cqdWasSet = FALSE;
  if (CmpCommon::getDefault(HBASE_SERIALIZATION) == DF_ON)
    {
      NAString value("OFF");
      ActiveSchemaDB()->getDefaults().validateAndInsert(
                                                        "hbase_serialization", value, FALSE);
      cqdWasSet = TRUE;
    }

  exprNode = parser.parseDML((const char*)gluedQuery, strlen(gluedQuery), 
                             CharInfo::ISO88591);
  if (! exprNode)
    return resetCQD(cqdWasSet, -1);
  
  RelExpr * rRoot = NULL;
  if (exprNode->getOperatorType() EQU STM_QUERY)
    {
      rRoot = (RelRoot*)exprNode->getChild(0);
    }
  else if (exprNode->getOperatorType() EQU REL_ROOT)
    {
      rRoot = (RelRoot*)exprNode;
    }
  
  if (! rRoot)
    return resetCQD(cqdWasSet, -1);
  
  ExprNode * ddlNode = NULL;
  DDLExpr * ddlExpr = NULL;
  
  ddlExpr = (DDLExpr*)rRoot->getChild(0);
  ddlNode = ddlExpr->getDDLNode();
  if (! ddlNode)
    return resetCQD(cqdWasSet, -1);
  
  if (ddlNode->getOperatorType() == DDL_CREATE_TABLE)
    {
      StmtDDLCreateTable * createTableNode =
        ddlNode->castToStmtDDLNode()->castToStmtDDLCreateTable();
      
      ElemDDLColDefArray &colArray = createTableNode->getColDefArray();
      ElemDDLColRefArray &keyArray = createTableNode->getPrimaryKeyColRefArray();
      
      numCols = colArray.entries();
      numKeys = keyArray.entries();
      
      colInfoArray = (ComTdbVirtTableColumnInfo*)
        new(CTXTHEAP) char[numCols * sizeof(ComTdbVirtTableColumnInfo)];
      
      keyInfoArray = (ComTdbVirtTableKeyInfo*)
        new(CTXTHEAP) char[numKeys * sizeof(ComTdbVirtTableKeyInfo)];
      
      if (buildColInfoArray(&colArray, colInfoArray, FALSE, 0, CTXTHEAP))
        {
          return resetCQD(cqdWasSet, -1);
        }

      if (buildKeyInfoArray(&colArray, &keyArray, colInfoArray, keyInfoArray, FALSE,
                            CTXTHEAP))
        {
          return resetCQD(cqdWasSet, -1);
        }

      // if index table defn, append "@" to the hbase col qual.
      if (isIndexTable)
        {
          for (Lng32 i = 0; i < numCols; i++)
            {
              ComTdbVirtTableColumnInfo &ci = colInfoArray[i];

              char hcq[100];
              strcpy(hcq, ci.hbaseColQual);
              
              ci.hbaseColQual = new(CTXTHEAP) char[strlen(hcq) + 1 +1];
              strcpy((char*)ci.hbaseColQual, (char*)"@");
              strcat((char*)ci.hbaseColQual, hcq);
            } // for

          for (Lng32 i = 0; i < numKeys; i++)
            {
              ComTdbVirtTableKeyInfo &ci = keyInfoArray[i];

              ci.hbaseColQual = new(CTXTHEAP) char[10];
              str_sprintf((char*)ci.hbaseColQual, "@%d", ci.keySeqNum);
            }
        } // if
    }
  else if (ddlNode->getOperatorType() == DDL_CREATE_INDEX)
    {
      StmtDDLCreateIndex * createIndexNode =
        ddlNode->castToStmtDDLNode()->castToStmtDDLCreateIndex();
      
      ComObjectName tableName(createIndexNode->getTableName());
      NAString extTableName = tableName.getExternalName(TRUE);
      
      //      ComObjectName indexName(createIndexNode->getIndexName());
      NAString extIndexName = TRAFODION_SYSCAT_LIT;
      extIndexName += ".";
      extIndexName += "\"";
      extIndexName += SEABASE_MD_SCHEMA;
      extIndexName += "\"";
      extIndexName += ".";
      extIndexName += createIndexNode->getIndexName();
      
      ElemDDLColRefArray & indexColRefArray = createIndexNode->getColRefArray();
      
      NAColumnArray btNAColArray;
      NAColumnArray btNAKeyArr;

      if (convertColAndKeyInfoArrays(btNumCols, btColInfoArray,
                                     btNumKeys, btKeyInfoArray,
                                     &btNAColArray, &btNAKeyArr))
        return resetCQD(cqdWasSet, -1);

      Lng32 keyColCount = 0;
      Lng32 nonKeyColCount = 0;
      Lng32 totalColCount = 0;
      
      Lng32 numIndexCols = 0;
      Lng32 numIndexKeys = 0;
      Lng32 numIndexNonKeys = 0;

      ComTdbVirtTableColumnInfo * indexColInfoArray = NULL;
      ComTdbVirtTableKeyInfo * indexKeyInfoArray = NULL;
      ComTdbVirtTableKeyInfo * indexNonKeyInfoArray = NULL;
      
      NAList<NAString> selColList;
      Lng32 keyLength = 0;
      
      if (createIndexColAndKeyInfoArrays(indexColRefArray,
                                         createIndexNode->isUniqueSpecified(),
                                         FALSE, // no syskey
                                         btNAColArray, btNAKeyArr,
                                         numIndexKeys, numIndexNonKeys, numIndexCols,
                                         indexColInfoArray, indexKeyInfoArray,
                                         selColList,
                                         keyLength,
                                         CTXTHEAP))
        return resetCQD(cqdWasSet, -1);

      numIndexNonKeys = numIndexCols - numIndexKeys;
      
      if (numIndexNonKeys > 0)
        indexNonKeyInfoArray = (ComTdbVirtTableKeyInfo*)
          new(CTXTHEAP) char[numIndexNonKeys *  sizeof(ComTdbVirtTableKeyInfo)];
      
      Lng32 ink = 0;
      for (Lng32 i = numIndexKeys; i < numIndexCols; i++)
        {
          ComTdbVirtTableColumnInfo &indexCol = indexColInfoArray[i];
          
          ComTdbVirtTableKeyInfo &ki = indexNonKeyInfoArray[ink];
          ki.colName = indexCol.colName;

          NAColumn * nc = btNAColArray.getColumn(ki.colName);
          Lng32 colNumber = nc->getPosition();

          ki.tableColNum = colNumber;
          ki.keySeqNum = i+1;
          ki.ordering = 0;
          ki.nonKeyCol = 1;
          
          ki.hbaseColFam = new(CTXTHEAP) char[strlen(SEABASE_DEFAULT_COL_FAMILY) + 1];
          strcpy((char*)ki.hbaseColFam, SEABASE_DEFAULT_COL_FAMILY);
          
          char qualNumStr[40];
          str_sprintf(qualNumStr, "@%d", ki.keySeqNum);
          
          ki.hbaseColQual = new(CTXTHEAP) char[strlen(qualNumStr)+1];
          strcpy((char*)ki.hbaseColQual, qualNumStr);

          ink++;
        } // for
      
      indexInfo = (ComTdbVirtTableIndexInfo*)
        new(CTXTHEAP) char[1 * sizeof(ComTdbVirtTableIndexInfo)];
      indexInfo->baseTableName = new(CTXTHEAP) char[extTableName.length()+ 1];
      strcpy((char*)indexInfo->baseTableName, extTableName.data());

      indexInfo->indexName = new(CTXTHEAP) char[extIndexName.length()+ 1];
      strcpy((char*)indexInfo->indexName, extIndexName.data());

      indexInfo->keytag = 1;
      indexInfo->isUnique =  createIndexNode->isUniqueSpecified() ? 1 : 0;
      indexInfo->isExplicit = 1;

      indexInfo->keyColCount = numIndexKeys;
      indexInfo->nonKeyColCount = numIndexNonKeys;
      indexInfo->keyInfoArray = indexKeyInfoArray;
      indexInfo->nonKeyInfoArray = indexNonKeyInfoArray;
      
      numCols = 0;
      colInfoArray = NULL;
      numKeys = 0;
      keyInfoArray = NULL;
    }
  else
    return resetCQD(cqdWasSet, -1);

  return resetCQD(cqdWasSet, 0);
}

// RETURN: -1, error.  0, all ok.
short CmpSeabaseDDL::createMDdescs(MDDescsInfo *&trafMDDescsInfo)
{
  if (trafMDDescsInfo)
    return 0;

  Lng32 numTables = sizeof(allMDtablesInfo) / sizeof(MDTableInfo);
  trafMDDescsInfo = (MDDescsInfo*) 
    new(CTXTHEAP) char[numTables * sizeof(MDDescsInfo)];
  Parser parser(CmpCommon::context());
  for (Lng32 i = 0; i < numTables; i++)
    {
      const MDTableInfo &mdti = allMDtablesInfo[i];
      MDDescsInfo &mddi = trafMDDescsInfo[i];
      
      if (!mdti.newDDL)
        continue;
      
      Lng32 numCols = 0;
      Lng32 numKeys = 0;
      
      ComTdbVirtTableColumnInfo * colInfoArray = NULL;
      ComTdbVirtTableKeyInfo * keyInfoArray = NULL;
      ComTdbVirtTableIndexInfo * indexInfo = NULL;

      if (processDDLandCreateDescs(parser,
                                   mdti.newDDL, mdti.sizeOfnewDDL,
                                   (mdti.isIndex ? TRUE : FALSE),
                                   0, NULL, 0, NULL,
                                   numCols, colInfoArray,
                                   numKeys, keyInfoArray,
                                   indexInfo))
        return -1;

      mddi.numNewCols = numCols;
      mddi.newColInfo = colInfoArray;
      
      mddi.numNewKeys = numKeys;
      mddi.newKeyInfo = keyInfoArray;
      
      if (mdti.oldDDL)
        {
          if (processDDLandCreateDescs(parser,
                                       mdti.oldDDL, mdti.sizeOfoldDDL,
                                       (mdti.isIndex ? TRUE : FALSE),
                                       0, NULL, 0, NULL,
                                       numCols, colInfoArray,
                                       numKeys, keyInfoArray,
                                       indexInfo))
            return -1;
        }
      
      mddi.numOldCols = numCols;
      mddi.oldColInfo = colInfoArray;
      
      mddi.numOldKeys = numKeys;
      mddi.oldKeyInfo = keyInfoArray;

      mddi.numIndexes = 0;
      mddi.indexInfo = NULL;

      if (mdti.indexDDL)
        {
          mddi.numIndexes = 1;
          mddi.indexInfo = NULL;

          ComTdbVirtTableIndexInfo * indexInfo = NULL;
          Lng32 numIndexCols = 0;
          Lng32 numIndexKeys = 0;
          ComTdbVirtTableColumnInfo * indexColInfoArray = NULL;
          ComTdbVirtTableKeyInfo * indexKeyInfoArray = NULL;
          
          if (processDDLandCreateDescs(parser,
                                       mdti.indexDDL, mdti.sizeOfIndexDDL,
                                       FALSE,
                                       numCols, colInfoArray,
                                       numKeys, keyInfoArray,
                                       numIndexCols, indexColInfoArray,
                                       numIndexKeys, indexKeyInfoArray,
                                       indexInfo))
            return -1;

          mddi.indexInfo = indexInfo;
        }
    } // for

  return 0;
}
                                              
NABoolean CmpSeabaseDDL::isHbase(const NAString &catName)
{
  if ((CmpCommon::getDefault(MODE_SEABASE) == DF_ON) &&
      (NOT catName.isNull()))
    {
      NAString hbaseDefCatName = "";
      CmpCommon::getDefault(HBASE_CATALOG, hbaseDefCatName, FALSE);
      hbaseDefCatName.toUpper();
      
       if ((catName == HBASE_SYSTEM_CATALOG) ||
           (catName == hbaseDefCatName))
         return TRUE;
    }
  
  return FALSE;
}

ComBoolean CmpSeabaseDDL::isHbase(const ComObjectName &name) 
{
  return isHbase(name.getCatalogNamePartAsAnsiString());
}

NABoolean CmpSeabaseDDL::isSeabase(const NAString &catName)
{
  if ((CmpCommon::getDefault(MODE_SEABASE) == DF_ON) &&
      (NOT catName.isNull()))
    {
      NAString seabaseDefCatName = "";
      CmpCommon::getDefault(SEABASE_CATALOG, seabaseDefCatName, FALSE);
      seabaseDefCatName.toUpper();
      
      if (catName == seabaseDefCatName)
         return TRUE;
    }
  
  return FALSE;
}

ComBoolean CmpSeabaseDDL::isSeabase(const ComObjectName &name) 
{
  return isSeabase(name.getCatalogNamePartAsAnsiString());
}

NABoolean CmpSeabaseDDL::isSeabaseMD(const NAString &catName,
                     const NAString &schName,
                     const NAString &objName)
{
  if ((CmpCommon::getDefault(MODE_SEABASE) == DF_ON) &&
      (NOT catName.isNull()))
    {
      NAString seabaseDefCatName = "";
      CmpCommon::getDefault(SEABASE_CATALOG, seabaseDefCatName, FALSE);
      seabaseDefCatName.toUpper();
      
      if ((catName == seabaseDefCatName) &&
          (schName == SEABASE_MD_SCHEMA))
        {
          return TRUE;
        }
    }

  return FALSE;
}

NABoolean CmpSeabaseDDL::isSeabaseReservedSchema(
                                                 const ComObjectName &name)
{
  const NAString &catName = name.getCatalogNamePartAsAnsiString(TRUE);
  const NAString &schName = name.getSchemaNamePartAsAnsiString(TRUE);

  if (NOT catName.isNull())
    {
      NAString seabaseDefCatName = "";
      CmpCommon::getDefault(SEABASE_CATALOG, seabaseDefCatName, FALSE);
      seabaseDefCatName.toUpper();
      
      if ((catName == seabaseDefCatName) &&
          ((schName == SEABASE_MD_SCHEMA) ||
           (schName == SEABASE_DTM_SCHEMA) ||
           (schName == SEABASE_REPOS_SCHEMA)))
        return TRUE;
    }

  return FALSE;
}

// ----------------------------------------------------------------------------
// Method:  isUserUpdatableSeabaseMD
//
// This method returns TRUE if it is an updatable metadata table.
// For the most part metadata tables are no allowed to be updated directly.
// However, there is a subset of tables that can be updated directly.
//
// Since only a few tables are updatable, we will check the names directly
// instead of adding a table attribute.
// ----------------------------------------------------------------------------
NABoolean CmpSeabaseDDL::isUserUpdatableSeabaseMD(const NAString &catName,
                                                  const NAString &schName,
                                                  const NAString &objName)
{
  if ((CmpCommon::getDefault(MODE_SEABASE) == DF_ON) &&
      (NOT catName.isNull()))
    {
      NAString seabaseDefCatName = "";
      CmpCommon::getDefault(SEABASE_CATALOG, seabaseDefCatName, FALSE);
      seabaseDefCatName.toUpper();
      
      if ((catName == seabaseDefCatName) &&
          (schName == SEABASE_MD_SCHEMA) &&
          (objName == SEABASE_DEFAULTS))
        {
          return TRUE;
        }
    }

  return FALSE;
}

ExpHbaseInterface* CmpSeabaseDDL::allocEHI(const char * server, 
                                           const char * port, 
                                           const char * interface,
                                           const char * zkPort,
                                           NABoolean raiseError)
{
  ExpHbaseInterface * ehi =  NULL;

  ehi = ExpHbaseInterface::newInstance
    (heap_, server, port, interface, zkPort);
    
  Lng32 retcode = ehi->init();
  if (retcode < 0)
    {
      if (raiseError) {
        *CmpCommon::diags() << DgSqlCode(-8448)
                          << DgString0((char*)"ExpHbaseInterface::init()")
                          << DgString1(getHbaseErrStr(-retcode))
                          << DgInt0(-retcode)
                          << DgString2((char*)GetCliGlobals()->getJniErrorStr().data());
      }

      deallocEHI(ehi); 
        
      return NULL;
    }
    
  return ehi;
}

ExpHbaseInterface* CmpSeabaseDDL::allocEHI(NADefaults * defs)
{
  ExpHbaseInterface * ehi =  NULL;

  NADefaults *defsL = defs;
  if (!defsL)
    defsL = &ActiveSchemaDB()->getDefaults();
  
  const char * server = defsL->getValue(HBASE_SERVER);
  const char * port = defsL->getValue(HBASE_THRIFT_PORT);
  const char* interface = defsL-> getValue(HBASE_INTERFACE);
  const char* zkPort = defsL->getValue(HBASE_ZOOKEEPER_PORT);

  ehi = allocEHI(server, port, interface, zkPort, TRUE);
    
  return ehi;
}

void CmpSeabaseDDL::deallocEHI(ExpHbaseInterface* &ehi)
{
  if (ehi)
    delete ehi;

  ehi = NULL;
}

ComBoolean CmpSeabaseDDL::isSeabaseMD(const ComObjectName &name) 
{
  return isSeabaseMD(name.getCatalogNamePartAsAnsiString(),
                   name.getSchemaNamePartAsAnsiString(TRUE),
                   name.getObjectNamePartAsAnsiString());
}

void CmpSeabaseDDL::getColName(const char * colFam, const char * colQual,
                               NAString &colName)
{
  char c;

  colName.resize(0);

  colName = colFam;
  colName += ":";
  c = str_atoi(colQual, strlen(colQual));
  colName += c;
}

short CmpSeabaseDDL::readAndInitDefaultsFromSeabaseDefaultsTable
(NADefaults::Provenance overwriteIfNotYet, Int32 errOrWarn,
 NADefaults *defs)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  if (defs->seabaseDefaultsTableRead())
    return 0;

  const char * server = defs->getValue(HBASE_SERVER);
  const char * port = defs->getValue(HBASE_THRIFT_PORT);
  const char * interface = defs->getValue(HBASE_INTERFACE);
  const char * zkPort = defs->getValue(HBASE_ZOOKEEPER_PORT);

  HbaseStr hbaseDefaults;
  NAString hbaseDefaultsStr(getSystemCatalog());
  hbaseDefaultsStr += ".";
  hbaseDefaultsStr += SEABASE_MD_SCHEMA;
  hbaseDefaultsStr += ".";
  hbaseDefaultsStr += SEABASE_DEFAULTS;
  hbaseDefaults.val = (char*)hbaseDefaultsStr.data();
  hbaseDefaults.len = hbaseDefaultsStr.length();

  NAString col1NameStr;
  NAString col2NameStr;

  getColName(SEABASE_DEFAULT_COL_FAMILY, "1", col1NameStr);
  getColName(SEABASE_DEFAULT_COL_FAMILY, "2", col2NameStr);

  NAList<Text> col1ValueList;
  NAList<Text> col2ValueList;
  NAList<Text> listUnused;
  Text col1TextStr(col1NameStr);
  Text col2TextStr(col2NameStr);
  Text colUnused;

  ExpHbaseInterface * ehi = allocEHI(server, port, interface, zkPort, FALSE);
  if (! ehi)
    {
      retcode = -1398;
      goto label_return;
    }

  retcode = existsInHbase(hbaseDefaultsStr, ehi);
  if (retcode != 1) // does not exist
    {
      retcode = -1394;
      goto label_return;
    }

  retcode = ehi->fetchAllRows(hbaseDefaults, 
                              2, // numCols
                              col1TextStr, col2TextStr, colUnused,
                              col1ValueList, col2ValueList, listUnused);
  if (retcode != HBASE_ACCESS_SUCCESS)
    {
      retcode = -1394;

      goto label_return;
    }

  retcode = 0;

  defs->setSeabaseDefaultsTableRead(TRUE);

  for (Lng32 i = 0; i < col1ValueList.entries(); i++)
    {
      NAString attrName(col1ValueList[i].data(), col1ValueList[i].length());
      NAString attrValue(col2ValueList[i].data(), col2ValueList[i].length());

      defs->validateAndInsert(attrName, attrValue, FALSE, errOrWarn, overwriteIfNotYet);
    }
 label_return:
  deallocEHI(ehi);

  return retcode;
}   

#define VERS_CV_MAJ 1
#define VERS_CV_MIN 0
#define VERS_CV_UPD 1
VERS_BIN(xx) // get rid of warning

short CmpSeabaseDDL::getSystemSoftwareVersion(Int64 &softMajVers, 
                                              Int64 &softMinVers,
                                              Int64 &softUpdVers)
{
  //  int cmaj, cmin, cupd;
  int pmaj, pmin, pupd;
  CALL_COMP_GET_PROD_VERS(xx,&pmaj,&pmin,&pupd);
  softMajVers = pmaj;
  softMinVers = pmin;
  softUpdVers = pupd;
  //  CALL_COMP_GET_COMP_VERS(xx,cmaj,cmin,cupd);
  //  printf("pvers=%d.%d.%d\n", pmaj, pmin, pupd);
  //  printf("cvers=%d.%d.%d\n", cmaj, cmin, cupd);

  return 0;
}

short CmpSeabaseDDL::validateVersions(NADefaults *defs, 
                                      ExpHbaseInterface * inEHI,
                                      Int64 * mdMajorVersion,
                                      Int64 * mdMinorVersion,
                                      Int64 * sysSWMajorVersion,
                                      Int64 * sysSWMinorVersion,
                                      Int64 * sysSWUpdVersion,
                                      Int64 * mdSWMajorVersion,
                                      Int64 * mdSWMinorVersion,
                                      Lng32 * hbaseErrNum,
                                      NAString * hbaseErrStr)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  processSystemCatalog(defs);

  HbaseStr hbaseVersions;

  NAString col1NameStr;
  NAString col2NameStr;
  NAString col3NameStr;

  NAList<Text> col1ValueList;
  NAList<Text> col2ValueList;
  NAList<Text> col3ValueList;

  getColName(SEABASE_DEFAULT_COL_FAMILY, "1", col1NameStr);
  getColName(SEABASE_DEFAULT_COL_FAMILY, "2", col2NameStr);
  getColName(SEABASE_DEFAULT_COL_FAMILY, "3", col3NameStr);

  Text col1TextStr(col1NameStr);
  Text col2TextStr(col2NameStr);
  Text col3TextStr(col3NameStr);

  NAString hbaseVersionsStr(getSystemCatalog());
  hbaseVersionsStr += ".";
  hbaseVersionsStr += SEABASE_MD_SCHEMA;
  hbaseVersionsStr += ".";
  hbaseVersionsStr += SEABASE_VERSIONS;
  hbaseVersions.val = (char*)hbaseVersionsStr.data();
  hbaseVersions.len = hbaseVersionsStr.length();

  NABoolean mdVersionFound = FALSE;
  NABoolean invalidMD = FALSE;
  ExpHbaseInterface * ehi = inEHI;
  if (! ehi)
    {
      const char * server = defs->getValue(HBASE_SERVER);
      const char * port = defs->getValue(HBASE_THRIFT_PORT);
      const char * interface = defs->getValue(HBASE_INTERFACE);
      const char * zkPort = defs->getValue(HBASE_ZOOKEEPER_PORT);
      
      ehi = allocEHI(server, port, interface, zkPort, TRUE);
      if (! ehi)
        {
          // extract error info from diags area.
          if ((CmpCommon::diags()) &&
              (CmpCommon::diags()->getNumber() > 0))
            {
              ComCondition &cc = (*CmpCommon::diags())[1];
              if (cc.getSQLCODE() == -8448)
                {
                  if (hbaseErrNum)
                    *hbaseErrNum = cc.getOptionalInteger(0);
                  if (hbaseErrStr)
                    *hbaseErrStr = cc.getOptionalString(2);
                }

              CmpCommon::diags()->clear();
            }

          retcode = -1398;
          goto label_return;
        }
    }

  retcode = isMetadataInitialized(ehi);
  if (retcode < 0)
    {
      if (hbaseErrNum)
        *hbaseErrNum = retcode;

      retcode = -1398;
      goto label_return;
    }

  if (retcode == 0) // not initialized
    {
      retcode = -1393;
      goto label_return;
    }

  if (retcode == 2)
    invalidMD = TRUE;

  retcode = existsInHbase(hbaseVersionsStr, ehi);
  if (retcode != 1) // does not exist
    {
      retcode = -1394;
      goto label_return;
    }

  retcode = ehi->fetchAllRows(hbaseVersions, 
                              3, // numCols
                              col1TextStr, col2TextStr, col3TextStr,
                              col1ValueList, col2ValueList, col3ValueList);
  if (retcode != HBASE_ACCESS_SUCCESS)
    {
      if (hbaseErrNum)
        *hbaseErrNum = retcode;

      retcode = -1394;
      goto label_return;
    }
  else
    retcode = 0;

  if (col1ValueList.entries() == 0)
    {
      retcode = -1394;
      goto label_return;
    }

  for (Lng32 i = 0; i < col1ValueList.entries(); i++)
    {
      NAString versionType(col1ValueList[i].data(), col1ValueList[i].length());
      Int64 majorVersion = *(Int64*)col2ValueList[i].data();
      Int64 minorVersion = *(Int64*)col3ValueList[i].data();

      NAString temp = versionType.strip(NAString::trailing, ' ');
      if (temp == "METADATA")
        {
          if (mdMajorVersion)
            *mdMajorVersion = majorVersion;
          if (mdMinorVersion)
            *mdMinorVersion = minorVersion;

          mdVersionFound = TRUE;
          if ((majorVersion != METADATA_MAJOR_VERSION) ||
              (minorVersion != METADATA_MINOR_VERSION))
            {
              // version mismatch. Check if metadata is corrupt or need to be upgraded.
              if (isOldMetadataInitialized(ehi))
                retcode = -1395;
              else
                retcode = -1394;
              goto label_return;
            }
        }

      if (temp == "DATAFORMAT")
        {
          if ((majorVersion != DATAFORMAT_MAJOR_VERSION) ||
              (minorVersion != DATAFORMAT_MINOR_VERSION))
            {
              retcode = -1396;
              goto label_return;
            }
        }

      if (temp == "SOFTWARE")
        {
          Int64 sysMajorVersion = 0;
          Int64 sysMinorVersion = 0;
          Int64 sysUpdVersion = 0;

          getSystemSoftwareVersion(sysMajorVersion, sysMinorVersion, sysUpdVersion);
          if (sysSWMajorVersion)
            *sysSWMajorVersion = sysMajorVersion;
          if (sysSWMinorVersion)
            *sysSWMinorVersion = sysMinorVersion;
          if (sysSWUpdVersion)
            *sysSWUpdVersion = sysUpdVersion;

          if (mdSWMajorVersion)
            *mdSWMajorVersion = majorVersion;
          if (mdSWMinorVersion)
            *mdSWMinorVersion = minorVersion;

          if ((sysMajorVersion != SOFTWARE_MAJOR_VERSION) ||
              (sysMinorVersion != SOFTWARE_MINOR_VERSION) ||
              (sysUpdVersion != SOFTWARE_UPDATE_VERSION))
            {
              retcode = -1397;
              goto label_return;
            }
        }
    }

  if ((NOT mdVersionFound) ||
      (invalidMD))
    {
      retcode = -1394;
      goto label_return;
    }
 label_return:
  if (! inEHI)
    deallocEHI(ehi);
  return retcode;
}   

short CmpSeabaseDDL::sendAllControlsAndFlags(CmpContext* prevContext)
{
  const NAString * val =
    ActiveControlDB()->getControlSessionValue("SHOWPLAN");
  NABoolean sendCSs = TRUE;
  if ((val) && (*val == "ON"))
    {
      // we are within a showplan session.
      // Do not send the SHOWPLAN control session or it will cause problems
      // in generation of metadata plans.
      sendCSs = FALSE;
    }

  // save the current parserflags setting from this compiler and executor context
  savedCmpParserFlags_ = Get_SqlParser_Flags (0xFFFFFFFF);
  SQL_EXEC_GetParserFlagsForExSqlComp_Internal(savedCliParserFlags_);

  if (sendAllControls(FALSE, FALSE, FALSE, COM_VERS_COMPILER_VERSION, sendCSs, prevContext) < 0)
    return -1;

  Lng32 cliRC;
  ExeCliInterface cliInterface(STMTHEAP);

  cliRC = cliInterface.holdAndSetCQD("volatile_schema_in_use", "OFF");
  if (cliRC < 0)
    return -1;

  cliRC = cliInterface.executeImmediate("control query shape hold;");
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  cliRC = cliInterface.holdAndSetCQD("hbase_filter_preds", "OFF");
  if (cliRC < 0)
    return -1;

  // turn off esp parallelism until optimizer fixes esp plan issue pbm.
  cliRC = cliInterface.holdAndSetCQD("attempt_esp_parallelism", "OFF");
  if (cliRC < 0)
    return -1;

  // this cqd causes problems when internal indexes are created.
  // disable it here for ddl operations.
  // Not sure if this cqd is used anywhere or is needed. Maybe we should remove it.
  cliRC = cliInterface.holdAndSetCQD("hide_indexes", "NONE");
  if (cliRC < 0)
    return -1;

  SQL_EXEC_SetParserFlagsForExSqlComp_Internal(INTERNAL_QUERY_FROM_EXEUTIL);

  Set_SqlParser_Flags(ALLOW_VOLATILE_SCHEMA_IN_TABLE_NAME);
  
  SQL_EXEC_SetParserFlagsForExSqlComp_Internal(ALLOW_VOLATILE_SCHEMA_IN_TABLE_NAME);

  return 0;
}

void CmpSeabaseDDL::restoreAllControlsAndFlags()
{
  Lng32 cliRC;
  ExeCliInterface cliInterface(STMTHEAP);

  cliRC = cliInterface.executeImmediate("control query shape restore;");

  cliRC = cliInterface.restoreCQD("volatile_schema_in_use");

  cliRC = cliInterface.restoreCQD("hbase_filter_preds");

  cliRC = cliInterface.restoreCQD("hide_indexes");

  cliRC = cliInterface.restoreCQD("attempt_esp_parallelism");

  // Restore parser flags settings of cmp and exe context to what they originally were
  Set_SqlParser_Flags (savedCmpParserFlags_);
  SQL_EXEC_AssignParserFlagsForExSqlComp_Internal(savedCliParserFlags_);
  
  return;
}

void CmpSeabaseDDL::processReturn(Lng32 retcode)
{
  return;
}

// Return value:
//   0: no metadata tables exist, metadata is not initialized
//   1: all metadata tables exists, metadata is initialized
//   2: some metadata tables exist, metadata is corrupted
//  -ve: error code
short CmpSeabaseDDL::isMetadataInitialized(ExpHbaseInterface * ehi)
{
  short retcode ;

  ExpHbaseInterface * ehil = ehi;
  if (ehil == NULL)
    {
      ehil = allocEHI();
      if (ehil == NULL)
        return 0;
    }

  // check to see if VERSIONS table exist. If it exist, then metadata is initialized.
  // This is a quick check.
  // We may still run into an issue where other metadata tables are missing or
  // corrupted. That will cause an error to be returned when that table is actually
  // accessed.
  HbaseStr hbaseVersions;
  NAString hbaseVersionsStr(getSystemCatalog());
  hbaseVersionsStr += ".";
  hbaseVersionsStr += SEABASE_MD_SCHEMA;
  hbaseVersionsStr += ".";
  hbaseVersionsStr += SEABASE_VERSIONS;
  hbaseVersions.val = (char*)hbaseVersionsStr.data();
  hbaseVersions.len = hbaseVersionsStr.length();
  retcode = ehi->exists(hbaseVersions);
  if (retcode == -1) // already exists
    {
      return 1; // metadata is initialized
    }
  else if (retcode != 0)
    {
      return retcode; // error accesing hbase
    }

  Lng32 numTotal = 0;
  Lng32 numExists = 0;
  retcode = 0;
  for (Int32 i = 0; 
       (((retcode == 0) || (retcode == -1)) && (i < sizeof(allMDtablesInfo)/sizeof(MDTableInfo))); i++)
    {
      const MDTableInfo &mdi = allMDtablesInfo[i];
      
      if (! mdi.newName)
        continue;

      if (mdi.droppedTable)
        continue;

      numTotal++;
      HbaseStr hbaseTables;
      NAString hbaseTablesStr(getSystemCatalog());
      hbaseTablesStr += ".";
      hbaseTablesStr += SEABASE_MD_SCHEMA;
      hbaseTablesStr += ".";
      hbaseTablesStr += mdi.newName;
      hbaseTables.val = (char*)hbaseTablesStr.data();
      hbaseTables.len = hbaseTablesStr.length();
      
      retcode = ehil->exists(hbaseTables);
      if (retcode == -1)
        numExists++;
    }
  
  if (ehi == NULL)
    deallocEHI(ehil); 

  if ((retcode != 0) && (retcode != -1))
    return retcode; // error accessing metadata

  if (numExists == 0) 
    return 0; // metadata not initialized

  if (numExists == numTotal)
    return 1; // metadata is initialized
  
  if (numExists < numTotal)
    return 2; // metadata is corrupt

  return -1;
}

// Return value:
//   0: not all old metadata tables exist, metadata is not initialized
//   1: all old metadata tables exists, metadata is initialized
//  -ve: error code
short CmpSeabaseDDL::isOldMetadataInitialized(ExpHbaseInterface * ehi)
{
  short retcode ;

  Lng32 numTotal = 0;
  Lng32 numExists = 0;
  retcode = 0;
  for (Int32 i = 0; 
       (((retcode == 0) || (retcode == -1)) && (i < sizeof(allMDtablesInfo)/sizeof(MDTableInfo))); i++)
    {
      const MDTableInfo &mdi = allMDtablesInfo[i];
      
      if (! mdi.oldName)
        continue;

      if (mdi.addedTable)
        continue;

      NAString oldName(mdi.oldName);
      size_t pos = oldName.index(OLD_MD_EXTENSION);
      oldName = oldName.remove(pos, sizeof(OLD_MD_EXTENSION));

      numTotal++;
      HbaseStr hbaseTables;
      NAString hbaseTablesStr(getSystemCatalog());
      hbaseTablesStr += ".";
      hbaseTablesStr += SEABASE_MD_SCHEMA;
      hbaseTablesStr += ".";
      hbaseTablesStr += oldName;
      hbaseTables.val = (char*)hbaseTablesStr.data();
      hbaseTables.len = hbaseTablesStr.length();
      
      retcode = ehi->exists(hbaseTables);
      if (retcode == -1) // exists
        numExists++;
    }
  
  if ((retcode != 0) && (retcode != -1))
    return retcode; // error accessing metadata

  if (numExists < numTotal) 
    return 0; // corrupted or uninitialized old metadata

  if (numExists == numTotal)
    return 1; // metadata is initialized
  
  return -1;
}

NABoolean CmpSeabaseDDL::isAuthorizationEnabled()
{
  if (authorizationEnabled_ == -1)
  {
    // Initiate the privilege manager interface class
    std::string privMDLoc(ActiveSchemaDB()->getDefaults().getValue(SEABASE_CATALOG));
    privMDLoc += std::string(".\"") +
                 std::string(SEABASE_PRIVMGR_SCHEMA) +
                 std::string("\"");
    PrivMgrCommands privInterface(privMDLoc, CmpCommon::diags());

    if (privInterface.isAuthorizationEnabled())
      authorizationEnabled_ = 1;
    else
      authorizationEnabled_ = 0;
  }
  return authorizationEnabled_;
}

short CmpSeabaseDDL::existsInHbase(const NAString &objName,
                                   ExpHbaseInterface * ehi)
{
  ExpHbaseInterface * ehil = ehi;
  if (! ehi)
    {
      ehil = allocEHI();
      if (ehil == NULL)
        return -1;
    }

  HbaseStr hbaseObj;
  hbaseObj.val = (char*)objName.data();
  hbaseObj.len = objName.length();
  Lng32 retcode = ehil->exists(hbaseObj);

  if (! ehi)
    {
      deallocEHI(ehil); 
    }

  if (retcode == -1) // already exists
    return 1;
  
  if (retcode == 0)
    return 0; // does not exist

  return retcode; // error
}

// ----------------------------------------------------------------------------
// Method:  processSystemCatalog
//
// This method sets up system catalog name in the CmpSeabaseDDL class
// 
// The system define called SEABASE_CATALOG can be used to overwrite the 
// default name of TRAFODION.
// ----------------------------------------------------------------------------
void CmpSeabaseDDL::processSystemCatalog(NADefaults *defs)
{
  NAString value(TRAFODION_SYSCAT_LIT);

  if (defs)
    defs->validateAndInsert("SEABASE_CATALOG", value, FALSE);
  else
    ActiveSchemaDB()->getDefaults().validateAndInsert(
                                                      "SEABASE_CATALOG", value, FALSE);

  seabaseSysCat_ = value;
}

const char * CmpSeabaseDDL::getSystemCatalog()
{
  return seabaseSysCat_.data();
}

NAString CmpSeabaseDDL::getSystemCatalogStatic()
{
  NAString value(TRAFODION_SYSCAT_LIT);

  if (CmpCommon::context() && ActiveSchemaDB())
    {
      const char* sysCat = ActiveSchemaDB()->getDefaults().getValue(SEABASE_CATALOG);

      value = sysCat;
    }

  return value;
}

NABoolean CmpSeabaseDDL::xnInProgress(ExeCliInterface *cliInterface)
{
  if (cliInterface->statusXn() == 0) // xn in progress
    return TRUE;
  else
    return FALSE;
}

short CmpSeabaseDDL::beginXn(ExeCliInterface *cliInterface)
{
  Lng32 cliRC = 0;

  cliRC = cliInterface->beginWork();
  return cliRC;
}

short CmpSeabaseDDL::commitXn(ExeCliInterface *cliInterface)
{
  Lng32 cliRC = 0;

  cliRC = cliInterface->commitWork();
  return cliRC;
}

short CmpSeabaseDDL::rollbackXn(ExeCliInterface *cliInterface)
{
  Lng32 cliRC = 0;

  cliRC = cliInterface->rollbackWork();
  return cliRC;
}

short CmpSeabaseDDL::populateKeyInfo(ComTdbVirtTableKeyInfo &keyInfo,
                                     OutputInfo * oi, NABoolean isIndex)
{

  // get the column name
  Lng32 len = strlen(oi->get(0));
  keyInfo.colName = new(STMTHEAP) char[len + 1];

  strcpy((char*)keyInfo.colName, (char*)oi->get(0));

  keyInfo.tableColNum = *(Lng32*)oi->get(1);

  keyInfo.keySeqNum = *(Lng32*)oi->get(2);

  keyInfo.ordering = *(Lng32*)oi->get(3);
  
  keyInfo.nonKeyCol = *(Lng32*)oi->get(4);

  if (isIndex)
    {
      keyInfo.hbaseColFam = new(STMTHEAP) char[strlen(SEABASE_DEFAULT_COL_FAMILY) + 1];
      strcpy((char*)keyInfo.hbaseColFam, SEABASE_DEFAULT_COL_FAMILY);
      
      char qualNumStr[40];
      str_sprintf(qualNumStr, "@%d", keyInfo.keySeqNum);
      
      keyInfo.hbaseColQual = new(STMTHEAP) char[strlen(qualNumStr)+1];
      strcpy((char*)keyInfo.hbaseColQual, qualNumStr);
    }
  else
    {
      keyInfo.hbaseColFam = NULL;
      keyInfo.hbaseColQual = NULL;
    }

  return 0;
}

NABoolean CmpSeabaseDDL::enabledForSerialization(NAColumn * nac)
{
  const NAType *givenType = nac->getType();
  if ((nac) &&
      ((NOT givenType->isEncodingNeeded()) ||
       (nac && CmpSeabaseDDL::isSerialized(nac->getHbaseColFlags()))))
    {
      return TRUE;
    }

  return FALSE;
}

NABoolean CmpSeabaseDDL::isEncodingNeededForSerialization(NAColumn * nac)
{
  const NAType *givenType = nac->getType();
  if ((nac) &&
      (CmpSeabaseDDL::isSerialized(nac->getHbaseColFlags())) &&
      ((givenType->isEncodingNeeded()) &&
       (NOT DFS2REC::isAnyVarChar(givenType->getFSDatatype()))))
    {
      return TRUE;
    }

  return FALSE;
}

short CmpSeabaseDDL::createHbaseTable(ExpHbaseInterface *ehi, 
                                      HbaseStr *table,
                                      const char * cf1, 
                                      const char * cf2, 
                                      const char * cf3,
                                      NAList<HbaseCreateOption*> * hbaseCreateOptions,
                                      const int numSplits,
                                      const int keyLength,
                                      char** encodedKeysBuffer,
                                      NABoolean doRetry)
{
  // this method is called after validating that the table doesn't exist in seabase
  // metadata. It creates the corresponding hbase table.
  short retcode = 0;

  // create HBASE_MD table.
  HBASE_NAMELIST colFamList;
  HbaseStr colFam;
  
  retcode = -1;
  Lng32 numTries = 0;
  Lng32 delaySecs = 500; // 5 secs to start with
  if (doRetry)
    {
      while ((numTries < 24) && (retcode == -1)) // max 2 min
        {
          retcode = ehi->exists(*table);
          if (retcode == -1)
            {
              // if this state is reached, it indicates that the table was not found in metadata
              // but exists in hbase. This may be due to that table being dropped from another
              // process or thread in an asynchronous manner.
              // Delay and check again.
              numTries++;
              
              DELAY(delaySecs); 
            }
        } // while
    }
  else
    retcode = ehi->exists(*table);
    
  if (retcode == -1)
    {
      *CmpCommon::diags() << DgSqlCode(-1390)
                          << DgString0(table->val);
      return -1;
    } 
  
  if (retcode < 0)
    {
      *CmpCommon::diags() << DgSqlCode(-8448)
                          << DgString0((char*)"ExpHbaseInterface::exists()")
                          << DgString1(getHbaseErrStr(-retcode))
                          << DgInt0(-retcode)
                          << DgString2((char*)GetCliGlobals()->getJniErrorStr().data());
      
      return -1;
    }

  NAText hbaseCreateOptionsArray[HBASE_MAX_OPTIONS];
  if (hbaseCreateOptions)
    {
      for (CollIndex i = 0; i < hbaseCreateOptions->entries(); i++)
        {
          HbaseCreateOption * hbaseOption = (*hbaseCreateOptions)[i];
          NAText &s = hbaseOption->val();
          NAText valInOrigCase;

          // trim leading and trailing spaces
          size_t startpos = s.find_first_not_of(" ");
          if (startpos != string::npos) // found a non-space character
            {
              size_t endpos = s.find_last_not_of(" ");
              s = s.substr( startpos, endpos-startpos+1 );
            }
          
          // upcase value, save original (now trimmed)
          valInOrigCase = s;
          std::transform(s.begin(), s.end(), s.begin(), ::toupper);
          
          NABoolean isError = FALSE;
          if (hbaseOption->key() == "NAME")
            {
              hbaseCreateOptionsArray[HBASE_NAME] = hbaseOption->val();
            }
          
          else if (hbaseOption->key() == "MAX_VERSIONS")
            {
              if (str_atoi(hbaseOption->val().data(), 
                           hbaseOption->val().length()) == -1)
                isError = TRUE;
              hbaseCreateOptionsArray[HBASE_MAX_VERSIONS] = hbaseOption->val();
            }
          else if (hbaseOption->key() == "MIN_VERSIONS")
            {
              if (str_atoi(hbaseOption->val().data(), 
                           hbaseOption->val().length()) == -1)
                isError = TRUE;
              hbaseCreateOptionsArray[HBASE_MIN_VERSIONS] = hbaseOption->val();
            }
          else if ((hbaseOption->key() == "TIME_TO_LIVE") ||
                   (hbaseOption->key() == "TTL"))
            {
              if (str_atoi(hbaseOption->val().data(), 
                           hbaseOption->val().length()) == -1)
                isError = TRUE;
              hbaseCreateOptionsArray[HBASE_TTL] = hbaseOption->val();
            }
          else if (hbaseOption->key() == "BLOCKCACHE")
            {
              hbaseCreateOptionsArray[HBASE_BLOCKCACHE] = hbaseOption->val();
            }
          else if (hbaseOption->key() == "IN_MEMORY")
            {
              hbaseCreateOptionsArray[HBASE_IN_MEMORY] = hbaseOption->val();
            }
          else if (hbaseOption->key() == "COMPRESSION")
            {
              hbaseCreateOptionsArray[HBASE_COMPRESSION] = hbaseOption->val();
            }
          else if (hbaseOption->key() == "BLOOMFILTER")
            {
              hbaseCreateOptionsArray[HBASE_BLOOMFILTER] = hbaseOption->val();
            }
          else if (hbaseOption->key() == "BLOCKSIZE")
            {
              if (str_atoi(hbaseOption->val().data(), 
                           hbaseOption->val().length()) == -1)
                isError = TRUE;
              hbaseCreateOptionsArray[HBASE_BLOCKSIZE] = hbaseOption->val();
            }
          else if (hbaseOption->key() == "DATA_BLOCK_ENCODING")
            {
              if (hbaseOption->val() != "NONE" &&
                  hbaseOption->val() != "PREFIX" &&
                  hbaseOption->val() != "DIFF" &&
                  hbaseOption->val() != "FAST_DIFF")
                isError = TRUE;
              hbaseCreateOptionsArray[HBASE_DATA_BLOCK_ENCODING] = 
                hbaseOption->val();
            }
          else if (hbaseOption->key() == "CACHE_BLOOMS_ON_WRITE")
            {
              hbaseCreateOptionsArray[HBASE_CACHE_BLOOMS_ON_WRITE] = 
                hbaseOption->val();
            }
          else if (hbaseOption->key() == "CACHE_DATA_ON_WRITE")
            {
              hbaseCreateOptionsArray[HBASE_CACHE_DATA_ON_WRITE] = 
                hbaseOption->val();
            }
          else if (hbaseOption->key() == "CACHE_INDEXES_ON_WRITE")
            {
              hbaseCreateOptionsArray[HBASE_CACHE_INDEXES_ON_WRITE] = 
                hbaseOption->val();
            }
          else if (hbaseOption->key() == "COMPACT_COMPRESSION")
            {
              hbaseCreateOptionsArray[HBASE_COMPACT_COMPRESSION] = 
                hbaseOption->val();
            }
          else if (hbaseOption->key() == "PREFIX_LENGTH_KEY")
            {
              if (str_atoi(hbaseOption->val().data(), 
                           hbaseOption->val().length()) == -1)
                isError = TRUE;
              hbaseCreateOptionsArray[HBASE_PREFIX_LENGTH_KEY] = 
                hbaseOption->val();
            }
          else if (hbaseOption->key() == "EVICT_BLOCKS_ON_CLOSE")
            {
              hbaseCreateOptionsArray[HBASE_EVICT_BLOCKS_ON_CLOSE] = 
                hbaseOption->val();
            }
          else if (hbaseOption->key() == "KEEP_DELETED_CELLS")
            {
              hbaseCreateOptionsArray[HBASE_KEEP_DELETED_CELLS] = 
                hbaseOption->val();
            }
          else if (hbaseOption->key() == "REPLICATION_SCOPE")
            {
              if (str_atoi(hbaseOption->val().data(), 
                           hbaseOption->val().length()) == -1)
                isError = TRUE;
              hbaseCreateOptionsArray[HBASE_REPLICATION_SCOPE] = 
                hbaseOption->val();
            }
          else if (hbaseOption->key() == "MAX_FILESIZE")
            {
              if (str_atoi(hbaseOption->val().data(), 
                           hbaseOption->val().length()) == -1)
                isError = TRUE;
              hbaseCreateOptionsArray[HBASE_MAX_FILESIZE] = hbaseOption->val();
            }
          else if (hbaseOption->key() == "COMPACT")
            {
              hbaseCreateOptionsArray[HBASE_COMPACT] = hbaseOption->val();
            }
          else if (hbaseOption->key() == "DURABILITY")
            {
              hbaseCreateOptionsArray[HBASE_DURABILITY] = hbaseOption->val();
            }
          else if (hbaseOption->key() == "MEMSTORE_FLUSH_SIZE")
            {
              if (str_atoi(hbaseOption->val().data(), 
                           hbaseOption->val().length()) == -1)
                isError = TRUE;
              hbaseCreateOptionsArray[HBASE_MEMSTORE_FLUSH_SIZE] = 
                hbaseOption->val();
            }
          else if (hbaseOption->key() == "SPLIT_POLICY")
            {
              // for now, restrict the split policies to some well-known
              // values, because specifying an invalid class gets us into
              // a hang situation in the region server
              if (valInOrigCase == "org.apache.hadoop.hbase.regionserver.ConstantSizeRegionSplitPolicy" ||
                  valInOrigCase == "org.apache.hadoop.hbase.regionserver.IncreasingToUpperBoundRegionSplitPolicy"
 ||
                  valInOrigCase == "org.apache.hadoop.hbase.regionserver.KeyPrefixRegionSplitPolicy")
                hbaseCreateOptionsArray[HBASE_SPLIT_POLICY] = valInOrigCase;
              else
                {
                  *CmpCommon::diags() << DgSqlCode(-8449)
                                      << DgString0(hbaseOption->key().data())
                                      << DgString1(valInOrigCase.data());
                  return -1;
                }
            }
          else
            isError = TRUE;

          if (isError)
            {
              retcode = -HBASE_CREATE_OPTIONS_ERROR;
              *CmpCommon::diags() << DgSqlCode(-8448)
                                  << DgString0((char*)"ExpHbaseInterface::create()")
                                  << DgString1(getHbaseErrStr(-retcode))
                                  << DgInt0(-retcode)
                                  << DgString2((char*)hbaseOption->key().data());
              
              return -1;
            }
        } // for
    }

  else
    {
      colFamList.clear();
      char cfName[1000];
      
      if (cf1)
        {
          strcpy(cfName, cf1);
          colFam.val = cfName;
          colFam.len = strlen(cfName);
          
          colFamList.insert(colFam);
        }
      
      char cfName2[1000];
      
      if (cf2)
        {
          strcpy(cfName2, cf2);
          colFam.val = cfName2;
          colFam.len = strlen(cfName2);
          
          colFamList.insert(colFam);
        }
      
      char cfName3[1000];
      
      if (cf3)
        {
          strcpy(cfName3, cf3);
          colFam.val = cfName3;
          colFam.len = strlen(cfName3);
          
          colFamList.insert(colFam);
        }
    }

  if (hbaseCreateOptions || (numSplits > 0) )
    {
      if (hbaseCreateOptionsArray[HBASE_NAME].empty())
        hbaseCreateOptionsArray[HBASE_NAME] = SEABASE_DEFAULT_COL_FAMILY;

      retcode = ehi->create(*table, hbaseCreateOptionsArray,
                            numSplits, keyLength,
                            (const char **)encodedKeysBuffer);
    }
  else
    {
      retcode = ehi->create(*table, colFamList);
    }

  if (retcode < 0)
    {
      *CmpCommon::diags() << DgSqlCode(-8448)
                          << DgString0((char*)"ExpHbaseInterface::create()")
                          << DgString1(getHbaseErrStr(-retcode))
                          << DgInt0(-retcode)
                          << DgString2((char*)GetCliGlobals()->getJniErrorStr().data());
      
      return -1;
    }
  
  return 0;
}

short CmpSeabaseDDL::dropHbaseTable(ExpHbaseInterface *ehi, 
                                    HbaseStr *table, NABoolean asyncDrop)
{
  short retcode = 0;

  retcode = ehi->exists(*table);
  if (retcode == -1) // exists
    {       
      if ((CmpCommon::getDefault(HBASE_ASYNC_DROP_TABLE) == DF_ON) ||
          (asyncDrop))
        retcode = ehi->drop(*table, TRUE);
      else
        retcode = ehi->drop(*table, FALSE);
      if (retcode < 0)
        {
          *CmpCommon::diags() << DgSqlCode(-8448)
                              << DgString0((char*)"ExpHbaseInterface::drop()")
                              << DgString1(getHbaseErrStr(-retcode))
                              << DgInt0(-retcode)
                              << DgString2((char*)GetCliGlobals()->getJniErrorStr().data());
          
          return -1;
        }
    }

  if (retcode != 0)
    {
      *CmpCommon::diags() << DgSqlCode(-8448)
                          << DgString0((char*)"ExpHbaseInterface::exists()")
                          << DgString1(getHbaseErrStr(-retcode))
                          << DgInt0(-retcode)
                          << DgString2((char*)GetCliGlobals()->getJniErrorStr().data());
      
      return -1;
    }
  
  return 0;
}

short CmpSeabaseDDL::copyHbaseTable(ExpHbaseInterface *ehi, 
                                    HbaseStr *currTable, HbaseStr* oldTable)
{
  short retcode = 0;

  retcode = ehi->exists(*currTable);
  if (retcode == -1) // exists
    {       
      retcode = ehi->copy(*currTable, *oldTable);
      if (retcode < 0)
        {
          *CmpCommon::diags() << DgSqlCode(-8448)
                              << DgString0((char*)"ExpHbaseInterface::copy()")
                              << DgString1(getHbaseErrStr(-retcode))
                              << DgInt0(-retcode)
                              << DgString2((char*)GetCliGlobals()->getJniErrorStr().data());
          
          return -1;
        }
    }

  if (retcode != 0)
    {
      *CmpCommon::diags() << DgSqlCode(-8448)
                          << DgString0((char*)"ExpHbaseInterface::copy()")
                          << DgString1(getHbaseErrStr(-retcode))
                          << DgInt0(-retcode)
                          << DgString2((char*)GetCliGlobals()->getJniErrorStr().data());
      
      return -1;
    }
  
  return 0;
}

short CmpSeabaseDDL::checkDefaultValue(
                                       const NAString & colExtName,
                                       const NAType   * colType,
                                       ElemDDLColDef   * colNode)
{
  short rc = 0;

  ConstValue *cvDefault = (ConstValue *)colNode->getDefaultValueExpr();

  NAType * newType = (NAType *)colType;
  if ((colType->getTypeQualifier() == NA_CHARACTER_TYPE) &&
      (cvDefault->getType()->getTypeQualifier() == NA_CHARACTER_TYPE) &&
      (colType->getNominalSize() > cvDefault->getType()->getNominalSize()))
    {
      newType = colType->newCopy(STMTHEAP);
      newType->setNominalSize(
                              MAXOF(cvDefault->getType()->getNominalSize(), 1));
    }

  NAString castToTypeStr(newType->getTypeSQLname(TRUE));

  char buf[1000];
  str_sprintf(buf, "CAST(@A1 AS %s)", castToTypeStr.data());

  rc = Generator::genAndEvalExpr(CmpCommon::context(),
                                 buf, 1, colNode->getDefaultValueExpr(), NULL,
                                 CmpCommon::diags());

  if (rc)
    {
      *CmpCommon::diags() 
        << DgSqlCode(-CAT_INCOMPATIBLE_DATA_TYPE_IN_DEFAULT_CLAUSE)
        << DgColumnName(colExtName)
        << DgString0(castToTypeStr.data())
        << DgString1(cvDefault->getConstStr());
    }

  return rc;
}

short CmpSeabaseDDL::getTypeInfo(const NAType * naType,
                                 Lng32 serializedOption,
                                 Lng32 &datatype,
                                 Lng32 &length,
                                 Lng32 &precision,
                                 Lng32 &scale,
                                 Lng32 &dtStart,
                                 Lng32 &dtEnd,
                                 Lng32 &upshifted,
                                 Lng32 &nullable,
                                 NAString &charset,
                                 CharInfo::Collation &collationSequence,
                                 ULng32 &colFlags)
{
  short rc = 0;

  datatype = 0;
  length = 0;
  precision = 0;
  scale = 0;
  dtStart = 0;
  dtEnd = 0;
  nullable = 0;
  upshifted = 0;

  charset = SQLCHARSETSTRING_UNKNOWN;
  collationSequence = CharInfo::DefaultCollation;

  datatype = (Lng32)naType->getFSDatatype();
  length = naType->getNominalSize();
  nullable = naType->supportsSQLnull();

  switch (naType->getTypeQualifier())
    {
    case NA_CHARACTER_TYPE:
      {
        CharType *charType = (CharType *)naType;
        
        scale = 0;

        precision = charType->getPrecisionOrMaxNumChars();
        charset = CharInfo::getCharSetName(charType->getCharSet());
        upshifted = (charType->isUpshifted() ? -1 : 0);

        collationSequence = charType->getCollation();
        if (serializedOption == 1) // option explicitly specified
          {
            setFlags(colFlags, SEABASE_SERIALIZED);
          }
        else if ((serializedOption == -1) && // not specified
                 (CmpCommon::getDefault(HBASE_SERIALIZATION) == DF_ON))
          {
            setFlags(colFlags, SEABASE_SERIALIZED);
          }
       }
      break;
      
    case NA_NUMERIC_TYPE:
      {
        NumericType *numericType = (NumericType *)naType;
        scale = numericType->getScale();
        
        if (datatype == REC_BPINT_UNSIGNED)
          precision = numericType->getPrecision();
        else if (numericType->binaryPrecision())
          precision = 0;
        else
          precision = numericType->getPrecision();

        if (serializedOption == 1) // option explicitly specified
          {
            if (DFS2REC::isBinary(datatype))
              setFlags(colFlags, SEABASE_SERIALIZED);
            else if (numericType->isEncodingNeeded())
              {
                *CmpCommon::diags() << DgSqlCode(-1191);
                return -1;
              }
          }
        else if ((serializedOption == -1) && // not specified
                 (CmpCommon::getDefault(HBASE_SERIALIZATION) == DF_ON) &&
                 (DFS2REC::isBinary(datatype)))
          {
            setFlags(colFlags, SEABASE_SERIALIZED);
          }
      }
      break;
      
    case NA_DATETIME_TYPE:
    case NA_INTERVAL_TYPE:
      {
        DatetimeIntervalCommonType *dtiCommonType = 
          (DatetimeIntervalCommonType *)naType;
        
        scale = dtiCommonType->getFractionPrecision();
        precision = dtiCommonType->getLeadingPrecision();
        
        dtStart = dtiCommonType->getStartField();
        dtEnd = dtiCommonType->getEndField();

        if ((serializedOption == 1) &&
            (dtiCommonType->isEncodingNeeded()))
          {
            *CmpCommon::diags() << DgSqlCode(-1191);
            return -1;
          }
      }
      break;
      

    default:
      {
        *CmpCommon::diags() << DgSqlCode(-1174);
        
        return -1; 
      }
      
    } // switch

  return 0;
}

short CmpSeabaseDDL::getColInfo(ElemDDLColDef * colNode, 
                                NAString &colName,
                                Lng32 &datatype,
                                Lng32 &length,
                                Lng32 &precision,
                                Lng32 &scale,
                                Lng32 &dtStart,
                                Lng32 &dtEnd,
                                Lng32 &upshifted,
                                Lng32 &nullable,
                                NAString &charset,
                                ComColumnDefaultClass &defaultClass, 
                                NAString &defVal,
                                NAString &heading,
                                LobsStorage &lobStorage,
                                ULng32 &colFlags)
{
  short rc = 0;

  colFlags = 0;

  colName = colNode->getColumnName();

  if (colNode->isHeadingSpecified())
    heading = colNode->getHeading();

  Lng32 serializedOption = -1; // not specified
  if (colNode->isSerializedSpecified())
    {
      serializedOption = (colNode->isSeabaseSerialized() ? 1 : 0);
    }

  NAType * naType = colNode->getColumnDataType();

  CharInfo::Collation collationSequence = CharInfo::DefaultCollation;
  rc = getTypeInfo(naType, serializedOption,
                   datatype, length, precision, scale, dtStart, dtEnd, upshifted, nullable,
                   charset, collationSequence, colFlags);

  if (colName == "SYSKEY")
    {
      resetFlags(colFlags, SEABASE_SERIALIZED);
    }

  if  (collationSequence != CharInfo::DefaultCollation)
    {
      // collation not supported
      *CmpCommon::diags() << DgSqlCode(-4069)
                          << DgColumnName(ToAnsiIdentifier(colName))
                          << DgString0(CharInfo::getCollationName(collationSequence));
      rc = -1;
    }
  
  if (rc)
    {
      return rc;
    }

  lobStorage = Lob_Invalid_Storage;
  if (naType->getTypeQualifier() == NA_LOB_TYPE)
    lobStorage = colNode->getLobStorage();
 
  NABoolean negateIt = FALSE;
  if (colNode->getDefaultClauseStatus() == ElemDDLColDef::NO_DEFAULT_CLAUSE_SPEC)
    defaultClass = COM_NO_DEFAULT;
  else if (colNode->getDefaultClauseStatus() == ElemDDLColDef::DEFAULT_CLAUSE_NOT_SPEC)
    {
      if (nullable)
        {
          defaultClass = COM_NULL_DEFAULT;
        }
      else
        defaultClass = COM_NO_DEFAULT;
    }
  else if (colNode->getDefaultClauseStatus() == ElemDDLColDef::DEFAULT_CLAUSE_SPEC)
    {
      ItemExpr * ie = colNode->getDefaultValueExpr();
      if (ie == NULL)
        if (colNode->getComputedDefaultExpr().isNull())
          defaultClass = COM_NO_DEFAULT;
        else
          {
            defaultClass = COM_ALWAYS_COMPUTE_COMPUTED_COLUMN_DEFAULT;
            defVal = colNode->getComputedDefaultExpr();
          }
      else if (ie->getOperatorType() == ITM_CURRENT_TIMESTAMP)
        {
          defaultClass = COM_CURRENT_DEFAULT;
        }
      else if ((ie->getOperatorType() == ITM_CAST) &&
               (ie->getChild(0)->castToItemExpr()->getOperatorType() == ITM_CURRENT_TIMESTAMP))
        {
          defaultClass = COM_CURRENT_DEFAULT;
        }
      else if ((ie->getOperatorType() == ITM_USER) ||
               (ie->getOperatorType() == ITM_CURRENT_USER) ||
               (ie->getOperatorType() == ITM_SESSION_USER))
        {
          // default USER not currently supported.
          *CmpCommon::diags() << DgSqlCode(-1084)
                              << DgColumnName(colName);
          
          return -1;
          
          defaultClass = COM_USER_FUNCTION_DEFAULT;
        }
     else if (ie->getOperatorType() == ITM_IDENTITY)
       {
         // identity column not currently supported.
         *CmpCommon::diags() << DgSqlCode(-4222)
                             << DgString0("IDENTITY");
         
         return -1;
       }
      else if (ie->castToConstValue(negateIt) != NULL)
        {
          if (ie->castToConstValue(negateIt)->isNull())
            {
              defaultClass = COM_NULL_DEFAULT;
            }
          else
            {
              defaultClass = COM_USER_DEFINED_DEFAULT;

              const ComString name = colNode->getColumnName();
              const NAType * genericType = colNode->getColumnDataType();
              
              rc = checkDefaultValue(ToAnsiIdentifier(name),
                                     genericType, colNode);
              if (rc)
                return -1;

              ie = colNode->getDefaultValueExpr();

              ConstValue * cv = ie->castToConstValue(negateIt);
              NAString nas(NAW_TO_NASTRING(cv->getConstWStr()));

              char def_val[1000];
              str_convertToHexAscii(nas.data(), nas.length()-2, def_val, 1000, TRUE);

              Lng32 currPos = 0;
              Lng32 totalLen = strlen(def_val);
              while (currPos < totalLen)
                {
                  short first2Bytes = *(short*)&def_val[currPos];
                  short last2Bytes = *(short*)&def_val[currPos+2];
                  
                  *(short*)&def_val[currPos] = last2Bytes;
                  *(short*)&def_val[currPos+2] = first2Bytes;

                  currPos += 4;
                }

              defVal.insert(0, def_val);
            }
        }
      else
        defaultClass = COM_NO_DEFAULT;
    }

  return 0;
}

short CmpSeabaseDDL::createRowId(NAString &key,
                                 NAString &part1, Lng32 part1MaxLen,
                                 NAString &part2, Lng32 part2MaxLen,
                                 NAString &part3, Lng32 part3MaxLen,
                                 NAString &part4, Lng32 part4MaxLen)
{
  if (part1.isNull())
    return 0;

  part1MaxLen = part2MaxLen = part3MaxLen = part4MaxLen = 20;

  NAString keyValPadded;

  keyValPadded = part1;
  if (part1.length() < part1MaxLen)
    keyValPadded.append(' ', (part1MaxLen - part1.length()));

  if (NOT part2.isNull())
    {
      keyValPadded += part2;
      if (part2.length() < part2MaxLen)
        keyValPadded.append(' ', (part2MaxLen - part2.length()));
    }

  if (NOT part3.isNull())
    {
      keyValPadded += part3;
      if (part3.length() < part3MaxLen)
        keyValPadded.append(' ', (part3MaxLen - part3.length()));
    }

  if (NOT part4.isNull())
    {
      keyValPadded += part4;
      if (part4.length() < part4MaxLen)
        keyValPadded.append(' ', (part4MaxLen - part4.length()));
    }

  // encode and convertToHex
  key = keyValPadded;

  return 0;
}

///////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
static short isValidHbaseName(const char * str)
{
  
  // A valid hbase name must contain 'word characters': [a-zA-Z_0-9-.]

  for (Lng32 i = 0; i < strlen(str); i++)
    {
      char c = str[i];

      if (NOT (((c >= '0') && (c <= '9')) ||
               ((c >= 'a') && (c <= 'z')) ||
               ((c >= 'A') && (c <= 'Z')) ||
               ((c == '_') || (c == '-') || (c == '.'))))
        return 0; // not a valid name
    }

  return -1; // valid name
}


short CmpSeabaseDDL::existsInSeabaseMDTable(
                                          ExeCliInterface *cliInterface,
                                          const char * catName,
                                          const char * schName,
                                          const char * objName,
                                          const char * objType,
                                          NABoolean checkForValidDef,
                                          NABoolean checkForValidHbaseName)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  char cfvd[100];
  strcpy(cfvd, " ");
  if (checkForValidDef)
    strcpy(cfvd, " and valid_def = 'Y' ");

  // Name must be a valid hbase name
  if ((checkForValidHbaseName) &&
      ((! isValidHbaseName(catName)) ||
       (! isValidHbaseName(schName)) ||
       (! isValidHbaseName(objName))))
    {
      *CmpCommon::diags() << DgSqlCode(-1422);

      return -1;
    }



  NAString quotedSchName;
  ToQuotedString(quotedSchName, NAString(schName), FALSE);
  NAString quotedObjName;
  ToQuotedString(quotedObjName, NAString(objName), FALSE);
  char buf[4000];
  if (objType == NULL)
    str_sprintf(buf, "select count(*) from %s.\"%s\".%s where catalog_name = '%s' and schema_name = '%s' and object_name = '%s' %s ",
                getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
                catName, quotedSchName.data(), quotedObjName.data(),
                cfvd);
  else
    str_sprintf(buf, "select count(*) from %s.\"%s\".%s where catalog_name = '%s' and schema_name = '%s' and object_name = '%s' and object_type = '%s' ",
                getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
                catName, quotedSchName.data(), quotedObjName.data(), objType,
                cfvd);
    
  Lng32 len = 0;
  Int64 rowCount = 0;
  cliRC = cliInterface->executeImmediate(buf, (char*)&rowCount, &len, NULL);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  if (rowCount > 0)
    return 1; // exists
  else
    return 0; // does not exist
}

Int64 CmpSeabaseDDL::getObjectUID(
                                   ExeCliInterface *cliInterface,
                                   const char * catName,
                                   const char * schName,
                                   const char * objName,
                                   const char * inObjType,
                                   char * outObjType)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  NAString quotedSchName;
  ToQuotedString(quotedSchName, NAString(schName), FALSE);
  NAString quotedObjName;
  ToQuotedString(quotedObjName, NAString(objName), FALSE);

  char buf[4000];
  if (inObjType)
    str_sprintf(buf, "select object_uid, object_type from %s.\"%s\".%s where catalog_name = '%s' and schema_name = '%s' and object_name = '%s'  and object_type = '%s' ",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
                catName, quotedSchName.data(), quotedObjName.data(),
                inObjType);
  else
    str_sprintf(buf, "select object_uid, object_type from %s.\"%s\".%s where catalog_name = '%s' and schema_name = '%s' and object_name = '%s' ",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
                catName, quotedSchName.data(), quotedObjName.data());
    
  cliRC = cliInterface->fetchRowsPrologue(buf, TRUE/*no exec*/);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  cliRC = cliInterface->clearExecFetchClose(NULL, 0);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  if (cliRC == 100) // did not find the row
    {
      *CmpCommon::diags() << DgSqlCode(-1389) << DgString0(objName);

      return -1;
    }

  char * ptr = NULL;
  Lng32 len = 0;
  cliInterface->getPtrAndLen(1, ptr, len);
  Int64 objUID = *(Int64*)ptr;

  if (outObjType)
    {
      cliInterface->getPtrAndLen(2, ptr, len);
      str_cpy_and_null(outObjType, ptr, len, '\0', ' ', TRUE);
    }

  cliInterface->fetchRowsEpilogue(NULL, TRUE);

  return objUID;
}

Int64 CmpSeabaseDDL::getObjectUIDandOwner(
                                   ExeCliInterface *cliInterface,
                                   const char * catName,
                                   const char * schName,
                                   const char * objName,
                                   const char * inObjType,
                                   char * outObjType,
                                   Int32 * objectOwner,
                                   NABoolean reportErrorNow )
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  NAString quotedSchName;
  ToQuotedString(quotedSchName, NAString(schName), FALSE);
  NAString quotedObjName;
  ToQuotedString(quotedObjName, NAString(objName), FALSE);

  char buf[4000];
  if (inObjType)
    str_sprintf(buf, "select object_uid, object_type, object_owner from %s.\"%s\".%s where catalog_name = '%s' and schema_name = '%s' and object_name = '%s'  and object_type = '%s' ",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
                catName, quotedSchName.data(), quotedObjName.data(),
                inObjType);
  else
    str_sprintf(buf, "select object_uid, object_type, object_owner from %s.\"%s\".%s where catalog_name = '%s' and schema_name = '%s' and object_name = '%s' ",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
                catName, quotedSchName.data(), quotedObjName.data());
    
  cliRC = cliInterface->fetchRowsPrologue(buf, TRUE/*no exec*/);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  cliRC = cliInterface->clearExecFetchClose(NULL, 0);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  if (cliRC == 100) // did not find the row
    {
      if ( reportErrorNow )
         *CmpCommon::diags() << DgSqlCode(-1389) << DgString0(objName);

      return -1;
    }

  char * ptr = NULL;
  Lng32 len = 0;
  cliInterface->getPtrAndLen(1, ptr, len);
  Int64 objUID = *(Int64*)ptr;

  if (outObjType)
    {
      cliInterface->getPtrAndLen(2, ptr, len);
      str_cpy_and_null(outObjType, ptr, len, '\0', ' ', TRUE);
    }

  if (objectOwner)
    {
      cliInterface->getPtrAndLen(3, ptr, len);
      *objectOwner = *(Int32*)ptr;
    }

  cliInterface->fetchRowsEpilogue(NULL, TRUE);

  return objUID;
}


short CmpSeabaseDDL::getObjectOwner(ExeCliInterface *cliInterface,
                     const char * catName,
                     const char * schName,
                     const char * objName,
                     const char * objType,
                     Int32 * objectOwner)
{
  Int32 retcode = 0;
  Int32 cliRC = 0;

  NAString stmt;

  stmt = "select object_owner from ";
  stmt += getSystemCatalog();
  stmt += ".\"";
  stmt += SEABASE_MD_SCHEMA;
  stmt += "\".";
  stmt += SEABASE_OBJECTS;
  stmt += " where catalog_name = '";
  stmt += catName;
  stmt += "' and schema_name = '";
  stmt += schName;  
  stmt += "' and object_name = '";
  stmt += objName ;
  stmt += "' and object_type = '";
  stmt += objType;
  stmt += "' for read uncommitted access"; 

  cliRC = cliInterface->fetchRowsPrologue(stmt.data(), TRUE/*no exec*/);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  cliRC = cliInterface->clearExecFetchClose(NULL, 0);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  if (cliRC == 100) // did not find the row
    {
      NAString strObjName (catName);
      strObjName += '.';
      strObjName += schName;
      strObjName += '.';
      strObjName += objName;    
      *CmpCommon::diags() << DgSqlCode(-1389)
                          << DgString0(strObjName.data());
      return -1;
    }

  char * ptr = NULL;
  Lng32 len = 0;
  cliInterface->getPtrAndLen(1, ptr, len);
  *objectOwner = *(Int32*)ptr;

  cliInterface->fetchRowsEpilogue(NULL, TRUE);

  return 0;
}

Int64 CmpSeabaseDDL::getConstraintOnIndex(
                                          ExeCliInterface *cliInterface,
                                          Int64 btUID,
                                          Int64 indexUID,
                                          const char * constrType,
                                          NAString &catName,
                                          NAString &schName,
                                          NAString &objName)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  char buf[4000];

  str_sprintf(buf, "select C.constraint_uid, O.catalog_name, O.schema_name, O.object_name from %s.\"%s\".%s C, %s.\"%s\".%s O where C.table_uid = %Ld and C.index_uid = %Ld and C.constraint_type = '%s' and C.constraint_uid = O.object_uid",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLE_CONSTRAINTS,
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              btUID, indexUID, constrType);

  Queue * constrsQueue = NULL;
  cliRC = cliInterface->fetchAllRows(constrsQueue, buf, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return cliRC;
    }
 
  if (constrsQueue->numEntries() == 0)
    return 0;

  constrsQueue->position();
  OutputInfo * vi = (OutputInfo*)constrsQueue->getNext(); 
  
  Int64 constrUID = *(Int64*)vi->get(0);
  catName = vi->get(1);
  schName = vi->get(2);
  objName = vi->get(3);

  return constrUID;
}

short CmpSeabaseDDL::getUsingObject(ExeCliInterface *cliInterface,
                                     Int64 objUID,
                                     NAString &usingObjName)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  char buf[4000];
  str_sprintf(buf, "select trim(catalog_name) || '.' || trim(schema_name) || '.' || trim(object_name) from %s.\"%s\".%s T, %s.\"%s\".%s VU where VU.used_object_uid = %Ld and T.object_uid = VU.using_view_uid  and T.valid_def = 'Y' ",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_VIEWS_USAGE,
              objUID);

  Queue * usingViewsQueue = NULL;
  cliRC = cliInterface->fetchAllRows(usingViewsQueue, buf, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return cliRC;
    }
 
  if (usingViewsQueue->numEntries() == 0)
    return 100;

  usingViewsQueue->position();
  OutputInfo * vi = (OutputInfo*)usingViewsQueue->getNext(); 
  
  char * viewName = vi->get(0);

  usingObjName = viewName;

  return 0;
}

short CmpSeabaseDDL::getUsingRoutine(ExeCliInterface *cliInterface,
                                     Int64 objUID,
                                     NAString &usingObjName)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  char buf[4000];
  str_sprintf(buf, "select trim(catalog_name) || '.' || trim(schema_name) || '.' || trim(object_name) from %s.\"%s\".%s T, %s.\"%s\".%s LU where LU.using_library_uid = %Ld and T.object_uid = LU.used_udr_uid  and T.valid_def = 'Y' ",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_LIBRARIES_USAGE,
              objUID);

  Queue * usingRoutinesQueue = NULL;
  cliRC = cliInterface->fetchAllRows(usingRoutinesQueue, buf, 0, 
                                     FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return cliRC;
    }
 
  if (usingRoutinesQueue->numEntries() == 0)
    return 100;

  usingRoutinesQueue->position();
  OutputInfo * rou = (OutputInfo*)usingRoutinesQueue->getNext(); 
  
  char * routineName = rou->get(0);

  usingObjName = routineName;

  return 0;
}

short CmpSeabaseDDL::getBaseTable(ExeCliInterface *cliInterface,
                                  const NAString &indexCatName,
                                  const NAString &indexSchName,
                                  const NAString &indexObjName,
                                  NAString &btCatName,
                                  NAString &btSchName,
                                  NAString &btObjName,
                                  Int64 &btUID)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  char buf[4000];

  str_sprintf(buf, "select trim(O.catalog_name), trim(O.schema_name), trim(O.object_name), O.object_uid from %s.\"%s\".%s O where O.object_uid = (select I.base_table_uid from %s.\"%s\".%s I where I.index_uid = (select O2.object_uid from %s.\"%s\".%s O2 where O2.catalog_name = '%s' and O2.schema_name = '%s' and O2.object_name = '%s' and O2.object_type = 'IX')) ",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_INDEXES,
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              (char*)indexCatName.data(), (char*)indexSchName.data(),
              (char*)indexObjName.data());

  Queue * usingTableQueue = NULL;
  cliRC = cliInterface->fetchAllRows(usingTableQueue, buf, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return cliRC;
    }
 
  if (usingTableQueue->numEntries() != 1)
    {
      // error
     *CmpCommon::diags() << DgSqlCode(-4082);
 
      return -1;
    }

  usingTableQueue->position();
  OutputInfo * vi = (OutputInfo*)usingTableQueue->getNext(); 
  
  btCatName = vi->get(0);
  btSchName = vi->get(1);
  btObjName = vi->get(2);
  btUID         = *(Int64*)vi->get(3);

  return 0;
}

short CmpSeabaseDDL::getUsingViews(ExeCliInterface *cliInterface,
                                    const NAString &usedObjName,
                                    NABoolean isTable,
                                    Queue * &usingViewsQueue)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  char buf[4000];
  char tableOrView[20];
  if (isTable)
    strcpy(tableOrView, "table");
  else
    strcpy(tableOrView, "view");

  str_sprintf(buf, "select trim(a) from (get all views on %s %s, no header, return full names) x(a) ",
              tableOrView,
              (char*)usedObjName.data());

  cliRC = cliInterface->fetchAllRows(usingViewsQueue, buf, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return cliRC;
    }
  
  return 0;
}

// Convert from hbase options string format to list of structs.
// String format:
//    HBASE_OPTIONS=>0002COMPRESSION='GZ'|BLOCKCACHE='10'|
//    
short CmpSeabaseDDL::genHbaseCreateOptions(
                                           const char * hbaseCreateOptionsStr,
                                           NAList<HbaseCreateOption*>* &hbaseCreateOptions,
                                           NAMemory * heap)
{
  hbaseCreateOptions = NULL;

  if (hbaseCreateOptionsStr == NULL)
    return 0;

  const char * hboStr = strstr(hbaseCreateOptionsStr, "HBASE_OPTIONS=>");
  if (! hboStr)
    return 0;

  char  numHBOstr[5];
  const char * startNumHBO = hboStr + strlen("HBASE_OPTIONS=>");
  memcpy(numHBOstr, startNumHBO, 4);
  numHBOstr[4] = 0;
  
  Lng32 numHBO = str_atoi(numHBOstr, 4);
  if (numHBO == 0)
    return 0;

  hbaseCreateOptions = new(heap) NAList<HbaseCreateOption*>;

  const char * optionStart = startNumHBO + 4;
  
  for (Lng32 i = 0; i < numHBO; i++)
    {
      // look for pattern    ='   to find the end of current option.
      const char * optionEnd = strstr(optionStart, "='");
      if (! optionEnd) // this is an error
        {
          delete hbaseCreateOptions;
          return -1;
        }

      const char * valStart = optionEnd + strlen("='");
      const char * valEnd = strstr(valStart, "'|");
      if (! valEnd) // this is an error
        {
          delete hbaseCreateOptions;
          return -1;
        }

      NAText key(optionStart, (optionEnd-optionStart));
      NAText val(valStart, (valEnd - valStart));

      HbaseCreateOption * hco = new(heap) HbaseCreateOption(key, val);

      hbaseCreateOptions->insert(hco);

      optionStart = valEnd + strlen("'|");
    }

  return 0;
}

short CmpSeabaseDDL::updateSeabaseMDTable(
                                         ExeCliInterface *cliInterface,
                                         const char * catName,
                                         const char * schName,
                                         const char * objName,
                                         const char * objType,
                                         const char * validDef, 
                                         ComTdbVirtTableTableInfo * tableInfo,
                                         Lng32 numCols,
                                         const ComTdbVirtTableColumnInfo * colInfo,
                                         Lng32 numKeys,
                                         const ComTdbVirtTableKeyInfo * keyInfo,
                                         Lng32 numIndexes,
                                         const ComTdbVirtTableIndexInfo * indexInfo,
                                         Int32 objOwnerID,
                                         Int64 &inUID)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  char buf[4000];

  Int64 objUID = 0;
  if (inUID < 0)
    {
      ComUID comUID;
      comUID.make_UID();
      objUID = comUID.get_value();
    }
  else
    objUID = inUID;

  // return the generated objUID
  inUID = objUID;
  
  Int64 createTime = NA_JulianTimestamp();

  NAString quotedSchName;
  ToQuotedString(quotedSchName, NAString(schName), FALSE);
  NAString quotedObjName;
  ToQuotedString(quotedObjName, NAString(objName), FALSE);

  str_sprintf(buf, "insert into %s.\"%s\".%s values ('%s', '%s', '%s', '%s', %Ld, %Ld, %Ld, '%s', %d )",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              catName, quotedSchName.data(), quotedObjName.data(),
              objType,
              objUID,
              createTime, 
              createTime,
              validDef,
              objOwnerID);
  cliRC = cliInterface->executeImmediate(buf);
  
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  if ((strcmp(objType, COM_BASE_TABLE_OBJECT_LIT) == 0) ||
      (strcmp(objType, COM_INDEX_OBJECT_LIT) == 0))
    {
      Lng32 isAudited = 1;
      const char * hbaseCreateOptions = NULL;
      if (tableInfo)
        {
          isAudited = tableInfo->isAudited;
          hbaseCreateOptions = tableInfo->hbaseCreateOptions;
        }


      str_sprintf(buf, "insert into %s.\"%s\".%s values (%Ld, '%s', '%s') ",
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLES,
                  objUID, (isAudited ? "Y" : "N"),
                  (hbaseCreateOptions ? hbaseCreateOptions : ""));
      cliRC = cliInterface->executeImmediate(buf);
      if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
          return -1;
        }
      
    } // BT

  for (Lng32 i = 0; i < numCols; i++)
    {
      NAString quotedColHeading;
      if (colInfo->colHeading)
        {
          ToQuotedString(quotedColHeading, colInfo->colHeading, FALSE);
        }

      str_sprintf(buf, "insert into %s.\"%s\".%s values (%Ld, '%s', %d, '%s', %d, %d, %d, %d, %d, %d, '%s', %d, %d, '%s', %d, %s'%s', '%s', '%s', '%s', '%s', '%s')",
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_COLUMNS,
                  objUID,
                  colInfo->colName, 
                  colInfo->colNumber,
                  colInfo->columnClass,
                  colInfo->datatype, 
                  colInfo->length,
                  colInfo->precision, 
                  colInfo->scale, 
                  colInfo->dtStart, 
                  colInfo->dtEnd,
                  (colInfo->upshifted ? "Y" : "N"),
                  colInfo->hbaseColFlags,
                  colInfo->nullable,
                  CharInfo::getCharSetName((CharInfo::CharSet)colInfo->charset),
                  (Lng32)colInfo->defaultClass,
                  colInfo->defVal ?
                  (colInfo->defaultClass == COM_USER_DEFINED_DEFAULT) ? "_UCS2 X" : "_UCS2" : " ",
                  colInfo->defVal ? colInfo->defVal : " ",
                  (colInfo->colHeading ? quotedColHeading.data() : ""),
                  colInfo->hbaseColFam ? colInfo->hbaseColFam : "" , 
                  colInfo->hbaseColQual ? colInfo->hbaseColQual : "",
                  colInfo->paramDirection,
                  colInfo->isOptional ? "Y" : "N");

      cliRC = cliInterface->executeImmediate(buf);
      if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

          return -1;
        }

      colInfo += 1;
    }

  for (Lng32 i = 0; i < numKeys; i++)
    {
      str_sprintf(buf, "insert into %s.\"%s\".%s values (%Ld, '%s', %d, %d, %d, %d)",
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_KEYS,
                  objUID,
                  keyInfo->colName, 
                  keyInfo->keySeqNum,
                  keyInfo->tableColNum,
                  keyInfo->ordering,
                  keyInfo->nonKeyCol);
      
      cliRC = cliInterface->executeImmediate(buf);
      if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

          return -1;
        }
      
      keyInfo += 1;
    }

  if ((strcmp(objType, COM_INDEX_OBJECT_LIT) == 0) &&
      (numIndexes > 0))
    {
      // this is an index, update the INDEXES table
      ComObjectName baseTableName(indexInfo->baseTableName);
      const NAString catalogNamePart = 
        baseTableName.getCatalogNamePartAsAnsiString();
      const NAString schemaNamePart = 
        baseTableName.getSchemaNamePartAsAnsiString(TRUE);
      const NAString objectNamePart = 
        baseTableName.getObjectNamePartAsAnsiString(TRUE);

      Int64 baseTableUID = 
        getObjectUID(cliInterface,
                     catalogNamePart.data(), schemaNamePart.data(), objectNamePart.data(),
                     COM_BASE_TABLE_OBJECT_LIT);
                        
      str_sprintf(buf, "insert into %s.\"%s\".%s values (%Ld, %d, %d, %d, %d, %d, %Ld) ",
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_INDEXES,
                  baseTableUID,
                  indexInfo->keytag,
                  indexInfo->isUnique,
                  indexInfo->keyColCount,
                  indexInfo->nonKeyColCount,
                  indexInfo->isExplicit, 
                  objUID);
      cliRC = cliInterface->executeImmediate(buf);
      if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

          return -1;
        }
    } // is an index


   // Grant owner privileges
  if (isAuthorizationEnabled())
    {
      NAString fullName (catName);
      fullName += ".";
      fullName += schName;
      fullName += ".";
      fullName += objName;
      if (!insertPrivMgrInfo(objUID,
                             fullName,
                             objType,
                             objOwnerID))
        {
          *CmpCommon::diags()
            << DgSqlCode(-CAT_UNABLE_TO_GRANT_PRIVILEGES)
            << DgTableName(objName);
          return -1;
        }

    }
  return 0;
}
short CmpSeabaseDDL::updateSeabaseMDSPJ(
                                        ExeCliInterface *cliInterface,
                                        const char * catName,
                                        const char * schName,
                                        const char * libName,
                                        const char * libPath,
                                        const ComTdbVirtTableRoutineInfo * routineInfo,
                                        Lng32 numCols,
                                        const ComTdbVirtTableColumnInfo * colInfo)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  char buf[4000];

  ComUID libUID;
  libUID.make_UID();
  Int64 libObjUID = libUID.get_value();
  
  Int64 createTime = NA_JulianTimestamp();

  NAString quotedSchName;
  ToQuotedString(quotedSchName, NAString(schName), FALSE);
  NAString quotedLibObjName;
  ToQuotedString(quotedLibObjName, NAString(libName), FALSE);

  str_sprintf(buf, "insert into %s.\"%s\".%s values ('%s', '%s', '%s', '%s', %Ld, %Ld, %Ld, '%s', %d )",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              catName, quotedSchName.data(), quotedLibObjName.data(),
              COM_LIBRARY_OBJECT_LIT,
              libObjUID,
              createTime, 
              createTime,
              "Y",
              SUPER_USER);
  cliRC = cliInterface->executeImmediate(buf);
  
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  str_sprintf(buf, "insert into %s.\"%s\".%s values (%Ld, '%s', %d)",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_LIBRARIES,
              libObjUID, libPath, routineInfo->library_version);
  
  cliRC = cliInterface->executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  NAString catalogNamePart(getSystemCatalog());
  NAString schemaNamePart;
  ToQuotedString(schemaNamePart, NAString(SEABASE_MD_SCHEMA), FALSE);
  NAString quotedSpjObjName;
  ToQuotedString(quotedSpjObjName, NAString(routineInfo->routine_name), FALSE);
  Int64 objUID = -1;
  if (updateSeabaseMDTable(cliInterface, 
                           catalogNamePart, schemaNamePart, quotedSpjObjName,
                           COM_USER_DEFINED_ROUTINE_OBJECT_LIT,
                           "Y",
                           NULL,
                           numCols,
                           colInfo,
                           0, NULL,
                           0, NULL, 
                           SUPER_USER,
                           objUID))
    {
      return -1;
    }

  Int64 spjObjUID = getObjectUID(cliInterface, 
                           catalogNamePart, schemaNamePart, quotedSpjObjName,
                           COM_USER_DEFINED_ROUTINE_OBJECT_LIT);
  if (spjObjUID == -1)
    return -1;
                                 

  str_sprintf(buf, "insert into %s.\"%s\".%s values (%Ld, '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', %d, %d, '%s', '%s', '%s', '%s', '%s', %Ld, '%s' )",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_ROUTINES,
              spjObjUID,
              routineInfo->UDR_type,
              routineInfo->language_type,
              routineInfo->deterministic ? "Y" : "N" ,
              routineInfo->sql_access,
              routineInfo->call_on_null ? "Y" : "N" ,
              routineInfo->isolate ? "Y" : "N" ,
              routineInfo->param_style,
              routineInfo->transaction_attributes,
              routineInfo->max_results,
              routineInfo->state_area_size,
              routineInfo->external_name,
              routineInfo->parallelism,
              routineInfo->user_version,
              routineInfo->external_security,
              routineInfo->execution_mode,
              libObjUID,
              routineInfo->signature);

  cliRC = cliInterface->executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  str_sprintf(buf, "insert into %s.\"%s\".%s values (%Ld, %Ld)",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_LIBRARIES_USAGE,
              libObjUID, spjObjUID);

  cliRC = cliInterface->executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  return 0;

}


short CmpSeabaseDDL::deleteFromSeabaseMDTable(
                                              ExeCliInterface *cliInterface,
                                              const char * catName,
                                              const char * schName,
                                              const char * objName,
                                              const char * objType)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  char buf[4000];

  NAString quotedSchName;
  ToQuotedString(quotedSchName, NAString(schName), FALSE);
  NAString quotedObjName;
  ToQuotedString(quotedObjName, NAString(objName), FALSE);

  Int64 objUID = getObjectUID(cliInterface, catName, schName, objName, objType);

  if (objUID < 0)
     return -1;

  str_sprintf(buf, "delete from %s.\"%s\".%s where catalog_name = '%s' and schema_name = '%s' and object_name = '%s' and object_type = '%s' ",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              catName, quotedSchName.data(), quotedObjName.data(), objType);
  cliRC = cliInterface->executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

      return -1;
    }

  if (strcmp(objType, COM_LIBRARY_OBJECT_LIT) == 0) 
    {
      str_sprintf(buf, "delete from %s.\"%s\".%s where library_uid = %Ld",
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_LIBRARIES,
                  objUID);
      cliRC = cliInterface->executeImmediate(buf);
      if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
          return -1;
        }
      return 0; // nothing else to do for libraries
    }
  
  if (strcmp(objType, COM_SEQUENCE_GENERATOR_OBJECT_LIT) == 0) 
    {
      str_sprintf(buf, "delete from %s.\"%s\".%s where seq_uid = %Ld",
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_SEQ_GEN,
                  objUID);
      cliRC = cliInterface->executeImmediate(buf);
      if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
          return -1;
        }
    }
  
  str_sprintf(buf, "delete from %s.\"%s\".%s where object_uid = %Ld",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_COLUMNS,
              objUID);
  cliRC = cliInterface->executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

      return -1;
    }

  if (strcmp(objType, COM_USER_DEFINED_ROUTINE_OBJECT_LIT) == 0)
  {
    str_sprintf(buf, "delete from %s.\"%s\".%s where udr_uid = %Ld",
                getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_ROUTINES,
                objUID);
    cliRC = cliInterface->executeImmediate(buf);
    if (cliRC < 0)
      {
        cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
        return -1;
      }
    str_sprintf(buf, "delete from %s.\"%s\".%s where used_udr_uid = %Ld",
                getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_LIBRARIES_USAGE,
                objUID);
    cliRC = cliInterface->executeImmediate(buf);
    if (cliRC < 0)
      {
        cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
        return -1;
      }
    return 0;  // nothing else to do for routines
  }

  str_sprintf(buf, "delete from %s.\"%s\".%s where object_uid = %Ld ",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_KEYS,
              objUID);
  cliRC = cliInterface->executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

      return -1;
    }

  str_sprintf(buf, "delete from %s.\"%s\".%s where table_uid = %Ld ",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLES,
              objUID);
  cliRC = cliInterface->executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

      return -1;
    }

  if (strcmp(objType, COM_INDEX_OBJECT_LIT) == 0)
    {
      str_sprintf(buf, "delete from %s.\"%s\".%s where index_uid = %Ld ",
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_INDEXES,
                  objUID);
      cliRC = cliInterface->executeImmediate(buf);
      if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

          return -1;
        }
    }

  if (strcmp(objType, COM_VIEW_OBJECT_LIT) == 0)
    {
      str_sprintf(buf, "delete from %s.\"%s\".%s where view_uid  = %Ld ",
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_VIEWS,
                  objUID);
      cliRC = cliInterface->executeImmediate(buf);
      if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

          return -1;
        }

      str_sprintf(buf, "delete from %s.\"%s\".%s where using_view_uid  = %Ld ",
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_VIEWS_USAGE,
                  objUID);
      cliRC = cliInterface->executeImmediate(buf);
      if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

          return -1;
        }

      // delete data from TEXT table
      str_sprintf(buf, "delete from %s.\"%s\".%s where object_uid = %Ld",
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TEXT,
                  objUID);
      cliRC = cliInterface->executeImmediate(buf);
      if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
          
          return -1;
        }
    }

   return 0;
}

short CmpSeabaseDDL::deleteConstraintInfoFromSeabaseMDTables(
                                              ExeCliInterface *cliInterface,
                                              Int64 tableUID,
                                              Int64 otherTableUID, // valid for ref constrs
                                              Int64 constrUID,
                                              Int64 otherConstrUID, // valid for ref constrs
                                              const char * constrCatName,
                                              const char * constrSchName,
                                              const char * constrObjName,
                                              const char * constrType)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  char buf[4000];

  if (deleteFromSeabaseMDTable(cliInterface, 
                               constrCatName, constrSchName, constrObjName, 
                               constrType))
    return -1;
  
  // delete data from table constraints MD
  str_sprintf(buf, "delete from %s.\"%s\".%s where table_uid  = %Ld and constraint_uid = %Ld",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLE_CONSTRAINTS,
              tableUID, constrUID);
  cliRC = cliInterface->executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      
      return -1;
    }
  
  // delete data from ref constraints MD
  str_sprintf(buf, "delete from %s.\"%s\".%s where ref_constraint_uid  = %Ld and unique_constraint_uid = %Ld",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_REF_CONSTRAINTS,
              constrUID, otherConstrUID);
  cliRC = cliInterface->executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      
      return -1;
    }

  // delete data from unique ref constraints usage MD
  str_sprintf(buf, "delete from %s.\"%s\".%s where unique_constraint_uid = %Ld and foreign_constraint_uid = %Ld",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_UNIQUE_REF_CONSTR_USAGE,
              otherConstrUID, constrUID);
  cliRC = cliInterface->executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      
      return -1;
    }

  // delete data from TEXT table
  str_sprintf(buf, "delete from %s.\"%s\".%s where object_uid = %Ld",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TEXT,
              constrUID);
  cliRC = cliInterface->executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      
      return -1;
    }
  
  return 0;
}

short CmpSeabaseDDL::updateObjectValidDef(
                                           ExeCliInterface *cliInterface,
                                           const char * catName,
                                           const char * schName,
                                           const char * objName,
                                           const char * objType,
                                           const char * validDef)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;
  
  char buf[4000];
  
  NAString quotedSchName;
  ToQuotedString(quotedSchName, NAString(schName), FALSE);
  NAString quotedObjName;
  ToQuotedString(quotedObjName, NAString(objName), FALSE);

  str_sprintf(buf, "update %s.\"%s\".%s set valid_def = '%s' where catalog_name = '%s' and schema_name = '%s' and object_name = '%s' and object_type = '%s' ",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              validDef,
              catName, quotedSchName.data(), quotedObjName.data(),
              objType);
  cliRC = cliInterface->executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

      return -1;
    }

  return 0;
}

short CmpSeabaseDDL::updateObjectName(
                                           ExeCliInterface *cliInterface,
                                           Int64 objUID,
                                           const char * catName,
                                           const char * schName,
                                           const char * objName)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;
  
  char buf[4000];
  
  NAString quotedSchName;
  ToQuotedString(quotedSchName, NAString(schName), FALSE);
  NAString quotedObjName;
  ToQuotedString(quotedObjName, NAString(objName), FALSE);

  str_sprintf(buf, "update %s.\"%s\".%s set catalog_name = '%s', schema_name = '%s', object_name = '%s' where object_uid = %Ld ",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              catName, quotedSchName.data(), quotedObjName.data(),
              objUID);
  cliRC = cliInterface->executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

      return -1;
    }

  return 0;
}

short CmpSeabaseDDL::updateObjectAuditAttr(
                                           ExeCliInterface *cliInterface,
                                           const char * catName,
                                           const char * schName,
                                           const char * objName,
                                           NABoolean audited,
                                           const NAString &objType)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;
  
  char buf[4000];
  
  Int64 objectUID = getObjectUID(cliInterface, catName, schName, objName, objType);
  
  if (objectUID < 0)
     return -1;
  str_sprintf(buf, "update %s.\"%s\".%s set is_audited = '%s' where  table_uid = %Ld ",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLES,
              (audited ? "Y" : "N"),
              objectUID);
  cliRC = cliInterface->executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());

      return -1;
    }

  return 0;
}

short CmpSeabaseDDL::buildColInfoArray(
                                       ElemDDLColDefArray *colArray,
                                       ComTdbVirtTableColumnInfo * colInfoArray,
                                       NABoolean implicitPK,
                                       CollIndex numSysCols,
                                       NAMemory * heap)
{
  size_t index = 0;
  for (index = 0; index < colArray->entries(); index++)
    {
      ElemDDLColDef *colNode = (*colArray)[index];
      
      NAString colName;
      Lng32 datatype, length, precision, scale, dt_start, dt_end, nullable, upshifted;
      ComColumnDefaultClass defaultClass;
      NAString charset, defVal;
      NAString heading;
      ULng32 colFlags;
      LobsStorage lobStorage;
      if (getColInfo(colNode,
                     colName,
                     datatype, length, precision, scale, dt_start, dt_end, upshifted, nullable,
                     charset, defaultClass, defVal, heading, lobStorage, colFlags))
        return -1;

      colInfoArray[index].hbaseColFlags = colFlags;
      
      char * col_name = new((heap ? heap : STMTHEAP)) char[colName.length() + 1];
      strcpy(col_name, (char*)colName.data());

      colInfoArray[index].colName = col_name;
      colInfoArray[index].colNumber = index;
      if (index < numSysCols)
        {
          strcpy(colInfoArray[index].columnClass, COM_SYSTEM_COLUMN_LIT);
          // a system column with a default clause is a computed column
          // (salt column)
          if (defaultClass == COM_USER_DEFINED_DEFAULT)
            defaultClass = COM_ALWAYS_COMPUTE_COMPUTED_COLUMN_DEFAULT;
        }
      else
        strcpy(colInfoArray[index].columnClass, COM_USER_COLUMN_LIT);
      
      colInfoArray[index].datatype = datatype;
      colInfoArray[index].length = length;
      colInfoArray[index].nullable = nullable;
      colInfoArray[index].charset = (SQLCHARSET_CODE)CharInfo::getCharSetEnum(charset); 
      
      colInfoArray[index].precision = precision;
      colInfoArray[index].scale = scale;
      colInfoArray[index].dtStart = dt_start;
      colInfoArray[index].dtEnd = dt_end;
      colInfoArray[index].upshifted = upshifted;
      colInfoArray[index].defaultClass = defaultClass;
      colInfoArray[index].defVal = NULL;

      if (defVal.length() > 0)
        {
          char * def_val = new((heap ? heap : STMTHEAP)) char[defVal.length() +1];
          str_cpy_all(def_val, (char*)defVal.data(), defVal.length());
          def_val[defVal.length()] = 0;
          colInfoArray[index].defVal = def_val;
        }

      colInfoArray[index].colHeading = NULL;
      if (heading.length() > 0)
        {
          char * head_val = new((heap ? heap : STMTHEAP)) char[heading.length() +1];
          str_cpy_all(head_val, (char*)heading.data(), heading.length());
          head_val[heading.length()] = 0;
          colInfoArray[index].colHeading = head_val;
        }

      colInfoArray[index].hbaseColFam = 
        new((heap ? heap : STMTHEAP)) char[strlen(SEABASE_DEFAULT_COL_FAMILY) +1];
      strcpy((char*)colInfoArray[index].hbaseColFam, (char*)SEABASE_DEFAULT_COL_FAMILY);

      char idxNumStr[40];
      str_itoa(index+1, idxNumStr);

      colInfoArray[index].hbaseColQual =
        new((heap ? heap : STMTHEAP)) char[strlen(idxNumStr) + 1];
      strcpy((char*)colInfoArray[index].hbaseColQual, idxNumStr);

      strcpy(colInfoArray[index].paramDirection, COM_UNKNOWN_PARAM_DIRECTION_LIT);
      colInfoArray[index].isOptional = FALSE;
    }

  return 0;
}

short CmpSeabaseDDL::buildColInfoArray(
                                       ElemDDLParamDefArray *paramArray,
                                       ComTdbVirtTableColumnInfo * colInfoArray
                                       )
{
  size_t index = 0;
  for (index = 0; index < paramArray->entries(); index++)
    {
      ElemDDLParamDef *paramNode = (*paramArray)[index];
      ElemDDLColDef colNode(paramNode->getParamName(), 
                            paramNode->getParamDataType(),
                            NULL, NULL, STMTHEAP);
      NAString colName;
      Lng32 datatype, length, precision, scale, dt_start, dt_end, nullable, upshifted;
      ComColumnDefaultClass defaultClass;
      NAString charset, defVal;
      NAString heading;
      ULng32 colFlags;
      LobsStorage lobStorage;
      if (getColInfo(&colNode,
                     colName,
                     datatype, length, precision, scale, dt_start, dt_end, 
                     upshifted, nullable, charset, defaultClass, defVal, 
                     heading, lobStorage, colFlags))
        return -1;

      colInfoArray[index].hbaseColFlags = colFlags;
      
      char * col_name = NULL;
      if (colName.length() == 0) {
        char idxNumStr[10];
        idxNumStr[0] = '#' ;
        idxNumStr[1] = ':' ;
        str_itoa(index, &(idxNumStr[2]));
        col_name = new(STMTHEAP) char[strlen(idxNumStr) + 1];
        strcpy(col_name, idxNumStr);
      }
      else {
        col_name = new(STMTHEAP) char[colName.length() + 1];
        strcpy(col_name, (char*)colName.data());
      }

      colInfoArray[index].colName = col_name;
      colInfoArray[index].colNumber = index;
      strcpy(colInfoArray[index].columnClass, COM_USER_COLUMN_LIT);
      colInfoArray[index].datatype = datatype;
      colInfoArray[index].length = length;
      colInfoArray[index].nullable = nullable;
      colInfoArray[index].charset = (SQLCHARSET_CODE)
        CharInfo::getCharSetEnum(charset); 
      colInfoArray[index].precision = precision;
      colInfoArray[index].scale = scale;
      colInfoArray[index].dtStart = dt_start;
      colInfoArray[index].dtEnd = dt_end;
      colInfoArray[index].upshifted = upshifted;
      colInfoArray[index].defaultClass = defaultClass;
      colInfoArray[index].defVal = NULL;
      colInfoArray[index].colHeading = NULL;
      colInfoArray[index].hbaseColFam = NULL;
      colInfoArray[index].hbaseColQual = NULL;
      
      if (paramNode->getParamDirection() == COM_INPUT_PARAM)
        strcpy(colInfoArray[index].paramDirection, COM_INPUT_PARAM_LIT);
      else if (paramNode->getParamDirection() == COM_OUTPUT_PARAM)
        strcpy(colInfoArray[index].paramDirection, COM_OUTPUT_PARAM_LIT);
      else if (paramNode->getParamDirection() == COM_INOUT_PARAM)
        strcpy(colInfoArray[index].paramDirection, COM_INOUT_PARAM_LIT);
      else
        strcpy(colInfoArray[index].paramDirection, 
               COM_UNKNOWN_PARAM_DIRECTION_LIT);

      colInfoArray[index].isOptional = 0; // Aways FALSE for now
    }

  return 0;
}

short CmpSeabaseDDL::buildKeyInfoArray(
                                       ElemDDLColDefArray *colArray,
                                       ElemDDLColRefArray *keyArray,
                                       ComTdbVirtTableColumnInfo * colInfoArray,
                                       ComTdbVirtTableKeyInfo * keyInfoArray,
                                       NABoolean allowNullableUniqueConstr,
                                       NAMemory * heap)
{
  size_t index = 0;
  for ( index = 0; index < keyArray->entries(); index++)
    {
      char * col_name = new((heap ? heap : STMTHEAP)) 
        char[strlen((*keyArray)[index]->getColumnName()) + 1];
      strcpy(col_name, (*keyArray)[index]->getColumnName());

      keyInfoArray[index].colName = col_name; //(*keyArray)[index]->getColumnName();

      keyInfoArray[index].keySeqNum = index+1;
      keyInfoArray[index].tableColNum = (Lng32)
        colArray->getColumnIndex((*keyArray)[index]->getColumnName());

      if (keyInfoArray[index].tableColNum == -1)
        {
          // this col doesn't exist. Return error.
          *CmpCommon::diags() << DgSqlCode(-1009)
                              << DgColumnName(keyInfoArray[index].colName);
          
          return -1;
        }
        
      keyInfoArray[index].ordering = 
        ((*keyArray)[index]->getColumnOrdering() == COM_ASCENDING_ORDER ? 0 : 1);
      keyInfoArray[index].nonKeyCol = 0;

      if ((colInfoArray[keyInfoArray[index].tableColNum].nullable != 0) &&
          (NOT allowNullableUniqueConstr))
        {
          *CmpCommon::diags() << DgSqlCode(-CAT_CLUSTERING_KEY_COL_MUST_BE_NOT_NULL_NOT_DROP)
                              << DgColumnName(keyInfoArray[index].colName);
          
          return -1;
        }

      keyInfoArray[index].hbaseColFam = NULL;
      keyInfoArray[index].hbaseColQual = NULL;
    }

  return 0;
}

short CmpSeabaseDDL::updateTextTable(ExeCliInterface *cliInterface,
                                     Int64 objUID, NAString &text)
{
  Lng32 cliRC = 0;

  char * buf = new(STMTHEAP) char[400+TEXTLEN];
  Lng32 textLen = text.length();
  Lng32 numRows = (textLen / TEXTLEN) + 1;
  Lng32 currPos = 0;
  for (Lng32 i = 0; i < numRows; i++)
    {
      NAString temp = 
        (i < numRows-1 ? text(currPos, TEXTLEN)
         : text(currPos, (textLen - currPos)));

      str_sprintf(buf, "insert into %s.\"%s\".%s values (%Ld, %d, '%s')",
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TEXT,
                  objUID,
                  i,
                  temp.data());
      cliRC = cliInterface->executeImmediate(buf);
      
      if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
          return -1;
        }

      currPos += TEXTLEN;
    }

  return 0;
}

short CmpSeabaseDDL::createEncodedKeysBuffer(char** &encodedKeysBuffer,
                                             desc_struct * colDescs, desc_struct * keyDescs,
                                             Lng32 numSplits, Lng32 numKeys, Lng32 keyLength)
{
  encodedKeysBuffer = NULL;
  
  if (numSplits <= 0)
    return 0;

    NAString ** inArray = createInArrayForLowOrHighKeys(colDescs, 
                                                        keyDescs,
                                                        numKeys, 
                                                        FALSE, 
                                                        STMTHEAP ); 

    char splitNumCharStr[5];
    NAString splitNumString;

    /* HBase creates 1 more split than
       the number of rows in the split array. In the example below we have a 
       salt column and an integer column as the key. KeyLength is 4 + 4 = 8.
       encodedKeysBuffer will have 4 elements, each of length 8. When this
       buffer is given to HBase through the Java API we get a table with 5 
       splits and begin/end keys as shown below
       
       Start Key                                                     End Key
       \x00\x00\x00\x01\x00\x00\x00\x00
       \x00\x00\x00\x01\x00\x00\x00\x00      \x00\x00\x00\x02\x00\x00\x00\x00   
       \x00\x00\x00\x02\x00\x00\x00\x00      \x00\x00\x00\x03\x00\x00\x00\x00
       \x00\x00\x00\x03\x00\x00\x00\x00      \x00\x00\x00\x04\x00\x00\x00\x00   
       \x00\x00\x00\x04\x00\x00\x00\x00 
    */
    encodedKeysBuffer = new (STMTHEAP) char*[numSplits];
    for(int i =0; i < numSplits; i++)
      encodedKeysBuffer[i] = new (STMTHEAP) char[keyLength];

    inArray[0] = &splitNumString;
    short retVal = 0;
    
    for(Int32 i =0; i < numSplits; i++) {
      sprintf(splitNumCharStr, "%d", i+1);
      splitNumString = splitNumCharStr;
      retVal = encodeKeyValues(colDescs,
                               keyDescs,
                               inArray, // INPUT
                               encodedKeysBuffer[i],  // OUTPUT
                               STMTHEAP,
                               CmpCommon::diags());

      if (retVal)
        return -1;
    }

  return 0;
}

short CmpSeabaseDDL::dropSeabaseObject(ExpHbaseInterface * ehi,
                                       const NAString &objName,
                                       NAString &currCatName, NAString &currSchName,
                                       const char * objType,
                                       NABoolean dropFromMD,
                                       NABoolean dropFromHbase)
{
  Lng32 retcode = 0;

  ComObjectName tableName(objName);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);

  const NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);
  const NAString extNameForHbase = catalogNamePart + "." + schemaNamePart + "." + objectNamePart;

  ExeCliInterface cliInterface(STMTHEAP);

  if (dropFromMD)
    {
      // Revoke owner privileges for object
      //   it would be more efficient to pass in the object owner and UID
      //   that to do an extra I/O.
      Int32 objOwnerID = 0;
      Int64 objUID = getObjectUIDandOwner(&cliInterface,
                                          catalogNamePart.data(), schemaNamePart.data(),
                                          objectNamePart.data(), objType,
                                          NULL, &objOwnerID);

      if (objUID < 0 || objOwnerID ==0)
        {
          return -1;
        }

      if (isAuthorizationEnabled())
        {
          if (!deletePrivMgrInfo ( objectNamePart, objUID, NAString(objType) )) 
            {
              return -1;
            }
          }

      if (deleteFromSeabaseMDTable(&cliInterface, 
                                   catalogNamePart, schemaNamePart, objectNamePart, objType ))
        return -1;
    }

  if (dropFromHbase)
    {
      if (objType && (NAString(objType) != COM_VIEW_OBJECT_LIT))
        {
          HbaseStr hbaseTable;
          hbaseTable.val = (char*)extNameForHbase.data();
          hbaseTable.len = extNameForHbase.length();
          retcode = dropHbaseTable(ehi, &hbaseTable);
          if (retcode < 0)
            {
              return -1;
            }
        }
    }

  return 0;
}

short CmpSeabaseDDL::dropSeabaseStats(ExeCliInterface *cliInterface,
                                      const char * catName,
                                      const char * schName,
                                      Int64 tableUID)
{
  Lng32 cliRC = 0;
  char buf[4000];
  
  str_sprintf(buf, "delete from %s.\"%s\".%s where table_uid = %Ld",
              catName, schName, HBASE_HIST_NAME, tableUID);

  cliRC = cliInterface->executeImmediate(buf);
  // if histogram table does not exist, return now without error
  // (could return on cliRC == 100, but that seems to happen
  // even when we deleted some histogram rows)
  if (cliRC == -4082)
    return 0;

  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  str_sprintf(buf, "delete from %s.\"%s\".%s where table_uid = %Ld",
              catName, schName, HBASE_HISTINT_NAME, tableUID);
  cliRC = cliInterface->executeImmediate(buf);

  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  return 0;
}

short CmpSeabaseDDL::updateSeabaseVersions(
                                           ExeCliInterface *cliInterface,
                                           const char* sysCat,
                                           Lng32 majorVersion)
{
  Lng32 cliRC = 0;
  char buf[4000];
  
  str_sprintf(buf, "delete from %s.\"%s\".%s ",
              sysCat, SEABASE_MD_SCHEMA, SEABASE_VERSIONS);

  cliRC = cliInterface->executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  Int64 initTime = NA_JulianTimestamp();

  str_sprintf(buf, "insert into %s.\"%s\".%s values ('METADATA', %d, %d, %Ld, 'initialize trafodion'), ('DATAFORMAT', %d, %d, %Ld, 'initialize trafodion'), ('SOFTWARE', %d, %d, %Ld, 'initialize trafodion') ",
              sysCat, SEABASE_MD_SCHEMA, SEABASE_VERSIONS,
              (majorVersion != -1 ? majorVersion : METADATA_MAJOR_VERSION),
              METADATA_MINOR_VERSION, initTime,
              DATAFORMAT_MAJOR_VERSION, DATAFORMAT_MINOR_VERSION, initTime,
              SOFTWARE_MAJOR_VERSION, SOFTWARE_MINOR_VERSION, initTime);
  cliRC = cliInterface->executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  return 0;
}

short CmpSeabaseDDL::updateSeabaseAuths(
                                         ExeCliInterface *cliInterface,
                                         const char* sysCat)
{
  Lng32 cliRC = 0;
  char buf[4000];

  str_sprintf(buf, "delete from %s.\"%s\".%s ",
              sysCat, SEABASE_MD_SCHEMA, SEABASE_AUTHS);

  cliRC = cliInterface->executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  Int64 initTime = NA_JulianTimestamp();

  str_sprintf(buf, "insert into %s.\"%s\".%s values (%d, 'DB__ROOT', 'TRAFODION', 'U', %d, 'Y', %d,%d) ",
              sysCat, SEABASE_MD_SCHEMA, SEABASE_AUTHS,
              SUPER_USER, SUPER_USER, initTime, initTime);
  cliRC = cliInterface->executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  return 0;
}

void CmpSeabaseDDL::initSeabaseMD()
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;
  NABoolean xnWasStartedHere = FALSE;

  Lng32 numTables = sizeof(allMDtablesInfo) / sizeof(MDTableInfo);

  Queue * tempQueue = NULL;

  // create metadata tables in hbase
  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    return;

  ExeCliInterface cliInterface(STMTHEAP);

  const char* sysCat = ActiveSchemaDB()->getDefaults().getValue(SEABASE_CATALOG);

  Lng32 hbaseErrNum = 0;
  NAString hbaseErrStr;
  Lng32 errNum = validateVersions(&ActiveSchemaDB()->getDefaults(), ehi,
                                  NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
                                  &hbaseErrNum, &hbaseErrStr);
  if (errNum != 0)
    {
      CmpCommon::context()->setIsUninitializedSeabase(TRUE);
      CmpCommon::context()->uninitializedSeabaseErrNum() = errNum;
      CmpCommon::context()->hbaseErrNum() = hbaseErrNum;
      CmpCommon::context()->hbaseErrStr() = hbaseErrStr;

      if (errNum != -1393) // 1393: metadata is not initialized
        {
          if (errNum == -1398)
            *CmpCommon::diags() << DgSqlCode(errNum)
                                << DgInt0(hbaseErrNum)
                                << DgString0(hbaseErrStr);
          else
            *CmpCommon::diags() << DgSqlCode(errNum);

          deallocEHI(ehi); 
          return;
        }
    }
  else
    {
      CmpCommon::context()->setIsUninitializedSeabase(FALSE);

      *CmpCommon::diags() << DgSqlCode(-1392);

      deallocEHI(ehi); 
      return;
    }

  // create hbase physical objects
  for (Lng32 i = 0; i < numTables; i++)
    {
      const MDTableInfo &mdti = allMDtablesInfo[i];

      HbaseStr hbaseObject;
      NAString hbaseObjectStr(sysCat);
      hbaseObjectStr += ".";
      hbaseObjectStr += SEABASE_MD_SCHEMA;
      hbaseObjectStr += ".";
      hbaseObjectStr += mdti.newName;
      hbaseObject.val = (char*)hbaseObjectStr.data();
      hbaseObject.len = hbaseObjectStr.length();
      if (createHbaseTable(ehi, &hbaseObject, SEABASE_DEFAULT_COL_FAMILY, NULL, NULL) == -1)
        {
          deallocEHI(ehi); 
          return;
        }

    } // for
 
  // cleanup cached entries in client object.
  ehi->cleanupClient();

  deallocEHI(ehi); 
  ehi = NULL;

  NAString installJar(getenv("MY_SQROOT"));
  installJar += "/export/lib/trafodion-UDR-0.7.0.jar";

  if (NOT xnInProgress(&cliInterface))
    {
      cliRC = cliInterface.beginXn();
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
          goto label_error;
        }

      xnWasStartedHere = TRUE;
    }

  // update MD with information about metadata objects
  for (Lng32 i = 0; i < numTables; i++)
    {
      const MDTableInfo &mdti = allMDtablesInfo[i];
      MDDescsInfo &mddi = CmpCommon::context()->getTrafMDDescsInfo()[i];

      if (mdti.isIndex)
        continue;

      Int64 objUID = -1;
      if (updateSeabaseMDTable(&cliInterface, 
                               sysCat, SEABASE_MD_SCHEMA, mdti.newName,
                               COM_BASE_TABLE_OBJECT_LIT,
                               "Y",
                               NULL,
                               mddi.numNewCols,
                               mddi.newColInfo,
                               mddi.numNewKeys,
                               mddi.newKeyInfo,
                               mddi.numIndexes,
                               mddi.indexInfo,
                               SUPER_USER,
                               objUID))
        {
          goto label_error;
        }

    } // for

  // update metadata with metadata indexes information
  ComTdbVirtTableTableInfo tableInfo;
  tableInfo.tableName = NULL,
  tableInfo.createTime = 0;
  tableInfo.redefTime = 0;
  tableInfo.objUID = 0;
  tableInfo.isAudited = 1;
  tableInfo.validDef = 0;
  tableInfo.hbaseCreateOptions = NULL;
  tableInfo.objOwnerID = SUPER_USER;

  for (Lng32 i = 0; i < numTables; i++)
    {
      const MDTableInfo &mdti = allMDtablesInfo[i];
      MDDescsInfo &mddi = CmpCommon::context()->getTrafMDDescsInfo()[i];

      if (NOT mdti.isIndex)
        continue;

      Int64 objUID = -1;
      if (updateSeabaseMDTable(&cliInterface, 
                               sysCat, SEABASE_MD_SCHEMA, mdti.newName,
                               COM_INDEX_OBJECT_LIT,
                               "Y",
                               &tableInfo,
                               mddi.numNewCols,
                               mddi.newColInfo,
                               mddi.numNewKeys,
                               mddi.newKeyInfo,
                               0, NULL,
                               SUPER_USER,
                               objUID))
                               //                              1, // numIndex
                               //                              seabaseMDObjectsUniqIdxIndexInfo))
        {
          goto label_error;
        }
    } // for

  // update SPJ info
  if (updateSeabaseMDSPJ(&cliInterface, sysCat, SEABASE_MD_SCHEMA, 
                         SEABASE_VALIDATE_LIBRARY,
                         installJar.data(),
                         &seabaseMDValidateRoutineInfo,
                         sizeof(seabaseMDValidateRoutineColInfo) / sizeof(ComTdbVirtTableColumnInfo),
                         seabaseMDValidateRoutineColInfo))
    {
      goto label_error;
    }

  updateSeabaseVersions(&cliInterface, sysCat);
  updateSeabaseAuths(&cliInterface, sysCat);

  if (xnWasStartedHere)
    {
      cliRC = cliInterface.commitXn();
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
          return;
        }
      xnWasStartedHere = FALSE;
    }

  CmpCommon::context()->setIsUninitializedSeabase(FALSE);
  CmpCommon::context()->uninitializedSeabaseErrNum() = 0;

  if (createSeqTable(&cliInterface))
    {
      goto label_error;
    }

  if (createMetadataViews(&cliInterface))
    {
      goto label_error;
    }

  return;

 label_error:
  if (xnWasStartedHere)
    {
      cliRC = cliInterface.rollbackXn();
    }
 
  return;
}

void CmpSeabaseDDL::createSeabaseMDviews()
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  ExeCliInterface cliInterface(STMTHEAP);

  if ((CmpCommon::context()->isUninitializedSeabase()) &&
      (CmpCommon::context()->uninitializedSeabaseErrNum() == -1393))
    {
      *CmpCommon::diags() << DgSqlCode(-1393);
      return;
    }

  if (createMetadataViews(&cliInterface))
    {
      return;
    }

}

void CmpSeabaseDDL::dropSeabaseMDviews()
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  ExeCliInterface cliInterface(STMTHEAP);

  if ((CmpCommon::context()->isUninitializedSeabase()) &&
      (CmpCommon::context()->uninitializedSeabaseErrNum() == -1393))
    {
      *CmpCommon::diags() << DgSqlCode(-1393);
      return;
    }

  if (dropMetadataViews(&cliInterface))
    {
      return;
    }

}

void CmpSeabaseDDL::createSeabaseSeqTable()
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  ExeCliInterface cliInterface(STMTHEAP);

  if ((CmpCommon::context()->isUninitializedSeabase()) &&
      (CmpCommon::context()->uninitializedSeabaseErrNum() == -1393))
    {
      *CmpCommon::diags() << DgSqlCode(-1393);
      return;
    }

  if (createSeqTable(&cliInterface))
    {
      return;
    }

}

short CmpSeabaseDDL::createSeqTable(ExeCliInterface *cliInterface)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  char queryBuf[5000];

  const QString * qs = NULL;
  Int32 sizeOfqs = 0;
  
  qs = seabaseSeqGenDDL;
  sizeOfqs = sizeof(seabaseSeqGenDDL);
  
  Int32 qryArraySize = sizeOfqs / sizeof(QString);
  char * gluedQuery;
  Lng32 gluedQuerySize;
  glueQueryFragments(qryArraySize,  qs,
                     gluedQuery, gluedQuerySize);
  
  param_[0] = getSystemCatalog();
  param_[1] = SEABASE_MD_SCHEMA;

  str_sprintf(queryBuf, gluedQuery,
              param_[0], param_[1]);

  NABoolean xnWasStartedHere = FALSE;
  if (NOT xnInProgress(cliInterface))
    {
      cliRC = cliInterface->beginXn();
      if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
          return -1;
        }
      
      xnWasStartedHere = TRUE;
    }

  cliRC = cliInterface->executeImmediate(queryBuf);
  if (cliRC == -1390)  // already exists
    {
      // ignore the error.
    }
  else if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }
  
  if (xnWasStartedHere)
    {
      cliRC = cliInterface->commitXn();
      if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
          return -1;
        }
    }
  
  return 0;
}

void  CmpSeabaseDDL::createSeabaseSequence(StmtDDLCreateSequence  * createSequenceNode,
                                           NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC;
  Lng32 retcode;

  char buf[4000];

  NAString sequenceName = (NAString&)createSequenceNode->getSeqName();

  ComObjectName seqName(sequenceName, COM_TABLE_NAME);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  seqName.applyDefaults(currCatAnsiName, currSchAnsiName);

  NAString catalogNamePart = seqName.getCatalogNamePartAsAnsiString();
  NAString schemaNamePart = seqName.getSchemaNamePartAsAnsiString(TRUE);
  NAString seqNamePart = seqName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extSeqName = seqName.getExternalName(TRUE);

  ElemDDLSGOptions * sgo = createSequenceNode->getSGoptions();

  ExeCliInterface cliInterface(STMTHEAP);
  
  NABoolean trustedCaller = FALSE;
  trustedCaller = TRUE;

  // Verify that the current user has authority to perform operation
  if (!trustedCaller && 
      !isDDLOperationAuthorized(SQLOperation::CREATE_SEQUENCE,seqName,
                                COM_SEQUENCE_GENERATOR_OBJECT_LIT))
  {
     *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
     processReturn ();
     return;
  }

  if (isSeabaseReservedSchema(seqName))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_CREATE_TABLE_NOT_ALLOWED_IN_SMD)
                          << DgTableName(extSeqName);
      return;
    }

  retcode = existsInSeabaseMDTable(&cliInterface, 
                                   catalogNamePart, schemaNamePart, seqNamePart, NULL);
  if (retcode < 0)
    {
      processReturn();

      return;
    }
  
  if (retcode == 1) // already exists
    {
      *CmpCommon::diags() << DgSqlCode(-1390)
                                << DgString0(extSeqName);
      
      processReturn();
      
      return;
    }

  ComUID seqUID;
  seqUID.make_UID();
  Int64 seqObjUID = seqUID.get_value();
  
  Int64 createTime = NA_JulianTimestamp();

  NAString quotedSchName;
  ToQuotedString(quotedSchName, schemaNamePart, FALSE);
  NAString quotedSeqObjName;
  ToQuotedString(quotedSeqObjName, seqNamePart, FALSE);

  str_sprintf(buf, "insert into %s.\"%s\".%s values ('%s', '%s', '%s', '%s', %Ld, %Ld, %Ld, '%s', %d )",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              catalogNamePart.data(), quotedSchName.data(), quotedSeqObjName.data(),
              COM_SEQUENCE_GENERATOR_OBJECT_LIT,
              seqObjUID,
              createTime, 
              createTime,
              "Y",
              SUPER_USER);
  cliRC = cliInterface.executeImmediate(buf);
  
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return;
    }

  str_sprintf(buf, "insert into %s.\"%s\".%s values ('%s', %Ld, %d, %Ld, %Ld, %Ld, %Ld, '%s', %Ld, %Ld, %Ld)",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_SEQ_GEN,
              (sgo->isExternalSG() ? COM_EXTERNAL_SG_LIT : COM_INTERNAL_SG_LIT),
              seqObjUID, 
              REC_BIN64_SIGNED, 
              sgo->getStartValue(),
              sgo->getIncrement(),
              sgo->getMaxValue(),
              sgo->getMinValue(),
              (sgo->getCycle() ? "Y" : "N"), 
              sgo->getCache(),
              sgo->getStartValue(),
              0LL);

  cliRC = cliInterface.executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return;
    }

  return;
}

void  CmpSeabaseDDL::alterSeabaseSequence(StmtDDLCreateSequence  * alterSequenceNode,
                                          NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC;
  Lng32 retcode;

  char buf[4000];

  NAString sequenceName = (NAString&)alterSequenceNode->getSeqName();

  ComObjectName seqName(sequenceName, COM_TABLE_NAME);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  seqName.applyDefaults(currCatAnsiName, currSchAnsiName);

  NAString catalogNamePart = seqName.getCatalogNamePartAsAnsiString();
  NAString schemaNamePart = seqName.getSchemaNamePartAsAnsiString(TRUE);
  NAString seqNamePart = seqName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extSeqName = seqName.getExternalName(TRUE);

  ElemDDLSGOptions * sgo = alterSequenceNode->getSGoptions();

  ExeCliInterface cliInterface(STMTHEAP);
  
  NABoolean trustedCaller = FALSE;
  trustedCaller = TRUE;

  // Verify that the current user has authority to perform operation
  if (!trustedCaller && 
      !isDDLOperationAuthorized(SQLOperation::ALTER_SEQUENCE,seqName,
                                COM_SEQUENCE_GENERATOR_OBJECT_LIT))
  {
     *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
     return;
  }

  retcode = existsInSeabaseMDTable(&cliInterface, 
                                   catalogNamePart, schemaNamePart, seqNamePart, NULL);
  if (retcode < 0)
    {
      processReturn();

      return;
    }
  
  if (retcode == 0) // does not exist
    {
      CmpCommon::diags()->clear();
      
      *CmpCommon::diags() << DgSqlCode(-1389)
                          << DgString0(extSeqName);

      processReturn();

      return;
    }

  Int64 seqUID = 
    getObjectUID(&cliInterface,
                 catalogNamePart.data(), schemaNamePart.data(), seqNamePart.data(),
                 COM_SEQUENCE_GENERATOR_OBJECT_LIT);
  
  char setOptions[2000];
  char tmpBuf[1000];

  strcpy(setOptions, " set ");
  if (sgo->isIncrementSpecified())
    {
      str_sprintf(tmpBuf, " increment = %Ld,", sgo->getIncrement());
      strcat(setOptions, tmpBuf);
    }

  if (sgo->isMaxValueSpecified())
    {
      str_sprintf(tmpBuf, " max_value = %Ld,", sgo->getMaxValue());
      strcat(setOptions, tmpBuf);
    }

  if (sgo->isMinValueSpecified())
    {
      str_sprintf(tmpBuf, " min_value = %Ld,", sgo->getMinValue());
      strcat(setOptions, tmpBuf);
    }

  if (sgo->isCacheSpecified())
    {
      str_sprintf(tmpBuf, " cache_size = %Ld,", sgo->getCache());
      strcat(setOptions, tmpBuf);
    }

 if (sgo->isCycleSpecified())
    {
      str_sprintf(tmpBuf, " cycle_option = '%s',", (sgo->getCycle() ? "Y" : "N"));
      strcat(setOptions, tmpBuf);
    }

  // remove last comma
  setOptions[strlen(setOptions)-1] = 0;

  str_sprintf(buf, "update %s.\"%s\".%s %s where seq_uid = %Ld",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_SEQ_GEN,
              setOptions,
              seqUID);

  cliRC = cliInterface.executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return;
    }

  CorrName cn(seqNamePart, STMTHEAP, schemaNamePart, catalogNamePart);
  cn.setSpecialType(ExtendedQualName::SG_TABLE);
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);

  return;
}

void  CmpSeabaseDDL::dropSeabaseSequence(StmtDDLDropSequence  * dropSequenceNode,
                                         NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  NAString sequenceName = (NAString&)dropSequenceNode->getSeqName();

  ComObjectName seqName(sequenceName, COM_TABLE_NAME);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  seqName.applyDefaults(currCatAnsiName, currSchAnsiName);

  NAString catalogNamePart = seqName.getCatalogNamePartAsAnsiString();
  NAString schemaNamePart = seqName.getSchemaNamePartAsAnsiString(TRUE);
  NAString objectNamePart = seqName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extSeqName = seqName.getExternalName(TRUE);

  ExeCliInterface cliInterface(STMTHEAP);

  retcode = existsInSeabaseMDTable(&cliInterface, 
                                   catalogNamePart, schemaNamePart, objectNamePart,
                                   NULL);
  if (retcode < 0)
    {
      processReturn();

      return;
    }
  
  if (retcode == 0) // does not exist
    {
      CmpCommon::diags()->clear();
      
      *CmpCommon::diags() << DgSqlCode(-1389)
                          << DgString0(extSeqName);

      processReturn();

      return;
    }

  // Check to see if the user has the authority to drop the table
  NABoolean trustedCaller = FALSE;
  trustedCaller = TRUE;

  ComObjectName verifyName;
  verifyName = seqName;
  if (!trustedCaller && 
      !isDDLOperationAuthorized(SQLOperation::DROP_SEQUENCE,verifyName,
                                COM_SEQUENCE_GENERATOR_OBJECT_LIT))
  {
     *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
     processReturn ();
     return;
  }

  if (deleteFromSeabaseMDTable(&cliInterface, 
                               catalogNamePart, schemaNamePart, objectNamePart, 
                               COM_SEQUENCE_GENERATOR_OBJECT_LIT))
    return;

  CorrName cn(objectNamePart, STMTHEAP, schemaNamePart, catalogNamePart);
  cn.setSpecialType(ExtendedQualName::SG_TABLE);
  ActiveSchemaDB()->getNATableDB()->removeNATable(cn);
 
  return;
}

short CmpSeabaseDDL::dropSeabaseObjectsFromHbase(const char * pattern)
{
  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    return -1;


  short retcode = ehi->dropAll(pattern, FALSE);

  if (retcode < 0)
    {
      *CmpCommon::diags() << DgSqlCode(-8448)
                          << DgString0((char*)"ExpHbaseInterface::dropAll()")
                          << DgString1(getHbaseErrStr(-retcode))
                          << DgInt0(-retcode)
                          << DgString2((char*)GetCliGlobals()->getJniErrorStr().data());

      return retcode;
    }

  return 0;
}

void CmpSeabaseDDL::dropSeabaseMD()
{
  Lng32 cliRC;
  Lng32 retcode = 0;
  NABoolean xnWasStartedHere = FALSE;

  if ((CmpCommon::context()->isUninitializedSeabase()) &&
      (CmpCommon::context()->uninitializedSeabaseErrNum() == -1393))
    {
      *CmpCommon::diags() << DgSqlCode(-1393);
      return;
    }
  // drop all objects that match the pattern "TRAFODION.*"
  dropSeabaseObjectsFromHbase("TRAFODION\\..*");

  SQL_EXEC_DeleteHbaseJNI();

  CmpCommon::context()->setIsUninitializedSeabase(TRUE);
  CmpCommon::context()->uninitializedSeabaseErrNum() = -1393;

  return;
}

void CmpSeabaseDDL::initSeabaseAuthorization()
{ 
  const char* sysCat = ActiveSchemaDB()->getDefaults().getValue(SEABASE_CATALOG);
  std::string privMDLoc(sysCat);
  privMDLoc += ".\"";
  privMDLoc += SEABASE_PRIVMGR_SCHEMA;
  privMDLoc += "\"";
  std::string objectsLocation(sysCat);
  objectsLocation += ".\"";
  objectsLocation += SEABASE_MD_SCHEMA;
  objectsLocation += "\".";
  std::string authsLocation(objectsLocation);
  objectsLocation += SEABASE_OBJECTS;
  authsLocation += SEABASE_AUTHS; 
  PrivMgrCommands privInterface(privMDLoc, CmpCommon::diags());
  PrivStatus retcode = privInterface.initializeAuthorizationMetadata
    (objectsLocation, authsLocation); 
  if (retcode == STATUS_ERROR && 
      CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
     SEABASEDDL_INTERNAL_ERROR("INITIALIZE AUTHORIZATION command");

  return;
}

void CmpSeabaseDDL::dropSeabaseAuthorization()
{
  const char* sysCat = ActiveSchemaDB()->getDefaults().getValue(SEABASE_CATALOG);
  std::string privMDLoc(sysCat);
  privMDLoc += ".\"";
  privMDLoc += SEABASE_PRIVMGR_SCHEMA;
  privMDLoc += "\"";
  PrivMgrCommands privInterface(privMDLoc, CmpCommon::diags());
  PrivStatus retcode = privInterface.dropAuthorizationMetadata(); 
  if (retcode == STATUS_ERROR && 
      CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
     SEABASEDDL_INTERNAL_ERROR("DROP AUTHORIZATION command");

  return;
}

// ----------------------------------------------------------------------------
// method: insertPrivMgrInfo
//
// this method calls the privilege manager interface to perform privilege
// grants that are required for the object owner.
//
// input:  the object's UID, name, type, and owner 
//       
// returns:
//    TRUE if the privileges were successfully inserted (granted)
//    FALSE if an error occurred (ComDiags is set up with the error)
// ----------------------------------------------------------------------------
NABoolean CmpSeabaseDDL::insertPrivMgrInfo(const Int64 objUID,
                                           const NAString &objName,
                                           const NAString &objType,
                                           const Int32 objOwnerID)
{
  // Only the following object types have associated privileges
  // If not in this list, return TRUE, no grants are needed
  // or grants are done elsewhere (e.g. views)
  if (!(objType == COM_BASE_TABLE_OBJECT_LIT ||
        objType == COM_LIBRARY_OBJECT_LIT ||
        objType == COM_USER_DEFINED_ROUTINE_OBJECT_LIT ))
   return TRUE;

  // Initiate the privilege manager interface class
  std::string privMDLoc(ActiveSchemaDB()->getDefaults().getValue(SEABASE_CATALOG));
  privMDLoc += std::string(".\"") +
               std::string(SEABASE_PRIVMGR_SCHEMA) +
               std::string("\"");
  PrivMgrCommands privInterface(privMDLoc, CmpCommon::diags());

  // If authorization is not enabled, return TRUE, no grants are needed
  // TDB: may want to add a "authorization enabled" flag in Context to 
  //      avoid extra I/O
  if (!privInterface.isAuthorizationEnabled())
    return TRUE;

  // get the username from the objOwnerID
  char userName[MAX_USERNAME_LEN+1];
  Int32 lActualLen = 0;
  Int16 status = ComUser::getUserNameFromUserID( (Int32) objOwnerID 
                                               , (char *)&userName
                                               , MAX_USERNAME_LEN
                                               , lActualLen );
  if (status != FEOK)
    *CmpCommon::diags() << DgSqlCode(-20235)
                        << DgInt0(status)
                        << DgInt1(objOwnerID);

  std::string grantee(userName);
  std::string grantor("_SYSTEM");

  // Once grant all works, there is no need to set up each privilege
  // this code needs to change 
  vector<std::string> userPermissions;
  userPermissions.push_back("ALL");

  PrivStatus retcode = 
    privInterface.grantObjectPrivilege(objUID, 
                                       std::string(objName), 
                                       std::string(objType),
                                       objOwnerID, grantee,
                                       SYSTEM_USER, grantor,
                                       userPermissions, true, true);
  if (retcode != STATUS_GOOD && retcode != STATUS_WARNING)
    return FALSE;
  return TRUE;
}

// ----------------------------------------------------------------------------
// method: deletePrivMgrInfo
//
// this method calls the privilege manager interface to perform revokes
// when objects are dropped to remove all privileges
//
// input:  the object's UID, name, and type
//       
// returns:
//    TRUE if the privileges were correctly deleted (revoked)
//    FALSE if an error occurred (ComDiags is set up with the error)
// ----------------------------------------------------------------------------
NABoolean CmpSeabaseDDL::deletePrivMgrInfo(const NAString &objectName,
                                           const Int64 objUID, 
                                           const NAString objectType)
{
  // Only the following object types have associated privileges
  // If not in this list, return TRUE, no revokes are needed
  if (!(objectType == COM_BASE_TABLE_OBJECT_LIT ||
        objectType == COM_LIBRARY_OBJECT_LIT ||
        objectType == COM_USER_DEFINED_ROUTINE_OBJECT_LIT ||
        objectType == COM_VIEW_OBJECT_LIT ))
   return TRUE;

  // Initiate the privilege manager interface class
  std::string privMDLoc(ActiveSchemaDB()->getDefaults().getValue(SEABASE_CATALOG));
  privMDLoc += std::string(".\"") +
               std::string(SEABASE_PRIVMGR_SCHEMA) +
               std::string("\"");
  PrivMgrCommands privInterface(privMDLoc, CmpCommon::diags());

  // If authorization is not enabled, return TRUE, no grants are needed
  // TDB: may want to add a "authorization enabled" flag in Context to 
  //      avoid extra I/O
  if (!privInterface.isAuthorizationEnabled())
    return TRUE;

  const std::string objName(objectName.data());
  PrivStatus retcode = 
    privInterface.revokeObjectPrivilege (objUID, objName, -2);
  if (retcode != STATUS_GOOD && retcode != STATUS_WARNING && retcode != STATUS_NOTFOUND)
    return FALSE;
  NegateAllErrors(CmpCommon::diags());
  return TRUE;
}

short CmpSeabaseDDL::dropMDTable(ExpHbaseInterface *ehi, const char * tab)
{
  Lng32 retcode = 0;

  HbaseStr hbaseObjStr;
  NAString hbaseObjPrefix = getSystemCatalog();
  hbaseObjPrefix += ".";
  hbaseObjPrefix += SEABASE_MD_SCHEMA;
  hbaseObjPrefix += ".";
  NAString hbaseObject = hbaseObjPrefix + tab;
  hbaseObjStr.val = (char*)hbaseObject.data();
  hbaseObjStr.len = hbaseObject.length();

  retcode = existsInHbase(hbaseObject, ehi);
  if (retcode == 1) // exists
    {
      retcode = dropHbaseTable(ehi, &hbaseObjStr, FALSE);
      return retcode;
    }

  return 0;
}

void CmpSeabaseDDL::updateVersion()
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  ExeCliInterface cliInterface(STMTHEAP);

  if ((CmpCommon::context()->isUninitializedSeabase()) &&
      (CmpCommon::context()->uninitializedSeabaseErrNum() == -1393))
    {
      *CmpCommon::diags() << DgSqlCode(-1393);
      return;
    }

  Int64 softMajorVersion;
  Int64 softMinorVersion;
  Int64 softUpdVersion;
  getSystemSoftwareVersion(softMajorVersion, softMinorVersion, softUpdVersion);

  char queryBuf[5000];
  
  Int64 updateTime = NA_JulianTimestamp();

  str_sprintf(queryBuf, "update %s.\"%s\".%s set major_version = %Ld, minor_version = %Ld, init_time = %Ld, comment = 'update version'  where version_type = 'SOFTWARE' ",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_VERSIONS,
              softMajorVersion, softMinorVersion,
              updateTime);
  NABoolean xnWasStartedHere = FALSE;
  if (NOT xnInProgress(&cliInterface))
    {
      cliRC = cliInterface.beginXn();
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
          return;
        }
      
      xnWasStartedHere = TRUE;
    }

  cliRC = cliInterface.executeImmediate(queryBuf);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return;
    }
  
  if (xnWasStartedHere)
    {
      cliRC = cliInterface.commitXn();
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
          return;
        }
    }
}

void CmpSeabaseDDL::purgedataHbaseTable(DDLExpr * ddlExpr,
                                       NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC;
  Lng32 retcode = 0;
  NABoolean xnWasStartedHere = FALSE;

  CorrName &purgedataTableName = ddlExpr->purgedataTableName();
  NAString tabName = ddlExpr->getQualObjName();

  ComObjectName tableName(tabName, COM_TABLE_NAME);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  tableName.applyDefaults(currCatAnsiName, currSchAnsiName);

  NAString catalogNamePart = tableName.getCatalogNamePartAsAnsiString();
  NAString schemaNamePart = tableName.getSchemaNamePartAsAnsiString(TRUE);
  NAString objectNamePart = tableName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extTableName = tableName.getExternalName(TRUE);
  const NAString extNameForHbase = catalogNamePart + "." + schemaNamePart + "." + objectNamePart;

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

  // special tables, like index, can only be purged in internal mode with special
  // flag settings. Otherwise, it can make database inconsistent.
  if ((purgedataTableName.isSpecialTable()) &&
     (!Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
    {
      *CmpCommon::diags()
        << DgSqlCode(-1010);
      
      processReturn();
      
      return;
    }
  
  ActiveSchemaDB()->getNATableDB()->useCache();

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  CorrName cn(tableName.getObjectNamePart().getInternalName(),
              STMTHEAP,
              tableName.getSchemaNamePart().getInternalName(),
              tableName.getCatalogNamePart().getInternalName());
  cn.setSpecialType(purgedataTableName);
  NATable *naTable = bindWA.getNATable(cn); 
  if (naTable == NULL || bindWA.errStatus())
    {
      processReturn();

      return;
    }

  // cannot purgedata a view
  if (naTable->getViewText())
    {
      *CmpCommon::diags()
        << DgSqlCode(-1010);
      
      processReturn();
      
      return;
    }

  if (naTable->getUniqueConstraints().entries() > 0)
    {
      const AbstractRIConstraintList &uniqueList = naTable->getUniqueConstraints();

      for (Int32 i = 0; i < uniqueList.entries(); i++)
        {
          AbstractRIConstraint *ariConstr = uniqueList[i];
          
          if (ariConstr->getOperatorType() != ITM_UNIQUE_CONSTRAINT)
            continue;
          
          UniqueConstraint * uniqConstr = (UniqueConstraint*)ariConstr;

          if (uniqConstr->hasRefConstraintsReferencingMe())
            {
              NAString reason("Reason: Foreign Key constraints from other tables are referencing this table");
 
              *CmpCommon::diags()
                << DgSqlCode(-1425)
                << DgTableName(extTableName)
                << DgString0(reason.data());
              
              deallocEHI(ehi); 
              
              processReturn();
              
              return;
            }

        } // for      
    }

  retcode = updateObjectValidDef(&cliInterface, 
                                 catalogNamePart.data(), schemaNamePart.data(), objectNamePart.data(),
                                 "BT", "N");
  if (retcode)
    {
      deallocEHI(ehi); 
      
      processReturn();
      
      return;
    }
                                 
  NABoolean asyncDrop = (CmpCommon::getDefault(HBASE_ASYNC_DROP_TABLE) == DF_ON);

  HbaseStr hbaseTable;
  hbaseTable.val = (char*)extNameForHbase.data();
  hbaseTable.len = extNameForHbase.length();

  // drop this table from hbase
  retcode = dropHbaseTable(ehi, &hbaseTable, FALSE);
  if (retcode)
    {
      deallocEHI(ehi); 
      
      processReturn();
      
      return;
    }

  // and recreate it.
  NAFileSet * naf = naTable->getClusteringIndex();

  NAList<HbaseCreateOption*> * hbaseCreateOptions = naTable->hbaseCreateOptions();
  Lng32 numSaltedPartitions = naTable->numSaltPartns();
  Lng32 numSplits = (numSaltedPartitions ? numSaltedPartitions - 1 : 0);
  Lng32 numKeys = naf->getIndexKeyColumns().entries();
  Lng32 keyLength = naf->getKeyLength();
  char ** encodedKeysBuffer = NULL;

  const desc_struct * tableDesc = naTable->getTableDesc();
  desc_struct * colDescs = tableDesc->body.table_desc.columns_desc; 
  desc_struct * keyDescs = (desc_struct*)naf->getKeysDesc();
  if (createEncodedKeysBuffer(encodedKeysBuffer,
                              colDescs, keyDescs, numSplits, numKeys, keyLength))
    {
      deallocEHI(ehi); 

      processReturn();
      
      return;
    }
  
  retcode = createHbaseTable(ehi, &hbaseTable, SEABASE_DEFAULT_COL_FAMILY, 
                             NULL, NULL,
                             hbaseCreateOptions, numSplits, keyLength, 
                             encodedKeysBuffer);
  if (retcode == -1)
    {
      deallocEHI(ehi); 

      processReturn();

      return;
    }

  if (naTable->hasSecondaryIndexes()) // user indexes
    {
      const NAFileSetList &indexList = naTable->getIndexList();
      
      // purgedata from all indexes
      for (Int32 i = 0; i < indexList.entries(); i++)
        {
          const NAFileSet * naf = indexList[i];
          if (naf->getKeytag() == 0)
            continue;
          
          const QualifiedName &qn = naf->getFileSetName();
          
          NAString catName = qn.getCatalogName();
          NAString schName = qn.getSchemaName();
          NAString idxName = qn.getObjectName();
          NAString extNameForIndex = catName + "." + schName + "." + idxName;

          HbaseStr hbaseIndex;
          hbaseIndex.val = (char*)extNameForIndex.data();
          hbaseIndex.len = extNameForIndex.length();
          
          // drop this table from hbase
          retcode = dropHbaseTable(ehi, &hbaseIndex, FALSE);
          if (retcode)
            {
              deallocEHI(ehi); 
              
              processReturn();
              
              return;
            }
          
          retcode = createHbaseTable(ehi, &hbaseIndex, SEABASE_DEFAULT_COL_FAMILY, 
                                     NULL, NULL,
                                     NULL, 0, 0,
                                     NULL);
          if (retcode == -1)
            {
              deallocEHI(ehi); 
              
              processReturn();
              
              return;
            }
          
        } // for
    } // secondary indexes

  retcode = updateObjectValidDef(&cliInterface, 
                                 catalogNamePart.data(), schemaNamePart.data(), objectNamePart.data(),
                                 "BT", "Y");
  if (retcode)
    {
      deallocEHI(ehi); 
      
      processReturn();
      
      return;
    }

  return;
}

short CmpSeabaseDDL::executeSeabaseDDL(DDLExpr * ddlExpr, ExprNode * ddlNode,
                                       NAString &currCatName, NAString &currSchName)
{
  Lng32 cliRC = 0;
  ExeCliInterface cliInterface(STMTHEAP);

  // error accessing hbase. Return.
  if ((CmpCommon::context()->isUninitializedSeabase()) &&
      (CmpCommon::context()->uninitializedSeabaseErrNum() == -1398))
    {
      *CmpCommon::diags() << DgSqlCode(CmpCommon::context()->uninitializedSeabaseErrNum())
                          << DgInt0(CmpCommon::context()->hbaseErrNum())
                          << DgString0(CmpCommon::context()->hbaseErrStr());
      
      return -1;
    }

  NABoolean xnWasStartedHere = FALSE;
  NABoolean ignoreUninitTrafErr = FALSE;
  if ((ddlExpr->dropHbase()) ||
      (ddlExpr->initHbase()) ||
      (ddlExpr->createMDViews()) ||
      (ddlExpr->dropMDViews()) ||
      (ddlExpr->addSeqTable()) ||
      (ddlExpr->updateVersion()))
    ignoreUninitTrafErr = TRUE;

  // if metadata views are being created during initialize trafodion, then this
  // context will be uninitialized.
  // Check for that and ignore uninitialize traf error.
  if ((ddlNode) && (ddlNode->getOperatorType() == DDL_CREATE_VIEW))
    {
      StmtDDLCreateView * createViewParseNode =
        ddlNode->castToStmtDDLNode()->castToStmtDDLCreateView();
      
      ComObjectName viewName(createViewParseNode->getViewName());
      if ((isSeabaseMD(viewName)) &&
          (Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)) &&
          (CmpCommon::context()->isUninitializedSeabase()))
        {
          ignoreUninitTrafErr = TRUE;
        }
    }
  // if metadata tables are being created during initialize trafodion, then this
  // context will be uninitialized.
  // Check for that and ignore uninitialize traf error.
  else if ((ddlNode) && (ddlNode->getOperatorType() == DDL_CREATE_TABLE))
    {
      StmtDDLCreateTable * createTableParseNode =
        ddlNode->castToStmtDDLNode()->castToStmtDDLCreateTable();
      
      ComObjectName tableName(createTableParseNode->getTableName());
      if ((isSeabaseMD(tableName)) &&
          (Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)) &&
          (CmpCommon::context()->isUninitializedSeabase()))
        {
          ignoreUninitTrafErr = TRUE;
        }
    }

  if ((CmpCommon::context()->isUninitializedSeabase()) &&
      (NOT ignoreUninitTrafErr))
    {
      *CmpCommon::diags() << DgSqlCode(CmpCommon::context()->uninitializedSeabaseErrNum());
      return -1;
    }

  if (sendAllControlsAndFlags())
    {
      CMPASSERT(0);
      return -1;
    }
  
  NABoolean startXn = TRUE;
  if ((ddlExpr->dropHbase()) ||
      (ddlExpr->purgedataHbase()) ||
      (ddlExpr->initHbase()) ||
      (ddlExpr->createMDViews()) ||
      (ddlExpr->dropMDViews()) ||
      (ddlExpr->initAuthorization()) ||
      (ddlExpr->dropAuthorization()) ||
      (ddlExpr->addSeqTable()) ||
      (ddlExpr->updateVersion()) ||
      ((ddlNode) &&
       ((ddlNode->getOperatorType() == DDL_DROP_SCHEMA) ||
        (ddlNode->getOperatorType() == DDL_ALTER_TABLE_ADD_CONSTRAINT_PRIMARY_KEY) ||
        ((ddlNode->getOperatorType() == DDL_CREATE_TABLE) &&
         ((ddlNode->castToStmtDDLNode()->castToStmtDDLCreateTable()->getAddConstraintUniqueArray().entries() > 0) ||
          (ddlNode->castToStmtDDLNode()->castToStmtDDLCreateTable()->getAddConstraintRIArray().entries() > 0) ||
          (ddlNode->castToStmtDDLNode()->castToStmtDDLCreateTable()->getAddConstraintCheckArray().entries() > 0))))))
    {
      // transaction will be started and commited in called methods.
      startXn = FALSE;
    }

  if ((NOT xnInProgress(&cliInterface)) &&
      (startXn))
    {
      cliRC = beginXn(&cliInterface);
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
          goto label_return;
        }

      xnWasStartedHere = TRUE;
    }

  if (ddlExpr->initHbase()) 
    {
      initSeabaseMD();
    }
  else if (ddlExpr->dropHbase())
    {
      dropSeabaseMD();
    }
  else if (ddlExpr->createMDViews())
    {
      createSeabaseMDviews();
    }
  else if (ddlExpr->dropMDViews())
    {
      dropSeabaseMDviews();
    }
  else if (ddlExpr->initAuthorization())
    {
      initSeabaseAuthorization();
    }
  else if (ddlExpr->dropAuthorization())
    {
      dropSeabaseAuthorization();
    }
  else if (ddlExpr->addSeqTable())
    {
      createSeabaseSeqTable();
    }
  else if (ddlExpr->updateVersion())
    {
      updateVersion();
    }
  else if (ddlExpr->purgedataHbase())
    {
      purgedataHbaseTable(ddlExpr, currCatName, currSchName);
    }
  else
    {
      CMPASSERT(ddlNode);
      
      if (ddlNode->getOperatorType() == DDL_CREATE_TABLE)
        {
          // create hbase table
          StmtDDLCreateTable * createTableParseNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLCreateTable();
          
          if ((createTableParseNode->getAddConstraintUniqueArray().entries() > 0) ||
              (createTableParseNode->getAddConstraintRIArray().entries() > 0) ||
              (createTableParseNode->getAddConstraintCheckArray().entries() > 0))
            createSeabaseTableCompound(createTableParseNode, currCatName, currSchName);
          else
            createSeabaseTable(createTableParseNode, currCatName, currSchName);
        }
      else if (ddlNode->getOperatorType() == DDL_CREATE_HBASE_TABLE)
        {
          // create hbase table
          StmtDDLCreateHbaseTable * createTableParseNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLCreateHbaseTable();
          
          createNativeHbaseTable(createTableParseNode, currCatName, currSchName);
        }
      else if (ddlNode->getOperatorType() == DDL_DROP_TABLE)
        {
          // drop seabase table
          StmtDDLDropTable * dropTableParseNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLDropTable();
          
          dropSeabaseTable(dropTableParseNode, currCatName, currSchName);
        }
      else if (ddlNode->getOperatorType() == DDL_DROP_HBASE_TABLE)
        {
          // drop hbase table
          StmtDDLDropHbaseTable * dropTableParseNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLDropHbaseTable();
          
          dropNativeHbaseTable(dropTableParseNode, currCatName, currSchName);
        }
       else if (ddlNode->getOperatorType() == DDL_CREATE_INDEX)
        {
          // create seabase index
          StmtDDLCreateIndex * createIndexParseNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLCreateIndex();
          
          createSeabaseIndex(createIndexParseNode, currCatName, currSchName);
        }
      else if (ddlNode->getOperatorType() == DDL_POPULATE_INDEX)
        {
          // populate seabase index
          StmtDDLPopulateIndex * populateIndexParseNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLPopulateIndex();
          
          populateSeabaseIndex(populateIndexParseNode, currCatName, currSchName);
        }
      else if (ddlNode->getOperatorType() == DDL_DROP_INDEX)
        {
          // drop seabase table
          StmtDDLDropIndex * dropIndexParseNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLDropIndex();
          
          dropSeabaseIndex(dropIndexParseNode, currCatName, currSchName);
        }
      else if (ddlNode->getOperatorType() == DDL_ALTER_TABLE_ADD_COLUMN)
        {
          StmtDDLAlterTableAddColumn * alterAddColNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLAlterTableAddColumn();
          
          alterSeabaseTableAddColumn(alterAddColNode, 
                                   currCatName, currSchName);
        }
      else if (ddlNode->getOperatorType() == DDL_ALTER_TABLE_DROP_COLUMN)
        {
          StmtDDLAlterTableDropColumn * alterDropColNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLAlterTableDropColumn();
          
          alterSeabaseTableDropColumn(alterDropColNode, 
                                                    currCatName, currSchName);
        }
      else if (ddlNode->getOperatorType() == DDL_ALTER_TABLE_ADD_CONSTRAINT_PRIMARY_KEY)
        {
          StmtDDLAddConstraint * alterAddConstraint =
            ddlNode->castToStmtDDLNode()->castToStmtDDLAddConstraint();
          
          alterSeabaseTableAddPKeyConstraint(alterAddConstraint, 
                                             currCatName, currSchName);
        }
      else if (ddlNode->getOperatorType() == DDL_ALTER_TABLE_ADD_CONSTRAINT_UNIQUE)
        {
          StmtDDLAddConstraint * alterAddConstraint =
            ddlNode->castToStmtDDLNode()->castToStmtDDLAddConstraint();
          
          alterSeabaseTableAddUniqueConstraint(alterAddConstraint, 
                                               currCatName, currSchName);
        }
      else if (ddlNode->getOperatorType() == DDL_ALTER_TABLE_ADD_CONSTRAINT_REFERENTIAL_INTEGRITY)
        {
          StmtDDLAddConstraint * alterAddConstraint =
            ddlNode->castToStmtDDLNode()->castToStmtDDLAddConstraint();
          
          alterSeabaseTableAddRIConstraint(alterAddConstraint, 
                                           currCatName, currSchName);
        }
      else if (ddlNode->getOperatorType() == DDL_ALTER_TABLE_ADD_CONSTRAINT_CHECK)
        {
          StmtDDLAddConstraint * alterAddConstraint =
            ddlNode->castToStmtDDLNode()->castToStmtDDLAddConstraint();
          
          alterSeabaseTableAddCheckConstraint(alterAddConstraint, 
                                           currCatName, currSchName);
        }
      else if (ddlNode->getOperatorType() == DDL_ALTER_TABLE_DROP_CONSTRAINT)
        {
          StmtDDLDropConstraint * alterDropConstraint =
            ddlNode->castToStmtDDLNode()->castToStmtDDLDropConstraint();
          
          alterSeabaseTableDropConstraint(alterDropConstraint, 
                                         currCatName, currSchName);
        }

      else if ((ddlNode->getOperatorType() == DDL_ALTER_TABLE_DISABLE_INDEX) ||
               (ddlNode->getOperatorType() == DDL_ALTER_TABLE_ENABLE_INDEX))
        {

          NABoolean allIndexes = FALSE;
          StmtDDLAlterTableDisableIndex * disableIdx = ddlNode->castToStmtDDLNode()->castToStmtDDLAlterTableDisableIndex();
          NAString tabNameNAS ;
          if (disableIdx)
          {
            allIndexes = disableIdx->getAllIndexes();
            tabNameNAS = disableIdx->getTableName();
          }
          else
          {
            StmtDDLAlterTableEnableIndex * enableIdx = ddlNode->castToStmtDDLNode()->castToStmtDDLAlterTableEnableIndex();
            allIndexes = enableIdx->getAllIndexes();
            tabNameNAS = enableIdx->getTableName();
          }

          if (!allIndexes)
            alterSeabaseTableDisableOrEnableIndex(ddlNode,
                                                currCatName,
                                                currSchName);
          else
          {
            alterSeabaseTableDisableOrEnableAllIndexes(ddlNode,
                                                     currCatName,
                                                     currSchName,
                                                     (NAString &) tabNameNAS);
          }
        }
     else if (ddlNode->getOperatorType() == DDL_ALTER_TABLE_RENAME)
        {
          StmtDDLAlterTableRename * alterRenameTable =
            ddlNode->castToStmtDDLNode()->castToStmtDDLAlterTableRename();

          renameSeabaseTable(alterRenameTable,
                                  currCatName, currSchName);
        }
      else if (ddlNode->getOperatorType() == DDL_CREATE_VIEW)
        {
          // create seabase view
          StmtDDLCreateView * createViewParseNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLCreateView();
          
          createSeabaseView(createViewParseNode, currCatName, currSchName);
        }
      else if (ddlNode->getOperatorType() == DDL_DROP_VIEW)
        {
          // drop seabase table
          StmtDDLDropView * dropViewParseNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLDropView();
          
          dropSeabaseView(dropViewParseNode, currCatName, currSchName);
        }
      else if (ddlNode->getOperatorType() == DDL_REGISTER_USER)
        {
         StmtDDLRegisterUser *registerUserParseNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLRegisterUser();
          StmtDDLRegisterUser::RegisterUserType ruType =
                registerUserParseNode->getRegisterUserType();
          if (ruType == StmtDDLRegisterUser::REGISTER_USER)
            registerSeabaseUser(registerUserParseNode);
          else
            unregisterSeabaseUser(registerUserParseNode);
        }
      else if (ddlNode->getOperatorType() == DDL_CREATE_ROLE)
        {
         StmtDDLCreateRole *createRoleParseNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLCreateRole();
            
         CmpSeabaseDDLrole role;
         
         if (createRoleParseNode->isCreateRole())
            role.createRole(getSystemCatalog(),createRoleParseNode);
         else
            role.dropRole(getSystemCatalog(),createRoleParseNode);
        }
      else if (ddlNode->getOperatorType() == DDL_ALTER_USER)
        {
         StmtDDLAlterUser *alterUserParseNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLAlterUser();
          alterSeabaseUser(alterUserParseNode);
        }
      else if (ddlNode->getOperatorType() == DDL_REGISTER_COMPONENT)
        {
         StmtDDLRegisterComponent *registerComponentParseNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLRegisterComponent();
         registerSeabaseComponent(registerComponentParseNode);
        }
      else if (ddlNode->getOperatorType() == DDL_CREATE_COMPONENT_PRIVILEGE)
        {
         StmtDDLCreateComponentPrivilege *createComponentOperationParseNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLCreateComponentPrivilege();
         createSeabaseComponentOperation(getSystemCatalog(),
                                         createComponentOperationParseNode);
        }
      else if (ddlNode->getOperatorType() == DDL_DROP_COMPONENT_PRIVILEGE)
        {
         StmtDDLDropComponentPrivilege *dropComponentOperationParseNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLDropComponentPrivilege();
         dropSeabaseComponentOperation(getSystemCatalog(),
                                       dropComponentOperationParseNode);
        }
      else if (ddlNode->getOperatorType() == DDL_GRANT_COMPONENT_PRIVILEGE)
        {
         StmtDDLGrantComponentPrivilege *grantComponentPrivilegeParseNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLGrantComponentPrivilege();
         grantSeabaseComponentPrivilege(getSystemCatalog(),
                                        grantComponentPrivilegeParseNode);
        }
      else if (ddlNode->getOperatorType() == DDL_REVOKE_COMPONENT_PRIVILEGE)
        {
         StmtDDLRevokeComponentPrivilege *revokeComponentPrivilegeParseNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLRevokeComponentPrivilege();
         revokeSeabaseComponentPrivilege(getSystemCatalog(),
                                         revokeComponentPrivilegeParseNode);
        }
      else if (ddlNode->getOperatorType() == DDL_GRANT_ROLE)
        {
         StmtDDLRoleGrant *grantRoleParseNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLRoleGrant();   
         grantRevokeSeabaseRole(getSystemCatalog(),grantRoleParseNode);
        }
      else if ((ddlNode->getOperatorType() == DDL_GRANT) ||
               (ddlNode->getOperatorType() == DDL_REVOKE))
        {
          // grant/revoke seabase table
          StmtDDLNode * stmtDDLParseNode =
            ddlNode->castToStmtDDLNode();
          
          seabaseGrantRevoke(stmtDDLParseNode,
                             (ddlNode->getOperatorType() == DDL_GRANT 
                              ? TRUE : FALSE),
                             currCatName, currSchName);
        }
      else if (ddlNode->getOperatorType() == DDL_CREATE_SCHEMA)
        {
          // no-op for now. Just return.
          StmtDDLCreateSchema * createSchemaParseNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLCreateSchema();
        }
      else if (ddlNode->getOperatorType() == DDL_DROP_SCHEMA)
        {
          // drop all tables in schema
          StmtDDLDropSchema * dropSchemaParseNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLDropSchema();
          
          dropSeabaseSchema(dropSchemaParseNode, currCatName, currSchName);
        }
      else if (ddlNode->getOperatorType() == DDL_CREATE_LIBRARY)
        {
          // create seabase library
          StmtDDLCreateLibrary * createLibraryParseNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLCreateLibrary();
          
          createSeabaseLibrary(createLibraryParseNode, currCatName, 
                               currSchName);
        }
      else if (ddlNode->getOperatorType() == DDL_DROP_LIBRARY)
        {
          // drop seabase library
          StmtDDLDropLibrary * dropLibraryParseNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLDropLibrary();
          
          dropSeabaseLibrary(dropLibraryParseNode, currCatName, currSchName);
        }
      else if (ddlNode->getOperatorType() == DDL_CREATE_ROUTINE)
        {
          // create seabase routine
          StmtDDLCreateRoutine * createRoutineParseNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLCreateRoutine();
          
          createSeabaseRoutine(createRoutineParseNode, currCatName, 
                               currSchName);
        }
      else if (ddlNode->getOperatorType() == DDL_DROP_ROUTINE)
        {
          // drop seabase routine
          StmtDDLDropRoutine * dropRoutineParseNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLDropRoutine();
          
          dropSeabaseRoutine(dropRoutineParseNode, currCatName, currSchName);
        }
       else if (ddlNode->getOperatorType() == DDL_CREATE_SEQUENCE)
        {
          // create seabase sequence
          StmtDDLCreateSequence * createSequenceParseNode =
            ddlNode->castToStmtDDLNode()->castToStmtDDLCreateSequence();
          
          if (createSequenceParseNode->isAlter())
            alterSeabaseSequence(createSequenceParseNode, currCatName, 
                                 currSchName);
          else
            createSeabaseSequence(createSequenceParseNode, currCatName, 
                                  currSchName);
        }
       else if (ddlNode->getOperatorType() == DDL_DROP_SEQUENCE)
         {
           // drop seabase sequence
           StmtDDLDropSequence * dropSequenceParseNode =
             ddlNode->castToStmtDDLNode()->castToStmtDDLDropSequence();
           
           dropSeabaseSequence(dropSequenceParseNode, currCatName, currSchName);
         }
      
    } // else
  
label_return:
  restoreAllControlsAndFlags();

  if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_))
    {
      if (xnWasStartedHere)
        cliRC = rollbackXn(&cliInterface);
      
      return -1;
    }

  if (xnWasStartedHere)
    {
      cliRC = commitXn(&cliInterface);
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
          
          return -1;
        }
    }

  return 0;
}

void CmpSeabaseDDL::registerSeabaseUser(StmtDDLRegisterUser * authParseNode)
{
  CmpSeabaseDDLuser user;
  user.registerUser(authParseNode);
}

void CmpSeabaseDDL::alterSeabaseUser(StmtDDLAlterUser * authParseNode)
{
  CmpSeabaseDDLuser user;
  user.alterUser(authParseNode);
}

void CmpSeabaseDDL::unregisterSeabaseUser(StmtDDLRegisterUser * authParseNode)
{
  CmpSeabaseDDLuser user;
  user.unregisterUser(getSystemCatalog(),authParseNode);
}

// *****************************************************************************
// *                                                                           *
// * Function: CmpSeabaseDDL::isDDLOperationAuthorized                         *
// *                                                                           *
// *   This member function determines if a user has authority to perform a    *
// * specific DDL operation.                                                   *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <operation>                  SQLOperation                       In       *
// *    is operation the user wants to perform.                                *
// *                                                                           *
// *  <objName>                    const ComObjectName &              In       *
// *    is the object the user wants to perform the DDL operation on.          *
// *                                                                           *
// *  <objType>                    const char *                       In       *
// *    is the two character code for the object type, one of                  *
// *  COM_*_OBJECT_LIT defined in common/ComSmallDefs.h.                       *
// *                                                                           *
// *****************************************************************************
bool CmpSeabaseDDL::isDDLOperationAuthorized(
   SQLOperation operation,
   const ComObjectName & objName,
   const char * objType)

{

// Root user is authorized for all operations.  For installations with no
// security, all users are mapped to root database user, so all users have
// full DDL authority.

int32_t currentUser = ComUser::getCurrentUser();

   if (currentUser == ComUser::getRootUserID())
      return true;

// If authorization is not enabled, then authorization should not be enabled
// either, and the previous check should have already returned.  But just in 
// case, verify authorization is enabled before proceeding.  Eventually this 
// state should be recorded somewhere, e.g. CLI globals.

   if (!isAuthorizationEnabled())
      return true;

// Authorization is enabled.  If the user owns the target object or the 
// schema containing the object, they have full DDL authority on the object.

// For create operations, the object does not exist so there is no owner.
// Get the owner from the OBJECTS table
   if (!PrivMgr::isSQLCreateOperation(operation))
   {
      const NAString catName = objName.getCatalogNamePartAsAnsiString();
      const NAString schName = objName.getSchemaNamePartAsAnsiString(true);
      const NAString objectName = objName.getObjectNamePartAsAnsiString(true);
  
      // TBD: check to see if the object exists in the NATable structure, if so
      // get the objectOwner from there instead of doing a separate I/O.
      ExeCliInterface cliInterface(STMTHEAP);
      Int32 objOwnerID;
      Int32 retcode = getObjectOwner(&cliInterface,
                                     catName.data(),
                                     schName.data(),
                                     objectName.data(),
                                     objType,
                                     &objOwnerID);

      // -1 is returned if object owner could not be extracted, 0 otherwise
      if (retcode == 0 && currentUser == objOwnerID)
         return true;
      //TODO: check schema owner when schemas have owners
   }
  
   std::string privMDLoc(ActiveSchemaDB()->getDefaults().getValue(SEABASE_CATALOG));
   privMDLoc += std::string(".\"") +
                std::string(SEABASE_PRIVMGR_SCHEMA) +
                std::string("\"");
   
PrivMgrComponentPrivileges componentPrivileges(privMDLoc,CmpCommon::diags());

   if (componentPrivileges.hasSQLPriv(currentUser,operation,true))
      return true;   
   
// TODO: check for schema level privileges.  This could be an AND with 
// component level privileges.

   return false;

}
//************** End of CmpSeabaseDDL::isDDLOperationAuthorized ****************


// *****************************************************************************
// *                                                                           *
// * Function: createSeabaseComponentOperation                                 *
// *                                                                           *
// *   This functions handles the CREATE COMPONENT PRIVILEGE command.          *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <systemCatalog>              const std::string &                In       *
// *    is catalog where system tables reside.                                 *
// *                                                                           *
// *  <pParseNode>                 StmtDDLCreateComponentPrivilege *  In       *
// *    is a pointer to parse node containing the data for the CREATE          *
// *  COMPONENT PRIVILEGE command.                                             *
// *                                                                           *
// *****************************************************************************
static void createSeabaseComponentOperation(
   const std::string & systemCatalog,
   StmtDDLCreateComponentPrivilege *pParseNode)
   
{

std::string privMDLoc(systemCatalog);

   privMDLoc += ".\"";
   privMDLoc += SEABASE_PRIVMGR_SCHEMA;
   privMDLoc += "\"";
   
PrivMgrCommands componentOperations(privMDLoc,CmpCommon::diags());

   if (!componentOperations.isAuthorizationEnabled())
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_AUTHORIZATION_NOT_ENABLED);
      return;
   }
   
const std::string componentName = pParseNode->getComponentName().data();
const std::string operationName = pParseNode->getComponentPrivilegeName().data();
const std::string operationCode = pParseNode->getComponentPrivilegeAbbreviation().data();
bool isSystem = pParseNode->isSystem();
const std::string operationDescription = pParseNode->getComponentPrivilegeDetailInformation().data();

PrivStatus retcode = STATUS_GOOD;

   retcode = componentOperations.createComponentOperation(componentName,
                                                          operationName,
                                                          operationCode,
                                                          isSystem,
                                                          operationDescription);
           
   if (retcode == STATUS_ERROR && 
       CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
      SEABASEDDL_INTERNAL_ERROR("CREATE COMPONENT PRIVILEGE command");
   
}
//****************** End of createSeabaseComponentOperation ********************

// *****************************************************************************
// *                                                                           *
// * Function: dropSeabaseComponentOperation                                   *
// *                                                                           *
// *   This functions handles the DROP COMPONENT PRIVILEGE command.            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <systemCatalog>                 const std::string &             In       *
// *    is catalog where system tables reside.                                 *
// *                                                                           *
// *  <pParseNode>                    StmtDDLDropComponentPrivilege * In       *
// *    is a pointer to parse node containing the data for the DROP            *
// *  COMPONENT PRIVILEGE command.                                             *
// *                                                                           *
// *****************************************************************************
static void dropSeabaseComponentOperation(
   const std::string & systemCatalog,
   StmtDDLDropComponentPrivilege *pParseNode)
   
{

std::string privMDLoc(systemCatalog);

   privMDLoc += ".\"";
   privMDLoc += SEABASE_PRIVMGR_SCHEMA;
   privMDLoc += "\"";
   
PrivMgrCommands componentOperations(privMDLoc,CmpCommon::diags());
  
   if (!componentOperations.isAuthorizationEnabled())
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_AUTHORIZATION_NOT_ENABLED);
      return;
   }
  
const std::string componentName = pParseNode->getComponentName().data();
const std::string operationName = pParseNode->getComponentPrivilegeName().data();

// Convert from SQL enums to PrivMgr enums.
PrivDropBehavior privDropBehavior;

   if (pParseNode->getDropBehavior() == COM_CASCADE_DROP_BEHAVIOR)
      privDropBehavior = PrivDropBehavior::CASCADE;
   else
      privDropBehavior = PrivDropBehavior::RESTRICT;

PrivStatus retcode = STATUS_GOOD;

   retcode = componentOperations.dropComponentOperation(componentName,
                                                        operationName,
                                                        privDropBehavior);
           
   if (retcode == STATUS_ERROR && 
       CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
      SEABASEDDL_INTERNAL_ERROR("DROP COMPONENT PRIVILEGE command");
   
}
//******************* End of dropSeabaseComponentOperation *********************



// *****************************************************************************
// *                                                                           *
// * Function: grantRevokeSeabaseRole                                          *
// *                                                                           *
// *   This function handles the GRANT ROLE and REVOKE ROLE commands.          *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <systemCatalog>              const std::string &                In       *
// *    is catalog where system tables reside.                                 *
// *                                                                           *
// *  <pParseNode>                 StmtDDLGrantRole *                 In       *
// *    is a pointer to parse node containing the data for the GRANT           *
// *  ROLE or REVOKE ROLE command.                                             *
// *                                                                           *
// *****************************************************************************
static void grantRevokeSeabaseRole(
   const std::string & systemCatalog,
   StmtDDLRoleGrant *pParseNode)
   
{

std::string privMDLoc(systemCatalog);

   privMDLoc += ".\"";
   privMDLoc += SEABASE_PRIVMGR_SCHEMA;
   privMDLoc += "\"";
   
PrivMgrCommands roleCommand(privMDLoc,CmpCommon::diags());

   if (!roleCommand.isAuthorizationEnabled())
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_AUTHORIZATION_NOT_ENABLED);
      return;
   }
      
// *****************************************************************************
// *                                                                           *
// *   The GRANT ROLE and REVOKE ROLE commands each take a list of roles       *
// * and a list of grantees (authorization names to grant the role to).        *
// * All items on both lists need to be verified for existence and no          *
// * duplication.  The results are stored in two parallel name/ID vectors.     *
// *                                                                           *
// *   Currently roles may only be granted to users, and may not be granted    *
// * to PUBLIC, so some code takes shortcuts and assumes users, while other    *
// * code is prepared for eventually supporting all authorization types.       *
// *                                                                           *
// *****************************************************************************
      
// *****************************************************************************
// *                                                                           *
// *  By default, the user issuing the GRANT or REVOKE ROLE command is         *
// * the grantor.  However, if the GRANTED BY clause is specified,             *
// * that authorization ID is the grantor.                                     *
// *                                                                           *
// *    If the GRANTED BY clause is NOT specified, and the user is             *
// * DB__ROOT, then the GRANT/REVOKE is assumed to have been                   *
// * issued by the owner/creator of the role.  So if no GRANTED BY             *
// * clause and grantor is DB__ROOT, note it, so we can look for the           *
// * role creator later.                                                       *
// *                                                                           *
// *****************************************************************************

int32_t grantorID = ComUser::getCurrentUser();
std::string grantorName;
bool grantorIsRoot = false;

ElemDDLGrantee *grantedBy = pParseNode->getGrantedBy();

   if (grantedBy != NULL)
   {
      if (grantorID != ComUser::getRootUserID())
      {
         PrivMgrComponentPrivileges componentPrivileges(privMDLoc,CmpCommon::diags());
         
         if (!componentPrivileges.hasSQLPriv(grantorID,
                                             SQLOperation::MANAGE_ROLES,
                                             true))
         {
            *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
            return;
         }
      }
   
      // BY clause specified.  Determine the grantor
      ComString grantedByName = grantedBy->getAuthorizationIdentifier();
      // TODO: Only works for users.  For roles, need a more generic 
      // authNameToAuthID.  Also, for users other than DB__ROOT, can only
      // specify a role if role has been granted to the user.  
      
      //TODO: Both authorization checks could be hardcoded for DB__ROOT or 
      // could rely on a COMPONENT PRIVILEGE, or both.
      // Give that this is a GRANT of a COMPONENT PRIVILEGE, could be an issue
      // if DB__ROOT is not automatic.
      
      //TODO: Finally, there is CURRENT USER syntax that means nothing other
      // than use the CURRENT USER (the default!), but that may need to be
      // supported for scripts.   
      
      //TODO: will need to update this if grant role to role is supported,
      // i.e., the granted by could be a role. getUserIDFromUserName() only
      // supports users.       

      if (ComUser::getUserIDFromUserName(grantedByName.data(),grantorID) != 0)
      {
         *CmpCommon::diags() << DgSqlCode(-CAT_AUTHID_DOES_NOT_EXIST_ERROR)
                             << DgString0(grantedByName.data());
         return;
      }
      grantorName = grantedByName.data();
   }  // grantedBy not null
   else
      if (grantorID == ComUser::getRootUserID())
      {
         grantorName = ComUser::getRootUserName();
         grantorIsRoot = true;
      }
      else
      {
         char GrantorNameString[MAX_DBUSERNAME_LEN + 1];
         int32_t length;
         
         Int16 retCode = ComUser::getAuthNameFromAuthID(grantorID,GrantorNameString,
                                               sizeof(GrantorNameString),length);
         if (retCode != 0)
            SEABASEDDL_INTERNAL_ERROR("Current user not registered");

         grantorName = GrantorNameString;
      }  
      
// *****************************************************************************
// *                                                                           *
// *   Next, walk through the list of roles being granted, making sure         *
// * each one exists and none appear more than once.  For each role,           *
// * if the grantor is DB__ROOT, determine the creator of the role and         *
// * use that data for the entries in the grantor vectors.                     *
// *                                                                           *
// *****************************************************************************

ElemDDLGranteeArray & roles = pParseNode->getRolesArray();

std::vector<int32_t> grantorIDs;
std::vector<std::string> grantorNames;
std::vector<int32_t> roleIDs;
std::vector<std::string> roleNames;

   for (size_t r = 0; r < roles.entries(); r++)
   {
      ComString roleName(roles[r]->getAuthorizationIdentifier());
      CmpSeabaseDDLrole roleInfo;
      int32_t roleID;
      
      // See if role exists
      if (!roleInfo.getRoleIDFromRoleName(roleName.data(),roleID))
      {
         *CmpCommon::diags() << DgSqlCode(-CAT_ROLE_NOT_EXIST)
                             << DgString0(roleName.data());
         return;
      }
      
      // See if this role has already been specified.
      if (hasValue(roleIDs,roleID))
      {
         *CmpCommon::diags() << DgSqlCode(-CAT_DUPLICATE_ROLES_IN_LIST)
                             << DgString0(roleName.data());
         return;
      }
      
      roleIDs.push_back(roleID);
      roleNames.push_back(roleName.data());
      
      // If grantor is DB__ROOT, substitute the role creator as the grantor.
      if (grantorIsRoot)
      {
         grantorIDs.push_back(roleInfo.getAuthCreator());
         
         char GrantorNameString[MAX_DBUSERNAME_LEN + 1];
         int32_t length;
         
         Int16 retCode = ComUser::getAuthNameFromAuthID(roleInfo.getAuthCreator(),
                                                        GrantorNameString,
                                                        sizeof(GrantorNameString),
                                                        length);
         
         if (retCode != 0)
            SEABASEDDL_INTERNAL_ERROR("Role administrator not registered");

         grantorNames.push_back(GrantorNameString);
      }
      else
      {
         grantorIDs.push_back(grantorID);     
         grantorNames.push_back(grantorName);
      }
   }
   
// *****************************************************************************
// *                                                                           *
// *   Now, walk throught the list of grantees, making sure they all exist     *
// * and none appear more than once.                                           *
// *                                                                           *
// *****************************************************************************

ElemDDLGranteeArray & grantees = pParseNode->getGranteeArray();
std::vector<int32_t> granteeIDs;
std::vector<std::string> granteeNames;
std::vector<PrivAuthClass> granteeClasses;

   for (size_t g = 0; g < grantees.entries(); g++)
   {
      int32_t granteeID;
      ComString granteeName(grantees[g]->getAuthorizationIdentifier());
      
      //TODO: the parser goes through a lot of work to segregrate PUBLIC from
      // other grantees, requiring more work here.  Could be simplified.
      // Note, _SYSTEM is not separated, but is included with other
      // grantee names.  
      if (grantees[g]->isPublic())
      {
         granteeID = ComUser::getPublicUserID();
         granteeName = ComUser::getPublicUserName();
      }
      else
      {
         granteeName = grantees[g]->getAuthorizationIdentifier();
         
         Int16 retCode = ComUser::getUserIDFromUserName(granteeName.data(),granteeID);
         //TODO: API only supports up/down on "is a user."  Could be a role
         // or PUBLIC.  Instead of "name does not exist" we could say 
         // "name is not a user" or "Roles can only be granted to users, name is 
         // not a user".  If support is added for granting to roles, a new 
         // API is needed.
         if (retCode != 0)
         {
            *CmpCommon::diags() << DgSqlCode(-CAT_USER_NOT_EXIST)
                                << DgString0(granteeName.data());
            return;
         }
      }
      
      // See if the grantee has already been specified.
      if (hasValue(granteeIDs,granteeID))
      {
         *CmpCommon::diags() << DgSqlCode(-CAT_DUPLICATE_USERS_IN_LIST)
                             << DgString0(granteeName.data());
         return;
      }
      granteeIDs.push_back(granteeID);
      granteeNames.push_back(granteeName.data());
      granteeClasses.push_back(PrivAuthClass::USER);
   }
   
// *****************************************************************************
// *                                                                           *
// *   The WITH ADMIN option means the grantee can grant the role to another   *
// * authorization ID.  In the case of REVOKE, this ability (but not the role  *
// * itself) is being taken from the grantee.                                  *
// *                                                                           *
// *****************************************************************************

int32_t grantDepth = 0;
bool withAdminOptionSpecified = false;

   if (pParseNode->isWithAdminOptionSpecified())
   {
      if (pParseNode->isGrantRole())
         grantDepth = -1;
      withAdminOptionSpecified = true;
   }
   
// *****************************************************************************
// *                                                                           *
// *   For REVOKE ROLE, the operation can either be RESTRICT, i.e. restrict    *
// * the command if any dependencies exist or CASCADE, in which case any       *
// * dependencies are silently removed.  Currently only RESTRICT is supported. *
// *                                                                           *
// *****************************************************************************

PrivDropBehavior privDropBehavior = PrivDropBehavior::RESTRICT;

   if (pParseNode->getDropBehavior() == COM_CASCADE_DROP_BEHAVIOR)
      privDropBehavior = PrivDropBehavior::CASCADE;
   else
      privDropBehavior = PrivDropBehavior::RESTRICT;
      
PrivStatus privStatus = STATUS_GOOD;
std::string commandString;

   if (pParseNode->isGrantRole())
   {
      commandString = "GRANT ROLE";
      privStatus = roleCommand.grantRole(roleIDs,
                                         roleNames,
                                         grantorIDs,
                                         grantorNames,
                                         PrivAuthClass::USER,
                                         granteeIDs,
                                         granteeNames,                                 
                                         granteeClasses,                               
                                         grantDepth);                                   
   }
   else
   {
      commandString = "REVOKE ROLE";
      privStatus = roleCommand.revokeRole(roleIDs,
                                          granteeIDs,
                                          granteeClasses,
                                          grantorIDs,
                                          withAdminOptionSpecified, 
                                          grantDepth,
                                          privDropBehavior);
   }
    
   if (privStatus == STATUS_ERROR && 
       CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
   {
      commandString += " command";
      SEABASEDDL_INTERNAL_ERROR(commandString.c_str());
   }
   
}
//********************** End of grantRevokeSeabaseRole *************************




// *****************************************************************************
// *                                                                           *
// * Function: grantSeabaseComponentPrivilege                                  *
// *                                                                           *
// *   This functions handles the GRANT COMPONENT PRIVILEGE command.           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <systemCatalog>              const std::string &                In       *
// *    is catalog where system tables reside.                                 *
// *                                                                           *
// *  <pParseNode>                 StmtDDLGrantComponentPrivilege *   In       *
// *    is a pointer to parse node containing the data for the GRANT           *
// *  COMPONENT PRIVILEGE command.                                             *
// *                                                                           *
// *****************************************************************************
static void grantSeabaseComponentPrivilege(
   const std::string & systemCatalog,
   StmtDDLGrantComponentPrivilege *pParseNode)
   
{

std::string privMDLoc(systemCatalog);

   privMDLoc += ".\"";
   privMDLoc += SEABASE_PRIVMGR_SCHEMA;
   privMDLoc += "\"";
   
PrivMgrCommands componentPrivileges(privMDLoc,CmpCommon::diags());
  
   if (!componentPrivileges.isAuthorizationEnabled())
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_AUTHORIZATION_NOT_ENABLED);
      return;
   }
  
const std::string componentName = pParseNode->getComponentName().data();
const ConstStringList & privList = pParseNode->getComponentPrivilegeNameList();

const NAString & granteeName = pParseNode->getUserRoleName(); 
int32_t granteeID;

   if (ComUser::getAuthIDFromAuthName(granteeName.data(),granteeID) != 0)
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_AUTHID_DOES_NOT_EXIST_ERROR)
                          << DgString0(granteeName.data());
      return;
   }

int32_t grantorID = ComUser::getCurrentUser();
std::string grantorName;

ElemDDLGrantee *grantedBy = pParseNode->getGrantedBy();

   if (grantedBy != NULL)
   {
      if (grantorID != ComUser::getRootUserID())
      {
         *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
         return;
      }
      
      // BY clause specified.  Determine the grantor
      ComString grantedByName = grantedBy->getAuthorizationIdentifier();
      // TODO: Only works for users.  For roles, need a more generic 
      // authNameToAuthID.  Also, for users other than DB__ROOT, can only
      // specify a role if role has been granted to the user.  
      
      //TODO: Both authorization checks could be hardcoded for DB__ROOT or 
      // could rely on a COMPONENT PRIVILEGE, or both.
      // Give that this is a GRANT of a COMPONENT PRIVILEGE, could be an issue
      // if DB__ROOT is not automatic.
      
      //TODO: Finally, there is CURRENT USER syntax that means nothing other
      // than use the CURRENT USER (the default!), but that may need to be
      // supported for scripts.         

      if (ComUser::getUserIDFromUserName(grantedByName.data(),grantorID) != 0)
      {
         *CmpCommon::diags() << DgSqlCode(-CAT_AUTHID_DOES_NOT_EXIST_ERROR)
                             << DgString0(grantedByName.data());
         return;
      }
      
      grantorName = grantedByName.data();
      
      //TODO: Cannot grant to _SYSTEM.  PUBLIC ok?
    
   }  // grantedBy not null
   else
   {
      char username[MAX_USERNAME_LEN + 1];
      Int32 maxLen = sizeof(username);
      Int32 actualLen;
      
      Int16 status = ComUser::getUserNameFromUserID(grantorID,username,
                                                    maxLen,actualLen);
      
      if (status != FEOK)
         *CmpCommon::diags() << DgSqlCode(-20235)
                             << DgInt0(status)
                             << DgInt1(grantorID);
      grantorName = username;
   }

int32_t grantDepth = 0;

   if (pParseNode->isWithGrantOptionSpecified())
      grantDepth = -1;
      
vector<std::string> operationNamesList;

   for (size_t i = 0; i < privList.entries(); i++)
   {
      const ComString * operationName = privList[i];
      operationNamesList.push_back(operationName->data());
   }   

PrivStatus retcode = STATUS_GOOD;

   retcode = componentPrivileges.grantComponentPrivilege(componentName,
                                                         operationNamesList,
                                                         grantorID,
                                                         grantorName,
                                                         granteeID,
                                                         granteeName.data(),
                                                         grantDepth);
           
   if (retcode == STATUS_ERROR && 
       CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
      SEABASEDDL_INTERNAL_ERROR("GRANT COMPONENT PRIVILEGE command");
   
}
//****************** End of grantSeabaseComponentPrivilege *********************


// *****************************************************************************
// *                                                                           *
// * Function: hasValue                                                        *
// *                                                                           *
// *   This function determines if a vector contains a value.                  *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <container>                  std::vector<int32_t>               In       *
// *    is the vector of 32-bit values.                                        *
// *                                                                           *
// *  <value>                      int32_t                            In       *
// *    is the value to be compared against existing values in the vector.     *
// *                                                                           *
// *****************************************************************************
static bool hasValue(
   std::vector<int32_t> container,
   int32_t value)
   
{

   for (size_t index = 0; index < container.size(); index++)
      if (container[index] == value)
         return true;
         
   return false;
   
}
//***************************** End of hasValue ********************************


// *****************************************************************************
// *                                                                           *
// * Function: CmpSeabaseDDL::registerSeabaseComponent                         *
// *                                                                           *
// *    This function handles register (adding) and unregister (drop) of       *
// *  components known to the Privilege Manager.                               *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <pParseNode>                    StmtDDLRegisterComponent *      In       *
// *    is a pointer to parse node containing the data for the REGISTER or     *
// *  UNREGISTER COMPONENT command.                                            *
// *                                                                           *
// *****************************************************************************
void CmpSeabaseDDL::registerSeabaseComponent(StmtDDLRegisterComponent *pParseNode)
{

  std::string privMDLoc = getSystemCatalog();
  privMDLoc += ".\"";
  privMDLoc += SEABASE_PRIVMGR_SCHEMA;
  privMDLoc += "\"";
  PrivMgrCommands component(privMDLoc,CmpCommon::diags());
  
   if (!component.isAuthorizationEnabled())
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_AUTHORIZATION_NOT_ENABLED);
      return;
   }
  
const std::string componentName = pParseNode->getExternalComponentName().data();
  
PrivStatus retcode = STATUS_GOOD;

   switch (pParseNode->getRegisterComponentType())
   {
      case StmtDDLRegisterComponent::REGISTER_COMPONENT:
      {
         const NAString details = pParseNode->getRegisterComponentDetailInfo();
         bool isSystem = pParseNode->isSystem();
         const std::string componentDetails = details.data();
         retcode = component.registerComponent(componentName,isSystem,componentDetails);
         break;
      }
      case StmtDDLRegisterComponent::UNREGISTER_COMPONENT:
      {
         PrivDropBehavior privDropBehavior;

         if (pParseNode->getDropBehavior() == COM_CASCADE_DROP_BEHAVIOR)
            privDropBehavior = PrivDropBehavior::CASCADE;
         else
            privDropBehavior = PrivDropBehavior::RESTRICT;
         
         retcode = component.unregisterComponent(componentName,privDropBehavior);
      } 
         break;
      default:
         retcode = STATUS_ERROR; 
   }
  
   if (retcode == STATUS_ERROR && 
       CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
      SEABASEDDL_INTERNAL_ERROR("REGISTER/UNREGISTER COMPONENT command");

}

//************** End of CmpSeabaseDDL::registerSeabaseComponent ****************


// *****************************************************************************
// *                                                                           *
// * Function: revokeSeabaseComponentPrivilege                                 *
// *                                                                           *
// *   This functions handles the REVOKE COMPONENT PRIVILEGE command.          *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <systemCatalog>              const std::string &                In       *
// *    is catalog where system tables reside.                                 *
// *                                                                           *
// *  <pParseNode>                 StmtDDLRevokeComponentPrivilege *  In       *
// *    is a pointer to parse node containing the data for the REVOKE          *
// *  COMPONENT PRIVILEGE command.                                             *
// *                                                                           *
// *****************************************************************************
static void revokeSeabaseComponentPrivilege(
   const std::string & systemCatalog,
   StmtDDLRevokeComponentPrivilege *pParseNode)
   
{

std::string privMDLoc(systemCatalog);

   privMDLoc += ".\"";
   privMDLoc += SEABASE_PRIVMGR_SCHEMA;
   privMDLoc += "\"";
   
PrivMgrCommands componentPrivileges(privMDLoc,CmpCommon::diags());
  
   if (!componentPrivileges.isAuthorizationEnabled())
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_AUTHORIZATION_NOT_ENABLED);
      return;
   }
  
const std::string componentName = pParseNode->getComponentName().data();
const ConstStringList & privList = pParseNode->getComponentPrivilegeNameList();

const NAString & granteeName = pParseNode->getUserRoleName(); 
int32_t granteeID;

   if (ComUser::getAuthIDFromAuthName(granteeName.data(),granteeID) != 0)
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_AUTHID_DOES_NOT_EXIST_ERROR)
                          << DgString0(granteeName.data());
      return;
   }

int32_t grantorID = ComUser::getCurrentUser();

ElemDDLGrantee *grantedBy = pParseNode->getGrantedBy();

   if (grantedBy != NULL)
   {
      if (grantorID != ComUser::getRootUserID())
      {
         *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
         return;
      }
      
      // BY clause specified.  Determine the grantor
      ComString grantedByName = grantedBy->getAuthorizationIdentifier();
      // TODO: Only works for users.  For roles, need a more generic 
      // authNameToAuthID.  Also, for users other than DB__ROOT, can only
      // specify a role if role has been granted to the user.  
      
      //TODO: Both authorization checks could be hardcoded for DB__ROOT or 
      // could rely on a COMPONENT PRIVILEGE, or both.
      
      //TODO: Finally, there is CURRENT USER syntax that means nothing other
      // than use the CURRENT USER (the default!), but that may need to be
      // supported for scripts.         

      if (ComUser::getUserIDFromUserName(grantedByName.data(),grantorID) != 0)
      {
         *CmpCommon::diags() << DgSqlCode(-CAT_AUTHID_DOES_NOT_EXIST_ERROR)
                             << DgString0(grantedByName.data());
         return;
      }
    
 }  // grantedBy not null

bool isGOFSpecified = false;

   if (pParseNode->isGrantOptionForSpecified())
      isGOFSpecified = true;
      
vector<std::string> operationNamesList;

   for (size_t i = 0; i < privList.entries(); i++)
   {
      const ComString * operationName = privList[i];
      operationNamesList.push_back(operationName->data());
   }   

PrivStatus retcode = STATUS_GOOD;
PrivDropBehavior dropBehavior = PrivDropBehavior::RESTRICT; 

   retcode = componentPrivileges.revokeComponentPrivilege(componentName,
                                                          operationNamesList,
                                                          grantorID,
                                                          granteeID,
                                                          isGOFSpecified,
                                                          dropBehavior);
   if (retcode == STATUS_ERROR && 
       CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
      SEABASEDDL_INTERNAL_ERROR("REVOKE COMPONENT PRIVILEGE command");
   
}
//****************** End of revokeSeabaseComponentPrivilege ********************



short 
CmpSeabaseDDL::setupHbaseOptions(ElemDDLHbaseOptions * hbaseOptionsClause,
                                 Int32 numSplits, const NAString& objName,
                                 NAList<HbaseCreateOption*>& hbaseCreateOptions,
                                 NAString& hco)
{
  NAText hbaseOptionsStr;
  NABoolean maxFileSizeOptionSpecified = FALSE;
  NABoolean splitPolicyOptionSpecified = FALSE;
  const char *maxFileSizeOptionString = "MAX_FILESIZE";
  const char *splitPolicyOptionString = "SPLIT_POLICY";

  Lng32 numHbaseOptions = 0;
  if (hbaseOptionsClause)
  {
    for (CollIndex i = 0; i < hbaseOptionsClause->getHbaseOptions().entries(); 
         i++)
    {
      HbaseCreateOption * hbaseOption =
        hbaseOptionsClause->getHbaseOptions()[i];

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

    numHbaseOptions += hbaseOptionsClause->getHbaseOptions().entries();
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
      hbaseOption = new(STMTHEAP) 
        HbaseCreateOption("MAX_FILESIZE", fileSizeOption);
      hbaseCreateOptions.insert(hbaseOption);

      if (ActiveSchemaDB()->getDefaults().userDefault(
               HBASE_SALTED_TABLE_MAX_FILE_SIZE) == TRUE)
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

      if (ActiveSchemaDB()->getDefaults().userDefault(
               HBASE_SALTED_TABLE_SET_SPLIT_POLICY) == TRUE)
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
  if  (hbaseOptionsStr.size() > 0)
  {
    hco += "HBASE_OPTIONS=>";

    char hbaseOptionsNumCharStr[HBASE_OPTION_MAX_INTEGER_LENGTH];
    sprintf(hbaseOptionsNumCharStr, "%04d", numHbaseOptions);
    hco += hbaseOptionsNumCharStr;

    hco += hbaseOptionsStr.data();
      
    hco += " "; // separator
  }
  
  if (numSplits > 0)
  {
    char splitNumCharStr[HBASE_OPTION_MAX_INTEGER_LENGTH];
    sprintf(splitNumCharStr, "%04d", numSplits+1);
 
    hco += "NUM_SALT_PARTNS=>";
    hco+=splitNumCharStr;
                 
    hco += " "; // separator
  }

  if (hco.length() > HBASE_OPTIONS_MAX_LENGTH)
  {
    *CmpCommon::diags() << DgSqlCode(-CAT_INVALID_HBASE_OPTIONS_CLAUSE)
                        << DgString0(objName);
    return -1 ;
  }
  return 0;
}

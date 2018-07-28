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
 * File:         CmpSeabaseDDLcleanup.cpp
 * Description:  Implements cleanup of metadata.
 *
 *
 * Created:    
 * Language:     C++
 *
 *
 *****************************************************************************
 */


#include "CmpSeabaseDDLincludes.h"
#include "CmpDDLCatErrorCodes.h"
#include "CmpSeabaseDDLcleanup.h"
#include "ExpLOB.h"

//////////////////////////////////////////////////////////////////////////
// Methods related to cleanup of objects from metadata.
//////////////////////////////////////////////////////////////////////////
CmpSeabaseMDcleanup::CmpSeabaseMDcleanup(NAHeap *heap)
  : CmpSeabaseDDL(heap),
    objUID_(-1),
    objectOwner_(-1),
    indexesUIDlist_(NULL),
    uniqueConstrUIDlist_(NULL),
    refConstrUIDlist_(NULL),
    seqUIDlist_(NULL),
    usingViewsList_(NULL),
    numLOBs_(0),
    lobNumList_(NULL),
    lobTypList_(NULL),
    lobLocList_(NULL),
    stopOnError_(FALSE),
    cleanupMetadataEntries_(FALSE),
    checkOnly_(FALSE),
    returnDetails_(FALSE),
    currReturnEntry_(0),
    numOrphanMetadataEntries_(0),
    numOrphanHbaseEntries_(0),
    numOrphanObjectsEntries_(0),
    numOrphanViewsEntries_(0),
    numInconsistentHiveEntries_(0),
    isHive_(FALSE)
{};

Int64 CmpSeabaseMDcleanup::getCleanupObjectUID(
                                               ExeCliInterface *cliInterface,
                                               const char * catName,
                                               const char * schName,
                                               const char * objName,
                                               const char * inObjType,
                                               char * outObjType,
                                               Int32 &objectOwner)
{
  Lng32 cliRC = 0;
  Int64 objUID = -1;
  objectOwner = -1;

  ExeCliInterface cqdCliInterface(STMTHEAP);
 
  // find object uid of this object from OBJECTS table
  char shapeBuf[1000];
  str_sprintf(shapeBuf, "control query shape scan (path '%s.\"%s\".%s')",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS) ;
  if (cqdCliInterface.setCQS(shapeBuf))
    {
      cqdCliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }
 
  objUID = getObjectUID(cliInterface, catName, schName, objName, 
                        (strcmp(inObjType, COM_UNKNOWN_OBJECT_LIT) == 0 ? NULL : inObjType),
                        NULL, outObjType, FALSE);
  cqdCliInterface.resetCQS();
  if (objUID != -1) // object exists
    {
      // find owner
      cliRC = getObjectOwner(cliInterface, catName, schName, objName, 
                             (strcmp(inObjType, COM_UNKNOWN_OBJECT_LIT) == 0 ? NULL : inObjType),
                             &objectOwner);
      if (cliRC < 0)
        {
          // error trying to get owner. Ignore it and return invalid owner.
          CmpCommon::diags()->clear();          
          objectOwner = -1;
        }

      return objUID;
    }

  // didnt find it in OBJECTS table. Look for it in OBJECTS_UNIQ_IDX table
  CmpCommon::diags()->clear();
  objectOwner = -1; // index does not contain owner info.
  objUID = getObjectUID(cliInterface, catName, schName, objName, 
                        (strcmp(inObjType, COM_UNKNOWN_OBJECT_LIT) == 0 ? NULL : inObjType),
                        NULL, outObjType, TRUE);
  if (objUID != -1)
    return objUID;

  // find object_uid of missing table by doing a select on COLUMNS with
  // the specified column name and check that the corresponding object_uid 
  // doesn't exist in OBJECTS table
  // *** TBD  ***

  return -1;
}
  
short CmpSeabaseMDcleanup::getCleanupObjectNameAndType(
                                                ExeCliInterface *cliInterface,
                                                Int64 objUID,
                                                NAString &catName,
                                                NAString &schName,
                                                NAString &objName,
                                                NAString &objType,
                                                Int32 &objectOwner)
{
  Lng32 cliRC = 0;
  char objTypeBuf[10];

  objectOwner = -1;

  // look in objects idx
  cliRC = getObjectName(cliInterface, objUID, catName, schName, objName, objTypeBuf, 
                        FALSE, TRUE);
  if ((cliRC < 0) &&
      (cliRC != -1389))
    return -1;

  if (cliRC == -1389) // not found
    {
      CmpCommon::diags()->clear();

      // look in objects table
      cliRC = getObjectName(cliInterface, objUID, catName, schName, objName, objTypeBuf, 
                            TRUE, FALSE);
      if ((cliRC < 0) &&
          (cliRC != -1389))
        return -1;
      
      if (cliRC == -1389) // not found
        {
          CmpCommon::diags()->clear();
          objName = "";
        }
    }

  if (cliRC == -1389) // not found
    {
      // assume object is base table type.
      objType = COM_BASE_TABLE_OBJECT_LIT;
    }
  else
    {
      // found this object id
      objType = objTypeBuf;

      // find the owner
      cliRC = getObjectOwner(cliInterface, catName, schName, objName, 
                             objType,
                             &objectOwner);
      if (cliRC < 0)
        {
          // error trying to get owner or not found. Ignore it and return invalid owner.
          CmpCommon::diags()->clear();          
          objectOwner = -1;
        }
      
    }

  return 0;
}

short CmpSeabaseMDcleanup::validateInputValues(
                                               StmtDDLCleanupObjects * stmtCleanupNode,
                                               ExeCliInterface *cliInterface)
{
  Lng32 cliRC;

  objUID_ = -1;
  objectOwner_ = -1;
  objType_ = "";

  NAString inObjType;
  if (stmtCleanupNode->getType() == StmtDDLCleanupObjects::TABLE_)
    inObjType = COM_BASE_TABLE_OBJECT_LIT;
  else if (stmtCleanupNode->getType() == StmtDDLCleanupObjects::INDEX_)
    inObjType = COM_INDEX_OBJECT_LIT;
  else if (stmtCleanupNode->getType() == StmtDDLCleanupObjects::SEQUENCE_)
    inObjType = COM_SEQUENCE_GENERATOR_OBJECT_LIT;
  else if (stmtCleanupNode->getType() == StmtDDLCleanupObjects::VIEW_)
    inObjType = COM_VIEW_OBJECT_LIT;
  else if (stmtCleanupNode->getType() == StmtDDLCleanupObjects::SCHEMA_PRIVATE_)
    inObjType = COM_PRIVATE_SCHEMA_OBJECT_LIT;
  else if (stmtCleanupNode->getType() == StmtDDLCleanupObjects::SCHEMA_SHARED_)
    inObjType = COM_SHARED_SCHEMA_OBJECT_LIT;
  else if (stmtCleanupNode->getType() == StmtDDLCleanupObjects::UNKNOWN_)
    inObjType = COM_UNKNOWN_OBJECT_LIT;

  if (NOT inObjType.isNull())
    {
      QualifiedName *qn = stmtCleanupNode->getTableNameAsQualifiedName();
      catName_ = qn->getCatalogName();
      schName_ = qn->getSchemaName();
      objName_ = qn->getObjectName();

      char outObjType[10];
      objUID_ = getCleanupObjectUID(cliInterface,
                                    catName_.data(), schName_.data(), objName_.data(),
                                    inObjType.data(),
                                    outObjType, objectOwner_);
      if ((objUID_ == -1) && // not found
          (inObjType != COM_UNKNOWN_OBJECT_LIT &&
           inObjType != COM_PRIVATE_SCHEMA_OBJECT_LIT &&
           inObjType != COM_SHARED_SCHEMA_OBJECT_LIT)) // type explicitly specified
        {
          // check if there is another object type with the same name.
          objUID_ = getCleanupObjectUID(cliInterface,
                                        catName_.data(), schName_.data(), objName_.data(),
                                        COM_UNKNOWN_OBJECT_LIT,
                                        outObjType, objectOwner_);
          if ((objUID_ != -1) &&// found it
              (inObjType != outObjType))
            {
              *CmpCommon::diags() << DgSqlCode(-4256) ;
              return -1;
            }
        }

      if ((objUID_ != -1) && 
          (stmtCleanupNode->getObjectUID() != -1) &&
          (objUID_ != stmtCleanupNode->getObjectUID()))
        {
          *CmpCommon::diags() << DgSqlCode(-4253) ;
          return -1;
        }

      objType_ = "";
      if (inObjType == COM_UNKNOWN_OBJECT_LIT)
        {
          if (objUID_ ==  -1)
            {
              objType_ = COM_BASE_TABLE_OBJECT_LIT;
              objectOwner_ = -1;
            }
          else
            objType_ = outObjType;
        }
      else
        objType_ = inObjType;

      if ((objUID_ == -1) &&
          (stmtCleanupNode->getObjectUID() != -1))
        {
          CmpCommon::diags()->clear();
          objUID_ = stmtCleanupNode->getObjectUID();
        }
    }
  else if (stmtCleanupNode->getType() == StmtDDLCleanupObjects::OBJECT_UID_)
    {
      objUID_ = stmtCleanupNode->getObjectUID();

      cliRC = getCleanupObjectNameAndType(cliInterface, objUID_, 
                                          catName_, schName_, objName_,
                                          objType_, objectOwner_);
      if (cliRC < 0)
        return -1;

    }
  else if (stmtCleanupNode->getType() == StmtDDLCleanupObjects::OBSOLETE_)
    {
      cleanupMetadataEntries_ = TRUE;
      objectOwner_ = -1;
    }

  isHive_ = FALSE;
  if (catName_ == HIVE_SYSTEM_CATALOG)
    isHive_ = TRUE;

  // generate hbase name that will be used to drop underlying hbase object
  extNameForHbase_ = "";
  if ((NOT isHive_) &&
      ((objType_ == COM_BASE_TABLE_OBJECT_LIT) ||
       (objType_ == COM_INDEX_OBJECT_LIT)))
    {
      if (NOT (catName_.isNull() || schName_.isNull() || objName_.isNull()))
        {
          extNameForHbase_ = catName_ + "." + schName_ + "." + objName_;
        }
    }
  else if (isHive_)
    {
      if (NOT ((schName_.compareTo(HIVE_DEFAULT_SCHEMA_EXE, NAString::ignoreCase) == 0) ||
               (schName_.compareTo(HIVE_SYSTEM_SCHEMA, NAString::ignoreCase) == 0)))              
        extNameForHive_ = schName_ + ".";

      extNameForHive_ += objName_;
    }

  // Make sure user has necessary privileges to perform drop
  if (!isDDLOperationAuthorized(SQLOperation::DROP_TABLE,
                                objectOwner_,
                                -1))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
      
      return -1;
    }

  if (stmtCleanupNode->checkOnly())
    checkOnly_ = TRUE;

  if (stmtCleanupNode->returnDetails())
    returnDetails_ = TRUE;

  return 0;
}

short CmpSeabaseMDcleanup::processCleanupErrors(ExeCliInterface *cliInterface,
                                         NABoolean &errorSeen)
{
  if (cliInterface)
    cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
  
  if (stopOnError_)
    return -1;
  
  CmpCommon::diags()->negateAllErrors();
  
  errorSeen = TRUE;

  return 0;
}

short CmpSeabaseMDcleanup::gatherDependentObjects(ExeCliInterface *cliInterface)
{
  Lng32 cliRC = 0;
  char query[1000];

  NABoolean errorSeen = FALSE;
  if (objType_ == COM_BASE_TABLE_OBJECT_LIT)
    {
      // generate dependent index uid list
      str_sprintf(query, "select index_uid from %s.\"%s\".%s where base_table_uid = %ld",
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_INDEXES,
                  objUID_);
      
      indexesUIDlist_ = NULL;
      cliRC = cliInterface->fetchAllRows(indexesUIDlist_, query, 0, FALSE, FALSE, TRUE);
      if (cliRC < 0)
        {
          if (processCleanupErrors(cliInterface, errorSeen))
            return -1;
        }
  
      // generate unique constr list
     str_sprintf(query, "select constraint_uid from %s.\"%s\".%s where table_uid = %ld and constraint_type = 'U' ",
                 getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLE_CONSTRAINTS,
                  objUID_);
      
      uniqueConstrUIDlist_ = NULL;
      cliRC = cliInterface->fetchAllRows(uniqueConstrUIDlist_, query, 0, FALSE, FALSE, TRUE);
      if (cliRC < 0)
        {
          if (processCleanupErrors(cliInterface, errorSeen))
            return -1;
        }

      // generate ref constr list
     str_sprintf(query, "select constraint_uid from %s.\"%s\".%s where table_uid = %ld and constraint_type = 'F' ",
                 getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLE_CONSTRAINTS,
                  objUID_);
      
      refConstrUIDlist_ = NULL;
      cliRC = cliInterface->fetchAllRows(refConstrUIDlist_, query, 0, FALSE, FALSE, TRUE);
      if (cliRC < 0)
        {
          if (processCleanupErrors(cliInterface, errorSeen))
            return -1;
        }

      // generate sequences list for identity columns
      str_sprintf(query, "select seq_uid from %s.\"%s\".%s where seq_uid in (select object_uid from %s.\"%s\".%s where catalog_name = '%s' and schema_name = '%s' and object_name like '\\_%s\\_%s\\_%s\\_%%' escape '\\' and object_type = 'SG')",
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_SEQ_GEN,
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
                  catName_.data(), schName_.data(), 
                  catName_.data(), schName_.data(), objName_.data());
      
      seqUIDlist_ = NULL;
      cliRC = cliInterface->fetchAllRows(seqUIDlist_, query, 0, FALSE, FALSE, TRUE);
      if (cliRC < 0)
        {
          if (processCleanupErrors(cliInterface, errorSeen))
            return -1;
        }
      
      if (NOT extNameForHbase_.isNull())
        {
          // Base object name exists. Generate LOB info list
          lobMDNameBuf_ = new(STMTHEAP) char[1024];
          Lng32 lobMDNameLen = 1024;
          
          char * lobDescHandleObjNamePrefix = 
            ExpLOBoper::ExpGetLOBDescHandleObjNamePrefix(objUID_,
                                                         lobMDNameBuf_, lobMDNameLen);
          
          str_sprintf(query, "select object_name from %s.\"%s\".%s where catalog_name = '%s' and schema_name = '%s' and object_name like '%s%%' and object_type = 'BT' ",
                      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
                      getSystemCatalog(), schName_.data(), lobDescHandleObjNamePrefix);
          
          Queue *lobDescList = NULL;
          cliRC = cliInterface->fetchAllRows(lobDescList, query, 0, FALSE, FALSE, TRUE);
          if (cliRC < 0)
            {
              if (processCleanupErrors(cliInterface, errorSeen))
                return -1;
            }
          
          if (lobDescList->numEntries() > 0)
            {
              numLOBs_ = lobDescList->numEntries();
              lobNumList_ = new (STMTHEAP) short[numLOBs_];
              lobTypList_ = new (STMTHEAP) short[numLOBs_];
              lobLocList_ = new (STMTHEAP) char*[numLOBs_];
              
              lobDescList->position();
              for (size_t i = 0; i < lobDescList->numEntries(); i++)
                {
                  OutputInfo * oi = (OutputInfo*)lobDescList->getCurr(); 
                  char * name = (char*)oi->get(0);
                  
                  Lng32 lobNum = 
                    ExpLOBoper::ExpGetLOBnumFromDescName(name, strlen(name));
                  lobNumList_[i] = lobNum;
                  
                  lobTypList_[i] = Lob_HDFS_File;
                  
                  char * loc = new (STMTHEAP) char[1024];
                  const char* f = ActiveSchemaDB()->getDefaults().
                    getValue(LOB_STORAGE_FILE_DIR);
                  strcpy(loc, f);
                  lobLocList_[i] = loc;
                  
                  lobDescList->advance();
                }   
              
              lobMDName_ = (char*)
                ExpLOBoper::ExpGetLOBMDName(schName_.length(), (char*)schName_.data(), objUID_,
                                            lobMDNameBuf_, lobMDNameLen);
            }
        }
    } // COM_BASE_TABLE_OBJECT

  if ((objType_ == COM_BASE_TABLE_OBJECT_LIT) ||
      (objType_ == COM_VIEW_OBJECT_LIT))
    {
      // get views that use this object.
      cliRC = getUsingViews(cliInterface, objUID_, usingViewsList_);
      if (cliRC < 0)
        {
          if (processCleanupErrors(NULL, errorSeen))
            return -1;
        }
    }

  if (isHive_)
    {
      // if this hive table has an external table, get its uid
      NAString extTableName;
      extTableName = ComConvertNativeNameToTrafName(catName_, schName_, objName_);
      if (NOT extTableName.isNull())
        {
          QualifiedName qn(extTableName, 3);
          Int64 extObjUID = 
            getObjectUID(cliInterface, 
                         qn.getCatalogName(), qn.getSchemaName(), qn.getObjectName(),
                         COM_BASE_TABLE_OBJECT_LIT,
                         NULL, NULL, FALSE,
                         FALSE);
          if (extObjUID > 0)
            {
              char query[1000];
              str_sprintf(query, "cleanup uid %ld", extObjUID);
              cliRC = cliInterface->executeImmediate(query);
              if (cliRC < 0)
                {
                  cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
                  return -1;
                }
            }
        }
    }

  if (errorSeen)
    return -1;
  else
    return 0;
}

short CmpSeabaseMDcleanup::deleteMDentries(ExeCliInterface *cliInterface)
{
  Lng32 cliRC = 0;
  char query[1000];
  
  NABoolean errorSeen = FALSE;

  // OBJECTS table

  // Must first hide the index to OBJECTS, because the delete plan would otherwise
  // likely access OBJECTS_UNIQ_IDX first then join that to OBJECTS (as OBJECT_UID
  // is the leading part of the index key). If the index row were missing, we'd 
  // fail to delete the base table row. Right now OBJECTS is the only metadata
  // table with an index, so this is the only place we need to take this precaution.

  cliRC = cliInterface->holdAndSetCQD("HIDE_INDEXES","ALL", NULL);
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    } 

  // Now delete from OBJECTS (but not its index)

  str_sprintf(query, "delete from %s.\"%s\".%s where object_uid = %ld",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              objUID_);
  cliRC = cliInterface->executeImmediate(query);
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    }
 
  // Restore previous setting of CQD HIDE_INDEXES

  cliRC = cliInterface->restoreCQD("HIDE_INDEXES", NULL);
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    } 
  
  // OBJECTS index
  str_sprintf(query, "delete from table(index_table %s.\"%s\".%s) where \"OBJECT_UID@\" = %ld",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS_UNIQ_IDX,
              objUID_);
  cliRC = cliInterface->executeImmediate(query);
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    }
  
  // COLUMNS table
  str_sprintf(query, "delete from %s.\"%s\".%s where object_uid = %ld",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_COLUMNS,
              objUID_);
  cliRC = cliInterface->executeImmediate(query);
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    }

  // KEYS table
  str_sprintf(query, "delete from %s.\"%s\".%s where object_uid = %ld",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_KEYS,
              objUID_);
  cliRC = cliInterface->executeImmediate(query);
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    }

  // TABLES table
  str_sprintf(query, "delete from %s.\"%s\".%s where table_uid = %ld",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLES,
              objUID_);
  cliRC = cliInterface->executeImmediate(query);
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    }
  
  // delete index entries
  str_sprintf(query, "delete from %s.\"%s\".%s where base_table_uid = %ld",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_INDEXES,
              objUID_);
  cliRC = cliInterface->executeImmediate(query);
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    }

  // TEXT entries
  str_sprintf(query, "delete from %s.\"%s\".%s where text_uid = %ld",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TEXT,
              objUID_);
  cliRC = cliInterface->executeImmediate(query);
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    }

  if (errorSeen)
    return -1;
  else
    return 0;
}

short CmpSeabaseMDcleanup::deleteMDConstrEntries(ExeCliInterface *cliInterface)
{
  Lng32 cliRC = 0;
  char query[1000];
  
  if (uniqueConstrUIDlist_)
    {
      uniqueConstrUIDlist_->position();
      for (size_t i = 0; i < uniqueConstrUIDlist_->numEntries(); i++)
        {
          Int64 ucUID;
          
          OutputInfo * oi = (OutputInfo*)uniqueConstrUIDlist_->getCurr(); 
          ucUID = *(Int64*)oi->get(0);
          
          str_sprintf(query, "delete from %s.\"%s\".%s where unique_constraint_uid = %ld",
                      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_REF_CONSTRAINTS,
                      ucUID);
          cliRC = cliInterface->executeImmediate(query);
          if (cliRC < 0)
            {
              cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
              return -1;
            }
          
          str_sprintf(query, "delete from %s.\"%s\".%s where unique_constraint_uid = %ld",
                      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_UNIQUE_REF_CONSTR_USAGE,
                      ucUID);
          cliRC = cliInterface->executeImmediate(query);
          if (cliRC < 0)
            {
              cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
              return -1;
            }

          uniqueConstrUIDlist_->advance();
        } // for
    }

  if (refConstrUIDlist_)
    {
      refConstrUIDlist_->position();
      for (size_t i = 0; i < refConstrUIDlist_->numEntries(); i++)
        {
          Int64 rcUID;
          
          OutputInfo * oi = (OutputInfo*)refConstrUIDlist_->getCurr(); 
          rcUID = *(Int64*)oi->get(0);
          str_sprintf(query, "delete from %s.\"%s\".%s where ref_constraint_uid = %ld",
                      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_REF_CONSTRAINTS,
                      rcUID);
          cliRC = cliInterface->executeImmediate(query);
          if (cliRC < 0)
            {
              cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
              return -1;
            }
          
          str_sprintf(query, "delete from %s.\"%s\".%s where foreign_constraint_uid = %ld",
                      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_UNIQUE_REF_CONSTR_USAGE,
                      rcUID);
          cliRC = cliInterface->executeImmediate(query);
          if (cliRC < 0)
            {
              cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
              return -1;
            }

          refConstrUIDlist_->advance();
        } // for
    }
      
  str_sprintf(query, "delete from %s.\"%s\".%s where object_uid in (select constraint_uid from %s.\"%s\".%s where table_uid = %ld)",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLE_CONSTRAINTS,
              objUID_);
  cliRC = cliInterface->executeImmediate(query);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }
  
  str_sprintf(query, "delete from %s.\"%s\".%s where table_uid = %ld",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TABLE_CONSTRAINTS,
              objUID_);
  cliRC = cliInterface->executeImmediate(query);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  return 0;
}

short CmpSeabaseMDcleanup::deleteMDViewEntries(ExeCliInterface *cliInterface)
{
  Lng32 cliRC = 0;
  char query[1000];

  str_sprintf(query, "delete from %s.\"%s\".%s where view_uid = %ld",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_VIEWS,
              objUID_);
  cliRC = cliInterface->executeImmediate(query);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  str_sprintf(query, "delete from %s.\"%s\".%s where used_object_uid = %ld and used_object_type = '%s' ",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_VIEWS_USAGE,
              objUID_,
              objType_.data());
  cliRC = cliInterface->executeImmediate(query);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  return 0;
}

short CmpSeabaseMDcleanup::deleteHistogramEntries(ExeCliInterface *cliInterface)
{
  if (isHistogramTable(objName_))
    return 0;

  Lng32 cliRC = 0;
  char query[1000];

  if ((objType_ == COM_BASE_TABLE_OBJECT_LIT) &&
      (objUID_ > 0) &&
      (NOT catName_.isNull()) &&
      (NOT schName_.isNull()))
    {
      if (NOT isHive_)
        {
          if (dropSeabaseStats(cliInterface, catName_.data(), schName_.data(), objUID_))
            return -1;
        }
      else
        {
          if (dropSeabaseStats(cliInterface, HIVE_STATS_CATALOG, 
                               HIVE_STATS_SCHEMA_NO_QUOTES, objUID_))
            return -1;
        }
    }

  return 0;
}

short CmpSeabaseMDcleanup::dropIndexes(ExeCliInterface *cliInterface)
{
  Lng32 cliRC = 0;
  char query[1000];

  if ((! indexesUIDlist_) ||
      (indexesUIDlist_->numEntries() == 0))
    return 0;

  indexesUIDlist_->position();
  for (size_t i = 0; i < indexesUIDlist_->numEntries(); i++)
    {
      Int64 iUID;
      
      OutputInfo * oi = (OutputInfo*)indexesUIDlist_->getCurr(); 
      iUID = *(Int64*)oi->get(0);

      str_sprintf(query, "cleanup uid %ld",
                  iUID);
      cliRC = cliInterface->executeImmediate(query);
      if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
          return -1;
        }

      indexesUIDlist_->advance();
    }

  return 0;
}

short CmpSeabaseMDcleanup::dropSequences(ExeCliInterface *cliInterface)
{
  Lng32 cliRC = 0;
  char query[1000];

  if ((! seqUIDlist_) ||
      (seqUIDlist_->numEntries() == 0))
    return 0;

  seqUIDlist_->position();
  for (size_t i = 0; i < seqUIDlist_->numEntries(); i++)
    {
      Int64 iUID;
      
      OutputInfo * oi = (OutputInfo*)seqUIDlist_->getCurr(); 
      iUID = *(Int64*)oi->get(0);

      str_sprintf(query, "cleanup uid %ld",
                  iUID);
      cliRC = cliInterface->executeImmediate(query);
      if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
          return -1;
        }

      seqUIDlist_->advance();
    }

  return 0;
}

short CmpSeabaseMDcleanup::dropLOBs(ExeCliInterface *cliInterface)
{
  Lng32 cliRC = 0;
  char query[1000];

  if (objUID_ == -1)
    return 0;
  
  if (catName_.isNull() || schName_.isNull())
    return 0;

  if (! lobMDName_)
    return 0;
  NABoolean lobTrace=FALSE;
  if (getenv("TRACE_LOB_ACTIONS"))
    lobTrace=TRUE;
  NAString newSchName = "\"" + catName_ + "\"" + "." + "\"" + schName_ + "\"";
  const char *lobHdfsServer = CmpCommon::getDefaultString(LOB_HDFS_SERVER);
  Int32 lobHdfsPort = (Lng32)CmpCommon::getDefaultNumeric(LOB_HDFS_PORT);
  cliRC = SQL_EXEC_LOBddlInterface((char*)newSchName.data(),
                                   newSchName.length(),
                                   objUID_,
                                   numLOBs_,
                                   LOB_CLI_CLEANUP,
                                   lobNumList_,
                                   lobTypList_,
                                   lobLocList_,
                                   NULL,
                                   (char *)lobHdfsServer,
                                   lobHdfsPort,0,lobTrace);

  return 0;
}

short CmpSeabaseMDcleanup::dropUsingViews(ExeCliInterface *cliInterface)
{
  Lng32 cliRC = 0;
  char query[1000];

  if ((! usingViewsList_) ||
      (usingViewsList_->numEntries() == 0))
    return 0;

  usingViewsList_->position();
  for (size_t i = 0; i < usingViewsList_->numEntries(); i++)
    {
      OutputInfo * oi = (OutputInfo*)usingViewsList_->getCurr(); 
      char * viewName = (char*)oi->get(0);

      str_sprintf(query, "cleanup view %s ",
                  viewName);
      cliRC = cliInterface->executeImmediate(query);
      if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
          return -1;
        }

      usingViewsList_->advance();
    }

  return 0;
}

short CmpSeabaseMDcleanup::deletePrivs(ExeCliInterface *cliInterface)
{
  Lng32 cliRC = 0;
  char query[1000];

  if (NOT isAuthorizationEnabled())
    return 0;

  str_sprintf(query, "delete from %s.\"%s\".%s where object_uid = %ld",
              getSystemCatalog(), SEABASE_PRIVMGR_SCHEMA, "OBJECT_PRIVILEGES",
              objUID_);
  cliRC = cliInterface->executeImmediate(query);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  return 0;
}

void CmpSeabaseMDcleanup::cleanupSchemaObjects(ExeCliInterface *cliInterface)
{
  Lng32 cliRC = 0;
  char query[1000];

  NABoolean errorSeen = FALSE;

  if (objUID_ == -1)
    {
      CmpCommon::diags()->clear();

      ComSchemaName sn(catName_, schName_);
      *CmpCommon::diags() << DgSqlCode(4255)
                          << DgSchemaName(sn.getExternalName().data());
    }

  Queue *schObjList = NULL;
  str_sprintf(query, "select object_uid, object_type, object_name from %s.\"%s\".%s where catalog_name = '%s' and schema_name = '%s' ",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              catName_.data(), schName_.data());
  
  schObjList = NULL;
  cliRC = cliInterface->fetchAllRows(schObjList, query, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return;
    }
  
  stopOnError_ = FALSE;
  NABoolean cannotDropSchema = FALSE;
 
  // Drop histogram tables first
  schObjList->position();
  for (size_t i = 0; i < schObjList->numEntries(); i++)
    {
      OutputInfo * oi = (OutputInfo*)schObjList->getCurr(); 
      Int64 uid = *(Int64*)oi->get(0);
      NAString obj_name((char*)oi->get(2));
      if (isHistogramTable(obj_name))
      {
        str_sprintf(query, "cleanup uid %ld", uid);
        cliRC = cliInterface->executeImmediate(query);
        if (cliRC < 0)
          {
            if (processCleanupErrors(NULL, errorSeen))
              return;
          }      
        CorrName cn(objName_, STMTHEAP, schName_, catName_);
        ActiveSchemaDB()->getNATableDB()->removeNATable
          (
               cn,
               ComQiScope::REMOVE_FROM_ALL_USERS,
               COM_BASE_TABLE_OBJECT,
               FALSE, FALSE
           );
      }
      schObjList->advance();
   }

  // Now drop remaining objects
  schObjList->position();
  for (size_t i = 0; i < schObjList->numEntries(); i++)
    {
      OutputInfo * oi = (OutputInfo*)schObjList->getCurr(); 
      Int64 uid = *(Int64*)oi->get(0);

      NAString obj_type((char*)oi->get(1));

      if ((obj_type == COM_LIBRARY_OBJECT_LIT) ||
          (obj_type == COM_STORED_PROCEDURE_OBJECT_LIT) ||
          (obj_type == COM_USER_DEFINED_ROUTINE_OBJECT))
        {
          schObjList->advance();
          cannotDropSchema = TRUE;
          continue;
        }

      if (NOT ((obj_type == COM_BASE_TABLE_OBJECT_LIT) || 
               (obj_type == COM_INDEX_OBJECT_LIT) ||
               (obj_type == COM_VIEW_OBJECT_LIT) ||
               (obj_type == COM_SEQUENCE_GENERATOR_OBJECT_LIT)))
        {
          schObjList->advance();
          continue;
        }

      str_sprintf(query, "cleanup uid %ld", uid);
      cliRC = cliInterface->executeImmediate(query);
      if (cliRC < 0)
        {
          if (processCleanupErrors(NULL, errorSeen))
            return;
        }      

      schObjList->advance();
    } // for

  if (NOT cannotDropSchema)
    {
      // delete schema object row from objects table
      str_sprintf(query, "delete from  %s.\"%s\".%s where catalog_name = '%s' and schema_name  = '%s' and object_name = '%s' ",
                  getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
                  (char*)catName_.data(),(char*)schName_.data(),
                  SEABASE_SCHEMA_OBJECTNAME);
      cliRC = cliInterface->executeImmediate(query);
      if (cliRC < 0)
        {
          if (processCleanupErrors(NULL, errorSeen))
            return;
        }      
    }

   return;
}

short CmpSeabaseMDcleanup::cleanupUIDs(ExeCliInterface *cliInterface,
                                       Queue *entriesList,
                                       CmpDDLwithStatusInfo *dws)
{
  Lng32 cliRC = 0;
  char query[1000];
  NABoolean errorSeen = FALSE;

  if (checkOnly_)
    return 0;

  entriesList->position();
  for (size_t i = 0; i < entriesList->numEntries(); i++)
    {
      OutputInfo * oi = (OutputInfo*)entriesList->getCurr(); 
      Int64 objUID = *(Int64*)oi->get(0);
      
      str_sprintf(query, "cleanup uid %ld", objUID);
      cliRC = cliInterface->executeImmediate(query);
      if (cliRC < 0)
        {
          if (processCleanupErrors(NULL, errorSeen))
            return -1;
        }      

      entriesList->advance();
    }
  
  return 0;
}

void CmpSeabaseMDcleanup::cleanupHBaseObject(const StmtDDLCleanupObjects * stmtCleanupNode,
                                             ExeCliInterface *cliInterface)
{

  Lng32 cliRC = 0;
  char query[1000];
  NABoolean errorSeen = FALSE;

  NAString objName(stmtCleanupNode->getTableNameAsQualifiedName()->getObjectName());

  // drop external table
  str_sprintf(query, "drop external table if exists %s;",
              objName.data());
  cliRC = cliInterface->executeImmediate(query);
  if (cliRC < 0)
    {
      if (processCleanupErrors(NULL, errorSeen))
        return;
    }          
  
  // unregister registered table
  str_sprintf(query, "unregister hbase table if exists %s cleanup;",
              objName.data());
  cliRC = cliInterface->executeImmediate(query);
  if (cliRC < 0)
    {
      if (processCleanupErrors(NULL, errorSeen))
        return;
    }

  return;
}

short CmpSeabaseMDcleanup::addReturnDetailsEntry(
                                                 ExeCliInterface * cliInterface,
                                                 Queue* &list, const char *value, 
                                                 NABoolean init,
                                                 NABoolean isUID)
{
  if (NOT returnDetails_)
    return 0;

  if (init)
    {
      cliInterface->initializeInfoList(list, TRUE);
      currReturnEntry_ = 0;

      return 0;
    }

  currReturnEntry_++;
  
  OutputInfo * oi = new(STMTHEAP) OutputInfo(1);
  
  char * r = new(STMTHEAP) char[100+strlen(value) + 1];
  str_sprintf(r, "    Entry #%d%s %s", currReturnEntry_,  
              (isUID ? "(UID):   " : "(OBJECT):"),
              value);
  oi->insert(0, r, strlen(r)+1);
  list->insert(oi);
  
  return 0;
}

short CmpSeabaseMDcleanup::addReturnDetailsEntryFromList(
                                                 ExeCliInterface * cliInterface,
                                                 Queue* fromList, Lng32 fromIndex,
                                                 Queue* toList,
                                                 NABoolean isUID)
{
  Lng32 cliRC = 0;
  char query[1000];
  NABoolean errorSeen = FALSE;

  if (NOT returnDetails_)
    return 0;

  fromList->position();
  for (size_t i = 0; i < fromList->numEntries(); i++)
    {
      OutputInfo * oi = (OutputInfo*)fromList->getCurr(); 
      char * val = (char*)oi->get(fromIndex);
      
      if (addReturnDetailsEntry(cliInterface, toList, val, FALSE, isUID))
        return -1;

      fromList->advance();
    }
    
  return 0;
}

short CmpSeabaseMDcleanup::cleanupOrphanObjectsEntries(ExeCliInterface *cliInterface,
                                                       ExpHbaseInterface *ehi,
                                                       CmpDDLwithStatusInfo *dws)
{
  Lng32 cliRC = 0;
  char query[1000];
  NABoolean errorSeen = FALSE;

  // find out all entries which do not have corresponding hbase objects.
  // Do not include metadata, repository and external tables.
  str_sprintf(query, "select object_uid, trim(catalog_name) || '.'  || trim(schema_name) || '.' || trim(object_name)  from %s.\"%s\".%s where catalog_name = '%s' and schema_name not in ( '_MD_', '_REPOS_', '_PRIVMGR_MD_') and schema_name not like '|_HV|_%%|_' escape '|'  and schema_name not like '|_HB|_%%|_' escape '|' and (object_type = 'BT' or object_type = 'IX') ",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              getSystemCatalog());
  cliRC = cliInterface->fetchRowsPrologue(query);
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    }
  
  obsoleteEntriesList_ = new(STMTHEAP) Queue(STMTHEAP);
  returnDetailsList_ = NULL;
  addReturnDetailsEntry(cliInterface, returnDetailsList_, NULL, TRUE);

  while (cliRC != 100)
    {
      cliRC = cliInterface->fetch();
      if (cliRC < 0)
        {
          if (processCleanupErrors(cliInterface, errorSeen))
            return -1;
        }
      
      if (cliRC != 100)
        {
          char * ptr;
          Lng32 len;

          cliInterface->getPtrAndLen(1, ptr, len);
          Int64 objUID = *(Int64*)ptr;

          cliInterface->getPtrAndLen(2, ptr, len);
          NAString extNameForHbase(ptr, len);

          // check to see if this object exists in hbase
          short rc = existsInHbase(extNameForHbase, ehi); // exists
          if (rc == 0) // does not exist
            {
              OutputInfo * oi = new(STMTHEAP) OutputInfo(1);
              char * r = new(STMTHEAP) char[sizeof(Int64)];
              str_cpy_all(r, (char*)&objUID, sizeof(Int64));
              oi->insert(0, r, sizeof(Int64));

              obsoleteEntriesList_->insert(oi);

              if (addReturnDetailsEntry(cliInterface, returnDetailsList_, 
                                        extNameForHbase.data(), FALSE))
                return -1;
            }
        }
    }

  cliRC = cliInterface->fetchRowsEpilogue(0);
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    }

  if (cleanupUIDs(cliInterface, obsoleteEntriesList_, dws))
    return -1;

  numOrphanMetadataEntries_ = obsoleteEntriesList_->numEntries();

  return 0;
}

// cleanup user objects that exist in hbase but not in metadata.
short CmpSeabaseMDcleanup::cleanupOrphanHbaseEntries(ExeCliInterface *cliInterface,
                                                     ExpHbaseInterface *ehi,
                                                     CmpDDLwithStatusInfo *dws)
{
  Lng32 cliRC = 0;
  char query[1000];
  NABoolean errorSeen = FALSE;

  // list all hbase objects that start with TRAFODION.* 
  NAArray<HbaseStr>* listArray = ehi->listAll("TRAFODION\\..*");
  if (! listArray)
    return -1;

  returnDetailsList_ = NULL;
  addReturnDetailsEntry(cliInterface, returnDetailsList_, NULL, TRUE);

  numOrphanHbaseEntries_ = 0;
  for (Int32 i = 0; i < listArray->entries(); i++)
    {
      char cBuf[1000];
      
      HbaseStr *hbaseStr = &listArray->at(i);
      if (hbaseStr->len >= sizeof(cBuf))
         hbaseStr->len = sizeof(cBuf)-1;
      strncpy(cBuf, hbaseStr->val, hbaseStr->len);
      cBuf[hbaseStr->len] = '\0';
      char *c = cBuf;
      Lng32 numParts = 0;
      char *parts[4];
      LateNameInfo::extractParts(c, cBuf, numParts, parts, FALSE);
      
      NAString catalogNamePart(parts[0]);
      NAString schemaNamePart(parts[1]);
      NAString objectNamePart(parts[2]);
      const NAString extTableName =
        catalogNamePart + "." + "\"" + schemaNamePart  + "\"" + "." + "\"" + objectNamePart + "\"";
      const NAString extHbaseName =
        catalogNamePart + "." + schemaNamePart  + "." + objectNamePart;

      if ((schemaNamePart ==  SEABASE_MD_SCHEMA) ||
          (schemaNamePart == SEABASE_REPOS_SCHEMA) ||
          (schemaNamePart == SEABASE_DTM_SCHEMA) ||
          (schemaNamePart == SEABASE_PRIVMGR_SCHEMA))
        continue;

      // check if this object exists in metadata.
      Int64 objUID = getObjectUID(cliInterface, 
                                  catalogNamePart.data(),
                                  schemaNamePart.data(),
                                  objectNamePart.data(),
                                  NULL);
      if (objUID != -1) // found in metadata
        continue;

      // not found or error. Clear diags and continue.
      CmpCommon::diags()->clear();

      numOrphanHbaseEntries_++;

      if (addReturnDetailsEntry(cliInterface, returnDetailsList_, 
                                extHbaseName.data(), FALSE))
        return -1;
 
      if (checkOnly_)
        continue;

      // this object could be a table or an index. Try to cleanup both.
      str_sprintf(query, "cleanup table %s", extTableName.data());
      cliRC = cliInterface->executeImmediate(query);
      if (cliRC < 0)
        {
          if (processCleanupErrors(NULL, errorSeen))
            return -1;
        }      

      str_sprintf(query, "cleanup index %s", extTableName.data());
      cliRC = cliInterface->executeImmediate(query);
      if (cliRC < 0)
        {
          if (processCleanupErrors(NULL, errorSeen))
            return -1;
        }      
      
    } // for
  deleteNAArray(ehi->getHeap(),listArray);
  return 0;
}

short CmpSeabaseMDcleanup::cleanupInconsistentObjectsEntries(ExeCliInterface *cliInterface,
                                                             ExpHbaseInterface *ehi,
                                                             CmpDDLwithStatusInfo *dws)
{
  Lng32 cliRC = 0;
  char query[1000];
  NABoolean errorSeen = FALSE;

  numOrphanObjectsEntries_ = 0;

  obsoleteEntriesList_ = NULL;
  returnDetailsList_ = NULL;

  addReturnDetailsEntry(cliInterface, returnDetailsList_, NULL, TRUE);

  str_sprintf(query, "control query shape join(scan(path '%s'), scan(path '%s'))",
              SEABASE_OBJECTS, SEABASE_OBJECTS_UNIQ_IDX);
  
  cliRC = cliInterface->executeImmediate(query);
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    }      

  // find out all entries that exist in OBJECTS but not in OBJECTS_UNIQ_IDX
  str_sprintf(query, "select object_uid, trim(catalog_name) || '.'  || trim(schema_name) || '.' || trim(object_name)  from %s.\"%s\".%s  where catalog_name = '%s' and schema_name not in ( '_MD_', '_REPOS_', '_PRIVMGR_MD_') and object_uid not in (select \"OBJECT_UID@\"  from table(index_table %s.\"%s\".%s))",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              getSystemCatalog(),
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS_UNIQ_IDX);
  
  cliRC = cliInterface->fetchAllRows(obsoleteEntriesList_, query, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    }

  if (cleanupUIDs(cliInterface, obsoleteEntriesList_, dws))
    return -1;

  addReturnDetailsEntryFromList(cliInterface, obsoleteEntriesList_, 1/*0 based*/,
                                returnDetailsList_);

  numOrphanObjectsEntries_ += obsoleteEntriesList_->numEntries();

  // find out all entries that exist in OBJECTS_UNIQ_IDX but not in OBJECTS
  str_sprintf(query, "control query shape join(scan(path '%s'), scan(path '%s'))",
              SEABASE_OBJECTS_UNIQ_IDX, SEABASE_OBJECTS);
  
  cliRC = cliInterface->executeImmediate(query);
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    }      

  str_sprintf(query, "select \"OBJECT_UID@\", trim(catalog_name) || '.'  || trim(schema_name) || '.' || trim(object_name)  from table(index_table %s.\"%s\".%s)  where catalog_name = '%s' and schema_name not in ( '_MD_', '_REPOS_', '_PRIVMGR_MD_') and \"OBJECT_UID@\" not in (select object_uid from %s.\"%s\".%s)",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS_UNIQ_IDX,
              getSystemCatalog(),
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS);
  
  cliRC = cliInterface->fetchAllRows(obsoleteEntriesList_, query, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    }

  cliRC = cliInterface->executeImmediate("control query shape cut;");
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    }      

  if (cleanupUIDs(cliInterface, obsoleteEntriesList_, dws))
    return -1;

  addReturnDetailsEntryFromList(cliInterface, obsoleteEntriesList_, 1/*0 based*/,
                                returnDetailsList_);

  numOrphanObjectsEntries_ += obsoleteEntriesList_->numEntries();

 // cleanup entries in columns table which are not in objects table
  str_sprintf(query, "control query shape groupby(join(scan(path '%s'), scan(path '%s')))",
              SEABASE_COLUMNS, SEABASE_OBJECTS);
  
  cliRC = cliInterface->executeImmediate(query);
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    }      

  str_sprintf(query, "select distinct object_uid, cast(object_uid as varchar(30) not null) from %s.\"%s\".%s where object_uid not in (select object_uid from %s.\"%s\".%s) ",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_COLUMNS,
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS);
  
  cliRC = cliInterface->fetchAllRows(obsoleteEntriesList_, query, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    }

  cliRC = cliInterface->executeImmediate("control query shape cut;");
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    }      

  if (cleanupUIDs(cliInterface, obsoleteEntriesList_, dws))
    return -1;

  addReturnDetailsEntryFromList(cliInterface, obsoleteEntriesList_, 1/*0 based*/,
                                returnDetailsList_, TRUE/*UID*/);

  numOrphanObjectsEntries_ += obsoleteEntriesList_->numEntries();

  return 0;
}

short CmpSeabaseMDcleanup::cleanupOrphanViewsEntries(ExeCliInterface *cliInterface,
                                                     ExpHbaseInterface *ehi,
                                                     CmpDDLwithStatusInfo *dws)
{
  Lng32 cliRC = 0;
  char query[1000];
  NABoolean errorSeen = FALSE;

  numOrphanViewsEntries_ = 0;
  obsoleteEntriesList_ = NULL;

  returnDetailsList_ = NULL;
  addReturnDetailsEntry(cliInterface, returnDetailsList_, NULL, TRUE);

  // find out all entries in views_usage that dont have corresponding used object
  str_sprintf(query, "control query shape groupby(join(scan(path '%s'), scan(path '%s')))",
              SEABASE_VIEWS_USAGE, SEABASE_OBJECTS);
  
  cliRC = cliInterface->executeImmediate(query);
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    }      

  str_sprintf(query, "select distinct used_object_uid, cast(used_object_uid as varchar(30) not null) from %s.\"%s\".%s  where used_object_uid not in (select object_uid from %s.\"%s\".%s)",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_VIEWS_USAGE,
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS);
  
  cliRC = cliInterface->fetchAllRows(obsoleteEntriesList_, query, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    }

  cliRC = cliInterface->executeImmediate("control query shape cut;");
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    }      

  if (cleanupUIDs(cliInterface, obsoleteEntriesList_, dws))
    return -1;
 
  addReturnDetailsEntryFromList(cliInterface, obsoleteEntriesList_, 1/*0 based*/,
                                returnDetailsList_, TRUE/*UID*/);
 
  numOrphanObjectsEntries_ += obsoleteEntriesList_->numEntries();
  
  // find out all entries in VIEWS that do not exist in OBJECTS
  str_sprintf(query, "control query shape join(scan(path '%s'), scan(path '%s'))",
              SEABASE_VIEWS, SEABASE_OBJECTS);
  
  cliRC = cliInterface->executeImmediate(query);
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    }      

  str_sprintf(query, "select view_uid, cast(view_uid as varchar(30) not null) from %s.\"%s\".%s  where view_uid not in (select object_uid from %s.\"%s\".%s)",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_VIEWS,
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS);
  
  cliRC = cliInterface->fetchAllRows(obsoleteEntriesList_, query, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    }

  cliRC = cliInterface->executeImmediate("control query shape cut;");
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    }      

  if (cleanupUIDs(cliInterface, obsoleteEntriesList_, dws))
    return -1;

  addReturnDetailsEntryFromList(cliInterface, obsoleteEntriesList_, 1/*0 based*/,
                                returnDetailsList_, TRUE/*UID*/);

  numOrphanObjectsEntries_ += obsoleteEntriesList_->numEntries();
  
  return 0;
}

// remove hive objects that are registered in traf metadata but the 
// corresponding object is missing in hive database.
short CmpSeabaseMDcleanup::cleanupInconsistentHiveEntries(
     ExeCliInterface *cliInterface,
     ExpHbaseInterface *ehi)
{
  Lng32 cliRC = 0;
  char query[4000];
  NABoolean errorSeen = FALSE;

  // get all registered tables that do not have corresponding hive objects.
  str_sprintf(query, "select trim(O.a), 'table' from "
              "(select lower(trim(catalog_name) || '.' || trim(schema_name)"
              " || '.' || trim(object_name)) from %s.\"%s\".%s "
              "where object_type = '%s' and catalog_name = 'HIVE') O(a) left join "
              "(select '%s' || '.' || trim(y) from "
              "(get tables in catalog %s, no header) x(y)) G(b) "
              "on O.a = G.b where G.b is null "
              "  union all "
              "select trim(O.a), 'view' from "
              "(select lower(trim(catalog_name) || '.' || trim(schema_name)"
              " || '.' || trim(object_name)) from %s.\"%s\".%s "
              "where object_type = '%s' and catalog_name = 'HIVE') O(a) left join "
              "(select '%s' || '.' || trim(y) from "
              "(get views in catalog %s, no header) x(y)) G(b) "
              "on O.a = G.b where G.b is null ;",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              COM_BASE_TABLE_OBJECT_LIT,
              HIVE_SYSTEM_CATALOG_LC,
              HIVE_SYSTEM_CATALOG_LC,
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              COM_VIEW_OBJECT_LIT,
              HIVE_SYSTEM_CATALOG_LC,
              HIVE_SYSTEM_CATALOG_LC);
  Queue * orphanHiveObjs = NULL;
  cliRC = cliInterface->fetchAllRows
    (orphanHiveObjs, query, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      if (processCleanupErrors(cliInterface, errorSeen))
        return -1;
    }

  numInconsistentHiveEntries_ = 0;
  returnDetailsList_ = NULL;
  addReturnDetailsEntry(cliInterface, returnDetailsList_, NULL, TRUE);

  numInconsistentHiveEntries_ += orphanHiveObjs->numEntries();

  orphanHiveObjs->position();
  for (size_t i = 0; i < orphanHiveObjs->numEntries(); i++)
    {
      
      OutputInfo * oi = (OutputInfo*)orphanHiveObjs->getCurr(); 
       
      if (addReturnDetailsEntry(cliInterface, returnDetailsList_, 
                                oi->get(0), FALSE))
        return -1;

      if (NOT checkOnly_)
        {
          if (strcmp(oi->get(1), "table") == 0)
            str_sprintf(query, "unregister hive table if exists %s cleanup;",
                        oi->get(0));
          else
            str_sprintf(query, "unregister hive view if exists %s cleanup;",
                        oi->get(0));
            
          cliRC = cliInterface->executeImmediate(query);
          if (cliRC < 0)
            {
              cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
              return -1;
            }
        }

      orphanHiveObjs->advance();
    }

  numOrphanMetadataEntries_ = orphanHiveObjs->numEntries();

  return 0;
}

void CmpSeabaseMDcleanup::populateBlackBox(ExeCliInterface *cliInterface,
                                           Queue *returnDetailsList,
                                           Int32 &blackBoxLen,
                                           char* &blackBox)
{
  blackBoxLen = 0;
  blackBox = NULL;
  if (returnDetailsList && returnDetailsList->numEntries() > 0)
    {
      Lng32 numEntries = returnDetailsList->numEntries();

      blackBoxLen += sizeof(Int32);
      returnDetailsList->position();
      for (Int32 i = 0; i < numEntries; i++)
        {
          OutputInfo * oi = (OutputInfo*)returnDetailsList->getCurr(); 
          char * val = (char*)oi->get(0);
          
          blackBoxLen += sizeof(Int32);
          blackBoxLen += ROUND4(strlen(val)+1);
          
          returnDetailsList->advance();
        }
      
      blackBoxLen = ROUND8(blackBoxLen);
      
      blackBox = new(STMTHEAP) char[blackBoxLen];
      
      char * currPtr = blackBox;
      *(Int32*)currPtr = numEntries;
      currPtr += sizeof(Int32);
      
      returnDetailsList->position();
      for (Int32 i = 0; i < numEntries; i++)
        {
          OutputInfo * oi = (OutputInfo*)returnDetailsList->getCurr(); 
          char * val = (char*)oi->get(0);
          
          *(Int32*)currPtr = strlen(val);
          currPtr += sizeof(Int32);
          
          strcpy(currPtr, val);
          currPtr += ROUND4(strlen(val)+1);
          
          returnDetailsList->advance();
        }
      
    }
}

void CmpSeabaseMDcleanup::cleanupMetadataEntries(ExeCliInterface *cliInterface,
                                                 ExpHbaseInterface *ehi,
                                                 CmpDDLwithStatusInfo *dws)
{
  Lng32 cliRC = 0;
  char query[1000];
  NABoolean errorSeen = FALSE;

  char buf[500];

  dws->setBlackBoxLen(0);
  dws->setBlackBox(NULL);

  while (1) // exit via return stmt in switch
    {
      switch (dws->step())
        {
        case START_CLEANUP:
          {
            if (checkOnly_)
              dws->setMsg("Metadata Cleanup: started, check only");
            else
              dws->setMsg("Metadata Cleanup: started");
            dws->setStep(ORPHAN_OBJECTS_ENTRIES);
            dws->setSubstep(0);
            dws->setEndStep(TRUE);
        
            return;
          }
          break;
  
        case ORPHAN_OBJECTS_ENTRIES:
          {
            switch (dws->subStep())
              {
              case 0:
                {
                  dws->setMsg("  Start: Cleanup Orphan Objects Entries");
                  dws->subStep()++;
                  dws->setEndStep(FALSE);
                  
                  return;
                }
                break;

              case 1:
                {
                  if (cleanupOrphanObjectsEntries(cliInterface, ehi, dws))
                    return;
                  
                  str_sprintf(buf, "  End:   Cleanup Orphan Objects Entries (%d %s %s)",
                              numOrphanMetadataEntries_,
                              (numOrphanMetadataEntries_ == 1 ? "entry" : "entries"),
                              (checkOnly_ ? "found" : "cleaned up"));
                  dws->setMsg(buf);

                  Int32 blackBoxLen = 0;
                  char * blackBox = NULL;
                  populateBlackBox(cliInterface, returnDetailsList_, blackBoxLen, blackBox);

                  dws->setBlackBoxLen(blackBoxLen);
                  dws->setBlackBox(blackBox);

                  dws->setStep(HBASE_ENTRIES);
                  dws->setSubstep(0);
                  dws->setEndStep(TRUE);
                  
                  return;
                }
                break;
              } // switch 
          } // MD_ENTRIES
          break;

        case HBASE_ENTRIES:
          {
            switch (dws->subStep())
              {
              case 0:
                {
                  dws->setMsg("  Start: Cleanup Orphan Hbase Entries");
                  dws->subStep()++;
                  dws->setEndStep(FALSE);
                  
                  return;
                }
                break;

              case 1:
                {
                  if (cleanupOrphanHbaseEntries(cliInterface, ehi, dws))
                    return;
                  
                  str_sprintf(buf, "  End:   Cleanup Orphan Hbase Entries (%d %s %s)",
                              numOrphanHbaseEntries_,
                              (numOrphanHbaseEntries_ == 1 ? "entry" : "entries"),
                              (checkOnly_ ? "found" : "cleaned up"));

                  Int32 blackBoxLen = 0;
                  char * blackBox = NULL;
                  populateBlackBox(cliInterface, returnDetailsList_, blackBoxLen, blackBox);

                  dws->setBlackBoxLen(blackBoxLen);
                  dws->setBlackBox(blackBox);
                  
                  dws->setMsg(buf);
                  dws->setStep(INCONSISTENT_OBJECTS_ENTRIES);
                  dws->setSubstep(0);
                  dws->setEndStep(TRUE);
                  
                  return;
                }
                break;
              } // switch
          } // HBASE_ENTRIES
          break;

        case INCONSISTENT_OBJECTS_ENTRIES:
          {
            switch (dws->subStep())
              {
              case 0:
                {
                  dws->setMsg("  Start: Cleanup Inconsistent Objects Entries");
                  dws->subStep()++;
                  dws->setEndStep(FALSE);
                  
                  return;
                }
                break;
                
              case 1:
                {
                  if (cleanupInconsistentObjectsEntries(cliInterface, ehi, dws))
                    return;
                  
                  str_sprintf(buf, "  End:   Cleanup Inconsistent Objects Entries (%d %s %s)",
                              numOrphanObjectsEntries_,
                              (numOrphanObjectsEntries_ == 1 ? "entry" : "entries"),
                              (checkOnly_ ? "found" : "cleaned up"));

                  Int32 blackBoxLen = 0;
                  char * blackBox = NULL;
                  populateBlackBox(cliInterface, returnDetailsList_, blackBoxLen, blackBox);

                  dws->setBlackBoxLen(blackBoxLen);
                  dws->setBlackBox(blackBox);
                   
                  dws->setMsg(buf);
                  dws->setStep(VIEWS_ENTRIES);
                  dws->setSubstep(0);
                  dws->setEndStep(TRUE);
                  
                  return;
                }
                break;
              } // switch
          } // OBJECTS_ENTRIES
          break;

        case VIEWS_ENTRIES:
          {
            switch (dws->subStep())
              {
              case 0:
                {
                  dws->setMsg("  Start: Cleanup Inconsistent Views Entries");
                  dws->subStep()++;
                  dws->setEndStep(FALSE);
                  
                  return;
                }
                break;
                
              case 1:
                {
                  if (cleanupOrphanViewsEntries(cliInterface, ehi, dws))
                    return;
                  
                  str_sprintf(buf, "  End:   Cleanup Inconsistent Views Entries (%d %s %s)",
                              numOrphanViewsEntries_,
                              (numOrphanViewsEntries_ == 1 ? "entry" : "entries"),
                              (checkOnly_ ? "found" : "cleaned up"));
                  
                  Int32 blackBoxLen = 0;
                  char * blackBox = NULL;
                  populateBlackBox(cliInterface, returnDetailsList_, blackBoxLen, blackBox);

                  dws->setBlackBoxLen(blackBoxLen);
                  dws->setBlackBox(blackBox);
                   
                  if ((numOrphanViewsEntries_ == 0) &&
                      (blackBoxLen > 0))
                    {
                      str_sprintf(buf, "  End:   Cleanup Inconsistent Views Entries (%d %s %s) [internal error: blackBoxLen = %d] ",
                                  numOrphanViewsEntries_,
                                  (numOrphanViewsEntries_ == 1 ? "entry" : "entries"),
                                  (checkOnly_ ? "found" : "cleaned up"),
                                  blackBoxLen);
                    }
                  
                  dws->setMsg(buf);
                  dws->setStep(HIVE_ENTRIES);
                  dws->setSubstep(0);
                  dws->setEndStep(TRUE);
                  
                  return;
                }
                break;
              } // switch
          } // VIEWS_ENTRIES

        case HIVE_ENTRIES:
          {
            switch (dws->subStep())
              {
              case 0:
                {
                  dws->setMsg("  Start: Cleanup Inconsistent Hive Entries");
                  dws->subStep()++;
                  dws->setEndStep(FALSE);
                  
                  return;
                }
                break;
                
              case 1:
                {

                  if (cleanupInconsistentHiveEntries(cliInterface, ehi))
                    return;
                  
                  str_sprintf(buf, "  End:   Cleanup Inconsistent Hive Entries (%d %s %s)",
                              numInconsistentHiveEntries_,
                              (numInconsistentHiveEntries_ == 1 ? "entry" : "entries"),
                              (checkOnly_ ? "found" : "cleaned up"));
                  
                  Int32 blackBoxLen = 0;
                  char * blackBox = NULL;
                  populateBlackBox(cliInterface, returnDetailsList_, blackBoxLen, blackBox);

                  dws->setBlackBoxLen(blackBoxLen);
                  dws->setBlackBox(blackBox);
                   
                  dws->setMsg(buf);
                  dws->setStep(DONE_CLEANUP);
                  dws->setSubstep(0);
                  dws->setEndStep(TRUE);
                  
                  return;
                }
                break;
              } // switch
          } // HIVE_ENTRIES

       case DONE_CLEANUP:
          {
            dws->setMsg("Metadata Cleanup: done");
            dws->setEndStep(TRUE);
            dws->setSubstep(0);
            dws->setDone(TRUE);
            
            return;
          }
          break;
          
        default:
          return;
          
        } // switch
      
    } // while
  
  return;
}

void CmpSeabaseMDcleanup::cleanupObjects(StmtDDLCleanupObjects * stmtCleanupNode,
                                         NAString &currCatName, NAString &currSchName,
                                         CmpDDLwithStatusInfo *dws)
{
  Lng32 cliRC = 0;
  ExeCliInterface cliInterface(STMTHEAP);

  if ((xnInProgress(&cliInterface)) &&
      (!Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL)))
     {
       *CmpCommon::diags() << DgSqlCode(-20125)
                           << DgString0("This CLEANUP");
      return;
    }

  // run each MD update in its own transaction. That way we can remove
  // as much as we can.
  cliInterface.autoCommit(TRUE);

  if (stmtCleanupNode)
    {
      if (validateInputValues(stmtCleanupNode, &cliInterface))
        return;
    }

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    {
      return;
    }

  checkOnly_ = FALSE;
  if (dws && dws->getMDcleanup())
    {
      if (dws->getCheckOnly())
        checkOnly_ = TRUE;
      if (dws->getReturnDetails())
        returnDetails_ = TRUE;
      cleanupMetadataEntries(&cliInterface, ehi, dws);
      return;
    }

  if (cleanupMetadataEntries_)
    {
      return cleanupMetadataEntries(&cliInterface, ehi, NULL);
    }

  if ((objType_ == COM_PRIVATE_SCHEMA_OBJECT_LIT) ||
      (objType_ == COM_SHARED_SCHEMA_OBJECT_LIT))
    {
      return cleanupSchemaObjects(&cliInterface);
    }

  if (stmtCleanupNode &&
      (stmtCleanupNode->getType() == StmtDDLCleanupObjects::HBASE_TABLE_))
    {
      return cleanupHBaseObject(stmtCleanupNode, &cliInterface);
    }

  if (gatherDependentObjects(&cliInterface))
    {
      if (stopOnError_)
        goto label_return;
    }

  if (((objType_ == COM_BASE_TABLE_OBJECT_LIT) ||
       (objType_ == COM_INDEX_OBJECT_LIT)) &&
      (NOT isHive_) &&
      (extNameForHbase_.isNull()))
    {
      // add warning that name couldnt be found. Hbase object cannot be removed.
      CmpCommon::diags()->clear();
      *CmpCommon::diags() << DgSqlCode(4250);
    }
  else if (((objType_ != COM_PRIVATE_SCHEMA_OBJECT_LIT) &&
            (objType_ != COM_SHARED_SCHEMA_OBJECT_LIT) &&
            (objType_ != COM_VIEW_OBJECT_LIT)) &&
           (objUID_ == -1))
    {
      CmpCommon::diags()->clear();       
      *CmpCommon::diags() << DgSqlCode(4251) ;
    }

  cliRC = deleteMDentries(&cliInterface);
  if (cliRC < 0)
    if (stopOnError_)
      goto label_error;
  
  cliRC = deleteMDConstrEntries(&cliInterface);
  if (cliRC < 0)
    if (stopOnError_)
      goto label_error;

  cliRC = deleteMDViewEntries(&cliInterface);
  if (cliRC < 0)
    if (stopOnError_)
      goto label_error;

  cliRC = deleteHistogramEntries(&cliInterface);
  if (cliRC < 0)
    if (stopOnError_)
      goto label_error;

  if ((NOT isHive_) &&
      (NOT extNameForHbase_.isNull()))
    {
      HbaseStr hbaseObject;
      hbaseObject.val = (char*)extNameForHbase_.data();
      hbaseObject.len = extNameForHbase_.length();
      
      // drop this object from hbase
      cliRC = dropHbaseTable(ehi, &hbaseObject, FALSE, FALSE);
      if (cliRC)
          if (stopOnError_)
            goto label_return;
    }

  // drop underlying Hive object
  if (isHive_)
    {
      NAString hiveQuery;
      if (objType_ == COM_BASE_TABLE_OBJECT_LIT)
        {
          hiveQuery = "drop table if exists " + extNameForHive_;
        }
      else if (objType_ == COM_VIEW_OBJECT_LIT)
        {
          hiveQuery = "drop view if exists " + extNameForHive_;
        }

      if (NOT hiveQuery.isNull())
        {
          if (HiveClient_JNI::executeHiveSQL(hiveQuery.data()) != HVC_OK)
            {
              if (stopOnError_)
                goto label_return;
            }
        }
    }

  cliRC = dropIndexes(&cliInterface);
  if (cliRC)
    if (stopOnError_)
      goto label_return;

  cliRC = dropSequences(&cliInterface);
  if (cliRC)
    if (stopOnError_)
      goto label_return;

  cliRC = dropUsingViews(&cliInterface);
  if (cliRC)
    if (stopOnError_)
      goto label_return;

  cliRC = dropLOBs(&cliInterface);
  if (cliRC)
    if (stopOnError_)
      goto label_return;
  
  cliRC = deletePrivs(&cliInterface);
  if (cliRC)
    if (stopOnError_)
      goto label_return;

  deallocEHI(ehi);
  
  if (NOT (catName_.isNull() || schName_.isNull() || objName_.isNull()))
    {
      CorrName cn(objName_, STMTHEAP, schName_, catName_);
      ActiveSchemaDB()->getNATableDB()->removeNATable
        (
             cn,
             ComQiScope::REMOVE_FROM_ALL_USERS, 
             (objType_ == COM_VIEW_OBJECT_LIT ? COM_VIEW_OBJECT :
              COM_BASE_TABLE_OBJECT),
             FALSE, FALSE);
    }

  return;

 label_error:
  if ((objType_ == COM_BASE_TABLE_OBJECT_LIT) &&
      (NOT extNameForHbase_.isNull()))
    {
      CorrName cn(objName_, STMTHEAP, schName_, catName_);
      ActiveSchemaDB()->getNATableDB()->removeNATable
        (cn,
         ComQiScope::REMOVE_FROM_ALL_USERS, 
         COM_BASE_TABLE_OBJECT,
         FALSE, FALSE);
    }

 label_return:
  deallocEHI(ehi);

  return;
}

 

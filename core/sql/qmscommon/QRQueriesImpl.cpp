// **********************************************************************
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
// **********************************************************************
// ***********************************************************************
//
// File:         QRQueriesImpl.cpp
//
// Description:  Classes used during QMS initialization
//               to obtain all MV descriptor text and MV attributes
//               for all MVs on the system.
//
// Created:      04/10/09
// ***********************************************************************

#include <ComCextdecs.h>
#include <Int64.h>
#include "QRSharedPtr.h"
#include "QRMessage.h"
#include "XMLUtil.h"
#include "QRDescriptor.h"
#include "QRLogger.h"
#include "QRIpc.h"
#include "QRQueriesImpl.h"
#include "QRQueries.h"
#include "QueryRewriteServer.h"

// module definition

SQLMODULE_ID QRQueries_mod = {
/* version */    	1,
/* module name */	"HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.MVQR_N29_000",
/* timestamp */  	212107632979614792LL,
/* char set */   	"ISO88591",
/* name len */   	42 };

/**
  * \file
  * Contains the class routines for QRQueriesImpl and 
  * QRMVDefinition, which are used to initialize
  * the QMS process.
  */

//========================================================================
//  Class QRQueriesImpl
//========================================================================

QRQueriesImpl::QRQueriesImpl(CollHeap* heap)
    : heap_(heap)
{
  char buffer[MAX_SYSTEM_DEFAULTS+10];

  data_ = new(heap_) QRMVData;
  queries_ = new(heap_) QRQueries();

  // Obtain the node name for this process
  initializeNodeName();

  // Set up CATSYS table name
  //sprintf(buffer, "NONSTOP_SQLMX_%.100s.SYSTEM_SCHEMA.CATSYS",getNodeName());
  strcpy (buffer, "HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.CATSYS");
  queries_->setCatsysName(buffer);

  // Set up SCHEMATA table name
  //sprintf(buffer, "NONSTOP_SQLMX_%.100s.SYSTEM_SCHEMA.SCHEMATA", getNodeName());
  strcpy (buffer, "HP_SYSTEM_CATALOG.SYSTEM_SCHEMA.SCHEMATA");
  queries_->setSchemataName(buffer);

  // Set up full name of SYSTEM_DEFAULTS table.
  //sprintf(buffer, "NONSTOP_SQLMX_%.100s.SYSTEM_DEFAULTS_SCHEMA.SYSTEM_DEFAULTS",getNodeName());
  strcpy (buffer, "HP_SYSTEM_CATALOG.SYSTEM_DEFAULTS_SCHEMA.SYSTEM_DEFAULTS");
  queries_->setSystemDefaultsName(buffer);
}  

// *************************************************************** 
// *************************************************************** 
QRQueriesImpl::~QRQueriesImpl()
{
  NADELETE (data_, QRMVData, heap_);
  NADELETE (queries_, QRQueries, heap_);
}  

// *************************************************************** 
// * Obtain the name of this system node.
// *************************************************************** 
void QRQueriesImpl::initializeNodeName()
{
  nodeNameWithBackslash_ = "\\NSK";
  nodeName_ = "NSK";
}  // End of initializeNodeName

// *************************************************************** 
// * Begin a transaction.
// * This routine calls a static SQL routine to execute "BEGIN WORK;"
// *************************************************************** 
void QRQueriesImpl::beginTransaction()
{
  Lng32 sqlCode = queries_->beginTransaction();
  assertLogAndThrow1(CAT_SQL_COMP_QR_COMMON, LL_ERROR, 
                     sqlCode==SQL_Success, QRDatabaseException, 
		    "Error %d performing Begin Transaction.", sqlCode); 

  return;
}  // End of beginTransaction

// *************************************************************** 
// * Commit the transaction
// * This routine calls a static SQL routine to execute "COMMIT WORK;"
// *************************************************************** 
void QRQueriesImpl::commitTransaction()
{
  Lng32 sqlCode = queries_->commitTransaction();
  assertLogAndThrow1(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
                     sqlCode==SQL_Success, QRDatabaseException, 
		    "Error %d performing Commit Transaction.", sqlCode); 

  return;
}  // End of commitTransaction

// *************************************************************** 
// * Rollback the transaction
// * This routine calls a static SQL routine to execute "ROLLBACK WORK;"
// *************************************************************** 
void QRQueriesImpl::rollbackTransaction()
{
  Lng32 sqlCode = queries_->rollbackTransaction();
  assertLogAndThrow1(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
                    sqlCode==SQL_Success, QRDatabaseException, 
		    "Error %d performing Rollback Transaction.", sqlCode); 

  return;
}  // End of rollbackTransaction

// *************************************************************** 
// *************************************************************** 
NABoolean QRQueriesImpl::getSystemDefault(const char* attribute, char* defValue)
{
  Lng32 sqlCode = SQL_Success;
  try
  {
    sqlCode = queries_->openSystemDefault(attribute);
    assertLogAndThrow1(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
                      sqlCode==SQL_Success, QRDatabaseException, 
		      "Error %d performing read from DEFAULTS table.", sqlCode); 

    sqlCode = queries_->fetchSystemDefault(defValue);
    assertLogAndThrow1(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
                      sqlCode==SQL_Success || sqlCode==SQL_Eof, QRDatabaseException, 
		      "Error %d performing read from DEFAULTS table.", sqlCode); 

    // Always close to free up resources
    queries_->closeSystemDefault();
  }
  catch(...)
  {
    // If we get an error here, just go with the defaults.
    return FALSE;
  }

  if (sqlCode == SQL_Success)
    return TRUE;
  else 
    return FALSE;
}

// *************************************************************** 
// *************************************************************** 
Lng32 QRQueriesImpl::controlQueryDefault(const NAString& cqdName, 
                                        const NAString& cqdValue)
{
  Lng32 sqlCode = SQL_Success;
  try
  {
    sqlCode = queries_->controlQueryDefault(cqdName, cqdValue);
    assertLogAndThrow1(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
                      sqlCode==SQL_Success, QRDatabaseException, 
		      "Error %d performing CONTROL QUERY DEFAULT.", sqlCode); 
  }
  catch(...)
  {
    // If we get an error here, just go with the defaults.
    return FALSE;
  }

  if (sqlCode == SQL_Success)
    return TRUE;
  else 
    return FALSE;
}

// *************************************************************** 
// * Get the MVQR_PRIVATE_QMS_INIT setting from the SYSTEM DEFAULTS TABLE
// * for National character set support.
// *
// * A static SQL query is used to read the MVQR_PRIVATE_QMS_INIT
// * attribute value from the system defaults table. 
// *************************************************************** 
const NAString* QRQueriesImpl::getMvqrPrivateQMSInit()
{
  // Prepare to obtain the system MVQR_PRIVATE_QMS_INIT attribute value
  // from the system defaults table
  static const char qmsInit[] = "MVQR_PRIVATE_QMS_INIT";
  char defValue[MAX_DEFAULTS_UTF8_VALUE_IN_BYTES];
  NAString* result = NULL;

  if (getSystemDefault(qmsInit, defValue) == FALSE)
    result = new(heap_) NAString("SMD", heap_); // This is the default value.
  else 
    result = new(heap_) NAString(defValue, heap_);

  return result;
}  // End of getMvqrPrivateQMSInit

// ***************************************************************
// ***************************************************************
NABoolean QRQueriesImpl::getCollectQMSStatsEnabled()
{
  static const char collectQMSStatsdEnabled[] = "MVQR_COLLECT_QMS_STATS";
  char defValue[MAX_DEFAULTS_UTF8_VALUE_IN_BYTES];

  if (getSystemDefault(collectQMSStatsdEnabled, defValue) == FALSE)
    return FALSE; // The default value.
  else if (!strcmp(defValue, "OFF"))
    return FALSE;
  else if (!strcmp(defValue, "ON"))
    return TRUE;
  else
    assertLogAndThrow1(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
                       FALSE, QRDatabaseException,
                       "Invalid value for MVQR_COLLECT_QMS_STATS system default: %s.", defValue);
}

// *************************************************************** 
// * Get the ISO_MAPPING setting from the SYSTEM DEFAULTS TABLE
// * for National character set support.
// *************************************************************** 
CharInfo::CharSet QRQueriesImpl::getIsoMapping()
{
  static const char isoMapping[] = "ISO_MAPPING";
  char defValue[MAX_DEFAULTS_UTF8_VALUE_IN_BYTES];

  if (getSystemDefault(isoMapping, defValue) == FALSE)
    return CharInfo::getCharSetEnum((const char *)"ISO88591");
  else
  { 
    NAString(defValue);
    TrimNAStringSpace(defValue);
    defValue.toUpper();
    if (defValue != "ISO88591")
    {
      QRLogger::instance().log(CAT_SQL_COMP_QR_COMMON, LL_WARN,
                               "The default attribute ISO_MAPPING is depricated "
                               "and can be set to ISO88591 only. "
                               "Its current default setting in SYSTEM_DEFAULTS is invalid: %s",
                               defValue.toCharStar());
    }
    return  CharInfo::getCharSetEnum((const char *)"ISO88591");
  }
}  // End of getIsoMapping

// *************************************************************** 
// * Get the DEFAULT_CHARSET setting from the SYSTEM DEFAULTS TABLE
// * for National character set support.
// *************************************************************** 
CharInfo::CharSet QRQueriesImpl::getDefaultCharset()
{
  static const char defaultCharset[] = "DEFAULT_CHARSET";
  char defValue[MAX_DEFAULTS_UTF8_VALUE_IN_BYTES];

  if (getSystemDefault(defaultCharset, defValue) == FALSE)
    return CharInfo::getCharSetEnum((const char *)"ISO88591");  // Use the default value
  else 
  { 
    NAString(defValue);
    TrimNAStringSpace(defValue);
    defValue.toUpper();
    if (defValue != "ISO88591") // This will break when we allow setting DEFAULT_CHARSET
    {
      QRLogger::instance().log(CAT_SQL_COMP_QR_COMMON, LL_WARN,
                               "The default attribute DEFAULT_CHARSET can be set to ISO88591 only. "
                               "Its current default setting in SYSTEM_DEFAULTS is invalid: %s",
                               defValue.toCharStar());
    }
    return CharInfo::getCharSetEnum((const char *)"ISO88591");
  }
}  // End of getDefaultCharset

// *************************************************************** 
// *************************************************************** 
Int32 QRQueriesImpl::getMvqrCpusPerQMS()
{
  static const char cpusPerQms[] = "MVQR_CPUS_PER_QMS";
  char defValue[MAX_DEFAULTS_UTF8_VALUE_IN_BYTES];

  if (getSystemDefault(cpusPerQms, defValue) == FALSE)
    return 0;  // Use the default value
  else 
  {
    Int32 result = 0;
    itoa(result, defValue, 10);
    return result;
  }
}  // End of getMvqrCpusPerQMS

// *************************************************************** 
// *************************************************************** 
Int32 QRQueriesImpl::getMvqrQMSCpuOffset()
{
  static const char qmsCpusOffset[] = "MVQR_QMS_CPU_OFFSET";
  char defValue[MAX_DEFAULTS_UTF8_VALUE_IN_BYTES];

  if (getSystemDefault(qmsCpusOffset, defValue) == FALSE)
    return 0;  // Use the default value
  else 
  {
    Int32 result = 0;
    itoa(result, defValue, 10);
    return result;
  }
}  // End of getMvqrQMSCpuOffset

// *************************************************************** 
// * Obtain the CAT_UID for a specific catalog
// *
// * A static SQL query is used to obtain the CATALOG UID
// * for the input CATALOG name from the system metadata. 
// * 
// * The catalog name input to getCatalogUID is the
// * name used to obtain the CATALOG UID from the
// * system metadata.
// *************************************************************** 
Int64 QRQueriesImpl::getCatalogUID(const NAString* catalog)
{
  _int64 catalogUID = 0;

  Lng32 sqlCode = queries_->openCatalogUID(catalog->data());
  assertLogAndThrow1(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
                    sqlCode==SQL_Success, QRDatabaseException, 
		    "Error %d performing read catalog UID.", sqlCode); 

  sqlCode = queries_->fetchCatalogUID(catalogUID);

  // SQL_Eof means the catalog name does not exist.
  assertLogAndThrow1(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
                    sqlCode != SQL_Eof, QRDatabaseException, 
		    "Unable to obtain the UID for catalog %s", catalog); 
  // Otherwise its an SQL error.
  assertLogAndThrow1(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
                    sqlCode==SQL_Success, QRDatabaseException, 
		    "Error %d performing read catalog UID.", sqlCode); 

  // Always close to free up resources
  queries_->closeCatalogUID();

  return catalogUID;
} // End of getCatalogUID

// *************************************************************** 
// * Obtain the definition schema version for a specific catalog.
// *
// * A static SQL query is used to obtain the version of
// * the schema for a specific CATALOG UID from the system metadata.
// *************************************************************** 
const NAString* QRQueriesImpl::getSchemaVersion(Int64 catalogUID)
{
  // Reset the version to 0
  Int32 version = 0;

  Lng32 sqlCode = queries_->openVersion(catalogUID);
  assertLogAndThrow2(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
     sqlCode==SQL_Success, QRDatabaseException, 
    "Error %d performing get schema version for catalog with IUD %lld.", sqlCode, catalogUID); 


  sqlCode = queries_->fetchVersion(version);

  // Otherwise its an SQL error.
  assertLogAndThrow2(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
     ((sqlCode==SQL_Success) || (sqlCode==SQL_Eof)), QRDatabaseException,
    "Error %d performing get schema version for catalog with IUD %lld.", sqlCode, catalogUID);

  // Always close to free up resources
  queries_->closeVersion();

  // SQL_Eof means the catalog UID is empty - return NULL.
  if (sqlCode == SQL_Eof)
  {
    return NULL;
  }

  char buf[5];
  snprintf(buf, sizeof(buf), "%d",version);
  buf[4] = '\0';
  const NAString* versionString = new(heap_) NAString(buf, heap_);
  return versionString;
} // End of getSchemaVersion

// *************************************************************** 
// * Obtain a catalog name for the catalog UID specified.
// *
// * A static SQL query is used to obtain the
// * CATALOG name from the system metadata. 
// *************************************************************** 
const NAString* QRQueriesImpl::getCatalogName(Int64 catalogUID)
{
  NAString* catalogName = new(heap_) NAString(heap_);
  Lng32 sqlCode = queries_->openCatalogName(catalogUID);
  assertLogAndThrow1(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
                    sqlCode==SQL_Success, QRDatabaseException, 
		    "Error %d performing get catalog name.", sqlCode); 

  sqlCode = queries_->fetchCatalogName(*catalogName);
  // SQL_Eof means the catalog UID is wrong.
  assertLogAndThrow(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
                    sqlCode != SQL_Eof, QRDatabaseException, 
		    "Unable to obtain catalog name for UID."); 
  // Otherwise its an SQL error.
  assertLogAndThrow1(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
                    sqlCode==SQL_Success, QRDatabaseException, 
		    "Error %d performing get catalog name.", sqlCode); 

  // Always close to free up resources
  queries_->closeCatalogName();

  return catalogName;
}  // End of getCatalogName

// *************************************************************** 
// * Prepare the full definition schema name for the input catalog
// * name and schema version.
// * The input buffrer must be at least MAX_CATALOG_DEFINITION_NAME.
// *************************************************************** 
NAString* QRQueriesImpl::prepareDefinitionSchemaName(const NAString* catalog,
				                     const NAString* version)
{
  char buffer[MAX_CATALOG_DEFINITION_NAME];

  // Create the necessary table names
  char tempCatalog[MAX_CATALOG_DEFINITION_NAME];
  memset (tempCatalog, ' ', MAX_CATALOG_DEFINITION_NAME);
  tempCatalog[MAX_CATALOG_DEFINITION_NAME -1] = '\0';

  // Prepare the catalog name to handle any double quotes
  // within the delimited name
  fixupDelimitedName(catalog->data(), tempCatalog);

  // For simplicity, all catalog names are treated as delimited names.
  memset (buffer, ' ', MAX_CATALOG_DEFINITION_NAME);
  buffer[MAX_CATALOG_DEFINITION_NAME -1] = '\0';
  snprintf(buffer, sizeof(buffer), "\"%s\".HP_DEFINITION_SCHEMA", tempCatalog);

  return new(heap_) NAString(buffer, heap_);
}

// *************************************************************** 
// * Obtain all the MV descriptor text and MV attributes
// * for a specific MV UID in a specific catalog.
// *
// * A static SQL query is used to obtain the MV descriptor
// * text rows for a specific CATALOG, system schema VERSION
// * and MV UID from the system metadata.
// *************************************************************** 
NAString* QRQueriesImpl::getMvDescriptor(const NAString& textTable,
				         const char * uid)
{
  _int64 objectUID = atoInt64(uid);

  Lng32 sqlCode = queries_->openMvDescriptorText(textTable, objectUID);
  assertLogAndThrow1(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
                    sqlCode==SQL_Success || sqlCode==SQL_Eof, QRDatabaseException, 
 	            "Error %d performing get MV descriptor.", sqlCode); 

  NAString* mvDescriptorText = new(heap_) NAString(heap_);
  Int32 buffNum = 1;
  while (sqlCode == SQL_Success)
  {
    sqlCode = queries_->fetchMvDescriptorText(data_);
    assertLogAndThrow1(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
                      sqlCode==SQL_Success || sqlCode==SQL_Eof, QRDatabaseException, 
   	              "Error %d performing get MV descriptor.", sqlCode); 

    if (sqlCode == SQL_Success)
    {
      *mvDescriptorText += data_->mvText_;
    }
  }

  // Always close to free up resources
  queries_->closeMvDescriptorText();

  TrimNAStringSpace ( *mvDescriptorText // NAString &ns
                    , FALSE             // NABoolean leading
                    , TRUE              // NABoolean trailing
                    );

  return mvDescriptorText;
}  // End of getMvDescriptor

// *************************************************************** 
// * Obtain all the MV UIDs in a specific catalog.
// *
// * The QRMVData stucture, data_, is used to set specific information
// * as input and return selected output for the individual
// * select queries.
// *
// * The method inputs of catalog and version pass in
// * the variable settings stored in the QRMVData structure
// * for the static SQL query execution.
// *
// * A static SQL query will then be executed to obtain all the  
// * MV UIDs for a specific catalog in the metadata. 
// *************************************************************** 
QRMVDefinitionList* QRQueriesImpl::getMvUIDAndDefinitions(const NAString& definitionSchema)
{
  QRMVDefinitionList* MVDefinitions = new(heap_) QRMVDefinitionList(heap_);

  Lng32 sqlCode = queries_->openMvInformation(definitionSchema);
  assertLogAndThrow1(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
                    sqlCode==SQL_Success, QRDatabaseException, 
 	            "Error %d performing get MV UIDs.", sqlCode); 

  while (sqlCode == SQL_Success)
  {
    sqlCode = queries_->fetchMvInformation(data_);
    assertLogAndThrow1(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
                      sqlCode==SQL_Success || sqlCode==SQL_Eof, QRDatabaseException, 
   	              "Error %d performing get MV UIDs.", sqlCode); 

    if (sqlCode == SQL_Success)
    {
      QRMVDefinition* mv = new(heap_) QRMVDefinition(
        (data_->hasIgnoreChanges_ > 0 ? TRUE : FALSE),
         data_->redefTime_,
         data_->refreshedAt_,
         data_->objectUID_,
         heap_);

      MVDefinitions->insert(mv);
    }
  }

  // Always close to free up resources
  queries_->closeMvInformation();

  return MVDefinitions;
}  // End of getMvUIDAndDefinitions

// *************************************************************** 
// * Obtain all the catalog names on the system.
// *
// * A static SQL query is used to obtain all the
// * CATALOG names from the system metadata. 
// *************************************************************** 
const NAStringList* QRQueriesImpl::getAllCatalogNames()
{
  Lng32 sqlCode = queries_->openCatalogNames();
  assertLogAndThrow1(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
                    sqlCode==SQL_Success, QRDatabaseException, 
		    "Error %d performing get catalog name.", sqlCode); 

  NAStringList* catalogs = new(heap_) NAStringList(heap_);
  while (sqlCode == SQL_Success)
  {
    NAString* catalogName = new(heap_) NAString(heap_);
    sqlCode = queries_->fetchCatalogNames(*catalogName);
    assertLogAndThrow1(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
                      sqlCode==SQL_Success || sqlCode==SQL_Eof, QRDatabaseException, 
   	              "Error %d performing get catalog name.", sqlCode); 

    if (sqlCode == SQL_Success)
    {
      catalogs->insert(catalogName);
    }
  }

  // Always close to free up resources
  queries_->closeCatalogNames();

  return catalogs;
} // End of getCatalogNames

// *************************************************************** 
// * Prepare the object name to handle any double quotes
// * within the delimited name
// *************************************************************** 
void QRQueriesImpl::fixupDelimitedName(const char * inName, char * outName)
{
  // Prepare the object name to handle any double quotes
  // within the delimited name

  assertLogAndThrow(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
                    inName!=NULL && outName!=NULL, QRLogicException, 
		    "Bad input parameters to fixupDelimitedName()."); 
  
  Int32 j = 0;
  short len = strlen(inName);

  for (Int32 i = 0; i < len; i++)
  {
    if (inName[i] == '"')
      outName[j++] = '"';

    outName[j++] = inName[i];
  }

  outName[j] = '\0';
}  // End of fixupDelimitedName

// *************************************************************** 
// *************************************************************** 
void QRQueriesImpl::openRewritePublishCursor()
{
  char rewriteTable[MAX_REWRITE_TABLE];

  // Set up REWRITE_PUBLISH table name
  memset (rewriteTable, ' ', MAX_REWRITE_TABLE);
  strcpy(rewriteTable, "MANAGEABILITY.MV_REWRITE.REWRITE_PUBLISH");

  Lng32 sqlCode = queries_->openRewritePublish(rewriteTable);
  assertLogAndThrow1(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
                    sqlCode==SQL_Success || sqlCode==SQL_Eof, QRDatabaseException, 
 	            "Error %d opening REWRITE_PUBLISH table.", sqlCode); 
}

// *************************************************************** 
// Fetch and delete rows from the stream
// *************************************************************** 
NABoolean QRQueriesImpl::fetchRewritePublishCursor(MVQR_Publish* publish)
{
  // sqlCode 8006 is stream timeout - not an error.
  Lng32 sqlCode = queries_->fetchRewritePublish(publish);
  assertLogAndThrow1(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
                    sqlCode==SQL_Success || sqlCode==SQL_Eof || sqlCode == -8006, QRDatabaseException, 
 	            "Error %d performing reading REWRITE_PUBLISH table.", sqlCode); 

  //return (sqlCode == -8006);
  return (sqlCode == SQL_Eof);
}

// *************************************************************** 
// *************************************************************** 
void QRQueriesImpl::closeRewritePublishCursor()
{
  Lng32 sqlCode = queries_->closeRewritePublish();
  assertLogAndThrow1(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
                    sqlCode==SQL_Success || sqlCode==SQL_Eof, QRDatabaseException, 
 	            "Error %d closing REWRITE_PUBLISH table.", sqlCode); 
}

// *************************************************************** 
// Find all the MVs in a specific catalog, and collect their names.
// *************************************************************** 
NABoolean QRQueriesImpl::collectCatalogMVs(const NAString& catalogName, 
                                           const NAString& definitionSchema,
                                           NAStringList&   mvNames)
{
  Lng32 sqlCode = queries_->openMVNames(definitionSchema);
  assertLogAndThrow1(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
                    sqlCode==SQL_Success, QRDatabaseException, 
 	            "Error %d performing get MV names.", sqlCode); 

  while (sqlCode == SQL_Success)
  {
    NAString objectName(heap_);
    NAString schemaName(heap_);
    sqlCode = queries_->fetchMVNames(objectName, schemaName);
    assertLogAndThrow1(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
                      sqlCode==SQL_Success || sqlCode==SQL_Eof, QRDatabaseException, 
   	              "Error %d performing get MV names.", sqlCode); 

    if (sqlCode == SQL_Success)
    {
      NAString *fullMvName = new(heap_) NAString(catalogName, heap_);
      *fullMvName += ".";
      *fullMvName += schemaName;
      *fullMvName += ".";
      *fullMvName += objectName;

      mvNames.insert(fullMvName);
    }
  }

  // Always close to free up resources
  sqlCode = queries_->closeMVNames();
  if (sqlCode)
  {
    QRLogger::log(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
      "Error %d closing statement ReDescribeCatalog", sqlCode);
    return FALSE;
  }

  return TRUE;
}  // End of reDescribeCatalog

// *************************************************************** 
// Set the special parser flags so we can use internal syntax.
// *************************************************************** 
void QRQueriesImpl::setParserFlags()
{
  queries_->setParserFlags();
}

// *************************************************************** 
// Call catman to regenerate the MV descriptor for an MV.
// *************************************************************** 
NABoolean QRQueriesImpl::reDescribeMV(const NAString* fullMvName, NABoolean rePublish)
{
  Lng32 sqlCode = queries_->reDescribeMV(*fullMvName, rePublish);
  if (sqlCode != SQL_Success)
  {
    QRLogger::log(CAT_SQL_COMP_QR_COMMON, LL_ERROR,
      "ReDescribing MV %s failed with error %d.", fullMvName->data(), sqlCode);
    return FALSE;
  }
  else
  {
    QRLogger::log(CAT_SQL_COMP_QR_COMMON, LL_DEBUG,
      "ReDescribing MV %s is OK.", fullMvName->data());
    return TRUE;
  }
}

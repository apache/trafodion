//*****************************************************************************
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
//*****************************************************************************
  
// ==========================================================================
// Contains non inline methods in the following classes
//   PrivMgrMDAdmmin
// ==========================================================================

// Needed for parser flag manipulation
#define   SQLPARSERGLOBALS_FLAGS  
#include "SqlParserGlobalsCmn.h"
#include "SQLCLIdev.h"

#include "PrivMgrMD.h"
#include "PrivMgrMDDefs.h"
#include "PrivMgrPrivileges.h"
#include "PrivMgrRoles.h"
#include "PrivMgrComponents.h"
#include "PrivMgrComponentOperations.h"
#include "PrivMgrComponentPrivileges.h"
#include "PrivMgrObjects.h"
#include "CmpSeabaseDDLauth.h"
#include "CmpSeabaseDDL.h"
#include "ComUser.h"

#include <set>
#include <string>
#include <algorithm>
#include "ComSmallDefs.h"
#include "ComDistribution.h"
// sqlcli.h included because ExExeUtilCli.h needs it (and does not include it!)
#include "sqlcli.h"
#include "ExExeUtilCli.h"
#include "ComDiags.h"
#include "ComQueue.h"
#include "CmpCommon.h"
#include "CmpContext.h"
#include "CmpDDLCatErrorCodes.h"
#include "ComUser.h"

// *****************************************************************************
//    PrivMgrMDAdmin static methods
// *****************************************************************************

static bool compareTableDefs (
  const char * tableNameOne,
  const char * tableNameTwo,
  const std::string &objectsLocation,
  const std::string &colsLocation,
  ExeCliInterface &cliInterface,
  ComDiagsArea * pDiags);

static int32_t createTable (
  const char *tableName,
  const TableDDLString *tableDDL,
  ExeCliInterface &cliInterface,
  ComDiagsArea * pDiags);

static int32_t dropTable (
  const char *objectName,
  ExeCliInterface &cliInterface,
  ComDiagsArea * pDiags);

static void cleanupTable (
  const char *objectName,
  ExeCliInterface &cliInterface,
  ComDiagsArea * pDiags);
static int32_t renameTable (
  const char *originalObjectName,
  const char *newObjectName,
  ExeCliInterface &cliInterface,
  ComDiagsArea *pDiags);


// *****************************************************************************
//    PrivMgrMDAdmin class methods
// *****************************************************************************
// -----------------------------------------------------------------------
// Default Constructor
// -----------------------------------------------------------------------
PrivMgrMDAdmin::PrivMgrMDAdmin () 
: PrivMgr()
{
};

// --------------------------------------------------------------------------
// Construct a PrivMgrMDAdmin object for with a different metadata locations
// --------------------------------------------------------------------------
PrivMgrMDAdmin::PrivMgrMDAdmin ( 
   const std::string & trafMetadataLocation,
   const std::string & metadataLocation,
   ComDiagsArea * pDiags)
: PrivMgr(trafMetadataLocation, metadataLocation,pDiags)
{ };

// -----------------------------------------------------------------------
// Construct a PrivMgrMDAdmin object for with a different metadata location
// -----------------------------------------------------------------------
PrivMgrMDAdmin::PrivMgrMDAdmin ( 
   const std::string & metadataLocation,
   ComDiagsArea * pDiags)
: PrivMgr(metadataLocation,pDiags)
{ };

// -----------------------------------------------------------------------
// Copy constructor
// -----------------------------------------------------------------------
PrivMgrMDAdmin:: PrivMgrMDAdmin ( const PrivMgrMDAdmin &other )
 : PrivMgr(other)
{
}

// -----------------------------------------------------------------------
// Destructor.
// -----------------------------------------------------------------------
PrivMgrMDAdmin::~PrivMgrMDAdmin() 
{
}

// ----------------------------------------------------------------------------
// Method:  initializeComponentPrivileges
//
// This method registers standard Trafodion components, creates the 
// standard operations, and grants the privilege on those operations to
// the role DB__ROOTROLE.  SQL DDL operations (CREATE_SCHEMA, SHOW) are 
// granted to PUBLIC.
//
// Returns PrivStatus
//    STATUS_GOOD
//    STATUS_ERROR
//
// A cli error is put into the diags area if there is an error
// ----------------------------------------------------------------------------

PrivStatus PrivMgrMDAdmin::initializeComponentPrivileges()
{
   std::string traceMsg;
   log(__FILE__, "Initializing component privileges", -1);
   PrivStatus privStatus = STATUS_GOOD;

   PrivMgrComponents components(metadataLocation_,pDiags_);
   size_t numComps = sizeof(componentList)/sizeof(ComponentListStruct);
   for (int c = 0; c < numComps; c++)
   {
      // Get description of component
      const ComponentListStruct &compDefinition = componentList[c];
      int64_t compUID(compDefinition.componentUID);
      std::string compName(compDefinition.componentName);
      std::string compDef("System component ");
      compDef += compName;

      log(__FILE__, compDef, -1);

      bool componentExists = (components.exists(compName));
      if (!componentExists)
      {
        // Register component
        privStatus = components.registerComponentInternal(compName,compUID,true,compDef);
        if (privStatus != STATUS_GOOD)
        {
           traceMsg = "ERROR: unable to register component ";
           traceMsg += compName.c_str();
           log(__FILE__, traceMsg.c_str(), -1);
           return STATUS_ERROR;
        }
      }

      // Component is registered, now create all the operations associated with
      // the component.  A grant from the system to the owner (DB__ROOT) will
      // be added for each operation. In addition, set up the list of grants
      // for different users/roles.
      //   allOpsList - list of operations (granted to owner)
      //   rootRoleList - list of operations granted to DB__ROOTROLE
      //   publicList - list of operations granted to PUBLIC
      std::vector<std::string> allOpsList;
      std::vector<std::string> rootRoleList;
      std::vector<std::string> publicList;

      PrivMgrComponentPrivileges componentPrivileges(metadataLocation_,pDiags_);
      PrivMgrComponentOperations componentOperations(metadataLocation_,pDiags_);
      int32_t DB__ROOTID = ComUser::getRootUserID();
      std::string DB__ROOTName(ComUser::getRootUserName());

      int32_t numOps = compDefinition.numOps;
      int32_t numExistingOps = 0;
      int32_t numExistingUnusedOps = 0;
      if (componentOperations.getCount(compUID, numExistingOps, numExistingUnusedOps) == STATUS_ERROR)
        return STATUS_ERROR;

      // Add any new operations
      if ( numExistingOps < numOps)
      {
         // The ComponentOpStruct describes the component operations required for
         // each component. Each entry contains the operationCode,
         // operationName, whether the privileges should be granted for 
         // DB__ROOTROLE, and PUBLIC, etc. 
         for (int i = 0; i < numOps; i++)
         {
            const ComponentOpStruct opDefinition = compDefinition.componentOps[i];

            std::string description = "Allow grantee to perform ";
            description += opDefinition.operationName;
            description += " operation";

            // create the operation
            privStatus = componentOperations.createOperationInternal(compUID,
                                                                     opDefinition.operationName,
                                                                     opDefinition.operationCode,
                                                                     opDefinition.unusedOp,
                                                                     description,
                                                                     DB__ROOTID,DB__ROOTName,-1,
                                                                     componentExists);
                                                       
           if (privStatus == STATUS_GOOD)
           {
              // All operations are included in the allOpsList
              allOpsList.push_back(opDefinition.operationName);
              if (opDefinition.isRootRoleOp)
                rootRoleList.push_back(opDefinition.operationCode);
              if (opDefinition.isPublicOp)
                publicList.push_back(opDefinition.operationCode);
           }
           else
           {
              traceMsg = "WARNING unable to create component operation: ";
              traceMsg += opDefinition.operationName;
              log(__FILE__, traceMsg, -1);
              return privStatus;
           }   
        }

        // In the unlikely event no operations were created, we are done.   
        if (allOpsList.size() == 0)
           return STATUS_GOOD;
      
        // Grant all SQL_OPERATIONS to DB__ROOTROLE WITH GRANT OPTION                                      
        privStatus = componentPrivileges.grantPrivilegeInternal(compUID,
                                                                rootRoleList,
                                                                DB__ROOTID,
                                                                ComUser::getRootUserName(),
                                                                ROOT_ROLE_ID,
                                                                DB__ROOTROLE,-1,
                                                                componentExists);
                                                           
        if (privStatus != STATUS_GOOD)
        {
           traceMsg = "ERROR unable to grant DB__ROOTROLE to components";
           log(__FILE__, traceMsg, -1);
           return privStatus;
        }
                                      
        // Grant privileges to PUBLIC
        privStatus = componentPrivileges.grantPrivilegeInternal(compUID,
                                                                publicList,
                                                                DB__ROOTID,
                                                                ComUser::getRootUserName(),
                                                                PUBLIC_USER,
                                                                PUBLIC_AUTH_NAME,0,
                                                                componentExists);
        if (privStatus != STATUS_GOOD)
        {
           traceMsg = "ERROR unable to grant PUBLIC to components";
           log(__FILE__, traceMsg, -1);
           return privStatus;
        }
      }

      // Update component_privileges and update operation codes appropriately
      size_t numUnusedOps = PrivMgr::getSQLUnusedOpsCount();
      if (numExistingOps > 0 /* doing upgrade */ &&
          (numUnusedOps != numExistingUnusedOps))
      {
         privStatus = componentOperations.updateOperationCodes(compUID);
         if (privStatus == STATUS_ERROR)
            return privStatus;
      }

      // Verify counts from tables.

      // Minimum number of privileges granted is:
      //   one for each operation (owner)
      //   one for each entry in rootRoleList and publicList
      // This check was added because of issues with insert/upsert, is it still needed?
      int64_t expectedPrivCount = numOps + rootRoleList.size() + publicList.size();

      if (componentPrivileges.getCount(compUID) < expectedPrivCount)
      {
         std::string message ("Expecting ");
         message += to_string((long long int)expectedPrivCount);
         message += " component privileges, instead ";
         message += PrivMgr::authIDToString(numExistingOps);
         message += " were found.";
         traceMsg = "ERROR: ";
         traceMsg += message;
         log(__FILE__, message, -1);
         PRIVMGR_INTERNAL_ERROR(message.c_str());
         return STATUS_ERROR;
      }
   }
   return STATUS_GOOD; 
}

// ----------------------------------------------------------------------------
// Method:  initializeMetadata
//
// This method creates the metadata tables needed for privilege management
//
// Returns PrivStatus
//    STATUS_GOOD
//    STATUS_WARNING
//    STATUS_NOTFOUND
//    STATUS_ERROR
//
// A cli error is put into the diags area if there is an error
// ----------------------------------------------------------------------------
PrivStatus PrivMgrMDAdmin::initializeMetadata (
  const std::string &objectsLocation,
  const std::string &authsLocation,
  const std::string &colsLocation,
  std::vector<std::string> &tablesCreated,
  std::vector<std::string> &tablesUpgraded)

{
  std::string traceMsg;
  log (__FILE__, "*** Initialize Authorization ***", -1);

  PrivStatus retcode = STATUS_GOOD;

  // Authorization check
  if (!isAuthorized())
  {
    *pDiags_ << DgSqlCode (-CAT_NOT_AUTHORIZED);
    return STATUS_ERROR;
  }

  Int32 cliRC = 0;
  ExeCliInterface cliInterface(STMTHEAP, 0, NULL, 
                          CmpCommon::context()->sqlSession()->getParentQid());
  
  // See what tables exist
  std::set<std::string> existingObjectList;
  PrivMDStatus initStatus = authorizationEnabled(existingObjectList);

  // If unable to access metadata, return STATUS_ERROR 
  //   (pDiags contains error details)
  if (initStatus == PRIV_INITIALIZE_UNKNOWN)
    return STATUS_ERROR;

  // Create the privilege manager schema if it doesn't yet exists.
  if (initStatus == PRIV_UNINITIALIZED)
  {
    log (__FILE__, "Creating _PRIVMGR_MD_ schema", -1);

    std::string schemaCommand("CREATE PRIVATE SCHEMA IF NOT EXISTS ");
  
    schemaCommand += metadataLocation_;
    cliRC = cliInterface.executeImmediate(schemaCommand.c_str());
    if (cliRC < 0)
      throw STATUS_ERROR;
  }
    
  // Create or upgrade the tables
  //   If table does not exist - create it
  //   If table exists 
  //     If doesn't need upgrading - done
  //     else - upgrade table 
  bool populateObjectPrivs = false;

  try
  {
    size_t numTables = sizeof(privMgrTables)/sizeof(PrivMgrTableStruct);
    bool doCreate = (initStatus == PRIV_UNINITIALIZED);

    log (__FILE__, "Creating _PRIVMGR_MD_ tables", -1);
    for (int ndx_tl = 0; ndx_tl < numTables; ndx_tl++)
    {
      const PrivMgrTableStruct &tableDefinition = privMgrTables[ndx_tl];
      std::string tableName = deriveTableName(tableDefinition.tableName);

      if (initStatus == PRIV_PARTIALLY_INITIALIZED)
      {
        // See if table needs to be created
        std::string metadataTable (tableDefinition.tableName);
        std::set<std::string>::iterator it;
        it = std::find(existingObjectList.begin(), existingObjectList.end(), metadataTable);
        doCreate = (it == existingObjectList.end());
      }

      // Create tables for installations or upgrades 
      if (doCreate)
      {
        traceMsg = ": ";
        traceMsg = tableName;
        log (__FILE__, traceMsg, -1);

        cliRC = createTable(tableName.c_str(), tableDefinition.tableDDL, 
                            cliInterface, pDiags_);

        // If create was successful, set flags to load default data
        if (cliRC < 0)
        {
          log (__FILE__, " create failed", -1);
          throw STATUS_ERROR;
        }
       
        tablesCreated.push_back(tableDefinition.tableName);

        if (tableDefinition.tableName == PRIVMGR_OBJECT_PRIVILEGES)
          populateObjectPrivs = true;
      }

      // upgrade tables
      else 
      {
#if 0
        retcode = upgradeMetadata(tableDefinition, cliInterface,
                                  objectsLocation, colsLocation); 
        if (retcode == STATUS_ERROR)
          throw STATUS_ERROR;

        tablesUpgraded.push_back(tableDefinition.tableName);
#endif
      }
    }
 
    // populate metadata tables
    PrivStatus privStatus = updatePrivMgrMetadata
      (objectsLocation,authsLocation,
       populateObjectPrivs);

    // if error occurs, drop tables already created
    if (privStatus == STATUS_ERROR)
      throw STATUS_ERROR;

    //TODO: should notify QI?
  } 

  catch (...)
  {
     tablesCreated.clear();
     tablesUpgraded.clear();

     // assume ddlTxn will be turned on
     // if not need to redo work just performed
     return STATUS_ERROR;
  }
  log(__FILE__, "*** Initialize authorization completed ***", -1);
  return STATUS_GOOD;
}

// ----------------------------------------------------------------------------
// Method:  upgradeMetadata
//
// This method checks to see if the metadata tables needs to be upgraded.
// If so, it is upgraded.
//
// Params:
//    tableDefinition - definition of table that may need upgrading
//    cliInterface - infrastructure for making SQL calls
//    objectsLocation - name of OBJECTS system metadata table
//    colsLocation - name of COLUMNS system metadata table
//
// Returns PrivStatus
//    STATUS_GOOD
//    STATUS_ERROR
//
// A cli error is put into the diags area if there is an error
// ----------------------------------------------------------------------------
//
PrivStatus PrivMgrMDAdmin::upgradeMetadata (
  const PrivMgrTableStruct &tableDefinition,
  ExeCliInterface &cliInterface,
  const std::string &objectsLocation,
  const std::string &colsLocation)
{
  // create a different table with the current definition
  std::string newTableName = tableDefinition.tableName + std::string("_NEW");
  std::string qualNewTableName = deriveTableName(newTableName.c_str());
  Int32 cliRC = createTable(qualNewTableName.c_str(), tableDefinition.tableDDL, 
                            cliInterface, pDiags_);
  if (cliRC < 0)
    return STATUS_ERROR;

  // if tables match, no upgrade is needed, return STATUS_GOOD
  if (compareTableDefs(newTableName.c_str(), tableDefinition.tableName, 
                       objectsLocation, colsLocation, 
                       cliInterface, pDiags_))
    {
      // Done with new table, go ahead and drop
      cliRC = dropTable(qualNewTableName.c_str(), cliInterface, pDiags_);
      if (cliRC < 0)
        return STATUS_ERROR;
      return STATUS_GOOD;
    }


  // TDB -- copy data

  // drop original table 
  cliRC = dropTable(qualNewTableName.c_str(), cliInterface, pDiags_);
  if (cliRC < 0)
    return STATUS_ERROR;

  // rename new version table
  // When using this code, error 1390 is returned:  <table> already exists
  cliRC = renameTable(tableDefinition.tableName, qualNewTableName.c_str(), 
                      cliInterface, pDiags_);
  if (cliRC < 0)
    return STATUS_ERROR;

  return STATUS_GOOD;
}

// ----------------------------------------------------------------------------
// Method:  dropMetadata
//
// This method drops the metadata tables used by privilege management
//
// Returns PrivStatus
//    STATUS_GOOD
//    STATUS_WARNING
//    STATUS_NOTFOUND
//    STATUS_ERROR
//
// A cli error is put into the diags area if there is an error
// ----------------------------------------------------------------------------
PrivStatus PrivMgrMDAdmin::dropMetadata (
  const std::vector<std::string> &objectsToDrop,
  bool doCleanup)
{
  std::string traceMsg;
  log (__FILE__, "*** Drop Authorization ***", -1);
  PrivStatus retcode = STATUS_GOOD;
    
  // Authorization check
  if (!isAuthorized())
  {
     *pDiags_ << DgSqlCode (-CAT_NOT_AUTHORIZED);
     return STATUS_ERROR;
   }

  ExeCliInterface cliInterface(STMTHEAP, 0, NULL, CmpCommon::context()->sqlSession()->getParentQid());
  Int32 cliRC = 0;
  if (doCleanup)
    cleanupMetadata(cliInterface);
  else
  {
    // See what does and does not exist
    std::set<std::string> existingObjectList;
    PrivMDStatus initStatus = authorizationEnabled(existingObjectList);

    // If unable to access metadata, return STATUS_ERROR 
    //   (pDiags contains error details)
    if (initStatus == PRIV_INITIALIZE_UNKNOWN)
    {
      log(__FILE__, "ERROR: unable to access PRIVMGR metadata", -1);
      return STATUS_ERROR;
    }

    // If metadata tables don't exist, just return STATUS_GOOD
    if (initStatus == PRIV_UNINITIALIZED)
    {
      log(__FILE__, "WARNING: authorization is not enabled", -1);
      return STATUS_GOOD;
    }
  }

  // Call Trafodion to drop the schema cascade

  log (__FILE__, "dropping _PRIVMGR_MD_ schema cascade", -1);
  std::string schemaDDL("DROP SCHEMA IF EXISTS ");
  schemaDDL += metadataLocation_;
  schemaDDL += "CASCADE";
  cliRC = cliInterface.executeImmediate(schemaDDL.c_str());
  if (cliRC < 0)
  {
    traceMsg = "ERROR unable to drop schema cascade: ";
    traceMsg += to_string((long long int)cliRC);
    log(__FILE__, traceMsg, -1);
    cliInterface.retrieveSQLDiagnostics(pDiags_);
    retcode = STATUS_ERROR;
  }

  CmpSeabaseDDLrole role;
  std::vector<std::string> rolesCreated;
  int32_t numberRoles = sizeof(systemRoles)/sizeof(SystemAuthsStruct);
  for (int32_t i = 0; i < numberRoles; i++)
  {
    const SystemAuthsStruct &roleDefinition = systemRoles[i];

    // special Auth includes roles that are not registered in the metadata
    if (roleDefinition.isSpecialAuth)
      continue;

    role.dropStandardRole(roleDefinition.authName);
  }

  int32_t actualSize = 0;
  char buf[500];
  ComUser::getRoleList(buf, actualSize, 500);
  buf[actualSize] = 0;
  traceMsg = "dropped roles: ";
  traceMsg + buf;
  log(__FILE__, traceMsg,  -1);

//TODO: should notify QI
  log (__FILE__, "*** drop authorization completed ***", -1);
  return retcode;
}

// ----------------------------------------------------------------------------
// Method:  cleanupMetadata
//
// This method cleans up the metadata tables used by privilege management
//
// Error messages are expected, so they are suppressed
// ----------------------------------------------------------------------------
void PrivMgrMDAdmin::cleanupMetadata (ExeCliInterface &cliInterface)
{
  std::string traceMsg;
  log (__FILE__, "cleaning up PRIVMGR tables: ", -1);

  // cleanup histogram tables, if they exist
  std::vector<std::string> histTables = CmpSeabaseDDL::getHistogramTables();
  Int32 numHistTables = histTables.size();
  for (Int32 i = 0; i < numHistTables; i++)
  {
    cleanupTable(histTables[i].c_str(), cliInterface, pDiags_);
  }

  size_t numTables = sizeof(privMgrTables)/sizeof(PrivMgrTableStruct);
  for (int ndx_tl = 0; ndx_tl < numTables; ndx_tl++)
  {
    const PrivMgrTableStruct &tableDefinition = privMgrTables[ndx_tl];
    std::string tableName = deriveTableName(tableDefinition.tableName);
    log (__FILE__, tableName, -1);
    cleanupTable(tableName.c_str(), cliInterface, pDiags_);
  }
}

// ----------------------------------------------------------------------------
// Method:  isAuthorized
//
// This method verifies that the current user is able to initialize or
// drop privilege manager metadata.  Currently this is restricted to the
// root database user, but in the future an operator or service ID may have
// the authority.
//
// Returns true if user is authorized
// ----------------------------------------------------------------------------
bool PrivMgrMDAdmin::isAuthorized (void)
{
  return ComUser::isRootUserID();
}

// ****************************************************************************
// method:  getColumnReferences
//
//  This method stores the list of columns for the object in the 
//  ObjectReference. 
// **************************************************************************** 
PrivStatus PrivMgrMDAdmin::getColumnReferences (ObjectReference *objectRef)
{
  std::string colMDTable = trafMetadataLocation_ + ".COLUMNS c";

  // Select column details for object 
  std::string selectStmt = "select c.column_number from ";
  selectStmt += colMDTable;
  selectStmt += " where c.object_uid = ";
  selectStmt += UIDToString(objectRef->objectUID);
  selectStmt += " order by column_number";

  ExeCliInterface cliInterface(STMTHEAP, 0, NULL,
  CmpCommon::context()->sqlSession()->getParentQid());
  Queue * objectsQueue = NULL;

  int32_t cliRC =  cliInterface.fetchAllRows(objectsQueue, (char *)selectStmt.c_str(), 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
  {
    cliInterface.retrieveSQLDiagnostics(pDiags_);
    return STATUS_ERROR;
  }

  if (cliRC == 100) // did not find the row
  {
    std::string message ("No columns found for referenced object");
    PRIVMGR_INTERNAL_ERROR(message.c_str());
    return STATUS_ERROR;
  }

  char * ptr = NULL;
  Int32 len = 0;

  objectRef->columnReferences = new std::vector<ColumnReference *>;

  // For each row, create a ColumnReference and add it to the objectRef
  objectsQueue->position();
  for (int idx = 0; idx < objectsQueue->numEntries(); idx++)
  {
    OutputInfo * pCliRow = (OutputInfo*)objectsQueue->getNext();
    ColumnReference *columnReference = new ColumnReference;

    // column 0:  columnNumber
    pCliRow->get(0,ptr,len);
    columnReference->columnOrdinal = *(reinterpret_cast<int32_t*>(ptr));

    objectRef->columnReferences->push_back(columnReference);
  }
  return STATUS_GOOD;
}

// ****************************************************************************
// method:  getViewColUsages
//
//  This method reads the TEXT table to obtain the view-col <=> referenced-col
//  relationship.
//
//  This relationship is stored in one or more text records with the text_type
//  COM_VIEW_REF_COLS_TEXT (8) see ComSmallDefs.h 
//
//  The text rows are concatenated together and saved in the ViewUsage.
// **************************************************************************** 
PrivStatus PrivMgrMDAdmin::getViewColUsages (ViewUsage &viewUsage)
{
  std::string textMDTable = trafMetadataLocation_ + ".TEXT t";

  // Select text rows describing view <=> object column relationships
  std::string selectStmt = "select text from ";
  selectStmt += textMDTable;
  selectStmt += " where t.text_uid = ";
  selectStmt += UIDToString(viewUsage.viewUID);
  selectStmt += "and t.text_type = 8";
  selectStmt += " order by seq_num";
  
  ExeCliInterface cliInterface(STMTHEAP, 0, NULL,
  CmpCommon::context()->sqlSession()->getParentQid());
  Queue * objectsQueue = NULL;
  
  int32_t cliRC =  cliInterface.fetchAllRows(objectsQueue, (char *)selectStmt.c_str(), 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
  { 
    cliInterface.retrieveSQLDiagnostics(pDiags_);
    return STATUS_ERROR;
  }
  
  // View prior to column privilege support do not store view <=> object column
  // relationships.  These views were not created based on column privileges
  // so just return.
  if (cliRC == 100) // did not find the row
  { 
    return STATUS_NOTFOUND;
  }
  
  char * ptr = NULL;
  Int32 len = 0;

  // the text column length in the TEXT table is 10000
  char value[10000 + 1];
  
  // For each row, add it to the existing viewColUsages string
  objectsQueue->position();
  for (int idx = 0; idx < objectsQueue->numEntries(); idx++)
  { 
    OutputInfo * pCliRow = (OutputInfo*)objectsQueue->getNext();

    // column 0: text 
    pCliRow->get(0,ptr,len);
    strncpy(value, ptr, len);
    value[len] = 0;
    viewUsage.viewColUsagesStr += value;
  }

  return STATUS_GOOD;
} 





// ****************************************************************************
// method:  getViewsThatReferenceObject
//
//  this method gets the list of views associated with the passed in 
//  objectUID that are owned by the granteeID.
// **************************************************************************** 
PrivStatus PrivMgrMDAdmin::getViewsThatReferenceObject (
  const ObjectUsage &objectUsage,
  std::vector<ViewUsage> &viewUsages )
{
  std::string objectMDTable = trafMetadataLocation_ + ".OBJECTS o";
  std::string viewUsageMDTable = trafMetadataLocation_ + ".VIEWS_USAGE u";
  std::string viewsMDTable = trafMetadataLocation_ + ".VIEWS v";
  std::string roleUsageMDTable = metadataLocation_ + ".ROLE_USAGE";

  // Select all the views that are referenced by the table or view owned by the granteeID
  std::string selectStmt = "select o.object_uid, o.object_owner, o.catalog_name, o.schema_name, o.object_name, v.is_insertable, v.is_updatable from ";
  selectStmt += objectMDTable;
  selectStmt += ", ";
  selectStmt += viewUsageMDTable;
  selectStmt += ", ";
  selectStmt += viewsMDTable;
  selectStmt += " where o.object_type = 'VI' and u.used_object_uid = ";
  selectStmt += UIDToString(objectUsage.objectUID);
  selectStmt += " and u.using_view_uid = o.object_uid ";
  selectStmt += "and o.object_uid = v.view_uid";

  // only return rows where user owns the view either directly or through one of
  // their granted roles
  selectStmt += " and (o.object_owner = ";
  selectStmt += UIDToString(objectUsage.granteeID);
  selectStmt += "  or o.object_owner in (select role_id from ";
  selectStmt += roleUsageMDTable;
  selectStmt += " where grantee_id = ";
  selectStmt += UIDToString(objectUsage.granteeID);

  // for role owners, get list of users granted role
  selectStmt += " ) or o.object_owner in (select grantee_id from ";
  selectStmt += roleUsageMDTable;
  selectStmt += " where role_id = ";
  selectStmt += UIDToString(objectUsage.granteeID);

  selectStmt += ")) order by o.create_time ";

  ExeCliInterface cliInterface(STMTHEAP, 0, NULL, 
  CmpCommon::context()->sqlSession()->getParentQid());
  Queue * objectsQueue = NULL;

// set pointer in diags area
int32_t diagsMark = pDiags_->mark();

  int32_t cliRC =  cliInterface.fetchAllRows(objectsQueue, (char *)selectStmt.c_str(), 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
  {
    cliInterface.retrieveSQLDiagnostics(pDiags_);
    return STATUS_ERROR;
  }

  if (cliRC == 100) // did not find the row
  {
    pDiags_->rewind(diagsMark);
    return STATUS_NOTFOUND;
  }

  char * ptr = NULL;
  Int32 len = 0;
  char value[MAX_SQL_IDENTIFIER_NAME_LEN + 1];

  // For each row returned, add it to the viewUsages structure.
  objectsQueue->position();
  for (int idx = 0; idx < objectsQueue->numEntries(); idx++)
  {
    OutputInfo * pCliRow = (OutputInfo*)objectsQueue->getNext();
    ViewUsage viewUsage;

    // column 0:  object uid
    pCliRow->get(0,ptr,len);
    viewUsage.viewUID = *(reinterpret_cast<int64_t*>(ptr));

    // column 1: object owner
    pCliRow->get(1,ptr,len);
    viewUsage.viewOwner = *(reinterpret_cast<int32_t*>(ptr));

    // column 2: catalog name
    pCliRow->get(2,ptr,len);
    assert (len < 257);
    strncpy(value, ptr, len);
    value[len] = 0;
    std::string viewName(value);
    viewName += ".";

    // column 3:  schema name
    pCliRow->get(3,ptr,len);
    assert (len < 257);
    strncpy(value, ptr, len);
    value[len] = 0;
    viewName += value;
    viewName += ".";

    // column 4:  object name
    pCliRow->get(4,ptr,len);
    assert (len < 257);
    strncpy(value, ptr, len);
    value[len] = 0;
    viewName += value;
    viewUsage.viewName = viewName;

    // column 5: is insertable
    pCliRow->get(5,ptr,len);
    viewUsage.isInsertable = (*ptr == 0) ? false : true;

    // column 6: is updatable
    pCliRow->get(6,ptr,len);
    viewUsage.isUpdatable = (*ptr == 0) ? false : true;

    viewUsages.push_back(viewUsage);
  }

  return STATUS_GOOD;
}

// ****************************************************************************
// method:  getObjectsThatViewReferences
//
//  this method gets the list of objects (tables or views) that are referenced
//  by the view.
// **************************************************************************** 
PrivStatus PrivMgrMDAdmin::getObjectsThatViewReferences (
   const ViewUsage &viewUsage,
   std::vector<ObjectReference *> &objectReferences )
{
  std::string objectMDTable = trafMetadataLocation_ + ".OBJECTS o";
  std::string viewUsageMDTable = trafMetadataLocation_ + ".VIEWS_USAGE u";

  // Select all the objects that are referenced by the view
  std::string selectStmt = "select o.object_uid, o.object_owner, o.object_type, ";
  selectStmt += "trim(o.catalog_name) || '.\"' || ";
  selectStmt += "trim (o.schema_name) || '\".\"' || ";
  selectStmt += "trim (o.object_name) || '\"' from ";
  selectStmt += objectMDTable;
  selectStmt += ", ";
  selectStmt += viewUsageMDTable;
  selectStmt += " where u.using_view_uid = ";
  selectStmt += UIDToString(viewUsage.viewUID);
  selectStmt += " and u.used_object_uid = o.object_uid ";
  selectStmt += " order by o.create_time ";

  ExeCliInterface cliInterface(STMTHEAP, 0, NULL, 
  CmpCommon::context()->sqlSession()->getParentQid());
  Queue * objectsQueue = NULL;

// set pointer in diags area
int32_t diagsMark = pDiags_->mark();

  int32_t cliRC =  cliInterface.fetchAllRows(objectsQueue, (char *)selectStmt.c_str(), 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
  {
    cliInterface.retrieveSQLDiagnostics(pDiags_);
    return STATUS_ERROR;
  }

  if (cliRC == 100) // did not find the row
  {
    pDiags_->rewind(diagsMark);
    return STATUS_NOTFOUND;
  }

  char * ptr = NULL;
  Int32 len = 0;
  char value[MAX_SQL_IDENTIFIER_NAME_LEN + 1];

  // For each row returned, add it to the viewUsages structure.
  objectsQueue->position();
  for (int idx = 0; idx < objectsQueue->numEntries(); idx++)
  {
    OutputInfo * pCliRow = (OutputInfo*)objectsQueue->getNext();
    ObjectReference *pObjectReference = new ObjectReference;

    // column 0:  object uid
    pCliRow->get(0,ptr,len);
    pObjectReference->objectUID = *(reinterpret_cast<int64_t*>(ptr));

    // column 1: object owner
    pCliRow->get(1,ptr,len);
    pObjectReference->objectOwner = *(reinterpret_cast<int32_t*>(ptr));

    // column 2: object type
    pCliRow->get(2,ptr,len);
    strncpy(value, ptr, len);
    value[len] = 0;
    pObjectReference->objectType = ObjectLitToEnum(value);

    // column 3: object name
    pCliRow->get(3,ptr,len);
    strncpy(value, ptr, len);
    value[len] = 0;
    pObjectReference->objectName = value;

    objectReferences.push_back(pObjectReference);
  }

  return STATUS_GOOD;
}

// ****************************************************************************
// method:  getUdrsThatReferenceLibrary
//
// This method gets the list of objects (functions & procedures) that are 
// referenced by the library.
//
// **************************************************************************** 
PrivStatus PrivMgrMDAdmin::getUdrsThatReferenceLibrary(
  const ObjectUsage &objectUsage,
  std::vector<ObjectReference *> &objectReferences )
{
  std::string objectMDTable = trafMetadataLocation_ + ".OBJECTS o";
  std::string librariesUsageMDTable = trafMetadataLocation_ + ".LIBRARIES_USAGE u";
  std::string roleUsageMDTable = metadataLocation_ + ".ROLE_USAGE r";

  // Select all the objects that are referenced by the library
  std::string selectStmt = "select o.object_uid, o.object_owner, o.object_type, ";
  selectStmt += "trim(o.catalog_name) || '.\"' || ";
  selectStmt += "trim (o.schema_name) || '\".\"' ||";
  selectStmt += "trim (o.object_name)|| '\"' from ";
  selectStmt += objectMDTable;
  selectStmt += ", ";
  selectStmt += librariesUsageMDTable;
  selectStmt += " where u.using_library_uid = ";
  selectStmt += UIDToString(objectUsage.objectUID);
  selectStmt += " and u.used_udr_uid = o.object_uid ";
  selectStmt += " and (o.object_owner = ";
  selectStmt += UIDToString(objectUsage.granteeID);
  selectStmt += "  or o.object_owner in (select role_id from ";
  selectStmt += roleUsageMDTable;
  selectStmt += " where grantee_id = ";
  selectStmt += UIDToString(objectUsage.granteeID);
  selectStmt += ")) order by o.create_time ";

  ExeCliInterface cliInterface(STMTHEAP, 0, NULL, 
  CmpCommon::context()->sqlSession()->getParentQid());
  Queue * objectsQueue = NULL;

  // set pointer in diags area
  int32_t diagsMark = pDiags_->mark();

  int32_t cliRC =  cliInterface.fetchAllRows(objectsQueue, (char *)selectStmt.c_str(), 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
  {
    cliInterface.retrieveSQLDiagnostics(pDiags_);
    return STATUS_ERROR;
  }

  if (cliRC == 100) // did not find the row
  {
    pDiags_->rewind(diagsMark);
    return STATUS_NOTFOUND;
  }

  char * ptr = NULL;
  Int32 len = 0;
  char value[MAX_SQL_IDENTIFIER_NAME_LEN + 1];

  // For each row returned, add it to the objectReferences structure.
  objectsQueue->position();
  for (int idx = 0; idx < objectsQueue->numEntries(); idx++)
  {
    OutputInfo * pCliRow = (OutputInfo*)objectsQueue->getNext();
    ObjectReference *pObjectReference = new ObjectReference;

    // column 0:  object uid
    pCliRow->get(0,ptr,len);
    pObjectReference->objectUID = *(reinterpret_cast<int64_t*>(ptr));

    // column 1: object owner
    pCliRow->get(1,ptr,len);
    pObjectReference->objectOwner = *(reinterpret_cast<int32_t*>(ptr));

    // column 2: object type
    pCliRow->get(2,ptr,len);
    strncpy(value, ptr, len);
    value[len] = 0;
    pObjectReference->objectType = ObjectLitToEnum(value);

    // column 3: object name
    pCliRow->get(3,ptr,len);
    strncpy(value, ptr, len);
    value[len] = 0;
    pObjectReference->objectName = value;

    objectReferences.push_back(pObjectReference);
  }

  return STATUS_GOOD;
}

// ****************************************************************************
//
// method:  getReferencingTablesForConstraints
//
// This method returns the list of underlying tables and their columns that are 
// associated with RI constraints referencing an ObjectUsage.
//
// An RI constraint is a relationship between a set of columns on one table 
// (the referencing table) with a set of columns on another table (the 
// referenced table).  The set of columns must be defined as an unique 
// constraint (which include primary key constraints) on the referenced table.  
// Relationships are stored through the constraints not their underlying tables.
//
//  for example:
//    user1:
//      create table dept (dept_no int not null primary key, ... );
//      grant references on dept to user2;
//    
//    user2:
//      create table empl (empl_no int not null primary key, dept_no int, ... );
//      alter table empl 
//          add constraint empl_dept foreign key(dept_no) references dept;
//
//  empl_dept is the name of the RI constraint
//     The empl table references dept 
//     The dept table is being referenced by empl
//
//  The following query is called to get the list of tables and their columns:  
//    <Gets underlying table and columns for the foreign key's constraint>
//    select (for referenced table)
//       objects (o): object_uid, object_owner, object_type, create_time, name,
//       list of columns (c): column_number
//    from table_constraints t, objects o, unique_ref_constr_usage u
//         (list of foreign key/unique constraint uids on referenced table) r,
//         (list of column numbers on referenced table's unique constraints) c
//    where o.object_uid = t.table_uid
//      and t.constraint_uid = r.foreign_constraint_uid
//      and r.unique_constraint_uid = c.object_uid
//    order by object_owner & create time
//
//    <Get list of foreign key/unique constraint uids on reference table>
//    (select foreign_constraint_uid, unique_constraint_uid
//     from table_constraints t, unique_ref_constr_usage u
//     where t.table_uid = objectUsage.objectUID
//       and t.constraint_uid = u.unique_constraint_uid)
//     order by o.create_time
//
//    <Get list of column numbers on referenced table's unique constraints>
//    (select object_uid, column_number, column_name
//     from TRAFODION."_MD_".KEYS 
//     where object_uid in
//       (select unique_constraint_uid
//        from TABLE_CONSTRAINTS t,UNIQUE_REF_CONSTR_USAGE u
//        where t.table_uid = objectUsage.objectUID
//          and t.constraint_uid = u.unique_constraint_uid))
//    
// input:  ObjectUsage - object desiring list of referencing tables
//         In the example above, this would be the DEPT table
//
// output: std::vector<ObjectReference *> - list of table references describing
//         the underlying table of one or more referencing constraints
//         In the example above, EMPL would be returned
//
// **************************************************************************** 
PrivStatus PrivMgrMDAdmin::getReferencingTablesForConstraints (
   const ObjectUsage &objectUsage,
   std::vector<ObjectReference *> &objectReferences )
{
  std::string objectsMDTable = 
    trafMetadataLocation_ + ".OBJECTS o";
  std::string tblConstraintsMDTable = 
    trafMetadataLocation_ + ".TABLE_CONSTRAINTS t";
  std::string keysMDTable = 
    trafMetadataLocation_ + ".KEYS k";
  std::string uniqueRefConstraintsMDTable = 
    trafMetadataLocation_ + ".UNIQUE_REF_CONSTR_USAGE u";

  // Select all the constraints that are referenced by the table
  std::string selectStmt = "select distinct o.object_uid, o.object_owner, o.object_type, o.create_time, ";
  selectStmt += "trim(o.catalog_name) || '.\"' || ";
  selectStmt +=   "trim (o.schema_name) || '\".\"' ||";
  selectStmt +=   "trim (o.object_name)|| '\"' ";
  selectStmt += ", c.column_number "; 
  selectStmt += "from " + uniqueRefConstraintsMDTable + std::string(", ");
  selectStmt += tblConstraintsMDTable + std::string(", ") + objectsMDTable;
  selectStmt += " , (select foreign_constraint_uid, unique_constraint_uid from ";
  selectStmt += tblConstraintsMDTable + std::string(", ") + uniqueRefConstraintsMDTable;
  selectStmt += " where t.table_uid = " + UIDToString(objectUsage.objectUID);
  selectStmt += " and t.constraint_uid = u.unique_constraint_uid) r ";
  selectStmt += " , (select object_uid, column_number, column_name from ";
  selectStmt += keysMDTable;
  selectStmt += " where object_uid in (select unique_constraint_uid from ";
  selectStmt += tblConstraintsMDTable + std::string(", ") ; 
  selectStmt += uniqueRefConstraintsMDTable + std::string(" where t.table_uid = ");
  selectStmt += UIDToString(objectUsage.objectUID);
  selectStmt += " and t.constraint_uid = u.unique_constraint_uid)) c ";
  selectStmt += "where o.object_uid = t.table_uid ";
  selectStmt += " and t.constraint_uid = r.foreign_constraint_uid ";
  selectStmt += " and r.unique_constraint_uid = c.object_uid ";
  selectStmt += " order by o.object_owner, o.create_time ";

  ExeCliInterface cliInterface(STMTHEAP, 0, NULL, 
  CmpCommon::context()->sqlSession()->getParentQid());
  Queue * objectsQueue = NULL;

  // set pointer in diags area
  int32_t diagsMark = pDiags_->mark();

  int32_t cliRC =  cliInterface.fetchAllRows(objectsQueue, (char *)selectStmt.c_str(), 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
  {
    cliInterface.retrieveSQLDiagnostics(pDiags_);
    return STATUS_ERROR;
  }

  if (cliRC == 100) // did not find the row
  {
    pDiags_->rewind(diagsMark);
    return STATUS_NOTFOUND;
  }

  char * ptr = NULL;
  Int32 len = 0;
  char value[MAX_SQL_IDENTIFIER_NAME_LEN + 1];
  objectsQueue->position();

  std::vector<ColumnReference *> *columnReferences = new std::vector<ColumnReference *>;
  ObjectReference *pObjectReference(NULL);
  ColumnReference *pColumnReference(NULL);
  int64_t currentObjectUID = 0;

  // Set up an objectReference for any objects found, the caller manages
  // space for this list
  for (int idx = 0; idx < objectsQueue->numEntries(); idx++)
  {
    OutputInfo * pCliRow = (OutputInfo*)objectsQueue->getNext();

    // column 0:  object uid
    pCliRow->get(0,ptr,len);
    int64_t nextObjectUID = *(reinterpret_cast<int64_t*>(ptr));

    // If a new object uid then save previous object reference
    // and prepare for new object reference
    if (nextObjectUID != currentObjectUID)
    {
      // Save off previous reference, if previous reference exists
      if (currentObjectUID > 0)
      {
        pObjectReference->columnReferences = columnReferences;
        objectReferences.push_back(pObjectReference);
      }
      currentObjectUID = nextObjectUID;

      // prepare for new object reference
      pObjectReference = new ObjectReference;
      columnReferences = new std::vector<ColumnReference *>;

      // object UID
      pObjectReference->objectUID = nextObjectUID;

      // column 1: object owner
      pCliRow->get(1,ptr,len);
      pObjectReference->objectOwner = *(reinterpret_cast<int32_t*>(ptr));

      // column 2: object type
      pCliRow->get(2,ptr,len);
      strncpy(value, ptr, len);
      value[len] = 0;
      pObjectReference->objectType = ObjectLitToEnum(value);

      // skip create_time (column 3)
    
      // column 4: object name
      pCliRow->get(4,ptr,len);
      strncpy(value, ptr, len);
      value[len] = 0;
      pObjectReference->objectName = value;
      
    }

    // set up the column reference
    // column 5: column number
    ColumnReference *pColumnReference = new ColumnReference;
    pCliRow->get(5,ptr,len);
    pColumnReference->columnOrdinal = *(reinterpret_cast<int32_t*>(ptr));

    // add to column list
    columnReferences->push_back(pColumnReference);
  }

  //  Add the final object reference to list
  pObjectReference->columnReferences = columnReferences;
  objectReferences.push_back(pObjectReference);

  return STATUS_GOOD;
}

// ****************************************************************************
// method:  getConstraintName
//
// This method returns the the name of the first referenced constraint involved 
// with a foreign key constraint between two tables (the referenced table and 
// the referencing table)
//
// This method is only called to obtain the constraint name to include in an
// error message.
//
//  assuming the example from getReferencingTablesForConstraints
//    user1:
//      create table dept (dept_no int not null primary key, ... );
//      grant references on dept to user2;
//    
//    user2:
//      create table empl (empl_no int not null primary key, dept_no int, ... );
//      alter table empl add constraint empl_dept foreign key(dept_no) references dept;
//
// This method returns the constraint named empl_dept
//
// The following query is called to get the constraint name:
//
// select [first 1] object_name as referenced_constraint_name 
//   from objects o, table_constraints t, keys k
//   where o.object_uid = t.constraint_uid 
//     and t.table_uid = <referencing table UID>
//     and t.constraint_uid in 
//        (select ref_constraint_uid from ref_constraints
//         where unique_constraint_uid in 
//            (select constraint_uid from table_constraints
//             where t.table_uid = <referenced table uid> 
//               and constraint_uid = k.object_uid
//               and k.column_number = <column number>))
//
// input:   referencedTableUID - the referenced table
//          in the example above, the is the DEPT table UID
//
//          referencingTableUID - the referencing table
//          in the example above, this is the EMPL table UID
//
// returns:  true, found constraint name
//           false, unable to get constraint name
//
//           This method does not clear out the ComDiags area in case of
//           a failure.
//
// ****************************************************************************
bool PrivMgrMDAdmin::getConstraintName(
  const int64_t referencedTableUID,
  const int64_t referencingTableUID,
  const int32_t columnNumber,
  std::string &constraintName)
{
  std::string objectsMDTable = trafMetadataLocation_ + ".OBJECTS o";
  std::string tblConstraintsMDTable = 
    trafMetadataLocation_ + ".TABLE_CONSTRAINTS t";
  std::string refConstraintsMDTable = 
    trafMetadataLocation_ + ".REF_CONSTRAINTS u";
  std::string keysMDTable = 
    trafMetadataLocation_ + ".KEYS k";

  // select object_name based on passed in object UID
  std::string quote("\"");
  std::string selectStmt = "select [first 1] ";
  selectStmt += "trim(o.catalog_name) || '.\"' || ";
  selectStmt += "trim (o.schema_name) || '\".\"' ||";
  selectStmt += "trim (o.object_name)|| '\"' from ";
  selectStmt += objectsMDTable + ", " + tblConstraintsMDTable;
  selectStmt += " where o.object_uid = t.constraint_uid";
  selectStmt += " and t.table_uid = " + UIDToString(referencingTableUID);
  selectStmt += " and t.constraint_uid in (select ref_constraint_uid from ";
  selectStmt += refConstraintsMDTable + ", " + keysMDTable;
  selectStmt += " where unique_constraint_uid in ";
  selectStmt += " (select constraint_uid from " + tblConstraintsMDTable;
  selectStmt += " where t.table_uid = " + UIDToString(referencedTableUID);
  selectStmt += ") and k.column_number = " + UIDToString(columnNumber);
  selectStmt += " and unique_constraint_uid = k.object_uid) ";

  ExeCliInterface cliInterface(STMTHEAP, 0, NULL, 
  CmpCommon::context()->sqlSession()->getParentQid());
  Queue * objectsQueue = NULL;

// set pointer in diags area
int32_t diagsMark = pDiags_->mark();

  int32_t cliRC =  cliInterface.fetchAllRows(objectsQueue, (char *)selectStmt.c_str(), 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
  {
    cliInterface.retrieveSQLDiagnostics(pDiags_);
    return false;
  }

  // did not find a row, or found too many rows
  if (cliRC == 100)
  {
    pDiags_->rewind(diagsMark);
    return false;
  }

  // Return the constraint name.  
  char * ptr = NULL;
  Int32 len = 0;
  char value[MAX_SQL_IDENTIFIER_NAME_LEN + 1];
  objectsQueue->position();
  OutputInfo * pCliRow = (OutputInfo*)objectsQueue->getNext();

  // column 0: object name
  pCliRow->get(0,ptr,len);
  strncpy(value, ptr, len);
  value[len] = 0;
  constraintName += std::string(value);
  
  return true;
}

// ----------------------------------------------------------------------------
// method: compareTableDefs
//
// this method looks at column attributes from two Seabase tables by
//   performing a query that compares column attributes of the two tables
//
// params:
//   tableNameOne 
//   tableNameTwo - current definition of the table
//   cliInterface - cli details 
//   objectsLocation - location of the system metadata table OBJECTS
//   colsLocation - loction of the system metadata table COLUMNS
//
// returns:
//   true  - table structures match
//   false - table structures don't match or unexpected error
//
// A cli error is put into the diags area if there is an error
// ----------------------------------------------------------------------------
static bool compareTableDefs (
  const char *tableNameOne,
  const char *tableNameTwo,
  const std::string &objectsLocation,
  const std::string &colsLocation,
  ExeCliInterface &cliInterface,
  ComDiagsArea *pDiags)
{
  // Perform a SQL command that compares column differences between two tables:
  //  select column_name, column_number, column_size, sql_data_type
  //  from 
  //   (select column_name, column_number, sql_data_type, column_size)
  //    from <colsLocation>
  //    where object_uid in 
  //     (select object_uid from <objectsLocation>
  //      where object_name in ('<tableNameOne> <tableNameTwo>') 
  //            and schema_name = '_PRIVMGR_MD_')
  //   group by column_name,column_number, 
  //            fs_data_type,sql_data_type, column_size
  //   having count(1)=1)
  // order by column_name


  // This code only checks a subset of column attributes.  These attributes are
  // sufficient for now.  If future changes require other attributes, then this
  // query should change.

  // Calculate size of generated query - 2 catalogs, 3 schemas, 4 tables, plus 300 for text
  char query[MAX_SQL_IDENTIFIER_NAME_LEN*2 + 
             MAX_SQL_IDENTIFIER_NAME_LEN*3 + 
             MAX_SQL_IDENTIFIER_NAME_LEN*4 + 300];

  Queue * tableColDiffs = NULL;
  str_sprintf(query, 
    "select column_name, column_number, fs_data_type, column_size "
    "from %s where object_uid in "
    "(select object_uid from %s where object_name in ('%s', '%s') "
    "and schema_name = '%s') " 
    "group by column_name, column_number, fs_data_type, column_size "
    "having count(1) = 1 order by column_name",
    colsLocation.c_str(), objectsLocation.c_str(),
    tableNameOne, tableNameTwo, SEABASE_PRIVMGR_SCHEMA); 

  Int32 cliRC = cliInterface.fetchAllRows(tableColDiffs, query, 0, FALSE, FALSE, TRUE);

  // Check the results of the select statement
  if (cliRC < 0)
  {
    cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
    return false;
  }

  // If no rows are returned, then table structures match
  return (tableColDiffs->numEntries() == 0);

  // could return the list of rows that don't match
#if 0
  std::vector<ColumnRow> colInfoArray;
  tableColDiffs->position();
  for (Lng32 idx = 0; idx < tableColDiffs->numEntries(); idx++)
  {
      OutputInfo * oi = (OutputInfo*)tableColDiffs->getNext();
      
      ColumnRow colInfo;
      char * data = NULL;
      Lng32 len = 0;

      // get the column name
      oi->get(0, data, len);
      colInfo.colName = new(STMTHEAP) char[len + 1];
      strcpy((char*)colInfo.colName, data);

      colInfo.colNumber = *(Lng32*)oi->get(1);
      colInfo.datatype = *(Lng32*)oi->get(2);
      colInfo.length = *(Lng32*)oi->get(3);

      // may want to include these in comparison
      colInfo.columnClass = COM_UNKNOWN_CLASS;
      colInfo.precision = 0;
      colInfo.scale = 0;
      colInfo.dtStart = 0;
      colInfo.dtEnd = 0;
      colInfo.upshifted = 0;
      colInfo.hbaseColFlags = 0;
      colInfo.nullable = 1;
      colInfo.charset = (SQLCHARSET_CODE)CharInfo::UnknownCharSet;
      colInfo.defaultClass = COM_NO_DEFAULT;
      colInfo.colHeading = NULL;
      colInfo.hbaseColFam = NULL;
      colInfo.hbaseColQual = NULL;
      strcpy(colInfo.paramDirection, " ");
      colInfo.isOptional = 0;
      colInfo.colFlags = 0;
      colInfoArray.push_back(colInfo);
   }
#endif
}

// ----------------------------------------------------------------------------
// Method:  createTable
//
// This method creates a table
//
// Params:
//   tableName - fully qualified name of table to create 
//   tableDDL - a TableDDLString describing the table
//   cliInterface - a reference to the CLI interface
//   pDiags - pointer to the diags area
//
// Returns:
//   The cli code returned from the create statement
//
// A cli error is put into the diags area if there is an error
// ----------------------------------------------------------------------------   
static int32_t createTable (
  const char *tableName,
  const TableDDLString *tableDDL,
  ExeCliInterface &cliInterface,
  ComDiagsArea *pDiags)
{
  std::string createStmt("CREATE TABLE ");
  createStmt += tableName;
  createStmt += tableDDL->str;

  Int32 cliRC = cliInterface.executeImmediate(createStmt.c_str());
  if (cliRC != 0)
    cliInterface.retrieveSQLDiagnostics(pDiags);

  return cliRC;
}

// ----------------------------------------------------------------------------
// Method:  dropTable
//
// This method drops a table
//
// Params:
//   tableName - fully qualified name of table to drop
//   cliInterface - a reference to the CLI interface
//   pDiags - pointer to the diags area
//
// Returns:
//   The cli code returned from the drop statement
//
// A cli error is put into the diags area if there is an error
// ----------------------------------------------------------------------------   
static int32_t dropTable (
  const char *tableName,
  ExeCliInterface &cliInterface,
  ComDiagsArea *pDiags)
{
  std::string tableDDL("DROP TABLE IF EXISTS ");
  tableDDL += tableName;

  Int32 cliRC = cliInterface.executeImmediate(tableDDL.c_str());
  if (cliRC < 0)
    cliInterface.retrieveSQLDiagnostics(pDiags);

  return cliRC;
}

// ----------------------------------------------------------------------------
// Method:  cleanupTable
//
// This method removes a table through the brute force "cleanup" method.
//
// Params:
//   tableName - fully qualified name of table to drop
//   cliInterface - a reference to the CLI interface
//   pDiags - pointer to the diags area
//
// This method is called when metadata is corrupt so errors are expected.
// Error messages are suppressed.
// ----------------------------------------------------------------------------
static void cleanupTable (
  const char *tableName,
  ExeCliInterface &cliInterface,
  ComDiagsArea *pDiags)
{
  std::string tableDDL("CLEANUP TABLE ");
  tableDDL += tableName;
  int32_t diagsMark = pDiags->mark();
  Int32 cliRC = cliInterface.executeImmediate(tableDDL.c_str());
  if (cliRC < 0)
    pDiags->rewind(diagsMark);
}

// ----------------------------------------------------------------------------
// Method:  renameTable
//
// This method renames a table
//
// Params:
//   originalObjectName - fully qualified name of object to rename
//   newObjectName - fully qualified new name
//   cliInterface - a reference to the CLI interface
//   pDiags - pointer to the diags area
//
// Returns:
//   The cli code returned from the rename statement
//
// A cli error is put into the diags area if there is an error
// ----------------------------------------------------------------------------   
static int32_t renameTable (
  const char *originalObjectName,
  const char *newObjectName,
  ExeCliInterface &cliInterface,
  ComDiagsArea *pDiags)
{
  std::string tableDDL("ALTER TABLE  ");
  tableDDL += newObjectName;
  tableDDL += " RENAME TO ";
  tableDDL += originalObjectName;

  Int32 cliRC = cliInterface.executeImmediate(tableDDL.c_str());
  if (cliRC < 0)
    cliInterface.retrieveSQLDiagnostics(pDiags);

  return cliRC;
}

// ****************************************************************************
// method:  updatePrivMgrMetadata
//
//  this method updates PrivMgr metadata tables as part of authorization 
//  initialization.
//  
// **************************************************************************** 
PrivStatus PrivMgrMDAdmin::updatePrivMgrMetadata(
   const std::string &objectsLocation,
   const std::string &authsLocation,
   const bool shouldPopulateObjectPrivs)
   
{
   std::string traceMsg;
   PrivStatus privStatus = STATUS_GOOD;

   if (shouldPopulateObjectPrivs)
   {
      PrivMgrPrivileges objectPrivs (metadataLocation_,pDiags_);
      privStatus = objectPrivs.populateObjectPriv(objectsLocation,authsLocation);
      if (privStatus != STATUS_GOOD && privStatus != STATUS_NOTFOUND)
         return STATUS_ERROR;
   }
   
   // Create any roles.  If this is an upgrade operation, some roles may
   // already exist, just create any new roles. If this is an initialize
   // operation, than all system roles are created.
   CmpSeabaseDDLrole role;
   std::vector<std::string> rolesCreated;
   int32_t numberRoles = sizeof(systemRoles)/sizeof(SystemAuthsStruct);
   for (int32_t i = 0; i < numberRoles; i++)
   {
     const SystemAuthsStruct &roleDefinition = systemRoles[i];

     // special Auth includes roles that are not registered in the metadata
     if (roleDefinition.isSpecialAuth)
       continue;

     // returns true is role was created, false if it already existed
     if (role.createStandardRole(roleDefinition.authName, roleDefinition.authID))
       rolesCreated.push_back(roleDefinition.authName);
   }

   // Report the number roles created
   traceMsg = "created roles ";
   char buf[MAX_AUTHNAME_LEN + 5];
   char sep = ' ';
   for (size_t i = 0; i < rolesCreated.size(); i++)
   {
      sprintf(buf, "%c'%s' ", sep, rolesCreated[i].c_str());
      traceMsg.append(buf);
      sep = ',';
   }
   log(__FILE__, traceMsg, -1);
   
   if (rolesCreated.size() > 0)
   {
      PrivMgrRoles role(" ",metadataLocation_,pDiags_);
                        
      privStatus = role.populateCreatorGrants(authsLocation, rolesCreated);
      if (privStatus != STATUS_GOOD)
         return STATUS_ERROR;
   }
 
   // If someone initializes authorization, creates some roles, then drops 
   // authorization, these roles exist in th system metadata (e.g. AUTHS table)
   // but all usages are lost, including the initial creator grants.
   // See if there are any roles that exist in AUTHS but do not have creator 
   // grants - probably should add creator grants.
   // TBD
    
   privStatus = initializeComponentPrivileges();
  
   if (privStatus != STATUS_GOOD)
      return STATUS_ERROR;
      
   // When new components and component operations are added
   // add an upgrade procedure
   
   return STATUS_GOOD;
   
}


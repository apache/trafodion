//*****************************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
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
//    PrivMgrMDAdmin methods
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

// First, let's start with a clean slate.  Drop all components as well as 
// their respective operations and and any privileges granted.  This should be  
// a NOP unless PrivMgr metadata was damaged and reintialization is occurring.

PrivMgrComponents components(metadataLocation_,pDiags_);

   components.dropAll();
   
// Next, register the component.

PrivStatus privStatus = STATUS_GOOD;

   privStatus = components.registerComponentInternal(SQL_OPERATION_NAME,
                                                     SQL_OPERATIONS_COMPONENT_UID,
                                                     true,"Component for SQL operations");
                                             
   if (privStatus != STATUS_GOOD)
      return STATUS_ERROR;  
      
// Component is registered, now create all the operations associated with
// the component.  A grant from the system to the grantee (DB__ROOT) will
// be added for each operation.                                         
                                
PrivMgrComponentOperations componentOperations(metadataLocation_,pDiags_);
std::vector<std::string> operationCodes;

int32_t DB__ROOTID = ComUser::getRootUserID();
std::string DB__ROOTName(ComUser::getRootUserName());

   for (SQLOperation operation = SQLOperation::FIRST_OPERATION;
        static_cast<int>(operation) <= static_cast<int>(SQLOperation::LAST_OPERATION); 
        operation = static_cast<SQLOperation>(static_cast<int>(operation) + 1))
   {
      const char *codePtr = PrivMgr::getSQLOperationCode(operation);
      privStatus = componentOperations.createOperationInternal(SQL_OPERATIONS_COMPONENT_UID,
                                                               PrivMgr::getSQLOperationName(operation),
                                                               codePtr,true,
                                                               PrivMgr::getSQLOperationDescription(operation),
                                                               DB__ROOTID,DB__ROOTName,-1);
                                                       
      if (privStatus == STATUS_GOOD)
         operationCodes.push_back(codePtr); 
      //TODO: report warning if not all operations added
   }

// In the unlikely event no operations were created, we are done.   
   if (operationCodes.size() == 0)
      return STATUS_GOOD;
      
PrivMgrComponentPrivileges componentPrivileges(metadataLocation_,pDiags_);
   
// Grant all SQL_OPERATIONS to DB__ROOTROLE WITH GRANT OPTION                                      
   privStatus = componentPrivileges.grantPrivilegeInternal(SQL_OPERATIONS_COMPONENT_UID,
                                                           operationCodes,
                                                           ComUser::getRootUserID(),
                                                           ComUser::getRootUserName(),
                                                           DB_ROOTROLE_ID,
                                                           DB_ROOTROLE_NAME,-1);
                                                           
   if (privStatus != STATUS_GOOD)
      return privStatus;
                                      
// Grant SQL_OPERATIONS CREATE_SCHEMA and SHOW to PUBLIC 
std::vector<std::string> CSOperationCodes;

   CSOperationCodes.push_back(PrivMgr::getSQLOperationCode(SQLOperation::CREATE_SCHEMA));
   CSOperationCodes.push_back(PrivMgr::getSQLOperationCode(SQLOperation::SHOW));
                                     
   privStatus = componentPrivileges.grantPrivilegeInternal(SQL_OPERATIONS_COMPONENT_UID,
                                                           CSOperationCodes,
                                                           ComUser::getRootUserID(),
                                                           ComUser::getRootUserName(),
                                                           PUBLIC_AUTH_ID,
                                                           PUBLIC_AUTH_NAME,0);
                                      
   if (privStatus != STATUS_GOOD)
      return privStatus;
      
// Verify counts for tables.

// Expected number of privileges granted is 2 for each operation (one each
// for DB__ROOT and DB__ROOTROLE) plus the two grants to PUBLIC.

int64_t expectedPrivCount = static_cast<int64_t>(SQLOperation::NUMBER_OF_OPERATIONS) * 2 + 2;

   if (components.getCount() != 1 ||
       componentOperations.getCount() != static_cast<int64_t>(SQLOperation::NUMBER_OF_OPERATIONS) ||
       componentPrivileges.getCount() != expectedPrivCount)
      return STATUS_ERROR;
     
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
PrivStatus PrivMgrMDAdmin::initializeMetadata (const std::string &objectsLocation,
                                               const std::string &authsLocation)
{
  std::vector<std::string> tablesCreated;
  PrivStatus retcode = STATUS_GOOD;
  try
  {
    // Authorization check
    if (!isAuthorized())
    {
       *pDiags_ << DgSqlCode (-CAT_NOT_AUTHORIZED);
       return STATUS_ERROR;
     }

    // See what does and does not exist
    PrivMDStatus initStatus = authorizationEnabled();

    // If unable to access metadata, return STATUS_ERROR 
    //   (pDiags_ contains error details)
    if (initStatus == PRIV_INITIALIZE_UNKNOWN)
      return STATUS_ERROR;

    // If metadata tables already initialize, just return STATUS_GOOD
    if (initStatus == PRIV_INITIALIZED)
      return STATUS_GOOD;
      
    // Create the privilege manager schema.
    ExeCliInterface cliInterface(STMTHEAP, NULL, NULL, 
  CmpCommon::context()->sqlSession()->getParentQid());
    std::string schemaCommand("CREATE PRIVATE SCHEMA ");
    
    schemaCommand += metadataLocation_;
    Int32 cliRC = cliInterface.executeImmediate(schemaCommand.c_str());
    if (cliRC < 0)
    {
      // If schema already exists, change to warning and continue
      cliInterface.retrieveSQLDiagnostics(pDiags_);
      if (cliRC == -CAT_SCHEMA_ALREADY_EXISTS)
      {
        NegateAllErrors(pDiags_);
        retcode = STATUS_WARNING;
      }
      else
        throw cliRC;
    }
      
    // Create the tables
    //   If table already exists, skip it and continue
    //   TBD: if the table already exists, then see if it needs to be upgraded
    size_t numTables = sizeof(privMgrTables)/sizeof(PrivMgrTableStruct);
    bool populateObjectPrivs = true;
    bool populateRoleGrants = true;
    for (int ndx_tl = 0; ndx_tl < numTables; ndx_tl++)
    {
      const PrivMgrTableStruct &tableDefinition = privMgrTables[ndx_tl];
      std::string tableDDL("CREATE TABLE ");
      tableDDL += deriveTableName(tableDefinition.tableName);
      tableDDL += tableDefinition.tableDDL->str;
      Int32 cliRC = cliInterface.executeImmediate(tableDDL.c_str());
      if (cliRC < 0)
      {
        // If object already exists, change to warning and continue
        cliInterface.retrieveSQLDiagnostics(pDiags_);
        if (cliRC == -CAT_TRAFODION_OBJECT_EXISTS)
        {
          NegateAllErrors(pDiags_);
          retcode = STATUS_WARNING;

          // If the table is OBJECT_PRIVILEGES no need to populate again
          if (tableDefinition.tableName == "OBJECT_PRIVILEGES")
            populateObjectPrivs = false;
          // If the table is ROLE_USAGE no need to populate again
          if (tableDefinition.tableName == "ROLE_USAGE")
            populateRoleGrants = false;
        }
        else
          throw cliRC;
      }
      // Add to list of tables created.
      else
        tablesCreated.push_back(tableDefinition.tableName);
    }
    
PrivStatus privStatus = STATUS_GOOD;

   privStatus = updatePrivMgrMetadata(objectsLocation,authsLocation,
                                      populateObjectPrivs,populateRoleGrants);


    // if error occurs, drop tables already created
    if (privStatus == STATUS_ERROR)
    {
      PrivStatus dropRetcode = dropMetadata(tablesCreated);
      *pDiags_ << DgSqlCode(-CAT_INIT_AUTHORIZATION_FAILED);
    }  
  }

  catch (...)
  {
    // drop any tables created before returning
    PrivStatus dropRetcode = dropMetadata(tablesCreated);
    *pDiags_ << DgSqlCode(-CAT_INIT_AUTHORIZATION_FAILED);
    return STATUS_ERROR;
  }
//TODO: should notify QI
  return retcode;
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
PrivStatus PrivMgrMDAdmin::dropMetadata (const std::vector<std::string> &objectsToDrop)
{
  PrivStatus retcode = STATUS_GOOD;
  try
  {
    // Authorization check
    if (!isAuthorized())
    {
       *pDiags_ << DgSqlCode (-CAT_NOT_AUTHORIZED);
       return STATUS_ERROR;
     }

    // See what does and does not exist
    PrivMDStatus initStatus = authorizationEnabled();

    // If unable to access metadata, return STATUS_ERROR 
    //   (pDiags contains error details)
    if (initStatus == PRIV_INITIALIZE_UNKNOWN)
      return STATUS_ERROR;

    // If metadata tables don't exist, just return STATUS_GOOD
    if (initStatus == PRIV_UNINITIALIZED)
      return STATUS_GOOD;

    // Call Trafodion to drop the requested tables
    // If one of the tables fail to drop, save retcode and continue
    ExeCliInterface cliInterface(STMTHEAP, NULL, NULL, 
  CmpCommon::context()->sqlSession()->getParentQid());
    for (int32_t i = 0; i < objectsToDrop.size();++i)
    {
      std::string objectName = objectsToDrop[i];
      std::string tableDDL("DROP TABLE IF EXISTS ");
      tableDDL += deriveTableName(objectName.c_str());
      tableDDL += ";";

      Int32 cliRC = cliInterface.executeImmediate(tableDDL.c_str());
      if (cliRC < 0)
      {
        cliInterface.retrieveSQLDiagnostics(pDiags_);
        retcode = STATUS_ERROR;
      }
    }
    
    std::string schemaDDL("DROP SCHEMA ");
    schemaDDL += metadataLocation_;
    Int32 cliRC = cliInterface.executeImmediate(schemaDDL.c_str());
    if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(pDiags_);
      retcode = STATUS_ERROR;
    }
    CmpSeabaseDDLrole role;
    
    role.dropStandardRole(DB_ROOTROLE_NAME);
    
  }

  catch (...)
  {
    return STATUS_ERROR;
  }
//TODO: should notify QI
  return retcode;
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
// method:  getViewsThatReferenceObject
//
//  this method gets the list of views associated with the passed in 
//  objectUID that are owned by the objectOwner.
// **************************************************************************** 
PrivStatus PrivMgrMDAdmin::getViewsThatReferenceObject (
  const ObjectUsage &objectUsage,
  std::vector<ViewUsage> &viewUsages )
{
  std::string objectMDTable = trafMetadataLocation_ + ".OBJECTS o";
  std::string viewUsageMDTable = trafMetadataLocation_ + ".VIEWS_USAGE u";
  std::string viewsMDTable = trafMetadataLocation_ + ".VIEWS v";
  std::string roleUsageMDTable = metadataLocation_ + ".ROLE_USAGE";

  // Select all the views that are referenced by the table or view owned by the objectOwner
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
  selectStmt += UIDToString(objectUsage.objectOwner);
  selectStmt += "  or o.object_owner in (select role_id from ";
  selectStmt += roleUsageMDTable;
  selectStmt += " where grantee_id = ";
  selectStmt += UIDToString(objectUsage.objectOwner);

  selectStmt += ")) order by o.create_time ";

  ExeCliInterface cliInterface(STMTHEAP, NULL, NULL, 
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
    viewUsage.isInsertable = (ptr == 0) ? false : true;

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

  ExeCliInterface cliInterface(STMTHEAP, NULL, NULL, 
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
  selectStmt += UIDToString(objectUsage.objectOwner);
  selectStmt += "  or o.object_owner in (select role_id from ";
  selectStmt += roleUsageMDTable;
  selectStmt += " where grantee_id = ";
  selectStmt += UIDToString(objectUsage.objectOwner);
  selectStmt += ")) order by o.create_time ";

  ExeCliInterface cliInterface(STMTHEAP, NULL, NULL, 
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
// This method returns the list of underlying tables that are associated with
// RI constraints referencing an ObjectUsage.
//
// An RI constraint is a relationship between a set of columns on one table 
// with a set of columns on another table.  Each set of columns must be
// defined as an unique constraint (which include primary key constraints.  
// Relationships are stored through the constraints not their underlying tables.
//
//  for example:
//    user1:
//      create table dept (dept_no int not null primary key, ... );
//      grant references on dept to user2;
//    
//    user2:
//      create table empl (empl_no int not null primary key, dept_no int, ... );
//      alter table empl add constraint empl_dept foreign key references dept;
//
//  empl_dept is the name of the RI constraint
//     The empl table references dept 
//     The dept table is being referenced by empl
//
//  The following query is called to get the list of tables:  
//    <Gets underlying table for the foreign key's constraint>
//    select distinct o.object_uid, o.object_owner, o.create_time
//    from table_constraints t, objects o
//      where o.object_uid = t.table_uid;
//      and t.constraint_uid in
//
//        <Gets foreign key's constraints referencing table>
//        (select foreign_constraint_uid
//        from table_constraints t, unique_ref_constr_usage u
//           where t.table_uid = objectUsage.objectUID
//           and t.constraint_uid = u.unique_constraint_uid)
//        order by o.create_time
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
  std::string uniqueRefConstraintsMDTable = 
    trafMetadataLocation_ + ".UNIQUE_REF_CONSTR_USAGE u";

  // Select all the constraints that are referenced by the table
  // create_time is included to order by the oldest to newest
  std::string selectStmt = "select distinct o.object_uid, o.object_owner, o.object_type, o.create_time, ";
  selectStmt += "trim(o.catalog_name) || '.\"' || ";
  selectStmt += "trim (o.schema_name) || '\".\"' ||";
  selectStmt += "trim (o.object_name)|| '\"' from ";
  selectStmt += tblConstraintsMDTable + std::string(", ") + objectsMDTable;
  selectStmt += " where o.object_uid = t.table_uid and t.constraint_uid in ";
  selectStmt += "(select foreign_constraint_uid from ";
  selectStmt += tblConstraintsMDTable + std::string(", ") + uniqueRefConstraintsMDTable;
  selectStmt += " where t.table_uid = " + UIDToString(objectUsage.objectUID);
  selectStmt += " and t.constraint_uid = u.unique_constraint_uid)";
  selectStmt += " order by o.create_time ";

  ExeCliInterface cliInterface(STMTHEAP, NULL, NULL, 
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

  // Set up an objectReference for any objects found, the caller manages
  // space for this list
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
//      alter table empl add constraint empl_dept foreign key references dept;
//
// This method returns the constraint named empl_dept
//
// The following query is called to get the constraint name:
//
// select [first 1] object_name as referenced_constraint_name 
//   from objects o, table_constraints t
//   where o.object_uid = t.constraint_uid 
//     and t.table_uid = <referencing table UID>
//     and t.constraint_uid in 
//        (select ref_constraint_uid from ref_constraints
//         where unique_constraint_uid in 
//            (select constraint_uid from table_constraints
//             where t.table_uid = <referenced table uid>)) 
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
  std::string &constraintName)
{
  std::string objectsMDTable = trafMetadataLocation_ + ".OBJECTS o";
  std::string tblConstraintsMDTable = 
    trafMetadataLocation_ + ".TABLE_CONSTRAINTS t";
  std::string refConstraintsMDTable = 
    trafMetadataLocation_ + ".REF_CONSTRAINTS u";

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
  selectStmt += refConstraintsMDTable;
  selectStmt += " where unique_constraint_uid in ";
  selectStmt += " (select constraint_uid from " + tblConstraintsMDTable;
  selectStmt += " where t.table_uid = " + UIDToString(referencedTableUID);
  selectStmt += "))";

  ExeCliInterface cliInterface(STMTHEAP, NULL, NULL, 
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
   const bool shouldPopulateObjectPrivs,
   const bool shouldPopulateRoleGrants)
   
{
   
PrivStatus privStatus = STATUS_GOOD;

   if (shouldPopulateObjectPrivs)
   {
      PrivMgrPrivileges objectPrivs (metadataLocation_,pDiags_);
      privStatus = objectPrivs.populateObjectPriv(objectsLocation,authsLocation);
      if (privStatus != STATUS_GOOD && privStatus != STATUS_NOTFOUND)
         return STATUS_ERROR;
   }
   
    
CmpSeabaseDDLrole role;
    
   role.createStandardRole(DB_ROOTROLE_NAME,DB_ROOTROLE_ID);
   
   if (shouldPopulateRoleGrants)
   {
      PrivMgrRoles role(" ",metadataLocation_,pDiags_);
                        
      privStatus = role.populateCreatorGrants(authsLocation);
      if (privStatus != STATUS_GOOD)
         return STATUS_ERROR;
   }
    
   privStatus = initializeComponentPrivileges();
   
   if (privStatus != STATUS_GOOD)
      return STATUS_ERROR;
      
   return STATUS_GOOD;
   
}


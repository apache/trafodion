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
  
// Needed for parser flag manipulation
#define   SQLPARSERGLOBALS_FLAGS  
#include "SqlParserGlobalsCmn.h"
  
#include "PrivMgr.h"

// c++ includes
#include <string>
#include <algorithm>

// PrivMgr includes
#include "PrivMgrComponents.h"
#include "PrivMgrComponentOperations.h"
#include "PrivMgrComponentPrivileges.h"
#include "PrivMgrPrivileges.h"
#include "PrivMgrRoles.h"

// Trafodion includes
#include "ComDistribution.h"
#include "sqlcli.h"
#include "ExExeUtilCli.h"
#include "ComDiags.h"
#include "ComQueue.h"
#include "CmpCommon.h"
#include "CmpContext.h"
#include "CmpDDLCatErrorCodes.h"
#include "logmxevent_traf.h"
#include "ComUser.h"
#include "NAUserId.h"


// ==========================================================================
// Contains non inline methods in the following classes
//   PrivMgr
// ==========================================================================

// Specified in expected order of likelihood. See sql/common/ComSmallDefs 
// for actual values.
static const literalAndEnumStruct objectTypeConversionTable [] =
{
  {COM_BASE_TABLE_OBJECT, COM_BASE_TABLE_OBJECT_LIT},
  {COM_INDEX_OBJECT, COM_INDEX_OBJECT_LIT},
  {COM_VIEW_OBJECT, COM_VIEW_OBJECT_LIT},
  {COM_STORED_PROCEDURE_OBJECT, COM_STORED_PROCEDURE_OBJECT_LIT},
  {COM_USER_DEFINED_ROUTINE_OBJECT, COM_USER_DEFINED_ROUTINE_OBJECT_LIT},
  {COM_UNIQUE_CONSTRAINT_OBJECT, COM_UNIQUE_CONSTRAINT_OBJECT_LIT},
  {COM_NOT_NULL_CONSTRAINT_OBJECT, COM_NOT_NULL_CONSTRAINT_OBJECT_LIT},
  {COM_CHECK_CONSTRAINT_OBJECT, COM_CHECK_CONSTRAINT_OBJECT_LIT},
  {COM_PRIMARY_KEY_CONSTRAINT_OBJECT, COM_PRIMARY_KEY_CONSTRAINT_OBJECT_LIT},
  {COM_REFERENTIAL_CONSTRAINT_OBJECT, COM_REFERENTIAL_CONSTRAINT_OBJECT_LIT},
  {COM_TRIGGER_OBJECT, COM_TRIGGER_OBJECT_LIT},
  {COM_LOCK_OBJECT, COM_LOCK_OBJECT_LIT},
  {COM_LOB_TABLE_OBJECT, COM_LOB_TABLE_OBJECT_LIT},
  {COM_TRIGGER_TABLE_OBJECT, COM_TRIGGER_TABLE_OBJECT_LIT},
  {COM_SYNONYM_OBJECT, COM_SYNONYM_OBJECT_LIT},
  {COM_PRIVATE_SCHEMA_OBJECT, COM_PRIVATE_SCHEMA_OBJECT_LIT},
  {COM_SHARED_SCHEMA_OBJECT, COM_SHARED_SCHEMA_OBJECT_LIT},
  {COM_LIBRARY_OBJECT, COM_LIBRARY_OBJECT_LIT},
  {COM_EXCEPTION_TABLE_OBJECT, COM_EXCEPTION_TABLE_OBJECT_LIT},
  {COM_SEQUENCE_GENERATOR_OBJECT, COM_SEQUENCE_GENERATOR_OBJECT_LIT},
  {COM_UNKNOWN_OBJECT, COM_UNKNOWN_OBJECT_LIT}
};

// -----------------------------------------------------------------------
// Default Constructor
// -----------------------------------------------------------------------
PrivMgr::PrivMgr() 
: trafMetadataLocation_ ("TRAFODION.\"_MD_\""),
  metadataLocation_ ("TRAFODION.\"_PRIVMGR_MD_\""),
  pDiags_(CmpCommon::diags()),
  authorizationEnabled_(PRIV_INITIALIZED)
{}

// -----------------------------------------------------------------------
// Construct a PrivMgr object specifying a different metadata location
// -----------------------------------------------------------------------
  

PrivMgr::PrivMgr( 
   const std::string & metadataLocation,
   ComDiagsArea * pDiags,
   PrivMDStatus authorizationEnabled)
: trafMetadataLocation_ ("TRAFODION.\"_MD_\""),
  metadataLocation_ (metadataLocation),
  pDiags_(pDiags),
  authorizationEnabled_(authorizationEnabled)
  
{

  if (pDiags == NULL)
     pDiags = CmpCommon::diags();

  setFlags();
}

PrivMgr::PrivMgr( 
   const std::string & trafMetadataLocation,
   const std::string & metadataLocation,
   ComDiagsArea * pDiags,
   PrivMDStatus authorizationEnabled)
: trafMetadataLocation_ (trafMetadataLocation),
  metadataLocation_ (metadataLocation),
  pDiags_(pDiags),
  authorizationEnabled_(authorizationEnabled)
  
{

  if (pDiags == NULL)
     pDiags = CmpCommon::diags();

  setFlags();
}


// -----------------------------------------------------------------------
// Copy constructor
// -----------------------------------------------------------------------
PrivMgr::PrivMgr(const PrivMgr &other)
{
  trafMetadataLocation_ = other.trafMetadataLocation_;
  metadataLocation_ = other.metadataLocation_;
  pDiags_ = other.pDiags_;
}


// -----------------------------------------------------------------------
// Destructor.
// -----------------------------------------------------------------------

PrivMgr::~PrivMgr() 
{
  resetFlags();
}

// *****************************************************************************
// * Method: getGranteeIDsForRoleIDs                              
// *                                                       
// *    Returns the grantees assigned to the passed in roles
// *    role list
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <roleIDs>    list of roles to check
// *  <granteeIDs> passed back the list (potentially empty) of users granted to 
// *               the roleIDs
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: Role list returned
// *           *: Unable to fetch granted roles, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgr::getGranteeIDsForRoleIDs(
  const std::vector<int32_t>  & roleIDs,
  std::vector<int32_t> & granteeIDs,
  bool includeSysGrantor)
{
  std::vector<int32_t> granteeIDsForRoleIDs;
  PrivMgrRoles roles(" ",metadataLocation_,pDiags_);
  if (roles.fetchGranteesForRoles(roleIDs, granteeIDsForRoleIDs, includeSysGrantor) == STATUS_ERROR)
    return STATUS_ERROR;
  for (size_t i = 0; i < granteeIDsForRoleIDs.size(); i++)
  {
     int32_t authID = granteeIDsForRoleIDs[i];
     if (std::find(granteeIDs.begin(), granteeIDs.end(), authID) == granteeIDs.end())
       granteeIDs.insert( std::upper_bound( granteeIDs.begin(), granteeIDs.end(), authID ), authID);
  }
  return STATUS_GOOD;
}

// ----------------------------------------------------------------------------
// method:  authorizationEnabled
//
// Input:  pointer to the error structure
//
// Returns:
//    PRIV_INITIALIZED means all metadata tables exist
//    PRIV_UNINITIALIZED means no metadata tables exist
//    PRIV_PARTIALLY_INITIALIZED means only part of the metadata tables exist
//    PRIV_INITIALIZE_UNKNOWN means unable to retrieve metadata table info
//
// A cli error is put into the diags area if there is an error
// ----------------------------------------------------------------------------
PrivMgr::PrivMDStatus PrivMgr::authorizationEnabled(
  std::set<std::string> &existingObjectList)
{
// Will require QI to reset on INITIALIZE AUTHORIZATION [,DROP]
  // get the list of tables from the schema
  // if the catalog name ever allows an embedded '.', this code will need 
  // to change.
  std::string metadataLocation = getMetadataLocation();
  size_t period = metadataLocation.find(".");
  std::string catName = metadataLocation.substr(0, period);
  std::string schName = metadataLocation.substr(period+1);
  char buf[1000];
  sprintf(buf, "get tables in schema %s.%s, no header",
              catName.c_str(), schName.c_str());

  ExeCliInterface cliInterface(STMTHEAP, 0, NULL, 
  CmpCommon::context()->sqlSession()->getParentQid());
  Queue * schemaQueue = NULL;

// set pointer in diags area
int32_t diagsMark = pDiags_->mark();

  int32_t cliRC =  cliInterface.fetchAllRows(schemaQueue, buf, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
  {
    cliInterface.retrieveSQLDiagnostics(pDiags_);
    return PRIV_INITIALIZE_UNKNOWN;
  }

  if (cliRC == 100) // did not find the row
  {
    pDiags_->rewind(diagsMark);
    return PRIV_UNINITIALIZED;
  }

  // Not sure how this can happen but code I cloned had the check
  if (schemaQueue->numEntries() == 0)
    return PRIV_UNINITIALIZED;

  // Gather the returned list of tables in existingObjectList
  schemaQueue->position();
  for (int idx = 0; idx < schemaQueue->numEntries(); idx++)
  {
    OutputInfo * row = (OutputInfo*)schemaQueue->getNext();
    std::string theName = row->get(0);
    existingObjectList.insert(theName);
  }

  // Gather the list of expected tables in expectedObjectList
  std::set<string> expectedObjectList;
  size_t numTables = sizeof(privMgrTables)/sizeof(PrivMgrTableStruct);
  for (int ndx_tl = 0; ndx_tl < numTables; ndx_tl++)
  {
    const PrivMgrTableStruct &tableDefinition = privMgrTables[ndx_tl];
    expectedObjectList.insert(tableDefinition.tableName);
  }

  // Compare the existing with the expected
  std::set<string> diffsObjectList;
  std::set_difference (expectedObjectList.begin(), expectedObjectList.end(),
                       existingObjectList.begin(), existingObjectList.end(),
                       std::inserter(diffsObjectList, diffsObjectList.end()));

  // If the number of existing tables match the expected, diffsObjectList 
  // is empty -> return initialized
  if (diffsObjectList.empty())
    return PRIV_INITIALIZED;
 
  // If the number of existing tables does not match the expected, 
  // initialization is required -> return not initialized
  if (existingObjectList.size() == diffsObjectList.size())
    return PRIV_UNINITIALIZED;
 
  // Otherwise, mismatch is found, return partially initialized
  return PRIV_PARTIALLY_INITIALIZED;
}


// ----------------------------------------------------------------------------
// static method: getAuthNameFromAuthID
//
// Converts the authorization ID into its corresponding database name
//
//   authID - ID to convert
//   authName - returned name
//
// returns:
//   true - conversion successful
//   false - conversion failed, ComDiags setup with error information
// ----------------------------------------------------------------------------
bool PrivMgr::getAuthNameFromAuthID(
 const int32_t authID, 
 std::string &authName)
{
  switch (authID)
  {
    case SYSTEM_USER:
      authName = SYSTEM_AUTH_NAME;
      break;  
    case PUBLIC_USER:
      authName = PUBLIC_AUTH_NAME;
      break;  
    case SUPER_USER:
      authName = DB__ROOT;
      break;
    case ROOT_ROLE_ID:
      authName = DB__ROOTROLE;
      break;
    case HIVE_ROLE_ID:
      authName = DB__HIVEROLE;
      break;
    case HBASE_ROLE_ID:
      authName = DB__HBASEROLE;
      break;
    default:
    {
      int32_t length = 0;
      char authNameFromMD[MAX_DBUSERNAME_LEN + 1];

      Int16 retcode = ComUser::getAuthNameFromAuthID(authID,authNameFromMD,
                                               MAX_DBUSERNAME_LEN,length);
      if (retcode != 0)
      {
        *CmpCommon::diags() << DgSqlCode(-20235)
                            << DgInt0(retcode)
                            << DgInt1(authID);
        return false;
      }
      authName = authNameFromMD;
    }
  }
  return true;
}

// *****************************************************************************
// * Function:  PrivMgr::getSQLUnusedOpsCount()
// *
// *    Returns the number of unused operations from the hard coded table
// *    in PrivMgrComponentDefs.h for the sql_operations component.
// *
// *****************************************************************************
int32_t PrivMgr::getSQLUnusedOpsCount()
{
  int32_t numUnusedOps = 0;
  size_t numOps = sizeof(sqlOpList)/sizeof(ComponentOpStruct);
  for (int i = 0; i < numOps; i++)
  {
    const ComponentOpStruct &opDefinition = sqlOpList[i];
    if (opDefinition.unusedOp)
      numUnusedOps++;
  }
  return numUnusedOps;
}


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgr::getSQLOperationName                                    *
// *                                                                           *
// *    Returns the operation name associated with the specified operation.    *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <operation>                     SQLOperation                    In       *
// *    is the operation.                                                      *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: const char                                                       *
// *                                                                           *
// *  If the operation exists, the corresponding name is returned, otherwise   *
// *  the string "UNKNOWN" is returned.                                        *
// *                                                                           *
// *****************************************************************************
const char * PrivMgr::getSQLOperationName(SQLOperation operation) 
    
{

   switch (operation)
   {
      case SQLOperation::ALTER: return "ALTER";
      case SQLOperation::ALTER_LIBRARY: return "ALTER_LIBRARY";
      case SQLOperation::ALTER_ROUTINE: return "ALTER_ROUTINE";
      case SQLOperation::ALTER_ROUTINE_ACTION: return "ALTER_ROUTINE_ACTION";
      case SQLOperation::ALTER_SCHEMA: return "ALTER_SCHEMA";
      case SQLOperation::ALTER_SEQUENCE: return "ALTER_SEQUENCE";
      case SQLOperation::ALTER_SYNONYM: return "ALTER_SYNONYM";
      case SQLOperation::ALTER_TABLE: return "ALTER_TABLE";
      case SQLOperation::ALTER_TRIGGER: return "ALTER_TRIGGER";
      case SQLOperation::ALTER_VIEW: return "ALTER_VIEW";
      case SQLOperation::CREATE: return "CREATE";
      case SQLOperation::CREATE_CATALOG: return "CREATE_CATALOG";
      case SQLOperation::CREATE_INDEX: return "CREATE_INDEX";
      case SQLOperation::CREATE_LIBRARY: return "CREATE_LIBRARY";
      case SQLOperation::CREATE_PROCEDURE: return "CREATE_PROCEDURE";
      case SQLOperation::CREATE_ROUTINE: return "CREATE_ROUTINE";
      case SQLOperation::CREATE_ROUTINE_ACTION: return "CREATE_ROUTINE_ACTION";
      case SQLOperation::CREATE_SCHEMA: return "CREATE_SCHEMA";
      case SQLOperation::CREATE_SEQUENCE: return "CREATE_SEQUENCE";
      case SQLOperation::CREATE_SYNONYM: return "CREATE_SYNONYM";
      case SQLOperation::CREATE_TABLE: return "CREATE_TABLE";
      case SQLOperation::CREATE_TRIGGER: return "CREATE_TRIGGER";
      case SQLOperation::CREATE_VIEW: return "CREATE_VIEW";
      case SQLOperation::DML_DELETE: return "DML_DELETE";
      case SQLOperation::DML_EXECUTE: return "DML_EXECUTE";
      case SQLOperation::DML_INSERT: return "DML_INSERT";
      case SQLOperation::DML_REFERENCES: return "DML_REFERENCES";
      case SQLOperation::DML_SELECT: return "DML_SELECT";
      case SQLOperation::DML_UPDATE: return "DML_UPDATE";
      case SQLOperation::DML_USAGE: return "DML_USAGE";
      case SQLOperation::DROP: return "DROP";
      case SQLOperation::DROP_CATALOG: return "DROP_CATALOG";
      case SQLOperation::DROP_INDEX: return "DROP_INDEX";
      case SQLOperation::DROP_LIBRARY: return "DROP_LIBRARY";
      case SQLOperation::DROP_PROCEDURE: return "DROP_PROCEDURE";
      case SQLOperation::DROP_ROUTINE: return "DROP_ROUTINE";
      case SQLOperation::DROP_ROUTINE_ACTION: return "DROP_ROUTINE_ACTION";
      case SQLOperation::DROP_SCHEMA: return "DROP_SCHEMA";
      case SQLOperation::DROP_SEQUENCE: return "DROP_SEQUENCE";
      case SQLOperation::DROP_SYNONYM: return "DROP_SYNONYM";
      case SQLOperation::DROP_TABLE: return "DROP_TABLE";
      case SQLOperation::DROP_TRIGGER: return "DROP_TRIGGER";
      case SQLOperation::DROP_VIEW: return "DROP_VIEW";
      case SQLOperation::MANAGE: return "MANAGE";
      case SQLOperation::MANAGE_COMPONENTS: return "MANAGE_COMPONENTS";
      case SQLOperation::MANAGE_LIBRARY: return "MANAGE_LIBRARY";
      case SQLOperation::MANAGE_LOAD: return "MANAGE_LOAD";
      case SQLOperation::MANAGE_PRIVILEGES: return "MANAGE_PRIVILEGES";
      case SQLOperation::MANAGE_ROLES: return "MANAGE_ROLES";
      case SQLOperation::MANAGE_STATISTICS: return "MANAGE_STATISTICS";
      case SQLOperation::MANAGE_USERS: return "MANAGE_USERS";
      case SQLOperation::QUERY_ACTIVATE: return "QUERY_ACTIVATE";
      case SQLOperation::QUERY_CANCEL: return "QUERY_CANCEL";
      case SQLOperation::QUERY_SUSPEND: return "QUERY_SUSPEND";
      case SQLOperation::REGISTER_HIVE_OBJECT: return "REGISTER_HIVE_OBJECT";
      case SQLOperation::REMAP_USER: return "REMAP_USER";
      case SQLOperation::SHOW: return "SHOW";
      case SQLOperation::UNREGISTER_HIVE_OBJECT: return "UNREGISTER_HIVE_OBJECT";
      case SQLOperation::USE_ALTERNATE_SCHEMA: return "USE_ALTERNATE_SCHEMA";
	  case SQLOperation::COMMENT: return "COMMENT";
      default:
         return "UNKNOWN";   
   }

   return "UNKNOWN";   

}    
//******************** End of PrivMgr::getSQLOperationName *********************
    
// *****************************************************************************
// *                                                                           *
// * Function: PrivMgr::getSQLOperationCode                                    *
// *                                                                           *
// *    Returns the operation code associated with the specified operation.    *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <operation>                     SQLOperation                    In       *
// *    is the operation.                                                      *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: const char                                                       *
// *                                                                           *
// *  If the operation exists, the corresponding code is returned, otherwise   *
// *  the string "  " is returned.                                             *
// *                                                                           *
// *****************************************************************************
const char * PrivMgr::getSQLOperationCode(SQLOperation operation) 

{

   switch (operation)
   {
      case SQLOperation::ALTER: return "A0";
      case SQLOperation::ALTER_LIBRARY: return "AL";
      case SQLOperation::ALTER_ROUTINE: return "AR";
      case SQLOperation::ALTER_ROUTINE_ACTION: return "AA";
      case SQLOperation::ALTER_SCHEMA: return "AH";
      case SQLOperation::ALTER_SEQUENCE: return "AQ";
      case SQLOperation::ALTER_SYNONYM: return "AY";
      case SQLOperation::ALTER_TABLE: return "AT";
      case SQLOperation::ALTER_TRIGGER: return "AG";
      case SQLOperation::ALTER_VIEW: return "AV";
      case SQLOperation::CREATE: return "C0";
      case SQLOperation::CREATE_CATALOG: return "CC";
      case SQLOperation::CREATE_INDEX: return "CI";
      case SQLOperation::CREATE_LIBRARY: return "CL";
      case SQLOperation::CREATE_PROCEDURE: return "CP";
      case SQLOperation::CREATE_ROUTINE: return "CR";
      case SQLOperation::CREATE_ROUTINE_ACTION: return "CA";
      case SQLOperation::CREATE_SCHEMA: return "CH";
      case SQLOperation::CREATE_SEQUENCE: return "CQ";
      case SQLOperation::CREATE_SYNONYM: return "CY";
      case SQLOperation::CREATE_TABLE: return "CT";
      case SQLOperation::CREATE_TRIGGER: return "CG";
      case SQLOperation::CREATE_VIEW: return "CV";
      case SQLOperation::DML_DELETE: return "PD";
      case SQLOperation::DML_EXECUTE: return "PE";
      case SQLOperation::DML_INSERT: return "PI";
      case SQLOperation::DML_REFERENCES: return "PR";
      case SQLOperation::DML_SELECT: return "PS";
      case SQLOperation::DML_UPDATE: return "PU";
      case SQLOperation::DML_USAGE: return "PG";
      case SQLOperation::DROP: return "D0";
      case SQLOperation::DROP_CATALOG: return "DC";
      case SQLOperation::DROP_INDEX: return "DI";
      case SQLOperation::DROP_LIBRARY: return "DL";
      case SQLOperation::DROP_PROCEDURE: return "DP";
      case SQLOperation::DROP_ROUTINE: return "DR";
      case SQLOperation::DROP_ROUTINE_ACTION: return "DA";
      case SQLOperation::DROP_SCHEMA: return "DH";
      case SQLOperation::DROP_SEQUENCE: return "DQ";
      case SQLOperation::DROP_SYNONYM: return "DY";
      case SQLOperation::DROP_TABLE: return "DT";
      case SQLOperation::DROP_TRIGGER: return "DG";
      case SQLOperation::DROP_VIEW: return "DV";
      case SQLOperation::MANAGE: return "M0";
      case SQLOperation::MANAGE_COMPONENTS: return "MC";
      case SQLOperation::MANAGE_LIBRARY: return "ML";
      case SQLOperation::MANAGE_LOAD: return "MT";
      case SQLOperation::MANAGE_PRIVILEGES: return "MP";
      case SQLOperation::MANAGE_ROLES: return "MR";
      case SQLOperation::MANAGE_STATISTICS: return "MS";
      case SQLOperation::MANAGE_USERS: return "MU";
      case SQLOperation::QUERY_ACTIVATE: return "QA";
      case SQLOperation::QUERY_CANCEL: return "QC";
      case SQLOperation::QUERY_SUSPEND: return "QS";
      case SQLOperation::REGISTER_HIVE_OBJECT: return "RH";
      case SQLOperation::REMAP_USER: return "RU";
      case SQLOperation::SHOW: return "SW";
      case SQLOperation::UNREGISTER_HIVE_OBJECT: return "UH";
      case SQLOperation::USE_ALTERNATE_SCHEMA: return "UA";
      case SQLOperation::COMMENT: return "CO";
      default:
         return "  ";   
   }

   return "  ";   

}
//******************** End of PrivMgr::getSQLOperationCode *********************



// *****************************************************************************
// *                                                                           *
// * Function: PrivMgr::getSQLOperationDescription                             *
// *                                                                           *
// *    Returns the description for the specified SQL operation.  Note, all    *
// * SQL operations have a description.                                        *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <operation>                     SQLOperation                    In       *
// *    is the operation.                                                      *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: const char                                                       *
// *                                                                           *
// *  If the operation exists, the corresponding description is returned,      *
// *  otherwise the empty string "" is returned.                               *
// *                                                                           *
// *****************************************************************************
const char * PrivMgr::getSQLOperationDescription(SQLOperation operation) 

{

   switch (operation)
   {
      case SQLOperation::ALTER: return "Allow grantee to alter database objects";
      case SQLOperation::ALTER_LIBRARY: return "Allow grantee to alter libraries";
      case SQLOperation::ALTER_ROUTINE: return "Allow grantee to alter routines";
      case SQLOperation::ALTER_ROUTINE_ACTION: return "Allow grantee to alter routine actions";
      case SQLOperation::ALTER_SCHEMA: return "Allow grantee to alter schemas";
      case SQLOperation::ALTER_SEQUENCE: return "Allow grantee to alter sequence generators";
      case SQLOperation::ALTER_SYNONYM: return "Allow grantee to alter synonyms";
      case SQLOperation::ALTER_TABLE: return "Allow grantee to alter tables";
      case SQLOperation::ALTER_TRIGGER: return "Allow grantee to alter triggers";
      case SQLOperation::ALTER_VIEW: return "Allow grantee to alter views";
      case SQLOperation::CREATE: return "Allow grantee to create database objects";
      case SQLOperation::CREATE_CATALOG: return "Allow grantee to create catalogs";
      case SQLOperation::CREATE_INDEX: return "Allow grantee to create indexes";
      case SQLOperation::CREATE_LIBRARY: return "Allow grantee to create libraries";
      case SQLOperation::CREATE_PROCEDURE: return "Allow grantee to create procedures";
      case SQLOperation::CREATE_ROUTINE: return "Allow grantee to create routines";
      case SQLOperation::CREATE_ROUTINE_ACTION: return "Allow grantee to create routine actions";
      case SQLOperation::CREATE_SCHEMA: return "Allow grantee to create schemas";
      case SQLOperation::CREATE_SEQUENCE: return "Allow grantee to create sequence generators";
      case SQLOperation::CREATE_SYNONYM: return "Allow grantee to create synonyms";
      case SQLOperation::CREATE_TABLE: return "Allow grantee to create tables";
      case SQLOperation::CREATE_TRIGGER: return "Allow grantee to create triggers";
      case SQLOperation::CREATE_VIEW: return "Allow grantee to create views";
      case SQLOperation::DML_DELETE: return "Allow grantee to delete rows";
      case SQLOperation::DML_EXECUTE: return "Allow grantee to execute functions";
      case SQLOperation::DML_INSERT: return "Allow grantee to insert rows";
      case SQLOperation::DML_REFERENCES: return "Allow grantee to reference columns";
      case SQLOperation::DML_SELECT: return "Allow grantee to select rows";
      case SQLOperation::DML_UPDATE: return "Allow grantee to update rows";
      case SQLOperation::DML_USAGE: return "Allow grantee to use libraries and sequences";
      case SQLOperation::DROP: return "Allow grantee to drop database objects";
      case SQLOperation::DROP_CATALOG: return "Allow grantee to drop catalogs";
      case SQLOperation::DROP_INDEX: return "Allow grantee to drop indexes";
      case SQLOperation::DROP_LIBRARY: return "Allow grantee to drop libraries";
      case SQLOperation::DROP_PROCEDURE: return "Allow grantee to drop procedures";
      case SQLOperation::DROP_ROUTINE: return "Allow grantee to drop routines";
      case SQLOperation::DROP_ROUTINE_ACTION: return "Allow grantee to drop routine actions";
      case SQLOperation::DROP_SCHEMA: return "Allow grantee to drop schemas";
      case SQLOperation::DROP_SEQUENCE: return "Allow grantee to drop sequence generators";
      case SQLOperation::DROP_SYNONYM: return "Allow grantee to drop synonyms";
      case SQLOperation::DROP_TABLE: return "Allow grantee to drop tables";
      case SQLOperation::DROP_TRIGGER: return "Allow grantee to drop triggers";
      case SQLOperation::DROP_VIEW: return "Allow grantee to drop views";
      case SQLOperation::MANAGE: return "Allow grantee to manage all SQL Operations";
      case SQLOperation::MANAGE_COMPONENTS: return "Allow grantee to manage components";
      case SQLOperation::MANAGE_LIBRARY: return "Allow grantee to manage libraries";
      case SQLOperation::MANAGE_LOAD: return "Allow grantee to perform LOAD and UNLOAD commands";
      case SQLOperation::MANAGE_PRIVILEGES: return "Allow grantee to manage privileges on SQL objects";
      case SQLOperation::MANAGE_ROLES: return "Allow grantee to manage roles";
      case SQLOperation::MANAGE_STATISTICS: return "Allow grantee to show and update statistics";
      case SQLOperation::MANAGE_USERS: return "Allow grantee to manage users";
      case SQLOperation::QUERY_ACTIVATE: return "Allow grantee to activate queries";
      case SQLOperation::QUERY_CANCEL: return "Allow grantee to cancel queries";
      case SQLOperation::QUERY_SUSPEND: return "Allow grantee to suspend queries";
      case SQLOperation::REGISTER_HIVE_OBJECT: return "Allow grantee to register hive object in traf metadata";
      case SQLOperation::REMAP_USER: return "Allow grantee to remap DB__ users to a different external username";
      case SQLOperation::SHOW: return "Allow grantee to view metadata information about objects";
      case SQLOperation::UNREGISTER_HIVE_OBJECT: return "Allow grantee to unregister hive object from traf metadata";
      case SQLOperation::USE_ALTERNATE_SCHEMA: return "Allow grantee to use non-default schemas";
	  case SQLOperation::COMMENT: return "Allow grantee to comment on objects and columns";
      default:
         return "";   
   }

   return "";   

}
//**************** End of PrivMgr::getSQLOperationDescription ******************


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgr::isAuthIDGrantedPrivs                                   *
// *                                                                           *
// *    Determines if the specified authorization ID has been granted one or   *
// * more privileges.                                                          *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <authID>                        const int32_t                   In       *
// *    is the authorization ID.                                               *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true: Authorization ID has been granted one or more privileges.           *
// * false: Authorization ID has not been granted any privileges.              *
// *                                                                           *
// *****************************************************************************
bool PrivMgr::isAuthIDGrantedPrivs(
   const int32_t authID,
   std::vector<PrivClass> privClasses,
   std::vector<int64_t> &objectUIDs) 
{

// Check for empty vector.
   if (privClasses.size() == 0)
      return false;
      
// If authorization is not enabled, no privileges were granted to anyone. 
   if (!isAuthorizationEnabled())
      return false;
      

// Special case of PrivClass::ALL.  Caller does not need to change when
// new a new PrivClass is added. 
   if (privClasses.size() == 1 && privClasses[0] == PrivClass::ALL)
   {
      PrivMgrPrivileges objectPrivileges(metadataLocation_,pDiags_); 
      
      if (objectPrivileges.isAuthIDGrantedPrivs(authID, objectUIDs))
         return true;
      
      PrivMgrComponentPrivileges componentPrivileges(metadataLocation_,pDiags_); 
      
      if (componentPrivileges.isAuthIDGrantedPrivs(authID))
         return true;
   
      return false;   
   }

// Called specified one or more specific PrivClass.  Note, ALL is not valid  
// in a list, only by itself.   
   for (size_t pc = 0; pc < privClasses.size(); pc++)
      switch (privClasses[pc])
      {
         case PrivClass::OBJECT:
         {
            PrivMgrPrivileges objectPrivileges(metadataLocation_,pDiags_); 
            
            if (objectPrivileges.isAuthIDGrantedPrivs(authID, objectUIDs))
               return true;
             
            break;
         
         } 
         case PrivClass::COMPONENT:
         {
            PrivMgrComponentPrivileges componentPrivileges(metadataLocation_,pDiags_); 
            
            if (componentPrivileges.isAuthIDGrantedPrivs(authID))
               return true;
         
            break;
         } 
         case PrivClass::ALL:
         default:
         {
            PRIVMGR_INTERNAL_ERROR("Switch statement in PrivMgr::isAuthIDGrantedPrivs()");
            return STATUS_ERROR;
            break;
         }
      }

// No grants of any privileges found for this authorization ID.   
   return false;
      
}
//******************* End of PrivMgr::isAuthIDGrantedPrivs *********************

// *****************************************************************************
// *                                                                           *
// * Function: PrivMgr::isSQLAlterOperation                                    *
// *                                                                           *
// *    Determines if a SQL operation is within the subset of alter operations *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <operation>                     SQLOperation                    In       *
// *    is the operation.                                                      *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true: operation is an alter operation.                                    *
// * false: operation is not an alter operation.                               *
// *                                                                           *
// *****************************************************************************
bool PrivMgr::isSQLAlterOperation(SQLOperation operation)

{

   if (operation == SQLOperation::ALTER_TABLE ||
       operation == SQLOperation::ALTER_VIEW ||
       operation == SQLOperation::ALTER_SCHEMA ||
       operation == SQLOperation::ALTER_SEQUENCE ||
       operation == SQLOperation::ALTER_TRIGGER ||
       operation == SQLOperation::ALTER_ROUTINE ||
       operation == SQLOperation::ALTER_ROUTINE_ACTION ||
       operation == SQLOperation::ALTER_LIBRARY)
      return true;
      
   return false;

}
//******************** End of PrivMgr::isSQLAlterOperation *********************


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgr::isSQLCreateOperation                                   *
// *                                                                           *
// *    Determines if a SQL operation is within the subset of create operations*
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <operation>                     SQLOperation                    In       *
// *    is the operation.                                                      *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true: operation is a create operation.                                    *
// * false: operation is not a create operation.                               *
// *                                                                           *
// *****************************************************************************
bool PrivMgr::isSQLCreateOperation(SQLOperation operation)

{

   if (operation == SQLOperation::CREATE_TABLE ||
       operation == SQLOperation::CREATE_VIEW ||
       operation == SQLOperation::CREATE_SEQUENCE ||
       operation == SQLOperation::CREATE_TRIGGER ||
       operation == SQLOperation::CREATE_SCHEMA ||
       operation == SQLOperation::CREATE_CATALOG ||
       operation == SQLOperation::CREATE_INDEX ||
       operation == SQLOperation::CREATE_LIBRARY ||
       operation == SQLOperation::CREATE_PROCEDURE ||
       operation == SQLOperation::CREATE_ROUTINE ||
       operation == SQLOperation::CREATE_ROUTINE_ACTION ||
       operation == SQLOperation::CREATE_SYNONYM ||
       operation == SQLOperation::REGISTER_HIVE_OBJECT)
      return true;
      
   return false;

}
//******************* End of PrivMgr::isSQLCreateOperation *********************


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgr::isSQLDropOperation                                     *
// *                                                                           *
// *    Determines if a SQL operation is within the subset of drop operations. *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <operation>                     SQLOperation                    In       *
// *    is the operation.                                                      *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true: operation is a drop operation.                                      *
// * false: operation is not a drop operation.                                 *
// *                                                                           *
// *****************************************************************************
bool PrivMgr::isSQLDropOperation(SQLOperation operation)

{

   if (operation == SQLOperation::DROP_TABLE ||
       operation == SQLOperation::DROP_VIEW ||
       operation == SQLOperation::DROP_SEQUENCE ||
       operation == SQLOperation::DROP_TRIGGER ||
       operation == SQLOperation::DROP_SCHEMA ||
       operation == SQLOperation::DROP_CATALOG ||
       operation == SQLOperation::DROP_INDEX ||
       operation == SQLOperation::DROP_LIBRARY ||
       operation == SQLOperation::DROP_PROCEDURE ||
       operation == SQLOperation::DROP_ROUTINE ||
       operation == SQLOperation::DROP_ROUTINE_ACTION ||
       operation == SQLOperation::DROP_SYNONYM ||
       operation == SQLOperation::UNREGISTER_HIVE_OBJECT) 
      return true;
      
   return false;

}
//******************** End of PrivMgr::isSQLDropOperation **********************


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgr::isSQLManageOperation                                   *
// *                                                                           *
// *    Determines if a SQL operation is within the list of manage operations. *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <operation>                     SQLOperation                    In       *
// *    is the operation.                                                      *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true: operation is a manage operation.                                    *
// * false: operation is not a manage operation.                               *
// *                                                                           *
// *****************************************************************************
bool PrivMgr::isSQLManageOperation(SQLOperation operation)

{

   if (operation == SQLOperation::MANAGE_COMPONENTS ||
       operation == SQLOperation::MANAGE_LIBRARY ||
       operation == SQLOperation::MANAGE_LOAD ||
       operation == SQLOperation::MANAGE_PRIVILEGES ||
       operation == SQLOperation::MANAGE_ROLES ||
       operation == SQLOperation::MANAGE_STATISTICS ||
       operation == SQLOperation::MANAGE_USERS)
      return true;
      
   return false;

}
//******************* End of PrivMgr::isSQLManageOperation *********************


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgr::isSQLManageOperation                                   *
// *                                                                           *
// *    Determines if a SQL operation is within the list of manage operations. *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <operation>                     SQLOperation                    In       *
// *    is the operation.                                                      *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true: operation is a manage operation.                                    *
// * false: operation is not a manage operation.                               *
// *                                                                           *
// *****************************************************************************
bool PrivMgr::isSQLManageOperation(const char * operationCode)

{
  size_t numOps = sizeof(sqlOpList)/sizeof(ComponentOpStruct);
  for (int i = 0; i < numOps; i++)
  {
    const ComponentOpStruct &opDefinition = sqlOpList[i];
    if (std::string(opDefinition.operationCode) == std::string(operationCode))
      return (PrivMgr::isSQLManageOperation((SQLOperation)opDefinition.operationID));
   }
   return false;
}
//******************* End of PrivMgr::isSQLManageOperation *********************


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgr::ObjectEnumToLit                                        *
// *                                                                           *
// *    Returns the two character literal associated with the object type enum.*
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <objectType>                    ComObjectType                   In       *
// *    is the object type enum.                                               *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: const char                                                       *
// *                                                                           *
// *****************************************************************************
const char * PrivMgr::ObjectEnumToLit(ComObjectType objectType)

{
  return comObjectTypeLit(objectType);
}
//********************* End of PrivMgr::ObjectEnumToLit ************************


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgr::ObjectLitToEnum                                        *
// *                                                                           *
// *    Returns the enum associated with the object type literal.              *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <objectType>                    ComObjectType                   In       *
// *    is the object type enum.                                               *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: ComObjectType                                                    *
// *                                                                           *
// *****************************************************************************
ComObjectType PrivMgr::ObjectLitToEnum(const char *objectLiteral)

{

   for (size_t i = 0; i < occurs(objectTypeConversionTable); i++)
   {
      const literalAndEnumStruct & elem = objectTypeConversionTable[i];
      if (!strncmp(elem.literal_,objectLiteral,2))
         return static_cast<ComObjectType>(elem.enum_);
   }
   
   return COM_UNKNOWN_OBJECT;
   
}

//********************* End of PrivMgr::ObjectLitToEnum ************************


static void translateObjectName(
  const std::string inputName,
  std::string &outputName)
{
  char prefix[inputName.length()];
  snprintf(prefix, sizeof(prefix), "%s.\"%s\"",
     HBASE_SYSTEM_CATALOG, HBASE_EXT_MAP_SCHEMA);
}

// ----------------------------------------------------------------------------
// method: isAuthorizationEnabled
//
// Return true if authorization has been enabled, false otherwise.
//
// ----------------------------------------------------------------------------
bool PrivMgr::isAuthorizationEnabled()
{
  // If authorizationEnabled_ not setup in class, go determine status
  std::set<std::string> existingObjectList;
  if (authorizationEnabled_ == PRIV_INITIALIZE_UNKNOWN)
    authorizationEnabled_ = authorizationEnabled(existingObjectList);

  // return true if PRIV_INITIALIZED
  return (authorizationEnabled_ == PRIV_INITIALIZED);
}

// ----------------------------------------------------------------------------
// method: resetFlags
//
// Resets parserflag settings.
// 
// At PrivMgr construction time, existing parserflags are saved and additional
// parserflags are turned on.  This is needed so privilege manager
// requests work without requiring special privileges.
//
// The parserflags are restored at class destruction. 
//
// Generally, the PrivMgr class is constructed, the operation performed and the
// class destructed.  If some code requires the class to be constructed and 
// kept around for awhile, the coder may want reset any parserflags set
// by the constructor between PrivMgr calls. This way code inbetween PrivMgr 
// calls won't have any unexpected parser flags set.
//
// If parserflags are reset, then setFlags must be called before the next
// PrivMgr request.
// ----------------------------------------------------------------------------
void PrivMgr::resetFlags()
{
  // restore parser flag settings
  // The parserflag requests return a unsigned int return code of 0
  SQL_EXEC_AssignParserFlagsForExSqlComp_Internal(parserFlags_);
}

// ----------------------------------------------------------------------------
// method: setFlags
//
// saves parserflag settings and sets the INTERNAL_QUERY_FROM_EXEUTIL 
// parserflag
//
// See comments for PrivMgr::reset for more details
//
// ----------------------------------------------------------------------------
void PrivMgr::setFlags()
{
  // set the EXEUTIL parser flag to allow all privmgr internal queries
  // to pass security checks
  // The parserflag requests return a unsigned int return code of 0
  SQL_EXEC_GetParserFlagsForExSqlComp_Internal(parserFlags_);
  SQL_EXEC_SetParserFlagsForExSqlComp_Internal(INTERNAL_QUERY_FROM_EXEUTIL);
}

// ----------------------------------------------------------------------------
// method::log
//
// sends a message to log4cxx implementation designed by SQL
//
// Input:
//    filename - code file that is performing the request 
//    message  - the message to log
//    index    - index for logging that loops through a list
//
// Background
//   Privilege manager code sets up a message and calls this log method
//   This method calls SQLMXLoggingArea::logPrivMgrInfo described in 
//      sqlmxevents/logmxevent_traf (.h & .cpp)
//   logPrivMgrInfo is a wrapper class around qmscommon/QRLogger (.h & .cpp)
//      log method
//   QRLogger generates a message calls the log method in 
//      sqf/commonLogger/CommonLogger (.h & .cpp) 
//   CommonLogger interfaces with the log4cxx code which eventually puts
//      a message into a log file called $TRAF_LOG/master_exec_0_pid.log.  
//      A new master log is created for each new SQL process started.
//
// Sometimes it is amazing that things actually work with all these levels
// of interfaces.  Perhaps we can skip a few levels...  
// ----------------------------------------------------------------------------
void PrivMgr::log(
  const std::string filename,
  const std::string message,
  const int_32 index)
{ 
  std::string logMessage (filename);
  logMessage += ": ";
  logMessage += message;
  if (index >= 0)
  {
    logMessage += ", index level is ";
    logMessage += to_string((long long int)index); 
  }

  SQLMXLoggingArea::logPrivMgrInfo("Privilege Manager", 0, logMessage.c_str(), 0);
  
}


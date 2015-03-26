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

// *****************************************************************************
//    PrivMgr methods
// *****************************************************************************
// -----------------------------------------------------------------------
// Default Constructor
// -----------------------------------------------------------------------
PrivMgr::PrivMgr() 
: trafMetadataLocation_ ("TRAFODION.\"_MD_\""),
  metadataLocation_ ("TRAFODION.\"_PRIVMGR_MD_\""),
  pDiags_(CmpCommon::diags()),
  authorizationEnabled_(PRIV_INITIALIZE_UNKNOWN)
{}

// -----------------------------------------------------------------------
// Construct a PrivMgr object specifying a different metadata location
// -----------------------------------------------------------------------
  

PrivMgr::PrivMgr( 
   const std::string & metadataLocation,
   ComDiagsArea * pDiags,
   PrivMDStatus authorizationEnabled)
: metadataLocation_ (metadataLocation),
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
PrivMgr::PrivMgr(const PrivMgrMDAdmin &other)

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
PrivMgr::PrivMDStatus PrivMgr::authorizationEnabled()
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

  ExeCliInterface cliInterface(STMTHEAP, NULL, NULL, 
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
  std::set<std::string> existingObjectList;
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
      case SQLOperation::MANAGE_COMPONENTS: return "MANAGE_COMPONENTS";
      case SQLOperation::MANAGE_LIBRARY: return "MANAGE_LIBRARY";
      case SQLOperation::MANAGE_LOAD: return "MANAGE_LOAD";
      case SQLOperation::MANAGE_ROLES: return "MANAGE_ROLES";
      case SQLOperation::MANAGE_STATISTICS: return "MANAGE_STATISTICS";
      case SQLOperation::MANAGE_USERS: return "MANAGE_USERS";
      case SQLOperation::QUERY_ACTIVATE: return "QUERY_ACTIVATE";
      case SQLOperation::QUERY_CANCEL: return "QUERY_CANCEL";
      case SQLOperation::QUERY_SUSPEND: return "QUERY_SUSPEND";
      case SQLOperation::REMAP_USER: return "REMAP_USER";
      case SQLOperation::SHOW: return "SHOW";
      case SQLOperation::USE_ALTERNATE_SCHEMA: return "USE_ALTERNATE_SCHEMA";
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
      case SQLOperation::MANAGE_COMPONENTS: return "MC";
      case SQLOperation::MANAGE_LIBRARY: return "ML";
      case SQLOperation::MANAGE_LOAD: return "MT";
      case SQLOperation::MANAGE_ROLES: return "MR";
      case SQLOperation::MANAGE_STATISTICS: return "MS";
      case SQLOperation::MANAGE_USERS: return "MU";
      case SQLOperation::QUERY_ACTIVATE: return "QA";
      case SQLOperation::QUERY_CANCEL: return "QC";
      case SQLOperation::QUERY_SUSPEND: return "QS";
      case SQLOperation::REMAP_USER: return "RU";
      case SQLOperation::SHOW: return "SW";
      case SQLOperation::USE_ALTERNATE_SCHEMA: return "UA";
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
      case SQLOperation::MANAGE_COMPONENTS: return "Allow grantee to manage components";
      case SQLOperation::MANAGE_LIBRARY: return "Allow grantee to manage libraries";
      case SQLOperation::MANAGE_LOAD: return "Allow grantee to perform LOAD and UNLOAD commands";
      case SQLOperation::MANAGE_ROLES: return "Allow grantee to manage roles";
      case SQLOperation::MANAGE_STATISTICS: return "Allow grantee to show and update statistics";
      case SQLOperation::MANAGE_USERS: return "Allow grantee to manage users";
      case SQLOperation::QUERY_ACTIVATE: return "Allow grantee to activate queries";
      case SQLOperation::QUERY_CANCEL: return "Allow grantee to cancel queries";
      case SQLOperation::QUERY_SUSPEND: return "Allow grantee to suspend queries";
      case SQLOperation::REMAP_USER: return "Allow grantee to remap DB__ users to a different external username";
      case SQLOperation::SHOW: return "Allow grantee to view metadata information about objects";
      case SQLOperation::USE_ALTERNATE_SCHEMA: return "Allow grantee to use non-default schemas";
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
   int32_t authID,
   std::vector<PrivClass> privClasses) 

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
      
      if (objectPrivileges.isAuthIDGrantedPrivs(authID))
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
            
            if (objectPrivileges.isAuthIDGrantedPrivs(authID))
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
       operation == SQLOperation::CREATE_SYNONYM)
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
       operation == SQLOperation::DROP_SYNONYM)
      return true;
      
   return false;

}
//******************** End of PrivMgr::isSQLDropOperation **********************


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

   for (size_t i = 0; i < occurs(objectTypeConversionTable); i++)
      if (objectType == objectTypeConversionTable[i].enum_)
         return objectTypeConversionTable[i].literal_;

   return COM_UNKNOWN_OBJECT_LIT;  
    
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


// ----------------------------------------------------------------------------
// method: isAuthorizationEnabled
//
// Return true if authorization has been enabled, false otherwise.
//
// ----------------------------------------------------------------------------
bool PrivMgr::isAuthorizationEnabled()
{
  // If authorizationEnabled_ not setup in class, go determine status
  if (authorizationEnabled_ == PRIV_INITIALIZE_UNKNOWN)
    authorizationEnabled_ = authorizationEnabled();

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


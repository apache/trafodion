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

// *****************************************************************************
// *
// * File:         CmpSeabaseDDLauth.cpp
// * Description:  Implements methods for user management
// *
// * Contains methods for classes:
// *   CmpSeabaseDDLauth
// *   CmpSeabaseDDLuser
// *
// *
// *****************************************************************************

#define   SQLPARSERGLOBALS_FLAGS  
#include "SqlParserGlobalsCmn.h"

#include "CmpSeabaseDDLauth.h"
#include "CmpSeabaseDDL.h"
#include "StmtDDLRegisterUser.h"
#include "StmtDDLAlterUser.h"
#include "StmtDDLCreateRole.h"
#include "ElemDDLGrantee.h"
#include "CompException.h"
#include "Context.h"
#include "dbUserAuth.h"
#include "ComUser.h"
#include "CmpDDLCatErrorCodes.h"
#include "NAStringDef.h"
#include "ExpHbaseInterface.h"
#include "PrivMgrCommands.h"
#include "PrivMgrRoles.h"
#include "PrivMgrComponentPrivileges.h"


#define  RESERVED_AUTH_NAME_PREFIX  "DB__"

inline static bool validateExternalUsername(
   const char *                                 externalUsername,
   DBUserAuth::AuthenticationConfiguration      configurationNumber,
   DBUserAuth::AuthenticationConfiguration &    foundConfigurationNumber);

// ****************************************************************************
// Class CmpSeabaseDDLauth methods
// ****************************************************************************

// ----------------------------------------------------------------------------
// Constructors
// ----------------------------------------------------------------------------
CmpSeabaseDDLauth::CmpSeabaseDDLauth(
   const NAString & systemCatalog,
   const NAString & MDSchema) 
: authID_(NA_UserIdDefault),
  authType_(COM_UNKNOWN_ID_CLASS),
  authCreator_(NA_UserIdDefault),
  authCreateTime_(0),
  authRedefTime_(0),
  authValid_(true),
  systemCatalog_(systemCatalog),
  MDSchema_(MDSchema)
{}

CmpSeabaseDDLauth::CmpSeabaseDDLauth() 
: authID_(NA_UserIdDefault),
  authType_(COM_UNKNOWN_ID_CLASS),
  authCreator_(NA_UserIdDefault),
  authCreateTime_(0),
  authRedefTime_(0),
  authValid_(true),
  systemCatalog_("TRAFODION"),
  MDSchema_("TRAFODION.\"_MD_\"")
  
{}

// ----------------------------------------------------------------------------
// public method:  authExists
//
// Input:
//   authName - name to look up
//   isExternal -
//       true - the auth name is the external name (auth_ext_name)
//       false - the auth name is the database name (auth_db_name)
//
// Output:
//   Returns true if authorization row exists in the metadata
//   Returns false if authorization row does not exist in the metadata or an 
//      unexpected error occurs
//
//  The diags area contains an error if any unexpected errors occurred.
//  Callers should check the diags area when false is returned
// ----------------------------------------------------------------------------
bool CmpSeabaseDDLauth::authExists (const NAString &authName, bool isExternal)
{
  // Read the auths table to get a count of rows for the authName
  Int64 rowCount = 0;
  try
  {
    NAString whereClause ("where ");
    whereClause += (isExternal) ? "auth_ext_name = '" : "auth_db_name = '";
    whereClause += authName;
    whereClause += "'";
    rowCount = selectCount(whereClause);
    return (rowCount > 0) ? true : false;
  }

  catch (...)
  {
    // If there is no error in the diags area, set up an internal error
    if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
       SEABASEDDL_INTERNAL_ERROR("CmpSeabaseDDLauth::authExists for authName");
    return false;
  }
}

// ----------------------------------------------------------------------------
// method:  describe
//
// This method is not valid for the base class
//
// Input:  none
//
// Output:  populates diag area, throws exception.
// ----------------------------------------------------------------------------
bool CmpSeabaseDDLauth::describe(const NAString &authName, NAString &authText)
{

   SEABASEDDL_INTERNAL_ERROR("CmpSeabaseDDLauth::describe");

UserException excp(NULL,0);

   throw excp;
   
   return 0;

}

// ----------------------------------------------------------------------------
// public method: getAuthDetails
//
// Populates the CmpSeabaseDDLauth class containing auth details for the
// requested authName
//
// Input:
//    authName - the database or external auth name
//    isExternal -
//       true - the auth name is the external name (auth_ext_name)
//       false - the auth name is the database name (auth_db_name)
//
// Output:
//    Returned parameter (AuthStatus):
//       STATUS_GOOD: authorization details are populated:
//       STATUS_NOTFOUND: authorization details were not found
//       STATUS_WARNING: (not 100) warning was returned, diags area populated
//       STATUS_ERROR: error was returned, diags area populated
// ----------------------------------------------------------------------------
CmpSeabaseDDLauth::AuthStatus 
CmpSeabaseDDLauth::getAuthDetails(const char *pAuthName, bool isExternal)
{
  // If the authname is a special PUBLIC authorization ID, set it up
  std::string authName = pAuthName;
  if (authName == PUBLIC_AUTH_NAME)
  {
    setAuthCreator(SUPER_USER);
    setAuthCreateTime(0);
    setAuthDbName(PUBLIC_AUTH_NAME);
    setAuthExtName(PUBLIC_AUTH_NAME);
    setAuthID(PUBLIC_USER);
    setAuthRedefTime(0);
    setAuthType(COM_ROLE_CLASS);
    setAuthValid(false);
    return STATUS_GOOD;
  }

  try
  {
    NAString whereClause ("where ");
    whereClause += (isExternal) ? "auth_ext_name = '" : "auth_db_name = '";
    whereClause += pAuthName;
    whereClause += "'";
    return selectExactRow(whereClause);
  }

  catch (...)
  {
    // If there is no error in the diags area, set up an internal error
    if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
       SEABASEDDL_INTERNAL_ERROR("CmpSeabaseDDLauth::getAuthDetails for authName");
    return STATUS_ERROR;
  }
}

// ----------------------------------------------------------------------------
// public method:  getAuthDetails
//
// Create the CmpSeabaseDDLauth class containing auth details for the
// request authID
//
// Input:
//    authID - the database authorization ID to search for
//
//  Output:
//    A returned parameter:
//       STATUS_GOOD: authorization details are populated:
//       STATUS_NOTFOUND: authorization details were not found
//       STATUS_WARNING: (not 100) warning was returned, diags area populated
//       STATUS_ERROR: error was returned, diags area populated
// ----------------------------------------------------------------------------
CmpSeabaseDDLauth::AuthStatus CmpSeabaseDDLauth::getAuthDetails(Int32 authID)
{
  try
  {
    char buf[1000];
    str_sprintf(buf, "where auth_id = %d ", authID);
    NAString whereClause (buf);
    return selectExactRow(whereClause);
  }

  catch (...)
  {
    // If there is no error in the diags area, set up an internal error
    if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
       SEABASEDDL_INTERNAL_ERROR("CmpSeabaseDDLauth::getAuthDetails for authID");

    return STATUS_ERROR;
  }
}

// ----------------------------------------------------------------------------
// public method:  getRoleIDs
//
// Return the list of roles that granted to the passed in authID
//
// Input:
//    authID - the database authorization ID to search for
//
//  Output:
//    A returned parameter:
//       STATUS_GOOD: list of roles returned
//       STATUS_NOTFOUND: no roles were granted
//       STATUS_ERROR: error was returned, diags area populated
// ----------------------------------------------------------------------------
CmpSeabaseDDLauth::AuthStatus CmpSeabaseDDLauth::getRoleIDs(
  const Int32 authID,
  std::vector<int32_t> &roleIDs,
  std::vector<int32_t> &grantees)
{
  NAString privMgrMDLoc;
  CONCAT_CATSCH(privMgrMDLoc,systemCatalog_.data(),SEABASE_PRIVMGR_SCHEMA);

  PrivMgrRoles role(std::string(MDSchema_.data()),
                    std::string(privMgrMDLoc.data()),
                    CmpCommon::diags());
  std::vector<std::string> roleNames;
  std::vector<int32_t> grantDepths;

  if (role.fetchRolesForAuth(authID,roleNames,roleIDs,grantDepths,grantees) == PrivStatus::STATUS_ERROR)
    return STATUS_ERROR;

  assert (roleIDs.size() == grantees.size());
  return STATUS_GOOD;
}

// ----------------------------------------------------------------------------
// public method:  getObjectName
//
// Returns the first object name from the list of passed in objectUIDs.
//
// Input:
//    objectUIDs - list of objectUIDs
//
//  Output:
//    returns the fully qualified object name
//    returns NULL string if no objects were found
// ----------------------------------------------------------------------------
NAString CmpSeabaseDDLauth::getObjectName (const std::vector <int64_t> objectUIDs)
{
  char longBuf [sizeof(int64_t)*8+1];
  bool isFirst = true;
  NAString objectList;
  NAString objectName;

  if (objectUIDs.size() == 0)
    return objectName;

  // convert objectUIDs into an "in" clause list
  for (int i = 0; i < objectUIDs.size(); i++)
  {
    if (isFirst)
      objectList = "(";
    else
      objectList += ", ";
    isFirst = false;
    sprintf (longBuf, "%ld", objectUIDs[i]);
    objectList += longBuf;
  }
  objectList += ")";

  NAString sysCat = CmpSeabaseDDL::getSystemCatalogStatic();
  char buf[1000 + objectList.length()];
  str_sprintf(buf, "select [first 1] trim(catalog_name) || '.' || "
                   "trim(schema_name) || '.' || trim(object_name) "
                   " from %s.\"%s\".%s where object_uid in %s",
                   sysCat.data(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS, objectList.data());

  ExeCliInterface cliInterface(STMTHEAP);
  Int32 cliRC = cliInterface.fetchRowsPrologue(buf, true/*no exec*/);
  if (cliRC != 0)
  {
    cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
    UserException excp (NULL, 0);
    throw excp;
  }

  cliRC = cliInterface.clearExecFetchClose(NULL, 0);
  if (cliRC < 0)
  {
    cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
    UserException excp (NULL, 0);
    throw excp;
  }

  // return an empty string
  if (cliRC == 100)
    return objectName;

  // get the objectname
  char * ptr = NULL;
  Lng32 len = 0;

  // object name returned
  cliInterface.getPtrAndLen(1,ptr,len);
  NAString returnedName(ptr,len);
  return returnedName;
}


// ----------------------------------------------------------------------------
// method:  getUniqueAuthID
//
// Return an unused auth ID between the requested ranges
// Input:  
//   minValue - the lowest value
//   maxValue - the highest value
//
// Output: unique ID to use
//   exception is generated if unable to generate a unique value
// ----------------------------------------------------------------------------
Int32 CmpSeabaseDDLauth::getUniqueAuthID(
  const Int32 minValue, 
  const Int32 maxValue)
{
  Int32 newUserID = 0;
  char buf[300];
  Int32 len = snprintf(buf, 300,
                       "SELECT [FIRST 1] auth_id FROM (SELECT auth_id, "
                       "LEAD(auth_id) OVER (ORDER BY auth_id) L FROM %s.%s ) "
                       "WHERE (L - auth_id > 1 or L is null) and auth_id >= %d ",
                       MDSchema_.data(),SEABASE_AUTHS, minValue);
  assert (len <= 300);
  
  len = 0;
  Int64 metadataValue = 0;
  bool nullTerminate = false;
  
  ExeCliInterface cliInterface(STMTHEAP);
  Lng32 cliRC = cliInterface.executeImmediate(buf, (char *)&metadataValue, &len, nullTerminate);
  if (cliRC < 0)
  { 
    cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
    return 0;
  }
                          
  // We have lots of available ID's.  Don't expect to run out of ID's for awhile
  if (cliRC == 100 || metadataValue > maxValue)
  { 
    SEABASEDDL_INTERNAL_ERROR("CmpSeabaseDDLauth::getUniqueAuthID failed, ran out of available IDs");
    UserException excp (NULL, 0);
    throw excp;
  }

  newUserID = (Int32)metadataValue;
  if (newUserID == 0)
     newUserID = ROOT_USER_ID + 1;
  else
     newUserID++;

  // There is a bug where grants are not being removed from component privileges
  // when a user is dropped.  So if this authID still shows up as a component
  // privilege grantee go ahead a cleanup the inconsistency.
  std::string privMDLoc(CmpSeabaseDDL::getSystemCatalogStatic().data());
  privMDLoc += std::string(".\"") +
               std::string(SEABASE_PRIVMGR_SCHEMA) +
               std::string("\"");

  PrivMgrComponentPrivileges componentPrivs(privMDLoc,CmpCommon::diags());
  if (componentPrivs.isAuthIDGrantedPrivs(newUserID))
  {
    if (!componentPrivs.dropAllForGrantee(newUserID))
    {
      *CmpCommon::diags() << DgSqlCode(CAT_WARN_USED_AUTHID)
                          << DgInt0(newUserID);

      Int32 newMinValue = newUserID+1;
      newUserID = getUniqueAuthID(newUserID + 1, maxValue);
    }
  }

  return newUserID;
}


// ----------------------------------------------------------------------------
// method: isAuthNameReserved
//
// Checks to see if proposed name is reserved
//
// Input: authorization name
//
// Output:
//   true - name is reserved
//   false - name is not reserved
// ----------------------------------------------------------------------------
bool CmpSeabaseDDLauth::isAuthNameReserved (const NAString &authName)
{
  bool result;
  result = authName.length() >= strlen(RESERVED_AUTH_NAME_PREFIX)
           &&
           authName.operator()(0,strlen(RESERVED_AUTH_NAME_PREFIX)) ==
                                                   RESERVED_AUTH_NAME_PREFIX
           ||
           authName == SYSTEM_AUTH_NAME
           ||
           authName == PUBLIC_AUTH_NAME
           ||
           authName == "NONE";

  return result;
}

// ----------------------------------------------------------------------------
// method: isAuthNameValid
//
// checks to see if the name contains valid character
//
// Input:
//    NAString - authName -- name string to check
//
// Returns:  true if valid, false otherwise
// ----------------------------------------------------------------------------
bool CmpSeabaseDDLauth::isAuthNameValid(const NAString &authName)
{
  string validChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_@./";
  string strToScan = authName.data();
  size_t found = strToScan.find_first_not_of(validChars);
  if (found == string::npos)
    return true;
  return false;
}



// ----------------------------------------------------------------------------
// method: isRoleID
//
// Determines if an authID is in the role ID range
//
// Input:
//    Int32 - authID -- numeric ID to check
//
// Returns:  true if authID is a role ID, false otherwise
// ----------------------------------------------------------------------------
bool CmpSeabaseDDLauth::isRoleID(Int32 authID) 

{

   return (authID >= MIN_ROLEID && authID <= MAX_ROLEID);
   
}


// ----------------------------------------------------------------------------
// method: isUserID
//
// Determines if an authID is in the user ID range
//
// Input:
//    Int32 - authID -- numeric ID to check
//
// Returns:  true if authID is a user ID, false otherwise
// ----------------------------------------------------------------------------
bool CmpSeabaseDDLauth::isUserID(Int32 authID) 

{

   return (authID >= MIN_USERID && authID <= MAX_USERID);
   
}

// ----------------------------------------------------------------------------
// method: isSystemAuth
//
// Checks the list of authorization IDs to see if the passed in authName is a
//   system auth. This replaces checks for reserved names.
// 
// isSpecialAuth indicates a system auth but it is not defined in the metadata
//
// Returns:
//    true - is a system auth
//    false - is not a system auth
// ----------------------------------------------------------------------------
bool CmpSeabaseDDLauth::isSystemAuth(
  const ComIdClass authType,
  const NAString &authName,
  bool &isSpecialAuth)
{
  bool isSystem = false;
  switch (authType)
  {
    case COM_ROLE_CLASS:
    {
      int32_t numberRoles = sizeof(systemRoles)/sizeof(SystemAuthsStruct);
      for (int32_t i = 0; i < numberRoles; i++)
      {
        const SystemAuthsStruct &roleDefinition = systemRoles[i];
        if (roleDefinition.authName == authName)
        {
          isSystem = true;
          isSpecialAuth = roleDefinition.isSpecialAuth;
          break;
        }
      }
      break;
    }

    case COM_USER_CLASS:
    {
      // Verify name is a standard name
      std::string authNameStr(authName.data());
      size_t prefixLength = strlen(RESERVED_AUTH_NAME_PREFIX);
      if (authNameStr.size() <= prefixLength ||
          authNameStr.compare(0,prefixLength,RESERVED_AUTH_NAME_PREFIX) == 0)
        isSystem =  true;
      break;
    }

    default:
    {
      // should never get here - assert?
      isSystem = false;
    }
  }
  return isSystem;
}

// ----------------------------------------------------------------------------
// protected method: createStandardAuth
//
// Inserts a standard user or role in the Trafodion metadata
// The authType needs to be set up before calling
//
// Input:  
//    authName
//    authID
// ----------------------------------------------------------------------------
bool CmpSeabaseDDLauth::createStandardAuth(
   const std::string authName,
   const int32_t authID)
{
  // check to see if authName is a system object
  bool isSpecialAuth = false;
  bool isSystem = isSystemAuth(getAuthType(), NAString(authName.c_str()), isSpecialAuth);

  // since this is being called by internal code, should not be trying to 
  // create non system object (isSystemAuth) or object that should not be 
  // registered in the metadata (isSpecialAuth), return internal error
  if (!isSystem || isSpecialAuth)
  {
    NAString errorMsg ("Invalid system authorization identifier for ");
    errorMsg += getAuthType() == COM_ROLE_CLASS ? "role " : "user ";
    errorMsg += authName.c_str();
    SEABASEDDL_INTERNAL_ERROR(errorMsg.data());
    return false;
  }

  setAuthDbName(authName.c_str());
  setAuthExtName(authName.c_str());
  setAuthValid(true); // assume a valid authorization ID

  Int64 createTime = NA_JulianTimestamp();
  setAuthCreateTime(createTime);
  setAuthRedefTime(createTime);  // make redef time the same as create time

  // Make sure authorization ID has not already been registered
  if (authExists(getAuthDbName(),false))
    return false;

  try
  {
    Int32 minAuthID = isRole() ? MIN_ROLEID : MIN_USERID;
    Int32 maxAuthID = isRole() ? MAX_ROLEID : MAX_USERID;
  
    Int32 newAuthID = (authID == NA_UserIdDefault) ? getUniqueAuthID(minAuthID, maxAuthID) : authID;
    if (isRole())
      assert(isRoleID(newAuthID));
    else if (isUser())
      assert (isUserID(newAuthID));

    setAuthID(newAuthID);
    setAuthCreator(ComUser::getRootUserID());

    // Add the role to AUTHS table
    insertRow();
  }

  catch (...)
  {
    // At this time, an error should be in the diags area.
    // If there is no error, set up an internal error
    if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
       SEABASEDDL_INTERNAL_ERROR("Unexpected error in CmpSeabaseDDLuser::createStandardAuth");
  }
  return true;
}

//-----------------------------------------------------------------------------
// Methods that perform metadata access
//
// All methods return a UserException if an unexpected error occurs
//-----------------------------------------------------------------------------

// Delete a row from the AUTHS table based on the AUTH_ID
void CmpSeabaseDDLauth::deleteRow(const NAString &authName)
{
  NAString systemCatalog = CmpSeabaseDDL::getSystemCatalogStatic();
  char buf[1000];
  ExeCliInterface cliInterface(STMTHEAP);
  str_sprintf(buf, "delete from %s.\"%s\".%s where auth_db_name = '%s'",
              systemCatalog.data(), SEABASE_MD_SCHEMA, SEABASE_AUTHS, authName.data());
  Lng32 cliRC = cliInterface.executeImmediate(buf);
  
  if (cliRC < 0)
  {
    cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
    UserException excp (NULL, 0);
    throw excp;
  }

  // Existence of the row is already checked, throw an assertion the row does not exist
  // With optimistic locking, this might occur if a concurrent session is deleting the
  // same row and gets in first
  CMPASSERT (cliRC == 100);

  // Not sure if it is possible to get a warning from a delete and
  // what it means if one is returned for now, it is ignored.
}

// Insert a row into the AUTHS table
void CmpSeabaseDDLauth::insertRow()
{
  char buf[1000];
  ExeCliInterface cliInterface(STMTHEAP);

  NAString authType;
  switch (getAuthType())
  {
    case COM_ROLE_CLASS:
      authType = COM_ROLE_CLASS_LIT;
      break;
    case COM_USER_CLASS:
      authType = COM_USER_CLASS_LIT;
      break;
    default:
      SEABASEDDL_INTERNAL_ERROR("Switch statement in CmpSeabaseDDLuser::deleteRow invalid authType");
      UserException excp (NULL, 0);
      throw excp;
  }

  NAString authValid = isAuthValid() ? "Y" : "N";

  NAString sysCat = CmpSeabaseDDL::getSystemCatalogStatic();
  str_sprintf(buf, "insert into %s.\"%s\".%s values (%d, '%s', '%s', '%s', %d, '%s', %ld, %ld, 0)",
              sysCat.data(), SEABASE_MD_SCHEMA, SEABASE_AUTHS,
              getAuthID(),
              getAuthDbName().data(),
              getAuthExtName().data(),
              authType.data(),
              getAuthCreator(),
              authValid.data(),
              getAuthCreateTime(),
              getAuthRedefTime());

  Int32 cliRC = cliInterface.executeImmediate(buf);

  if (cliRC < 0)
  {
    cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
    UserException excp (NULL, 0);
    throw excp;
  }

  // Not sure if it is possible to get a warning from an insert and
  // what it means if one is returned, for now it is ignored.
  
}

// update a row in AUTHS table based on the passed in setClause
void CmpSeabaseDDLauth::updateRow(NAString &setClause)
{
  char buf[1000];
  ExeCliInterface cliInterface(STMTHEAP);

  NAString sysCat = CmpSeabaseDDL::getSystemCatalogStatic();
  str_sprintf(buf, "update %s.\"%s\".%s %s where auth_id = %d",
              sysCat.data(), SEABASE_MD_SCHEMA, SEABASE_AUTHS,
              setClause.data(),
              getAuthID());

  Int32 cliRC = cliInterface.executeImmediate(buf);

  if (cliRC < 0)
  {
    cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
    UserException excp (NULL, 0);
    throw excp;
  }
  
  // Existence of the row is already checked, throw an assertion the row does not exist
  // With optimistic locking, this might occur if a concurrent session is deleting the
  // same row and gets in first
  CMPASSERT (cliRC == 100);

  // Not sure if it is possible to get a warning from an update and
  // what it means if one is returned, for now it is ignored.
  
}

// select exact based on the passed in whereClause
CmpSeabaseDDLauth::AuthStatus 
CmpSeabaseDDLauth::selectExactRow(const NAString & whereClause)
{
   NAString sysCat = CmpSeabaseDDL::getSystemCatalogStatic();
   char buf[1000];
   str_sprintf(buf, "select auth_id, auth_db_name, auth_ext_name, auth_type, "
                    "auth_creator, auth_is_valid, auth_create_time, auth_redef_time"
                    " from %s.%s %s ",
               MDSchema_.data(), SEABASE_AUTHS, whereClause.data());
    NAString cmd (buf);

  ExeCliInterface cliInterface(STMTHEAP);

  Int32 cliRC = cliInterface.fetchRowsPrologue(cmd.data(), true/*no exec*/);
  if (cliRC != 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      DDLException excp (cliRC, NULL, 0);
      throw excp;
    }

  cliRC = cliInterface.clearExecFetchClose(NULL, 0);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      DDLException excp (cliRC, NULL, 0);
      throw excp;
    }

  if (cliRC == 100) // did not find the row
  {
    cliInterface.clearGlobalDiags();
    return STATUS_NOTFOUND;
  }

  // Set the return status
  CmpSeabaseDDLauth::AuthStatus authStatus = 
    (cliRC == 0) ? STATUS_GOOD : STATUS_WARNING;

  // Populate the class
  char * ptr = NULL;
  Lng32 len = 0;
  char type [6];

  // value 1:  auth_id (int32)
  cliInterface.getPtrAndLen(1, ptr, len);
  setAuthID(*(Int32*)ptr);

  // value 2: auth_db_name (NAString)
  cliInterface.getPtrAndLen(2,ptr,len);
  NAString dbName(ptr,len);
  setAuthDbName(dbName);

  // value 3: auth_ext_name (NAString)
  cliInterface.getPtrAndLen(3,ptr,len);
  NAString extName(ptr,len);
  setAuthExtName(extName);

  // value 4: auth_type (char)
  // str_cpy_and_null params: *tgt, *src, len, endchar, blank, null term
  cliInterface.getPtrAndLen(4,ptr,len);
  str_cpy_and_null(type, ptr, len, '\0', ' ', true);
  if ( type[0] == 'U')
    setAuthType(COM_USER_CLASS);
  else if (type[0] == 'R')
    setAuthType(COM_ROLE_CLASS);
  else
    setAuthType(COM_UNKNOWN_ID_CLASS);

  // value 5: auth_creator (int32)
  cliInterface.getPtrAndLen(5,ptr,len);
  setAuthCreator(*(Int32*)ptr);

  // value 6: auth_is_valid (char)
  cliInterface.getPtrAndLen(6,ptr,len);
  str_cpy_and_null(type, ptr, len, '\0', ' ', true);
  if (type[0] == 'Y')
    setAuthValid(true);
  else
    setAuthValid(false);

  // value 7: auth_create_time (int64)
  cliInterface.getPtrAndLen(7,ptr,len);
  Int64 intValue = *(Int64*)ptr;
  setAuthCreateTime((ComTimestamp) intValue);

  // value 8: auth_redef_time (int64)
  cliInterface.getPtrAndLen(8,ptr,len);
  intValue = *(Int64*)ptr;
  setAuthRedefTime((ComTimestamp) intValue);

  cliInterface.fetchRowsEpilogue(NULL, true);
  return authStatus;
}

// selectCount - returns the number of rows based on the where clause
Int64 CmpSeabaseDDLauth::selectCount(const NAString & whereClause)
{
  NAString sysCat = CmpSeabaseDDL::getSystemCatalogStatic();
  char buf[1000];
  str_sprintf(buf, "select count(*) from %s.%s %s ",
                MDSchema_.data(), SEABASE_AUTHS, 
                whereClause.data());

  Lng32 len = 0;
  Int64 rowCount = 0;
  ExeCliInterface cliInterface(STMTHEAP);
  Lng32 cliRC = cliInterface.executeImmediate(buf, (char*)&rowCount, &len, FALSE);

  // If unexpected error occurred, return an exception
  if (cliRC < 0)
  {
    cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
    DDLException excp (cliRC, NULL, 0);
    throw excp;
  }

  return rowCount;
}

// selectMaxAuthID - gets the last used auth ID
Int32 CmpSeabaseDDLauth::selectMaxAuthID(const NAString &whereClause)
{
  NAString sysCat = CmpSeabaseDDL::getSystemCatalogStatic();
  char buf[400];
  str_sprintf(buf, "select nvl(max (auth_id),0) from %s.%s %s" , 
              MDSchema_.data(),SEABASE_AUTHS,whereClause.data());

  Lng32 len = 0;
  Int64 maxValue = 0;
  bool nullTerminate = false;
  ExeCliInterface cliInterface(STMTHEAP);
  Lng32 cliRC = cliInterface.executeImmediate(buf, (char *)&maxValue, &len, nullTerminate);
  if (cliRC != 0)
  {
    cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
    UserException excp (NULL, 0);
    throw excp;
  }

  return static_cast<Int32>(maxValue);
}

// ----------------------------------------------------------------------------
// method: verifyAuthority
//
// makes sure user has privilege to perform the operation
//
// Input: none
//
// Output:  
//   true - authority granted
//   false - no authority or unexpected error
// ----------------------------------------------------------------------------
bool CmpSeabaseDDLauth::verifyAuthority(const SQLOperation operation)
{

   // If authorization is not enabled, just return with no error
   if (!CmpCommon::context()->isAuthorizationEnabled())
     return true;

   int32_t currentUser = ComUser::getCurrentUser();

   // Root user has authority to manage users.
   if (currentUser == ComUser::getRootUserID())
      return true;

   NAString systemCatalog = CmpSeabaseDDL::getSystemCatalogStatic();
   std::string privMDLoc(systemCatalog.data());

   privMDLoc += std::string(".\"") +
                std::string(SEABASE_PRIVMGR_SCHEMA) +
                std::string("\"");

   PrivMgrComponentPrivileges componentPrivileges(privMDLoc,CmpCommon::diags());

   // See if non-root user has authority to manage users.       
   if (componentPrivileges.hasSQLPriv(currentUser, operation, true))
   {
      return true;
   }

   return false;
}

// ****************************************************************************
// Class CmpSeabaseDDLuser methods
// ****************************************************************************

// ----------------------------------------------------------------------------
// Constructor
// ----------------------------------------------------------------------------
CmpSeabaseDDLuser::CmpSeabaseDDLuser(
   const NAString & systemCatalog,
   const NAString & MDSchema) 
: CmpSeabaseDDLauth(systemCatalog,MDSchema)

{}

CmpSeabaseDDLuser::CmpSeabaseDDLuser() 
: CmpSeabaseDDLauth()

{}

// ----------------------------------------------------------------------------
// public method: getUserDetails
//
// Create the CmpSeabaseDDLuser class containing user details for the
// requested userID
//
// Input:
//    userID - the database authorization ID to search for
//
//  Output:
//    Returned parameter:
//       STATUS_GOOD: authorization details are populated:
//       STATUS_NOTFOUND: authorization details were not found
//       STATUS_WARNING: (not 100) warning was returned, diags area populated
//       STATUS_ERROR: error was returned, diags area populated
// ----------------------------------------------------------------------------
CmpSeabaseDDLauth::AuthStatus CmpSeabaseDDLuser::getUserDetails(Int32 userID)
{
  CmpSeabaseDDLauth::AuthStatus retcode = getAuthDetails(userID);
  if (retcode == STATUS_GOOD && !isUser())
  {
    *CmpCommon::diags() << DgSqlCode (-CAT_IS_NOT_A_USER)
                        << DgString0(getAuthDbName().data());
     retcode =  STATUS_ERROR;
  }
  return retcode;
}

// ----------------------------------------------------------------------------
// public method: getUserDetails
//
// Create the CmpSeabaseDDLuser class containing user details for the
// requested username
//
// Input:
//    userName - the database username to retrieve details for
//    isExternal -
//       true - the username is the external name (auth_ext_name)
//       false - the username is the database name (auth_db_name)
//
//  Output:
//    Returned parameter:
//       STATUS_GOOD: authorization details are populated:
//       STATUS_NOTFOUND: authorization details were not found
//       STATUS_WARNING: (not 100) warning was returned, diags area populated
//       STATUS_ERROR: error was returned, diags area populated
// ----------------------------------------------------------------------------
CmpSeabaseDDLauth::AuthStatus 
CmpSeabaseDDLuser::getUserDetails(const char *pUserName, bool isExternal)
{
  CmpSeabaseDDLauth::AuthStatus retcode = getAuthDetails(pUserName, isExternal);
  if (retcode == STATUS_GOOD && !isUser())
  {
    *CmpCommon::diags() << DgSqlCode (-CAT_IS_NOT_A_USER)
                        << DgString0(getAuthDbName().data());
     retcode = STATUS_ERROR;
  }
  return retcode;
}

// ----------------------------------------------------------------------------
// Public method: registerUser
//
// registers a user in the Trafodion metadata
//
// Input:  parse tree containing a definition of the user
// Output: the global diags area is set up with the result
// ----------------------------------------------------------------------------
void CmpSeabaseDDLuser::registerUser(StmtDDLRegisterUser * pNode)
{
  // Set up a global try/catch loop to catch unexpected errors
  try
  {
    // Verify user is authorized to perform REGISTER USER requests
    if (!verifyAuthority(SQLOperation::MANAGE_USERS))
    {
      // No authority.  We're outta here.
      *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
      return;
    }

    // Verify that the specified user name is not reserved
    setAuthDbName(pNode->getDbUserName());
    if (isAuthNameReserved(pNode->getDbUserName()))
    {
      *CmpCommon::diags() << DgSqlCode (-CAT_AUTH_NAME_RESERVED)
                          << DgString0(pNode->getDbUserName().data());
      return;
    }


    // Verify that the name does not include unsupported special characters
    if (!isAuthNameValid(getAuthDbName()))
    {
      *CmpCommon::diags() << DgSqlCode (-CAT_INVALID_CHARS_IN_AUTH_NAME)
                          << DgString0(pNode->getDbUserName().data());
      return;
    }

    setAuthExtName(pNode->getExternalUserName());
    if (!isAuthNameValid(getAuthExtName()))
    {
      *CmpCommon::diags() << DgSqlCode (-CAT_INVALID_CHARS_IN_AUTH_NAME)
                          << DgString0(pNode->getExternalUserName().data());
      return;
    }

    // set up class members from parse node
    setAuthType(COM_USER_CLASS);  // we are a user
    setAuthValid(true); // assume a valid user

    Int64 createTime = NA_JulianTimestamp();
    setAuthCreateTime(createTime);
    setAuthRedefTime(createTime);  // make redef time the same as create time

    // Make sure db user has not already been registered
    if (authExists(getAuthDbName(), false))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_AUTHID_ALREADY_EXISTS)
                          << DgString0(getAuthDbName().data());
      return;
    }
    // unexpected error occurred - ?
    if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) > 0)
      return;

    // Make sure external user has not already been registered
    if (authExists(getAuthExtName(), true))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_LDAP_USER_ALREADY_EXISTS)
                          << DgString0(getAuthExtName().data());
       return;
    }
    // unexpected error occurred - ?
    if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) > 0)
      return;

DBUserAuth::AuthenticationConfiguration configurationNumber = DBUserAuth::DefaultConfiguration;
DBUserAuth::AuthenticationConfiguration foundConfigurationNumber = DBUserAuth::DefaultConfiguration;

    // Verify that the external user exists in configured identity store
    if (!validateExternalUsername(pNode->getExternalUserName().data(),
                                  configurationNumber,
                                  foundConfigurationNumber))
       return;

    // Get a unique auth ID number
    Int32 userID = getUniqueAuthID(MIN_USERID, MAX_USERID);
    assert(isUserID(userID));
    setAuthID (userID);

    // get effective user from the Context
    Int32 *pUserID = GetCliGlobals()->currContext()->getDatabaseUserID();
    setAuthCreator(*pUserID);

    // Add the user to AUTHS table
    insertRow();
#if 0    
    if (pNode->isSchemaSpecified())
    {
       ExeCliInterface cliInterface(STMTHEAP);
       char buf [1000];
       NAString csStmt;
                      COM_SCHEMA_CLASS_PRIVATE = 3,
                      COM_SCHEMA_CLASS_SHARED = 4,
                      COM_SCHEMA_CLASS_DEFAULT = 5};
       
       csStmt = "CREATE ";
       switch (pNode->getSchemaClass())
       {
          case COM_SCHEMA_CLASS_SHARED:
             csStmt += "SHARED ";
             break;
          case COM_SCHEMA_CLASS_DEFAULT:
          case COM_SCHEMA_CLASS_PRIVATE:
             csStmt += "PRIVATE ";
             break;
          default:
          {
             SEABASEDDL_INTERNAL_ERROR("Unknown schema class in registerUser");
             return;
          }
       }
           
       csStmt += pNode->getSchemaName()->getCatalogName();
       csStmt +- ".";  
       csStmt += pNode->getSchemaName()->getSchemaName();
       
       str_sprintf(buf, "CREATE %s SCHEMA %s \"%s\".\"%s\".\"%s\" cascade",
                   (char*)catName.data(), (char*)schName.data(), objName);
       
       cliRC = cliInterface.executeImmediate(buf);
    }
#endif       
  }
  catch (...)
  {
    // At this time, an error should be in the diags area.
    // If there is no error, set up an internal error
    if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
       SEABASEDDL_INTERNAL_ERROR("Unexpected error in CmpSeabaseDDLuser::registerUser");
  }
}

// ----------------------------------------------------------------------------
// public method:  unregisterUser
//
// This method removes a user from the database
//
// Input:  parse tree containing a definition of the user
// Output: the global diags area is set up with the result
// ----------------------------------------------------------------------------
void CmpSeabaseDDLuser::unregisterUser(StmtDDLRegisterUser * pNode)

{
  try
  {
    // CASCADE option not yet supported
    if (pNode->getDropBehavior() == COM_CASCADE_DROP_BEHAVIOR)
    {
      *CmpCommon::diags() << DgSqlCode (-CAT_ONLY_SUPPORTING_RESTRICT_DROP_BEHAVIOR);
      return;
    }

    // Verify that the specified user name is not reserved
    if (isAuthNameReserved(pNode->getDbUserName()))
    {
      *CmpCommon::diags() << DgSqlCode (-CAT_AUTH_NAME_RESERVED)
                          << DgString0(pNode->getDbUserName().data());
       return;
    }

    // read user details from the AUTHS table
    const NAString dbUserName(pNode->getDbUserName());
    CmpSeabaseDDLauth::AuthStatus retcode =  getUserDetails(dbUserName);
    if (retcode == STATUS_ERROR)
      return;
    if (retcode == STATUS_NOTFOUND)
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_USER_NOT_EXIST)
                          << DgString0(dbUserName.data());
      return;
    }

    if (ComUser::getCurrentUser() != getAuthCreator() && 
        !verifyAuthority(SQLOperation::MANAGE_USERS))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
      return;
    }

    NAString whereClause(" WHERE AUTH_TYPE = 'R' AND AUTH_CREATOR = ");
    
    char authIDString[20];
    
    sprintf(authIDString,"%d",getAuthID());
    whereClause += authIDString;
    
    if (selectCount(whereClause) > 0)
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_NO_UNREG_USER_OWNS_ROLES);
      return;
    }
    
    NAString privMgrMDLoc;
    CONCAT_CATSCH(privMgrMDLoc,systemCatalog_.data(),SEABASE_PRIVMGR_SCHEMA);

    // User does not own any roles, but may have been granted roles.
    if (CmpCommon::context()->isAuthorizationEnabled())
    {
    
      PrivMgrRoles role(std::string(MDSchema_.data()),
                        std::string(privMgrMDLoc.data()),
                        CmpCommon::diags());
    
      if (role.isUserGrantedAnyRole(getAuthID()))
      {
         *CmpCommon::diags() << DgSqlCode(-CAT_NO_UNREG_USER_GRANTED_ROLES);
         return;
      }
    }
    
    // Does user own any objects?
    NAString whereClause2(" WHERE OBJECT_OWNER = ");
      
    NAString sysCat = CmpSeabaseDDL::getSystemCatalogStatic();
    char buf[1000];
    str_sprintf(buf, "SELECT COUNT(*) FROM %s.\"%s\".%s %s %d",
                sysCat.data(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS, 
                whereClause2.data(),getAuthID());
      
    Int32 len = 0;
    Int64 rowCount = 0;
    ExeCliInterface cliInterface(STMTHEAP);
    Lng32 cliRC = cliInterface.executeImmediate(buf, (char*)&rowCount, &len, FALSE);
    if (cliRC != 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      UserException excp (NULL, 0);
      throw excp;
    }
    
    if (rowCount > 0)
    {
       *CmpCommon::diags() << DgSqlCode(-CAT_NO_UNREG_USER_OWNS_OBJECT);
       return;
    }
                
    // Is user granted any privileges?
    if (CmpCommon::context()->isAuthorizationEnabled())
    {
      PrivMgr privMgr(std::string(privMgrMDLoc),CmpCommon::diags());
      std::vector<PrivClass> privClasses;
    
      privClasses.push_back(PrivClass::ALL);
    
      std::vector<int64_t> objectUIDs;
      if (privMgr.isAuthIDGrantedPrivs(getAuthID(),privClasses, objectUIDs))
      {
         NAString objectName = getObjectName(objectUIDs);
         if (objectName.length() > 0)
         {
            *CmpCommon::diags() << DgSqlCode(-CAT_NO_UNREG_USER_HAS_PRIVS)
                                << DgString0(dbUserName.data())
                                << DgString1(objectName.data());

            return;
         }
      }
    }
    
    // remove any component privileges granted to this user
    if (CmpCommon::context()->isAuthorizationEnabled())
    {
      PrivMgrComponentPrivileges componentPrivileges(privMgrMDLoc.data(),CmpCommon::diags());
      std::string componentUIDString = "1";
      if (!componentPrivileges.dropAllForGrantee(getAuthID()))
      {
        UserException excp (NULL, 0);
        throw excp;
      }
    }

    // delete the row
    deleteRow(getAuthDbName());
  }
  catch (...)
  {
    // At this time, an error should be in the diags area.
    // If there is no error, set up an internal error
    if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
      SEABASEDDL_INTERNAL_ERROR("Switch statement in CmpSeabaseDDLuser::unregisterUser");
  }
}

// ----------------------------------------------------------------------------
// public method:  alterUser
//
// This method changes a user definition
//
// Input:  parse tree containing a definition of the user change
// Output: the global diags area is set up with the result
// ----------------------------------------------------------------------------
void CmpSeabaseDDLuser::alterUser (StmtDDLAlterUser * pNode)
{
  try
  {
    // read user details from the AUTHS table
    const NAString dbUserName(pNode->getDatabaseUsername());
    CmpSeabaseDDLauth::AuthStatus retcode = getUserDetails(dbUserName);
    if (retcode == STATUS_ERROR)
      return;
    if (retcode == STATUS_NOTFOUND)
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_USER_NOT_EXIST)
                          << DgString0(dbUserName.data());
      return;
    }

    if ((ComUser::getCurrentUser() != getAuthCreator()) && 
         !verifyAuthority(SQLOperation::MANAGE_USERS))
    {
       // No authority.  We're outta here.
       *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
       return;
    }

    // Process the requested operation
    NAString setClause("set ");
    switch (pNode->getAlterUserCmdSubType())
    {
      case StmtDDLAlterUser::SET_EXTERNAL_NAME:
      {
        // If authExtName already set to specified name, we are done
        if (getAuthExtName() == pNode->getExternalUsername()) 
          return;

        // Make sure external user has not already been registered
        if (authExists(pNode->getExternalUsername(), true))
        {
          *CmpCommon::diags() << DgSqlCode(-CAT_LDAP_USER_ALREADY_EXISTS)
                              << DgString0(pNode->getExternalUsername().data());
           return;
        }
        // unexpected error occurred - ?
        if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) > 0)
          return;

        DBUserAuth::AuthenticationConfiguration 
          configurationNumber = DBUserAuth::DefaultConfiguration;
        DBUserAuth::AuthenticationConfiguration 
          foundConfigurationNumber = DBUserAuth::DefaultConfiguration;

        // Verify that the external user exists in configured identity store
        if (!validateExternalUsername(pNode->getExternalUsername().data(),
                                      configurationNumber,
                                      foundConfigurationNumber))
          return;

        setAuthExtName(pNode->getExternalUsername());
        setClause += "auth_ext_name = '";
        setClause += getAuthExtName();
        setClause += "'";
        break;
     }

      case StmtDDLAlterUser::SET_IS_VALID_USER:
      {
        if (isAuthNameReserved(getAuthDbName()))
        {
          *CmpCommon::diags() << DgSqlCode(-CAT_AUTH_NAME_RESERVED)
                              << DgString0(getAuthDbName().data());
          return;
        }

        setAuthValid(pNode->isValidUser());
        setClause += (isAuthValid()) ? "auth_is_valid = 'Y'" : "auth_is_valid = 'N'";
        break;
      }

      default:
      {
        *CmpCommon::diags() << DgSqlCode (-CAT_UNSUPPORTED_COMMAND_ERROR );
        return;
      }
    }
    updateRow(setClause);
  }
  catch (...)
  {
    // At this time, an error should be in the diags area.
    // If there is no error, set up an internal error
    if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
      SEABASEDDL_INTERNAL_ERROR("Switch statement in CmpSeabaseDDLuser::alterUser");
  }
}

// ----------------------------------------------------------------------------
// method: registerStandardUser
//
// Creates a standard user ie. (DB__ROOT) in the Trafodion metadata
//
// Input:  
//    authName
//    authID
// ----------------------------------------------------------------------------
void CmpSeabaseDDLuser::registerStandardUser(
   const std::string authName,
   const int32_t authID)
{
  setAuthType(COM_USER_CLASS);  // we are a user
  createStandardAuth(authName, authID);
}

// -----------------------------------------------------------------------------
// *                                                                           *
// * Function: validateExternalUsername                                        *
// *                                                                           *
// *    Determines if an external username is valid.                           *
// *                                                                           *
// -----------------------------------------------------------------------------
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <externalUsername>          const char *                             In  *
// *    is the username to be validated.                                       *
// *                                                                           *
// *  <configurationNumber>      DBUserAuth::AuthenticationConfiguration   In  *
// *    specifies which configuration to use to validate the username; a       *
// *  configuration designates one or more identity stores and their           *
// *  parameters.  A value of zero indicates the "default" configuration       *
// *  should be used.                                                          *
// *                                                                           *
// *  <foundConfigurationNumber> DBUserAuth::AuthenticationConfiguration & In  *
// *    passes back the configuration used to validate the username.           *
// *                                                                           *
// -----------------------------------------------------------------------------
inline static bool validateExternalUsername(
   const char *                                 externalUsername,
   DBUserAuth::AuthenticationConfiguration      configurationNumber,
   DBUserAuth::AuthenticationConfiguration &    foundConfigurationNumber)

{

// During initialization external checking needs to be disabled to setup
// standard database users.
   if (Get_SqlParser_Flags(DISABLE_EXTERNAL_USERNAME_CHECK))
      return true;

// Verify that the external username is defined in the identity store.
DBUserAuth::CheckUserResult chkUserRslt = DBUserAuth::UserDoesNotExist;

   chkUserRslt = DBUserAuth::CheckExternalUsernameDefined(externalUsername,
                                                          configurationNumber,
                                                          foundConfigurationNumber);

// Username was found!
   if (chkUserRslt == DBUserAuth::UserExists)
      return true;

// Who?
   if (chkUserRslt == DBUserAuth::UserDoesNotExist)
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_LDAP_USER_NOT_FOUND)
                          << DgString0(externalUsername);
      return false;
   }

// Problem looking up the username.  Could be a bad configuration, a
// problem at the identity store, or communicating with the identity store.
   if (chkUserRslt == DBUserAuth::ErrorDuringCheck)
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_LDAP_COMM_ERROR);
      return false;
   }

   return false;

}
//---------------------- End of validateExternalUsername -----------------------

// -----------------------------------------------------------------------------
// public method:  describe
//
// This method returns the showddl text for the requested user in string format
//
// Input:
//   authName - name of user to describe
//
// Input/Output:  
//   authText - the REGISTER USER text
//
// returns result:
//    true -  successful
//    false - failed (ComDiags area will be set up with appropriate error)
//-----------------------------------------------------------------------------
bool CmpSeabaseDDLuser::describe (const NAString &authName, NAString &authText)
{
  // If current user matches authName, allow request
  NAString currentUserName (ComUser::getCurrentUsername());
  if ((currentUserName != authName) && !verifyAuthority(SQLOperation::SHOW))
  {
    // No authority.  We're outta here.
    *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
    return false;
  }

  CmpSeabaseDDLauth::AuthStatus retcode = getUserDetails(authName.data());
  // If the user was not found, set up an error
  if (retcode == STATUS_NOTFOUND)
  {
    *CmpCommon::diags() << DgSqlCode(-CAT_USER_NOT_EXIST)
                        << DgString0(authName.data());
    return false;
  }

  // If an error was detected, return
  if (retcode == STATUS_ERROR)
    return false;

  // Generate output text
  authText = "REGISTER USER \"";
  authText += getAuthExtName();
  if (getAuthExtName() != getAuthDbName())
  {
    authText += "\" AS \"";
    authText += getAuthDbName();
  }
  authText += "\";\n";

  if (!isAuthValid())
  {
    authText += "ALTER USER \"";
    authText += getAuthDbName();
    authText += "\" SET OFFLINE;\n";
  }
  return true;
}
//------------------------------ End of describe -------------------------------


// ****************************************************************************
// Class CmpSeabaseDDLrole methods
// ****************************************************************************

// ----------------------------------------------------------------------------
// Constructors
// ----------------------------------------------------------------------------
CmpSeabaseDDLrole::CmpSeabaseDDLrole(
   const NAString & systemCatalog,
   const NAString & MDSchema) 
: CmpSeabaseDDLauth(systemCatalog,MDSchema)
{}

CmpSeabaseDDLrole::CmpSeabaseDDLrole(const NAString & systemCatalog) 
: CmpSeabaseDDLauth()
{

   systemCatalog_ = systemCatalog;

   CONCAT_CATSCH(MDSchema_,systemCatalog.data(),SEABASE_MD_SCHEMA);

}

CmpSeabaseDDLrole::CmpSeabaseDDLrole() 
: CmpSeabaseDDLauth()
{}


// ----------------------------------------------------------------------------
// Public method: createRole
//
// Creates a role in the Trafodion metadata
//
// Input:  parse tree containing a definition of the role
// Output: the global diags area is set up with the result
// ----------------------------------------------------------------------------
void CmpSeabaseDDLrole::createRole(StmtDDLCreateRole * pNode)
   
{

   // Don't allow roles to be created unless authorization is enabled
   if (!CmpCommon::context()->isAuthorizationEnabled())
   {
     *CmpCommon::diags() << DgSqlCode(-CAT_AUTHORIZATION_NOT_ENABLED);
     return;
   }

   // Verify user is authorized to perform CREATE ROLE requests
   if (!verifyAuthority(SQLOperation::MANAGE_ROLES))
   {
     *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
     return;
   }

   // Set up a global try/catch loop to catch unexpected errors
   try
   {

      // Verify that the specified role name is not reserved
      setAuthDbName(pNode->getRoleName());
      if (isAuthNameReserved(getAuthDbName()))
      {
         *CmpCommon::diags() << DgSqlCode(-CAT_AUTH_NAME_RESERVED)
                             << DgString0(getAuthDbName().data());
         return;
      }

      // Verify that the name does not include unsupported special characters
      if (!isAuthNameValid(getAuthDbName()))
      {
         *CmpCommon::diags() << DgSqlCode (-CAT_INVALID_CHARS_IN_AUTH_NAME)
                             << DgString0(getAuthDbName().data());
         return;
      }

      // set up class members from parse node
      setAuthType(COM_ROLE_CLASS);  // we are a role
      setAuthValid(true); // assume a valid role

      Int64 createTime = NA_JulianTimestamp();
      setAuthCreateTime(createTime);
      setAuthRedefTime(createTime);  // make redef time the same as create time

      // Make sure role has not already been registered
      if (authExists(getAuthDbName(),false))
      {
         *CmpCommon::diags() << DgSqlCode(-CAT_AUTHID_ALREADY_EXISTS)
                             << DgString0(getAuthDbName().data());
         return;
      }
      
      // unexpected error occurred - ?
      if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) > 0)
         return;

      // Get a unique role ID number
      Int32 roleID = getUniqueAuthID(MIN_ROLEID, MAX_ROLEID); //TODO: add role support
      assert (isRoleID(roleID));
      setAuthID(roleID);
      
      std::string creatorUsername;

      // If the WITH ADMIN clause was specified, then create the role on behalf of the
      // authorization ID specified in this clause.
      // Need to translate the creator name to its authID
      if (pNode->getOwner() == NULL)
      {
         // get effective user from the Context
         Int32 userID = ComUser::getCurrentUser();
         setAuthCreator(userID);
         creatorUsername = ComUser::getCurrentUsername();
      }
      else
      {
         const NAString creatorName =
                pNode->getOwner()->getAuthorizationIdentifier();
         Int32 authID = NA_UserIdDefault;
         Int32 result = ComUser::getUserIDFromUserName(creatorName.data(),authID);
         if (result != 0)
         {
            *CmpCommon::diags() << DgSqlCode(-CAT_USER_NOT_EXIST)
                                << DgString0(creatorName.data());
            return;
         }
         
         // TODO: verify creator can create roles
         setAuthCreator(authID);
         creatorUsername = creatorName.data();
      }
      // For roles, external and database names are the same.
      setAuthExtName(getAuthDbName());
      // Add the role to AUTHS table
      insertRow();
      
      // Grant this role to the creator of the role if authorization is enabled.
      NAString privMgrMDLoc;

      CONCAT_CATSCH(privMgrMDLoc,systemCatalog_.data(),SEABASE_PRIVMGR_SCHEMA);
      
      PrivMgrRoles roles(std::string(MDSchema_.data()),std::string(privMgrMDLoc),
                         CmpCommon::diags());
      
      PrivStatus privStatus = roles.grantRoleToCreator(roleID,
                                                       getAuthDbName().data(),
                                                       getAuthCreator(),
                                                        creatorUsername);
      if (privStatus != PrivStatus::STATUS_GOOD)
      {
         SEABASEDDL_INTERNAL_ERROR("Unable to grant role to role administrator");
         return;
      }
   }
   catch (...)
   {
      // At this time, an error should be in the diags area.
      // If there is no error, set up an internal error
      if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
         SEABASEDDL_INTERNAL_ERROR("Switch statement in CmpSeabaseDDLrole::createRole");
   }
}


// ----------------------------------------------------------------------------
// Public method: createStandardRole
//
// Creates a standard role (ie. DB__nameROLE) in the Trafodion metadata
//
// Input:  
//    role name
//    role ID
//
// returns:  true - added role,
//           false - did not add role
// ----------------------------------------------------------------------------
bool CmpSeabaseDDLrole::createStandardRole(
   const std::string roleName,
   const int32_t roleID)
{
  setAuthType(COM_ROLE_CLASS);  // we are a role
  return createStandardAuth(roleName, roleID);
}


// -----------------------------------------------------------------------------
// public method:  describe
//
// This method returns the showddl text for the requested role in string format
//
// Input:
//   roleName - name of role to describe
//
// Input/Output:  
//   roleText - the CREATE ROLE (and GRANT ROLE) text
//
// returns result:
//    true -  successful
//    false - failed (ComDiags area will be set up with appropriate error)
//-----------------------------------------------------------------------------
bool CmpSeabaseDDLrole::describe(
   const NAString & roleName, 
   NAString &roleText)
   
{

   try
   {
      // Can current user perform request
      Int32 roleID = NA_UserIdDefault;
      if (ComUser::getAuthIDFromAuthName(roleName.data(), roleID) != 0)
        roleID = NA_UserIdDefault;

      if (!ComUser::currentUserHasRole(roleID) && !verifyAuthority(SQLOperation::SHOW))
      {
         // No authority.  We're outta here.
         *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
         return false;
      }

      CmpSeabaseDDLauth::AuthStatus retcode = getRoleDetails(roleName.data());
      
      // If the role was not found, set up an error
      if (retcode == STATUS_NOTFOUND)
      {
        *CmpCommon::diags() << DgSqlCode(-CAT_ROLE_NOT_EXIST)
                            << DgString0(roleName.data());
        return false;
      }

      // If an error was detected, throw an exception so the catch handler will 
      // put a value in ComDiags area in case no message exists
      if (retcode == STATUS_ERROR)
        return false;
 
      // Generate output text
      roleText = "CREATE ROLE \"";
      roleText += getAuthDbName();
      roleText += "\"";
    
      // If the role owner is not DB__ROOT, list the user who administers the role.
      if (getAuthCreator() != ComUser::getRootUserID())
      {
         roleText += " WITH ADMIN \"";
         
         char creatorName[MAX_DBUSERNAME_LEN + 1];
         int32_t length = 0;
         
         Int16 retCode = ComUser::getAuthNameFromAuthID(getAuthCreator(),creatorName,
                                                        sizeof(creatorName),length);
         if (retCode != 0)
         {
            SEABASEDDL_INTERNAL_ERROR("Role administrator not registered");
            UserException excp (NULL, 0);
            throw excp;
         }
         roleText += creatorName; 
         roleText += "\"";
      }
    
      roleText += ";\n";
      
      // See if authorization is enabled.  If so, need to list any grants of this
      // role.  Otherwise, we are outta here.
      if (!CmpCommon::context()->isAuthorizationEnabled())
         return true;

      NAString privMgrMDLoc;

      CONCAT_CATSCH(privMgrMDLoc,systemCatalog_.data(),SEABASE_PRIVMGR_SCHEMA);
      
      PrivMgrRoles roles(std::string(MDSchema_.data()),std::string(privMgrMDLoc),
                         CmpCommon::diags());
    
         
      std::vector<std::string> granteeNames;
      std::vector<int32_t> grantDepths;
      std::vector<int32_t> grantorIDs;


      PrivStatus privStatus = roles.fetchGranteesForRole(getAuthID() ,granteeNames,
                                                         grantorIDs,grantDepths);

      // If nobody was granted this role, nothing to do.                                     
      if (privStatus == PrivStatus::STATUS_NOTFOUND || granteeNames.size() == 0)
         return true;

      if (privStatus == PrivStatus::STATUS_ERROR)                                   
         SEABASEDDL_INTERNAL_ERROR("Could not fetch users granted role.");
         
      // If CQD to display privilege grants is off, return now
      if ((CmpCommon::getDefault(SHOWDDL_DISPLAY_PRIVILEGE_GRANTS) == DF_OFF) || 
          ((CmpCommon::getDefault(SHOWDDL_DISPLAY_PRIVILEGE_GRANTS) == DF_SYSTEM) 
              && getenv("SQLMX_REGRESS")))
        return true;

      // Report on each grantee.
      for (size_t r = 0; r < granteeNames.size(); r++)
      {
         // If the grantor is system, we want to show the grant, but exclude 
         // it from executing in a playback script.
         if (grantorIDs[r] == ComUser::getSystemUserID())
            roleText += "-- ";
         roleText += "GRANT ROLE \"";
         roleText += getAuthDbName();
         roleText += "\" TO \"";
         roleText += granteeNames[r].c_str();
         roleText += "\"";
         // Grant depth is either zero or non-zero for now.  If non-zero,
         // report WITH ADMIN OPTION.
         if (grantDepths[r] != 0)
            roleText += " WITH ADMIN OPTION";
            
         // If the grantor is not DB__ROOT or _SYSTEM, list the grantor.
         if (grantorIDs[r] != ComUser::getRootUserID() &&
             grantorIDs[r] != ComUser::getSystemUserID())
         {
            roleText += " GRANTED BY \"";
            char grantorName[MAX_DBUSERNAME_LEN + 1];
            int32_t length = 0;
            
            Int16 retCode = ComUser::getAuthNameFromAuthID(grantorIDs[r],
                                                           grantorName,
                                                           sizeof(grantorName),
                                                           length);
            if (retCode != 0)
            {
               SEABASEDDL_INTERNAL_ERROR("Role grantor not registered");
               UserException excp(NULL,0);
               throw excp;
            }
            
            roleText += grantorName; 
            roleText += "\"";
         }
         roleText += ";\n";
      }
  }

  catch (...)
  {
   // At this time, an error should be in the diags area.
   // If there is no error, set up an internal error
   if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
      SEABASEDDL_INTERNAL_ERROR("Switch statement in CmpSeabaseDDLrole::describe");

    return false;
  }

  return true;
}
//------------------------------ End of describe -------------------------------

// ----------------------------------------------------------------------------
// Public method: dropRole
//
// Drops a role from the Trafodion metadata
//
// Input:  parse tree containing a definition of the role
// Output: the global diags area is set up with the result
// ----------------------------------------------------------------------------
void CmpSeabaseDDLrole::dropRole(StmtDDLCreateRole * pNode)
        
{

// Set up a global try/catch loop to catch unexpected errors
   try
   {
      const NAString roleName(pNode->getRoleName());
      // Verify that the specified user name is not reserved
      setAuthDbName(roleName);
      if (isAuthNameReserved(getAuthDbName()))
      {
         *CmpCommon::diags() << DgSqlCode(-CAT_AUTH_NAME_RESERVED)
                             << DgString0(roleName.data());
         return;
      }
      
      // read role details from the AUTHS table
      CmpSeabaseDDLauth::AuthStatus retcode = getRoleDetails(roleName);
      if (retcode == STATUS_ERROR)
         return;
        
      if (retcode == STATUS_NOTFOUND)
      {
         *CmpCommon::diags() << DgSqlCode(-CAT_ROLE_NOT_EXIST)
                             << DgString0(roleName.data());
         return;
      }
      
      if (ComUser::getCurrentUser() != getAuthCreator())
      {
         // If the user does not have privilege, allow the drop if 
         //   the user has been granted an admin role and
         //   the role being dropped is not an admin role and
         //   the authCreator of the role being dropped matches the admin role
         if (verifyAuthority(SQLOperation::MANAGE_ROLES) == false)
         {
           *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
           return;
         }
      }

      NAString privMgrMDLoc;

      CONCAT_CATSCH(privMgrMDLoc,systemCatalog_.data(),SEABASE_PRIVMGR_SCHEMA);
      
      PrivMgrRoles role(std::string(MDSchema_.data()),std::string(privMgrMDLoc),
                        CmpCommon::diags());
      
      // If authorization is not enabled and a role has been defined, skip
      // looking for dependencies and just remove the role from auths.
      if (CmpCommon::context()->isAuthorizationEnabled())
      {
         //TODO: Could support a CASCADE option that would clean up
         // grants and dependent objects.
         
         // First, see if role has been granted
         bool roleIsGranted = role.isGranted(getAuthID(),true); 
         if (roleIsGranted)
         {
            *CmpCommon::diags() << DgSqlCode(-CAT_ROLE_IS_GRANTED_NO_DROP);
            return;
         }
         
         // Now see if the role has been granted any privileges.
         // TODO: could allow priv grants if no dependent objects.
         PrivMgr privMgr(std::string(privMgrMDLoc),CmpCommon::diags());
         std::vector<PrivClass> privClasses;
         
         privClasses.push_back(PrivClass::ALL);
         std::vector<int64_t> objectUIDs;

         if (privMgr.isAuthIDGrantedPrivs(getAuthID(),privClasses, objectUIDs))
         {
            NAString objectName = getObjectName(objectUIDs);
            if (objectName.length() > 0)
            {
              *CmpCommon::diags() << DgSqlCode(-CAT_ROLE_HAS_PRIVS_NO_DROP)
                                  << DgString0(roleName.data())
                                  << DgString1(objectName.data());
              return;
            }
         }

         // Role has not been granted and no privileges have been granted to
         // the role.  Remove the system grant.
         PrivStatus privStatus = role.revokeRoleFromCreator(getAuthID(),
                                                            getAuthCreator());

         if (privStatus != PrivStatus::STATUS_GOOD)
         {
            SEABASEDDL_INTERNAL_ERROR("Unable to remove grant to role administrator");
            return;
         }
      }
      
      // delete the row
      deleteRow(roleName);
   }
   catch (...)
   {
      // At this time, an error should be in the diags area.
      // If there is no error, set up an internal error
      if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
         SEABASEDDL_INTERNAL_ERROR("Switch statement in CmpSeabaseDDLrole::dropRole");
   }
}


// ----------------------------------------------------------------------------
// Public method: dropStandardRole
//
// Drops a standard role (ie. DB__nameROLE) from the Trafodion metadata
//
// Input:  role name
// ----------------------------------------------------------------------------
void CmpSeabaseDDLrole::dropStandardRole(const std::string roleName)

{

// Verify name is a standard name

size_t prefixLength = strlen(RESERVED_AUTH_NAME_PREFIX);

   if (roleName.size() <= prefixLength ||
       roleName.compare(0,prefixLength,RESERVED_AUTH_NAME_PREFIX) != 0)
   {
       *CmpCommon::diags() << DgSqlCode(-CAT_ROLE_NOT_EXIST)
                           << DgString0(roleName.c_str());
       return;
   }
   // delete the row
   deleteRow(roleName.c_str());

}


// ----------------------------------------------------------------------------
// public method: getRoleDetails
//
// Create the CmpSeabaseDDLuser class containing user details for the
// requested username
//
// Input:
//    userName - the database username to retrieve details for
//    isExternal -
//       true - the username is the external name (auth_ext_name)
//       false - the username is the database name (auth_db_name)
//
//  Output:
//    Returned parameter:
//       STATUS_GOOD: authorization details are populated:
//       STATUS_NOTFOUND: authorization details were not found
//       STATUS_WARNING: (not 100) warning was returned, diags area populated
//       STATUS_ERROR: error was returned, diags area populated
// ----------------------------------------------------------------------------
CmpSeabaseDDLauth::AuthStatus 
CmpSeabaseDDLrole::getRoleDetails(const char *pRoleName)
{
  CmpSeabaseDDLauth::AuthStatus retcode = getAuthDetails(pRoleName,false);
  if (retcode == STATUS_GOOD && !isRole())
  {
    *CmpCommon::diags() << DgSqlCode (-CAT_IS_NOT_A_ROLE)
                        << DgString0(getAuthDbName().data());
     retcode = STATUS_ERROR;
  }
  return retcode;
}


// ----------------------------------------------------------------------------
// Public method: getRoleIDFromRoleName
//
// Lookup a role name in the Trafodion metadata
//
// Input:  Role name to lookup
// Output: Role ID if role was found
//    true returned if role found
//   false returned if role not found
// ----------------------------------------------------------------------------
bool CmpSeabaseDDLrole::getRoleIDFromRoleName(
   const char * roleName,
   Int32 & roleID) 
   
{

CmpSeabaseDDLauth::AuthStatus authStatus = getAuthDetails(roleName,false);

    if (authStatus != STATUS_GOOD && authStatus != STATUS_WARNING)
       return false;
       
    if (getAuthType() != COM_ROLE_CLASS)
       return false;
      
    roleID = getAuthID();
    return true;
    
}

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
    setAuthID(PUBLIC_AUTH_ID);
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
// method:  getUniqueID
//
// This method is not valid for the base class
//
// Input:  none
//
// Output:  populates diag area, throws exception.
// ----------------------------------------------------------------------------
Int32 CmpSeabaseDDLauth::getUniqueID()
{

   SEABASEDDL_INTERNAL_ERROR("CmpSeabaseDDLauth::getUniqueID");
                          
UserException excp(NULL,0);

   throw excp;
   
   return 0;
    
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
           authName == "_SYSTEM"
           ||
           authName == "PUBLIC"
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
  str_sprintf(buf, "insert into %s.\"%s\".%s values (%d, '%s', '%s', '%s', %d, '%s', %Ld, %Ld, 0)",
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
void CmpSeabaseDDLauth::updateRow(const NAString &setClause)
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
  Lng32 cliRC = cliInterface.executeImmediate(buf, (char*)&rowCount, &len, NULL);

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
// method:  getUniqueID
//
// This method returns a unique user ID
//
// Input:  none
//
// Output:  returns a unique user ID
// ----------------------------------------------------------------------------
Int32 CmpSeabaseDDLuser::getUniqueID()
{
  Int32 newUserID = 0;
  char userIDString[MAX_AUTHID_AS_STRING_LEN];

  NAString whereClause ("where auth_id >= ");
  sprintf(userIDString,"%d",MIN_USERID);
  whereClause += userIDString;
  whereClause += " and auth_id < ";
  sprintf(userIDString, "%d", MAX_USERID);
  whereClause += userIDString;
 
  newUserID = selectMaxAuthID(whereClause);
  // DB__ROOT should always be registered as MIN_USERID.  Just in case ...
  if (newUserID == 0)
     newUserID = MIN_USERID + 1;
  else
     newUserID++;
  return newUserID;
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
    verifyAuthority();

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
    Int32 userID = getUniqueID();
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
       SEABASEDDL_INTERNAL_ERROR("Switch statement in CmpSeabaseDDLuser::registerUser");
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
    verifyAuthority();

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

    NAString whereClause(" WHERE AUTH_TYPE = 'R' AND AUTH_CREATOR = ");
    
    char authIDString[20];
    
    sprintf(authIDString,"%d",getAuthID());
    whereClause += authIDString;
    
    if (selectCount(whereClause) > 0)
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_NO_UNREG_USER_OWNS_ROLES);
      return;
    }
    
    // User does not own any roles, but may have been granted roles.
    NAString privMgrMDLoc;

    CONCAT_CATSCH(privMgrMDLoc,systemCatalog_.data(),SEABASE_PRIVMGR_SCHEMA);
    
    PrivMgrRoles role(std::string(MDSchema_.data()),
                      std::string(privMgrMDLoc.data()),
                      CmpCommon::diags());
    
    if (role.isAuthorizationEnabled() &&
        role.isUserGrantedAnyRole(getAuthID()))
    {
       *CmpCommon::diags() << DgSqlCode(-CAT_NO_UNREG_USER_GRANTED_ROLES);
       return;
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
    Lng32 cliRC = cliInterface.executeImmediate(buf, (char*)&rowCount, &len, NULL);
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
                
    PrivMgr privMgr(std::string(privMgrMDLoc),CmpCommon::diags());
    std::vector<PrivClass> privClasses;
    
    privClasses.push_back(PrivClass::ALL);
    
    if (privMgr.isAuthIDGrantedPrivs(getAuthID(),privClasses))
    {
       *CmpCommon::diags() << DgSqlCode(-CAT_NO_UNREG_USER_HAS_PRIVS);
       return;
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
    StmtDDLAlterUser::AlterUserCmdSubType cmdSubType = pNode->getAlterUserCmdSubType();
    verifyAuthority(cmdSubType == StmtDDLAlterUser::SET_EXTERNAL_NAME);

    // Verify that that user name being altered is not a reserved name.
    // Altering of external name for DB__ROOT is the exception.
    if (isAuthNameReserved(pNode->getDatabaseUsername()))
    {
      if (cmdSubType != StmtDDLAlterUser::SET_EXTERNAL_NAME ||
          !(pNode->getDatabaseUsername() == ComUser::getRootUserName()))
      {
        *CmpCommon::diags() << DgSqlCode(-CAT_AUTH_NAME_RESERVED)
                            << DgString0(pNode->getDatabaseUsername().data());
        return;
      }
    }

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

    // Process the requested operation
    NAString setClause("set ");
    switch (cmdSubType)
    {
       case StmtDDLAlterUser::SET_EXTERNAL_NAME:
       {
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
  try
  {
    CmpSeabaseDDLauth::AuthStatus retcode = getUserDetails(authName.data());
    
    // If the user was not found, set up an error
    if (retcode == STATUS_NOTFOUND)
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_USER_NOT_EXIST)
                          << DgString0(authName.data());
      return false;
    }

    // If an error was detected, throw an exception so the catch handler will 
    // put a value in ComDiags area in case no message exists
    if (retcode == STATUS_ERROR)
    {
      UserException excp (NULL, 0);
      throw excp;
    }
  
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
  }

  catch (...)
  {
   // At this time, an error should be in the diags area.
   // If there is no error, set up an internal error
   if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
      SEABASEDDL_INTERNAL_ERROR("Switch statement in CmpSeabaseDDLuser::describe");
    return false;
  }

  return true;
}
//------------------------------ End of describe -------------------------------

// ----------------------------------------------------------------------------
// method: verifyAuthority
//
// makes sure user has privilege to perform user operation
//
// Input: none
//
// Output:  an exception is generated if user does not have authority
// ----------------------------------------------------------------------------
void CmpSeabaseDDLuser::verifyAuthority(bool isRemapUser)

{

int32_t currentUser = ComUser::getCurrentUser();

// Root user has authority to manage users.
   if (currentUser == ComUser::getRootUserID())
      return;
      
// Verify authorization is enabled.  If not, no restrictions.
NAString systemCatalog = CmpSeabaseDDL::getSystemCatalogStatic();
std::string privMDLoc(systemCatalog.data());
  
   privMDLoc += std::string(".\"") +
                std::string(SEABASE_PRIVMGR_SCHEMA) +
                std::string("\"");
                
PrivMgrComponentPrivileges componentPrivileges(privMDLoc,CmpCommon::diags());

    if (!componentPrivileges.isAuthorizationEnabled())
       return;

// Authorization enabled.  See if non-root user has authority to manage users.       
   if (componentPrivileges.hasSQLPriv(currentUser,SQLOperation::MANAGE_USERS,true))
   {
      if (!isRemapUser)
         return; 
      if (componentPrivileges.hasSQLPriv(currentUser,SQLOperation::REMAP_USER,true))
         return;
   }   
           
// No authority.  We're outta here.
   *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
   UserException excp (NULL, 0);
   throw excp;

}

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

// Set up a global try/catch loop to catch unexpected errors
   try
   {
      // Verify user is authorized to perform CREATE ROLE requests
      verifyAuthority();

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
      Int32 roleID = getUniqueID(); //TODO: add role support
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
      
      if (roles.isAuthorizationEnabled())
      { 
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
// ----------------------------------------------------------------------------
void CmpSeabaseDDLrole::createStandardRole(
   const std::string roleName,
   const int32_t roleID)

{

// Verify name is a standard name

size_t prefixLength = strlen(RESERVED_AUTH_NAME_PREFIX);

   if (roleName.size() <= prefixLength ||
       roleName.compare(0,prefixLength,RESERVED_AUTH_NAME_PREFIX) != 0)
   {
       *CmpCommon::diags() << DgSqlCode(-CAT_ROLE_NOT_EXIST)
                           << DgString0(roleName.data());
       return;
   }

   setAuthDbName(roleName.c_str());
   setAuthExtName(roleName.c_str());
   setAuthType(COM_ROLE_CLASS);  // we are a role
   setAuthValid(true); // assume a valid role

   Int64 createTime = NA_JulianTimestamp();
   setAuthCreateTime(createTime);
   setAuthRedefTime(createTime);  // make redef time the same as create time

   // Make sure role has not already been registered
   if (authExists(getAuthDbName(),false))
      return;
   
   setAuthID(roleID);
   setAuthCreator(ComUser::getRootUserID());

// Add the role to AUTHS table
   insertRow();

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
      {
        UserException excp (NULL, 0);
        throw excp;
      }
    
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
      
      // See if authorization is enable.  If so, need to list any grants of this
      // role.  Otherwise, we are outta here.
      NAString privMgrMDLoc;

      CONCAT_CATSCH(privMgrMDLoc,systemCatalog_.data(),SEABASE_PRIVMGR_SCHEMA);
      
      PrivMgrRoles roles(std::string(MDSchema_.data()),std::string(privMgrMDLoc),
                         CmpCommon::diags());
    
      if (!roles.isAuthorizationEnabled())
         return true;
         
      std::vector<std::string> granteeNames;
      std::vector<int32_t> grantDepths;
      std::vector<int32_t> grantorIDs;
      
      PrivStatus privStatus = PrivStatus::STATUS_GOOD;
      
      privStatus = roles.fetchUsersForRole(getAuthID(),granteeNames,
                                           grantorIDs,grantDepths);
      
      // If no users were granted this role, nothing to do.                                     
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
      
      // Verify user is authorized to perform DROP ROLE requests
      if (ComUser::getCurrentUser() != getAuthCreator())
         verifyAuthority();
      
      NAString privMgrMDLoc;

      CONCAT_CATSCH(privMgrMDLoc,systemCatalog_.data(),SEABASE_PRIVMGR_SCHEMA);
      
      PrivMgrRoles role(std::string(MDSchema_.data()),std::string(privMgrMDLoc),
                        CmpCommon::diags());
      
      if (role.isAuthorizationEnabled())
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
         
         if (privMgr.isAuthIDGrantedPrivs(getAuthID(),privClasses))
         {
            *CmpCommon::diags() << DgSqlCode(-CAT_ROLE_HAS_PRIVS_NO_DROP);
            return;
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


// ----------------------------------------------------------------------------
// method:  getUniqueID
//
// This method returns a unique role ID
//
// Input:  none
//
// Output:  returns a unique role ID
// ----------------------------------------------------------------------------
Int32 CmpSeabaseDDLrole::getUniqueID()
{
  Int32 newRoleID = 0;
  char roleIDString[MAX_AUTHID_AS_STRING_LEN];

  NAString whereClause ("where auth_id >= ");
  sprintf(roleIDString,"%d",DB_ROOTROLE_ID);
  whereClause += roleIDString;

  newRoleID = selectMaxAuthID(whereClause);
  if (newRoleID == 0)
     newRoleID = DB_ROOTROLE_ID + 1;
  else
     newRoleID++;
  return newRoleID;
}

// ----------------------------------------------------------------------------
// method: verifyAuthority
//
// makes sure user has privilege to perform role operation
//
// Input: none
//
// Output:  an exception is generated if user does not have authority
// ----------------------------------------------------------------------------
void CmpSeabaseDDLrole::verifyAuthority()

{

int32_t currentUser = ComUser::getCurrentUser();

// Root user has authority to manage roles.
   if (currentUser == ComUser::getRootUserID())
      return;
      
// Verify authorization is enabled.  If not, no restrictions.
NAString systemCatalog = CmpSeabaseDDL::getSystemCatalogStatic();
std::string privMDLoc(systemCatalog.data());
  
   privMDLoc += std::string(".\"") +
                std::string(SEABASE_PRIVMGR_SCHEMA) +
                std::string("\"");
                
PrivMgrComponentPrivileges componentPrivileges(privMDLoc,CmpCommon::diags());

    if (!componentPrivileges.isAuthorizationEnabled())
       return;

// Authorization enabled.  See if non-root user has authority to manage roles.       
   if (componentPrivileges.hasSQLPriv(currentUser,SQLOperation::MANAGE_ROLES,true))
      return;   
       
// No authority.  We're outta here.
   *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
   UserException excp (NULL, 0);
   throw excp;

}


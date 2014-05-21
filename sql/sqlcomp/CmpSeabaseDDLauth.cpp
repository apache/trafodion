/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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
 * File:         CmpSeabaseDDLauth.cpp
 * Description:  Implements methods for user management
 *
 * Contains methods for classes:
 *   CmpSeabaseDDLauth
 *   CmpSeabaseDDLuser
 *
 *
 *****************************************************************************
 */

#include "StmtDDLRegisterUser.h"
#include "CmpSeabaseDDLauth.h"
#include "CmpSeabaseDDL.h"
#include "CmpSeabaseDDLincludes.h"
#include "ComSmallDefs.h"
#include "CompException.h"
#include "Globals.h"
#include "Context.h"
#include "dbUserAuth.h"
#include "ComUser.h"

#define  RESERVED_AUTH_NAME_PREFIX  "DB__"

inline static bool validateExternalUsername(
   const char *                                 externalUsername,
   DBUserAuth::AuthenticationConfiguration      configurationNumber,
   DBUserAuth::AuthenticationConfiguration &    foundConfigurationNumber);

// ****************************************************************************
// Class CmpSeabaseDDLauth methods
// ****************************************************************************

// ----------------------------------------------------------------------------
// Default constructor
// ----------------------------------------------------------------------------
CmpSeabaseDDLauth::CmpSeabaseDDLauth()
: authID_(NA_UserIdDefault),
  authType_(COM_UNKNOWN_ID_CLASS),
  authCreator_(NA_UserIdDefault),
  authCreateTime_(0),
  authRedefTime_(0),
  authValid_(true),
  authImmutable_(false)
{}

// ----------------------------------------------------------------------------
// method:  authExists
//
// Input: none
//
// Output:
//   Returns true if authorization row exists in the metadata
//   Returns false if authorization row does not exist in the metadata
//
//  An exception is thrown if any unexpected errors occurred.
// ----------------------------------------------------------------------------
bool  CmpSeabaseDDLauth::authExists (bool isExternal)
{
  // Read the auths table based on the auth_db_name
  NAString sysCat = CmpSeabaseDDL::getSystemCatalogStatic();
  NAString colName = (isExternal) ? "auth_ext_name" : "auth_db_name";
  NAString authName = (isExternal) ?  getAuthExtName() : getAuthDbName();
  char buf[1000];
  str_sprintf(buf, "select count(*) from %s.\"%s\".%s where %s = '%s' ",
                sysCat.data(), SEABASE_MD_SCHEMA, SEABASE_AUTHS, colName.data(),
                authName.data());

  Lng32 len = 0;
  Int64 rowCount = 0;
  ExeCliInterface cliInterface(STMTHEAP);
  Lng32 cliRC = cliInterface.executeImmediate(buf, (char*)&rowCount, &len, NULL);

  // If unexpected error occurred, return an exception
  if (cliRC < 0)
  {
    cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
    UserException excp (NULL, 0);
    excp.throwException();
  }

  return (rowCount > 0) ? true : false;
}

// ----------------------------------------------------------------------------
// method: getAuthDetails
//
// Creates the CmpSeabaseDDLauth class containing auth details for the
// requested authName
//
// Input:
//    authName - the database auth name to retrieve details for
//    isExternal -
//       true - the auth name is the external name (auth_ext_name)
//       false - the auth name is the database name (auth_db_name)
//
// Output:
//    A returned parameter:
//       0 - authorization details are available
//       < 0 - an error was returned trying to get details
//       100 - authorization details were not found
//       > 0 - (not 100) warning was returned
// ----------------------------------------------------------------------------
Int32 CmpSeabaseDDLauth::getAuthDetails(const char *pAuthName,
                                        bool isExternal)
{
  try
  {
    NAString sysCat = CmpSeabaseDDL::getSystemCatalogStatic();
    char buf[1000];
    NAString authNameCol = isExternal ? "auth_ext_name " : "auth_db_name ";
   str_sprintf(buf, "select auth_id, auth_db_name, auth_ext_name, auth_type, auth_creator, auth_is_valid, auth_create_time, auth_redef_time from %s.\"%s\".%s where %s = '%s' ",
              sysCat.data(), SEABASE_MD_SCHEMA, SEABASE_AUTHS, authNameCol.data(), pAuthName);
    NAString cmd (buf);

    if (selectExactRow(cmd))
      return 0;
    return 100;
  }
  catch (DDLException e)
  {
    return e.getSqlcode();
  }
  catch (...)
  {
    return -1;
  }
}

// ----------------------------------------------------------------------------
// method:  getAuthDetails
//
// Create the CmpSeabaseDDLauth class containing auth details for the
// request authID
//
// Input:
//    authID - the database authorization ID to search for
//
//  Output:
//    A returned parameter:
//       0 - authorization details are available
//       < 0 - an error was returned trying to get details
//       100 - authorization details were not found
//       > 0 - (not 100) warning was returned
// ----------------------------------------------------------------------------
Int32 CmpSeabaseDDLauth::getAuthDetails (Int32 authID)
{
  try
  {
    NAString sysCat = CmpSeabaseDDL::getSystemCatalogStatic();
    char buf[1000];
   str_sprintf(buf, "select auth_id, auth_db_name, auth_ext_name, auth_type, auth_creator, auth_is_valid, auth_create_time, auth_redef_time from %s.\"%s\".%s where auth_id = %s ",
              sysCat.data(), SEABASE_MD_SCHEMA, SEABASE_AUTHS, authID);
    NAString cmd (buf);

    if (selectExactRow(cmd))
      return 0;
    return 100;
  }
  catch (DDLException e)
  {
    return e.getSqlcode();
  }
  catch (...)
  {
    return -1;
  }
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
// method: verifyAuthority
//
// makes sure user has privilege to perform operation
//
// Input: none
//
// Output:  an exception is generated if user does not have authority
// ----------------------------------------------------------------------------
void CmpSeabaseDDLauth::verifyAuthority()
{
  // get effective user from the Context
  // TBD - replace this call with SQL_EXEC_ call
  const char *databaseUserName = GetCliGlobals()->currContext()->getDatabaseUserName();
  NAString userNameStr = databaseUserName;
  NAString rootNameStr = ComUser::getRootUserName();
  if (rootNameStr != userNameStr)
  {
    *CmpCommon::diags() << DgSqlCode (-CAT_NOT_AUTHORIZED);
    UserException excp (NULL, 0);
    excp.throwException();
  }
}

// ----------------------------------------------------------------------------
// Methods that access the AUTHS table
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
    excp.throwException();
  }
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
      authType = COM_UNKNOWN_ID_CLASS_LIT;
  }

  NAString authValid = isAuthValid() ? "Y" : "N";
  NAString immutable = isAuthImmutable() ? "Y" : "N";

  NAString sysCat = CmpSeabaseDDL::getSystemCatalogStatic();
  str_sprintf(buf, "insert into %s.\"%s\".%s values (%d, '%s', '%s', '%s', %d, '%s', %Ld, %Ld)",
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
    excp.throwException();
  }
}

// select exact
bool CmpSeabaseDDLauth::selectExactRow(const NAString & cmd)
{
  ExeCliInterface cliInterface(STMTHEAP);

  Int32 cliRC = cliInterface.fetchRowsPrologue(cmd.data(), true/*no exec*/);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      DDLException excp (cliRC, NULL, 0);
      excp.throwException();
    }

  cliRC = cliInterface.clearExecFetchClose(NULL, 0);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      DDLException excp (cliRC, NULL, 0);
      excp.throwException();
    }

  if (cliRC == 100) // did not find the row
    return false;

  char * ptr = NULL;
  Lng32 len = 0;
  char type [6];

  // value 1:  auth_id (int32)
  cliInterface.getPtrAndLen(1, ptr, len);
  setAuthID(*(Int32*)ptr);

  // value 2: auth_db_name (NAString)
  cliInterface.getPtrAndLen(2,ptr,len);
  NAString value(ptr);
  setAuthDbName(value);

  // value 3: auth_ext_name (NAString)
  cliInterface.getPtrAndLen(3,ptr,len);
  value = ptr;
  setAuthExtName(value);

  // value 4: auth_type (char)
  // str_cpy_and_null params: *tgt, *src, len, endchar, blank, null term
  cliInterface.getPtrAndLen(4,ptr,len);
  str_cpy_and_null(type, ptr, len, '\0', ' ', true);
  if ( type[0] == 'U')
    setAuthType(COM_USER_CLASS);
  else if (type[0] == 'R')
    setAuthType(COM_USER_CLASS);
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
  return true;
}

// ****************************************************************************
// Class CmpSeabaseDDLuser methods
// ****************************************************************************

// ----------------------------------------------------------------------------
// Default constructor
// ----------------------------------------------------------------------------
CmpSeabaseDDLuser::CmpSeabaseDDLuser()
: CmpSeabaseDDLauth()
{}

// ----------------------------------------------------------------------------
// method: getUserDetails
//
// Create the CmpSeabaseDDLuser class containing user details for the
// request userID
//
// Input:
//    userID - the database authorization ID to search for
//
//  Output:
//    A returned parameter:
//       0 - authorization details are available
//       < 0 - an error was returned trying to get details
//       100 - authorization details were not found
//       > 0 - (not 100) warning was returned
// ----------------------------------------------------------------------------
Int32 CmpSeabaseDDLuser::getUserDetails(Int32 userID)
{
  Int32 retcode = getAuthDetails(userID);
  if (retcode == 0)
  {
    if (!isUser())
      *CmpCommon::diags() << DgSqlCode (-CAT_IS_NOT_A_USER)
                          << DgString0 (getAuthDbName());
    return -CAT_IS_NOT_A_USER;
  }
  return retcode;
}

// ----------------------------------------------------------------------------
// method: getUserDetails
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
//    A returned parameter:
//       0 - authorization details are available
//       < 0 - an error was returned trying to get details
//       100 - authorization details were not found
//       > 0 - (not 100) warning was returned
// ----------------------------------------------------------------------------
Int32 CmpSeabaseDDLuser::getUserDetails(const char *pUserName,
                                        bool isExternal)
{
  Int32 retcode = getAuthDetails(pUserName, isExternal);
  if (retcode == 0)
  {
    if (!isUser())
    {
      *CmpCommon::diags() << DgSqlCode (-CAT_IS_NOT_A_USER)
                          << DgString0 (pUserName);
      return -CAT_IS_NOT_A_USER;
    }
  }
  return retcode;
}

// ----------------------------------------------------------------------------
// method:  getUniqueUserID
//
// This method returns a unique user ID
//
// Input:  none
//
// Output:  returns a unique user ID
//   An exception is generated, if a unique ID could not be generated
// ----------------------------------------------------------------------------
Int32 CmpSeabaseDDLuser::getUniqueUserID()
{
  NAString sysCat = CmpSeabaseDDL::getSystemCatalogStatic();
  char buf[400];
  str_sprintf(buf, "select max (auth_id) from %s.\"%s\".%s where auth_id >= 0 and auth_id < 1000000" , sysCat.data(), SEABASE_MD_SCHEMA, SEABASE_AUTHS);

  Lng32 len = 0;
  Int32 maxValue = 0;
  ExeCliInterface cliInterface(STMTHEAP);
  Lng32 cliRC = cliInterface.executeImmediate(buf, (char *)&maxValue, &len, true);
  if (cliRC != 0)
  {
    cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
    UserException excp (NULL, 0);
    excp.throwException();
  }

  maxValue++;
  return maxValue;
}

// ----------------------------------------------------------------------------
// Method: registerUser
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
    // Verify user is authorized

    verifyAuthority();

    // Verify that the specified user name is not reserved
    // TBD - add the isCatman concept
    if (isAuthNameReserved(pNode->getDbUserName()))
    {
      *CmpCommon::diags() << DgSqlCode (-CAT_AUTH_NAME_RESERVED)
                          << DgString0(pNode->getDbUserName().data());
      DDLException excp (CAT_AUTH_NAME_RESERVED, NULL, 0);
      excp.throwException();
    }


    // set up class members from parse node
    setAuthDbName(pNode->getDbUserName());
    setAuthExtName(pNode->getExternalUserName());
    setAuthImmutable(pNode->isImmutable());
    setAuthType(COM_USER_CLASS);  // we are a user
    setAuthValid(true); // assume a valid user

    Int64 createTime = NA_JulianTimestamp();
    setAuthCreateTime(createTime);
    setAuthRedefTime(createTime);  // make redef time the same as create time

    // Make sure db user has not already been registered
    if (authExists())
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_AUTHID_ALREADY_EXISTS)
                          << DgString0(getAuthDbName().data());
      DDLException excp (CAT_AUTHID_ALREADY_EXISTS, NULL, 0);
      excp.throwException();
    }

    // Make sure external user has not already been registered
    if (authExists(true))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_LDAP_USER_ALREADY_EXISTS)
                          << DgString0(getAuthExtName().data());
      DDLException excp (CAT_LDAP_USER_ALREADY_EXISTS, NULL, 0);
      excp.throwException();
    }

DBUserAuth::AuthenticationConfiguration configurationNumber = DBUserAuth::DefaultConfiguration;
DBUserAuth::AuthenticationConfiguration foundConfigurationNumber = DBUserAuth::DefaultConfiguration;

    if (!validateExternalUsername(pNode->getExternalUserName().data(),
                                  configurationNumber,
                                  foundConfigurationNumber))
       return;

    // Get a unique auth ID number
    // TBD - check a parserflag (or something) to add DB__ROOT
    Int32 userID = 0;
    if (getAuthDbName() == DB__ROOT)
      userID = SUPER_USER;
    else
      userID = getUniqueUserID();

    setAuthID (userID);

    // If the BY clause was specified, then register the user on behalf of the
    // authorization ID specified in this clause.
    // Need to translate the creator name to its authID
    if (pNode->getOwner() == NULL)
    {
      // get effective user from the Context
      // TBD - replace this call with SQL_EXEC_ call
      Int32 *pUserID = GetCliGlobals()->currContext()->getDatabaseUserID();
      setAuthCreator(*pUserID);
    }
    else
    {
      const NAString creatorName =
        pNode->getOwner()->getAuthorizationIdentifier();
      // TBD: get the authID for the creatorName
      // TBD: verify creator can register users
      setAuthCreator(NA_UserIdDefault);
    }

    // Add the user to AUTHS table
    insertRow();
  }
  catch (...)
  {
    // At this time, an error should be in the diags area.
    // If there is no error, set up an internal error
    Int32 errorIndex = CmpCommon::diags()->getNumber(DgSqlCode::ERROR_);
    if (errorIndex == 0)
      *CmpCommon::diags() << DgSqlCode (-CAT_INTERNAL_EXCEPTION_ERROR);
  }
}

// ----------------------------------------------------------------------------
// method:  unregisterUser
//
// This method removes a user from the database
//
// Input:  parse tree containing a definition of the user
// Output: the global diags area is set up with the result
// ----------------------------------------------------------------------------
void CmpSeabaseDDLuser::unregisterUser (StmtDDLRegisterUser * pNode)
{
  try
  {
    verifyAuthority();

    // CASCADE option not yet supported
    if (pNode->getDropBehavior() == COM_CASCADE_DROP_BEHAVIOR)
    {
      *CmpCommon::diags() << DgSqlCode (-CAT_ONLY_SUPPORTING_RESTRICT_DROP_BEHAVIOR);
       DDLException excp (CAT_ONLY_SUPPORTING_RESTRICT_DROP_BEHAVIOR, NULL, 0);
       excp.throwException();
    }

    // Verify that the specified user name is not reserved
    // TBD - add the isCatman concept
    if (isAuthNameReserved(pNode->getDbUserName()))
    {
      *CmpCommon::diags() << DgSqlCode (-CAT_AUTH_NAME_RESERVED)
                          << DgString0(pNode->getDbUserName().data());
       DDLException excp (CAT_AUTH_NAME_RESERVED, NULL, 0);
       excp.throwException();
    }

    // set up class members from parse node
    // Read the row from the AUTHS table
    const NAString dbUserName(pNode->getDbUserName());
    NAString sysCat = CmpSeabaseDDL::getSystemCatalogStatic();
    char buf[1000];
    str_sprintf(buf, "select auth_id, auth_db_name, auth_ext_name, auth_type, auth_creator, auth_is_valid, auth_create_time, auth_redef_time from %s.\"%s\".%s where auth_db_name = '%s' ",
              sysCat.data(), SEABASE_MD_SCHEMA, SEABASE_AUTHS, dbUserName.data());

    NAString cmd (buf);
    if (!selectExactRow(cmd))
    {
      // TBD - add CQD to return okay if user has already been unregistered
      *CmpCommon::diags() << DgSqlCode(-CAT_USER_NOT_EXIST)
                          << DgString0(dbUserName.data());
       DDLException excp (CAT_USER_NOT_EXIST, NULL, 0);
       excp.throwException();
    }

    // Cannot unregister immutable users
    if (isAuthImmutable())
    {
      *CmpCommon::diags() << DgSqlCode(-1387);
       DDLException excp (1387, NULL, 0);
       excp.throwException();
    }

    // TBD, check to see if the user owns anything before removing

    // delete the row
    deleteRow(getAuthDbName());
  }
  catch (...)
  {
    // At this time, an error should be in the diags area.
    // If there is no error, set up an internal error
    Int32 errorIndex = CmpCommon::diags()->getNumber(DgSqlCode::ERROR_);
    if (errorIndex == 0)
      *CmpCommon::diags() << DgSqlCode (-CAT_INTERNAL_EXCEPTION_ERROR);
  }
}


// *****************************************************************************
// *                                                                           *
// * Function: validateExternalUsername                                        *
// *                                                                           *
// *    Determines if an external username is valid.                           *
// *                                                                           *
// *****************************************************************************
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
// *****************************************************************************
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
//********************** End of validateExternalUsername ***********************


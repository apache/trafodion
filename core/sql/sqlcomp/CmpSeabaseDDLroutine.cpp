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
 * File:         CmpSeabaseDDLroutine.cpp
 * Description:  Implements ddl views for SQL/seabase tables.
 *
 *
 * Created:     3/14/2014
 * Language:     C++
 *
 *
 *****************************************************************************
 */

#define   SQLPARSERGLOBALS_FLAGS	// must precede all #include's
#define   SQLPARSERGLOBALS_NADEFAULTS

#include "ComObjectName.h"
#include "ComUser.h"
#include "CmpSeabaseDDLroutine.h"

#include "StmtDDLCreateRoutine.h"
#include "StmtDDLDropRoutine.h"
#include "StmtDDLCreateLibrary.h"
#include "StmtDDLDropLibrary.h"
#include "StmtDDLAlterLibrary.h"

#include "ElemDDLColDefArray.h"
#include "ElemDDLColRefArray.h"
#include "ElemDDLParamDefArray.h"
#include "ElemDDLParamDef.h"

#include "SchemaDB.h"
#include "CmpSeabaseDDL.h"

#include "ExpHbaseInterface.h"

#include "ExExeUtilCli.h"
#include "Generator.h"
#include "ComSmallDefs.h"
#include "CmpDDLCatErrorCodes.h"

#include "PrivMgrComponentPrivileges.h"
#include "PrivMgrCommands.h"
#include "ComUser.h"

#include "NumericType.h"
#include "DatetimeType.h" 
#include "LmJavaSignature.h"

#include "ComCextdecs.h"
#include <sys/stat.h>


// *****************************************************************************
// *                                                                           *
// * Function: validateLibraryFileExists                                       *
// *                                                                           *
// *    Determines if a library file exists, and if not, reports an error.     *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <libraryFilename>               const ComString &               In       *
// *    is the file whose existence is to be validated.                        *
// *                                                                           *
// *  <isSystemObject>                bool                            In       *
// *    if true, indicates the filename should be prepended with the value of  *
// *  TRAF_HOME before validating existence.                                   *
// *                                                                            *
// *****************************************************************************
static int validateLibraryFileExists(
   const NAString    &libraryFilename,
   bool                isSystemObject)
   
{
      
  NAString completeLibraryFilename(libraryFilename);
  
  if (isSystemObject) {
    completeLibraryFilename.insert(0,'/');
    completeLibraryFilename.insert(0,getenv("TRAF_HOME"));
  }
  else
    if (CmpCommon::getDefault(CAT_LIBRARY_PATH_RELATIVE) == DF_ON)
      completeLibraryFilename.insert(0,getenv("MY_UDR_ROOT"));
   
  char *libraryFilenameString = convertNAString(completeLibraryFilename,STMTHEAP);
  struct stat sts;

  if ((stat(libraryFilenameString,&sts)) == -1 && errno == ENOENT) {
    *CmpCommon::diags() << DgSqlCode(-1382)
                        << DgString0(libraryFilename);
    return 1;
  }
 
  return 0;
}
//********************* End of validateLibraryFileExists ***********************

static void getRoutineTypeLit(ComRoutineType val, NAString& result)
  {
    if (val == COM_PROCEDURE_TYPE)
      result = COM_PROCEDURE_TYPE_LIT;
    else if (val == COM_SCALAR_UDF_TYPE)
      result = COM_SCALAR_UDF_TYPE_LIT;
    else if (val == COM_TABLE_UDF_TYPE)
      result = COM_TABLE_UDF_TYPE_LIT;
    else
      result = COM_UNKNOWN_ROUTINE_TYPE_LIT;
  }
 
static void getLanguageTypeLit(ComRoutineLanguage val, NAString& result)
  {
    if (val == COM_LANGUAGE_JAVA)
      result = COM_LANGUAGE_JAVA_LIT;
    else if (val == COM_LANGUAGE_C)
      result = COM_LANGUAGE_C_LIT;
    else if (val == COM_LANGUAGE_CPP)
      result = COM_LANGUAGE_CPP_LIT;
    else if (val == COM_LANGUAGE_SQL)
      result = COM_LANGUAGE_SQL_LIT;
    else
      result = COM_UNKNOWN_ROUTINE_LANGUAGE_LIT;
  }
 
static void getSqlAccessLit(ComRoutineSQLAccess val, NAString& result)
  {
    if (val == COM_NO_SQL)
      result = COM_NO_SQL_LIT;
    else if (val == COM_CONTAINS_SQL)
      result = COM_CONTAINS_SQL_LIT;
    else if (val == COM_READS_SQL)
      result = COM_READS_SQL_LIT;
    else if (val == COM_MODIFIES_SQL)
      result = COM_MODIFIES_SQL_LIT;
    else 
      result = COM_UNKNOWN_ROUTINE_SQL_ACCESS_LIT;
  }

static void getParamStyleLit(ComRoutineParamStyle val, NAString& result)
  {
    if (val == COM_STYLE_GENERAL)
      result = COM_STYLE_GENERAL_LIT;
    else if (val == COM_STYLE_JAVA_CALL)
      result = COM_STYLE_JAVA_CALL_LIT;
    else if (val == COM_STYLE_JAVA_OBJ)
      result = COM_STYLE_JAVA_OBJ_LIT;
    else if (val == COM_STYLE_SQL)
      result = COM_STYLE_SQL_LIT;
    else if (val == COM_STYLE_SQLROW)
      result = COM_STYLE_SQLROW_LIT;
     else if (val == COM_STYLE_SQLROW_TM)
      result = COM_STYLE_SQLROW_TM_LIT;
     else if (val == COM_STYLE_CPP_OBJ)
      result = COM_STYLE_CPP_OBJ_LIT;
    else 
      result = COM_UNKNOWN_ROUTINE_PARAM_STYLE_LIT;
  }
 
static void getTransAttributesLit(ComRoutineTransactionAttributes val, NAString& result)
  {
    if (val == COM_NO_TRANSACTION_REQUIRED)
      result = COM_NO_TRANSACTION_REQUIRED_LIT;
    else if (val == COM_TRANSACTION_REQUIRED)
      result = COM_TRANSACTION_REQUIRED_LIT;
    else 
      result = COM_UNKNOWN_ROUTINE_TRANSACTION_ATTRIBUTE_LIT;
  }

static void getParallelismLit(ComRoutineParallelism val, NAString& result)
  {
    if (val == COM_ROUTINE_NO_PARALLELISM)
      result = COM_ROUTINE_NO_PARALLELISM_LIT;
    else 
      result = COM_ROUTINE_ANY_PARALLELISM_LIT;
  }
 
static void getExternalSecurityLit(ComRoutineExternalSecurity val, NAString& result)
  {
    if (val == COM_ROUTINE_EXTERNAL_SECURITY_DEFINER)
      result = COM_ROUTINE_EXTERNAL_SECURITY_DEFINER_LIT;
    else if (val == COM_ROUTINE_EXTERNAL_SECURITY_IMPLEMENTATION_DEFINED)
      result = COM_ROUTINE_EXTERNAL_SECURITY_IMPLEMENTATION_DEFINED_LIT;
    else
      result = COM_ROUTINE_EXTERNAL_SECURITY_INVOKER_LIT ; // the default
  }

static void getExecutionModeLit(ComRoutineExecutionMode val, NAString& result)
  {
      if (val == COM_ROUTINE_SAFE_EXECUTION)
        result = COM_ROUTINE_SAFE_EXECUTION_LIT;
      else
        result = COM_ROUTINE_FAST_EXECUTION_LIT;
  }

short CmpSeabaseDDL::getUsingRoutines(ExeCliInterface *cliInterface,
                                     Int64 objUID,
                                     Queue * & usingRoutinesQueue)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  char buf[4000];
  str_sprintf(buf, "select trim(catalog_name) || '.' || trim(schema_name) || '.' || trim(object_name), "
                   "object_type, object_uid from %s.\"%s\".%s T, %s.\"%s\".%s LU "
                   "where LU.using_library_uid = %ld and "
                   "T.object_uid = LU.used_udr_uid  and T.valid_def = 'Y' ",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_LIBRARIES_USAGE,
              objUID);

  usingRoutinesQueue = NULL;
  cliRC = cliInterface->fetchAllRows(usingRoutinesQueue, buf, 0, 
                                     FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return cliRC;
    }
 
  if (usingRoutinesQueue->numEntries() == 0)
    return 100;


  return 0;
}


void CmpSeabaseDDL::createSeabaseLibrary(
				      StmtDDLCreateLibrary * createLibraryNode,
				      NAString &currCatName, 
                                      NAString &currSchName)
{
  Lng32 retcode = 0;
 
  ComObjectName libraryName(createLibraryNode->getLibraryName());
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  libraryName.applyDefaults(currCatAnsiName, currSchAnsiName);
  const NAString catalogNamePart = 
    libraryName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = 
    libraryName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = 
    libraryName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extLibraryName = libraryName.getExternalName(TRUE);
  const NAString extNameForHbase = catalogNamePart + "." + schemaNamePart + 
    "." + objectNamePart;
  
  // Verify that the requester has MANAGE_LIBRARY privilege.
  if (isAuthorizationEnabled() && !ComUser::isRootUserID())
    {
      NAString privMgrMDLoc;
      CONCAT_CATSCH(privMgrMDLoc, getSystemCatalog(), SEABASE_PRIVMGR_SCHEMA);

      PrivMgrComponentPrivileges componentPrivileges(std::string(privMgrMDLoc.data()),CmpCommon::diags());

      if (!componentPrivileges.hasSQLPriv
            (ComUser::getCurrentUser(),SQLOperation::MANAGE_LIBRARY,true))
      {
         *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
         processReturn ();
         return;
      }
    }

  // Check to see if user has the authority to create the library
  ExeCliInterface cliInterface(STMTHEAP, 0, NULL,
    CmpCommon::context()->sqlSession()->getParentQid());
  Int32 objectOwnerID = SUPER_USER;
  Int32 schemaOwnerID = SUPER_USER;
  ComSchemaClass schemaClass;

  retcode = verifyDDLCreateOperationAuthorized(&cliInterface,
                                               SQLOperation::CREATE_LIBRARY,
                                               catalogNamePart,
                                               schemaNamePart,
                                               schemaClass,
                                               objectOwnerID,
                                               schemaOwnerID);
  if (retcode != 0)
  {
     handleDDLCreateAuthorizationError(retcode,catalogNamePart,schemaNamePart);
     return;
  }
     
  ExpHbaseInterface * ehi = NULL;

  ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();
      return;
    }

  retcode = existsInSeabaseMDTable(&cliInterface, 
				   catalogNamePart, schemaNamePart, 
                                   objectNamePart, COM_LIBRARY_OBJECT, 
                                   TRUE, FALSE);
  if (retcode < 0)
    {
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  if (retcode == 1) // already exists
    {
      *CmpCommon::diags() << DgSqlCode(-1390)
			  << DgString0(extLibraryName);
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  NAString libFileName = createLibraryNode->getFilename() ;
  // strip blank spaces
  libFileName = libFileName.strip(NAString::both, ' ');
  if (validateLibraryFileExists(libFileName, FALSE))
    {
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  ComTdbVirtTableTableInfo * tableInfo = new(STMTHEAP) ComTdbVirtTableTableInfo[1];
  tableInfo->tableName = NULL,
  tableInfo->createTime = 0;
  tableInfo->redefTime = 0;
  tableInfo->objUID = 0;
  tableInfo->objOwnerID = objectOwnerID;
  tableInfo->schemaOwnerID = schemaOwnerID;
  tableInfo->isAudited = 1;
  tableInfo->validDef = 1;
  tableInfo->hbaseCreateOptions = NULL;
  tableInfo->numSaltPartns = 0;
  tableInfo->rowFormat = COM_UNKNOWN_FORMAT_TYPE;
  tableInfo->objectFlags = 0;
  
  Int64 objUID = -1;
  if (updateSeabaseMDTable(&cliInterface, 
			   catalogNamePart, schemaNamePart, objectNamePart,
			   COM_LIBRARY_OBJECT,
			   "N",
			   tableInfo,
			   0,
			   NULL,
			   0,			       
			   NULL,
			   0, NULL,
                           objUID))
    {
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  if (objUID == -1)
    {
      deallocEHI(ehi); 
      processReturn();
      return;
    }
 
  char * query = new(STMTHEAP) char[1000];
  str_sprintf(query, "insert into %s.\"%s\".%s values (%ld, '%s', %d, 0)",
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_LIBRARIES,
	      objUID,
              libFileName.data(),
              createLibraryNode->getVersion());
  
  Lng32 cliRC = cliInterface.executeImmediate(query);

  NADELETEBASIC(query, STMTHEAP);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      processReturn();
      return;
    }

  // hope to remove this call soon by setting thevalid flag to Y sooner
  if (updateObjectValidDef(&cliInterface, 
			   catalogNamePart, schemaNamePart, objectNamePart,
			   COM_LIBRARY_OBJECT_LIT,
			   "Y"))
    {
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  processReturn();

  return;
}

void CmpSeabaseDDL::dropSeabaseLibrary(StmtDDLDropLibrary * dropLibraryNode,
                                       NAString &currCatName, 
                                       NAString &currSchName)
{
  Lng32 cliRC = 0;
  Lng32 retcode = 0;

  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  NARoutineDB *pRoutineDBCache  = ActiveSchemaDB()->getNARoutineDB();
  const NAString &objName = dropLibraryNode->getLibraryName();

  ComObjectName libraryName(objName);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  libraryName.applyDefaults(currCatAnsiName, currSchAnsiName);

  const NAString catalogNamePart = libraryName.
    getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = libraryName.
    getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = libraryName.
    getObjectNamePartAsAnsiString(TRUE);
  const NAString extLibraryName = libraryName.getExternalName(TRUE);

  ExeCliInterface cliInterface(STMTHEAP, 0, NULL, 
    CmpCommon::context()->sqlSession()->getParentQid());

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    return;

  retcode = existsInSeabaseMDTable(&cliInterface, 
				   catalogNamePart, schemaNamePart, 
                                   objectNamePart,
				   COM_LIBRARY_OBJECT, TRUE, FALSE);
  if (retcode < 0)
    {
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  if (retcode == 0) // does not exist
    {
      *CmpCommon::diags() << DgSqlCode(-1389)
			  << DgString0(extLibraryName);
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  Int32 objectOwnerID = 0;
  Int32 schemaOwnerID = 0;
  Int64 objectFlags = 0;
  Int64 objUID = getObjectInfo(&cliInterface,
			      catalogNamePart.data(), schemaNamePart.data(), 
			      objectNamePart.data(), COM_LIBRARY_OBJECT,
                              objectOwnerID,schemaOwnerID,objectFlags);
  if (objUID < 0 || objectOwnerID == 0 || schemaOwnerID == 0)
    {
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  if (!isDDLOperationAuthorized(SQLOperation::DROP_LIBRARY,
                                objectOwnerID,
                                schemaOwnerID))
  {
     *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
     processReturn ();
     return;
  }
  
  Queue * usingRoutinesQueue = NULL;
  cliRC = getUsingRoutines(&cliInterface, objUID, usingRoutinesQueue);
  if (cliRC < 0)
    {
      deallocEHI(ehi); 
      processReturn();
      return;
    }
  // If RESTRICT and the library is being used, return an error
  if (cliRC != 100 && dropLibraryNode->getDropBehavior() == COM_RESTRICT_DROP_BEHAVIOR) 
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_DEPENDENT_ROUTINES_EXIST);

      deallocEHI(ehi); 
      processReturn();
      return;
    }
    
  usingRoutinesQueue->position();
  for (size_t i = 0; i < usingRoutinesQueue->numEntries(); i++)
  { 
     OutputInfo * rou = (OutputInfo*)usingRoutinesQueue->getNext(); 
     
     char * routineName = rou->get(0);
     ComObjectType objectType = PrivMgr::ObjectLitToEnum(rou->get(1));

     if (dropSeabaseObject(ehi, routineName,
                           currCatName, currSchName, objectType,
                           dropLibraryNode->ddlXns(),
                           TRUE, FALSE))
     {
       deallocEHI(ehi); 
       processReturn();
       return;
     }

     // Remove routine from DBRoutinCache
     ComObjectName objectName(routineName);
     QualifiedName qualRoutineName(objectName, STMTHEAP);
     NARoutineDBKey key(qualRoutineName, STMTHEAP);
     NARoutine *cachedNARoutine = pRoutineDBCache->get(&bindWA, &key);

     if (cachedNARoutine)
     {
       Int64 routineUID = *(Int64*)rou->get(2);
       pRoutineDBCache->removeNARoutine(qualRoutineName,
                                        ComQiScope::REMOVE_FROM_ALL_USERS,
                                        routineUID,
                                        dropLibraryNode->ddlXns(), FALSE);
     }

   }
 
  // can get a slight perf. gain if we pass in objUID
  if (dropSeabaseObject(ehi, objName,
                        currCatName, currSchName, COM_LIBRARY_OBJECT,
                        dropLibraryNode->ddlXns(),
                        TRUE, FALSE))
    {
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  deallocEHI(ehi);      
  processReturn();
  return;
}

void  CmpSeabaseDDL::alterSeabaseLibrary(StmtDDLAlterLibrary  *alterLibraryNode,
					 NAString &currCatName, 
					 NAString &currSchName)
{
  Lng32 cliRC;
  Lng32 retcode;
  
  NAString libraryName = alterLibraryNode->getLibraryName();
  NAString libFileName = alterLibraryNode->getFilename();
  
  ComObjectName libName(libraryName, COM_TABLE_NAME);
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  libName.applyDefaults(currCatAnsiName, currSchAnsiName);
  
  NAString catalogNamePart = libName.getCatalogNamePartAsAnsiString();
  NAString schemaNamePart = libName.getSchemaNamePartAsAnsiString(TRUE);
  NAString libNamePart = libName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extLibName = libName.getExternalName(TRUE);
  
  ExeCliInterface cliInterface(STMTHEAP, 0, NULL,
			       CmpCommon::context()->sqlSession()->getParentQid());
  
  retcode = existsInSeabaseMDTable(&cliInterface, 
				   catalogNamePart, schemaNamePart, libNamePart,
				   COM_LIBRARY_OBJECT, TRUE, FALSE);
  if (retcode < 0)
    {
      processReturn();
      return;
    }
  
  if (retcode == 0) // does not exist
    {
      CmpCommon::diags()->clear();
      *CmpCommon::diags() << DgSqlCode(-1389)
			  << DgString0(extLibName);
      processReturn();
      return;
    }
  
  // strip blank spaces
  libFileName = libFileName.strip(NAString::both, ' ');
  if (validateLibraryFileExists(libFileName, FALSE))
    {
      processReturn();
      return;
    }
  
  Int32 objectOwnerID = 0;
  Int32 schemaOwnerID = 0;
  Int64 objectFlags = 0;
  Int64 libUID = getObjectInfo(&cliInterface,
			       catalogNamePart.data(), schemaNamePart.data(),
			       libNamePart.data(), COM_LIBRARY_OBJECT,
			       objectOwnerID,schemaOwnerID,objectFlags);
  
  // Check for error getting metadata information
  if (libUID == -1 || objectOwnerID == 0)
    {
      if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
	SEABASEDDL_INTERNAL_ERROR("getting object UID and owner for alter library");
      processReturn();
      return;
    }
  
  // Verify that the current user has authority to perform operation
  if (!isDDLOperationAuthorized(SQLOperation::ALTER_LIBRARY,
				objectOwnerID,schemaOwnerID))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
      processReturn();
      return;
    }
  
  Int64 redefTime = NA_JulianTimestamp();
  
  char buf[2048]; // filename max length is 512. Additional bytes for long
  // library names.
  str_sprintf(buf, "update %s.\"%s\".%s set library_filename = '%s' where library_uid = %ld",
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_LIBRARIES,
	      libFileName.data(),
	      libUID);
  
  cliRC = cliInterface.executeImmediate(buf);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return;
    }
  
  if (updateObjectRedefTime(&cliInterface,
			    catalogNamePart, schemaNamePart, libNamePart,
			    COM_LIBRARY_OBJECT_LIT,
			    redefTime))
    {
      processReturn();
      return;
   }
  
  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  NARoutineDB *pRoutineDBCache  = ActiveSchemaDB()->getNARoutineDB();
  Queue * usingRoutinesQueue = NULL;
  cliRC = getUsingRoutines(&cliInterface, libUID, usingRoutinesQueue);
  if (cliRC < 0)
    {
      processReturn();
      return;
    }
  usingRoutinesQueue->position();
  for (size_t i = 0; i < usingRoutinesQueue->numEntries(); i++)
    { 
      OutputInfo * rou = (OutputInfo*)usingRoutinesQueue->getNext();    
      char * routineName = rou->get(0);
      ComObjectType objectType = PrivMgr::ObjectLitToEnum(rou->get(1));
      // Remove routine from DBRoutinCache
      ComObjectName objectName(routineName);
      QualifiedName qualRoutineName(objectName, STMTHEAP);
      NARoutineDBKey key(qualRoutineName, STMTHEAP);
      NARoutine *cachedNARoutine = pRoutineDBCache->get(&bindWA, &key);
      if (cachedNARoutine)
	{
	  Int64 routineUID = *(Int64*)rou->get(2);
	  pRoutineDBCache->removeNARoutine(qualRoutineName,
					   ComQiScope::REMOVE_FROM_ALL_USERS,
					   routineUID,
					   alterLibraryNode->ddlXns(), FALSE);
	}
    }
  
  return;
}

void CmpSeabaseDDL::createSeabaseRoutine(
				      StmtDDLCreateRoutine * createRoutineNode,
				      NAString &currCatName, 
                                      NAString &currSchName)
{
  Lng32 retcode = 0;
 
  ComObjectName routineName(createRoutineNode->getRoutineName());
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  routineName.applyDefaults(currCatAnsiName, currSchAnsiName);
  const NAString catalogNamePart = 
    routineName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = 
    routineName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = 
    routineName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extRoutineName = routineName.getExternalName(TRUE);
  ComRoutineType rType          = createRoutineNode->getRoutineType();
  ComRoutineLanguage language   = createRoutineNode->getLanguageType();
  ComRoutineParamStyle ddlStyle = createRoutineNode->getParamStyle();
  ComRoutineParamStyle style    = ddlStyle;
  NABoolean isJava              = (language == COM_LANGUAGE_JAVA);

  // Check to see if user has the authority to create the routine
  ExeCliInterface cliInterface(STMTHEAP, 0, NULL, 
    CmpCommon::context()->sqlSession()->getParentQid());
  Int32 objectOwnerID = SUPER_USER;
  Int32 schemaOwnerID = SUPER_USER;
  ComSchemaClass schemaClass;

  retcode = verifyDDLCreateOperationAuthorized(&cliInterface,
                                               SQLOperation::CREATE_ROUTINE,
                                               catalogNamePart,
                                               schemaNamePart,
                                               schemaClass,
                                               objectOwnerID,
                                               schemaOwnerID);
  if (retcode != 0)
  {
     handleDDLCreateAuthorizationError(retcode,catalogNamePart,schemaNamePart);
     return;
  }
  
  ExpHbaseInterface * ehi = NULL;

  ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();
      return;
    }
  
  retcode = existsInSeabaseMDTable(&cliInterface, 
				   catalogNamePart, schemaNamePart, 
                                   objectNamePart, COM_USER_DEFINED_ROUTINE_OBJECT, 
                                   TRUE, FALSE);
  if (retcode < 0)
    {
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  if (retcode == 1) // already exists
    {
      if (! createRoutineNode->createIfNotExists())
        *CmpCommon::diags() << DgSqlCode(-1390)
  			    << DgString0(extRoutineName);
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  
  ComObjectName libName(createRoutineNode->
                        getLibraryName().getQualifiedNameAsAnsiString());
  libName.applyDefaults(currCatAnsiName, currSchAnsiName);	
  NAString libCatNamePart = libName.getCatalogNamePartAsAnsiString();
  NAString libSchNamePart = libName.getSchemaNamePartAsAnsiString(TRUE);
  NAString libObjNamePart = libName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extLibraryName = libName.getExternalName(TRUE);
  char externalPath[512] ;
  Lng32 cliRC = 0;
	
  // this call needs to change
  Int64 libUID = getObjectUID(&cliInterface, 
                              libCatNamePart, 
                              libSchNamePart, 
                              libObjNamePart,
                              COM_LIBRARY_OBJECT_LIT);

  if (libUID < 0)
    {
      processReturn();    
      return;
    }

  if (libUID == 0) // does not exist
    {
      *CmpCommon::diags() << DgSqlCode(-1361)
			  << DgString0(extLibraryName);
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  // read the library path name from the LIBRARIES metadata table

  char * buf = new(STMTHEAP) char[200];
  str_sprintf(buf, "select library_filename from %s.\"%s\".%s"
              " where library_uid = %ld for read uncommitted access",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_LIBRARIES, libUID);

  cliRC = cliInterface.fetchRowsPrologue(buf, TRUE/*no exec*/);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  cliRC = cliInterface.clearExecFetchClose(NULL, 0);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      deallocEHI(ehi); 
      processReturn();
      return;
    }
  if (cliRC == 100) // did not find the row
    {
      *CmpCommon::diags() << DgSqlCode(-1231)
                          << DgString0(extRoutineName);
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  char * ptr = NULL;
  Lng32 len = 0;
  cliInterface.getPtrAndLen(1, ptr, len);
  str_cpy_all(externalPath, ptr, len);
  externalPath[len] = '\0'; 

  // determine language and parameter styles based on the library
  // type, unless already specified
  if (!createRoutineNode->isLanguageTypeSpecified())
    {
      NAString extPath(externalPath);
      size_t lastDot = extPath.last('.');
      NAString libSuffix;

      if (lastDot != NA_NPOS)
        libSuffix = extPath(lastDot,extPath.length()-lastDot);

      libSuffix.toUpper();

      if (libSuffix == ".JAR")
        {
          isJava = TRUE;
          language = COM_LANGUAGE_JAVA;
        }
      else if (libSuffix == ".SO" ||
               libSuffix == ".DLL")
        {
          // a known C/C++ library, set
          // language and parameter style below
        }
      else
        {
          // language not specified and library name
          // is inconclusive, issue an error
          *CmpCommon::diags() << DgSqlCode( -3284 )
                              << DgString0( externalPath );
          processReturn();
        }
    }

  // set parameter style and also language, if not already
  // specified, based on routine type and type of library
  if (isJava)
    {
      // library is a jar file

      if (rType == COM_PROCEDURE_TYPE)
        // Java stored procedures use the older Java style
        style = COM_STYLE_JAVA_CALL;
      else
        // Java UDFs use the newer Java object style
        style = COM_STYLE_JAVA_OBJ;
    }
  else
    {
      // assume the library is a DLL with C or C++ code
      if (rType == COM_TABLE_UDF_TYPE &&
          (language == COM_LANGUAGE_CPP ||
           !createRoutineNode->isLanguageTypeSpecified()))
        {
          // Table UDFs (TMUDFs) default to the C++ interface
          language = COM_LANGUAGE_CPP;
          style    = COM_STYLE_CPP_OBJ;
        }
      else if (rType == COM_SCALAR_UDF_TYPE &&
               (language == COM_LANGUAGE_C ||
                !createRoutineNode->isLanguageTypeSpecified()))
        {
          // scalar UDFs default to C and SQL parameter style
          language = COM_LANGUAGE_C;
          style    = COM_STYLE_SQL;
        }
      else
        {
          // some invalid combination of routine type, language and
          // library type
          *CmpCommon::diags() << DgSqlCode(-3286);
          processReturn();
          return;
        }
    } // C/C++ DLL

  if (createRoutineNode->isParamStyleSpecified() &&
      ddlStyle != style)
    {
      // An unsupported PARAMETER STYLE was specified
      *CmpCommon::diags() << DgSqlCode(-3280);
      processReturn();
      return;
    }

  NAString externalName;
  if (language == COM_LANGUAGE_JAVA &&
      style == COM_STYLE_JAVA_CALL)
    {
      // the external name is a Java method signature
      externalName = createRoutineNode->getJavaClassName();
      externalName += "." ;
      externalName += createRoutineNode->getJavaMethodName();
    }
  else
    // the external name is a C/C++ entry point or a
    // Java class name
    externalName = createRoutineNode->getExternalName();

  // Verify that current user has authority to create the routine
  // User must be DB__ROOT or have privileges
  if (isAuthorizationEnabled() && !ComUser::isRootUserID())
    {

      // For now, go get privileges directly.  If we ever cache routines, then
      // make sure privileges are stored in the cache.
      NAString privMgrMDLoc;
      CONCAT_CATSCH(privMgrMDLoc, getSystemCatalog(), SEABASE_PRIVMGR_SCHEMA);
      PrivMgrCommands privInterface(privMgrMDLoc.data(), CmpCommon::diags());
      PrivMgrUserPrivs privs;
      PrivStatus retcode = privInterface.getPrivileges(libUID, COM_LIBRARY_OBJECT, 
                                                       ComUser::getCurrentUser(), privs);
      if (retcode != STATUS_GOOD)
        {
          if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
            SEABASEDDL_INTERNAL_ERROR("checking routine privilege");
          processReturn();
          return;
        }

      // Requester must have USAGE privilege on the library
      NABoolean hasPriv = TRUE;
      if ( !privs.hasUsagePriv() )
        {
          *CmpCommon::diags() << DgSqlCode( -4481 )
                              << DgString0( "USAGE" )
                              << DgString1( extLibraryName.data());
          processReturn();
          return;
        }
    }

  ElemDDLParamDefArray &routineParamArray =
        createRoutineNode->getParamArray();
  Lng32 numParams = routineParamArray.entries();

  if ((createRoutineNode->getRoutineType() == COM_SCALAR_UDF_TYPE) &&
      (numParams > 32))
    {
      *CmpCommon::diags() << DgSqlCode( -1550 )
                          << DgString0( extRoutineName )
                          << DgInt0( numParams );
      deallocEHI(ehi); 
      processReturn();
      return;
    }
#define MAX_SIGNATURE_LENGTH 8193
  // Allocate buffer for generated signature
  char sigBuf[MAX_SIGNATURE_LENGTH];
  sigBuf[0] = '\0';

  if (style == COM_STYLE_JAVA_CALL) 
  {
     // validate routine for Java call based on signature
     Lng32 numJavaParam = 0;
     ComFSDataType *paramType = new ComFSDataType[numParams];
     ComUInt32     *subType   = new ComUInt32    [numParams];
     ComColumnDirection *direction = new ComColumnDirection[numParams];
     NAType *genericType;

     // Gather the param attributes for LM from the paramDefArray previously
     // populated and from the routineparamList generated from paramDefArray.

     for (CollIndex i = 0; (Int32)i < numParams; i++)
     {
       paramType[i] = (ComFSDataType)routineParamArray[i]->getParamDataType()->getFSDatatype();
       subType[i] = 0;  // default
       // Set subType for special cases detected by LM
       switch ( paramType[i] )
       {
         case COM_SIGNED_BIN8_FSDT :
         case COM_UNSIGNED_BIN8_FSDT :
         case COM_SIGNED_BIN16_FSDT :
         case COM_SIGNED_BIN32_FSDT :
         case COM_SIGNED_BIN64_FSDT :
         case COM_UNSIGNED_BIN16_FSDT :
         case COM_UNSIGNED_BIN32_FSDT :
         case COM_UNSIGNED_BPINT_FSDT :
         {
           genericType = routineParamArray[i]->getParamDataType() ;
           if (genericType->getTypeName() == LiteralNumeric)
             subType[i] = genericType->getPrecision();
           else
             subType[i] = 0 ;

           break;
         }

         case COM_DATETIME_FSDT :
         {
           genericType = routineParamArray[i]->getParamDataType() ;
           DatetimeType & datetimeType = (DatetimeType &) *genericType;
            if (datetimeType.getSimpleTypeName() EQU "DATE")
              subType[i] = 1 ;
            else if (datetimeType.getSimpleTypeName() EQU "TIME")
              subType[i] = 2;
            else if (datetimeType.getSimpleTypeName() EQU "TIMESTAMP")
              subType[i] = 3;
         }
       } // end switch paramType[i]

       direction[i] = (ComColumnDirection) routineParamArray[i]->getParamDirection();
     }
    
     // If the syntax specified a signature, pass that to LanguageManager.
     NAString specifiedSig( createRoutineNode->getJavaSignature() );
     char* optionalSig;
     if ( specifiedSig.length() == 0 )
       optionalSig = NULL;
     else
       optionalSig = (char *)specifiedSig.data();
     
     ComBoolean isJavaMain =
       ((str_cmp_ne(createRoutineNode->getJavaMethodName(), "main") == 0) ? TRUE : FALSE);

     LmResult createSigResult;
     LmJavaSignature *lmSignature =  new (STMTHEAP) LmJavaSignature(NULL,
                                                                    STMTHEAP);
     createSigResult = lmSignature->createSig(paramType, subType, direction,
                                              numParams, COM_UNKNOWN_FSDT, 0,
                                              createRoutineNode->getMaxResults(), optionalSig, isJavaMain, sigBuf,
                                              MAX_SIGNATURE_LENGTH,
                                              CmpCommon::diags());
     NADELETE(lmSignature, LmJavaSignature, STMTHEAP);
     delete [] paramType;
     delete [] subType;
     delete [] direction;

     // Lm returned error. Lm fills diags area, so no need to worry about diags.
     if (createSigResult == LM_ERR)
     {
       *CmpCommon::diags() << DgSqlCode(-1231)
			  << DgString0(extRoutineName);
       deallocEHI(ehi); 
       processReturn();
       return;
     }

     numJavaParam = (isJavaMain ? 1 : numParams);

     if (validateRoutine(&cliInterface, 
                         createRoutineNode->getJavaClassName(),
                         createRoutineNode->getJavaMethodName(),
                         externalPath,
                         sigBuf,
                         numJavaParam,
                         createRoutineNode->getMaxResults(),
                         optionalSig))
     {
       *CmpCommon::diags() << DgSqlCode(-1231)
                           << DgString0(extRoutineName);
       deallocEHI(ehi); 
       processReturn();
       return;
     }
  }
  else if (style == COM_STYLE_JAVA_OBJ ||
           style == COM_STYLE_CPP_OBJ)
  {
    // validate existence of the C++ or Java class in the library
    Int32 routineHandle = NullCliRoutineHandle;
    NAString externalPrefix(externalPath);
    NAString externalNameForValidation(externalName);
    NAString containerName;

    if (language == COM_LANGUAGE_C || language == COM_LANGUAGE_CPP)
      {
        // separate the actual DLL name from the prefix
        char separator = '/';
        size_t separatorPos = externalPrefix.last(separator);

        if (separatorPos != NA_NPOS)
          {
            containerName = externalPrefix(separatorPos+1,
                                           externalPrefix.length()-separatorPos-1);
            externalPrefix.remove(separatorPos,
                                  externalPrefix.length()-separatorPos);
          }
        else
          {
            // assume the entire string is a local name
            containerName = externalPrefix;
            externalPrefix = ".";
          }
      }
    else
      {
        // For Java, the way the language manager works is that the
        // external path is the fully qualified name of the jar and
        // the container is the class name (external name).  We load
        // the container (the class) by searching in the path (the
        // jar). The external name is the method name, which in this
        // case is the constructor of the class, <init>.

        // leave externalPrevix unchanged, fully qualified jar file
        containerName = externalName;
        externalNameForValidation = "<init>";
      }

    // use a CLI call to validate that the library contains the routine
    if (cliInterface.getRoutine(
             NULL, // No InvocationInfo specified in this step
             0,
             NULL,
             0,
             (Int32) language,
             (Int32) style,
             externalNameForValidation.data(),
             containerName.data(),
             externalPrefix.data(),
             extLibraryName.data(),
             &routineHandle,
             CmpCommon::diags()) != LME_ROUTINE_VALIDATED)
      {
        if (routineHandle != NullCliRoutineHandle)
          cliInterface.putRoutine(routineHandle,
                                  CmpCommon::diags());

        CMPASSERT(CmpCommon::diags()->mainSQLCODE() < 0);
        processReturn();
        return;
      }

    cliInterface.putRoutine(routineHandle,
                            CmpCommon::diags());
  }

  ComTdbVirtTableColumnInfo * colInfoArray = (ComTdbVirtTableColumnInfo*)
    new(STMTHEAP) ComTdbVirtTableColumnInfo[numParams];

  if (buildColInfoArray(&routineParamArray, colInfoArray))
    {
      processReturn();
      return;
    }

  ComTdbVirtTableTableInfo * tableInfo = new(STMTHEAP) ComTdbVirtTableTableInfo[1];
  tableInfo->tableName = NULL,
  tableInfo->createTime = 0;
  tableInfo->redefTime = 0;
  tableInfo->objUID = 0;
  tableInfo->objOwnerID = objectOwnerID;
  tableInfo->schemaOwnerID = schemaOwnerID;
  tableInfo->isAudited = 1;
  tableInfo->validDef = 1;
  tableInfo->hbaseCreateOptions = NULL;
  tableInfo->numSaltPartns = 0;
  tableInfo->rowFormat = COM_UNKNOWN_FORMAT_TYPE;
  tableInfo->objectFlags = 0;

  Int64 objUID = -1;
  if (updateSeabaseMDTable(&cliInterface, 
			   catalogNamePart, schemaNamePart, objectNamePart,
			   COM_USER_DEFINED_ROUTINE_OBJECT,
			   "N",
			   tableInfo,
			   numParams,
			   colInfoArray,
			   0, NULL,
			   0, NULL,
                           objUID))
    {
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  if (objUID == -1)
    {
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  NAString udrType;
  getRoutineTypeLit(createRoutineNode->getRoutineType(), udrType);
  NAString languageType;
  getLanguageTypeLit(language, languageType);
  NAString sqlAccess;
  getSqlAccessLit(createRoutineNode->getSqlAccess(), sqlAccess);
  NAString paramStyle;
  getParamStyleLit(style, paramStyle);
  NAString transactionAttributes;
  getTransAttributesLit(createRoutineNode->getTransactionAttributes(), transactionAttributes);
  NAString parallelism;
  getParallelismLit(createRoutineNode->getParallelism(), parallelism);
  NAString externalSecurity;
  getExternalSecurityLit(createRoutineNode->getExternalSecurity(), externalSecurity);
  NAString executionMode;
  getExecutionModeLit(createRoutineNode->getExecutionMode(), executionMode);
  

  char * query = new(STMTHEAP) char[2000+MAX_SIGNATURE_LENGTH];
  str_sprintf(query, "insert into %s.\"%s\".%s values (%ld, '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', %d, %d, '%s', '%s', '%s', '%s', '%s', %ld, '%s', 0)",
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_ROUTINES,
	      objUID,
              udrType.data(),
              languageType.data(),
              createRoutineNode->isDeterministic() ? "Y" : "N" ,
              sqlAccess.data(),
              createRoutineNode->isCallOnNull() ? "Y" : "N" ,
              createRoutineNode->isIsolate() ? "Y" : "N" ,
              paramStyle.data(),
              transactionAttributes.data(),
              createRoutineNode->getMaxResults(),
              createRoutineNode->getStateAreaSize(),
              externalName.data(),
              parallelism.data(),
              createRoutineNode->getUserVersion().data(),
              externalSecurity.data(),
              executionMode.data(),
              libUID,
              sigBuf);
  
  cliRC = cliInterface.executeImmediate(query);
  NADELETEBASIC(query, STMTHEAP);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      processReturn();
      return;
    }

  char * query1 = new(STMTHEAP) char[1000];
  str_sprintf(query1, "insert into %s.\"%s\".%s values (%ld, %ld, 0)",
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_LIBRARIES_USAGE,
	      libUID, objUID);
  
  cliRC = cliInterface.executeImmediate(query1);
  NADELETEBASIC(query1, STMTHEAP);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      processReturn();
      return;
    }

  // hope to remove this call soon by setting the valid flag to Y sooner
  if (updateObjectValidDef(&cliInterface, 
			   catalogNamePart, schemaNamePart, objectNamePart,
			   COM_USER_DEFINED_ROUTINE_OBJECT_LIT,
			   "Y"))
    {
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  // Remove cached entries in other processes
  NARoutineDB *pRoutineDBCache  = ActiveSchemaDB()->getNARoutineDB();
  QualifiedName qualRoutineName(routineName, STMTHEAP);
  pRoutineDBCache->removeNARoutine(qualRoutineName, 
                                   ComQiScope::REMOVE_FROM_ALL_USERS,
                                   objUID,
                                   createRoutineNode->ddlXns(), FALSE);

  processReturn();
  return;
}

void CmpSeabaseDDL::dropSeabaseRoutine(StmtDDLDropRoutine * dropRoutineNode,
                                       NAString &currCatName, 
                                       NAString &currSchName)
{
  Lng32 retcode = 0;
 
  ComObjectName routineName(dropRoutineNode->getRoutineName());
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  routineName.applyDefaults(currCatAnsiName, currSchAnsiName);
  const NAString catalogNamePart = 
    routineName.getCatalogNamePartAsAnsiString();
  const NAString schemaNamePart = 
    routineName.getSchemaNamePartAsAnsiString(TRUE);
  const NAString objectNamePart = 
    routineName.getObjectNamePartAsAnsiString(TRUE);
  const NAString extRoutineName = routineName.getExternalName(TRUE);
  
  ExpHbaseInterface * ehi = NULL;
  ExeCliInterface cliInterface(STMTHEAP, 0, NULL, 
    CmpCommon::context()->sqlSession()->getParentQid());

  ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();
      return;
    }

  retcode = existsInSeabaseMDTable(&cliInterface, 
				   catalogNamePart, schemaNamePart, 
                                   objectNamePart, COM_USER_DEFINED_ROUTINE_OBJECT, 
                                   TRUE, FALSE);
  if (retcode < 0)
    {
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  if (retcode == 0) // does not exist
    {
      if (NOT dropRoutineNode->dropIfExists())
        *CmpCommon::diags() << DgSqlCode(-1389)
	      		    << DgString0(extRoutineName);
      deallocEHI(ehi); 
      processReturn();
      return;
    }
  
  // get objectOwner
  Int64 objUID = 0;
  Int32 objectOwnerID = 0;
  Int32 schemaOwnerID = 0;
  Int64 objectFlags = 0;

  // see if routine is cached
  BindWA bindWA(ActiveSchemaDB(), CmpCommon::context(), FALSE/*inDDL*/);
  NARoutineDB *pRoutineDBCache  = ActiveSchemaDB()->getNARoutineDB();
  QualifiedName qualRoutineName(routineName, STMTHEAP);
  NARoutineDBKey key(qualRoutineName, STMTHEAP);

  NARoutine *cachedNARoutine = pRoutineDBCache->get(&bindWA, &key);
  if (cachedNARoutine)
    {
      objUID = cachedNARoutine->getRoutineID();
      objectOwnerID = cachedNARoutine->getObjectOwner();
      schemaOwnerID = cachedNARoutine->getSchemaOwner();
    }
  else
    {
      objUID = getObjectInfo(&cliInterface,
			      catalogNamePart.data(), schemaNamePart.data(), 
			      objectNamePart.data(), COM_USER_DEFINED_ROUTINE_OBJECT,
                              objectOwnerID,schemaOwnerID,objectFlags);
    if (objUID < 0 || objectOwnerID == 0 || schemaOwnerID == 0)
      {
        deallocEHI(ehi); 
        processReturn();
        return;
      }
    }

  // Verify user has privilege to drop routine
  if (!isDDLOperationAuthorized(SQLOperation::DROP_ROUTINE,objectOwnerID,schemaOwnerID))
  {
     *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
     deallocEHI(ehi);
     processReturn ();
     return;
  }
  
  // Determine if this function is referenced by any other objects.
  Lng32 cliRC = 0;
  Queue * usingViewsQueue = NULL;
  if (dropRoutineNode->getDropBehavior() == COM_RESTRICT_DROP_BEHAVIOR)
    {
      NAString usingObjName;
      cliRC = getUsingObject(&cliInterface, objUID, usingObjName);
      if (cliRC < 0)
        {
          deallocEHI(ehi); 
          processReturn();
          
          return;
        }

      if (cliRC != 100) // found an object
        {
          *CmpCommon::diags() << DgSqlCode(-CAT_DEPENDENT_VIEW_EXISTS)
                              << DgTableName(usingObjName);

          deallocEHI(ehi); 
          processReturn();

          return;
        }
    }
  else 
    if (dropRoutineNode->getDropBehavior() == COM_CASCADE_DROP_BEHAVIOR)
    {
      cliRC = getUsingViews(&cliInterface, objUID, usingViewsQueue);
      if (cliRC < 0)
        {
          deallocEHI(ehi); 
          processReturn();
          
          return;
        }
    }
  
  if (usingViewsQueue)
    {
      usingViewsQueue->position();
      for (int idx = 0; idx < usingViewsQueue->numEntries(); idx++)
        {
          OutputInfo * vi = (OutputInfo*)usingViewsQueue->getNext(); 
          
          char * viewName = vi->get(0);
          
          if (dropOneTableorView(cliInterface,viewName,COM_VIEW_OBJECT,false))
          
            {
              deallocEHI(ehi); 
              processReturn();
              
              return;
            }
        }
    }
  
  // Removed routine from metadata 
  if (dropSeabaseObject(ehi, dropRoutineNode->getRoutineName(),
                        currCatName, currSchName, 
                        COM_USER_DEFINED_ROUTINE_OBJECT,
                        dropRoutineNode->ddlXns(),
                        TRUE, FALSE))
    {
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  // Remove cached entries in other processes
  pRoutineDBCache->removeNARoutine(qualRoutineName, 
                                   ComQiScope::REMOVE_FROM_ALL_USERS,
                                   objUID,
                                   dropRoutineNode->ddlXns(), FALSE);

  deallocEHI(ehi);      
  processReturn();
  return;
}

short CmpSeabaseDDL::validateRoutine(ExeCliInterface *cliInterface,
                                     const char * className,
                                     const char * methodName,
                                     const char * externalPath,
                                     char * signature,
                                     Int32 numSqlParam,
                                     Int32 maxResultSets,
                                     const char * optionalSig)
{
  
  //
  // Now proceed with the internal CALL statement...
  //

  Lng32 sigLen = 0;
  if (signature)
    sigLen = str_len(signature) + 1;

  char * query = new(STMTHEAP) char[2000+sigLen];
  str_sprintf(query, "call %s.\"%s\".%s ('%s', '%s', '%s', '%s', %d, %d, %d, ?x, ?y, ?z)",
	      getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_VALIDATE_SPJ,
	      className, methodName, externalPath, signature,
              numSqlParam, maxResultSets, optionalSig ? 1 : 0); 
             
  Lng32 cliRC = cliInterface->fetchRowsPrologue(query, TRUE/*no exec*/);
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
 
  NADELETEBASIC(query, STMTHEAP);

  char * ptr = NULL;
  Lng32 len = 0;
  Int32 errCode = 0;

  cliInterface->getPtrAndLen(1, ptr, len);
  str_cpy_all(signature, ptr, len);
  signature[len] =  '\0';
  cliInterface->getPtrAndLen(2, ptr, len);
  errCode = *(Int32 *)ptr;

  // Check for errors returned from VALIDATEROUTINE
  switch (errCode)
  {
    case 0://Success - Check to see if returned signature is null
        if (signature[0] NEQ '\0')
          return 0;
        else
          return -1;
      break;
    case 11205://Class not found
        *CmpCommon::diags() << DgSqlCode(-errCode)
                            << DgString0(className)
                            << DgString1(externalPath);
      break;
    case 11206://Class definition not found
        *CmpCommon::diags() << DgSqlCode(-errCode)
                  << DgString0(className);
      break;
    case 11230://Overloaded methods were found
        *CmpCommon::diags() << DgSqlCode(-errCode)
                            << DgString0(methodName)
                            << DgString1(className);
      break;
    case 11239://No compatible methods were found
      *CmpCommon::diags() << DgSqlCode(-errCode)
                          << DgString0(methodName)
                          << DgString1(className);
      break;
    case 11231://Method found but not public
      if(signature[0] NEQ '\0')
        *CmpCommon::diags() << DgSqlCode(-errCode)
                            << DgString0(NAString(methodName) + signature)
                            << DgString1(className);
      break;
    case 11232://Method found but not static
      if(signature[0] NEQ '\0')
        *CmpCommon::diags() << DgSqlCode(-errCode)
                            << DgString0(NAString(methodName) + signature)
                            << DgString1(className);
      break;
    case 11233://Method found but not void
        if(signature[0] NEQ '\0')
          *CmpCommon::diags() << DgSqlCode(-errCode)
                              << DgString0(NAString(methodName) + signature)
                              << DgString1(className);
        break;
    case 11234://Method not found
        if(signature[0] NEQ '\0')
          *CmpCommon::diags() << DgSqlCode(-errCode)
                              << DgString0(NAString(methodName) + signature)
                              << DgString1(className);
        break;
    default://Unknown error code
      break ;
  }
  return -1;

} // CmpSeabaseDDL::validateRoutine

short CmpSeabaseDDL::createSeabaseLibmgr(ExeCliInterface * cliInterface)
{
  if (!ComUser::isRootUserID())
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
      return -1;
    }

  Lng32 cliRC = 0;
  
  if ((CmpCommon::context()->isUninitializedSeabase()) &&
      (CmpCommon::context()->uninitializedSeabaseErrNum() == -TRAF_NOT_INITIALIZED))
    {
      *CmpCommon::diags() << DgSqlCode(-TRAF_NOT_INITIALIZED);
      return -1;
    }

  NAString jarLocation(getenv("TRAF_HOME"));
  jarLocation += "/export/lib/lib_mgmt.jar";
  char queryBuf[strlen(getSystemCatalog()) + strlen(SEABASE_LIBMGR_SCHEMA) +
                strlen(SEABASE_LIBMGR_LIBRARY) + strlen(DB__LIBMGRROLE) + 
                jarLocation.length() + 100];

  // Create the SEABASE_LIBMGR_SCHEMA schema
  snprintf(queryBuf, sizeof(queryBuf),
           "create schema if not exists %s.\"%s\" authorization %s ",
           getSystemCatalog(),SEABASE_LIBMGR_SCHEMA, DB__ROOT);

  cliRC = cliInterface->executeImmediate(queryBuf);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  // Create the SEABASE_LIBMGR_LIBRARY library
  snprintf(queryBuf, sizeof(queryBuf),
           "create library %s.\"%s\".%s file '%s'",
           getSystemCatalog(), SEABASE_LIBMGR_SCHEMA, SEABASE_LIBMGR_LIBRARY,
           jarLocation.data());

  cliRC = cliInterface->executeImmediate(queryBuf);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  if (createSeabaseLibmgrCPPLib(cliInterface) < 0)
    return -1;

  return (createLibmgrProcs(cliInterface));
}

short CmpSeabaseDDL::createLibmgrProcs(ExeCliInterface * cliInterface)
{
  Lng32 cliRC = 0;

 // Create the UDRs if they don't already exist
  for (Int32 i = 0; i < sizeof(allLibmgrRoutineInfo)/sizeof(LibmgrRoutineInfo); i++)
    {
      // Get the next routine details
      const LibmgrRoutineInfo &prd = allLibmgrRoutineInfo[i];

      const QString * qs = NULL;
      Int32 sizeOfqs = 0;
      const char *libName = NULL;

      qs = prd.newDDL;
      sizeOfqs = prd.sizeOfnewDDL;

      Int32 qryArraySize = sizeOfqs / sizeof(QString);
      char * gluedQuery;
      Lng32 gluedQuerySize;
      glueQueryFragments(qryArraySize,  qs,
                         gluedQuery, gluedQuerySize);

      switch (prd.whichLib)
        {
        case LibmgrRoutineInfo::JAVA_LIB:
          libName = SEABASE_LIBMGR_LIBRARY;
          break;
        case LibmgrRoutineInfo::CPP_LIB:
          libName = SEABASE_LIBMGR_LIBRARY_CPP;
          break;
        default:
          CMPASSERT(0);
        }

      param_[0] = getSystemCatalog();
      param_[1] = SEABASE_LIBMGR_SCHEMA;
      param_[2] = getSystemCatalog();
      param_[3] = SEABASE_LIBMGR_SCHEMA;
      param_[4] = libName;

      // Review comment - make sure size of queryBuf is big enough to hold
      // generated text.
      char queryBuf[strlen(getSystemCatalog())*2 + strlen(SEABASE_LIBMGR_SCHEMA)*2 +
                    strlen(SEABASE_LIBMGR_LIBRARY) + gluedQuerySize + 200]; 

      snprintf(queryBuf, sizeof(queryBuf),
               gluedQuery, param_[0], param_[1], param_[2], param_[3], param_[4]);
      NADELETEBASICARRAY(gluedQuery, STMTHEAP);

      cliRC = cliInterface->executeImmediate(queryBuf);
      if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
          return -1;
        }
    } // for

  return (grantLibmgrPrivs(cliInterface));
}

// If authorization is enabled, grant privileges to DB__LIBMGRROLE
short CmpSeabaseDDL::grantLibmgrPrivs(ExeCliInterface *cliInterface)
{
  if (!isAuthorizationEnabled())
    return 0;

  Lng32 cliRC = 0;
  char queryBuf[strlen(getSystemCatalog()) + strlen(SEABASE_LIBMGR_SCHEMA) +
                strlen(SEABASE_LIBMGR_LIBRARY) +
                MAXOF(strlen(DB__LIBMGRROLE), strlen(PUBLIC_AUTH_NAME)) + 200];
  for (Int32 i = 0; i < sizeof(allLibmgrRoutineInfo)/sizeof(LibmgrRoutineInfo); i++)
    {
      // Get the next procedure routine details
      const LibmgrRoutineInfo &prd = allLibmgrRoutineInfo[i];
      const char *grantee = NULL;
      const char *grantOption = "";

      switch (prd.whichRole)
        {
        case LibmgrRoutineInfo::LIBMGR_ROLE:
          grantee = DB__LIBMGRROLE;
          grantOption = " with grant option";
          break;
        case LibmgrRoutineInfo::PUBLIC:
          grantee = PUBLIC_AUTH_NAME;
          break;
        default:
          CMPASSERT(0);
        }

      snprintf(queryBuf, sizeof(queryBuf),
               "grant execute on %s %s.\"%s\".%s to %s%s",
               prd.udrType,
               getSystemCatalog(),
               SEABASE_LIBMGR_SCHEMA,
               prd.newName,
               grantee,
               grantOption);
      cliRC = cliInterface->executeImmediate(queryBuf);
      if (cliRC < 0)
        {
          cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
          return -1;
        }
    }
  return 0;
}

short CmpSeabaseDDL::upgradeSeabaseLibmgr(ExeCliInterface * cliInterface)
{
  if (!ComUser::isRootUserID())
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
      return -1;
    }

  Lng32 cliRC = 0;

  cliRC = existsInSeabaseMDTable(cliInterface,
                                 getSystemCatalog(), SEABASE_LIBMGR_SCHEMA,
                                 SEABASE_LIBMGR_LIBRARY,
                                 COM_LIBRARY_OBJECT, TRUE, FALSE);
  if (cliRC < 0)
    return -1;

  if (cliRC == 0) // does not exist
    {
      // give an error if the Java library does not exist, since that is
      // an indication that we never ran
      // INITIALIZE TRAFODION, CREATE LIBRARY MANAGEMENT
      NAString libraryName(getSystemCatalog());
      libraryName + ".\"" + SEABASE_LIBMGR_SCHEMA + "\"" + SEABASE_LIBMGR_LIBRARY;
      *CmpCommon::diags() << DgSqlCode(-1389)
                          << DgString0(libraryName.data());
      return -1;
    }

  // Update the jar locations for system procedures and functions.  This should 
  // be done before adding any new jar's since we use a system procedure to add
  // procedures.
  NAString jarLocation(getenv("TRAF_HOME"));
  jarLocation += "/export/lib";

  char queryBuf[1000];

  // trafodion-sql_currversion.jar 
  Int32 stmtSize = snprintf(queryBuf, sizeof(queryBuf), "update %s.\"%s\".%s  "
           "set library_filename = '%s/trafodion-sql-currversion.jar' "
           "where library_uid = "
           "(select object_uid from %s.\"%s\".%s "
           " where object_name = '%s'  and object_type = 'LB')",
           getSystemCatalog(),SEABASE_MD_SCHEMA, SEABASE_LIBRARIES, jarLocation.data(),
           getSystemCatalog(),SEABASE_MD_SCHEMA, SEABASE_OBJECTS, SEABASE_VALIDATE_LIBRARY);
  CMPASSERT(stmtSize < sizeof(queryBuf));

  cliRC = cliInterface->executeImmediate(queryBuf);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  // lib_mgmt.jar
  stmtSize = snprintf(queryBuf, sizeof(queryBuf), "update %s.\"%s\".%s  "
           "set library_filename = '%s/lib_mgmt.jar' "
           "where library_uid = "
           "(select object_uid from %s.\"%s\".%s "
           " where object_name = '%s'  and object_type = 'LB')",
           getSystemCatalog(),SEABASE_MD_SCHEMA, SEABASE_LIBRARIES, jarLocation.data(),
           getSystemCatalog(),SEABASE_MD_SCHEMA, SEABASE_OBJECTS, SEABASE_LIBMGR_LIBRARY);
  CMPASSERT(stmtSize < sizeof(queryBuf));

  cliRC = cliInterface->executeImmediate(queryBuf);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }

  // libudr_predef.so
  NAString dllLocation(getenv("TRAF_HOME"));
  dllLocation += "/export/lib64";
  if (strcmp(getenv("SQ_MBTYPE"), "64d") == 0)
    dllLocation += "d";

  stmtSize = snprintf(queryBuf, sizeof(queryBuf), "update %s.\"%s\".%s  "
           "set library_filename = '%s/libudr_predef.so' "
           "where library_uid = "
           "(select object_uid from %s.\"%s\".%s "
           " where object_name = '%s'  and object_type = 'LB')",
           getSystemCatalog(),SEABASE_MD_SCHEMA, SEABASE_LIBRARIES, dllLocation.data(),
           getSystemCatalog(),SEABASE_MD_SCHEMA, SEABASE_OBJECTS, SEABASE_LIBMGR_LIBRARY_CPP);
  CMPASSERT(stmtSize < sizeof(queryBuf));

  cliRC = cliInterface->executeImmediate(queryBuf);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }


  // now check for the C++ library, which was added in Trafodion 2.3
  cliRC = existsInSeabaseMDTable(cliInterface,
                                 getSystemCatalog(), SEABASE_LIBMGR_SCHEMA,
                                 SEABASE_LIBMGR_LIBRARY_CPP,
                                 COM_LIBRARY_OBJECT, TRUE, FALSE);
  if (cliRC < 0)
    return -1;

  if (cliRC == 0)
    {
      // The Java library exists, but the C++ library does not yet
      // exist. This means that we last created or upgraded the
      // library management subsystem in Trafodion 2.2 or earlier.
      // Create the C++ library, as it is needed for Trafodion 2.3
      // and higher.
      if (createSeabaseLibmgrCPPLib(cliInterface) < 0)
        return -1;
    }

  return (createLibmgrProcs(cliInterface));
}

short CmpSeabaseDDL::dropSeabaseLibmgr(ExeCliInterface *cliInterface)
{
    if (!ComUser::isRootUserID())
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
      return -1;
    }

  Lng32 cliRC = 0;

  char queryBuf[strlen(getSystemCatalog()) + strlen(SEABASE_LIBMGR_SCHEMA) + 100];

  // save the current parserflags setting
  ULng32 savedParserFlags = Get_SqlParser_Flags(0xFFFFFFFF);
  Set_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL);

  str_sprintf(queryBuf, "drop schema if exists %s.\"%s\" cascade ",
              getSystemCatalog(),SEABASE_LIBMGR_SCHEMA);

  // Drop the SEABASE_LIBMGR_SCHEMA schema
  cliRC = cliInterface->executeImmediate(queryBuf);

  // Restore parser flags settings to what they originally were
  Assign_SqlParser_Flags(savedParserFlags);

  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }
  return 0;
}
  
short CmpSeabaseDDL::createSeabaseLibmgrCPPLib(ExeCliInterface * cliInterface)
{
  Int32 cliRC = 0;
  NAString dllLocation(getenv("TRAF_HOME"));
  dllLocation += "/export/lib64";
  if (strcmp(getenv("SQ_MBTYPE"), "64d") == 0)
    dllLocation += "d";
  // for now we use the same DLL as for the predefined UDRs
  dllLocation += "/libudr_predef.so";
  char queryBuf[strlen(getSystemCatalog()) + strlen(SEABASE_LIBMGR_SCHEMA) +
                strlen(SEABASE_LIBMGR_LIBRARY_CPP) +
                dllLocation.length() + 100];

  // Create the SEABASE_LIBMGR_LIBRARY_CPP library
  snprintf(queryBuf, sizeof(queryBuf),
           "create library %s.\"%s\".%s file '%s'",
           getSystemCatalog(), SEABASE_LIBMGR_SCHEMA, SEABASE_LIBMGR_LIBRARY_CPP,
           dllLocation.data());

  cliRC = cliInterface->executeImmediate(queryBuf);
  if (cliRC < 0)
    {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
    }
  return 0;
}

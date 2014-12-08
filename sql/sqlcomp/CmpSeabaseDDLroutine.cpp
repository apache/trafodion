/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2014 Hewlett-Packard Development Company, L.P.
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

#include "StmtDDLCreateRoutine.h"
#include "StmtDDLDropRoutine.h"
#include "StmtDDLCreateLibrary.h"
#include "StmtDDLDropLibrary.h"

#include "ElemDDLColDefArray.h"
#include "ElemDDLColRefArray.h"
#include "ElemDDLParamDefArray.h"
#include "ElemDDLParamDef.h"

#include "SchemaDB.h"
#include "CmpSeabaseDDL.h"

#include "ExpHbaseInterface.h"

#include "ExExeUtilCli.h"
#include "Generator.h"
#include "desc.h"
#include "ComSmallDefs.h"
#include "CmpDDLCatErrorCodes.h"

#include "PrivMgrComponentPrivileges.h"
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
// *  MY_SQROOT before validating existence.                                   *
// *                                                                            *
// *****************************************************************************
static int validateLibraryFileExists(
   const NAString    &libraryFilename,
   bool                isSystemObject)
   
{
      
  NAString completeLibraryFilename(libraryFilename);
  
  if (isSystemObject) {
    completeLibraryFilename.insert(0,'/');
    completeLibraryFilename.insert(0,getenv("MY_SQROOT"));
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
    else if (val == COM_AGGREGATE_UDF_TYPE)
      result = COM_AGGREGATE_UDF_TYPE_LIT;
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
    else if (val == COM_STYLE_JAVA)
      result = COM_STYLE_JAVA_LIT;
    else if (val == COM_STYLE_SQL)
      result = COM_STYLE_SQL_LIT;
    else if (val == COM_STYLE_SQLROW)
      result = COM_STYLE_SQLROW_LIT;
     else if (val == COM_STYLE_TM)
      result = COM_STYLE_SQLROW_TM;
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
  if (!ComUser::isRootUserID())
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
  if (!isDDLOperationAuthorized(SQLOperation::CREATE_LIBRARY,
                                ComUser::getCurrentUser()))
  {
     *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
     processReturn ();
     return;
  }

  ComUserVerifyObj verifyAuth(libraryName, ComUserVerifyObj::OBJ_OBJ_TYPE);
  Int32 objOwnerID = verifyAuth.getEffectiveUserID(ComUser::CREATE_LIBRARY);

  ExpHbaseInterface * ehi = NULL;
  ExeCliInterface cliInterface(STMTHEAP);

  ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();
      return;
    }

  retcode = existsInSeabaseMDTable(&cliInterface, 
				   catalogNamePart, schemaNamePart, 
                                   objectNamePart, COM_LIBRARY_OBJECT_LIT, 
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
  
  Int64 objUID = -1;
  if (updateSeabaseMDTable(&cliInterface, 
			   catalogNamePart, schemaNamePart, objectNamePart,
			   COM_LIBRARY_OBJECT_LIT,
			   "N",
			   NULL,
			   0,
			   NULL,
			   0,
			   NULL,
			   0, NULL,
                           objOwnerID,
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
  str_sprintf(query, "insert into %s.\"%s\".%s values (%Ld, '%s', %d)",
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

  ExeCliInterface cliInterface(STMTHEAP);

  ExpHbaseInterface * ehi = allocEHI();
  if (ehi == NULL)
    return;

  retcode = existsInSeabaseMDTable(&cliInterface, 
				   catalogNamePart, schemaNamePart, 
                                   objectNamePart,
				   COM_LIBRARY_OBJECT_LIT, TRUE, FALSE);
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

  Int32 objOwnerID = 0;
  Int64 objUID = getObjectUIDandOwner(&cliInterface,
			      catalogNamePart.data(), schemaNamePart.data(), 
			      objectNamePart.data(), COM_LIBRARY_OBJECT_LIT,
                              NULL, &objOwnerID);
  if (objUID < 0 || objOwnerID == 0)
    {
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  if (!isDDLOperationAuthorized(SQLOperation::DROP_LIBRARY, objOwnerID))
  {
     *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
     processReturn ();
     return;
  }
  
  NAString usingObjName;
  cliRC = getUsingRoutine(&cliInterface, objUID, usingObjName);
  if (cliRC < 0)
    {
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  if (cliRC != 100) // found an object
    {
      *CmpCommon::diags() << DgSqlCode(-1366) ;
        // << DgTableName(usingObjName);

      deallocEHI(ehi); 
      processReturn();
      return;
    }
 

  // can get a slight perf. gain if we pass in objUID
  if (dropSeabaseObject(ehi, objName,
                        currCatName, currSchName, COM_LIBRARY_OBJECT_LIT,
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
  
  // Check to see if user has the authority to create the routine
  if (!isDDLOperationAuthorized(SQLOperation::CREATE_ROUTINE,
                                ComUser::getCurrentUser()))
  {
     *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
     processReturn ();
     return;
  }
  
  ComUserVerifyObj verifyAuth(routineName, ComUserVerifyObj::OBJ_OBJ_TYPE);
  Int32 objOwnerID = verifyAuth.getEffectiveUserID(ComUser::CREATE_ROUTINE);

  ExpHbaseInterface * ehi = NULL;
  ExeCliInterface cliInterface(STMTHEAP);

  ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();
      return;
    }
  
  retcode = existsInSeabaseMDTable(&cliInterface, 
				   catalogNamePart, schemaNamePart, 
                                   objectNamePart, COM_USER_DEFINED_ROUTINE_OBJECT_LIT, 
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

  ElemDDLParamDefArray &routineParamArray =
        createRoutineNode->getParamArray();
  Lng32 numParams = routineParamArray.entries();

#define MAX_SIGNATURE_LENGTH 8193
  // Allocate buffer for generated signature
  char sigBuf[MAX_SIGNATURE_LENGTH];
  sigBuf[0] = '\0';
  Lng32 cliRC = 0;
  // validate routine
  if (createRoutineNode->getLanguageType() == COM_LANGUAGE_JAVA) 
  {
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

     char * buf = new(STMTHEAP) char[200];
     str_sprintf(buf, "select library_filename from %s.\"%s\".%s"
                 " where library_uid = %Ld for read uncommitted access",
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
     char externalPath[512] ;
     cliInterface.getPtrAndLen(1, ptr, len);
     str_cpy_all(externalPath, ptr, len);
     externalPath[len] = '\0'; 

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

  ComTdbVirtTableColumnInfo * colInfoArray = (ComTdbVirtTableColumnInfo*)
    new(STMTHEAP) ComTdbVirtTableColumnInfo[numParams];

  if (buildColInfoArray(&routineParamArray, colInfoArray))
    {
      processReturn();
      return;
    }

  Int64 objUID = -1;
  if (updateSeabaseMDTable(&cliInterface, 
			   catalogNamePart, schemaNamePart, objectNamePart,
			   COM_USER_DEFINED_ROUTINE_OBJECT_LIT,
			   "N",
			   NULL,
			   numParams,
			   colInfoArray,
			   0, NULL,
			   0, NULL,
                           objOwnerID,
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
  getLanguageTypeLit(createRoutineNode->getLanguageType(), languageType);
  NAString sqlAccess;
  getSqlAccessLit(createRoutineNode->getSqlAccess(), sqlAccess);
  NAString paramStyle;
  getParamStyleLit(createRoutineNode->getParamStyle(), paramStyle);
  NAString transactionAttributes;
  getTransAttributesLit(createRoutineNode->getTransactionAttributes(), transactionAttributes);
  NAString parallelism;
  getParallelismLit(createRoutineNode->getParallelism(), parallelism);
  NAString externalSecurity;
  getExternalSecurityLit(createRoutineNode->getExternalSecurity(), externalSecurity);
  NAString executionMode;
  getExecutionModeLit(createRoutineNode->getExecutionMode(), executionMode);
  NAString externalName;
  if (createRoutineNode->getLanguageType() == COM_LANGUAGE_JAVA)
    {
      externalName = createRoutineNode->getJavaClassName();
      externalName += "." ;
      externalName += createRoutineNode->getJavaMethodName();
    }
  else
    externalName = createRoutineNode->getExternalName() ;
  

  char * query = new(STMTHEAP) char[2000+MAX_SIGNATURE_LENGTH];
  str_sprintf(query, "insert into %s.\"%s\".%s values (%Ld, '%s', '%s', '%s', '%s', '%s', '%s', '%s', '%s', %d, %d, '%s', '%s', '%s', '%s', '%s', %Ld, '%s' )",
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
  str_sprintf(query1, "insert into %s.\"%s\".%s values (%Ld, %Ld)",
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
  ExeCliInterface cliInterface(STMTHEAP);

  ehi = allocEHI();
  if (ehi == NULL)
    {
      processReturn();
      return;
    }

  retcode = existsInSeabaseMDTable(&cliInterface, 
				   catalogNamePart, schemaNamePart, 
                                   objectNamePart, COM_USER_DEFINED_ROUTINE_OBJECT_LIT, 
                                   TRUE, FALSE);
  if (retcode < 0)
    {
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  if (retcode == 0) // does not exist
    {
      *CmpCommon::diags() << DgSqlCode(-1389)
			  << DgString0(extRoutineName);
      deallocEHI(ehi); 
      processReturn();
      return;
    }
  
  // get objectOwner
  Int32 objOwnerID = 0;
  Int64 objUID = getObjectUIDandOwner(&cliInterface,
			      catalogNamePart.data(), schemaNamePart.data(), 
			      objectNamePart.data(), COM_USER_DEFINED_ROUTINE_OBJECT_LIT,
                              NULL, &objOwnerID);
  if (objUID < 0 || objOwnerID == 0)
    {
      deallocEHI(ehi); 
      processReturn();
      return;
    }

  // Verify user has privilege to drop routine
  if (!isDDLOperationAuthorized(SQLOperation::DROP_ROUTINE, objOwnerID))
  {
     *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
     deallocEHI(ehi);
     processReturn ();
     return;
  }

  if (dropSeabaseObject(ehi, dropRoutineNode->getRoutineName(),
                        currCatName, currSchName, COM_USER_DEFINED_ROUTINE_OBJECT_LIT,
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

  // in code below methodName may need to be added to the signature that is printed 
  // out in some error messages.
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
                            << DgString0(signature)
                            << DgString1(className);
      break;
    case 11232://Method found but not static
      if(signature[0] NEQ '\0')
        *CmpCommon::diags() << DgSqlCode(-errCode)
                            << DgString0(signature)
                            << DgString1(className);
      break;
    case 11233://Method found but not void
        if(signature[0] NEQ '\0')
          *CmpCommon::diags() << DgSqlCode(-errCode)
                              << DgString0(signature)
                              << DgString1(className);
        break;
    case 11234://Method not found
        if(signature[0] NEQ '\0')
          *CmpCommon::diags() << DgSqlCode(-errCode)
                              << DgString0(signature)
                              << DgString1(className);
        break;
    default://Unknown error code
      break ;
  }
  return -1;

} // CmpSeabaseDDL::validateRoutine

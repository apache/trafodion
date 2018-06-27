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
 * File:         CmpSeabaseDDLschema.cpp
 * Description:  Implements ddl operations for Seabase schemas.
 *
 *
 * Created:     10/30/2014
 * Language:     C++
 *
 *
 *****************************************************************************
 */


#include "CmpSeabaseDDLincludes.h"
#include "StmtDDLCreateSchema.h"
#include "StmtDDLDropSchema.h"
#include "StmtDDLAlterSchema.h"
#include "StmtDDLGive.h"
#include "ElemDDLColDefault.h"
#include "NumericType.h"
#include "ComUser.h"
#include "keycolumns.h"
#include "ElemDDLColRef.h"
#include "ElemDDLColName.h"

#include "CmpDDLCatErrorCodes.h"
#include "Globals.h"
#include "CmpMain.h"
#include "Context.h"
#include "PrivMgrCommands.h"
#include "PrivMgrObjects.h"
#include <vector>

static bool dropOneTable(
   ExeCliInterface & cliInterface,
   const char * catalogName, 
   const char * schemaName, 
   const char * objectName,
   bool isVolatile,
   bool ifExists,
   bool ddlXns);
   
static bool transferObjectPrivs(
   const char * systemCatalogName, 
   const char * catalogName,
   const char * schemaName,
   const int32_t newOwnerID,
   const char * newOwnerName);   

// *****************************************************************************
// *                                                                           *
// * Function: CmpSeabaseDDL::addSchemaObject                                  *
// *                                                                           *
// *    Inserts a schema object row into the OBJECTS table.                    *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <cliInterface>                  ExeCliInterface &               In       *
// *    is a reference to an Executor CLI interface handle.                    *
// *                                                                           *
// *  <schemaName>                    const ComSchemaName &           In       *
// *    is a reference to a ComSchemaName instance.  The catalog name must be  *
// *  set.                                                                     *
// *                                                                           *
// *  <schemaClass>                   ComSchemaClass                  In       *
// *    is the class (private or shared) of the schema to be added.            *
// *                                                                           *
// *  <ownerID>                       Int32                           In       *
// *    is the authorization ID that will own the schema.                      *
// *                                                                           *
// *  <ignoreIfExists>                NABoolean                       In       *
// *    do not return an error is schema already exists                        *
// *****************************************************************************
// *                                                                           *
// * Returns: status
// *                                                                           *
// *   0: Schema was added                                                     *
// *  -1: Schema was not added.  A CLI error is put into the diags area.       *
// *   1: Schema already exists and ignoreIfExists is specified.               *
// *      No error is added to the diags area.                                 *
// *                                                                           *
// *****************************************************************************
int CmpSeabaseDDL::addSchemaObject(
   ExeCliInterface & cliInterface,
   const ComSchemaName & schemaName,
   ComSchemaClass schemaClass,
   Int32 ownerID,
   NABoolean ignoreIfExists)
   
{

NAString catalogName = schemaName.getCatalogNamePartAsAnsiString();
ComAnsiNamePart schemaNameAsComAnsi = schemaName.getSchemaNamePart();
NAString schemaNamePart = schemaNameAsComAnsi.getInternalName();

ComObjectName objName(catalogName,schemaNamePart,NAString(SEABASE_SCHEMA_OBJECTNAME), 
                      COM_TABLE_NAME,TRUE);
                      
   if (isSeabaseReservedSchema(objName) &&
       !Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_RESERVED_METADATA_SCHEMA_NAME)
                          << DgSchemaName(schemaName.getExternalName().data());
      return -1;
   }
                      
NAString objectNamePart = objName.getObjectNamePartAsAnsiString(TRUE);

Lng32 retcode = existsInSeabaseMDTable(&cliInterface,catalogName,schemaNamePart, 
                                       objectNamePart, COM_UNKNOWN_OBJECT, FALSE);
   if (retcode < 0)
      return -1;
  
   if (retcode == 1 ) // already exists
   {
      if (ignoreIfExists)
        return 1;
      else
        *CmpCommon::diags() << DgSqlCode(-CAT_SCHEMA_ALREADY_EXISTS)
                            << DgSchemaName(schemaName.getExternalName().data());
      return -1;
   }

char buf[4000];

ComUID schemaUID;

   schemaUID.make_UID();
   
Int64 schemaObjectUID = schemaUID.get_value();
  
Int64 createTime = NA_JulianTimestamp();

NAString quotedSchName;
NAString quotedObjName;

   ToQuotedString(quotedSchName,schemaNamePart,FALSE);
   ToQuotedString(quotedObjName,NAString(SEABASE_SCHEMA_OBJECTNAME),FALSE);

char schemaObjectLit[3] = {0};
   
   switch (schemaClass)
   {
      case COM_SCHEMA_CLASS_PRIVATE:
      {
         strncpy(schemaObjectLit,COM_PRIVATE_SCHEMA_OBJECT_LIT,2);
         break;
      }
      case COM_SCHEMA_CLASS_SHARED:
      {
         strncpy(schemaObjectLit,COM_SHARED_SCHEMA_OBJECT_LIT,2);
         break;
      }
      case COM_SCHEMA_CLASS_DEFAULT:
      default:
      {
         // Schemas are private by default, but could choose a different
         // default class here based on CQD or other attribute.
         strncpy(schemaObjectLit,COM_PRIVATE_SCHEMA_OBJECT_LIT,2);
         break;
      } 
   }

   str_sprintf(buf, "insert into %s.\"%s\".%s values ('%s', '%s', '%s', '%s', %ld, %ld, %ld, '%s', '%s', %d, %d, 0)",
               getSystemCatalog(),SEABASE_MD_SCHEMA,SEABASE_OBJECTS,
               catalogName.data(), quotedSchName.data(), quotedObjName.data(),
               schemaObjectLit,
               schemaObjectUID,
               createTime, 
               createTime,
               COM_YES_LIT, // valid_def
               COM_NO_LIT,  // droppable
               ownerID,ownerID);
               
Int32 cliRC = cliInterface.executeImmediate(buf);
   
   if (cliRC < 0)
   {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
   }

   return 0;

}
//******************* End of CmpSeabaseDDL::addSchemaObject ********************

// *****************************************************************************
// *                                                                           *
// * Function: CmpSeabaseDDL::createSeabaseSchema                              *
// *                                                                           *
// *    Implements the CREATE SCHEMA command.                                  *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <createSchemaNode>              StmtDDLCreateSchema  *          In       *
// *    is a pointer to a create schema parser node.                           *
// *                                                                           *
// *  <currentCatalogName>            NAString &                      In       *
// *    is the name of the current catalog.                                    *
// *                                                                           *
// *****************************************************************************
void CmpSeabaseDDL::createSeabaseSchema(
   StmtDDLCreateSchema  * createSchemaNode,
   NAString             & currentCatalogName)
   
{
   ComSchemaName schemaName (createSchemaNode->getSchemaName());
  
   if (schemaName.getCatalogNamePart().isEmpty())
      schemaName.setCatalogNamePart(currentCatalogName);  
  
   NAString catName = schemaName.getCatalogNamePartAsAnsiString();
   ComAnsiNamePart schNameAsComAnsi = schemaName.getSchemaNamePart();
   NAString schName = schNameAsComAnsi.getInternalName();

   ExeCliInterface cliInterface(STMTHEAP, 0, NULL,
   CmpCommon::context()->sqlSession()->getParentQid());
   ComSchemaClass schemaClass;
   Int32 objectOwner = NA_UserIdDefault;
   Int32 schemaOwner = NA_UserIdDefault;


   // If creating the hive statistics schema, make owners
   // the HIVE_ROLE_ID and skip authorization check.
   // Schema is being created as part of an update statistics cmd
   if (schName == HIVE_STATS_SCHEMA_NO_QUOTES &&
       Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
   {
      objectOwner = HIVE_ROLE_ID;
      schemaOwner = HIVE_ROLE_ID;
   }
   else
   {
      int32_t retCode = verifyDDLCreateOperationAuthorized(&cliInterface,
                                                           SQLOperation::CREATE_SCHEMA,
                                                           catName,
                                                           schName,
                                                           schemaClass,
                                                           objectOwner,
                                                           schemaOwner);
      if (retCode != 0)
      {
         handleDDLCreateAuthorizationError(retCode,catName,schName);
         return;
      }
   }
   
   Int32 schemaOwnerID = NA_UserIdDefault; 

   // If the AUTHORIZATION clause was not specified, the current user becomes
   // the schema owner. 

   if (createSchemaNode->getAuthorizationID().isNull())
      schemaOwnerID = ComUser::getCurrentUser();
   else
      if (ComUser::getAuthIDFromAuthName(createSchemaNode->getAuthorizationID().data(),
                                         schemaOwnerID) != 0)
      {
         *CmpCommon::diags() << DgSqlCode(-CAT_AUTHID_DOES_NOT_EXIST_ERROR)
                             << DgString0(createSchemaNode->getAuthorizationID().data());
         return;
      }
   
   if (addSchemaObject(cliInterface,
                       schemaName,
                       createSchemaNode->getSchemaClass(),
                       schemaOwnerID,
                       createSchemaNode->createIfNotExists()))
     return;

   // Create histogram tables for schema, if the schema is not volatile and 
   // not reserved
   NAString tableNotCreated;

   if (!ComIsTrafodionReservedSchemaName(schName))
   {
      if (createHistogramTables(&cliInterface, schemaName.getExternalName(), 
                                FALSE, tableNotCreated))
      {
         *CmpCommon::diags() << DgSqlCode(-CAT_HISTOGRAM_TABLE_NOT_CREATED)
                             << DgTableName(tableNotCreated.data());
         return;
      }
   }

}
//***************** End of CmpSeabaseDDL::createSeabaseSchema ******************

// *****************************************************************************
// *                                                                           *
// * Function: CmpSeabaseDDL::describeSchema                                   *
// *                                                                           *
// *    Provides text for SHOWDDL SCHEMA comnmand.                             *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <catalogName>                   const NAString &                In       *
// *    is a reference to a catalog name.                                      *
// *                                                                           *
// *  <schemaName>                    const NAString &                In       *
// *    is a reference to a schema name.                                       *
// *                                                                           *
// *  <output>                        NAString &                      Out      *
// *    passes back text for the SHOWDDL SCHEMA command, specifically the      *
// *  command to create the specified schema.                                  *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true: Text returned for specified schema.                                 *
// * false: Could not retrieve information for specified schema.               *
// *                                                                           *
// *****************************************************************************
bool CmpSeabaseDDL::describeSchema(
   const NAString & catalogName,
   const NAString & schemaName,
   NABoolean isHiveRegistered,
   std::vector<std::string> & outlines)
   
{

ExeCliInterface cliInterface(STMTHEAP, NULL, NULL, 
CmpCommon::context()->sqlSession()->getParentQid());
ComSchemaClass schemaClass;
Int32 objectOwner;
Int32 schemaOwner;
ComObjectType objectType;

   NABoolean isHive = FALSE;
   NAString lcat(catalogName);
   lcat.toLower();
   if (lcat == HIVE_SYSTEM_CATALOG_LC)
     isHive = TRUE;

   NAString output;
   if (isHive)
     {
       output = "/* Hive DDL */";
       outlines.push_back(output.data());

       output = "CREATE SCHEMA HIVE.";
       NAString lsch(schemaName);
       output += lsch.data();
       output += ";";

       outlines.push_back(output.data());

       outlines.push_back(" ");

       if (isHiveRegistered)
         {
           output = "REGISTER /*INTERNAL*/ HIVE SCHEMA HIVE.";
           output += lsch.data();
           output += ";";
           
           outlines.push_back(output.data());
         }

       return true;
     }

   CmpSeabaseDDL cmpSBD(STMTHEAP);
   if (cmpSBD.switchCompiler())
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_PRIVS);
      return false;
   }

Int64 schemaUID = getObjectTypeandOwner(&cliInterface,
                                        catalogName.data(),
                                        schemaName.data(),
                                        SEABASE_SCHEMA_OBJECTNAME,
                                        objectType,
                                        objectOwner);
                                        
 if (schemaUID < 0)
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_SCHEMA_DOES_NOT_EXIST_ERROR)
                                  << DgString0(catalogName)
                                  << DgString1(schemaName);
      cmpSBD.switchBackCompiler();
      return false;
   }
      
char username[MAX_USERNAME_LEN+1];
Int32 lActualLen = 0;
Int16 status = ComUser::getAuthNameFromAuthID(objectOwner,username, 
                                              MAX_USERNAME_LEN+1,lActualLen);
   if (status != FEOK)
   {
      *CmpCommon::diags() << DgSqlCode(-20235) // Error converting user ID.
                          << DgInt0(status)
                          << DgInt1(objectOwner);
      cmpSBD.switchBackCompiler();
      return false;
   }
      
// Generate output text
   output = "CREATE ";
   switch (objectType)
     {
     case COM_PRIVATE_SCHEMA_OBJECT:
       output += "PRIVATE";
       break;
     case COM_SHARED_SCHEMA_OBJECT:
       output += "SHARED";
       break;
     default:
       return false;
     }
   output += " SCHEMA \"";
   output += catalogName.data();
   output += "\".\"";
   output += schemaName.data();

// AUTHORIZATION clause is rarely used, but include it for replay.
   output += "\" AUTHORIZATION \"";
   output += username;
   output += "\";";
   
   outlines.push_back(output.data());

   // Display Comment of schema
    {
      ComTdbVirtObjCommentInfo objCommentInfo;
      if (cmpSBD.getSeabaseObjectComment(schemaUID, objectType, objCommentInfo, STMTHEAP))
        {
          *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_RETRIEVE_COMMENTS);
          cmpSBD.switchBackCompiler();
          return -1;
        }

      if (objCommentInfo.objectComment != NULL)
        {
          outlines.push_back(" ");

          output = "COMMENT ON SCHEMA ";
          output += catalogName.data();
          output += ".";
          output += schemaName.data();
          output += " IS '";
          output += objCommentInfo.objectComment;
          output += "' ;";

          outlines.push_back(output.data());
        }
    }

   cmpSBD.switchBackCompiler();
   return true;
   
}
//******************* End of CmpSeabaseDDL::describeSchema *********************



// *****************************************************************************
// *                                                                           *
// * Function: CmpSeabaseDDL::dropSeabaseSchema                                *
// *                                                                           *
// *    Implements the DROP SCHEMA command.                                    *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <dropSchemaNode>                StmtDDLDropSchema *             In       *
// *    is a pointer to a create schema parser node.                           *
// *                                                                           *
// *****************************************************************************
void CmpSeabaseDDL::dropSeabaseSchema(StmtDDLDropSchema * dropSchemaNode)
   
{
   Lng32 cliRC = 0;

   ComSchemaName schemaName(dropSchemaNode->getSchemaName());
   NAString catName = schemaName.getCatalogNamePartAsAnsiString();
   ComAnsiNamePart schNameAsComAnsi = schemaName.getSchemaNamePart();
   NAString schName = schNameAsComAnsi.getInternalName();
   ComObjectName objName(catName,schName,NAString("dummy"),COM_TABLE_NAME,TRUE);

   ExeCliInterface cliInterface(STMTHEAP, 0, NULL, 
                                CmpCommon::context()->sqlSession()->getParentQid());
   Int32 objectOwnerID = 0;
   Int32 schemaOwnerID = 0;
   ComObjectType objectType;

   bool isVolatile = (memcmp(schName.data(),"VOLATILE_SCHEMA",strlen("VOLATILE_SCHEMA")) == 0);
   int32_t length = 0;
   Int64 rowCount = 0;
   bool someObjectsCouldNotBeDropped = false;
   char errorObjs[1010];
   Queue * objectsQueue = NULL;
   Queue * otherObjectsQueue = NULL;

   NABoolean dirtiedMetadata = FALSE;

   errorObjs[0] = 0;

   Int64 schemaUID = getObjectTypeandOwner(&cliInterface,catName.data(),schName.data(),
                               SEABASE_SCHEMA_OBJECTNAME,objectType,schemaOwnerID);
   
   // if schemaUID == -1, then either the schema does not exist or an unexpected error occurred
   if (schemaUID == -1)
   {
      // If an error occurred, return
      if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) > 0)
          goto label_error;
 
      // schema does not exist and IF EXISTS specified, then ignore and continue
      if (dropSchemaNode->dropIfExists())
        goto label_error;

      // A Trafodion schema does not exist if the schema object row is not
      // present: CATALOG-NAME.SCHEMA-NAME.__SCHEMA__.
      *CmpCommon::diags() << DgSqlCode(-CAT_SCHEMA_DOES_NOT_EXIST_ERROR)
                                  << DgString0(catName.data())
                                  << DgString1(schName.data());
      goto label_error;
   }

   if (!isDDLOperationAuthorized(SQLOperation::DROP_SCHEMA,
                                 schemaOwnerID,schemaOwnerID))
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
      goto label_error;
   }
 

   if ((isSeabaseReservedSchema(objName) ||
        (schName == SEABASE_SYSTEM_SCHEMA)) &&
       !Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_USER_CANNOT_DROP_SMD_SCHEMA)
                          << DgSchemaName(schemaName.getExternalName().data());
      goto label_error;
   }

   // Can't drop a schema whose name begins with VOLATILE_SCHEMA unless the 
   // keyword VOLATILE was specified in the DROP SCHEMA command. 
   if (isVolatile && !dropSchemaNode->isVolatile())
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_RESERVED_METADATA_SCHEMA_NAME)
                          << DgTableName(schName);
      goto label_error;
   }

   // Get a list of all objects in the schema, excluding the schema object itself.
   char query[4000];

   // select objects in the schema to drop, don't return PRIMARY_KEY_CONSTRAINTS,
   // they always get removed when the parent table is dropped.
   // Filter out the LOB depenedent tables too - they will get dropped when 
   //the main LOB table is dropped. 
   str_sprintf(query,"SELECT distinct TRIM(object_name), TRIM(object_type) "
                     "FROM %s.\"%s\".%s "
                     "WHERE catalog_name = '%s' AND schema_name = '%s' AND "
                     "object_name <> '" SEABASE_SCHEMA_OBJECTNAME"' AND "
                     "object_type <> 'PK' "
                     "FOR READ COMMITTED ACCESS",
               getSystemCatalog(),SEABASE_MD_SCHEMA,SEABASE_OBJECTS,
               (char*)catName.data(),(char*)schName.data());
  
   cliRC = cliInterface.fetchAllRows(objectsQueue, query, 0, FALSE, FALSE, TRUE);
   if (cliRC < 0)
   {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      goto label_error;
   }

   // Check to see if non histogram objects exist in schema, if so, then 
   // cascade is required
   if (dropSchemaNode->getDropBehavior() == COM_RESTRICT_DROP_BEHAVIOR)
   {
     objectsQueue->position();
     for (size_t i = 0; i < objectsQueue->numEntries(); i++)
     {
       OutputInfo * vi = (OutputInfo*)objectsQueue->getNext(); 
       NAString objName = vi->get(0);

       if (!isHistogramTable(objName))
       {
          OutputInfo * oi = (OutputInfo*)objectsQueue->getCurr(); 
      
          *CmpCommon::diags() << DgSqlCode(-CAT_SCHEMA_IS_NOT_EMPTY)
                              << DgTableName(objName.data());
          goto label_error;
       }
     }
   }

   // Drop procedures (SPJs), UDFs (functions), and views 
    objectsQueue->position();
    for (int idx = 0; idx < objectsQueue->numEntries(); idx++)
    {
       OutputInfo * vi = (OutputInfo*)objectsQueue->getNext(); 

       char * objName = vi->get(0);
       NAString objectTypeLit = vi->get(1);
       ComObjectType objectType = PrivMgr::ObjectLitToEnum(objectTypeLit.data());
       char buf[1000];
       NAString objectTypeString;
       NAString cascade = " ";
      
       switch (objectType)
       {
          // These object types are handled later and can be ignored for now.
          case COM_BASE_TABLE_OBJECT:
          case COM_INDEX_OBJECT:
          case COM_CHECK_CONSTRAINT_OBJECT:
          case COM_NOT_NULL_CONSTRAINT_OBJECT:
          case COM_REFERENTIAL_CONSTRAINT_OBJECT:
          case COM_SEQUENCE_GENERATOR_OBJECT:
          case COM_UNIQUE_CONSTRAINT_OBJECT:
          case COM_LIBRARY_OBJECT:
          {
             continue;
          }

          // If the library where procedures and functions reside is dropped
          // before its procedures and routines, then these objects may
          // not exist anymore, use the IF EXISTS to prevent the drop from
          // incurring errors.
          case COM_STORED_PROCEDURE_OBJECT:
          {
             objectTypeString = "PROCEDURE IF EXISTS ";
             break;
          }
          case COM_USER_DEFINED_ROUTINE_OBJECT:
          {
             objectTypeString = "FUNCTION IF EXISTS ";
             cascade = "CASCADE";
             break;
          }
          case COM_VIEW_OBJECT:
          {
             objectTypeString = "VIEW";
             cascade = "CASCADE";
             break;
          }
          // These object types should not be seen.
          case COM_MV_OBJECT: 
          case COM_MVRG_OBJECT:    
          case COM_TRIGGER_OBJECT:
          case COM_LOB_TABLE_OBJECT:
          case COM_TRIGGER_TABLE_OBJECT:
          case COM_SYNONYM_OBJECT:
          case COM_PRIVATE_SCHEMA_OBJECT:
          case COM_SHARED_SCHEMA_OBJECT:
          case COM_EXCEPTION_TABLE_OBJECT:
          case COM_LOCK_OBJECT:
          case COM_MODULE_OBJECT:
          default:
             SEABASEDDL_INTERNAL_ERROR("Unrecognized object type in schema");
             goto label_error;
       }
         
       dirtiedMetadata =  TRUE;
       str_sprintf(buf, "drop %s \"%s\".\"%s\".\"%s\" %s",
                   objectTypeString.data(),(char*)catName.data(),(char*)schName.data(), 
                   objName,cascade.data());
         
       cliRC = cliInterface.executeImmediate(buf);
       if (cliRC < 0 && cliRC != -CAT_OBJECT_DOES_NOT_EXIST_IN_TRAFODION)
         {
           appendErrorObjName(errorObjs, objName);
           if (dropSchemaNode->ddlXns())
             goto label_error;
           else
             someObjectsCouldNotBeDropped = true;
         }
   } 

   // Drop libraries in the schema
   objectsQueue->position();
   for (int idx = 0; idx < objectsQueue->numEntries(); idx++)
   {
      OutputInfo * vi = (OutputInfo*)objectsQueue->getNext();

      char * objName = vi->get(0);
      NAString objType = vi->get(1);

      if (objType == COM_LIBRARY_OBJECT_LIT)
      {
         char buf [1000];

         dirtiedMetadata = TRUE;
         str_sprintf(buf, "DROP LIBRARY \"%s\".\"%s\".\"%s\" CASCADE",
                     (char*)catName.data(), (char*)schName.data(), objName);
         cliRC = cliInterface.executeImmediate(buf);

         if (cliRC < 0 && cliRC != -CAT_OBJECT_DOES_NOT_EXIST_IN_TRAFODION)
           {
             appendErrorObjName(errorObjs, objName);
             if (dropSchemaNode->ddlXns())
               goto label_error;
             else
               someObjectsCouldNotBeDropped = true;

           }
      }
   }

   // Drop all tables in the schema.  This will also drop any associated constraints. 

   objectsQueue->position();
   for (int idx = 0; idx < objectsQueue->numEntries(); idx++)
     {
       OutputInfo * vi = (OutputInfo*)objectsQueue->getNext(); 

       NAString objName = vi->get(0);
       NAString objType = vi->get(1);

       // drop user objects first
       if (objType == COM_BASE_TABLE_OBJECT_LIT) 
	 {
	   // Histogram tables are dropped later. Sample tables
	   // are dropped when their corresponding tables are dropped
	   // so we don't need to drop them directly. Also,
	   // avoid any tables that match LOB dependent tablenames
	   // (there is no special type for these tables).
	   if (!isHistogramTable(objName) &&
               !isSampleTable(objName) &&
               !isLOBDependentNameMatch(objName))
	     {
	       dirtiedMetadata = TRUE;
	       if (dropOneTable(cliInterface,(char*)catName.data(), 
				(char*)schName.data(),(char*)objName.data(),
				isVolatile, FALSE,dropSchemaNode->ddlXns()))
                 {
                   appendErrorObjName(errorObjs, objName.data());
                   if (dropSchemaNode->ddlXns())
                     goto label_error;
                   else
                     someObjectsCouldNotBeDropped = true;
                 }
	     }
	 } 
     } 

   // If there are any user tables having the LOB dependent name pattern, they
   // will still be around. Drop those. The real LOB dependent tables, would
   //have been dropped in the previous step 
  

    objectsQueue->position();
   for (int idx = 0; idx < objectsQueue->numEntries(); idx++)
     {
       OutputInfo * vi = (OutputInfo*)objectsQueue->getNext(); 

       NAString objName = vi->get(0);
       NAString objType = vi->get(1);

       if (objType == COM_BASE_TABLE_OBJECT_LIT)
	 {
	   if (!isHistogramTable(objName) && isLOBDependentNameMatch(objName))
	     {
	       dirtiedMetadata = TRUE;
	       // Pass in TRUE for "ifExists" since the lobDependent tables 
	       // would have already been dropped and we don't want those to 
	       // raise errors. We just want to catch any user tables that 
	       // happen to have the same name patterns.
	       if (dropOneTable(cliInterface,(char*)catName.data(), 
				(char*)schName.data(),(char*)objName.data(),
				isVolatile,TRUE, dropSchemaNode->ddlXns()))
                 {
                   appendErrorObjName(errorObjs, objName.data());
                   if (dropSchemaNode->ddlXns())
                     goto label_error;
                   else
                     someObjectsCouldNotBeDropped = true;
                 }
	     }
	 } 
     } 
 
   // Drop any remaining indexes.

   str_sprintf(query,"SELECT TRIM(object_name), TRIM(object_type) "
                       "FROM %s.\"%s\".%s "
                       "WHERE catalog_name = '%s' AND "
                       "      schema_name = '%s' AND "
                       "      object_type = '%s' "
                       "FOR READ COMMITTED ACCESS ",
                 getSystemCatalog(),SEABASE_MD_SCHEMA,SEABASE_OBJECTS,
                 (char*)catName.data(),(char*)schName.data(), 
                 COM_INDEX_OBJECT_LIT);
   cliRC = cliInterface.fetchAllRows(otherObjectsQueue,query,0,FALSE,FALSE,TRUE);
   if (cliRC < 0)
   {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      goto label_error;
   }

   otherObjectsQueue->position();
   for (int idx = 0; idx < otherObjectsQueue->numEntries(); idx++)
   {
      OutputInfo * vi = (OutputInfo*)otherObjectsQueue->getNext(); 
      char * objName = vi->get(0);
      NAString objType = vi->get(1);
   
      if (objType == COM_INDEX_OBJECT_LIT)
      {
         char buf [1000];

         dirtiedMetadata = TRUE;
         str_sprintf(buf, "DROP INDEX \"%s\".\"%s\".\"%s\" CASCADE",
                     (char*)catName.data(), (char*)schName.data(), objName);
         cliRC = cliInterface.executeImmediate(buf);

         if (cliRC < 0 && cliRC != -CAT_OBJECT_DOES_NOT_EXIST_IN_TRAFODION)
           { 
             appendErrorObjName(errorObjs, objName);
             if (dropSchemaNode->ddlXns())
               goto label_error;
             else
               someObjectsCouldNotBeDropped = true;

           }
      }  
   }  

   // Drop any remaining sequences.

   str_sprintf(query,"SELECT TRIM(object_name), TRIM(object_type) "
                     "FROM %s.\"%s\".%s "
                     "WHERE catalog_name = '%s' AND "
                     "      schema_name = '%s' AND "
                     "      object_type = '%s' "
                     "FOR READ COMMITTED ACCESS ",
               getSystemCatalog(),SEABASE_MD_SCHEMA,SEABASE_OBJECTS,
               (char*)catName.data(),(char*)schName.data(), 
               COM_SEQUENCE_GENERATOR_OBJECT_LIT);
  
   cliRC = cliInterface.fetchAllRows(otherObjectsQueue,query,0,FALSE,FALSE,TRUE);
   if (cliRC < 0)
   {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      goto label_error;
   }

   otherObjectsQueue->position();
   for (int idx = 0; idx < otherObjectsQueue->numEntries(); idx++)
   {
      OutputInfo * vi = (OutputInfo*)otherObjectsQueue->getNext(); 
      char * objName = vi->get(0);
      NAString objType = vi->get(1);
    
      if (objType == COM_SEQUENCE_GENERATOR_OBJECT_LIT)
      {
         char buf [1000];

         dirtiedMetadata = TRUE;
         str_sprintf(buf, "DROP SEQUENCE \"%s\".\"%s\".\"%s\"",
                     (char*)catName.data(), (char*)schName.data(), objName);
         cliRC = cliInterface.executeImmediate(buf);

         if (cliRC < 0 && cliRC != -CAT_OBJECT_DOES_NOT_EXIST_IN_TRAFODION)
           {
             appendErrorObjName(errorObjs, objName);
             if (dropSchemaNode->ddlXns())
               goto label_error;
             else
               someObjectsCouldNotBeDropped = true;

           }
      }  
   }  

   // Drop histogram tables last
   objectsQueue->position();
   for (size_t i = 0; i < objectsQueue->numEntries(); i++)
   {
     OutputInfo * vi = (OutputInfo*)objectsQueue->getNext(); 
     NAString objName = vi->get(0);

     if (isHistogramTable(objName))
     {
       dirtiedMetadata = TRUE;
       if (dropOneTable(cliInterface,(char*)catName.data(),
                        (char*)schName.data(),(char*)objName.data(),
                        isVolatile, FALSE, dropSchemaNode->ddlXns()))
         {
           appendErrorObjName(errorObjs, objName.data());

           if (dropSchemaNode->ddlXns())
             goto label_error;
           else
             someObjectsCouldNotBeDropped = true;
         }
     }
   }

   if (someObjectsCouldNotBeDropped)
     {
       NAString reason;
       reason = "Reason: Some objects could not be dropped in schema " 
         + schName + ". ObjectsInSchema: " 
         + errorObjs;
       *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_DROP_SCHEMA)
                           << DgSchemaName(catName + "." + schName)
                           << DgString0(reason);
       goto label_error;
     }

   // For volatile schemas, sometimes only the objects get dropped.    
   // If the dropObjectsOnly flag is set, just exit now, we are done.
   if (dropSchemaNode->dropObjectsOnly())
      return;

   // Verify all objects in the schema have been dropped. 
   str_sprintf(query,"SELECT COUNT(*) "
                     "FROM %s.\"%s\".%s "
                     "WHERE catalog_name = '%s' AND schema_name = '%s' AND "
                     "object_name <> '" SEABASE_SCHEMA_OBJECTNAME"'" 
                     "FOR READ COMMITTED ACCESS",
               getSystemCatalog(),SEABASE_MD_SCHEMA,SEABASE_OBJECTS,
               (char*)catName.data(),(char*)schName.data());
               
   cliRC = cliInterface.executeImmediate(query,(char*)&rowCount,&length,FALSE);
  
   if (cliRC < 0)
   {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      goto label_error;
   }
   
   if (rowCount > 0)
     {
       CmpCommon::diags()->clear();
       
       str_sprintf(query,"SELECT TRIM(object_name) "
                   "FROM %s.\"%s\".%s "
                   "WHERE catalog_name = '%s' AND schema_name = '%s' AND "
                   "object_name <> '" SEABASE_SCHEMA_OBJECTNAME"' AND "
                   "object_type <> 'PK' "
                   "FOR READ COMMITTED ACCESS",
                   getSystemCatalog(),SEABASE_MD_SCHEMA,SEABASE_OBJECTS,
                   (char*)catName.data(),(char*)schName.data());
       
       cliRC = cliInterface.fetchAllRows(objectsQueue, query, 0, FALSE, FALSE, TRUE);
       if (cliRC < 0)
         {
           cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
           goto label_error;
         }     
       
       for (int i = 0; i < objectsQueue->numEntries(); i++)
         {
           OutputInfo * vi = (OutputInfo*)objectsQueue->getNext(); 
           NAString objName = vi->get(0);

           appendErrorObjName(errorObjs, objName.data());
         }

       NAString reason;
       reason = "Reason: schema " 
         + schName + " is not empty. ObjectsInSchema: " 
         + errorObjs;
       *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_DROP_SCHEMA)
                           << DgSchemaName(catName + "." + schName)
                           << DgString0(reason);
       goto label_error;
     }
 
   // After all objects in the schema have been dropped, drop the schema object itself.
    
   char buf [1000];

   dirtiedMetadata = TRUE;
   str_sprintf(buf,"DELETE FROM %s.\"%s\".%s "
                   "WHERE CATALOG_NAME = '%s' AND SCHEMA_NAME = '%s' AND " 
                   "OBJECT_NAME = '" SEABASE_SCHEMA_OBJECTNAME"'",
               getSystemCatalog(),SEABASE_MD_SCHEMA,SEABASE_OBJECTS,
               (char*)catName.data(),(char*)schName.data());
   cliRC = cliInterface.executeImmediate(buf);
   if (cliRC < 0 && cliRC != -CAT_OBJECT_DOES_NOT_EXIST_IN_TRAFODION) 
   {
     NAString reason;
     reason = "Reason: Delete of object " + 
       NAString(SEABASE_SCHEMA_OBJECTNAME) + " failed.";
      *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_DROP_SCHEMA)
                          << DgSchemaName(catName + "." + schName)
                          << DgString0(reason);
      goto label_error;
   }

   //Drop comment in TEXT table for schema
   str_sprintf(buf, "delete from %s.\"%s\".%s where text_uid = %ld",
               getSystemCatalog(),SEABASE_MD_SCHEMA,SEABASE_TEXT,
               schemaUID);
   cliRC = cliInterface.executeImmediate(buf);
   if (cliRC < 0)
     {
       cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
       goto label_error;
     }

  // Everything succeeded, return
  return;
    
label_error:
  // If metadata has not been changed, just return
  if (!dirtiedMetadata)
  {
    return;
  }
   
  // Add an error asking for user to cleanup schema
  *CmpCommon::diags() << DgSqlCode(-CAT_ATTEMPT_CLEANUP_SCHEMA)
                      << DgSchemaName(catName + "." + schName);

  return;
}
//****************** End of CmpSeabaseDDL::dropSeabaseSchema *******************


// *****************************************************************************
// *                                                                           *
// * Function: CmpSeabaseDDL::alterSeabaseSchema                                *
// *                                                                           *
// *    Implements the ALTER SCHEMA command.                                    *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <alterSchemaNode>                StmtDDLAlterSchema *             In       *
// *    is a pointer to a create schema parser node.                           *
// *                                                                           *
// *****************************************************************************
void CmpSeabaseDDL::alterSeabaseSchema(StmtDDLAlterSchema * alterSchemaNode)
   
{
   Lng32 cliRC = 0;

   ComSchemaName schemaName(alterSchemaNode->getSchemaName());
   NAString catName = schemaName.getCatalogNamePartAsAnsiString();
   ComAnsiNamePart schNameAsComAnsi = schemaName.getSchemaNamePart();
   NAString schName = schNameAsComAnsi.getInternalName();
   ComObjectName objName(catName,schName,NAString("dummy"),COM_TABLE_NAME,TRUE);

   ExeCliInterface cliInterface(STMTHEAP, 0, NULL, 
                                CmpCommon::context()->sqlSession()->getParentQid());
   Int32 objectOwnerID = 0;
   Int32 schemaOwnerID = 0;
   ComObjectType objectType;

   bool isVolatile = (memcmp(schName.data(),"VOLATILE_SCHEMA",strlen("VOLATILE_SCHEMA")) == 0);
   int32_t length = 0;
   Int64 rowCount = 0;
   bool someObjectsCouldNotBeAltered = false;
   char errorObjs[1010];
   Queue * objectsQueue = NULL;
   Queue * otherObjectsQueue = NULL;

   NABoolean dirtiedMetadata = FALSE;
   Int32 checkErr = 0;

   StmtDDLAlterTableStoredDesc::AlterStoredDescType sdo = 
     alterSchemaNode->getStoredDescOperation();

   errorObjs[0] = 0;

   Int64 schemaUID = getObjectTypeandOwner(&cliInterface,catName.data(),schName.data(),
                               SEABASE_SCHEMA_OBJECTNAME,objectType,schemaOwnerID);
   
   // if schemaUID == -1, then either the schema does not exist or an unexpected error occurred
   if (schemaUID == -1)
   {
      // If an error occurred, return
      if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) > 0)
          goto label_error;
 
      // A Trafodion schema does not exist if the schema object row is not
      // present: CATALOG-NAME.SCHEMA-NAME.__SCHEMA__.
      *CmpCommon::diags() << DgSqlCode(-CAT_SCHEMA_DOES_NOT_EXIST_ERROR)
                                  << DgString0(catName.data())
                                  << DgString1(schName.data());
      goto label_error;
   }

   if (!isDDLOperationAuthorized(SQLOperation::ALTER_SCHEMA,
                                 schemaOwnerID,schemaOwnerID))
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
      goto label_error;
   }

   if ((isSeabaseReservedSchema(objName) ||
        (schName == SEABASE_SYSTEM_SCHEMA)) &&
       !Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_USER_CANNOT_DROP_SMD_SCHEMA)
                          << DgSchemaName(schemaName.getExternalName().data());
      goto label_error;
   }

   // Can't alter a schema whose name begins with VOLATILE_SCHEMA unless the 
   // keyword VOLATILE was specified in the ALTER SCHEMA command. 
   if (isVolatile && !alterSchemaNode->isVolatile())
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_RESERVED_METADATA_SCHEMA_NAME)
                          << DgTableName(schName);
      goto label_error;
   }

   if (alterSchemaNode->isDropAllTables())
     {
       // should not reach here, is transformed to DropSchema during parsing
       *CmpCommon::diags() << DgSqlCode(-3242)
                           << DgString0("Should not reach here. Should have been transformed to DropSchema during parsing.");
      goto label_error;
     }

   if (alterSchemaNode->isRenameSchema())
     {
       // Not yet supported
       *CmpCommon::diags() << DgSqlCode(-3242)
                           << DgString0("Cannot rename a schema.");
      goto label_error;
     }

   if (NOT alterSchemaNode->isAlterStoredDesc())
     {
       // unsupported option
       *CmpCommon::diags() << DgSqlCode(-3242)
                           << DgString0("Unsupported option specified.");
      goto label_error;
     }

   // Get a list of all objects in the schema, excluding the schema object itself.
   char query[4000];

   // select objects in the schema to alter
   str_sprintf(query,"SELECT distinct TRIM(object_name), TRIM(object_type), object_uid "
                     "FROM %s.\"%s\".%s "
                     "WHERE catalog_name = '%s' AND schema_name = '%s' AND "
                     "object_name <> '" SEABASE_SCHEMA_OBJECTNAME"' AND "
                     "(object_type = 'BT' OR "
                     " object_type = 'VI') "
                     "FOR READ COMMITTED ACCESS",
               getSystemCatalog(),SEABASE_MD_SCHEMA,SEABASE_OBJECTS,
               (char*)catName.data(),(char*)schName.data());
  
   cliRC = cliInterface.fetchAllRows(objectsQueue, query, 0, FALSE, FALSE, TRUE);
   if (cliRC < 0)
   {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      goto label_error;
   }

   objectsQueue->position();
   for (int idx = 0; idx < objectsQueue->numEntries(); idx++)
     {
       OutputInfo * vi = (OutputInfo*)objectsQueue->getNext(); 

       NAString objName = vi->get(0);
       NAString objType = vi->get(1);
       Int64 objUID = *(Int64*)vi->get(2);
       
       if (sdo == StmtDDLAlterTableStoredDesc::GENERATE)
         {
           cliRC = 
             updateObjectRedefTime(&cliInterface, 
                                   getSystemCatalog(), schName, objName,
                                   COM_BASE_TABLE_OBJECT_LIT,
                                   -1, objUID, TRUE);
           if (cliRC < 0)
             {
               // append error and move on to next one
               appendErrorObjName(errorObjs, objName);
               someObjectsCouldNotBeAltered = true;               

               CmpCommon::diags()->clear();
             }
         }
       else if (sdo == StmtDDLAlterTableStoredDesc::DELETE)
         {
           cliRC = deleteFromTextTable
             (&cliInterface, objUID, COM_STORED_DESC_TEXT, 0);
           if (cliRC < 0)
             {
               // append error and move on to next one
               appendErrorObjName(errorObjs, objName);
               someObjectsCouldNotBeAltered = true;  

               CmpCommon::diags()->clear();
             }    
           
         } // delete stored desc
       else if (sdo == StmtDDLAlterTableStoredDesc::ENABLE)
         {
           Int64 flags = MD_OBJECTS_DISABLE_STORED_DESC;
           cliRC = updateObjectFlags(&cliInterface, objUID, flags, TRUE);
           if (cliRC < 0)
             {
               appendErrorObjName(errorObjs, objName);
               someObjectsCouldNotBeAltered = true;  

               CmpCommon::diags()->clear();
             }    
         }
       else if (sdo == StmtDDLAlterTableStoredDesc::DISABLE)
         {
           Int64 flags = MD_OBJECTS_DISABLE_STORED_DESC;
           cliRC = updateObjectFlags(&cliInterface, objUID, flags, FALSE);
           if (cliRC < 0)
             {
               appendErrorObjName(errorObjs, objName);
               someObjectsCouldNotBeAltered = true;  

               CmpCommon::diags()->clear();
             }    
         }
       else if (sdo == StmtDDLAlterTableStoredDesc::CHECK)
         {
           cliRC = checkAndGetStoredObjectDesc(&cliInterface, objUID, NULL);
           CmpCommon::diags()->clear();
           if (cliRC < 0)
             {
               checkErr = cliRC;
               appendErrorObjName(errorObjs, objName);
               someObjectsCouldNotBeAltered = true;  
             }    
         }
       
     } // for

   if (someObjectsCouldNotBeAltered)
     {
       NAString reason;
       if (sdo == StmtDDLAlterTableStoredDesc::CHECK)
         {
           reason = "Reason: Following objects failed stored descriptor check";
           if (checkErr == -1)
             reason += " (object could not be accessed) ";
           else if (checkErr == -2)
             reason += " (object does not exist) ";
           else if (checkErr == -3)
             reason += " (change in stored structures) ";
           reason += ": ";
           reason += errorObjs;
         }
       else 
         reason = "Reason: Some objects could not be accessed in schema " 
           + schName + ". ObjectsInSchema: " 
           + errorObjs;
       *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_ALTER_SCHEMA)
                           << DgSchemaName(catName + "." + schName)
                           << DgString0(reason);
       goto label_error;
     }

  // Everything succeeded, return
  return;
    
label_error:
  return;
}
//****************** End of CmpSeabaseDDL::alterSeabaseSchema *****************


// *****************************************************************************
// *                                                                           *
// * Function: CmpSeabaseDDL::giveSeabaseSchema                                *
// *                                                                           *
// *    Implements the GIVE SCHEMA command.                                    *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <giveSchemaNode>                StmtDDLGiveSchema *             In       *
// *    is a pointer to a create schema parser node.                           *
// *                                                                           *
// *  <currentCatalogName>            NAString &                      In       *
// *    is the name of the current catalog.                                    *
// *                                                                           *
// *****************************************************************************
void CmpSeabaseDDL::giveSeabaseSchema(
   StmtDDLGiveSchema * giveSchemaNode,
   NAString          & currentCatalogName)
   
{

ComDropBehavior dropBehavior = giveSchemaNode->getDropBehavior(); 
NAString catalogName = giveSchemaNode->getCatalogName();
NAString schemaName = giveSchemaNode->getSchemaName();

   if (catalogName.isNull())
      catalogName = currentCatalogName;  

ExeCliInterface cliInterface(STMTHEAP, 0, NULL,
CmpCommon::context()->sqlSession()->getParentQid());
Int32 objectOwnerID = 0;
Int32 schemaOwnerID = 0;
ComObjectType objectType;

Int64 schemaUID = getObjectTypeandOwner(&cliInterface,catalogName.data(),
                                        schemaName.data(),SEABASE_SCHEMA_OBJECTNAME,
                                        objectType,schemaOwnerID);
                                       
   if (schemaUID == -1)
   {
      // A Trafodion schema does not exist if the schema object row is not
      // present: CATALOG-NAME.SCHEMA-NAME.__SCHEMA__.
      *CmpCommon::diags() << DgSqlCode(-CAT_SCHEMA_DOES_NOT_EXIST_ERROR)
                                  << DgString0(catalogName.data())
                                  << DgString1(schemaName.data());
      return;
   }
   
// *****************************************************************************
// *                                                                           *
// *    A schema owner can give their own schema to another authID, but they   *
// * cannot give the objects in a shared schema to another authID.  Only       *
// * DB__ROOT or a user with the ALTER_SCHEMA privilege can change the owners  *
// * of objects in a shared schema.  So if the schema is private, or if only   *
// * the schema is being given, we do standard authentication checking.  But   *
// * if giving all the objects in a shared schema, we change the check ID to   *
// * the default user to force the ALTER_SCHEMA privilege check.               *
// *                                                                           *
// *****************************************************************************

int32_t checkID = schemaOwnerID;

   if (objectType == COM_SHARED_SCHEMA_OBJECT && 
       dropBehavior == COM_CASCADE_DROP_BEHAVIOR)
      checkID = NA_UserIdDefault; 

   if (!isDDLOperationAuthorized(SQLOperation::ALTER_SCHEMA,checkID,checkID))
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
      return;
   }
 
ComObjectName objName(catalogName,schemaName,NAString("dummy"),COM_TABLE_NAME,TRUE);

   if (isSeabaseReservedSchema(objName) &&
       !Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_USER_CANNOT_DROP_SMD_SCHEMA)
                          << DgSchemaName(schemaName.data());
      return;
   }
   
bool isVolatile = (memcmp(schemaName.data(),"VOLATILE_SCHEMA",strlen("VOLATILE_SCHEMA")) == 0);

// Can't give a schema whose name begins with VOLATILE_SCHEMA. 
   if (isVolatile)
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_RESERVED_METADATA_SCHEMA_NAME)
                          << DgTableName(schemaName);
      return;
   }
   
int32_t newOwnerID = -1;

   if (ComUser::getAuthIDFromAuthName(giveSchemaNode->getAuthID().data(),
                                      newOwnerID) != 0)
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_AUTHID_DOES_NOT_EXIST_ERROR)
                          << DgString0(giveSchemaNode->getAuthID().data());
      return;
   }

// *****************************************************************************
// *                                                                           *
// *   Drop behavior is only relevant for shared schemas.  For shared schemas, *
// * ownership of the schema OR the schema and all its objects may be given to *
// * another authorization ID.  For private schemas, all objects are owned by  *
// * the schema owner, so the drop behavior is always CASCADE.                 *
// *                                                                           *
// * NOTE: The syntax for drop behavior always defaults to RESTRICT; for       *
// *       private schemas this is simply ignored, as opposed to requiring     *
// *       users to always specify CASCASE.                                    *
// *                                                                           *
// *****************************************************************************

Lng32 cliRC = 0;
char buf[4000];
   
   if (objectType == COM_SHARED_SCHEMA_OBJECT && 
       dropBehavior == COM_RESTRICT_DROP_BEHAVIOR)
   {
      str_sprintf(buf,"UPDATE %s.\"%s\".%s "
                      "SET object_owner = %d "
                      "WHERE object_UID = %ld",
                  getSystemCatalog(),SEABASE_MD_SCHEMA,SEABASE_OBJECTS,
                  newOwnerID,schemaUID);
      cliRC = cliInterface.executeImmediate(buf);
      if (cliRC < 0)
         cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
                      
      return;
   }
//
// At this point, we are giving all objects in the schema (as well as the 
// schema itself) to the new authorization ID.  If authentication is enabled,
// update the privileges first.
//
   if (isAuthorizationEnabled())
   {
      int32_t rc = transferObjectPrivs(getSystemCatalog(),catalogName.data(),
                                       schemaName.data(),newOwnerID,
                                       giveSchemaNode->getAuthID().data());
      if (rc != 0)
      {
         if (CmpCommon::diags()->getNumber(DgSqlCode::ERROR_) == 0)
         {
          //TODO: add error
         
         }
         return;
      }
   }
   
// Now update the object owner for all objects in the schema.
      
   str_sprintf(buf,"UPDATE %s.\"%s\".%s "
                   "SET object_owner = %d "
                   "WHERE catalog_name = '%s' AND schema_name = '%s'",
               getSystemCatalog(),SEABASE_MD_SCHEMA,SEABASE_OBJECTS,
               newOwnerID,catalogName.data(),schemaName.data());
   cliRC = cliInterface.executeImmediate(buf);
   if (cliRC < 0)
   {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return;
   }             

// Verify all objects in the schema have been given to the new owner.   
   str_sprintf(buf,"SELECT COUNT(*) "
                   "FROM %s.\"%s\".%s "
                   "WHERE catalog_name = '%s' AND schema_name = '%s' AND "
                   "object_name <> '" SEABASE_SCHEMA_OBJECTNAME"' AND "
                   "object_owner <> %d " 
                   "FOR READ COMMITTED ACCESS",
               getSystemCatalog(),SEABASE_MD_SCHEMA,SEABASE_OBJECTS,
               catalogName.data(),schemaName.data(),newOwnerID);
               
int32_t length = 0;
Int64 rowCount = 0;

   cliRC = cliInterface.executeImmediate(buf,(char*)&rowCount,&length,FALSE);
  
   if (cliRC < 0)
   {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return;
   }
   
   if (rowCount > 0)
   {
      SEABASEDDL_INTERNAL_ERROR("Not all objects in schema were given");
      return;
   }
    
}
//****************** End of CmpSeabaseDDL::giveSeabaseSchema *******************



// *****************************************************************************
//    Private/static functions
// *****************************************************************************


// *****************************************************************************
// *                                                                           *
// * Function: createHistogramTables                                           *
// *                                                                           *
// *    Creates all the histogram tables                                       *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <cliInterface>                  ExeCliInterface *               In       *
// *    is a reference to an Executor CLI interface handle.                    *
// *                                                                           *
// *  <schemaName>                    NAString &                      In       *
// *    is the catalog.schema of the histogram table to create.                *
// *                                                                           *
// *  <ignoreIfExists>                NABoolean                       In       *
// *    do not return an error if table already exists                         *
// *                                                                           *
// *  <tableNotCreeated>              NAString &                      Out      *
// *    returns the name of first histogram table that could not be created    *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: Int32                                                            *
// *                                                                           *
// * -mainSQLCODE: Could not create histogram tables.                          *
// *            0: Create was successful.                                      *
// *                                                                           *
// *****************************************************************************
short CmpSeabaseDDL::createHistogramTables(
  ExeCliInterface *cliInterface,
  const NAString &schemaName,
  const NABoolean ignoreIfExists,
  NAString &tableNotCreated)
{
   Int32 cliRC = 0;
   tableNotCreated = "";

   // allMDHistInfo (CmpSeabaseDDLmd.h) is the list of all histogram tables,
   // MDTableInfo describes the table attributes,
   // create each table found in the list
   Int32 numHistTables = sizeof(allMDHistInfo) / sizeof(MDTableInfo);
   NAString prefixText = ignoreIfExists ? "IF NOT EXISTS " : "";
   for (Int32 i = 0; i < numHistTables; i++)
   {
      const MDTableInfo &mdh = allMDHistInfo[i];
      Int32 qryArraySize = mdh.sizeOfnewDDL / sizeof(QString);

      // Concatenate the create table text into a single string
      NAString concatenatedQuery;
      for (Int32 j = 0; j < qryArraySize; j++)
      {
         NAString tempStr = mdh.newDDL[j].str;
         concatenatedQuery += tempStr.strip(NAString::leading, ' ');
      }

      // qualify create table text with (optional) "IF NOT EXISTS" & schema name
      // and place in front of the table name:
      //     "create table <textInsertion> hist-table ..."
      std::string tableDDL (concatenatedQuery.data());
      NAString textInsertion = prefixText + schemaName + ".";
      size_t pos = tableDDL.find_first_of(mdh.newName);
      if (pos == string::npos)
      {
        NAString errorText ("Unexpected error occurred while parsing create text for histogram table ");
        errorText += mdh.newName;
        SEABASEDDL_INTERNAL_ERROR(errorText.data());
        tableNotCreated = mdh.newName;
        return -CAT_INTERNAL_EXCEPTION_ERROR;
      }
      tableDDL = tableDDL.insert(pos, textInsertion.data());

      // If the caller does not send in cliInterface, instantiate one now
      ExeCliInterface cli; 
      if (cliInterface == NULL)
      {
         ExeCliInterface newCli(STMTHEAP, NULL, NULL,
            CmpCommon::context()->sqlSession()->getParentQid());
         cli = newCli;
      }
      else
         cli = *cliInterface;

      // Create the table
      cliRC = cli.executeImmediate(tableDDL.c_str());
      if (cliRC < 0)
      {
        cli.retrieveSQLDiagnostics(CmpCommon::diags());
        tableNotCreated = mdh.newName;
        return cliRC;
      }
   }
   return 0;
}
//************************ End of createHistogramTables ************************


// *****************************************************************************
// *                                                                           *
// * Function: adjustHiveExternalSchemas                                       *
// *                                                                           *
// *    Changes the ownership and privilege grants to DB__HIVEROLE             *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <cliInterface>                  ExeCliInterface *               In       *
// *    is a reference to an Executor CLI interface handle.                    *
// *****************************************************************************
// *                                                                           *
// * Returns: Int32                                                            *
// *                                                                           *
// *            0: Adjustment was successful                                   *
// *           -1: Adjustment failed                                           *
// *                                                                           *
// *****************************************************************************
short CmpSeabaseDDL::adjustHiveExternalSchemas(ExeCliInterface *cliInterface)
{
  char buf[sizeof(SEABASE_MD_SCHEMA) + 
           sizeof(SEABASE_OBJECTS) + 
           strlen(getSystemCatalog()) + 300];

  // get all the objects in special hive schemas
  sprintf(buf, "SELECT catalog_name, schema_name, object_name, object_uid, object_type, object_owner "
               " from %s.\"%s\".%s WHERE schema_name like '_HV_%c_'",
               getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS, '%');

   Queue * objectsQueue = NULL;
   Int32 cliRC = cliInterface->fetchAllRows(objectsQueue, buf, 0, FALSE, FALSE, TRUE);
   if (cliRC < 0)
   {
      cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
      return -1;
   }

   // adjust owner and privilege information for external hive objects
   objectsQueue->position();
   for (size_t i = 0; i < objectsQueue->numEntries(); i++)
   {
     OutputInfo * vi = (OutputInfo*)objectsQueue->getNext();
     NAString catName = vi->get(0);
     NAString schName = vi->get(1);
     NAString objName = vi->get(2);
     Int64 objUID     = *(Int64*)vi->get(3);
     NAString objectTypeLit = vi->get(4);
     Int32 objOwner   = *(Int32*)vi->get(5);
     ComObjectType objType = PrivMgr::ObjectLitToEnum(objectTypeLit.data());

     // If object owner is already the HIVE_ROLE_ID, then we are done.
     if (objOwner == HIVE_ROLE_ID)
       continue;
     else
     {
       // only need to adjust privileges on securable items
       if (PrivMgr::isSecurableObject(objType))
       {
         ComObjectName tblName(catName, schName, objName, COM_TABLE_NAME, 
                               ComAnsiNamePart::INTERNAL_FORMAT, STMTHEAP);

         NAString extTblName = tblName.getExternalName(TRUE);

         // remove existing privs on object
         if (!deletePrivMgrInfo(extTblName, objUID, objType))
           return -1;

         // add owner privs
         if (!insertPrivMgrInfo(objUID, extTblName, objType, 
                                HIVE_ROLE_ID, HIVE_ROLE_ID, ComUser::getCurrentUser()))
           return -1;
       }

       // update schema_owner and objectOwner for object
       sprintf(buf,"UPDATE %s.\"%s\".%s SET object_owner = %d "
                   ", schema_owner = %d WHERE object_uid = %ld ",
                   getSystemCatalog(),SEABASE_MD_SCHEMA,SEABASE_OBJECTS, 
                   HIVE_ROLE_ID, HIVE_ROLE_ID, objUID);
       cliRC = cliInterface->executeImmediate(buf);
       if (cliRC < 0)
       {
         cliInterface->retrieveSQLDiagnostics(CmpCommon::diags());
         return -1;
       }
     }
  }

  return 0;
}
//********************* End of adjustHiveExternalTables ************************

// *****************************************************************************
// *                                                                           *
// * Function: dropOneTable                                                    *
// *                                                                           *
// *    Drops a table and all its dependent objects.                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <cliInterface>                  ExeCliInterface &               In       *
// *    is a reference to an Executor CLI interface handle.                    *
// *                                                                           *
// *  <catalogName>                   const char *                    In       *
// *    is the catalog of the table to drop.                                   *
// *                                                                           *
// *  <schemaName>                    const char *                    In       *
// *    is the schema of the table to drop.                                    *
// *                                                                           *
// *  <objectName>                    const char *                    In       *
// *    is the name of the table to drop.                                      *
// *                                                                           *
// *  <isVolatile>                    bool                            In       *
// *    is true if the object is volatile or part of a volatile schema.        *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true: Could not drop table or one of its dependent objects.               *
// * false: Drop successful or could not set CQD for NATable cache reload.     *
// *                                                                           *
// *****************************************************************************
static bool dropOneTable(
   ExeCliInterface & cliInterface,
   const char * catalogName, 
   const char * schemaName, 
   const char * objectName,
   bool isVolatile,
   bool ifExists,
   bool ddlXns)
   
{

char buf [1000];

bool someObjectsCouldNotBeDropped = false;

char volatileString[20] = {0};
 char ifExistsString[20] = {0};
Lng32 cliRC = 0;



   if (isVolatile && 
       strcmp(objectName, HBASE_HIST_NAME) != 0 && 
       strcmp(objectName, HBASE_HISTINT_NAME) != 0 && 
       strcmp(objectName, HBASE_PERS_SAMP_NAME) != 0)
      strcpy(volatileString,"VOLATILE");

   if (ifExists)
    strcpy(ifExistsString,"IF EXISTS");

   if (ComIsTrafodionExternalSchemaName(schemaName))
     str_sprintf(buf,"DROP EXTERNAL TABLE \"%s\" FOR \"%s\".\"%s\".\"%s\" CASCADE",
                 objectName,catalogName,schemaName,objectName);
   else
     str_sprintf(buf,"DROP %s  TABLE %s \"%s\".\"%s\".\"%s\" CASCADE",
                 volatileString, ifExistsString, catalogName,schemaName,objectName);
 
ULng32 savedParserFlags = Get_SqlParser_Flags(0xFFFFFFFF);

   try
   {            
      Set_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL);
               
      cliRC = cliInterface.executeImmediate(buf);
   }
   catch (...)
   {
      // Restore parser flags settings to what they originally were
      Assign_SqlParser_Flags(savedParserFlags);
      
      throw;
   }
   
// Restore parser flags settings to what they originally were
   Assign_SqlParser_Flags(savedParserFlags);

   if (cliRC < 0 && cliRC != -CAT_OBJECT_DOES_NOT_EXIST_IN_TRAFODION)
     {
       cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());

       someObjectsCouldNotBeDropped = true;
     }

// remove NATable entry for this table
   CorrName cn(objectName,STMTHEAP,schemaName,catalogName);

   ActiveSchemaDB()->getNATableDB()->removeNATable
     (cn,
      ComQiScope::REMOVE_FROM_ALL_USERS, COM_BASE_TABLE_OBJECT,
      ddlXns, FALSE);

   return someObjectsCouldNotBeDropped;
   
}
//**************************** End of dropOneTable *****************************

// *****************************************************************************
// *                                                                           *
// * Function: transferObjectPrivs                                             *
// *                                                                           *
// *    Transfers object privs from current owner to new owner.                *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <systemCatalogName>             const char *                    In       *
// *    is the location of the system catalog.                                 *
// *                                                                           *
// *  <catalogName>                   const char *                    In       *
// *    is the catalog of the schema whose objects are getting a new owner.    *
// *                                                                           *
// *  <schemaName>                    const char *                    In       *
// *    is the schema whose objects are getting a new owner.                   *
// *                                                                           *
// *  <newOwnerID>                    const int32_t                   In       *
// *    is the ID of the new owner for the objects.                            *
// *                                                                           *
// *  <newOwnerName                   const char *                    In       *
// *    is the database username or role name of the new owner for the objects.*
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true: Privileges for object(s) transferred to new owner.                  *
// * false: Privileges for object(s) NOT transferred to new owner.             *
// *                                                                           *
// *****************************************************************************
static bool transferObjectPrivs(
   const char * systemCatalogName, 
   const char * catalogName,
   const char * schemaName,
   const int32_t newOwnerID,
   const char * newOwnerName)
   
   
{

PrivStatus privStatus = STATUS_GOOD;
      
// Initiate the privilege manager interface class
NAString privMgrMDLoc;

   CONCAT_CATSCH(privMgrMDLoc,systemCatalogName,SEABASE_PRIVMGR_SCHEMA);
   
PrivMgrCommands privInterface(std::string(privMgrMDLoc.data()),CmpCommon::diags());
   
std::vector<UIDAndOwner> objectRows;
std::string whereClause(" WHERE catalog_name = '");
   
   whereClause += catalogName;
   whereClause += "' AND schema_name = '";
   whereClause += schemaName;
   whereClause += "'";
   
std::string orderByClause(" ORDER BY OBJECT_OWNER");
std::string metadataLocation(systemCatalogName);  
      
   metadataLocation += ".\"";
   metadataLocation += SEABASE_MD_SCHEMA;
   metadataLocation += "\"";
      
PrivMgrObjects objects(metadataLocation,CmpCommon::diags());
   
   privStatus = objects.fetchUIDandOwner(whereClause,orderByClause,objectRows); 

   if (privStatus != STATUS_GOOD || objectRows.size() == 0)
      return false;
   
int32_t lastOwner = objectRows[0].ownerID;
std::vector<int64_t> objectUIDs;

   for (size_t i = 0; i < objectRows.size(); i++)
   {
      if (objectRows[i].ownerID != lastOwner)
      {
         privStatus = privInterface.givePrivForObjects(lastOwner,
                                                       newOwnerID,
                                                       newOwnerName,
                                                       objectUIDs);
                                                       
         objectUIDs.clear();
      }
      objectUIDs.push_back(objectRows[i].UID);
      lastOwner = objectRows[i].ownerID;
   }
   
   privStatus = privInterface.givePrivForObjects(lastOwner,
                                                 newOwnerID,
                                                 newOwnerName,
                                                 objectUIDs);
                                                 
   return true;                                             

}
//************************ End of transferObjectPrivs **************************

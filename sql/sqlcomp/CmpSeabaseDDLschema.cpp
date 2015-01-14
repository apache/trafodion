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
#include <vector>

static bool dropOneTable(
   ExeCliInterface & cliInterface,
   const char * catalogName, 
   const char * schemaName, 
   const char * objectName,
   bool isVolatile);

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
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// *   0: Schema as added                                                      *
// *  -1: Schema was not added.  A CLI error is put into the diags area.       *
// *                                                                           *
// *****************************************************************************
int CmpSeabaseDDL::addSchemaObject(
   ExeCliInterface & cliInterface,
   const ComSchemaName & schemaName,
   ComSchemaClass schemaClass,
   Int32 ownerID)
   
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
                                       objectNamePart);
   if (retcode < 0)
      return -1;
  
   if (retcode == 1) // already exists
   {
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
   
   str_sprintf(buf, "insert into %s.\"%s\".%s values ('%s', '%s', '%s', '%s', %Ld, %Ld, %Ld, '%s', '%s', %d, %d, 0)",
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

ExeCliInterface cliInterface(STMTHEAP);
ComSchemaClass schemaClass;
Int32 objectOwner = NA_UserIdDefault;
Int32 schemaOwner = NA_UserIdDefault;

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
   
   addSchemaObject(cliInterface,
                   schemaName,
                   createSchemaNode->getSchemaClass(),
                   schemaOwnerID);

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
   NAString & output)
   
{

ExeCliInterface cliInterface(STMTHEAP);
ComSchemaClass schemaClass;
Int32 objectOwner;
Int32 schemaOwner;
ComObjectType objectType;

Int64 schemaUID = getObjectTypeandOwner(&cliInterface,
                                        catalogName.data(),
                                        schemaName.data(),
                                        SEABASE_SCHEMA_OBJECTNAME,
                                        objectType,
                                        objectOwner);
                                        
   if (schemaUID < 0)
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_SCHEMA_DOES_NOT_EXIST_ERROR)
                          << DgSchemaName(catalogName + "." + schemaName);
      return false;
   }
      
char username[MAX_USERNAME_LEN+1];
Int32 lActualLen = 0;
Int16 status = ComUser::getAuthNameFromAuthID(objectOwner,username, 
                                              MAX_USERNAME_LEN,lActualLen);
   if (status != FEOK)
   {
      *CmpCommon::diags() << DgSqlCode(-20235) // Error converting user ID.
                          << DgInt0(status)
                          << DgInt1(objectOwner);
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
// *  <dropSchemaNode>                StmtDDLCreateSchema  *          In       *
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

ExeCliInterface cliInterface(STMTHEAP);
Int32 objectOwnerID = 0;
Int32 schemaOwnerID = 0;
ComObjectType objectType;

   if (getObjectTypeandOwner(&cliInterface,catName.data(),schName.data(),
                             SEABASE_SCHEMA_OBJECTNAME,objectType,schemaOwnerID))
   {
      // A Trafodion schema does not exist if the schema object row is not
      // present: CATALOG-NAME.SCHEMA-NAME.__SCHEMA__.
      *CmpCommon::diags() << DgSqlCode(-CAT_SCHEMA_DOES_NOT_EXIST_ERROR)
                          << DgSchemaName(schemaName.getExternalName().data());
      return;
   }

   if (!isDDLOperationAuthorized(SQLOperation::DROP_SCHEMA,
                                 schemaOwnerID,schemaOwnerID))
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
      return;
   }
 
ComObjectName objName(catName,schName,NAString("dummy"),COM_TABLE_NAME,TRUE);

   if (isSeabaseReservedSchema(objName) &&
       !Get_SqlParser_Flags(INTERNAL_QUERY_FROM_EXEUTIL))
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_USER_CANNOT_DROP_SMD_SCHEMA)
                          << DgSchemaName(schemaName.getExternalName().data());
      return;
   }
   
bool isVolatile = (memcmp(schName.data(),"VOLATILE_SCHEMA",strlen("VOLATILE_SCHEMA")) == 0);

// Can't drop a schema whose name begins with VOLATILE_SCHEMA unless the 
// keyword VOLATILE was specified in the DROP SCHEMA command. 
   if (isVolatile && !dropSchemaNode->isVolatile())
   {
      *CmpCommon::diags() << DgSqlCode(-CAT_RESERVED_METADATA_SCHEMA_NAME)
                          << DgTableName(schName);
      return;
   }

// Get a list of all objects in the schema, excluding the schema object itself.
char query[4000];

   str_sprintf(query,"SELECT TRIM(object_name), TRIM(object_type) "
                     "FROM %s.\"%s\".%s "
                     "WHERE catalog_name = '%s' AND schema_name = '%s' AND "
                     "object_name <> '"SEABASE_SCHEMA_OBJECTNAME"'" 
                     "FOR READ COMMITTED ACCESS",
               getSystemCatalog(),SEABASE_MD_SCHEMA,SEABASE_OBJECTS,
               (char*)catName.data(),(char*)schName.data());
  
Queue * objectsQueue = NULL;

   cliRC = cliInterface.fetchAllRows(objectsQueue, query, 0, FALSE, FALSE, TRUE);
   if (cliRC < 0)
   {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return;
   }

   objectsQueue->position();
   if ((dropSchemaNode->getDropBehavior() == COM_RESTRICT_DROP_BEHAVIOR) &&
       (objectsQueue->numEntries() > 0))
   {
      OutputInfo * oi = (OutputInfo*)objectsQueue->getCurr(); 
      
      *CmpCommon::diags() << DgSqlCode(-CAT_SCHEMA_IS_NOT_EMPTY)
                          << DgTableName(oi->get(0));
      return;
   }

bool someObjectsCouldNotBeDropped = false;

// Drop libraries, sequence generators, procedures (SPJs), UDFs (functions) and views 
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
         case COM_PRIMARY_KEY_CONSTRAINT_OBJECT:
         case COM_REFERENTIAL_CONSTRAINT_OBJECT:
         case COM_UNIQUE_CONSTRAINT_OBJECT:
         {
            continue;
         }
         case COM_LIBRARY_OBJECT:
         {
            objectTypeString = "LIBRARY";
            cascade = "CASCADE";
            break;
         }
         case COM_SEQUENCE_GENERATOR_OBJECT:
         {
            objectTypeString = "SEQUENCE";
            break;
         }
         case COM_STORED_PROCEDURE_OBJECT:
         {
            objectTypeString = "PROCEDURE";
            break;
         }
         case COM_USER_DEFINED_ROUTINE_OBJECT:
         {
            objectTypeString = "FUNCTION";
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
            return;
      }
         
      str_sprintf(buf, "drop %s \"%s\".\"%s\".\"%s\" %s",
                  objectTypeString.data(),(char*)catName.data(),(char*)schName.data(), 
                  objName,cascade.data());
         
      cliRC = cliInterface.executeImmediate(buf);
      if (cliRC < 0 && cliRC != -CAT_OBJECT_DOES_NOT_EXIST_IN_TRAFODION)
         someObjectsCouldNotBeDropped = true;
   } 

// Drop all tables in the schema.  This will also drop any associated constraints. 
// Drop of histogram tables is deferred.
bool histExists = false;

   objectsQueue->position();
   for (int idx = 0; idx < objectsQueue->numEntries(); idx++)
   {
      OutputInfo * vi = (OutputInfo*)objectsQueue->getNext(); 

      NAString objName = vi->get(0);
      NAString objType = vi->get(1);

      // drop user objects first
      if (objType == COM_BASE_TABLE_OBJECT_LIT)
      {
         if (!(objName == HBASE_HIST_NAME || objName == HBASE_HISTINT_NAME))
         {
            if (dropOneTable(cliInterface,(char*)catName.data(), 
                             (char*)schName.data(),(char*)objName.data(),
                             isVolatile))
               someObjectsCouldNotBeDropped = true;
         }
         else
            histExists = true;
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
   
   cliRC = cliInterface.fetchAllRows(objectsQueue,query,0,FALSE,FALSE,TRUE);
   if (cliRC < 0)
   {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return;
   }

   objectsQueue->position();
   for (int idx = 0; idx < objectsQueue->numEntries(); idx++)
   {
      OutputInfo * vi = (OutputInfo*)objectsQueue->getNext(); 

      char * objName = vi->get(0);
      NAString objType = vi->get(1);
    
      if (objType == COM_INDEX_OBJECT_LIT)
      {
         char buf [1000];

         str_sprintf(buf, "DROP INDEX \"%s\".\"%s\".\"%s\" CASCADE",
                     (char*)catName.data(), (char*)schName.data(), objName);
         cliRC = cliInterface.executeImmediate(buf);

         if (cliRC < 0 && cliRC != -CAT_OBJECT_DOES_NOT_EXIST_IN_TRAFODION)
            someObjectsCouldNotBeDropped = true;
      }  
   }  

// For volatile schemas, sometimes only the objects get dropped.    
// If the dropObjectsOnly flag is set, just exit now, we are done.
   if (dropSchemaNode->dropObjectsOnly())
      return;

// Now drop any histogram objects
   if (histExists)
   {
      if (dropOneTable(cliInterface,(char*)catName.data(),(char*)schName.data(), 
                      (char*)HBASE_HISTINT_NAME,false))
         someObjectsCouldNotBeDropped = true;
      
      if (dropOneTable(cliInterface,(char*)catName.data(),(char*)schName.data(), 
                       (char*)HBASE_HIST_NAME,false))
         someObjectsCouldNotBeDropped = true;
   }

   if (someObjectsCouldNotBeDropped)
   {
      CmpCommon::diags()->clear();
      
      *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_DROP_SCHEMA)
                          << DgSchemaName(catName + "." + schName);
      return;
   }
   
// If all objects in the schema have been dropped, drop the schema object itself.
    
char buf [1000];

   str_sprintf(buf,"DELETE FROM %s.\"%s\".%s "
                   "WHERE CATALOG_NAME = '%s' AND SCHEMA_NAME = '%s' AND " 
                   "OBJECT_NAME = '"SEABASE_SCHEMA_OBJECTNAME"'",
               getSystemCatalog(),SEABASE_MD_SCHEMA,SEABASE_OBJECTS,
               (char*)catName.data(),(char*)schName.data());
   cliRC = cliInterface.executeImmediate(buf);
   if (cliRC < 0) 
      *CmpCommon::diags() << DgSqlCode(-CAT_UNABLE_TO_DROP_SCHEMA)
                          << DgSchemaName(catName + "." + schName);
    
}
//****************** End of CmpSeabaseDDL::dropSeabaseSchema *******************



// *****************************************************************************
//    Private/static functions
// *****************************************************************************


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
   bool isVolatile)
   
{

char buf [1000];

Lng32 cliRC = cliInterface.holdAndSetCQD("TRAF_RELOAD_NATABLE_CACHE", "ON");

   if (cliRC < 0)
   {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      
      return false;
   }

bool someObjectsCouldNotBeDropped = false;

char volatileString[20] = {0};

   if (isVolatile)
      strcpy(volatileString,"VOLATILE");

   str_sprintf(buf,"DROP %s TABLE \"%s\".\"%s\".\"%s\" CASCADE",
               volatileString,catalogName,schemaName,objectName);
   cliRC = cliInterface.executeImmediate(buf);
   
   if (cliRC < 0 && cliRC != -CAT_OBJECT_DOES_NOT_EXIST_IN_TRAFODION)
      someObjectsCouldNotBeDropped = true;
   
// remove NATable entry for this table
CorrName cn(catalogName,STMTHEAP,schemaName,objectName);

   ActiveSchemaDB()->getNATableDB()->removeNATable(cn,
     NATableDB::REMOVE_FROM_ALL_USERS, COM_BASE_TABLE_OBJECT);
   cliRC = cliInterface.restoreCQD("TRAF_RELOAD_NATABLE_CACHE");

   return someObjectsCouldNotBeDropped;
   
}
//**************************** End of dropOneTable *****************************


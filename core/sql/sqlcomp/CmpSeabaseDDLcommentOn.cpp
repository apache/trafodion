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
 * File:         CmpSeabaseDDLcommentOn.cpp
 * Description:  Implements ddl operations for Seabase indexes.
 *
 *
 * Created:     8/17/2017
 * Language:     C++
 *
 *
 *****************************************************************************
 */

#define   SQLPARSERGLOBALS_FLAGS	// must precede all #include's
#define   SQLPARSERGLOBALS_NADEFAULTS

#include "ComObjectName.h"

#include "CmpDDLCatErrorCodes.h"
#include "ElemDDLHbaseOptions.h"

#include "SchemaDB.h"
#include "CmpSeabaseDDL.h"
#include "CmpDescribe.h"

#include "ExpHbaseInterface.h"

#include "ExExeUtilCli.h"
#include "Generator.h"

#include "ComCextdecs.h"
#include "ComUser.h"

#include "NumericType.h"

#include "PrivMgrCommands.h"

#include "StmtDDLCommentOn.h"

#include "PrivMgrComponentPrivileges.h"
#include "PrivMgrCommands.h"
#include "ComUser.h"


static char *doEscapeComment(char *src, CollHeap *heap)
{
  char *ret = NULL;
  int head = 0;
  int tail = 0;
  int src_len = strlen(src);

  if (NULL == src)
  {
    NAAssert("invalid comment string fetched", __FILE__ , __LINE__ );
  }

  ret = new (heap) char[src_len * 2 + 1];
  ret[0] = '\0';
  for (; tail <= src_len; tail++)
  {
    if (src[tail] == '\'')
    {
      strncat(ret, src+head, tail-head+1);
      strcat(ret, "'");
      head = tail + 1;
    }
    else if (src[tail] == '\0')
    {
      strncat(ret, src+head, tail-head+1);
    }
  }

  return ret;
}

short CmpSeabaseDDL::getSeabaseObjectComment(Int64 object_uid, 
                                                    enum ComObjectType object_type, 
                                                    ComTdbVirtObjCommentInfo & comment_info,
                                                    CollHeap * heap)
{
  Lng32 retcode = 0;
  Lng32 cliRC = 0;

  char query[4000];

  comment_info.objectUid = object_uid;
  comment_info.objectComment = NULL;
  comment_info.numColumnComment = 0;
  comment_info.columnCommentArray = NULL;
  comment_info.numIndexComment = 0;
  comment_info.indexCommentArray = NULL;

  ExeCliInterface cliInterface(STMTHEAP, NULL, NULL, 
                               CmpCommon::context()->sqlSession()->getParentQid());

  //get object comment
  sprintf(query, "select TEXT from %s.\"%s\".%s where TEXT_UID = %ld and TEXT_TYPE = %d and SUB_ID = %d ; ",
                 getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TEXT,
                 object_uid, COM_OBJECT_COMMENT_TEXT, 0);

  Queue * objQueue = NULL;
  cliRC = cliInterface.fetchAllRows(objQueue, query, 0, FALSE, FALSE, TRUE);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      processReturn();
      return -1;
    }

  //We should have only 1 comment for object
  if (objQueue->numEntries() > 0)
    {
      objQueue->position();
      OutputInfo * vi = (OutputInfo*)objQueue->getNext();
      comment_info.objectComment = doEscapeComment((char*)vi->get(0), heap);
    }

  //get index comments of table
  if (COM_BASE_TABLE_OBJECT == object_type)
    {
      sprintf(query, "select O.CATALOG_NAME||'.'||O.SCHEMA_NAME||'.'||O.OBJECT_NAME as INDEX_QUAL, T.TEXT "
                        "from %s.\"%s\".%s as O, %s.\"%s\".%s as T, %s.\"%s\".%s as I "
                        "where I.BASE_TABLE_UID = %ld and O.OBJECT_UID = I.INDEX_UID and T.TEXT_UID = O.OBJECT_UID "
                        "  and T.TEXT_TYPE = %d and SUB_ID = %d "
                        "order by INDEX_QUAL ; ",
                     getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_OBJECTS,
                     getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TEXT,
                     getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_INDEXES,
                     object_uid, COM_OBJECT_COMMENT_TEXT, 0);

      Queue * indexQueue = NULL;
      cliRC = cliInterface.fetchAllRows(indexQueue, query, 0, FALSE, FALSE, TRUE);
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
          processReturn();
          return -1;
        }

      if (indexQueue->numEntries() > 0)
        {
          comment_info.numIndexComment = indexQueue->numEntries();
          comment_info.indexCommentArray = new(heap) ComTdbVirtIndexCommentInfo[comment_info.numIndexComment];

          indexQueue->position();
          for (Lng32 idx = 0; idx < comment_info.numIndexComment; idx++)
          {
            OutputInfo * oi = (OutputInfo*)indexQueue->getNext(); 
            ComTdbVirtIndexCommentInfo &indexComment = comment_info.indexCommentArray[idx];

            // get the index full name
            indexComment.indexFullName = (char*) oi->get(0);
            indexComment.indexComment = doEscapeComment((char*) oi->get(1), heap);
          }
       }
    }

  //get column comments of table and view
  if (COM_BASE_TABLE_OBJECT == object_type || COM_VIEW_OBJECT == object_type)
    {
      sprintf(query, "select C.COLUMN_NAME, T.TEXT from %s.\"%s\".%s as C, %s.\"%s\".%s as T "
                     "where C.OBJECT_UID = %ld and T.TEXT_UID = C.OBJECT_UID and T.TEXT_TYPE = %d and C.COLUMN_NUMBER = T.SUB_ID "
                     "order by C.COLUMN_NUMBER ; ",
                     getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_COLUMNS,
                     getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TEXT,
                     object_uid, COM_COLUMN_COMMENT_TEXT);

      Queue * colQueue = NULL;
      cliRC = cliInterface.fetchAllRows(colQueue, query, 0, FALSE, FALSE, TRUE);
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
          processReturn();
          return -1;
        }

      if (colQueue->numEntries() > 0)
        {
          comment_info.numColumnComment = colQueue->numEntries();
          comment_info.columnCommentArray = new(heap) ComTdbVirtColumnCommentInfo[comment_info.numColumnComment];

          colQueue->position();
          for (Lng32 idx = 0; idx < comment_info.numColumnComment; idx++)
            {
              OutputInfo * oi = (OutputInfo*)colQueue->getNext(); 
              ComTdbVirtColumnCommentInfo &colComment = comment_info.columnCommentArray[idx];

              // get the column name
              colComment.columnName = (char*) oi->get(0);
              colComment.columnComment = doEscapeComment((char*) oi->get(1), heap);
            }
       }
    }

  return 0;
}

void  CmpSeabaseDDL::doSeabaseCommentOn(StmtDDLCommentOn   *commentOnNode,
                                                NAString &currCatName, 
                                                NAString &currSchName)
{
  Lng32 cliRC;
  Lng32 retcode;

  enum ComObjectType enMDObjType = COM_UNKNOWN_OBJECT;

  ComObjectName objectName(commentOnNode->getObjectName());
  ComAnsiNamePart currCatAnsiName(currCatName);
  ComAnsiNamePart currSchAnsiName(currSchName);
  objectName.applyDefaults(currCatAnsiName, currSchAnsiName);

  enum StmtDDLCommentOn::COMMENT_ON_TYPES commentObjectType = commentOnNode->getObjectType();

  switch (commentObjectType)
    {
        case StmtDDLCommentOn::COMMENT_ON_TYPE_TABLE:
            enMDObjType = COM_BASE_TABLE_OBJECT;
            break;

        case StmtDDLCommentOn::COMMENT_ON_TYPE_COLUMN:
            if (TRUE == commentOnNode->getIsViewCol())
              {
                enMDObjType = COM_VIEW_OBJECT;
              }
            else
              {
                enMDObjType = COM_BASE_TABLE_OBJECT;
              }
            break;

        case StmtDDLCommentOn::COMMENT_ON_TYPE_INDEX:
            enMDObjType = COM_INDEX_OBJECT;
            break;

        case StmtDDLCommentOn::COMMENT_ON_TYPE_SCHEMA:
            enMDObjType = COM_PRIVATE_SCHEMA_OBJECT;
            break;

        case StmtDDLCommentOn::COMMENT_ON_TYPE_VIEW:
            enMDObjType = COM_VIEW_OBJECT;
            break;

        case StmtDDLCommentOn::COMMENT_ON_TYPE_LIBRARY:
            enMDObjType = COM_LIBRARY_OBJECT;
            break;

        case StmtDDLCommentOn::COMMENT_ON_TYPE_PROCEDURE:
        case StmtDDLCommentOn::COMMENT_ON_TYPE_FUNCTION:
            enMDObjType = COM_USER_DEFINED_ROUTINE_OBJECT;
            break;

        case StmtDDLCommentOn::COMMENT_ON_TYPE_SEQUENCE:
            enMDObjType = COM_SEQUENCE_GENERATOR_OBJECT;

        default:
            break;

    }

  NAString catalogNamePart = objectName.getCatalogNamePartAsAnsiString();
  NAString schemaNamePart = objectName.getSchemaNamePartAsAnsiString(TRUE);
  NAString objNamePart = objectName.getObjectNamePartAsAnsiString(TRUE);

  const NAString extObjName = objectName.getExternalName(TRUE);

  ExeCliInterface cliInterface(STMTHEAP, NULL, NULL,
                                       CmpCommon::context()->sqlSession()->getParentQid());
  Int64 objUID = 0;
  Int32 objectOwnerID = ROOT_USER_ID;
  Int32 schemaOwnerID = ROOT_USER_ID;
  Int64 objectFlags = 0;

  //get UID of object
  objUID = getObjectInfo(&cliInterface,
                              catalogNamePart.data(), schemaNamePart.data(), objNamePart.data(), 
                              enMDObjType,
                              objectOwnerID,
                              schemaOwnerID,
                              objectFlags);
  if (objUID < 0 || objectOwnerID == 0 || schemaOwnerID == 0)
    {
      CmpCommon::diags()->clear();
      *CmpCommon::diags() << DgSqlCode(-1389)
                          << DgString0(extObjName);
      processReturn();
      return;
    }

  // Verify that the requester has COMMENT privilege.
  if (!isDDLOperationAuthorized(SQLOperation::COMMENT, schemaOwnerID, schemaOwnerID))
    {
      *CmpCommon::diags() << DgSqlCode(-CAT_NOT_AUTHORIZED);
      processReturn ();
      return;
    }


  //check for overflow
  if (commentOnNode->getComment().length() > COM_MAXIMUM_LENGTH_OF_COMMENT)
    {
      *CmpCommon::diags() << DgSqlCode(-8402);
      processReturn ();
      return;
    }

  NAString comment = commentOnNode->getCommentEscaped();

  // add, remove, change comment of object/column
  enum ComTextType textType = COM_OBJECT_COMMENT_TEXT;
  Lng32 subID = 0;

  if (StmtDDLCommentOn::COMMENT_ON_TYPE_COLUMN == commentObjectType)
    {
      textType = COM_COLUMN_COMMENT_TEXT;
      subID = commentOnNode->getColNum();
    }

  /* Not using function updateTextTable(), because can not insert Chinese properly by function updateTextTable().
     * For storing COMMENT in TEXT table is a temp solution, so updating TEXT table directly here.
     * Will change this implementation until next upgrade of MD.
     */
  //like function updateTextTable(), delete entry first
  cliRC = deleteFromTextTable(&cliInterface, objUID, textType, subID);
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      processReturn();
      return;
    }

  if (comment.length() > 0)
    {
      //add or modify comment
      char query[2048];

      str_sprintf(query, "insert into %s.\"%s\".%s values (%ld, %d, %d, %d, 0, '%s') ; ",
              getSystemCatalog(), SEABASE_MD_SCHEMA, SEABASE_TEXT,
              objUID, 
              textType, 
              subID, 
              0,
              comment.data());
      cliRC = cliInterface.executeImmediate(query);
      
      if (cliRC < 0)
        {
          cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
          processReturn();
          return;
        }
    }

  processReturn();
  return;
}


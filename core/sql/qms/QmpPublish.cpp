// **********************************************************************
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
// **********************************************************************

#include "QmpPublish.h"
#include <Int64.h>
#include "QRQueries.h"
#include "QueryRewriteServer.h"
#include "DefaultConstants.h"

// *************************************************************** 
// *************************************************************** 
NABoolean QmpPublish::setTarget(PublishTarget target, const char* targetFilename)
{
  target_  = target;

  switch(target)
  {
    case PUBLISH_TO_QMM:
        server_ = MvQueryRewriteServer::getQmmServer();
        if (!server_)
        {
          QRLogger::log(CAT_QR_IPC, LL_ERROR, 
            "Unable to obtain QMM server.");
          return FALSE;
        }
        targetName_ = "QMM";
        break;

    case PUBLISH_TO_QMS:
        server_ = MvQueryRewriteServer::getQmsServer(DF_PRIVATE);
        if (!server_)
        {
          QRLogger::log(CAT_QR_IPC, LL_ERROR,
            "Unable to obtain QMS server.");
          return FALSE;
        }
        targetName_ = "QMS";
        break;

    case PUBLISH_TO_FILE:
        outFile_ = new ofstream(targetFilename);
        if (!outFile_->rdbuf()->is_open())
        {
          QRLogger::log(CAT_QMP, LL_ERROR,
            "Can't open output file ", targetFilename);
          return FALSE;
        }
        targetName_ = targetFilename;
        break;
  }

  return TRUE;
}


// *************************************************************** 
// * Read from the REWRITE_PUBLISH table using a delete stream cursor.
// *
// * A static SQL query is used to perform
// * a stream delete read from the REWRITE_PUBLISH table.
// *
// * performRewritePublishReading() will open and
// * fetch from the ObtainRewritePublish cursor.
// *
// * Only an error during fetch will cause the ObtainRewritePublish cursor
// * to be closed.
// *************************************************************** 
void QmpPublish::performRewritePublishReading()
{
  MVQR_Publish publishData;

  try
  {
    // Start a transaction
    sqlInterface_.beginTransaction();

    // Fetch from the REWRITE_PUBLISH delete stream cursor
    NABoolean wasTimeout = FALSE;
    while (!wasTimeout)
    {
      // open the cursor
      sqlInterface_.openRewritePublishCursor();

      // Fetch and delete rows from the stream
      wasTimeout = sqlInterface_.fetchRewritePublishCursor(&publishData);

      /* send publish message to QMM, if we have any rows */
      if (!wasTimeout)
        preparePublishRewritePublishRowToSend(&publishData);

      // We are done processing the delete from stream
      sqlInterface_.closeRewritePublishCursor();

      // commit the transaction and start a new one.
      QRLogger::log(CAT_QMP, LL_INFO, "chain to next transaction...");
      sqlInterface_.commitTransaction();
      sqlInterface_.beginTransaction();

      /* check system defaults table based on a periodic indicator*/

      /* Any other housekeeping needed */
    }

    // commit the last transaction.
    sqlInterface_.commitTransaction();
  }
  catch(QRDatabaseException ex)
  {
    // Handle database errors here.
    sqlInterface_.rollbackTransaction();
    QRLogger::log(CAT_QMP, LL_ERROR, 
      "Error reading PUBLISH table from database - exiting.");
    exit(0);
  }
}  // End of performRewritePublishReading

// *************************************************************** 
// * 
// *
// *************************************************************** 
void QmpPublish::preparePublishRewritePublishRowToSend(MVQR_Publish* publishData)
{
  // Get the catalog name for the CATALOG_UID
  const NAString* catalogName = 
    sqlInterface_.getCatalogName(publishData->catalogUID_);

  // Get the schema definition version for this catalog
  const NAString* version = 
    sqlInterface_.getSchemaVersion(publishData->catalogUID_);
  if (version == NULL)
  {
    QRLogger::log(CAT_QMP, LL_ERROR,
      "Not expecting an empty catalog.");
    return;
  }
  //assertLogAndThrow(version != NULL, QRDatabaseException, 
  //		    "Not expecting an empty catalog."); 

  char convert[21];
  convertInt64ToAscii(publishData->objectUID_, convert);

  NAString* textTable = 
    sqlInterface_.prepareDefinitionSchemaName(catalogName, version);
  *textTable += ".TEXT";

  // Get all the MV descriptor text and attributes
  // for the MV object UID from the REWRITE_PUBLISH row.  
  // The MV descriptor text is allocated here and deleted as 
  // part of the Publish descriptor.
  NAString* mvDescriptorText = 
    sqlInterface_.getMvDescriptor(*textTable, convert);

  // prepare the XML  
  QRPublishDescriptorPtr pubDesc = 
    createPublishDescriptor(publishData, mvDescriptorText);
  XMLFormattedString pubDescString;
  pubDesc->toXML(pubDescString);

  if (target_ == PUBLISH_TO_FILE)
  {
    *outFile_ << pubDescString;
  }
  else
  {
    QRLogger::log(CAT_QR_IPC, LL_INFO, "Sending publish message...");
    QR::QRRequestResult resultCode = 
      MvQueryRewriteServer::sendPublishMessage(&pubDescString, targetName_,
                                               server_, getNAHeap());
  }

  // Cleanup all resources handling these descriptor text buffers
  deletePtr(pubDesc);
  delete catalogName;
} // End of preparePublishRewritePublishRowToSend

// *************************************************************** 
// * Create the Publish descriptor including all the sub elements
// * and details.
// *************************************************************** 
QRPublishDescriptorPtr QmpPublish::createPublishDescriptor(MVQR_Publish* publishData,
                                                           NAString* mvDescriptorText)
{
  NAMemory* heap = getNAHeap();
  QRPublishDescriptorPtr pubDesc = new(getNAHeap()) 
    QRPublishDescriptor(ADD_MEMCHECK_ARGS(heap));
    
  char redefTimestampChar[21];
  char refreshedAtTimestampChar[21];
  convertInt64ToAscii(publishData->redefTime_, redefTimestampChar);
  convertInt64ToAscii(publishData->refreshedAt_, refreshedAtTimestampChar);
  NAString redefTimestamp(redefTimestampChar, heap);
  NAString refreshedAtTimestamp(refreshedAtTimestampChar, heap);

  const NAString& objectName = publishData->objectName_;
  const NAString& objectNewName =
	  ( publishData->nullindObjectNewName_ < 0 ?
	    "" : publishData->objectNewName_);

  NABoolean ignoreChangesUsed = FALSE;
  if (publishData->ignoreChangesUsed_ && strcmp(publishData->ignoreChangesUsed_, "1") == 0)
    ignoreChangesUsed = TRUE;

  pubDesc->initialize(convertOperationType(publishData->operationType_),
		      &redefTimestamp,
		      mvDescriptorText, 
		      &objectName,
		      ignoreChangesUsed,
		      &refreshedAtTimestamp,
		      &objectNewName,
		      heap);

  return pubDesc;
}

// *************************************************************** 
// * Convert the character string to the MVQR operation type.
// *
// *************************************************************** 
ComPublishMVOperationType QmpPublish::convertOperationType(char * operation)
{
  if (operation == NULL)
    return COM_PUBLISH_MV_UNKNOWN;

  if (strncmp(operation,COM_PUBLISH_MV_CREATE_LIT,2) == 0)
    return COM_PUBLISH_MV_CREATE;
  else if (strncmp(operation,COM_PUBLISH_MV_CREATE_AND_REFRESH_LIT,2) == 0)
    return COM_PUBLISH_MV_CREATE_AND_REFRESH;
  else if (strncmp(operation,COM_PUBLISH_MV_DROP_LIT,2) == 0)
    return COM_PUBLISH_MV_DROP;
  else if (strncmp(operation,COM_PUBLISH_MV_REFRESH_LIT,2) == 0)
    return COM_PUBLISH_MV_REFRESH;
  else if (strncmp(operation,COM_PUBLISH_MV_REFRESH_RECOMPUTE_LIT,2) == 0)
    return COM_PUBLISH_MV_REFRESH_RECOMPUTE;
  else if (strncmp(operation,COM_PUBLISH_MV_RENAME_LIT,2) == 0)
    return COM_PUBLISH_MV_RENAME;
  else if (strncmp(operation,COM_PUBLISH_MV_ALTER_IGNORE_CHANGES_LIT,2) == 0)
    return COM_PUBLISH_MV_ALTER_IGNORE_CHANGES;
  else if (strncmp(operation,COM_PUBLISH_MV_TOUCH_LIT,2) == 0)
    return COM_PUBLISH_MV_TOUCH;
  else if (strncmp(operation,COM_PUBLISH_MV_REPUBLISH_LIT,2) == 0)
    return COM_PUBLISH_MV_REPUBLISH;
  else if (strncmp(operation,COM_PUBLISH_MV_UNKNOWN_LIT,2) == 0)
    return COM_PUBLISH_MV_UNKNOWN;
  else
    return COM_PUBLISH_MV_UNKNOWN;

}  // End of convertOperationType


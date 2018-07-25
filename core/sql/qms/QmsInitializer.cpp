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

#include <Int64.h>
#include "QmsQms.h"
#include "XMLUtil.h"
#include "QRDescriptor.h"
#include "QRLogger.h"
#include "QmsInitializer.h"

/**
 * \file
 * Contains the main() function for the qms executable, which processes the
 * input file presented as a command-line argument, or, when launched by QMM as
 * a server process, enters a loop in which it waits for requests from client
 * processes (either QMM or MXCMP). Requests are processed using a number of
 * static, nonmember functions defined in this file, as well as classes derived
 * from QRRequest.
 */


// ***************************************************************
// * performInitialization:
// *
// * Initialize the QMS process with MV Descriptor information.
// *
// * Helper routines are used to execute static SQL queries
// * to obtain all the CATALOG names known to the system.
// *
// * The individual CATALOG names are then processed to
// * obtain individual MV UIDs and MV descriptor text for
// * all MVs known to a catalog.
// *************************************************************** 

QmsInitializer* QmsInitializer::instance_ = NULL;

Lng32 QmsInitializer::performInitialization()
{
  // Ignore request if initialization has already been done.
  if (qms_->isInitialized())
    {
      QRLogger::log(CAT_MEMORY, LL_WARN,
                    "QmsInitializer::performInitialization(): "
                    "Extraneous QMS initialization attempted");
      return SQL_Success;
    }

  // start a transaction, unless one is already in progress
  NABoolean startedTran = FALSE;
  if (SQL_EXEC_Xact(SQLTRANS_STATUS, 0) != 0)
    {
      sqlInterface_.beginTransaction();
      startedTran = TRUE;
    }

  // check if stats need to be collected - used for testing
  NABoolean collectQMSStats = sqlInterface_.getCollectQMSStatsEnabled();
  if (collectQMSStats)
  {
    collectQMSStats_ = TRUE;

    // report initial heap size of QMS
    logQMSStats ();
  }

  // Obtain all the MV descriptor text for all the MVs known for
  // each CATALOG on the system.

  const NAStringList* catalogNames = sqlInterface_.getAllCatalogNames();

  // Process the catalogs looking for MVs
  processCatalogs(catalogNames);

  // Cleanup.
  CollIndex maxEntries = catalogNames->entries();
  for (CollIndex i=0; i<maxEntries; i++)
    delete (*catalogNames)[i];
  delete catalogNames;

  // end the transaction, if we started it
  if (startedTran)
    sqlInterface_.commitTransaction();

  qms_->setInitialized(TRUE);
  return SQL_Success; //sqlcode_;

}  // End of performInitialization

/**
 * dump QMS statistics to the log
 * statistics collection and logging is controlled
 * by default MVQR_COLLECT_QMS_STATS
 */
void QmsInitializer::logQMSStats ()
{
   Int64  totalSize     = 0;
   Int64  allocatedSize = 0;
   NAMemory* qmsHeap = qms_->getHeap();
   if (qmsHeap)
   {
      totalSize = qmsHeap->getTotalSize ();
      allocatedSize = qmsHeap->getAllocSize ();
   }
   QRLogger::log(CAT_MEMORY, LL_DEBUG, "QMS total memory size: (%Ld)", totalSize);
   QRLogger::log(CAT_MEMORY, LL_DEBUG, "QMS allocated memory size: (%Ld)", allocatedSize);
}

// ***************************************************************
// * processCharset
// * Obtain ISO_MAPPING and DEFAULT_CHARSET from the system
// * defaults table.
// *
// * Two helper routines are called that execute static SQL queries
// * to read the ISO_MAPPING and DEFAULT_CHARSET attribute values
// * from the system defaults table.  
// *************************************************************** 
void QmsInitializer::processCharset()
{
  isoMapping_ = sqlInterface_.getIsoMapping();
  defaultCharset_ = sqlInterface_.getDefaultCharset();

  return;

}  // End of processCharset

// *************************************************************** 
// * Process the results of the catalog and MV searches
// *************************************************************** 

void QmsInitializer::processCatalogs(const NAStringList* catalogNames)
{

  if (catalogNames->isEmpty())
    return;

  numberOfCatalogs_ = catalogNames->entries();
  for (CollIndex i=0; i < numberOfCatalogs_; i++)
  {
    const NAString* catalog = (*catalogNames)[i];

    // Get the CAT_UID for this catalog
    Int64 catalogUID = sqlInterface_.getCatalogUID(catalog);

    // Get the schema definition version for this catalog
    const NAString* schemaVersion = sqlInterface_.getSchemaVersion(catalogUID);
    if (schemaVersion == NULL)
    {
      // This catalog is empty.
      QRLogger::log(CAT_QMS_INIT, LL_INFO,
      "Cannot get catalog Schema Version, catalog <%s> is empty",
      catalog->data());

      continue;
    }
  
    NAString* definitionSchema = 
      sqlInterface_.prepareDefinitionSchemaName(catalog, schemaVersion);
    NAString textTable(*definitionSchema, heap_);
    textTable += ".TEXT";

    // Get all the MVs for this catalog
    QRMVDefinitionList* mvDefinitions = 
      sqlInterface_.getMvUIDAndDefinitions(*definitionSchema);

    if (!mvDefinitions->isEmpty())
    {
      CollIndex maxEntries = mvDefinitions->entries();
      for (CollIndex j=0; j < maxEntries; j++)
      {
        char uid[21];
        const QRMVDefinition* mvDef = (*mvDefinitions)[j];
        convertInt64ToAscii(mvDef->objectUID_, uid);

        // Get all the MV descriptor text buffers and attributes
	// for the MVs in this specific catalog  
        const NAString* mvDescriptorText = 
          sqlInterface_.getMvDescriptor(textTable, (const char*) uid);

        if (mvDescriptorText && mvDescriptorText->length())
        {
           // Parse and retain the MV descriptor text buffers
           // and the MV attributes.  Process any MV 
           // descriptor buffers found.
           numberOfMVs_++;
           processMvDescriptor(mvDef, mvDescriptorText);
           delete mvDescriptorText;
        }
      }

      // Cleanup all resources handling these descriptors
      maxEntries = mvDefinitions->entries();
      for (CollIndex k=0; k<maxEntries; k++)
        NADELETE((*mvDefinitions)[k], QRMVDefinition, heap_);
      mvDefinitions->clear();
    }

    delete mvDefinitions;
    delete schemaVersion;
    delete definitionSchema;
  }

  return;

} // End of processCatalogs

// *************************************************************** 
// * Process the results of the MV UID and MV descriptor searches
// *************************************************************** 

void QmsInitializer::processMvDescriptor(const QRMVDefinition* mvDef, 
                                         const NAString* mvDescriptorText)
{
  Int32 len = 0;

  try
  {
    QRElementMapper em;
    XMLDocument doc = XMLDocument((NAHeap*)heap_, em);
    XMLElementPtr mvDescriptor = NULL;
    mvDescriptor = doc.parse(mvDescriptorText->data(), mvDescriptorText->length(), TRUE);

    // If parsed successfully, make sure it was an MV descriptor instead of
    // something else.
    if (mvDescriptor->getElementType() !=  ET_MVDescriptor)
    {
      QRLogger::log(CAT_QMS_INIT, LL_WARN,
        "XML document parsed ok, but had wrong document element -- %s",
                  mvDescriptor->getElementName());
      return;
    }

    QRMVDescriptorPtr mvDesc = static_cast<QRMVDescriptorPtr>(mvDescriptor);
    QRTablePtr mvTableName = mvDesc->getTable();
    assertLogAndThrow(CAT_QMS_INIT, LL_ERROR,
                      mvTableName != NULL, QRLogicException, 
		      "MV name missing from MV Descriptor.");
    const NAString& mvName = mvTableName->getTableName();

    // If QMS already contains this MV, drop it first, and then re-insert.
    if (qms_->contains(mvName))
    {
      QRLogger::log(CAT_QMS_INIT, LL_INFO,
        "Dropping older definition of %s before re-inserting it.",
        mvName.data());
      qms_->drop(mvName);
    }
    else
    {
      QRLogger::log(CAT_QMS_INIT, LL_INFO,
        "Reading MV %s from SMD.", mvName.data());
    }

    qms_->insert(mvDesc, mvDef);   
  }
  catch (QRException ex)
  {
    QRLogger::log(CAT_QMS_INIT, LL_MVQR_FAIL,
      "Exception thrown processing MV Descriptor: %s. Skipping.", ex.getMessage());
  }
  catch (...)
  {
    QRLogger::log(CAT_QMS_INIT, LL_MVQR_FAIL,
      "Unknown exception thrown processing MV Descriptor. Skipping.");
  }

  return;

} // End of processMvDescriptorTextBuffers

// *************************************************************** 
// For each and ever MV defined, call catman to regenerate the 
// MV descriptor.
// *************************************************************** 
void QmsInitializer::reDescriber(NAString* mvName, NABoolean rePublish)
{
  NABoolean result = 0;

  // start a transaction
  sqlInterface_.beginTransaction();

  // Specific MV only
  // First set the special parser flags so we can use the 
  // TANDEM_CAT_REQUEST syntax.
  sqlInterface_.setParserFlags();
  sqlInterface_.controlQueryDefault("MVQR_REWRITE_LEVEL", "1");

  if (mvName)
  {
    QRLogger::log(CAT_QMS_INIT, LL_INFO,
      "ReDescribing MV %s with rePublish %s", mvName->data(),
                rePublish ? "ON" : "OFF" );

    // Regenerate the descriptor.
    sqlInterface_.reDescribeMV(mvName, rePublish);
  }
  else
  {
    QRLogger::log(CAT_QMS_INIT, LL_INFO,
      "ReDescribing all MVs with rePublish %s", rePublish ? "ON" : "OFF" );

    // Obtain all the catalog names on the system
    const NAStringList* catalogNames = sqlInterface_.getAllCatalogNames();

    // Collect the MV names from all the catalogs.
    NAStringList mvNames(heap_);
    result = collectMVNames(catalogNames, mvNames);

    // Call catman to regenerate the MV descriptor for all the MVs.
    if (result)
      reDescribeMVs(mvNames, rePublish);

    // Cleanup.
    CollIndex maxEntries = catalogNames->entries();
    for (CollIndex i=0; i<maxEntries; i++)
      delete (*catalogNames)[i];
    delete catalogNames;
  }

  // end the transaction
  sqlInterface_.commitTransaction();

}  // End of reDescriber

// *************************************************************** 
// Collect the names of all the MVs in the input list of catalogs
// and add them to mvNames.
// *************************************************************** 
NABoolean QmsInitializer::collectMVNames(const NAStringList* catalogNames, 
                                         NAStringList&       mvNames)
{
  NABoolean result = 0;

  if (catalogNames->isEmpty())
    return TRUE;

  numberOfCatalogs_ = catalogNames->entries();
  for (CollIndex i=0; i < numberOfCatalogs_; i++)
  {
    const NAString* catalog = (*catalogNames)[i];

    // Get the CAT_UID for this catalog
    Int64 catalogUID = sqlInterface_.getCatalogUID(catalog);

    // Get the schema definition version for this catalog
    const NAString* schemaVersion = 
      sqlInterface_.getSchemaVersion(catalogUID);
    if (schemaVersion == NULL)
    {
      // This catalog is empty.
      QRLogger::log(CAT_QMS_INIT, LL_INFO,
      "Cannot get catalog Schema Version, catalog <%s> is empty",
      catalog->data());

      continue;
    }

    NAString* definitionSchema = 
      sqlInterface_.prepareDefinitionSchemaName(catalog, schemaVersion);

    // Get all the MVs for this catalog
    result = 
      sqlInterface_.collectCatalogMVs(*catalog, *definitionSchema, mvNames);

    delete schemaVersion;
    delete definitionSchema;

    if (result == FALSE)
      return FALSE;
  }

  return TRUE;
} // End of collectMVNames

// *************************************************************** 
// For each MV in the list of MV names, call catman to regenerate
// the MV descriptor.
// *************************************************************** 
void QmsInitializer::reDescribeMVs(NAStringList& mvNames, NABoolean rePublish)
{
  // For each MV
  for (Int32 i = (Int32)mvNames.entries()-1; i>=0; i--)
  {
    const NAString* mv = mvNames[i];

    // Regenerate the descriptor.
    sqlInterface_.reDescribeMV(mv, rePublish);

    // And cleanup the memory
    mvNames.remove(mv);
    delete mv;
  }

  return;

} // End of reDescribeMVs

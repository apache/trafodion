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
// ***********************************************************************
//
// File:         QRQueriesImpl.h
//
// Description:  Header for the classes used during QMS initialization
//               to obtain all MV descriptor text and MV attributes
//               for all MVs on the system.
//
//               It is also used by QMP for easy access to QRQueries.
//
// Created:      04/10/09
// ***********************************************************************
#ifndef _QRQUERIESIMPL_H_
#define _QRQUERIESIMPL_H_

#include "NAString.h"
#include "NAHeap.h"
#include "NAMemory.h"
#include "QRQueries.h"
#include "ComSmallDefs.h"
#include "QRMessage.h"
#include "CollHeap.h"
#include "QRMVDefinition.h"
#include "QRDescriptor.h"

/**
 * \file
 * Contains the class definition for QRQueriesImpl
 * which is used to initialize the QMS process.
 *
 * Additional methods are here for easy access to QRQueries.
 * Examples include, transaction control and charset processing.
 */

typedef NAList<const NAString*>    NAStringList;
typedef NAList<QRMVDefinition*>    QRMVDefinitionList;

#define XML_BUFF_SIZE       32768

#define CURRENT_VERSION_STR "2400"
#define CURRENT_VERSION     2400
#define DEFAULT_ISOMAPPING  "ISO88591"

/**
 * Class representing the initialization procedures when a QMS process
 * is starting.  
 */
class QRQueriesImpl
{
public:
  QRQueriesImpl(CollHeap* heap);

  virtual ~QRQueriesImpl();

  /**
    * getHeap() obtains the address to the XMLParser heap
    * @return The CollHeap heap address.
    */
  inline CollHeap * getHeap() { return heap_; };

  /**
    * getNAHeap() obtains the NAHeap address to the XMLParser heap
    * @return The NAHeap heap address.
    */
  inline NAHeap * getNAHeap() { return (NAHeap *) heap_; };

  NAString* prepareDefinitionSchemaName(const NAString* catalog,
			                const NAString* version);

  /**
    * getMvqrPrivateQMSInit obtains the MVQR_PRIVATE_QMS_INIT attribute value
    * from the system defaults table.
    * @return The return code from internal SQL query execution.
    */
  const NAString* getMvqrPrivateQMSInit();

  /**
    * getCatalogNames obtains all the catalog names
    * on the system
    */
  const NAStringList* getAllCatalogNames();

  /**
    * getIsoMapping obtains the ISO_MAPPING attribute value
    * from the system defaults table.
    */
  CharInfo::CharSet getIsoMapping();

  /**
    * getDefaultCharset obtains the DEFAULT_CHARSET attribute value
    * from the system defaults table.
    */
  CharInfo::CharSet getDefaultCharset();

  /**
   *
   * @return
   */
  NABoolean getCollectQMSStatsEnabled();

  Int32 getMvqrCpusPerQMS();
  Int32 getMvqrQMSCpuOffset();

  /**
    * getCatalogUID obtains the CAT_UID for a
    * specific catalog.
    * @param catalog [IN] The name of the catalog.
    */
  Int64 getCatalogUID(const NAString* catalog);

  /**
    * getSchemaVersion obtains the version number for the
    * definition schema for a specific catalog
    */
  const NAString* getSchemaVersion(Int64 catalogUID);

  /**
    * getMvUIDs obtains all the MV UIDs for a specific catalog.
    * @param catalog [IN] The name of the catalog.
    * @param version [IN] The version for the catalog.
    */
  QRMVDefinitionList* getMvUIDAndDefinitions(const NAString& definitionSchema);

  /**
    * getMvDescriptor obtains all the MV descriptor text and
    * MV attributes for a specific MV UID under a specific catalog.
    * @param catalog [IN] The name of the catalog.
    * @param version [IN] The version for the catalog.
    * @param uid [IN] The MV UID.
    */
  NAString* getMvDescriptor(const NAString& textTable,
			    const char * uid);

  /**
    * beginTransaction starts a transaction
    */
  void beginTransaction();

  /**
    * commitTransaction ends a transaction
    */
  void commitTransaction();

  /**
    * rollbackTransaction rolls back a transaction
    * @return The return code from the ROLLBACK WORK.
    */
  void rollbackTransaction();

  /**
    * open the cursor for stream delete reading from the PUBLISH_REWRITE table
    */
  void openRewritePublishCursor();

  /**
    * fetch from the cursor for stream delete reading from the PUBLISH_REWRITE table
    */
  NABoolean fetchRewritePublishCursor(MVQR_Publish* publish);

  /**
    * close the cursor for stream delete reading from the PUBLISH_REWRITE table
    */
  void closeRewritePublishCursor();

  /**
    * getCatalogName obtains the catalog name for
    * a specific catalog UID
    */
  const NAString* getCatalogName(Int64 catalogUID);

  /**
   * Find all the MVs in a specific catalog, and collect their names.
   * @param catalogName [IN] The catalog name.
   * @param definitionSchema [IN] The name of the definition schema.
   * @param mvNames [OUT] The list of MV names.
   * @return TRUE for success, FALSE for failure.
   */
  NABoolean collectCatalogMVs(const NAString& catalogName, 
                              const NAString& definitionSchema,
                              NAStringList&   mvNames);
  /**
   * Set the special parser flags so we can use internal syntax.
   */
  void setParserFlags();

  Lng32 controlQueryDefault(const NAString& cqdName, 
                           const NAString& cqdValue);

  /**
   * Call catman to regenerate the MV descriptor for an MV.
   * @param fullMvName The full name of the MV.
   * @param rePublish TRUE if the new MV descriptor is to be re-published.
   * @return TRUE for success, FALSE for failure.
   */
  NABoolean reDescribeMV(const NAString* fullMvName, NABoolean rePublish);

protected: 

  /**
    * initializeNodeName obtains the local node name
    */
  void initializeNodeName();

  /**
    * getNodeNameWithBackslash returns a character string
    * containing the name of the local node, with a backslash.
    * @return The name of the local node with a backslash.
    */
  inline const char * getNodeNameWithBackslash()
  { return nodeNameWithBackslash_.data(); };

  /**
    * getNodeName returns a character string
    * containing the name of the local node.
    * @return The name of the local node.
    */
  inline const char * getNodeName()
  { return nodeName_.data(); };

  /**
    * fixupDelimitedName returns a character string
    * handling special characters such as double quote
    * for an object name.
    * @param inName The input object name to fixup.
    * @param outName The output object name to fixup.
    */
  void fixupDelimitedName(const char * inName, char * outName);

protected:
  NABoolean getSystemDefault(const char* attribute, char* defValue);

private:
  NAString   nodeNameWithBackslash_;
  NAString   nodeName_;
  CollHeap*  heap_;
  QRQueries* queries_;  
  QRMVData*  data_;

  // Copy construction/assignment not defined.
  QRQueriesImpl(const QRQueriesImpl&);
  QRQueriesImpl& operator=(const QRQueriesImpl&);
  
}; // QRQueriesImpl

#endif  /* _QRQUERIESIMPL_H_ */

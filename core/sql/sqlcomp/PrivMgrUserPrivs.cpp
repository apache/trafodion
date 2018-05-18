//*****************************************************************************
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
//*****************************************************************************
  
// ==========================================================================
// Contains non inline methods in the following classes
//   PrivMgrObjectInfo
//   PrivMgrCommands
// ==========================================================================

#include "PrivMgrCommands.h"
#include "PrivMgrMD.h"
#include "DgBaseType.h"
#include "NATable.h"
#include "NAColumn.h"
#include "ComSecurityKey.h"
#include "ComUser.h"
#include "ComMisc.h"
#include "CmpSeabaseDDL.h"
#include <cstdio>
#include <algorithm>

// ****************************************************************************
// Class: PrivMgrObjectInfo
// ****************************************************************************
PrivMgrObjectInfo::PrivMgrObjectInfo(
  const NATable *naTable)
: objectOwner_ (naTable->getOwner()),
  schemaOwner_ (naTable->getSchemaOwner()),
  objectUID_   (naTable->objectUid().get_value()),
  objectType_  (naTable->getObjectType())
{
  // Map hbase map table to external name
  if (ComIsHBaseMappedIntFormat(naTable->getTableName().getCatalogName(),
                                naTable->getTableName().getSchemaName()))
  {
    NAString newCatName;
    NAString newSchName;
    ComConvertHBaseMappedIntToExt(naTable->getTableName().getCatalogName(), 
                                  naTable->getTableName().getSchemaName(),
                                  newCatName, newSchName);
    CONCAT_CATSCH(objectName_, newCatName, newSchName);
    objectName_ += std::string(".") + 
                   naTable->getTableName().getUnqualifiedObjectNameAsAnsiString().data();
  }
  else
    objectName_ = naTable->getTableName().getQualifiedNameAsAnsiString().data();

  const NAColumnArray &colNameArray = naTable->getNAColumnArray();
  for (size_t i = 0; i < colNameArray.entries(); i++)
  {
    const NAColumn * naCol = colNameArray.getColumn(i);
    std::string columnName(naCol->getColName().data());
    columnList_.push_back( columnName);
  }
}
 
// ****************************************************************************
// Class: PrivMgrUserPrivs
// ****************************************************************************
bool PrivMgrUserPrivs::initUserPrivs(
  const std::vector<int32_t> & roleIDs,
  const TrafDesc *priv_desc,
  const int32_t userID,
  const int64_t objectUID,
  ComSecurityKeySet & secKeySet)
{
  hasPublicPriv_ = false;

  // generate PrivMgrUserPrivs from the priv_desc structure
  TrafDesc *priv_grantees_desc = priv_desc->privDesc()->privGrantees;
  NAList<PrivMgrDesc> descList(NULL);

  // Find relevant descs for the user
  while (priv_grantees_desc)
  {
    Int32 grantee = priv_grantees_desc->privGranteeDesc()->grantee;
    bool addDesc = false;
    if (grantee == userID)
      addDesc = true;

    if (PrivMgr::isRoleID(grantee))
    {
      if ((std::find(roleIDs.begin(), roleIDs.end(), grantee)) != roleIDs.end())
        addDesc = true;
    }

    if (ComUser::isPublicUserID(grantee))
    {
      addDesc = true;
      hasPublicPriv_ = true;
    }

    // Create a list of PrivMgrDesc contain privileges for user, user's roles,
    // and public
    if (addDesc)
    {
      TrafDesc *objectPrivs = priv_grantees_desc->privGranteeDesc()->objectBitmap;

      PrivMgrCoreDesc objectDesc(objectPrivs->privBitmapDesc()->privBitmap,
                                 objectPrivs->privBitmapDesc()->privWGOBitmap);
      
      TrafDesc *priv_grantee_desc = priv_grantees_desc->privGranteeDesc();
      TrafDesc *columnPrivs = priv_grantee_desc->privGranteeDesc()->columnBitmaps;
      NAList<PrivMgrCoreDesc> columnDescs(NULL);
      while (columnPrivs)
      {
        PrivMgrCoreDesc columnDesc(columnPrivs->privBitmapDesc()->privBitmap,
                                   columnPrivs->privBitmapDesc()->privWGOBitmap,
                                   columnPrivs->privBitmapDesc()->columnOrdinal);
        columnDescs.insert(columnDesc);
        columnPrivs = columnPrivs->next;
      }

      PrivMgrDesc privs(priv_grantees_desc->privGranteeDesc()->grantee);
      privs.setTablePrivs(objectDesc);
      privs.setColumnPrivs(columnDescs);
      privs.setHasPublicPriv(hasPublicPriv_);

      descList.insert(privs);
    }
    priv_grantees_desc = priv_grantees_desc->next;
  }

  // Union privileges from all grantees together to create a single set of
  // bitmaps.  Create security invalidation keys
  for (int i = 0; i < descList.entries(); i++)
  {
    PrivMgrDesc privs = descList[i];

    // Set up object level privileges
    objectBitmap_ |= privs.getTablePrivs().getPrivBitmap();
    grantableBitmap_ |= privs.getTablePrivs().getWgoBitmap();

    // Set up column level privileges
    NAList<PrivMgrCoreDesc> columnPrivs = privs.getColumnPrivs();
    std::map<size_t,PrivColumnBitmap>::iterator it;
    for (int j = 0; j < columnPrivs.entries(); j++)
    {
      PrivMgrCoreDesc colDesc = columnPrivs[j];
      Int32 columnOrdinal = colDesc.getColumnOrdinal();
      it = colPrivsList_.find(columnOrdinal);
      if (it == colPrivsList_.end())
      {
        colPrivsList_[columnOrdinal] = colDesc.getPrivBitmap();
        colGrantableList_[columnOrdinal] = colDesc.getWgoBitmap();
      }
      else
      {
        colPrivsList_[columnOrdinal] |= colDesc.getPrivBitmap();
        colGrantableList_[columnOrdinal] |= colDesc.getWgoBitmap();
      }
    }

    // set up security invalidation keys
    if (!buildSecurityKeys(userID, privs.getGrantee(), objectUID, privs.getTablePrivs(), secKeySet))
      return false;

    for (int k = 0; k < colPrivsList_.size(); k++)
    {
      PrivMgrCoreDesc colDesc(colPrivsList_[k], colGrantableList_[k]);
      if (!buildSecurityKeys(userID, privs.getGrantee(), objectUID, colDesc, secKeySet))
        return false;
    }
  }

  // TBD - add schema privilege bitmaps
  return true;
}

// ----------------------------------------------------------------------------
// method: initUserPrivs
//
// Creates a PrivMgrUserPrivs object from a PrivMgrDesc object
// ----------------------------------------------------------------------------
void PrivMgrUserPrivs::initUserPrivs(PrivMgrDesc &privsOfTheUser)
{
  objectBitmap_ = privsOfTheUser.getTablePrivs().getPrivBitmap();
  grantableBitmap_ = privsOfTheUser.getTablePrivs().getWgoBitmap();

  for (int32_t i = 0; i < privsOfTheUser.getColumnPrivs().entries(); i++)
  {
    const int32_t columnOrdinal = privsOfTheUser.getColumnPrivs()[i].getColumnOrdinal();
    colPrivsList_[columnOrdinal] = privsOfTheUser.getColumnPrivs()[i].getPrivBitmap();
    colGrantableList_[columnOrdinal] = privsOfTheUser.getColumnPrivs()[i].getWgoBitmap();
  }
  hasPublicPriv_ = privsOfTheUser.getHasPublicPriv();
}


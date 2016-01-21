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
  
#include "PrivMgrPrivileges.h"

#include "PrivMgrMD.h"
#include "PrivMgrMDTable.h"
#include "PrivMgrDesc.h"
#include "PrivMgrDefs.h"
#include "PrivMgrRoles.h"
#include "PrivMgrComponentPrivileges.h"
#include "PrivMgrObjects.h"
#include "PrivMgrCommands.h"

#include <numeric>
#include <cstdio>
#include <algorithm>
#include <iterator>
#include "sqlcli.h"
#include "ComSmallDefs.h"
#include "ExExeUtilCli.h"
#include "ComDiags.h"
#include "ComQueue.h"
#include "CmpCommon.h"
#include "CmpContext.h"
#include "CmpDDLCatErrorCodes.h"
#include "ComSecurityKey.h"
#include "NAUserId.h"
#include "ComUser.h"
#include "CmpSeabaseDDLutil.h"
#include "logmxevent_traf.h"
class ColPrivGrant;
class ColumnPrivsMDTable;
 
// ****************************************************************************
// File: PrivMgrPrivileges.h
//
// This file contains:
//   class ObjectPrivsMDRow
//   class ObjectPrivsMDTable
//   non inline methods for class PrivMgrPrivileges
// ****************************************************************************



// *****************************************************************************
// * Class:         ObjectPrivsMDRow
// * Description:  This class represents a row from the OBJECT_PRIVILEGES table
// *                
// * An object row can be uniquely identified by its object UID, granteeID 
// * and grantorID.
// *****************************************************************************
class ObjectPrivsMDRow : public PrivMgrMDRow
{
public:

// -------------------------------------------------------------------
// Constructors and destructors:
// -------------------------------------------------------------------
   ObjectPrivsMDRow()
   : PrivMgrMDRow(PRIVMGR_OBJECT_PRIVILEGES, OBJECT_PRIVILEGES_ENUM),
     objectUID_(0),
     grantorID_(0),
     granteeID_(0)
   { };
   
   ObjectPrivsMDRow(const ObjectPrivsMDRow &other)
   : PrivMgrMDRow(other)
   {
      objectUID_ = other.objectUID_;
      objectName_ = other.objectName_;
      objectType_ = other.objectType_;
      granteeID_ = other.granteeID_;
      granteeName_ = other.granteeName_;
      granteeType_ = other.granteeType_;
      grantorID_ = other.grantorID_;
      grantorName_ = other.grantorName_;
      grantorType_ = other.grantorType_;
      privsBitmap_ = other.privsBitmap_;
      grantableBitmap_ = other.grantableBitmap_;
      current_ = other.current_;
      visited_ = other.visited_;
   };
   virtual ~ObjectPrivsMDRow() {};
   
// Methods used to determine changes after processing revoked privileges
   PrivMgrCoreDesc& accessCurrent() { return current_; }
   PrivMgrCoreDesc& accessVisited() { return visited_; }
   void clearVisited() { visited_.setAllPrivAndWgo(false); };
   bool isChanged() 
    { return (current_ == PrivMgrCoreDesc(privsBitmap_, grantableBitmap_)); }

   void setToOriginal() { current_ = PrivMgrCoreDesc(privsBitmap_, grantableBitmap_); }
;


// Return True iff some current flag is set, where visited is not.
   NABoolean anyNotVisited() const {return current_.anyNotSet( visited_ );}

// Clear current where current was set and visited was not.
//  Return True iff some current flag gets cleared.
   NABoolean cascadeLosses();

// Describe a row for tracing
   void describeRow (std::string &rowDetails);

// -------------------------------------------------------------------
// Data Members:
// -------------------------------------------------------------------
     
   int64_t            objectUID_;
   std::string        objectName_;
   ComObjectType      objectType_;
   int32_t            granteeID_;
   std::string        granteeName_;
   std::string        granteeType_;
   int32_t            grantorID_;
   std::string        grantorName_;
   std::string        grantorType_;
   PrivObjectBitmap   privsBitmap_;
   PrivObjectBitmap   grantableBitmap_;
   
   PrivMgrCoreDesc  visited_;
   PrivMgrCoreDesc  current_;
};


// *****************************************************************************
// * Class:         ObjectPrivsMDTable
// * Description:  This class represents the OBJECT_PRIVILEGES table 
// *                
// *    An object privileges row can be uniquely identified by:
// *       objectUID
// *       granteeID
// *       grantorID
// *****************************************************************************
class ObjectPrivsMDTable : public PrivMgrMDTable
{
public:
   ObjectPrivsMDTable(
      const std::string & tableName,
      ComDiagsArea * pDiags = NULL) 
   : PrivMgrMDTable(tableName,OBJECT_PRIVILEGES_ENUM, pDiags)
     {};

   virtual ~ObjectPrivsMDTable() 
   {};

   virtual PrivStatus insert(const PrivMgrMDRow &row);
   virtual PrivStatus selectWhereUnique(
      const std::string & whereClause,
      PrivMgrMDRow & row);
   PrivStatus selectWhere(
      const std::string & whereClause,
      const std::string & orderByClause,
      std::vector<PrivMgrMDRow *> &rowList);
   PrivStatus deleteRow(const ObjectPrivsMDRow & row);
   virtual PrivStatus deleteWhere(const std::string & whereClause);
   PrivStatus updateRow(const ObjectPrivsMDRow & row);
   PrivStatus updateWhere(
      const std::string & setClause,
      const std::string & whereClause);
   PrivStatus insertSelect(
      const std::string & objectsLocation,
      const std::string & authsLocation);
   PrivStatus insertSelectOnAuthsToPublic(
      const std::string &objectsLocation,
      const std::string &authsLocation);

private:   
   ObjectPrivsMDTable();
   void setRow(OutputInfo *pCliRow, ObjectPrivsMDRow &rowOut);

};

// *****************************************************************************
// * Class:         ColumnPrivsMDRow
// * Description:  This class represents a row from the COLUMN_PRIVILEGES table
// *                
// * An column row can be uniquely identified by its object UID, granteeID, 
// * grantorID, and column ordinal.
// *****************************************************************************
class ColumnPrivsMDRow : public PrivMgrMDRow
{
public:

// -------------------------------------------------------------------
// Constructors and destructors:
// -------------------------------------------------------------------
   ColumnPrivsMDRow()
   : PrivMgrMDRow(PRIVMGR_COLUMN_PRIVILEGES, COLUMN_PRIVILEGES_ENUM),
     objectUID_(0),
     grantorID_(0),
     granteeID_(0),
     columnOrdinal_(0)
   { };
   
   ColumnPrivsMDRow(const ColumnPrivsMDRow &other)
   : PrivMgrMDRow(other)
   {
      objectUID_ = other.objectUID_;
      objectName_ = other.objectName_;
      granteeID_ = other.granteeID_;
      granteeName_ = other.granteeName_;
      grantorID_ = other.grantorID_;
      grantorName_ = other.grantorName_;
      columnOrdinal_ = other.columnOrdinal_;
      privsBitmap_ = other.privsBitmap_;
      grantableBitmap_ = other.grantableBitmap_;
   };
   virtual ~ColumnPrivsMDRow() {};

// Describe a row for tracing
   void describeRow (std::string &rowDetails);


// -------------------------------------------------------------------
// Data Members:
// -------------------------------------------------------------------
     
   int64_t            objectUID_;
   std::string        objectName_;
   int32_t            granteeID_;
   std::string        granteeName_;
   int32_t            grantorID_;
   std::string        grantorName_;
   int32_t            columnOrdinal_;
   PrivColumnBitmap   privsBitmap_;
   PrivColumnBitmap   grantableBitmap_;
   
};


// *****************************************************************************
// * Class:         ColumnPrivsMDTable
// * Description:  This class represents the COLUMN_PRIVILEGES table 
// *                
// *    An column privileges row can be uniquely identified by:
// *       objectUID
// *       granteeID
// *       grantorID
// *       columnOrdinal
// *****************************************************************************
class ColumnPrivsMDTable : public PrivMgrMDTable
{
public:
   ColumnPrivsMDTable(
      const std::string & tableName,
      ComDiagsArea * pDiags = NULL) 
   : PrivMgrMDTable(tableName,COLUMN_PRIVILEGES_ENUM, pDiags)
     {};

   virtual ~ColumnPrivsMDTable() 
   {};

   virtual PrivStatus insert(const PrivMgrMDRow &row);
   virtual PrivStatus selectWhereUnique(
      const std::string & whereClause,
      PrivMgrMDRow & row);
   PrivStatus selectWhere(
      const std::string & whereClause,
      const std::string & orderByClause,
      std::vector<PrivMgrMDRow *> &rowList);
   PrivStatus updateColumnRow(
      const ColumnPrivsMDRow & row,
      const std::string whereBase);

private:   
   ColumnPrivsMDTable();
   void setRow(
      OutputInfo *pCliRow, 
      ColumnPrivsMDRow &rowOut);
};

class ColPrivEntry
{
public:
   int32_t            columnOrdinal;
   PrivColumnBitmap   privsBitmap;
   PrivColumnBitmap   grantableBitmap;
   bool               isUpdate;
   ColPrivEntry()
   : columnOrdinal(0),isUpdate(false){};
};

class ColObjectGrants
{
public:
   ColObjectGrants(int64_t objectUID) : object_uid_(objectUID) {};
   ~ColObjectGrants();
   
   const ColPrivEntry * getColPrivGrant(int32_t columnOrdinal) const
   {
      for (size_t i = 0; i < colPrivGrants_.size(); i++)
         if (colPrivGrants_[i].columnOrdinal == columnOrdinal)
            return &colPrivGrants_[i];
      return NULL;
   }
   
   void clear()
   {
      colPrivGrants_.clear();
   }   
      
private:
   int64_t object_uid_;
   std::vector<ColPrivEntry> colPrivGrants_;
};

// *****************************************************************************
// *   PrivMgrPrivileges.cpp static function declarations                      *
// *****************************************************************************

static PrivStatus buildPrivText(
   const std::vector<PrivMgrMDRow *> rowList,
   const PrivMgrObjectInfo         & objectInfo,
   PrivLevel                         privLevel,
   ComDiagsArea                    * pDiags_,
   std::string                     & privilegeText);

void static buildGrantText(
   const std::string & privText,
   const std::string & objectGranteeText, 
   const int32_t grantorID,
   const std::string grantorName,
   bool isWGO,
   const int32_t ownerID,
   std::string & grantText);

static PrivStatus buildColumnSecurityKeys(
   const int64_t objectUID,
   const PrivColList & colPrivsList,
   const int32_t userID, 
   std::vector<ComSecurityKey *> & secKeySet);

static PrivStatus buildUserSecurityKeys(
   const std::vector<int32_t> & roleIDs,
   const int32_t userID, 
   std::vector <ComSecurityKey *> & secKeySet);
   
void static closeColumnList(std::string & columnList);   

static void deleteRowList(std::vector<PrivMgrMDRow *> & rowList);

static ColPrivEntry * findColumnEntry(
   std::vector<ColPrivEntry> & colPrivsToGrant,
   const int32_t columnsOrdinal);
   
static PrivStatus getColRowsForGrantee(
   const std::vector <PrivMgrMDRow *> &columnRowList,
   const int32_t granteeID,
   const std::vector<int32_t> & roleIDs,
   std::vector<ColumnPrivsMDRow> &rowList,
   std::vector <ComSecurityKey *>* secKeySet);   

static void getColRowsForGranteeGrantor(
   const std::vector <PrivMgrMDRow *> & columnRowList,
   const int32_t granteeID,
   const int32_t grantorID,
   std::vector<ColPrivEntry> &colPrivGrants);
   
static bool hasAllDMLPrivs(
   ComObjectType objectType,
   PrivObjectBitmap privBitmap);   
   
static bool hasGrantedColumnPriv(
   const std::vector <PrivMgrMDRow *> & columnRowList,
   int32_t grantorID,
   int32_t granteeID,
   const std::vector<ColPrivSpec> & colPrivsArray,
   PrivStatus & privStatus,
   std::string & privilege,
   std::vector<ColPrivEntry> & grantedColPrivs);   
  
static bool isDelimited( const std::string &identifier);

// *****************************************************************************
//    PrivMgrPrivileges methods
// *****************************************************************************

// -----------------------------------------------------------------------
// Default Constructor
// -----------------------------------------------------------------------
PrivMgrPrivileges::PrivMgrPrivileges () 
: PrivMgr(),
  objectUID_(0),
  grantorID_(0)
{ 
  objectTableName_ = metadataLocation_ + "." + PRIVMGR_OBJECT_PRIVILEGES;
  columnTableName_ = metadataLocation_ + "." + PRIVMGR_COLUMN_PRIVILEGES;
} 

// -----------------------------------------------------------------------
// Construct a PrivMgrPrivileges object for a new object privilege.
// -----------------------------------------------------------------------
PrivMgrPrivileges::PrivMgrPrivileges (
   const int64_t objectUID,
   const std::string &objectName,
   const int32_t grantorID,
   const std::string &metadataLocation,
   ComDiagsArea * pDiags)
: PrivMgr(metadataLocation,pDiags),
  objectUID_(objectUID),
  objectName_(objectName),
  grantorID_(grantorID)
{
  objectTableName_  = metadataLocation + "." + PRIVMGR_OBJECT_PRIVILEGES;
  columnTableName_  = metadataLocation + "." + PRIVMGR_COLUMN_PRIVILEGES;
} 

// ----------------------------------------------------------------------------
// Construct PrivMgrPrivileges object for describe statements
// ----------------------------------------------------------------------------
PrivMgrPrivileges::PrivMgrPrivileges (
   const PrivMgrObjectInfo &objectInfo,
   const std::string &metadataLocation,
   ComDiagsArea *pDiags)
: PrivMgr(metadataLocation, pDiags),
  objectUID_(((PrivMgrObjectInfo)objectInfo).getObjectUID()),
  objectName_(((PrivMgrObjectInfo)objectInfo).getObjectName()),
  grantorID_(0)
{
  objectTableName_  = metadataLocation + "." + PRIVMGR_OBJECT_PRIVILEGES;
  columnTableName_  = metadataLocation + "." + PRIVMGR_COLUMN_PRIVILEGES;
}

// ----------------------------------------------------------------------------
// Construct a PrivMgrPrivileges object for an objectUID
// ----------------------------------------------------------------------------
PrivMgrPrivileges::PrivMgrPrivileges (
  const int64_t objectUID,
  const std::string &metadataLocation,
  ComDiagsArea *pDiags)
: PrivMgr(metadataLocation, pDiags),
  objectUID_(objectUID),
  grantorID_(0)
{
  objectTableName_  = metadataLocation + "." + PRIVMGR_OBJECT_PRIVILEGES;
  columnTableName_  = metadataLocation + "." + PRIVMGR_COLUMN_PRIVILEGES;
}

// ----------------------------------------------------------------------------
// Construct a basic PrivMgrPrivileges object
// ----------------------------------------------------------------------------
PrivMgrPrivileges::PrivMgrPrivileges (
   const std::string &metadataLocation,
   ComDiagsArea *pDiags)
: PrivMgr(metadataLocation, pDiags),
  objectUID_(0),
  grantorID_(0)
{
  objectTableName_  = metadataLocation + "." + PRIVMGR_OBJECT_PRIVILEGES;
  columnTableName_  = metadataLocation + "." + PRIVMGR_COLUMN_PRIVILEGES;
} 

// -----------------------------------------------------------------------
// Copy constructor
// -----------------------------------------------------------------------
PrivMgrPrivileges::PrivMgrPrivileges(const PrivMgrPrivileges &other)
: PrivMgr(other)
{
  objectUID_ = other.objectUID_;
  objectName_ = other.objectName_;
  grantorID_ = other.grantorID_;
  objectTableName_ = other.objectTableName_;
  columnTableName_ = other.columnTableName_;
  objectRowList_ = other.objectRowList_;
  columnRowList_ = other.columnRowList_;
}

// -----------------------------------------------------------------------
// Destructor.
// -----------------------------------------------------------------------
PrivMgrPrivileges::~PrivMgrPrivileges() 
{ 
  deleteRowList(objectRowList_);
  deleteRowList(columnRowList_);
}

// *****************************************************************************
// * Method: buildSecurityKeys                                
// *                                                       
// *    Builds security keys for the current object and specified user.
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <granteeID> is the unique identifier for the grantee
// *  <privs> is the list of privileges the user has on the object
// *  <secKeySet> is the set of security keys to be passed back.  Caller is 
// *              responsible for freeing keys.
// *                                                                  
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: Security keys were built
// *           *: Security keys were not built, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::buildSecurityKeys(
   const int32_t granteeID, 
   const PrivMgrCoreDesc &privs,
   std::vector <ComSecurityKey *> & secKeySet)
  
{
  // Only need to generate keys for DML privileges
  for ( size_t i = FIRST_DML_PRIV; i <= LAST_DML_PRIV; i++ )
  {
    if ( privs.getPriv(PrivType(i)))
    {
      ComSecurityKey *key = new ComSecurityKey(granteeID, 
                                               objectUID_,
                                               PrivType(i),
                                               ComSecurityKey::OBJECT_IS_OBJECT);
      if (key->isValid())
         secKeySet.push_back(key);
      else
      {
        PRIVMGR_INTERNAL_ERROR("ComSecurityKey is null");
        return STATUS_ERROR;
      }
    }
  }
   
  return STATUS_GOOD;
}

// *****************************************************************************
// * Method: getColPrivsForUser                                
// *                                                       
// *    Returns the column privileges a user has been granted on the object.
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <granteeID> is the unique identifier for the grantee
// *  <roleIDs> specifies a list of roles granted to the grantee
// *  <colPrivsList> passes back the list of privs granted
// *  <colGrantableList> passes back the list the user has WGO for
// *  <secKeySet> if not NULL, returns a set of keys for user
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: Privileges were returned
// *           *: Unable to lookup privileges, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::getColPrivsForUser(
   const int32_t granteeID,
   const std::vector<int32_t> & roleIDs,
   PrivColList & colPrivsList,
   PrivColList & colGrantableList,
   std::vector <ComSecurityKey *>* secKeySet) 

{

std::vector<ColumnPrivsMDRow> rowList;

// Get the privileges for the columns of the object granted to the grantee
PrivStatus privStatus = getColRowsForGrantee(columnRowList_,granteeID,roleIDs,
                                             rowList,secKeySet);
                                             
   if (privStatus == STATUS_ERROR)
      return privStatus; 
     
   for (int32_t i = 0; i < rowList.size();++i)
   {
      const int32_t columnOrdinal = rowList[i].columnOrdinal_;
      colPrivsList[columnOrdinal] = rowList[i].privsBitmap_;
      colGrantableList[columnOrdinal] = rowList[i].grantableBitmap_;
      
      if (secKeySet != NULL)
      {
         privStatus = buildColumnSecurityKeys(objectUID_,colPrivsList,
                                             rowList[i].granteeID_,*secKeySet);
         if (privStatus != STATUS_GOOD)
            return privStatus;    
      }
   }

   return STATUS_GOOD;
}  


// *****************************************************************************
// * Method: getPrivRowsForObject                                
// *                                                       
// *    returns rows describing all the privileges that have been
// *    granted on the object
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <objectPrivsRows> Zero or more rows of grants for the object.
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD    : Rows were returned
// * STATUS_NOTFOUND: No rows were returned
// *               *: Unable to read privileges, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::getPrivRowsForObject(
   const int64_t objectUID,
   std::vector<ObjectPrivsRow> & objectPrivsRows)
   
{
  PrivStatus retcode = STATUS_GOOD;

  if (objectUID_ == 0)
  {
    PRIVMGR_INTERNAL_ERROR("objectUID is 0 for getPrivRowsForObject()");
    return STATUS_ERROR;
  }

  // generate the list of privileges granted to the object and store in class
  if (generateObjectRowList() == STATUS_ERROR)
    return STATUS_ERROR;

  for (size_t i = 0; i < objectRowList_.size(); i++)
  {
    ObjectPrivsMDRow &row = static_cast<ObjectPrivsMDRow &> (*objectRowList_[i]);
    if (row.grantorID_ != SYSTEM_USER)
    {
      ObjectPrivsRow newRow;
    
      strcpy(newRow.objectName,row.objectName_.c_str());
      newRow.objectType = row.objectType_;
      newRow.granteeID = row.granteeID_;
      strcpy(newRow.granteeName,row.granteeName_.c_str());
      newRow.granteeType = CmGetComGranteeAsGranteeType(row.granteeType_.c_str());
      newRow.grantorID = row.grantorID_;
      strcpy(newRow.grantorName,row.grantorName_.c_str());
      newRow.grantorType = CmGetComGrantorAsGrantorType(row.grantorType_.c_str());
      newRow.privilegesBitmap = row.privsBitmap_.to_ulong();
      newRow.grantableBitmap = row.grantableBitmap_.to_ulong();
    
      objectPrivsRows.push_back(newRow);
    }
  }

  return retcode;
}
// ----------------------------------------------------------------------------
// method: getTreeOfGrantors
//
// Returns the list of grantors that have granted privileges to the grantee, 
// either directly or through another grantor in the tree.
//
// The list is determined by first looking at the direct grantors. For each 
// returned grantor, this function is called recursively to get the previous set
// of grantors until there are no more grantors.
//
// The list is returned in grantor ID order. 
//
// For example:
//   user1 (owner) grants to:
//      user6 who grants to:
//         user3
//         user4 who grants to:
//           user5
//           user2
//      user3 who grants to:
//         user4
//         user5
// The following grantors are returned for granteeID user4:
//    user1, user3, user6
// 
// Params:
//  granteeID - where to start the search
//  listOfGrantors - returns the list of grantors
// ----------------------------------------------------------------------------
void PrivMgrPrivileges::getTreeOfGrantors(
  const int32_t granteeID,
  std::set<int32_t> &listOfGrantors)
{
  // search the rowList for a match
  for (size_t i = 0; i < objectRowList_.size(); i++)
  {
    ObjectPrivsMDRow &row = static_cast<ObjectPrivsMDRow &> (*objectRowList_[i]);
    if (row.granteeID_ == granteeID)
    {
       // We found a grant to the granteeID
       // go up to the next level using the grantorID
       getTreeOfGrantors( row.grantorID_, listOfGrantors);
       listOfGrantors.insert(granteeID);
    }
  }
}


// *****************************************************************************
// * Method: givePrivForObjects                                
// *                                                       
// *    Updates one or more rows in the OBJECT_PRIVILEGES table to reflect 
// *    a new owner of one or more objects.
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <currentOwnerID> is the unique identifier for the current owner
// *  <newOwnerID> is the unique identifier for the new owner
// *  <newOwnerName> is the name of the new owner (upper cased)
// *  <objectUIDs> is the list of objects with a new owner
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: Privileges were updated to reflect new owner
// *           *: Unable to update privileges, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::givePrivForObjects(
      const int32_t currentOwnerID,
      const int32_t newOwnerID,
      const std::string &newOwnerName,
      const std::vector<int64_t> &objectUIDs)
      
{

PrivStatus privStatus = STATUS_GOOD;
  
  for (size_t i = 0; i < objectUIDs.size(); i++)
  {
     privStatus = givePriv(currentOwnerID,newOwnerID,newOwnerName,objectUIDs[i]);
     if (privStatus != STATUS_GOOD)
        return privStatus;
  }
      
  return STATUS_GOOD;
  
}
  

// *****************************************************************************
// * Method: givePriv                                
// *                                                       
// *    Updates rows in the OBJECT_PRIVILEGES table to reflect a new owner of
// *    an objects.
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <granteeID> is the unique identifier for the new owner
// *  <granteeName> is the name of the new owner (upper cased)
// *  <objectUIDs> is the list of objects with a new owner
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: Privileges were updated to reflect new owner
// *           *: Unable to update privileges, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::givePriv(
   const int32_t currentOwnerID,
   const int32_t newOwnerID,
   const std::string &newOwnerName,
   const int64_t objectUID)
      
{

// *****************************************************************************
// *                                                                           *
// *    The set of grants for a given object can be thought of as a tree with  *
// * many branches and one root.  At the root, is the grant from the system    *
// * (grantor is _SYSTEM) to the owner of the object.  The grant from the      *
// * system is for all privileges, with grant option.                          *
// *                                                                           *
// *    The owner then grants privileges to one or more authIDs.  Each of      *
// * these grants can be viewed as a branch off the system grant root.  With   *
// * WITH GRANT OPTION, each of these branches can have its own set of         *
// * branches, potentially resulting in a dense tree.  See the rough           *
// * drawing:                                                                  *
// *                                                                           *
// *         USERG  USERC   USERD  USERC  USERB                                *
// *             \    |     /   \    |     /                                   *
// *              \   |    /     \   |    /                                    *
// *               \  |  /        \  |  /                                      *
// *                USERE  USERD   USERF  USERB  USERG                         *
// *                    \    |     /   \    |     /                            *
// *                     \   |    /     \   |    /                             *
// *                      \  |  /        \  |  /                               *
// *                       USERB  USERC  USERD                                 *
// *                           \    |     /                                    *
// *                            \   |    /                                     *
// *                             \  |  /                                       *
// *                              USERA                                        *
// *                                 |                                         *
// *                             _SYSTEM                                       *
// *                                                                           *
// *                                                                           *
// *    Some of the rules are:                                                 *
// *  1) No grants to the owner (USERA)                                        *
// *  2) A user can appear an unlimited number of times in the tree, but       *
// *     all grants from the user (for a given privilege) are from the same    *
// *     node.  For example, USERB is granted 3 times, but all grants from     *
// *     USERB emanate from the same node.  This is because when USERB grants  *
// *     a privilege, we start from the root and find the first node where     *
// *     USERB has WITH GRANT OPTION.                                          *
// *                                                                           *
// *    When an object is given to another user, the current owner loses       *
// * their grant from the system; instead, the new owner is granted from the   *
// * system.  All existing grants from the current owner are now from the new  *
// * owner.  Based on rule #1 above, the new owner can no longer appear in     *
// * other nodes in the tree except the root node.  So, in the case where the  *
// * object is given to USERB, the new grant tree should be:                   *
// *                                                                           *
// *                        USERD  USERC                                       *
// *                         |  \    |                                         *
// *                         |   \   |                                         *
// *                 USERC   |    \  |                                         *
// *                    \    |    USERF           USERG                        *
// *                     \   |      |  \           /                           *
// *                      \  |      |   \         /                            *
// *                       \ |      |    \      /                              *
// *        USERG ---------USERE    |    USERD                                 *
// *                           \    |     /                                    *
// *                            \   |    /                                     *
// *                             \  |  /                                       *
// *                              USERB ---- USERC                             *
// *                                 |                                         *
// *                             _SYSTEM                                       *
// *                                                                           *
// *                                                                           *
// *   Note, previously USERA had three grants, and USERB had three grants,    *
// * and now USERB has four grants--not six.  First, the grant from USERA to   *
// * USERB is removed.  USERB is the grantee on only one node, from the system *
// * Second, USERA and USERB both had a grant to USERD; these grants need to   *
// * be combined.                                                              *
// *                                                                           *
// *   Essentially, the subtree (or branch) of USERB has been grafted into     *
// * the root, and all USERB leaf nodes have been removed.  Any duplicate      *
// * nodes, where USERA and USERB were grantors to the same grantee, have      *
// * been merged.                                                              *
// *                                                                           *
// *    The algorithm is therefore three steps:                                *
// *                                                                           *
// * 1) Delete any leaf nodes where the new owner is the grantee.              *
// * 2) Get the list of nodes where the original or the new owner is the       *
// *    grantor.  Merge any duplicates (update bitmaps), and update to new     *
// *    owner for old nodes.                                                   *
// * 3) Update grantee on system grant to new owner.                           *
// *                                                                           *
// *****************************************************************************

// 
// Delete the leaf nodes:
// 
// DELETE FROM OBJECT_PRIVIELGES 
//        WHERE OBJECT_UID = objectUID and GRANTEE_ID = granteeID
// 

PrivStatus privStatus = STATUS_GOOD;
ObjectPrivsMDTable objectPrivsTable(objectTableName_,pDiags_);
std::vector<PrivMgrMDRow *> rowList;
char buf[1000];

   sprintf(buf,"WHERE object_uid = %ld AND grantee_ID = %d",
           objectUID,newOwnerID);
          
   privStatus = objectPrivsTable.deleteWhere(buf); 
   
   if (privStatus != STATUS_GOOD)
      return privStatus;
   
   sprintf(buf, "WHERE object_uid = %ld AND (grantor_ID = %d OR grantor_ID = %d) ", 
           objectUID,newOwnerID,currentOwnerID);
   std::string orderByClause (" ORDER BY grantee_ID ");

   privStatus = objectPrivsTable.selectWhere(buf, orderByClause ,rowList);
   if (privStatus != STATUS_GOOD)
   {
      deleteRowList(rowList);
      return privStatus;
   }
         
   for (size_t i = rowList.size(); i > 0; i--)
   {
      ObjectPrivsMDRow &currentRow = static_cast<ObjectPrivsMDRow &>(*rowList[i - 1]);
      if (i == 1 && currentRow.grantorID_ == currentOwnerID)
      {
         currentRow.grantorID_ = newOwnerID;
         currentRow.grantorName_ = newOwnerName;
         privStatus = objectPrivsTable.updateRow(currentRow);
         if (privStatus != STATUS_GOOD)
         {
            deleteRowList(rowList);
            return privStatus;
         }
         continue;
      }   
         
      ObjectPrivsMDRow &previousRow = static_cast<ObjectPrivsMDRow &>(*rowList[i - 1]);
      
      // If both granted to the same ID, merge the rows, delete one,
      // and update the other.
      if (currentRow.granteeID_ == previousRow.granteeID_)
      {
         previousRow.privsBitmap_ |= currentRow.privsBitmap_;
         previousRow.grantableBitmap_ |= currentRow.grantableBitmap_;
         previousRow.grantorID_ = newOwnerID;
         previousRow.grantorName_ = newOwnerName;
         privStatus = objectPrivsTable.deleteRow(currentRow);
         if (privStatus != STATUS_GOOD)
         {
            deleteRowList(rowList);
            return privStatus;
         }
         privStatus = objectPrivsTable.updateRow(previousRow);
         if (privStatus != STATUS_GOOD)
         {
            deleteRowList(rowList);
            return privStatus;
         }
         i--;
         continue;
      }   

      // If this is a grant from the old owner, update to the new owner.
      if (currentRow.grantorID_ == currentOwnerID)
      {
         currentRow.grantorID_ = newOwnerID;
         currentRow.grantorName_ = newOwnerName;
         privStatus = objectPrivsTable.updateRow(currentRow);
         if (privStatus != STATUS_GOOD)
         {
            deleteRowList(rowList);
            return privStatus;
         }
         continue;
      }   
      
      // Grant from the new owner.  Will automatically be grafted into the
      // tree in the next step.
   }
   
   deleteRowList(rowList);
   
// Update the root node.
char setClause[1000];
char whereClause[1000];

   sprintf(setClause," SET GRANTEE_ID = %d, GRANTEE_NAME = '%s' ",
           newOwnerID,newOwnerName.c_str());
   sprintf(whereClause," WHERE GRANTOR_ID = %d ",SYSTEM_USER);
   
   privStatus = objectPrivsTable.updateWhere(setClause,whereClause);
   if (privStatus != STATUS_GOOD)
      return privStatus;
   
   return STATUS_GOOD;

}




// *****************************************************************************
// * Method: grantColumnPrivileges                                
// *                                                       
// *    Adds or updates a row in the COLUMN_PRIVILEGES table.
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <objectType> is the type of the subject object.
// *  <granteeID> is the unique identifier for the grantee
// *  <granteeName> is the name of the grantee (upper cased)
// *  <grantorName> is the name of the grantor (upper cased)
// *  <colPrivsArray> is the list of columns and privileges to grant
// *  <isWGOSpecified> is true then also allow the grantee to grant the set
// *                   of privileges to other grantees
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: Privileges were granted
// *           *: Unable to grant privileges, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::grantColumnPrivileges(
   const ComObjectType objectType,
   const int32_t granteeID,
   const std::string &granteeName,
   const std::string &grantorName,
   const std::vector<ColPrivSpec> & colPrivsArrayIn,
   const bool isWGOSpecified)
{

PrivStatus privStatus = STATUS_GOOD;
std::vector<ColPrivSpec> &colPrivsArray = 
   const_cast<std::vector<ColPrivSpec> &>(colPrivsArrayIn); 
  
  log (__FILE__, "checking column privileges", -1);

// generate the list of column privileges granted to the object and store in 
// class (columnRowList_)
  if (generateColumnRowList() == STATUS_ERROR)
    return STATUS_ERROR;

// get roleIDs for the grantor
std::vector<int_32> roleIDs;
  privStatus = getRoleIDsForUserID(grantorID_,roleIDs);
  if (privStatus == STATUS_ERROR)
    return privStatus;


// Determine if the grantor has WITH GRANT OPTION (WGO) for all the
// columns to be granted.  If not, return an error.
ObjectPrivsMDTable objectPrivsTable(objectTableName_,pDiags_);
ColumnPrivsMDTable columnPrivsTable(columnTableName_,pDiags_);

// Grantor may have WGO from two sources, object-level grants on the object,
// and column-level grants.  First check the object-level grants.  If there 
// are privileges still to grant, check for requisite column-level grants.
  
std::vector<ColPrivEntry> grantedColPrivs;

   if (!hasColumnWGO(colPrivsArrayIn,roleIDs,privStatus))
   {
      if (privStatus == STATUS_NOTFOUND)
         *pDiags_ << DgSqlCode(-CAT_PRIVILEGE_NOT_GRANTED);
      else
         PRIVMGR_INTERNAL_ERROR("Cannot fetch privileges");
      return STATUS_ERROR;   
   }


// Grantor has authority to grant all privileges requested.  See if some of
// the grants are already present. (may be adding WGO)
   
// Get existing column grants from grantor to the specified grantee.
  getColRowsForGranteeGrantor(columnRowList_,
                              granteeID,grantorID_,
                              grantedColPrivs);
                                                 
// Merge the column-privilege-to-grant entries (colPrivArray) into one entry 
// per column ordinal.
//
// Example: Given a commands such as 
//
// GRANT SELECT(COL4),INSERT(COL2,COL4) ON TAB TO USER;
// 
// three entries are generated by the parser, but only two rows are written;  
// one for column 2 (insert) and one for column 4 (insert and select).  
//
// Input may have same column ordinal in multiple entries, but the input is 
// guaranteed not to contain same ordinal and privType more than once.

std::vector<ColPrivEntry> colPrivsToGrant;

   for (size_t i = 0; i < colPrivsArray.size(); i++)
   {
      const ColPrivSpec &colPrivEntry = colPrivsArray[i];
      
      ColPrivEntry *existingEntry = findColumnEntry(colPrivsToGrant,
                                                    colPrivEntry.columnOrdinal);
      if (existingEntry != NULL)
      {
         existingEntry->privsBitmap.set(colPrivEntry.privType);
         if (isWGOSpecified)
            existingEntry->grantableBitmap.set(colPrivEntry.privType);
      }
      else
      {
         ColPrivEntry colPrivToGrant;
         
         colPrivToGrant.columnOrdinal = colPrivEntry.columnOrdinal;   
         colPrivToGrant.privsBitmap.set(colPrivEntry.privType);
         if (isWGOSpecified)
            colPrivToGrant.grantableBitmap.set(colPrivEntry.privType);
            
         colPrivsToGrant.push_back(colPrivToGrant);
      }
   }

// Walk the list of column privileges to grant, and either insert a new
// row in the COLUMN_PRIVILEGES table or update an existing row.  

bool rowWritten = false;

std::string whereBase(" WHERE object_uid = ");

   whereBase += UIDToString(objectUID_);
   whereBase += " AND grantor_id = ";
   whereBase += authIDToString(grantorID_);
   whereBase += " AND grantee_id = ";
   whereBase += authIDToString(granteeID);
   whereBase += " AND column_number = ";
    
   for (size_t i = 0; i < colPrivsToGrant.size(); i++)
   {
      ColPrivEntry &colPrivToGrant = colPrivsToGrant[i];
      bool updateOperation = false;
      bool skipOperation = false; 

      // Look for any existing granted privileges on the column for which
      // privileges are to be granted.
      for (size_t g = 0; g < grantedColPrivs.size(); g++)
      {
         const ColPrivEntry &grantedColPriv = grantedColPrivs[g];
         // See if there is an existing column privilege granted for this column.
         // If not, check the next granted column privilege.  If none are found
         // for this column, it is an insert operation.
         if (colPrivToGrant.columnOrdinal != grantedColPriv.columnOrdinal)
            continue;
            
         // An existing row with the same column has been found, it is one of four cases:
         //
         // 1) Adding a privilege (e.g., authID had SELECT, now granting INSERT) [update operation]
         // 2) AuthID had privilege, now adding WGO [update operation]
         // 3) AuthID already has privilege and/or WGO specified [skip operation]
         // 4) AuthID had privilege and WGO, now trying to take away WGO [error]
         
         // If the privilege bitmaps are not the same, adding a privilege.
         // This is an update operation, break out of for loop.
         if (colPrivToGrant.privsBitmap != grantedColPriv.privsBitmap)
         {
            updateOperation = true;  // Case #1
            colPrivToGrant.privsBitmap |= grantedColPriv.privsBitmap;  
            colPrivToGrant.grantableBitmap |= grantedColPriv.grantableBitmap; 
            break;
         }
         
         // Privilege bitmaps are the same, could be adding WGO.
         if (colPrivToGrant.grantableBitmap.any())
         {
            // If WGO was specified, and adding, this is an update.
            // If user already has WGO, it is a NOP, so skip this entry.
            if (colPrivToGrant.grantableBitmap == grantedColPriv.grantableBitmap)
            {
               skipOperation = true; //Case #3
               break;
            }
            // Adding WGO
            updateOperation = true; //Case #2
         }
         else // WGO not specified
         {
            // If user already has WGO, error.  Cannot revoke WGO via GRANT.
            if (grantedColPriv.grantableBitmap.any())
            {
               *pDiags_ << DgSqlCode(-CAT_PRIVILEGE_NOT_GRANTED);  //TODO: Add error for removing WGO in GRANT
               return STATUS_ERROR;  // Case #4
            }
            // WGO not specified, current privs same as privs to grant,
            // nothing to do.
            skipOperation = true; //Case #3
            break;
         }
         
         updateOperation = true;
         colPrivToGrant.privsBitmap |= grantedColPriv.privsBitmap;  
         colPrivToGrant.grantableBitmap |= grantedColPriv.grantableBitmap; 
         // Found an existing row for this column ordinal, so break out of loop.
         break;  
      }
      
      if (skipOperation)
         continue;
      
      ColumnPrivsMDRow row;  
       
      row.objectUID_ = objectUID_;
      row.objectName_ = objectName_;
      row.granteeID_ = granteeID;     
      row.granteeName_ = granteeName;
      row.grantorID_ = grantorID_; 
      row.grantorName_ = grantorName;
      row.privsBitmap_ = colPrivToGrant.privsBitmap;
      row.grantableBitmap_ = colPrivToGrant.grantableBitmap;
      row.columnOrdinal_ = colPrivToGrant.columnOrdinal;

      if (updateOperation)
         privStatus = columnPrivsTable.updateColumnRow(row,whereBase);
      else
         privStatus = columnPrivsTable.insert(row);
         
      if (privStatus == STATUS_ERROR)
         return privStatus;

      rowWritten = true;
   } 

//TODO: Could issue a warning if no privileges were granted; means all 
// requested grants already exist.
   
// if (!rowWritten)
      // Report Warning;
      
   return STATUS_GOOD;
  
}
//************* End of PrivMgrPrivileges::grantColumnPrivileges ****************



// *****************************************************************************
// * Method: grantObjectPriv                                
// *                                                       
// *    Adds or updates a row in the OBJECT_PRIVILEGES table.
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <objectType> is the type of the subject object.
// *  <granteeID> is the unique identifier for the grantee
// *  <granteeName> is the name of the grantee (upper cased)
// *  <grantorName> is the name of the grantor (upper cased)
// *  <privsList> is the list of privileges to grant
// *  <colPrivsArray> is the list of columns and privileges to grant
// *  <isAllSpecified> if true then all privileges valid for the object
// *                        type will be granted
// *  <isWGOSpecified> is true then also allow the grantee to grant the set
// *                   of privileges to other grantees
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: Privileges were granted
// *           *: Unable to grant privileges, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::grantObjectPriv(
      const ComObjectType objectType,
      const int32_t granteeID,
      const std::string &granteeName,
      const std::string &grantorName,
      const std::vector<PrivType> &privsList,
      const std::vector<ColPrivSpec> & colPrivsArray,
      const bool isAllSpecified,
      const bool isWGOSpecified)
{
  PrivStatus retcode = STATUS_GOOD;
 
  std::string traceMsg;
  log (__FILE__, "****** GRANT operation begins ******", -1);

  if (objectUID_ == 0)
  {
    PRIVMGR_INTERNAL_ERROR("objectUID is 0 for grant command");
    return STATUS_ERROR;
  }

  // If this grant request is called during the creation of the OBJECT_PRIVILEGES
  // table, just return okay.  Fixes a chicken and egg problem.
  char theQuote = '"';
  std::string nameRequested(objectName_);
  std::string nameToCheck(objectTableName_);

  // Delimited name issue.  The passed in objectName may enclose name parts in
  // double quotes even if the name part contains only [a-z][A-Z][0-9]_
  // characters. The same is true for the stored metadataLocation_.
  // To allow equality checks to work, we strip off the double quote delimiters
  // from both names. Fortunately, the double quote character is not allowed in
  // any SQL identifier except as delimiters - so this works.
  nameRequested.erase(std::remove(nameRequested.begin(), nameRequested.end(), theQuote), nameRequested.end());
  nameToCheck.erase(std::remove(nameToCheck.begin(), nameToCheck.end(), theQuote), nameToCheck.end());

  if (nameRequested == nameToCheck && grantorID_ == SYSTEM_USER)
    return STATUS_GOOD;

  // If the granting to self or DB__ROOT, return an error
  if (grantorID_ == granteeID || granteeID == ComUser::getRootUserID())
  {
    *pDiags_ << DgSqlCode(-CAT_CANT_GRANT_TO_SELF_OR_ROOT);
    return STATUS_ERROR;
  }

  // generate the list of privileges granted to the object and store in class
  if (generateObjectRowList() == STATUS_ERROR)
    return STATUS_ERROR;

  // get roleIDs for the grantor
  std::vector<int_32> roleIDs;
  retcode = getRoleIDsForUserID(grantorID_,roleIDs);
  if (retcode == STATUS_ERROR)
    return retcode;

  if (!colPrivsArray.empty())
  {
    retcode = grantColumnPrivileges(objectType,granteeID,granteeName,grantorName,
                                    colPrivsArray,isWGOSpecified);
    if (retcode != STATUS_GOOD)
      return retcode;
    // If only column-level privileges were specified, no problem.  
    if (privsList.empty())
    {
      log (__FILE__, "****** GRANT operation succeeded ******", -1);
      return STATUS_GOOD;
    }
  }
  
  // verify the privileges list and create a desc to contain them
  PrivMgrDesc privsToGrant(granteeID);
  retcode = convertPrivsToDesc(objectType, 
                               isAllSpecified, 
                               isWGOSpecified, 
                               false,
                               privsList, 
                               privsToGrant); 
  if (retcode != STATUS_GOOD)
    return retcode;

  // check for circular dependency.  If USERX grants to USERY WGO, then USERY 
  // cannot grant back to USERX. Theoretically, USERX can grant select, update 
  // to USERY and USERY can grant delete, insert to USERX but for simplicity, 
  // we will reject the request independent on the set of privileges involved.
  std::set<int32_t> listOfGrantors;
  getTreeOfGrantors(grantorID_, listOfGrantors); 

  // If we find the grantee in the list of grantors, return an error
  if (listOfGrantors.find(granteeID) != listOfGrantors.end())
  {
    *pDiags_ << DgSqlCode(-CAT_CIRCULAR_PRIVS)
             << DgString0(grantorName.c_str())
             << DgString1(granteeName.c_str());
    return STATUS_ERROR;
  }

  // get privileges for the grantor and make sure the grantor can grant
  // at least one of the requested privileges
  //
  // SQL Ansi states that privileges that can be granted should be done so
  // even if some requested privilege are not grantable.
  PrivMgrDesc privsOfTheGrantor(grantorID_);
  bool hasManagePrivileges;
  retcode = getUserPrivs(objectType, grantorID_, roleIDs, privsOfTheGrantor, 
                         hasManagePrivileges, NULL ); 
  if (retcode != STATUS_GOOD)
    return retcode;
  
  // If null, the grantor has no privileges
  if ( privsOfTheGrantor.isNull() )
  {
     *pDiags_ << DgSqlCode (-CAT_PRIVILEGE_NOT_GRANTED);
     return STATUS_ERROR;
   }

  // Remove any privsToGrant which are not held GRANTABLE by the Grantor.
  // TBD: if not all privileges are grantable, should at least report
  //      which ones were not granted.
  bool warnNotAll = false;
  if ( privsToGrant.limitToGrantable( privsOfTheGrantor ) )
  {
    // limitToGrantable true ==> some specified privs were not grantable.
    if ( isAllSpecified )
    {
      // This is ok.  Can specify ALL without having ALL.
    }
    else
      warnNotAll = true;  // Not all the specified privs are grantable.
  }

  // If nothing left to grant, we are done.
  if ( privsToGrant.isNull() )
  {
    *pDiags_ << DgSqlCode (-CAT_PRIVILEGE_NOT_GRANTED);
    return STATUS_ERROR;
  }

  // See if grantor has previously granted privileges to the grantee
  bool foundRow = false;

  ObjectPrivsMDRow row;
  retcode = getGrantedPrivs(granteeID, row);
  if (retcode == STATUS_NOTFOUND)
    foundRow = false;
  else if (retcode == STATUS_GOOD)
    foundRow = true;
  else
    return retcode;

  // if privileges exist, set currentPrivs to existing list
  PrivMgrCoreDesc currentPrivs; // creates an empty descriptor
  if (foundRow)
  {
    PrivMgrCoreDesc tempPrivs(row.privsBitmap_, row.grantableBitmap_);
    currentPrivs = tempPrivs;
  }

  PrivMgrCoreDesc savedOriginalPrivs = currentPrivs;

  // get the list of additional privileges to grant
  // some privileges may have already been granted
  PrivMgrDesc privsToAdd = privsToGrant;
  PrivMgrCoreDesc::PrivResult result = privsToAdd.grantTablePrivs(currentPrivs);

  // nothing to grant - everything is already granted
  if ( result == PrivMgrCoreDesc::NONE )
    return STATUS_GOOD;

  // Internal consistency check.  We should have granted something.
  assert( result != PrivMgrCoreDesc::NEUTRAL );

  // There is something to grant, update/insert metadata

  // set up row if it does not exist
  if (!foundRow)
  {
    row.objectUID_ = objectUID_;
    row.objectName_ = objectName_;
    row.objectType_ = objectType;
    row.granteeID_ = granteeID;
    row.granteeName_ = granteeName;
    row.granteeType_ = USER_GRANTEE_LIT;
    row.grantorID_ = grantorID_;
    row.grantorName_ = grantorName;
    row.grantorType_ = USER_GRANTOR_LIT;
    row.privsBitmap_ = privsToGrant.getTablePrivs().getPrivBitmap();
    row.grantableBitmap_ = privsToGrant.getTablePrivs().getWgoBitmap();
  }

  // combine privsToGrant with existing privs
  else
  {
    privsToGrant.unionOfPrivs(currentPrivs);
    row.privsBitmap_ = privsToGrant.getTablePrivs().getPrivBitmap();
    row.grantableBitmap_ = privsToGrant.getTablePrivs().getWgoBitmap();
  }

  ObjectPrivsMDTable objectPrivsTable (objectTableName_, pDiags_);
  char buf[1000];

  if (foundRow)
  {
    ObjectUsage objectUsage;
    objectUsage.objectUID = objectUID_;
    objectUsage.objectOwner = granteeID;
    objectUsage.objectName = row.objectName_;
    objectUsage.objectType = row.objectType_;

    PrivMgrDesc originalPrivs (row.granteeID_);
    originalPrivs.setTablePrivs(savedOriginalPrivs);
    objectUsage.originalPrivs = originalPrivs;
    objectUsage.updatedPrivs = privsToGrant;
  
    // get list of all objects that need to change, the table object and 
    // views
    std::vector<ObjectUsage *> listOfObjects;
    PrivCommand command = PrivCommand::GRANT_OBJECT;
    retcode = getAffectedObjects(objectUsage, command, listOfObjects);
    if (retcode == STATUS_ERROR)
    {
      deleteListOfAffectedObjects(listOfObjects);
      return retcode;
    }

    traceMsg = "updating all affected objects, number of objects is ";
    traceMsg += to_string((long long int)listOfObjects.size());
    log (__FILE__, traceMsg, -1);

    // update the OBJECT_PRIVILEGES row for each effected object
    for (size_t i = 0; i < listOfObjects.size(); i++)
    {
      ObjectUsage *pObj = listOfObjects[i];

      pObj->describe(traceMsg);
      log (__FILE__, traceMsg, i);

      int32_t theGrantor = (pObj->objectType == COM_VIEW_OBJECT) ? SYSTEM_USER : grantorID_;
      int32_t theGrantee = pObj->objectOwner;
      int64_t theUID = pObj->objectUID;
      PrivMgrCoreDesc thePrivs = pObj->updatedPrivs.getTablePrivs();
  
      sprintf(buf, "where grantee_id = %d and grantor_id =  %d and object_uid = %ld",
              theGrantee, theGrantor, theUID);
      std::string whereClause (buf);

      sprintf(buf, "set privileges_bitmap  = %ld, grantable_bitmap =  %ld",
              thePrivs.getPrivBitmap().to_ulong(),
              thePrivs.getWgoBitmap().to_ulong());
      std::string setClause (buf);
      // update the row
      retcode = objectPrivsTable.updateWhere(setClause, whereClause);
      if (retcode == STATUS_ERROR)
      {
        deleteListOfAffectedObjects(listOfObjects);
        return retcode;
      }
    }
    deleteListOfAffectedObjects(listOfObjects);
  }
  else
  {
    row.describeRow(traceMsg);
    traceMsg.insert(0, "adding new privilege row ");
    log (__FILE__, traceMsg, -1);

    // insert the row
    retcode = objectPrivsTable.insert(row);
  }

  log (__FILE__, "****** GRANT operation succeeded ******", -1);

  return retcode;
}

// *****************************************************************************
// * Method: grantObjectPriv                                
// *                                                       
// *    Adds or update a row in the OBJECT_PRIVILEGES table representing the
// *    owner privileges.  The privileges and grantable bitmaps as passed in.
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <objectType> is the type of the subject object.
// *  <granteeID> is the unique identifier for the grantee
// *  <granteeName> is the name of the grantee (upper cased)
// *  <privBitmap> is the list of privileges to grant
// *  <grantableBitmap> is the grantable privileges to grant
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: Privileges were granted
// *           *: Unable to grant privileges, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::grantObjectPriv(
      const ComObjectType objectType,
      const int32_t granteeID,
      const PrivObjectBitmap privsBitmap,
      const PrivObjectBitmap grantableBitmap)
{
  PrivStatus retcode = STATUS_GOOD;

  if (objectUID_ == 0)
  {
    PRIVMGR_INTERNAL_ERROR("objectUID is 0 for grant command");
    return STATUS_ERROR;
  }

  // get the associated grantorName and granteeName
  std::string grantorName;
  if (!getAuthNameFromAuthID(grantorID_, grantorName))
    return STATUS_ERROR;

  std::string granteeName;
  if (!getAuthNameFromAuthID(granteeID, granteeName))
    return STATUS_ERROR;

  // set up the values of the row to insert
  ObjectPrivsMDRow row;
  row.objectUID_ = objectUID_;
  row.objectName_ = objectName_;
  row.objectType_ = objectType;
  row.granteeID_ = granteeID;
  row.granteeName_ = granteeName;
  row.granteeType_ = USER_GRANTEE_LIT;
  row.grantorID_ = grantorID_;
  row.grantorName_ = grantorName;
  row.grantorType_ = (grantorID_ == SYSTEM_USER) ? SYSTEM_GRANTOR_LIT : USER_GRANTOR_LIT;
  row.privsBitmap_ = privsBitmap;
  row.grantableBitmap_ = grantableBitmap;

  // Insert the new row, the row should not exist since the request
  // is coming during the creation of a new object.
  ObjectPrivsMDTable objectPrivsTable (objectTableName_, pDiags_);
  retcode = objectPrivsTable.insert(row);

  return retcode;
}

// *****************************************************************************
// * Method: grantToOwners                                
// *                                                       
// *   Performs the initial grant from the system to the owner.  For private 
// * schemas, where the creator is not the schema/object owner, a grant from 
// * the object owner to the creator is also performed.                                                     
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <objectType> is the type of the subject object.
// *  <granteeID> is the unique identifier for the grantee
// *  <granteeName> is the name of the grantee (upper cased)
// *  <ownerID> is the unique identifier for the owner of the object
// *  <ownerName> is the name of the owner (upper cased)
// *  <creatorID> is the unique identifier for the creator of the object
// *  <creatorName> is the name of the creator (upper cased)
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: All DML privs were granted 
// *           *: Not all privs were granted.  Error in CLI diags area.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::grantToOwners(
   const ComObjectType objectType,
   const Int32 granteeID,
   const std::string & granteeName,
   const Int32 ownerID,
   const std::string & ownerName,
   const Int32 creatorID,
   const std::string & creatorName)

{

ObjectPrivsMDRow row;
PrivMgrCoreDesc corePrivs;
PrivObjectBitmap privsBitmap; 
PrivObjectBitmap grantableBitmap; 

   corePrivs.setAllObjectGrantPrivilege(objectType,true);
   privsBitmap = corePrivs.getPrivBitmap();
   grantableBitmap = corePrivs.getWgoBitmap();
   
// Add the root grant from the system.   
   row.objectUID_ = objectUID_;
   row.objectName_ = objectName_;
   row.objectType_ = objectType;
   row.granteeID_ = ownerID;
   row.granteeName_ = ownerName;
   row.granteeType_ = USER_GRANTEE_LIT;
   row.grantorID_ = SYSTEM_USER;
   row.grantorName_ = SYSTEM_AUTH_NAME;  
   row.grantorType_ = COM_SYSTEM_GRANTOR_LIT;
   row.privsBitmap_ = privsBitmap;
   row.grantableBitmap_ = grantableBitmap;
 
ObjectPrivsMDTable objectPrivsTable(objectTableName_,pDiags_);

PrivStatus privStatus = objectPrivsTable.insert(row);

   if (privStatus != STATUS_GOOD)
      return privStatus;

// If the owner and creator are the same, we are done.
// If not, this is an object being created in a private schema, and 
// we need to grant privileges to the creator.  If the creator is DB__ROOT,
// no need to grant privileges.
// 
// This creator grant may be controlled by a CQD in the future.
   if (ownerID == creatorID || creatorID == ComUser::getRootUserID())
      return STATUS_GOOD;
 
// Add a grant from the private schema owner to the creator.     
   row.grantorID_ = row.granteeID_;
   row.grantorName_ = row.granteeName_;
   row.grantorType_ = USER_GRANTOR_LIT;
   row.granteeID_ = creatorID;
   row.granteeName_ = creatorName;
      
   return objectPrivsTable.insert(row); 
  
}

// *****************************************************************************
// * Method: insertPrivRowsForObject                                
// *                                                       
// *    writes rows that add grants of privileges for an object.
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <objectPrivsRows> One or more rows of grants for the object.
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD    : Rows were returned
// * STATUS_NOTFOUND: No rows were returned
// *               *: Unable to read privileges, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::insertPrivRowsForObject(
   const int64_t objectUID,
   const std::vector<ObjectPrivsRow> & objectPrivsRows)
   
{
  PrivStatus retcode = STATUS_GOOD;

  if (objectUID_ == 0)
  {
    PRIVMGR_INTERNAL_ERROR("objectUID is 0 for insertPrivRowsForObject()");
    return STATUS_ERROR;
  }

  ObjectPrivsMDTable objectPrivsTable(objectTableName_,pDiags_);
  
  for (int32_t i = 0; i < objectPrivsRows.size();++i)
  {
    ObjectPrivsMDRow row;
    const ObjectPrivsRow &rowIn = objectPrivsRows[i];
    char granteeTypeString[3] = {0};
    char grantorTypeString[3] = {0};
    
    row.objectUID_ = objectUID_;
    row.objectName_ = rowIn.objectName;
    row.objectType_ = rowIn.objectType;
    row.granteeID_ = rowIn.granteeID;
    row.granteeName_ = rowIn.granteeName;
    CmGetComGranteeAsLit(rowIn.granteeType,granteeTypeString);
    row.granteeType_ = granteeTypeString;
    
    row.grantorID_ = rowIn.grantorID;
    row.grantorName_ = rowIn.grantorName;
    CmGetComGrantorAsLit(rowIn.grantorType,grantorTypeString);
    row.grantorType_ = grantorTypeString;
    
    row.privsBitmap_ = rowIn.privilegesBitmap;
    row.grantableBitmap_ = rowIn.grantableBitmap;
    
    retcode = objectPrivsTable.insert(row);
    if (retcode != STATUS_GOOD)
       return retcode;
  }

  return retcode;
}



// ****************************************************************************
// method:  dealWithConstraints
//
// This method finds all the constraints associated with the table and 
// determines if any are adversely affected by the privilege change.
//
// Params:
//   objectUsage - the affected object
//   listOfAffectedObjects - returns the list of affected objects
//
// Returns: PrivStatus                                               
//    STATUS_GOOD: No problems were encountered
//              *: Errors were encountered, ComDiags area is set up
//                                                                
// ****************************************************************************
PrivStatus PrivMgrPrivileges::dealWithConstraints(
  const ObjectUsage &objectUsage,
  std::vector<ObjectUsage *> &listOfAffectedObjects)
{
  PrivStatus retcode = STATUS_GOOD;

  std::string traceMsg;
  objectUsage.describe(traceMsg);
  traceMsg.insert (0, "checking referencing constraints for ");
  log (__FILE__, traceMsg, -1);

  // RI constraints can only be defined for base tables
  if (objectUsage.objectType != COM_BASE_TABLE_OBJECT)
    return STATUS_GOOD;
  
  // get the underlying tables for all RI constraints that reference the object
  std::vector<ObjectReference *> objectList;
  PrivMgrMDAdmin admin(trafMetadataLocation_, metadataLocation_, pDiags_);
  retcode = admin.getReferencingTablesForConstraints(objectUsage, objectList);
  traceMsg = "getting constraint usages: number usages found ";
  traceMsg += to_string((long long int)objectList.size());
  traceMsg += ", retcode is ";
  traceMsg += privStatusEnumToLit(retcode);
  log (__FILE__, traceMsg, -1);

  if (retcode == STATUS_ERROR)
    return retcode;

  // objectList contain ObjectReferences for all tables that reference the
  // ObjectUsage (object losing privilege) through an RI constraint
  PrivMgrDesc originalPrivs;
  PrivMgrDesc currentPrivs;
  int32_t lastObjectOwnerID = 0;
  std::vector<int32_t> roleIDs;
  
  for (size_t i = 0; i < objectList.size(); i++)
  {
    ObjectReference *pObj = objectList[i];
    
    pObj->describe(traceMsg);
    log (__FILE__, traceMsg, i);

    if (lastObjectOwnerID != pObj->objectOwner)
    {
      roleIDs.clear();
      retcode = getRoleIDsForUserID(pObj->objectOwner,roleIDs);
      if (retcode == STATUS_ERROR)
        return retcode;
    }

    // get the summarized original and current privs for the referencing table 
    // current privs contains any adjustments
    retcode = summarizeCurrentAndOriginalPrivs(pObj->objectUID,
                                               pObj->objectOwner,
                                               roleIDs,
                                               listOfAffectedObjects,
                                               originalPrivs,
                                               currentPrivs);
    if (retcode != STATUS_GOOD)
      return retcode;

    // If the underlying table no long has REFERENCES privileges return
    // a dependency error.
    PrivMgrCoreDesc thePrivs = objectUsage.updatedPrivs.getTablePrivs();
    if (!thePrivs.getPriv(REFERENCES_PRIV))
    {
      std::string referencingTable;
      if (admin.getConstraintName(objectUsage.objectUID, pObj->objectUID, referencingTable) == false)
      {
        referencingTable = "UNKNOWN, Referencing table ID is ";
        referencingTable += UIDToString(pObj->objectUID);
      }

      *pDiags_ << DgSqlCode (-CAT_DEPENDENT_OBJECTS_EXIST)
               << DgString0 (referencingTable.c_str());
      return STATUS_ERROR;
    }
  }

  return STATUS_GOOD;
}
 
  
// ****************************************************************************
// method:  dealWithUdrs
//
// This method finds all the udrs associated with the library and 
// determines if any are adversely affected by the privilege change.
//
// Params:
//   objectUsage - the affected object
//   listOfAffectedObjects - returns the list of affected objects
//
// Returns: PrivStatus                                               
//    STATUS_GOOD: No problems were encountered
//              *: Errors were encountered, ComDiags area is set up
//                                                                
// ****************************************************************************
PrivStatus PrivMgrPrivileges::dealWithUdrs(
  const ObjectUsage &objectUsage,
  std::vector<ObjectUsage *> &listOfAffectedObjects)
{
  PrivStatus retcode = STATUS_GOOD;

  // udrs (functions and procedures) can only be defined for in libraries
  if (objectUsage.objectType != COM_LIBRARY_OBJECT)
    return STATUS_GOOD;

  std::string traceMsg;
  objectUsage.describe(traceMsg);
  traceMsg.insert (0, "checking referencing routines for ");
  log (__FILE__, traceMsg, -1);

  // get the udrs that reference the library for the grantee
  std::vector<ObjectReference *> objectList;
  PrivMgrMDAdmin admin(trafMetadataLocation_, metadataLocation_, pDiags_);
  retcode = admin.getUdrsThatReferenceLibrary(objectUsage, objectList);
  traceMsg = "getting routine usages: number usages found ";
  traceMsg += to_string((long long int)objectList.size());
  traceMsg += ", retcode is ";
  traceMsg += privStatusEnumToLit(retcode);
  log (__FILE__, traceMsg, -1);

  if (retcode == STATUS_ERROR)
    return retcode;

  // objectList contain ObjectReferences for all udrs that reference the
  // ObjectUsage (object losing privilege) through a library
  PrivMgrDesc originalPrivs;
  PrivMgrDesc currentPrivs;

  if (objectList.size() > 0)
  {
    std::vector<int32_t> roleIDs;
    retcode = getRoleIDsForUserID(objectUsage.objectOwner,roleIDs);
    if (retcode == STATUS_ERROR)
      return retcode;
    
    // if the grantee owns any udrs, get the summarized original and current
    // privs for the library
    // current privs contains any adjustments
    retcode = summarizeCurrentAndOriginalPrivs(objectUsage.objectUID,
                                               objectUsage.objectOwner,
                                               roleIDs,
                                               listOfAffectedObjects,
                                               originalPrivs,
                                               currentPrivs);
    if (retcode != STATUS_GOOD)
      return retcode;

    // If the udr can no longer be created due to lack of USAGE privilege,
    // return a dependency error.
    PrivMgrCoreDesc thePrivs = objectUsage.updatedPrivs.getTablePrivs();
    if (!thePrivs.getPriv(USAGE_PRIV))
    {
      // There could be multiple udrs, just pick the first one in the list
      // for the error message.
      ObjectReference *pObj = objectList[0];
      *pDiags_ << DgSqlCode (-CAT_DEPENDENT_OBJECTS_EXIST)
               << DgString0 (pObj->objectName.c_str());
      return STATUS_ERROR;
    }
  }

  return STATUS_GOOD;
}


// ****************************************************************************
// method:  dealWithViews
//
// This method finds all the views that referenced the object.
// This method recursively calls itself to find other views referenced in the
// tree of referencing views
//
// Params:
//   objectUsage - the affected object
//   command - type of command - grant, revoke restrict, revoke cascade
//   listOfAffectedObjects - returns the list of affected objects
//
// Returns: PrivStatus                                               
//    STATUS_GOOD: No problems were encountered
//              *: Errors were encountered, ComDiags area is set up
//                                                                 
// In the future, we want to cache the lists of objects instead of going to the
// metadata everytime.
// ****************************************************************************
PrivStatus PrivMgrPrivileges::dealWithViews(
  const ObjectUsage &objectUsage,
  const PrivCommand command,
  std::vector<ObjectUsage *> &listOfAffectedObjects)
{
  PrivStatus retcode = STATUS_GOOD;
  std::string traceMsg;
  objectUsage.describe(traceMsg);
  traceMsg.insert (0, "checking referencing views for ");
  log (__FILE__, traceMsg, -1);

  // Get any views that referenced this object to see if the privilege changes 
  // should be propagated
  std::vector<ViewUsage> viewUsages;
  PrivMgrMDAdmin admin(trafMetadataLocation_, metadataLocation_, pDiags_);
  retcode = admin.getViewsThatReferenceObject(objectUsage, viewUsages);
  traceMsg = "getting view usages: number usages found ";
  traceMsg += to_string((long long int)viewUsages.size());
  traceMsg += ", retcode is ";
  traceMsg += privStatusEnumToLit(retcode);
  log (__FILE__, traceMsg, -1);
  if (retcode == STATUS_NOTFOUND)
   return STATUS_GOOD;
  if (retcode != STATUS_GOOD && retcode != STATUS_WARNING)
    return retcode;

  // for each entry in the viewUsages list calculate the changed
  // privileges and call dealWithViews recursively
  for (size_t i = 0; i < viewUsages.size(); i++)
  {
   
    ViewUsage viewUsage = viewUsages[i];

    viewUsage.describe(traceMsg);
    log (__FILE__, traceMsg, i);

    // this method recreates privileges for the view based on the original
    // and the current.  Updated descriptors are stored in the viewUsage
    // structure.
    retcode = gatherViewPrivileges(viewUsage, listOfAffectedObjects);
    traceMsg = "gathered view privs: retcode is ";
    traceMsg += privStatusEnumToLit(retcode);
    log (__FILE__, traceMsg, -1);

    if (retcode != STATUS_GOOD && retcode != STATUS_WARNING)
    {
      return retcode;
    }

    // check to see if privileges changed
    if (viewUsage.originalPrivs == viewUsage.updatedPrivs)
    {}
    else
    {
      // this view is affected by the grant/revoke request, add to list
      // and check to see if anything down stream needs to change
      ObjectUsage *pUsage = new (ObjectUsage);
      pUsage->objectUID = viewUsage.viewUID;
      pUsage->objectOwner = viewUsage.viewOwner;
      pUsage->objectName = viewUsage.viewName;
      pUsage->objectType = COM_VIEW_OBJECT;
      pUsage->originalPrivs = viewUsage.originalPrivs;
      pUsage->updatedPrivs = viewUsage.updatedPrivs;
      listOfAffectedObjects.push_back(pUsage);

      traceMsg = "adding new objectUsage for ";
      pUsage->describe(traceMsg);
      log (__FILE__, traceMsg, i);

      retcode = dealWithViews(*pUsage, command, listOfAffectedObjects);
      if (retcode != STATUS_GOOD && retcode != STATUS_WARNING)
        return retcode;
    }
  } 
  
  return STATUS_GOOD;
}

// ----------------------------------------------------------------------------
// method: gatherViewPrivileges
//
// This method gathers privileges for the view both the original and current
//
// parameters:
//   viewUsage - description of the view
//   listOfAffectedObjects - list of changed privileges so far
//
// Returns: PrivStatus                                               
//                                                                    
// STATUS_GOOD: Privileges were gathered
//           *: Unable to gather privileges, see diags.     
//                                                                 
// ----------------------------------------------------------------------------
PrivStatus PrivMgrPrivileges::gatherViewPrivileges(
  ViewUsage &viewUsage,
  const std::vector<ObjectUsage *> listOfAffectedObjects)
{
  PrivStatus retcode = STATUS_GOOD;
  std::string traceMsg;

  // initialize summarized descriptors and set all applicable privileges
  // TBD:  if view is not updatable, should initialize correctly.
  // views have same privileges as tables
  bool setWGOtrue = true;
  PrivMgrDesc summarizedOriginalPrivs;
  summarizedOriginalPrivs.setAllTableGrantPrivileges(setWGOtrue);
  PrivMgrDesc summarizedCurrentPrivs;
  summarizedCurrentPrivs.setAllTableGrantPrivileges(setWGOtrue);

  // Get list of objects referenced by the view
  std::vector<ObjectReference *> objectList;
  PrivMgrMDAdmin admin(trafMetadataLocation_, metadataLocation_, pDiags_);
  retcode = admin.getObjectsThatViewReferences(viewUsage, objectList);
  traceMsg += "getting object references: number references found ";
  traceMsg += to_string((long long int)objectList.size());
  traceMsg += ", retcode is ";
  traceMsg += privStatusEnumToLit(retcode);
  log (__FILE__, traceMsg, -1);
  if (retcode == STATUS_ERROR)
    return retcode;


  // For each referenced object, summarize the original and current
  // privileges
  PrivMgrDesc originalPrivs;
  PrivMgrDesc currentPrivs;
  int32_t lastObjectOwnerID = 0;
  std::vector<int32_t> roleIDs;
 
  for (size_t i = 0; i < objectList.size(); i++)
  {
    ObjectReference *pObj = objectList[i];

    pObj->describe(traceMsg);
    log (__FILE__, traceMsg, i);

    if (lastObjectOwnerID != pObj->objectOwner)
    {
      roleIDs.clear();
      retcode = getRoleIDsForUserID(pObj->objectOwner,roleIDs);
      if (retcode == STATUS_ERROR)
        return retcode;
    }
    // get the summarized original and current privs for the 
    // referenced object that have been granted to the view owner
    // listOfAffectedObjects contain the privilege adjustments needed
    //   to generate the current privs
    retcode = summarizeCurrentAndOriginalPrivs(pObj->objectUID,
                                               viewUsage.viewOwner, 
                                               roleIDs,
                                               listOfAffectedObjects,
                                               originalPrivs,
                                               currentPrivs);
    if (retcode != STATUS_GOOD)
      return retcode;

    // If user no longer has select privilege on referenced object
    // returns an error
    // When cascade is supported, then referenced objects will be removed
    //if (command == PrivCommand::REVOKE_OBJECT_RESTRICT )
    //{
      PrivMgrCoreDesc thePrivs = currentPrivs.getTablePrivs();
      if (!thePrivs.getPriv(SELECT_PRIV))
      {
         *pDiags_ << DgSqlCode (-CAT_DEPENDENT_OBJECTS_EXIST)
                  << DgString0 (viewUsage.viewName.c_str());
         return STATUS_ERROR;
      }
    //}

    // add the returned privilege to the summarized privileges
    // for all objects
    summarizedOriginalPrivs.intersectionOfPrivs(originalPrivs);
    summarizedCurrentPrivs.intersectionOfPrivs(currentPrivs);

    originalPrivs.resetTablePrivs();
    currentPrivs.resetTablePrivs();
  }

  // Update view usage with summarized privileges
  viewUsage.originalPrivs = summarizedOriginalPrivs;
  viewUsage.updatedPrivs = summarizedCurrentPrivs;

  return STATUS_GOOD;
}

// ****************************************************************************
// method: generateColumnRowList
//
// generate the list of privileges granted to columns of the object and 
// store in columnRowList_ class member
//
// The list is ordered by grantor/grantee/column_number
//
// Returns:
//   STATUS_GOOD     - list of rows was generated
//   STATUS_NOTFOUND - no column privileges were found
//   STATUS_ERROR    - the diags area is populated with any errors
// ****************************************************************************
PrivStatus PrivMgrPrivileges::generateColumnRowList()
{
  std::string whereClause ("where object_uid = ");
  whereClause += UIDToString(objectUID_);
  std::string orderByClause (" order by grantor_id, grantee_id, column_number ");

  ColumnPrivsMDTable columnPrivsTable(columnTableName_,pDiags_);
  PrivStatus privStatus = 
   columnPrivsTable.selectWhere(whereClause, orderByClause, columnRowList_);

  std::string traceMsg ("getting column privileges, number privileges is ");
  traceMsg += to_string((long long int)columnRowList_.size());
  log (__FILE__, traceMsg, -1);
  for (size_t i = 0; i < columnRowList_.size(); i++)
  {
    ColumnPrivsMDRow privRow = static_cast<ColumnPrivsMDRow &> (*columnRowList_[i]);
    privRow.describeRow(traceMsg);
    log (__FILE__, traceMsg, i);
  }

  return privStatus;
}

// ****************************************************************************
// method: generateObjectRowList
//
// generate the list of privileges granted to the object and 
// store in objectRowList_ class member
//
// The list is ordered by grantor/grantee
//
// Returns:
//   STATUS_GOOD     - list of rows was generated
//   STATUS_NOTFOUND - in most cases, there should be at least one row in the
//                     OBJECT_PRIVILEGES table.  But there is at least one case
//                     where this is not true - trying to get privilege info
//                     for indexes when using the table (index_table) syntax.
//   STATUS_ERROR    - the diags area is populated with any errors
// ****************************************************************************
PrivStatus PrivMgrPrivileges::generateObjectRowList()
{
  std::string whereClause ("where object_uid = ");
  whereClause += UIDToString(objectUID_);
  std::string orderByClause(" order by grantor_id, grantee_id ");

  ObjectPrivsMDTable objectPrivsTable(objectTableName_,pDiags_);
  PrivStatus privStatus = 
    objectPrivsTable.selectWhere(whereClause, orderByClause, objectRowList_);
  std::string traceMsg ("getting object privileges, number privleges is ");
  traceMsg += to_string((long long int)objectRowList_.size());
  log (__FILE__, traceMsg, -1);
  for (size_t i = 0; i < objectRowList_.size(); i++)
  {
    ObjectPrivsMDRow privRow = static_cast<ObjectPrivsMDRow &> (*objectRowList_[i]);
    privRow.describeRow(traceMsg);
    log (__FILE__, traceMsg, i);
  }
  return privStatus;
}


// ****************************************************************************
// method:  getAffectedObjects
//
// This method adds the current object to the listOfAffectedObjects and then
// looks for dependent objects such as constraints and views that will be 
// affected by the privilege change.
//
// Params:
//   objectUsage - the affected object
//   command - GRANT or REVOKE RESTRICT or REVOKE CASCADE
//   listOfAffectedObjects - returns the list of affected objects
// ****************************************************************************
PrivStatus PrivMgrPrivileges::getAffectedObjects(
  const ObjectUsage &objectUsage,
  const PrivCommand command,
  std::vector<ObjectUsage *> &listOfAffectedObjects)
{
  PrivStatus retcode = STATUS_GOOD;
  std::string traceMsg;

  // found an object whose privileges need to be updated
  ObjectUsage *pUsage = new (ObjectUsage);
  pUsage->objectUID = objectUsage.objectUID;
  pUsage->objectOwner = objectUsage.objectOwner;
  pUsage->objectName = objectUsage.objectName;
  pUsage->objectType = objectUsage.objectType;
  pUsage->originalPrivs = objectUsage.originalPrivs;
  pUsage->updatedPrivs = objectUsage.updatedPrivs;
  listOfAffectedObjects.push_back(pUsage); 

  // Find list of affected constraints
  if (command != PrivCommand::GRANT_OBJECT)  
  {
    retcode = dealWithConstraints (objectUsage, listOfAffectedObjects);
    if (retcode != STATUS_GOOD && retcode != STATUS_WARNING)
     return retcode;

    retcode = dealWithUdrs (objectUsage, listOfAffectedObjects);
    if (retcode != STATUS_GOOD && retcode != STATUS_WARNING)
     return retcode;

  }

  // Find list of affected views
  retcode = dealWithViews (objectUsage, command, listOfAffectedObjects);
  if (retcode != STATUS_GOOD && retcode != STATUS_WARNING)
    return retcode;

  return STATUS_GOOD;
}

// ----------------------------------------------------------------------------
// method: getGrantedPrivs
//
// This method reads the metadata to get privilege information for the
// object, grantor, and grantee.
//
// input:  granteeID
// output: a row from the object_privileges table describing privilege details
//
//  Returns: PrivStatus                                               
//                                                                   
//      STATUS_GOOD: row was found (and returned)
//  STATUS_NOTFOUND: no privileges have been granted
// ----------------------------------------------------------------------------
PrivStatus PrivMgrPrivileges::getGrantedPrivs(
  const int32_t granteeID,
  PrivMgrMDRow &rowOut)
{
  ObjectPrivsMDRow & row = static_cast<ObjectPrivsMDRow &>(rowOut);

  for (size_t i = 0; i < objectRowList_.size(); i++)
  {
    ObjectPrivsMDRow privRow = static_cast<ObjectPrivsMDRow &> (*objectRowList_[i]); 
    if (privRow.grantorID_ == grantorID_ && privRow.granteeID_ == granteeID)
    {
      row = privRow;
      return STATUS_GOOD;
    } 
  }

  return STATUS_NOTFOUND;
}
 
// ----------------------------------------------------------------------------
// method: getGrantorDetailsForObject
//
// returns the effective grantor ID and grantor name for grant and revoke
// object statements
//
// Input:
//   isGrantedBySpecified - true if grant request included a GRANTED BY clause
//   grantedByName - name specified in GRANTED BY clause
//   objectOwner - owner of object that is the subject for the grant or revoke
// 
// Output:
//   effectiveGrantorID - the ID to use for grant and revoke
//   effectiveGrantorName - the name to use for grant and revoke
//
// returns PrivStatus with the results of the operation.  The diags area 
// contains error details.
// ----------------------------------------------------------------------------
PrivStatus PrivMgrPrivileges::getGrantorDetailsForObject(
   const bool isGrantedBySpecified,
   const std::string grantedByName,
   const int_32 objectOwner,
   int_32 &effectiveGrantorID,
   std::string &effectiveGrantorName)
   
{

int_32 currentUser = ComUser::getCurrentUser();
short retcode = 0;
  
  if (!isGrantedBySpecified)
  {
    // If the user is DB__ROOT, a grant or revoke operation is implicitly on
    // behalf of the object owner.  Likewise, if a user has been granted the
    // MANAGE_PRIVILEGES component-level privilege they can grant on 
    // behalf of the owner implicitly.  Otherwise, the grantor is the user.
    if (!ComUser::isRootUserID())
    {
      PrivMgrComponentPrivileges componentPrivileges(metadataLocation_,pDiags_);

      if (!componentPrivileges.hasSQLPriv(currentUser,SQLOperation::MANAGE_PRIVILEGES,
                                          true))
      {
        effectiveGrantorName = ComUser::getCurrentUsername();
        effectiveGrantorID = currentUser;
        return STATUS_GOOD; 
      }
    }
    // User is DB__ROOT.  Get the effective grantor name.
    char authName[MAX_USERNAME_LEN+1];
    Int32 actualLen = 0;
    retcode = ComUser::getAuthNameFromAuthID(objectOwner,authName,
                                             MAX_USERNAME_LEN,actualLen);
    if (retcode != FEOK)
    {
      *pDiags_ << DgSqlCode(-20235)
               << DgInt0(retcode)
               << DgInt1(objectOwner);
      return STATUS_ERROR;
    }
    effectiveGrantorID = objectOwner;
    effectiveGrantorName = authName;
    return STATUS_GOOD;
  }
  
// GRANTED BY was specified, first see if authorization name is valid.  Then  
// determine if user has authority to use the clause.

// Get the grantor ID from the grantorName
  retcode = ComUser::getAuthIDFromAuthName(grantedByName.c_str(),effectiveGrantorID);

  if (retcode == FENOTFOUND)
  {
    *pDiags_ << DgSqlCode(-CAT_AUTHID_DOES_NOT_EXIST_ERROR)
             << DgString0(grantedByName.c_str());
    return STATUS_ERROR;
  }

  if (retcode != FEOK)
  {
    *pDiags_ << DgSqlCode(-20235)
             << DgInt0(retcode)
             << DgInt1(objectOwner);
    return STATUS_ERROR;
  }
  effectiveGrantorName = grantedByName;
  
// Name exists, does user have authority?  
// 
// GRANTED BY is allowed if any of the following are true:
//
// 1) The user is DB__ROOT.
// 2) The user is owner of the object.
// 3) The user has been granted the MANAGE_PRIVILEGES component-level privilege.
// 4) The grantor is a role and the user has been granted the role.

  if (ComUser::isRootUserID() || currentUser == objectOwner)
    return STATUS_GOOD;

PrivMgrComponentPrivileges componentPrivileges(metadataLocation_,pDiags_);

  if (componentPrivileges.hasSQLPriv(currentUser,SQLOperation::MANAGE_PRIVILEGES,
                                     true))
    return STATUS_GOOD;

// If the grantor is not a role, user does not have authority.    
  if (!isRoleID(effectiveGrantorID))
  {
    *pDiags_ << DgSqlCode(-CAT_NOT_AUTHORIZED);
    return STATUS_ERROR;
  }

// Role specified in BY clause must be granted to the current user for user
// to have authority.
PrivMgrRoles roles(trafMetadataLocation_,metadataLocation_,pDiags_);

  if (roles.hasRole(currentUser,effectiveGrantorID))
    return STATUS_GOOD;

  *pDiags_ << DgSqlCode(-CAT_NOT_AUTHORIZED);
  return STATUS_ERROR;
    
}

// *****************************************************************************
// * Method: revokeColumnPrivileges                                
// *                                                       
// *    Adds or updates a row in the COLUMN_PRIVILEGES table.
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <objectType> is the type of the subject object.
// *  <granteeID> is the unique identifier for the grantee
// *  <granteeName> is the name of the grantee (upper cased)
// *  <grantorName> is the name of the grantor (upper cased)
// *  <colPrivsArray> is the list of columns and privileges to grant
// *  <isWGOSpecified> is true then also allow the grantee to grant the set
// *                   of privileges to other grantees
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: Privileges were revoked
// *           *: Unable to revoke privileges, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::revokeColumnPrivileges(
   const ComObjectType objectType,
   const int32_t granteeID,
   const std::string & granteeName,
   const std::string & grantorName,
   const std::vector<ColPrivSpec> & colPrivsArrayIn,
   const bool isWGOSpecified)
{

  PrivStatus privStatus = STATUS_GOOD;

  log (__FILE__, "checking column privileges", -1);

  std::vector<ColPrivSpec> &colPrivsArray = 
    const_cast<std::vector<ColPrivSpec> &>(colPrivsArrayIn); 
  ColumnPrivsMDTable columnPrivsTable(columnTableName_,pDiags_);
  std::string privilege;
  std::vector<ColPrivEntry> grantedColPrivs;

  // get the list of column privileges for the object
  if (generateColumnRowList() == STATUS_ERROR)
    return STATUS_ERROR;

  // First verify the grantor has granted all the privileges they wish to revoke.
  // If not, report the first privilege that cannot be revoked.
   if (!hasGrantedColumnPriv(columnRowList_,grantorID_,granteeID,
                             colPrivsArrayIn,privStatus,privilege,grantedColPrivs))
   {
      if (privStatus == STATUS_NOTFOUND)
      {
         std::string privOnObject(privilege + " on ");
         
         privOnObject += objectName_;
         
         *pDiags_ << DgSqlCode(-CAT_GRANT_NOT_FOUND) 
                  << DgString0(privOnObject.c_str()) 
                  << DgString1(grantorName.c_str()) 
                  << DgString2(granteeName.c_str());
         return STATUS_ERROR;
      }
   
      return privStatus;
   }
   
// Create a privsToRevoke array using the passed in revoke entries and the
// list of currently granted column privileges.  Combine multiple privileges 
// for the same column into one entry.

std::vector<ColPrivEntry> colPrivsToRevoke;

   for (size_t i = 0; i < colPrivsArray.size(); i++)
   {
      const ColPrivSpec &colPrivSpecEntry = colPrivsArray[i];
      
      ColPrivEntry *existingEntry = findColumnEntry(colPrivsToRevoke,
                                                    colPrivSpecEntry.columnOrdinal);
      if (existingEntry != NULL)
         existingEntry->privsBitmap.set(colPrivSpecEntry.privType);
      else
      {
         ColPrivEntry colPrivToRevoke;
         
         colPrivToRevoke.columnOrdinal = colPrivSpecEntry.columnOrdinal;   
         colPrivToRevoke.privsBitmap.set(colPrivSpecEntry.privType);
            
         colPrivsToRevoke.push_back(colPrivToRevoke);
      }
   }
   
// At this point we have an array of privsToRevoke with column ordinal and 
// priv bitmap.
//
// Three revoke column cases:
// 
//   Spec    Spec Priv bitmap compare 
// GOF Priv  to granted priv bitmap    Action
//  T    1          NA                 Removing WGO only.  Update operation.  
//                                     Reset privType bit in grantable bitmap,
//                                     copy priv bitmap from granted privs.
//
//  F    1        Equal                Revoking all privs on this column, plus
//                                     WGO.  Delete operation.
//
//  F    1       Not equal             Revoking some privs on this column plus
//                                     WGO for the revoked privs.  Reset bits in 
//                                     both bitmaps.  Update operation.


//TODO: When revoking WGO, need to check for dependent objects, e.g. views.

bool rowRevoked = false;
PrivColumnBitmap revokedPrivs;

std::string whereBase(" WHERE object_uid = ");

   whereBase += UIDToString(objectUID_);
   whereBase += " AND grantor_id = ";
   whereBase += authIDToString(grantorID_);
   whereBase += " AND grantee_id = ";
   whereBase += authIDToString(granteeID);
   whereBase += " AND column_number = ";
    
   for (size_t i = 0; i < colPrivsToRevoke.size(); i++)
   {
      ColPrivEntry &colPrivToRevoke = colPrivsToRevoke[i];
      bool updateRow = false;
      bool deleteRow = false;

      // Look for any existing granted privileges on the column for which
      // privileges are to be granted.
      for (size_t g = 0; g < grantedColPrivs.size(); g++)
      {
         const ColPrivEntry &grantedColPriv = grantedColPrivs[g];
         // See if there is an existing column privilege granted for this column.
         // If not, check the next granted column privilege.  If none are found
         // for this column, it is an internal error.
         if (colPrivToRevoke.columnOrdinal != grantedColPriv.columnOrdinal)
            continue;
            
         // Found row with grant for this column.
         
         // Verify privilge(s) being revoked was/were granted.  If not, internal error.
         if ((colPrivToRevoke.privsBitmap & grantedColPriv.privsBitmap) == 0)
         {
            PRIVMGR_INTERNAL_ERROR("Privilege to revoke not found");
            return STATUS_ERROR;
         }
         
         if (isWGOSpecified)
         {
            // We want to clear the corresponding bits in the grantable bitmap. 
            // Flip the bits of the privs to revoke bitmap, then and the
            // negation with the current grantable bitmap. Not revoking any   
            // privileges, so update with current priv bitmap.
            PrivColumnBitmap revokeBitmap = ~colPrivToRevoke.privsBitmap; 
            colPrivToRevoke.privsBitmap = grantedColPriv.privsBitmap;
            colPrivToRevoke.grantableBitmap = grantedColPriv.grantableBitmap & revokeBitmap;
            updateRow = true;
         }
         else
         {
            if (colPrivToRevoke.privsBitmap == grantedColPriv.privsBitmap)
               deleteRow = true;
            else
            {
               PrivColumnBitmap revokeBitmap = ~colPrivToRevoke.privsBitmap; 
               colPrivToRevoke.privsBitmap = grantedColPriv.privsBitmap & revokeBitmap;
               colPrivToRevoke.grantableBitmap = grantedColPriv.grantableBitmap & revokeBitmap;
               updateRow = true;
            }
            revokedPrivs |= colPrivToRevoke.privsBitmap; 
         }
         break;   
      }
      
      if (deleteRow)
      {
         std::string whereClause(whereBase + authIDToString(colPrivToRevoke.columnOrdinal));
      
         privStatus = columnPrivsTable.deleteWhere(whereClause);
         if (privStatus == STATUS_ERROR)
            return privStatus;
      
         rowRevoked = true;
         continue;
      }
      
      if (!updateRow)
      {
         PRIVMGR_INTERNAL_ERROR("Column privilege not found to revoke");
         return STATUS_ERROR;
      }   
      
      ColumnPrivsMDRow row;  
       
      row.objectUID_ = objectUID_;
      row.objectName_ = objectName_;
      row.granteeID_ = granteeID;     
      row.granteeName_ = granteeName;
      row.grantorID_ = grantorID_; 
      row.grantorName_ = grantorName;
      row.privsBitmap_ = colPrivToRevoke.privsBitmap;
      row.grantableBitmap_ = colPrivToRevoke.grantableBitmap;
      row.columnOrdinal_ = colPrivToRevoke.columnOrdinal;

      privStatus = columnPrivsTable.updateColumnRow(row,whereBase);
         
      if (privStatus == STATUS_ERROR)
         return privStatus;
      
      rowRevoked = true;
   } 
   
// Send revoked privs to RMS
SQL_QIKEY siKeyList[NBR_DML_COL_PRIVS];
size_t siIndex = 0;

   for (size_t i = FIRST_DML_COL_PRIV; i <= LAST_DML_COL_PRIV; i++ )
   {
      if (!revokedPrivs.test(PrivType(i)))
         continue;
         
      ComSecurityKey secKey(granteeID,objectUID_,PrivType(i),
                            ComSecurityKey::OBJECT_IS_OBJECT);
      
      siKeyList[siIndex].revokeKey.subject = secKey.getSubjectHashValue();
      siKeyList[siIndex].revokeKey.object = secKey.getObjectHashValue();
      std::string actionString;
      secKey.getSecurityKeyTypeAsLit(actionString);
      strncpy(siKeyList[siIndex].operation,actionString.c_str(),2);
      siIndex++;                          
   }      
   
   if (siIndex > 0)   
      SQL_EXEC_SetSecInvalidKeys(siIndex,siKeyList);
      
//   if (!rowRevoked)
   // Warning
 //     ;
      
   return STATUS_GOOD;

}
//************* End of PrivMgrPrivileges::revokeColumnPrivileges ***************

// *****************************************************************************
// * Method: revokeObjectPriv                                
// *                                                       
// *    Deletes or updates a row in the OBJECT_PRIVILEGES table.
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <objectType> is the type of the subject object.
// *  <granteeID> is the unique identifier for the grantee
// *  <privsList> is the list of privileges to revoke
// *  <isAllSpecified> if true then all privileges valid for the object
// *                        type will be revoked
// *  <isGOFSpecified> if true then remove the ability for  the grantee 
// *                   to revoke the set of privileges to other grantees
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: Privileges were granted
// *           *: Unable to grant privileges, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::revokeObjectPriv (const ComObjectType objectType,
                                                const int32_t granteeID,
                                                const std::string & granteeName,
                                                const std::string & grantorName,
                                                const std::vector<PrivType> &privsList,
                                                const std::vector<ColPrivSpec> & colPrivsArray,
                                                const bool isAllSpecified,
                                                const bool isGOFSpecified)
{
  PrivStatus retcode = STATUS_GOOD;

  std::string traceMsg;
  log (__FILE__, "****** REVOKE operation begins ******", -1);

  if (objectUID_ == 0)
  {
    PRIVMGR_INTERNAL_ERROR("objectUID is 0 for revoke command");
    return STATUS_ERROR;
  }

  // get roleIDs for grantor
  std::vector<int_32> roleIDs;
  retcode = getRoleIDsForUserID(grantorID_,roleIDs);
  if (retcode == STATUS_ERROR)
    return retcode;

  if (!colPrivsArray.empty())
  {
    retcode = revokeColumnPrivileges(objectType,granteeID,granteeName,
                                     grantorName,colPrivsArray,isGOFSpecified);
    if (retcode != STATUS_GOOD)
      return retcode;
    
    // If only column-level privileges were specified, no problem.  
    if (privsList.empty())
    {
      log (__FILE__, "****** REVOKE operation succeeded ******", -1);
      return STATUS_GOOD;
    }
  }
  
  // Convert the privsList into a PrivMgrDesc
  // convertPrivsToDesc sets up any errors in the diags area
  
  // revokeWGOWithPriv and isGOFSpecified interaction:
  //    isGOFSpecified is true if only GRANT OPTION FOR is being revoked.
  //       The privilege will still be available but the user can no longer
  //       grant the privilege to others.
  //    revokeWGOWithPriv is always set to true.  This means that both the
  //       priv and wgo is revoked.  It does not make sense to revoke the priv
  //       and not the WITH GRANT OPTION option.
  bool revokeWGOWithPriv = true;
  PrivMgrDesc privsToRevoke(granteeID);
  retcode = convertPrivsToDesc(objectType, 
                               isAllSpecified, 
                               revokeWGOWithPriv, 
                               isGOFSpecified, 
                               privsList, 
                               privsToRevoke); 
  if (retcode != STATUS_GOOD)
    return retcode;


  // get all privilege descriptors for the object
  if (generateObjectRowList() == STATUS_ERROR)
    return STATUS_ERROR;

  // get privileges for the grantor and make sure the grantor can revoke
  // at least one of the requested privileges
  PrivMgrDesc privsOfTheGrantor(grantorID_);
  bool hasManagePrivileges;
  retcode = getUserPrivs(objectType, grantorID_, roleIDs, privsOfTheGrantor, 
                         hasManagePrivileges, NULL ); 
  if (retcode != STATUS_GOOD)
    return retcode;

  // If null, the grantor has no privileges
  if ( privsOfTheGrantor.isNull() )
  {
     *pDiags_ << DgSqlCode (-CAT_PRIVILEGE_NOT_REVOKED);
     return STATUS_ERROR;
   }

  // Remove any privsToRevoke which are not held grantable by the Grantor.
  // If limitToGrantable returns true, some privs are not revokable.
  bool warnNotAll = false;
  if ( privsToRevoke.limitToGrantable( privsOfTheGrantor ) )
  {
    if ( isAllSpecified )
    {
      // This is ok.  Can specify ALL without having all privileges set.
    }
    else
      warnNotAll = true;  // Not all the specified privs can be revoked
  }

  // If nothing left to revoke, we are done.
  if ( privsToRevoke.isNull() )
  {
    *pDiags_ << DgSqlCode (-CAT_PRIVILEGE_NOT_REVOKED);
    return STATUS_ERROR;
  }

  // See if grantor has previously granted privileges to the grantee
  ObjectPrivsMDRow row;
  retcode = getGrantedPrivs(granteeID, row);
  if (retcode == STATUS_NOTFOUND)
  {
    // Set up parameters for the error message: privileges, grantor, & grantee
    // privilege list
    std::string privListStr;
    for (size_t i = 0; i < privsList.size(); i++)
      privListStr += PrivMgrUserPrivs::convertPrivTypeToLiteral(privsList[i]) + ", ";
    
    // Remove the last ", "
    privListStr.erase(privListStr.length()-2, privListStr.length());
    if (isGOFSpecified)
      privListStr += " WITH GRANT OPTION";

    *pDiags_ << DgSqlCode (CAT_GRANT_NOT_FOUND)
             << DgString0 (privListStr.c_str())
             << DgString1 (grantorName.c_str())
             <<DgString2 (granteeName.c_str());
    return STATUS_WARNING;
  }
  if (retcode != STATUS_GOOD)
    return retcode;

  // if privileges exist, set currentPrivs to existing list
  // save a copy of the original privs
  PrivMgrCoreDesc currentPrivs; // creates an empty descriptor
  PrivMgrCoreDesc tempPrivs(row.privsBitmap_, row.grantableBitmap_);
  currentPrivs = tempPrivs;
  PrivMgrCoreDesc savedOriginalPrivs = currentPrivs;

  // TDB:  if user privs have already been revoked, just return
  
  // save the privsToRevoke for query invalidation(QI) later
  PrivMgrDesc listOfRevokedPrivileges = privsToRevoke;
 
  // merge requested changes with existing row
  // First flip privsToRevoke to turn off the privilege and then union 
  // the current privs with the privsToRevoke to generate the final bitmaps
  privsToRevoke.complement();
  privsToRevoke.intersectionOfPrivs(currentPrivs);

  row.privsBitmap_ = privsToRevoke.getTablePrivs().getPrivBitmap();
  row.grantableBitmap_ = privsToRevoke.getTablePrivs().getWgoBitmap();

  // See if there are any dependencies that need to be removed before
  // removing the privilege
  ObjectUsage objectUsage;
  objectUsage.objectUID = objectUID_;
  objectUsage.objectOwner = granteeID;
  objectUsage.objectName = row.objectName_;
  objectUsage.objectType = row.objectType_;

  PrivMgrDesc originalPrivs (row.granteeID_);
  originalPrivs.setTablePrivs(savedOriginalPrivs);
  objectUsage.originalPrivs = originalPrivs;
  objectUsage.updatedPrivs = privsToRevoke;

  // get list of dependent objects that need to change 
  std::vector<ObjectUsage *> listOfObjects;
  retcode = getAffectedObjects(objectUsage, 
                               PrivCommand::REVOKE_OBJECT_RESTRICT, 
                               listOfObjects);
  if (retcode == STATUS_ERROR)
  {
    deleteListOfAffectedObjects(listOfObjects);
    return retcode;
  }

  char buf[1000];
  ObjectPrivsMDTable objectPrivsTable (objectTableName_, pDiags_);
  ColumnPrivsMDTable columnPrivsTable(columnTableName_,pDiags_);

  // update the OBJECT_PRIVILEGES row for each effected object
  for (size_t i = 0; i < listOfObjects.size(); i++)
  {
    ObjectUsage *pObj = listOfObjects[i];
    PrivMgrCoreDesc thePrivs = pObj->updatedPrivs.getTablePrivs();

    int32_t theGrantor = grantorID_;
    int32_t theGrantee = pObj->objectOwner;
    int64_t theUID = pObj->objectUID;

    sprintf(buf, "where grantee_id = %d and grantor_id =  %d and object_uid = %ld",
            theGrantee, theGrantor, theUID);
    std::string whereClause (buf);

    if (thePrivs.isNull())
    {
      pObj->describe(traceMsg);
      traceMsg.insert (0, "deleted object usage ");

      // delete the row
      retcode = objectPrivsTable.deleteWhere(whereClause);
      if (retcode == STATUS_ERROR)
      {
        deleteListOfAffectedObjects(listOfObjects);
        return retcode;
      }
      // Delete any corresponding column-level privileges.
      retcode = columnPrivsTable.deleteWhere(whereClause);
      if (retcode == STATUS_ERROR)
      {
        deleteListOfAffectedObjects(listOfObjects);
        return retcode;
      }
    }
    else
    {
      sprintf(buf, "set privileges_bitmap  = %ld, grantable_bitmap =  %ld",
              thePrivs.getPrivBitmap().to_ulong(),
              thePrivs.getWgoBitmap().to_ulong());
      std::string setClause (buf);

      pObj->describe(traceMsg);
      traceMsg.insert (0, "updated object usage ");
      // update the row
      retcode = objectPrivsTable.updateWhere(setClause, whereClause);
      if (retcode == STATUS_ERROR)
      {
        deleteListOfAffectedObjects(listOfObjects);
        return retcode;
      }
      // Update any corresponding column-level privileges.
      retcode = columnPrivsTable.updateWhere(setClause,whereClause);
      if (retcode == STATUS_ERROR)
      {
        deleteListOfAffectedObjects(listOfObjects);
        return retcode;
      }
    }
  }
  
  deleteListOfAffectedObjects(listOfObjects);
  
  // Go rebuild the privilege tree to see if it is broken
  // If it is broken, return an error
  if (checkRevokeRestrict (row, objectRowList_))
    return STATUS_ERROR;

  // Send a message to the Trafodion RMS process about the revoke operation.
  // RMS will contact all master executors and ask that cached privilege 
  // information be re-calculated
  retcode = sendSecurityKeysToRMS(granteeID, listOfRevokedPrivileges);

  // SQL Ansi states that privileges that can be revoked should be done so
  // even if some requested privilege are not revokable.
  // TDB:  report which privileges were not revoked
  if (warnNotAll)
    *pDiags_ << DgSqlCode(CAT_NOT_ALL_PRIVILEGES_REVOKED);

  log (__FILE__, "****** REVOKE operation succeeded ******", -1);

  return retcode;
}

// *****************************************************************************
// * Method: revokeObjectPriv                                
// *                                                       
// *    Deletes rows in the OBJECT_PRIVILEGES table associated with the object
// *    This code assumes that all dependent and referencing objects such as
// *    views have been (or will be) dropped.  No extra checks are performed.
// *                                                       
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: Privileges were revoked
// *           *: Unable to revoke privileges, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::revokeObjectPriv ()
{
  PrivStatus retcode = STATUS_GOOD;

  if (objectUID_ == 0)
  {
    PRIVMGR_INTERNAL_ERROR("objectUID is 0 for revoke command");
    return STATUS_ERROR;
  }

  char buf[100];
  sprintf(buf, "where object_uid = %ld", objectUID_);
  std::string whereClause  = buf;

  // delete all the rows for this object
  ObjectPrivsMDTable objectPrivsTable (objectTableName_, pDiags_);
  retcode = objectPrivsTable.deleteWhere(whereClause);

  return retcode;
}

// ----------------------------------------------------------------------------
// method: checkRevokeRestrict
//
// This method starts at the beginning of the privilege tree and rebuilds
// it from top to bottom.  If the revoke causes part of the tree to be 
// unaccessible (a broken branch), it returns true; otherwise, revoke can 
// proceed - returns false.
//
// Params:
//     rowIn -   the row containing proposed changes from the requested
//               revoke statement.
//     rowList - a list of all the rows associated with the object
//
//  true - unable to perform revoke because of dependencies
//  false - able to perform revoke.privileges
//
// The diags area is set up with where the tree was broken
// ---------------------------------------------------------------------------- 
bool PrivMgrPrivileges::checkRevokeRestrict ( 
  PrivMgrMDRow &rowIn,
  std::vector <PrivMgrMDRow *> &rowList )
{
  // Search the list of privileges associated with the object and replace 
  // the bitmaps of the current row with the bitmaps of the row sent in (rowIn).  
  // At the same time, clear visited_ and set current_ to row values
  ObjectPrivsMDRow updatedRow = static_cast<ObjectPrivsMDRow &>(rowIn);

  std::string traceMsg;
  log (__FILE__, "checking grant tree for broken branches", -1);

  for (int32_t i = 0; i < rowList.size(); i++)
  {
    //  if rowIn matches this row, then update the bitmaps to use the 
    // updated bitmaps
    ObjectPrivsMDRow &currentRow = static_cast<ObjectPrivsMDRow &> (*rowList[i]);

    if (updatedRow.granteeID_ == currentRow.granteeID_ &&
        updatedRow.grantorID_ == currentRow.grantorID_ )
    {
      currentRow.privsBitmap_ = updatedRow.privsBitmap_;
      currentRow.grantableBitmap_ = updatedRow.grantableBitmap_;
    } 
    // reset visited_ and current_ PrivMgrCoreDesc
    currentRow.clearVisited();
    currentRow.setToOriginal();
  }

  // Reconstruct the privilege tree 
  // Each privilege tree starts with the root - system grantor (-2)
  for ( size_t i = 0; i < NBR_OF_PRIVS; i++ )
  {
    PrivType pType = PrivType(i);

    int32_t systemGrantor = SYSTEM_USER;
    scanObjectBranch (pType, systemGrantor, rowList);
    // TDB - add a scan for column privileges
  }

  // If a branch of the tree was not visited, then we have a broken
  // tree.  Therefore, revoke restrict will leave abandoned privileges
  // in the case, return true.
  bool  notVisited = false;
  for (size_t i = 0; i < rowList.size(); i++)
  {
    ObjectPrivsMDRow &currentRow = static_cast<ObjectPrivsMDRow &> (*rowList[i]);
    currentRow.describeRow(traceMsg);
    log (__FILE__, traceMsg, i);

    if (currentRow.anyNotVisited())
    {
      *pDiags_ << DgSqlCode(-CAT_DEPENDENT_PRIV_EXISTS)
               << DgString0(currentRow.grantorName_.c_str())
               << DgString1(currentRow.granteeName_.c_str());

      log (__FILE__, "found a branch that is not accessible", -1);
      notVisited = true;
      break;
    }
  }
  return notVisited;
}

// ----------------------------------------------------------------------------
//  method:  scanObjectBranch 
// 
//   scans the privsList entries for match on Grantor,
//   keeping track of which priv/wgo entries have been encountered
//   by setting "visited" flag in the entry.
//
//   For each entry discovered, set visited flag to indicate that
//   priv and wgo were seen.  For wgo, if the wgo visited flag has not
//   already been set, call scanObjectBranch recursively with this grantee
//   as grantor.  (By observing the state of the wgo visited flag
//   we avoid redundantly exploring the sub-tree rooted in a grantor
//   which has already been discovered as having wgo from some other
//   ancestor grantor.)
//
//   This algorithm produces a depth-first scan of all nodes of the 
//   directed graph of privilege settings which can currently be reached
//   by an uninterrupted chain of wgo values.
//
//   The implementation is dependent on the fact that PrivsList 
//   entries are ordered by Grantor, Grantee, and within each of these 
//   by Primary uid value, type.  Entries for system grantor (_SYSTEM) are the
//   first entries in the list.
//
// -----------------------------------------------------------------------------
void PrivMgrPrivileges::scanObjectBranch( const PrivType pType, // in
                  const int32_t& grantor,              // in
                  const std::vector<PrivMgrMDRow *> & privsList  )   // in
{

  // The PrivMgrMDRow <list> is maintained in order by
  //  Grantee within Grantor value - through an order by clause.

  // Skip over Grantors lower than the specified one.
  size_t i = 0;
  while (  i < privsList.size() )
  {
    ObjectPrivsMDRow &currentRow = static_cast<ObjectPrivsMDRow &> (*privsList[i]);
    if (currentRow.grantorID_ < grantor)
     i++;
   else
     break;
  }

  // For matching Grantor, process each Grantee.
  while (  i < privsList.size() )
  {
    ObjectPrivsMDRow &privEntry = static_cast<ObjectPrivsMDRow &> (*privsList[i]);
    if (privEntry.grantorID_ == grantor)
    {
      PrivMgrCoreDesc& current = privEntry.accessCurrent();
      if ( current.getPriv(pType) )
      {
         // This grantee has priv.  Set corresponding visited flag.
         PrivMgrCoreDesc& visited = privEntry.accessVisited();
         visited.setPriv(pType, true);

         if ( current.getWgo(pType))
         {
              // This grantee has wgo.  
            if ( visited.getWgo(pType) )
              {   // Already processed this subtree.
              }
            else
              {
                visited.setWgo(pType, true);

                int32_t thisGrantee( privEntry.granteeID_ );
                if ( ComUser::isPublicUserID(thisGrantee) )
                  scanPublic( pType, //  Deal with PUBLIC grantee wgo.
                              privsList );
                else
                  {
                    int32_t granteeAsGrantor(thisGrantee);
                    scanObjectBranch( pType, // Scan for this grantee as grantor.
                                 granteeAsGrantor,
                                 privsList );
                  }
              }
         }  // end this grantee has wgo
      }  // end this grantee has this priv

      i++;  // on to next privsList entry
    }
    else
      break;

  }  // end scan privsList over Grantees for this Grantor
}  // end scanCurrent


/* *******************************************************************
   scanPublic --  a grant wgo to PUBLIC has been encountered for the 
   current privilege type, so *all* users are able to grant this privilege.
   Scan the privsList for all grantees who have this priv from any grantor,
   marking each such entry as visited.

****************************************************************** */

void PrivMgrPrivileges::scanPublic( const PrivType pType, // in
                 const std::vector<PrivMgrMDRow *>& privsList )    // in
{
     // PUBLIC has a priv wgo.  So *every* grant of this priv
     //   is allowed, by any Grantor.

   for ( size_t i = 0; i < privsList.size(); i++ )
   {
      ObjectPrivsMDRow &privEntry = static_cast<ObjectPrivsMDRow &> (*privsList[i]);
      const PrivMgrCoreDesc& current = privEntry.accessCurrent();
      if ( current.getPriv(pType) )
      {
           // This grantee has priv.  Set corresponding visited flag.
         PrivMgrCoreDesc& visited = privEntry.accessVisited();
         visited.setPriv(pType, true);

         if ( current.getWgo(pType) )
           visited.setWgo(pType, true);
      }
   }  // end scan privsList over all Grantees/Grantors
} // end scanPublic


// ****************************************************************************
// method: sendSecurityKeysToRMS
//
// This method generates a security key for each privilege revoked for the
// grantee.  It then makes a cli call sending the keys.
// SQL_EXEC_SetSecInvalidKeys will send the security keys to RMS and RMS
// sends then to all the master executors.  The master executors check this
// list and recompile any queries to recheck privileges.
//
// input:
//    granteeID - the UID of the user losing privileges
//       the granteeID is stored in the PrivMgrDesc class - extra?
//    listOfRevokePrivileges - the list of privileges that were revoked
//
// Returns: PrivStatus                                               
//                                                                  
// STATUS_GOOD: Privileges were granted
//           *: Unable to send keys,  see diags.     
// ****************************************************************************
PrivStatus PrivMgrPrivileges::sendSecurityKeysToRMS(
  const int32_t granteeID, 
  const PrivMgrDesc &listOfRevokedPrivileges)
{
  // Go through the list of table privileges and generate SQL_QIKEYs
#if 0
  // Only need to generate keys for SELECT, INSERT, UPDATE, and DELETE
  std::vector<ComSecurityKey *> keyList;
  PrivMgrCoreDesc privs = listOfRevokedPrivileges.getTablePrivs();
  for ( size_t i = 0; i < NBR_OF_PRIVS; i++ )
  {
    PrivType pType = PrivType(i);
    if (pType == SELECT_PRIV || pType == INSERT_PRIV || 
        pType == UPDATE_PRIV || pType == DELETE_PRIV)
    {
      if (privs.getPriv(pType))
      {
        ComSecurityKey *key = new ComSecurityKey(granteeID, 
                                                 objectUID_,
                                                 pType, 
                                                 ComSecurityKey::OBJECT_IS_OBJECT);
        if (key->isValid())
          keyList.push_back(key);
        else
        {
           // Probably should report a different error.  Is an error possible?
          *pDiags_ << DgSqlCode (-CAT_NOT_AUTHORIZED);
          return STATUS_ERROR;
        }
      }
    }
  }
#endif
  std::vector<ComSecurityKey *> keyList;
  PrivMgrCoreDesc privs = listOfRevokedPrivileges.getTablePrivs();
  PrivStatus privStatus = buildSecurityKeys(granteeID,privs,keyList);
  if (privStatus != STATUS_GOOD)
     return privStatus;
  // TDB: add column privileges
  
  // Create an array of SQL_QIKEYs
  int32_t numKeys = keyList.size();
  SQL_QIKEY siKeyList[numKeys];
  for (size_t j = 0; j < keyList.size(); j++)
  {
    ComSecurityKey *pKey = keyList[j];
    siKeyList[j].revokeKey.subject = pKey->getSubjectHashValue();
    siKeyList[j].revokeKey.object = pKey->getObjectHashValue();
    std::string actionString;
    pKey->getSecurityKeyTypeAsLit(actionString);
    strncpy(siKeyList[j].operation, actionString.c_str(), 2);
  }
  
  // delete the security list
  for(size_t k = 0; k < keyList.size(); k++)
   delete keyList[k]; 
  keyList.clear();

  // Call the CLI to send details to RMS
  SQL_EXEC_SetSecInvalidKeys(numKeys, siKeyList);

  return STATUS_GOOD;
}


// *****************************************************************************
// * Method: populateObjectPriv                                
// *                                                       
// *    Inserts rows into the OBJECT_PRIVILEGES table during initialization to
// *    reflect object owner privileges
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <objectLocation> the location of the Trafodion OBJECTS table which is
// *                   used to extract all the objects
// *  <authsLocation> the location of the Trafodion AUTHS table which is used
// *                  to map owner IDs to grantees
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: Privileges were inserted
// *           *: Unable to insert privileges, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::populateObjectPriv(
   const std::string &objectsLocation,
   const std::string &authsLocation)
{
  // bug - sometimes, if don't wait, the insert command
  // does not find rows to insert 
  //sleep(60);
  ObjectPrivsMDTable objectPrivsTable(objectTableName_, pDiags_);
  return objectPrivsTable.insertSelect(objectsLocation, authsLocation);
}

// *****************************************************************************
// * Method: getPrivBitmaps                                
// *                                                       
// *    Reads the OBJECT_PRIVILEGES table to get the privilege bitmaps for 
// *    rows matching a where clause.
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <whereClause> specifies the rows to be returned
// *  <orderByClause> specifies the order of the rows to be returned
// *  <privBitmaps> passes back a vector of bitmaps.
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: Privileges were returned
// *           *: Unable to read privileges, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::getPrivBitmaps(
   const std::string & whereClause,
   const std::string & orderByClause,
   std::vector<PrivObjectBitmap> & privBitmaps)
   
{

std::vector<PrivMgrMDRow *> rowList;

ObjectPrivsMDTable objectPrivsTable(objectTableName_,pDiags_);
 
PrivStatus privStatus = objectPrivsTable.selectWhere(whereClause, orderByClause,rowList);

   if (privStatus != STATUS_GOOD)
   {
      deleteRowList(rowList);
      return privStatus;
   }
   
   for (size_t r = 0; r < rowList.size(); r++)
   {
      ObjectPrivsMDRow &row = static_cast<ObjectPrivsMDRow &>(*rowList[r]);
      privBitmaps.push_back(row.privsBitmap_);
   }
   deleteRowList(rowList);
   
   return STATUS_GOOD;

}



// *****************************************************************************
// * Method: getPrivTextForObject                                
// *                                                       
// *    returns GRANT statements describing all the privileges that have been
// *    granted on the object
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <objectInfo> Metadata details for object.
// *  <privilegeText> The resultant GRANT statement(s)
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD    : Grants were found
// * STATUS_NOTFOUND: No grants were found
// *               *: Unable to insert privileges, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::getPrivTextForObject(
   const PrivMgrObjectInfo & objectInfo,
   std::string & privilegeText)
{
  PrivStatus retcode = STATUS_GOOD;

  if (objectUID_ == 0)
  {
    PRIVMGR_INTERNAL_ERROR("objectUID is 0 for describe privileges command");
    return STATUS_ERROR;
  }

 // generate the list of privileges granted to the object and store in class
  if (generateObjectRowList() == STATUS_ERROR)
    return STATUS_ERROR;

  if (generateColumnRowList() == STATUS_ERROR)
    return STATUS_ERROR;

// No failures possible for objects, all information in rowList.  
  buildPrivText(objectRowList_,objectInfo,PrivLevel::OBJECT,pDiags_,privilegeText);

// build text for columns
  retcode = buildPrivText(columnRowList_,objectInfo,PrivLevel::COLUMN,
                          pDiags_,privilegeText);

  return retcode;
  
}


// *****************************************************************************
// * Method: getPrivsOnObjectForUser                                
// *                                                       
// *    returns privileges granted to the requested user for the requested 
// *    object
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <objectUID> identifies the object
// *  <userID> identifies the user
// *  <userPrivs> the list of privileges is returned
// *  <grantablePrivs> the list of grantable privileges is returned
// *  <colPrivsList> the list of column-level privileges is returned
// *  <colGrantableList> the list of grantable column-level privileges is returned
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: Privileges were gathered
// *           *: Unable to gather privileges, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::getPrivsOnObjectForUser(
  const int64_t objectUID,
  ComObjectType objectType,
  const int32_t userID,
  PrivObjectBitmap &userPrivs,
  PrivObjectBitmap &grantablePrivs,
  PrivColList & colPrivsList,
  PrivColList & colGrantableList,
  std::vector <ComSecurityKey *>* secKeySet)
{
  PrivStatus retcode = STATUS_GOOD;
  
  objectUID_ = objectUID;
  if (objectUID == 0)
  {
    PRIVMGR_INTERNAL_ERROR("objectUID is 0 for get privileges command");
    return STATUS_ERROR;
  }

 // generate the list of privileges granted to the object and store in class
  if (generateObjectRowList() == STATUS_ERROR)
    return STATUS_ERROR;

  objectUID_ = objectUID;
  PrivMgrDesc privsOfTheUser(userID);
  bool hasManagePrivileges = false;
  std::vector<int32_t> roleIDs;
  
  retcode = getRoleIDsForUserID(userID,roleIDs);
  if (retcode == STATUS_ERROR)
    return retcode;

  retcode = getUserPrivs(objectType, userID, roleIDs, privsOfTheUser, 
                         hasManagePrivileges, secKeySet);
  if (retcode != STATUS_GOOD)
    return retcode;
 
  if (hasManagePrivileges && hasAllDMLPrivs(objectType,privsOfTheUser.getTablePrivs().getPrivBitmap()))
  {
    userPrivs = privsOfTheUser.getTablePrivs().getPrivBitmap();
    grantablePrivs = userPrivs;
    return STATUS_GOOD; 
  }
    
 // generate the list of column-level privileges granted to the object and store in class
  if (generateColumnRowList() == STATUS_ERROR)
    return STATUS_ERROR;

  retcode = getColPrivsForUser(userID,roleIDs,colPrivsList,colGrantableList,secKeySet);
  if (retcode != STATUS_GOOD)
    return retcode;

  userPrivs = privsOfTheUser.getTablePrivs().getPrivBitmap();
  if (hasManagePrivileges)
    grantablePrivs = userPrivs;
  else
    grantablePrivs = privsOfTheUser.getTablePrivs().getWgoBitmap();
  
  return retcode;
}


// *****************************************************************************
// * Method: getRoleIDsForUserID                                
// *                                                       
// *    Returns the roleIDs for the roles granted to the user.
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <userID> is the unique identifier for the user
// *  <roleIDs> passes back the list (potentially empty) of roles granted to the user
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: Role list returned
// *           *: Unable to fetch granted roles, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::getRoleIDsForUserID(
   int32_t userID,
   std::vector<int32_t> & roleIDs)
   
{

PrivStatus retcode = STATUS_GOOD;

PrivMgrRoles roles(" ",metadataLocation_,pDiags_);
std::vector<std::string> roleNames;
std::vector<int32_t> roleDepths;
  
   retcode =  roles.fetchRolesForUser(userID,roleNames,roleIDs,roleDepths);
   return retcode;
}
//*************** End of PrivMgrPrivileges::getRoleIDsForUserID ****************

// *****************************************************************************
// * Method: getUserPrivs                                
// *                                                       
// *    Accumulates privileges for a user summarized over all grantors
// *    including PUBLIC
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <objectType> is the type of the subject object.
// *  <granteeID> specifies the userID to accumulate
// *  <roleIDs> specifies a list of roles granted to the grantee
// *  <summarizedPrivs> contains the summarized privileges
// *  <hasManagePrivileges> returns whether the grantee has MANAGE_PRIVILEGES authority
// *  <secKeySet> if not NULL, returns a set of keys for user
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: Privileges were gathered
// *           *: Unable to gather privileges, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::getUserPrivs(
  ComObjectType objectType,
  const int32_t granteeID,
  const std::vector<int32_t> & roleIDs,
  PrivMgrDesc &summarizedPrivs,
  bool & hasManagePrivileges,
  std::vector <ComSecurityKey *>* secKeySet 
  )
{
   PrivStatus retcode = STATUS_GOOD;
   PrivMgrDesc temp(granteeID);

   retcode = getPrivsFromAllGrantors( objectUID_,
                                      objectType,
                                      granteeID,
                                      roleIDs,
                                      temp,
                                      hasManagePrivileges,
                                      secKeySet
                                      );
   if (retcode != STATUS_GOOD)
    return retcode;

   summarizedPrivs = temp;

   // TBD - set all column granted if the table level privilege is set
  return retcode;
}

// *****************************************************************************
// * Method: getPrivsFromAllGrantors                                
// *                                                       
// *    Accumulates privileges for a specified userID
// *    Does the actual accumulation orchestrated by getUserPrivs
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <objectUID> object to gather privileges for
// *  <objectType> is the type of the subject object.
// *  <granteeID> specifies the userID to accumulate
// *  <roleIDs> is vector of roleIDs granted to the grantee
// *  <hasManagePrivileges> returns whether the grantee has MANAGE_PRIVILEGES authority
// *  <summarizedPrivs> contains the summarized privileges
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: Privileges were accumulated
// *           *: Unable to accumulate privileges, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::getPrivsFromAllGrantors(
   const int64_t objectUID,
   ComObjectType objectType,
   const int32_t granteeID,
   const std::vector<int32_t> & roleIDs,
   PrivMgrDesc &summarizedPrivs,
   bool & hasManagePrivileges,
   std::vector <ComSecurityKey *>* secKeySet 
   )
{
  PrivStatus retcode = STATUS_GOOD;
  hasManagePrivileges = false;
  
  // Check to see if the granteeID is the system user
  // if so, the system user has all privileges.  Set up appropriately
  if (ComUser::isSystemUserID(granteeID))
  {
    PrivObjectBitmap bitmap;
    bitmap.set();
    PrivMgrCoreDesc coreTablePrivs(bitmap, bitmap);
    summarizedPrivs.setTablePrivs(coreTablePrivs);
    hasManagePrivileges = true;
    return STATUS_GOOD;
  }
  
  PrivObjectBitmap systemPrivs;
  PrivMgrComponentPrivileges componentPrivileges(metadataLocation_,pDiags_);
  
  componentPrivileges.getSQLDMLPrivileges(granteeID,roleIDs,systemPrivs,
                                          hasManagePrivileges);

  if (hasManagePrivileges && hasAllDMLPrivs(objectType,systemPrivs))
  {
    PrivMgrCoreDesc coreTablePrivs(systemPrivs,systemPrivs);
    summarizedPrivs.setTablePrivs(coreTablePrivs);
    return STATUS_GOOD; 
  }
  
  std::vector<PrivMgrMDRow *> rowList;
  retcode = getRowsForGrantee(objectUID, granteeID, true, roleIDs, rowList, secKeySet);
  if (retcode == STATUS_ERROR)
    return retcode; 

  // Get the privileges for the object granted to the grantee
  PrivMgrCoreDesc coreTablePrivs;
  for (int32_t i = 0; i < rowList.size();++i)
  {
    ObjectPrivsMDRow &row = static_cast<ObjectPrivsMDRow &> (*rowList[i]);
    
    if (secKeySet != NULL)
    {
      PrivMgrCoreDesc privs(row.privsBitmap_,0);
      retcode = buildSecurityKeys(row.granteeID_,privs,*secKeySet);
      if (retcode != STATUS_GOOD)
        return retcode;    
    }

    PrivMgrCoreDesc temp (row.privsBitmap_, row.grantableBitmap_);
    coreTablePrivs.unionOfPrivs(temp);
  }
  
  PrivObjectBitmap grantableBitmap;
  
  if (hasManagePrivileges)
     grantableBitmap = systemPrivs;
  
  PrivMgrCoreDesc temp2(systemPrivs,grantableBitmap);
  coreTablePrivs.unionOfPrivs(temp2);
  
  summarizedPrivs.setTablePrivs(coreTablePrivs);

  return STATUS_GOOD;
}


// *****************************************************************************
// * Method: getRowsForGrantee                                
// *                                                       
// *    Reads OBJECT_PRIVILEGES table to obtain all  privileges granted to the
// *    specified granteeID for the object (objectUID)
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <objectUID> object to gather privileges for
// *  <granteeID> specifies the userID to gather privileges
// *  <roleIDs> the list of roles granted to the userID
// *  <isObjectTable> true if OBJECT_PRIVILEGES table
// *  <rowList> returns the list of granted privileges as a vector list
// *    consisiting of the grantor, grantee, and privileges for the object
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: Privileges were retrieved
// *           *: Unable to retrieve privileges, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::getRowsForGrantee(
  const int64_t objectUID,
  const int32_t granteeID,
  const bool isObjectTable,
  const std::vector<int_32> &roleIDs,
  std::vector<PrivMgrMDRow *> &rowList,
  std::vector <ComSecurityKey *>* secKeySet) 
{
  PrivStatus retcode = STATUS_GOOD;

#if 0
  if (isObjectTable)
  {
    if (objectRowList_.size() == 0)
    {
      PRIVMGR_INTERNAL_ERROR("privilege list for object has not been created");
      return STATUS_ERROR;
    }
  }

  else // isColumnTable
  {
    if (columnRowList_.size() == 0)
    {
      PRIVMGR_INTERNAL_ERROR("privilege list for columns have not been created");
      return STATUS_ERROR;
    }
  }
#endif

  // create the list of row pointers from the cached list
  std::vector<int32_t> authIDs = roleIDs;
  authIDs.push_back(granteeID);
  authIDs.push_back(PUBLIC_USER);
  std::vector<int32_t>::iterator it;
  std::vector<PrivMgrMDRow *> privRowList;
  if (isObjectTable)
    privRowList = objectRowList_;
  else
    privRowList = columnRowList_;

  for (size_t i = 0; i < privRowList.size(); i++)
  {
     if (isObjectTable)
     {
       ObjectPrivsMDRow &row = static_cast<ObjectPrivsMDRow &> (*privRowList[i]);
       it = std::find(authIDs.begin(), authIDs.end(), row.granteeID_);
     }
     else
     {
       ColumnPrivsMDRow &row = static_cast<ColumnPrivsMDRow &> (*privRowList[i]);
       it = std::find(authIDs.begin(), authIDs.end(), row.granteeID_);
     }
     if (it != authIDs.end())
       rowList.push_back(privRowList[i]);
  }
  
  if (secKeySet != NULL)
  {
    retcode = buildUserSecurityKeys(roleIDs,granteeID,*secKeySet);   
    if (retcode == STATUS_ERROR)
    {
      PRIVMGR_INTERNAL_ERROR("Unable to build user security key");
      return STATUS_ERROR;   
    }
  }

  return STATUS_GOOD;
}

// *****************************************************************************
// * Method: summarizeCurrentAndOriginalPrivs                                
// *                                                       
// *    Accumulates privileges for a specified object and grantee
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <objectUID> object to summarize privileges for
// *  <granteeID> specifies the userID to accumulate
// *  <roleIDs> the list of roles granted to the userID
// *  <summarizedOriginalPrivs> contains the original summarized privileges
// *  <summarizedCurrentPrivs> contains the current summarized privileges
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: Privileges were summarized
// *           *: Unable to summarize privileges, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::summarizeCurrentAndOriginalPrivs(
   const int64_t objectUID,
   const int32_t granteeID,
   const std::vector<int32_t> & roleIDs,
   const std::vector<ObjectUsage *> listOfChangedPrivs,
   PrivMgrDesc &summarizedOriginalPrivs,
   PrivMgrDesc &summarizedCurrentPrivs)
{
  PrivStatus retcode = STATUS_GOOD;

  // get OBJECT_PRIVILEGES rows where the grantee has received privileges
  std::vector<PrivMgrMDRow *> rowList;
  retcode = getRowsForGrantee(objectUID, granteeID, true, roleIDs, rowList, NULL);

  // rowList contains the original privileges, 
  // listOfChangedPrivs contains any updates to privileges
  // go through the list and summarize the original and current privileges
  // We do a union operation to capture privileges from all grantors
  for (int32_t i = 0; i < rowList.size();++i)
  {
    ObjectPrivsMDRow &row = static_cast<ObjectPrivsMDRow &> (*rowList[i]);
    PrivMgrCoreDesc original(row.privsBitmap_, row.grantableBitmap_);
    PrivMgrCoreDesc current = original;
    for (size_t j = 0; j < listOfChangedPrivs.size(); j++)
    {
      ObjectUsage *pObj = listOfChangedPrivs[j];
      if (pObj->objectUID == row.objectUID_ &&
          grantorID_ == row.grantorID_ &&
          pObj->objectOwner == row.granteeID_ )
      {
        current = pObj->updatedPrivs.getTablePrivs();
      }
    }
    summarizedOriginalPrivs.unionOfPrivs(original);
    summarizedCurrentPrivs.unionOfPrivs(current);
  }

  return STATUS_GOOD;
}


// *****************************************************************************
// * Method: isAuthIDGrantedPrivs                                
// *                                                       
// *    Determines if the specified authorization ID has been granted one or   
// * more object privileges.                                                
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <authID> identifies the user or role.
// *                                                                     
// * Returns: bool                                               
// *                                                                  
// *  true: Authorization ID has been granted one or more object privileges.
// * false: Authorization ID has not been granted any object privileges.     
// *                                                               
// *****************************************************************************
bool PrivMgrPrivileges::isAuthIDGrantedPrivs(const int32_t authID)

{

   std::string whereClause(" WHERE GRANTEE_ID = ");   

   char authIDString[20];

   sprintf(authIDString,"%d",authID);

   whereClause += authIDString; 

   // set pointer in diags area
   int32_t diagsMark = pDiags_->mark();

   int64_t rowCount = 0;   
   ObjectPrivsMDTable myTable(objectTableName_,pDiags_);

   PrivStatus privStatus = myTable.selectCountWhere(whereClause,rowCount);

   if ((privStatus == STATUS_GOOD || privStatus == STATUS_WARNING) &&
        rowCount > 0)
      return true;
      
   pDiags_->rewind(diagsMark);
   return false;

}

// *****************************************************************************
// * Method: convertPrivsToDesc                                
// *                                                       
// *    Converts the list of requested privileges into a PrivMgrDesc
// *    This code also checks for duplicate entries in the privilege list
// *    and that the list of privileges is compatible with the object type.
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <objectType> type of object
// *  <isAllSpecified> if true then all privileges valid for the object
// *                        type will be revoked
// *  <isWGOSpecified> if true then remove the ability for  the grantee 
// *                   to revoke the set of privileges to other grantees
// *  <privsList> is the list of privileges to check
// *  <PrivMgrCoreDesc>  the core descriptor containing privileges
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: Privileges were inserted
// *           *: Unable to insert privileges, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::convertPrivsToDesc( 
  const ComObjectType objectType,
  const bool isAllSpecified,
  const bool isWgoSpecified,
  const bool isGOFSpecified,
  const std::vector<PrivType> privsList,
  PrivMgrDesc &privsToProcess)
{

  // Categorize the objectType
  bool isLibrary = false;
  bool isUdr = false;
  bool isObject = false;
  bool isSequence = false;
  switch (objectType)
  {
     case COM_BASE_TABLE_OBJECT:
     case COM_VIEW_OBJECT:
       isObject = true;
       break;
     case COM_LIBRARY_OBJECT:
       isLibrary = true;
       break;
     case COM_USER_DEFINED_ROUTINE_OBJECT:
       isUdr = true;
       break;
     case COM_SEQUENCE_GENERATOR_OBJECT:
       isSequence = true;
       break;
     default:
     {
       char objectTypeLit[3] = {0};
       strncpy(objectTypeLit,ObjectEnumToLit(objectType),2);
       *pDiags_ << DgSqlCode(-4219)
                << DgString1(objectTypeLit);
       return STATUS_ERROR;
     }
  }

  // If all is specified, set bits appropriate for the object type and return
  if (isAllSpecified)
  {
    if (isLibrary)
      privsToProcess.setAllLibraryGrantPrivileges(isWgoSpecified);
    else if (isUdr)
      privsToProcess.setAllUdrGrantPrivileges(isWgoSpecified);
    else if (isSequence)
      privsToProcess.setAllSequenceGrantPrivileges(isWgoSpecified);
    else
      privsToProcess.setAllTableGrantPrivileges(isWgoSpecified);
    return STATUS_GOOD;
  }

  PrivMgrCoreDesc tableCorePrivs;

  // For each privilege specified in the privsList:
  //    make sure it is not a duplicate
  //    make sure it is appropriate for the objectType
  bool isIncompatible = false;
  for (int32_t i = 0; i < privsList.size();++i)
  {
    switch (privsList[i])
    {
      case EXECUTE_PRIV:
        if (!isUdr)
          isIncompatible = true;
        else
          tableCorePrivs.testAndSetBit(privsList[i],isWgoSpecified,isGOFSpecified);  
        break;
      case DELETE_PRIV:
      case INSERT_PRIV:
      case REFERENCES_PRIV:
      case SELECT_PRIV:
        if (!isObject)
          isIncompatible = true;
        else
          tableCorePrivs.testAndSetBit(privsList[i],isWgoSpecified,isGOFSpecified);  
        break;
      case UPDATE_PRIV:
        if (!isObject && !isLibrary)
          isIncompatible = true;
        else
          tableCorePrivs.testAndSetBit(privsList[i],isWgoSpecified,isGOFSpecified);  
        break;
      case USAGE_PRIV:
        if (!isLibrary && !isSequence)
          isIncompatible = true;
        else
          tableCorePrivs.testAndSetBit(privsList[i],isWgoSpecified,isGOFSpecified);  
        break;
      case ALL_DML:
        if (!isObject)
          isIncompatible = true;
        else
          if (isGOFSpecified)
            tableCorePrivs.setWgo(ALL_DML,true);
          else
          {
            tableCorePrivs.setPriv(ALL_DML,true);
            tableCorePrivs.setWgo(ALL_DML,isWgoSpecified); 
          }
        break;
      default:
      {
        *pDiags_ << DgSqlCode(-CAT_INVALID_PRIVILEGE_FOR_GRANT_OR_REVOKE)
                 << DgString0(PrivMgrUserPrivs::convertPrivTypeToLiteral(privsList[i]).c_str());
        return STATUS_ERROR;
      }
    }
    // Report error if privilege is incompatible with objectType
    if (isIncompatible)
    {
      *pDiags_ << DgSqlCode(-CAT_PRIVILEGE_NOT_ALLOWED_FOR_THIS_OBJECT_TYPE)
                << DgString0(PrivMgrUserPrivs::convertPrivTypeToLiteral(privsList[i]).c_str());
      return STATUS_ERROR;
    }
  } // end for

  privsToProcess.setTablePrivs(tableCorePrivs);
  return STATUS_GOOD;      
}


// *****************************************************************************
//    PrivMgrPrivileges.cpp static functions                                   *
// *****************************************************************************



// *****************************************************************************
// * Function: buildPrivText                                                   *
// *                                                                           *
// *    Builds priv portion of SHOWDDL output.                                 *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <rowList>                    const std::vector<PrivMgrMDRow *>  In       *
// *    is a list of rows describing the privileges granted.                   *
// *                                                                           *
// *  <objectInfo>                 const PrivMgrObjectInf &           In       *
// *    object details needed to create appropriate text                       *
// *                                                                           *
// *  <privLevel>                  PrivLevel                          In       *
// *    is the privilege level, either OBJECT or COLUMN.                       *
// *                                                                           *
// *  <pDiags_>                    ComDiagsArea *                     In       *
// *    is where to report an internal error.                                  *
// *                                                                           *
// *  <privilegeText>              std::string &                      Out      *
// *    passes back the set of grant commands that describe the privileges on  *
// *  the object.                                                              *
// *                                                                           *
// *****************************************************************************
static PrivStatus buildPrivText(
   const std::vector<PrivMgrMDRow *> rowList,
   const PrivMgrObjectInfo         & objectInfoIn,
   PrivLevel                         privLevel,
   ComDiagsArea                    * pDiags_,
   std::string                     & privilegeText)
{
  PrivMgrObjectInfo objectInfo = (PrivMgrObjectInfo) objectInfoIn;

  // Build a grant statement for each grantor/grantee row returned.
  // TDB: If we support multiple grantees per grant statement, 
  //      this code can be improved
  std::string grantStmt;
  std::string grantWGOStmt;
  std::string objectText("ON ");
  
  // Append object type if not base table or view
  if (objectInfo.getObjectType() != COM_BASE_TABLE_OBJECT &&
      objectInfo.getObjectType() != COM_VIEW_OBJECT)
    objectText += comObjectTypeName(objectInfo.getObjectType());
  objectText += objectInfo.getObjectName() + " TO ";
  
  std::string lastGranteeName;
  int32_t lastGranteeID = 0;
  
  std::vector<std::string> privString;
  std::vector<std::string> WGOString;
  std::vector<bool> hasWGO;
  std::vector<bool> hasPriv;
  std::vector<std::string> columnNames;
  bool mergeStrings = false;

  // Note, this creates entries for DELETE and USAGE that are unused.  
   if (privLevel == PrivLevel::COLUMN)
      for (size_t p = FIRST_DML_COL_PRIV; p <= LAST_DML_COL_PRIV; p++ )
      {
         privString.push_back(PrivMgrUserPrivs::convertPrivTypeToLiteral((PrivType)p) + "(");  
         WGOString.push_back(privString[p]);  
         hasPriv.push_back(false);
         hasWGO.push_back(false);
      }
  
   for (int32_t i = 0; i < rowList.size();++i)
   {
      std::string objectGranteeText(objectText);

      std::string withoutWGO;
      std::string withWGO;
      int32_t grantorID = 0;
      std::string grantorName;
      if (privLevel == PrivLevel::OBJECT)
      {
         ObjectPrivsMDRow &row = static_cast<ObjectPrivsMDRow &> (*rowList[i]);
         
         grantorID = row.grantorID_;
         grantorName = row.grantorName_;
         PrivObjectBitmap privsBitmap = row.privsBitmap_;
         PrivObjectBitmap wgoBitmap = row.grantableBitmap_;
         bool delimited = isDelimited(row.granteeName_);
         if (delimited)
           objectGranteeText += "\"";
         objectGranteeText += row.granteeName_;
         if (delimited)
           objectGranteeText += "\"";
         for (size_t p = FIRST_DML_PRIV; p <= LAST_DML_PRIV; p++ )
            if (privsBitmap.test(p))
            {
               std::string privTypeString = 
                 PrivMgrUserPrivs::convertPrivTypeToLiteral((PrivType)p);
               if (wgoBitmap.test(p))
                  withWGO += privTypeString + ", ";
               else
                  withoutWGO += privTypeString + ", ";
            }
      }
      else
      {
         ColumnPrivsMDRow &row = static_cast<ColumnPrivsMDRow &> (*rowList[i]);
         // For column-level privileges we are building a piece of the 
         // output for each privilege on every loop.  Privileges are stored
         // per column, but GRANT syntax accepts via a privilege and a 
         // list of columns.  For each privilege granted to a grantee, need
         // to list all the columns.  Substrings are merged when the end of the
         // list of grants is reached or there is a new grantor or grantee.
         if (i + 1 == rowList.size())
            mergeStrings = true;
         else
         {
            ColumnPrivsMDRow &nextRow = static_cast<ColumnPrivsMDRow &> (*rowList[i + 1]);
            
            if (nextRow.grantorID_ != row.grantorID_ ||
                nextRow.granteeID_ != row.granteeID_)
               mergeStrings = true;
         }
         
         grantorID = row.grantorID_;
         grantorName = row.grantorName_;
         PrivColumnBitmap privsBitmap = row.privsBitmap_;
         PrivColumnBitmap wgoBitmap = row.grantableBitmap_; 
         
         // Get name of the grantee.  If we have changed grantees, fetch the
         // name of the grantee.
         if (row.granteeID_ != lastGranteeID)
         {
            lastGranteeID = row.granteeID_;
            lastGranteeName = row.granteeName_;
         }   
         bool delimited = isDelimited(lastGranteeName);
         if (delimited)
           objectGranteeText += "\"";
         objectGranteeText += lastGranteeName;
         if (delimited)
           objectGranteeText += "\"";
         
         // Get the column name for the row
         const std::vector<std::string> &columnList = objectInfo.getColumnList();
         if (columnList.size() < row.columnOrdinal_)
         {
            std::string errorText("Unable to look up column name for column number ");
            
            errorText += PrivMgr::authIDToString(row.columnOrdinal_);
            PRIVMGR_INTERNAL_ERROR(errorText.c_str());
            return STATUS_ERROR;
         }
         std::string columnName(columnList.at(row.columnOrdinal_));
        
         // Build the list of columns granted for each privilege.  WGOString 
         // and privString have been pre-populated with PRIVNAME(.
         for (size_t p = FIRST_DML_COL_PRIV; p <= LAST_DML_COL_PRIV; p++ )
            if (privsBitmap.test(p))
            {
               if (wgoBitmap.test(p))
               {
                  WGOString[p] += columnName + ", ";
                  hasWGO[p] = true;
               }
               else
               {
                  privString[p] += columnName + ", ";
                  hasPriv[p] = true;
               }
            }
      
         // Check if there are column priv substrings that need to be merged.
         if (mergeStrings)
         {
            for (size_t p = FIRST_DML_COL_PRIV; p <= LAST_DML_COL_PRIV; p++ )
            {
               if (!isDMLPrivType(static_cast<PrivType>(p)))
                  continue;
                 
               if (hasWGO[p])
               {
                  closeColumnList(WGOString[p]);
                  withWGO += WGOString[p];
                  // Reset to original value
                  WGOString[p].assign(PrivMgrUserPrivs::convertPrivTypeToLiteral((PrivType)p) + "(");
                  hasWGO[p] = false;
               }
               else
                  if (hasPriv[p])
                  {
                     closeColumnList(privString[p]);
                     withoutWGO += privString[p];
                     // Reset to original value
                     privString[p].assign(PrivMgrUserPrivs::convertPrivTypeToLiteral((PrivType)p) + "(");
                     hasPriv[p] = false;
                  }
            }
            mergeStrings = false;
         }
      }//End of PrivLevel::COLUMN
          
      if (!withoutWGO.empty())
         buildGrantText(withoutWGO,objectGranteeText,
                        grantorID,grantorName,false,
                        objectInfo.getObjectOwner(),grantStmt);

      if (!withWGO.empty())
         buildGrantText(withWGO,objectGranteeText,
                        grantorID,grantorName,true,
                        objectInfo.getObjectOwner(),grantWGOStmt);
      privilegeText += grantStmt + grantWGOStmt;
      grantStmt.clear();
      grantWGOStmt.clear();
   }
  
  return STATUS_GOOD;

}
//*************************** End of buildPrivText *****************************


// *****************************************************************************
// * Function: buildGrantText                                                  *
// *                                                                           *
// *    Builds GRANT statement.                                                *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <privText>                   const std::string &                In       *
// *    is the privileges granted.                                             *
// *                                                                           *
// *  <objectGranteeText>          const std::string &                In       *
// *    is the object the privileges are granted on and to whom.               *
// *                                                                           *
// *  <grantorID>                  const int32_t                      In       *
// *    is the ID of the authID who granted the privilege(s).  If the system   *
// * (_SYSTEM) granted the privilege, the command is prefixed with "--" to     *
// * prevent execution in a playback script.                                   *
// *                                                                           *
// *  <isWGO>                      bool                               In       *
// *    if true, add the clause WITH GRANT OPTION to the command.              *
// *                                                                           *
// *  <grantText>                  std::string &                      Out      *
// *    passes back the grant command.                                         *
// *                                                                           *
// *****************************************************************************
void static buildGrantText(
   const std::string & privText,
   const std::string & objectGranteeText, 
   const int32_t grantorID,
   const std::string grantorName,
   bool isWGO,
   const int32_t objectOwner,
   std::string & grantText)
   
{

   if (grantorID == SYSTEM_USER)
      grantText += "-- ";

   grantText += "GRANT ";
   grantText += privText;
   
  // remove last ','
  size_t commaPos = grantText.find_last_of(",");

   if (commaPos != std::string::npos)
      grantText.replace(commaPos, 1, "");
      
   grantText += objectGranteeText;
  
   if (isWGO)
      grantText += " WITH GRANT OPTION";
   else

   if (grantorID != objectOwner &&
       grantorID != SYSTEM_USER)
    {
      grantText += " GRANTED BY ";
      bool delimited = isDelimited(grantorName);
      if (delimited)
        grantText += "\"";
      grantText += grantorName;
      if (delimited)
        grantText += "\"";
    }

    grantText += ";\n";
    
}
//*************************** End of buildGrantText ****************************

// *****************************************************************************
// * Function: buildColumnSecurityKeys                                         *
// *                                                                           *
// *    Builds security keys for privileges granted on one or more columns of  *
// * an object.                                                                *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *                                                                           *
// *  <objectUID>                  const int64_t                      In       *
// *    is the unique ID of the object.                                        *
// *                                                                           *
// *  <roleIDs>                    const PrivColList & colPrivsList   In       *
// *    is a list of the column privileges granted on this object to the       *
// * specified grantee.                                                        *
// *                                                                           *
// *  <granteeID>                  const int32_t                      In       *
// *    is the ID of the user granted the column privilege(s).                 *
// *                                                                           *
// *  <secKeySet>                  std::vector <ComSecurityKey *> &   Out      *
// *    passes back a list of SUBJECT_IS_OBJECT security keys for each of the  *
// *  privileges granted on the object to the grantee.                         *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Security keys were built                                     *
// *           *: Security keys were not built, see diags.                     *
// *                                                                           *
// *****************************************************************************
static PrivStatus buildColumnSecurityKeys(
   const int64_t objectUID,
   const PrivColList & colPrivsList,
   const int32_t granteeID, 
   std::vector<ComSecurityKey *> & secKeySet)
  
{

// *****************************************************************************
// *                                                                           *
// *   Optimizer currently does not support OBJECT_IS_COLUMN, so we combine    *
// * all column-level privileges to one priv bitmap and create a key for       *
// * each priv type the grantee has on the object.                             *
// *                                                                           *
// *****************************************************************************

PrivColumnBitmap privBitmap;

   for (PrivColIterator columnIterator = colPrivsList.begin();
        columnIterator != colPrivsList.end(); ++columnIterator)
      privBitmap |= columnIterator->second;
      
   for (size_t i = FIRST_DML_COL_PRIV; i <= LAST_DML_COL_PRIV; i++ )
   {
      if (!privBitmap.test(PrivType(i)))
         continue;
   
      ComSecurityKey *key = new ComSecurityKey(granteeID, 
                                               objectUID,
                                               PrivType(i),
                                               ComSecurityKey::OBJECT_IS_OBJECT);
      if (!key->isValid())
         return STATUS_ERROR;
         
      secKeySet.push_back(key);
   }
   
   return STATUS_GOOD;
   
}
//********************** End of buildColumnSecurityKeys ************************

// *****************************************************************************
// * Function: buildUserSecurityKeys                                           *
// *                                                                           *
// *    Builds security keys for a user and the roles granted to the user.     *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *                                                                           *
// *  <roleIDs>                    const std::vector<int32_t> &       In       *
// *    is a reference to a vector of roleIDs that have been granted to the    *
// *  user.                                                                    *
// *                                                                           *
// *                                                                           *
// *  <userID>                     const int32_t                      In       *
// *    is the ID of the user granted the role(s).                             *
// *                                                                           *
// *  <secKeySet>                  std::vector <ComSecurityKey *> &   Out      *
// *    passes back a list of SUBJECT_IS_USER security keys for each of the    *
// *  roles granted to the user.                                               *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Security keys were built                                     *
// *           *: Security keys were not built, see diags.                     *
// *                                                                           *
// *****************************************************************************
static PrivStatus buildUserSecurityKeys(
   const std::vector<int32_t> & roleIDs,
   const int32_t userID, 
   std::vector<ComSecurityKey *> & secKeySet)
  
{

   for ( size_t i = 0; i < roleIDs.size(); i++ )
   {
      ComSecurityKey *key = new ComSecurityKey(userID,roleIDs[i],
                                               ComSecurityKey::SUBJECT_IS_USER);
      if (key->isValid())
         secKeySet.push_back(key);
      else
         return STATUS_ERROR;
   }
   
   return STATUS_GOOD;
}
//*********************** End of buildUserSecurityKeys *************************

// *****************************************************************************
// * Function: closeColumnList                                                 *
// *                                                                           *
// *    This function closes a list of the form "(column, column, column, ...".*
// *  The last comma is removed and a closing parenthesis is added.            *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <columnList>             const std::string &                In/Out       *
// *    is the list of columns in string.                                      *
// *                                                                           *
// *****************************************************************************
void static closeColumnList(std::string & columnList)

{

size_t commaPos = columnList.find_last_of(",");

// If there is no comma in the string, input not recognized, return unchanged.
   if (commaPos == std::string::npos)
      return;
      
// Replace the trailing comma and space with a parenthesis and trailing comma.
// Add an additional trailing space for readability.    
   columnList.replace(commaPos,2,"),");
   columnList += " ";

}
//************************** End of closeColumnList ****************************

// *****************************************************************************
// *                                                                           *
// * Function: deleteRowList                                                  *
// *                                                                           *
// *    Deletes elements from a vector of PrivMgrMDRows.                       *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <rowList>                std::vector<PrivMgrMDRow *> &      In/Out       *
// *    is the list of rows to delete.                                         *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
static void deleteRowList(std::vector<PrivMgrMDRow *> & rowList)

{

   while(!rowList.empty())
      delete rowList.back(), rowList.pop_back();
      
}
//************************** End of deleteRowList *****************************



// *****************************************************************************
// * Function: findColumnEntry                                                 *
// *                                                                           *
// *    This function searches a vector of ColPrivEntry for a matching         *
// *  column ordinal.                                                          *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *                                                                           *
// *  <colPrivEntries>             std::vector<ColPrivEntry> &        In       *
// *    is a reference to a vector of ColPrivEntry.                            *
// *                                                                           *
// *  <columnOrdinal>              const int32_t                      In       *
// *    is the column ordinal to search for.                                   *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: ColPrivEntry                                                     *
// *                                                                           *
// * NULL: No entry found with that column ordinal                             *
// *    *: Entry found with matching column ordinal                            *
// *                                                                           *
// *****************************************************************************
static ColPrivEntry * findColumnEntry(
   std::vector<ColPrivEntry> & colPrivEntries,
   const int32_t columnOrdinal)
   
{

   for (size_t i = 0; i < colPrivEntries.size(); i++)
      if (colPrivEntries[i].columnOrdinal == columnOrdinal)
         return & colPrivEntries[i];
         
   return NULL;

}   
//************************** End of findColumnEntry ****************************



// *****************************************************************************
// * Function: getColRowsForGrantee                                            *
// *                                                                           *
// *    Returns the list of column privileges granted for the object that have *
// *    been granted to the granteeID.                                         *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <columnRowList>              std::vector<PrivMgrMDRow *> &      In       *
// *    is the list of column privileges granted on the object.                *
// *                                                                           *
// *  <granteeID>                  const int32_t                      In       *
// *    is the authID granted the privileges.                                  *
// *                                                                           *
// *  <roleIDs>                    const std::vector<int32_t> &       In       *
// *    is a list of roles granted to the grantee.                             *
// *                                                                           *
// *  <rowList>                    std::vector<PrivMgrMDRow *> &      Out      *
// *    passes back a list rows representing the privileges granted.           *
// *                                                                           *
// *  <secKeySet>                  std::vector <ComSecurityKey *> &   Out      *
// *    passes back a list of SUBJECT_IS_USER security keys for each of the    *
// *  roles granted to the grantee.                                            *
// *                                                                           *
// *****************************************************************************
// * Returns: PrivStatus                                                       * 
// *                                                                           *  
// * STATUS_GOOD: Row returned.                                                *
// * STATUS_NOTFOUND: No matching rows were found                              *
// *****************************************************************************
static PrivStatus getColRowsForGrantee(
   const std::vector <PrivMgrMDRow *> &columnRowList,
   const int32_t granteeID,
   const std::vector<int32_t> & roleIDs,
   std::vector<ColumnPrivsMDRow> & rowList,
   std::vector <ComSecurityKey *>* secKeySet)
    
{

  std::vector<int32_t> authIDs = roleIDs;
  authIDs.push_back(granteeID);
  authIDs.push_back(PUBLIC_USER);
  std::vector<int32_t>::iterator it;

  std::vector<PrivMgrMDRow *> privRowList;

   // returns the list of rows for the grantee, roles that the grantee has been
   // granted, and PUBLIC
   for (size_t i = 0; i < columnRowList.size(); i++)
   {
      ColumnPrivsMDRow &row = static_cast<ColumnPrivsMDRow &> (*columnRowList[i]);
      it = std::find(authIDs.begin(), authIDs.end(), row.granteeID_);
      if (it != authIDs.end())
         rowList.push_back(row);
   }

   if (rowList.empty())
     return STATUS_NOTFOUND;

   if (secKeySet != NULL)
      return buildUserSecurityKeys(roleIDs,granteeID,*secKeySet);   

   return STATUS_GOOD;
   
}
//*********************** End of getColRowsForGrantee **************************


// *****************************************************************************
// * Function: getColRowsForGranteeGrantor                                     *
// *                                                                           *
// *    Returns the list of column privileges granted for the object that have *
// *    been granted by the grantorID to the granteeID.                        *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <columnRowList>              std::vector <PrivMgrMDRow *> &       In       *
// *    is the list of column privileges granted to the object.                *
// *                                                                           *
// *  <granteeID>                  const int32_t                      In       *
// *    is the authID granted the privileges.                                  *
// *                                                                           *
// *  <grantorID>                  const int32_t                      In       *
// *    is the authID who granted the privileges.                              *
// *                                                                           *
// *  <grantedColPrivs>            std::vector<ColPrivEntry> &        Out      *
// *    passes back a privileges granted to <granteeID> by <grantorID>.        *
// *                                                                           *
// *****************************************************************************
static void getColRowsForGranteeGrantor(
   const std::vector <PrivMgrMDRow *> &columnRowList,
   const int32_t granteeID,
   const int32_t grantorID,
   std::vector<ColPrivEntry> & grantedColPrivs)
   
{

   for (size_t i = 0; i < columnRowList.size(); ++i)
   {
      ColumnPrivsMDRow &row = static_cast<ColumnPrivsMDRow &> (*columnRowList[i]);
      ColPrivEntry colPrivGrant;
      
      if (row.grantorID_ == grantorID && row.granteeID_ == granteeID)
      {
         colPrivGrant.columnOrdinal = row.columnOrdinal_;
         colPrivGrant.privsBitmap = row.privsBitmap_.to_ulong();
         colPrivGrant.grantableBitmap = row.grantableBitmap_.to_ulong();
      
         grantedColPrivs.push_back(colPrivGrant);
      }
   }
   
}
//******************* End of getColRowsForGranteeGrantor ***********************


// *****************************************************************************
// * Function: hasAllDMLPrivs                                                  *
// *                                                                           *
// *    This function determines if a privilege bitmap has all the DML         *
// * privileges for a specified object type.                                   *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <objectType>                 ComObjectType                      In       *
// *    is the type of the object.                                             *
// *                                                                           *
// *  <privBitmap>                 PrivObjectBitmap                   In       *
// *    is the bitmap representing the privileges.                             *
// *                                                                           *
// *****************************************************************************
static bool hasAllDMLPrivs(
   ComObjectType objectType,
   PrivObjectBitmap privBitmap)

{

   switch (objectType)
   {
      case COM_BASE_TABLE_OBJECT:
      case COM_VIEW_OBJECT:
         if (privBitmap.test(DELETE_PRIV) && privBitmap.test(INSERT_PRIV) &&
             privBitmap.test(REFERENCES_PRIV) && privBitmap.test(SELECT_PRIV) &&
             privBitmap.test(UPDATE_PRIV))
            return true;
         break;
      case COM_LIBRARY_OBJECT:
         if (privBitmap.test(UPDATE_PRIV) && privBitmap.test(USAGE_PRIV))
            return true;
         break;      
      case COM_STORED_PROCEDURE_OBJECT:
      case COM_USER_DEFINED_ROUTINE_OBJECT:
         if (privBitmap.test(EXECUTE_PRIV))
            return true;
         break;
      case COM_SEQUENCE_GENERATOR_OBJECT:
         if (privBitmap.test(USAGE_PRIV))
            return true;
         break;      
      default:
         return false;           
   }
   
   return false;

}
//************************** End of hasAllDMLPrivs *****************************


// *****************************************************************************
// * Function: hasColumnWGO                                                    *
// *                                                                           *
// *    This function determines if the grantor has the authority to grant     *
// * the specified privileges.                                                 *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <colPrivsArrayIn>            const std::vector<ColPrivSpec> &   In       *
// *    is the list of privileges the grantor wants to grant.                  *
// *                                                                           *
// *  <roleIDs>                    std::vector<int_32> &              In       *
// *    is the list of role IDs granted to the grantor.                        *
// *                                                                           *
// *  <privStatus>                 PrivStatus &                       In       *
// *    passes back the PrivStatus.                                            *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// *  true: Grantor has WGO.                                                   *
// * false: Grantor does NOT have WGO.  See privStatus.                        *
// *                                                                           *
// *****************************************************************************
bool PrivMgrPrivileges::hasColumnWGO(
   const std::vector<ColPrivSpec> & colPrivsArrayIn,
   const std::vector<int32_t> &roleIDs,
   PrivStatus & privStatus)
{

std::vector<ColPrivSpec> &colPrivsArray = 
   const_cast<std::vector<ColPrivSpec> &>(colPrivsArrayIn); 

   privStatus = STATUS_GOOD;

// Grantor may have column WGO from two sources, object-level grants on the  
// object and column-level grants.  First check the object-level grants. 
  
std::vector<PrivMgrMDRow *> objRowList;

  // Get object privileges that the grantor has been granted - that is, the 
  // grantor becomes the grantee.  
  privStatus = getRowsForGrantee(objectUID_,grantorID_,true,roleIDs,objRowList,NULL);
  if (privStatus == STATUS_ERROR)
    return privStatus;
      
// For each privilege to grant, see if the grantor has been granted that 
// privilege WITH GRANT OPTION (WGO).  If so, note it in the colPrivsArray entry.
// If the grantor does not have WGO, note that we have to check column 
// privileges for at least one grant.
bool checkColumnPrivs = false;
 
   for (size_t i = 0; i < colPrivsArray.size(); i++)
   {
      ColPrivSpec &colPrivEntry = colPrivsArray[i];
      colPrivEntry.grantorHasWGO = false;
      for (size_t j = 0; j < objRowList.size(); j++)
      {
         ObjectPrivsMDRow &objectRow = static_cast<ObjectPrivsMDRow &> (*objRowList[j]);
         
         if (objectRow.grantableBitmap_.test(colPrivEntry.privType))
         {
            colPrivEntry.grantorHasWGO = true;
            break;
         }
      }
      if (!colPrivEntry.grantorHasWGO)
         checkColumnPrivs = true;
   }
   
// If object-level privileges are sufficient to grant the column-level 
// privileges, no need to read COLUMN_PRIVILEGES table.
   if (!checkColumnPrivs)
      return true;


// The grantor did not have WGO at the object level for at least one
// of the privileges to be granted; see if they have the column privilege WGO.  

// Fetch any relevant WGO rows from COLUMN_PRIVILEGES.
std::vector<PrivMgrMDRow *> colRowList;
    
  // Get object privileges that the grantor has been granted - that is, the 
  // grantor becomes the grantee.  
  privStatus = getRowsForGrantee(objectUID_,grantorID_,false,roleIDs,colRowList,NULL);
  if (privStatus == STATUS_ERROR)
    return privStatus;

   for (size_t i = 0; i < colPrivsArray.size(); i++)
   {
      // If the grantor already has the authority to grant this privilege 
      // from another source, move to the next privilege to be granted.
      if (colPrivsArray[i].grantorHasWGO)
         continue;
         
      ColPrivSpec &colPrivEntry = colPrivsArray[i];
      
      // See if the grantor has been granted WGO at column-level for priv.  
      for (size_t j = 0; j < colRowList.size(); j++)
      {
         ColumnPrivsMDRow &columnRow = static_cast<ColumnPrivsMDRow &> (*colRowList[i]);
         if (columnRow.grantableBitmap_.test(colPrivEntry.privType))
         {
            colPrivEntry.grantorHasWGO = true;
            break;
         }
      }
      // If the grantor does not have an object-level or column-level WGO
      // for one of the privs to grant, return an error.
      if (!colPrivEntry.grantorHasWGO)
      {
         privStatus = STATUS_NOTFOUND;
         return false;
      }
   }
  
   return true;
  
}
//*************************** End of hasColumnWGO ******************************



   
// *****************************************************************************
// *                                                                           *
// * Function: hasGrantedColumnPriv                                            *
// *                                                                           *
// *   This function determines if a grantor has granted the specified         *
// * set of privileges on the specified object to the specified grantee.       *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <colRowList>                 std::vector<PrivMgrMDRow *> &      In       *
// *    is the list of all column privileges granted to the object.            *
// *                                                                           *
// *  <grantorID>                  const int32_t                      In       *
// *    is the authorization ID of the grantor.                                *
// *                                                                           *
// *  <granteeID>                  const int32_t                      In       *
// *    is the authorization ID of the grantee.                                *
// *                                                                           *
// *  <colPrivsArray>              const std::vector<ColPrivSpec> &   In       *
// *    is an array of column privilege specifications, with one entry per     *
// *  privilege and column.                                                    *
// *                                                                           *
// *  <privStatus>                 PrivStatus                         Out      *
// *    passes back the PrivStatus for last operation.                         *
// *                                                                           *
// *  <privilege>                  std::string &                      Out      *
// *    passes back the first privilege not granted.                           *
// *                                                                           *
// *  <grantedColPrivs>            std::vector<ColPrivEntry> &        Out      *
// *    passes back an array of column privilege entries, with one entry per   *
// *  column--privileges are combined into one bitmap.                         *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// *  true: All specified privileges have been granted.                        *
// * false: One or more privileges have not been granted                       *
// *                                                                           *
// *****************************************************************************
static bool hasGrantedColumnPriv(
   const std::vector<PrivMgrMDRow *> &colRowList,
   int32_t grantorID,
   int32_t granteeID,
   const std::vector<ColPrivSpec> & colPrivsArray,
   PrivStatus & privStatus,
   std::string & privilege,
   std::vector<ColPrivEntry> & grantedColPrivs)
   
{

   privStatus = STATUS_GOOD;
  
// For each privilege to revoke, see if the grantor has granted that privilege 
// to the grantee for the specified column.  If not, return an error.
 
   for (size_t i = 0; i < colPrivsArray.size(); i++)
   {
      const ColPrivSpec &colPrivEntry = colPrivsArray[i];
      bool grantFound = false;
      for (size_t j = 0; j < colRowList.size(); j++)
      {
         ColumnPrivsMDRow &columnRow = static_cast<ColumnPrivsMDRow &> (*colRowList[j]);

         // Only look at rows with the requested grantor and grantee
         if (columnRow.grantorID_ == grantorID && columnRow.granteeID_ == granteeID)
         {
            if (columnRow.columnOrdinal_ == colPrivEntry.columnOrdinal &&
                columnRow.privsBitmap_.test(colPrivEntry.privType))
            {
               grantFound = true;
               break;
            }
         }
      }

      if (!grantFound)
      {
         privilege = PrivMgrUserPrivs::convertPrivTypeToLiteral((PrivType)colPrivEntry.privType);
         privStatus = STATUS_NOTFOUND;
         return false;
      }
   }

// Build array of granted privileges.  One entry per column granted a privilege
// from grantor to grantee.
   for (size_t j = 0; j < colRowList.size(); j++)
   {
      ColumnPrivsMDRow &columnRow = static_cast<ColumnPrivsMDRow &> (*colRowList[j]);
      if (columnRow.grantorID_ == grantorID && columnRow.granteeID_ == granteeID)
      {
         ColPrivEntry grantedColPriv;
      
         grantedColPriv.columnOrdinal = columnRow.columnOrdinal_;
         grantedColPriv.grantableBitmap = columnRow.grantableBitmap_;
         grantedColPriv.privsBitmap = columnRow.privsBitmap_;
         grantedColPrivs.push_back(grantedColPriv);
      }
   }

   return true;
  
}
//*********************** End of hasGrantedColumnPriv **************************
   
// *****************************************************************************
// *                                                                           *
// * Function: isDelimited                                                     *
// *                                                                           *
// *   This function checks the passed in string for characters other than     *
// *   alphanumeric and underscore characters.  If so, the name is delimited   *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <strToScan>                  const std::string &                In       *
// *    is the string to search for delimited characters                       *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// *  true: the passed in string contains delimited characters                 *
// * false: the passed in string contains no delimited characters              *
// *                                                                           *
// *****************************************************************************
static bool isDelimited( const std::string &strToScan)
{
  char firstChar = strToScan[0];
  if (isdigit(firstChar) || strToScan[0] == '_' )
    return true;
  string validChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
  size_t found = strToScan.find_first_not_of(validChars);
  if (found == string::npos)
    return false;
  return true;
}
//*********************** End of isDelimited ***********************************
   

// *****************************************************************************
//    ObjectPrivsMDRow methods
// *****************************************************************************

void ObjectPrivsMDRow::describeRow (std::string &rowDetails)
{
  rowDetails = "OBJECT_PRIVILEGES row: type is ";
  char objectTypeLit[3] = {0};
  strncpy(objectTypeLit,PrivMgr::ObjectEnumToLit(objectType_),2);
  rowDetails += objectTypeLit;
  rowDetails += ", UID is ";
  rowDetails += to_string((long long int) objectUID_);
  rowDetails += ", grantor is ";
  rowDetails += to_string((long long int)grantorID_);
  rowDetails += ", grantee is ";
  rowDetails += to_string((long long int) granteeID_);
}


// *****************************************************************************
//    ObjectPrivsMDTable methods
// *****************************************************************************

// *****************************************************************************
// * method: ObjectPrivsMDTable::selectWhereUnique
// *                                      
// *  Select the row from the OBJECT_PRIVILEGES table based on the specified
// *  WHERE clause - where clause should only return a single row
// *                                                                 
// *  Parameters:                                                   
// *                                                               
// *  <whereClause> is the WHERE clause specifying a unique row.             
// *  <rowOut>  passes back a set of OBJECT_PRIVILEGES rows
// *                                                         
// * Returns: PrivStatus                                   
// *                                                      
// * STATUS_GOOD: Row returned.                          
// *           *: Select failed. A CLI error is put into the diags area. 
// *****************************************************************************
PrivStatus ObjectPrivsMDTable::selectWhereUnique(
   const std::string & whereClause,
   PrivMgrMDRow & rowOut)
{
   ObjectPrivsMDRow & row = static_cast<ObjectPrivsMDRow &>(rowOut);

   PrivStatus retcode = STATUS_GOOD;
   // Should space be allocated and deleted from the heap for the rowList?
   //   -- how many rows will be returned?
   std::vector<PrivMgrMDRow* > rowList;
   std::string orderByClause;
   retcode = selectWhere(whereClause, orderByClause, rowList);
   if (retcode == STATUS_GOOD)
   {
     // The I/O should be performed on a primary key so only one row returned
     // If not, return an internal error
     if (rowList.size() != 1)
     {
       while(!rowList.empty())
         delete rowList.back(), rowList.pop_back();
       PRIVMGR_INTERNAL_ERROR("Select unique for object_privileges table returned more than 1 row");
       return STATUS_ERROR;
     }
     row = static_cast<ObjectPrivsMDRow &>(*rowList[0]);
   }
   while(!rowList.empty())
     delete rowList.back(), rowList.pop_back();
   return retcode;
}

// *****************************************************************************
// * method: ObjectPrivsMDTable::selectWhere
// *                                      
// *  Selects rows from the OBJECT_PRIVILEGES table based on the specified
// *  WHERE clause.                                                    
// *                                                                 
// *  Parameters:                                                   
// *                                                               
// *  <whereClause> is the WHERE clause
// *  <orderByClause> is the ORDER BY clause defining returned row order.
// *  <rowOut>  passes back a set of OBJECT_PRIVILEGES rows
// *                                                         
// * Returns: PrivStatus                                   
// *                                                      
// * STATUS_GOOD: Row returned.                          
// *           *: Select failed. A CLI error is put into the diags area. 
// *****************************************************************************
PrivStatus ObjectPrivsMDTable::selectWhere(
   const std::string & whereClause,
  const std::string & orderByClause,
   std::vector<PrivMgrMDRow *> &rowList)
{
  std::string selectStmt ("SELECT OBJECT_UID, OBJECT_NAME, OBJECT_TYPE, ");
  selectStmt += ("GRANTEE_ID, GRANTEE_NAME, GRANTEE_TYPE, ");
  selectStmt += ("GRANTOR_ID, GRANTOR_NAME, GRANTOR_TYPE, ");
  selectStmt += ("PRIVILEGES_BITMAP, GRANTABLE_BITMAP FROM ");
  selectStmt += tableName_;
  selectStmt += " ";
  selectStmt += whereClause;
  selectStmt += orderByClause;

  // set pointer in diags area
  int32_t diagsMark = pDiags_->mark();

  ExeCliInterface cliInterface(STMTHEAP, NULL, NULL, 
  CmpCommon::context()->sqlSession()->getParentQid());
  Queue * tableQueue = NULL;
  int32_t cliRC =  cliInterface.fetchAllRows(tableQueue, (char *)selectStmt.c_str(), 0, false, false, true);

  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return STATUS_ERROR;
    }
  if (cliRC == 100) // did not find the row
  {
    pDiags_->rewind(diagsMark);
    return STATUS_NOTFOUND;
  }

  tableQueue->position();
  for (int idx = 0; idx < tableQueue->numEntries(); idx++)
  {
    OutputInfo * pCliRow = (OutputInfo*)tableQueue->getNext();
    ObjectPrivsMDRow *pRow = new ObjectPrivsMDRow();
    setRow(pCliRow, *pRow);
    rowList.push_back(pRow);
  }    

  return STATUS_GOOD;
}

// *****************************************************************************
// * method: ObjectPrivsMDTable::setRow
// *                                      
// *  Create an ObjectPrivsMDRow object from the information returned from the
// *  cli.
// *                                                                 
// *  Parameters:                                                   
// *                                                               
// *  <OutputInfo> row destails from the cli
// *  <rowOut>  passes back the ObjectPrivsMDRow row
// *                                                         
// * no errors should be generated
// *****************************************************************************
// Row read successfully.  Extract the columns.
void ObjectPrivsMDTable::setRow (OutputInfo *pCliRow,
                                 ObjectPrivsMDRow &row)
{
  char * ptr = NULL;
  Int32 len = 0;
  char value[500];

  // column 1:  object uid
  pCliRow->get(0,ptr,len);
  row.objectUID_ = *(reinterpret_cast<int64_t*>(ptr));

   // column 2:  object name
  pCliRow->get(1,ptr,len);
  assert (len < 257);
  strncpy(value, ptr, len);
  value[len] = 0;
  row.objectName_ = value;

  // column 3: object type
  pCliRow->get(2,ptr,len);
  assert (len < 3);
  strncpy(value, ptr, len);
  value[len] = 0;
  row.objectType_ = PrivMgr::ObjectLitToEnum(value);

  // column 4: grantee uid
  pCliRow->get(3,ptr,len);
  row.granteeID_ = *(reinterpret_cast<int32_t*>(ptr));

  // column 5: grantee name
  pCliRow->get(4,ptr,len);
  assert (len < 257);
  strncpy(value, ptr, len);
  value[len] = 0;
  row.granteeName_ = value;

  // column 6: grantee type
  pCliRow->get(5,ptr,len);
  assert (len < 3);
  strncpy(value, ptr, len);
  value[len] = 0;
  row.granteeType_ = value;

  // column 7: grantor uid
  pCliRow->get(6,ptr,len);
  row.grantorID_ = *(reinterpret_cast<int32_t*>(ptr));

  //column 8: grantor name
  pCliRow->get(7,ptr,len);
  assert (len < 257);
  strncpy(value, ptr, len);
  value[len] = 0;
  row.grantorName_ = value;

  //column 9: grantor type
  pCliRow->get(8,ptr,len);
  assert (len < 3);
  strncpy(value, ptr, len);
  value[len] = 0;
  row.grantorType_ = value;

  // column 10: privileges bitmap   
  pCliRow->get(9,ptr,len);
  int64_t bitmapInt = *(reinterpret_cast<int64_t*>(ptr));
  row.privsBitmap_ = bitmapInt;

  // column 11: grantable bitmap
  pCliRow->get(10,ptr,len);
  bitmapInt = *(reinterpret_cast<int64_t*>(ptr));
  row.grantableBitmap_ = bitmapInt;

  // set current_
  PrivMgrCoreDesc tempDesc (row.privsBitmap_, row.grantableBitmap_);
  row.current_= tempDesc; 
  row.visited_.setAllPrivAndWgo(false);
}

// *****************************************************************************
// * method: ObjectPrivsMDTable::insert
// *                                  
// *    Inserts a row into the OBJECT_PRIVILEGES table.     
// *                                               
// *  Parameters:                                 
// *                                             
// *  <rowIn> is a ObjectPrivsMDRow to be inserted.  
// *                                                                    
// * Returns: PrivStatus
// *                   
// * STATUS_GOOD: Row inserted. 
// *           *: Insert failed. A CLI error is put into the diags area. 
// *****************************************************************************
PrivStatus ObjectPrivsMDTable::insert(const PrivMgrMDRow &rowIn)
{

  char insertStmt[2000];
  const ObjectPrivsMDRow &row = static_cast<const ObjectPrivsMDRow &>(rowIn);

  int64_t privilegesBitmapLong = row.privsBitmap_.to_ulong();
  int64_t grantableBitmapLong = row.grantableBitmap_.to_ulong();
  char objectTypeLit[3] = {0};
  
  strncpy(objectTypeLit,PrivMgr::ObjectEnumToLit(row.objectType_),2);
  
  sprintf(insertStmt, "insert into %s values (%ld, '%s', '%s', %d, '%s', '%s', %d, '%s', '%s', %ld, %ld)",
              tableName_.c_str(),
              row.objectUID_,
              row.objectName_.c_str(),
              objectTypeLit,
              row.granteeID_,
              row.granteeName_.c_str(),
              row.granteeType_.c_str(),
              row.grantorID_,
              row.grantorName_.c_str(),
              row.grantorType_.c_str(),
              privilegesBitmapLong,
              grantableBitmapLong);

  ExeCliInterface cliInterface(STMTHEAP, NULL, NULL, 
  CmpCommon::context()->sqlSession()->getParentQid());
  int32_t cliRC = cliInterface.executeImmediate(insertStmt);

   if (cliRC < 0)
   {
      cliInterface.retrieveSQLDiagnostics(pDiags_);
      return STATUS_ERROR;
   }
  
   // For some reason, insert sometimes returns error even though
   // the row is inserted, so unless an errors, return STATUS_GOOD
   return STATUS_GOOD;

}

// *****************************************************************************
// * method: ObjectPrivsMDTable::deleteRow
// *                                  
// *    Deletes a row from the OBJECT_PRIVILEGES table based on the primary key
// *    contents of the row.
// *                                               
// *  Parameters:                                 
// *                                             
// *  <row> defines what row should be deleted
// *                                                                    
// * Returns: PrivStatus
// *                   
// * STATUS_GOOD: Row deleted. 
// *           *: Insert failed. A CLI error is put into the diags area. 
// *****************************************************************************
PrivStatus ObjectPrivsMDTable::deleteRow(const ObjectPrivsMDRow & row)

{

char whereClause[1000];

   sprintf(whereClause," WHERE object_uid = %ld AND grantor_id = %d AND grantee_id = %d ",
           row.objectUID_,row.grantorID_,row.granteeID_);
           
   return deleteWhere(whereClause);

}


// *****************************************************************************
// * method: ObjectPrivsMDTable::deleteWhere
// *                                  
// *    Deletes a row from the OBJECT_PRIVILEGES table based on the where clause
// *                                               
// *  Parameters:                                 
// *                                             
// *  <whereClause> defines what rows should be deleted
// *                                                                    
// * Returns: PrivStatus
// *                   
// * STATUS_GOOD: Row(s) deleted. 
// *           *: Insert failed. A CLI error is put into the diags area. 
// *****************************************************************************
PrivStatus ObjectPrivsMDTable::deleteWhere(const std::string & whereClause)
{
  std::string deleteStmt ("DELETE FROM ");
  deleteStmt += tableName_;
  deleteStmt += " ";
  deleteStmt += whereClause;

  // set pointer in diags area
  int32_t diagsMark = pDiags_->mark();

  ExeCliInterface cliInterface(STMTHEAP, NULL, NULL, 
  CmpCommon::context()->sqlSession()->getParentQid());

  int32_t cliRC = cliInterface.executeImmediate(deleteStmt.c_str());
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return STATUS_ERROR;
    }


  if (cliRC == 100) // did not find any rows
  {
    pDiags_->rewind(diagsMark);
    return STATUS_NOTFOUND;
  }

  if (cliRC > 0)
    return STATUS_WARNING;

  return STATUS_GOOD;
}

// *****************************************************************************
// * method: ObjectPrivsMDTable::updateRow
// *                                  
// *    Updates grantor and bitmaps for a row in the OBJECT_PRIVILEGES table 
// *    based on the contents of the row.
// *                                               
// *  Parameters:                                 
// *                                             
// *  <row> defines what row should be updated
// *                                                                    
// * Returns: PrivStatus
// *                   
// * STATUS_GOOD: Row(s) deleted. 
// *           *: Insert failed. A CLI error is put into the diags area. 
// *****************************************************************************
PrivStatus ObjectPrivsMDTable::updateRow(const ObjectPrivsMDRow & row)

{

char setClause[1000];
int64_t privilegesBitmapLong = row.privsBitmap_.to_ulong();
int64_t grantableBitmapLong = row.grantableBitmap_.to_ulong();

   sprintf(setClause," SET grantor_id = %d, grantor_name = '%s', "
                     "     privileges_bitmap = %ld, grantable_bitmap = %ld  ",
           row.grantorID_,row.grantorName_.c_str(),privilegesBitmapLong,grantableBitmapLong);
           
char whereClause[1000];

   sprintf(whereClause," WHERE object_uid = %ld AND grantor_id = %d AND grantee_id = %d ",
           row.objectUID_,row.grantorID_,row.granteeID_);
           
   return updateWhere(setClause,whereClause);

}



// ----------------------------------------------------------------------------
// method: updateWhere
//
// This method updates one or more rows from the OBJECT_PRIVILEGES table
// The number of rows affected depend on the passed in set clause
//
// Input:  setClause
//         whereClause
// Output:  status of the operation
//
// A cli error is put into the diags area if there is an error
// ----------------------------------------------------------------------------
PrivStatus ObjectPrivsMDTable::updateWhere(const std::string & setClause,
                                           const std::string & whereClause)
{
  std::string updateStmt ("UPDATE ");
  updateStmt += tableName_;
  updateStmt += " ";
  updateStmt += setClause;
  updateStmt += " ";
  updateStmt += whereClause;

  // set pointer in diags area
  int32_t diagsMark = pDiags_->mark();

  ExeCliInterface cliInterface(STMTHEAP, NULL, NULL, 
  CmpCommon::context()->sqlSession()->getParentQid());
  int32_t cliRC = cliInterface.executeImmediate(updateStmt.c_str());
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return STATUS_ERROR;
    }

  if (cliRC == 100) // did not find any rows
  {
    pDiags_->rewind(diagsMark);
    return STATUS_NOTFOUND;
  }

  if (cliRC > 0)
    return STATUS_WARNING;

  return STATUS_GOOD;
}


// ----------------------------------------------------------------------------
// method::insertSelect
//
// This method inserts owner rows into the OBJECT_PRIVILEGES table
//
// Input:   objectsLocation - name of objects table
//          authsLocation - name of auths table
//
// Output:  PrivStatus
//
// the following is a sample insert select statement that gets processed:
//
//  insert into OBJECT_PRIVILEGES
//  select distinct
//    object_uid,
//    <catalogName> "<schema_name>"."<object_name>", 
//    object_type,
//    object_owner, -- granteeID
//    coalesce((select auth_db_name from AUTHS where auth_id = object_owner),
//             'DB__ROOT') --granteeName
//    USER_GRANTEE_LIT, -- "U"
//    SYSTEM_USER,  -- system grantor ID (-2)
//    SYSTEM_AUTH_NAME, -- grantorName (_SYSTEM)
//    SYSTEM_GRANTOR_LIST, -- "S"
//    case
//      when object_type = 'BT' then 47
//      when object_type = 'VI' then 1
//      when object_type = 'LB' then 24
//      when object_type = 'UR' then 64
//      when object_type = 'SG' then 16
//      else 0  
//    end as privilegesBitmap,
//    case
//      when object_type = 'BT' then 47
//      when object_type = 'VI' then 0
//      when object_type = 'LB' then 24
//      when object_type = 'UR' then 0
//      when object_type = 'SG' then 16
//      else 0 
//    end as grantableBitmap
//  from OBJECTS 
//  where object_type in ('VI','BT','LB','UR','SG')
//
// The "coalesce" for the granteeName above is in case the auth_id is
// invalid (that is, does not appear in the AUTHS table). If we don't
// know who the auth_id is, we'll put DB__ROOT, the super user, there.
//   
// The ComDiags area is set up with unexpected errors
// ----------------------------------------------------------------------------
PrivStatus ObjectPrivsMDTable::insertSelect(
   const std::string &objectsLocation,
   const std::string &authsLocation)
{
  // Before inserting rows, make sure that the OBJECT_PRIVILEGES table is empty
  char buf[2000];
  sprintf(buf, "select count(*) from %s", tableName_.c_str());
  Int64 rowsSelected = 0;
  Lng32 theLen = 0;
  ExeCliInterface cliInterface(STMTHEAP, NULL, NULL, 
  CmpCommon::context()->sqlSession()->getParentQid());
  int32_t cliRC = cliInterface.executeImmediate(buf, (char*)&rowsSelected, &theLen, NULL);
  if (cliRC < 0)
  {
    cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
    return STATUS_ERROR;
  }

  if (rowsSelected != 0)
  {
    std::string message ("Found ");
    message += to_string((long long int)rowsSelected);
    message += " rows in OBJECT_PRIVILEGES table, expecting 0 rows";
    PRIVMGR_INTERNAL_ERROR(message.c_str());
    return STATUS_ERROR;
  }

  // Create bitmaps for all supported object types;
  PrivMgrDesc privDesc;
  privDesc.setAllTableGrantPrivileges(true);
  int64_t tableBits = privDesc.getTablePrivs().getPrivBitmap().to_ulong();
 
  privDesc.setAllLibraryGrantPrivileges(true);
  int64_t libraryBits = privDesc.getTablePrivs().getPrivBitmap().to_ulong();

  privDesc.setAllUdrGrantPrivileges(true);
  int64_t udrBits = privDesc.getTablePrivs().getPrivBitmap().to_ulong();

  privDesc.setAllSequenceGrantPrivileges(true);
  int64_t sequenceBits = privDesc.getTablePrivs().getPrivBitmap().to_ulong();

  // for views, privilegesBitmap is set to 1 (SELECT), wgo to 0 (no)
  std::string systemGrantor(SYSTEM_AUTH_NAME);

  // Generate case stmt for grantable bitmap
  sprintf (buf, "case when object_type = 'BT' then %ld "
                "     when object_type = 'VI' then 1 "
                "     when object_type = 'LB' then %ld "
                "     when object_type = 'UR' then %ld "
                "     when object_type = 'SG' then %ld "
                "  else 0 end", 
           tableBits, libraryBits, udrBits, sequenceBits);
  std::string privilegesClause(buf);

  sprintf (buf, "case when object_type = 'BT' then %ld "
                "     when object_type = 'VI' then 0 "
                "     when object_type = 'LB' then %ld "
                "     when object_type = 'UR' then %ld "
                "     when object_type = 'SG' then %ld "
                " else 0 end", 
           tableBits, libraryBits, udrBits, sequenceBits);
  std::string grantableClause(buf);

  sprintf(buf, "insert into %s select distinct object_uid, "
          "trim(catalog_name) || '.\"' || trim(schema_name) ||  '\".\"' || trim(object_name) || '\"', "
          "object_type, object_owner, "
          "coalesce((select auth_db_name from %s where auth_id = o.object_owner),'DB__ROOT') as auth_db_name, "
          "'%s', %d, '%s', '%s', %s, %s from %s o " 
          "where o.object_type in ('VI','BT','LB','UR','SG')",
          tableName_.c_str(),
          authsLocation.c_str(),
          USER_GRANTEE_LIT,
          SYSTEM_USER, SYSTEM_AUTH_NAME, SYSTEM_GRANTOR_LIT,
          privilegesClause.c_str(), grantableClause.c_str(),
          objectsLocation.c_str());

  // set pointer in diags area
  int32_t diagsMark = pDiags_->mark();

  Int64 rowsInserted = 0;
  cliRC = cliInterface.executeImmediate(buf, NULL, NULL, FALSE, &rowsInserted);
  if (cliRC < 0)
  {
    cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
    return STATUS_ERROR;
  }

  // Bug:  for some reasons, insert returns NOTFOUND even though the 
  //       operations succeeded.
  if (cliRC == 100) 
  {
    pDiags_->rewind(diagsMark);
    cliRC = 0;
  }

  // Make sure rows were inserted correctly.
  // Get the expected number of rows
 sprintf(buf, "select count(*) from %s o where o.object_type in ('VI','BT','LB','UR', 'SG')"  
              " and object_owner > 0",
              objectsLocation.c_str());
  Lng32 len = 0;
  cliRC = cliInterface.executeImmediate(buf, (char*)&rowsSelected, &len, NULL);
  if (cliRC < 0)
  {
    cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
    return STATUS_ERROR;
  }

  // Check to see if the number of rows selected match the rows inserted
  if (rowsInserted != rowsSelected)
  {
    std::string message ("Expected to insert ");
    message += to_string((long long int)rowsSelected);
    message += " rows into OBJECT_PRIVILEGES table, instead ";
    message += to_string((long long int)rowsInserted);
    message += " were found.";
    PRIVMGR_INTERNAL_ERROR(message.c_str());
    return STATUS_ERROR;
  }

 
  return STATUS_GOOD;
}

// ----------------------------------------------------------------------------
// method::insertSelect
//
// This method inserts a grant of SELECT on the AUTHS table to PUBLIC
//  into the OBJECT_PRIVILEGES table
//
// Input:   objectsLocation - name of objects table
//          authsLocation - name of auths table
//
// Output:  PrivStatus
//
// The ComDiags area is set up with unexpected errors
// ----------------------------------------------------------------------------
PrivStatus ObjectPrivsMDTable::insertSelectOnAuthsToPublic(
   const std::string &objectsLocation,
   const std::string &authsLocation)
{

  char buf[2000];

  sprintf(buf, "insert into %s select o.object_uid,'%s','BT',-1,'PUBLIC','U',"
               "%d,'DB__ROOT','U',1,0 FROM %s O WHERE O.OBJECT_NAME = 'AUTHS'", 
              tableName_.c_str(),authsLocation.c_str(), MIN_USERID, objectsLocation.c_str());

  // set pointer in diags area
  int32_t diagsMark = pDiags_->mark();

  Int64 rowsInserted = 0;
  ExeCliInterface cliInterface(STMTHEAP, NULL, NULL, 
  CmpCommon::context()->sqlSession()->getParentQid());
  int32_t cliRC = cliInterface.executeImmediate(buf, NULL, NULL, FALSE, &rowsInserted);
  if (cliRC < 0)
  {
    cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
    return STATUS_ERROR;
  }

  // Bug:  for some reasons, insert returns NOTFOUND even though the 
  //       operations succeeded.
  if (cliRC == 100) 
  {
    pDiags_->rewind(diagsMark);
    cliRC = 0;
  }
 
  return STATUS_GOOD;
  
}

// *****************************************************************************
//    ColumnPrivsMDRow methods
// *****************************************************************************

void ColumnPrivsMDRow::describeRow (std::string &rowDetails)
{
  rowDetails = "COLUMN_PRIVILEGES row: UID is ";
  rowDetails += to_string((long long int) objectUID_);
  rowDetails += ", column number is ";
  rowDetails += to_string((long long int) columnOrdinal_);
  rowDetails += ", grantor is ";
  rowDetails += to_string((long long int)grantorID_);
  rowDetails += ", grantee is ";
  rowDetails += to_string((long long int) granteeID_);
}

// *****************************************************************************
//    ColumnPrivsMDTable methods
// *****************************************************************************

// *****************************************************************************
// *                                                                           *
// * Function: ColumnPrivsMDTable::insert                                      *
// *                                                                           *
// *    Inserts a row into the COLUMN_PRIVILEGES table.                        *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <rowIn>                         const PrivMgrMDRow &            In       *
// *    is a ColumnPrivsMDRow to be inserted.                                  *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Row inserted.                                                *
// *           *: Insert failed. A CLI error is put into the diags area.       *
// *                                                                           *
// *****************************************************************************
PrivStatus ColumnPrivsMDTable::insert(const PrivMgrMDRow &rowIn)

{

char insertStmt[2000];
const ColumnPrivsMDRow &row = static_cast<const ColumnPrivsMDRow &>(rowIn);

int64_t privilegesBitmapLong = row.privsBitmap_.to_ulong();
int64_t grantableBitmapLong = row.grantableBitmap_.to_ulong();
  
   sprintf(insertStmt, "INSERT INTO %s VALUES (%ld, '%s', %d, '%s', %d, '%s', %d, %ld, %ld)",
           tableName_.c_str(),
           row.objectUID_,
           row.objectName_.c_str(),
           row.granteeID_,
           row.granteeName_.c_str(),
           row.grantorID_,
           row.grantorName_.c_str(),
           row.columnOrdinal_,
           privilegesBitmapLong,
           grantableBitmapLong);
              
   return CLIImmediate(insertStmt);

}
//********************* End of ColumnPrivsMDTable::insert **********************

// *****************************************************************************
// *                                                                           *
// * Function: ColumnPrivsMDTable::selectWhere                                 *
// *                                                                           *
// *    Selects rows from the COLUMN_PRIVILEGES table based on the specified   *
// * WHERE clause.  Output is sorted by grantor, grantee, column in that order.*
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <whereClause>                   const std::string &             In       *
// *    is the WHERE clause specifying a unique row.                           *
// *                                                                           *
// *  <orderByClause> is the ORDER BY clause defining returned row order.
// *  <rowList>                       std::vector<PrivMgrMDRow *> &   Out      *
// *    passes back a set of ColumnPrivsMDRow rows.                            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Rows returned.                                               *
// *           *: Select failed. A CLI error is put into the diags area.       *
// *                                                                           *
// *****************************************************************************
PrivStatus ColumnPrivsMDTable::selectWhere(
   const std::string & whereClause,
      const std::string & orderByClause,
   std::vector<PrivMgrMDRow *> &rowList)
{

std::string selectStmt("SELECT object_uid,object_name,"
                       "grantee_id,grantee_name,"
                       "grantor_id,grantor_name,column_number,"
                       "privileges_bitmap,grantable_bitmap FROM ");

  selectStmt += tableName_ + " ";
  selectStmt += whereClause + orderByClause;

// set pointer in diags area
int32_t diagsMark = pDiags_->mark();

ExeCliInterface cliInterface(STMTHEAP);
Queue * tableQueue = NULL;
int32_t cliRC =  cliInterface.fetchAllRows(tableQueue, (char *)selectStmt.c_str(), 0, false, false, true);

   if (cliRC < 0)
   {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return STATUS_ERROR;
   }
   if (cliRC == 100) // did not find the row
   {
      pDiags_->rewind(diagsMark);
      return STATUS_NOTFOUND;
   }

   tableQueue->position();
   for (int idx = 0; idx < tableQueue->numEntries(); idx++)
   {
      OutputInfo * pCliRow = (OutputInfo*)tableQueue->getNext();
      ColumnPrivsMDRow *pRow = new ColumnPrivsMDRow();
      setRow(pCliRow,*pRow);
      rowList.push_back(pRow);
   }    

   return STATUS_GOOD;
    
}
//****************** End of ColumnPrivsMDTable::selectWhere ********************

// *****************************************************************************
// *                                                                           *
// * Function: ColumnPrivsMDTable::selectWhereUnique                           *
// *                                                                           *
// *    Selects a row from the COLUMN_PRIVILEGES table based on the specified  *
// * WHERE clause.                                                             *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <whereClause>                   const std::string &             In       *
// *    is the WHERE clause specifying a unique row.                           *
// *                                                                           *
// *  <row>                           PrivMgrMDRow &                  Out      *
// *    passes back a ColumnPrivsMDRow row.                                    *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Row returned.                                                *
// *           *: Select failed. A CLI error is put into the diags area.       *
// *                                                                           *
// *****************************************************************************
PrivStatus ColumnPrivsMDTable::selectWhereUnique(
   const std::string & whereClause,
   PrivMgrMDRow & row) 
   
{
//TODO: Currently unused.  Added due to virtual declaration.  Will be fleshed 
// out when run-time column privilege checking added.
   return STATUS_GOOD;

}
//************** End of ColumnPrivsMDTable::selectWhereUnique ******************

// *****************************************************************************
// *                                                                           *
// * Function: MyTable::setRow                                                 *
// *                                                                           *
// *  Create a ColumnPrivsMDRow object from the information returned from the
// *  CLI.
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <pCliRow>                       OutputInfo &                    In       *
// *    is a pointer to the CLI interface to the row data that was read.       *
// *                                                                           *
// *  <row>                        PrivMgrMDRow &                     Out      *
// *    passes back a ColumnPrivsMDRow.                                        *
// *                                                                           *
// *****************************************************************************
void ColumnPrivsMDTable::setRow(
   OutputInfo *pCliRow,
   ColumnPrivsMDRow &row)
   
{
  char * ptr = NULL;
  Int32 len = 0;
  char value[500];

  // column 0:  object uid
  pCliRow->get(0,ptr,len);
  row.objectUID_ = *(reinterpret_cast<int64_t*>(ptr));

  // column 1: object name
  pCliRow->get(1,ptr,len);
  assert (len < 257);
  strncpy(value, ptr, len);
  value[len] = 0;
  row.objectName_ = value;

  // column 2: grantee id
  pCliRow->get(2,ptr,len);
  row.granteeID_ = *(reinterpret_cast<int32_t*>(ptr));

  // column 3: grantee name
  pCliRow->get(3,ptr,len);
  assert (len < 257);
  strncpy(value, ptr, len);
  value[len] = 0;
  row.granteeName_ = value;

  // column 4: grantor id
  pCliRow->get(4,ptr,len);
  row.grantorID_ = *(reinterpret_cast<int32_t*>(ptr));

  // column 5:  grantor name
  pCliRow->get(5,ptr,len);
  assert (len < 257);
  strncpy(value, ptr, len);
  value[len] = 0;
  row.grantorName_ = value;

  // column 6: column_number
  pCliRow->get(6,ptr,len);
  row.columnOrdinal_ = *(reinterpret_cast<int32_t*>(ptr));

  // column 7: privileges bitmap   
  pCliRow->get(7,ptr,len);
  int64_t bitmapInt = *(reinterpret_cast<int64_t*>(ptr));
  row.privsBitmap_ = bitmapInt;

  // column 8: grantable bitmap
  pCliRow->get(8,ptr,len);
  bitmapInt = *(reinterpret_cast<int64_t*>(ptr));
  row.grantableBitmap_ = bitmapInt;

}
//******************* End of ColumnPrivsMDTable::setRow ************************

// *****************************************************************************
// *                                                                           *
// * Function: ColumnPrivsMDTable::updateRow                                   *
// *                                                                           *
// *   Updates the bitmaps for a row in the COLUMN_PRIVILEGES table based on   *
// * the contents of the row.                                                  *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <row>                           PrivMgrMDRow &                  In       *
// *    is the row to be updated.                                              *
// *                                                                           *
// *  <whereBase>                    const std::string &              In       *
// *    is the WHERE clause specifying the primary keys except for the         *
// * column number, which is added within this function.                       *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Row returned.                                                *
// *           *: Select failed. A CLI error is put into the diags area.       *
// *                                                                           *
// *****************************************************************************
PrivStatus ColumnPrivsMDTable::updateColumnRow(
   const ColumnPrivsMDRow & row,
   const std::string whereBase)

{

char setClause[1000];
int64_t privilegesBitmapLong = row.privsBitmap_.to_ulong();
int64_t grantableBitmapLong = row.grantableBitmap_.to_ulong();

   sprintf(setClause," SET privileges_bitmap = %ld, grantable_bitmap = %ld  ",
           privilegesBitmapLong,grantableBitmapLong);
           
char whereClause[1000];

   sprintf(whereClause," %s %d",whereBase.c_str(),row.columnOrdinal_);
           
   return updateWhere(setClause,whereClause);

}
//*************** End of ColumnPrivsMDTable::updateColumnRow *******************





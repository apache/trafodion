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
#include <vector>
#include "sqlcli.h"
#include "ComSmallDefs.h"
#include "ExExeUtilCli.h"
#include "ComDiags.h"
#include "ComQueue.h"
#include "CmpCommon.h"
#include "CmpContext.h"
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

// sets the privilege and grantable bitmaps to 0
   void clearVisited() 
   { 
     visited_.setColumnOrdinal(columnOrdinal_);
     visited_.setAllPrivAndWgo(false);
   }

// sets the current entry to match the original privileges
// before they are adjusted by a revoke command
   void setCurrentToOriginal() 
   { 
     current_.setColumnOrdinal(columnOrdinal_);
     current_.setPrivBitmap(privsBitmap_);
     current_.setWgoBitmap(grantableBitmap_);
   }
 
// compares the current privileges with the visited grant tree to
// see if there are any broken branches
   NABoolean anyNotVisited() 
   {return current_.getPrivBitmap() != visited_.getPrivBitmap() || 
           current_.getWgoBitmap() != visited_.getWgoBitmap();}


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
   
   PrivMgrCoreDesc    visited_;
   PrivMgrCoreDesc    current_;

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

void static closeColumnList(std::string & columnList);   

static void deleteRowList(std::vector<PrivMgrMDRow *> & rowList);

static PrivMgrCoreDesc * findColumnEntry(
   NAList<PrivMgrCoreDesc> & colPrivsToGrant,
   const int32_t columnsOrdinal);
   
static void getColRowsForGranteeGrantor(
   const std::vector <PrivMgrMDRow *> & columnRowList,
   const int32_t granteeID,
   const int32_t grantorID,
   NAList<PrivMgrCoreDesc> &colPrivGrants);
   
static bool hasAllDMLPrivs(
   ComObjectType objectType,
   PrivObjectBitmap privBitmap);   
   
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
// * Method: getPrivsOnObject                                
// *                                                       
// * Creates a set of priv descriptors for all user grantees on an object
// * Used by Trafodion compiler to store as part of the table descriptor.
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <objectType> is the type of object
// *  <privDescs> is the list of privileges the on the object
// *                                                                  
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: privilege descriptors were built
// *           *: unexpected error occurred, see diags.     
// *  
// *****************************************************************************
PrivStatus PrivMgrPrivileges::getPrivsOnObject (
  const ComObjectType objectType,
  std::vector<PrivMgrDesc> & privDescs )
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
  
  // generate the list of privileges granted to the column and store in class
  if (generateColumnRowList() == STATUS_ERROR)
    return STATUS_ERROR;
  
  // Gets all the user grantees (userIDs) for the object and column lists
  // Gets all the role grantees (roleIDs) for the object and column lists
  // The public auth ID is included in the userIDs list
  std::vector<int32_t> userIDs;
  std::vector<int32_t> roleIDs;
  if (getDistinctIDs(objectRowList_, columnRowList_, userIDs, roleIDs) == STATUS_ERROR)
    return STATUS_ERROR;

  // Gather privilege descriptors for each user
  for (size_t i = 0; i < userIDs.size(); i++)
  {
    int32_t userID = userIDs[i];

    PrivMgrDesc privsOfTheUser(userID);
    bool hasManagePrivileges = false;
    std::vector <int32_t> emptyRoleIDs;
 
    // getUserPrivs returns object and column privileges summarized across all
    // grantors.  Just get direct grants to the user (role list is empty) 
    if (getUserPrivs(objectType, userID, emptyRoleIDs, privsOfTheUser,
                     hasManagePrivileges) != STATUS_GOOD)
      return STATUS_ERROR;
    
    if (!privsOfTheUser.isNull())
      privDescs.push_back(privsOfTheUser);
  }
  
  // Attach roles to priv lists
  for (size_t i = 0; i < roleIDs.size(); i++)
  {
    int32_t grantee = roleIDs[i];
    
    PrivMgrDesc privsOfTheUser(grantee);
    bool hasManagePrivileges = false;
    std::vector <int32_t> emptyRoleIDs;
    
    // getUserPrivs returns object and column privileges summarized across
    // all grantors. 
    if (getUserPrivs(objectType, grantee, emptyRoleIDs, privsOfTheUser,
                     hasManagePrivileges) != STATUS_GOOD)
      return STATUS_ERROR;
    
    if (!privsOfTheUser.isNull())
      privDescs.push_back(privsOfTheUser);
  }

  return STATUS_GOOD;
}


// *****************************************************************************
// Function: getColRowsForGranteeOrdinal                                     
//                                                                           
//    Returns the list of column privileges granted for the object that have 
//    been granted to the granteeID for a particular column                  
//
//  Parameters:                                                             
//                                                                         
//  <granteeID> is the authID granted the privileges.                              
//  <columnOrdinal> is the column number to gather privileges
//  <columnRowList> is the list of column privs for the object
//  <roleIDs> is the list of roles assigned the granteeID
//  <rowList> the privileges granted to <granteeID> on <columnOrdinal>.  
//                                                                          
// *****************************************************************************
void PrivMgrPrivileges::getColRowsForGranteeOrdinal(
  const int32_t granteeID,
  const int32_t columnOrdinal,
  const std::vector <PrivMgrMDRow *> &columnRowList,
  const std::vector<int32_t> &roleIDs,
  std::vector<PrivMgrMDRow *> &rowList)
{
  for (size_t i = 0; i < columnRowList.size(); ++i)
  {
    ColumnPrivsMDRow &row = static_cast<ColumnPrivsMDRow &> (*columnRowList[i]);
    PrivMgrCoreDesc colPrivGrant;

    if (row.columnOrdinal_ == columnOrdinal)
    {
      if (row.granteeID_ == granteeID || 
          std::find(roleIDs.begin(), roleIDs.end(), row.granteeID_) != roleIDs.end()) 
      {
         ColumnPrivsMDRow *pRow = new ColumnPrivsMDRow();
         *pRow = row;
         rowList.push_back(pRow);
      }
    }
  }
}

// *****************************************************************************
// * Method: getDistinctIDs                                
// *
// * Finds all the userIDs that have been granted at least one privilege on
// * object. This list includes the public authorization ID
// *
// * Finds all the roleIDs that have been granted at least one privilege on
// * object
// *****************************************************************************
PrivStatus PrivMgrPrivileges::getDistinctIDs(
  const std::vector <PrivMgrMDRow *> &objectRowList,
  const std::vector <PrivMgrMDRow *> &columnRowList,
  std::vector<int32_t> &userIDs,
  std::vector<int32_t> &roleIDs)
{
  int32_t authID;

  // DB__ROOTROLE is a special role.  If the current user has been granted 
  // this role, then they have privileges.  Add it to the roleIDs list for
  // processing.
  roleIDs.push_back(ROOT_ROLE_ID);

  // The direct object grants are stored in memory (objectRowList) so no I/O
  for (size_t i = 0; i < objectRowList.size(); i++)
  {
    ObjectPrivsMDRow &row = static_cast<ObjectPrivsMDRow &> (*objectRowList[i]);
    authID = row.granteeID_;

    // insert authID into correct list, if not already present
    if (ComUser::isPublicUserID(authID) || CmpSeabaseDDLauth::isUserID(authID)) 
    {
       if (std::find(userIDs.begin(), userIDs.end(), authID) == userIDs.end())
         userIDs.insert( std::upper_bound( userIDs.begin(), userIDs.end(), authID ), authID);
    }
    else
    {
       if (std::find(roleIDs.begin(), roleIDs.end(), authID) == roleIDs.end())
         roleIDs.insert( std::upper_bound( roleIDs.begin(), roleIDs.end(), authID ), authID);
    }
  }

  // The direct column grants are stored in memory (columnRowList) so no I/O
  // Save grants to roles for further processing
  for (size_t i = 0; i < columnRowList.size(); i++)
  {
    ColumnPrivsMDRow &row = static_cast<ColumnPrivsMDRow &> (*columnRowList[i]);
    authID = row.granteeID_;

    // insert authID into correct list, if not already present
    if (ComUser::isPublicUserID(authID) ||
        CmpSeabaseDDLauth::isUserID(authID)) 
    {
      if (std::find(userIDs.begin(), userIDs.end(), authID) == userIDs.end())
        userIDs.insert( std::upper_bound( userIDs.begin(), userIDs.end(), authID ), authID);
    }
    else
    {
       if (std::find(roleIDs.begin(), roleIDs.end(), authID) == roleIDs.end())
         roleIDs.insert( std::upper_bound( roleIDs.begin(), roleIDs.end(), authID ), authID);
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
// * Method: grantColumnPriv                                
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
PrivStatus PrivMgrPrivileges::grantColumnPriv(
   const ComObjectType objectType,
   const int32_t granteeID,
   const std::string &granteeName,
   const std::string &grantorName,
   const PrivMgrDesc &privsToGrant,
   const bool isWGOSpecified)
{

  std::string traceMsg;

  PrivStatus privStatus = STATUS_GOOD;
  
  log (__FILE__, "Checking column privileges", -1);

  NAList<PrivMgrCoreDesc> colPrivsToGrant = privsToGrant.getColumnPrivs();

  // Verify that view-col <=> referenced_col relationship exists
  if (objectType == COM_VIEW_OBJECT)
  {
    ViewUsage myUsage;
    myUsage.viewUID = objectUID_;
    PrivMgrMDAdmin admin(trafMetadataLocation_, metadataLocation_, pDiags_);
    if (admin.getViewColUsages(myUsage) == STATUS_ERROR)
      return STATUS_ERROR;
    if (myUsage.viewColUsagesStr.empty())
    {
      *pDiags_ << DgSqlCode (-CAT_COLUMN_PRIVILEGE_NOT_ALLOWED)
               << DgTableName (objectName_.c_str());
       return STATUS_ERROR;
    }
  }


  bool rowWritten = false;

  std::string whereBase(" WHERE object_uid = ");

  whereBase += UIDToString(objectUID_);
  whereBase += " AND grantor_id = ";
  whereBase += authIDToString(grantorID_);
  whereBase += " AND grantee_id = ";
  whereBase += authIDToString(granteeID);
  whereBase += " AND column_number = ";
    
  ColumnPrivsMDTable columnPrivsTable(columnTableName_,pDiags_);

  // Get privileges on the object for grantor/grantee
  ObjectPrivsMDRow objRow;
  PrivObjectBitmap objPrivsBitmap;
  PrivObjectBitmap objGrantableBitmap;
  privStatus = getGrantedPrivs(granteeID, objRow);

  // getGrantedPrivs returns STATUS_GOOD or STATUS_NOTFOUND
  // if STATUS_NOTFOUND, bitmaps are empty (as initialized above)
  // if STATUS_GOOD, set current bitmaps
  if (privStatus == STATUS_GOOD)
  {
    objPrivsBitmap = objRow.privsBitmap_;
    objGrantableBitmap = objRow.grantableBitmap_;
  }

  // Get existing column grants from grantor to the specified grantee.
  NAList<PrivMgrCoreDesc> grantedColPrivs(STMTHEAP);
  getColRowsForGranteeGrantor(columnRowList_,
                              granteeID,grantorID_,
                              grantedColPrivs);


  // Walk the list of column privileges to grant, and either insert a new
  // row in the COLUMN_PRIVILEGES table or update an existing row.  
  for (size_t i = 0; i < colPrivsToGrant.entries(); i++)
  {
    PrivMgrCoreDesc &colPrivToGrant = colPrivsToGrant[i];

    // TBD - add a describe function to PrivMgrCoreDesc and PrivMgrDesc so
    // information can ge logged.

    bool updateOperation = false;
    bool skipOperation = false; 

    PrivMgrCoreDesc *grantedColPriv = findColumnEntry(grantedColPrivs, colPrivToGrant.getColumnOrdinal());
    if (grantedColPriv)
    {
      // An existing row with the same column has been found, it is one of four cases:
      //
      // 1) AuthID had WGO, now trying to take away WGO [error]
      // 2) Adding a privilege (e.g., authID had SELECT, now granting INSERT) [update operation]
      // 3) AuthID had privilege, now adding WGO [update operation]
      // 4) AuthID already has privilege and/or WGO specified [skip operation]
         
      // case 1: see if trying to take away WGO -  anyNotSet returns true iff any 
      // WGO bit set in grantedColPriv is not set in colPrivToGrant - can this 
      // really occur?
      if (colPrivToGrant.getPrivBitmap() == grantedColPriv->getPrivBitmap() &&
          grantedColPriv->anyNotSet(colPrivToGrant))
      {
         PRIVMGR_INTERNAL_ERROR("trying to remove WGO during grant");
         return STATUS_ERROR;
      }
 
      // Case 2: If the privilege bitmaps are not the same, adding a privilege.
      // This is an update operation
      if (colPrivToGrant.getPrivBitmap() != grantedColPriv->getPrivBitmap())
      {
        updateOperation = true; 
        colPrivToGrant.unionOfPrivs(*grantedColPriv);
      }
         
      else 
        // Case 3: the privileges match, see if there are any additional WGO privs 
        // to set and mark updatable 
        // anyNotSet returns true iff any WGO bit set in colPrivToGrant is not set in 
        // grantedColPriv, this means more WGO bits need to be set.
        if (colPrivToGrant.anyNotSet(*grantedColPriv))
        {
          updateOperation = true; 
          colPrivToGrant.unionOfPrivs(*grantedColPriv);
        }
        // Case 4: no changes to priv or WGO bits -  no updates required - skip
        else
          skipOperation = true;
    }
      
    // Done with this entry, go to the next one
    if (skipOperation)
      continue;
      
    // Create an ObjectUsage to propagate the privilege change to dependent
    // objects
    ObjectUsage objectUsage;
    objectUsage.objectUID = objectUID_;
    objectUsage.granteeID = granteeID;
    objectUsage.objectName = objectName_;
    objectUsage.objectType = objectType;

    PrivMgrCoreDesc currentPrivs; // creates an empty descriptor
    PrivMgrCoreDesc tempPrivs(objPrivsBitmap, objGrantableBitmap);
    objectUsage.originalPrivs.setTablePrivs(tempPrivs);
    objectUsage.updatedPrivs.setTablePrivs(tempPrivs); 

    // Create list of ColumnReferences
    objectUsage.columnReferences = new std::vector<ColumnReference *>;
    for (size_t i = 0; i < colPrivsToGrant.entries(); i++)
    {
       PrivMgrCoreDesc &colPrivToGrant = colPrivsToGrant[i];
       PrivMgrCoreDesc *grantedColPriv = findColumnEntry(grantedColPrivs, colPrivToGrant.getColumnOrdinal());

       ColumnReference *adjustedCol = new ColumnReference;
       adjustedCol->columnOrdinal = colPrivToGrant.getColumnOrdinal();

       PrivMgrCoreDesc adjustedPrivs;
       if (grantedColPriv)
          adjustedPrivs = *grantedColPriv;

       adjustedCol->originalPrivs = adjustedPrivs;
       adjustedPrivs.unionOfPrivs(colPrivToGrant);
       adjustedCol->updatedPrivs = adjustedPrivs;
       objectUsage.columnReferences->push_back(adjustedCol);
    }

    // Propagate the privilege change to dependent objects
    if (updateDependentObjects(objectUsage, PrivCommand::GRANT_COLUMN) == STATUS_ERROR)
      return STATUS_ERROR;

    // Update the COLUMN_PRIVILEGES table with new privileges
    // Prepare for the insert or update request
    ColumnPrivsMDRow row;  
       
    row.objectUID_ = objectUID_;
    row.objectName_ = objectName_;
    row.granteeID_ = granteeID;     
    row.granteeName_ = granteeName;
    row.grantorID_ = grantorID_; 
    row.grantorName_ = grantorName;
    row.privsBitmap_ = colPrivToGrant.getPrivBitmap();
    row.grantableBitmap_ = colPrivToGrant.getWgoBitmap();
    row.columnOrdinal_ = colPrivToGrant.getColumnOrdinal();

    if (updateOperation)
      privStatus = columnPrivsTable.updateColumnRow(row,whereBase);
    else
      privStatus = columnPrivsTable.insert(row);
         
    if (privStatus == STATUS_ERROR)
      return privStatus;

    rowWritten = true;
  } 

  return STATUS_GOOD;
  
}
//************* End of PrivMgrPrivileges::grantColumnPriv ****************



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

  PrivMgrDesc privsToGrant(granteeID);
  PrivMgrDesc privsOfTheGrantor(grantorID_);
  std::vector<int_32> roleIDs;
  retcode = initGrantRevoke(objectType, granteeID, grantorName,
                            privsList, colPrivsArray,
                            isAllSpecified, isWGOSpecified, true,
                            privsToGrant, privsOfTheGrantor, roleIDs);
  if (retcode != STATUS_GOOD)
    return retcode;

  // Make sure the grantor can grant at least one of the requested privileges
  // SQL Ansi states that privileges that can be granted should be done so
  // even if some requested privilege are not grantable.
  // Remove any privsToGrant which are not held GRANTABLE by the Grantor.
  bool warnNotAll = false;
  PrivMgrDesc origPrivsToGrant = privsToGrant;

  // if limitToGrantable true ==> some specified privs were not grantable.
  if ( privsToGrant.limitToGrantable( privsOfTheGrantor ) )
    warnNotAll = true;

  // If nothing left to grant, we are done.
  // If one of the users roles has privilege, indicate in error message
  if ( privsToGrant.isNull() )
  {
    std::string rolesWithPrivs;
    if (getRolesToCheck(grantorID_, roleIDs, objectType, rolesWithPrivs)== STATUS_GOOD)
    {
      if (rolesWithPrivs.size() > 0)
      {
        *pDiags_ << DgSqlCode (-CAT_PRIVILEGE_NOT_GRANTED)
                 << DgString0 (grantorName.c_str())
                 << DgString1 (rolesWithPrivs.c_str());
        return STATUS_ERROR;
      }
    }

    *pDiags_ << DgSqlCode (-CAT_PRIVILEGE_NOT_GRANTED)
             << DgString0 (grantorName.c_str());
    return STATUS_ERROR;
  }

  // grant any column level privileges
  if ( !privsToGrant.isColumnLevelNull() )
  {
    retcode = grantColumnPriv(objectType,granteeID,granteeName,grantorName,
                              privsToGrant,isWGOSpecified); 
    if (retcode != STATUS_GOOD)
      return retcode;
  }
    
  // If only column-level privileges were specified, no problem.  
  if (privsToGrant.getTablePrivs().isNull())
  {
    // report any privs not granted
    if (warnNotAll)
      reportPrivWarnings(origPrivsToGrant,
                         privsToGrant,
                         CAT_NOT_ALL_PRIVILEGES_GRANTED);

    log (__FILE__, "****** GRANT operation succeeded ******", -1);
    return STATUS_GOOD;
  }
  
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
  {
    if (warnNotAll)
      reportPrivWarnings(origPrivsToGrant,
                         privsToGrant,
                         CAT_NOT_ALL_PRIVILEGES_GRANTED);
      return STATUS_GOOD;
  }

  // Internal consistency check.  We should have granted something.
  assert( result != PrivMgrCoreDesc::NEUTRAL );

  // There is something to grant, update/insert metadata

  // set up row if it does not exist and add it to the objectRowList
  if (!foundRow)
  {
    ObjectPrivsMDRow *pRow = new ObjectPrivsMDRow();
    pRow->objectUID_ = objectUID_;
    pRow->objectName_ = objectName_;
    pRow->objectType_ = objectType;
    pRow->granteeID_ = granteeID;
    pRow->granteeName_ = granteeName;
    pRow->granteeType_ = USER_GRANTEE_LIT;
    pRow->grantorID_ = grantorID_;
    pRow->grantorName_ = grantorName;
    pRow->grantorType_ = USER_GRANTOR_LIT;
    pRow->privsBitmap_.reset();
    pRow->grantableBitmap_.reset(); 
    objectRowList_.push_back(pRow);
    row = *pRow;
  }

  // combine privsToGrant with existing privs
  else
  {
    privsToGrant.unionOfPrivs(currentPrivs);
    row.privsBitmap_ = privsToGrant.getTablePrivs().getPrivBitmap();
    row.grantableBitmap_ = privsToGrant.getTablePrivs().getWgoBitmap();
  }

  // Update dependent objects
  ObjectUsage objectUsage;
  objectUsage.objectUID = objectUID_;
  objectUsage.granteeID = granteeID;
  objectUsage.grantorIsSystem = false;
  objectUsage.objectName = row.objectName_;
  objectUsage.objectType = row.objectType_;
  objectUsage.columnReferences = NULL;

  PrivMgrDesc originalPrivs (row.granteeID_);
  originalPrivs.setTablePrivs(savedOriginalPrivs);
  objectUsage.originalPrivs = originalPrivs;
  objectUsage.updatedPrivs = privsToGrant;
  
  if (updateDependentObjects(objectUsage, PrivCommand::GRANT_OBJECT) == STATUS_ERROR)
    return STATUS_ERROR;

  ObjectPrivsMDTable objectPrivsTable (objectTableName_, pDiags_);
  if (foundRow)
  {
    row.describeRow(traceMsg);
    traceMsg.insert(0, "updating existing privilege row ");
    log (__FILE__, traceMsg, -1);

    // update the row
    retcode = objectPrivsTable.updateRow(row);
  }
  else
  {
    row.privsBitmap_ = privsToGrant.getTablePrivs().getPrivBitmap();
    row.grantableBitmap_ = privsToGrant.getTablePrivs().getWgoBitmap();
    row.describeRow(traceMsg);
    traceMsg.insert(0, "adding new privilege row ");
    log (__FILE__, traceMsg, -1);

    // insert the row
    retcode = objectPrivsTable.insert(row);
  }

  if (warnNotAll)
    reportPrivWarnings(origPrivsToGrant, 
                       privsToGrant, 
                       CAT_NOT_ALL_PRIVILEGES_GRANTED);

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

   corePrivs.setAllObjectPrivileges(objectType,true/*priv*/,true/*wgo*/);
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
   if (ownerID == creatorID || creatorID == ComUser::getRootUserID() ||
       ownerID == HIVE_ROLE_ID || ownerID == HBASE_ROLE_ID)
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
// method: initGrantRevoke
//
// This method initializes structures needed to complete grant and revoke 
// operations.
//
// It returns:
//    privsToApply - the list of requested privileges to grant/revoke 
//    privsOfTheGrantor - privileges of the grantor to be used in later checks
//
// It also reads privileges for the object at table and column level from the
// metadata and stores the results in the class
// *****************************************************************************
PrivStatus PrivMgrPrivileges::initGrantRevoke(
    const ComObjectType objectType,
    const int32_t granteeID,
    const std::string &grantorName,
    const std::vector<PrivType> &privList,
    const std::vector<ColPrivSpec> & colPrivsArray,
    const bool isAllSpecified,
    const bool isGOSpecified,
    const bool isGrant,
    PrivMgrDesc &privsToApply,
    PrivMgrDesc &privsOfTheGrantor,
    std::vector<int32_t> & roleIDs)
{
  if (objectUID_ == 0)
  {
    PRIVMGR_INTERNAL_ERROR("objectUID is 0 for grant or revoke command");
    return STATUS_ERROR;
  }

  // Generate the list of privilege descriptors that were requested 
  PrivStatus retcode = convertPrivsToDesc(objectType,
                               isAllSpecified,
                               (isGrant) ? isGOSpecified : true, // WGO
                               (isGrant) ? false : isGOSpecified, // GOF
                               privList,
                               colPrivsArray,
                               privsToApply);
  if (retcode != STATUS_GOOD)
    return retcode;

 // generate the list of privileges granted to the object and store in class
  if (generateObjectRowList() == STATUS_ERROR)
    return STATUS_ERROR;
    
  // generate the list of privileges granted to object columns and store in class
  if (generateColumnRowList() == STATUS_ERROR)
    return STATUS_ERROR; 
    
  // get column and object privileges across all grantors 
  bool hasManagePrivileges;
  retcode = getUserPrivs(objectType, grantorID_, roleIDs, privsOfTheGrantor,
                         hasManagePrivileges);
  if (retcode != STATUS_GOOD)
    return retcode;

  // get roleIDs for the grantor
  retcode = getRoleIDsForUserID(grantorID_,roleIDs);
  if (retcode == STATUS_ERROR)
    return retcode;

  // If null, the grantor has no privileges
 if ( privsOfTheGrantor.isNull() )
  {
    std::string rolesWithPrivs;
    if (getRolesToCheck(grantorID_, roleIDs, objectType, rolesWithPrivs)== STATUS_GOOD)
    {
      if (rolesWithPrivs.size() > 0)
      {
        *pDiags_ << DgSqlCode (-CAT_PRIVILEGE_NOT_GRANTED)
                 << DgString0 (grantorName.c_str())
                 << DgString1 (rolesWithPrivs.c_str());
        return STATUS_ERROR;
      }
    }

    *pDiags_ << DgSqlCode (-CAT_PRIVILEGE_NOT_GRANTED)
             << DgString0 (grantorName.c_str());
    return STATUS_ERROR;
  }

  return STATUS_GOOD;
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

  int32_t lastObjectOwnerID = 0;
  std::vector<int32_t> roleIDs;
  
  // objectList contains the list of objects referencing the referenced table,
  // see if the requested privilege change causes an RI constraint to be invalid
  for (size_t i = 0; i < objectList.size(); i++)
  {
    ObjectReference *pObjectRef = objectList[i];
    PrivMgrDesc originalPrivs;
    PrivMgrDesc currentPrivs;
    
    pObjectRef->describe(traceMsg);
    log (__FILE__, traceMsg, i);
 
    // getRoleIDsForUserID does I/O to get information.  The referencing 
    // list is returned by object owner to avoid rereading information for 
    // the same user
    // At some time, we should cache user and role information
    if (lastObjectOwnerID != pObjectRef->objectOwner)
    {
      roleIDs.clear();
      retcode = getRoleIDsForUserID(pObjectRef->objectOwner,roleIDs);
      if (retcode == STATUS_ERROR)
        return retcode;
    }

    // get the summarized original and current privs for the referencing table 
    // current privs contains any adjustments due to the privilege change
    retcode = summarizeCurrentAndOriginalPrivs(objectUsage.objectUID,
                                               pObjectRef->objectOwner,
                                               grantorID_,
                                               roleIDs,
                                               listOfAffectedObjects,
                                               originalPrivs,
                                               currentPrivs);
    if (retcode != STATUS_GOOD)
      return retcode;

    PrivMgrCoreDesc thePrivs = currentPrivs.getTablePrivs();
    if (!thePrivs.getPriv(REFERENCES_PRIV))
    {
      log (__FILE__, "User does not have reference privilege on the object", -1);

      // no longer have REFERENCES privilege on the table, 
      // see if privileges are granted on all required columns
      std::vector<ColumnReference *> summarizedColRefs;
      summarizeColPrivs(*pObjectRef, 
                        pObjectRef->objectOwner, 
                        grantorID_,
                        roleIDs, 
                        listOfAffectedObjects, 
                        summarizedColRefs); 

      // check summarized privileges to see if still have priv through other privs
      std::vector<ColumnReference *> neededColRefs = *pObjectRef->columnReferences;
      for (size_t i = 0; i < neededColRefs.size(); i++)
      {

        // neededColRefs contains the list of all columns referenced by this object
        // summarizedColRefs are the current privileges with the privilege change
        //   incorporated.
        // if user still has necessary privilege through column privileges, then
        // revoke can proceed
        ColumnReference *neededColRef = neededColRefs[i];
        for (size_t j = 0; j < summarizedColRefs.size(); j++)
        {
          ColumnReference *existingRef = summarizedColRefs[j];
          traceMsg = "Checking if have references for col: ";
          traceMsg += to_string((long long int)existingRef->columnOrdinal);
          log (__FILE__, traceMsg, -1);
          if (existingRef->columnOrdinal == neededColRef->columnOrdinal)
          {
            PrivMgrCoreDesc colPrivs = existingRef->updatedPrivs;
            traceMsg = "References setting: ";
            traceMsg += (colPrivs.getPriv(REFERENCES_PRIV) ? "y" : "n");
            log (__FILE__, traceMsg, -1);
            if (!colPrivs.getPriv(REFERENCES_PRIV))
            {
              std::string referencingTable;
              if (!admin.getConstraintName(objectUsage.objectUID, 
                                          pObjectRef->objectUID, 
                                          neededColRef->columnOrdinal, referencingTable)) 
              {
                referencingTable = "UNKNOWN, Referencing table ID is ";
                referencingTable += UIDToString(pObjectRef->objectUID         );
              }

              *pDiags_ << DgSqlCode (-CAT_DEPENDENT_OBJECTS_EXIST)
                       << DgString0 (referencingTable.c_str());
              retcode = STATUS_ERROR;
            }
            break;
          }
        }
      }

      // remove list of summarized columns
      while (!summarizedColRefs.empty())
        delete summarizedColRefs.back(), summarizedColRefs.pop_back();
    }
  }

  return retcode;
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
    retcode = getRoleIDsForUserID(objectUsage.granteeID,roleIDs);
    if (retcode == STATUS_ERROR)
      return retcode;
    
    // if the grantee owns any udrs, get the summarized original and current
    // privs for the library
    // current privs contains any adjustments
    retcode = summarizeCurrentAndOriginalPrivs(objectUsage.objectUID,
                                               objectUsage.granteeID,
                                               grantorID_,
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
      ObjectReference *pObjectRef = objectList[0];
      *pDiags_ << DgSqlCode (-CAT_DEPENDENT_OBJECTS_EXIST)
               << DgString0 (pObjectRef->objectName.c_str());
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
  const int32_t grantorID,
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

  // for each entry in the viewUsages list calculate the changed privileges
  // if privileges change, add viewUsage to listOfAffectedObjects 
  for (size_t i = 0; i < viewUsages.size(); i++)
  {
    ViewUsage viewUsage = viewUsages[i];

    viewUsage.describe(traceMsg);
    log (__FILE__, traceMsg, i);

    // gatherViewPrivileges recreates privileges for the view based on changes
    // requested by the current grant/revoke request
    // Updated priv descriptors are stored in the viewUsage structure.
    retcode = gatherViewPrivileges(viewUsage, command, grantorID, listOfAffectedObjects);
    traceMsg = "gathered view privs: retcode is ";
    traceMsg += privStatusEnumToLit(retcode);
    log (__FILE__, traceMsg, -1);

    if (retcode != STATUS_GOOD && retcode != STATUS_WARNING)
      return retcode;

    // check to see if privileges changed
    if (viewUsage.originalPrivs == viewUsage.updatedPrivs)
      continue;
    else
    {
      // This view privs have been changed by the grant/revoke request
      // However, only INSERT, DELETE, and UPDATE privs can be propagated
      // If the view is not updatable or insertable then, we are done with
      // this viewUsage
      if (viewUsage.isUpdatable || viewUsage.isInsertable)
      {
        // Add viewUsage to list of affected objects
        ObjectUsage *pUsage = new (ObjectUsage);
        pUsage->objectUID = viewUsage.viewUID;
        pUsage->granteeID = viewUsage.viewOwner;
        pUsage->grantorIsSystem = true;
        pUsage->objectName = viewUsage.viewName;
        pUsage->objectType = COM_VIEW_OBJECT;
        pUsage->originalPrivs = viewUsage.originalPrivs;
        pUsage->updatedPrivs = viewUsage.updatedPrivs;
        listOfAffectedObjects.push_back(pUsage);

        traceMsg = "adding new objectUsage for ";
        pUsage->describe(traceMsg);
        log (__FILE__, traceMsg, i);

#if 0
        // When cascade is supported, the list of down stream views must be 
        // included in the list of affected objects.
        // Also, need to understand ANSI SQL rules for propagating view privs,
        // that is, should down stream views be updated if it parent view
        // privs are changed  and the parent has WGO specified.

        // get list of grantee ID's that have been granted privileges on the
        // current view.  
        std::set<int32_t> granteeList;
        if (getGranteesForViewUsage(viewUsage, granteeList) == STATUS_ERROR)
        {
          PRIVMGR_INTERNAL_ERROR("Error while getting grantees for down stream views");
          return STATUS_ERROR;
        }

        // Call dealWithViews to see if the down stream views should be adjusted.
        for (std::set<int32_t>::iterator it = granteeList.begin(); it!= granteeList.end(); ++it)
        {
          pUsage->granteeID = *it;
          retcode = dealWithViews(*pUsage, command, viewUsage.viewOwner, listOfAffectedObjects);
          if (retcode != STATUS_GOOD && retcode != STATUS_WARNING)
            return retcode;
        }
        pUsage->granteeID = viewUsage.viewOwner;
#endif
      } // updatable views 
    } // view privs changed
  } // list of view usages
  
  return STATUS_GOOD;
}

// ----------------------------------------------------------------------------
// method: gatherViewColUsages
//
// This method gathers the view-col <=> referenced-col usages and creates a
// list of ComViewColUsage's for the referenced object. Each ComViewColUsage 
// is identified the view column number, and the related UID and column number 
// of the referenced object.
//
// parameters:
//   objectRef - description of the referenced object 
//   viewUsage - description of the view
//   viewColUsages - list of view column, referenced column usages
//
// Returns: PrivStatus                                               
//                                                                    
// STATUS_GOOD: Operation successful
//           *: Unable to gather usages, see diags.     
//                                                                 
// ----------------------------------------------------------------------------
PrivStatus PrivMgrPrivileges::gatherViewColUsages(
  ObjectReference *objectRef,
  ViewUsage &viewUsage,
  std::vector<ComViewColUsage> &viewColUsages)
{
  std::string traceMsg;
  PrivStatus retcode;

  PrivMgrMDAdmin admin(trafMetadataLocation_, metadataLocation_, pDiags_);

  // Get columns for referenced object if they are  not already present
  if (objectRef->columnReferences == NULL)
  {
    retcode = admin.getColumnReferences(objectRef);
    if (objectRef->columnReferences)
    {
      traceMsg += "getting column references: number references found ";
      traceMsg += to_string((long long int)objectRef->columnReferences->size());
      traceMsg += ", ";
    }
    traceMsg += "retcode is ";
    traceMsg += privStatusEnumToLit(retcode);
    log (__FILE__, traceMsg, -1);
    if (retcode == STATUS_ERROR)
      return retcode;
  }

  // Extract view <=> object column relationship from the TEXT table 
  // and add it to the viewUsage
  if (viewUsage.viewColUsagesStr.empty())
  {
    retcode = admin.getViewColUsages(viewUsage);
    traceMsg = "getting view column usages";
    traceMsg += ", retcode is ";
    traceMsg += privStatusEnumToLit(retcode);
    log (__FILE__, traceMsg, -1);
    if (retcode != STATUS_GOOD)
      return retcode;
  }

  // Setup the view column to referenced column relationship
  char * beginStr  = (char *)viewUsage.viewColUsagesStr.c_str();
  char * endStr = strchr(beginStr, ';');
  while (endStr != NULL)
  {
    ComViewColUsage colUsage;
    std::string currentUsage(beginStr, endStr - beginStr + 1);
    colUsage.unpackUsage (currentUsage.c_str());
    viewColUsages.push_back(colUsage);
    beginStr = endStr+1;
    endStr = strchr(beginStr, ';');
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
  const PrivCommand command,
  const int32_t grantorID,
  const std::vector<ObjectUsage *> listOfAffectedObjects)
{
  PrivStatus retcode = STATUS_GOOD;
  std::string traceMsg;

  // initialize summarized descriptors and set all applicable privileges
  // TBD:  if view is not updatable, should initialize correctly.
  // views have same privileges as tables
  bool setWGOtrue = true;
  PrivMgrDesc summarizedOriginalPrivs;
  summarizedOriginalPrivs.setAllTableGrantPrivileges(true/*priv*/, setWGOtrue);
  PrivMgrDesc summarizedCurrentPrivs;
  summarizedCurrentPrivs.setAllTableGrantPrivileges(true/*priv*/, setWGOtrue);

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
  retcode = getRoleIDsForUserID(viewUsage.viewOwner,roleIDs);
  if (retcode == STATUS_ERROR)
    return retcode;
 
  // First gather privileges on the object
  for (size_t i = 0; i < objectList.size(); i++)
  {
    ObjectReference *pObjectRef = objectList[i];
    pObjectRef->describe(traceMsg);
    log (__FILE__, traceMsg, i);

    // get the summarized original and current privs for the 
    // referenced object that have been granted to the view owner
    // listOfAffectedObjects contain the privilege adjustments needed
    //   to generate the current privs
    retcode = summarizeCurrentAndOriginalPrivs(pObjectRef->objectUID,
                                               viewUsage.viewOwner, 
                                               grantorID,
                                               roleIDs,
                                               listOfAffectedObjects,
                                               originalPrivs,
                                               currentPrivs);
    if (retcode != STATUS_GOOD)
      return retcode;

    // If the referenced object is a sequence generator, then the grantee
    // must still have the USAGE_PRIV.  The USAGE_PRIV is not a column
    // level privilege so no column level check is necessary.
    if (isRevokeCommand(command))
    {
      if (pObjectRef->objectType == COM_SEQUENCE_GENERATOR_OBJECT)
      {
        PrivMgrCoreDesc thePrivs = currentPrivs.getTablePrivs();
        if (!thePrivs.getPriv(USAGE_PRIV))
        {
           *pDiags_ << DgSqlCode (-CAT_DEPENDENT_OBJECTS_EXIST)
                    << DgString0 (viewUsage.viewName.c_str());
           return STATUS_ERROR;
        }
      }
    }

    // "and" the returned privilege to the summarized privileges
    // for all objects
    summarizedOriginalPrivs.intersectionOfPrivs(originalPrivs);
    summarizedCurrentPrivs.intersectionOfPrivs(currentPrivs);

    // reset to prepare for next object
    originalPrivs.resetTablePrivs();
    currentPrivs.resetTablePrivs();
  }

  // If view has all grantable col privs already, no need to check further
  bool checkCols = false;
  PrivMgrCoreDesc thePrivs = summarizedCurrentPrivs.getTablePrivs();
  for (size_t i = FIRST_DML_COL_PRIV; i <= LAST_DML_COL_PRIV; i++ )
  {
    PrivType privType = PrivType(i);
    if (thePrivs.getPriv(privType) && thePrivs.getWgo(privType))
      continue;
    checkCols = true;
    break;
  }

  // No need to check col privs, return with summarized privs
  if (!checkCols)
  {
    // Update view usage with summarized privileges
    viewUsage.originalPrivs = summarizedOriginalPrivs;
    viewUsage.updatedPrivs = summarizedCurrentPrivs;
    return STATUS_GOOD;
  }

  // Turn on bits to prepare for intersecting with object privileges
  originalPrivs.setAllTableGrantPrivileges(true/*priv*/, setWGOtrue);
  currentPrivs.setAllTableGrantPrivileges(true/*priv*/, setWGOtrue);

  std::vector<ColumnReference *> summarizedColRefs;

  // Gather column privileges on all objects referencing the view.
  //   for grants, a view might gain privs 
  //   for revokes, a view might lose privs
  // For example:
  //   as user1:
  //     create table sch1.t1 (c1 int, c2 int, ....);
  //     grant select on sch1.t1 to user2;
  //     grant insert (c1) on sch1.t1 to user2
  //   as user2:
  //     create view sch2.v1 as select c1 from sch1.t1;
  //   view sch2.v1 gains the insert privilege since all columns that 
  //     sch2.v1 references have granted user2 the insert privilege.
  for (size_t i = 0; i < objectList.size(); i++)
  {
    ObjectReference *pObjectRef = objectList[i];

    // gather the view-col <=> referenced-col usages
    std::vector <ComViewColUsage> viewColUsages;
    retcode = gatherViewColUsages(pObjectRef, viewUsage, viewColUsages);
    if (retcode == STATUS_ERROR)
      return retcode;

    // The view-col <=> referenced_col relationship is not found
    // No need to proceed further.
    // If this is a grant of a column privilege, return an error
    // User needs to drop and recreate view
    // The view-col <=> referenced-col usages are only being started in 
    // metadata after column privileges are officially supported.  Any views
    // create before this time, do not have this information, so column privs
    // either do not exist (revoke) or cannot be added (grant).  The view needs
    // to be dropped and recreated (or add a fixup procedure).
    if (retcode == STATUS_NOTFOUND)
    {
      if (command == GRANT_COLUMN) 
      {
         *pDiags_ << DgSqlCode (-CAT_COLUMN_PRIVILEGE_NOT_ALLOWED)
                  << DgTableName (viewUsage.viewName.c_str());
         return STATUS_ERROR;
       }
       return STATUS_GOOD;
    }

    
    // gather the updated summarized column privileges on the referenced object
    summarizeColPrivs(*pObjectRef, 
                      viewUsage.viewOwner, 
                      grantorID,
                      roleIDs, 
                      listOfAffectedObjects, 
                      summarizedColRefs); 

    // For each view column, get the corresponding referenced column priv
    for (size_t j = 0; j < viewColUsages.size(); j++)
    {
      ComViewColUsage colUsage = viewColUsages[j];

      // See if the referenced object contains the associated view column
      if (colUsage.getRefdUID() != pObjectRef->objectUID) 
        continue;

      // Get the referenced column, if NULL then something is wrong
      ColumnReference *neededColRef = pObjectRef->find(colUsage.getRefdColNumber());
      if (neededColRef == NULL)
      {
         PRIVMGR_INTERNAL_ERROR("View column to referenced column relationship wrong");
         return STATUS_ERROR;
      }

      // Find and update the current bitmaps for column
      for (size_t k = 0; k < summarizedColRefs.size(); k++)
      {
        ColumnReference *existingRef = summarizedColRefs[k];
        if (existingRef->columnOrdinal == neededColRef->columnOrdinal)
        {
          originalPrivs.intersectionOfPrivs(existingRef->originalPrivs);
          currentPrivs.intersectionOfPrivs(existingRef->updatedPrivs);
          break;
        }
      }
    }
  }

  // Union or "or" the returned privileges to the summarized privileges
  // for all columns. The privilege is set in currentPrivs only if all
  // columns in the view have the privilege.
  summarizedOriginalPrivs.unionOfPrivs(originalPrivs);
  summarizedCurrentPrivs.unionOfPrivs(currentPrivs);

  // If user no longer has select privilege on referenced object
  // returns an error
  // When cascade is supported, then referenced objects will be removed
  if (isRevokeCommand(command))
  {
    if (!summarizedCurrentPrivs.getTablePrivs().getPriv(SELECT_PRIV))
    {
      *pDiags_ << DgSqlCode (-CAT_DEPENDENT_OBJECTS_EXIST)
               << DgString0 (viewUsage.viewName.c_str());
      return STATUS_ERROR;
    }
  }

  // If view is not updatable or insertable, turn off privs in bitmaps
  if (!viewUsage.isUpdatable)
    {
      summarizedCurrentPrivs.getTablePrivs().setPriv(UPDATE_PRIV,false);
      summarizedCurrentPrivs.getTablePrivs().setPriv(UPDATE_PRIV, false);
      summarizedCurrentPrivs.getTablePrivs().setPriv(DELETE_PRIV,false);
      summarizedCurrentPrivs.getTablePrivs().setPriv(DELETE_PRIV, false);
    }

  if (!viewUsage.isInsertable)
    {
      summarizedCurrentPrivs.getTablePrivs().setPriv(INSERT_PRIV,false);
      summarizedCurrentPrivs.getTablePrivs().setPriv(INSERT_PRIV, false);
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
// Returns:
//   STATUS_GOOD     - list of rows was generated
//   STATUS_NOTFOUND - no column privileges were found
//   STATUS_ERROR    - the diags area is populated with any errors
// ****************************************************************************
PrivStatus PrivMgrPrivileges::generateColumnRowList()
{
  // If columnRowList_ already allocated, just return
  if (columnRowList_.size() > 0)
    return STATUS_GOOD;
  PrivStatus privStatus = getColumnRowList(objectUID_, columnRowList_);
  if (privStatus == STATUS_ERROR)
    return privStatus;
  return STATUS_GOOD;
}
 
// ****************************************************************************
// method: getColumnRowList
//
// generate the list of privileges granted to the passed in objectUID and 
// returns in the columnRowList parameter
//
// The list is ordered by grantor/grantee/column_number
//
// Returns:
//   STATUS_GOOD     - list of rows was generated
//   STATUS_NOTFOUND - in most cases, there should be at least one row in the
//                     OBJECT_PRIVILEGES table.  But there is at least one case
//                     where this is not true - trying to get privilege info
//                     for indexes when using the table (index_table) syntax.
//   STATUS_ERROR    - the diags area is populated with any errors
// ****************************************************************************

PrivStatus PrivMgrPrivileges::getColumnRowList(
   const int64_t objectUID,
   std::vector<PrivMgrMDRow *> &columnRowList)
{
  std::string whereClause ("where object_uid = ");
  whereClause += UIDToString(objectUID);
  std::string orderByClause (" order by grantor_id, grantee_id, column_number ");

  ColumnPrivsMDTable columnPrivsTable(columnTableName_,pDiags_);
  PrivStatus privStatus = 
   columnPrivsTable.selectWhere(whereClause, orderByClause, columnRowList);

  std::string traceMsg ("getting column privileges, number privileges is ");
  traceMsg += to_string((long long int)columnRowList.size());
  log (__FILE__, traceMsg, -1);
  for (size_t i = 0; i < columnRowList.size(); i++)
  {
    ColumnPrivsMDRow privRow = static_cast<ColumnPrivsMDRow &> (*columnRowList[i]);
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
  // If objectRowList_ already allocated, just return
  if (objectRowList_.size() > 0)
    return STATUS_GOOD;
  PrivStatus privStatus = getObjectRowList(objectUID_, objectRowList_);
  if (privStatus == STATUS_ERROR)
    return privStatus;
  return STATUS_GOOD;
}

// ****************************************************************************
// method: getObjectRowList
//
// generate the list of privileges granted to the passed in objectUID and 
// returns in the objectRowList parameter
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
PrivStatus PrivMgrPrivileges::getObjectRowList(
   const int64_t objectUID, 
   std::vector<PrivMgrMDRow *> &objectRowList)
{ 
  std::string whereClause ("where object_uid = ");
  whereClause += UIDToString(objectUID);
  std::string orderByClause(" order by grantor_id, grantee_id ");

  ObjectPrivsMDTable objectPrivsTable(objectTableName_,pDiags_);
  PrivStatus privStatus = 
    objectPrivsTable.selectWhere(whereClause, orderByClause, objectRowList);
  if (privStatus == STATUS_ERROR)
    return privStatus;

  std::string traceMsg ("getting object privileges, number privleges is ");
  traceMsg += to_string((long long int)objectRowList.size());
  log (__FILE__, traceMsg, -1);
  for (size_t i = 0; i < objectRowList.size(); i++)
  {
    ObjectPrivsMDRow privRow = static_cast<ObjectPrivsMDRow &> (*objectRowList[i]);
    privRow.describeRow(traceMsg);
    log (__FILE__, traceMsg, i);
  }
  return STATUS_GOOD;
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
  pUsage->granteeID = objectUsage.granteeID;
  pUsage->grantorIsSystem = objectUsage.grantorIsSystem;
  pUsage->objectName = objectUsage.objectName;
  pUsage->objectType = objectUsage.objectType;
  pUsage->originalPrivs = objectUsage.originalPrivs;
  pUsage->updatedPrivs = objectUsage.updatedPrivs;
  pUsage->copyColumnReferences(objectUsage.columnReferences);

  listOfAffectedObjects.push_back(pUsage); 


  // Find list of affected constraints
  if (isRevokeCommand(command))
  {
    // TBD optimization: if no "references" privilege has been revoked, skip
    retcode = dealWithConstraints (objectUsage, listOfAffectedObjects);
    if (retcode == STATUS_ERROR)
     return retcode;

    // TBD optimization: if no "execute" privilege has been revoked, skip
    retcode = dealWithUdrs (objectUsage, listOfAffectedObjects);
    if (retcode != STATUS_GOOD && retcode != STATUS_WARNING)
     return retcode;

  }

  // Find list of affected views
  retcode = dealWithViews (objectUsage, command, grantorID_, listOfAffectedObjects);
  if (retcode != STATUS_GOOD && retcode != STATUS_WARNING)
    return retcode;

  return STATUS_GOOD;
}

// ----------------------------------------------------------------------------
// method: getGrantedPrivs
//
// This method searches the list of object privs to get information for the
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
// method: getGranteesForViewUsage
//
// returns the list of grantees that have been granted privileges by the view
// owner
//
// input:  viewUsage
// output: a unique list of grantees
//
//  Returns: PrivStatus                                               
//                                                                   
//   STATUS_GOOD: list was generated
//  STATUS_ERROR: unexpected error occurred
// ----------------------------------------------------------------------------
PrivStatus PrivMgrPrivileges::getGranteesForViewUsage (
  const ViewUsage &viewUsage,
  std::set<int32_t> &granteeList)
{
  PrivStatus privStatus = STATUS_GOOD;

  std::string whereClause ("where object_uid = ");
  whereClause += UIDToString(viewUsage.viewUID);
  whereClause += " and grantor_id <> -2 ";
  std::string orderByClause("order by grantee_id");
  std::vector<PrivMgrMDRow *> rowList;
  
  // get list of grantees from object_privileges
  ObjectPrivsMDTable objectPrivsTable(objectTableName_,pDiags_);
  privStatus = objectPrivsTable.selectWhere(whereClause,orderByClause,rowList);
  if (privStatus == STATUS_ERROR)
  {
    deleteRowList(rowList);
    return privStatus;
  }

  for (size_t i = 0; i < rowList.size(); i++)
  {
    ObjectPrivsMDRow &currentRow = static_cast<ObjectPrivsMDRow &> (*rowList[i]);
    granteeList.insert(currentRow.granteeID_);
  }
  deleteRowList(rowList);
    
  // get list of grantees for column_privileges
  ColumnPrivsMDTable columnPrivsTable(columnTableName_,pDiags_);
  privStatus = columnPrivsTable.selectWhere(whereClause, orderByClause, rowList);
  if (privStatus == STATUS_ERROR)
  {
    deleteRowList(rowList);
    return privStatus;
  }

  for (size_t i = 0; i < rowList.size(); i++)
  {
    ColumnPrivsMDRow &currentRow = static_cast<ColumnPrivsMDRow &> (*rowList[i]);
    granteeList.insert(currentRow.granteeID_);
  }
  deleteRowList(rowList);
  return STATUS_GOOD;
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
                                             MAX_USERNAME_LEN+1,actualLen);
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
// * Method: revokeColumnPriv                                
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
PrivStatus PrivMgrPrivileges::revokeColumnPriv(
   const ComObjectType objectType,
   const int32_t granteeID,
   const std::string & granteeName,
   const std::string & grantorName,
   const PrivMgrDesc & privsToRevoke,
   const bool isWGOSpecified)
{

  PrivStatus privStatus = STATUS_GOOD;

  log (__FILE__, "checking column privileges", -1);

  NAList<PrivMgrCoreDesc> colPrivsToRevoke = privsToRevoke.getColumnPrivs();

  // Get existing column grants from grantor to the specified grantee.
  NAList<PrivMgrCoreDesc> grantedColPrivs(STMTHEAP);
  getColRowsForGranteeGrantor(columnRowList_,
                              granteeID,grantorID_,
                              grantedColPrivs);

   // set up the object usage 
   ObjectUsage objectUsage;
   objectUsage.objectUID = objectUID_;
   objectUsage.granteeID = granteeID;
   objectUsage.objectName = objectName_;
   objectUsage.objectType = objectType;

   // Create list of ColumnReferences
   objectUsage.columnReferences = new std::vector<ColumnReference *>;
   for (size_t i = 0; i < colPrivsToRevoke.entries(); i++)
   {
      PrivMgrCoreDesc &colPrivToRevoke = colPrivsToRevoke[i];
      PrivMgrCoreDesc *grantedColPriv = findColumnEntry(grantedColPrivs, colPrivToRevoke.getColumnOrdinal());
      if (grantedColPriv)
      {
        if (colPrivToRevoke.anyNotSet(*grantedColPriv))
        {
          // sanity check -> verify that privileges to revoke actually are set
          // in the granted list 
          for (size_t p = FIRST_DML_COL_PRIV; p <= LAST_DML_COL_PRIV; p++ )
          {
            PrivType type = (PrivType)p;

            // If trying to revoke a privilege that is not granted or 
            // if trying to revoke grant option that is not granted, report it
            //if ((colPrivToRevoke.getPriv(type) && !grantedColPriv->getPriv(type)) ||
            //    (!colPrivToRevoke.getPriv(type) && colPrivToRevoke.getWgo(type) && !grantedColPriv->getWgo(type)))
            bool printWarning = false;
            bool printWgo = false;
            if (colPrivToRevoke.getPriv(type))
            {
              if ( !grantedColPriv->getPriv(type))
                printWarning = true;
            }
            else
            {
              if (colPrivToRevoke.getWgo(type) && !grantedColPriv->getWgo(type))
              {
                printWarning = true;
                printWgo = true;
              }
            }

            if (printWarning)
            {
              char buf[1000];
              sprintf(buf, "%s %s (columm number %d) on %s",
                            PrivMgrUserPrivs::convertPrivTypeToLiteral(type).c_str(),
                            (colPrivToRevoke.getWgo(type)) ? "WITH GRANT OPTION" : "",
                            colPrivToRevoke.getColumnOrdinal(), objectName_.c_str());
              *pDiags_ << DgSqlCode(CAT_GRANT_NOT_FOUND)
                       << DgString0(buf)
                       << DgString1(grantorName.c_str())
                       << DgString2(granteeName.c_str());
            }
          }
        }
        ColumnReference *adjustedCol = new ColumnReference;
        adjustedCol->columnOrdinal = colPrivToRevoke.getColumnOrdinal();
        adjustedCol->originalPrivs = *grantedColPriv;     
        PrivMgrCoreDesc adjustedPrivs = *grantedColPriv;
        adjustedPrivs.AndNot(colPrivToRevoke);
        adjustedCol->updatedPrivs = adjustedPrivs;
        objectUsage.columnReferences->push_back(adjustedCol);
      }
      else
      {
        // report errors for any missing grants
        for (size_t p = FIRST_DML_COL_PRIV; p <= LAST_DML_COL_PRIV; p++ )
        {
          // If trying to revoke a privilege that is not granted or 
          // if trying to revoke grant option that is not granted, report it
          if (colPrivToRevoke.getPriv((PrivType)p) || colPrivToRevoke.getWgo((PrivType)p))
          {
            char buf[1000];
            sprintf(buf, "%s %s (columm number %d) on %s",
                          PrivMgrUserPrivs::convertPrivTypeToLiteral((PrivType)p).c_str(),
                          (colPrivToRevoke.getWgo((PrivType)p)) ? "WITH GRANT OPTION" : "",
                          colPrivToRevoke.getColumnOrdinal(), objectName_.c_str());
            *pDiags_ << DgSqlCode(CAT_GRANT_NOT_FOUND)
                     << DgString0(buf)
                     << DgString1(grantorName.c_str())
                     << DgString2(granteeName.c_str());
          }
        }
      }
   }

   if (objectUsage.columnReferences->size() == 0)
     return STATUS_GOOD;

   // Get privileges on the object for grantor/grantee
   ObjectPrivsMDRow objRow;
   privStatus = getGrantedPrivs(granteeID, objRow);

   // getGrantedPrivs returns STATUS_GOOD or STATUS_NOTFOUND
   // if STATUS_NOTFOUND, bitmaps are empty (as initialized above)
   // if STATUS_GOOD, set current bitmaps
   if (privStatus == STATUS_GOOD)
   {
     PrivMgrCoreDesc coreDesc(objRow.privsBitmap_, objRow.grantableBitmap_);
     objectUsage.originalPrivs.setTablePrivs(coreDesc);
     objectUsage.updatedPrivs.setTablePrivs(coreDesc); 
   }
   
   // check to see if can revoke if there are referenced items when
   // revoke cascade is supported, this returns the list of referenced
   // items that need to change. 
   if ( updateDependentObjects(objectUsage, PrivCommand::REVOKE_COLUMN_RESTRICT) == STATUS_ERROR)
     return STATUS_ERROR;
   
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


   if (checkColumnRevokeRestrict (granteeID, colPrivsToRevoke, columnRowList_))
     return STATUS_ERROR;

   bool rowRevoked = false;
   PrivColumnBitmap revokedPrivs;

   std::string whereBase(" WHERE object_uid = ");

   whereBase += UIDToString(objectUID_);
   whereBase += " AND grantor_id = ";
   whereBase += authIDToString(grantorID_);
   whereBase += " AND grantee_id = ";
   whereBase += authIDToString(granteeID);
   whereBase += " AND column_number = ";
    
   ColumnPrivsMDTable columnPrivsTable(columnTableName_,pDiags_);

   for (size_t i = 0; i < colPrivsToRevoke.entries(); i++)
   {
      PrivMgrCoreDesc &colPrivToRevoke = colPrivsToRevoke[i];
      bool updateRow = false;
      bool deleteRow = false;

      // Look for any existing granted privileges on the column for which
      // privileges are to be granted.
      PrivMgrCoreDesc *grantedColPriv = findColumnEntry(grantedColPrivs, colPrivToRevoke.getColumnOrdinal());
      if (grantedColPriv)
      {
         // Found row with grant for this column.
         
         // Verify privilege(s) being revoked was/were granted.  If not, continue
         if (!isWGOSpecified && 
             ((colPrivToRevoke.getPrivBitmap() & grantedColPriv->getPrivBitmap()) == 0))
            continue;
         
         // If all privileges are revoked, delete corresponding row
         if (!isWGOSpecified && 
             (colPrivToRevoke.getPrivBitmap() == grantedColPriv->getPrivBitmap()))
           deleteRow = true;
         else
           updateRow = true;

         // generate the final bitmaps to store in metadata
         // removing any privileges that already have been revoked
         PrivMgrCoreDesc adjustedPrivs = *grantedColPriv;
         adjustedPrivs.AndNot(colPrivToRevoke);

         // If only removing WGO, then the privsBitmap does not change
         // Not sure if this is needed ??
         if (isWGOSpecified)
           adjustedPrivs.setPrivBitmap(grantedColPriv->getPrivBitmap());

         // set adjusted privileges
         colPrivToRevoke.setPrivBitmap(adjustedPrivs.getPrivBitmap());
         colPrivToRevoke.setWgoBitmap(adjustedPrivs.getWgoBitmap());

         // Using the list of privs to revoke, change so adjustedPrivs contains
         // Some privileges may have been requested to revoke that aren't
         // currently granted - flip adjusted bits to final list of privs
         revokedPrivs |= adjustedPrivs.getPrivBitmap(); 
      }
      else
         // if column not exist, just continue (ComDiags contains a warning)
         continue;

      
      if (deleteRow)
      {
         std::string whereClause(whereBase + authIDToString(colPrivToRevoke.getColumnOrdinal()));
      
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
      row.privsBitmap_ = colPrivToRevoke.getPrivBitmap();
      row.grantableBitmap_ = colPrivToRevoke.getWgoBitmap();
      row.columnOrdinal_ = colPrivToRevoke.getColumnOrdinal();

      privStatus = columnPrivsTable.updateColumnRow(row,whereBase);
         
      if (privStatus == STATUS_ERROR)
         return privStatus;
      
      rowRevoked = true;
   } 
   
   // if (!rowRevoked)
   //   Warning
      
   return STATUS_GOOD;

}
//************* End of PrivMgrPrivileges::revokeColumnPriv ***************


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

  PrivMgrDesc privsToRevoke(granteeID);
  PrivMgrDesc privsOfTheGrantor(grantorID_);
  std::vector<int_32> roleIDs;
  retcode = initGrantRevoke(objectType, granteeID, grantorName,
                            privsList, colPrivsArray,
                            isAllSpecified, isGOFSpecified, false,
                            privsToRevoke, privsOfTheGrantor, roleIDs);
  if (retcode != STATUS_GOOD)
    return retcode;

  // Remove any privsToRevoke which are not held grantable by the Grantor.
  // If limitToGrantable returns true, some privs are not revokable.
  bool warnNotAll = false;
  PrivMgrDesc origPrivsToRevoke = privsToRevoke;
  if ( privsToRevoke.limitToGrantable( privsOfTheGrantor ) )
  {
     // This is ok.  Can specify ALL without having all privileges set.
     if (!isAllSpecified )
       warnNotAll = true;  // Not all the specified privs can be revoked
  }

  // If nothing left to revoke, we are done.
  if ( privsToRevoke.isNull() )
  {
    std::string rolesWithPrivs;
    if (getRolesToCheck(grantorID_, roleIDs, objectType, rolesWithPrivs)== STATUS_GOOD)
    {
      if (rolesWithPrivs.size() > 0)
      {
        *pDiags_ << DgSqlCode (-CAT_PRIVILEGE_NOT_GRANTED)
                 << DgString0 (grantorName.c_str())
                 << DgString1 (rolesWithPrivs.c_str());
        return STATUS_ERROR;
      }
    }

    *pDiags_ << DgSqlCode (-CAT_PRIVILEGE_NOT_GRANTED)
             << DgString0 (grantorName.c_str());
    return STATUS_ERROR;
  }

  // revoke any column level privileges
  if ( !privsToRevoke.isColumnLevelNull() )
  {
    retcode = revokeColumnPriv(objectType,granteeID,granteeName,
                               grantorName,privsToRevoke,isGOFSpecified);
    if (retcode != STATUS_GOOD)
      return retcode;
  }
    
  // If only column-level privileges were specified, no problem.  
  if (privsToRevoke.getTablePrivs().isNull())
  {
    // Send a message to the Trafodion RMS process about the revoke operation.
    // RMS will contact all master executors and ask that cached privilege 
    // information be re-calculated
    retcode = sendSecurityKeysToRMS(granteeID, privsToRevoke);

    // report any privs not granted
    if (warnNotAll)
      reportPrivWarnings(origPrivsToRevoke,
                         privsToRevoke,
                         CAT_NOT_ALL_PRIVILEGES_REVOKED);
    log (__FILE__, "****** REVOKE operation succeeded ******", -1);
    return STATUS_GOOD;
  }

  // See if grantor has previously granted privileges to the grantee
  ObjectPrivsMDRow row;
  retcode = getGrantedPrivs(granteeID, row);
  if (retcode == STATUS_ERROR)
    return retcode;

  if (retcode == STATUS_NOTFOUND)
  {
    // Set up parameters for the error message: privileges, grantor, & grantee
    // privilege list
    std::string privListStr;
    for (size_t i = 0; i < privsList.size(); i++)
      privListStr += PrivMgrUserPrivs::convertPrivTypeToLiteral(privsList[i]) + ", ";
    
    privListStr.erase(privListStr.length()-2, privListStr.length());
    if (isGOFSpecified)
      privListStr += " WITH GRANT OPTION";

    *pDiags_ << DgSqlCode (CAT_GRANT_NOT_FOUND)
             << DgString0 (privListStr.c_str())
             << DgString1 (grantorName.c_str())
             <<DgString2 (granteeName.c_str());
  
    return STATUS_WARNING;
  }

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
  objectUsage.granteeID = granteeID;
  objectUsage.grantorIsSystem = false;
  objectUsage.objectName = row.objectName_;
  objectUsage.objectType = row.objectType_;
  objectUsage.columnReferences = NULL;

  PrivMgrDesc originalPrivs (row.granteeID_);
  originalPrivs.setTablePrivs(savedOriginalPrivs);
  objectUsage.originalPrivs = originalPrivs;
  objectUsage.updatedPrivs = privsToRevoke;

  if (updateDependentObjects(objectUsage, PrivCommand::REVOKE_OBJECT_RESTRICT) == STATUS_ERROR)
    return STATUS_ERROR;

  // Go rebuild the privilege tree to see if it is broken
  // If it is broken, return an error
  if (checkRevokeRestrict (row, objectRowList_))
    return STATUS_ERROR;

  ObjectPrivsMDTable objectPrivsTable (objectTableName_, pDiags_);
  if (privsToRevoke.getTablePrivs().isNull())
  {
    row.describeRow(traceMsg);
    traceMsg.insert(0, "deleting privilege row ");
    log (__FILE__, traceMsg, -1);

    // delete the row
    retcode = objectPrivsTable.deleteRow(row);
  }
  else
  {
    row.describeRow(traceMsg);
    traceMsg.insert(0, "updating existing privilege row ");
    log (__FILE__, traceMsg, -1);

    // update the row
    retcode = objectPrivsTable.updateRow(row);
  }

  // Send a message to the Trafodion RMS process about the revoke operation.
  // RMS will contact all master executors and ask that cached privilege 
  // information be re-calculated
  retcode = sendSecurityKeysToRMS(granteeID, listOfRevokedPrivileges);

  // SQL Ansi states that privileges that can be revoked should be done so
  // even if some requested privilege are not revokable.
  // TDB:  report which privileges were not revoked
  if (warnNotAll)
      reportPrivWarnings(origPrivsToRevoke,
                         privsToRevoke,
                         CAT_NOT_ALL_PRIVILEGES_REVOKED);

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
  ColumnPrivsMDTable columnPrivsTable (objectTableName_, pDiags_);
  retcode = columnPrivsTable.deleteWhere(whereClause);

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
                    int32_t granteeAsGrantor;
                    if (isRoleID(thisGrantee))
                    {
                      std::vector<int32_t> roleIDs;
                      std::vector<int32_t> userIDs;
                      roleIDs.push_back(thisGrantee);
                      if (getGranteeIDsForRoleIDs(roleIDs,userIDs) == STATUS_ERROR)
                        return;

                      for (size_t j = 0; j < userIDs.size(); j++)
                      {
                         granteeAsGrantor = userIDs[j];
                         scanObjectBranch( pType, // Scan for this grantee as grantor.
                                           granteeAsGrantor,
                                           privsList );
                      }
                    }
                    granteeAsGrantor = thisGrantee;
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

// ----------------------------------------------------------------------------
// method: checkColumnRevokeRestrict
//
// This method starts at the beginning of the privilege tree and rebuilds
// it from top to bottom.  If the revoke causes part of the tree to be 
// unaccessible (a broken branch), it returns true; otherwise, revoke can 
// proceed - returns false.
//
// Params:
//     granteeID - the target AuthID
//     colPrivsToRevoke - the list of column entries containing proposed 
//                        changes from the requested revoke statement.
//     rowList - a list of all the rows associated with the object
//
//  true - unable to perform revoke because of dependencies
//  false - able to perform revoke.privileges
//
// The diags area is set up with where the tree was broken
// ---------------------------------------------------------------------------- 
bool PrivMgrPrivileges::checkColumnRevokeRestrict (
  int32_t granteeID,
  const NAList<PrivMgrCoreDesc> &colPrivsToRevoke,
  std::vector <PrivMgrMDRow *> &rowList )
{
  std::string traceMsg;
  log (__FILE__, "checking column grant tree for broken branches", -1);

  // Clear visited_ bitmaps and set current_ bitmaps to current priv values
  // Search the list of privileges associated with the object and turn off 
  // the bitmaps in current_ that are no longer available when the revoke 
  // completes - based on colPrivsToRevoke. 
  for (int32_t i = 0; i < rowList.size(); i++)
  {
    ColumnPrivsMDRow &currentRow = static_cast<ColumnPrivsMDRow &> (*rowList[i]);
    currentRow.setCurrentToOriginal();
    currentRow.clearVisited();

    // only look at rows for the current grantor and grantee
    if (currentRow.grantorID_ == grantorID_ &&
        currentRow.granteeID_ == granteeID)
    {
      // Adjust rows that have had their privileges updated
      for (int32_t j = 0; j < colPrivsToRevoke.entries(); j++)
      {
        PrivMgrCoreDesc updatedEntry = (PrivMgrCoreDesc)colPrivsToRevoke[j];

        if (updatedEntry.getColumnOrdinal() == currentRow.columnOrdinal_)
        {
          PrivColumnBitmap newPrivBitmap = updatedEntry.getPrivBitmap() ^= currentRow.privsBitmap_;
          PrivColumnBitmap temp = updatedEntry.getWgoBitmap().flip();
          PrivColumnBitmap newGrantableBitmap =  temp &= currentRow.grantableBitmap_;
          currentRow.current_.setPrivBitmap(newPrivBitmap);
          currentRow.current_.setWgoBitmap(newGrantableBitmap);
          traceMsg = "Adjusted current_ to reflect revoked privileges";
          traceMsg += ", grantor is ";
          traceMsg += to_string((long long int)currentRow.grantorID_);
          traceMsg += ", grantee is ";
          traceMsg += to_string((long long int) currentRow.granteeID_);
          log (__FILE__, traceMsg, -1);
        }
      }
    }
  }

  // Reconstruct the privilege tree based on the adjusted privileges 
  // starting with the object owner - get the object owner.
  PrivMgrObjects objects(trafMetadataLocation_,pDiags_);
  int32_t objectOwner = 0;
  PrivStatus privStatus = objects.fetchObjectOwner(objectUID_,objectOwner);
  if (privStatus == STATUS_ERROR)
  {
     PRIVMGR_INTERNAL_ERROR("Could not fetch object owner");
     return true;
  }

  // Create the list of columns that have been changed, during 
  // reconstruction, only look at rows that have changes.  
  // std::set does not add entries if they already exist.
  std::set<int32_t> listOfColumnOrdinals;
  for ( size_t i = 0; i < colPrivsToRevoke.entries(); i++)
  {
    PrivMgrCoreDesc colPrivToRevoke = colPrivsToRevoke[i];
    listOfColumnOrdinals.insert(colPrivToRevoke.getColumnOrdinal());
  }

  // Reconstruct tree
  for ( size_t i = 0; i < NBR_DML_COL_PRIVS; i++ )
  {
    scanColumnBranch (PrivType(i), objectOwner, listOfColumnOrdinals, rowList);
  }

  // If a branch of the tree was not visited, then we have a broken
  // tree.  Therefore, revoke restrict will leave abandoned privileges
  // in the case, return true.
  bool  notVisited = false;
  for (size_t i = 0; i < rowList.size(); i++)
  {
    ColumnPrivsMDRow &currentRow = static_cast<ColumnPrivsMDRow &> (*rowList[i]);

    // Only look at rows that have been changed
    for (std::set<int32_t>::iterator it = listOfColumnOrdinals.begin(); 
         it!= listOfColumnOrdinals.end(); ++it)
    {
      if (*it == currentRow.columnOrdinal_)
      {
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
    }
  }
  return notVisited;
}

// ----------------------------------------------------------------------------
//  method:  scanColumnBranch 
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
//   The implementation is dependent on the fact that rowList 
//   entries are ordered by Grantor, Grantee, columnOrdinal
// -----------------------------------------------------------------------------
void PrivMgrPrivileges::scanColumnBranch( const PrivType pType,
  const int32_t& grantor,
  const std::set<int32_t> &listOfColumnOrdinals,
  const std::vector<PrivMgrMDRow *> & rowList  ) 
{

  // The PrivMgrMDRow <list> is maintained in order by
  //  columnOrdinal within Grantee within Grantor - through an order by clause.

  // Skip over Grantors lower than the specified one.
  size_t i = 0;
  while (  i < rowList.size() )
  {
    ColumnPrivsMDRow &currentRow = static_cast<ColumnPrivsMDRow &> (*rowList[i]);
    if (currentRow.grantorID_ < grantor)
     i++;
   else
     break;
  }

  // For matching Grantor, process each Grantee.
  while (  i < rowList.size() )
  {
    ColumnPrivsMDRow &currentRow = static_cast<ColumnPrivsMDRow &> (*rowList[i]);
    if (currentRow.grantorID_ == grantor)
    {
      // Just look at rows that have had privileges changed
      // The listOfColumnOrdinals has this list
      PrivMgrCoreDesc current = currentRow.current_;
      std::set<int32_t>::iterator it;
      it = std::find(listOfColumnOrdinals.begin(), listOfColumnOrdinals.end(), current.getColumnOrdinal());
      if (it != listOfColumnOrdinals.end())
      {
        if ( current.getPrivBitmap().test(pType) )
        {
          // This grantee has priv.  Set corresponding visited flag.
          currentRow.visited_.setPriv(pType, true);

          if ( current.getWgoBitmap().test(pType))
          {
            // This grantee has wgo.  
            if ( currentRow.visited_.getWgoBitmap().test(pType) )
            {   // Already processed this subtree.
            }
            else
            {
              currentRow.visited_.setWgo(pType, true);
 
              // To check:  since column level privileges do not have
              // an anchor, we choose the object owner as the root.
              int32_t thisGrantee( currentRow.granteeID_ );
              if ( ComUser::isPublicUserID(thisGrantee) )
                scanPublic( pType, //  Deal with PUBLIC grantee wgo.
                            rowList );
  
              else
              {
                int32_t granteeAsGrantor;
                if (isRoleID(thisGrantee))
                {
                  std::vector<int32_t> roleIDs;
                  std::vector<int32_t> userIDs;
                  roleIDs.push_back(thisGrantee);
                  if (getGranteeIDsForRoleIDs(roleIDs,userIDs) == STATUS_ERROR)
                    return;
                  for (size_t j = 0; j < userIDs.size(); j++)
                  {
                     granteeAsGrantor = userIDs[j];
                     scanColumnBranch( pType, // Scan for this grantee as grantor.
                                       granteeAsGrantor,
                                       listOfColumnOrdinals,
                                       rowList );
                  }
                }
                granteeAsGrantor = thisGrantee;
                scanColumnBranch( pType, // Scan for this grantee as grantor.
                                  granteeAsGrantor,
                                  listOfColumnOrdinals,
                                  rowList );
              } // end process non public auth ID
            } // end visit row
          }  // end this grantee has wgo
        }  // end this grantee has this priv
      } // correct column ordinal
      i++;  // on to next rowList entry
    }
    else
      break;  // done with the grantor
  }  // end scan rowList over Grantees for this Grantor
}

/* *******************************************************************
   scanColumnPublic --  a grant wgo to PUBLIC has been encountered for the 
   current privilege type, so *all* users are able to grant this privilege.
   Scan the privsList for all grantees who have this priv from any grantor,
   marking each such entry as visited.

****************************************************************** */

void PrivMgrPrivileges::scanColumnPublic( 
  const PrivType pType, // in
  const std::set<int32_t> &listOfColumnOrdinals,
  const std::vector<PrivMgrMDRow *>& rowList )    // in
{
   // PUBLIC has a priv wgo.  So *every* grant of this priv
   //   is allowed, by any Grantor.
   for ( size_t i = 0; i < rowList.size(); i++ )
   {
      ColumnPrivsMDRow &currentRow = static_cast<ColumnPrivsMDRow &> (*rowList[i]);

      // Just look at rows that have had privileges changed
      // The listOfColumnOrdinals has this list
      PrivMgrCoreDesc current = currentRow.current_;
      std::set<int32_t>::iterator it;
      it = std::find(listOfColumnOrdinals.begin(), listOfColumnOrdinals.end(), current.getColumnOrdinal());
      if (it != listOfColumnOrdinals.end())
      {
        if ( current.getPrivBitmap().test(pType) )
        {
           // This grantee has priv.  Set corresponding visited flag.
           currentRow.visited_.setPriv(pType, true);

            // This grantee has wgo.  
            if ( currentRow.visited_.getWgoBitmap().test(pType) )
              currentRow.visited_.setWgo(pType, true);
        }
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
//    revokedPrivs - the list of privileges that were revoked
//
// Returns: PrivStatus                                               
//                                                                  
// STATUS_GOOD: Privileges were granted
//           *: Unable to send keys,  see diags.     
// ****************************************************************************
PrivStatus PrivMgrPrivileges::sendSecurityKeysToRMS(
  const int32_t granteeID, 
  const PrivMgrDesc &revokedPrivs)
{
  NAList<int32_t> roleGrantees (STMTHEAP);

  // If the privilege is revoked from a role, then need to generate QI keys for 
  // the users associated with the role.
  if (PrivMgr::isRoleID(granteeID))
  {
    // Get users granted to role (granteeID)
    std::vector<int32_t> roleID;
    roleID.push_back(granteeID);
    std::vector<int32_t> granteeIDs;
    if (getGranteeIDsForRoleIDs(roleID, granteeIDs, false) == STATUS_ERROR)
      return STATUS_ERROR;

    for (size_t g = 0; g < granteeIDs.size(); g++)
      roleGrantees.insert(granteeIDs[g]);
  }
  else
    roleGrantees.insert(grantorID_);

  // Go through the list of table privileges and generate SQL_QIKEYs
  ComSecurityKeySet keyList(NULL);
  const PrivMgrCoreDesc &privs = revokedPrivs.getTablePrivs();
  if (!buildSecurityKeys(roleGrantees,
                         granteeID,
                         objectUID_,
                         false, /* isColumn */
                         revokedPrivs.getTablePrivs(),
                         keyList))
  {
    PRIVMGR_INTERNAL_ERROR("ComSecurityKey is null");
    return STATUS_ERROR;
  }


  for (int i = 0; i < revokedPrivs.getColumnPrivs().entries(); i++)
  {
    const NAList<PrivMgrCoreDesc> &columnPrivs = revokedPrivs.getColumnPrivs();
    if (!buildSecurityKeys(roleGrantees,
                           granteeID,
                           objectUID_,
                           true, /* isColumn */
                           columnPrivs[i],
                           keyList))
    {
      PRIVMGR_INTERNAL_ERROR("ComSecurityKey is null");
      return STATUS_ERROR;
    }
  }

  // Create an array of SQL_QIKEYs
  int32_t numKeys = keyList.entries();
  SQL_QIKEY siKeyList[numKeys];
  for (size_t j = 0; j < numKeys; j++)
  {
    ComSecurityKey key = keyList[j];
    siKeyList[j].revokeKey.subject = key.getSubjectHashValue();
    siKeyList[j].revokeKey.object = key.getObjectHashValue();
    std::string actionString;
    key.getSecurityKeyTypeAsLit(actionString);
    strncpy(siKeyList[j].operation, actionString.c_str(), 2);
  }
  
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
// *  <privsOfTheUser> the list of privileges is returned
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
  PrivMgrDesc &privsOfTheUser)
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

  // generate the list of column-level privileges granted to the object and store in class
  if (generateColumnRowList() == STATUS_ERROR)
    return STATUS_ERROR;
  
  // generate the list of roles for the user
  std::vector<int32_t> roleIDs;
  if (getRoleIDsForUserID(userID,roleIDs) == STATUS_ERROR)
    return STATUS_ERROR;

  // generate the list of privileges for the user
  privsOfTheUser.setGrantee(userID);
  bool hasManagePrivileges = false;
  
  retcode = getUserPrivs(objectType, userID, roleIDs, privsOfTheUser, 
                         hasManagePrivileges);

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
std::vector<int32_t> grantees;
  
   retcode =  roles.fetchRolesForAuth(userID,roleNames,roleIDs,roleDepths,grantees);
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
  bool & hasManagePrivileges
  )
{
   PrivStatus retcode = STATUS_GOOD;
   PrivMgrDesc temp(granteeID);

   retcode = getPrivsFromAllGrantors( objectUID_,
                                      objectType,
                                      granteeID,
                                      roleIDs,
                                      temp,
                                      hasManagePrivileges
                                      );
   if (retcode != STATUS_GOOD)
    return retcode;

   summarizedPrivs = temp;


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
// *  <hasManagePrivPriv> returns whether the grantee has MANAGE_PRIVILEGES authority
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
   bool & hasManagePrivPriv
   )
{
  PrivStatus retcode = STATUS_GOOD;
  hasManagePrivPriv = false;
  bool hasPublicGrantee = false;
  
  // Check to see if the granteeID is the system user
  // if so, the system user has all privileges.  Set up appropriately
  if (ComUser::isSystemUserID(granteeID))
  {
    PrivObjectBitmap bitmap;
    bitmap.set();
    PrivMgrCoreDesc coreTablePrivs(bitmap, bitmap);
    summarizedPrivs.setTablePrivs(coreTablePrivs);
    hasManagePrivPriv = true;
    return STATUS_GOOD;
  }
  
  std::vector<PrivMgrMDRow *> rowList;
  PrivObjectBitmap systemPrivs;
  PrivMgrComponentPrivileges componentPrivileges(metadataLocation_,pDiags_);
  
  bool hasSelectMetadata = false;
  bool hasAnyManagePriv = false;
  componentPrivileges.getSQLCompPrivs(granteeID,roleIDs,systemPrivs,
                                      hasManagePrivPriv, hasSelectMetadata,
                                      hasAnyManagePriv);

  if (hasManagePrivPriv && hasAllDMLPrivs(objectType,systemPrivs))
  {
    PrivMgrCoreDesc coreTablePrivs(systemPrivs,systemPrivs);
    summarizedPrivs.setTablePrivs(coreTablePrivs);
  }
  
  // Accumulate object level privileges
  else
  {
    retcode = getRowsForGrantee(objectUID, granteeID, true, roleIDs, rowList);
    if (retcode == STATUS_ERROR)
      return retcode; 

    // Get the privileges for the object granted to the grantee
    PrivMgrCoreDesc coreTablePrivs;
    for (int32_t i = 0; i < rowList.size();++i)
    {
      ObjectPrivsMDRow &row = static_cast<ObjectPrivsMDRow &> (*rowList[i]);
    
      if (ComUser::isPublicUserID(row.granteeID_))
        hasPublicGrantee = true;

      PrivMgrCoreDesc temp (row.privsBitmap_, row.grantableBitmap_);
      coreTablePrivs.unionOfPrivs(temp);
    }
  
    PrivObjectBitmap grantableBitmap;
  
    if (hasManagePrivPriv)
       grantableBitmap = systemPrivs;
  
    PrivMgrCoreDesc temp2(systemPrivs,grantableBitmap);
    coreTablePrivs.unionOfPrivs(temp2);
  
    summarizedPrivs.setTablePrivs(coreTablePrivs);
  }

  // Add accumulated column level privileges
  rowList.clear();
  retcode = getRowsForGrantee(objectUID, granteeID, false, roleIDs, rowList);
  if (retcode == STATUS_ERROR)
    return retcode;

  NAList<PrivMgrCoreDesc> coreColumnPrivs(STMTHEAP);
  for (int32_t i = 0; i < rowList.size();++i)
  {
    ColumnPrivsMDRow &row = static_cast<ColumnPrivsMDRow &> (*rowList[i]);

    if (ComUser::isPublicUserID(row.granteeID_))
      hasPublicGrantee = true;


    // See if the ordinal has already been specified
    PrivMgrCoreDesc temp (row.privsBitmap_, row.grantableBitmap_, row.columnOrdinal_);
    PrivMgrCoreDesc *coreColumnPriv = findColumnEntry(coreColumnPrivs, row.columnOrdinal_);
    if (coreColumnPriv)
      coreColumnPriv->unionOfPrivs(temp);
    else
      coreColumnPrivs.insert(temp);
  } 

  summarizedPrivs.setColumnPrivs(coreColumnPrivs);
  summarizedPrivs.setHasPublicPriv(hasPublicGrantee);

  return STATUS_GOOD;
}


// ----------------------------------------------------------------------------
// method: getRolesToCheck
//
// This method checks all the roles granted to the user and returns a comma
// separated list of those roles that have privileges on the target object.
// ----------------------------------------------------------------------------
PrivStatus PrivMgrPrivileges::getRolesToCheck(
  const int32_t grantorID,
  const std::vector<int32_t> & roleIDs,
  const ComObjectType objectType,
  std::string &rolesWithPrivs)
{
  int32_t length;
  char roleName[MAX_DBUSERNAME_LEN + 1];
  std::vector<int_32> emptyRoleIDs;
  bool hasManagePrivPriv = false;

  for (size_t r = 0; r < roleIDs.size(); r++)
  {
    PrivMgrDesc privsOfTheRole(roleIDs[r],true);
    if (getUserPrivs(objectType, roleIDs[r], emptyRoleIDs, privsOfTheRole,
                     hasManagePrivPriv) != STATUS_GOOD)
      return STATUS_ERROR;

    if (!privsOfTheRole.isNull())
    {
      // just return what getAuthNameFromAuthID returns
      ComUser::getAuthNameFromAuthID(roleIDs[r],roleName, sizeof(roleName),length);
      if (rolesWithPrivs.size() > 0)
        rolesWithPrivs += ", ";
      rolesWithPrivs += roleName;
    }
  }

  if (rolesWithPrivs.size() > 0)
  {
    rolesWithPrivs.insert (0,"Please retry using the BY clause for one of the following roles (");
    rolesWithPrivs += ").";
  }

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
  std::vector<PrivMgrMDRow *> &rowList)
{
  PrivStatus retcode = STATUS_GOOD;

  // create the list of row pointers 
  std::vector<int32_t> authIDs = roleIDs;
  authIDs.push_back(granteeID);
  authIDs.push_back(PUBLIC_USER);
  std::vector<int32_t>::iterator it;
  std::vector<PrivMgrMDRow *> privRowList;
  if (isObjectTable)
  {
    if (objectUID == objectUID_)
      privRowList = objectRowList_;
    else
    {
      retcode = getObjectRowList(objectUID, privRowList);
      if (retcode == STATUS_ERROR)
        return retcode;
    }
  }
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
  
  return STATUS_GOOD;
}

// ****************************************************************************
// method:  summarizeColPrivs
//
// This method summarizes column privileges across all grantors.
//
// Params:
//   objectReference - the affected object
//   roleIDs - list of roles for the current object owner
//   listOfAffectedObjects - list of affected objects
//   summarizedColRefs - a list of ColumnReference pointers that contain the
//                       summarized privileges (the caller is responsible
//                       for deleting memory for this parameter
// ****************************************************************************
void PrivMgrPrivileges::summarizeColPrivs(
  const ObjectReference &objReference,
  const int32_t granteeID,
  const int32_t grantorID,
  const std::vector<int32_t> &roleIDs,
  const std::vector<ObjectUsage *> &listOfAffectedObjects,
  std::vector<ColumnReference *> &summarizedColRefs)
{
  std::string traceMsg;
  objReference.describe(traceMsg);
  traceMsg.insert (0, "summarizing column privileges ");
  log (__FILE__, traceMsg, -1);

  // objReference.columnReferences is the list of columns 
  // referencing the referenced table
  std::vector<ColumnReference *> *colRefs = objReference.columnReferences;
  for (size_t i = 0; i < colRefs->size(); i++)
  {
    ColumnReference *colRef = (*colRefs)[i];
    colRef->describe(traceMsg);
    log (__FILE__, traceMsg, i);

    // get COLUMN_PRIVILEGES rows where the grantee for the column has received 
    // privileges -  the row list is in memory so this does not require I/O
    std::vector<PrivMgrMDRow *> rowList;
    getColRowsForGranteeOrdinal(granteeID,
                                colRef->columnOrdinal,
                                columnRowList_,
                                roleIDs,
                                rowList);
    
    // go through the rowList to summarize the original and current privileges
    // We do a union operation to capture privileges from all grantors
    ColumnReference *summarized = new ColumnReference; 
    summarized->columnOrdinal = colRef->columnOrdinal;

    if (rowList.size() > 0)
    {
      for (int32_t j = 0; j < rowList.size();++j)
      {
        ColumnPrivsMDRow &row = static_cast<ColumnPrivsMDRow &> (*rowList[j]);
        PrivMgrCoreDesc originalPrivs(row.privsBitmap_, row.grantableBitmap_);
        PrivMgrCoreDesc updated = originalPrivs;

        // Update if privileges have been changed by request
        for (size_t k = 0; k < listOfAffectedObjects.size(); k++)
        {
          ObjectUsage *currentObj = listOfAffectedObjects[k];
          if (currentObj->objectUID == row.objectUID_ &&
              grantorID == row.grantorID_ &&
              currentObj->granteeID == row.granteeID_ )
          {
            ColumnReference *changedRef = currentObj->findColumn(row.columnOrdinal_);
            if (changedRef)
              updated = changedRef->updatedPrivs;
          }
        }
        summarized->originalPrivs.unionOfPrivs(originalPrivs);
        summarized->updatedPrivs.unionOfPrivs(updated);
      }
    }
    else
    {
      // For grants, this may be the first column privs added, adjust to
      // include new privs
      PrivMgrCoreDesc updated;
      for (size_t k = 0; k < listOfAffectedObjects.size(); k++)
      {
        ObjectUsage *currentObj = listOfAffectedObjects[k];
        ColumnReference *changedRef = currentObj->findColumn(colRef->columnOrdinal);
        if (changedRef)
          updated = changedRef->updatedPrivs;
      }
      // There are no originalPrivs
      summarized->updatedPrivs.unionOfPrivs(updated);
    }
      
    // Add column ref to the list
    summarizedColRefs.push_back(summarized);
  }
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
   const int32_t grantorID,
   const std::vector<int32_t> & roleIDs,
   const std::vector<ObjectUsage *> listOfChangedPrivs,
   PrivMgrDesc &summarizedOriginalPrivs,
   PrivMgrDesc &summarizedCurrentPrivs)
{
  PrivStatus retcode = STATUS_GOOD;

  // get OBJECT_PRIVILEGES rows where the grantee has received privileges
  std::vector<PrivMgrMDRow *> rowList;
  retcode = getRowsForGrantee(objectUID, granteeID, true, roleIDs, rowList);

  // rowList contains the original privileges, 
  // listOfChangedPrivs contains any updates to privileges
  // go through the list and summarize the original and current privileges
  // We do a union operation to capture privileges from all grantors
  for (int32_t i = 0; i < rowList.size();++i)
  {
    ObjectPrivsMDRow &row = static_cast<ObjectPrivsMDRow &> (*rowList[i]);
    PrivMgrCoreDesc original(row.privsBitmap_, row.grantableBitmap_);
    PrivMgrCoreDesc updated = original;
    for (size_t j = 0; j < listOfChangedPrivs.size(); j++)
    {
      ObjectUsage *pObjectUsage = listOfChangedPrivs[j];
      if (pObjectUsage->objectUID == row.objectUID_ &&
        grantorID == row.grantorID_ &&
        pObjectUsage->granteeID == row.granteeID_ )
      {
        updated = pObjectUsage->updatedPrivs.getTablePrivs();
      }
    }
    summarizedOriginalPrivs.unionOfPrivs(original);
    summarizedCurrentPrivs.unionOfPrivs(updated);
  }

  return STATUS_GOOD;
}

// *****************************************************************************
// * Method: updateDependentObjects
// *                                                       
// * This code gets the list of dependent objects that have had their privileges
// * changed because of the ongoing grant/revoke request.  It then updates
// * the object privileges table to change any dependencies.
// *
// * SQL ANSI general rules state 
// *
// * - When granting INSERT, UPDATE, or DELETE object or column privilege to
// *   a table that is referenced by one or more views, then the privilege  
// *   should be propagated to any updatable views that reference the table. 
// *   The grant request to the these views should be executed as though the 
// *   current user is _SYSTEM.
// *
// * - If the table already has SELECT privilege and a new grant is 
// *   performed that adds the WITH GRANT OPTION, then the WITH GRANT OPTION 
// *   is to be propagated to referencing views.  The grant request should
// *   be executed as though the current user is _SYSTEM.
// *
// * - When revoking INSERT, UPDATE, or DELETE object or column privilege from
// *   a table that is referenced by one or more views, then the privilege  
// *   should be revoked on any updatable views that reference the table. 
// *   The revoke request to the these views should be executed as though the 
// *   current user is _SYSTEM.
// *
// * - If the revoke is performed that removes the WITH GRANT OPTION, then 
// *   the WITH GRANT OPTION is to be removed frome referencing views.  The 
// *   revoke request should be executed as though the current user is _SYSTEM.
// *
// *****************************************************************************
PrivStatus PrivMgrPrivileges::updateDependentObjects(
  const ObjectUsage &objectUsage,
  const PrivCommand command)
{
  std::string traceMsg;

  // get list of any objects that need to change because of the grant/revoke
  std::vector<ObjectUsage *> listOfObjects;
  PrivStatus privStatus = getAffectedObjects(objectUsage, command, listOfObjects);
  if (privStatus == STATUS_ERROR)
  {
    deleteListOfAffectedObjects(listOfObjects);
    return privStatus;
  }

  traceMsg = "list of dependent objects, number of objects is ";
  traceMsg += to_string((long long int)listOfObjects.size());
  log (__FILE__, traceMsg, -1);

  char buf [1000];
  ObjectPrivsMDTable objectPrivsTable (objectTableName_, pDiags_);

  // update the OBJECT_PRIVILEGES row for each effected object
  // Starting index is 1  - the first entry is the original row that is
  // updated as part of the grant/revoke request
  size_t i = 1;
  for (i; i < listOfObjects.size(); i++)
  {
    ObjectUsage *pObjectUsage = listOfObjects[i];

    pObjectUsage->describe(traceMsg);
    log (__FILE__, traceMsg, i);

    // Determine row contents
    int32_t theGrantor = (pObjectUsage->grantorIsSystem) ? SYSTEM_USER : grantorID_;
    int32_t theGrantee = pObjectUsage->granteeID;
    int64_t theUID = pObjectUsage->objectUID;
    PrivMgrCoreDesc thePrivs = pObjectUsage->updatedPrivs.getTablePrivs();
    sprintf(buf, "where grantee_id = %d and grantor_id =  %d and object_uid = %ld",
            theGrantee, theGrantor, theUID);
    std::string whereClause (buf);

    if (thePrivs.isNull())
    {
      pObjectUsage->describe(traceMsg);
      traceMsg.insert (0, "deleted object usage ");

      // delete the row
      privStatus = objectPrivsTable.deleteWhere(whereClause);
      if (privStatus == STATUS_ERROR)
      {
        deleteListOfAffectedObjects(listOfObjects);
        return privStatus;
      }
    }
    else
    {
      sprintf(buf, "set privileges_bitmap  = %ld, grantable_bitmap =  %ld",
              thePrivs.getPrivBitmap().to_ulong(),
              thePrivs.getWgoBitmap().to_ulong());
      std::string setClause (buf);
      privStatus = objectPrivsTable.updateWhere(setClause, whereClause);

      if (privStatus == STATUS_ERROR)
      {
        deleteListOfAffectedObjects(listOfObjects);
        return privStatus;
      }
    }
  }
  deleteListOfAffectedObjects(listOfObjects);
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
bool PrivMgrPrivileges::isAuthIDGrantedPrivs(
  const int32_t authID, 
  std::vector<int64_t> &objectUIDs)
{

   std::string whereClause(" WHERE GRANTEE_ID = ");   
   std::string orderByClause (" ORDER BY object_name ");

   char authIDString[20];

   sprintf(authIDString,"%d",authID);

   whereClause += authIDString; 

   // set pointer in diags area
   int32_t diagsMark = pDiags_->mark();

   std::vector<PrivMgrMDRow *> rowList;
   ObjectPrivsMDTable myTable(objectTableName_,pDiags_);

   PrivStatus privStatus = myTable.selectWhere(whereClause, orderByClause ,rowList);
   if (privStatus != STATUS_GOOD)
   {
      deleteRowList(rowList);
      return privStatus;
   }

   int32_t rowCount = rowList.size();
   for (size_t i = 0; i < rowCount; i++)
   {
      ObjectPrivsMDRow &row = static_cast<ObjectPrivsMDRow &> (*rowList[i]);
      objectUIDs.push_back(row.objectUID_);
   }
   deleteRowList(rowList);

   if ((privStatus == STATUS_GOOD || privStatus == STATUS_WARNING) &&
        rowCount > 0)
     return true;
      
   // TBD - check for granted column level privs
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
// *  <isWGOSpecified> for grants, include the with grant option
// *                   for revokes (always true), if revoke the priv, also
// *                   revoke the with grant option
// *  <isGofSpecified> for grants - alway false (not applicable)
// *                   for revokes - only revoke the with grant option
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
  const bool isGofSpecified,
  const std::vector<PrivType> privsList,
  const std::vector<ColPrivSpec> & colPrivsList,
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
    // For grant:
    //    WGO is set if WITH GRANT OPTION specified in syntax
    //    GOF is false, so always turn on the priv bits
    // For revoke:
    //    WGO is always true, so always remove the WGO bits
    //    GOF is set if GRANT OPTION FOR specified in syntax, so don't set privs
    //       bit if only removing the grant option (JIRA 3194)
    privsToProcess.setAllObjectPrivileges(objectType, !isGofSpecified, isWgoSpecified);
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
          tableCorePrivs.testAndSetBit(privsList[i],isWgoSpecified,isGofSpecified);  
        break;
      case DELETE_PRIV:
      case INSERT_PRIV:
      case REFERENCES_PRIV:
      case SELECT_PRIV:
        if (!isObject)
          isIncompatible = true;
        else
          tableCorePrivs.testAndSetBit(privsList[i],isWgoSpecified,isGofSpecified);  
        break;
      case UPDATE_PRIV:
        if (!isObject && !isLibrary)
          isIncompatible = true;
        else
          tableCorePrivs.testAndSetBit(privsList[i],isWgoSpecified,isGofSpecified);  
        break;
      case USAGE_PRIV:
        if (!isLibrary && !isSequence)
          isIncompatible = true;
        else
          tableCorePrivs.testAndSetBit(privsList[i],isWgoSpecified,isGofSpecified);  
        break;
      case ALL_DML:
        if (!isObject)
          isIncompatible = true;
          if (isGofSpecified)
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
  NAList<PrivMgrCoreDesc> columnCorePrivs(STMTHEAP);
  for (size_t i = 0; i < colPrivsList.size(); i++)
  {
    const ColPrivSpec &colPrivSpec = colPrivsList[i];

    PrivMgrCoreDesc *existingEntry = findColumnEntry(columnCorePrivs,
                                                     colPrivSpec.columnOrdinal);
    if (existingEntry != NULL)
      existingEntry->testAndSetBit(colPrivSpec.privType,isWgoSpecified,isGofSpecified);
    else
    {
      PrivMgrCoreDesc colPrivToGrant;
      colPrivToGrant.setColumnOrdinal(colPrivSpec.columnOrdinal);
      colPrivToGrant.testAndSetBit(colPrivSpec.privType,isWgoSpecified,isGofSpecified);

      columnCorePrivs.insert(colPrivToGrant);
    }
  }

  privsToProcess.setColumnPrivs(columnCorePrivs);

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
                 
               // See if any WGO statement exist
               if (hasWGO[p])
               {
                  closeColumnList(WGOString[p]);
                  withWGO += WGOString[p];
                  // Reset to original value
                  WGOString[p].assign(PrivMgrUserPrivs::convertPrivTypeToLiteral((PrivType)p) + "(");
                  hasWGO[p] = false;
               }
               
               // See if any WGO statement exist
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
// *    This function searches a vector of PrivMgrCoreDesc for a matching         *
// *  column ordinal.                                                          *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *                                                                           *
// *  <colPrivEntries>             NAList<PrivMgrCoreDesc> &        In       *
// *    is a reference to a vector of ColPrivEntry.                            *
// *                                                                           *
// *  <columnOrdinal>              const int32_t                      In       *
// *    is the column ordinal to search for.                                   *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivMgrCoreDesc                                                     *
// *                                                                           *
// * NULL: No entry found with that column ordinal                             *
// *    *: Entry found with matching column ordinal                            *
// *                                                                           *
// *****************************************************************************
static PrivMgrCoreDesc * findColumnEntry(
   NAList<PrivMgrCoreDesc> & colPrivEntries,
   const int32_t columnOrdinal)
   
{
   for (size_t i = 0; i < colPrivEntries.entries(); i++)
      if (colPrivEntries[i].getColumnOrdinal() == columnOrdinal)
         return & colPrivEntries[i];
         
   return NULL;
}   
//************************** End of findColumnEntry ****************************


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
// *  <grantedColPrivs>            NAList<PrivMgrCoreDesc> &        Out      *
// *    passes back a privileges granted to <granteeID> by <grantorID>.        *
// *                                                                           *
// *****************************************************************************
static void getColRowsForGranteeGrantor(
   const std::vector <PrivMgrMDRow *> &columnRowList,
   const int32_t granteeID,
   const int32_t grantorID,
   NAList<PrivMgrCoreDesc> & grantedColPrivs)
   
{

   for (size_t i = 0; i < columnRowList.size(); ++i)
   {
      ColumnPrivsMDRow &row = static_cast<ColumnPrivsMDRow &> (*columnRowList[i]);
      PrivMgrCoreDesc colPrivGrant;
      
      if (row.grantorID_ == grantorID && row.granteeID_ == granteeID)
      {
         colPrivGrant.setColumnOrdinal(row.columnOrdinal_);
         colPrivGrant.setPrivBitmap(row.privsBitmap_.to_ulong());
         colPrivGrant.setWgoBitmap(row.grantableBitmap_.to_ulong());
      
         grantedColPrivs.insert(colPrivGrant);
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
// method: reportPrivWarnings
//
// Ansi states that when a grant statement is executed, a set of privilege 
// descriptors (CPD) is created based on existing privileges for the object and 
// object’s columns. Each CPD contains the grantee, action (privileges), object, 
// column and grantor. A similar list of privilege descriptors is created based 
// on the grant/revoke statement (GPD).
//
// If there is an element in the GPD (what the user requested) that is not in 
// the  CPD (what was actually granted/revoked), then a warning – privilege not 
// granted/revoked is displayed.
// 
// This method compares the list of actual privileges granted/revoked 
// (actualPrivs)to the list privileges requested (origPrivs).  If a privilege 
// was requested but not granted/revoked report a warning.
// *****************************************************************************
void PrivMgrPrivileges::reportPrivWarnings(
    const PrivMgrDesc &origPrivs,
    const PrivMgrDesc &actualPrivs,
    const CatErrorCode warningCode)
{
  PrivMgrCoreDesc objPrivsNotApplied = origPrivs.getTablePrivs();
  objPrivsNotApplied.suppressDuplicatedPrivs(actualPrivs.getTablePrivs());
  if (!objPrivsNotApplied.isNull())
  {
    for ( size_t i = FIRST_DML_PRIV; i <= LAST_DML_PRIV; i++ )
    {
      PrivType privType = PrivType(i);
      if (objPrivsNotApplied.getPriv(privType))
      {
        *pDiags_ << DgSqlCode(warningCode)
                 << DgString0(PrivMgrUserPrivs::convertPrivTypeToLiteral(privType).c_str());
      }
    }
  }

  NAList<PrivMgrCoreDesc> colPrivs = origPrivs.getColumnPrivs();
  for (int i = 0; i < colPrivs.entries(); i++)
  {
     PrivMgrCoreDesc colPrivsNotApplied = colPrivs[i];

     int index = actualPrivs.getColumnPriv(i);
     if (index >= 0)
     {
       PrivMgrCoreDesc colPrivsActual = actualPrivs.getColumnPrivs()[index];
       colPrivsNotApplied.suppressDuplicatedPrivs(colPrivsActual);
     }

     if (!colPrivsNotApplied.isNull())
     {
       for ( size_t j = FIRST_DML_PRIV; j <= LAST_DML_PRIV; j++ )
       {
         PrivType privType = PrivType(j);
         if (colPrivsNotApplied.getPriv(privType))
         {
           // would be better to add column name instead of number
           // would require an I/O to read COLUMNS to get the name
           // associated with the number
           char privStr[100];
           snprintf(privStr, sizeof(privStr), "%s (column number %d) ", 
                    PrivMgrUserPrivs::convertPrivTypeToLiteral(privType).c_str(),
                    colPrivsNotApplied.getColumnOrdinal());
           *pDiags_ << DgSqlCode(warningCode)
                    << DgString0(privStr);
         }
       }
     }
  }
}

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

  ExeCliInterface cliInterface(STMTHEAP, 0, NULL, 
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

  ExeCliInterface cliInterface(STMTHEAP, 0, NULL, 
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

  ExeCliInterface cliInterface(STMTHEAP, 0, NULL, 
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

  ExeCliInterface cliInterface(STMTHEAP, 0, NULL, 
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
  ExeCliInterface cliInterface(STMTHEAP, 0, NULL, 
  CmpCommon::context()->sqlSession()->getParentQid());
  int32_t cliRC = cliInterface.executeImmediate(buf, (char*)&rowsSelected, &theLen, FALSE);
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
  privDesc.setAllTableGrantPrivileges(true, true);
  int64_t tableBits = privDesc.getTablePrivs().getPrivBitmap().to_ulong();
 
  privDesc.setAllLibraryGrantPrivileges(true, true);
  int64_t libraryBits = privDesc.getTablePrivs().getPrivBitmap().to_ulong();

  privDesc.setAllUdrGrantPrivileges(true, true);
  int64_t udrBits = privDesc.getTablePrivs().getPrivBitmap().to_ulong();

  privDesc.setAllSequenceGrantPrivileges(true, true);
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
  cliRC = cliInterface.executeImmediate(buf, (char*)&rowsSelected, &len, FALSE);
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







//*****************************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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
//*****************************************************************************
  
#include "PrivMgrPrivileges.h"

#include "PrivMgrMD.h"
#include "PrivMgrMDTable.h"
#include "PrivMgrDesc.h"
#include "PrivMgrDefs.h"
#include "PrivMgrMDDefs.h"
#include "PrivMgrRoles.h"

#include <string>
#include <cstdio>
#include <algorithm>
#include "sqlcli.h"
#include "ComSmallDefs.h"
#include "ExExeUtilCli.h"
#include "ComDiags.h"
#include "ComQueue.h"
#include "CmpCommon.h"
#include "CmpDDLCatErrorCodes.h"
#include "ComSecurityKey.h"
#include "NAUserId.h"

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
   : PrivMgrMDRow("OBJECT_PRIVILEGES"),
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
   
   inline const int64_t getObjectUID (void) const { return objectUID_; };
   inline const std::string getObjectName (void) const { return objectName_; };
   inline const std::string getObjectType (void) const { return objectType_; };
   inline const int64_t getGrantee (void) const { return granteeID_; };
   inline const std::string getGranteeName (void) const { return granteeName_; };
   inline const int64_t getGrantor (void) const { return grantorID_; };
   inline const std::string getGrantorName (void) const { return grantorName_; };
   inline const std::bitset<NBR_OF_PRIVS>& getPrivilegesBitmap (void) const { return privsBitmap_; };
   inline const std::bitset<NBR_OF_PRIVS>& getGrantableBitmap (void) const { return grantableBitmap_; };

   inline void setObjectUID (int64_t objectUID)
       { objectUID_ = objectUID; };
   inline void setObjectName (std::string objectName)
       { objectName_ = objectName; };
   inline void setObjectType (std::string objectType)
       { objectType_ = objectType; };
   inline void setGrantee (int64_t grantee)
       { granteeID_ = grantee; };
   inline void setGranteeName (std::string granteeName)
       { granteeName_ = granteeName; };
   inline void setGrantor (int64_t grantor)
       { grantorID_ = grantor; };
   inline void setGrantorName (std::string grantorName)
       { grantorName_ = grantorName; };
   inline void setPrivilegesBitmap (int64_t bitmapInt);
   inline void setPrivilegesBitmap (std::bitset<NBR_OF_PRIVS> bitmap)
       { privsBitmap_ = bitmap; };
   inline void setGrantableBitmap (int64_t bitmapInt);
   inline void setGrantableBitmap (std::bitset<NBR_OF_PRIVS> bitmap)
       { grantableBitmap_ = bitmap; };

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

// -------------------------------------------------------------------
// Data Members:
// -------------------------------------------------------------------
     
   int64_t            objectUID_;
   std::string        objectName_;
   std::string        objectType_;
   int32_t            granteeID_;
   std::string        granteeName_;
   std::string        granteeType_;
   int32_t            grantorID_;
   std::string        grantorName_;
   std::string        grantorType_;
   std::bitset<NBR_OF_PRIVS>      privsBitmap_;
   std::bitset<NBR_OF_PRIVS>      grantableBitmap_;
   
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
   : PrivMgrMDTable(tableName,pDiags)
     {};

   virtual ~ObjectPrivsMDTable() 
   {};

   virtual PrivStatus insert(const PrivMgrMDRow &row);
   virtual PrivStatus selectWhereUnique(
      const std::string & whereClause,
      PrivMgrMDRow & row);
   PrivStatus selectWhere(
      const std::string & whereClause,
      std::vector<PrivMgrMDRow *> &rowList);
   virtual PrivStatus deleteWhere(const std::string & whereClause);
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
  fullTableName_ = metadataLocation_ + ".OBJECT_PRIVILEGES";
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
  fullTableName_  = metadataLocation + ".OBJECT_PRIVILEGES";
} 

// ----------------------------------------------------------------------------
// Construct a basic PrivMgrPrivileges object
// ----------------------------------------------------------------------------
PrivMgrPrivileges::PrivMgrPrivileges (
   const std::string &metadataLocation,
   ComDiagsArea *pDiags)
: PrivMgr(metadataLocation, pDiags)
{
  fullTableName_  = metadataLocation + ".OBJECT_PRIVILEGES";
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
  fullTableName_ = other.fullTableName_;
}

// -----------------------------------------------------------------------
// Destructor.
// -----------------------------------------------------------------------
PrivMgrPrivileges::~PrivMgrPrivileges() 
{ }

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

// Only need to generate keys for SELECT, INSERT, UPDATE, and DELETE
   for ( size_t i = 0; i < NBR_OF_PRIVS; i++ )
   {
      PrivType pType = PrivType(i);
      if ((pType == SELECT_PRIV || pType == INSERT_PRIV || 
           pType == UPDATE_PRIV || pType == DELETE_PRIV) && privs.getPriv(pType))
      {
         ComSecurityKey *key = new ComSecurityKey(granteeID, 
                                                  objectUID_,
                                                  pType, 
                                                  ComSecurityKey::OBJECT_IS_OBJECT);
         if (key->isValid())
            secKeySet.push_back(key);
         else
         {
            // Probably should report a different error.  Is an error possible?
            *pDiags_ << DgSqlCode (-CAT_NOT_AUTHORIZED);
            return STATUS_ERROR;
         }
      }
   }
   
   return STATUS_GOOD;

}

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
      const std::string &objectType,
      const int32_t granteeID,
      const std::string &granteeName,
      const std::string &grantorName,
      const std::vector<std::string> &privsList,
      const bool isAllSpecified,
      const bool isWGOSpecified)
{
  PrivStatus retcode = STATUS_GOOD;

  if (objectUID_ == 0)
  {
    PRIVMGR_INTERNAL_ERROR("objectUID is 0 for grant command");
    return STATUS_ERROR;
  }

  // If this grant request is called during the creation of the OBJECT_PRIVILEGES
  // table, just return okay.  Fixes a chicken and egg problem.
  char theQuote = '"';
  std::string nameRequested(objectName_);
  std::string nameToCheck(fullTableName_);

  // Delimited name issue.  The passed in objectName may enclose name parts in
  // double quotes even if the name part contains only [a-z][A-Z][0-9]_
  // characters. The same is true for the stored metadataLocation_.
  // To allow equality checks to work, we strip off the double quote delimiters
  // from both names. Fortunately, the double quote character is not allowed in
  // any SQL identifier except as delimiters - so this works.
  nameRequested.erase(std::remove(nameRequested.begin(), nameRequested.end(), theQuote), nameRequested.end());
  nameToCheck.erase(std::remove(nameToCheck.begin(), nameToCheck.end(), theQuote), nameToCheck.end());

  if (nameRequested == nameToCheck && grantorID_ == SYSTEM_AUTH_ID)
    return STATUS_GOOD;

  // TDB:  add support for the BY clause

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

  // get privileges for the grantor and make sure the grantor can grant
  // at least one of the requested privileges
  //
  // SQL Ansi states that privileges that can be granted should be done so
  // even if some requested privilege are not grantable.
  PrivMgrDesc privsOfTheGrantor(grantorID_);
  bool withRoles = true;
  retcode = getUserPrivs( grantorID_, privsOfTheGrantor, withRoles ); 
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

  ObjectPrivsMDTable objectPrivsTable (fullTableName_, pDiags_);
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

    // update the OBJECT_PRIVILEGES row for each effected object
    for (size_t i = 0; i < listOfObjects.size(); i++)
    {
      ObjectUsage *pObj = listOfObjects[i];
      int32_t theGrantor = (pObj->objectType == VIEW_OBJECT_LIT) ? SYSTEM_AUTH_ID : grantorID_;
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
   // insert the row
   retcode = objectPrivsTable.insert(row);

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
      const std::string &objectType,
      const int32_t granteeID,
      const std::string &granteeName,
      const PrivMgrBitmap privsBitmap,
      const PrivMgrBitmap grantableBitmap)
{
  PrivStatus retcode = STATUS_GOOD;

  if (objectUID_ == 0)
  {
    PRIVMGR_INTERNAL_ERROR("objectUID is 0 for grant command");
    return STATUS_ERROR;
  }

  // set up the values of the row to insert
  ObjectPrivsMDRow row;
  row.objectUID_ = objectUID_;
  row.objectName_ = objectName_;
  row.objectType_ = objectType;
  row.granteeID_ = granteeID;
  row.granteeName_ = granteeName;
  row.granteeType_ = USER_GRANTEE_LIT;
  row.grantorID_ = SYSTEM_AUTH_ID;
  row.grantorName_ = "_SYSTEM";
  row.grantorType_ = SYSTEM_GRANTOR_LIT;
  row.privsBitmap_ = privsBitmap;
  row.grantableBitmap_ = grantableBitmap;

  // Insert the new row, the row should not exist since the request
  // is coming during the creation of a new object.
  ObjectPrivsMDTable objectPrivsTable (fullTableName_, pDiags_);
  retcode = objectPrivsTable.insert(row);

  return retcode;
}

// *****************************************************************************
// * Method: grantSelectOnAuthsToPublic                                
// *                                                       
// *    Inserts a row into the OBJECT_PRIVILEGES table during initialization to
// *    grant SELECT on the AUTHS table to PUBLIC.  
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <objectLocation> the location of the Trafodion OBJECTS table which is
// *                   used to extract object UID of the AUTHS table
// *  <authsLocation> the location of the Trafodion AUTHS table which is 
// *                  stored in the row
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: Grant to PUBLIC was inserted
// *           *: Unable to insert grant to PUBLIC, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::grantSelectOnAuthsToPublic(
   const std::string & objectsLocation,
   const std::string & authsLocation)

{

  ObjectPrivsMDTable objectPrivsTable(fullTableName_,pDiags_);

  return objectPrivsTable.insertSelectOnAuthsToPublic(objectsLocation,authsLocation);
  
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

  // RI constraints can only be defined for base tables
  if (objectUsage.objectType != BASE_TABLE_OBJECT_LIT)
    return STATUS_GOOD;
  
  // get the underlying tables for all RI constraints that reference the object
  std::vector<ObjectReference *> objectList;
  PrivMgrMDAdmin admin(trafMetadataLocation_, metadataLocation_, pDiags_);
  retcode = admin.getReferencingTablesForConstraints(objectUsage, objectList);
  if (retcode == STATUS_ERROR)
    return retcode;

  // objectList contain ObjectReferences for all tables that reference the
  // ObjectUsage (object losing privilege) through an RI constraint
  PrivMgrDesc originalPrivs;
  PrivMgrDesc currentPrivs;
  for (size_t i = 0; i < objectList.size(); i++)
  {
    ObjectReference *pObj = objectList[i];

    // get the summarized original and current privs for the referencing table 
    // current privs contains any adjustments
    bool withRoles = true;
    retcode = summarizeCurrentAndOriginalPrivs(pObj->objectUID,
                                               pObj->objectOwner,
                                               withRoles,
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

  // Get any views that referenced this object to see if the privilege changes 
  // should be propagated
  std::vector<ViewUsage> viewUsages;
  PrivMgrMDAdmin admin(trafMetadataLocation_, metadataLocation_, pDiags_);
  retcode = admin.getViewsThatReferenceObject(objectUsage, viewUsages);
  if (retcode == STATUS_NOTFOUND)
   return STATUS_GOOD;
  if (retcode != STATUS_GOOD && retcode != STATUS_WARNING)
    return retcode;
 
  // for each entry in the viewUsages list calculate the changed
  // privileges and call dealWithViews recursively
  for (size_t i = 0; i < viewUsages.size(); i++)
  {
    ViewUsage viewUsage = viewUsages[i];

    // this method recreates privileges for the view based on the original
    // and the current.  Updated descriptors are stored in the viewUsage
    // structure.
    retcode = gatherViewPrivileges(viewUsage, listOfAffectedObjects);
    if (retcode != STATUS_GOOD && retcode != STATUS_WARNING)
      return retcode;

    // Performance optimization:
    // If this is a revoke restrict and the view no longer has SELECT privilege
    // return an error.  Don't continue searching
    if (command == PrivCommand::REVOKE_OBJECT_RESTRICT )
    {
      PrivMgrCoreDesc thePrivs = objectUsage.updatedPrivs.getTablePrivs();

      // If view no longer has select privilege, throw an error
      if (!thePrivs.getPriv(SELECT_PRIV))
      {
         *pDiags_ << DgSqlCode (-CAT_DEPENDENT_OBJECTS_EXIST)
                  << DgString0 (viewUsage.viewName.c_str());
         return STATUS_ERROR;
      }
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
      pUsage->objectType = VIEW_OBJECT_LIT;
      pUsage->originalPrivs = viewUsage.originalPrivs;
      pUsage->updatedPrivs = viewUsage.updatedPrivs;
      listOfAffectedObjects.push_back(pUsage);
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
  if (retcode == STATUS_ERROR)
    return retcode;

  // For each referenced object, summarize the original and current
  // privileges
  PrivMgrDesc originalPrivs;
  PrivMgrDesc currentPrivs;
  for (size_t i = 0; i < objectList.size(); i++)
  {
    ObjectReference *pObj = objectList[i];

    // get the summarized original and current privs for the 
    // referenced object that have been granted to the view owner
    // listOfAffectedObjects contain the privilege adjustments needed
    //   to generate the current privs
    bool withRoles = true;
    retcode = summarizeCurrentAndOriginalPrivs(pObj->objectUID,
                                               viewUsage.viewOwner, 
                                               withRoles,
                                               listOfAffectedObjects,
                                               originalPrivs,
                                               currentPrivs);
    if (retcode != STATUS_GOOD)
      return retcode;

    // add the returned privilege to the summarized privileges
    // for all objects
    summarizedOriginalPrivs.intersectionOfPrivs(originalPrivs);
    summarizedCurrentPrivs.intersectionOfPrivs(currentPrivs);
  }

  // Update view usage with summarized privileges
  viewUsage.originalPrivs = summarizedOriginalPrivs;
  viewUsage.updatedPrivs = summarizedCurrentPrivs;

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
//   listOfAffectedObjects - returns the list of affected objects
//
// In the future, we want to cache the lists of objects instead of going to the
// metadata everytime.
// ****************************************************************************
PrivStatus PrivMgrPrivileges::getAffectedObjects(
  const ObjectUsage &objectUsage,
  const PrivCommand command,
  std::vector<ObjectUsage *> &listOfAffectedObjects)
{
  PrivStatus retcode = STATUS_GOOD;

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
  }

  // Find list of affected views
  retcode = dealWithViews (objectUsage, command, listOfAffectedObjects);
  if (retcode != STATUS_GOOD && retcode != STATUS_WARNING)
    return retcode;

  // TBD:  add checks for UDRs,
 
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
//                *: unable to grant privileges, see diags.     
// ----------------------------------------------------------------------------
PrivStatus PrivMgrPrivileges::getGrantedPrivs(
  const int32_t granteeID,
  PrivMgrMDRow &row)
{
  ObjectPrivsMDTable objectPrivsTable (fullTableName_, pDiags_);
  char buf[1000];
  sprintf(buf, "where grantee_id = %d and grantor_id =  %d and object_uid = %ld",
              granteeID, grantorID_, objectUID_);
  std::string whereClause (buf);
  return objectPrivsTable.selectWhereUnique (whereClause, row);
}
 
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
PrivStatus PrivMgrPrivileges::revokeObjectPriv (const std::string &objectType,
                                                const int32_t granteeID,
                                                const std::vector<std::string> &privsList,
                                                const bool isAllSpecified,
                                                const bool isGOFSpecified)
{
  PrivStatus retcode = STATUS_GOOD;

  if (objectUID_ == 0)
  {
    PRIVMGR_INTERNAL_ERROR("objectUID is 0 for revoke command");
    return STATUS_ERROR;
  }

  // TDB:  add support for the BY clause

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


  // TDB:  performance enhancement, get the rows associated with the object
  // once and then search this list to find privsOfTheGrantor, etc.  Now
  // the OBJECT_PRIVILEGES table is accessed anytime new information is
  // required.
 

  // get privileges for the grantor and make sure the grantor can revoke
  // at least one of the requested privileges
  
  // SQL Ansi states that privileges that can be revoked should be done so
  // even if some requested privilege are not revokable.
  PrivMgrDesc privsOfTheGrantor(grantorID_);
  bool withRoles = true;
  retcode = getUserPrivs( grantorID_, privsOfTheGrantor, withRoles );
  if (retcode != STATUS_GOOD)
    return retcode;

  // If null, the grantor has no privileges
  if ( privsOfTheGrantor.isNull() )
  {
     *pDiags_ << DgSqlCode (-CAT_NOT_AUTHORIZED);
     return STATUS_ERROR;
   }

  // Remove any privsToRevoke which are not held grantable by the Grantor.
  // TBD: if not all privileges are revokeable, should at least report
  //      which ones were not revoked.
  bool warnNotAll = false;
  if ( privsToRevoke.limitToGrantable( privsOfTheGrantor ) )
  {
    // limitToGrantable true ==> some specified privs were not grantable.
    if ( isAllSpecified )
    {
      // This is ok.  Can specify ALL without having ALL privileges set.
    }
    else
      warnNotAll = true;  // Not all the specified privs are grantable.
  }

  // If nothing left to grant, we are done.
  if ( privsToRevoke.isNull() )
  {
    *pDiags_ << DgSqlCode (-CAT_PRIVILEGE_NOT_GRANTED);
    return STATUS_ERROR;
  }

  // See if grantor has previously granted privileges to the grantee
  ObjectPrivsMDRow row;
  retcode = getGrantedPrivs(granteeID, row);
  if (retcode == STATUS_NOTFOUND)
  {
    return STATUS_GOOD;
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


  // get all privilege descriptors for the object
  char buf[1000];

  sprintf(buf, " where object_uid = %ld", objectUID_);
  std::string whereClause (buf);

  ObjectPrivsMDTable objectPrivsTable(fullTableName_, pDiags_);

#if 0
  std::vector<PrivMgrMDRow *> rowList;

  retcode = objectPrivsTable.selectWhere(whereClause, rowList);
  if (retcode != STATUS_GOOD)
  {
    deleteRowsForGrantee(rowList);
    return retcode;
  }

  // Go rebuild the privilege tree to see if it is broken
  // If it is broken, return an error
  // TBD: This code was ported from another implementation and it is not
  //      working yet, comment out at this time.
  PrivMgrMDRow &tblRow = static_cast<PrivMgrMDRow &>(row);
  if (checkRevokeRestrict (tblRow, rowList))
  {
     deleteRowsForGrantee(rowList);
     *pDiags_ << DgSqlCode (-CAT_DEPENDENT_OBJECTS_EXIST);
     return STATUS_ERROR;
  }
  deleteRowsForGrantee(rowList);
#endif

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

  
  // update the OBJECT_PRIVILEGES row for each effected object
  for (size_t i = 0; i < listOfObjects.size(); i++)
  {
    ObjectUsage *pObj = listOfObjects[i];
    PrivMgrCoreDesc thePrivs = pObj->updatedPrivs.getTablePrivs();

    // If view no longer has select privilege, throw an error
    if (pObj->objectType == VIEW_OBJECT_LIT)
    {
      if (!thePrivs.getPriv(SELECT_PRIV))
      {
         std::string objectName = listOfObjects[1]->objectName;
         deleteListOfAffectedObjects(listOfObjects);
         *pDiags_ << DgSqlCode (-CAT_DEPENDENT_OBJECTS_EXIST)
                  << DgString0 (objectName.c_str());
         return STATUS_ERROR;
      }
    }

    int32_t theGrantor = (pObj->objectType == VIEW_OBJECT_LIT) ? SYSTEM_AUTH_ID : grantorID_;
    int32_t theGrantee = pObj->objectOwner;
    int64_t theUID = pObj->objectUID;

    sprintf(buf, "where grantee_id = %d and grantor_id =  %d and object_uid = %ld",
            theGrantee, theGrantor, theUID);
    std::string whereClause (buf);

    if (thePrivs.isNull())
    {
      // delete the row
      retcode = objectPrivsTable.deleteWhere(whereClause);
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

      // update the row
      retcode = objectPrivsTable.updateWhere(setClause, whereClause);
      if (retcode == STATUS_ERROR)
      {
        deleteListOfAffectedObjects(listOfObjects);
        return retcode;
      }
    }
  }
  
  deleteListOfAffectedObjects(listOfObjects);
  
  // Send a message to the Trafodion RMS process about the revoke operation.
  // RMS will contact all master executors and ask that cached privilege 
  // information be re-calculated
  retcode = sendSecurityKeysToRMS(granteeID, listOfRevokedPrivileges);
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
  ObjectPrivsMDTable objectPrivsTable (fullTableName_, pDiags_);
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
// The diags area is not touched
// ---------------------------------------------------------------------------- 
bool PrivMgrPrivileges::checkRevokeRestrict ( 
  PrivMgrMDRow &rowIn,
  std::vector <PrivMgrMDRow *> &rowList )
{
  // Search the list of privileges associated with the object and replace 
  // the bitmaps of the current row with the bitmaps of the row sent in (rowIn).  
  // At the same time, clear visited_ and set current_ to row values
  ObjectPrivsMDRow updatedRow = static_cast<ObjectPrivsMDRow &>(rowIn);
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

    int32_t systemGrantor = SYSTEM_AUTH_ID;
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
    if (currentRow.anyNotVisited())
    {
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
    if (currentRow.getGrantor() < grantor)
     i++;
   else
     break;
  }

  // For matching Grantor, process each Grantee.
  while (  i < privsList.size() )
  {
    ObjectPrivsMDRow &privEntry = static_cast<ObjectPrivsMDRow &> (*privsList[i]);
    if (privEntry.getGrantor() == grantor)
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

                int32_t thisGrantee( privEntry.getGrantee() );
                if ( isPublicUser(thisGrantee) )
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
  ObjectPrivsMDTable objectPrivsTable(fullTableName_, pDiags_);
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
   std::vector<PrivMgrBitmap> & privBitmaps)
   
{

std::vector<PrivMgrMDRow *> rowList;

ObjectPrivsMDTable objectPrivsTable(fullTableName_,pDiags_);
 
PrivStatus privStatus = objectPrivsTable.selectWhere(whereClause + orderByClause,rowList);

   if (privStatus != STATUS_GOOD)
   {
      deleteRowsForGrantee(rowList);
      return privStatus;
   }
   
   for (size_t r = 0; r < rowList.size(); r++)
   {
      ObjectPrivsMDRow &row = static_cast<ObjectPrivsMDRow &>(*rowList[r]);
      privBitmaps.push_back(row.privsBitmap_);
   }
   deleteRowsForGrantee(rowList);
   
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
// *  <privilegeText> The resultant GRANT statement(s)
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD    : Grants were found
// * STATUS_NOTFOUND: No grants were found
// *               *: Unable to insert privileges, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::getPrivTextForObject(std::string &privilegeText)
{
  PrivStatus retcode = STATUS_GOOD;

  if (objectUID_ == 0)
  {
    PRIVMGR_INTERNAL_ERROR("objectUID is 0 for describe privileges command");
    return STATUS_ERROR;
  }


  // extract the privilege row for object from OBJECT_PRIVILEGES
  char buf[100];
  sprintf(buf, "where object_uid = %ld", objectUID_);
  std::string whereClause (buf);

  // Should space be allocated and deleted from the heap for the rowList?
  //   -- how many rows will be returned?
  std::vector<PrivMgrMDRow *> rowList;

  ObjectPrivsMDTable objectPrivsTable(fullTableName_, pDiags_);
  retcode = objectPrivsTable.selectWhere(whereClause, rowList);
  if (retcode != STATUS_GOOD)
  {
    deleteRowsForGrantee(rowList);
    return retcode;
  }
  
  // Build a grant statement for each row returned
  // TDB: Need to also describe WGO for each privilege
  // TDB: If we support multiple grantees per grant statement, 
  //      this code can be improved
  std::string grantStmt;
  for (int32_t i = 0; i < rowList.size();++i)
  {
    ObjectPrivsMDRow &row = static_cast<ObjectPrivsMDRow &> (*rowList[i]);

    std::string postfix = "ON ";
    postfix += row.objectName_;
    postfix += " TO ";
    postfix += row.getGranteeName();

    std::string withoutWGO;
    std::string withWGO;
    std::bitset<NBR_OF_PRIVS> privsBitmap = row.getPrivilegesBitmap();
    std::bitset<NBR_OF_PRIVS> wgoBitmap = row.getGrantableBitmap(); 
    if (privsBitmap.test(DELETE_PRIV))
    {
      if (wgoBitmap.test(DELETE_PRIV))
        withWGO += "DELETE, ";
      else
        withoutWGO += "DELETE, ";
    }
    if (privsBitmap.test(INSERT_PRIV))
    {
      if (wgoBitmap.test(INSERT_PRIV))
        withWGO += "INSERT, ";
      else
        withoutWGO += "INSERT, ";
    }
    if (privsBitmap.test(SELECT_PRIV))
    {
      if (wgoBitmap.test(SELECT_PRIV))
        withWGO += "SELECT, ";
      else
        withoutWGO += "SELECT, ";
    }
    if (privsBitmap.test(UPDATE_PRIV))
    {
      if (wgoBitmap.test(UPDATE_PRIV))
        withWGO += "UPDATE, ";
      else
        withoutWGO += "UPDATE, ";
    }
    if (privsBitmap.test(USAGE_PRIV))
    {
      if (wgoBitmap.test(USAGE_PRIV))
        withWGO += "USAGE, ";
      else
        withoutWGO += "USAGE, ";
    }
    if (privsBitmap.test(REFERENCES_PRIV))
    {
      if (wgoBitmap.test(REFERENCES_PRIV))
        withWGO += "REFERENCES, ";
      else
        withoutWGO += "REFERENCES, ";
    }
    if (privsBitmap.test(EXECUTE_PRIV))
    {
      if (wgoBitmap.test(EXECUTE_PRIV))
        withWGO += "EXECUTE, ";
      else
        withoutWGO += "EXECUTE, ";
    }

    if (!withoutWGO.empty())
    {
      // remove last ','
      size_t commaPos = withoutWGO.find_last_of(",");
      if (commaPos != std::string::npos)
        withoutWGO.replace(commaPos, 1, "");

      if (row.grantorID_ == SYSTEM_AUTH_ID)
        grantStmt += "-- ";

      grantStmt += "GRANT ";
      grantStmt += withoutWGO;
      grantStmt += postfix;
      grantStmt += ";\n";
    }

    if (!withWGO.empty())
    {
      // remove last ','
      size_t commaPos = withWGO.find_last_of(",");
      if (commaPos != std::string::npos)
        withWGO.replace(commaPos, 1, "");
    
      if (row.grantorID_ == SYSTEM_AUTH_ID)
        grantStmt += "-- ";

      grantStmt += "GRANT ";
      grantStmt += withWGO;
      grantStmt += postfix;
      grantStmt += " WITH GRANT OPTION;\n";
    }

    privilegeText += grantStmt;
    grantStmt.clear();
    postfix.clear();
  }

  deleteRowsForGrantee(rowList);
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
// *  <userWGOPrivs> the list of grantable privileges is returned
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: Privileges were gathered
// *           *: Unable to gather privileges, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::getPrivsOnObjectForUser(
  const int64_t objectUID,
  const int32_t userID,
  std::bitset<NBR_OF_PRIVS> &userPrivs,
  std::bitset<NBR_OF_PRIVS> &grantablePrivs)
{
  PrivStatus retcode = STATUS_GOOD;

  if (objectUID == 0)
  {
    PRIVMGR_INTERNAL_ERROR("objectUID is 0 for get privileges command");
    return STATUS_ERROR;
  }

  objectUID_ = objectUID;
  PrivMgrDesc privsOfTheUser(userID);
  bool withRoles = true;
  retcode = getUserPrivs( userID, privsOfTheUser, withRoles );
  if (retcode != STATUS_GOOD)
    return retcode;

  userPrivs = privsOfTheUser.getTablePrivs().getPrivBitmap();
  grantablePrivs = privsOfTheUser.getTablePrivs().getWgoBitmap();

  return retcode;
}

// *****************************************************************************
// * Method: getUIDandPrivs                                
// *                                                       
// *    Reads OBJECT_PRIVILEGES table to obtain all privileges granted to the
// *    specified granteeID for all objects of the specified type.
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <granteeID> specifies the authID to gather privileges
// *  <objectType> specfies the type of object to gather privileges for.  
// *               COM_UNKNOWN_OBJECT indicates all object types.
// *  <UIDandPrivs> returns the list of objects and granted privileges as a vector list
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: Privileges were retrieved
// *           *: Unable to retrieve privileges, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::getUIDandPrivs(
   const int32_t granteeID,
   ComObjectType objectType,
   std::vector<UIDAndPrivs> & UIDandPrivs) 
   
{

std::string whereClause(" WHERE GRANTEE_ID = ");

   whereClause += authIDToString(granteeID);
   
   if (objectType != COM_UNKNOWN_OBJECT)
   {
      std::string objectLit = ObjectEnumToLit(objectType);
      
      whereClause += " AND OBJECT_TYPE = '";
      whereClause += objectLit;
      whereClause += "'";
   }
   
ObjectPrivsMDTable objectPrivsTable(fullTableName_,pDiags_);
std::vector<PrivMgrMDRow *> rowList;

PrivStatus privStatus = objectPrivsTable.selectWhere(whereClause,rowList);

   if (privStatus == STATUS_ERROR)
      return privStatus; 

   for (size_t i = 0; i < rowList.size();++i)
   {
      ObjectPrivsMDRow &row = static_cast<ObjectPrivsMDRow &>(*rowList[i]);
      
      UIDAndPrivs element;
      
      element.objectUID = row.objectUID_;
      element.privsBitmap = row.privsBitmap_;
      
      UIDandPrivs.push_back(element);
   }
   
   return STATUS_GOOD;
   
}


// *****************************************************************************
// * Method: getUserPrivs                                
// *                                                       
// *    Accumulates privileges for a user summarized over all grantors
// *    including PUBLIC
// *                                                       
// *  Parameters:    
// *                                                                       
// *  <granteeID> specifies the userID to accumulate
// *  <summarizedPrivs> contains the summarized privileges
// *  <withRoles> the summarized list includes roles granted to the userID
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: Privileges were gathered
// *           *: Unable to gather privileges, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::getUserPrivs(
  const int32_t granteeID,
  PrivMgrDesc &summarizedPrivs,
  const bool withRoles
  )
{
   PrivStatus retcode = STATUS_GOOD;
   PrivMgrDesc temp(granteeID);

   // Change code to get the row list and send in row list to getPrivsFromAllGrantors
   // Add chosen privs specifically for this grantee.
   retcode = getPrivsFromAllGrantors( objectUID_,
                                      granteeID,
                                      temp,
                                      withRoles
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
// *  <granteeID> specifies the userID to accumulate
// *  <summarizedPrivs> contains the summarized privileges
// *  <withRoles> the summarized list includes roles granted to the userID
// *                                                                     
// * Returns: PrivStatus                                               
// *                                                                  
// * STATUS_GOOD: Privileges were accumulated
// *           *: Unable to accumulate privileges, see diags.     
// *                                                               
// *****************************************************************************
PrivStatus PrivMgrPrivileges::getPrivsFromAllGrantors(
   const int64_t objectUID,
   const int32_t granteeID,
   PrivMgrDesc &summarizedPrivs,
   const bool withRoles
   )
{
  PrivStatus retcode = STATUS_GOOD;
  
  // Check to see if the granteeID is the system user
  // if so, the system user has all privileges.  Set up appropriately
  if (isSystemUser(granteeID))
  {
    PrivMgrBitmap bitmap;
    bitmap.set();
    PrivMgrCoreDesc coreTablePrivs(bitmap, bitmap);
    summarizedPrivs.setTablePrivs(coreTablePrivs);
    return STATUS_GOOD;
  }

  std::vector<PrivMgrMDRow *> rowList;
  retcode = getRowsForGrantee(objectUID, granteeID, withRoles, rowList);
  if (retcode == STATUS_ERROR)
    return retcode; 

  // Get the privileges for the object granted to the grantee
  PrivMgrCoreDesc coreTablePrivs;
  for (int32_t i = 0; i < rowList.size();++i)
  {
    ObjectPrivsMDRow &row = static_cast<ObjectPrivsMDRow &> (*rowList[i]);
    PrivMgrCoreDesc temp (row.privsBitmap_, row.grantableBitmap_);
    coreTablePrivs.unionOfPrivs(temp);
  }
  
  summarizedPrivs.setTablePrivs(coreTablePrivs);

  // TBD:  Add core privileges for columnLevel_
  
  deleteRowsForGrantee(rowList);
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
// *  <withRoles> the summarized list includes roles granted to the userID
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
  const bool withRoles,
  std::vector<PrivMgrMDRow *> &rowList)
{
  PrivStatus retcode = STATUS_GOOD;

  // set of list of IDs to access
  // Include -1 public user in the list
  std::vector<int32_t> listOfIDs;
  listOfIDs.push_back(PUBLIC_AUTH_ID);
  listOfIDs.push_back(granteeID);

  // add list of roles that the user has been granted
  if (withRoles)
  {
    PrivMgrRoles roles(" ",metadataLocation_, pDiags_);
    std::vector<std::string> roleNames;
    std::vector<int32_t> roleDepths;
    retcode = roles.fetchRolesForUser(granteeID, roleNames, listOfIDs, roleDepths);
    if (retcode == STATUS_ERROR)
     return retcode;
  }

  // Get the list of privileges for the object and all the UIDs
  std::string whereClause ("where object_uid = ");
  whereClause += UIDToString(objectUID);
  whereClause += " and grantee_id in (";
  for (size_t i = 0; i < listOfIDs.size(); i++)
  {
    whereClause += UIDToString(listOfIDs[i]);
    if (i+1 != listOfIDs.size())
      whereClause += ", ";
  }
  whereClause += ")"; 

  ObjectPrivsMDTable objectPrivsTable(fullTableName_, pDiags_);
  retcode = objectPrivsTable.selectWhere(whereClause, rowList);
  if (retcode == STATUS_ERROR)
  {
    deleteRowsForGrantee(rowList);
    return retcode;
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
// *  <withRoles> the summarized list includes roles granted to the userID
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
   const bool withRoles,
   const std::vector<ObjectUsage *> listOfChangedPrivs,
   PrivMgrDesc &summarizedOriginalPrivs,
   PrivMgrDesc &summarizedCurrentPrivs)
{
  PrivStatus retcode = STATUS_GOOD;

  // get OBJECT_PRIVILEGES rows where the grantee has received privileges
  std::vector<PrivMgrMDRow *> rowList;
  retcode = getRowsForGrantee(objectUID, granteeID, withRoles, rowList);

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
  deleteRowsForGrantee(rowList);

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

   int64_t rowCount = 0;   
   ObjectPrivsMDTable myTable(fullTableName_,pDiags_);

   PrivStatus privStatus = myTable.selectCountWhere(whereClause,rowCount);

   if ((privStatus == STATUS_GOOD || privStatus == STATUS_WARNING) &&
        rowCount > 0)
      return true;
      
   pDiags_->clear();
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
  const std::string objectType,
  const bool isAllSpecified,
  const bool isWgoSpecified,
  const bool isGOFSpecified,
  const std::vector<std::string> privsList,
  PrivMgrDesc &privsToProcess)
{

  // Categorize the objectType
  bool isLibrary = false;
  bool isUdr = false;
  bool isObject = false;
  bool isSequence = false;
  if (objectType == BASE_TABLE_OBJECT_LIT)
    isObject = true;
  else if (objectType == VIEW_OBJECT_LIT)
    isObject = true;
  else if (objectType == LIBRARY_OBJECT_LIT)
    isLibrary = true;
  else if (objectType == USER_DEFINED_ROUTINE_OBJECT_LIT)
    isUdr = true;
  else if (objectType == SEQUENCE_GENERATOR_OBJECT_LIT)
    isSequence = true;
  else
  {
    *pDiags_ << DgSqlCode (-4219)
            << DgString1 (objectType.c_str());
    return STATUS_ERROR;
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
  bool isDup = false;
  bool isIncompatible = false;
  std::string privStr;
  for (int32_t i = 0; i < privsList.size();++i)
  {
    privStr = privsList[i];
    if (privStr == "SELECT")
    {
      if (!isObject)
      {
        isIncompatible = true;
        break;
      }
      isDup = (tableCorePrivs.testAndSetBit(SELECT_PRIV, isWgoSpecified, isGOFSpecified)) ? false : true;    
    }
    else if (privStr == "INSERT")
    {
      if (!isObject)
      {
        isIncompatible = true;
        break;
      }
      isDup = (tableCorePrivs.testAndSetBit(INSERT_PRIV, isWgoSpecified, isGOFSpecified)) ? false : true;    
    }
    else if (privStr == "DELETE")
    {
      if (!isObject)
      {
        isIncompatible = true;
        break;
      }
      isDup = (tableCorePrivs.testAndSetBit(DELETE_PRIV, isWgoSpecified, isGOFSpecified)) ? false : true;    
    }
    else if (privStr == "UPDATE")
    {
      if (!isObject && !isLibrary)
      {
        isIncompatible = true;
        break;
      }
      isDup = (tableCorePrivs.testAndSetBit(UPDATE_PRIV, isWgoSpecified, isGOFSpecified)) ? false : true;    
    }
    else if (privStr == "USAGE")
    {
      if (!isLibrary && !isSequence)
      {
        isIncompatible = true;
        break;
      }
      isDup = (tableCorePrivs.testAndSetBit(USAGE_PRIV, isWgoSpecified, isGOFSpecified)) ? false : true;    
    }
    else if (privStr == "EXECUTE")
    {
      if (!isUdr)
      {
        isIncompatible = true;
        break;
      }
      isDup = (tableCorePrivs.testAndSetBit(EXECUTE_PRIV, isWgoSpecified, isGOFSpecified)) ? false : true;    
    }
    else if (privStr == "REFERENCES")
    {
      if (!isObject)
      {
        isIncompatible = true;
        break;
      }
      isDup = (tableCorePrivs.testAndSetBit(REFERENCES_PRIV, isWgoSpecified, isGOFSpecified)) ? false : true;    
    }
    else if (privStr == "ALL_DML")
    {
      if (!isObject)
      {
        isIncompatible = true;
        break;
      }
      // don't bother to check for duplicate privileges
      if (isGOFSpecified)
        tableCorePrivs.setWgo(ALL_DML, true);
      else
      {
        tableCorePrivs.setPriv(ALL_DML, true);
        tableCorePrivs.setWgo(ALL_DML, isWgoSpecified); 
      }
   }

   // Add privs CREATE, DROP, and ALTER
   // Report unknown privilege
    else 
    {
      *pDiags_ << DgSqlCode(-CAT_INVALID_PRIVILEGE_FOR_GRANT_OR_REVOKE)
              << DgString0(privStr.c_str());
      return STATUS_ERROR;
    }
  } // end for

  // Report error if privilege specified more than one
  if (isDup)
  {
    *pDiags_ << DgSqlCode (-CAT_DUPLICATE_PRIVILEGES);
    return STATUS_ERROR;
  }

  // Report error if privilege is incompatible with objectType
  if (isIncompatible)
  {
    *pDiags_ << DgSqlCode (-CAT_PRIVILEGE_NOT_ALLOWED_FOR_THIS_OBJECT_TYPE)
            << DgString0 (privStr.c_str());
    return STATUS_ERROR;
  }

  privsToProcess.setTablePrivs(tableCorePrivs);
  return STATUS_GOOD;      
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
   retcode = selectWhere(whereClause, rowList);
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
// *  <rowOut>  passes back a set of OBJECT_PRIVILEGES rows
// *                                                         
// * Returns: PrivStatus                                   
// *                                                      
// * STATUS_GOOD: Row returned.                          
// *           *: Select failed. A CLI error is put into the diags area. 
// *****************************************************************************
PrivStatus ObjectPrivsMDTable::selectWhere(
   const std::string & whereClause,
   std::vector<PrivMgrMDRow *> &rowList)
{
  std::string selectStmt ("SELECT OBJECT_UID, OBJECT_NAME, OBJECT_TYPE, ");
  selectStmt += ("GRANTEE_ID, GRANTEE_NAME, GRANTEE_TYPE, ");
  selectStmt += ("GRANTOR_ID, GRANTOR_NAME, GRANTOR_TYPE, ");
  selectStmt += ("PRIVILEGES_BITMAP, GRANTABLE_BITMAP FROM ");
  selectStmt += tableName_;
  selectStmt += " ";
  selectStmt += whereClause;
  selectStmt += " order by grantor_id, grantee_id";

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
    cliInterface.clearGlobalDiags();
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
  row.objectType_ = value;

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

  int64_t privilegesBitmapLong = row.getPrivilegesBitmap().to_ulong();
  int64_t grantableBitmapLong = row.getGrantableBitmap().to_ulong();
  sprintf(insertStmt, "insert into %s values (%ld, '%s', '%s', %ld, '%s', '%s', %ld, '%s', '%s', %ld, %ld)",
              tableName_.c_str(),
              row.getObjectUID(),
              row.getObjectName().c_str(),
              row.getObjectType().c_str(),
              row.getGrantee(),
              row.getGranteeName().c_str(),
              row.granteeType_.c_str(),
              row.getGrantor(),
              row.getGrantorName().c_str(),
              row.grantorType_.c_str(),
              privilegesBitmapLong,
              grantableBitmapLong);

  ExeCliInterface cliInterface(STMTHEAP);
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
// * method: ObjectPrivsMDTable::delete
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

  ExeCliInterface cliInterface(STMTHEAP);

  int32_t cliRC = cliInterface.executeImmediate(deleteStmt.c_str());
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return STATUS_ERROR;
    }


  if (cliRC == 100) // did not find any rows
  {
    cliInterface.clearGlobalDiags();
    return STATUS_NOTFOUND;
  }

  if (cliRC > 0)
    return STATUS_WARNING;

  return STATUS_GOOD;
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

  ExeCliInterface cliInterface(STMTHEAP);
  int32_t cliRC = cliInterface.executeImmediate(updateStmt.c_str());
  if (cliRC < 0)
    {
      cliInterface.retrieveSQLDiagnostics(CmpCommon::diags());
      return STATUS_ERROR;
    }

  if (cliRC == 100) // did not find any rows
  {
    cliInterface.clearGlobalDiags();
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
//    (select auth_db_name from AUTHS where auth_id = object_owner), --granteeName
//    USER_GRANTEE_LIT, -- "U"
//    SYSTEM_AUTH_ID,  -- system grantor ID (-2)
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
  ExeCliInterface cliInterface(STMTHEAP);
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
  std::string systemGrantor("_SYSTEM");

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
                "     when object_type = 'UR' then 0 "
                "     when object_type = 'SG' then %ld "
                " else 0 end", 
           tableBits, libraryBits, sequenceBits);
  std::string grantableClause(buf);

  sprintf(buf, "insert into %s select distinct object_uid, trim(catalog_name) || '.\"' || trim(schema_name) ||  '\".\"' || trim(object_name) || '\"', object_type, object_owner, (select auth_db_name from %s where auth_id = o.object_owner) as auth_db_name, '%s', %d, '%s', '%s', %s, %s from %s o where o.object_type in ('VI','BT','LB','UR','SG')",
              tableName_.c_str(),
              authsLocation.c_str(),
              USER_GRANTEE_LIT,
              -2, systemGrantor.c_str(), SYSTEM_GRANTOR_LIT,
              privilegesClause.c_str(), grantableClause.c_str(),
              objectsLocation.c_str());

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
    cliInterface.clearGlobalDiags();
    cliRC = 0;
  }

  // Make sure rows were inserted correctly.
  // Get the expected number of rows
 sprintf(buf, "select count(*) from %s o where o.object_type in ('VI','BT','LB','UR', 'SG')",
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

  Int64 rowsInserted = 0;
  ExeCliInterface cliInterface(STMTHEAP);
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
    cliInterface.clearGlobalDiags();
    cliRC = 0;
  }
 
  return STATUS_GOOD;
  
}




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
//// @@@ END COPYRIGHT @@@
//*****************************************************************************

#ifndef PRIVMGR_COMMANDS_H
#define PRIVMGR_COMMANDS_H

#include <string>
#include <vector>
#include <bitset>
#include <iterator>
#include "PrivMgrMD.h"
#include "PrivMgrDefs.h"
#include "TrafDDLdesc.h"
#include "ComSecurityKey.h"

class ComDiagsArea;
class ComSecurityKey;
struct TrafDesc;

// *****************************************************************************
// This file contains classes used by callers of privilege manager
//
//  Privilege manager (PrivMgr) is a component that manages metadata associated 
//  with privileges.  This includes granting/revoking object and component (and 
//  eventually column) privileges, returning privilege descriptions in the form
//  of grant statements for SHOWDDL, returning privileges on objects for 
//  specific users, and commands to support components
// *****************************************************************************

// Contents of file
class PrivMgrObjectInfo;
class ObjectPrivsRow;
class PrivMgrUserPrivs;
class PrivMgrCommands;

// Forward references
class PrivMgrPrivileges;
class NATable;

// ****************************************************************************
// *
// * Class:        PrivMgrObjectInfo
// * Description:  This class describes object details needed to perform 
// *               describe requests
// *
// ****************************************************************************
class PrivMgrObjectInfo
{
  public:

  PrivMgrObjectInfo( const int64_t objectUID,
                     const std::string objectName,
                     const int32_t objectOwner,
                     const int32_t schemaOwner,
                     const ComObjectType objectType)
  : objectOwner_ (objectOwner),
    objectName_ (objectName),
    schemaOwner_ (schemaOwner),
    objectUID_ (objectUID),
    objectType_ (objectType)
  {}


  PrivMgrObjectInfo(const NATable *naTable);

  const int32_t getObjectOwner()                  { return objectOwner_; }
  const std::string getObjectName()               { return objectName_; }
  const int32_t getSchemaOwner()                  { return schemaOwner_; }
  const int64_t getObjectUID()                    { return objectUID_; }
  const ComObjectType getObjectType()             { return objectType_; }
  const std::vector<std::string> &getColumnList() { return columnList_; }

  private:

  int64_t                  objectUID_;
  std::string              objectName_;
  int32_t                  objectOwner_;
  int32_t                  schemaOwner_;
  ComObjectType            objectType_;
  std::vector<std::string> columnList_;
};

// ****************************************************************************
// class: ObjectPrivsRow
//
// ****************************************************************************
class ObjectPrivsRow
{
public:
   char objectName[(MAX_SQL_IDENTIFIER_NAME_LEN*3) + 2 + 1];
   ComObjectType objectType;
   int32_t granteeID;
   char granteeName[MAX_USERNAME_LEN * 2 + 1];
   ComGranteeType granteeType;
   int32_t grantorID;
   char grantorName[MAX_USERNAME_LEN * 2 + 1];
   ComGrantorType grantorType;
   int64_t privilegesBitmap;
   int64_t grantableBitmap;
};


// *****************************************************************************
// *
// * Class:        PrivMgrUserPrivs
// * Description:  This class encapsulates privileges associated with an object
// *               for a user.
// *
// *****************************************************************************
class PrivMgrUserPrivs
{
  public:

  PrivMgrUserPrivs()
  : hasPublicPriv_(false)
  {}

  static std::string convertPrivTypeToLiteral(PrivType which)
  {
    std::string privilege;
    switch (which)
    {
      case SELECT_PRIV:
        privilege = "SELECT";
        break;
      case INSERT_PRIV:
        privilege = "INSERT";
        break;
      case DELETE_PRIV:
        privilege = "DELETE";
        break;
      case UPDATE_PRIV:
        privilege = "UPDATE";
        break;
      case USAGE_PRIV:
        privilege = "USAGE";
        break;
      case REFERENCES_PRIV:
        privilege = "REFERENCES";
        break;
      case EXECUTE_PRIV:
        privilege = "EXECUTE";
        break;
      case CREATE_PRIV:
        privilege = "CREATE";
        break;
      case ALTER_PRIV:
        privilege = "ALTER";
        break;
      case DROP_PRIV:
        privilege = "DROP";
        break;
      case ALL_DML:
        privilege = "ALL_DML";
        break;
      case ALL_DDL:
        privilege = "ALL_DDL";
        break;
      case ALL_PRIVS:
        privilege = "ALL";
        break;
      default:
        privilege = "UNKNOWN";
    }
  return privilege;
}

 
  // Object level
  bool hasObjectSelectPriv() const    {return objectBitmap_.test(SELECT_PRIV);}
  bool hasObjectInsertPriv() const    {return objectBitmap_.test(INSERT_PRIV);}
  bool hasObjectDeletePriv() const    {return objectBitmap_.test(DELETE_PRIV);}
  bool hasObjectUpdatePriv() const    {return objectBitmap_.test(UPDATE_PRIV);}
  bool hasObjectUsagePriv() const     {return objectBitmap_.test(USAGE_PRIV);}
  bool hasObjectReferencePriv() const {return objectBitmap_.test(REFERENCES_PRIV);}
  bool hasObjectExecutePriv() const   {return objectBitmap_.test(EXECUTE_PRIV);}
  bool hasObjectAlterPriv() const     {return objectBitmap_.test(ALTER_PRIV);}
  bool hasObjectDropPriv() const      {return objectBitmap_.test(DROP_PRIV);}
  bool hasSelectPriv() const    {return schemaPrivBitmap_.test(SELECT_PRIV) || objectBitmap_.test(SELECT_PRIV);}
  bool hasInsertPriv() const    {return schemaPrivBitmap_.test(INSERT_PRIV) || objectBitmap_.test(INSERT_PRIV);}
  bool hasDeletePriv() const    {return schemaPrivBitmap_.test(DELETE_PRIV) || objectBitmap_.test(DELETE_PRIV);}
  bool hasUpdatePriv() const    {return schemaPrivBitmap_.test(UPDATE_PRIV) || objectBitmap_.test(UPDATE_PRIV);}
  bool hasUsagePriv() const     {return schemaPrivBitmap_.test(USAGE_PRIV) || objectBitmap_.test(USAGE_PRIV);}
  bool hasReferencePriv() const {return schemaPrivBitmap_.test(REFERENCES_PRIV) || objectBitmap_.test(REFERENCES_PRIV);}
  bool hasExecutePriv() const   {return schemaPrivBitmap_.test(EXECUTE_PRIV) || objectBitmap_.test(EXECUTE_PRIV);}
  bool hasCreatePriv() const    {return schemaPrivBitmap_.test(CREATE_PRIV);}
  bool hasAlterPriv() const     {return schemaPrivBitmap_.test(ALTER_PRIV) || objectBitmap_.test(ALTER_PRIV);}
  bool hasDropPriv() const      {return schemaPrivBitmap_.test(DROP_PRIV) || objectBitmap_.test(DROP_PRIV);}
  bool hasAllObjectPriv() const {return objectBitmap_.all();}
  bool hasAnyObjectPriv() const {return objectBitmap_.any();}
  bool hasPriv(PrivType which) const
  {
    bool hasPriv = false;
    switch (which)
    {
      case SELECT_PRIV:
        hasPriv = hasSelectPriv();
        break;
      case INSERT_PRIV:
        hasPriv = hasInsertPriv();
        break;
      case DELETE_PRIV:
        hasPriv = hasDeletePriv();
        break;
      case UPDATE_PRIV:
        hasPriv = hasUpdatePriv();
        break;
      case USAGE_PRIV:
        hasPriv = hasUsagePriv();
        break;
      case REFERENCES_PRIV:
        hasPriv = hasReferencePriv();
        break;
      case EXECUTE_PRIV:
        hasPriv = hasExecutePriv();
        break;

      // other privileges defined in the PrivType enum are not yet supported
      default:
        hasPriv = false;
    }
    return hasPriv;
  }

  bool hasObjectPriv(PrivType which) const
  {
    bool hasPriv = false;
    switch (which)
    {
      case SELECT_PRIV:
        hasPriv = hasObjectSelectPriv();
        break;
      case INSERT_PRIV:
        hasPriv = hasObjectInsertPriv();
        break;
      case DELETE_PRIV:
        hasPriv = hasObjectDeletePriv();
        break;
      case UPDATE_PRIV:
        hasPriv = hasObjectUpdatePriv();
        break;
      case USAGE_PRIV:
        hasPriv = hasObjectUsagePriv();
        break;
      case REFERENCES_PRIV:
        hasPriv = hasObjectReferencePriv();
        break;
      case EXECUTE_PRIV:
        hasPriv = hasObjectExecutePriv();
        break;

      // other privileges defined in the PrivType enum are not yet supported
      default:
        hasPriv = false;
    }
    return hasPriv;
  }

  bool hasAllDMLPriv() const
  {
    return (hasSelectPriv() && 
            hasInsertPriv() && 
            hasDeletePriv() && 
            hasUpdatePriv() && 
            hasReferencePriv());
  } 
            
  bool hasAllLibraryPriv() const
  { return (hasUpdatePriv() && hasUsagePriv()); }

  bool hasAllUdrPriv() const
  { return hasExecutePriv(); }

  bool hasAllDDLPriv() const
   {return (hasCreatePriv() && hasAlterPriv() && hasDropPriv());}

  bool hasWGOOption(PrivType privType) const 
   {return grantableBitmap_.test(privType);}

  bool hasColSelectPriv(const int32_t ordinal) const {return hasColPriv(SELECT_PRIV,ordinal);}
  bool hasColInsertPriv(const int32_t ordinal) const {return hasColPriv(INSERT_PRIV,ordinal);}
  bool hasColUpdatePriv(const int32_t ordinal) const {return hasColPriv(UPDATE_PRIV,ordinal);}
  bool hasColReferencePriv(const int32_t ordinal) const {return hasColPriv(REFERENCES_PRIV,ordinal);}
  bool hasAnyColPriv() const       
  {
     return (!colPrivsList_.empty());
  }
  
  bool hasAnyColPriv(const PrivType privType) const       
  {

     PrivColIterator columnIterator;
     for (columnIterator = colPrivsList_.begin();
          columnIterator != colPrivsList_.end(); ++columnIterator)
     {
        if (columnIterator->second.test(privType))
           return true;
     
     }
     
     return false;     
  }


  bool hasColPriv(PrivType privType,const int32_t ordinal) const
  {
    // If no privileges for that column, return false.
    if (colPrivsList_.count(ordinal) <= 0)
       return false;
       
    switch (privType)
    {
      case SELECT_PRIV:
      case INSERT_PRIV:
      case REFERENCES_PRIV:
      case UPDATE_PRIV:
      {
        PrivColIterator columnIterator = colPrivsList_.find(ordinal);
        if (columnIterator == colPrivsList_.end())
           return false;
        
        return columnIterator->second.test(privType);
        break;
      }
      // other privileges not column privs
      default:
        return false;
    }
    return false;
  }

  PrivColList & getColPrivList() {return colPrivsList_;}
  void setColPrivList(PrivColList colPrivsList)
     {colPrivsList_ = colPrivsList;}
  
  PrivColList & getColGrantableList() {return colGrantableList_;}
  void setColGrantableList(PrivColList colGrantableList)
     {colGrantableList_ = colGrantableList;}
 
  PrivColumnBitmap getColumnPrivBitmap(const int32_t ordinal) 
  {
     if (colPrivsList_.empty() || colPrivsList_.count(ordinal) == 0)
        return emptyBitmap_;
  
     return colPrivsList_[ordinal];
  }

  PrivColumnBitmap getColumnGrantableBitmap(const int32_t ordinal) 
  {
     if (colGrantableList_.empty() || colGrantableList_.count(ordinal) == 0)
        return emptyBitmap_;
  
     return colGrantableList_[ordinal];
  }

  PrivMgrBitmap getObjectBitmap() {return objectBitmap_;}
  void setObjectBitmap (PrivMgrBitmap objectBitmap)
     {objectBitmap_ = objectBitmap;}

  PrivMgrBitmap getGrantableBitmap() {return grantableBitmap_;}
  void setGrantableBitmap (PrivMgrBitmap grantableBitmap)
     {grantableBitmap_ = grantableBitmap;}

  void setOwnerDefaultPrivs() 
     { objectBitmap_.set(); grantableBitmap_.set(); } 
     
  PrivSchemaBitmap getSchemaPrivBitmap() {return schemaPrivBitmap_;}
  void setSchemaPrivBitmap (PrivSchemaBitmap schemaPrivBitmap)
     {schemaPrivBitmap_ = schemaPrivBitmap;}

  PrivSchemaBitmap getSchemaGrantableBitmap() {return schemaGrantableBitmap_;}
  void setSchemaGrantableBitmap (PrivSchemaBitmap schemaGrantableBitmap)
     {schemaGrantableBitmap_ = schemaGrantableBitmap;}

  bool getHasPublicPriv() { return hasPublicPriv_; }
  void setHasPublicPriv(bool hasPublicPriv) {hasPublicPriv_ = hasPublicPriv;}
  void initUserPrivs (PrivMgrDesc &privsOfTheGrantor);
  bool initUserPrivs ( const std::vector<int32_t> &roleIDs,
                       const TrafDesc *priv_desc,
                       const int32_t userID,
                       const int64_t objectUID,
                       NASet<ComSecurityKey> & secKeySet);

 private:
   PrivObjectBitmap objectBitmap_;
   PrivObjectBitmap grantableBitmap_;
   PrivColList colPrivsList_;
   PrivColList colGrantableList_;
   PrivSchemaBitmap schemaPrivBitmap_;
   PrivSchemaBitmap schemaGrantableBitmap_;
   PrivColumnBitmap emptyBitmap_;
   bool hasPublicPriv_;
};


// *****************************************************************************
// *
// * Class:        PrivMgrCommands
// * Description:  This class represents the commands that can be performed 
// *               through the privilege manager component
// *
// *****************************************************************************

class PrivMgrCommands : public PrivMgr
{
public:

// ---------------------------------------------------------------------
// Constructors/Destructor
// ---------------------------------------------------------------------
   PrivMgrCommands ();
   PrivMgrCommands ( 
      const std::string trafMetadataLocation,
      const std::string &metadataLocation,
      ComDiagsArea *pDiags,
      PrivMDStatus authorizationEnabled = PRIV_INITIALIZE_UNKNOWN );
   PrivMgrCommands ( const std::string &metadataLocation 
                   , ComDiagsArea *pDiags
                   , PrivMDStatus authorizationEnabled = PRIV_INITIALIZE_UNKNOWN );
   PrivMgrCommands ( const PrivMgrCommands &rhs ); 
   virtual ~PrivMgrCommands ( void );

   // ------------------------------------------------------------------------
   // Operations:
   // ------------------------------------------------------------------------
   bool authorizationEnabled();

   PrivStatus createComponentOperation(
      const std::string & componentName,
      const std::string & operationName,
      const std::string & operationCode,
      bool isSystem,
      const std::string & operationDescription);
      
   bool describeComponents(const std::string & componentName,  
                           std::vector<std::string> & outlines);
    
   bool describePrivileges(
      const PrivMgrObjectInfo &objectInfo,
      std::string &privilegeText);

   PrivStatus dropAuthorizationMetadata(bool doCleanup);
   
   PrivStatus dropComponentOperation(
      const std::string & componentName,
      const std::string & operationName,
      PrivDropBehavior dropBehavior);
   
   PrivStatus getGrantorDetailsForObject(
      const bool isGrantedBySpecified,
      const std::string grantedByName,
      const int_32 objectOwner,
      int_32 &effectiveGrantorID,
      std::string &effectiveGrantorName);

   PrivStatus getPrivileges(
      NATable *naTable,
      const int32_t granteeUID,
      PrivMgrUserPrivs &userPrivileges,
      std::vector <ComSecurityKey *>* secKeySet = NULL);

   PrivStatus getPrivileges(
      const int64_t objectUID,
      ComObjectType objectType,
      std::vector<PrivMgrDesc> &userPrivileges);
     
   PrivStatus getPrivileges(
      const int64_t objectUID,
      ComObjectType objectType,
      const int32_t granteeUID,
      PrivMgrUserPrivs &userPrivileges,
      std::vector <ComSecurityKey *>* secKeySet = NULL);
     
   PrivStatus getPrivRowsForObject(
      const int64_t objectUID,
      std::vector<ObjectPrivsRow> & objectPrivsRows);
      
   PrivStatus getRoles(
      const int32_t grantee,
      std::vector<int32_t> &roleIDs);

   PrivStatus givePrivForObjects(
         const int32_t currentOwnerID,
         const int32_t newOwnerID,
         const std::string &newOwnerName,
         const std::vector<int64_t> &objectUIDs);
         
   PrivStatus grantComponentPrivilege(
      const std::string & componentName,
      const std::vector<std::string> & operationNamesList,
      const int32_t grantorID,
      const std::string & grantorName,
      const int32_t granteeID,
      const std::string & granteeName,
      const int32_t grantDepth);
      
   PrivStatus grantObjectPrivilege(
      const int64_t objectUID,
      const std::string &objectName,
      const ComObjectType objectType,
      const int32_t granteeUID,
      const std::string &granteeName,
      const int32_t grantorUID,
      const std::string &grantorName,
      const std::vector<PrivType> &privs,
      const std::vector<ColPrivSpec> & colPrivsArray,
      const bool isAllSpecified,
      const bool isWGOSpecified);
      
   PrivStatus grantObjectPrivilege(
      const int64_t objectUID,
      const std::string &objectName,
      const ComObjectType objectType,
      const int32_t grantorUID,
      const int32_t granteeUID,
      const PrivMgrBitmap &objectPrivs,
      const PrivMgrBitmap &grantablePrivs);

   PrivStatus grantRole(
      const std::vector<int32_t> & roleIDs,
      const std::vector<std::string> & roleNames,
      const std::vector<int32_t> & grantorIDs,
      const std::vector<std::string> & grantorNames,
      PrivAuthClass grantorClass,
      const std::vector<int32_t> & granteeIDs,
      const std::vector<std::string> & granteeNames,
      const std::vector<PrivAuthClass> & granteeClasses,
      const int32_t grantDepth);
   
   PrivStatus initializeAuthorizationMetadata(
      const std::string &objectsLocation,
      const std::string &authsLocation,
      const std::string &colsLocation,
      std::vector<std::string> &tablesCreated,
      std::vector<std::string> &tablesUpgraded);

   PrivStatus insertPrivRowsForObject(
      const int64_t objectUID,
      const std::vector<ObjectPrivsRow> & objectPrivsRows);
   
   bool isPrivMgrTable(const std::string &objectName);
 
   PrivStatus registerComponent(
      const std::string &componentName,
      const bool isSystem,
      const std::string &componentDetails);
      
   PrivStatus revokeComponentPrivilege(
      const std::string & componentName,
      const std::vector<std::string> & operationNamesList,
      const int32_t grantorID,
      const int32_t granteeID,
      const bool isGOFSpecified,
      PrivDropBehavior dropBehavior); 
      
   PrivStatus revokeObjectPrivilege(
      const int64_t objectUID,
      const std::string &objectName,
      const  ComObjectType objectType,
      const int32_t granteeUID,
      const std::string &granteeName,
      const int32_t grantorUID,
      const std::string &grantorName,
      const std::vector<PrivType> &privs,
      const std::vector<ColPrivSpec> & colPrivsArray,
      const bool isAllSpecified,
      const bool isGOFSpecified);
      
  PrivStatus revokeObjectPrivilege(
      const int64_t objectUID,
      const std::string &objectName,
      const int32_t grantorUID);

   PrivStatus revokeRole(
      const std::vector<int32_t> & roleIDs,
      const std::vector<int32_t> & granteeIDs,
      const std::vector<PrivAuthClass> & granteeClasses,
      const std::vector<int32_t> & grantorIDs,
      const bool isGOFSpecified,
      const int32_t newGrantDepth,
      PrivDropBehavior dropBehavior);      

   PrivStatus unregisterComponent(
      const std::string & componentName,
      PrivDropBehavior dropBehavior);

    // -------------------------------------------------------------------
    // Accessors:
    // -------------------------------------------------------------------
    inline ComDiagsArea *    getDiags (void) {return pDiags_;}
    inline const std::string getMetadataLocation (void) {return metadataLocation_;}
    inline const std::string getTrafMetadataLocation (void) { return trafMetadataLocation_;}

  protected:

    // -------------------------------------------------------------------
    // Mutators:
    // -------------------------------------------------------------------
    inline void setDiags (ComDiagsArea *pDiags) {pDiags_ = pDiags;}
    inline void setMetadataLocation (std::string &metadataLocation)
       { metadataLocation_ = metadataLocation; }
    inline void setTrafMetadataLocation (std::string &trafMetadataLocation)
       { trafMetadataLocation_ = trafMetadataLocation; }


  private:

    // -------------------------------------------------------------------
    // Private accessors:
    // -------------------------------------------------------------------
    PrivMgrCommands& operator=(const PrivMgrCommands& other);
    
    
}; // class PrivMgrCommands

#endif


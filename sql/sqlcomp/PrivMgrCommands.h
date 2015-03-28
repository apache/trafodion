//*****************************************************************************
// @@@ START COPYRIGHT @@@
//
//// (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
////
////  Licensed under the Apache License, Version 2.0 (the "License");
////  you may not use this file except in compliance with the License.
////  You may obtain a copy of the License at
////
////      http://www.apache.org/licenses/LICENSE-2.0
////
////  Unless required by applicable law or agreed to in writing, software
////  distributed under the License is distributed on an "AS IS" BASIS,
////  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
////  See the License for the specific language governing permissions and
////  limitations under the License.
////
//// @@@ END COPYRIGHT @@@
//*****************************************************************************

#ifndef PRIVMGR_COMMANDS_H
#define PRIVMGR_COMMANDS_H

#include <string>
#include <vector>
#include <bitset>
#include "PrivMgrMD.h"
#include "PrivMgrDefs.h"

class ComDiagsArea;
class ComSecurityKey;

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
class PrivMgrUserPrivs;
class PrivMgrPrivileges;

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

  PrivMgrUserPrivs(const int32_t nbrCols = 0){};

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

      // other privileges defined in PrivMgrDefs.h are not yet supported
      default:
        privilege = "UNKNOWN";
    }
  return privilege;
}

 
  // Object level
  bool hasSelectPriv() const    {return objectBitmap_.test(SELECT_PRIV);}
  bool hasInsertPriv() const    {return objectBitmap_.test(INSERT_PRIV);}
  bool hasDeletePriv() const    {return objectBitmap_.test(DELETE_PRIV);}
  bool hasUpdatePriv() const    {return objectBitmap_.test(UPDATE_PRIV);}
  bool hasUsagePriv() const     {return objectBitmap_.test(USAGE_PRIV);}
  bool hasReferencePriv() const {return objectBitmap_.test(REFERENCES_PRIV);}
  bool hasExecutePriv() const   {return objectBitmap_.test(EXECUTE_PRIV);}
  bool hasCreatePriv() const    {return objectBitmap_.test(CREATE_PRIV);}
  bool hasAlterPriv() const     {return objectBitmap_.test(ALTER_PRIV);}
  bool hasDropPriv() const      {return objectBitmap_.test(DROP_PRIV);}
  bool hasAllPriv() const       {return objectBitmap_.all();}
  bool hasAnyPriv() const       {return objectBitmap_.any();}
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

  // TBD - Column level
  bool hasColSelectPriv(const int32_t ordinal) const;
  bool hasColInsertPriv(const int32_t ordinal) const;
  bool hasColDeletePriv(const int32_t ordinal) const;
  bool hasColUpdatePriv(const int32_t ordinal) const;
  bool hasColReferencePriv(const int32_t ordinal) const;

  PrivMgrBitmap getObjectBitmap() {return objectBitmap_;}
  void setObjectBitmap (PrivMgrBitmap objectBitmap)
     {objectBitmap_ = objectBitmap;}

  PrivMgrBitmap getGrantableBitmap() {return grantableBitmap_;}
  void setGrantableBitmap (PrivMgrBitmap grantableBitmap)
     {grantableBitmap_ = grantableBitmap;}

  void setOwnerDefaultPrivs() 
     { objectBitmap_.set(); grantableBitmap_.set(); } 

 private:
   std::bitset<NBR_OF_PRIVS> objectBitmap_;
   std::vector <std::bitset<NBR_OF_PRIVS>> columnBitmaps_;
   std::bitset<NBR_OF_PRIVS> grantableBitmap_;
   std::vector <std::bitset<NBR_OF_PRIVS>>columnGrantableBitmaps_;
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
      const int64_t objectUID,
      const std::string &objectName,
      std::string &privilegeText);

   PrivStatus dropAuthorizationMetadata();
   
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
      const int64_t objectUID,
      const int32_t granteeUID,
      PrivMgrUserPrivs &userPrivileges,
      std::vector <ComSecurityKey *>* secKeySet = NULL);
     
   PrivStatus getPrivRowsForObject(
      const int64_t objectUID,
      std::vector<ObjectPrivsRow> & objectPrivsRows);
      
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
      const std::vector<std::string> &privs,
      const bool isAllSpecified,
      const bool isWGOSpecified);
      
   PrivStatus grantObjectPrivilege(
      const int64_t objectUID,
      const std::string &objectName,
      const ComObjectType objectType,
      const int32_t granteeUID,
      const std::string &granteeName,
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
      const std::string &authsLocation);

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
      const int32_t grantorUID,
      const std::vector<std::string> &privs,
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

   PrivStatus upgradeAuthorizationMetadata();
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


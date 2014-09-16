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
#include "PrivMgrRoles.h"
  
#include "PrivMgrMD.h"
#include "PrivMgrMDTable.h"

#include <string>
#include <cstdio>
#include <vector>
#include "ComSmallDefs.h"

// sqlcli.h included because ExExeUtilCli.h needs it (and does not include it!)
#include "sqlcli.h"
#include "ExExeUtilCli.h"
#include "ComDiags.h"
#include "ComQueue.h"
// CmpCommon.h contains STMTHEAP declaration
#include "CmpCommon.h"
#include "CmpDDLCatErrorCodes.h"
#include "ComUser.h"

static bool hasValue(
   std::vector<int32_t> container,
   int32_t value);

namespace Roles 
{
// *****************************************************************************
// * Class:        MyRow
// * Description:  This class represents a row in the ROLE_USAGE table.
// *****************************************************************************
class MyRow : public PrivMgrMDRow
{
public:
// -------------------------------------------------------------------
// Constructors and destructors:
// -------------------------------------------------------------------
   MyRow(std::string tableName)
   : PrivMgrMDRow(tableName),
     roleID_(0)
   { };
   MyRow(const MyRow &other)
   : PrivMgrMDRow(other)
   {
      roleID_ = other.roleID_;              
      roleName_ = other.roleName_;
      granteeID_ = other.granteeID_;
      granteeName_ = other.granteeName_;
      granteeAuthClass_ = other.granteeAuthClass_;
      grantorID_ = other.grantorID_;
      grantorName_ = other.grantorName_;
      grantorAuthClass_ = other.grantorAuthClass_;
      grantDepth_ = other.grantDepth_;
   };
   virtual ~MyRow() {};
    
// -------------------------------------------------------------------
// Data Members:
// -------------------------------------------------------------------
//  From ROLE_USAGE
    int32_t            roleID_;
    std::string        roleName_;
    int32_t            granteeID_;
    std::string        granteeName_;
    PrivAuthClass      granteeAuthClass_;
    int32_t            grantorID_;
    std::string        grantorName_;
    PrivAuthClass      grantorAuthClass_;
    int32_t            grantDepth_;
    
private: 
   MyRow();
  
};


// *****************************************************************************
// * Class:        MyTable
// * Description:  This class represents the ROLE_USAGE table.
// *                
// *****************************************************************************
class MyTable : public PrivMgrMDTable
{
public:
   MyTable(
      const std::string & tableName,
      ComDiagsArea * pDiags = NULL) 
   : PrivMgrMDTable(tableName,pDiags),
     lastRowRead_(tableName)
    {};
   
   virtual PrivStatus insert(const PrivMgrMDRow &row);
   
   PrivStatus insertSelect(const std::string & authsLocation); 
   
   PrivStatus selectAllWhere(
      const std::string & whereClause,
      const std::string & orderByClause,
      std::vector<MyRow> & rows);
      
   virtual PrivStatus selectWhereUnique(
      const std::string & whereClause,
      PrivMgrMDRow & row);
      
   void setRow(
      OutputInfo &cliInterface,
      PrivMgrMDRow &rowOut);
      
private:   
   MyTable();
   MyRow lastRowRead_;

};
}//End namespace Roles
using namespace Roles;

// *****************************************************************************
//    PrivMgrRoles methods
// *****************************************************************************

// -----------------------------------------------------------------------
// Construct a PrivMgrRoles object 
// -----------------------------------------------------------------------
PrivMgrRoles::PrivMgrRoles(
   const std::string & metadataLocation,
   ComDiagsArea * pDiags)
: PrivMgr(metadataLocation,pDiags),
  fullTableName_(metadataLocation_ + ".ROLE_USAGE"),
  myTable_(*new MyTable(fullTableName_,pDiags)) 
{ };

// -----------------------------------------------------------------------
// Copy constructor
// -----------------------------------------------------------------------
PrivMgrRoles::PrivMgrRoles(const PrivMgrRoles &other)
: PrivMgr(other),
  myTable_(*new MyTable(fullTableName_,pDiags_))
{
   fullTableName_ = other.fullTableName_;
}

// -----------------------------------------------------------------------
// Destructor.
// -----------------------------------------------------------------------
PrivMgrRoles::~PrivMgrRoles() 
{ }

// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrRoles::dependentObjectsExist                             *
// *                                                                           *
// *    Determines if a specific user owns objects whose existence depend upon *
// * one or more privileges granted to the specified role.  Used to determine  *
// * if a role can be revoked from a user without consequence.                 *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <userID>                        const int32_t                   In       *
// *    is the authorization ID of the user.                                   *
// *                                                                           *
// *  <roleID>                        const int32_t                   In       *
// *    is the role ID.                                                        *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true: One or more objects exist due to grants to role.                    *
// * false: No user objects are dependent on privileges granted to this role.  *
// *                                                                           *
// *****************************************************************************
bool PrivMgrRoles::dependentObjectsExist(
   int32_t userID,
   int32_t roleID) 

{

//TODO: Scan objects owned by user to see if user has privilege to create
// the object without any privileges granted to the role.
// Some options: 
//    Short circuit if role has no granted privileges.
//    Short circuit if user owns no objects (might be part of normal algorithm).
// When locking is available, need to lock records related to the revoke.
// This could be a long operation, could lock out other operations, especially
// if following multi-branch grant trees.  

   return false;


}
//**************** End of PrivMgrRoles::dependentObjectsExist ******************


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrRoles::fetchRolesForUser                                 *
// *                                                                           *
// *    Returns all unique roles granted to an authorization ID.  If a role is *
// * granted more than once, it is only returned one time.  If one or more of  *
// * the grants was WITH ADMIN OPTION, the grantDepth will reflect that.       *
// * Therefore, a grant depth of zero means none of the grants of the role to  *
// * the authorization ID included WITH GRANT OPTION and the auth ID cannot    *
// * grant the role.                                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <authID>                        const int32_t                   In       *
// *    is the authorization ID whose roles are to be fetched.                 *
// *                                                                           *
// *  <roleNames>                     std::vector<std::string> &      Out      *
// *    passes back a list of the names of roles granted to the                *
// *    authorization ID.                                                      *
// *                                                                           *
// *  <roleIDs>                    std::vector<int32_t> &          Out         *
// *    passes back a list of role IDs granted to the authorization ID.        *
// *                                                                           *
// *  <grantDepths>                   std::vector<int32_t> &          Out      *
// *    passes back a list of grantDepths corresponding to roleNames and IDs.  *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Grants for authorization ID were returned.                   *
// *           *: Grants for authorization ID were not returned due to SQL     *
// *              error.  The error is put into the diags area.                *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrRoles::fetchRolesForUser(
   const int32_t authID,
   std::vector<std::string> & roleNames,
   std::vector<int32_t> & roleIDs,
   std::vector<int32_t> & grantDepths)

{

std::string whereClause(" WHERE GRANTEE_ID = ");

char authIDString[20];

   sprintf(authIDString,"%d",authID);
   
   whereClause += authIDString;
   
std::vector<MyRow> rows;
MyTable &myTable = static_cast<MyTable &>(myTable_);

std::string orderByClause(" ORDER BY ROLE_ID");

PrivStatus privStatus = myTable.selectAllWhere(whereClause,orderByClause,rows);
   
   if (privStatus != STATUS_GOOD && privStatus != STATUS_WARNING)
      return privStatus;
   
   for (size_t r = 0; r < rows.size(); r++)
   {
      MyRow &row = rows[r];
      
      if (hasValue(roleIDs,row.roleID_))
      {
         if (grantDepths.back() == 0)
            grantDepths.back() = row.grantDepth_;
         continue;
      }
      roleNames.push_back(row.granteeName_);
      roleIDs.push_back(row.roleID_);
      grantDepths.push_back(row.grantDepth_);
   }

   return STATUS_GOOD;

}
//****************** End of PrivMgrRoles::fetchRolesForUser ********************

// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrRoles::fetchUsersForRole                                 *
// *                                                                           *
// *    Returns all grantees of a role.                                        *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <roleID>                        const int32_t                   In       *
// *    is the role ID.                                                        *
// *                                                                           *
// *  <granteeNames>                  std::vector<std::string> &      Out      *
// *    passes back a list of names of grantees for a role.                    *
// *                                                                           *
// *  <grantorIDs>                    std::vector<int32_t> &          Out      *
// *    passes back a list of grantorIDs corresponding to granteeNames.        *
// *                                                                           *
// *  <grantDepths>                   std::vector<int32_t> &          Out      *
// *    passes back a list of grantDepths corresponding to granteeNames.       *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Grants for role were returned.                               *
// *           *: Grants for roles were not returned due to SQL error.         *
// *              A CLI error is put into the diags area.                      *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrRoles::fetchUsersForRole(
   const int32_t roleID,
   std::vector<std::string> & granteeNames,
   std::vector<int32_t> & grantorIDs,
   std::vector<int32_t> & grantDepths)

{

std::string whereClause(" WHERE ROLE_ID = ");

char roleIDString[20];

   sprintf(roleIDString,"%d",roleID);
   
   whereClause += roleIDString;
   
std::vector<MyRow> rows;
MyTable &myTable = static_cast<MyTable &>(myTable_);

std::string orderByClause(" ORDER BY GRANTEE_NAME");

PrivStatus privStatus = myTable.selectAllWhere(whereClause,orderByClause,rows);
   
   if (privStatus != STATUS_GOOD && privStatus != STATUS_WARNING)
      return privStatus;
   
   for (size_t r = 0; r < rows.size(); r++)
   {
      MyRow &row = rows[r];
      
      granteeNames.push_back(row.granteeName_);
      grantorIDs.push_back(row.grantorID_);
      grantDepths.push_back(row.grantDepth_);
   }

   return STATUS_GOOD;

}
//****************** End of PrivMgrRoles::fetchUsersForRole ********************


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrRoles::grantRole                                         *
// *                                                                           *
// *    Grants one or more roles to one or more authIDs.                       *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <roleIDs>                       const std::vector<int32_t> &     In      *
// *    is a vector of roleIDs.  The caller is responsible for ensuring all    *
// *  IDs are unique.                                                          *
// *                                                                           *
// *  <roleNames>                     const std::vector<std::string> & In      *
// *    is a list of role names.  The elements in <roleIDs> must correspond    *
// *  to the elements in <roleNames>, i.e., roleIDs[n] is the role ID for      *
// *  the role with name roleNames[n].                                         *
// *                                                                           *
// *  <grantorIDs>                    const std::vector<int32_t> &     In      *
// *    is a vector of authIDs granting the role.  The elements of the vector  *
// *  are for the corresponding role entry.                                    *
// *                                                                           *
// *  <grantorNames>                  const std::vector<std::string> & In      *
// *    is a list of names of the authID granting the role in the              *
// *  corresponding position in the role vectors.                              *
// *                                                                           *
// *  <grantorClass                   PrivAuthClass                    In      *
// *    is the auth class of the authID granting the role.  Currently only     *
// *  user is supported.                                                       *
// *                                                                           *
// *  <granteeIDs>                    const std::vector<int32_t> &     In      *
// *    is a vector of authIDs being granted the role.  The caller is          *
// *  responsible for ensuring all IDs are unique.                             *
// *                                                                           *
// *  <granteeNames>                  const std::vector<std::string> & In      *
// *    is a list of grantee names.  The elements in <grantees> must correspond*
// *  to the elements in <granteeNames>, i.e., grantees[n] is the numeric      *
// *  auth ID for authorization ID with the name granteeNames[n].              *
// *                                                                           *
// *  <granteeClasses>                const std::vector<PrivAuthClass> & In    *
// *     is a list of classes for each grantee.  List must correspond to       *
// *  <grantees>.  Currently only user is supported.                           *
// *                                                                           *
// *  <grantDepth>                    const int64_t                    In      *
// *    is the number of levels this role may be granted by the grantee.       *
// *  Initially this is either 0 or -1.                                        *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Roles were granted.                                          *
// *           *: One or more roles were not granted.                          *
// *              A CLI error is put into the diags area.                      *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrRoles::grantRole(
   const std::vector<int32_t> & roleIDs,
   const std::vector<std::string> & roleNames,
   const std::vector<int32_t> & grantorIDs,
   const std::vector<std::string> & grantorNames,
   PrivAuthClass grantorClass,
   const std::vector<int32_t> & granteeIDs,
   const std::vector<std::string> & granteeNames,
   const std::vector<PrivAuthClass> & granteeClasses,
   const int32_t grantDepth) 
     
{

bool roleWasGranted = false;
MyTable &myTable = static_cast<MyTable &>(myTable_);

   for (size_t r = 0; r < roleIDs.size(); r++)
   {
      // For each role ID we loop through the list of grantees.
      // Most of the WHERE clause is known for this role (only 
      // difference is the grantee), so build the header now.
      int32_t roleID = roleIDs[r]; 
      std::string whereClauseHeader(" WHERE ROLE_ID = ");
      
      char roleIDString[20];
      sprintf(roleIDString,"%d",roleID);
      
      whereClauseHeader += roleIDString;
      
      char grantorIDString[20];
      sprintf(grantorIDString,"%d",grantorIDs[r]);
      
      whereClauseHeader += " AND GRANTOR_ID = ";
      whereClauseHeader += grantorIDString;
      whereClauseHeader += " AND GRANTEE_ID = ";
           
      for (size_t g = 0; g < granteeIDs.size(); g++)
      {
         // Currently roles cannot be granted to PUBLIC.  This restriction
         // could be lifted in the future.  Grants to _SYSTEM never make sense.
         if (granteeIDs[g] == SYSTEM_UID || granteeIDs[g] == PUBLIC_UID)
         {
            *pDiags_ << DgSqlCode(-CAT_NO_GRANT_ROLE_TO_PUBLIC_OR_SYSTEM);
            return STATUS_ERROR;
         } 
         
         if (granteeIDs[g] == grantorIDs[r] || 
             granteeIDs[g] == ComUser::getRootUserID())
         {
            *pDiags_ << DgSqlCode(-CAT_CANT_GRANT_TO_SELF_OR_ROOT);
            return STATUS_ERROR;
         } 
         
         //  Determine if the grant is new (auth ID does not have role),
         // an update (authID has role, but not with grant option),
         // or is a NOP (authID already has role and with grant is not changing).
         bool update = false;
         int32_t thisGrantDepth;
         
         if (grantExists(roleID,grantorIDs[r],granteeIDs[g],thisGrantDepth))
         {
            //Does this grant already exist?  No error, just move on to the
            // next grantee.
            if (thisGrantDepth == grantDepth)
               continue; 
            //TODO: if grantDepth is zero, this is an internal error
            update = true;        
         }
         
         // Does grantor have authority to grant this role?
         //TODO: include role name in error message, specific msg for grant role
         if (!hasWGO(grantorIDs[r],roleID))
         {   
            *pDiags_ << DgSqlCode(-CAT_NOT_AUTHORIZED);
            return STATUS_ERROR;
         }
         
         roleWasGranted = true;
         
         if (update)  // Set new grant depth
         {
            char granteeIDString[20];
            sprintf(granteeIDString,"%d",granteeIDs[g]);
            
            std::string whereClause = whereClauseHeader + granteeIDString;
            
            std::string setClause(" SET GRANT_DEPTH = ");
            
            char grantDepthString[20];
            
            sprintf(grantDepthString,"%d",grantDepth);
            setClause += grantDepthString + whereClause;
            
            myTable.update(setClause);
         }
         else // Grant role to grantee from grantor.
         {
            MyRow row(fullTableName_);
            
            row.roleID_ = roleIDs[r];
            row.roleName_ = roleNames[r];
            row.granteeID_ = granteeIDs[g];
            row.granteeName_ = granteeNames[g];
            row.granteeAuthClass_ = granteeClasses[g];
            row.grantorID_ = grantorIDs[r];
            row.grantorName_ = grantorNames[r];
            row.grantorAuthClass_ = grantorClass;
            row.grantDepth_ = grantDepth;
            
            myTable.insert(row);
         }
      }//grantees
   }//roles

//TODO: if we didn't have any errors, but no roles were granted, then all
// grants are already performed.  Should issue some message.
// Related, need option to suppress already exists and does not exist errors.
//   if (!roleWasGranted)
//   {
//      //TODO: error or warning
//   }   
   
   return STATUS_GOOD;

}  
//********************** End of PrivMgrRoles::grantRole ************************




// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrRoles::grantRoleToCreator                                *
// *                                                                           *
// *    Grants role to creator from _SYSTEM.                                   *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <roleID>                        const int32_t                    In      *
// *    is the ID of the role being created.                                   *
// *                                                                           *
// *  <roleName>                      const std::string &              In      *
// *    is the name of the role being created.                                 *
// *                                                                           *
// *  <granteeID>                     const int32_t                    In      *
// *    is the ID of the creator of the role.                                  *
// *                                                                           *
// *  <granteeName>                   const std::string &              In      *
// *    is the name of the creator of the role.                                *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Role was granted to the user by _SYSTEM.                     *
// *           *: Role was not granted due to I/O error.                       *
// *              A CLI error is put into the diags area.                      *
// *                                                                           *
// *****************************************************************************
    
PrivStatus PrivMgrRoles::grantRoleToCreator(
   const int32_t roleID,
   const std::string & roleName,
   const int32_t granteeID,
   const std::string granteeName) 
     
{

MyTable &myTable = static_cast<MyTable &>(myTable_);

MyRow row(fullTableName_);
   
   row.roleID_ = roleID;
   row.roleName_ = roleName;
   row.granteeID_ = granteeID;
   row.granteeName_ = granteeName;
   row.granteeAuthClass_ = PrivAuthClass::USER;
   row.grantorID_ = SYSTEM_UID;
   row.grantorName_ = "_SYSTEM";
   row.grantorAuthClass_ = PrivAuthClass::USER;
   row.grantDepth_ = -1;
   
   return myTable.insert(row);

}  
//***************** End of PrivMgrRoles::grantRoleToCreator ********************


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrRoles::hasRole                                           *
// *                                                                           *
// *    Determines if an authID has been granted a role.                       *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <authID>                        const int32_t                   In       *
// *    is the authorization ID.                                               *
// *                                                                           *
// *  <roleID>                        const int32_t                   In       *
// *    is the role ID.                                                        *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true: Role has been granted to this authorization ID.                     *
// * false: Role has not been granted to this authorization ID, or there was   *
// * an error trying to read from the ROLE_USAGE table.                        *
// *                                                                           *
// *****************************************************************************
bool PrivMgrRoles::hasRole(
   int32_t authID,
   int32_t roleID)
   
{

MyTable &myTable = static_cast<MyTable &>(myTable_);

std::string whereClause ("WHERE ROLE_ID = ");

char roleIDString[20];
char granteeIDString[20];

   sprintf(roleIDString,"%d",roleID);
   sprintf(granteeIDString,"%d",authID);

   whereClause += roleIDString; 
   whereClause += " AND GRANTEE_ID = ";          
   whereClause += granteeIDString;

int64_t rowCount = 0;

PrivStatus privStatus = myTable.selectCountWhere(whereClause,rowCount);

   if ((privStatus == STATUS_GOOD || privStatus == STATUS_WARNING) &&
        rowCount > 0)
      return true;
      
   pDiags_->clear();
   return false;

}
//*********************** End of PrivMgrRoles::hasRole *************************



// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrRoles::hasWGO                                            *
// *                                                                           *
// *    Determines if an authID has been granted a role with admin (aka with   *
// * grant option).                                                            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <authID>                        const int32_t                   In       *
// *    is the authorization ID.                                               *
// *                                                                           *
// *  <roleID>                        const int32_t                   In       *
// *    is the role ID.                                                        *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true: Role has been granted to this authorization ID with admin.          *
// * false: Role has not been granted with admin to this authorization ID,     *
// * or there was an error trying to read from the ROLE_USAGE table.           *
// *                                                                           *
// *****************************************************************************
bool PrivMgrRoles::hasWGO(
   int32_t authID,
   int32_t roleID)

{

MyTable &myTable = static_cast<MyTable &>(myTable_);

std::string whereClause (" WHERE ROLE_ID = ");

char roleIDString[20];
char authIDString[20];

   sprintf(roleIDString,"%d",roleID);
   sprintf(authIDString,"%d",authID);

   whereClause += roleIDString; 
   whereClause += " AND GRANTEE_ID = ";          
   whereClause += authIDString;
   whereClause += " AND GRANT_DEPTH <> 0";
   
int64_t rowCount = 0;

PrivStatus privStatus = myTable.selectCountWhere(whereClause,rowCount);

   if ((privStatus == STATUS_GOOD || privStatus == STATUS_WARNING) &&
        rowCount > 0)
      return true;
      
   pDiags_->clear();
   return false;
   
}
//************************ End of PrivMgrRoles::hasWGO *************************


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrRoles::grantExists                                       *
// *                                                                           *
// *    Determines if a specific authorization ID (granteee) has been granted  *
// * a role by a specific authorization ID (grantor).                          *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <roleID>                        const int32_t                   In       *
// *    is the role ID.                                                        *
// *                                                                           *
// *  <grantorID>                     const int32_t                   In       *
// *    is the authorization ID of the grantor.                                *
// *                                                                           *
// *  <granteeID>                     const int32_t                   In       *
// *    is the authorization ID of the grantee.                                *
// *                                                                           *
// *  <grantDepth>                    int32_t &                       In       *
// *    passes back the grant depth if the role grant exists.                  *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true: Role has been granted to grantee by grantor.                        *
// * false: Role has not been granted to grantee by grantor, or there was an   *
// * error trying to read from the ROLE_USAGE table.                           *
// *                                                                           *
// *****************************************************************************
bool PrivMgrRoles::grantExists(
   const int32_t roleID,
   const int32_t grantorID,
   const int32_t granteeID,
   int32_t & grantDepth) 

{

MyTable &myTable = static_cast<MyTable &>(myTable_);

std::string whereClause ("WHERE ROLE_ID = ");

char roleIDString[20];
char grantorIDString[20];
char granteeIDString[20];

   sprintf(roleIDString,"%d",roleID);
   sprintf(grantorIDString,"%d",grantorID);
   sprintf(granteeIDString,"%d",granteeID);

   whereClause += roleIDString; 
   whereClause += " AND GRANTOR_ID = ";          
   whereClause += grantorIDString; 
   whereClause += " AND GRANTEE_ID = ";          
   whereClause += granteeIDString; 
   
MyRow row(fullTableName_);
   
PrivStatus privStatus = myTable.selectWhereUnique(whereClause,row);

   if (privStatus == STATUS_GOOD || privStatus == STATUS_WARNING) 
   {
      grantDepth = row.grantDepth_;
      return true;
   }
      
   pDiags_->clear();
   return false;

}
//********************** End of PrivMgrRoles::grantExists **********************



// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrRoles::isGranted                                         *
// *                                                                           *
// *    Determines if a role has been granted, i.e., if it is used in the      *
// *  ROLE_USAGE table.                                                        *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <roleID>                        const int32_t                   In       *
// *    is the role ID.                                                        *
// *                                                                           *
// *  <shouldExcludeGrantsBySystem>   const bool                      [In]     *
// *    if true, don't consider the system grant to the creator.               *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true: Role has been granted to one or more authIDs.                       *
// * false: Role has not been granted, or there was an error trying to         *
// * read from the ROLE_USAGE table.                                           *
// *                                                                           *
// *****************************************************************************
bool PrivMgrRoles::isGranted(
   const int32_t roleID,
   const bool shouldExcludeGrantsBySystem)
  
{

MyRow row(fullTableName_);
MyTable &myTable = static_cast<MyTable &>(myTable_);

std::string whereClause("WHERE ROLE_ID = ");

char roleIDString[20];

   sprintf(roleIDString,"%d",roleID);

   whereClause += roleIDString;
   if (shouldExcludeGrantsBySystem)
      whereClause += " AND GRANTOR_ID <> -2";
      
int64_t rowCount = 0;
   
PrivStatus privStatus = myTable.selectCountWhere(whereClause,rowCount);

   if ((privStatus == STATUS_GOOD || privStatus == STATUS_WARNING) &&
        rowCount > 0)
      return true;
      
   pDiags_->clear();
   return false;
    
}      
//*********************** End of PrivMgrRoles::isGranted ***********************



// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrRoles::isUserGrantedAnyRole                              *
// *                                                                           *
// *    Determines if an authorization ID has been granted any role.           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <authID>                        const int32_t                   In       *
// *    is the authorization ID.                                               *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true: Authorization ID has been granted one or more roles.                *
// * false: Authorization ID has not been granted any roles, or there was      *
// * an error trying to read from the ROLE_USAGE table.                        *
// *                                                                           *
// *****************************************************************************
bool PrivMgrRoles::isUserGrantedAnyRole(const int32_t authID)

{

MyRow row(fullTableName_);
MyTable &myTable = static_cast<MyTable &>(myTable_);

std::string whereClause(" WHERE GRANTEE_ID = ");

char authIDString[20];

   sprintf(authIDString,"%d",authID);

   whereClause += authIDString;
      
int64_t rowCount = 0;
   
PrivStatus privStatus = myTable.selectCountWhere(whereClause,rowCount);

   if ((privStatus == STATUS_GOOD || privStatus == STATUS_WARNING) &&
        rowCount > 0)
      return true;
      
   pDiags_->clear();
   return false;
    
}      
//***************** End of PrivMgrRoles::isUserGrantedAnyRole ******************



// *****************************************************************************
// * Function: PrivMgrRoles::populateCreatorGrants                             *
// *                                                                           *
// *    Inserts rows into the ROLE_USAGE table during authorization            *
// * initialization for existing roles.                                        *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Parameters:                                                               *
// *                                                                           *
// *  <authsLocation>                 const std::string &             In       *
// *    is the location of the AUTHS table containing the Trafodion users      *
// * and roles.                                                                *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: ROLE_USAGE table was populated with owner grants.            *
// *           *: ROLE_USAGE was not populated. A CLI error is put into        *
// *              the diags area.                                              *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrRoles::populateCreatorGrants(const std::string & authsLocation)

{

MyTable &myTable = static_cast<MyTable &>(myTable_);

   return myTable.insertSelect(authsLocation);
   
}
//***************** End of PrivMgrRoles::populateCreatorGrants *****************


// *****************************************************************************
// * Function: PrivMgrRoles::revokeAllForGrantor                               *
// *                                                                           *
// *    Revokes grants from a specified grantor.                               *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Parameters:                                                               *
// *                                                                           *
// *  <grantorID>                     const int32_t                    In      *
// *    is the unique ID of the authID whose grants are to be revoked.         *
// *                                                                           *
// *  <roleID>                        const int32_t                    In      *
// *    is the ID of the role to be revoked.                                   *
// *                                                                           *
// *  <isGOFSpecified>             const bool                          In      *
// *    is true if admin rights are being revoked.                             *
// *                                                                           *
// *  <newGrantDepth>              const int32_t                       In      *
// *    is the number of levels this privilege may be granted by the grantee.  *
// *  Initially this is always 0 when revoking.                                *
// *                                                                           *
// *  <dropBehavior>                  PrivDropBehavior                 In      *
// *    indicates whether restrict or cascade behavior is requested.           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: All grants of the role by this grantor were revoked (or      *
// *              there were none).                                            *
// *           *: Could not revoke grants.  A CLI error is put into            *
// *              the diags area.                                              *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrRoles::revokeAllForGrantor(
   const int32_t grantorID,
   const int32_t roleID,
   const bool isGOFSpecified,
   const int32_t newGrantDepth,
   PrivDropBehavior dropBehavior) 
   
{

// *****************************************************************************
// *                                                                           *
// *  First, get all the rows where the grantor has granted this role to any   *
// * authorization ID.                                                         *
// *                                                                           *
// *****************************************************************************

char roleIDString[20];
char grantorIDString[20];
         
   sprintf(roleIDString,"%d",roleID);
   sprintf(grantorIDString,"%d",grantorID);

std::string whereClause("WHERE ROLE_ID = ");

   whereClause += roleIDString;
   whereClause += " AND GRANTOR_ID = ";
   whereClause += grantorIDString;
   
std::string orderByClause;
   
std::vector<MyRow> rows;
MyTable &myTable = static_cast<MyTable &>(myTable_);

PrivStatus privStatus = myTable.selectAllWhere(whereClause,orderByClause,rows);

// *****************************************************************************
// *                                                                           *
// *   If the grantor has no active grants for this role, we are done.         *
// *                                                                           *
// *****************************************************************************

   if (privStatus == STATUS_NOTFOUND)
      return STATUS_GOOD;

// *****************************************************************************
// *                                                                           *
// *   Unexpected problem, let the caller deal with it.                        *
// *                                                                           *
// *****************************************************************************

   if (privStatus != STATUS_GOOD && privStatus != STATUS_WARNING)
      return privStatus;
      
// *****************************************************************************
// *                                                                           *
// *   If there are grants and drop behavior is RESTRICT, return an error.     *
// *                                                                           *
// *****************************************************************************

   if (dropBehavior == PrivDropBehavior::RESTRICT)
   {  
      *pDiags_ << DgSqlCode(-CAT_ROLE_IS_GRANTED_NO_REVOKE) 
               << DgString0(rows[0].roleName_.c_str()) 
               << DgString1(rows[0].grantorName_.c_str()); 
      return STATUS_ERROR;
   }
      
// *****************************************************************************
// *                                                                           *
// *   There are one more grants from the grantor of this role.  Build the     *
// * lists of grantees and auth types, plus a one element role ID list, and    *
// * call revokeRole (our likely caller).                                      *
// *                                                                           *
// *****************************************************************************
      
std::vector<int32_t> granteeIDs;
std::vector<PrivAuthClass> granteeClasses;
std::vector<int32_t> grantorIDs;

   for (size_t r = 0; r < rows.size(); r++)
   {
      MyRow &row = rows[r];
      
      granteeIDs.push_back(row.granteeID_);
      granteeClasses.push_back(row.granteeAuthClass_);
      grantorIDs.push_back(grantorID);
   }
   
std::vector<int32_t> roleIDs;

   roleIDs.push_back(roleID);
   
   return revokeRole(roleIDs,granteeIDs,granteeClasses,grantorIDs,
                     isGOFSpecified,newGrantDepth,dropBehavior);

}
//****************** End of PrivMgrRoles::revokeAllForGrantor ******************


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrRoles::revokeRole                                        *
// *                                                                           *
// *    This function revokes one or more roles from one or more               *
// * authorization IDs.                                                        *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <roleIDs>                    const std::vector<int32_t> &        In      *
// *    is a vector of roleIDs.  The caller is responsible for ensuring all    *
// *  IDs are unique.                                                          *
// *                                                                           *
// *  <granteeIDs>                 const std::vector<int32_t> &        In      *
// *    is a vector of granteeIDs.  The caller is responsible for ensuring all *
// *  IDs are unique.                                                          *
// *                                                                           *
// *  <granteeClasses>             const std::vector<PrivAuthClass> &  In      *
// *    is a vector of PrivAuthClass corresponding the granteeID in the same   *
// *  position in the granteeIDs vector.                                       *
// *                                                                           *
// *  <grantorIDs>                    const std::vector<int32_t> &     In      *
// *    is a vector of authIDs that granted the role.  The elements of the     *
// *  vector are for the corresponding role entry.                             *
// *                                                                           *
// *  <isGOFSpecified>             const bool                          In      *
// *    is true if admin rights are being revoked.                             *
// *                                                                           *
// *  <newGrantDepth>              const int32_t                       In      *
// *    is the number of levels this privilege may be granted by the grantee.  *
// *  Initially this is always 0 when revoking.                                *
// *                                                                           *
// *  <dropBehavior>               PrivDropBehavior                    In      *
// *    indicates whether restrict or cascade behavior is requested.           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Role(s) were revoked                                         *
// *           *: One or more roles were not revoked.                          *
// *              A CLI error is put into the diags area.                      *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrRoles::revokeRole(
   const std::vector<int32_t> & roleIDs,
   const std::vector<int32_t> & granteeIDs,
   const std::vector<PrivAuthClass> & granteeClasses,
   const std::vector<int32_t> & grantorIDs,
   const bool isGOFSpecified,
   const int32_t newGrantDepth,
   PrivDropBehavior dropBehavior) 
  
{

//TODO: Currently only RESTRICT behavior is supported.

   if (dropBehavior == PrivDropBehavior::CASCADE)
   {
      *pDiags_ << DgSqlCode(-21001) << DgString0("CASCADE");
      return STATUS_ERROR;
   }
   
PrivStatus privStatus = STATUS_GOOD;

   for (size_t r = 0; r < roleIDs.size(); r++)
   {
      int32_t roleID = roleIDs[r];
      
      for (size_t g = 0; g < granteeIDs.size(); g++)
      {
         if (hasWGO(granteeIDs[g],roleID))
         {
            privStatus = revokeAllForGrantor(granteeIDs[g],roleID,
                                             isGOFSpecified,newGrantDepth,
                                             dropBehavior);
            if (privStatus != STATUS_GOOD)
            {
               return STATUS_ERROR;
            }
         }
         if (dependentObjectsExist(granteeIDs[g],roleID))
         {
            *pDiags_ << DgSqlCode(-CAT_DEPENDENT_ROLE_PRIVILEGES_EXIST);
            //TODO: include name of dependent object
            return STATUS_ERROR;
         }
      }
   }

// All checks completed, all dependent grants revoked, and when CASCADE is
// supported, all dependent objects dropped. It's now safe to revoke the roles.

std::string setClause("SET GRANT_DEPTH = ");

   if (isGOFSpecified)
   {
      char grantDepthString[20];
      
      sprintf(grantDepthString,"%d",newGrantDepth);
      setClause += grantDepthString;
   }

   for (size_t r2 = 0; r2 < roleIDs.size(); r2++)
   {
      for (size_t g2 = 0; g2 < granteeIDs.size(); g2++)
      {
         std::string whereClause(" WHERE ROLE_ID = ");
         
         char roleIDString[20];
         char granteeIDString[20];
         char grantorIDString[20];

         sprintf(grantorIDString,"%d",grantorIDs[r2]);
         sprintf(roleIDString,"%d",roleIDs[r2]);
         sprintf(granteeIDString,"%d",granteeIDs[g2]);
         
         whereClause += roleIDString;
         whereClause += " AND GRANTEE_ID = ";
         whereClause += granteeIDString;
         whereClause += " AND GRANTOR_ID = ";
         whereClause += grantorIDString;
         
         MyTable &myTable = static_cast<MyTable &>(myTable_);
         
         if (isGOFSpecified)
         {
            std::string updateClause = setClause + whereClause;
            
            privStatus = myTable.update(updateClause);
         }
         else
            privStatus = myTable.deleteWhere(whereClause); 
            
         if (privStatus != STATUS_GOOD)
            return STATUS_INTERNAL;
      }
   }   
      
   return STATUS_GOOD;
   
}  
//******************** End of PrivMgrRoles::revokePrivilege ********************


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrRoles::revokeRoleFromCreator                             *
// *                                                                           *
// *    Revokes the system granted role to the creator of the role.            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <roleID>                        const int32_t                   In       *
// *    is the role ID.                                                        *
// *                                                                           *
// *  <granteeID>                     const int32_t                   In       *
// *    is the ID of the authorization ID that created the role.               *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Role was revoked.                                            *
// *           *: Role was not revoked. A CLI error is put into the diags area *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrRoles::revokeRoleFromCreator(
   const int32_t roleID,
   const int32_t granteeID)  

{

MyTable &myTable = static_cast<MyTable &>(myTable_);

std::string whereClause(" WHERE ROLE_ID = ");

char roleIDString[20];
char granteeIDString[20];

   sprintf(roleIDString,"%d",roleID);

   whereClause += roleIDString;
   whereClause += " AND GRANTEE_ID = ";
   
   sprintf(granteeIDString,"%d",granteeID);
   
   whereClause += granteeIDString;
   whereClause += " AND GRANTOR_ID = -2";
   
   return myTable.deleteWhere(whereClause);

}
//*********************** End of PrivMgrRoles::isGranted ***********************




// *****************************************************************************
//    MyTable methods
// *****************************************************************************


// *****************************************************************************
// *                                                                           *
// * Function: MyTable::insert                                                 *
// *                                                                           *
// *    Inserts a row into the ROLE_USAGE table.                               *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <rowIn>                         const PrivMgrMDRow &            In       *
// *    is a MyRow to be inserted.                                             *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Row inserted.                                                *
// *           *: Insert failed. A CLI error is put into the diags area.       *
// *                                                                           *
// *****************************************************************************
PrivStatus MyTable::insert(const PrivMgrMDRow & rowIn)
{

const MyRow & row = static_cast<const MyRow &>(rowIn);
char insertStatement[1000];

char granteeAuthClass[3] = {0};

   switch (row.granteeAuthClass_)
   {
      case PrivAuthClass::USER:
         granteeAuthClass[0] = 'U';
         break; 
      case PrivAuthClass::ROLE:
         granteeAuthClass[0] = 'R';
         break;
      default:
         granteeAuthClass[0] = ' ';
   }
       
char grantorAuthClass[3] = {0};

   switch (row.grantorAuthClass_)
   {
      case PrivAuthClass::USER:
         grantorAuthClass[0] = 'U';
         break; 
      case PrivAuthClass::ROLE:
         grantorAuthClass[0] = 'R';
         break;
      default:
         grantorAuthClass[0] = ' ';
   }
       
   sprintf(insertStatement,"insert into %s values (%d, '%s', %d, '%s', '%s', %d, '%s', '%s', %d)",     
                           tableName_.c_str(),
                           row.roleID_,
                           row.roleName_.c_str(),
                           row.granteeID_,
                           row.granteeName_.c_str(),
                           granteeAuthClass,
                           row.grantorID_,
                           row.grantorName_.c_str(),
                           grantorAuthClass,
                           row.grantDepth_);

   return CLIImmediate(insertStatement);

}
//************************** End of MyTable::insert ****************************



// *****************************************************************************
// *                                                                           *
// * Function: MyTable::insertSelect                                           *
// *                                                                           *
// *    Inserts a row into the ROLE_USAGE table granting a role to its         *
// * owner for every role defined in the AUTHS table.                          *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <authsLocation>                 const std::string &             In       *
// *    is the fully qualified AUTHS table name.                               *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Row inserted.                                                *
// *           *: Insert failed. A CLI error is put into the diags area.       *
// *                                                                           *
// *****************************************************************************
PrivStatus MyTable::insertSelect(const std::string & authsLocation)

{

char insertStatement[2000];

   sprintf(insertStatement, "INSERT INTO %s SELECT A1.AUTH_ID, A1.AUTH_DB_NAME, A1.AUTH_CREATOR,"
           "(SELECT AUTH_DB_NAME FROM %s A2 WHERE A2.auth_ID = A1.AUTH_CREATOR)," 
           "(SELECT AUTH_TYPE FROM %s A3 WHERE A3.auth_ID = A1.AUTH_CREATOR),"
           "-2,'_SYSTEM','%c',-1 FROM %s A1 WHERE A1.AUTH_TYPE = 'R'",
           tableName_.c_str(),authsLocation.c_str(),authsLocation.c_str(), 
           'U',authsLocation.c_str()); 
               
   return CLIImmediate(insertStatement);
                  
}
//*********************** End of MyTable::insertSelect *************************






// *****************************************************************************
// *                                                                           *
// * Function: MyTable::selectAllWhere                                         *
// *                                                                           *
// *    Selects rows from the ROLE_USAGE table based on the specified          *
// *  WHERE clause and sorted per an optional ORDER BY clause.                 *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <whereClause>                   const std::string &             In       *
// *    is the WHERE clause specifying a unique row.                           *
// *                                                                           *
// *  <orderByClause>                 const std::string &             In       *
// *    is an optional ORDER BY clause.                                        *
// *                                                                           *
// *  <rowOut>                        PrivMgrMDRow &                  Out      *
// *    passes back a MyRow.                                                   *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Row returned.                                                *
// *           *: Select failed. A CLI error is put into the diags area.       *
// *                                                                           *
// *****************************************************************************
PrivStatus MyTable::selectAllWhere(
   const std::string & whereClause,
   const std::string & orderByClause,
   std::vector<MyRow> & rows)
   
{

std::string selectStmt ("SELECT ROLE_ID, ROLE_NAME, GRANTEE_ID, GRANTEE_NAME, "
                               "GRANTEE_AUTH_CLASS, GRANTOR_ID, GRANTOR_NAME, "
                               "GRANTOR_AUTH_CLASS, GRANT_DEPTH FROM ");

   selectStmt += tableName_;
   selectStmt += " ";
   selectStmt += whereClause;
   selectStmt += " ";
   selectStmt += orderByClause;
  
Queue * tableQueue = NULL;

PrivStatus privStatus = executeFetchAll(selectStmt,tableQueue);

   if (privStatus != STATUS_GOOD)
      return privStatus;
      
   if (tableQueue == NULL)
      return STATUS_ERROR;

   tableQueue->position();
   for (int r = 0; r < tableQueue->numEntries(); r++)
   {
      OutputInfo * pCliRow = (OutputInfo*)tableQueue->getNext();
      MyRow row(tableName_);
      setRow(*pCliRow,row);
      rows.push_back(row);
   }    

  return STATUS_GOOD;

}
//********************** End of MyTable::selectAllWhere ************************



// *****************************************************************************
// *                                                                           *
// * Function: MyTable::selectWhereUnique                                      *
// *                                                                           *
// *    Selects a row from the ROLE_USAGE table based on the specified         *
// *  WHERE clause.                                                            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <whereClause>                   const std::string &             In       *
// *    is the WHERE clause specifying a unique row.                           *
// *                                                                           *
// *  <rowOut>                        PrivMgrMDRow &                  Out      *
// *    passes back a MyRow.                                                   *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Row returned.                                                *
// *           *: Select failed. A CLI error is put into the diags area.       *
// *                                                                           *
// *****************************************************************************
PrivStatus MyTable::selectWhereUnique(
   const std::string & whereClause,
   PrivMgrMDRow & rowOut)
   
{

// Generate the select statement
std::string selectStmt ("SELECT ROLE_ID, ROLE_NAME, GRANTEE_ID, GRANTEE_NAME, "
                               "GRANTEE_AUTH_CLASS, GRANTOR_ID, GRANTOR_NAME, "
                               "GRANTOR_AUTH_CLASS, GRANT_DEPTH FROM ");

selectStmt += tableName_;
selectStmt += " ";
selectStmt += whereClause;

Queue * tableQueue = NULL;

PrivStatus privStatus = executeFetchAll(selectStmt,tableQueue);

   if (privStatus != STATUS_GOOD)
      return privStatus;
      
   if (tableQueue->numEntries() != 1)
      return STATUS_ERROR;
   
OutputInfo * pCliRow = (OutputInfo*)tableQueue->getNext();

   setRow(*pCliRow,rowOut);

   return STATUS_GOOD;
   
}
//********************* End of MyTable::selectWhereUnique **********************

// *****************************************************************************
// *                                                                           *
// * Function: MyTable::setRow                                                 *
// *                                                                           *
// *    Sets the fields of a row.                                              *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <cliInterface>                  OutputInfo &                    In       *
// *    is a reference to CLI interface to the row data that was read.         *
// *                                                                           *
// *  <rowOut>                        PrivMgrMDRow &                  Out      *
// *    passes back a MyRow.                                                   *
// *                                                                           *
// *****************************************************************************
void MyTable::setRow(
   OutputInfo & cliInterface,
   PrivMgrMDRow & rowOut)
   
{

MyRow & row = static_cast<MyRow &>(rowOut);
char * ptr = NULL;
int32_t length = 0;
char value[500];
  
   // column 1:  role_id
   cliInterface.get(0,ptr,length);
   row.roleID_ = *(reinterpret_cast<int32_t*>(ptr));

   // column 2:  role_name
   cliInterface.get(1,ptr,length);
   strncpy(value,ptr,length);
   value[length] = 0;
   row.roleName_ = value;

   // column 3:  grantee_ID
   cliInterface.get(2,ptr,length);
   row.granteeID_ = *(reinterpret_cast<int32_t*>(ptr));

   // column 4:  grantee_name
   cliInterface.get(3,ptr,length);
   strncpy(value,ptr,length);
   value[length] = 0;
   row.granteeName_ = value;

   // column 5:  grantee_auth_class
   cliInterface.get(4,ptr,length);
   strncpy(value,ptr,length);
   value[length] = 0;
   switch (value[0])
   {
      case 'U': 
         row.granteeAuthClass_ = PrivAuthClass::USER;
         break;
      case 'R': 
         row.granteeAuthClass_ = PrivAuthClass::ROLE;
         break;
      default:
         row.granteeAuthClass_ = PrivAuthClass::UNKNOWN;
   }
 
   // column 6:  grantor_ID
   cliInterface.get(5,ptr,length);
   row.grantorID_ = *(reinterpret_cast<int32_t*>(ptr));

   // column 7:  grantor_name
   cliInterface.get(6,ptr,length);
   strncpy(value,ptr,length);
   value[length] = 0;
   row.grantorName_ = value;

   // column 8:  grantor_auth_class
   cliInterface.get(7,ptr,length);
   strncpy(value,ptr,length);
   value[length] = 0;
   switch (value[0])
   {
      case 'U': 
         row.grantorAuthClass_ = PrivAuthClass::USER;
         break;
      case 'R': 
         row.grantorAuthClass_ = PrivAuthClass::USER;
         break;
      default:
         row.grantorAuthClass_ = PrivAuthClass::UNKNOWN;
   }
 
   // column 9:  grant_depth
   cliInterface.get(8,ptr,length);
   row.grantDepth_ = *(reinterpret_cast<int32_t*>(ptr));
   
   lastRowRead_ = row;

}
//************************** End of MyTable::setRow ****************************



// *****************************************************************************
// *                                                                           *
// * Function: hasValue                                                        *
// *                                                                           *
// *   This function determines if a vector contains a value.                  *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <container>                  std::vector<int32_t>               In       *
// *    is the vector of 32-bit values.                                        *
// *                                                                           *
// *  <value>                      int32_t                            In       *
// *    is the value to be compared against existing values in the vector.     *
// *                                                                           *
// *****************************************************************************
static bool hasValue(
   std::vector<int32_t> container,
   int32_t value)
   
{

   for (size_t index = 0; index < container.size(); index++)
      if (container[index] == value)
         return true;
         
   return false;
   
}
//***************************** End of hasValue ********************************



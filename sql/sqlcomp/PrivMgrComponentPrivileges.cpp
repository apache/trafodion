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
#include "PrivMgrComponentPrivileges.h"
  
#include "PrivMgrMD.h"
#include "PrivMgrMDTable.h"
#include "PrivMgrComponents.h"
#include "PrivMgrComponentOperations.h"
#include "PrivMgrRoles.h"

#include <string>
#include <cstdio>
#include <vector>
#include "ComSmallDefs.h"

// sqlcli.h included because ExExeUtilCli.h needs it (and does not include it!)
#include "sqlcli.h"
#include "ExExeUtilCli.h"
#include "ComQueue.h"
#include "ComDiags.h"
#include "ComQueue.h"
// CmpCommon.h contains STMTHEAP declaration
#include "CmpCommon.h"
#include "CmpDDLCatErrorCodes.h"
#include "ComUser.h"

namespace ComponentPrivileges 
{
// *****************************************************************************
// * Class:        MyRow
// * Description:  This class represents a row in the COMPONENT_PRIVILEGES table.
// *****************************************************************************
class MyRow : public PrivMgrMDRow
{
public:
// -------------------------------------------------------------------
// Constructors and destructors:
// -------------------------------------------------------------------
   MyRow(std::string tableName)
   : PrivMgrMDRow(tableName),
     componentUID_(0)
   { };
   MyRow(const MyRow &other)
   : PrivMgrMDRow(other)
   {
      componentUID_ = other.componentUID_;              
      operationCode_ = other.operationCode_;
      granteeID_ = other.granteeID_;
      granteeName_ = other.granteeName_;
      grantorID_ = other.grantorID_;
      grantorName_ = other.grantorName_;
      grantDepth_ = other.grantDepth_;
   };
   virtual ~MyRow() {};
   inline void clear() {componentUID_ = 0;};
    
// -------------------------------------------------------------------
// Data Members:
// -------------------------------------------------------------------

//  From COMPONENT_PRIVILEGES
    int64_t            componentUID_;
    std::string        operationCode_;
    int32_t            granteeID_;
    std::string        granteeName_;
    int32_t            grantorID_;
    std::string        grantorName_;
    int32_t            grantDepth_;
    
private: 
   MyRow();
  
};

// *****************************************************************************
// * Class:        MyTable
// * Description:  This class represents the COMPONENT_PRIVILEGES table.
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
    
   inline void clear() { lastRowRead_.clear(); };
   
   PrivStatus fetchOwner(
      const int64_t componentUID,
      const std::string & operationCode,
      int32_t & grantee);    
   
   virtual PrivStatus insert(const PrivMgrMDRow & row);
   
   PrivStatus selectAllWhere(
      const std::string & whereClause,
      const std::string & orderByClause,
      std::vector<MyRow> & rows);
      
   virtual PrivStatus selectWhereUnique(
      const std::string & whereClause,
      PrivMgrMDRow & row);
      
   PrivStatus selectWhere(
      const std::string & whereClause,  
      std::vector<MyRow *> &rowList);
      
   void setRow(
      OutputInfo & cliInterface,
      PrivMgrMDRow & rowOut);
      
private:   
   MyTable();
   
   MyRow lastRowRead_;

};
}//End namespace ComponentPrivileges
using namespace ComponentPrivileges;

// *****************************************************************************
//    PrivMgrComponentPrivileges methods
// *****************************************************************************
// -----------------------------------------------------------------------
// Construct a PrivMgrComponentPrivileges object for a new component operation.
// -----------------------------------------------------------------------
PrivMgrComponentPrivileges::PrivMgrComponentPrivileges(
   const std::string & metadataLocation,
   ComDiagsArea * pDiags)
: PrivMgr(metadataLocation,pDiags),
  fullTableName_(metadataLocation_ + ".COMPONENT_PRIVILEGES"),
  myTable_(*new MyTable(fullTableName_,pDiags)) 
{ };

// -----------------------------------------------------------------------
// Copy constructor
// -----------------------------------------------------------------------
PrivMgrComponentPrivileges::PrivMgrComponentPrivileges(const PrivMgrComponentPrivileges &other)
: PrivMgr(other),
  myTable_(*new MyTable(fullTableName_,pDiags_))
{
   fullTableName_ = other.fullTableName_;
}

// -----------------------------------------------------------------------
// Destructor.
// -----------------------------------------------------------------------
PrivMgrComponentPrivileges::~PrivMgrComponentPrivileges() 
{ 

   delete &myTable_;

}

// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentPrivileges::clear                               *
// *                                                                           *
// *    This function clears any cache associated with this object.            *
// *                                                                           *
// *****************************************************************************
void PrivMgrComponentPrivileges::clear()

{

MyTable &myTable = static_cast<MyTable &>(myTable_);

   myTable.clear();
   
}
//****************** End of PrivMgrComponentPrivileges::clear ******************


  
// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentPrivileges::describeComponentPrivileges         *
// *  lookup "PRIVMGR_MD".COMPONENT_PRIVILEGES joined with "_MD_".AUTHS        *
// *  to generate GRANT COMPONENT PRIVILEGE statements                         *
// *  for a specified operation code and component name/UID.                   *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  componentUIDString               const std::string &                  In *
// *    used with operationCode to find wanted component privileges.           *
// *                                                                           *
// *  componentName                     const std::string &                 In *
// *    used for generate grant statement on the component.                    *
// *                                                                           *
// *  operationCode                        const std::string &              In *
// *    used with componentUIDString to find wanted component privileges.      *
// *                                                                           *
// *  operationName                        const std::string &              In *
// *     used for generate grant statement as granted operation.               *
// *                                                                           *
// *  outlines                                   std::vector<std::string> & Out*
// *      output generated GRANT statements to this array.                     *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Component privilege(s) were revoked                          *
// *           *: One or more component privileges were not revoked.           *
// *              A CLI error is put into the diags area.                      *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrComponentPrivileges::describeComponentPrivileges(
   const std::string & componentUIDString,
   const std::string & componentName,
   const std::string & operationCode, 
   const std::string & operationName,
   std::vector<std::string> & outlines) 
   
{
//do the following joined select to get database user name 
/*
SELECT 
AU.AUTH_DB_NAME, 
CP.GRANTEE_ID,
CP.GRANTOR_ID,
CP.COMPONENT_UID,
CP.OPERATION_CODE,
CP.GRANT_DEPTH
FROM 
TRAFODION."PRIVMGR_MD".COMPONENT_PRIVILEGES CP 
LEFT JOIN 
TRAFODION."_MD_".AUTHS AU
ON
AU.AUTH_ID = CP.GRANTEE_ID
WHERE 
CP.OPERATION_CODE = operationCode 
AND 
CP.COMPONENT_UID = componentUIDString;
*/
    std::string selectJoinStmt;
    selectJoinStmt += "SELECT "
                        "AU.AUTH_DB_NAME, "
                        "CP.GRANTEE_ID, "
                        "CP.GRANTOR_ID, "
                        "CP.COMPONENT_UID, "
                        "CP.OPERATION_CODE, "
                        "CP.GRANT_DEPTH "
                       "FROM " 
                        "TRAFODION.\"PRIVMGR_MD\".COMPONENT_PRIVILEGES CP "
                       "LEFT JOIN " 
                        "TRAFODION.\"_MD_\".AUTHS AU "
                       "ON "
                        "AU.AUTH_ID = CP.GRANTEE_ID "
                       "WHERE ";

    selectJoinStmt += "CP.OPERATION_CODE = '" + operationCode + "'";
    selectJoinStmt += " AND CP.COMPONENT_UID = " + componentUIDString;
    ExeCliInterface cliInterface(STMTHEAP);
    Queue * tableQueue = NULL;
    int32_t cliRC =  cliInterface.fetchAllRows(tableQueue, 
                                               (char *)selectJoinStmt.c_str(), 
                                                0, false, false, true);

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
    char * ptr = NULL;
    Int32 len = 0;
    char value[500];
    std::string auth_db_name;
    int32_t grantee_id;
    int32_t grantor_id;
    int64_t component_uid;
    std::string operation_code;
    int32_t grant_depth;
    for (int idx = 0; idx < tableQueue->numEntries(); idx++)
    {
        OutputInfo * pCliRow = (OutputInfo*)tableQueue->getNext();
        std::string componentText;
        // column 1:  AUTH_DB_NAME
        pCliRow->get(0,ptr,len);
        strncpy(value, ptr, len);
        value[len] = 0;
        auth_db_name= value;

         // column 2:  GRANTEE_ID
        pCliRow->get(1,ptr,len);
        grantee_id = *(reinterpret_cast<int32_t*>(ptr));

        // column 3: GRANTOR_ID
        pCliRow->get(2,ptr,len);
        grantor_id = *(reinterpret_cast<int32_t*>(ptr));

        // column 4: COMPONENT_UID
        pCliRow->get(3,ptr,len);
        component_uid = *(reinterpret_cast<int64_t*>(ptr));

        // column 5: OPERATION_CODE
        pCliRow->get(4,ptr,len);
        strncpy(value, ptr, len);
        value[len] = 0;
        operation_code = value;

        // column 6: GRANT_DEPTH
        pCliRow->get(5,ptr,len);
        grant_depth = *(reinterpret_cast<int32_t*>(ptr));
        
        //generate grant statement
        if (grantor_id == SYSTEM_AUTH_ID)
           componentText += "-- ";
        componentText += " GRANT COMPONENT PRIVILEGE ";
        componentText += operationName;
        outlines.push_back(componentText);
        if(grant_depth != 0){
          componentText = " ON ";
          componentText += componentName ;
          componentText += " TO ";
          if(grantee_id == -1)
            componentText += "PUBLIC";
          else
            componentText += auth_db_name;
          outlines.push_back(componentText);
          componentText = " WITH GRANT OPTION;";
          outlines.push_back(componentText);
        }
        else
        {
          componentText = " ON ";
          componentText += componentName ;
          componentText += " TO ";
          if(grantee_id == -1)
            componentText += "PUBLIC;";
          else
            componentText += auth_db_name + ";";
          outlines.push_back(componentText);
        }
        outlines.push_back("");
    }    
    return STATUS_GOOD;
    
}
//****** End of PrivMgrComponentPrivileges::describeComponentPrivileges ********


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentPrivileges::dropAll                             *
// *                                                                           *
// *    This function drops all component privileges.                          *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// *  STATUS_GOOD: All rows were deleted.                                      *
// * STATUS_ERROR: Execution failed. A CLI error is put into the diags area.   *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrComponentPrivileges::dropAll()

{

MyTable &myTable = static_cast<MyTable &>(myTable_);

std::string whereClause(" ");

   return myTable.deleteWhere(whereClause);

}
//**************** End of PrivMgrComponentPrivileges::dropAll ******************



// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentPrivileges::dropAllForComponent                 *
// *                                                                           *
// *    This function drops all component privileges associated with the       *
// *  specified component.                                                     *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentUIDString>            const std::string &             In       *
// *    is a string representation of the unique ID associated with the        *
// *    component.                                                             *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// *  STATUS_GOOD: All rows associated with the component were deleted.        *
// * STATUS_ERROR: Execution failed. A CLI error is put into the diags area.   *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrComponentPrivileges::dropAllForComponent(const std::string & componentUIDString)

{

MyTable &myTable = static_cast<MyTable &>(myTable_);

std::string whereClause("WHERE ");

   whereClause += "COMPONENT_UID = ";
   whereClause += componentUIDString.c_str();
   
   return myTable.deleteWhere(whereClause);

}
//*********** End of PrivMgrComponentPrivileges::dropAllForComponent ***********

// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentPrivileges::dropAllForOperation                 *
// *                                                                           *
// *    This function drops all component privileges associated with the       *
// *  specified component and operation.                                       *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentUIDString>            const std::string &             In       *
// *    is a string representation of the unique ID associated with the        *
// *    component.                                                             *
// *                                                                           *
// *  <operationCode>                 const std::string &             In       *
// *    is the two character code associated with the component operation.     *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// *  STATUS_GOOD: All rows associated with the component and operation        *
// *               were deleted.                                               *
// * STATUS_ERROR: Execution failed. A CLI error is put into the diags area.   *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrComponentPrivileges::dropAllForOperation(
   const std::string & componentUIDString,
   const std::string & operationCode) 
   
{

MyTable &myTable = static_cast<MyTable &>(myTable_);

std::string whereClause("WHERE ");

   whereClause += "COMPONENT_UID = ";
   whereClause += componentUIDString.c_str();
   whereClause += " AND OPERATION_CODE = '";
   whereClause += operationCode.c_str();
   whereClause += "'";
   
   return myTable.deleteWhere(whereClause);

}
//*********** End of PrivMgrComponentPrivileges::dropAllForOperation ***********




// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentPrivileges::getCount                            *
// *                                                                           *
// *    Returns the number of grants of component privileges.                  *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: int64_t                                                          *
// *                                                                           *
// *    Returns the number of grants of component privileges.                  *
// *                                                                           *
// *****************************************************************************
int64_t PrivMgrComponentPrivileges::getCount()
   
{
                                   
std::string whereClause(" ");   

int64_t rowCount = 0;   
MyTable &myTable = static_cast<MyTable &>(myTable_);

PrivStatus privStatus = myTable.selectCountWhere(whereClause,rowCount);

   if (privStatus != STATUS_GOOD)
      pDiags_->clear();
      
   return rowCount;

}
//***************** End of PrivMgrComponentPrivileges::getCount ****************



   
// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentPrivileges::grantExists                         *
// *                                                                           *
// *    Determines if a specific authorization ID (granteee) has been granted  *
// * a component privilege by a specific authorization ID (grantor).           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentUIDString>            const std::string &             In       *
// *    is the component unique ID.                                            *
// *                                                                           *
// *  <operationCode>                 const std::string &             In       *
// *    is the two character code associated with the component operation.     *
// *                                                                           *
// *  <grantorID>                     const int32_t                   In       *
// *    is the authorization ID of the grantor.                                *
// *                                                                           *
// *  <granteeID>                     const int32_t                   In       *
// *    is the authorization ID of the grantee.                                *
// *                                                                           *
// *  <grantDepth>                    int32_t &                       In       *
// *    passes back the grant depth if the component privilege grant exists.   *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true: Component privilege has been granted to grantee by grantor.         *
// * false: Component privilege has not been granted to grantee by grantor,    *
// * or there was an error trying to read from the COMPONENT_PRIVILEGES table. *
// *                                                                           *
// *****************************************************************************
bool PrivMgrComponentPrivileges::grantExists(
   const std::string componentUIDString,
   const std::string operationCode,
   const int32_t grantorID,
   const int32_t granteeID,
   int32_t & grantDepth) 

{

MyTable &myTable = static_cast<MyTable &>(myTable_);

// Not found in cache, look for the component name in metadata.
std::string whereClause("WHERE COMPONENT_UID = ");

   whereClause += componentUIDString;
   whereClause += " AND OPERATION_CODE = '";
   whereClause += operationCode; 
   whereClause += "' AND GRANTOR_ID = ";          
   whereClause += authIDToString(grantorID); 
   whereClause += " AND GRANTEE_ID = ";          
   whereClause += authIDToString(granteeID); 
   
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
//*************** End of PrivMgrComponentPrivileges::grantExists ***************


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentPrivileges::grantPrivilege                      *
// *                                                                           *
// *    Grants the authority to perform one or more operations on a            *
// *  component to an authID; a row is added to the COMPONENT_PRIVILEGES       *
// *  table for each grant.                                                    *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentName>                 const std::string &              In      *
// *    is the component name.                                                 *
// *                                                                           *
// *  <operations>                    const std::vector<std::string> & In      *
// *    is a list of component operations to be granted.                       *
// *                                                                           *
// *  <grantorIDIn>                   const int32_t                    In      *
// *    is the auth ID granting the privilege.                                 *
// *                                                                           *
// *  <grantorName>                   const std::string &              In      *
// *    is the name of the authID granting the privilege.                      *
// *                                                                           *
// *  <granteeID>                     const int32_t                    In      *
// *    is the the authID being granted the privilege.                         *
// *                                                                           *
// *  <granteeName>                   const std::string &              In      *
// *    is the name of the authID being granted the privilege.                 *
// *                                                                           *
// *  <grantDepth>                    const int32_t                    In      *
// *    is the number of levels this privilege may be granted by the grantee.  *
// *  Initially this is either 0 or -1.                                        *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Component privilege(s) were granted                          *
// *           *: One or more component privileges were not granted.           *
// *              A CLI error is put into the diags area.                      *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrComponentPrivileges::grantPrivilege(
   const std::string & componentName,
   const std::vector<std::string> & operations,
   const int32_t grantorIDIn,
   const std::string & grantorName,
   const int32_t granteeID,
   const std::string & granteeName,
   const int32_t grantDepth)
  
{

// Determine if the component exists.

PrivMgrComponents component(metadataLocation_,pDiags_);

   if (!component.exists(componentName))
   {
      *pDiags_ << DgSqlCode(-CAT_TABLE_DOES_NOT_EXIST_ERROR)
               << DgTableName(componentName.c_str());
      return STATUS_ERROR;
   }
 
std::string componentUIDString;
int64_t componentUID;
bool isSystemComponent;
std::string tempStr;
PrivStatus privStatus = STATUS_GOOD;

   component.fetchByName(componentName,
                         componentUIDString,
                         componentUID,
                         isSystemComponent,
                         tempStr);
                         
// OK, the component is defined, what about the operations?
MyTable &myTable = static_cast<MyTable &>(myTable_);
PrivMgrComponentOperations componentOperations(metadataLocation_,pDiags_);
std::vector<std::string> operationCodes;
int32_t grantorID = grantorIDIn;

   for (size_t i = 0; i < operations.size(); i ++)
   {
      std::string operationName = operations[i];
      
      //TODO: 
      // If we can't find an operation in the list, we give up.  No warnings or 
      // transactions currently.  Need to revisit this when we have 
      // transactions and warnings.  Also need to consider adding a general
      // mechanism to return what was done.  Useful with lists when one or 
      // more items in the list fail and in cases of "ALL".                                             
      if (!componentOperations.nameExists(componentUID,operationName))
      {
         *pDiags_ << DgSqlCode(-CAT_TABLE_DOES_NOT_EXIST_ERROR)
                  << DgTableName(operationName.c_str());
         return STATUS_ERROR;
      }
      
      std::string operationCode;
      bool isSystemOperation;
      std::string operationDescription;
      
      componentOperations.fetchByName(componentUIDString,
                                      operationName,
                                      operationCode,
                                      isSystemOperation,
                                      operationDescription);
     
      // Component and this operation both exist.
      
      // If grantorID is DB__ROOT, then we use the "owner" of the 
      // operation, which is the user granted the privilege by _SYSTEM.
      // Read COMPONENT_PRIVILEGES to get grantorID.
      //TODO: Need vector of grantorIDs.
      if (grantorIDIn == ComUser::getRootUserID())
      {
         privStatus = myTable.fetchOwner(componentUID,operationCode,
                                         grantorID);
         // If component operation is defined, there should be a grant to the
         // owner from the system. Just in case...                                
         if (privStatus != STATUS_GOOD) 
         {
            *pDiags_ << DgSqlCode(-CAT_INVALID_PRIVILEGE_FOR_GRANT_OR_REVOKE);
            return STATUS_ERROR;
         }
      }
      else
         grantorID = grantorIDIn;
      
      if (!hasWGO(grantorID,componentUIDString,operationCode))
      {
         *pDiags_ << DgSqlCode(-CAT_NOT_AUTHORIZED);
         return STATUS_ERROR;
      }

      operationCodes.push_back(operationCode);
   }
   
//
// Operations are valid, add or update each entry.
//

MyRow row(fullTableName_);

   row.componentUID_ = componentUID;
   row.grantDepth_ = grantDepth;
   row.granteeID_ = granteeID;
   row.granteeName_ = granteeName;
   row.grantorID_ = grantorID;
   row.grantorName_ = grantorName;
   
std::string whereClauseHeader(" WHERE COMPONENT_UID = ");

   whereClauseHeader += componentUIDString;
   whereClauseHeader += " AND GRANTEE_ID = ";          
   whereClauseHeader += authIDToString(granteeID); 
   whereClauseHeader += " AND GRANTOR_ID = ";          
   whereClauseHeader += authIDToString(grantorID); 
   whereClauseHeader += " AND OPERATION_CODE = '";
   
   for (size_t oc = 0; oc < operationCodes.size(); oc++)
   {
      int32_t thisGrantDepth;
      
      // If privilege is already granted, just move on to the next entry.
      // Note, if adding WITH GRANT OPTION, perform an update.
      if (grantExists(componentUIDString,operationCodes[oc],grantorID,granteeID,
                      thisGrantDepth))  
      {
         if (grantDepth == thisGrantDepth || grantDepth == 0)
            continue;
      
         std::string whereClause = whereClauseHeader + operationCodes[oc] + "'";
         
         // Set new grant depth
         std::string setClause(" SET GRANT_DEPTH = ");
         
         char grantDepthString[20];
         
         sprintf(grantDepthString,"%d",grantDepth);
         setClause += grantDepthString + whereClause;
         
         privStatus = myTable.update(setClause);
      }
      else
      {
         row.operationCode_ = operationCodes[oc];
         privStatus = myTable.insert(row);
      }
      
      if (privStatus != STATUS_GOOD)
         return privStatus;
   }
   
   return STATUS_GOOD;

}  
//************* End of PrivMgrComponentPrivileges::grantPrivilege **************

// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentPrivileges::grantPrivilegeInternal              *
// *                                                                           *
// *    Internal function to grant one or more component privileges.           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentUID>                  const int64_t                   In       *
// *    is the unique ID associated with the component.                        *
// *                                                                           *
// *  <operationCodes>                const std::vector<std::string> & In      *
// *    is a list of component operation codes for the operation to be granted.*
// *                                                                           *
// *  <grantorID>                     const int32_t                    In      *
// *    is the auth ID granting the privilege.                                 *
// *                                                                           *
// *  <grantorName>                   const std::string &              In      *
// *    is the name of the authID granting the privilege.                      *
// *                                                                           *
// *  <granteeID>                     const int32_t                    In      *
// *    is the the authID being granted the privilege.                         *
// *                                                                           *
// *  <granteeName>                   const std::string &              In      *
// *    is the name of the authID being granted the privilege.                 *
// *                                                                           *
// *  <grantDepth>                    const int32_t                    In      *
// *    is the number of levels this privilege may be granted by the grantee.  *
// *  Initially this is either 0 or -1.                                        *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Component privilege(s) were granted                          *
// *           *: One or more component privileges were not granted.           *
// *              A CLI error is put into the diags area.                      *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrComponentPrivileges::grantPrivilegeInternal(
   const int64_t componentUID,
   const std::vector<std::string> & operationCodes,
   const int32_t grantorID,
   const std::string & grantorName,
   const int32_t granteeID,
   const std::string & granteeName,
   const int32_t grantDepth)
  
{

MyTable &myTable = static_cast<MyTable &>(myTable_);
MyRow row(fullTableName_);

   row.componentUID_ = componentUID;
   row.grantDepth_ = grantDepth;
   row.granteeID_ = granteeID;
   row.granteeName_ = granteeName;
   row.grantorID_ = grantorID;
   row.grantorName_ = grantorName;
   
   for (size_t oc = 0; oc < operationCodes.size(); oc++)
   {
      row.operationCode_ = operationCodes[oc];
      PrivStatus privStatus = myTable.insert(row);
      
      if (privStatus != STATUS_GOOD)
         return privStatus;
   }
   
   return STATUS_GOOD;

}  
//********* End of PrivMgrComponentPrivileges::grantPrivilegeInternal **********

// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentPrivileges::grantPrivilegeToCreator             *
// *                                                                           *
// *    Grants privilege on operation to creator from _SYSTEM.                 *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentUID>                  const int64_t                   In       *
// *    is the unique ID associated with the component.                        *
// *                                                                           *
// *  <operationCode>                 const std::string &             In       *
// *    is the two character code associated with the component operation.     *
// *                                                                           *
// *  <granteeID>                    const int32_t                    In       *
// *    is the auth ID of the creator of the component operation.              *
// *                                                                           *
// *  <granteeName>                   const std::string &             In       *
// *    is the name of the creator of the component operation.                 *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Component operation was granted to the auth ID by _SYSTEM.   *
// *           *: Component operation was not granted due to I/O error.        *
// *              A CLI error is put into the diags area.                      *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrComponentPrivileges::grantPrivilegeToCreator(
   const int64_t componentUID,
   const std::string & operationCode,
   const int32_t granteeID,
   const std::string & granteeName)
     
{

MyTable &myTable = static_cast<MyTable &>(myTable_);

MyRow row(fullTableName_);

   row.componentUID_ = componentUID;
   row.operationCode_ = operationCode;
   row.granteeID_ = granteeID;
   row.granteeName_ = granteeName;
   row.grantorID_ = SYSTEM_AUTH_ID;
   row.grantorName_ = "_SYSTEM";
   row.grantDepth_ = -1;
   
   return myTable.insert(row);

}  
//************** End of PrivMgrRoles::grantPrivilegeToCreator ******************

// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentPrivileges::hasPriv                             *
// *                                                                           *
// *    Determines if an authID has been granted the privilege on the          *
// * specified component operation.                                            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <authID>                        const int32_t                   In       *
// *    is the authorization ID.                                               *
// *                                                                           *
// *  <componentName>                 const std::string &             In       *
// *    is the name of the component.                                          *
// *                                                                           *
// *  <operationName>                 const std::string &             In       *
// *    is the name of the operation.                                          *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true: This authorization ID has the component privilege.                  *
// * false: This authorization ID does not have the component privilege, or    *
// *   there was an error trying to read from the COMPONENT_PRIVILEGES table.  *
// *                                                                           *
// *****************************************************************************
bool PrivMgrComponentPrivileges::hasPriv(
   const int32_t authID,
   const std::string & componentName,
   const std::string & operationName)
   
{

// Determine if the component exists.

PrivMgrComponents component(metadataLocation_,pDiags_);

   if (!component.exists(componentName))
   {
      *pDiags_ << DgSqlCode(-CAT_TABLE_DOES_NOT_EXIST_ERROR)
               << DgTableName(componentName.c_str());
      return STATUS_ERROR;
   }
 
std::string componentUIDString;
int64_t componentUID;
bool isSystemComponent;
std::string tempStr;

   component.fetchByName(componentName,
                         componentUIDString,
                         componentUID,
                         isSystemComponent,
                         tempStr);
    
// OK, the component is defined, what about the operation?

PrivMgrComponentOperations componentOperations(metadataLocation_,pDiags_);

   if (!componentOperations.nameExists(componentUID,operationName))
   {
      *pDiags_ << DgSqlCode(-CAT_TABLE_DOES_NOT_EXIST_ERROR)
               << DgTableName(operationName.c_str());
      return STATUS_ERROR;
   }
   
std::string operationCode;
bool isSystemOperation;
std::string operationDescription;
   
   componentOperations.fetchByName(componentUIDString,
                                   operationName,
                                   operationCode,
                                   isSystemOperation,
                                   operationDescription);
                                   
std::string whereClause(" WHERE COMPONENT_UID = ");   

   whereClause += componentUIDString;
   whereClause += " AND OPERATION_CODE = '";
   whereClause += operationCode;
   whereClause += "' AND GRANTEE_ID = ";
   whereClause += authIDToString(authID); 
   
int64_t rowCount = 0;   
MyTable &myTable = static_cast<MyTable &>(myTable_);

PrivStatus privStatus = myTable.selectCountWhere(whereClause,rowCount);

   if ((privStatus == STATUS_GOOD || privStatus == STATUS_WARNING) &&
        rowCount > 0)
      return true;
      
   pDiags_->clear();
   return false;

}
//***************** End of PrivMgrComponentPrivileges::hasPriv *****************


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentPrivileges::hasSQLPriv                          *
// *                                                                           *
// *    Determines if an authID has been granted the privilege on the          *
// * SQL_OPERATIONS component operation.                                       *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <authID>                        const int32_t                   In       *
// *    is the authorization ID.                                               *
// *                                                                           *
// *  <operation>                     const SQLOperation              In       *
// *    is the enum for the SQL operation.                                     *
// *                                                                           *
// *  <includeRoles>                  const bool                      In       *
// *    if true, indicates privileges granted to roles granted to the          *
// * authorization ID should be included.                                      *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true: This authorization ID has the component privilege.                  *
// * false: This authorization ID does not have the component privilege, or    *
// *   there was an error trying to read from the COMPONENT_PRIVILEGES table.  *
// *                                                                           *
// *****************************************************************************
bool PrivMgrComponentPrivileges::hasSQLPriv(
   const int32_t authID,
   const SQLOperation operation,
   const bool includeRoles)
   
{


const std::string & operationCode = PrivMgr::getSQLOperationCode(operation);

std::string whereClause(" WHERE COMPONENT_UID = 1 AND (OPERATION_CODE = '");   

   whereClause += operationCode;
   
   if (PrivMgr::isSQLCreateOperation(operation))
   {
      whereClause += "' OR OPERATION_CODE = '";
      whereClause += PrivMgr::getSQLOperationCode(SQLOperation::CREATE);
   }
   else 
      if (PrivMgr::isSQLDropOperation(operation))
      {
         whereClause += "' OR OPERATION_CODE = '";
         whereClause += PrivMgr::getSQLOperationCode(SQLOperation::DROP);
      }
      else
         if (PrivMgr::isSQLAlterOperation(operation))
         {
            whereClause += "' OR OPERATION_CODE = '";
            whereClause += PrivMgr::getSQLOperationCode(SQLOperation::ALTER);
         }
   
   whereClause += "') AND (GRANTEE_ID = -1 OR GRANTEE_ID = ";
   whereClause += authIDToString(authID);
    
// *****************************************************************************
// *                                                                           *
// *   If component privileges granted to roles granted to the authorization   *
// * ID should be considered, get the list of roles granted to the auth ID     *
// * and add each one as a potential grantee.                                  *
// *                                                                           *
// *****************************************************************************
   
   if (includeRoles)
   {
      std::vector<std::string> roleNames;
      std::vector<int32_t> roleIDs;
      std::vector<int32_t> grantDepths;
      
      PrivMgrRoles roles(" ",metadataLocation_,pDiags_);
      
      PrivStatus privStatus = roles.fetchRolesForUser(authID,roleNames,
                                                      roleIDs,grantDepths);
      
      for (size_t r = 0; r < roleIDs.size(); r++)
      {
         whereClause += " OR GRANTEE_ID = ";
         whereClause += authIDToString(roleIDs[r]);
      }
   }
   
   whereClause += ")"; 
   
int64_t rowCount = 0;   
MyTable &myTable = static_cast<MyTable &>(myTable_);

PrivStatus privStatus = myTable.selectCountWhere(whereClause,rowCount);

   if ((privStatus == STATUS_GOOD || privStatus == STATUS_WARNING) &&
        rowCount > 0)
      return true;
      
   pDiags_->clear();
   return false;

}
//*************** End of PrivMgrComponentPrivileges::hasSQLPriv ****************





// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentPrivileges::hasWGO                              *
// *                                                                           *
// *    Determines if an authID has been granted the ability to grant a        *
// * specific component operation.                                             *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <authID>                        const int32_t                   In       *
// *    is the authorization ID.                                               *
// *                                                                           *
// *  <componentUIDString>            const std::string &             In       *
// *    is the unique ID of the component in string format.                    *
// *                                                                           *
// *  <operationCode>                 const std::string &             In       *
// *    is the two character code for the component operation.                 *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true: This authorization ID has with grant option.                        *
// * false: This authorization ID does not have with grant option, or there    *
// *        was an error trying to read from the ROLE_USAGE table.             *
// *                                                                           *
// *****************************************************************************
bool PrivMgrComponentPrivileges::hasWGO(
   int32_t authID,
   const std::string & componentUIDString,
   const std::string & operationCode)
   
{

MyTable &myTable = static_cast<MyTable &>(myTable_);

std::string whereClause (" WHERE GRANTEE_ID = ");

   whereClause += authIDToString(authID); 
   whereClause += " AND COMPONENT_UID = ";          
   whereClause += componentUIDString;
   whereClause += " AND OPERATION_CODE = '";          
   whereClause += operationCode;
   whereClause += "' AND GRANT_DEPTH <> 0";
   
int64_t rowCount = 0;

PrivStatus privStatus = myTable.selectCountWhere(whereClause,rowCount);

   if ((privStatus == STATUS_GOOD || privStatus == STATUS_WARNING) &&
        rowCount > 0)
      return true;
      
   pDiags_->clear();
   return false;
   
}
//***************** End of PrivMgrComponentPrivileges::hasWGO ******************

// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentPrivileges::isAuthIDGrantedPrivs                *
// *                                                                           *
// *    Determines if the specified authorization ID has been granted one or   *
// * more component privileges.                                                *
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
// * true: Authorization ID has been granted one or more component privileges. *
// * false: Authorization ID has not been granted any component privileges.    *
// *                                                                           *
// *****************************************************************************
bool PrivMgrComponentPrivileges::isAuthIDGrantedPrivs(const int32_t authID)

{

std::string whereClause(" WHERE GRANTEE_ID =  ");   

   whereClause += authIDToString(authID); 

int64_t rowCount = 0;   
MyTable &myTable = static_cast<MyTable &>(myTable_);

PrivStatus privStatus = myTable.selectCountWhere(whereClause,rowCount);

   if ((privStatus == STATUS_GOOD || privStatus == STATUS_WARNING) &&
        rowCount > 0)
      return true;
      
   pDiags_->clear();
   return false;

}
//*********** End of PrivMgrComponentPrivileges::isAuthIDGrantedPrivs **********



// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentPrivileges::isGranted                           *
// *                                                                           *
// *    Determines if a component operation has been granted, i.e., if it      *
// * is used in the COMPONENT_PRIVILEGES table.                                *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentUIDString>            const std::string &             In       *
// *    is the component unique ID.                                            *
// *                                                                           *
// *  <operationCode>                 const std::string &             In       *
// *    is the two character code associated with the component operation.     *
// *                                                                           *
// *  <shouldExcludeGrantsBySystem>   const bool                      [In]     *
// *    if true, don't consider the system grant to the creator.               *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true: Component operation has been granted to one or more authIDs.        *
// * false: Component operation has not been granted, or there was an error    *
// * trying to read from the COMPONENT_PRIVILEGES table.                       *
// *                                                                           *
// *****************************************************************************
bool PrivMgrComponentPrivileges::isGranted(
  const std::string & componentUIDString,
  const std::string & operationCode,
  const bool shouldExcludeGrantsBySystem)
  
{

MyRow row(fullTableName_);
MyTable &myTable = static_cast<MyTable &>(myTable_);

std::string whereClause("WHERE COMPONENT_UID = ");

   whereClause += componentUIDString;
   whereClause += " AND OPERATION_CODE = '";
   whereClause += operationCode;
   whereClause += "'";
   if (shouldExcludeGrantsBySystem)
      whereClause += " AND GRANTOR_ID <> -2";
   
PrivStatus privStatus = myTable.selectWhereUnique(whereClause,row);

   if (privStatus == STATUS_GOOD || privStatus == STATUS_WARNING)
      return true;
      
   pDiags_->clear();
   return false;
    
}      
//**************** End of PrivMgrComponentPrivileges::isGranted ****************





// *****************************************************************************
// * Function: PrivMgrComponentPrivileges::revokeAllForGrantor                 *
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
PrivStatus PrivMgrComponentPrivileges::revokeAllForGrantor(
   const int32_t grantorID,
   const std::string componentName,
   const std::string componentUIDString,
   const std::string operationName,
   const std::string operationCode,
   const bool isGOFSpecified,
   const int32_t newGrantDepth,
   PrivDropBehavior dropBehavior) 
   
{

// *****************************************************************************
// *                                                                           *
// *  First, get all the rows where the grantor has granted this operation     *
// * to any authorization ID.                                                  *
// *                                                                           *
// *****************************************************************************

std::string whereClause("WHERE COMPONENT_UID = ");

   whereClause += componentUIDString;
   whereClause += " AND OPERATION_CODE = '";
   whereClause += operationCode;
   whereClause += "' AND GRANTOR_ID = ";
   whereClause += authIDToString(grantorID);
   
std::string orderByClause;
   
std::vector<MyRow> rows;
MyTable &myTable = static_cast<MyTable &>(myTable_);

PrivStatus privStatus = myTable.selectAllWhere(whereClause,orderByClause,rows);

// *****************************************************************************
// *                                                                           *
// *   If the grantor has no active grants for this operation, we are done.    *
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
      //TODO: Need better error message. 
      *pDiags_ << DgSqlCode(-CAT_DEPENDENT_OBJECTS_EXIST);
      return STATUS_ERROR; 
      return STATUS_ERROR;
   }
      
// *****************************************************************************
// *                                                                           *
// *   There are one more grants from the grantor of this operation.  Create a *
// * vector for the operationCode and call revokePrivilege.                    *
// *                                                                           *
// *****************************************************************************
      
std::vector<std::string> operationNames;  

   operationNames.push_back(operationName);  
   
   for (size_t r = 0; r < rows.size(); r++) 
   {
      privStatus = revokePrivilege(componentName,operationNames,grantorID,
                                   rows[r].granteeID_,isGOFSpecified,
                                   newGrantDepth,dropBehavior); 
      if (privStatus != STATUS_GOOD)
         return privStatus;
   }
   
   return STATUS_GOOD;

}
//****************** End of PrivMgrRoles::revokeAllForGrantor ******************



// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentPrivileges::revokePrivilege                     *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentName>                 const std::string &              In      *
// *    is the component name.                                                 *
// *                                                                           *
// *  <operations>                    const std::vector<std::string> & In      *
// *    is a list of component operations to be revoked.                       *
// *                                                                           *
// *  <grantorID>                     const int32_t                    In      *
// *    is the authID revoking the privilege.                                  *
// *                                                                           *
// *  <granteeID>                     const int32_t                    In      *
// *    is the authID the privilege is being revoked from.                     *
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
// * STATUS_GOOD: Component privilege(s) were revoked                          *
// *           *: One or more component privileges were not revoked.           *
// *              A CLI error is put into the diags area.                      *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrComponentPrivileges::revokePrivilege(
   const std::string & componentName,
   const std::vector<std::string> & operations,
   const int32_t grantorID,
   const int32_t granteeID, 
   const bool isGOFSpecified,
   const int32_t newGrantDepth,
   PrivDropBehavior dropBehavior) 
  
{
// Determine if the component exists.

PrivMgrComponents component(metadataLocation_,pDiags_);

   if (!component.exists(componentName))
   {
      *pDiags_ << DgSqlCode(-CAT_TABLE_DOES_NOT_EXIST_ERROR)
               << DgTableName(componentName.c_str());
      return STATUS_ERROR;
   }
 
std::string componentUIDString;
int64_t componentUID;
bool isSystemComponent;
std::string tempStr;

   component.fetchByName(componentName,
                         componentUIDString,
                         componentUID,
                         isSystemComponent,
                         tempStr);
    
// OK, the component is defined, what about the operations?

PrivMgrComponentOperations componentOperations(metadataLocation_,pDiags_);
std::vector<std::string> operationCodes;

   for (size_t i = 0; i < operations.size(); i++)
   {
      std::string operationName = operations[i];
      //TODO: We stop if one on the list is not found.  See comment in
      // grantPrivilege() for more details.                                             
      if (!componentOperations.nameExists(componentUID,operationName))
      {
         *pDiags_ << DgSqlCode(-CAT_TABLE_DOES_NOT_EXIST_ERROR)
                  << DgTableName(operationName.c_str());
         return STATUS_ERROR;
      }
      
      std::string operationCode;
      bool isSystemOperation;
      std::string operationDescription;
      
      componentOperations.fetchByName(componentUIDString,
                                      operationName,
                                      operationCode,
                                      isSystemOperation,
                                      operationDescription);
   
      // Component and this operation both exist. Save operation code.
      operationCodes.push_back(operationCode);
   }
   
PrivStatus privStatus = STATUS_GOOD;
   
   for (size_t oc = 0; oc < operationCodes.size(); oc++)
   {
   
      int32_t grantDepth;
       
      if (!grantExists(componentUIDString,operationCodes[oc],grantorID,
                       granteeID,grantDepth))
      {
         *pDiags_ << DgSqlCode(-CAT_NOT_AUTHORIZED);
         return STATUS_ERROR;
      }
   
      if (!hasWGO(granteeID,componentUIDString,operationCodes[oc]))
         continue;

      privStatus = revokeAllForGrantor(granteeID,
                                       componentName,
                                       componentUIDString,
                                       operations[oc],
                                       operationCodes[oc],
                                       isGOFSpecified,
                                       newGrantDepth,
                                       dropBehavior); 
      if (privStatus != STATUS_GOOD)
      {
         return STATUS_ERROR;
      }
   }
    
MyTable &myTable = static_cast<MyTable &>(myTable_);

std::string whereClauseHeader("WHERE COMPONENT_UID = ");

   whereClauseHeader += componentUIDString;
   whereClauseHeader += " AND GRANTOR_ID = ";
   whereClauseHeader += authIDToString(grantorID);;
   whereClauseHeader += " AND GRANTEE_ID = ";
   whereClauseHeader += authIDToString(granteeID);;
   whereClauseHeader += " AND OPERATION_CODE = '";
   
std::string setClause("SET GRANT_DEPTH = ");

   if (isGOFSpecified)
   {
      char grantDepthString[20];
      
      sprintf(grantDepthString,"%d",newGrantDepth);
      setClause += grantDepthString;
   }
   
bool someNotRevoked = false;
  
   for (size_t c = 0; c < operationCodes.size(); c++)
   {
      std::string whereClause(whereClauseHeader);

      whereClause += operationCodes[c];
      whereClause += "'";
      
      if (isGOFSpecified)
      {
         std::string updateClause = setClause + whereClause;
         
         privStatus = myTable.update(updateClause);
      }
      else
         privStatus = myTable.deleteWhere(whereClause);
      
      if (privStatus == STATUS_NOTFOUND)
      {
         someNotRevoked = true;
         continue;
      }
      
      if (privStatus != STATUS_GOOD)
         return privStatus;
   }
   
   if (someNotRevoked)
   {
      *pDiags_ << DgSqlCode(CAT_NOT_ALL_PRIVILEGES_REVOKED);
      return STATUS_WARNING;
   }
   
   return STATUS_GOOD;
  
}  
//************* End of PrivMgrComponentPrivileges::revokePrivilege *************
  

// *****************************************************************************
//    MyTable methods
// *****************************************************************************


// *****************************************************************************
// *                                                                           *
// * Function: MyTable::fetchOwner                                             *
// *                                                                           *
// *    Reads from the COMPONENT_PRIVILEGES table and returns the authID       *
// *  granted the specified privilege by the system.                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentUID>                  const int64_t                   In       *
// *    is the unique ID associated with the component.                        *
// *                                                                           *
// *  <operationCode>                 const std::string &             In       *
// *    is the two digit code for the operation.                               *
// *                                                                           *
// *  <granteeID>                     int32_t &                       Out      *
// *    passes back the authID granted the privilege by the system.            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Found row with system grantor, grantee returned.             *
// *           *: Row not found or error encountered.                          *
// *                                                                           *
// *****************************************************************************
PrivStatus MyTable::fetchOwner(
   const int64_t componentUID,
   const std::string & operationCode,
   int32_t & granteeID)
   
{

// Check the last row read before reading metadata.

   if (lastRowRead_.grantorID_ == SYSTEM_AUTH_ID &&
       lastRowRead_.componentUID_ == componentUID &&
       lastRowRead_.operationCode_ == operationCode)
   {
      granteeID = lastRowRead_.granteeID_;
      return STATUS_GOOD;
   }    
       
// Not found in cache, look for the system grantor in metadata.
std::string whereClause("WHERE COMPONENT_UID = ");

   whereClause += PrivMgr::UIDToString(componentUID);
   whereClause += " AND OPERATION_CODE = '";
   whereClause += operationCode;
   whereClause += "' AND GRANTOR_ID = -2";
   
MyRow row(tableName_);

PrivStatus privStatus = selectWhereUnique(whereClause,row);
   
   switch (privStatus)
   {
      // Return status to caller to handle
      case STATUS_NOTFOUND:
      case STATUS_ERROR:
         return privStatus;
         break;

      // Object exists 
      case STATUS_GOOD:
      case STATUS_WARNING:
         granteeID = row.granteeID_;
         return STATUS_GOOD;
         break;

      // Should not occur, internal error
      default:
         PRIVMGR_INTERNAL_ERROR("Switch statement in PrivMgrComponentPrivileges::MyTable::fetchOwner()");
         return STATUS_ERROR;
         break;
   }
   
   return STATUS_GOOD;

}   
//********************** End of MyTable::fetchByName ***************************




// *****************************************************************************
// *                                                                           *
// * Function: MyTable::insert                                                 *
// *                                                                           *
// *    Inserts a row into the COMPONENT_PRIVILEGES table.                     *
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

char insertStatement[1000];

const MyRow & row = static_cast<const MyRow &>(rowIn);

   sprintf(insertStatement, "insert into %s values (%d, %d, %ld, '%s', '%s', '%s', %d)",     
           tableName_.c_str(),
           row.granteeID_,
           row.grantorID_,
           row.componentUID_,
           row.operationCode_.c_str(),
           row.granteeName_.c_str(),
           row.grantorName_.c_str(),
           row.grantDepth_);

   return CLIImmediate(insertStatement);

}
//************************** End of MyTable::insert ****************************


// *****************************************************************************
// *                                                                           *
// * Function: MyTable::selectAllWhere                                         *
// *                                                                           *
// *    Selects rows from the COMPONENT_PRIVILEGES table based on the          *
// *  specified WHERE clause and sorted per an optional ORDER BY clause.       *
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

std::string selectStmt ("SELECT COMPONENT_UID, OPERATION_CODE, GRANTEE_ID, GRANTOR_ID, GRANTEE_NAME, GRANTOR_NAME, GRANT_DEPTH FROM ");  
   selectStmt += tableName_;
   selectStmt += " ";
   selectStmt += whereClause;
   selectStmt += " ";
   selectStmt += orderByClause;
  
Queue * tableQueue = NULL;

PrivStatus privStatus = executeFetchAll(selectStmt,tableQueue);

   tableQueue->position();
   for (int idx = 0; idx < tableQueue->numEntries(); idx++)
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
// *    Selects a row from the COMPONENT_PRIVILEGES table based on the         *
// *  specified WHERE clause.                                                  *
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
std::string selectStmt ("SELECT COMPONENT_UID, OPERATION_CODE, GRANTEE_ID, GRANTOR_ID, GRANTEE_NAME, GRANTOR_NAME, GRANT_DEPTH FROM ");  
selectStmt += tableName_;
selectStmt += " ";
selectStmt += whereClause;

ExeCliInterface cliInterface(STMTHEAP);

PrivStatus privStatus = CLIFetch(cliInterface,selectStmt);   
   
   if (privStatus != STATUS_GOOD && privStatus != STATUS_WARNING)
      return privStatus;   

char * ptr = NULL;
Lng32 len = 0;
char value[500];
  
MyRow & row = static_cast<MyRow &>(rowOut);
  
   // column 1:  component_uid
   cliInterface.getPtrAndLen(1,ptr,len);
   row.componentUID_ = *(reinterpret_cast<int64_t*>(ptr));

   // column 2:  operation_code
   cliInterface.getPtrAndLen(2,ptr,len);
   strncpy(value,ptr,len);
   value[len] = 0;
   row.operationCode_ = value;

   // column 3:  granteeID
   cliInterface.getPtrAndLen(3,ptr,len);
   row.granteeID_ = *(reinterpret_cast<int32_t*>(ptr));

   // column 4:  grantorID
   cliInterface.getPtrAndLen(4,ptr,len);
   row.grantorID_ = *(reinterpret_cast<int32_t*>(ptr));

   // column 5:  grantee_name
   cliInterface.getPtrAndLen(5,ptr,len);
   strncpy(value,ptr,len);
   value[len] = 0;
   row.granteeName_ = value;

   // column 6:  grantor_name
   cliInterface.getPtrAndLen(6,ptr,len);
   strncpy(value,ptr,len);
   value[len] = 0;
   row.grantorName_ = value;

   // column 7:  grant_depth
   cliInterface.getPtrAndLen(7,ptr,len);
   row.grantDepth_ = *(reinterpret_cast<int32_t*>(ptr));
   
   lastRowRead_ = row;

   return STATUS_GOOD;
   
}
//************************ End of MyTable::selectWhere *************************


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
std::string selectStmt ("SELECT COMPONENT_UID, OPERATION_CODE, GRANTEE_ID, GRANTOR_ID, GRANTEE_NAME, GRANTOR_NAME, GRANT_DEPTH FROM ");  
  
   // column 1:  component_uid
   cliInterface.get(0,ptr,length);
   row.componentUID_ = *(reinterpret_cast<int64_t*>(ptr));

   // column 2:  operation_code
   cliInterface.get(1,ptr,length);
   strncpy(value,ptr,length);
   value[length] = 0;
   row.operationCode_ = value;

   // column 3:  grantee_ID
   cliInterface.get(2,ptr,length);
   row.granteeID_ = *(reinterpret_cast<int32_t*>(ptr));

   // column 4:  grantor_ID
   cliInterface.get(3,ptr,length);
   row.grantorID_ = *(reinterpret_cast<int32_t*>(ptr));

   // column 5:  grantee_name
   cliInterface.get(4,ptr,length);
   strncpy(value,ptr,length);
   value[length] = 0;
   row.granteeName_ = value;

   // column 6:  grantor_name
   cliInterface.get(5,ptr,length);
   strncpy(value,ptr,length);
   value[length] = 0;
   row.grantorName_ = value;

   // column 7:  grant_depth
   cliInterface.get(6,ptr,length);
   row.grantDepth_ = *(reinterpret_cast<int32_t*>(ptr));
   
   lastRowRead_ = row;

}
//************************** End of MyTable::setRow ****************************


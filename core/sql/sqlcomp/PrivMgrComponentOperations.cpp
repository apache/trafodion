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
  
#include "PrivMgrComponentOperations.h"
#include "PrivMgrMD.h"
#include "PrivMgrMDTable.h"
#include "PrivMgrComponents.h"
#include "PrivMgrComponentPrivileges.h"

#include <string>
#include <cstdio>
#include "ComSmallDefs.h"

// sqlcli.h included because ExExeUtilCli.h needs it (and does not include it!)
#include "sqlcli.h"
#include "ExExeUtilCli.h"
#include "ComQueue.h"
#include "ComDiags.h"
// CmpCommon.h contains STMTHEAP declaration
#include "CmpCommon.h"
#include "CmpContext.h"
#include "CmpDDLCatErrorCodes.h"
#include "ComUser.h"

namespace ComponentOperations 
{
// *****************************************************************************
// * Class:         MyRow
// * Description:  This class represents a component operations row which contains:
// *                - UID of the component 
// *                - 2 character code of the component operation
// *                - ANSI name of the component operation
// *                - Description of the component operation
// *                
// *    A component operation can be uniquely identified by its component UID 
// *    and either its ANSI name or its 2 character code.
// *****************************************************************************
class MyRow : public PrivMgrMDRow
{
public:

// -------------------------------------------------------------------
// Constructors and destructors:
// -------------------------------------------------------------------
   MyRow(std::string tableName)
   : PrivMgrMDRow(tableName, COMPONENT_OPERATIONS_ENUM),
     componentUID_(0)
   { };
   MyRow(const MyRow &other)
   : PrivMgrMDRow(other)
   {
      componentUID_ = other.componentUID_;              
      operationCode_ = other.operationCode_;
      operationName_ = other.operationName_;
      operationType_ = other.operationType_;
      operationDescription_ = other.operationDescription_;
   };
   virtual ~MyRow() {};
   
   inline void clear() {componentUID_ = 0;};
   
   bool lookupByCode(
      const int64_t componentUID,
      const std::string & operationCode,
      std::string & operationName,
      PrivMgrComponentOperations::OperationType & operationType,
      std::string & operationDescription); 
   
   bool lookupByName(
      const int64_t componentUID,
      const std::string & operationName,
      std::string & operationCode,
      PrivMgrComponentOperations::OperationType & operationType,
      std::string & operationDescription);
    
// -------------------------------------------------------------------
// Data Members:
// -------------------------------------------------------------------

//  From COMPONENT_OPERATIONS
    int64_t            componentUID_;
    std::string        operationCode_;
    std::string        operationName_;
    PrivMgrComponentOperations::OperationType      operationType_;
    std::string        operationDescription_;
    
private: 
   MyRow();
  
};


// *****************************************************************************
// * Class:         MyTable
// * Description:  This class represents the COMPONENT_OPERATIONS table containing:
// *                - the fully qualified name of the table 
// *                
// *    A component operation can be uniquely identified by a component UID and 
// * either the name of the operation or the operation code.
// *****************************************************************************
class MyTable : public PrivMgrMDTable
{
public:
   MyTable(
      const std::string & tableName,
      PrivMgrTableEnum myTableEnum,
      ComDiagsArea * pDiags) 
   : PrivMgrMDTable(tableName,COMPONENT_OPERATIONS_ENUM, pDiags),
     lastRowRead_(tableName)
    {};
   
   inline void clear() { lastRowRead_.clear(); };
   
   PrivStatus fetchByCode(
      const int64_t componentUID,
      const std::string & operationCode,
      MyRow & row);
      
   PrivStatus fetchByName(
      const std::string & componentUIDString,
      const std::string & operationName,
      MyRow & row);
      
   PrivStatus fetchByName(
      const int64_t componentUID,
      const std::string & operationName,
      MyRow & row);
      
   virtual PrivStatus insert(const PrivMgrMDRow &row);
   
   virtual PrivStatus selectWhereUnique(
      const std::string & whereClause,
      PrivMgrMDRow & row);

   PrivStatus selectWhere(
      const std::string & whereClause,  
      std::vector<MyRow *> &rowList);

   PrivStatus update(
      const std::string &setClause,
      const std::string &whereClause);

private:   
   MyTable();
   void setRow(OutputInfo *pCliRow, MyRow &rowOut);
   MyRow lastRowRead_;

};
}//End namespace ComponentOperations
using namespace ComponentOperations;

// *****************************************************************************
//    PrivMgrComponentOperations methods
// *****************************************************************************
// -----------------------------------------------------------------------
// Construct a PrivMgrComponentOperations object for a new component operation.
// -----------------------------------------------------------------------
PrivMgrComponentOperations::PrivMgrComponentOperations(
   const std::string & metadataLocation,
   ComDiagsArea * pDiags)
: PrivMgr(metadataLocation, pDiags),
  fullTableName_(metadataLocation_ + "." + PRIVMGR_COMPONENT_OPERATIONS),
  myTable_(*new MyTable(fullTableName_,COMPONENT_OPERATIONS_ENUM, pDiags)) 
{ };

// -----------------------------------------------------------------------
// Copy constructor
// -----------------------------------------------------------------------
PrivMgrComponentOperations::PrivMgrComponentOperations(const PrivMgrComponentOperations &other)
: PrivMgr(other),
  myTable_(*new MyTable(fullTableName_,COMPONENT_OPERATIONS_ENUM, pDiags_))
{
   fullTableName_ = other.fullTableName_;
}

// -----------------------------------------------------------------------
// Destructor.
// -----------------------------------------------------------------------
PrivMgrComponentOperations::~PrivMgrComponentOperations() 
{ 

   delete &myTable_;

}

// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentOperations::clear                               *
// *                                                                           *
// *    This function clears any cache associated with this object.            *
// *                                                                           *
// *****************************************************************************
void PrivMgrComponentOperations::clear()

{

MyTable &myTable = static_cast<MyTable &>(myTable_);

   myTable.clear();
   
}
//******************* End of PrivMgrComponentOperations::clear *****************


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentOperations::codeExists                          *
// *                                                                           *
// *    This function determines if a specific component operation code has    *
// * been defined in Privilege Manager metadata.                               *
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
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// *  true: Operation has been created.                                        *
// * false: Operation does not exist or error encountered.                     *
// *                                                                           *
// *****************************************************************************
bool PrivMgrComponentOperations::codeExists(
   const int64_t componentUID,
   const std::string & operationCode)

{

MyRow row(fullTableName_);
MyTable &myTable = static_cast<MyTable &>(myTable_);

PrivStatus privStatus = myTable.fetchByCode(componentUID,operationCode,row);

   if (privStatus == STATUS_GOOD || privStatus == STATUS_WARNING)
      return true;
      
   return false;
      
}
//********************* End of PrivMgrComponents::codeExists *******************



// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentOperations::createOperation                     *
// *                                                                           *
// *    Add an operation for the specified component to the                    *
// *  COMPONENT_OPERATIONS table.                                              *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentName>                 const std::string &             In       *
// *    is the component name.                                                 *
// *                                                                           *
// *  <operationName>                 const std::string &             In       *
// *    is the name of the operation to be added.                              *
// *                                                                           *
// *  <operationCode>                 const std::string &             In       *
// *    is a 2 character code associated with the operation unique to the      *
// *    component.                                                             *
// *                                                                           *
// *  <isSystem>                      bool                            In       *
// *    is true if the operation is a system operation.                        *
// *                                                                           *
// *  <operationDescription>          const std::string &             In       *
// *    is a descrption of the operation.                                      *
// *                                                                           *
// *  <existsErrorOK>                 const bool                      [In]     *
// *    if true, exists errors are silently ignored.                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Row added.                                                   *
// *           *: Create failed. A CLI error is put into the diags area.       *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrComponentOperations::createOperation(
   const std::string & componentName,
   const std::string & operationName,
   const std::string & operationCode,
   bool isSystem,
   const std::string & operationDescription,
   const bool existsErrorOK) 
  
{

PrivMgrComponentPrivileges componentPrivileges(metadataLocation_, pDiags_);

   if (!ComUser::isRootUserID()&&
       !componentPrivileges.hasSQLPriv(ComUser::getCurrentUser(), SQLOperation::MANAGE_COMPONENTS, true))
   {   
      *pDiags_ << DgSqlCode(-CAT_NOT_AUTHORIZED);
      return STATUS_ERROR;
   }
   
   if (operationCode.size() != 2 || (operationCode.size() == 2 &&
       (operationCode[0] == ' ' || operationCode[1] == ' ')))
   {
      *pDiags_ << DgSqlCode(-CAT_CODE_MUST_CONTAIN_2_NONBLANKS);
      return STATUS_ERROR;
   }

// Determine if the component exists.

PrivMgrComponents component(metadataLocation_,pDiags_);

   if (!component.exists(componentName))
   {
      *pDiags_ << DgSqlCode(-CAT_TABLE_DOES_NOT_EXIST_ERROR)
               << DgTableName(componentName.c_str());
      return STATUS_ERROR;
   }
 
// Component exists, fetch data for this component

std::string componentUIDString;
int64_t componentUID;
bool isSystemComponent;
std::string tempStr;
 
   component.fetchByName(componentName,
                         componentUIDString,
                         componentUID,
                         isSystemComponent,
                         tempStr);
   
// OK, the component is defined, what about the operation?  If it already is
// defined, return an error. Both the operation name and code must be 
// unique within a component.

   if (nameExists(componentUID,operationName))
   {
      if (existsErrorOK)
         return STATUS_GOOD;
         
      *pDiags_ << DgSqlCode(-CAT_COMPONENT_PRIVILEGE_NAME_EXISTS)
               << DgString0(operationName.c_str());
      return STATUS_ERROR;
   }

   if (codeExists(componentUID,operationCode))
   {
      if (existsErrorOK)
         return STATUS_GOOD;
         
      *pDiags_ << DgSqlCode(-CAT_COMPONENT_PRIVILEGE_CODE_EXISTS)
               << DgString0(operationCode.c_str());
      return STATUS_ERROR;
   }

// An operation can only be a system operation if its component is a 
// system component.   
   if (isSystem && !isSystemComponent)
   {
      *pDiags_ << DgSqlCode(-CAT_COMPONENT_NOT_SYSTEM);
      return STATUS_ERROR;
   }
   
// Name and code are not used, add an entry.    
MyRow row(fullTableName_);

   row.componentUID_ = componentUID;
   row.operationCode_ = operationCode;
   row.operationName_ = operationName;
   row.operationType_ = (isSystem ? OP_TYPE_SYSTEM : OP_TYPE_USER);
   row.operationDescription_ = operationDescription;
   
MyTable &myTable = static_cast<MyTable &>(myTable_);

PrivStatus privStatus = myTable.insert(row);
   
   if (privStatus != STATUS_GOOD)
      return privStatus;
      
// Grant authority to creator
PrivMgrComponentPrivileges componentPrivilege(metadataLocation_,pDiags_);

   return componentPrivilege.grantPrivilegeToCreator(componentUID,
                                                     operationCode,
                                                     ComUser::getCurrentUser(),
                                                     ComUser::getCurrentUsername());
}  
//************ End of PrivMgrComponentOperations::createOperation **************
  


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentOperations::createOperationInternal             *
// *                                                                           *
// *    Add an operation for the specified component to the                    *
// *  COMPONENT_OPERATIONS table.                                              *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentUID>                  const int64_t                   In       *
// *    is a unique ID for the component.                                      *
// *                                                                           *
// *  <operationName>                 const std::string &             In       *
// *    is the name of the operation to be added.                              *
// *                                                                           *
// *  <operationCode>                 const std::string &             In       *
// *    is a 2 character code associated with the operation unique to the      *
// *    component.                                                             *
// *                                                                           *
// *  <operationTypeUnused>           const bool                      In       *
// *    type of component, user, system, or unused.                            *
// *                                                                           *
// *  <operationDescription>          const std::string &             In       *
// *    is a descrption of the operation.                                      *
// *                                                                           *
// *  <granteeID>                     const int32_t                    In      *
// *    is the the authID to be granted the privilege on the newly created     *
// *  component operation.                                                     *
// *                                                                           *
// *  <granteeName>                   const std::string &              In      *
// *    is the name of the authID to be granted the privilege on the newly     *
// *  created component operation.                                             *
// *                                                                           *
// *  <grantDepth>                    const int32_t                    In      *
// *    is the number of levels this privilege may be granted by the grantee.  *
// *  Initially this is either 0 or -1.                                        *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Row added.                                                   *
// *           *: Create failed. A CLI error is put into the diags area.       *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrComponentOperations::createOperationInternal(
   const int64_t componentUID,
   const std::string & operationName,
   const std::string & operationCode,
   const bool operationTypeUnused,
   const std::string & operationDescription,
   const int32_t granteeID,
   const std::string & granteeName,
   const int32_t grantDepth,
   const bool checkExistence)
{
   PrivStatus privStatus = STATUS_GOOD;

   // If operation already created, no need to create
   if (checkExistence && nameExists(componentUID,operationName))
      return STATUS_GOOD;

   MyRow row(fullTableName_);

   row.componentUID_ = componentUID;
   row.operationCode_ = operationCode;
   row.operationName_ = operationName;
   row.operationType_ = (operationTypeUnused ? OP_TYPE_UNUSED : OP_TYPE_SYSTEM);
   row.operationDescription_ = operationDescription;
   
   MyTable &myTable = static_cast<MyTable &>(myTable_);

   privStatus = myTable.insert(row);
   
   if (privStatus != STATUS_GOOD)
      return privStatus;
      
   // Grant authority to creator
   PrivMgrComponentPrivileges componentPrivileges(metadataLocation_,pDiags_);

   std::vector<std::string> operationCodes;

   operationCodes.push_back(operationCode);                                                     
                                                     
   privStatus = componentPrivileges.grantPrivilegeInternal(componentUID,
                                                           operationCodes,
                                                           SYSTEM_USER,
                                                           ComUser::getSystemUserName(),
                                                           granteeID,
                                                           granteeName,grantDepth,
                                                           checkExistence);
                                                     
   return privStatus;
                                                     
}  
//************ End of PrivMgrComponentOperations::createOperation **************
  

// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentOperations::describeComponentOperations         *
// *                                                                           *
// *    Reads all rows of componentUIDString from COMPONENT_OPERATIONS,        *
// *    for each row,                                                          *
// *    generate a CREATE COMPONENT PRIVILEGE statement,                       *
// *    and call PrivMgrComponentPrivileges::describeComponentPrivileges()     *
// *    to generate GRANT COMPONENT PRIVILEGE statements.                      *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  componentUIDString            const std::string &                   In   *
// *    is the component unique ID as a numeric string.                        *
// *                                                                           *
// *  componentName                 const std::string &                   In   *
// *    is the name of the component                                           *
// *                                                                           *
// *  outlines                            std::vector<std::string> &      Out  *
// *    array of strings with CREATE and GRANT statements                      *
// *                                                                           *
// *  componentPrivileges       PrivMgrComponentPrivileges *              In   *
// *    if specified use PrivMgrComponentPrivileges object                     *
// *    to generate GRANT statement for each component operation               *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// *     STATUS_GOOD: Row read successfully, data returned.                    *
// *                                                                           *
// * STATUS_NOTFOUND: No rows that matched, or error encountered.              *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrComponentOperations::describeComponentOperations(
    const std::string & componentUIDString,
    const std::string & componentName,
    std::vector<std::string> & outlines,
    PrivMgrComponentPrivileges * componentPrivileges)
{
  std::vector<MyRow *> rowList;

  MyTable &myTable = static_cast<MyTable &>(myTable_);
  
  std::string whereClause("WHERE COMPONENT_UID = ");
  whereClause += componentUIDString;
  whereClause += " and is_system <> 'U'";
  
  PrivStatus privStatus = myTable.selectWhere(whereClause, rowList);

 if (privStatus == STATUS_GOOD)
 {
   for(int i = 0; i < rowList.size(); i++)
   {
      MyRow* myRow = rowList[i];
      if (myRow->operationType_ == OP_TYPE_UNUSED)
        continue;

      std::string componentText;
      componentText += "CREATE COMPONENT PRIVILEGE ";
      componentText += myRow->operationName_ + " AS "; 
      componentText += "'" + myRow->operationCode_ + "'";
      componentText += " ON " + componentName;
      
      if(myRow->operationType_ == OP_TYPE_SYSTEM)
        componentText += " SYSTEM";

      if(!myRow->operationDescription_.empty())
          componentText += " DETAIL '" + myRow->operationDescription_ + "'";

      componentText += ";";
      outlines.push_back(componentText);
      outlines.push_back("");
      if(componentPrivileges)
        componentPrivileges->describeComponentPrivileges(componentUIDString, 
                                                         componentName,
                                                         myRow->operationCode_,
                                                         myRow->operationName_,
                                                         outlines);
      delete myRow;
      outlines.push_back("");
   }
 }
 return privStatus;
}
//****** End of PrivMgrComponentOperations::describeComponentOperations ********




// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentOperations::dropAll                             *
// *                                                                           *
// *    Deletes all rows in the COMPONENT_OPERATIONS table.                    *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Row(s) deleted.                                              *
// *           *: Delete failed. A CLI error is put into the diags area.       *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrComponentOperations::dropAll()
  
{

std::string whereClause (" ");

MyTable &myTable = static_cast<MyTable &>(myTable_);

PrivStatus privStatus = myTable.deleteWhere(whereClause);

   if (privStatus != STATUS_GOOD)
      return privStatus;
      
PrivMgrComponentPrivileges componentPrivileges(metadataLocation_,pDiags_);
   
   return componentPrivileges.dropAll();

}
//**************** End of PrivMgrComponentOperations::dropAll ******************


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentOperations::dropAll                             *
// *                                                                           *
// *    Deletes all rows in the COMPONENT_OPERATIONS table that match the      *
// *  specified component unique ID.  In addition, and granted privileges      *
// *  for the component are deleted.                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentUID>                  const std::string &             In       *
// *    is the component unique ID.  All rows containing this UID will be      *
// *  deleted.                                                                 *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Row(s) deleted.                                              *
// *           *: Delete failed. A CLI error is put into the diags area.       *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrComponentOperations::dropAll(const std::string & componentUID)
  
{

std::string whereClause ("WHERE COMPONENT_UID = ");

   whereClause += componentUID;
   
MyTable &myTable = static_cast<MyTable &>(myTable_);

PrivStatus privStatus = myTable.deleteWhere(whereClause);

   if (privStatus != STATUS_GOOD)
      return privStatus;
      
PrivMgrComponentPrivileges componentPrivileges(metadataLocation_,pDiags_);
   
   return componentPrivileges.dropAllForComponent(componentUID);

}
//**************** End of PrivMgrComponentOperations::dropAll  *****************




// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentOperations::dropOperation                       *
// *                                                                           *
// *    Deletes operation for the specified component from the                 *
// *  COMPONENT_OPERATIONS table.                                              *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentName>                 const std::string &             In       *
// *    is the component name.                                                 *
// *                                                                           *
// *  <operationName>                 const std::string &             In       *
// *    is the operation name.                                                 *
// *                                                                           *
// *  <dropBehavior>                  PrivDropBehavior                In       *
// *    indicates whether restrict or cascade behavior is requested.           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Row(s) deleted.                                              *
// *           *: Delete failed. A CLI error is put into the diags area.       *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrComponentOperations::dropOperation(
   const std::string & componentName,
   const std::string & operationName,
   PrivDropBehavior dropBehavior) 
  
{

PrivMgrComponentPrivileges componentPrivileges(metadataLocation_, pDiags_);

   if (!ComUser::isRootUserID()&&
       !componentPrivileges.hasSQLPriv(ComUser::getCurrentUser(), SQLOperation::MANAGE_COMPONENTS, true))
   {   
      *pDiags_ << DgSqlCode(-CAT_NOT_AUTHORIZED);
      return STATUS_ERROR;
   }

// Determine if the component exists.

PrivMgrComponents component(metadataLocation_,pDiags_);

   if (!component.exists(componentName))
   {
      *pDiags_ << DgSqlCode(-CAT_TABLE_DOES_NOT_EXIST_ERROR)
               << DgTableName(componentName.c_str());
      return STATUS_ERROR;
   }
   
// Component exists, fetch data for this component

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

   if (!nameExists(componentUID,operationName))
   {
      *pDiags_ << DgSqlCode(-CAT_TABLE_DOES_NOT_EXIST_ERROR)
               << DgTableName(operationName.c_str());
      return STATUS_ERROR;
   }
   
//Operation exists, get the data.

std::string operationCode;
bool isSystemOperation = FALSE;

   fetchByName(componentUIDString,operationName,operationCode,isSystemOperation,
               tempStr);
                         
//
// Has operation been granted to any authID?
// 

   if (dropBehavior == PrivDropBehavior::RESTRICT &&
       componentPrivileges.isGranted(componentUIDString,operationCode,true))  
   {
      *pDiags_ << DgSqlCode(-CAT_DEPENDENT_OBJECTS_EXIST);
      return STATUS_ERROR; 
   }
   
// Either CASCADE, or RESTRICT and there are no user grants - drop away!   
PrivStatus privStatus = componentPrivileges.dropAllForOperation(componentUIDString,
                                                                operationCode);
   
   if (privStatus != STATUS_GOOD)
      return privStatus;

// Delete row in COMPONENT_OPERATIONS table.      
MyTable &myTable = static_cast<MyTable &>(myTable_);

std::string whereClause("WHERE COMPONENT_UID = ");

   whereClause += componentUIDString;
   whereClause += " AND OPERATION_NAME = '";
   whereClause += operationName;
   whereClause += "'";
   
   return myTable.deleteWhere(whereClause);

}
//************* End of PrivMgrComponentOperations::dropOperation ***************


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentOperations::fetchByName                         *
// *                                                                           *
// *    This function reads the row in the COMPONENT_OPERATIONS tables for     *
// *  the specified component operation and returns the associated operation   *
// *  code and description.                                                    *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentUIDString>            const std::string &             In       *
// *    is the component unique ID as a numeric string.                        *
// *                                                                           *
// *  <operationName>                 const std::string &             In       *
// *    is the name of the component operation in upper case.                  *
// *                                                                           *
// *  <operationCode>                 std::string &                   Out      *
// *    passes back the code associated with the component operation.          *
// *                                                                           *
// *  <isSystem>                      bool &                          Out      *
// *    passes back true if the component operation is a system level          *
// *                                                                           *
// *  <operationDescription>          std::string &                   Out      *
// *    passes back the description of the component operation.                *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// *     STATUS_GOOD: Row read successfully, data returned.                    *
// *                                                                           *
// * STATUS_NOTFOUND: No rows that matched, or error encountered.              *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrComponentOperations::fetchByName(
   const std::string & componentUIDString,
   const std::string & operationName,
   std::string & operationCode,
   bool isSystem,
   std::string & operationDescription) 
   
{
  
MyRow row(fullTableName_);
MyTable &myTable = static_cast<MyTable &>(myTable_);

PrivStatus privStatus = myTable.fetchByName(componentUIDString,operationName,row);

   if (privStatus == STATUS_NOTFOUND || privStatus == STATUS_ERROR)
      return STATUS_NOTFOUND;

   operationCode = row.operationCode_;
   isSystem = (row.operationType_ == OP_TYPE_SYSTEM);
   operationDescription = row.operationDescription_;
   return STATUS_GOOD;

}
//*************** End of PrivMgrComponentOperations::fetchByName ***************



// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentOperations::getCount                            *
// *                                                                           *
// *    Returns:                                                               *
// *       the total number of operations                                      *
// *       the number of unused operations                                     *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// *    STATUS_GOOD     : found operations                                     *
// *    STATUS_NOTFOUND : no operations were found                             *
// *    STATUS_ERROR    : unexpected error reading metadata                    *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrComponentOperations::getCount(
  const int64_t &componentUID,
  int32_t &numOps,
  int32_t &numUnusedOps)
{
  char buf[getMetadataLocation().size() + 300];
  snprintf (buf, sizeof(buf), "select distinct is_system, count(is_system) over "
            "(partition by is_system) from %s.%s where component_uid = %ld",
            getMetadataLocation().c_str(),PRIVMGR_COMPONENT_OPERATIONS,
            componentUID);

  // set pointer in diags area
  int32_t diagsMark = pDiags_->mark();

  ExeCliInterface cliInterface(STMTHEAP, 0, NULL,
  CmpCommon::context()->sqlSession()->getParentQid());
  Queue * tableQueue = NULL;
  int32_t cliRC =  cliInterface.fetchAllRows(tableQueue, buf, 0, false, false, true);

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

  numOps = 0;
  numUnusedOps = 0;

  char * ptr = NULL;
  int32_t len = 0;
  char value[3];

  int32_t opTypeCount;

  // column 0: operation type 
  // column 1: count of rows for operation type
  tableQueue->position();
  for (int idx = 0; idx < tableQueue->numEntries(); idx++)
  {
    OutputInfo * pCliRow = (OutputInfo*)tableQueue->getNext();
    pCliRow->get(0,ptr,len);
    strncpy(value,ptr,len);
    value[len] = 0;
    pCliRow->get(1,ptr,len);
    opTypeCount = *(reinterpret_cast<int32_t*>(ptr));

    numOps += opTypeCount;
    if (value[0] == 'U')
      numUnusedOps += opTypeCount;
  }

  return STATUS_GOOD;
}
//***************** End of PrivMgrComponentOperations::getCount ****************



// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentOperations::isComponentUsed                     *
// *                                                                           *
// *    Determines if a component is used in the COMPONENT_OPERATIONS table.   *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentUIDString>            const std::string &             In       *
// *    is the component unique ID as a numeric string.                        *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// * true: Component is used in COMPONENT_OPERATIONS table.                    *
// * false: Component is used in COMPONENT_OPERATIONS table, or error trying   *
// *        to read from COMPONENT_OPERATIONS table.                           *
// *                                                                           *
// *****************************************************************************
bool PrivMgrComponentOperations::isComponentUsed(const std::string & componentUIDString)

{

MyRow row(fullTableName_);

std::string whereClause("WHERE COMPONENT_UID = ");

   whereClause += componentUIDString;
   
// set pointer in diags area
int32_t diagsMark = pDiags_->mark();

MyTable &myTable = static_cast<MyTable &>(myTable_);
PrivStatus privStatus = myTable.selectWhereUnique(whereClause,row);

   if (privStatus == STATUS_GOOD || privStatus == STATUS_WARNING)
      return true;

// If not found or any other error is returned, rewind the diagnostics and
// return false.        
   pDiags_->rewind(diagsMark);
   return false;
    
}      
//************* End of PrivMgrComponentOperations::isComponentUsed *************


// *****************************************************************************
//  method:  updateOperationCodes
// 
// Goes through the ComponentOpStruct for the sql_operations component and
//   creates two lists:
//     list of unused operations
//     list of system operations.
//
// Updates the component_operations table and 
//   sets is_system to "U" for unused operations
//   sets is_system to "Y" for system operations
//
// TBD - add support for all components, not just sql_operations
// *****************************************************************************
PrivStatus PrivMgrComponentOperations::updateOperationCodes(
  const int64_t & componentUID  )
{
   if (componentUID != SQL_OPERATIONS_COMPONENT_UID)
   {
      PRIVMGR_INTERNAL_ERROR("Invalid component UID in PrivMgrComponentOperations::updateOperationCodes");
      return STATUS_ERROR;
   }

   std::string unusedItems ("where component_uid = ");
   unusedItems += UIDToString(componentUID);
   unusedItems += " and operation_code in (";
   std::string systemItems(unusedItems);

   size_t numOps = sizeof(sqlOpList)/sizeof(ComponentOpStruct);
   bool firstUnusedOp = true;
   bool firstSystemOp = true;
   for (int i = 0; i < numOps; i++)
   {
      const ComponentOpStruct &opDefinition = sqlOpList[i];
      if (opDefinition.unusedOp)
      {
         if (firstUnusedOp)
         {
            unusedItems += "'";
            firstUnusedOp = false;
         }
         else
            unusedItems += ", '";

         unusedItems += opDefinition.operationCode;
         unusedItems += "'";
      }


     else
      {
         if (firstSystemOp)
         {
            systemItems += "'";
            firstSystemOp = false;
         }
         else
            systemItems += ", '";

         systemItems += opDefinition.operationCode;
         systemItems += "'";
      }

   }

   MyTable &myTable = static_cast<MyTable &>(myTable_);

   // Change system components to unused components
   if (!firstUnusedOp)
   {
      unusedItems += ")";
      std::string setClause("set is_system = 'U' ");
      if (myTable.update(setClause, unusedItems) == STATUS_ERROR)
         return STATUS_ERROR;
   }

    // Change unused components to system components
   if (!firstSystemOp)
   {
      systemItems += ")";
      std::string setClause("set is_system = 'Y' ");
      if (myTable.update(setClause, systemItems) == STATUS_ERROR)
         return STATUS_ERROR;
   }
   return STATUS_GOOD;
}

// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponentOperations::nameExists                          *
// *                                                                           *
// *    This function determines if a specific component operation has been    *
// * defined in Privilege Manager metadata.                                    *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentUID>                  const int64_t                   In       *
// *    is the unique ID associated with the component.                        *
// *                                                                           *
// *  <operationName>                 const std::string &             In       *
// *    is the name of the operation in upper case.                            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// *  true: Operation has been created.                                        *
// * false: Operation does not exist or error encountered.                     *
// *                                                                           *
// *****************************************************************************
bool PrivMgrComponentOperations::nameExists(
   const int64_t componentUID,
   const std::string & operationName)

{

MyRow row(fullTableName_);
MyTable &myTable = static_cast<MyTable &>(myTable_);

PrivStatus privStatus = myTable.fetchByName(componentUID,operationName,row);

   if (privStatus == STATUS_GOOD || privStatus == STATUS_WARNING)
      return true;
      
   return false;
      
}
//******************** End of PrivMgrComponents::nameExists ********************


// *****************************************************************************
//    MyTable methods
// *****************************************************************************


// *****************************************************************************
// *                                                                           *
// * Function: MyTable::fetchByCode                                            *
// *                                                                           *
// *    Reads from the COMPONENT_OPERATIONS table and returns the row          *
// *    associated with the specified operation code.                          *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentUID>                  const int64_t                   In       *
// *    is the unique ID associated with the component.                        *
// *                                                                           *
// *  <operationCode>                 const std::string &             In       *
// *    is the two digit code associated with the operation.                   *
// *                                                                           *
// *  <row>                           MyRow &                         Out      *
// *    passes back a reference to MyRow, containing the data read.            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Found code, row returned.                                    *
// *           *: Code not found or error encountered.                         *
// *                                                                           *
// *****************************************************************************
PrivStatus MyTable::fetchByCode(
   const int64_t componentUID,
   const std::string & operationCode,
   MyRow & row)
   
{

// Check the last row read before reading metadata.
   if (lastRowRead_.lookupByCode(componentUID,operationCode,
                                 row.operationName_,row.operationType_,
                                 row.operationDescription_))
   {
      row.componentUID_ = componentUID; 
      return STATUS_GOOD;
   }
   
// Not found in cache, look for the component name in metadata.
std::string whereClause("WHERE COMPONENT_UID = ");

   whereClause += PrivMgr::UIDToString(componentUID);
   whereClause += " AND OPERATION_CODE = '";
   whereClause += operationCode;
   whereClause += "'";
   
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
         return STATUS_GOOD;
         break;

      // Should not occur, internal error
      default:
         PRIVMGR_INTERNAL_ERROR("Switch statement in PrivMgrComponentOperations::MyTable::fetchByCode()");
         return STATUS_ERROR;
         break;
   }
   
   return STATUS_GOOD;

}   
//********************** End of MyTable::fetchByCode ***************************




// *****************************************************************************
// *                                                                           *
// * Function: MyTable::fetchByName                                            *
// *                                                                           *
// *    Reads from the COMPONENT_OPERATIONS table and returns the row          *
// *    associated with the specified operation name.                          *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentUIDString>            const std::string &             In       *
// *    is the unique ID associated with the component in string format.       *
// *                                                                           *
// *  <operationName>                 const std::string &             In       *
// *    is the name of the operation.  Name is assumed to be upper case.       *
// *                                                                           *
// *  <row>                           MyRow &                         Out      *
// *    passes back a reference to MyRow, containing the data read.            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Found name, row returned.                                    *
// *           *: Name not found or error encountered.                         *
// *                                                                           *
// *****************************************************************************
PrivStatus MyTable::fetchByName(
   const std::string & componentUIDString,
   const std::string & operationName,
   MyRow & row)
   
{

int64_t componentUID = atol(componentUIDString.c_str());

   return fetchByName(componentUID,operationName,row);

}   
//********************** End of MyTable::fetchByName ***************************
  
// *****************************************************************************
// *                                                                           *
// * Function: MyTable::fetchByName                                            *
// *                                                                           *
// *    Reads from the COMPONENT_OPERATIONS table and returns the row          *
// *    associated with the specified operation name.                          *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentUID>                  const int64_t                   In       *
// *    is the unique ID associated with the component.                        *
// *                                                                           *
// *  <operationName>                 const std::string &             In       *
// *    is the name of the operation.  Name is assumed to be upper case.       *
// *                                                                           *
// *  <row>                           MyRow &                         Out      *
// *    passes back a reference to MyRow, containing the data read.            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Found name, row returned.                                    *
// *           *: Name not found or error encountered.                         *
// *                                                                           *
// *****************************************************************************
PrivStatus MyTable::fetchByName(
   const int64_t componentUID,
   const std::string & operationName,
   MyRow & row)
   
{

// Check the last row read before reading metadata.
   if (lastRowRead_.lookupByName(componentUID,operationName,
                                 row.operationCode_,row.operationType_,
                                 row.operationDescription_))
   {
      row.componentUID_ = componentUID; 
      return STATUS_GOOD;
   }
   
// Not found in cache, look for the component name in metadata.
std::string whereClause("WHERE COMPONENT_UID = ");

   whereClause += PrivMgr::UIDToString(componentUID);
   whereClause += " AND OPERATION_NAME = '";
   whereClause += operationName;
   whereClause += "'";
   
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
         return STATUS_GOOD;
         break;

      // Should not occur, internal error
      default:
         PRIVMGR_INTERNAL_ERROR("Switch statement in PrivMgrComponentOperations::MyTable::fetchByName()");
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
// *    Inserts a row into the COMPONENT_OPERATIONS table.                     *
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
   char operationType = PrivMgrComponentOperations::compTypeToLit(row.operationType_);

   sprintf(insertStatement, "insert into %s values (%ld, '%s', '%s', '%c', '%s')",     
           tableName_.c_str(),
           row.componentUID_,
           row.operationCode_.c_str(),
           row.operationName_.c_str(),
           operationType,
           row.operationDescription_.c_str());
           
   return CLIImmediate(insertStatement);
   
}
//************************** End of MyTable::insert ****************************



// *****************************************************************************
// *                                                                           *
// * Function: MyTable::selectWhereUnique                                      *
// *                                                                           *
// *    Selects a row from the COMPONENT_OPERATIONS table based on the         *
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
// *    passes back a refernce to a MyRow.                                     *
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
std::string selectStmt ("SELECT COMPONENT_UID, OPERATION_CODE, OPERATION_NAME, IS_SYSTEM, OPERATION_DESCRIPTION FROM ");  

   selectStmt += tableName_;
   selectStmt += " ";
   selectStmt += whereClause;
   
ExeCliInterface cliInterface(STMTHEAP, 0, NULL, 
  CmpCommon::context()->sqlSession()->getParentQid());
   
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

   // column 3:  operation_name
   cliInterface.getPtrAndLen(3,ptr,len);
   strncpy(value,ptr,len);
   value[len] = 0;
   row.operationName_ = value;

   // column 4:  is_system
   cliInterface.getPtrAndLen(4,ptr,len);
   strncpy(value,ptr,len);
   value[len] = 0;
   row.operationType_ = PrivMgrComponentOperations::compTypeToEnum(value[0]);
      
   // column 5: operation_description
   cliInterface.getPtrAndLen(5,ptr,len);
   strncpy(value,ptr,len);
   value[len] = 0;
   row.operationDescription_ = value;
   
   lastRowRead_ = row;

   return STATUS_GOOD;
   
}
//********************* End of MyTable::selectWhereUnique **********************


// *****************************************************************************
// method:  update
//
// Updates metadata based on the passed in set and where clauses.
// *****************************************************************************
PrivStatus MyTable::update(
  const std::string & setClause,
  const std::string & whereClause)
{
   char updateStatement[setClause.size() + whereClause.size() + tableName_.size() + 100];

   sprintf(updateStatement, "update %s %s %s",
           tableName_.c_str(),
           setClause.c_str(),
           whereClause.c_str());
           
   return CLIImmediate(updateStatement);
}

// *****************************************************************************
//    MyRow methods
// *****************************************************************************


// *****************************************************************************
// *                                                                           *
// * Function: MyRow::lookupByCode                                             *
// *                                                                           *
// *    Looks for a specified component operation name in cache, and if found, *
// * returns the associated data.                                              *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentUID>                  const int64_t                   In       *
// *    is the unique ID associated with the component.                        *
// *                                                                           *
// *  <operationCode>                 const std::string &             In       *
// *    is the code associated with the component operation in upper case.     *
// *                                                                           *
// *  <operationName>                 std::string &                   Out      *
// *    passes back the name of the component operation.                       *
// *                                                                           *
// *  <operationType>                 OperationType &                 Out      *
// *    passes back the component type, system, user, or unused.               *
// *                                                                           *
// *  <operationDescription>          std::string &                   Out      *
// *    passes back the description of the component operation.                *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// *  true: Component name found in cache.                                     *
// * false: Component name not found in cache.                                 *
// *                                                                           *
// *****************************************************************************
bool MyRow::lookupByCode(
   const int64_t componentUID,
   const std::string & operationCode,
   std::string & operationName,
   PrivMgrComponentOperations::OperationType & operationType,
   std::string & operationDescription) 
   
{

// If componentUID_ is zero, that means data is uninitialized.  

   if (componentUID_ == 0 || 
       componentUID != componentUID_ ||
       operationCode != operationCode)
      return false;
      
   operationType = operationType_;
   operationName = operationName_;
   operationDescription = operationDescription_;
   return true;
   
}  
//************************ End of MyRow::lookupByCode **************************



// *****************************************************************************
// *                                                                           *
// * Function: MyRow::lookupByName                                             *
// *                                                                           *
// *    Looks for a specified component operation name in cache, and if found, *
// * returns the associated data.                                              *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentUID>                  const int64_t                   Out      *
// *    is the unique ID associated with the component.                        *
// *                                                                           *
// *  <operationName>                 const std::string &             In       *
// *    is the name of the operation.  Name is assumed to be upper case.       *
// *                                                                           *
// *  <operationCode>                 std::string &                   Out      *
// *    passes back the code associated with the component operation.          *
// *                                                                           *
// *  <OperationType>                 operationType &                 Out      *
// *    passes back the component type, system, user, or unused.               *
// *                                                                           *
// *  <operationDescription>          std::string &                   Out      *
// *    passes back the description of the component operation.                *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// *  true: Component name found in cache.                                     *
// * false: Component name not found in cache.                                 *
// *                                                                           *
// *****************************************************************************
bool MyRow::lookupByName(
   const int64_t componentUID,
   const std::string & operationName,
   std::string & operationCode,
   PrivMgrComponentOperations::OperationType & operationType,
   std::string & operationDescription) 
   
{

// If componentUID_ is zero, that means data is uninitialized.  

   if (componentUID_ == 0 || 
       componentUID != componentUID_ ||
       operationName != operationName_)
      return false;
      
   operationType = operationType_;
   operationCode = operationCode_;
   operationDescription = operationDescription_;
   return true;
   
}  
//************************ End of MyRow::lookupByName **************************


// ****************************************************************************
// * method: MyTable::selectWhere
// *                                      
// *  Selects rows from the COMPONENT_OPERATIONS table based on the specified
// *  WHERE clause.                                                    
// *                                                                 
// *  Parameters:                                                   
// *                                                               
// *  whereClause is the WHERE clause
// *  rowList  passes back array of wanted COMPONENT_OPERATIONS rows
// *                                                         
// * Returns: PrivStatus                                   
// *                                                      
// * STATUS_GOOD: Row returned.                          
// *           *: Select failed. A CLI error is put into the diags area. 
// *****************************************************************************
PrivStatus MyTable::selectWhere(
      const std::string & whereClause,  
      std::vector<MyRow *> &rowList)
{
  std::string selectStmt("SELECT COMPONENT_UID, OPERATION_CODE, OPERATION_NAME, IS_SYSTEM, TRIM(OPERATION_DESCRIPTION) FROM ");
  selectStmt += tableName_;
  selectStmt += " ";
  selectStmt += whereClause;

// set pointer in diags area
int32_t diagsMark = pDiags_->mark();

  ExeCliInterface cliInterface(STMTHEAP, 0, NULL, 
  CmpCommon::context()->sqlSession()->getParentQid());
  Queue * tableQueue = NULL;
  int32_t cliRC =  cliInterface.fetchAllRows(tableQueue, 
                                             (char *)selectStmt.c_str(), 
                                              0, false, false, true);

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
    MyRow *pRow = new MyRow(tableName_);
    setRow(pCliRow, *pRow);
    rowList.push_back(pRow);
  }    

  // TBD:  need code to delete the rowList
  return STATUS_GOOD;
}


// *****************************************************************************
// * method: MyTable::setRow
// *                                      
// *  Extract information(OutputInfo) returned from cli,
// *  and fill a COMPONENT_OPERATIONS row object with the information.
// *                                                                 
// *  Parameters:                                                   
// *                                                               
// *  pCliRow row destails from the cli
// *  row  passes back filled row object
// *                                                         
// * no errors should be generated
// *****************************************************************************
// Row read successfully.  Extract the columns.
void MyTable::setRow(OutputInfo *pCliRow, MyRow &row)
{
  char * ptr = NULL;
  Int32 len = 0;
  char value[500];
  
  // column 1:  COMPONENT_UID
  pCliRow->get(0,ptr,len);
  row.componentUID_ = *(reinterpret_cast<int64_t*>(ptr));

   // column 2:  OPERATION_CODE
  pCliRow->get(1,ptr,len);
  strncpy(value, ptr, len);
  value[len] = 0;
  row.operationCode_= value;

  // column 3: OPERATION_NAME
  pCliRow->get(2,ptr,len);
  strncpy(value, ptr, len);
  value[len] = 0;
  row.operationName_= value;

  // column 4: IS_SYSTEM
  pCliRow->get(3,ptr,len);
  strncpy(value,ptr,len);
  value[len] = 0;
  row.operationType_ = PrivMgrComponentOperations::compTypeToEnum(value[0]);

  // column 5: OPERATION_DESCRIPTION
  pCliRow->get(4,ptr,len);
  strncpy(value, ptr, len);
  value[len] = 0;
  row.operationDescription_ = value;
}


//*****************************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2013-2015 Hewlett-Packard Development Company, L.P.
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
  
#include "PrivMgrComponents.h"

#include "PrivMgrMD.h"
#include "PrivMgrMDTable.h"
#include "PrivMgrComponentOperations.h"
#include "PrivMgrComponentPrivileges.h"

#include <string>
#include <cstdio>
#include "ComSmallDefs.h"
// sqlcli.h included because ExExeUtilCli.h needs it (and does not include it!)
#include "sqlcli.h"
#include "ExExeUtilCli.h"
#include "ComDiags.h"
//#include "ComQueue.h"
// CmpCommon.h contains STMTHEAP declaration
#include "CmpCommon.h"
#include "CmpDDLCatErrorCodes.h"
#include "ComUser.h"

namespace Components 
{
// *****************************************************************************
// * Class:         MyRow
// * Description:  This class represents a row in the COMPONENTS table containing:
// *                - UID of the component 
// *                - ANSI name of the component
// *                - if the component is a system component (intrinsic to Trafodion)
// *                - Description of the component
// *                
// *    A component can be uniquely identified by its ANSI name or its UID.
// *****************************************************************************
class MyRow : public PrivMgrMDRow
{
public:

// -------------------------------------------------------------------
// Constructors and destructors:
// -------------------------------------------------------------------
   MyRow(std::string tableName)
   : PrivMgrMDRow(tableName, COMPONENTS_ENUM),
     componentUID_(0)
   { };
   
   MyRow(const MyRow &other)
   : PrivMgrMDRow(other)
   {
      componentUID_ = other.componentUID_;
      componentName_ = other.componentName_;
      isSystem_ = other.isSystem_;
      componentDescription_ = other.componentDescription_;
   };
   virtual ~MyRow(){};
   inline void clear() {componentUID_ = 0;};
   bool lookupByName(
      const std::string & componentName,
      int64_t & componentUID,
      bool & isSystem,
      std::string & componentDescription);
   
   
// -------------------------------------------------------------------
// Data Members:
// -------------------------------------------------------------------
     
   int64_t            componentUID_;
   std::string        componentName_;
   std::string        componentDescription_;
   bool               isSystem_;
   
private:
   MyRow();
   
};


// *****************************************************************************
// * Class:         MyTable
// * Description:  This class represents the COMPONENTS table containing:
// *                - the fully qualified name of the table 
// *                
// *    A component can be uniquely identified by its ANSI name or its UID.
// *****************************************************************************
class MyTable : public PrivMgrMDTable
{
public:
   MyTable(
      const std::string & tableName,
      ComDiagsArea * pDiags) 
   : PrivMgrMDTable(tableName,COMPONENTS_ENUM, pDiags),
     lastRowRead_(tableName)
     {};

   virtual PrivStatus insert(const PrivMgrMDRow &row);
   virtual PrivStatus selectWhereUnique(
      const std::string & whereClause,
      PrivMgrMDRow & row);
      
   inline void clear() { lastRowRead_.clear(); };
   
   PrivStatus fetchByName(
      const std::string & componentName,
      MyRow &myRow);
   
private:   
   MyTable();
   MyRow lastRowRead_;

};
}//End namespace Components
using namespace Components;

// *****************************************************************************
//    PrivMgrComponents methods
// *****************************************************************************
// -----------------------------------------------------------------------
// Construct a PrivMgrComponents object for a new component.
// -----------------------------------------------------------------------
PrivMgrComponents::PrivMgrComponents (
   const std::string & metadataLocation,
   ComDiagsArea * pDiags)
: PrivMgr(metadataLocation,pDiags),
  fullTableName_(metadataLocation + ".COMPONENTS"),
  myTable_(*(new MyTable(fullTableName_,pDiags))) 
{ } 

// -----------------------------------------------------------------------
// Copy constructor
// -----------------------------------------------------------------------
PrivMgrComponents::PrivMgrComponents(const PrivMgrComponents &other)
: PrivMgr(other),
  myTable_(*new MyTable(fullTableName_,pDiags_))
{

   fullTableName_ = other.fullTableName_;
  
}

// -----------------------------------------------------------------------
// Destructor.
// -----------------------------------------------------------------------
PrivMgrComponents::~PrivMgrComponents() 
{ 

   delete &myTable_;

}

// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponents::clear                                        *
// *                                                                           *
// *    This function clears any cache associated with this object.            *
// *                                                                           *
// *****************************************************************************
void PrivMgrComponents::clear()

{

MyTable &myTable = static_cast<MyTable &>(myTable_);

   myTable.clear();
   
}
//*********************** End of PrivMgrComponents::clear **********************

// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponents::describeComponents                           *
// *                                                                           *
// * lookup "PRIVMGR_MD".COMPONENTS                                            *
// * using the specified component name(unique),                               *
// * and generate a REGISTER COMPONENT statement,                              *
// * return component UID.                                                     *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentName>                 const std::string &              In      *
// *  Unique component name.                                                   *
// *                                                                           *
// *  <componentUIDString>            std::string &                    Out     *
// *  Component UID in string of the specified component name.                 *
// *                                                                           *
// *  <componentUID>                  int64_t                          Out     *
// *  Component UID of the specified component name.                           *
// *                                                                           *
// *  <outlines>                      std::vector<std::string>         Out     *
// *  Output lines in a string array.                                          *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrComponents::describeComponents(
   const  std::string & componentName, 
   std::string & componentUIDString, 
   int64_t & componentUID, 
   std::vector<std::string> & outlines)
   
{

    bool isSystem;
    std::string detailStr;
    std::string componentText;
    PrivStatus privStatus = fetchByName(componentName, componentUIDString, componentUID, isSystem, detailStr);
    if(privStatus == STATUS_GOOD)
    {
        componentText += "REGISTER COMPONENT "; 
        componentText += componentName; 
        componentText += isSystem?" SYSTEM":"";
        componentText += (!detailStr.empty())?" DETAIL '"+detailStr + "';":";";
        outlines.push_back(componentText);
        outlines.push_back("");
    }
    return privStatus;
    
}
//**************** End of PrivMgrComponents::describeComponents ****************


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponents::dropAll                                      *
// *                                                                           *
// *    This function removes all components from the system.                  *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// *  STATUS_GOOD: All components were deleted.                                *
// * STATUS_ERROR: Execution failed. A CLI error is put into the diags area.   *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrComponents::dropAll()

{

PrivMgrComponentOperations componentOperations(metadataLocation_,pDiags_);
PrivStatus privStatus = STATUS_GOOD;

// Function dropAll() also drops any privileges granted on
// those operations.  
   privStatus = componentOperations.dropAll();
      
   if (privStatus != STATUS_GOOD)
   {
      if (pDiags_->getNumber(DgSqlCode::ERROR_) == 0)
         PRIVMGR_INTERNAL_ERROR("Unable to drop operations for component");
      return STATUS_ERROR;
   }

// If there were any operations or granted privileges on any component, they
// are now gone.  Delete all components from the COMPONENTS table.

   std::string whereClause(" ");
    
MyTable &myTable = static_cast<MyTable &>(myTable_);

   return myTable_.deleteWhere(whereClause);

}
//********************* End of PrivMgrComponents::dropAll **********************



// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponents::exists                                       *
// *                                                                           *
// *    This function determines if a specific component has been defined in   *
// * Privilege Manager metadata.                                               *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentName>                 const std::string &             In       *
// *    is the name of the component.  Name is assumed to be upper case.       *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: bool                                                             *
// *                                                                           *
// *  true: Component is registered.                                           *
// * false: Component is not registered or error encountered.                  *
// *                                                                           *
// *****************************************************************************
bool PrivMgrComponents::exists(const std::string & componentName)

{

MyRow row(fullTableName_);
MyTable &myTable = static_cast<MyTable &>(myTable_);

PrivStatus privStatus = myTable.fetchByName(componentName,row);

   if (privStatus == STATUS_GOOD || privStatus == STATUS_WARNING)
      return true;
      
   return false;
      
}
//*********************** End of PrivMgrComponents::exists *********************




// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponents::fetchByName                                  *
// *                                                                           *
// *    Reads and returns data in row associated with a specified              *
// * component name.                                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentName>                 const std::string &             In       *
// *    is the name of the component.  Name is assumed to be upper case.       *
// *                                                                           *
// *  <componentUIDString>            std::string &                   Out      *
// *    passes back the unique ID associated with the component as a string.   *
// *                                                                           *
// *  <componentUID>                  int64_t &                       Out      *
// *    passes back the unique ID associated with the component as a number.   *
// *                                                                           *
// *  <isSystem>                      bool &                          Out      *
// *    passes back true if the component is a system component, otherwise     *
// *    false is passed back.                                                  *
// *                                                                           *
// *  <componentDescription>          std::string &                   Out      *
// *    passes back the description of the component.  Note, providing a       *
// *  description is optional when the component is registered, so the         *
// *  value may be empty.                                                      *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// *     STATUS_GOOD: Found name, UID returned.                                *
// * STATUS_NOTFOUND: Name not found or error encountered.                     *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrComponents::fetchByName(
   const std::string & componentName,
   std::string & componentUIDString,
   int64_t & componentUID,
   bool & isSystem,
   std::string & componentDescription)
   
{

MyRow row(fullTableName_);
MyTable &myTable = static_cast<MyTable &>(myTable_);

PrivStatus privStatus = myTable.fetchByName(componentName,row);

   if (privStatus == STATUS_NOTFOUND || privStatus == STATUS_ERROR)
      return STATUS_NOTFOUND;

   componentUIDString = UIDToString(row.componentUID_);
   componentUID = row.componentUID_;
   isSystem = row.isSystem_;
   componentDescription = row.componentDescription_;
   return STATUS_GOOD;

}
//******************** End of PrivMgrComponents::fetchByName *******************


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponents::getCount                                     *
// *                                                                           *
// *    Returns the number of registered components.                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: int64_t                                                          *
// *                                                                           *
// *    Returns the number of registered components.                           *
// *                                                                           *
// *****************************************************************************
int64_t PrivMgrComponents::getCount()
   
{
                                   
std::string whereClause(" ");   

int64_t rowCount = 0;   
MyTable &myTable = static_cast<MyTable &>(myTable_);

// set pointer in diags area
int32_t diagsMark = pDiags_->mark();

PrivStatus privStatus = myTable.selectCountWhere(whereClause,rowCount);

   if (privStatus != STATUS_GOOD)
      pDiags_->rewind(diagsMark);
      
   return rowCount;

}
//********************* End of PrivMgrComponents::getCount *********************



// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponents::getUniqueID                                  *
// *                                                                           *
// *    Finds the highest current unique ID and return the next value.         *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: int64_t                                                          *
// *                                                                           *
// *    0: Error, could not get highest current UID, cannot determine a        *
// *       unique value to use.                                                *
// *   >0: A value guaranteed to be unique in the COMPONENTS table.            *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
int64_t PrivMgrComponents::getUniqueID()
{

Lng32 len = 0;
char stmtBuf[1000];

   strcpy(stmtBuf,"SELECT NVL(MAX(component_uid),0) FROM ");
   strcat(stmtBuf,fullTableName_.c_str());

int64_t maxValue = 0;

ExeCliInterface cliInterface(STMTHEAP);
Lng32 cliRC = cliInterface.executeImmediate(stmtBuf,(char *)&maxValue,&len,false);

   if (cliRC != 0)
   {
      return 0;
   }
 
   return ++maxValue;
   
}
//******************** End of PrivMgrComponents::getUniqueID *******************


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponents::registerComponent                            *
// *                                                                           *
// *    Adds a row to the COMPONENTS table.                                    *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentName>                 const std::string &             In       *
// *    is the name of the component.  Name is assumed to be upper case.       *
// *                                                                           *
// *  <isSystem>                      const bool                      In       *
// *    is true if this is a system level component, false otherwise.  If a    *
// *  component is system level, Trafodion software assumes is it defined.     *
// *  System components may only be unregistered by DB__ROOT.  Only DB__ROOT   *
// *  may register a system component (unless a component privilege is added   *
// *  to authorize the operation).                                             *
// *                                                                           *
// *  <componentDescription>          const std::string &             In       *
// *    is a description of the component.                                     *
// *                                                                           *
// *  <existsErrorOK>                 const bool                      [In]     *
// *    if true, exists errors are silently ignored.                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Component name was registered.                               *
// *           *: Unable to register component name, see diags.                *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrComponents::registerComponent(
   const std::string & componentName,
   const bool isSystem,
   const std::string & componentDescription,
   const bool existsErrorOK)

{

//TODO: Could check for setting isSystem, could be separate
// privilege, or restricted to DB__ROOT.
PrivMgrComponentPrivileges componentPrivileges(metadataLocation_, pDiags_);

   if (!ComUser::isRootUserID()&&
       !componentPrivileges.hasSQLPriv(ComUser::getCurrentUser(), SQLOperation::MANAGE_COMPONENTS, true))
   {   
      *pDiags_ << DgSqlCode(-CAT_NOT_AUTHORIZED);
      return STATUS_ERROR;
   }

// Is component already registered?

   if (exists(componentName))
   {
      if (existsErrorOK)
         return STATUS_GOOD;
   
      *pDiags_ << DgSqlCode(-CAT_TABLE_ALREADY_EXISTS)
               << DgTableName(componentName.c_str());
      return STATUS_ERROR;
   }

// Set up new component entry
int64_t uid = getUniqueID();

   if (uid == 0)
     return STATUS_ERROR;
      
MyRow row(fullTableName_);

   row.componentUID_ = uid;
   row.componentName_ = componentName;
   row.isSystem_ = isSystem;
   row.componentDescription_ = componentDescription;
   
// write the row to the components table
   return myTable_.insert(row);

}
//***************** End of PrivMgrComponents::registerComponent ****************


// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponents::registerComponentInternal                    *
// *                                                                           *
// *    Internal function to register a component.  No checks are made prior   *
// * to inserting row into the COMPONENTS table.                               *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentName>                 const std::string &             In       *
// *    is the name of the component.  Name is assumed to be upper case.       *
// *                                                                           *
// *  <componentUID>                  const int64_t                   In       *
// *    is a unique ID for the component.                                      *
// *                                                                           *
// *  <isSystem>                      const bool                      In       *
// *    is true if this is a system level component, false otherwise.          *
// *                                                                           *
// *  <componentDescription>          const std::string &             In       *
// *    is a description of the component.                                     *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Component name was registered.                               *
// *           *: Unable to register component name, see diags.                *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrComponents::registerComponentInternal(
   const std::string & componentName,
   const int64_t componentUID,
   const bool isSystem,
   const std::string & componentDescription)

{
      
MyRow row(fullTableName_);

   row.componentUID_ = componentUID;
   row.componentName_ = componentName;
   row.isSystem_ = isSystem;
   row.componentDescription_ = componentDescription;
   
// write the row to the components table
   return myTable_.insert(row);

}
//************* End of PrivMgrComponents::registerComponentInternal ************



// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrComponents::unregisterComponent                          *
// *                                                                           *
// *    Removes component from the COMPONENTS table and removes all operations *
// * associated with the component and any granted privileges related to the   *
// * component.                                                                *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentName>                 const std::string &             In       *
// *    is the name of the component.  Name is assumed to be upper case.       *
// *                                                                           *
// *  <dropBehavior>                  PrivDropBehavior                In       *
// *    indicates whether restrict or cascade behavior is requested.           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// * STATUS_GOOD: Component was unregistered.                                  *
// *           *: Unable to unregister component, see diags.                   *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrComponents::unregisterComponent(
   const std::string & componentName,
   PrivDropBehavior dropBehavior)
   
{

//TODO: Related, could check below for unregistering system level components.
PrivMgrComponentPrivileges componentPrivileges(metadataLocation_, pDiags_);

   if (!ComUser::isRootUserID()&&
       !componentPrivileges.hasSQLPriv(ComUser::getCurrentUser(), SQLOperation::MANAGE_COMPONENTS, true))
   {   
      *pDiags_ << DgSqlCode(-CAT_NOT_AUTHORIZED);
      return STATUS_ERROR;
   }
   
   if (!exists(componentName))
   {
      *pDiags_ << DgSqlCode(-CAT_TABLE_DOES_NOT_EXIST_ERROR)
               << DgTableName(componentName.c_str());
      return STATUS_ERROR;
   }

std::string componentUIDString;
int64_t componentUID;
bool isSystem;
std::string tempStr;

   fetchByName(componentName,componentUIDString,componentUID,isSystem,tempStr);

//TODO: check authority if it is a system component

PrivMgrComponentOperations componentOperations(metadataLocation_,pDiags_);
PrivStatus privStatus = STATUS_GOOD;

   if (dropBehavior == PrivDropBehavior::RESTRICT)
   {
      if (componentOperations.isComponentUsed(componentUIDString))
      {
         *pDiags_ << DgSqlCode(-CAT_DEPENDENT_OBJECTS_EXIST);
         return STATUS_ERROR; 
      }
   }
   else  //CASCADE
   {  
      // Function dropAll() also drops any privileges granted on
      // those operations.  
      privStatus = componentOperations.dropAll(componentUIDString);
      
      if (privStatus != STATUS_GOOD)
      {
         if (pDiags_->getNumber(DgSqlCode::ERROR_) == 0)
            PRIVMGR_INTERNAL_ERROR("Unable to drop operations for component");
         return STATUS_ERROR;
      }
   }

// If there were any operations or granted privileges on this component, they
// are now gone.  Delete the component from the COMPONENTS table.

   std::string whereClause("WHERE COMPONENT_NAME = '");
   whereClause += componentName;
   whereClause += "'";
   
   return myTable_.deleteWhere(whereClause);

}
//*************** End of PrivMgrComponents::unregisterComponent ****************

// *****************************************************************************
//    MyRow methods
// *****************************************************************************

// *****************************************************************************
// *                                                                           *
// * Function: MyRow::lookupByName                                             *
// *                                                                           *
// *    Looks for a specified component name in cache, and if found, returns   *
// *  associated data.                                                         *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentName>                 const std::string &             In       *
// *    is the name of the component.  Name is assumed to be upper case.       *
// *                                                                           *
// *  <componentUID>                  int64_t &                       Out      *
// *    passes back the unique ID associated with the component.               *
// *                                                                           *
// *  <isSystem>                      bool &                          Out      *
// *    passes back true if the component is a system component, otherwise     *
// *    false is passed back.                                                  *
// *                                                                           *
// *  <componentDescription>          std::string &                   Out      *
// *    passes back the description of the component.  Note, providing a       *
// *  description is optional when the component is registered, so the         *
// *  value may be empty.                                                      *
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
   const std::string & componentName,
   int64_t & componentUID,
   bool & isSystem,
   std::string & componentDescription) 
   
{

// If componentUID_ is zero, that means data is uninitialized.  

   if (componentUID_ == 0 || componentName != componentName_)
      return false;
      
   componentUID = componentUID_;
   isSystem = isSystem_;
   componentDescription = componentDescription_;
   return true;
   
}  
//*********************** End of MyRow::lookupByName ***************************



// *****************************************************************************
//    MyTable methods
// *****************************************************************************


// *****************************************************************************
// *                                                                           *
// * Function: MyTable::fetchByName                                            *
// *                                                                           *
// *    Reads from the COMPONENTS table and returns the row associated with    *
// *    a specified component name.                                            *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *  <componentName>                 const std::string &             In       *
// *    is the name of the component.  Name is assumed to be upper case.       *
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
   const std::string & componentName,
   MyRow & row)
   
{

// Check the last row read before reading metadata.
   if (lastRowRead_.lookupByName(componentName,row.componentUID_,
                                 row.isSystem_,row.componentDescription_))
      return STATUS_GOOD;
      
// Not found in cache, look for the component name in metadata.
std::string whereClause ("WHERE COMPONENT_NAME = '");

   whereClause += componentName;  //TODO: is character set OK?  Do we need to do anything to make sure?
   whereClause += "'";            // For instance, prefix with _UTF8?
   
PrivStatus privStatus = selectWhereUnique(whereClause,row);

   switch (privStatus)
   {
      // Return status to caller to handle
      case STATUS_NOTFOUND:
      case STATUS_ERROR:
         return privStatus;
         break;

      // Object exists - break out and continue
      case STATUS_GOOD:
      case STATUS_WARNING:
         return STATUS_GOOD;
         break;

      // Should not occur, internal error
      default:
         PRIVMGR_INTERNAL_ERROR("Switch statement in PrivMgrComponents::MyTable::fetchByName()");
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
// *    Inserts a row into the COMPONENTS table.                               *
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
char isSystem[3] = {0};

   if (row.isSystem_)
      isSystem[0] = 'Y';
   else
      isSystem[0] = 'N';

   sprintf(insertStatement, "insert into %s values (%ld, '%s', '%s', '%s')",
           tableName_.c_str(),
           row.componentUID_,
           row.componentName_.c_str(),
           isSystem,
           row.componentDescription_.c_str());
           
   return CLIImmediate(insertStatement);

}
//************************** End of MyTable::insert ****************************




// *****************************************************************************
// *                                                                           *
// * Function: MyTable::selectWhereUnique                                      *
// *                                                                           *
// *    Selects a row from the COMPONENTS table based on the specified         *
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
std::string selectStmt ("SELECT COMPONENT_UID, COMPONENT_NAME, IS_SYSTEM, COMPONENT_DESCRIPTION FROM ");

   selectStmt += tableName_;
   selectStmt += " ";
   selectStmt += whereClause;

ExeCliInterface cliInterface(STMTHEAP);

PrivStatus privStatus = CLIFetch(cliInterface,selectStmt);   
   
   if (privStatus != STATUS_GOOD && privStatus != STATUS_WARNING)
      return privStatus;   
  
// Row read successfully.  Extract the columns.

char * ptr = NULL;
Lng32 len = 0;
char value[500];
MyRow & row = static_cast<MyRow &>(rowOut);

   // column 1:  component_uid
   cliInterface.getPtrAndLen(1,ptr,len);
   row.componentUID_ = *(reinterpret_cast<int64_t*>(ptr));

   // column 2:  component_name
   cliInterface.getPtrAndLen(2,ptr,len);
   strncpy(value,ptr,len);
   value[len] = 0;
   row.componentName_ = value;

   // column 3:  is_system
   cliInterface.getPtrAndLen(3,ptr,len);
   strncpy(value,ptr,len);
   value[len] = 0;
   if (value[0] == 'Y')
      row.isSystem_ = true;
   else
      row.isSystem_ = false;
   
   // column 4: component_description
   cliInterface.getPtrAndLen(4,ptr,len);
   strncpy(value, ptr,len);
   value[len] = 0;
   row.componentDescription_ = value;
   
   lastRowRead_ = row;

   return STATUS_GOOD;
   
}
//******************* End of MyTable::selectWhereUnique ************************






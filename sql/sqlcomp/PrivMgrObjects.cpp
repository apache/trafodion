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
  
#include "PrivMgrObjects.h"

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
#include "ComQueue.h"
// CmpCommon.h contains STMTHEAP declaration
#include "CmpCommon.h"
#include "CmpDDLCatErrorCodes.h"
#include "ComUser.h"

namespace Objects 
{
// *****************************************************************************
// * Class:         MyRow
// * Description:  This class represents a row in the OBJECTS table containing:
// *                - UID of the component 
// *                - ANSI name of the component
// *                - if the component is a system component (intrinsic to Trafodion)
// *                - Description of the component
// *                
// *    An object can be uniquely identified by its ANSI name and object type 
// * or by its UID.
// *****************************************************************************
class MyRow : public PrivMgrMDRow
{
public:

// -------------------------------------------------------------------
// Constructors and destructors:
// -------------------------------------------------------------------
   MyRow(std::string tableName)
   : PrivMgrMDRow(tableName),
     objectUID_(0)
   { };
   
   MyRow(const MyRow &other)
   : PrivMgrMDRow(other)
   {
      objectUID_ = other.objectUID_;
      catalogName_ = other.catalogName_;
      schemaName_ = other.schemaName_;
      objectName_ = other.objectName_;
      objectType_ = other.objectType_;
      createTime_ = other.createTime_;
      redefTime_ = other.redefTime_;
      isValid_ = other.isValid_;
      objectOwner_ = other.objectOwner_;
   };
   virtual ~MyRow(){};
   inline void clear() {objectUID_ = 0;};
   
   
// -------------------------------------------------------------------
// Data Members:
// -------------------------------------------------------------------
     
   int64_t            objectUID_;
   std::string        catalogName_;
   std::string        schemaName_;
   std::string        objectName_;
   ComObjectType      objectType_;
   int64_t            createTime_;
   int64_t            redefTime_;
   int32_t            objectOwner_;
   bool               isValid_;
   
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
   : PrivMgrMDTable(tableName,pDiags),
     lastRowRead_(tableName)
     {};

   inline void clear() { lastRowRead_.clear(); };

   virtual PrivStatus insert(const PrivMgrMDRow &row);
   
   PrivStatus selectAllWhere(
      const std::string & whereClause,
      const std::string & orderByClause,
      std::vector<MyRow> & rows);
      
   virtual PrivStatus selectWhereUnique(
      const std::string & whereClause,
      PrivMgrMDRow & row);
      
   void setRow(
      OutputInfo & cliInterface,
      PrivMgrMDRow & rowOut);
   
private:   
   MyTable();
   MyRow lastRowRead_;

};
}//End namespace Objects
using namespace Objects;

// *****************************************************************************
//    PrivMgrObjects methods
// *****************************************************************************
// -----------------------------------------------------------------------
// Construct a PrivMgrObjects object for a new component.
// -----------------------------------------------------------------------
PrivMgrObjects::PrivMgrObjects (
   const std::string & metadataLocation,
   ComDiagsArea * pDiags)
: PrivMgr(metadataLocation,pDiags),
  fullTableName_(metadataLocation + ".OBJECTS"),
  myTable_(*(new MyTable(fullTableName_,pDiags))) 
{ } 

// -----------------------------------------------------------------------
// Copy constructor
// -----------------------------------------------------------------------
PrivMgrObjects::PrivMgrObjects(const PrivMgrObjects &other)
: PrivMgr(other),
  myTable_(*new MyTable(fullTableName_,pDiags_))
{

   fullTableName_ = other.fullTableName_;
  
}

// -----------------------------------------------------------------------
// Destructor.
// -----------------------------------------------------------------------
PrivMgrObjects::~PrivMgrObjects() 
{ 

   delete &myTable_;

}

// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrObjects::clear                                           *
// *                                                                           *
// *    This function clears any cache associated with this object.            *
// *                                                                           *
// *****************************************************************************
void PrivMgrObjects::clear()

{

MyTable &myTable = static_cast<MyTable &>(myTable_);

   myTable.clear();
   
}
//************************* End of PrivMgrObjects::clear ***********************

// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrObjects::fetchUIDandTypes                                *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrObjects::fetchQualifiedName(
   const int64_t objectUID,
   std::string & qualifiedObjectName) 
        
{

MyTable &myTable = static_cast<MyTable &>(myTable_);
MyRow row(fullTableName_);

std::string whereClause(" WHERE OBJECT_UID = ");

   whereClause += UIDToString(objectUID);

PrivStatus privStatus = myTable.selectWhereUnique(whereClause,row);

   if (privStatus != STATUS_GOOD)
      return STATUS_ERROR;

   qualifiedObjectName = row.catalogName_ + ".";
   qualifiedObjectName += row.schemaName_ + ".";
   qualifiedObjectName += row.objectName_;   

   return STATUS_GOOD;
    
}
//******************* End of PrivMgrObjects::fetchUIDandTypes ******************





// *****************************************************************************
// *                                                                           *
// * Function: PrivMgrObjects::fetchUIDandTypes                                *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// *  Parameters:                                                              *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
// *                                                                           *
// * Returns: PrivStatus                                                       *
// *                                                                           *
// *                                                                           *
// *****************************************************************************
PrivStatus PrivMgrObjects::fetchUIDandTypes(
   const std::string whereClause,
   vector<UIDAndType> & UIDandTypes) 
        
{

std::string orderByClause;
   
std::vector<MyRow> rows;
MyTable &myTable = static_cast<MyTable &>(myTable_);

PrivStatus privStatus = myTable.selectAllWhere(whereClause,orderByClause,rows);

   if (privStatus != STATUS_GOOD)
      return STATUS_ERROR;
   
   for (size_t r = 0; r < rows.size(); r++)
   {
      UIDAndType element;
      
      element.UID = rows[r].objectUID_;
      element.objectType = rows[r].objectType_;
      
      UIDandTypes.push_back(element);
   }

   return STATUS_GOOD;
    
}
//******************* End of PrivMgrObjects::fetchUIDandTypes ******************





// *****************************************************************************
//    MyTable methods
// *****************************************************************************


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
#if 0
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
#endif           
   return CLIImmediate(insertStatement);

}
//************************** End of MyTable::insert ****************************


// *****************************************************************************
// *                                                                           *
// * Function: MyTable::selectAllWhere                                         *
// *                                                                           *
// *    Selects rows from the OBJECTS table based on the specified WHERE       *
// *   clause and sorted per an optional ORDER BY clause.                      *
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

std::string selectStmt("SELECT CATALOG_NAME, SCHEMA_NAME, OBJECT_NAME,"
                       "OBJECT_TYPE, OBJECT_UID, CREATE_TIME, REDEF_TIME,"
                       "VALID_DEF, OBJECT_OWNER FROM ");
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
std::string selectStmt("SELECT CATALOG_NAME, SCHEMA_NAME, OBJECT_NAME,"
                       "OBJECT_TYPE, OBJECT_UID, CREATE_TIME, REDEF_TIME,"
                       "VALID_DEF, OBJECT_OWNER FROM ");

   selectStmt += tableName_;
   selectStmt += " ";
   selectStmt += whereClause;

ExeCliInterface cliInterface(STMTHEAP);

PrivStatus privStatus = CLIFetch(cliInterface,selectStmt);   
   
   if (privStatus != STATUS_GOOD && privStatus != STATUS_WARNING)
      return privStatus;   
  
// Row read successfully.  Extract the columns.

char * ptr = NULL;
Lng32 length = 0;
char value[500];
MyRow & row = static_cast<MyRow &>(rowOut);

   // column 1:  CATALOG_NAME
   cliInterface.getPtrAndLen(1,ptr,length);
   strncpy(value,ptr,length);
   value[length] = 0;
   row.catalogName_ = value;

   // column 2:  SCHEMA_NAME
   cliInterface.getPtrAndLen(2,ptr,length);
   strncpy(value,ptr,length);
   value[length] = 0;
   row.schemaName_ = value;

   // column 3:  OBJECT_NAME
   cliInterface.getPtrAndLen(3,ptr,length);
   strncpy(value,ptr,length);
   value[length] = 0;
   row.objectName_ = value;

   // column 4:  OBJECT_TYPE
   cliInterface.getPtrAndLen(4,ptr,length);
   strncpy(value,ptr,length);
   value[length] = 0;
   row.objectType_ = PrivMgr::ObjectLitToEnum(value);

   // column 5:  OBJECT_UID
   cliInterface.getPtrAndLen(5,ptr,length);
   row.objectUID_ = *(reinterpret_cast<int64_t*>(ptr));

   // column 6:  CREATE_TIME
   cliInterface.getPtrAndLen(6,ptr,length);
   row.createTime_ = *(reinterpret_cast<int64_t*>(ptr));

   // column 7:  REDEF_TIME
   cliInterface.getPtrAndLen(7,ptr,length);
   row.redefTime_ = *(reinterpret_cast<int64_t*>(ptr));
   
   // column 8:  VALID_DEF
   cliInterface.getPtrAndLen(8,ptr,length);
   strncpy(value,ptr,length);
   value[length] = 0;
   row.isValid_ = (value[0] == 'Y' ? true : false);
   
   // column 9:  OBJECT_OWNER
   cliInterface.getPtrAndLen(9,ptr,length);
   row.objectOwner_ = *(reinterpret_cast<int32_t*>(ptr));
   
   lastRowRead_ = row;

   return STATUS_GOOD;
   
}
//******************* End of MyTable::selectWhereUnique ************************



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
  
   // column 1:  CATALOG_NAME
   cliInterface.get(0,ptr,length);
   strncpy(value,ptr,length);
   value[length] = 0;
   row.catalogName_ = value;

   // column 2:  SCHEMA_NAME
   cliInterface.get(1,ptr,length);
   strncpy(value,ptr,length);
   value[length] = 0;
   row.schemaName_ = value;

   // column 3:  OBJECT_NAME
   cliInterface.get(2,ptr,length);
   strncpy(value,ptr,length);
   value[length] = 0;
   row.objectName_ = value;

   // column 4:  OBJECT_TYPE
   cliInterface.get(3,ptr,length);
   strncpy(value,ptr,length);
   value[length] = 0;
   row.objectType_ = PrivMgr::ObjectLitToEnum(value);

   // column 5:  OBJECT_UID
   cliInterface.get(4,ptr,length);
   row.objectUID_ = *(reinterpret_cast<int64_t*>(ptr));

   // column 6:  CREATE_TIME
   cliInterface.get(5,ptr,length);
   row.createTime_ = *(reinterpret_cast<int64_t*>(ptr));

   // column 7:  REDEF_TIME
   cliInterface.get(6,ptr,length);
   row.redefTime_ = *(reinterpret_cast<int64_t*>(ptr));
   
   // column 8:  VALID_DEF
   cliInterface.get(7,ptr,length);
   strncpy(value,ptr,length);
   value[length] = 0;
   row.isValid_ = (value[0] == 'Y' ? true : false);
   
   // column 9:  OBJECT_OWNER
   cliInterface.get(8,ptr,length);
   row.objectOwner_ = *(reinterpret_cast<int32_t*>(ptr));
   
   lastRowRead_ = row;

}
//************************** End of MyTable::setRow ****************************






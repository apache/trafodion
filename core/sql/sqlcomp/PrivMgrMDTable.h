//*****************************************************************************
// @@@ START COPYRIGHT @@@
//
//// (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
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

#ifndef PRIVMGR_MDTABLE_H
#define PRIVMGR_MDTABLE_H

#include <string>
#include <vector>
#include "PrivMgrMDDefs.h"
#include "PrivMgrDefs.h"
class ExeCliInterface;

// *****************************************************************************
// *
// * File:         PrivMgrMDTable.h
// * Description:  This file contains the base classes used for Privilege Manager  
// *                metadata tables
// *               
// * Language:     C++
// *
// *****************************************************************************

#include <string>
class ComDiagsArea;
class Queue;

// *****************************************************************************
// * Class:         PrivMgrMDRow
// * Description:  This is the base class for rows of metadata tables in the 
// *               PrivMgrMD schema.
// *****************************************************************************
class PrivMgrMDRow
{
public:
   PrivMgrMDRow(std::string myTableName);
   PrivMgrMDRow(const PrivMgrMDRow &other);
   virtual ~PrivMgrMDRow();

protected:
   std::string myTableName_;
   
private:
   PrivMgrMDRow();

};

// *****************************************************************************
// * Class:         PrivMgrMDTable
// * Description:  This is the base class for metadata tables in the 
// *               PrivMgrMD schema.
// *****************************************************************************
class PrivMgrMDTable
{
public:

// -------------------------------------------------------------------
// Constructors and destructors:
// -------------------------------------------------------------------
   PrivMgrMDTable(
      const std::string & tableName,
      ComDiagsArea * pDiags = NULL);
   PrivMgrMDTable(const PrivMgrMDTable &other);
   virtual ~PrivMgrMDTable();

// -------------------------------------------------------------------
// Public functions:
// -------------------------------------------------------------------
   PrivStatus CLIFetch(
      ExeCliInterface & cliInterface, 
      const std::string & SQLStatement);
   
   PrivStatus CLIImmediate(const std::string & SQLStatement);
   
   PrivStatus executeFetchAll(
      const std::string & SQLStatement,
      Queue * & queue);
   
   virtual PrivStatus insert(const PrivMgrMDRow &row) = 0;
   
   virtual PrivStatus selectCountWhere(
      const std::string & whereClause,
      int64_t & rowCount);
   
   virtual PrivStatus selectWhereUnique(
      const std::string & whereClause,
      PrivMgrMDRow & row) = 0;
      
   virtual PrivStatus deleteWhere(const std::string & whereClause);
   
   virtual PrivStatus update(const std::string & setClause);

protected: 
// -------------------------------------------------------------------
// Data Members:
// -------------------------------------------------------------------
// Fully qualified table name      
std::string    tableName_;
ComDiagsArea * pDiags_;

private:
   PrivMgrMDTable();

};
#endif // PRIVMGR_MDTABLE_H










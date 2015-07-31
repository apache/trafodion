/* -*-C++-*- */
/**********************************************************************
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
**********************************************************************/
/* -*-C++-*-
******************************************************************************
*
* File:         ReadTableDef.h
* Description:
*   This class and set of functions provide an interface between the 
*   compiler and metadata.  It gets information from catman cache through
*   the SOL layer and puts it into a desc structure.  The desc structure is
*   returned to the compiler to be placed in the NATable structure.
*
* Created:      01/18/96
* Language:     C++
*
*
******************************************************************************
*/

#ifndef __READTABLEDEF_H
#define __READTABLEDEF_H

// SQL/MX definitions
#include "BaseTypes.h"
#include "ObjectNames.h"
#include "ComSmallDefs.h"        // added for ComDiskFileFormat enum

// SQL/MX SqlCat definitions
#include "desc.h"

#include "sqlca.h"

// Forward references
class BeforeAndAfterTriggers;
class MVInfoForDML;
class BindWA;
class NARoutine;
class CatRORoutine;
class CatRORoutineAction;
class CatROObject;

#define READTABLEDEF_VERSION 2	// Incr this if any defs herein are changed!
#define NO_ACTIVE_REPLYTAG -1

enum RTDTransactions
{
  TRANS_MATCHED                 = 0,    // All transaction identifiers matched
  TRANS_MISMATCHED              = 1,    // Transaction identifiers mismatched
  TRANS_NONE_ACTIVE             = 2,    // No ReadTableDef transaction active
  TRANS_READTABLEDEF_CLEANED    = 3     // ReadTableDef transaction was cleaned up
};

//***********************************************************************
// ReadTableDef class definition
//***********************************************************************

class ReadTableDef : public NABasicObject
{
public:

  enum TransactionState	{ TXN_IN_PROGRESS, NO_TXN };

  // constructor
  ReadTableDef();

  // destructor
  ~ReadTableDef();

  void deleteTree(desc_struct * top) const;
  void displayTree(const desc_struct * top) const;	// debugging only 

private:
  void initSQLCA(NABoolean force = FALSE);

  void deleteTreeMX(desc_struct * top) const;
  void displayTreeMX(const desc_struct * top,
                      const char * caller) const;       // debugging only

  // Versioning Light: Methods for Lazy Update
#if 0
  void dealWithCLIError (void);
  short lazyUpdateNeedSeparateTx (void);
  void lazyUpdate (const ComObjectName & objectName);
#endif //if 0

  // data members
  TransactionState      transactionState_;
  NABoolean		transInProgress_;
  Int64                 transId_;
}; // class ReadTableDef


  
#endif // READTABLEDEF_H

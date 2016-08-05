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
* File:         ReadTableDef.cpp
* Description:	
*   This class and set of functions provide an interface between the 
*   compiler and metadata.  It gets information from catman cache through
*   the SOL layer and puts it into a desc structure.  The desc structure is
*   returned to the compiler to be placed in the NATable structure.
*
* Created:	01/18/96
* Language:     C++
*
*
******************************************************************************
*/

//------------------------------------------------------------------------
// Include files
//------------------------------------------------------------------------
#include "Platform.h"			// must be first

#define  SQLPARSERGLOBALS_NADEFAULTS	// must precede other #include's
#define  READTABLEDEF_IMPLEMENTATION	// for ReadTableDef.h and dfs2rec.h

#include "ReadTableDef.h"
#include "readRealArk.h"
#include "SQLCLIdev.h"
#include <ctype.h>
#include <iostream>
#include <iomanip>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "CmpCommon.h"		// HEAP stuff
#include "CmpContext.h"		// CmpCommon::context()->readTableDef_
#include "DefaultValidator.h"	// for ValidateCollationList
#include "ExSqlComp.h"		// for NAExecTrans
#include "IntervalType.h"
#include "DatetimeType.h"
#include "SchemaDB.h"

#include "NAAssert.h"
#include "SqlParserGlobals.h"	// must be last #include!


//------------------------------------------------------------------------
// Global variables declarations
//------------------------------------------------------------------------
extern Lng32 SQLCODE;

//------------------------------------------------------------------------
// Constructor
//------------------------------------------------------------------------

ReadTableDef::ReadTableDef()
: transactionState_   (NO_TXN),
  transInProgress_    (FALSE),
  transId_            (-1)
{
} // ctor

//------------------------------------------------------------------------
// Destructor
//------------------------------------------------------------------------

ReadTableDef::~ReadTableDef()
{
  // end any transactions started by "me"
}


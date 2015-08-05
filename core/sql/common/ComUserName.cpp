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
 *****************************************************************************
 *
 * File:         ComExternaluser.C
 * Description:  methods for class ComExternaluser
 *               
 *               
 * Created:      
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */


#define  SQLPARSERGLOBALS_NADEFAULTS		// first

#include <string.h>
#include "ComASSERT.h"
#include "ComMPLoc.h"
#include "ComUserName.h"
#include "ComSqlText.h"
#include "NAString.h"
#include "ComSchLevelOp.h"

#include "SqlParserGlobals.h"			// last


//
// constructors
//
ComdbUser::ComdbUser () 
{
}

//
// initializing constructor
//
ComdbUser::ComdbUser (const NAString &dbUserName)
	:dbUserName_(dbuserName)
{
  scan(dbUserName);
}

//
// virtual destructor
//
ComdbUser::~ComdbUser ()
{
}


//
//  private methods
//
//
// Scans (parses) input external-format schema name.
//
NABoolean
ComdbUser::scan(const NAString &dbUserName)
{
	//validate the user name by querying USERS table.
  return TRUE;
}

//
// default constructor
//
ComExternaluser::ComExternaluser () 
{
}

//
// initializing constructor
//
ComExternaluser::ComExternaluser (const NAString &externalUserName)
	:externalUserName_(externaluserName)
{
  scan(externalUserName);
}

//
// virtual destructor
//
ComExternaluser::~ComExternaluser ()
{
}


//
//  private methods
//
//
// Scans (parses) input external-format schema name.
//
NABoolean
ComExternaluser::scan(const NAString &externalUserName)
{
	//validate the user name by querying USERS table.
  return TRUE;
}



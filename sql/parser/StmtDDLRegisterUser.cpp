/* -*-C++-*-
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2011-2014 Hewlett-Packard Development Company, L.P.
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
 ****************************************************************************
 *
 * File:         StmtDDLRegisterUser.cpp
 * Description:  Methods for classes representing DDL (Un)Register User Statements
 *
 *               Also contains definitions of non-inline methods of
 *               classes relating to view usages.
 *
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */

#define   SQLPARSERGLOBALS_FLAGS	// must precede all #include's

#include <stdlib.h>
#ifndef NDEBUG
#include <iostream>
#endif
#include "AllElemDDLPartition.h"
#include "AllElemDDLParam.h"
#include "AllElemDDLUdr.h"
#include "StmtDDLRegisterUser.h"
#include "BaseTypes.h"
#include "ComDiags.h"
#include "ComOperators.h"
#ifndef   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#define   SQLPARSERGLOBALS_CONTEXT_AND_DIAGS
#endif

#include "SqlParserGlobals.h"	// must be last #include


// -----------------------------------------------------------------------
// methods for class StmtDDLRegisterUser
// -----------------------------------------------------------------------

//
// constructor
//
// constructor used for REGISTER USER
StmtDDLRegisterUser::StmtDDLRegisterUser(const NAString & externalUserName,
                                         const NAString * pDbUserName,
                                         ElemDDLNode *pOwner,
                                         CollHeap * heap)
  : StmtDDLNode(DDL_REGISTER_USER),
    externalUserName_(externalUserName,heap),
    registerUserType_(REGISTER_USER),
    dropBehavior_(COM_UNKNOWN_DROP_BEHAVIOR)
{
  if (pDbUserName == NULL)
  {
    NAString userName(externalUserName_, heap);
    dbUserName_ = userName;
  }
  else
  {
    NAString userName(*pDbUserName, heap);
    dbUserName_ = userName;
  }
  if (pOwner)
  {
    pOwner_ = pOwner->castToElemDDLGrantee();
    ComASSERT(pOwner_ NEQ NULL);
  }
  else
    pOwner_ = NULL;
}

// constructor used for UNREGISTER USER
StmtDDLRegisterUser::StmtDDLRegisterUser(const NAString & dbUserName,
                                         const ComDropBehavior  dropBehavior,
                                         CollHeap * heap)
  : StmtDDLNode(DDL_REGISTER_USER),
    dbUserName_(dbUserName, heap),
    externalUserName_("", heap),
    registerUserType_(UNREGISTER_USER),
    dropBehavior_(dropBehavior) 
{
} // StmtDDLRegisterUser::StmtDDLRegisterUser()

//
// virtual destructor
//
StmtDDLRegisterUser::~StmtDDLRegisterUser()
{
  // delete all children
  if (pOwner_)
    delete pOwner_;
}

//
// cast
//
StmtDDLRegisterUser *
StmtDDLRegisterUser::castToStmtDDLRegisterUser()
{
  return this;
}

// -----------------------------------------------------------------------
// methods for class StmtDDLRegisterUserArray
// -----------------------------------------------------------------------

// virtual destructor
// Do the list of user commands need to be removed?
StmtDDLRegisterUserArray::~StmtDDLRegisterUserArray()
{
}



//
// End of File
//


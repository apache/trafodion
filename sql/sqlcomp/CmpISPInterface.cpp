/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1994-2014 Hewlett-Packard Development Company, L.P.
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
**********************************************************************/
 /* -*-C++-*-
******************************************************************************
*
* File:         CmpISPInterface.cpp
* Description:  
* Created:      3/26/2014 (relocate to this file)
* Language:     C++
*
*
*
*
******************************************************************************
*/

#include "CmpISPInterface.h"
#include "CmpISPStd.h"
#include "CmpStoredProc.h"
#include "QueryCacheSt.h"
#include "NATable.h"
#include "NATableSt.h"
#include "NARoutine.h"


CmpISPInterface::CmpISPInterface()
{
  initCalled_ = FALSE;
}

void CmpISPInterface::InitISPFuncs()
{
  SP_REGISTER_FUNCPTR regFunc = &(CmpISPFuncs::RegFuncs);

  if ( initCalled_ )
    return;

  // todo, error handling.
  // Query cache virtual tables
  QueryCacheStatStoredProcedure::Initialize(regFunc);
  QueryCacheEntriesStoredProcedure::Initialize(regFunc);
  QueryCacheDeleteStoredProcedure::Initialize(regFunc);

  HybridQueryCacheStatStoredProcedure::Initialize(regFunc);
  HybridQueryCacheEntriesStoredProcedure::Initialize(regFunc);
  
  // NATable cache statistics virtual table
  NATableCacheStatStoredProcedure::Initialize(regFunc);
  NATableCacheEntriesStoredProcedure::Initialize(regFunc);

    // NATable cache statistics delete
  NATableCacheDeleteStoredProcedure::Initialize(regFunc);

  // NARoutine cache statistics virtual table
  NARoutineCacheStatStoredProcedure::Initialize(regFunc);

  // NARoutine cache statistics delete
  NARoutineCacheDeleteStoredProcedure::Initialize(regFunc);

  // insert an empty entry to indicate end of array
  CmpISPFuncs::procFuncsArray_.insert(CmpISPFuncs::ProcFuncsStruct());
  initCalled_ = TRUE;
}

CmpISPInterface::~CmpISPInterface()
{
}

//
// NOTE: The cmpISPInterface variable is being left as a global
//       because, once it is initialized, it is never changed
//       AND the initial thread in the process should initialize it
//       before any other Compiler Instance threads can be created.
//
CmpISPInterface cmpISPInterface;



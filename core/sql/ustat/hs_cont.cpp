/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1996-2014 Hewlett-Packard Development Company, L.P.
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
 *****************************************************************************
 *
 * File:         hs_cont.C
 * Description:  Context functions.
 * Created:      03/25/96
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */

// -----------------------------------------------------------------------

#define HS_FILE "hs_cont"

#include "hs_globals.h"
#include "hs_cont.h"


static HSGlobalsClass *hs_cont[USTAT_MAX_NUM_CONTEXTS] =
       {NULL, NULL, NULL, NULL, NULL};

// -----------------------------------------------------------------------
// Context management functions.
// -----------------------------------------------------------------------
UstatContextID AddHSContext(HSGlobalsClass *hs_globals)
{
  hs_cont[0] = hs_globals;
  return 0;
}

void DeleteHSContext(UstatContextID id)
{
  hs_cont[id] = NULL;
}

HSGlobalsClass* GetHSContext(UstatContextID id)
{
  if (id == NULL_USTAT_ID)
    id = 0;
  return (hs_cont[id]);
}





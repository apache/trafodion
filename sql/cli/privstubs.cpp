/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1998-2014 Hewlett-Packard Development Company, L.P.
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
 * File:         PrivStubs.cpp
 * Description: contains stubs required for the cli priv SRL
 *
 * Created:      10/19/98
 * Language:     C++
 * Status:       $State: Exp $
 *
 *
 *****************************************************************************
 */
#include "Platform.h"

/*
extern "C"
int putenv(const char *)
{
  return -1;
}

extern "C"
const char * getenv(const char *)
{
  return 0;
}
*/

void * operator new(UInt32 s)
{
  return 0;
}

void operator delete(void * ptr)
{

}


extern "C"

void __pl__FPCcRC8NAString(){}; 



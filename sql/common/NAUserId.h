/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2002-2014 Hewlett-Packard Development Company, L.P.
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
#ifndef NAUSERID_H
#define NAUSERID_H
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         NAUserId.h
 * Description:  Definition of NAUserId
 * Created:      8/19/2002
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */
#include "Platform.h"

#define MAX_USERID_LEN 4 // int 32
#define MAX_DBUSERNAME_LEN 128
#define MAX_USERNAME_LEN 128
#define MAX_AUTHNAME_LEN 128
#define MAX_AUTHID_AS_STRING_LEN 20
#define MIN_USERID 33333
#define MAX_USERID 999999
#define MIN_ROLEID 1000000
#define MAX_ROLEID 1499999
#define NA_UserId Int32
#define NA_AuthID Int32
#define NA_UserIdDefault 0

#endif  /*  NAUSERID_H*/

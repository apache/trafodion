/* -*-C++-*-
 *****************************************************************************
 *
 * File:         TempIncludes.C
 * Description:  Contains code to convert NSK params to C env variables.
 *               
 * Created:      12/6/1996
 * Language:     C++
 *
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
 *
 *****************************************************************************
 */


#include <stdlib.h>
#include "Platform.h"
#include "TempIncludes.h"
#include "string.h"

void tempInit()
{ 
  const size_t MAX_BUF = 120;
  char env_buf[MAX_BUF];
  const char *env_ptr;

  env_ptr  = getenv("SQL^CMP^LOG");
  if (env_ptr && strlen(env_ptr) < MAX_BUF-100) {
    strcpy(env_buf, "SQL_CMP_LOG=");
    strcat(env_buf, env_ptr);
    putenv(env_buf);
  }

#ifndef NDEBUG
  env_ptr  = getenv("SQLMP^READTABLEDEF");
  if (env_ptr && strlen(env_ptr) < MAX_BUF-100) {
    strcpy(env_buf, "SQLMP_READTABLEDEF=");
    strcat(env_buf, env_ptr);
    putenv(env_buf);
  }
#endif

}

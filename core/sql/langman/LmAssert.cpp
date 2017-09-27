/* -*-C++-*-
 *****************************************************************************
 *
 * File:         LmAssert.cpp
 * Description:  assertion function of the language manager
 *               
 *               
 * Created:      5/4/02
 * Language:     C++
 *
 *
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
 *
 *
 *****************************************************************************
 */
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include "Platform.h"
#include "LmExtFunc.h"

// Exclude this function from coverage as it is called only when there is an assertion in LM
// which results in UDR server abend, so no coverage info can be generated.
void lmAssert(const char *file, Int32 linenum, const char *msg)
{
  if (!file)
    file = "";
  if (!msg)
    msg = "";

  cout << "LM Assertion: " << msg << endl
       << "at FILE: " << file << " LINE: " << linenum << endl;

  char message[1060];  // 1024 for 'msg'

  snprintf(message, 1060,  "Language Manager internal error : %s", msg);

  lmMakeTFDSCall(message, file, linenum);
  // should not reach here
}





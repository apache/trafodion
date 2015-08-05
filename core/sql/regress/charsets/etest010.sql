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
// Test: etest010.sql (part of TEST006)
// name in DUTP: NG2 and NG3
// Functionality: This tests SQL/C hostvars can not be declared
/                 UNICODE and VARNCHAR is not a valid keyword.
// Expected files:   part of EXPECTED006
// Tables created:   none
// Limitations: None


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
long SQLCODE;

EXEC SQL BEGIN DECLARE SECTION;
    char CHARACTER SET IS UNICODE char_unicode[256];
    VARCHAR CHARACTER SET IS UNICODE varchar_unicode[256];
    VARNCHAR varnchar_unicode[256];
EXEC SQL END DECLARE SECTION;

int main(int argc, char **argv)
{
  return 0;
}



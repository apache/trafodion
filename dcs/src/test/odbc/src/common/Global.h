/*************************************************************************
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
**************************************************************************/
#ifndef ODBC_GLOBAL_H
#define ODBC_GLOBAL_H

#ifdef _WIN64
#include <windows.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sql.h>
#include <sqlext.h>
#else
#include <unistd.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sql.h>
#include <sqlext.h>
#include <stdio.h>
#include <math.h>
#endif // _WIN64

using namespace std;

#define MAX_SQLSTRING_LEN		1000
#define STATE_SIZE				6
#define MAX_CONNECT_STRING      256
#define ENDIAN_PRECISION_MAX	39

#define TRAF_ISO88591			0x0001
#define TRAF_UTF8				0x0002
#define TRAF_ALL				TRAF_ISO88591|TRAF_UTF8

/************************************************************************/
/* conversion from char array to numeric				                */
/************************************************************************/
int ConvertCharToCNumeric(SQL_NUMERIC_STRUCT& numericTmp, CHAR* cTmpBuf);

#endif // !ODBC_GLOBAL_H
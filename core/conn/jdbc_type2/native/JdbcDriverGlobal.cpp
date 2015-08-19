/**************************************************************************
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
#include "platform_ndcs.h"
#include "JdbcDriverGlobal.h"
#include "jni.h"

char *gJNILayerErrorMsgs[] =
{
    "Unknown Error - ",
    "Programming Error - ",
    "Operation Cancelled - ",
    "Numeric value out of range ",
    "String data right-truncated ",
    "TMF error has occurred : ",
    "Error while obtaining the system catalog name : ",
    "All parameters are not set ",
    "Invalid transaction state",
    "Module Error - ",
    "Invalid Statement/Connection handle",
    "No error message in SQL/MX diagnostics area, but sqlcode is non-zero",
    "Invalid or null SQL string",
    "Invalid or null statement label or name",
    "Invalid or null module name",
    "Unsupported character set encoding",
	"Data type not supported",
	"Exceeded JVM allocated memory",
	"Restricted data type  ",
    NULL
};

JNIEnv *gJEnv = NULL;
JNICache_def gJNICache;

const char *defaultEncodingOption = "DEFAULT";
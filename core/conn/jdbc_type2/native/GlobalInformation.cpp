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
#include "GlobalInformation.h"
#include <jni.h>
#include <string.h>

//GlobalInformation globalInfo;
long int		GlobalInformation::SQLMX_Version = 0;
SQLCTX_HANDLE	GlobalInformation::CurrentContext;
jint			GlobalInformation::charset;											// SQL/MX supported char sets (from sqlcli.h) 
//const char		GlobalInformation::*encodingName = NULL;							// Normal C encoding name
//jstring			GlobalInformation::encodingNameJava;								// Normal Java encoding name
//jboolean		GlobalInformation::useDefaultEncoding = JNI_FALSE;					// Allows a "default" value for the encoding
//int				GlobalInformation::totalCharsets = 0;
//jint			GlobalInformation::defaultCharset;
//short			GlobalInformation::nowaitFilenum;
//char			GlobalInformation::NskSystemCatalogName[MAX_NSKCATALOGNAME_LEN+1];	// MP system catalog name
//char			GlobalInformation::DefaultCatalog[MAX_CHAR_SET_STRING_LENGTH + 1];
//char			GlobalInformation::DefaultSchema[MAX_CHAR_SET_STRING_LENGTH + 1];
//char			GlobalInformation::SystemCatalog[MAX_MX_SYSTEM_CATALOG_NAME + 1];	// MX system catalog name

GlobalInformation::GlobalInformation()
{
}

GlobalInformation::~GlobalInformation()
{
}

/*
void GlobalInformation::setMX_SystemsCatalog (char *sysCat) {
	strncpy(SystemCatalog, sysCat, MAX_MX_SYSTEM_CATALOG_NAME);
	SystemCatalog[MAX_MX_SYSTEM_CATALOG_NAME + 1] = '\0';
	return;
}
*/

//GlobalInformation GlobalInformation::globalInfo;

//void GlobalInformation::setUpGlobalInfo (){
//	GlobalInformation::globalInfo = GlobalInformation();
//}

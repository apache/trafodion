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
#ifndef _GLOBALINFORMATION_H_
#define _GLOBALINFORMATION_H_

#include <jni.h>
#include "Debug.h"
#include <sqlcli.h>


#define MAX_NSKCATALOGNAME_LEN	25
#define MAX_MX_SYSTEM_CATALOG_NAME 128

class GlobalInformation {
	static long int			SQLMX_Version;
	static SQLCTX_HANDLE	CurrentContext;
	static jint				charset;			// SQL/MX supported char sets (from sqlcli.h) 
//	static const char		*encodingName;		// Normal C encoding name
//	static jstring			encodingNameJava;	// Normal Java encoding name
//	static jboolean			useDefaultEncoding;	// Allows a "default" value for the encoding
//	static int				totalCharsets;
//	static jint				defaultCharset;
//	static char				NskSystemCatalogName[MAX_NSKCATALOGNAME_LEN+1]; // MP system catalog name
//	static char				DefaultCatalog[MAX_CHAR_SET_STRING_LENGTH + 1];
//	static char				DefaultSchema[MAX_CHAR_SET_STRING_LENGTH + 1];
//	static char				SystemCatalog[MAX_MX_SYSTEM_CATALOG_NAME+1];	// MX system catalog name
public:
	GlobalInformation();
	~GlobalInformation();
	
	inline static void setSQLMX_Version () {
		//SQLMX_Version = CLI_CLI_VERSION ();
		return;
	}
	
	inline static long int getSQLMX_Version () {
		return (SQLMX_Version);
	}

	inline static void setCurrentContext (SQLCTX_HANDLE context) {
		CurrentContext = context;
		return;
	}
	
	inline static SQLCTX_HANDLE getCurrentContext () {
		return (CurrentContext);
	}
	
/*	
	static void setMX_SystemsCatalog ( char *sysCat );
	
	inline static char *getMX_SystemsCatalog () {
		return (SystemCatalog);
	}

	static void setDefaultCatalogName (char *catName);
	
	static char *getDefaultCatalogName () {
		return (DefaultCatalog);
	}
	
	static void setDefaultSchemaName (char *schemaName);
	
	static char *getDefaultSchemaName () {
		return (DefaultSchema);
	}
*/	
};

#endif //_GLOBALINFORMATION_H_

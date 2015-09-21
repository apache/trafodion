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
********************************************************************/ 
// TranslationDll.h : include file 
//

#ifndef TRANSLATION_H
#define TRANSLATION_H

#include <windows.h>
#include <sql.h>
#include <sqltypes.h>
#include <sqlext.h>
#include <string.h>
#include <limits.h>
#include "stdafx.h"
#include "csconvert.cpp"

extern "C"{
BOOL SQL_API SQLDriverToDataSource(
	UDWORD	fOption,
	SWORD	fSqlType,
	PTR		rgbValueIn,
	SDWORD	cbValueIn,
	PTR		rgbValueOut,
	SDWORD	cbValueOutMax,
	SDWORD*	pcbValueOut,
	UCHAR*	szErrorMsg,
	SWORD	cbErrorMsgMax,
	SWORD*	pcbErrorMsg);

BOOL SQL_API SQLDataSourceToDriver(
	UDWORD	fOption,
	SWORD	fSqlType,
	PTR		rgbValueIn,
	SDWORD	cbValueIn,
	PTR		rgbValueOut,
	SDWORD	cbValueOutMax,
	SDWORD*	pcbValueOut,
	UCHAR*	szErrorMsg,
	SWORD	cbErrorMsgMax,
	SWORD*	pcbErrorMsg,
	PTR		replacementChar=NULL);
}

void HandleCnvError (int cnv_error, unsigned char* errorMsg, long errorMsgMax);

#endif // TRANSLATION_H
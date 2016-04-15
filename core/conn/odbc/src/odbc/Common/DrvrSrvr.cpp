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
/**************************************************************************
**************************************************************************/
//
// MODULE: SrvrDrvr.cpp
//
// PURPOSE: Functions Needed for both Drvr and Srvr
//			Ensure that there is no Drvr and Srvr dependencies in these functions
//		
//

#include <platform_ndcs.h>
#include <sqltypes.h>
#include <stdio.h>
#include <memory.h>
#include "sqlcli.h"
#include "cee.h"
#include "odbcCommon.h"
#include "glu.h"
#include "DrvrSrvr.h"


using namespace SRVR;

int SRVR::getAllocLength(int DataType, int Length)
{
	int AllocLength;

	switch (DataType) {
		case SQLTYPECODE_CHAR:
		//case SQLTYPECODE_CHAR_UP: // sqlcli doesn't support anymore
		case SQLTYPECODE_VARCHAR:
			AllocLength = Length+1;	
			break;
		case SQLTYPECODE_VARCHAR_WITH_LENGTH:
		//case SQLTYPECODE_VARCHAR_UP: // sqlcli doesn't support anymore
		case SQLTYPECODE_VARCHAR_LONG:
		case SQLTYPECODE_BLOB:
		case SQLTYPECODE_CLOB:
			AllocLength = Length+3;
			break;
		default:
			AllocLength = Length;
			break;
		}
	return AllocLength;
}


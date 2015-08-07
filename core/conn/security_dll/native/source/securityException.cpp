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
#include "securityException.h"

// Security Exception constructor

#define SQLSTATE "38001"

SecurityException::SecurityException(int errNum, char* param)
{
	int msgLen = strlen(TableErrArray[errNum].errMsg) + 1;
    err_code = errNum;
	SQLerr_code = TableErrArray[errNum].SQLErrNum;
    memcpy(&sql_state[0], SQLSTATE, strlen(SQLSTATE));
	sql_state[6] = '\0';
	if (param)
	{
		msgLen += strlen(param) - 2; //subtract "%s" which is replaced by the param
        sprintf((char *)err_msg, TableErrArray[errNum].errMsg, param);
	}
	else
		sprintf((char *)err_msg, TableErrArray[errNum].errMsg);
	err_msg[msgLen] = '\0';

}

// Security Exception destructor

SecurityException::~SecurityException() {};


char* SecurityException::getMsg()
{
	return err_msg;
}

int SecurityException::getSQLErrCode()
{
	return SQLerr_code;
}

char* SecurityException::getSQLState()
{
	return sql_state;
}

int SecurityException::getErrCode()
{
	return err_code;
}


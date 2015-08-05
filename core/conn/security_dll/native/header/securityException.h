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
//
**********************************************************************/

#ifndef securityexceptionh
#define securityexceptionh

#include <stdio.h>
#include <string.h>
#include "clientErrCode.h"

#ifdef _WINDOWS
class __declspec( dllexport ) SecurityException
#else
class SecurityException
#endif
{
   public:
      SecurityException();
	  SecurityException(int errNum, char* param);
      ~SecurityException();

	  char* getMsg();
	  int getErrCode();
	  int getSQLErrCode();
	  char* getSQLState();

   private:
      int SQLerr_code;           // error code from file clientErrCode.h
      char err_msg[400];      // error info 
	  int err_code;
      char sql_state[6];
};

#endif

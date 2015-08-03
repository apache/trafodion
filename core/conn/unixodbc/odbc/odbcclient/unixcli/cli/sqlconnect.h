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
//
#ifndef SQLCONNECT_H
#define SQLCONNECT_H

#include <windows.h>
#include <sql.h>

namespace ODBC {

extern SQLRETURN Connect(SQLHDBC ConnectionHandle,
           SQLCHAR *ServerName, 
		   SQLSMALLINT NameLength1,
           SQLCHAR *UserName, 
		   SQLSMALLINT NameLength2,
           SQLCHAR *Authentication, 
		   SQLSMALLINT NameLength3);
extern SQLRETURN Disconnect(SQLHDBC ConnectionHandle);
extern SQLRETURN SetConnectAttr(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER Value,
           SQLINTEGER StringLength);
extern SQLRETURN GetConnectAttr(SQLHDBC ConnectionHandle,
           SQLINTEGER Attribute, 
		   SQLPOINTER ValuePtr,
           SQLINTEGER BufferLength,
		   SQLINTEGER *StringLengthPtr);
extern SQLRETURN GetInfo(SQLHDBC ConnectionHandle,
           SQLUSMALLINT InfoType, 
		   SQLPOINTER InfoValuePtr,
           SQLSMALLINT BufferLength,
		   SQLSMALLINT *StringLengthPtr);
extern SQLRETURN DriverConnect(SQLHDBC ConnectionHandle,
			SQLHWND		       WindowHandle,
			SQLCHAR 		  *InConnectionString,
			SQLSMALLINT        StringLength1,
			SQLCHAR           *OutConnectionString,
			SQLSMALLINT        BufferLength,
			SQLSMALLINT 	  *StringLength2Ptr,
			SQLUSMALLINT       DriverCompletion);
extern SQLRETURN NativeSql(
    SQLHDBC            ConnectionHandle,
    SQLCHAR 		  *InStatementText,
    SQLINTEGER         TextLength1,
    SQLCHAR 		  *OutStatementText,
    SQLINTEGER         BufferLength,
    SQLINTEGER 		  *TextLength2Ptr);
extern SQLRETURN BrowseConnect(SQLHDBC  ConnectionHandle,
    SQLCHAR 		  *InConnectionString,
    SQLSMALLINT        StringLength1,
    SQLCHAR 		  *OutConnectionString,
    SQLSMALLINT        BufferLength,
    SQLSMALLINT       *StringLength2Ptr);
}

#endif

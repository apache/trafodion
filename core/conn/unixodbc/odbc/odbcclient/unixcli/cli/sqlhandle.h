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
#ifndef SQLHANDLE_H
#define SQLHANDLE_H

#include <windows.h>
#include <sql.h>

namespace ODBC {

extern SQLRETURN AllocHandle(SQLSMALLINT HandleType,
			SQLHANDLE InputHandle, 
			SQLHANDLE *OutputHandle);

extern SQLRETURN FreeHandle(SQLSMALLINT HandleType, 
			SQLHANDLE Handle);

extern SQLRETURN GetDiagRec(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT RecNumber,
				SQLCHAR *Sqlstate,
				SQLINTEGER *NativeError, 
				SQLCHAR *MessageText,
				SQLSMALLINT BufferLength, 
				SQLSMALLINT *TextLength);

extern SQLRETURN GetDiagField(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT RecNumber, 
				SQLSMALLINT DiagIdentifier,
				SQLPOINTER DiagInfo, 
				SQLSMALLINT BufferLength,
				SQLSMALLINT *StringLength);

extern SQLRETURN EndTran(SQLSMALLINT HandleType, 
				SQLHANDLE Handle,
				SQLSMALLINT CompletionType);

}
#endif

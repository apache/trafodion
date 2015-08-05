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
#ifndef DLL_EXPORT_DEFINES_H
#define DLL_EXPORT_DEFINES_H

#undef SQLCLI_LIB_FUNC

#if (defined (CLI_DLL) && !defined (NA_NSK))
	#ifdef SQLCLI_LIB
		#define SQLCLI_LIB_FUNC __declspec( dllexport )
	#else
		#define SQLCLI_LIB_FUNC __declspec( dllimport )
	#endif
#else
	#define SQLCLI_LIB_FUNC
#endif
#ifdef NA_64BIT
  // dg64 - get rid of _declspec
  #undef SQLCLI_LIB_FUNC
  #define SQLCLI_LIB_FUNC
#endif

#endif // DLL_EXPORT_DEFINES.h

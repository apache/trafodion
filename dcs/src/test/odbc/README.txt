/*************************************************************************
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

To build odbc test on Windows:
1.Enter directory <WHERE_YOU_PLACE_ODBC_FOLDER>\odbc\build
2.Open odbc.vcxproj in Microsoft visual studio
3.Choose Platform as "x64" and Configuration as "Debug" in visual studio BUILD--Configuration Manager
4.Build project "odbc" in visual studio

To run odbc test on Windows using command line tool:
>cd [<WHERE_YOU_PLACE_ODBC_FOLDER>]\odbc\build\x64\Debug
>odbc -d <datasource> -u <userid> -p <password>

To build odbc test on Linux:
>cd <WHERE_YOU_PLACE_ODBC_FOLDER>\odbc\build
>make

To run odbc test on Linux:
1.Add odbc configuration option "AppUnicodeType" as "utf16" in the installed TRAFDSN file like:
================================
[ODBC]
......
AppUnicodeType          = utf16
================================
2.Type commands as follows:
>cd <WHERE_YOU_PLACE_ODBC_FOLDER>\odbc\build
>./odbc -d <datasource> -u <userid> -p <password>
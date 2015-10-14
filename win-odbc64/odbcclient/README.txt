# @@@ START COPYRIGHT @@@
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
# @@@ END COPYRIGHT @@@

Prerequisite:

openssl >= 0.9.8
zlib >= 1.2.8
Microsoft Visual Studio >= 2013
Inno Setup >= 5

Build steps

1. In order to make the build script able to find these prerequisites,
   some variables on the top of build_os.bat and pkg.bat need to be set accordingly.

   Set variable OPENSSL_LIB_PATH to point to openssl library files folder, for example:
	set OPENSSL_PATH=C:\openssl-1.0.1e\lib

   Set variable OPENSSL_INCLUDE_PATH to point to openssl header files folder, for example:
    set OPENSSL_INCLUDE_PATH=C:\openssl-1.0.1e\include

   Set variable ZLIB_INCLUDE_PATH to point to zlib header files folder, for example:
    set ZLIB_INCLUDE_PATH=C:\zlib\include

   Set variable ZLIB_LIB_PATH to point to zlib library files folder, for example:
    set ZLIB_LIB_PATH=C:\zlib\lib

   Set variable MSBUILD_PATH to point to msbuild system, for example:
	set MSBUILD_PATH=C:\Windows\Microsoft.NET\Framework64\v4.0.30319

   Set variable PACKDIR to conn folder of Trafodion source tree, for example:
	set PACKDIR=C:\Build\winodbc64

   Set variable INNO_SETUP_PATH to inno setup install folder, for example:
	set INNO_SETUP_PATH="C:\Program Files (x86)\Inno Setup 5"

2. To build, open a cmd window, change to win-odbc64\odbcclient and type
	build_os.bat

The final Win ODBC driver installer package can be found at %PACKDIR%
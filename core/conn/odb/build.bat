@echo off
REM @@@ START COPYRIGHT @@@
REM
REM Licensed to the Apache Software Foundation (ASF) under one
REM or more contributor license agreements.  See the NOTICE file
REM distributed with this work for additional information
REM regarding copyright ownership.  The ASF licenses this file
REM to you under the Apache License, Version 2.0 (the
REM "License"); you may not use this file except in compliance
REM with the License.  You may obtain a copy of the License at
REM
REM   http://www.apache.org/licenses/LICENSE-2.0
REM
REM Unless required by applicable law or agreed to in writing,
REM software distributed under the License is distributed on an
REM "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
REM KIND, either express or implied.  See the License for the
REM specific language governing permissions and limitations
REM under the License.
REM
REM @@@ END COPYRIGHT @@@

set PACKDIR=C:\Build\odb

SET BUILDDIR=%CD%
set MSBUILD_PATH=C:\Windows\Microsoft.NET\Framework64\v4.0.30319
set PATH=%MSBUILD_PATH%;%PATH%

REM set this to zlib header files directory
set ZLIB_INCLUDE_PATH=C:\zlib\include
REM set this to zlib library files directory
set ZLIB_LIB_PATH=C:\zlib\lib

REM set this to openssl header files directory
set OPENSSL_INCLUDE_PATH=C:\openssl-1.0.1e\include
REM set this to openssl library files directory
set OPENSSL_LIB_PATH=C:\openssl-1.0.1e\lib

set ALL_SUCCESS=0

echo=
echo ===============================
echo     BUILD WIN64 RELEASE
echo ===============================
echo=

echo Building ODB - Win64 Release...
cd %BUILDDIR%\odb
msbuild.exe /t:rebuild odb.vcxproj /p:Platform=x64 /p:Configuration=Release /p:ZlibLibDir=%ZLIB_LIB_PATH%
cd %BUILDDIR%
if %ERRORLEVEL% == 0 (
	copy /Y odb\x64\Release\odb.exe %PACKDIR%
	echo Build windows ODB success
) else (
	echo Build windows ODB failed
)
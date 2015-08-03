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
@echo off
rem This script builds (or cleans up) the message DLL for ODBC/MX.
rem
rem Usage:
rem     cd $PSQL\msg\ODBC
rem     call msgbuild.bat <target>
rem where <target> is ALL or CLEAN.  If omitted, we assume ALL.
rem This script runs "msgbuild.mak" passing <target>.
rem
rem Environment variables:
rem PSQL_MSGLANG specifies the name of a language-specific subdirectory
rem     (under the project message directory) where the message definition
rem     file (.MC file) lives.
rem     If it is not specified, this script sets it to "enu" (English).
rem PSQL_MSGLIB specifies the directory where the message DLL will go.
rem     If it is not specified, this script sets it to "..\lib"
rem   (thus pointing to $PSQL\msg\lib).
rem

rem Set PSQL_MSGLANG and PSQL_MSGLIB if not already set.
if .%PSQL_MSGLANG% == . set PSQL_MSGLANG=enu
if .%PSQL_MSGLIB%  == . set PSQL_MSGLIB=..\lib
set BUILDDIR=%PSQL%\msg\odbc
rem set BUILDDIR=".\"

:TestDirExistence
if not exist %PSQL_MSGLIB% goto MsgLibNotFoundError
if not exist %BUILDDIR% goto BuildDirNotFoundError
if not exist %PSQL% goto PSQLDirNotFoundError

rem Verify the caller passed an appropriate argument.
set MsgArg=%1
if .%MsgArg% == .     set MsgArg=ALL
if %MsgArg%  == ALL   goto ODBCBuildDlls
if %MsgArg%  == CLEAN goto ODBCBuildClean
if %MsgArg%  == -CLEAN goto ODBCBuildClean
if %MsgArg%  == -clean goto ODBCBuildClean
if %MsgArg%  == -?    goto usage
if %MsgArg% == /h goto usage
if %MsgArg% == /? goto usage
if /I %MsgArg% == /Help goto usage
if /I %MsgArg% == -help goto usage
if %MsgArg% == ""  goto usage
echo Argument must be ALL or CLEAN: %1
goto failure

:ODBCBuildDlls
rem Delete any existing log file.
if exist %BUILDDIR%\makelog.txt del %BUILDDIR%\makelog.txt
echo Building ODBC/MX Message Dlls
echo Building ODBC/MX Message Dlls >> %BUILDDIR%\makelog.txt
set BuildError=0

rem Run the message DLL Makefile for each project in ODBC.

cd %BUILDDIR%\SrvrMsg
echo Building SrvrMsg
echo Building SrvrMsg >> %BUILDDIR%\makelog.txt
nmake /c /s /x %BUILDDIR%\ODBCMake.log /f msgbuild.mak ALL >> %BUILDDIR%\makelog.txt
if ErrorLevel 1 set BuildError=1
type %BUILDDIR%\ODBCMake.log >> %BUILDDIR%\makelog.txt
del %BUILDDIR%\ODBCMake.log
if %BuildError% == 1 goto failure

cd %BUILDDIR%\DrvMsg
echo Building DrvMsg
echo Building DrvMsg >> %BUILDDIR%\makelog.txt
nmake /c /s /x %BUILDDIR%\ODBCMake.log /f msgbuild.mak ALL >> %BUILDDIR%\makelog.txt
if ErrorLevel 1 set BuildError=1
type %BUILDDIR%\ODBCMake.log >> %BUILDDIR%\makelog.txt
del %BUILDDIR%\ODBCMake.log
if %BuildError% == 1 goto failure

cd %BUILDDIR%
goto success

:usage
echo
echo Usage:
echo     cd $PSQL\msg\ODBC
echo     call msgbuild.bat {target}
echo where {target} is ALL or CLEAN.  If omitted, we assume ALL.
echo This script runs "msgbuild.mak" passing {target}.
echo
echo Environment variables:
echo PSQL_MSGLANG specifies the name of a language-specific subdirectory
echo     (under the project message directory) where the message definition
echo     file (.MC file) lives.
echo     If it is not specified, this script sets it to "enu" (English).
echo PSQL_MSGLIB specifies the directory where the message DLL will go.
echo     If it is not specified, this script sets it to "..\lib"
echo   (thus pointing to $PSQL\msg\lib).
echo
goto finish

:ODBCBuildClean
cd %BUILDDIR%
rem echo cleaning files in %BUILDDIR%
rem pause
echo Cleaning DrvMsg
echo Cleaning DrvMsg >> %BUILDDIR%\makelog.txt
cd %BUILDDIR%\DrvMsg
nmake /c /s /x %BUILDDIR%\ODBCMake.log /f msgbuild.mak CLEAN >> %BUILDDIR%\makelog.txt
echo Cleaning SrvrMsg
echo Cleaning SrvrMsg >> %BUILDDIR%\makelog.txt
cd %BUILDDIR%\SrvrMsg
nmake /c /s /x %BUILDDIR%\ODBCMake.log /f msgbuild.mak CLEAN >> %BUILDDIR%\makelog.txt
cd %BUILDDIR%
goto finish

:MsgLibNotFoundError
echo Error%0:%PSQL_MSGLIB% Directory Not Found
echo Error%0:%PSQL_MSGLIB% Directory Not Found >> makelog.txt
goto failure

:BuildDirNotFoundError
echo Error %0:%BUILDDIR% Not Found
echo Error %0:%BUILDDIR% Not Found >> %BUILDDIR%\makelog.txt
goto failure

:PSQLDirNotFoundError
echo Error%0:%PSQL% Directory Not Found
echo Error%0:%PSQL% Directory Not Found >> %BUILDDIR%\makelog.txt
goto failure

:failure
rem Set the exit code so our caller can notice the failure.
rem Assumes we include false.exe in our tools.
echo.
echo ***
echo *** %0 ABORTED 
echo ***
echo. >> %BUILDDIR%\makelog.txt
echo *** >> %BUILDDIR%\makelog.txt
echo *** %0 ABORTED >> %BUILDDIR%\makelog.txt
echo *** >> %BUILDDIR%\makelog.txt
false
goto finish

:success

:finish
cd %BUILDDIR%

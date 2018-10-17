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

REM set this to the directory where you want to put the driver package file
set PACKDIR=C:\Build\winodbc64

@set INNO_SETUP_PATH="C:\Program Files (x86)\Inno Setup 5"
@set PATH=%INNO_SETUP_PATH%;%PATH%

set MSBUILD_PATH=C:\Windows\Microsoft.NET\Framework64\v4.0.30319
REM get the build directory automatically, if failed please set this to top directory include everything for build
CD ..\..
SET BUILDDIR=%CD%
CD win-odbc64\odbcclient

REM set this to zlib header files directory
set ZLIB_INCLUDE_PATH=C:\zlib\include
REM set this to zlib library files directory
set ZLIB_LIB_PATH=C:\zlib\lib

REM set this to openssl header files directory
set OPENSSL_INCLUDE_PATH=C:\openssl-1.0.1e\include
REM set this to openssl library files directory
set OPENSSL_LIB_PATH=C:\openssl-1.0.1e\lib

set SRCDIR=%BUILDDIR%\win-odbc64
set LIBDIR=%BUILDDIR%\lib
set PATH=%MSBUILD_PATH%\;%PATH%

set ALL_SUCCESS=0

echo=
echo ===============================
echo     BUILD WIN64 RELEASE
echo ===============================
echo=

echo Building Drvr35Msg - Win64 Release...
cd %SRCDIR%\odbcclient\Drvr35Msg
msbuild.exe /t:rebuild Drvr35Msg_os.vcxproj /p:Platform=x64 /p:Configuration=Release
set BUILD_STATUS=%ERRORLEVEL%
if %BUILD_STATUS%==0 (
	echo Build Drvr35Msg success
) else (
	echo Build Drvr35Msg failed
	goto Exit
)


echo Building Drvr35 - Win64 Release...
cd %SRCDIR%\odbcclient\Drvr35
msbuild.exe /t:rebuild Drvr35_os.vcxproj /p:Platform=x64 /p:Configuration=Release /p:OpenSSLIncludeDir=%OPENSSL_INCLUDE_PATH% /p:OpenSSLLibraryDir=%OPENSSL_LIB_PATH% /p:ZlibIncludeDir=%ZLIB_INCLUDE_PATH% /p:ZlibLibDir=%ZLIB_LIB_PATH%
set BUILD_STATUS=%ERRORLEVEL%
if %BUILD_STATUS%==0 (
	echo Build Drvr35 success
) else (
	echo Build Drvr35 failed
	goto Exit
)

echo Building Drvr35Adm - Win64 Release...
cd %SRCDIR%\odbcclient\Drvr35Adm
msbuild.exe /t:rebuild Drvr35Adm_os.vcxproj /p:Platform=x64 /p:Configuration=Release
set BUILD_STATUS=%ERRORLEVEL%
if %BUILD_STATUS%==0 (
	echo Build Drvr35Adm success
) else (
	echo Build Drvr35Adm failed
	goto Exit
)

REM echo Building Drvr35Trace - Win64 Release...
REM cd %SRCDIR%\odbcclient\Drvr35Trace
REM msbuild.exe /t:rebuild Drvr35Trace_os.vcxproj /p:Platform=x64 /p:Configuration=Release
REM set BUILD_STATUS=%ERRORLEVEL%
REM if %BUILD_STATUS%==0 (
REM	echo Build Drvr35Trace success
REM ) else (
REM	echo Build Drvr35Trace failed
REM	goto Exit
REM )

echo Building TCPIPV4 - Win64 Release...
cd %SRCDIR%\odbcclient\Drvr35\TCPIPV4
msbuild.exe /t:rebuild TCPIPV4_os.vcxproj /p:Platform=x64 /p:Configuration=Release /p:ZlibIncludeDir=%ZLIB_INCLUDE_PATH% /p:ZlibLibDir=%ZLIB_LIB_PATH%
set BUILD_STATUS=%ERRORLEVEL%
if %BUILD_STATUS%==0 (
	echo Build TCPIPV4 success
) else (
	echo Build TCPIPV4 failed
	goto Exit
)

echo Building TCPIPV6 - Win64 Release...
cd %SRCDIR%\odbcclient\Drvr35\TCPIPV6
msbuild.exe /t:rebuild TCPIPV6_os.vcxproj /p:Platform=x64 /p:Configuration=Release /p:ZlibIncludeDir=%ZLIB_INCLUDE_PATH% /p:ZlibLibDir=%ZLIB_LIB_PATH%
set BUILD_STATUS=%ERRORLEVEL%
if %BUILD_STATUS%==0 (
	echo Build TCPIPV6 success
) else (
	echo Build TCPIPV6 failed
	goto Exit
)

echo Building TranslationDll - Win64 Release...
cd %SRCDIR%\odbcclient\TranslationDll
msbuild.exe /t:rebuild TranslationDll_os.vcxproj /p:Platform=x64 /p:Configuration=Release
set BUILD_STATUS=%ERRORLEVEL%
if %BUILD_STATUS%==0 (
	echo Build TranslationDll success
) else (
	echo Build TranslationDll failed
	goto Exit
)

echo Building Drvr35Res - Win64 Release...
cd %SRCDIR%\odbcclient\Drvr35Res
msbuild.exe /t:rebuild Drvr35Res_os.vcxproj /p:Platform=x64 /p:Configuration=Release
set BUILD_STATUS=%ERRORLEVEL%
if %BUILD_STATUS%==0 (
	echo Build Drvr35Res success
) else (
	echo Build Drvr35Res failed
	goto Exit
)

echo Building SetCertificateDirReg InstallHelper - Win64 Release...
cd %SRCDIR%\Install\SetCertificateDirReg\SetCertificateDirReg
msbuild.exe /t:rebuild SetCertificateDirReg_os.vcxproj /p:Platform=x64 /p:Configuration=Release
set BUILD_STATUS=%ERRORLEVEL%
if %BUILD_STATUS%==0 (
	echo Build SetCertificateDirReg success
) else (
	echo Build SetCertificateDirReg failed
	goto Exit
)

ISCC.exe /Q %BUILDDIR%\win-odbc64\Install\win64_installer\installer.iss
copy /Y %BUILDDIR%\win-odbc64\Install\win64_installer\Output\TFODBC64-2.3.0.exe %PACKDIR%
@echo on

if exist %PACKDIR%\TFODBC64-2.3.0.exe (
	set ALL_SUCCESS=1
)
cd %BUILDDIR%\win-odbc64\odbcclient

:Exit
if %ALL_SUCCESS%==1 (
	echo=
	echo ========================================
	echo     BUILD WIN64 RELEASE SUCCESSFULLY
	echo ========================================
	echo=
)

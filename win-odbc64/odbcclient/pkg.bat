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

set PACKDRIVER=E:
set SRCDIR=%PACKDRIVER%\win-odbc64
set LIBDIR=%PACKDRIVER%\lib
set ISCMDBLD_PATH="E:\Program Files (x86)\Installshield\2012\System"
set INSTALLDIR=%SRCDIR%\Install
set INSTALLFILENAME=TFODBC64.msi
set SAVEPATH=%PATH%
set PATH=%ISCMDBLD_PATH%;%PATH%

echo "Building Media for %INSTALLFILENAME% ..."
set FILENAME=%INSTALLDIR%\win64_installer\win64_installer_os.ism

ISCmdBld.exe -p "%FILENAME%"

echo "Moving %INSTALLFILENAME% ..."

move /Y "%INSTALLDIR%\win64_installer\win64_installer_os\PROJECT_ASSISTANT\SINGLE_MSI_IMAGE\DiskImages\DISK1\Trafodion ODBC64 1.0.msi" %LIBDIR%\x64\Release\%INSTALLFILENAME%

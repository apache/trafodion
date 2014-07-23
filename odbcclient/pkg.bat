REM @@@ START COPYRIGHT @@@
REM
REM (C) Copyright 2013-2014 Hewlett-Packard Development Company, L.P.
REM
REM  Licensed under the Apache License, Version 2.0 (the "License");
REM  you may not use this file except in compliance with the License.
REM  You may obtain a copy of the License at
REM
REM      http://www.apache.org/licenses/LICENSE-2.0
REM
REM  Unless required by applicable law or agreed to in writing, software
REM  distributed under the License is distributed on an "AS IS" BASIS,
REM  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM  See the License for the specific language governing permissions and
REM  limitations under the License.
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

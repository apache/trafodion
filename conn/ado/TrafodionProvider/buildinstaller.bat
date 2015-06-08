// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2015 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
@echo on
@REM setup system environment

@echo preparing...
if not exist "%VS100COMNTOOLS%" @(
	@echo *** Error: VsualStudio 2010 is required
	goto failedend
)

@call buildinfo.bat
@set "PATH=%InstallShield_Home%\System;%PATH%"

:building
@echo building...
@Call "%VS100COMNTOOLS%vsvars32.bat"
@RD /S /Q Trafodion.Data\bin
@DEL /S /F /Q %build_dir%\installer
@mkdir "%build_dir%\installer"
@if "%REVISION%"=="" @(
	@for /f "tokens=2" %%a in ('svn info ^| findstr /i revision') DO @set REVISION=%%a
)
@echo REVISION is %REVISION% 

@perl update_version.pl %Major% %Minor% %SPVersion% %REVISION% "%Trafodion-ADO_Release%" "./Trafodion.Data/Public/ProductVersion.cs"
@perl update_version.pl %Major% %Minor% %SPVersion% %REVISION% "%Trafodion-ADO_Release%" "./Trafodion.Data/Public/DisplayVersion.cs"

@DEL /F /Q %build_dir%\Trafodion.Data.dll
@DEL /F /Q %build_dir%\Trafodion.Data.ETL.dll
@DEL /F /Q %build_dir%\Trafodion.Data.VisualStudio.dll
@DEL /F /Q %build_dir%\Trafodion.Data.Installation.dll

@DEL /F /Q Release\Trafodion.Data.dll
@DEL /F /Q Release\Trafodion.Data.ETL.dll
@DEL /F /Q Release\Trafodion.Data.VisualStudio.dll
@DEL /F /Q Release\Trafodion.Data.Installation.dll
@devenv TrafodionProvider.sln /Build "release" /Project Trafodion.Data
@copy Release\Trafodion.Data.dll "%build_dir%"
if not exist "%build_dir%\Trafodion.Data.dll" goto failedend

@devenv TrafodionProvider.sln /Build "release" /Project Trafodion.Data.ETL
@copy Release\Trafodion.Data.ETL.dll "%build_dir%"
if not exist "%build_dir%\Trafodion.Data.ETL.dll" goto failedend

@devenv TrafodionProvider.sln /Build "release" /Project Trafodion.Data.VisualStudio
@copy Release\Trafodion.Data.VisualStudio.dll "%build_dir%"
if not exist "%build_dir%\Trafodion.Data.VisualStudio.dll" goto failedend

@devenv TrafodionProvider.sln /Build "release" /Project Trafodion.Data.Installation
@copy Release\Trafodion.Data.Installation.dll "%build_dir%"
if not exist "%build_dir%\Trafodion.Data.Installation.dll" goto failedend

@set ERRORLEVEL=0
@IsCmdBld -p installer\ADO.NET.ism -l PATH_TO_TRAFODIONADO="%build_dir%" -j 2.0.50727 -c COMP -b "%build_dir%\installer" -y %ADO_NET_Version%.%REVISION% -r ADO.NET -f ADO.NET -z "ProductName=Trafodion ADO.NET %ADO_NET_Version% PACKAGE" -z "ADO_NET_VERSION=%ADO_NET_Version%" -z "RELEASE_VERSION=%Trafodion-ADO_Release%" -z "SVN_REVISION=%REVISION%"
@if exist "%build_dir%\installer\PROJECT_ASSISTANT\ADO.NET\DiskImages\DISK1\setup.exe" @(
	@copy "%build_dir%\installer\PROJECT_ASSISTANT\ADO.NET\DiskImages\DISK1\setup.exe" "%build_dir%\installer\Trafodion-ADO.NET_%ADO_NET_Version%.exe"
)
@if exist "%build_dir%\installer\PROJECT_ASSISTANT\ADO.NET\DiskImages\DISK1\setup.msi" @(
	@copy "%build_dir%\installer\PROJECT_ASSISTANT\ADO.NET\DiskImages\DISK1\setup.msi" "%build_dir%\installer\Trafodion-ADO.NET_%ADO_NET_Version%.msi"
)

@if "%ERRORLEVEL%" == "1" @(
	@echo Installer was built failed by InstallShield, please check privious errors.
	goto failedend
)
@REM RD /S /Q %build_dir%\installer\PROJECT_ASSISTANT

@echo on
@echo *** The installers are under %build_dir%\installer ***
@echo *** SUCCESS ***
@goto end

:failedend
@echo on
@echo ***  build failed ***

:end


@ECHO OFF
REM     @@@ START COPYRIGHT @@@                                                 
REM                                                                             
REM (C) Copyright 2015 Hewlett-Packard Development Company, L.P.            
REM                                                                             
REM     Licensed under the Apache License, Version 2.0 (the "License");         
REM you may not use this file except in compliance with the License.        
REM You may obtain a copy of the License at                                 
REM 
REM          http://www.apache.org/licenses/LICENSE-2.0                         
REM 
REM Unless required by applicable law or agreed to in writing, software     
REM     distributed under the License is distributed on an "AS IS" BASIS,       
REM WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
REM     See the License for the specific language governing permissions and     
REM     limitations under the License.                                          
REM                                                                             
REM     @@@ END COPYRIGHT @@@
REM ########## Requirements to run build #######################################
REM Must have Visual Studio 2012 or higher Installed 
REM Must be run from a Visual Studio Command Prompt for correct environment to be set
REM Need to have code signing tool installed.
REM Must not have Visual Studio application running

REM ############### Attributes to set for Trafodion Manager build ##############

setlocal

SET COMPVERSION=1.0.0
SET RELVERSION=1.0.0
SET ProductName=Trafodion Database Manager 1.0

REM ############### Do not change anything past here ##########################

REM read variable from parameter
FOR %%P in (%*) do (
        set "%%~P"
)

REM get BuildVersion
REM echo "Please enter build & revision numbers:"
REM set /p BuildVersion= BuildVersion -^>
REM set /p RevisionVersion= RevisionVersion -^>
if "%BuildVersion%"=="" (
	FOR /F "tokens=2 delims=:" %%I in ('findstr BuildVersion buildconfig.txt') do set BuildVersion=%%I
)
REM trim space
SET BuildVersion=%BuildVersion: =%

REM get RevisionVersion
if "%RevisionVersion%"=="" (
	FOR /F "tokens=2 delims=:" %%I in ('findstr RevisionVersion buildconfig.txt') do set RevisionVersion=%%I
)
REM trim space
SET RevisionVersion=%RevisionVersion: =%

SET DEVBUILD=TRUE
SET BUILDTYPE=Debug

REM get InstallShieldRoot
if "%InstallShieldRoot%"=="" (
	FOR /F "tokens=2 delims==" %%I in ('findstr InstallShieldRoot buildconfig.txt') do set InstallShieldRoot=%%I
)

REM get BuildOutputFolder
if "%BuildOutputFolder%"=="" (
	FOR /F "tokens=2 delims==" %%I in ('findstr BuildOutputFolder buildconfig.txt') do set BuildOutputFolder=%%I
)

REM   current directory
SET BUILDROOT=%cd%
SET PROJECTROOT=%BUILDROOT%\..
SET SRCDIR=%BUILDROOT%\..\src
REM SET BuildOutputFolder=C:\TrafMgrBuild
SET TrafMgrSolutionFile=Trafodion.Manager.sln
SET BUILDLOG=%BUILDROOT%\build.log
SET ISProFileForMSI=%PROJECTROOT%\installShield\TrafodionManager.ism

SET PROJECT=TrafodionManager
if "%RevisionVersion%"=="" (
	SET RevisionVersion=0
)
SET ProductVersion=%COMPVERSION%.%RevisionVersion%
if "%BuildVersion%"=="" (
	SET BuildVersion=0
)

REM Plugin Assembly files that need to be updated
SET FrameworkAssembly=%SRCDIR%\Trafodion.Manager\Trafodion.Manager\Properties\AssemblyInfo.cs
SET MainExecutableAssembly=%SRCDIR%\Trafodion.Manager\Trafodion.Manager.Main\Properties\AssemblyInfo.cs
SET DatabaseAssembly=%SRCDIR%\Trafodion.Manager.DatabaseArea\Properties\AssemblyInfo.cs
SET OverviewAssembly=%SRCDIR%\Trafodion.Manager.Overview\Properties\AssemblyInfo.cs
SET ConnectivityAssembly=%SRCDIR%\Trafodion.Manager.ConnectivityArea\Properties\AssemblyInfo.cs
SET PluginLoaderAssembly=%SRCDIR%\Trafodion.Manager\PluginLoader\Properties\AssemblyInfo.cs
SET UniversalWidgetAssembly=%SRCDIR%\Trafodion.Manager\Trafodion.Manager.UniversalWidget\Properties\AssemblyInfo.cs
SET MetricMinerAssembly=%SRCDIR%\Trafodion.Manager\Trafodion.Manager.MetricMiner\Properties\AssemblyInfo.cs
SET WorkloadsAreaAssembly=..\src\Trafodion.Manager.WorkloadArea\Properties\AssemblyInfo.cs
SET UserManagementAssembly=..\src\Trafodion.Manager.UserManagement\Properties\AssemblyInfo.cs

REM Local Vars
SET VersionUpdateStatus=Failed
SET installerStatus=Failed


SET VersionFile=..\src\Trafodion.Manager\Trafodion.Manager\Properties\AssemblyInfo.cs
@set PATH=%VS120COMNTOOLS:~0,-15%\Common7\IDE;%VS120COMNTOOLS:~0,-15%\VC\BIN;%VS120COMNTOOLS:~0,-15%\Common7\Tools;C:\WINDOWS\Microsoft.NET\Framework\v3.5;C:\WINDOWS\Microsoft.NET\Framework\v2.0.50727;%VS120COMNTOOLS:~0,-15%\VC\VCPackages;%PATH%


REM mv -f build.log build.log.old

rename  build.log build.log.old

echo ************************Build Environment************************     > %BUILDLOG%
echo Is Development build     : %devbuild%                                 >> %BUILDLOG%
echo Build Type               : %buildtype%                                >> %BUILDLOG%
echo BuildRoot                : %BUILDROOT%                                >> %BUILDLOG%
echo Build output folder      : %BuildOutputFolder%                        >> %BUILDLOG%
echo Version source file      : %VprocFile%                                >> %BUILDLOG%
echo Build Version            : %BuildVersion%                             >> %BUILDLOG%
echo Revision                 : %RevisionVersion%                          >> %BUILDLOG%
echo *****************************************************************     >> %BUILDLOG%
REM set /p Continue = Press Any Key to continue -^>

cscript //Nologo vproc.vbs "%VersionFile%" "%COMPVERSION%" "%RELVERSION%" "%BUILDNUM%"
SET VPROCUpdateStatus=Success

REM VPROC is updated, now do Build

REM 
REM Update the AssemblyFileInfo
REM
echo Updating AssemblyFileInfo...

cscript //Nologo updateFileVersion.vbs "%FrameworkAssembly%" "%BuildVersion%" "%RevisionVersion%" >> %BUILDLOG%
cscript //Nologo updateFileVersion.vbs "%MainExecutableAssembly%" "%BuildVersion%" "%RevisionVersion%" >> %BUILDLOG%
cscript //Nologo updateFileVersion.vbs "%DatabaseAssembly%" "%BuildVersion%" "%RevisionVersion%" >> %BUILDLOG%
cscript //Nologo updateFileVersion.vbs "%OverviewAssembly%" "%BuildVersion%" "%RevisionVersion%" >> %BUILDLOG%
cscript //Nologo updateFileVersion.vbs "%ConnectivityAssembly%" "%BuildVersion%" "%RevisionVersion%" >> %BUILDLOG%
cscript //Nologo updateFileVersion.vbs "%PluginLoaderAssembly%" "%BuildVersion%" "%RevisionVersion%" >> %BUILDLOG%
cscript //Nologo updateFileVersion.vbs "%MetricMinerAssembly%" "%BuildVersion%" "%RevisionVersion%" >> %BUILDLOG%
cscript //Nologo updateFileVersion.vbs "%UniversalWidgetAssembly%" "%BuildVersion%" "%RevisionVersion%" >> %BUILDLOG%
cscript //Nologo updateFileVersion.vbs "%WorkloadsAreaAssembly%" "%BuildVersion%" "%RevisionVersion%" >> %BUILDLOG%
cscript //Nologo updateFileVersion.vbs "%UserManagementAssembly%" "%BuildVersion%" "%RevisionVersion%" >> %BUILDLOG%

if errorlevel == 500 (
    echo Error: Update of AssemblyInfo.cs failed     
    GOTO BuildFailed
) 

REM clean files from earlier build
echo Removing old build files...                        
rmdir /S/Q %BuildOutputFolder%
mkdir %BuildOutputFolder%
mkdir %BuildOutputFolder%\install
mkdir %BuildOutputFolder%\install\Plugins
mkdir %BuildOutputFolder%\install\setup
mkdir %BuildOutputFolder%\install\signed

REM Rebuild OneGui executable

REM change directory to the solution folder
cd %SRCDIR%\Trafodion.Manager

REM Build delivery project, will rebuild all exe and dll's
echo Cleaning project... >> %BUILDLOG%
devenv %TrafMgrSolutionFile% /clean %BUILDTYPE% >> %BUILDLOG%

REM for build to work right must do one debug build first for release
echo Rebuilding project... >> %BUILDLOG%
devenv %TrafMgrSolutionFile% /Rebuild %BUILDTYPE% >> %BUILDLOG%
If errorlevel 1 (
	echo Build %PROJECT% source failed: Error building source  >> %BUILDLOG%
	GOTO BuildFailed
)

echo Rebuilding project completed... >> %BUILDLOG%
echo Proceed to copy files... >> %BUILDLOG%

copy /B %PROJECTROOT%\bin\%buildtype%\Trafodion.Manager.Main.exe %BuildOutputFolder%\install\  /Y
copy /B %PROJECTROOT%\bin\%buildtype%\Trafodion.Manager.dll %BuildOutputFolder%\install\      /Y
copy /B %PROJECTROOT%\bin\%buildtype%\PluginLoader.dll %BuildOutputFolder%\install\    /Y
copy /B %PROJECTROOT%\bin\%buildtype%\Trafodion.Manager.UniversalWidget.dll %BuildOutputFolder%\install\   /Y
copy /B %PROJECTROOT%\bin\%buildtype%\plugins\Trafodion.Manager.MetricMiner.dll %BuildOutputFolder%\install\  /Y
copy /B %PROJECTROOT%\bin\%buildtype%\TenTec.Windows.iGridLib.iGrid.dll %BuildOutputFolder%\install\  /Y
copy /B %PROJECTROOT%\bin\%buildtype%\log4net.dll %BuildOutputFolder%\install\    /Y
copy /B %PROJECTROOT%\bin\%buildtype%\TrafodionSPJ.XSD %BuildOutputFolder%\install\    /Y
copy /B %PROJECTROOT%\build\log4net-1.2.11-LICENSE %BuildOutputFolder%\install\    /Y
copy /B %PROJECTROOT%\build\Newtonsoft.Json.dll %BuildOutputFolder%\install\    /Y
mkdir %BuildOutputFolder%\install\Reports
copy /B %PROJECTROOT%\Reports\Traf*.widget %BuildOutputFolder%\install\Reports\	/Y
copy /B %PROJECTROOT%\bin\%buildtype%\plugins\Trafodion.Manager.DatabaseArea.dll %BuildOutputFolder%\install\Plugins     /Y
copy /B %PROJECTROOT%\bin\%buildtype%\plugins\Trafodion.Manager.OverviewArea.dll %BuildOutputFolder%\install\Plugins    /Y
copy /B %PROJECTROOT%\bin\%buildtype%\plugins\Trafodion.Manager.ConnectivityArea.dll %BuildOutputFolder%\install\Plugins /Y
copy /B %PROJECTROOT%\bin\%buildtype%\plugins\Trafodion.Manager.WorkloadArea.dll %BuildOutputFolder%\install\Plugins    /Y
copy /B %PROJECTROOT%\bin\%buildtype%\plugins\Trafodion.Manager.UserManagement.dll %BuildOutputFolder%\install\Plugins /Y
copy /B %PROJECTROOT%\installShield\License.rtf %BuildOutputFolder%\install\  /Y

copy /B %ISProFileForMSI% %ISProFileForMSI%.bak  /Y
cscript %BUILDROOT%\replaceGUID.vbs %ISProFileForMSI% %BuildOutputFolder%

"%InstallShieldRoot%\System\ISCmdBld.exe" -p "%ISProFileForMSI%" -c COMP -b %BuildOutputFolder% -y %ProductVersion% -z "ProductName=%ProductName%"
If errorlevel 1 (
	echo Build %PROJECT% installshield failed: Error running installshield  >> %BUILDLOG%
	GOTO BuildFailed
)
move %ISProFileForMSI%.bak %ISProFileForMSI%

move "%BuildOutputFolder%\Product Configuration 1\Release 1\DiskImages\DISK1\%ProductName%.msi" "%BuildOutputFolder%\TrafodionDatabaseManagerInstaller.msi"

SET installerStatus=Success
echo Installer build completed... >> %BUILDLOG%

Echo ****************Standard package is successfully completed.****************


Echo Build Complete
Echo Build Complete >> %BUILDLOG%

Echo Perform cleanup... >> %BUILDLOG%
REM rd /s /q "%BuildOutputFolder%\Product Configuration 1"
rd /s /q %BuildOutputFolder%\install\setup
rd /s /q %BuildOutputFolder%\install\signed
Echo Cleanup Complete >> %BUILDLOG%

GOTO done

REM #######################
REM
REM Installer Build FAILED!
REM
REM ###############

:BuildFailed

Echo ""
ECHO Build failed with errors. Check the build.log file.

:Done

cd %BUILDROOT%
Echo Installer Build Status: %installerStatus%
pause
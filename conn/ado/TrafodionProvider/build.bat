@echo off

rem *********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1998-2015 Hewlett-Packard Development Company, L.P.
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
rem ********************************************************************/


REM 
call "%VS90COMNTOOLS%vsvars32.bat"

if "%1"=="help" goto help
if {%1}=={} goto recevier

REM varify sln file
if not exist "TrafodionProvider.sln" goto unexist
if not exist "build.proj" goto unexist
else goto build



:help
echo.
echo Hewlett-Packard Development Company, L.P.
echo use: 
echo 		bulder.bat [Location Path]
goto end



:recevier
echo.
echo Please input the installation path:
set /p install_path=
echo Path is [%install_path%], is it correct?
set /p par=Yes or No:
if "%par%" == "yes" goto build
if not "%par%"=="yes" goto recevier


:build
perl .\update_version.pl
mkdir %install_path%
echo.
echo ++ Build Started now ++
echo.
rem msbuild "TrafodionProvider.sln"
rem msbuild build.proj /p:outputpath=%install_path%
devenv TrafodionProvider.sln /Build debug /Project Deploy
echo.
echo ++ Build Successed ++
REM echo copy the .msi into target folder
REM need to be replaced with better way
REM VS donot provide a way to config vdproj
REM copy ..\..\..\..\..\..\Build\TrafodionAdo\*.msi %install_path%
goto end



:unexist
echo "TrafodionProvider.sln" doesnot exist, Please make sure .BAT file is located under the same path with .sln
goto end



:error
echo Error occured
goto end



:end
echo.
pause


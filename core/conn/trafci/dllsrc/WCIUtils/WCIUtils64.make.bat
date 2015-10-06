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

del CIUtils.obj
del CIUtils.exp
del CIUtils.lib
del WCIUtils64.dll

@echo on
echo.
echo  Adding Java2 SDK include paths to the MSVC C++ Compiler Include Path
echo  If its different on your PC, please edit the WNCIUtils.make.bat file.
echo.
set include=%include%;C:\Program Files\Java\jdk1.6.0_30\include;C:\Program Files\Java\jdk1.6.0_30\include\win32;


del org_trafodion_ci_WCIUtils.h

javah -jni -o org_trafodion_ci_WCIUtils.h -classpath C:\Temp\trafci-Build\classes org.trafodion.ci.WCIUtils

rem  === See ReadMe.txt for cl options

rem cl /O2 /GL /EHsc /W3 /Wp64 /TP -LD CIUtils.cpp


rem Building 64 bit dll without MSVC RT dependency by linking it statically(/MT)
cl /O2 /GL /EHsc /W3 /Wp64 /TP /MT -LD CIUtils.cpp
 

@echo off

rem ===  The next command will create a smaller file with a MSVC RT dependency
rem ===  and also create a manifest file to be used with mt. Adds /MD and /Zi to the 
rem ===  cl options.
rem ===
rem cl /O2 /GL /EHsc /MD /W3 /Wp64 /Zi /TP -LD CIUtils.cpp
rem

@echo on
rename CIUtils.dll WCIUtils64.dll
rem cp WCIUtils64.dll ../../src 

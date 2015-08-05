@echo off

rem *********************************************************************
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
rem ********************************************************************/


REM
REM	Module  : KrypyonBuild.bat
REM Purpose : This batch file is created since custom build commands has a bug while building through
REM           scripts. Krypton project is not build through MakeFile. 
REM	      IMPORTANT: This bug has been fixed and this batch file is no longer used.
REM           Any changes made to Krypton project settings should be tested with nmake. 
REM     
REM           No Paramaters needed for this batch file. You need to be at the krypton folder to run this 
REM			  batch file.

CALL .\IDLCNP.BAT %NSKTOOLS%\Kds\bin ..\Krypton odbcCommon _header
if ErrorLevel 1 goto error
CALL .\IDLCNP.BAT %NSKTOOLS%\Kds\bin ..\Krypton odbcsrvrcommon _header
if ErrorLevel 1 goto error
REM CALL .\IDLCNP.BAT %NSKTOOLS%\Kds\bin . odbc _clsv
CALL .\IDLCNP.BAT %NSKTOOLS%\Kds\bin ..\Krypton odbc _both
if ErrorLevel 1 goto error
REM    CALL .\IDLCNP.BAT %NSKTOOLS%\Kds\bin . odbccfg _clsv
CALL .\IDLCNP.BAT %NSKTOOLS%\Kds\bin ..\Krypton odbccfg _both
if ErrorLevel 1 goto error
CALL .\IDLCNP.BAT %NSKTOOLS%\Kds\bin ..\Krypton odbcas _clsv
if ErrorLevel 1 goto error
CALL .\IDLCNP.BAT %NSKTOOLS%\Kds\bin ..\Krypton ca _clsv
if ErrorLevel 1 goto error
CALL .\IDLCNP.BAT %NSKTOOLS%\Kds\bin ..\Krypton ceecfg _header
if ErrorLevel 1 goto error
CALL .\IDLCNP.BAT %NSKTOOLS%\Kds\bin ..\Krypton ceercv _header
if ErrorLevel 1 goto error
goto end
:error
false
:end
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

REM IDLCNP.BAT
REM This file will compile IDL and then run CNPGEN Program
REM to create Client and server header and stub files
REM it replaces IDLBOTH and IDLCMP.  
REM Usage is IDLCNP2 <Bin Directory> 
REM                 <IDL/PYF Directory> 
REM                 <IDLFilename excluding path and extension> 
REM					<_clsv | _header | _both>
REM  
 
echo IDL Compiling
%1\IDL -Ao,.\%3.pyf %2\%3.idl
if ErrorLevel 1 goto error
echo

if "%4" == "_clsv" goto clsv
if "%4" == "_both" goto both
if "%4" == "_header" goto header
goto error

:both
REM build client headers
echo CNP Client %3.h header
%1\CNPGEN -d -p -eh -o .\%3.h .\%3.pyf
if ErrorLevel 1 goto error
echo CNP Client %3.c Stub
%1\CNPGEN -ec -o .\%3.c .\%3.pyf
if ErrorLevel 1 goto error

:clsv
REM build client headers
echo CNP Client %3_cl.h header
%1\CNPGEN -d -p -ehc -o .\%3_cl.h .\%3.pyf
if ErrorLevel 1 goto error
echo CNP Client %3_cl.c Stub
%1\CNPGEN -ecc -o .\%3_cl.c .\%3.pyf
if ErrorLevel 1 goto error

REM build server headers
echo CNP Server %3_sv.h header
%1\CNPGEN -d -p -ehs -o .\%3_sv.h .\%3.pyf
if ErrorLevel 1 goto error
echo CNP Server %3_sv.c Stub
%1\CNPGEN -ecs -o .\%3_sv.c .\%3.pyf
if ErrorLevel 1 goto error
goto end

:header
echo CNP header %3.h header
%1\CNPGEN -d -p -eh -o .\%3.h .\%3.pyf
if ErrorLevel 1 goto error
goto end

:error
false
:end
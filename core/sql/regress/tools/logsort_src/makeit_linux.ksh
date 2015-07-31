#! /bin/sh
#######################################################################
# @@@ START COPYRIGHT @@@
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
# @@@ END COPYRIGHT @@@
#######################################################################
#
# USAGE: makeit
#        
#        This script builds the logsort.exe executable for Linux
#
compiler=`which g++`
cmd="$compiler -fPIC -w -g -O0 -g -Wno-unknown-pragmas -fshort-wchar -DNA_LINUX -DNGG -D_M_DG -D_NSKFS_ -D_FULL_BUILD -DARKFS_GENERATOR -D_DP2NT_ -D_LITTLE_ENDIAN_ -DARG_PRESENT_OMIT -DNSK_USE_MSGSYS_SHELLS -D_GLIBCXX_DEPRECATED -D_TNS_R_TARGET -D_X86_ -DWIN32_LEAN_AND_MEAN -D_WIN32_WINNT=0x402 -DNA_ITOA_NOT_SUPPORTED -DMPI_ -D_DEBUG"

for obj in line logsort row rowlst symtab tokstr
do
  rm -f $obj.is $obj.o

  echo "$cmd -c $obj.c -o $obj.o >> $obj.is 2>&1" | tee -a $obj.is

  $cmd -c $obj.c -o $obj.o >> $obj.is 2>&1

done

roo="logsort"

linkcmd="$cmd line.o logsort.o row.o rowlst.o symtab.o tokstr.o -o logsort.exe"
echo "$linkcmd >> $logsort.is 2>&1" | tee -a $logsort.is
$linkcmd >> $logsort.is 2>&1

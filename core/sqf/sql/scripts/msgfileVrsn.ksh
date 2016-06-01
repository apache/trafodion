#! /bin/sh
#
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
#
#######################################################################

# Emit msgcode 10, sqlstate 00000, with date and user info
# in {curly braces} for sqlci Env command to pick out.
# (sqlmsg/GetErrorMessage.h SQLERRORS_MSGFILE_VERSION_INFO is msgcode 10.)

arch=`uname | tr a-z A-Z |
      sed -e 's/WINDOWS_*//' -e 's/OS$//' -e 's/NONSTOP.*/NSK/'`
mach=${COMPUTERNAME:-`uname -n | tr a-z A-Z`}
mach=${mach+$mach/}
user=${USERNAME:-$LOGNAME}
vrsn="`date +'%Y-%m-%d %R'` $arch:$mach$user"
echo "10   00000 99999 UUUUUUUU UUUUU UUUUUUU Message file version: {$vrsn}."

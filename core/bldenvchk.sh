#!/bin/bash
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

# This script checks that build variables refer to real directories and that expected
# tools are on the current PATH.  Script returns zero only if both statements are true.
RETVAL=0
CUR_DIR=$(pwd)

if [[ "$SQ_VERBOSE" == "1" ]]; then
   echo "Checking that build variables reference existing directories and files..."
   echo CUR_DIR=$CUR_DIR
fi

if [[ "$TRAF_HOME" != "$CUR_DIR/sqf" ]]; then
   echo "*** Error: TRAF_HOME is set to: $TRAF_HOME"
   echo "           TRAF_HOME should be: $CUR_DIR/sqf"
   echo "           Be sure to source in sqf/sqenv.sh"
   RETVAL=1
fi

# These files should be on the PATH or fully qualified.
# In sqenvcom.sh, ANT, BISON and MAVEN can reference specific files.
VARFILELIST="ANT AR FLEX CXX BISON MAVEN"
for AVAR in $VARFILELIST; do
   AVAL="$(eval "echo \$$AVAR")"
   AVALUE="$(which $AVAL 2> /dev/null)"
   RESULT=$?
   if [[ "$SQ_VERBOSE" == "1" ]]; then
      printf '%s =\t%s\n' $AVAR $AVALUE
   fi
   if [[ $RESULT != 0 ]]; then
      echo "*** Error: $AVAR file not found in PATH or file not fully qualified."
      RETVAL=1
   fi
done

# These directories should exist.
VARDIRLIST="JAVA_HOME MPI_TMPDIR"
VARDIRLIST="$VARDIRLIST LLVM UDIS86 ICU MPICH_ROOT ZOOKEEPER_DIR PROTOBUFS"
VARDIRLIST="$VARDIRLIST THRIFT_LIB_DIR THRIFT_INC_DIR"
# QT_TOOLKIT is optional; if it is not set correctly then the SQL Compiler Debugger should
# not build.
if [ ! -d "${QT_TOOLKIT}" ]; then
   echo "*** Warning: QT_TOOLKIT does not point to an existing directory."
   echo "*** Warning: SQL Compiler Debugger will not be built."
fi

for AVAR in $VARDIRLIST; do
   AVALUE="$(eval "echo \$$AVAR")"
   if [[ "$SQ_VERBOSE" == "1" ]]; then
      printf '%s =\t%s\n' $AVAR $AVALUE
   fi
   if [[ ! -d $AVALUE ]]; then
      echo "*** Error: $AVAR directory not found: $AVALUE"
      RETVAL=1
   fi
done

exit $RETVAL

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

if [[ "$MY_SQROOT" != "$CUR_DIR/sqf" ]]; then
   echo "*** Error: MY_SQROOT is set to: $MY_SQROOT"
   echo "           MY_SQROOT should be: $CUR_DIR/sqf"
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
      echo "*** Error: $AVAR file not found in PATH."
      RETVAL=1
   fi
done

# These directories should exist.
VARDIRLIST="JAVA_HOME PERL5LIB MPI_TMPDIR"
VARDIRLIST="$VARDIRLIST LLVM UDIS86 ICU MPICH_ROOT ZOOKEEPER_DIR THRIFT_LIB_DIR"
VARDIRLIST="$VARDIRLIST THRIFT_INC_DIR PROTOBUFS"
VARDIRLIST="$VARDIRLIST HADOOP_INC_DIR HADOOP_LIB_DIR"
# QT_TOOLKIT is optional; if it is not set then the SQL Compiler Debugger should
# not build.  Check the value of QT_TOOLKIT variable only if set, including when
# set to blanks or empty string.
if [ ! -z ${QT_TOOLKIT+x} ]; then
   VARDIRLIST="$VARDIRLIST QT_TOOLKIT"
else
   if [[ "$SQ_VERBOSE" == "1" ]]; then
      echo "QT_TOOLKIT is not set.  SQL Compiler Debugger will not be built."
   fi
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

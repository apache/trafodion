#!/bin/sh
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

# This script can be invoked from SQL/MX regression tests to build a
# DLL. The first argument is the simple name of a C/C++ source file,
# without any leading directory names. The file is assumed to exist in
# $REGRTSTDIR. The output file is written to $REGRRUNDIR.

#  Usage: dll-compile <source-file> [<compiler-options> <linker-options>]

if [ "$REGRTSTDIR" = "" ]; then
  echo "*** ERROR: \$REGRTSTDIR not defined"
  exit 1
fi

SRC=$1
shift

  NSK=0
  LINUX=1

if [ ${SRC%.cpp} != $SRC ]; then
  # a C++ file
  CC=g++
  BASE=$(basename $SRC .cpp)
elif [ ${SRC%.c} != $SRC ]; then
  CC=gcc
  BASE=$(basename $SRC .c)
else
  echo "Expecting a file argument ending in .c or .cpp"
  exit 1
fi
  

CC_OPTS=
LD=
LD_OPTS=
TARGET=

    # give preference to tools on /opt/home, use PATH on clusters
    if [ -x /opt/home/tools/gcc-4.4.6/bin/${CC} ]; then
      CC=/opt/home/tools/gcc-4.4.6/bin/${CC}
    else
      CC=`which $CC`
    fi
  CC_OPTS="-g "
  CC_OPTS="$CC_OPTS -I$TRAF_HOME/sql/sqludr"
  CC_OPTS="$CC_OPTS -I$TRAF_HOME/export/include/sql  -I$TRAF_HOME/export/include/nsk -I${JAVA_HOME}/include -I${JAVA_HOME}/include/linux"
  CC_OPTS="$CC_OPTS -w -O0 -Wno-unknown-pragmas -fPIC -fshort-wchar -c -o $BASE.o $1"
  TARGET=$BASE.dll
  LD=$CC
  LD_OPTS=" -w -O0 -Wno-unknown-pragmas -fshort-wchar"
  LD_OPTS="$LD_OPTS -shared -rdynamic -o $TARGET -lc -lhdfs -ljvm -L$TRAF_HOME/export/lib${SQ_MBTYPE} -ltdm_sqlcli -L${JAVA_HOME}/jre/lib/amd64/server $2 $BASE.o"

LONGLINE=\
------------------------------------------------------------------------------
echo $LONGLINE

echo -- Building DLL $TARGET from $SRC
echo -- Executing: $CC $CC_OPTS \$REGRTSTDIR/$SRC
$CC $CC_OPTS $REGRTSTDIR/$SRC
STATUS=$?
echo -- $CC returned $STATUS
echo $LONGLINE

if [ $NSK -ne 0 -a $STATUS -eq 0 -o $LINUX -ne 0 -a $STATUS -eq 0 ]; then
  echo -- Linking target $TARGET
  echo -- Executing: $LD $LD_OPTS
  $LD $LD_OPTS
  STATUS=$?
  echo -- $LD returned $STATUS
  echo $LONGLINE
fi

exit $STATUS

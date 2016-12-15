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
#
# This script can be invoked from SQL/MX regression tests to compile
# Java source files. Arguments are the simple names of a Java source
# file without leading directory names. The files are assumed to exist
# in $REGRTSTDIR. Output files will have a .class file extension
# and will be written to $REGRRUNDIR.
#
#   Usage: java-compile <source file>...
#

LONGLINE=\
------------------------------------------------------------------------------

echo $LONGLINE
echo -- Compiling Java source files: $*

SrcList=
BaseNameList=
while [ -n "$1" ]; do
    BaseNameList="$BaseNameList \$REGRTSTDIR/$1"
    SrcList="$SrcList $REGRTSTDIR/$1"
    shift
done

case `uname` in
  Linux)
    if [ "$JAVA_HOME" = "" ]; then
    echo -- \$JAVA_HOME is not set
    echo $LONGLINE

    exit 1
    else
     #record which javac we are using
     echo Using $JAVA_HOME/bin/javac >&2
    fi

    javac=$JAVA_HOME/bin/javac
  ;;
  *)
    if [ "$javac" = "" ]; then
      javac=javac
    fi
  ;;
esac

if [ "$rundir" = "" ]; then
  rundir=$(cd ..; pwd)
fi

if [ "$scriptsdir" = "" ]; then
  scriptsdir=$(cd ..; pwd)
fi

if [ "$REGRTSTDIR" = "" ]; then
  REGRTSTDIR=$(pwd)
fi

if [ "$REGRRUNDIR" = "" ]; then
  REGRRUNDIR=$(pwd)
fi

export CLASSPATH=$CLASSPATH:$TRAF_HOME/export/lib/spjsql.jar

# Consider using -nowarn to turn off warnings
echo -- Executing: \$javac -d \$REGRRUNDIR $BaseNameList
"$javac" -d "$REGRRUNDIR" $SrcList
STATUS=$?

echo -- \$javac returned $STATUS
echo $LONGLINE

exit $STATUS

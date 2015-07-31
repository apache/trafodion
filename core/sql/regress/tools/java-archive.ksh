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
# This script can be invoked from SQL/MX regression tests to create
# Java archive (jar) files. Arguments are the target jar name followed
# by a set of class file names.
#
#   Usage: java-archive <jar file> <class file>...
#

LONGLINE=\
------------------------------------------------------------------------------

echo $LONGLINE

JARFILE="$1"
shift

case `uname` in
  Linux)
    if [ "$JAVA_HOME" = "" ]; then

      echo -- \$JAVA_HOME is not set
      echo $LONGLINE

      exit 1

    else
      # Record which jar utility we used
      echo "Using $JAVA_HOME/bin/jar to archive" >&2
    fi

    jar=$JAVA_HOME/bin/jar
  ;;
  *)
    if [ "$jar" = "" ] ; then
      jar=jar
    fi
  ;;
esac



echo -- Archiving Java class files:
for F in "$@"
do
    echo "--    $F"
done
echo -- Archive will be written to: $JARFILE

echo -- Executing: \$jar cMf $JARFILE "$@"
"$jar" cMf $JARFILE "$@"
STATUS=$?

echo -- \$jar returned $STATUS
echo $LONGLINE

exit $STATUS

#!/bin/sh
#/**
# @@@ START COPYRIGHT @@@
#
#Licensed to the Apache Software Foundation (ASF) under one
#or more contributor license agreements.  See the NOTICE file
#distributed with this work for additional information
#regarding copyright ownership.  The ASF licenses this file
#to you under the Apache License, Version 2.0 (the
#"License"); you may not use this file except in compliance
#with the License.  You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
#Unless required by applicable law or agreed to in writing,
#software distributed under the License is distributed on an
#"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#KIND, either express or implied.  See the License for the
#specific language governing permissions and limitations
#under the License.
#
# @@@ END COPYRIGHT @@@
# */
# This is used for starting multiple servers on the same machine.
# run it from 'bin/dcs'

bin=`dirname "${BASH_SOURCE-$0}"`
bin=`cd "$bin" >/dev/null && pwd`

if [ $# -lt 3 ]; then
  S=`basename "${BASH_SOURCE-$0}"`
  echo "Usage: $S [--config <conf-dir>] [start|stop] offset(s)"
  echo ""
  echo "    e.g. $S start 1 2"
  exit
fi

#check to see if the conf dir is given as an optional argument
while [ $# -gt 1 ]
do
  if [ "--config" = "$1" ]
  then
    shift
    confdir="--config $1"
    shift
    DCS_CONF_DIR=$confdir
    break
  fi
done

export DCS_SERVER_OPTS=" "

run_server () {
  DN=$2
  export DCS_IDENT_STRING="$USER-$DN"
  "$bin"/dcs-daemon.sh $DCS_CONF_DIR $1 server $DN
}

cmd=$1
shift;

for i in $*
do
  run_server $cmd $i
done

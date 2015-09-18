#!/usr/bin/env bash
#/**
* @@@ START COPYRIGHT @@@
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
* @@@ END COPYRIGHT @@@
# */
# 
# Run a shell command on all server hosts.
#
# Environment Variables
#
#   WMS_SERVERS    File naming remote hosts.
#
# Modelled after $HADOOP_HOME/bin/slaves.sh.

usage="Usage: servers [--config <wms-confdir>] command..."

# if no args specified, show usage
if [ $# -le 0 ]; then
  echo $usage
  exit 1
fi

bin=`dirname "${BASH_SOURCE-$0}"`
bin=`cd "$bin">/dev/null; pwd`

. "$bin"/wms-config.sh

# If the servers file is specified in the command line,
# then it takes precedence over the definition in 
# wms-env.sh. Save it here.
HOSTLIST=$WMS_SERVERS

if [ "$HOSTLIST" = "" ]; then
  if [ "$WMS_SERVERS" = "" ]; then
    export HOSTLIST="${WMS_CONF_DIR}/servers"
  else
    export HOSTLIST="${WMS_SERVERS}"
  fi
fi

instance=1

for server in `cat "$HOSTLIST"`; do
  if [ "$server" == "localhost" ] || [ "$server" == "$HOSTNAME" ] ; then
    echo "local server"
    eval $"${@// /\\ } $instance" 2>&1 | sed "s/^/$server: /" &
  else 
    echo "remote server"
    if ${WMS_SLAVE_PARALLEL:-true}; then 
      ssh $WMS_SSH_OPTS $server $"${@// /\\ } $instance" \
        2>&1 | sed "s/^/$server: /" &
    else # run each command serially 
      ssh $WMS_SSH_OPTS $server $"${@// /\\ } $instance" \
        2>&1 | sed "s/^/$server: /" &
    fi
  fi
  
  let instance++ 
  
  if [ "$WMS_SLAVE_SLEEP" != "" ]; then
    sleep $WMS_SLAVE_SLEEP
  fi
done

wait

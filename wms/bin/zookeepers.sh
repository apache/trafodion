#!/usr/bin/env bash
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
#
# Run a shell command on all zookeeper hosts.
#
# Environment Variables
#
#   WMS_CONF_DIR  Alternate WMS conf dir. Default is ${TRAF_CONF}/wms.
#   WMS_SLAVE_SLEEP Seconds to sleep between spawning remote commands.
#   WMS_SSH_OPTS Options passed to ssh when running remote commands.
#

usage="Usage: zookeepers [--config <wms-confdir>] command..."

# if no args specified, show usage
if [ $# -le 0 ]; then
  echo $usage
  exit 1
fi

bin=`dirname "${BASH_SOURCE-$0}"`
bin=`cd "$bin">/dev/null; pwd`

. "$bin"/wms-config.sh

if [ "$WMS_MANAGES_ZK" = "" ]; then
  WMS_MANAGES_ZK=true
fi

if [ "$WMS_MANAGES_ZK" = "true" ]; then
  hosts=`"$bin"/wms org.trafodion.wms.zookeeper.ZKServerTool | grep '^ZK host:' | sed 's,^ZK host:,,'`
  cmd=$"${@// /\\ }"
  
  instance=1
  for zookeeper in $hosts; do
    if [ "$zookeeper" == "localhost" ] || [ "$zookeeper" == "$HOSTNAME" ] ; then
      echo "local server"
      eval $cmd $instance 2>&1 | sed "s/^/$zookeeper: /" &
    else 
      echo "remote server"
      ssh $WMS_SSH_OPTS $zookeeper $cmd $instance 2>&1 | sed "s/^/$zookeeper: /" &
    fi
    
    let instance++ 
    
    if [ "$WMS_SLAVE_SLEEP" != "" ]; then
      sleep $WMS_SLAVE_SLEEP
    fi
   
  done
fi

wait

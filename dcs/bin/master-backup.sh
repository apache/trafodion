#!/usr/bin/env bash
#/**
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
#*/
# Run a shell command on all backup master hosts.
#
# Environment Variables
#
#   DCS_BACKUP_MASTERS File naming remote hosts.
#     Default is ${DCS_CONF_DIR}/backup-masters
#   DCS_CONF_DIR  Alternate Dcs conf dir. Default is ${DCS_HOME}/conf.
#   DCS_SLAVE_SLEEP Seconds to sleep between spawning remote commands.
#   DCS_SSH_OPTS Options passed to ssh when running remote commands.
#
# Modelled after $HADOOP_HOME/bin/slaves.sh.

usage="Usage: $0 [--config <dcs-confdir>] command..."

# if no args specified, show usage
if [ $# -le 0 ]; then
  echo $usage
  exit 1
fi

bin=`dirname "${BASH_SOURCE-$0}"`
bin=`cd "$bin">/dev/null; pwd`

. "$bin"/dcs-config.sh

# If the master backup file is specified in the command line,
# then it takes precedence over the definition in 
# dcs-env.sh. Save it here.
HOSTLIST=$DCS_BACKUP_MASTERS

if [ "$HOSTLIST" = "" ]; then
  if [ "$DCS_BACKUP_MASTERS" = "" ]; then
    export HOSTLIST="${DCS_CONF_DIR}/backup-masters"
  else
    export HOSTLIST="${DCS_BACKUP_MASTERS}"
  fi
fi


args=${@// /\\ }
args=${args/master-backup/master}

instance=2

if [ -f $HOSTLIST ]; then
  while read master
  do
    if [ "$master" == "localhost" ] || [ "$master" == "$HOSTNAME" ] ; then
      eval $"$args $instance" 2>&1 | sed "s/^/$master: /" &
    else
      if ${DCS_SLAVE_PARALLEL:-true}; then
        ssh -q -n $DCS_SSH_OPTS $master $"$args $instance"\
          2>&1 | sed "s/^/$master: /" &
      else # run each command serially
        ssh -q -n $DCS_SSH_OPTS $master $"$args $instance" \
          2>&1 | sed "s/^/$master: /" &
      fi
    fi
  
    if [ "$DCS_SLAVE_SLEEP" != "" ]; then
      sleep $DCS_SLAVE_SLEEP
    fi
  
    let instance++

  done < "$HOSTLIST"
fi

wait

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
# */

# Start dcs daemons.
usage="Usage: start-dcs.sh"

bin=`dirname "${BASH_SOURCE-$0}"`
bin=`cd "$bin">/dev/null; pwd`

. "$bin"/dcs-config.sh

# start dcs daemons
errCode=$?
if [ $errCode -ne 0 ]
then
  exit $errCode
fi

"$bin"/dcs-daemons.sh --config "${DCS_CONF_DIR}" start zookeeper

master=`$bin/dcs --config "${DCS_CONF_DIR}" org.trafodion.dcs.zookeeper.ZkUtil /$USER/dcs/master`
errCode=$?
if [ $errCode -ne 0 ]
then
  exit $errCode
fi

if [ -z "$master" ] ; then
  if [ ! -z "${DCS_PRIMARY_MASTER}" ] && [ -s ${DCS_PRIMARY_MASTER} ] ; then
    master_node=`cat ${DCS_PRIMARY_MASTER}| egrep -v '^#|^$'`
    if [ ! -z "$master_node" ] ; then
      master=`echo $master_node | awk '{print $1}'`
    fi
  fi
fi

if [ "$master" == "" ] || [ "$master" == "localhost" ] || [ "$master" == "$(hostname -f)" ] ; then
  "$bin"/dcs-daemon.sh --config "${DCS_CONF_DIR}" start master 
else
  remote_cmd="cd ${DCS_HOME}; $bin/dcs-daemon.sh --config ${DCS_CONF_DIR} start master"
  ssh -q -n $DCS_SSH_OPTS $master $remote_cmd 2>&1 | sed "s/^/$master: /"
fi

"$bin"/dcs-daemons.sh --config "${DCS_CONF_DIR}" --hosts "${DCS_SERVERS}" start server
"$bin"/dcs-daemons.sh --config "${DCS_CONF_DIR}" --hosts "${DCS_BACKUP_MASTERS}" start master-backup

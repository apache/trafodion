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

# Stop dcs daemons.

bin=`dirname "${BASH_SOURCE-$0}"`
bin=`cd "$bin">/dev/null; pwd`

. "$bin"/dcs-config.sh

# start dcs daemons
errCode=$?
if [ $errCode -ne 0 ]
then
  exit $errCode
fi

"$bin"/dcs-daemons.sh --config "${DCS_CONF_DIR}" --hosts "${DCS_MASTERS}" stop master-backup

master=`$bin/dcs --config "${DCS_CONF_DIR}" org.trafodion.dcs.zookeeper.ZkUtil /$USER/dcs/master|tail -n 1`
errCode=$?
zkerror=`echo $master| grep -i error`
if ( [ ${errCode} -ne 0 ] || [ -n "${zkerror}" ] );
then
  echo "Zookeeper exception occurred, killing all DcsMaster and DcsServers..."
  "$bin"/dcs-daemon.sh --config "${DCS_CONF_DIR}" stop master 
  "$bin"/dcs-daemons.sh --config "${DCS_CONF_DIR}" --hosts "${DCS_SERVERS}" stop server 
  exit $errCode
fi

    activeMaster=$($DCS_INSTALL_DIR/bin/getActiveMaster.sh)

    remote_cmd="cd ${DCS_HOME}; $bin/dcs-daemon.sh --config ${DCS_CONF_DIR} stop master"
    L_PDSH="ssh -q -n $DCS_SSH_OPTS"

    if [[ ! -z $activeMaster ]]; then
        ${L_PDSH} $activeMaster $remote_cmd 2>&1 | sed "s/^/$activeMaster: /"
    else
        ${L_PDSH} $master $remote_cmd 2>&1 | sed "s/^/$master: /"
    fi

"$bin"/dcs-daemons.sh --config "${DCS_CONF_DIR}" --hosts "${DCS_SERVERS}" stop server 
"$bin"/dcs-daemons.sh --config "${DCS_CONF_DIR}" stop zookeeper

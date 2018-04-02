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

# Get the activeMaster hostname

setup_sqpdsh
A_PDSH=$SQPDSHA

function getActiveMaster {

   tmpdcsconfig=`mktemp -t`
   if [[ $? != 0 ]]; then
     echo "Error while getting a temporary file for tmpdcsconfig. Exiting."
     exit 3
   fi

   python $DCS_INSTALL_DIR/bin/scripts/parse_dcs_site.py > $tmpdcsconfig
   masterport=`cat $tmpdcsconfig |grep "^dcs.master.port:"| cut -f2 -d":"`
  
   if [[ ! -z $CLUSTERNAME ]]; then
    if [[ $ENABLE_HA == "true" ]]; then

      floatip_interface=`cat $tmpdcsconfig |grep "^dcs.master.floating.ip.external.interface:"| cut -f2 -d":"`
      keepalived=`cat $tmpdcsconfig |grep "^dcs.master.keepalived:"| cut -f2 -d":"`

      if [[ $floatip_interface == "default" ]]; then
           floatip_interface=`/sbin/route |grep "0.0.0.0" |awk '{print $8}'`
      fi

      if ! grep -q ^ec2 /sys/hypervisor/uuid 2>/dev/null ; then
          # Non-AWS system
          interface_to_use=$floatip_interface":"$masterport
      else
          interface_to_use=$floatip_interface
      fi

      if [[ ${keepalived} != "true" ]]; then
           activeMaster=`$A_PDSH /sbin/ip addr show |grep $interface_to_use$ |cut -d':' -f1`
      else
           activeMaster=`$A_PDSH /sbin/ifconfig |grep $interface_to_use |cut -d':' -f1`
      fi
    else
      tmpnetstat=`$A_PDSH /bin/netstat -antp 2>/dev/null |grep -w :$masterport`
      tmpcurrentMaster=`echo $tmpnetstat |cut -f1 -d":" |awk '{print $1}'`
      if [[ ${tmpcurrentMaster} == "tcp" ]]; then
         activeMaster=`hostname -f`
      else
         activeMaster=$tmpcurrentMaster
      fi
    fi
   else
    activeMaster=localhost
   fi

   rm -f $tmpdcsconfig
   echo $activeMaster
}

getActiveMaster

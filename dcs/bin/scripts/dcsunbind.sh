#!/bin/bash
#
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
#
#

function check_node {
    for myinterface in `$SQ_PDSH -N -w $1 /sbin/ip link show|awk -F': ' '/^[0-9]+:.*/ {print $2;}'`; do
	ip_output=$($SQ_PDSH -N -w $1 /sbin/ip addr show $myinterface)
	if [ $gv_externalip_set -eq 1 -a $external_only -eq 1 ]; then
            myifport=`echo "$ip_output" | grep $gv_float_external_ip`
	    status=$?
	    if [ $status -eq 0 ]; then
	       tempinterface=`echo $gv_float_external_interface:$gv_port`
	       # check if another interface is bound to this virtual ip address
	       echo "$myifport" | grep "$tempinterface"  > /dev/null
	       if [ $? -eq 1 -o "$1" != "$gv_myhostname" ]; then
                   unbindip=`echo "$myifport" | awk '{print $2}'`
		   unbindlb=`echo "$myifport"|awk '{print $NF}'`
		   echo "External ip $gv_float_external_ip is in use on node $1 bound to interface $myinterface($unbindlb) - unbind..."
		   $SQ_PDSH -S -w $1 sudo /sbin/ip addr del $unbindip dev $myinterface label $unbindlb
                   status=$?
		   if [ $status -ne 0 ]; then
		      echo "Failed to unbind - status is $status"
		      exit -1 
                   else
                      echo "Unbind successful"
                   fi
	       fi # endif node+name match
	    fi # endif looking for external ip
        fi #endif checking external ip is set or not
    done
}

function Check_VirtualIP_InUse_And_Unbind {
    echo "check all nodes to see if external virtual ip address is in use and unbind if necessary"
    mynode=""
    externalNodes=`$SQ_PDSH $MY_NODES /sbin/ip addr show | grep $gv_float_external_ip | awk -F' ' '/^.+:[[:space:]]+.*/ {print $1;}' | cut -d':' -f1 | sed '/^$/d'`
    if [ ! -z "$externalNodes" ]; then
	external_only=1
	internal_only=0
	for mynode in $externalNodes; do
	   check_node $mynode
	done
    fi
}

#Main program

if [[ $ENABLE_HA == "false" ]]; then
 exit 0
fi

gv_float_internal_ip=`python $DCS_INSTALL_DIR/bin/scripts/parse_dcs_site.py|cut -d$'\n' -f2`
gv_float_external_ip=`python $DCS_INSTALL_DIR/bin/scripts/parse_dcs_site.py|cut -d$'\n' -f2`
gv_float_interface=`python $DCS_INSTALL_DIR/bin/scripts/parse_dcs_site.py|cut -d$'\n' -f1`
gv_port=`python $DCS_INSTALL_DIR/bin/scripts/parse_dcs_site.py|cut -d$'\n' -f3`
if [[ -z $gv_port ]]; then
   gv_port=23400
fi
gv_externalip_set=1
gv_internalip_set=1

if [[ $AWS_CLOUD == "true" ]]; then
   awscmd="/usr/local/bin/aws ec2 --output text "
   #Get the network interface
   NETWORKINTERFACE=`$awscmd describe-network-interfaces| grep -i networkinterfaces| grep -i $gv_float_internal_ip|cut -f5`

   # Get the attachment id for the network interface
   ATTACH_ID=`$awscmd describe-network-interfaces --network-interface-ids $NETWORKINTERFACE |grep -i attachment |cut -f3`

   echo "Detaching attachment Id:" $ATTACH_ID
   if [ ! -z "$ATTACH_ID" ]; then
      $awscmd detach-network-interface --attachment-id $ATTACH_ID
      echo "Detached interface :" $NETWORKINTERFACE
   fi
else
   Check_VirtualIP_InUse_And_Unbind
fi
exit 0

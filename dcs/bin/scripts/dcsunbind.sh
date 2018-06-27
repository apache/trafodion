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
    for myinterface in `$L_PDSH -w $1 /sbin/ip link show|cut -d: -f2- | cut -c2- | awk -F': ' '/^[0-9]+:.*/ {print $2;}'`; do
	ip_output=$($L_PDSH -w $1 /sbin/ip addr show $myinterface | cut -d: -f2- | cut -c2-)
	if [ $gv_externalip_set -eq 1 -a $external_only -eq 1 ]; then
            myifport=`echo "$ip_output" | grep -w $gv_float_external_ip`
	    status=$?
	    if [ $status -eq 0 ]; then
	       tempinterface=`echo $gv_float_interface:$gv_port`
	       # check if another interface is bound to this virtual ip address
	       echo "$myifport" | grep "$tempinterface"  > /dev/null
	       if [ $? -eq 1 -o "$1" != "$gv_myhostname" ]; then
                   unbindip=`echo "$myifport" | awk '{print $2}'`
		   unbindlb=`echo "$myifport"|awk '{print $NF}'`
		   echo "Virtual ip $gv_float_external_ip is in use on node $1 bound to interface $myinterface($unbindlb) - unbinding..."
		   $L_PDSH -w $1 sudo /sbin/ip addr del $unbindip dev $myinterface
                   status=$?
		   if [ $status -ne 0 ]; then
		      echo "Failed to unbind - status is $status"
		      exit -1 
                   fi
	       fi # endif node+name match
	    fi # endif looking for external ip
        fi #endif checking external ip is set or not
    done
}

function Check_VirtualIP_InUse_And_Unbind {
    mynode=""
    externalNodes=`$L_PDSH $MY_NODES /sbin/ip addr show | grep -w $gv_float_external_ip | awk -F' ' '/^.+:[[:space:]]+.*/ {print $1;}' | cut -d':' -f1 | sed '/^$/d'`
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
dcsunbindtmp=`mktemp -t`
python $DCS_INSTALL_DIR/bin/scripts/parse_dcs_site.py > $dcsunbindtmp
gv_float_internal_ip=`cat $dcsunbindtmp |grep "^dcs.master.floating.ip.external.ip.address:"| cut -f2 -d":"`
gv_float_external_ip=$gv_float_internal_ip
gv_float_interface=`cat $dcsunbindtmp |grep "^dcs.master.floating.ip.external.interface:"| cut -f2 -d":"`
device_index_to_use=`echo $gv_float_interface | sed 's/[^0-9]//g'`
gv_port=`cat $dcsunbindtmp |grep "^dcs.master.port:"| cut -f2 -d":"`
if [[ -z $gv_port ]]; then
   gv_port=23400
fi
gv_externalip_set=1
gv_internalip_set=1

if grep -q ^ec2 /sys/hypervisor/uuid 2>/dev/null ; then
   # AWS system
   awscmd="/usr/bin/aws ec2 --output text "

   #Get the network interface
   NETWORKINTERFACE=`$awscmd describe-network-interfaces --query 'NetworkInterfaces[*].[NetworkInterfaceId,PrivateIpAddress]' |grep -i -w $gv_float_internal_ip |cut -f1`

   # Get the attachment id for the network interface
   ATTACH_ID=`$awscmd describe-network-interfaces --network-interface-ids $NETWORKINTERFACE --filters Name=attachment.device-index,Values=$device_index_to_use --query 'NetworkInterfaces[*].[Attachment.AttachmentId]'`

   echo "Detaching attachment Id:" $ATTACH_ID
   if [ ! -z "$ATTACH_ID" ]; then
      $awscmd detach-network-interface --attachment-id $ATTACH_ID
      echo "Detached interface :" $NETWORKINTERFACE
   fi
else
   # non-AWS
   L_PDSH="pdsh -S"

   Check_VirtualIP_InUse_And_Unbind
fi
rm -f $dcsunbindtmp
exit 0

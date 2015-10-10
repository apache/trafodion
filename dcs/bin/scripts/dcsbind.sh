#!/bin/bash
#/**
#* @@@ START COPYRIGHT @@@
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
#* @@@ END COPYRIGHT @@@
# */
#

function Usage {
    dcsEcho
    dcsEcho "Usage: $0 -i <nn>  -a <xxx.xxx.xxx.xxx> -p <nn> { -h }"
    dcsEcho
    dcsEcho "-i <nn>  External interface e.g., eth1"    
    dcsEcho "-a <nn>  External IP address"   
    dcsEcho "-p <nn>  Port number"  
    dcsEcho "-h       Help"
    dcsEcho
    exit $gv_error;
}

function HelpUsage {
    echo "Usage: dcsbind -i <nn> -a <xxx.xxx.xxx.xxx> -p <nn> { -h }"
    echo
    echo "-i <nn>  External interface e.g., eth1"    
    echo "-a <nn>  External IP address"    
    echo "-p <nn>  Port number" 
    echo "-h       Help"
    echo
    exit $gv_warn;
}

function GetOpts {
    
    while getopts ":i:a:p:b:h" arg
    do
       case $arg in
          
       i) gv_float_external_interface=${OPTARG}
          ;;

       a) gv_float_external_ip=${OPTARG}
          ;;

       p) gv_port=${OPTARG}
          ;;

       *) HelpUsage
          ;;

       esac
    done

}

function dcsEcho {
   echo "$1" "$2" 
}

#
# Gets floating ip addresses that have been configured for the dcs process
#
function GetFloatingIpAdrress {

if [ -z $gv_float_external_ip ]; then
   gv_externalip_set=0
   dcsEcho "no floating external ip address has been set"
else
   dcsEcho "floating external interface  is: " "$gv_float_external_interface"
fi

if  [ -z $gv_float_external_ip ] ; then
    dcsEcho "No floating external ip address is set"
   exit $gv_ok
fi

}

function check_node {
	 dcsEcho "checking node $1"
    for myinterface in `pdsh -N -w $1 /sbin/ip link show|awk -F': ' '/^[0-9]+:.*/ {print $2;}'`; do
		  ip_output=$(pdsh -N -w $1 /sbin/ip addr show $myinterface)
		  if [ $gv_externalip_set -eq 1 -a $external_only -eq 1 ]; then
				myifport=`echo "$ip_output" | grep $gv_float_external_ip`
				status=$?
				if [ $status -eq 0 ]; then
					 tempinterface=`echo $gv_float_external_interface:$gv_port`
					 
	            # check if another interface is bound to this virtual ip address
					 echo "$myifport" | grep "$tempinterface"  > /dev/null
					 if [ $? -eq 1 -o "$1" != "$gv_myhostname" ]; then
						  unbindip=`echo "$myifport"|awk '{print $2}'`
						  unbindlb=`echo "$myifport"|awk '{print $NF}'`
						  dcsEcho "external ip $gv_float_external_ip is already in use on node $1 bound to interface $myinterface($unbindlb) - unbind..."
						  dcsEcho "pdsh -S -w $1 sudo /sbin/ip addr del $unbindip dev $myinterface label $unbindlb"
						  pdsh -S -w $1 sudo /sbin/ip addr del $unbindip dev $myinterface label $unbindlb

						  status=$?
						  if [ $status -ne 0 ]; then
								dcsEcho "failed - status is $status"
								exit $gv_error
						  fi
					 else
						  dcsEcho "external ip $gv_float_external_ip is already bound to $myinterface on node $1 - skip unbind"
					 fi # endif node+name match
				fi # endif looking for external ip
		  fi #endif checking external ip is set or not
    done
}

function Check_VirtualIP_InUse_Unbind {
     dcsEcho "check all nodes to see if external virtual ip address is in use and unbind if necessary"
	 mynode=""
	 allMyNodes="$MY_NODES"
	 
	#check if external ip is in use
    dcsEcho "check all nodes $allMyNodes"
    externalNodes=`pdsh $allMyNodes /sbin/ip addr show | grep $gv_float_external_ip | awk -F' ' '/^.+:[[:space:]]+.*/ {print $1;}' | cut -d':' -f1 | sed '/^$/d'`
    if [ ! -z "$externalNodes" ]; then
		  dcsEcho "find possible node `echo $externalNodes`"
		  external_only=1
		  internal_only=0
		  for mynode in $externalNodes; do
				check_node $mynode
		  done
    fi

    dcsEcho "checks completed"
}

function BindFloatIp {

# bind the floating external ip
if [ $gv_externalip_set -eq 1 ]; then
   dcsEcho "Binding external ip $gv_float_external_ip on node $gv_myhostname"
   bcast=`/sbin/ip addr show $gv_float_external_interface | grep "inet .*$gv_float_external_interface\$" | awk '{print $4}'`
   mask=`/sbin/ip addr show $gv_float_external_interface | grep "inet .*$gv_float_external_interface\$" | awk '{print $2}' | cut -d'/' -f2`
	
   /sbin/ip addr show| grep 'inet [^[:space:]]\+ '| awk '{print $2}'| sed -e 's/\/.*//'|grep $gv_float_external_ip > /dev/null
   status=$?
   if [ $status -eq 0 ]; then
      dcsEcho "external ip is already bound on node $gv_myhostname - skip bind step"
   else
      dcsEcho "sudo /sbin/ip addr add $gv_float_external_ip/$mask broadcast $bcast dev $gv_float_external_interface label $gv_float_external_interface:$gv_port"
      sudo /sbin/ip addr add $gv_float_external_ip/$mask broadcast $bcast dev $gv_float_external_interface label $gv_float_external_interface:$gv_port
      status=$?
      if [ $status -ne 0 ]; then
       dcsEcho "failed - status is $status"
       exit $gv_error
      fi
      dcsEcho "sudo /sbin/arping -U -w 30 -I $gv_float_external_interface $gv_float_external_ip"
      sudo /sbin/arping -U -w 3 -c 3 -I $gv_float_external_interface $gv_float_external_ip
      if [ $status -ne 0 ]; then
       dcsEcho "failed - status is $status"
       exit $gv_error
      fi
   fi
fi
}

#
# Checks if we have permissions to execute the ip and arping commands
# (it needs to be set in the /etc/sudoers file)
#
CheckSudo()
{

rm dcsbind.tmp > /dev/null 2>&1
touch dcsbind.tmp
sudo -l -S 1> dcsbind.tmp 2>&1  < dcsbind.tmp

dcsEcho "Checking /sbin/ip privileges"
grep "/sbin/ip" dcsbind.tmp  > /dev/null
status=$?
if [ $status -ne 0 ]; then
   dcsEcho "ip command is not on the sudo list"
   rm dcsbind.tmp
   exit $gv_error;
fi

dcsEcho "Checking arping privileges"
grep arping dcsbind.tmp  > /dev/null
status=$?
if [ $status -ne 0 ]; then
   dcsEcho "arping command is not on the sudo list"
   rm dcsbind.tmp
   exit $gv_error
fi
rm dcsbind.tmp

dcsEcho "DONE...sudo check"

}

#
# validate the environment and parameters 
#
ValidateParams()
{

# check if we're on a cluster
if [ -z $CLUSTERNAME ]; then
   dcsEcho "script should only be run on clusters"
   exit $gv_warn
fi

if [ $gv_port -eq 0 ]; then
    dcsEcho
    dcsEcho "Please specify port number."
    Usage
    exit $gv_error
fi

}

#########################################################
# MAIN portion of dcsbind begins here
#########################################################

gv_float_external_interface=""
gv_float_external_ip=""
gv_port=0

gv_ok=0
gv_warn=1
gv_error=-1

gv_externalip_set=1
gv_internalip_set=1

gv_myhostname=`hostname | cut -d'.' -f1`

GetOpts $1 $2 $3 $4 $5 $6 $7 $8 $9

dcsEcho "dcsbind invoked with parameters -i $gv_float_external_interface -a $gv_float_external_ip -p $gv_port"


ValidateParams
CheckSudo
GetFloatingIpAdrress
Check_VirtualIP_InUse_Unbind
BindFloatIp

exit $gv_ok

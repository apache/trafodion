#!/usr/bin/sh
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
#
# Stop automated stats.

# Get system name.  If this is a NEO system with a non-standard name,
# then the current node is assumed to be the master.

if [ `uname` != "Linux" ]; then
   if [[ $mxcidir != "" && $mxlibdir != "" ]]; then
     # This is test debug environment.  Set vars accordingly
     autoloc="$mxlibdir"
   else
     # This is a production environment.
     autoloc="/usr/tandem/mx_ustat"
   fi

   SYSTEM_NAME=$(uname -a | cut -d" " -f2)
   first=$(print $SYSTEM_NAME | cut -c1-3 | tr 'a-zA-Z' '[F*]')
   last=$(print $SYSTEM_NAME | cut -c4-7 | tr '0-9' '[L*]')
   if [[ $first = "FFF" ]] && [[ $last = "LLLL" ]] && [[ ${#SYSTEM_NAME} = "7" ]];
   then
     MstrSeg=$(uname -a | cut -d" " -f2 | cut -c1-3)"0101"
   else
     MstrSeg=$SYSTEM_NAME
   fi

   autoloc_prefix=/E/${MstrSeg}

else
   autoloc=$TRAF_HOME/export/lib/mx_ustat
   alias print=echo
   autoloc_prefix=""
fi

# Create STOP_AUTO_STATS which will stop USAS.sh
touch $autoloc_prefix${autoloc}/autodir/STOP_AUTO_STATS
# Allow anyone to remove.
chmod 666 $autoloc_prefix${autoloc}/autodir/STOP_AUTO_STATS

# Get RunLogUstats.sh process ids
pids="$(cat $autoloc_prefix${autoloc}/autodir/USTAT_AUTO_PROGRESS/run*)" 2>/dev/null

# Kill RunLogUstats.sh processes


if [ `uname` != "Linux" ]; then
   if [[ $SYSTEM_NAME = $MstrSeg ]]; then
     kill -9 $pids > ${autoloc}/autodir/STOP_AUTO_STATS 2>&1
   else
     print "kill -9 $pids" > /E/${MstrSeg}${autoloc}/autodir/STOP_AUTO_STATS 2>&1
     gtacl -s -c "\\${MstrSeg}.osh < ${autoloc}/autodir/STOP_AUTO_STATS > ${autoloc}/autodir/STOP_AUTO_STATS_OUT 2>&1" > ${autoloc}/autodir/STOP_AUTO_STATS_OUT 2>&1 &
   fi
else
   pdsh=$(which pdsh 2>/dev/null)

   if [ "$pdsh" = "" ]; then
     pdsh_a=""
   else
     pdsh_a="pdsh -a"
   fi

   $pdsh_a kill -9 $pids > ${autoloc}/autodir/STOP_AUTO_STATS 2>&1
fi

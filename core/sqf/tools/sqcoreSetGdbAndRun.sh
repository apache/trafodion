#!/bin/bash
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

# This script takes a core filename and sets up the environment for it.
# It then starts GDB for it.

dis_usage()
{
	echo "Usage $0 -n <corefile>"
	echo "   -n does not start gdb but instead displays the command to set environment."
	echo ""
	echo "This script will examine the core file and set up the PATH and LD_LIBRARY_PATH"
	echo "variables to match what was used when the core was created.  GDB will then be"
	echo "started for it.  If you want to save the needed commands, use -n and > to"
	echo "redirect output to a file."
	exit 0
}

doGDB=1   # set to 1, means start gdb, 0 means to not start it.

while getopts 'hn' parmOpt
do
    case $parmOpt in
    h)  dis_usage
	exit 0
        ;;
    n)  doGDB=0
	;;
    ?)  echo "Invalid option specified.   Only -h and -n are allowed."
        exit 0
        ;;
    esac
done
shift $((${OPTIND} - 1))

if [ $# -lt 1 ] ; then
	dis_usage
	exit 0
fi

CORENAME_FULL=$1
if [ ! -e ${CORENAME_FULL} ] ; then
	echo "Specified core file does not exist."
	exit 0
fi

if [ $(file ${CORENAME_FULL} | grep -c "core file") -eq 0 ] ; then
	echo "Specified core file does not seem to be a valid core file."
	echo ">file ${CORENAME_FULL}"
	file ${CORENAME_FULL}
	exit 0
fi

echo "#Processing ${CORENAME_FULL}"

# get the execution that caused this core.  gdb tells us that.
PROGRAM_NAME=$(gdb -nx -batch -c ${CORENAME_FULL} -ex quit | grep '^Core was')
echo ""
echo "#${PROGRAM_NAME}"
PROGRAM_NAME=$(echo ${PROGRAM_NAME} | awk '/^Core was/ {command=substr($5,2); cmdlen=index($5,"."); if (cmdlen==0) cmdlen=length($5)+2; print substr($5,2,cmdlen-3)}')

echo ""
echo "#Scanning for path and ld_library_path names.   Be patient."
PROGDIRS=$(awk 'BEGIN {ldPathFound=0;pathFound=0}
     /\x00LD_LIBRARY_PATH=/,/\x00/ {if (ldPathFound==1) {next}
          else {ldPathFound=1;startStr=match($0,"\x00LD_LIBRARY_PATH=");stopStr=match(substr($0,startStr+1),"\x00"); print substr($0, startStr+1, stopStr-1)}}
     /\x00PATH=/,/\x00/            {if (pathFound==1)   {next}
          else {pathFound=1;  startStr=match($0,"\x00PATH=");stopStr=match(substr($0,startStr+1),"\x00"); print substr($0, startStr+1, stopStr-1)}}
         { if ( ldPathFound + pathFound == 2 ) exit }' ${CORENAME_FULL})
#
#grep -o -a -P '\000(LD_LIBRARY_)?PATH=[^\000]*' ${CORENAME_FULL} | \
#	awk -F= 'BEGIN {ldPathFound=0;pathFound=0}
#		/^\x00LD_LIBRARY_PATH/ {if (ldPathFound==1) {next} else {ldPathFound=1;print substr($0,2)}}
#		/^\x00PATH/            {if (pathFound==1)   {next} else {pathFound=1;  print substr($0,2)}}')
if [ ${#PROGDIRS} -gt 0 ] ; then
	wantDir=$(echo "${PROGDIRS}" | awk -F= '/^LD_LIBRARY_PATH/ {print $2}')
	if [ ${#wantDir} -gt 0 ] ; then
		echo ""
		echo "#LD_LIBRARY_PATH needed."
		echo "export LD_LIBRARY_PATH=${wantDir}"
		if [ ${doGDB} -eq 1 ] ; then
			export LD_LIBRARY_PATH="${wantDir}"
		fi
	else
		echo ""
		echo "#LD_LIBRARY_PATH was not found.  Can not set it."
	fi
	wantDir=$(echo "${PROGDIRS}" | awk -F= '/^PATH/ {print $2}')
	if [ ${#wantDir} -gt 0 ] ; then
		echo ""
		echo "#PATH needed."
		echo "export PATH=${wantDir}"
		export PATH="${wantDir}"
	else
		echo ""
		echo "#PATH was not found.  Can not set it."
	fi
fi

echo ""
echo "#To start GDB."
echo "gdb ${PROGRAM_NAME} ${CORENAME_FULL}"
if [ ${doGDB} -eq 1 ] ; then
	gdb ${PROGRAM_NAME} ${CORENAME_FULL}
fi

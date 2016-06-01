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

# script that executes the 'sqdiag_core_mask' script in certain SQ directories.
# based on sqdiag_core_compare and sqdiag_core_mask.
#  Feb 6, 2013.   Fixed scriptdir link.
#  Feb 26, 2013.  Route gdb stderr to null to eliminate the RH6 missing lib msgs.
#  Mar 7, 2013.   Modified sed script to mask 64-bit addresses.  Mask partition names an queries.
#  Jun 10, 2013.  Modified to keep a count of the cores.

mode="m"        # m indicates master mode, s indicates slave mode.
wantDelete=0    # 0 indicates info only, 1 indicates to actually delete duplicates.
deleteTooOld="" # if -g, then delete cores that don't have matching code anymore.
remHost=""
cleanUpLog=0    # if 1, then clean up diag directory first.  IE:  start from scratch.
redoMask=""     # if -m, then all mask files are removed and recreated.

while getopts 'chgdms:' parmOpt
do
    case $parmOpt in
    h)  echo "Usage $0 -h -d -g -c -m"
	echo "  -h to get this help text."
	echo "  -d to get list of unique files and delete all duplicates."
	echo "  -g also delete all cores that don't have matching code anymore."
	echo "     This is detected when gdb backtrace shows 'in ?? ().'"
	echo "  -c cleanup.  IE: start from scratch.  default is to do new files only."
	echo "  -m recreate the mask files."
	echo ""
	echo "This routine compares all core files in the cluster and returns list"
	echo "of unique cores.  This is done by masking all ADDRESSES in the core"
	echo "backtraces.  IE: Only list of procedures is left."
	echo "With the -d switch, all duplicates are removed."
	exit 0
        ;;
    d)  wantDelete=1
	;;
    g)  deleteTooOld="-g"
	;;
    c)  cleanUpLog=1
	;;
    m)  redoMask="-m"
	;;
    s)  mode="s"
	remHost="${OPTARG}"
        ;;
    ?)  echo "Invalid option specified.   Only -h, -g, -c, -m and -d are allowed."
        exit 0
        ;;
    esac
done

scriptName=$(readlink -f $0)
if [ ! -x ${scriptName} ] ; then
    scriptName=$(type -p $0)
    if [ ${#scriptName} -eq 0 ] ; then
        echo "Can't find $0."
        exit 1
    fi
fi
scriptDir=$(dirname ${scriptName})
CORE_DIR=/local/cores/$UID
DIAG_DIR=${CORE_DIR}/sqdiag_tmp
DIAG_LOG=${DIAG_DIR}/sqdiag_core_compare.log

#########################################################################################
#  If in master mode, ask all nodes to do their masking stuff.
#  Identify duplicates and uniqueness, then if wanted, tell nodes to delete duplicates.
if [ ${mode} == "m" ] ; then
	if [ ${cleanUpLog} -eq 1 ] ; then
		# First delete the temp directory.
		echo "Deleting temporary directory    ${DIAG_DIR}"
		pdsh $MY_NODES rm -rf ${DIAG_DIR}
	fi
	mkdir ${DIAG_DIR} 2>/dev/null           # create the local one so that remotes can copy to it.
	echo ""

	# Now tell all nodes to process their cores.  This is done in parallel.
	echo "Creating core mask files on all nodes."
	pdsh $MY_NODES "${scriptName} -s $(uname -n) ${redoMask}"

	echo ""
	echo "=============================="
	echo "Identifying unique core files."
	echo "=============================="
	if [ ! -d ${DIAG_DIR} ] ; then
		echo "No remote files to process."
		exit 0
	fi

	# find unique core files
	rm -f ${DIAG_LOG}
	cksum ${DIAG_DIR}/mask_* | sort -n -k1,1 -k2,2 -k3,3 | \
		awk 'BEGIN {count=0; xsum=-1; eof=-1; fname=""}
		       { indexval=index($3,"+")
		         addcount=1
		         if (indexval > 0) addcount+=int(substr($3,indexval))
		         if (($1 == xsum) && ($2 == eof)) {
		             count+=addcount
		             next
		         }
		         if (length(fname) > 0) {printf "%s", fname; if (count > 0) {printf "+%d\n", count } else printf "\n"}
		         fname=gensub("mask_core", "core", 1, $3)
		         indexval=index(fname,"+")
		         if (indexval > 0) {count=int(substr(fname, indexval)); fname=substr(fname,1,indexval - 1)} else count=0
		         xsum=$1
		         eof=$2
		        }
		     END {if (length(fname) > 0) {printf "%s", fname; if (count > 0) {printf "+%d\n", count } else printf "\n"}}
                ' | tee ${DIAG_LOG}
	numUnique=$(wc -l ${DIAG_LOG} | awk '{print $1}')
	echo ""
	echo "============================================================"
	echo "NOTE: ${numUnique} unique core file list saved to"
	echo "      ${DIAG_LOG}"
	pdcp $MY_NODES -p ${DIAG_LOG} ${DIAG_DIR}    # Ship unique list to all nodes.
	echo "============================================================"

	# Cleanup remote mask files.  Only leave files that belong on this node.
	echo "Cleaning master temporary directory    ${DIAG_DIR}"
	pushd ${DIAG_DIR} >/dev/null
	for CORENAME_MASK in $(ls mask_core* 2>/dev/null | grep -v $(uname -n))
	do
		if [ ! -e ${CORENAME_MASK} ] ; then
			continue
		fi
		CORENAME_SHORT=${CORENAME_MASK/mask_core/core}
		CORENAME_BT=bt_${CORENAME_SHORT}
		rm -f ${CORENAME_MASK}
		rm -f ${CORENAME_BT}
	done
	popd >/dev/null

	if [ ${wantDelete} -eq 1 ] ; then
		# Now tells all nodes to delete duplicates.
		echo ""
		echo "Deleting duplicates."
		echo ""
		pdsh $MY_NODES "${scriptName} -s $(uname -n) -d ${deleteTooOld}"
	fi

	echo ""
	echo "Done."
	exit 0
fi

#########################################################################################
# If we reach this portion, it's because we're in slave mode.
# wantDelete tells us if we're to delete or if we're in gathering mode.
if [ ${wantDelete} -eq 1 ] ; then
	#We're in delete mode.
	if [ ! -d ${CORE_DIR} ] ; then
		echo "Directory ${CORE_DIR} does not exist.  Can't delete."
		exit 0
	fi

	if [ ! -f ${DIAG_LOG} ] ; then
		echo "Diagnostic file ${DIAG_LOG} does not exist.  Can't delete."
		exit 0
	fi

	#  Now do the deletes.
	pushd ${CORE_DIR} >/dev/null
	for COREFILE in core.*
	do
		SHORTCORE=${COREFILE%%+[0-9]*}
		if [ ! -e ${COREFILE} ] ; then
			continue
		fi
		if [ ! -f ${DIAG_DIR}/mask_${COREFILE} ] || [ $(wc -c ${DIAG_DIR}/mask_${COREFILE} | awk '{print $1}') -eq 0 ] ; then
			echo "Mask file for ${COREFILE} is missing or is of length 0, not deleting."
		elif [ $(grep -c "${SHORTCORE}" ${DIAG_LOG}) -eq 0 ] ; then
			echo "${COREFILE} deleted, duplicate."
			rm -f ${COREFILE}
			rm -f ${DIAG_DIR}/bt_${COREFILE}
			rm -f ${DIAG_DIR}/mask_${COREFILE}
		elif [ $(grep -c "${SHORTCORE}+" ${DIAG_LOG}) -gt 0 ] ; then
			# We have   +number suffix.   Rename our local file to match it.
			newName=$(grep "${SHORTCORE}" ${DIAG_LOG} | sed -e 's@^.*/core.@core.@')
			mv ${COREFILE} ${newName}
			mv ${DIAG_DIR}/bt_${COREFILE} ${DIAG_DIR}/bt_${newName}
			mv ${DIAG_DIR}/mask_${COREFILE} ${DIAG_DIR}/mask_${newName}
		elif [ ${#deleteTooOld} -gt 0 ] ; then
			if [ $(grep -cF ' in ?? (' ${DIAG_DIR}/bt_${COREFILE}) -gt 0 ] ; then
				echo "${COREFILE} deleted, too old."
				rm -f ${COREFILE}
				rm -f ${DIAG_DIR}/bt_${COREFILE}
				rm -f ${DIAG_DIR}/mask_${COREFILE}
			fi
		fi
	done
	popd >/dev/null

	exit 0
fi

#########################################################################################
# We are in slave mode and in information mode, ie: mask mode.
if [ ${#remHost} -eq 0 ] ; then
	echo "Internal error.  Remotehost name must be supplied for slave/mask mode."
	exit 0
fi

# First verify if any of our mask or bt files can be deleted.
if [ -d ${DIAG_DIR} ] ; then
	pushd ${DIAG_DIR} >/dev/null
	for CORENAME_MASK in $(ls mask_core* 2>/dev/null | grep $(uname -n))   # make sure we look at our node only.
	do
		if [ ! -e ${CORENAME_MASK} ] ; then
			continue
		fi
		CORENAME_SHORT=${CORENAME_MASK/mask_core/core}
		CORENAME_BT=bt_${CORENAME_SHORT}
		if [ ! -e ${CORE_DIR}/${CORENAME_SHORT} ] ; then
			rm -f ${CORENAME_MASK}
			rm -f ${CORENAME_BT}
		fi
	done
	popd >/dev/null
fi

#Next portion was based on sqdiag_core_mask.
#
#	get traces from all core files
#

CORENAME_LIST=$(find -L ${CORE_DIR} -name "core.*" 2>/dev/null)

mkdir ${DIAG_DIR} 2>/dev/null
GDB_FILE=${DIAG_DIR}/gdb_bt_command
echo "set output-radix 0x10" > ${GDB_FILE}
echo "bt" >> ${GDB_FILE}
echo "quit" >> ${GDB_FILE}

SED_FILE=${DIAG_DIR}/sed_script
echo '# Explanation of sed script below.' > ${SED_FILE}
echo '# Removes all lines to the first stack trace line.' >> ${SED_FILE}
echo '1,/#0 / d' >> ${SED_FILE}
echo '# Changes all hex addresses to 8?' >> ${SED_FILE}
echo 's@0x[0-9a-f]\{8,\}@0x????????@g' >> ${SED_FILE}
echo '# Changes all lines numbers to 3?' >> ${SED_FILE}
echo 's@:[0-9][0-9]*@:???@g' >> ${SED_FILE}
echo '# Changes all Hex numbers to 8?' >> ${SED_FILE}
echo 's@0x[0-9a-f][0-9a-f]*@0x????????@g' >> ${SED_FILE}
echo '# Changes all process names $Z* to $Z6?' >> ${SED_FILE}
echo 's@$Z[0-9a-zA-Z]*@$Z??????@g' >> ${SED_FILE}
echo '# Changes all partition names $DBxxxx.ZSDxxxxx.xxxxxx to $DB????.ZSD?????.????????' >> ${SED_FILE}
echo 's@\$DB[[:alnum:]]\{1,\}\.ZSD[[:alnum:]]\{1,\}\.[[:alnum:]]\{1,\}@\$DB????\.ZSD?????\.????????@'g >> ${SED_FILE}
echo '# Changes all queries "SELECT ...." to "SELECT ??????"  NOTE First need to eliminate all \" and ...' >> ${SED_FILE}
echo 's@\\"@@g' >> ${SED_FILE}
echo 's@"\.\.\.@"@g' >> ${SED_FILE}
echo 's@"SELECT [^"]*"@"SELECT ????????"@gI' >> ${SED_FILE}
echo 's@"INSERT [^"]*"@"INSERT ????????"@gI' >> ${SED_FILE}
echo 's@"UPDATE [^"]*"@"UPDATE ????????"@gI' >> ${SED_FILE}
echo 's@"DELETE [^"]*"@"DELETE ????????"@gI' >> ${SED_FILE}

SAVEPATH="${PATH}"
SAVELIBPATH="${LD_LIBRARY_PATH}"
for CORENAME_FULL in ${CORENAME_LIST}; do
	if [ ! -e ${CORENAME_FULL} ] ; then
		continue
	fi

	echo "Processing ${CORENAME_FULL}"

	# get trace from core file
	CORENAME_SHORT=$(echo ${CORENAME_FULL} | sed -e 's@^.*/core.@core.@')
	CORENAME_BT=${DIAG_DIR}/bt_${CORENAME_SHORT}
	CORENAME_MASK=${DIAG_DIR}/mask_${CORENAME_SHORT}

	if [ -e ${CORENAME_MASK} ] ; then
		if [ ${#redoMask} -gt 0 ] ; then
			# We're asking to redo the masks.
			rm ${CORENAME_MASK}
		else
			# We already have a mask for this core.  No need to regenerate it.
			continue
		fi
	fi

	if [ ! -e ${CORENAME_BT} ] ; then
	        # BT and MASK files don't exist.   So create them.
		# get the execution that caused this core.  gdb tells us that.
		PROGRAM_NAME=$(gdb -nx -batch -c ${CORENAME_FULL} -ex quit 2>/dev/null | \
		    awk '/^Core was/ {command=substr($5,2); cmdlen=index($5,"."); if (cmdlen==0) cmdlen=length($5)+2; print substr($5,2,cmdlen-3)}')

		# First try with the current defautls.
		gdb ${PROGRAM_NAME} ${CORENAME_FULL} -x ${GDB_FILE} > ${CORENAME_BT} 2>/dev/null

		if [ $(grep -cF ' in ?? (' ${CORENAME_BT}) -gt 0 ] ; then
		        # Get the path and ld_library_path used for this core since current default don't work.
		        # This is hidden in core file.   Slow to get it.
			PROGDIRS=$(awk 'BEGIN {ldPathFound=0;pathFound=0}
			     /\x00LD_LIBRARY_PATH=/,/\x00/ {if (ldPathFound==1) {next}
		          else {ldPathFound=1;startStr=match($0,"\x00LD_LIBRARY_PATH=");stopStr=match(substr($0,startStr+1),"\x00"); print substr($0, startStr+1, stopStr-1)}}
			     /\x00PATH=/,/\x00/            {if (pathFound==1)   {next}
		          else {pathFound=1;  startStr=match($0,"\x00PATH=");stopStr=match(substr($0,startStr+1),"\x00"); print substr($0, startStr+1, stopStr-1)}}
			         { if ( ldPathFound + pathFound == 2 ) exit }' ${CORENAME_FULL})
			if [ ${#PROGDIRS} -gt 0 ] ; then
				wantDir=$(echo "${PROGDIRS}" | awk -F= '/^LD_LIBRARY_PATH/ {print $2}')
				if [ ${#wantDir} -gt 0 ] ; then
					LD_LIBRARY_PATH="${wantDir}"
				fi
				wantDir=$(echo "${PROGDIRS}" | awk -F= '/^PATH/ {print $2}')
				if [ ${#wantDir} -gt 0 ] ; then
					PATH="${wantDir}"
				fi
				gdb ${PROGRAM_NAME} ${CORENAME_FULL} -x ${GDB_FILE} > ${CORENAME_BT} 2>/dev/null
			fi
		fi
	fi

	#  Mask the traces so they can be compared
	if [ $(grep -cF ' in ?? (' ${CORENAME_BT}) -gt 0 ] ; then
		# We don't have correct environment, so don't do a mask.  Just copy the full trace.
		cp ${CORENAME_BT} ${CORENAME_MASK}
	else
		# Explanation of sed script is above where it is created.
		sed -f  ${SED_FILE} ${CORENAME_BT} > ${CORENAME_MASK}
	fi
	PATH="${SAVEPATH}"
	LD_LIBRARY_PATH="${SAVELIBPATH}"
done

# Copy mask files to common node (which was a parameter)
if [ ${remHost} != "$(uname -n)" ]; then
	pdcp -w ${remHost} -p ${DIAG_DIR}/mask_* ${DIAG_DIR} 2>/dev/null
fi

# We're done with the slave stuff.
exit 0

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
#
#  11/12/22        Used $MY_NODES instead of -a to get list of cores.
#                  Look for filenames if it does not exist.

tempFile=~/temp.zzzz
rm -f ${tempFile}
rm -f ${tempFile}last

coreDir=/local/cores/$UID
# searches standard core location /cores/$UID for core files
echo "Getting list of core files."
pdsh $MY_NODES "find -L ${coreDir} -name core\.\* -exec ls --full-time '{}' \; | grep -v '^Total'" 2> /dev/null | awk '{print $6,$7,$8,substr($1,1,length($1)-1),$10}' | sed 's@\.000000000@@' | sort > ${tempFile}

lastSize=0
lastName=""
lastNode=""
lastExec=""
lastFname=""
lastBTfile=""
numDeleted=0
numFiles=0
echo "Processing files".
exec 3< ${tempFile}
while read -r -u 3 fileSize fileDate fileTime fileNode fileName ; do
    numFiles=$((${numFiles} + 1))
    fileFname=$(echo ${fileName} | awk -F/ '{print $NF}')
    fileExec=$(echo ${fileFname} | awk -F. '{print $NF}')

    if [ ${fileSize} -eq 0 ] ; then
        echo ""
        echo "${fileFname} is empty.   Deleting it."
        ssh ${fileNode} rm -f ${fileName}
        numDeleted=$((${numDeleted} + 1))
        continue
    fi

    if [ ${fileSize} -eq ${lastSize} ] && [ "${fileExec}" = "${lastExec}" ] ; then
        realExec=$fileExec
        if [ -z "$(which $fileExec 2>&-)" ] ; then
            for currDir in $(echo $PATH | sed "s/:/ /g") ; do
                if [ -n "$(ls $currDir/$fileExec* 2>&- | head -1)" ] ; then
                    realExec=$(ls $currDir/$fileExec* 2>&-)
                    break
                fi
            done
        fi
        ssh ${fileNode} "gdb -batch -ex bt ${realExec} ${fileName} 2>/dev/null | grep '^#' | sed 's/(.*)//g'" > ${tempFile}curr
        if [ ! -e "${tempFile}last" ] ; then
            ssh ${lastNode} "gdb -batch -ex bt ${realExec} ${lastName} 2>/dev/null | grep '^#' | sed 's/(.*)//g'" > ${tempFile}last
        fi

        if [ $(diff ${tempFile}last ${tempFile}curr | wc -l) -eq 0 ] ; then
            echo ""
            echo "${fileFname} matches ${lastFname} and will be deleted."
            ssh ${fileNode} rm -f ${fileName}
            numDeleted=$((${numDeleted} + 1))
            rm -f ${tempFile}curr
            continue
        fi
    fi

    #if we reach this point.   Files don't match.
    echo -n "."
    lastSize=${fileSize}
    lastName=${fileName}
    lastFname=${fileFname}
    lastExec=${fileExec}
    lastNode=${fileNode}
    if [ -e "${tempFile}curr" ] ; then
        mv -f ${tempFile}curr ${tempFile}last
    else
        rm -f ${tempFile}last
    fi
done
exec 3<&-

echo ""
echo "${numDeleted} out of ${numFiles} core files were deleted."
rm -f ${tempFile}
rm -f ${tempFile}curr
rm -f ${tempFile}last

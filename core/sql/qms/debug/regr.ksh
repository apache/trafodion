#! /bin/sh
#/* -*-C++-*-
#// @@@ START COPYRIGHT @@@
#//
#// Licensed to the Apache Software Foundation (ASF) under one
#// or more contributor license agreements.  See the NOTICE file
#// distributed with this work for additional information
#// regarding copyright ownership.  The ASF licenses this file
#// to you under the Apache License, Version 2.0 (the
#// "License"); you may not use this file except in compliance
#// with the License.  You may obtain a copy of the License at
#//
#//   http://www.apache.org/licenses/LICENSE-2.0
#//
#// Unless required by applicable law or agreed to in writing,
#// software distributed under the License is distributed on an
#// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#// KIND, either express or implied.  See the License for the
#// specific language governing permissions and limitations
#// under the License.
#//
#// @@@ END COPYRIGHT @@@
# *****************************************************************************
# */

typeset -i dStartIndex
typeset -i dEndIndex
typeset -i dIndex

let dStartIndex=1
let dEndIndex=15
generateExpected=0

function Usage {
  echo "Usage: $0 [-s <nn> | -e <nn> | -g]"
  echo "-s <nn> : The start index of the test file"
  echo "-e <nn> : The end index of the test file"
  echo "-g      : generate expected files"
  echo "-h      : help"
}

function GetOpts {

    while getopts "gs:e:h" arg
      do
      case $arg in
          g)
              generateExpected=1;
              ;;
          s)
              dStartIndex=$OPTARG;
              ;;
          e)
              dEndIndex=$OPTARG;
              ;;
          h)
              Usage;
              exit 0;
      esac
    done

}

GetOpts $1 $2 $3 $4 $5 $6 $7 $8 $9

if [ $generateExpected -eq 1 ]; then
   echo "Going to generate expected files"
fi

dIndex=$dStartIndex

while [ $dIndex -le $dEndIndex ]; do

    echo "-- Test $dIndex --"

    # run sqlci to generate the mvqr files
    sqlci -i f$dIndex > f$dIndex.sql.out

    # run qms
    qms mvqr.in mvqr.out

    if [ $generateExpected -eq 1 ]; then

	cp mvqr.mv.xml f$dIndex.cmp.mv.xml
	cp qrlog f$dIndex.cmp.qrlog
	echo "Generated expected files: f$dIndex.cmp.mv.xml, f$dIndex.cmp.qrlog"

    else

	diff mvqr.mv.xml f$dIndex.cmp.mv.xml
	echo "Test $dIndex: diff mvqr.mv.xml f$dIndex.cmp.mv.xml... return code: $?"

	diff qrlog f$dIndex.cmp.qrlog
	echo "Test $dIndex: diff qrlog f$dIndex.cmp.qrlog... return code: $?"

    fi

    # save the output 
    mv qrlog f$dIndex.qrlog
    mv mvqr.mv.xml f$dIndex.mv.xml
    mv mvqr.analysis f$dIndex.analysis

    dIndex=$dIndex+1

done

exit 0

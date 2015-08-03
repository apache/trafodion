-- @@@ START COPYRIGHT @@@
--
-- Licensed to the Apache Software Foundation (ASF) under one
-- or more contributor license agreements.  See the NOTICE file
-- distributed with this work for additional information
-- regarding copyright ownership.  The ASF licenses this file
-- to you under the Apache License, Version 2.0 (the
-- "License"); you may not use this file except in compliance
-- with the License.  You may obtain a copy of the License at
--
--   http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing,
-- software distributed under the License is distributed on an
-- "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
-- KIND, either express or implied.  See the License for the
-- specific language governing permissions and limitations
-- under the License.
--
-- @@@ END COPYRIGHT @@@
# for each file defined in FILES, replace all parameters defined in PARAMS by
# their corresponding values defined in PARAMFILE.
#
PARAMFILE="parameters.txt"
FILES="cidefs import_exeperf_tables.ksh clear_cache.ksh"
PARAMS="ExeperfLocationOrders ExeperfLocationLineitem ExeperfPartition1 ExeperfPartition2 ExeperfPartition3 ExeperfPartition4"

if [ ! -e $PARAMFILE ] ; then
  echo "ERROR: In order to run the exeperf test module you must have a parameters.txt file in your exeperf directory."
  exit 1
fi

#load parameters from ParameFile
. $PARAMFILE

for file in $FILES; do
  filesrc="$file.src"
  filetmp="$file.tmp"
  echo "Replacing paramters in file $filesrc ..."
  cp $filesrc $filetmp
  for param in $PARAMS; do
    eval paramvalue=\$$param
    sedcmd="s#\#${param}\##${paramvalue}#g"
    sed -e "$sedcmd" $filetmp > $file
    cp $file $filetmp
  done
  rm -f $filetmp
  [[ $file = *\.ksh ]] && chmod +x $file
done

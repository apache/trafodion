#!/bin/sh

#-- @@@ START COPYRIGHT @@@
#--
#-- Licensed to the Apache Software Foundation (ASF) under one
#-- or more contributor license agreements.  See the NOTICE file
#-- distributed with this work for additional information
#-- regarding copyright ownership.  The ASF licenses this file
#-- to you under the Apache License, Version 2.0 (the
#-- "License"); you may not use this file except in compliance
#-- with the License.  You may obtain a copy of the License at
#--
#--   http://www.apache.org/licenses/LICENSE-2.0
#--
#-- Unless required by applicable law or agreed to in writing,
#-- software distributed under the License is distributed on an
#-- "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#-- KIND, either express or implied.  See the License for the
#-- specific language governing permissions and limitations
#-- under the License.
#--
#-- @@@ END COPYRIGHT @@@
#
# This script generates the run-time stats for all the queries currently running.
# The statistics outputs are channeled to stdout.
#
# If no argument is given, the per-operator run-time stats are generated. If arguments
# are given, they are passed to the SQL command "get statistics for qid" without 
# modification. A common argument is 'default', which generates the per-table stats.

lso=$TRAF_HOME/export/limited-support-tools/LSO

qids=`$lso/offender -s active -t 10 | grep -o 'MXID[^ ]*'`

for qid in $qids 
do
  if [ $# = 0 ]; then
    echo "get statistics for qid $qid ;" | sqlci
  else
    echo "get statistics for qid $qid $*;" | sqlci
  fi
done


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

# report version number of hadoop components

if [[ $1 == "hadoop" ]]
then
  hadoop version | sed -n 's/^.*Hadoop \(.*\)$/\1/p'
elif [[ $1 == "hbase" ]]
then
  hbase version 2>&1 | sed -n 's/^.*HBase \(.*\)$/\1/p'
elif [[ $1 == "hive" ]]
then
  hive --version 2>&1 | sed -n 's/^.*Hive \(.*\)$/\1/p'
elif [[ $1 == "zookeeper" ]]
then
   # zookeeper does not report full version number
   #echo status | nc localhost 2181 | sed -n 's/^.*Zookeeper version: \(.*\)$/\1/p'
   # so go back to rpm package
   rpm -qa | grep -E '^zookeeper' | sed -n 's/^[^-]*-\(.*\).el6.*$/\1/p'
fi
exit 0

#
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

#!/bin/sh
echo "===Starting environment in trace mode"
./xshell -c "'startup trace 2'" 2>/dev/null &
sleep 5
echo "===Test1-Client/Server, priority, open, close, processinfo"
./xshell -a test1.sub
echo "===attach test"
mpirun -ha -spawn -np 1 ./attach
echo "===Restarting environment"
./xshell -c shutdown
echo "===Test2-Client/Server Multi pass"
./xshell -c "'startup trace 0'" 2>/dev/null &
sleep 5
./xshell -a test2.sub
echo "===Test3-NonStop Process Pairs"
./xshell -a test3.sub
echo "===Test4-Configuration tests"
./xshell -a test4.sub
echo "===Test5-Get TM seq#"
./xshell -a test5.sub
echo "===Test6-Process Death notices"
./xshell -a test6.sub
echo "===Test7-Load-balance large number of processes"
./xshell -a test7.sub
echo "===Test8-DTM Sync"
./xshell -a test8.sub
echo "===Exit DTMs"
./xshell -c "'event {DTM} 6'"
./xshell -c ps
./xshell -c "'shutdown !'"

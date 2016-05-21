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

goall runs all the currently working tests.
NOTE: your PATH must include ".".
NOTE: Make sure that there is no seaquest environment running.
      e.g. sqps - should show 'Environment has not been started!
If the environment is setup correctly, you should be able to 'goall'.

To use goall:
  goall [-cluster] [-verbose]
  options:
    -cluster will run tests in a cluster [otherwise virtual-cluster].
    -verbose will display shell script output.
    if goall is not working correctly, -verbose is recommended.

To run an individual test, e.g. t<number>:
  go<number> [-cluster] [-verbose]

An adhoc cluster (set of workstations using nfs) can be 
used by creating a nmap.conf file which contains a list of hosts
for the cluster (one host per line).
'goall -cluster -verbose' can then be used to run the tests.

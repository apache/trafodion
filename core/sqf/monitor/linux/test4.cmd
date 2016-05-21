!
! @@@ START COPYRIGHT @@@
!
! Licensed to the Apache Software Foundation (ASF) under one
! or more contributor license agreements.  See the NOTICE file
! distributed with this work for additional information
! regarding copyright ownership.  The ASF licenses this file
! to you under the Apache License, Version 2.0 (the
! "License"); you may not use this file except in compliance
! with the License.  You may obtain a copy of the License at
!
!   http://www.apache.org/licenses/LICENSE-2.0
!
! Unless required by applicable law or agreed to in writing,
! software distributed under the License is distributed on an
! "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
! KIND, either express or implied.  See the License for the
! specific language governing permissions and limitations
! under the License.
!
! @@@ END COPYRIGHT @@@
!

startup
echo ===set cluster global group
set key1=cluster1
set key2=cluster2
set key3=cluster3
echo ===reset key2 to cluster2a
set key2=cluster2a
echo ===set node 0 local group
set {nid 0}key1=node0-1
set {nid 0}key2=node0-2
set {nid 0}key3=node0-3
echo ===reset key2 to node0-2a
set {nid 0}key2=node0-2a
echo ===set node 1 local group
set {nid 1}key1=node1-1
set {nid 1}key2=node1-2
set {nid 1}key3=node1-3
echo ===reset key2 to node1-2a
monitor 1
set {nid 1}key2=node1-2a
echo ===set node 2 local group
set {nid 2}key1=node2-1
set {nid 2}key2=node2-2
set {nid 2}key3=node2-3
echo ===reset key2 to node2-2a
monitor 2
set {nid 2}key2=node2-2a
echo ===set process $ABC local group
set {process $abc}key1=abc-1
set {process $abc}key2=abc-2
set {process $abc}key3=abc-3
echo ===reset key2 to abc-2a
set {process $abc}key2=abc-2a
echo ===set process $DEF local group
set {process $def}key1=def-1
set {process $def}key2=def-2
set {process $def}key3=def-3
echo ===reset key2 to def-2a
set {process $def}key2=def-2a
echo === show configuration from monitor in node0
monitor 0
show
show {nid 0}
show {nid 1}
show {nid 2}
show {process $abc}
show {process $DEF}
echo === show configuration from monitor in node1
monitor 1
show
show {nid 0}
show {nid 1}
show {nid 2}
show {process $abc}
show {process $DEF}
echo === show configuration from monitor in node2
monitor 2
show
show {nid 0}
show {nid 1}
show {nid 2}
show {process $abc}
show {process $DEF}

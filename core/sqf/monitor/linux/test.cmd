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

startup trace 2
node
ps
exec {nowait,name $SERV0,nid 0}server
exec {nowait,pri 5,name $SERV1}server
exec {nowait,pri 10,name $CLIENT,nid 0}client
delay 1
ps
delay 1
ps
ps
ps
delay 1
ps
delay 5




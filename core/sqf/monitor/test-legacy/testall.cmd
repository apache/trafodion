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

startup trace 0
echo ===Test1-Client/Server, priority, open, close, processinfo
exec shell test1.sub
echo ===Test2-Client/Server Multi pass
!shutdown
!startup trace 0
!exec shell test2.sub
echo ===Test3-NonStop Process Pairs
exec shell test3.sub
echo ===Test4-Configuration Tests
!exec shell test4.sub
echo ===Test5-Get TM seq#
exec shell test5.sub
echo ===Test6-Process Death notices
!exec shell test6.sub
echo ===Test7-Start Lots of processes
exec shell test7.sub
echo ===Test8-DTM Sync
exec shell test8.sub
echo Start Test5 - DOWN Node test
down 2
delay 1
ps
! should abort
event {DTM} 5
delay 5
! should commit
event {DTM} 5
delay 5

echo Exit DTMs
event {DTM} 6
delay 1
ps
shutdown

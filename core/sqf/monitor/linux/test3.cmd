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

! NonStop Process pair testing.
!
startup trace 0
exec {nowait,name $SERV0,nid 0}nsserver
delay 5
exec {nowait,name $CLI,nid 0}nsclient
ps
delay 15
! *** We need to kill $CLI because it currently is locking up
!kill $CLI
!kill 2,2
delay 15
ps
!kill $SERV0
exit

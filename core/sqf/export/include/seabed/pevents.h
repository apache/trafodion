//------------------------------------------------------------------
//
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@

//
// Process-events module
//
#ifndef __SB_PEVENTS_H_
#define __SB_PEVENTS_H_

// Events
enum { LSEM     = 0x0001 };  // extended semaphore
enum { PWU      = 0x0002 };  // private wakeup
enum { LRABBIT  = 0x0008 };  // server class event
enum { LQIO     = 0x0008 };  // queued I/O (for shared memory)
enum { LCHLD    = 0x0020 };  // POSIX process' child exited
enum { LPIPE    = 0x0040 };  // POSIX pipe server/socket event
enum { LSIG     = 0x0080 };  // POSIX signal wakeup
enum { LREQ     = 0x0100 };  // requesting a link
enum { LTMF     = 0x0200 };  // TMF request
enum { LDONE    = 0x0400 };  // link done
enum { LCAN     = 0x0800 };  // link cancel
enum { LINSP    = 0x1000 };  // INSPECT event
enum { INTR     = 0x2000 };  // interrupt
enum { IOPON    = 0x4000 };  // I/O power on
enum { PON      = 0x8000 };  // power on

#endif // !__SB_PEVENTS_H_

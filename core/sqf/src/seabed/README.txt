# @@@ START COPYRIGHT @@@
#
# (C) Copyright 2006-2014 Hewlett-Packard Development Company, L.P.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#
# @@@ END COPYRIGHT @@@

Seabed [over sockets].

-----------

To build:
  make

-----------

To cleanup:
  make clean

-----------

Tests:
  See TEST-README.txt

Test run:
  goall # run all single-node tests

-----------

Various notes:

-----------

Implementation files:
  apictr.cpp		- API counters
  cap.cpp		- Capacity
  compq.cpp             - Completion Queue Object
  cpu.cpp               - Processor API
  debug.cpp             - Debug API
  env.cpp		- Environment
  excep.cpp             - Exception Object
  fs.cpp                - File-System API
  fsfname.cpp           - File-System Filename API
  fsi.cpp               - File-System Internals
  fstrace.cpp           - File-System Trace
  fsutil.cpp            - File-System Utilities
  imap.cpp              - Int Map Object (no STL)
  labelmaps.cpp		- Label maps
  labels.cpp		- Label access
  llmap.cpp             - Long-long Map Object (no STL)
  lmap.cpp              - Long Map Object (no STL)
  log.cpp               - Logging API
  logaggr.cpp		- Log aggregation
  logalt.cpp		- Log alternate
  logsys.cpp		- Log system
  loopstream.cpp        - Loop Transport Stream Object
  map64.cpp             - 64-bit map Object (no STL)
  mapcom.cpp		- Map common
  mapmd.cpp		- MD map
  mapstats.cpp		- Map stats
  monclio.cpp           - Monitor client local-io
  mpitmsg.cpp           - MPI Transport Message Object
  msaggr.cpp            - Message-System Aggregation API
  ms.cpp                - Message-System API
  mserr.cpp             - Error mapper
  mserrmsg.cpp		- Error message
  mseventmgr.cpp        - Message-System Event Manager
  msic.cpp              - Message-System Interceptor
  msicctr.cpp		- Message-System Interceptor counters
  mslabelmaps.cpp	- Message-System label maps
  mstracevars.cpp	- Message-System trace variables
  msvars.cpp		- Message-System variables
  msmon.cpp             - Message-System Monitor API
  mstrace.cpp           - Message-System Trace
  msutil.cpp            - Message-System Utilities
  npvmap.cpp		- Nid/Pid/Verifier Map
  otimer.cpp		- Timer Object
  otrace.cpp            - Trace Object (no STL)
  pctl.cpp              - Process Control API
  props.cpp             - Properties Object (no STL)
  propsx.cpp		- Properties pseudo object
  queue.cpp             - Queue Object (no STL)
  queuemd.cpp           - Queue (MD) Object (no STL)
  recvq.cpp             - Receive-Queue Object (no STL)
  sautil.cpp		- Save assert
  slotmgr.cpp           - Slot Manager Object (no STL)
  smap.cpp              - String Map Object (no STL)
  sock.cpp              - Socket Objects
  sockstream.cpp        - Socket Transport Stream Object
  sqstatehi.cpp		- Sqstate HI
  sqstateic.cpp		- Sqstate interceptors
  sqstateicvars.cpp	- Sqstate interceptors variables
  sqstatepi.cpp		- Sqstate
  stream.cpp            - Transport Stream Object
  thread.cpp            - Thread Object
  threadl.cpp           - Thread Linux Object
  threadltls.cpp	- Thread linux TLS
  threadrs.cpp          - Thread Resume/Suspend API
  threadtls.cpp		- Thread TLS
  time.cpp              - Time API
  timer.cpp             - Timer API
  timermap.cpp		- Timer Map
  trace.cpp             - Trace API
  util.cpp              - Utilities
  tmap.cpp		- Template Map
  utilalloc.cpp		- Allocator
  utracex.cpp		- Micro trace

Header files:
  apictr.h		- API counters
  array.h               - Array Template (no STL)
  auto.h                - Automatic monitor messages
  buf.h                 - Buffer object
  cap.h			- Capacity
  chk.h                 - Check error
  compq.h               - Completion Queue
  ecid.h		- Eye catcher
  env.h			- Environment
  fsi.h                 - File-System Internal
  fstrace.h             - File-System Trace
  fsutil.h              - File-System Utilities
  fsx.h                 - File-System Exchange
  imap.h                - int Map Object (no STL)
  labelmapsx.h		- Label maps
  llmap.h               - long-long Map Object (no STL)
  lmap.h                - long Map Object (no STL)
  logaggr.h		- Log aggregation
  logsys.h		- Log system
  looptrans.h           - Loop Transport
  mapcom.h		- Map common
  mapmd.h		- MD Map
  mapstats.h		- Map stats
  ml.h                  - Map Link (no STL)
  monclio.h             - Monitor Client Local-IO
  mondef.h              - Monitor Definitions
  monmsgtype.h		- Monitor message type
  msaggr.h              - Message-System Aggregation
  mserr.h               - Message-System Errors
  mserrmsg.h		- Message-System Error message
  mseventmgr.h          - Message-System Event Manager
  msic.h                - Message-System Interceptor
  mslabelmapsx.h	- Message-System Label Maps
  msi.h                 - Message-System Internal
  msicctr.h		- Message-System Interceptor counters
  msix.h                - Message-System Internal-exchange
  mslog.h		- Message-System
  msmon.h               - Message-System Monitor services
  msod.h		- Message-System
  mstrace.h             - Message-System Trace
  msutil.h              - Message-System Utilities
  msvars.h		- Message-System
  msx.h                 - Message-System Exchange
  npvmap.h		- Nid/Pid/Verifier Map
  pctlx.h               - Process Control Exchange
  phan.h                - Phandle
  props.h               - Properties Object (no STL)
  qid.h                 - Queue Ids
  ql.h			- Queue link
  queue.h               - Queue Object (no STL)
  queuemd.h             - Queue MD (no STL)
  recvq.h               - Receive-Queue (no STL)
  sbconst.h		- Constants
  slotmgr.h             - Slot Manager Object (no STL)
  smap.h                - String Map Object (no STL)
  socktrans.h           - Socket Transport Object
  sqstatehi.cpp		- Sqstate HI
  sqstateicvars.h	- Sqstate interceptor variables
  sqstatei.h		- Sqstate internal
  sqstatesb.h		- Sqstate SB
  streamutil.h		- Stream utilities
  tablemgr.h		- Table manager
  threadltls.cpp	- Thread linux TLS
  threadtls.cpp		- Thread TLS
  threadtlsx.h		- Thread TLS external
  timeri.h		- Timer internal
  timermap.h		- Timer map
  timerx.h              - Timer Exchange
  tmap.h		- Template map
  tracex.h              - Trace Exchange
  trans.h               - Transactions
  transport.h           - Transport
  util.h                - Utilities
  utilalloc.h		- Allocator
  utilatomic.h		- Atomics
  utracex.h		- Micro trace

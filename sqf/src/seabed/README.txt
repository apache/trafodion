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

Seabed [over MPI].

Client view:
  See ES and IS.

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
  compq.cpp             - Completion Queue Object
  cpu.cpp               - Processor API
  debug.cpp             - Debug API
  excep.cpp             - Exception Object
  fs.cpp                - File-System API
  fsfname.cpp           - File-System Filename API
  fsi.cpp               - File-System Internals
  fstrace.cpp           - File-System Trace
  fsutil.cpp            - File-System Utilities
  imap.cpp              - Int Map Object (no STL)
  llmap.cpp             - Long-long Map Object (no STL)
  lmap.cpp              - Long Map Object (no STL)
  log.cpp               - Logging API
  loopstream.cpp        - Loop Transport Stream Object
  map64.cpp             - 64-bit map Object (no STL)
  monclio.cpp           - Monitor client local-io
  mpitmsg.cpp           - MPI Transport Message Object
  msaggr.cpp            - Message-System Aggregation API
  ms.cpp                - Message-System API
  mserr.cpp             - Error mapper
  mseventmgr.cpp        - Message-System Event Manager
  msic.cpp              - Message-System Interceptor
  msmon.cpp             - Message-System Monitor API
  mstrace.cpp           - Message-System Trace
  msutil.cpp            - Message-System Utilities
  otrace.cpp            - Trace Object (no STL)
  pctl.cpp              - Process Control API
  props.cpp             - Properties Object (no STL)
  queue.cpp             - Queue Object (no STL)
  queuemd.cpp           - Queue (MD) Object (no STL)
  recvq.cpp             - Receive-Queue Object (no STL)
  slotmgr.cpp           - Slot Manager Object (no STL)
  smap.cpp              - String Map Object (no STL)
  sock.cpp              - Socket Objects
  sockstream.cpp        - Socket Transport Stream Object
  stream.cpp            - Transport Stream Object
  thread.cpp            - Thread Object
  threadl.cpp           - Thread Linux Object
  threadrs.cpp          - Thread Resume/Suspend API
  time.cpp              - Time API
  timer.cpp             - Timer API
  trace.cpp             - Trace API
  util.cpp              - Utilities


Header files:
  array.h               - Array Template (no STL)
  auto.h                - Automatic monitor messages
  buf.h                 - Buffer object
  chk.h                 - Check error
  compq.h               - Completion Queue
  fsi.h                 - File-System Internal
  fstrace.h             - File-System Trace
  fsutil.h              - File-System Utilities
  fsx.h                 - File-System Exchange
  imap.h                - int Map Object (no STL)
  llmap.h               - long-long Map Object (no STL)
  lmap.h                - long Map Object (no STL)
  looptrans.h           - Loop Transport
  ml.h                  - Map Link (no STL)
  monclio.h             - Monitor Client Local-IO
  mondef.h              - Monitor Definitions
  msaggr.h              - Message-System Aggregation
  mserr.h               - Message-System Errors
  mseventmgr.h          - Message-System Event Manager
  msic.h                - Message-System Interceptor
  msi.h                 - Message-System Internal
  msix.h                - Message-System Internal-exchange
  msmon.h               - Message-System Monitor services
  mstrace.h             - Message-System Trace
  msutil.h              - Message-System Utilities
  msx.h                 - Message-System Exchange
  pctlx.h               - Process Control Exchange
  phan.h                - Phandle
  props.h               - Properties Object (no STL)
  qid.h                 - Queue Ids
  queue.h               - Queue Object (no STL)
  queuemd.h             - Queue MD (no STL)
  recvq.h               - Receive-Queue (no STL)
  slotmgr.h             - Slot Manager Object (no STL)
  smap.h                - String Map Object (no STL)
  socktrans.h           - Socket Transport Object
  timerx.h              - Timer Exchange
  tracex.h              - Trace Exchange
  trans.h               - Transactions
  transport.h           - Transport
  util.h                - Utilities

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

setup:
1.  Make sure sqconfig is set up correctly and sqgen run.
    (some tests require a minimum number of nodes.  Tests should check
     that number of nodes needed are actually available)

2.  make sure directory containing executables is on PATH
        export PATH=$PATH:$PWD/Linux-x86_64/dbg

3.  Set the following environment variable so the shell looks in the
    current directory instead of $TRAF_HOME/sql/scripts:
       export SQ_SHELL_NOCWD=1

-----------------------------------
Child Exit
-----------------------------------

Description:
   Verifies that when a process is killed its children are also killed.
   Verifies that process death notifications are received for each process.

Discussion:

How to run:
   sqshell
      startup
      exec shell childExit.sub
      shutdown
      quit


Files for this test:
   childExitChild.cxx
   childExitCtrl.cxx
   childExitParent.cxx
   childExit.sub

Relationship to original monitor tests:
   none


Possible improvements:
   1. should verify that at least 2 nodes are configured
      [note: regTestCtrl uses validateConfiguration()]
   2. childExitParent could check how many nodes and create processes on all
   3. problem: if only one node in configuration test does not seem to execute
      but does not report an error

----------------------------------
Multi-node
-----------------------------------

Description:
   General test for monitor functionality. This includes
   Open, Close, NewProcess, ProcessInfo and sync node state
   between nodes.

   Steps:
      1.  starts two server processes $SERV0 and $SERV1 which run
          the "server" executable.
      2.  starts a $CLIENT process which runs the "client" executable.
      3.  The $CLIENT process starts two more server processes $SERV2
          and $SERV3.
      4.  The $CLIENT process interacts with each of the server processes
          sending requests and expecting replies.
      5.  Each of the server processes eventually exits in response
          to a request from the client
      6.  The $CLIENT expects server death messages from $SERV2 and $SERV3
      7.  The $CLIENT process exits

Discussion:

How to run:
   sqshell
      startup
      exec shell multiNode.sub
      shutdown
      quit

Files for this test:
   client.cxx
   server.cxx
   multiNode.sub

Relationship to original monitor tests:
   Replaces the first part of test1.sub

Possible improvements:
   none

-----------------------------------
regTest
-----------------------------------

Description:
   Exercise the various monitor registry capabilities:
   1. Set registry entries of various types
   2. Retrieve specific registry values of various types
   3. Retrieve all registry values of a given type
   4. Verify that configuration change notices are generated as expected
   5. Verify that "continuation" feature of the "get" request works.

Discussion:

How to run:
   sqshell
      startup
      exec shell regTest.sub

Files for this test:
   regTestCtrl.cxx
   regTest.sub

Relationship to original monitor tests:
   Replaces test4.sub

Possible improvements:
1.  Verify that can get all expected cluster wide values from each node
2.  Verify that can get all "process" values for a process from each node
3.  Make sure set of any already set value overwrites the value
4.  Node configuration data should only be local to a node


-----------------------------------
deathNotice
-----------------------------------


Description:
   Exercises ability to register and cancel process death notices
   from non parent process.

   Part 1:
      Repeats the following several times:
      a. Creates a child process and sends commands to it so that
         it registers death notice interest in several processes.
      b. Sends a command so the created process exits normally.
   Part 2:
      a. opens each of the 5 server processes
      b. sends a death notification request to the monitor for each of the
         5 processes.  There is a transaction id associated with some of
         the notices.
      c. Cancels some of the notices.  Transaction id is used.
      d. sends kill request to monitor for 4 of the processes
      e. waits for death notices to arrive.
      f. verifies that the expected death notices are received.
   Part 3:
      a. Creates 3 child processes and sends commands to them for
         them to register process death interest in a specific process
      b. Kills the process for which the 3 child processes registered
         death interest
      c. Sends commands to the three child processes to find out
         whether or not they received death notices.
      d. Reports results.

Discussion:
   Part 1 of the test exercise the monitor's ability to register death
   notices and also to unregister interest when a process exits.  Note
   that the test cannot verify that the monitor actually unregistered
   interest.  A debugger can be attached to the monitor to verify this
   and this was done during test development.

   Part 2 of the test also registers interest in death notices.  It
   does this by using the "death notice" flag during an "open" request
   and by using the "notice" request.   It verifies correct behavior
   by ensuring that the expected death notices are delivered by the monitor.
   Note that for part 2 of the test to exercise the intended monitor
   capabilities it cannot create the worker processes.   That's because
   the monitor has a different mechanism for notifying the parent
   process when a child process exits.

   Part 3 of the test verifies the ability of multiple separate
   processes to receive a death notice for the same process.

How to run:
   sqshell
      startup
      exec shell deathNotice.sub

   If "Test PASSED" is output then the test passed.

   To see the detailed steps executed by the test edit the
   deathNotice.sub script and add "-t" to the end of the final "exec" command.

Files for this test:
   deathNotice.cxx
   deathNotice.h
   deathNotice.sub
   deathUnreg.cxx

Relationship to original monitor tests:
   Replaces test6.sub (and includes new test capabilities)

-----------------------------------
persistentProc
-----------------------------------

Description:
   Exercises the monitor persistent process capability.

Discussion:
   The test performs the following steps:
      - Configures persistent attributes in the registry for process $ABC
      - Starts $ABC on specific node
      - Kills $ABC which is restarted is the same node it was originally
        created
      - Kills $ABC again - it should not be restarted because the configured
        number of retries has been exceeded
      - Starts $ABC on specific node again
      - Downs the node where $ABC was started
      - Expects $ABC to be restarted on the other PERSISTENT_ZONES configured
      - Kills $ABC again - it should not be restarted

   When the last shell exits, there should be no Trafodion processes running

How to run:
   sqshell
      startup
      exec shell persistentProc.sub

   If "Test PASSED" is output then the test passed.

   To see the detailed steps executed by the test edit the
   persistentProc.sub script and add "-t" to the end of the final "exec"
   command.

Relationship to original monitor tests:
   Replaces test10.sub

Additional test ideas:
   - verify "number of retries" counter gets reset if wait longer than
     the configured "retry reset time"
   - exercise additional variations on restarting

-----------------------------------
tmSync
-----------------------------------

Description:
   Test TM sync requests with and without collisions

How to run on a virtual cluster:
   sqshell
      startup
      exec shell tmSyncVirtual.sub

Relationship to original monitor tests:
   Replaces test8.sub

Discussion:
   sub-test 1:
      similar to sub-test 7 but spare node is available
      every node that participates should commit transaction

      all start transaction, 1 dies, spare node activated
      (funky numbers depending on when node goes away)
      total 5
      total committed 5

   sub-test 3:
      only one tm starts 2 phase protocol for commit
      other nodes always reply "commit"

   sub-test 4:
      node 1 starts transaction, no others do
      all 6 abort the transaction

   sub-test 5:
      each tm starts 10 transactions
      all commit transactions
      total transactions = 60
      commits = 60

   sub-test 6:
      similar to sub-test 5 but:
         each tm starts 10 transactions
         only 1 monitor's transaction is committed, other 5 aborted
         commit should be 10
         abort should be 50
         total should be 60

   sub-test 7:
      all nodes start 1 transaction
      1 node's transaction is comitted, others are aborted
      one node goes down and no spare is available
      abort 4, commit 1, total 5


To do:
1) need to verify on real cluster
2) add real-cluster tests (as run originally using test8.sub.sn,
   test8-8.sub.sn, test8-10.sub.sn) [1/6/12: waiting for fix for
   spare node startup problem]

-----------------------------------
spx test
-----------------------------------

Description:
   Verify monitor capabilities for SPX process type (SeaPilot Proxy Process)

Discussion:
   The test performs the following steps:
   1.  Verifies that the configuration of Trafodion nodes is sufficient
       for the test.
   2.  Verify ability to start an SPX process on each of the physical nodes
   3.  Verify that if an SPX process dies each of the other SPX
       process receives a process death notification.
   4.  Verify that can only start one SPX process on a given logical node.
   5.  Verify that cannot start an SPX process on a logical node that
       shares a physical node with another logical node where an SPX
       process is running.

How to run:
   sqshell
      startup
      exec shell spxTest.sub

Relationship to original monitor tests:
   Replaces test11.sub (and includes new test capabilities)

-----------------------------------
process creation test
-----------------------------------

Description:
   Verify various process creation options

Discussion:
   The test performs the following steps:
   1. Creates processes
      one same node as parent, waited
      on different node from parent, waited
      one same node as parent, no-waited
      on different node from parent, no-waited
   2. Kills created processes
   3. Verifies that process creation notices are received for no-waited
      process creates
   4. Verifies that processes are created on the specified node
   5. Verifies that death notices are received


How to run:
   sqshell
      startup
      exec shell procCreate.sub

Relationship to original monitor tests:
   none


===========================================================================
Old/new test correspondence
===========================================================================

Original monitor test    New self-checking test
   test1.sub               multiNode (for 1st part of test1)
   test2.sub
   test3.sub
   test4.sub               regTest
   test5.sub
   test6.sub               deathNotice
   test7.sub
   test8.sub               tmSync
   test10.sub              persistentProc
   test11.sub              spxCtrl
   test12.sub
      ---                  childExit

===========================================================================
Tracing
===========================================================================

To enable trace statements in clio.cxx
   SQ_LOCAL_IO_SHELL_TRACE=1

trace output goes to:
   $MPI_TMPDIR/shell.trace.<pid>

To enable tracing in the test:
   add -t to command line
output goes to stdout


To enable tracing for shell:

   Use command line arguments when starting shell or set environment
   variables.  Refer to comments in shell.cxx.

To trace sub-shells started by shell, add trace environment settings
to mon.env:
   SHELL_TRACE_CMD=1
   SHELL_TRACE_INIT=1
   SHELL_TRACE_LIO=1
   SHELL_TRACE_ENTRY_EXIT=1


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

##################
Monitor unit tests
##################

Scripts:
  o runtest - driver test script for all test

    Execute 'runtest' with no run time options to display usage:
    
    Example:

    runtest { -cluster | -virtual } [ -nogen | -trace | -test <num> ]

    Where: <num> is one of the following tests:
             1     - Child Exit
             2     - Multi-Node
             3     - Registry
             4     - Death Notice
             5     - Persistent Process
             6     - DTM Process
             7     - Process Create
             8     - Node down before startup

  o monpkillall   - to forcibly terminate test programs 
  o monpstat      - test programs process status
  o monshell      - shell wrapper supporting virtual nodes configuration
  o montestgen    - to compile test configuration with virtual nodes
  o montestgen.pl - supports compile of test configuration with virtual nodes

Configuration files used by 'runtest':

  o sqconfig.monitor.cluster  (-cluster run time option)
    - Update the 'node section' in this file to execute tests in a 'real cluster'
  o sqconfig.monitor.virtual  (-virtual run time option)
    - No changes required
  o sqconfig.persist
    - No changes required 
  o sqconfig.persist.dtm
    - No changes required 

Setup:

1.  Build the test programs before running 'runtest'
2.  Make sure directory containing executables is on PATH
        export PATH=$PATH:$PWD/Linux-x86_64/dbg

Test results:

  o Each test displays a PASSED or FAILED indicating the result of the test
  o In addition, after each test is executed, the instance is 'shutdown' and
    a check is made to determine that the 'shutdown' stopped all
    processes with a PASSED or FAILED indicating that the shutdown
    was successful or not.

-----------------------------------
Child Exit
-----------------------------------

Description:
   Verifies that when a process is killed its children are also killed.
   Verifies that process death notifications are received for each process.

Discussion:

How to run:
   runtest { -cluster | -virtual } [-nogen] [-trace] -test 1 

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
   runtest { -cluster | -virtual } [-nogen] [-trace] -test 2

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
   runtest { -cluster | -virtual } [-nogen] [-trace] -test 3

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
   runtest { -cluster | -virtual } [-nogen] [-trace] -test 4

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
   runtest { -cluster | -virtual } [-nogen] [-trace] -test 5

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
DTM Process
-----------------------------------

Description:
   Exercises the monitor DTM process management rules.

Discussion:
   The test performs the following steps:
     1.  Verify ability to start an DTM process on each of the logical nodes
     2.  Verify that if an DTM process dies each of the other DTM
         process DOES NOT receiv a process death OR tmRestarted notification.
     3.  Verify that DTM as a persistent process when restarted
         sends TmReady request to monitor.
     4.  Verify that only one DTM process can be started on a logical node.
     5.  Verify that DTM as a persistent process and exceeds restart limits
         brings node down.

   When the last shell exits, there should be no Trafodion processes running

How to run:
   runtest { -cluster | -virtual } [-nogen] [-trace] -test 6

   If "Test PASSED" is output then the test passed.

Relationship to original monitor tests:
   None

Additional test ideas:
   - none

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
   runtest { -cluster | -virtual } [-nogen] [-trace] -test 7

Relationship to original monitor tests:
   none

-----------------------------------
Shutdown and node down before startup test
-----------------------------------

Description:
   Verify that process cleanup occurs in monitor when process is created
   but has not sent its 'startup' message request to the monitor when
   the instance is shutdown or the node goes down.

Discussion:
   The test performs the following steps:
   1. Creates processes


How to run:
   runtest { -cluster | -virtual } [-nogen] [-trace] -test 7

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
   test8.sub               
   test10.sub              persistentProc
   test11.sub              
   test12.sub
      ---                  childExit
      ---                  DTM process
      ---                  Node down before startup

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


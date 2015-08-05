-- @@@ START COPYRIGHT @@@
--
-- Licensed to the Apache Software Foundation (ASF) under one
-- or more contributor license agreements.  See the NOTICE file
-- distributed with this work for additional information
-- regarding copyright ownership.  The ASF licenses this file
-- to you under the Apache License, Version 2.0 (the
-- "License"); you may not use this file except in compliance
-- with the License.  You may obtain a copy of the License at
--
--   http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing,
-- software distributed under the License is distributed on an
-- "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
-- KIND, either express or implied.  See the License for the
-- specific language governing permissions and limitations
-- under the License.
--
-- @@@ END COPYRIGHT @@@
####################################
# Introduction
####################################

Exeperf is a test module designed for the executor performance regression tests. This test module should be run with exclusive usage of the test machine so that the performance numbers won't be interfered with other tasks. The baseline numbers are defined in perfbaseline.dat. The default baseline values are set to large numbers so the tests will always pass. For individual developers who want to find out if your code changes have caused any regressions in the executor, here is what you can do:

1. Run exeperf test module WITHOUT your code changes to generate the baseline numbers.
2. Run "runregr -archive". This will update the baseline numbers using the results generated in the last step. Without this step the default values in perfbaseline.dat will be used as the baseline numbers.
3. Run exeperf test module WITH your code changes. The results will be compared with the baseline numbers to detect any regressions caused by your code changes.


####################################
# How to run exeperf test module
####################################

1. Go to .../regress/exeperf directory.

2. Edit parameters.txt file for following user defined parameters:

  - ExeperfLocationOrders: the exeperf test database contains two tables, orders and lineitem, that will be created and loaded from raw data files during the setup phase of the test. This parameter specifies the location of the raw data file for table orders.
  - ExeperfLocationLineitem: specifies the location of the raw data file for table lineitem.
  - ExeperfPartition1: the exeperf test tables (orders and lineitem) are created across four disk partitions. This parameter specifies the first disk partition. Please note that '$' is a special character in shell programming and it needs to be escaped with a preceding backslash. Example: \$FC0000.
  - ExeperfPartition2: specifies the second disk partition.
  - ExeperfPartition3: specifies the third disk partition.
  - ExeperfPartition4: specifies the fourth disk partition.

3. Copy your test module to the destination machine. You can use the nightly build script to do it:

  NightlyRun.ksh -buildParamFile <your_build_param_file> -targetYos -doYosCopyRegr

4. Logon to the destination machine and run .../debug/SetupDebugOshScript.yos.

5. go to .../debug/regress/exeperf directory.

6. If this is the first time you run the experf test module, you need to setup the test database first:

  runregr -setup

7. Now run the exeperf regression tests:

  runregr

8. When tests finish there is a summary table indicating which tests passed and which failed.

Note: above steps described how to run exeperf test module for the debug build. You can also run exeperf test module for the release build in similar fashion. Just use the corresponding release directory and scripts instead.


####################################
# How to update baseline numbers
####################################

The baseline numbers for the test execution time are saved in perfbaseline.dat and then inserted into the baseperf table. You can update those numbers with your own test execution time:

  runregr -archive

<!--
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
-->
This page describes the Trafodion test suites and their usage.

# Component Tests
Trafodion comes with several component-specific testing libraries.

## SQL Core
The SQL core components are written in a combination of C++ and Java. 

You should ensure that the current set of regression tests pass each time you add or modify a SQL feature.

If adding a new feature, then check that it is either covered by an existing regression test or add a new test to an existing test suite.

### Test Suites
**Location**: ```core/sql/regress```

Directory              | Usage
-----------------------|---------------------------------------------------------------------------
**```catman1```**      | Tests the Catalog Manager.
**```charsets```**     | Tests Character Sets.
**```compGeneral```**  | Compiler test suite; tests optimizer-specific features.
**```core```**         | Tests a subset/sample of all features from all the test suites.
**```executor```**     | Tests the SQL Executor.
**```fullstack2```**   | Similar to core but a very limited subset.
**```hive```**         | Tests HDFS access to Hive tables.
**```newregr```**      | Unused/Saved repository for some unpublished features. These are not run.
**```qat```**          | Tests basic DDL and DML syntax.
**```seabase```**      | Tests JNI interface to HBase.
**```tools```**        | Regression driver scripts and general regression scripts.
**```udr```**          | Tests the User Defined Routines (UDR) and TMUDF functionality.

### Check Test Results
On completion, the test run prints out a test summary. All tests should pass, or pass with known diffs.

Test results are written to the **```runregr-sb.log```** file in each component's directory. Therefore, you can check the test results after the fact as follows:

    cd $MY_SQROOT/rundir
    grep FAIL */runregr-sb.log
 
A successful test run shows no failures.

### Run Full Test Suite
This suite tests:

* SQL Compiler
* SQL Executor
* Transactions
* Foundation


Do the following:

    cd $MY_SQHOME
    . ./sqenv.sh
    cd $MY_SQROOT/../sql/regress
    tools/runallsb

### Run Individual Test Suites
You can select individual test suites as follows:

    cd $MY_SQHOME
    . ./sqenv.sh
    cd $MY_SQROOT/../sql/regress
    tools/runallsb <suite1> <suite2>

### Running an Individual Test
#### If You've Already Run the Test Suite
If you have already run the suite once, then you will have all your directories set up and you can run one test as follows:

    cd $MY_SQROOT/../sql/regress/<suite>
    # You can add the following two exports to .bashrc or .profile for convenience
    export rundir=$MY_SQROOT/rundir
    export scriptsdir=$MY_SQROOT/../sql/regress
    # run the test
    cd $rundir/<suite>
    $scriptsdir/<suite>/runregr -sb <test>

**Example**

    cd $rundir/executor
    $scriptsdir/executor/runregr -sb TEST130

#### If You've Not Run the Test Suite
If you have not run any regression suites so far, then you will not have the required sub directories set up. You manually create them for each suite you want to run.

    cd $MY_SQROOT/../sql/regress/<suite>
    # You can add the following two exports to .bashrc or .profile for convenience
    export rundir=$MY_SQROOT/rundir
    export scriptsdir=$MY_SQROOT/../sql/regress
    mkdir $rundir
    cd $rundir
    # <suitename> should match the name of each directory in $scriptsdir
    mkdir <suitename>
    # run the test
    cd $rundir/<suite>
    $scriptsdir/<suite>/runregr -sb <test>

### Detecting Failures
If you see failures in any of your tests, you want to try running that suite or test individually as detailed above.
 
Open up the DIFF file and correlate them to the LOG and EXPECTED files.
 
* DIFF files are in **```$rundir/<suite name>```**.
* LOG files are in **```$rundir/<suite name>```**.
* EXPECTED files are in **```$scriptsdir/<suite name>```**.
 
To narrow down the failure, open up the test file (for example: **```TEST130```**) in **```$scriptsdir/executor```**. 

Recreate the problem with a smaller set of SQL commands and create a script to run from **```sqlci```**. If it's an issue that can be recreated only by running the whole suite, you can add a line to the test just before the command that fails to include a **```wait```** or a **```sleep```** **```sh sleep 60```** will make the test pause and give you time to attach the **```sqlci```** process to the debugger. (You can find the PID of the **```sqlci```** process using **```sqps```** on the command line)
 
Introducing a **```wait```** in the test will wait forever until you enter a character. This is another way to make the test pause to attach the debugger to the **```sqlci```** process.    

### Modifying an Existing Test
If you would like to add coverage for your new change, you can modify an existing test.

Run the test after your modifications. If you are satisfied with your results, you need to modify the **```EXPECTED<test number>```** file to reflect your new change. The standard way to do it is to copy the **```LOG<test number>```** file to **```EXPECTED<test number>```** file.

## Database Connectivity Services (DCS)
The DCS test suite is organized per the Maven standard. 

## JDBC T4
The code is written in Java, and is built and unit tested using Maven. The test suite organization and use follow Maven standards.

Instructions for setting up and running the test can be found in source tree at **```dcs/src/test/jdbc_test```**.

## ODBC Tests
The code is written for the Python 2.7 [```unittest```](https://docs.python.org/2/library/unittest.html) framework. 

It is run via the **```Testr```** and **```Tox```**. 

    cd dcs/src/test/pytests
    ./config.sh -d <host>t:<port> -t <Location of your Linux ODBC driver tar file>
    tox -e py27

Further instructions for setting up and running the test can be found in source tree at **```dcs/src/test/pytests```**.

# Functional Tests

## Phoenix
The Phoenix tests provides basic functional tests for Trafodion. These tests were originally adapted from their counterpart at salesforce.com.

The tests are executed using Maven with a Python wrapper. You can run them the same way on your own workstation instance just like the way Jenkins runs them. Do the following:

<!-- This part is done in raw HTML because it's too complex to do this level of formatting in markdown. -->
<ol>
   <li>Prior to running Phoenix tests, you need to bring up your Trafodion instance and DCS. You need to configure at least 2-4 servers for DCS. The tests need at least two mxosrvrs as they make two connections at any given time. We recommend configuring DCS with four mxosrvrs since we have seen situations that mxosrvrs do not get released in time for the next connection if there are only two mxosrvrs.</li>
  <li><p>Run the Phoenix tests from source tree</p>
       <p style="text-indent=20px">
          <pre>
cd tests/phx
phoenix_test.py --target=&lt;host&gt;:&lt;port&gt; --user=dontcare --pw=dontcare --targettype=TR --javahome=&lt;jdk&gt; --jdbccp=&lt;jdir&gt;/jdbcT4.jar</pre>
          </p>
       <p style="text-indent=20px">
         <ul>
           <li><strong>&lt;host&gt;</strong>: your workstation name or IP address.</li>
           <li><strong>&lt;port&gt;</strong>: your DCS master port number.</li>
           <li><strong>&lt;jdk&gt;</strong>:  the directory containing the jdk1.7.0_21_64 or later version of the JDK.</li>
           <li><strong>&lt;jdir&gt;</strong>: the directory containing your JDBC T4 jar file. (export/lib if you downloaded a Trafodion binary package.)</li>
         </ul>
       </p>
  </li>
  <li><p>Analyze the results. The test results can be found in <strong><code>phoenix_test/target/surefire-reports</code></strong>. If there are any failures, they would come with file names and line numbers.</p>
   <p>The source code can be found in <strong><code>phoenix_test/src/test/java/com/trafodion/phoenix/end2end</code></strong>.</p> 
   <p>These are JDBC tests written in java.</p>
  </li>
 </ol>
    
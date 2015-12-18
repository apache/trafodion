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
This page describes how to add or modify Trafodion tests and/or test suites. Please refer to the [Contribute](contribute.html) page for information about other ways to contribute to the Trafodion project.

**Note**: The [Test](testing.html) page describes how the different Trafodion test libraries are organized and how you run different test. You should familiarize yourself with that page before modifying the Trafodion tests.

# SQL Tests
The SQL tests are located in: **```core/sql/regress```**. Please refer to the [Test](testing.html) page for information about each directory. Below, each of these directories is referred to as **```$scriptdir```**.

The default output from **```core/sql/regress/tools/runallsb```** is located in **```$MY_SQROOT/rundir```**. This directory is referred to as **```$rundir```** below.

Files associated with individual tests use the following naming convention: **```\<Type\>\<number\>```**. The files are organized as follows:

Type       | Usage                              | Location
-----------|------------------------------------|---------------------------------------------
TEST       | The test itself.                   | **```$scriptdir/<test suite>```** 
EXPECTED   | The expected output from the test. | **```$scriptdir/<test suite>```**
FILTER     | Filters out variable results (for example, timestamps) from test results to ensure consistent runs. | **```$scriptdir/<test suite>```**
LOG        | The output from running the test.  | **```$rundir/<test suite>```**
DIFF       | Diff between LOG and EXPECTED.     | **```$rundir/<test suite>```**

## Modify a Test
The comments in a test provides information of what it does. Make your changes and then do the following:

* Run the test.
* Verify that the LOG output is as desired.
* Copy the LOG output to EXPECTED.
* Rerun the test and verify that the test passes.

### Output with Variable Data
If there are time stamps or generated names that may vary from run to run or from user to user, then you need to create a ```FILTER\<test number\>``` file. This will filter out the variable portions of the test results so the test results are consistent.
 
Example filter file: **```core/sql/regress/core/FILTER024```**  

**```core/sql/regress/tools```** contains generic filter files. These cover general/common variables in test output. You will need to add a new test specific filer if it’s specific to your new test output only. 

### Check In Changes
Check in the TEST and EXPECTED files in **```$scriptsdir```**.

## Create New Test
Creating a test is just a variation of [Modify a Test](#Modify_a_Test). The test should be associated with a test suite that covers the component you want to test. Please contact the [Trafodion Developer List](mail-lists.html) if you need help chosing the correct test suite.

Once you've chosen the test suite and, therefore, the test source directory, you do the following:

* Add the new test file to the test file in the selected test-suite directory.
    * Use the naming convention found in the directory. (Normally: **```TEST\<nnn\>```**)
    * Refer to **```core/sql/regress/runregr_\<test suite\>.ksh```** to verify the naming convention used.
* The test DDL should use the schema name associated with the test suite.
    * Refer to other test suites for examples.
* Test and validate as described in [Modify a Test](#Modify_a_Test) above. 

## Create New Test Suite
The test suites cover different SQL components. Most components should be covered with the existing test suites.

Do the following to create a new test suite:

* Create a new directory with the suite name under **```core/sql/regress```**; for example: **```\<new test suite\>```**.
* Refer to [Create New Test](#Create_New_Test) for instructions on how to add tests to the new test suite.
* Add a test driver in **```core/sql/regress/tools```** directory with name **```runregr_\<new test suite\>.ksh```**. (Just copy and modify one of the other test drivers.)
* Add the new test suite to **```core/sql/regress/runallsb/TEST_SUBDIRS```**.
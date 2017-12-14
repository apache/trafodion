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

===============================
Python ODBC Tests for Trafodion
===============================

Python ODBC tests for Trafodion.


Running Tests
=============

* First, add the bin directory of Python 2.7 built with UCS2 support to your PATH ::

    export PATH=/PATH_to_Python2.7_Home/bin:$PATH
    
* Secondly, run ``config.sh`` to configure the Python ODBC test.
  If you need help on using this command then type ``./config.sh -h``. 
  For example, if the unixODBC driver manager is installed in the default location and the Trafodion ODBC driver
  is not yet installed in the ``odbc_driver`` sub-directory then run the following ::
  
    ./config.sh -d <Fully_Qualified_Domain_Name_Of_Machine_With_Trafodion>:<Port> -t /Absolute_PATH_to/TRAF_ODBC_Linux_Driver_64.tar.gz

  This will configure your test and install the Trafodion ODBC driver in the ``odbc_driver`` sub-directory.

  .. note:: Do NOT put the TRAF_ODBC_Linux_Driver_64.tar.gz in the ``odbc_driver`` subdirectory. The contents of this sub-directory is 
     removed every time config.sh is run with the -t option!
    
* Next run the `tox <http://tox.readthedocs.org/en/latest/>`_ command to create a python virtual environment for
  your test and automatically kick off the tests using `testr <https://testrepository.readthedocs.org/en/latest/MANUAL.html>`_ ::
  
    tox -e py27


Debugging Tests with testr (Basic)
==================================

Assuming that ``config.sh`` has been run and the python virtual environment has been set up, do the following to debug the tests

1. Activate the Python virtual environment ::

    source .tox/py27/bin/activate
    
2. Source in the environment variables needed ::

    source env.sh
    
3. See what tests were failing ::

    testr failing

4. Make some fixes and rerun the failing tests ::
    
    testr run --failing 

   OR if you want to run some specific tests use the command ``tox -e py27 -- <Test_Name_as_Listed_by_testr_seperated_by_space>``

   For example, : ::

    tox -e py27 -- test_p2.ConnectTest.test11 test_p2.ConnectTest.test12
    
5. Repeat Steps 3-4 until there are no failures.

6. When debugging is finished deactivate the Python virtual environment ::

    deactivate


Debugging Tests with testr (Advanced)
=====================================

Assuming that ``config.sh`` has been run and the python virtual environment has been set up, do the following to debug the tests

* Activate the Python virtual environment ::

    source .tox/py27/bin/activate

* Source in the environment variables needed ::

    source env.sh
    
* Look for last testr run number in the directory .testrepository.  Look for a file with the highest number in this directory.
  If the test has only been run once then the run number should be 0. ::

    ls -l .testrepository

* Look at the file .testrepository/$LAST_TEST_RUN_NUMBER and find the test that failed.  Then under the tags section 
  you should see something like ::
  
    tags: worker-0
    
  With this worker name we can extract the list of tests that ran in that test run on that worker. ::
  
    testr last --subunit | subunit-filter -s --xfail --with-tag=worker-0 | subunit-ls > slave-0.list
    
  Using this test list we can run that set of tests in the same order that caused the failure with : ::
  
    testr run --load-list=slave-0.list
    
* When debugging is finished deactivate the Python virtual environment ::

    deactivate
    
    
Other Useful testr Commands
===========================

* List all the tests that ran ::

    testr list-tests
    
* Run only one test : ``testr run <Test_Name_as_Listed_by_testr>``.  For example, ::

    testr run test_p2.ConnectTest.test11

* Get all test results of the last test run in csv format ::

    testr last --subunit | subunit-1to2 | subunit2csv
    
* Get all test results of the last test run in pyunit format ::

    testr last --subunit | subunit-1to2 | subunit2pyunit
    
* Get all test results of the last test run in JUnit format ::

    testr last --subunit | subunit-1to2 | subunit2junitxml
    

Adding New Tests
================

* Make sure the file's name follows the naming format : ``test_*.py``
* Add any new required Python packages to the file ``test-requirements.txt``


Known Issues
============

* If the Trafodion sqf/sqenv.sh file has been sourced into your environment it is likely the test will run into the error :
  ``[unixODBC][Driver Manager]Can't open lib '/ABSOLUTE_PATH/TO/libtrafodbc_drvr64.so' : file not found``


Other Resources
===============

* `Testr - OpenStack <https://wiki.openstack.org/wiki/Testr>`_



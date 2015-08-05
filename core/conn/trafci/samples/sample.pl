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
################################################################################
##  Include the library path here
################################################################################
use lib '##TRAFCI_PERL_LIB_CLASSPATH##';
use Session;

################################################################################
##  create a new session
################################################################################
$sess = Session->new();
$sess2 = Session->new();

################################################################################
##  connect to the database
##
##  To pass in a rolename during connect, use the following API
##  $sess->connect("USERNAME",'PASSWORD',"ROLENAME", "HOSTNAME","PORTNUMBER","DSNAME");
################################################################################
$sess->connect("user-name","pass-word","host-name","22500","Admin_Load_DataSource");
$sess2->connect("user-name","pass-word","host-name","22500","Admin_Load_DataSource");

################################################################################
## execute the script file
################################################################################
$retval=$sess->executeScript("../samples/sample.sql","../samples/sample.sql.log");
print $retval;

################################################################################
## Execute sample queries
################################################################################
$retval=$sess->execute("set schema TRAFODION.CI_SAMPLE");
print $retval;
$retval=$sess->execute("select count(*) from employee");
print $retval;

print "\n\nSession 1:  Disconnecting first session. \n\n";
$sess->disconnect();

$retval2=$sess2->execute("set markup xml ");

print "Session 2:  Count from job ---->\n\n";
$retval2=$sess2->execute("select count(*) from TRAFODION.CI_SAMPLE.job");
print $retval2;

################################################################################
### disconnect the session
################################################################################
print "\n\n Session 2:  Disconnecting second session. \n\n";
$sess2->disconnect();

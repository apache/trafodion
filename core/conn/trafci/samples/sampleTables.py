#!/usr/bin/jython
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
import os
import sys
## Modify this path 
sys.path.append("##TRAFCI_PYTHON_LIB_CLASSPATH##")
import Session
import re

# removes whitespace and \r\n
def trim( tmpstr ):
    return tmpstr.strip(' \r\n')
##################################


########################################################################
# create a session object
########################################################################
sess = Session.Session()

########################################################################
# Connect to the database
########################################################################
x=sess.__connect__("USERNAME","PASSWORD","HOSTNAME","PORTNUMBER","DSNAME")

########################################################################
# Execute sample queries
#    __execute takes the query string as argument
########################################################################
sess.__execute__("set schema TRAFODION.CI_SAMPLE")
sess.__execute__("set markup csv")

strFilter = "*"
# get filter from args
if(len(sys.argv)>1):
    strFilter = sys.argv[1]


result=sess.__execute__("get tables ")

if(result.find("java.lang.NullPointerException") != -1):
    sys.exit(1)
    
result=result.split('\n')

output_counter = 0
for i in result:
    i = trim(i)
    if(i != ''):
        count = sess.__execute__("select count(*) from " + i )

        if(count.find("*** ERROR") == -1):                    
            output_counter += 1

            # print heading if we are on the first item
            if(output_counter == 1):
                print "TableName".ljust(36), "Row Count".ljust(15)
                print "-"*36,"-"*15

            if(len(i) > 36):
                print i
                print " "*36, trim(str(count)).ljust(15)
            else:
                rstr = trim(str(count));
                if(len(rstr)>15):
                    rstr = rstr[:15]
                print i.ljust(36), rstr.ljust(15)

       
if(output_counter == 0):
    print "No tables found."

########################################################################
# disconnect the session
########################################################################
sess.__disconnect__()
del sess
sess=None

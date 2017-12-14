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


########################################################################
# create a session object
########################################################################
sess = Session.Session()

########################################################################
# Connect to the database
########################################################################
x=sess.__connect__("USERNAME","PASSWORD","HOSTNAME","PORTNUMBER","DSNAME")

########################################################################
# Execute sample scripts
#      __executeScript__ takes the script file name & log file name as 
#         arguments
########################################################################
sess.__executeScript__("../samples/sample.sql","../samples/sample.sql.log")

########################################################################
# Execute sample queries
#    __execute takes the query string as argument
########################################################################

dropEmpTable     = "drop table TRAFODION.CI_SAMPLE.employee";
dropDeptTable    = "drop table TRAFODION.CI_SAMPLE.dept"
dropProjectTable = "drop table TRAFODION.CI_SAMPLE.project"
dropJobTable     = "drop table TRAFODION.CI_SAMPLE.job"
dropEmpView      = "drop view TRAFODION.CI_SAMPLE.emplist"
dropMgrView      = "drop view TRAFODION.CI_SAMPLE.mgrlist"
dropEmpIndex     = "drop index TRAFODION.CI_SAMPLE.xempname"
dropSchema       = "drop schema TRAFODION.CI_SAMPLE"

createSchema   = "create schema TRAFODION.CI_SAMPLE";
createEmpTable = "CREATE TABLE TRAFODION.CI_SAMPLE.employee (        "+ \
       "  empnum          NUMERIC (4) UNSIGNED          "+ \
       "                  NO DEFAULT                    "+ \
       "                  NOT NULL                      "+ \
       " ,first_name      CHARACTER (15)                "+ \
       "                  DEFAULT ' '                   "+ \
       "                  NOT NULL                      "+ \
       " ,last_name       CHARACTER (20)                "+ \
       "                  DEFAULT ' '                   "+ \
       "                  NOT NULL                      "+ \
       " ,deptnum         NUMERIC (4)                   "+ \
       "                  UNSIGNED                      "+ \
       "                  NO DEFAULT                    "+ \
       "                  NOT NULL                      "+ \
       " ,jobcode         NUMERIC (4) UNSIGNED          "+ \
       "                  DEFAULT NULL                  "+ \
       " ,salary          NUMERIC (8, 2) UNSIGNED       "+ \
       "                  DEFAULT NULL                  "+ \
       " ,PRIMARY KEY     (empnum)                      "+ \
       " )                                              ";
      
createEmpView= "CREATE VIEW TRAFODION.CI_SAMPLE.emplist "+ \
          "  AS SELECT             "+ \
          "    empnum              "+ \
          "    ,first_name            "+ \
          "    ,last_name             "+ \
          "    ,deptnum            "+ \
          "    ,jobcode            "+ \
          "  FROM                  "+ \
          "   TRAFODION.CI_SAMPLE.employee        ";
                         

createEmpNameIndex="CREATE INDEX xempname             "+ \
         "   ON TRAFODION.CI_SAMPLE.employee ( "+ \
         "   last_name           "+ \
         "   ,first_name            "+ \
         "   )             "+ \
         "   ;             "
                            
          
createDeptTable= "CREATE TABLE TRAFODION.CI_SAMPLE.dept     (               "+ \
      "   deptnum         NUMERIC (4) UNSIGNED      "+ \
      "                   NO DEFAULT             "+ \
      "                   NOT NULL            "+ \
      "  ,deptname        CHARACTER (12)         "+ \
      "                   NO DEFAULT             "+ \
      "                   NOT NULL            "+ \
      "  ,manager         NUMERIC (4) UNSIGNED                 "+ \
      "                   NO DEFAULT             "+ \
      "                   NOT NULL            "+ \
      "  ,rptdept         NUMERIC (4) UNSIGNED      "+ \
      "                   DEFAULT 0           "+ \
      "                   NOT NULL            "+ \
      "  ,location        VARCHAR (18)        "+ \
      "                   DEFAULT ' '            "+ \
      "                   NOT NULL            "+ \
      "  ,PRIMARY KEY     (deptnum)           "+ \
      "  )                                                     ";
   
createMgrView = "CREATE VIEW TRAFODION.CI_SAMPLE.mgrlist  (     "+ \
       "      first_name                      "+ \
       "     ,last_name                    "+ \
       "     ,department                      "+ \
       "     )                       "+ \
       " AS SELECT                         "+ \
       "   first_name                      "+ \
       "   ,last_name                      "+ \
       "   ,deptname                    "+ \
       " FROM                        "+ \
       "   TRAFODION.CI_SAMPLE.dept dept               "+ \
       "  ,TRAFODION.CI_SAMPLE.employee emp               "+ \
       " WHERE                       "+ \
       "  dept.manager = emp.empnum                     ";
                 

createJobTable = "CREATE TABLE TRAFODION.CI_SAMPLE.job      (     "+ \
       "       jobcode         NUMERIC (4) UNSIGNED       "+ \
       "                       NO DEFAULT           "+ \
       "                       NOT NULL             "+ \
       "      ,jobdesc         VARCHAR (18)         "+ \
       "                       DEFAULT ' '             "+ \
       "                       NOT NULL             "+ \
       "      ,PRIMARY KEY     (jobcode)            "+ \
       "      )                                           ";                  
                                  
                                 
          
createProjectTable = "CREATE TABLE TRAFODION.CI_SAMPLE.project  (         "+ \
      "   projcode        NUMERIC (4) UNSIGNED         "+ \
      "                   NO DEFAULT                "+ \
      "                   NOT NULL               "+ \
      "  ,empnum          NUMERIC (4) UNSIGNED         "+ \
      "                   NO DEFAULT                "+ \
      "                   NOT NULL               "+ \
      "  ,projdesc        VARCHAR (18)           "+ \
      "                   DEFAULT ' '                        "+ \
      "                   NOT NULL               "+ \
      "  ,start_date      DATE                "+ \
      "                   DEFAULT CURRENT_DATE         "+ \
      "                   NOT NULL               "+ \
      "  ,ship_timestamp  TIMESTAMP              "+ \
      "                   DEFAULT CURRENT_TIMESTAMP          "+ \
      "                   NOT NULL               "+ \
      "  ,est_complete    INTERVAL DAY           "+ \
      "                   DEFAULT INTERVAL '30' DAY          "+ \
      "                   NOT NULL               "+ \
      "  ,PRIMARY KEY     (projcode)                         "+ \
      "                   )                                   ";                       


                            
#Contruct a list of SQL statements to be executed              
queryList = [dropEmpView,dropMgrView,dropEmpIndex,dropEmpTable,dropDeptTable,dropProjectTable,dropJobTable,createSchema,createEmpTable,createEmpView,createEmpNameIndex,createDeptTable,createJobTable,createProjectTable,createMgrView]
print "\n";    
         
for query in queryList:
   print sess.__execute__(query)
   

print sess.__execute__("set schema TRAFODION.CI_SAMPLE")
count=sess.__execute__("select count(*) from TRAFODION.CI_SAMPLE.employee")
print "count: "+count

########################################################################
# disconnect the session
########################################################################
sess.__disconnect__()
del sess
sess=None


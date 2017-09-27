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
#
# Purpose: To test the performance of single subset vs. MDAM
# plans for a simple SQL scan, and to check what plan the 
# optimizer picks. The script prepares, explains 
# and executes a SQL statement for each plan,
# capturing certain details from explain and execute and
# packaging that as data that can be inserted into a SQL table
# for later analysis.
#
# The type of SQL statement expected is select * from t
# where <predicates on t>. That is, the script is just looking
# for a single SCAN node in the explain output.
#
# The script generates a sequence of INSERT statements that
# capture plan and execution time information. It assumes the
# following DDL:
#
#  create table results2
#    (
#      test_run        numeric(17,6) not null,
#      plan_name       varchar(28) not null,
#      statement_text  varchar(4096) not null,
#      prepare_error   varchar(256) not null,
#      execute_error   varchar(256) not null,
#      fragment_type   char(10) not null,
#      executor_preds  varchar(1024) not null,
#      mdam_disjunct   varchar(1024) not null,
#      begin_key       varchar(1024) not null,
#      end_key         varchar(1024) not null,
#      primary key (test_run)
#    );
#
#  create table resultstats2
#    (
#      test_run           numeric(17,6) not null,
#      execution_instance smallint not null,
#      start_time         timestamp(6) not null,
#      end_time           timestamp(6) not null,
#      records_accessed   largeint not null,
#      records_used       largeint not null,
#      hdfs_ios           largeint not null,
#      hdfs_iobytes       largeint not null,
#      hdfs_usecs         largeint not null,
#      primary key (test_run, execution_instance)
#    );
#
#
#
# Flow:
#
# Input: A SQL statement, a file to append output
#
# Start sqlci process
# Do a showshape of the SQL statement, to determine the
# fully-qualified name of the table.
# exit;
#
# For each plan:
#   Start a sqlci process
#   Send the following commands to it:
#   cqd last0_mode 'ON';
#   a cqd or cqs to force a particular plan (or nothing, 
#     if checking the Optimizer's choice)
#   prepare s from <input text>
#   explain s;
#   execute s;
#   display statistics;
#   execute s;
#   display statistics;
#   execute s;
#   display statistics;
#   exit;
#
#   Process the output from sqlci
#   Iterate through the lines
#   Extract: error or success from prepare
#   Extract: various info from explain
#   Extract: error or success from each execute
#   Extract: statistical info from each display
#
# The plans considered are:
# 0. Optimizer choice
# 1. New MDAM costing code choice (CQD MDAM_COSTING_REWRITE 'ON')
# 2. MDAM off (CQD MDAM_SCAN_METHOD "OFF")
# 3-n. MDAM forced (CQS, with varying numbers of columns),
#      varying ESP parallelism on or off for each
#
import os
import sys
import subprocess 
import time
import re
import sets    # not needed?
import argparse  # requires Python 2.7

# global used to enumerate input and log files
sqlciSessionNumber = 0


def sqlciInputFileName():
    # Returns name to use for sqlci input file
    global sqlciSessionNumber
    return 'temp.' + str(os.getpid()) + '.' + str(sqlciSessionNumber) + '.sql'


def sqlciLogFileName():
    # Returns name to use for sqlci input file
    global sqlciSessionNumber
    fileName = 'temp.' + str(os.getpid()) + '.' + str(sqlciSessionNumber) + '.log'
    sqlciSessionNumber = sqlciSessionNumber + 1
    return fileName    


def findFullyQualifiedTableName(statement,retain):
    # Find the fully qualified name of the table referenced
    # in the SQL statement
    inputFileName = sqlciInputFileName()
    logFileName = sqlciLogFileName()

    f = open(inputFileName,'w')
    f.write('log '+logFileName+';\n')
    f.write('showshape\n')
    f.write(statement+';\n')
    f.write('exit;\n')
    f.close()

    # look for "path 'X.Y.Z'"
    pattern = r".*path (?P<name>\'[A-Z0-9_]*\.[A-Z0-9_]*\.[A-Z0-9_]*\').*"
    prog = re.compile(pattern)

    tableName = None

    p1 = subprocess.Popen(["sqlci","-i",inputFileName], stdout=subprocess.PIPE)
    for line in p1.stdout:
        result = prog.match(line)
        if result:
            tableName = result.group('name')
            break


    if not retain:
        os.unlink(inputFileName)
        os.unlink(logFileName)

    return tableName


def processOnePlan(statement,planNumber,tableName,g,retain):
    # Prepare, Explain and Execute the statement for a particular plan

    planCQDorCQS = None
    planName = "Invalid"
    if planNumber == 0:
        planCQDorCQS = None  # optimizer's choice
        planName = "Optimizer choice"
    elif planNumber == 1:
        planCQDorCQS = "CQD MDAM_COSTING_REWRITE 'ON';\n" # new MDAM Costing code
        planName = "New cost choice"
    elif planNumber == 2:
        planCQDorCQS = "CQD MDAM_SCAN_METHOD 'OFF';\n"  # simple scan
        planName = "Simple scan"
    elif planNumber > 2:
        planCQDorCQS = "control query shape "
        depth = (planNumber-1) / 2;
        if (planNumber-1) % 2:
            planCQDorCQS = planCQDorCQS + "esp_exchange("
            planName = "Parallel mdam " + str(depth) + " deep"
        else:
            planName = "Serial mdam " + str(depth) + " deep"
        planCQDorCQS = planCQDorCQS + "scan(path " + tableName + ", mdam forced, mdam_columns(dense"
        while depth > 1:
            planCQDorCQS = planCQDorCQS + ",sparse"
            depth = depth - 1
        planCQDorCQS = planCQDorCQS + "))"
        if (planNumber-1) % 2:
            planCQDorCQS = planCQDorCQS + ");\n"
        else:
            planCQDorCQS = planCQDorCQS + ";\n"

    # create an input file to contain the SQLCI commands

    inputFileName = sqlciInputFileName()
    logFileName = sqlciLogFileName()
    f = open(inputFileName,'w')
    f.write('log '+logFileName+';\n')
    if planCQDorCQS:
        f.write(planCQDorCQS)
    f.write("CQD LAST0_MODE 'ON';\n")
    f.write('prepare s from '+statement+';\n')
    f.write('explain s;\n')
    f.write('execute s;\n')
    f.write('display statistics;\n')
    f.write('execute s;\n')
    f.write('display statistics;\n')
    f.write('execute s;\n')
    f.write('display statistics;\n')
    f.write('exit;\n')
    f.close()    

    # execute the SQLCI commands

    p1 = subprocess.Popen(["sqlci","-i",inputFileName], stdout=subprocess.PIPE)

    # initialize extracted data items

    timeStamp = time.time()
    prepareError = ''
    executeError = ''
    fragmentType = ''
    execPreds = ''
    mdamDisj = ''
    beginKey = ''
    endKey = ''
    executionInstance = -1
    startTime = ''
    endTime = ''
    recordsAccessed = '0'
    recordsUsed = '0'
    hdfsIOs = '0'
    hdfsIOBytes = '0'
    hdfsAccessTime = '0' 

    # process the lines, extracting the information, and appending it to the
    # output file

    state = 0
    for line in p1.stdout:
        if state < 6:  #  if no errors found yet
            if line.startswith('>>prepare s'):
                state = 1
            elif line.startswith('>>explain s'):
                state = 2
                explainState = 0
            elif line.startswith('>>execute s'):
                state = 3
            elif line.startswith('>>display statistics'):
                state = 4
                displayState = 0
                executionInstance = executionInstance + 1
                startTime = ''
                endTime = ''
                recordsAccessed = '0'
                recordsUsed = '0'
                hdfsIOs = '0'
                hdfsIOBytes = '0'
                hdfsAccessTime = '0'
            elif line.startswith('>>exit'):
                state = 5
            elif state == 1:
                if line.startswith('*** ERROR'):
                    prepareError = line.rstrip()
                    state = 6
            elif state == 2:
                if explainState == 0:
                    if line.startswith('TRAFODION_SCAN ='):
                        explainState = 1
                        appendState = ''
                elif explainState == 1:
                    if line.startswith('  fragment_type .......... '):
                        fragmentType = line.rstrip()
                        fragmentType = fragmentType[len('  fragment_type .......... '):]
                        appendState = ''
                    elif line.startswith('  executor_predicates .... '):
                        execPreds = line.rstrip()
                        execPreds = execPreds[len('  executor_predicates .... '):]
                        appendState = 'e'
                    elif line.startswith('  mdam_disjunct .......... '):
                        # actually there may be more than one of these; we just get the last one
                        mdamDisj = line.rstrip()
                        mdamDisj = mdamDisj[len('  mdam_disjunct .......... '):]
                        appendState = 'm'
                    elif line.startswith('  begin_key .............. '):
                        beginKey = line.rstrip()
                        beginKey = beginKey[len('  begin_key .............. '):]
                        appendState = 'b'
                    elif line.startswith('  end_key ................ '):
                        endKey = line.rstrip()
                        endKey = endKey[len('  end_key ................ '):]
                        appendState = 'f'
                    elif line.startswith('                           '):
                        # continuation of a previous line
                        if appendState == 'm':
                            mdamDisj = mdamDisj + ' ' + line.strip()
                        elif appendState == 'e':
                            execPreds = execPreds + ' ' + line.strip()
                        elif appendState == 'b':
                            beginKey = beginKey + ' ' + line.strip()
                        elif appendState == 'f':
                            endKey = endKey + ' ' + line.strip()
                    elif line.find('====',1) > 0:
                        # we have passed the end of TRAFODION_SCAN to some other operator
                        explainState = 2
                    else:
                        appendState = ''
            elif state == 3:
                if line.startswith('*** ERROR'):
                    executeError = line.rstrip()
                    state = 6
            elif state == 4:
                if displayState == 0:
                    if line.startswith('Start Time'):
                        startTime = line[len('Start Time'):]
                        startTime = startTime.strip()
                    elif line.startswith('End Time'):
                        endTime = line[len('End Time'):]
                        endTime = endTime.strip()
                    elif line.startswith('Table Name'):
                        displayState = 1
                elif displayState == 1:  # second header line (after Table Name)
                    displayState = 2
                elif displayState == 2:  # the actual table name
                    displayState = 3
                elif displayState == 3:  # a set of 5 integers
                    displayState = 4
                    values = line.split()
                    recordsAccessed = values[0]
                    recordsUsed = values[1]
                    hdfsIOs = values[2]
                    hdfsIOBytes = values[3]
                    hdfsAccessTime = values[4]
                    extractedStats = "INSERT INTO RESULTSTATS2 VALUES("+str(timeStamp)+","+\
                        str(executionInstance)+",TIMESTAMP '"+\
                        startTime+"',TIMESTAMP '"+\
                        endTime+"',"+\
                        recordsAccessed+","+\
                        recordsUsed+","+\
                        hdfsIOs+","+\
                        hdfsIOBytes+","+\
                        hdfsAccessTime+");\n"
                    g.write(extractedStats)

        
    extractedData = "INSERT INTO RESULTS2 VALUES("+str(timeStamp)+",'"+\
        planName+"','"+\
        args.statement+"','"+\
        prepareError+"','"+\
        executeError+"','"+\
        fragmentType+"','"+\
        execPreds+"','"+\
        mdamDisj+"','"+\
        beginKey+"','"+\
        endKey+"');\n"

    g.write(extractedData)

    if not retain:
        os.unlink(inputFileName)
        os.unlink(logFileName)

    return


# process command line arguments

parser = argparse.ArgumentParser(
    description='This script prepares, explains and executes a SQL statement and extracts information from these.')

parser.add_argument('--statement',required=True,
                    help='SQL statement to be processed.')
parser.add_argument('--output',required=True,
                    help='File to append extracted info to.')
parser.add_argument('--traversaldepth', required=True, type=int, choices=[1, 2, 3, 4, 5, 6, 7, 8],
                    help='How many key columns MDAM should traverse.')
parser.add_argument('--retainfiles', action='store_true',
                    help='If specified, the sqlci input files and logs are retained.')


args = parser.parse_args()  # exits and prints help if args are incorrect

exitCode = 0

tableName = findFullyQualifiedTableName(args.statement,args.retainfiles)
if tableName:
    print "Testing statement " + args.statement
    print
    
    g = open(args.output,"a")
    plansToTry = 2 * (args.traversaldepth + 1) + 1
    for planNumber in range(plansToTry):
        processOnePlan(args.statement,planNumber,tableName,g,args.retainfiles)

    g.close()
else:
    print "Could not obtain qualified table name from " + args.statement
    print
    exitCode = 1

exit(exitCode)





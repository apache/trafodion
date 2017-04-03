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
#  This script extracts data from an Optimizer trace, created
#  using the following CQDs:
#
#  cqd nsk_dbg_log_file 'optimizerTrace.txt';
#
#  cqd NSK_DBG_GENERIC 'ON';
#  cqd NSK_DBG_PRINT_COST 'ON';
#  cqd NSK_DBG_PRINT_COST_LIMIT 'ON';
#  cqd NSK_DBG_PRINT_LOG_PROP 'ON';
#  cqd NSK_DBG_PRINT_PHYS_PROP 'ON';
#  cqd NSK_DBG_PRINT_TASK 'ON';
#  cqd NSK_DBG_PRINT_TASK_STACK 'ON';
#
#  cqd NSK_DBG 'ON';
#
#  The script extracts completed plans and shows their shape
#  and costs, along with whether the plan was chosen. The 
#  goal of the script is to present the trace information in
#  a simple and high-level way.
#

import os
import sys
import subprocess 
import sets
import datetime
import argparse  # requires Python 2.7



def indentLevel(line):
    i = 0
    while i < len(line) and line[i] == ' ':
        i = i + 1
    return i


class ParseTrace:
    #

    def __init__(self, OptimizerTraceFileName):
        self.OptimizerTraceFileName = OptimizerTraceFileName
     

    def parseTrace(self):
        #
        # The things we are looking for are:
        #
        # "Query to optimize:"  
        # This shows the start of a trace for a particular query
        #
        # "Task: 8 ParentTask: 7 Pass: 2 GroupId: 1   7 1 Create plan (0) ..."
        # Plan fragments show up as a result of "Create plan" tasks being executed.
        # Plan fragments may result in incomplete plans; we ignore those.
        #
        # "*** Qualified plan ***"
        # This is the beginning of a qualified plan fragment, created by a "Create
        # plan" task.
        #
        # "*** Chosen plan ***" and "*** Non-optimal plan ***"
        # These indicate the end of a qualified plan fragment, and indicate whether
        # the particular fragment was chosen for its group.
        #
        # "SQL node(<node description>)"
        # This indicates a node within the plan. The tree structure of the nodes
        # is indicated by the relative indentation of each of these. Python-like.
        #
        # "**** Roll-up Cost ****"
        # This indicates that cost info for the subtree follows.
        #
        # "**** Operator Cost ****"
        # This indicates that cost info for the node itself follows.
        #
        # "elapsed time = <floating point number>"
        # This gives the cost, reduced to a single number.
        # 
        # For now, we ignore the other information in the trace.
        #

        try:
            f = open(self.OptimizerTraceFileName)
            saveSQLNode = ''
            state = 0
            for line in f:
                #print 'state = ' + str(state)
                line = line.rstrip('\n')  # get rid of trailing return character
                if state == 0 and line == "Query to optimize:":
                    print "Trace for the following query:"
                    state = 1
                elif state == 1:
                    if len(line) == 0:
                        state = 2
                    else:
                        print line
                elif state == 2:
                    if line == "Query to optimize:":
                        state = 20  # ignore the rest of the file
                    tempLine = line.lstrip()
                    if tempLine.startswith('Task: ') and tempLine.find('Create plan'):
                        print
                        print tempLine
                        print
                        state = 3
                elif state == 3:
                    tempLine = line.lstrip()
                    if tempLine == "*** Qualified plan ***":
                        state = 4
                elif state == 4:
                    tempLine = line.lstrip()
                    if tempLine.startswith('SQL node('):
                        saveSQLNode = line
                        state = 5
                elif state == 5:
                    tempLine = line.lstrip()
                    if tempLine.startswith('**** Roll-up Cost ****'):
                        state = 6
                elif state == 6:
                    tempLine = line.lstrip()
                    if tempLine.startswith('elapsed time = '):
                        planCost = 'Plan cost ' + tempLine
                        print planCost
                        print
                        print saveSQLNode
                        n = indentLevel(line)
                        rollupCost = line[0:n] + 'Rollup cost ' + tempLine
                        print rollupCost
                        state = 9
                elif state == 7:
                    tempLine = line.lstrip()
                    if tempLine.startswith('**** Roll-up Cost ****'):
                        state = 8
                elif state == 8:
                    tempLine = line.lstrip()
                    if tempLine.startswith('elapsed time = '):
                        n = indentLevel(line)
                        rollupCost = line[0:n] + 'Rollup cost ' + tempLine
                        print rollupCost
                        state = 9
                elif state == 9:
                    tempLine = line.lstrip()
                    if tempLine.startswith('**** Operator Cost ****'):
                        state = 10
                elif state == 10:
                    tempLine = line.lstrip()
                    if tempLine.startswith('elapsed time = '):
                        n = indentLevel(line)
                        operatorCost = line[0:n] + 'Operator cost ' + tempLine
                        print operatorCost
                        state = 11
                elif state == 11:
                    tempLine = line.lstrip()
                    if tempLine.startswith('SQL node('):
                        print line
                        state = 7
                    elif tempLine.startswith('Task: ') and tempLine.find('Create plan'):
                        print
                        print tempLine
                        print
                        state = 3
                    elif tempLine.startswith('*** Chosen plan ***') or tempLine.startswith('*** Non-optimal plan ***'):
                        print
                        print line
                        print
                        state = 2          
            f.close()
         
        except IOError as detail:
            print "Could not open " + self.OptimizerTraceFileName
            print detail        
        




# beginning of main


# process command line arguments

parser = argparse.ArgumentParser(
    description='This script parses out interesting data from an optimizer debug trace.')
parser.add_argument("OptimizerTraceFileName", help='The name of the trace file you wish to parse.')

args = parser.parse_args()  # exits and prints help if args are incorrect

exitCode = 0

Traceparser = ParseTrace(args.OptimizerTraceFileName)

Traceparser.parseTrace()

exit(exitCode)   



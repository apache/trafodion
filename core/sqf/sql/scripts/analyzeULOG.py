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
#  This script analyzes data from a ULOG. It looks for timer
#  events, and pulls these out, generating SQL INSERT 
#  statements with their data. This data can then be loaded
#  into a SQL table for further analysis.
#
#  The shape of the table assumed by the script is as follows:
#
#  create table ulog_data
# ( l1 int not null,
#   l2 int not null,
#   l3 int not null,
#   l4 int not null,
#   l5 int not null,
#   l6 int not null,
#   l7 int not null,
#   l8 int not null,
#   l9 int not null,
#   l10 int not null,
#   task varchar(80) not null,
#   elapsed char(12) not null,
#   primary key (l1,l2,l3,l4,l5,l6,l7,l8,l9,l10) );
#
#  The idea here is to capture the nested structure of the ULOG
#  timers. L1 represents the top-most level and is incremented 
#  for each UPDATE STATISTICS statement. L2 is the set of timers
#  within the scope of an L1 timer, and so on.
#
#  By querying this ulog_data, one can get an idea of where
#  UPDATE STATISTICS spends its time, and how expensive each
#  task is.
#
import os
import sys
import subprocess 
import sets
import datetime
import argparse  # requires Python 2.7

# An object of class BeginEndInterval is created for each timer
# found in the ULOG.

class BeginEndInterval:
    #
    
    def __init__(self, desc, level, previousItemNumber):
        #print "On entry to ctor, level = " + str(level) + ", prevIN = " + str(previousItemNumber)
        self.description = desc
        self.elapsedTime = None
        self.itemNumber = list(previousItemNumber)
        if len(previousItemNumber) <= level:
            self.itemNumber.append(1)
        else:
            lastIndex = len(previousItemNumber)-1
            self.itemNumber[lastIndex] = self.itemNumber[lastIndex]+1
        #print "Exiting ctor, Resulting in " + str(self.itemNumber)

    def setEnd(self, desc, elapsed):
        assert desc == self.description
        self.elapsedTime = elapsed

    def generateData(self):
        text = "INSERT INTO ULOG_DATA VALUES("
        for i in range(10):
            if i < len(self.itemNumber):
                text = text + str(self.itemNumber[i]) + ","
            else:
                text = text + "0,"
        text = text + "'" + self.description + "','" + self.elapsedTime + "');"
        print text


class ParseULOG:
    #

    def __init__(self, ULOGFileName):
        self.ULOGFileName = ULOGFileName
        self.beginEndIntervals = []
     

    def parseULOG(self):
        #
        # Begin/End entries look like this:
        #
        # [Tue Mar 21 21:02:27 2017] :|  BEGIN Allocate storage for columns
        # [Tue Mar 21 21:02:30 2017] :|  END   Allocate storage for columns elapsed time (00:00:02.666)
        #
        # The number of vertical bars varies and indicates levels of nesting.
        # For now we ignore the timestamp, and just pick out the description
        # and the elapsed time.
        #

        try:
            f = open(self.ULOGFileName)
            previousItemNumber = []
            messageNumberStr = None
            messageText = None
            for line in f:
                #originalLine = line
                line = line.rstrip('\n')  # get rid of trailing return character
                index = line.find('] ')
                if index >= 0:
                    line = line[index+2:] # strip off leading timestamp part
                if line.startswith(':'):
                    line = line[1:]  # strip off colon
                    level = 0
                    while line.startswith('|  '):
                        level = level + 1
                        line = line[3:]
                    if line.startswith('BEGIN '):
                        description = line[6:]  # strip off BEGIN; description is what's left
                        # create a BeginEnd interval and add to stack
                        beginEndInterval = BeginEndInterval(description,level,previousItemNumber)
                        previousItemNumber = list(beginEndInterval.itemNumber)
                        self.beginEndIntervals.append(beginEndInterval)
                    elif line.startswith('END   '):
                        index = line.find(' elapsed time (')
                        if index >= 0:
                            description = line[6:index]
                            elapsedTime = line[index+15:]
                            elapsedTime = elapsedTime.rstrip(')')
                            # pop the last BeginEnd interval from the stack
                            beginEndInterval = self.beginEndIntervals.pop()
                            beginEndInterval.setEnd(description,elapsedTime)
                            beginEndInterval.generateData()
                            previousItemNumber = list(beginEndInterval.itemNumber)
                            del beginEndInterval                 
            f.close()
         
        except IOError as detail:
            print "Could not open " + self.ULOGFileName
            print detail        
        




# beginning of main


# process command line arguments

parser = argparse.ArgumentParser(
    description='This script parses out interesting data from a ULOG.')
parser.add_argument("ULOGFileName", help='The name of the ULOG file you wish to parse.')

args = parser.parse_args()  # exits and prints help if args are incorrect

exitCode = 0

ULOGparser = ParseULOG(args.ULOGFileName)

ULOGparser.parseULOG()

exit(exitCode)   



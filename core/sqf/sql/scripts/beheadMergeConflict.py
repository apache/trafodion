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
#  This script is useful for resolving merge conflicts in regress
#  test expected results files.
#
#  Very often a conflict occurs because a test has been updated
#  in two branches, and that test has output containing DDL timestamps
#  or plan IDs or other unimportant information that varies on every
#  regress run. When these are the only differences, the conflict
#  resolution is simple: Delete all the HEAD stuff and accept all the
#  new stuff. This script does exactly that.
#
#  The script looks for lines of the following form:
#
#  <<<<<<< HEAD
#  -- Definition current  Wed Nov 29 08:53:46 2017
#  =======
#  -- Definition current  Wed Dec  6 19:32:34 2017
#  >>>>>>> b149874... [TRAFODION-2827] Turn update costing code on by default
#
#  When it finds this, it removes the lines from <<<<<< HEAD to ======,
#  and removes the final >>>>>> line, leaving just:
#
#  -- Definition current  Wed Dec  6 19:32:34 2017
#

import os
import sys
import subprocess 
import sets
import datetime
import argparse  # requires Python 2.7

# The class BeheadMergeConflicts does the heavy lifting

class BeheadMergeConflicts:
    #

    def __init__(self, FileName):
        self.FileName = FileName
        self.state = 0

    def removeConflicts(self):

        try:
            f = open(self.FileName)
            state = 0
            for line in f:
                line = line.rstrip('\n')  # get rid of trailing return character
                if state == 0:
                    if line == "<<<<<<< HEAD":
                        state = 1
                    else:
                        print line
                elif state == 1:
                    if line == "=======":
                        state = 2
                elif state == 2:
                    if line.startswith(">>>>>>> "):
                        state = 0
                    else:
                        print line
                
            f.close()
         
        except IOError as detail:
            print "Could not open " + self.FileName
            print detail        
        




# beginning of main


# process command line arguments

parser = argparse.ArgumentParser(
    description='This script removes the "HEAD" part of git merge conflicts.')
parser.add_argument("FileName", help='The name of the file that you wish to remove merge conflicts from.')

args = parser.parse_args()  # exits and prints help if args are incorrect

exitCode = 0

conflictBeheader = BeheadMergeConflicts(args.FileName)

conflictBeheader.removeConflicts()

exit(exitCode)   



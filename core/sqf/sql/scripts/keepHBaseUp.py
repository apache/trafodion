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
#  This script is useful on workstations when doing overnight
#  development regression runs using the local hadoop. It
#  periodically checks to see if the HMaster is up. If it 
#  isn't, it attempts to restart it.
#
import os
import sys
import subprocess 
import re
import sets
import time
import argparse  # requires Python 2.7




# beginning of main

# pseudocode
#
# giveUp = false
# while not giveUp
#    check to see if HMaster is up
#    if not
#       try to start it
#    end while
#    if we've had too many failures
#       giveUp = true
#    else
#       sleep for a while
#    end if
# end while
#


# process command line arguments

parser = argparse.ArgumentParser(
    description='This script watches to see if HMaster goes away and tries to restart it if so.')

args = parser.parse_args()  # exits and prints help if args are incorrect

exitCode = 0

giveUp = False
# sleep time units are in minutes in this script
retryStartSleepTime = 1
normalSleepTime = 5
lastSleepTime = normalSleepTime
sleepTimeThisGoRound = 0  # so we check right away the first time

while giveUp == False:

    # sleep function takes seconds
    time.sleep(60 * sleepTimeThisGoRound)
  
    p1 = subprocess.Popen(["jps"], stdout=subprocess.PIPE)
    p2 = subprocess.Popen(["grep", "HMaster"], stdin=p1.stdout, stdout=subprocess.PIPE, close_fds=True)
    foundIt = False
    for ip in p2.stdout: 
        foundIt = True

    # gets rid of <defunct> jps and grep
    p1.wait()
    p2.wait()

    sleepTimeThisGoRound = 0
    if foundIt == False:
        print "At %s, HMaster was not running." % time.ctime()
        if lastSleepTime == normalSleepTime:
            # it was up the last time we checked; use minimal sleep time
            retryStartSleepTime = 1
        else:         
            # double the sleep time for consecutive HBase restarts up to 64
            retryStartSleepTime = 2 * retryStartSleepTime

        sleepTimeThisGoRound = retryStartSleepTime
        if retryStartSleepTime > 64:
            giveUp = True
        else:
            retcode = subprocess.call(["swstarthbase"])
            print "retcode from swstarthbase call was " + str(retcode)
    else:
        print "At %s, HMaster was up." % time.ctime()
        sleepTimeThisGoRound = normalSleepTime

    lastSleepTime = sleepTimeThisGoRound

print "Too many consecutive failures; giving up."
exit(exitCode)   



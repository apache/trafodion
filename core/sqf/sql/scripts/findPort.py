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
#  This script finds ranges of ports suitable for input
#  to the Trafodion install_local_hadoop script.
#
import os
import sys
import subprocess 
import re
import sets
import argparse  # requires Python 2.7




# beginning of main

# pseudocode
#
# do a "netstat | grep localhost" to obtain raw port-in-use info
#
# inUseRanges = empty set
# for each line
#    for each occurrance of the string "localhost:nnnnn"
#       rangeInUse = integer(nnnnn/200) (assume a range of 200 ports is needed)
#       add rangeInUse to inUseRanges
#    end for
# end for
#
# for each possible range starting from the smallest in rangeInUse and ending at 48800
#     if it isn't in the inUseRanges set
#         print it out as a possible port range to use
#     end if
# end for
  


# process command line arguments

parser = argparse.ArgumentParser(
    description='This script finds possible port ranges to use with install_local_hadoop.')

args = parser.parse_args()  # exits and prints help if args are incorrect

exitCode = 0

# get the set of lines to process ( netstat | grep localhost )

p1 = subprocess.Popen(["netstat"], stdout=subprocess.PIPE)
p2 = subprocess.Popen(["grep","localhost"], stdin=p1.stdout, stdout=subprocess.PIPE, close_fds=True)

# process the lines, looking for port numbers

pattern = r'localhost:(?P<portNumber>[0-9]{5})'
matcher = re.compile(pattern)

inUseRanges = sets.Set()

for line in p2.stdout: 
    result = matcher.findall(line)
    for occurrance in result:
        rangeInUse = int(occurrance)/200
        inUseRanges.add(rangeInUse)

# avoid recommending low ranges; our lowest recommendation
# will be the first unused range above the lowest in-use range

# avoid ranges in the ephemeral port range 49191-65535 also

print

foundOne = False
minInUse = min(inUseRanges)
for r in range(min(inUseRanges)+1,48800/200):
    if r not in inUseRanges:
        if not foundOne:
            print "Port ranges not in use:"
        foundOne = True
        print 200*r

if not foundOne:
    print "All port ranges from " + str(200*minInUse) + " through 48800 are in use."
    exitCode = 1

print

exit(exitCode)

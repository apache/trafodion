#/**
#* @@@ START COPYRIGHT @@@
#
#Licensed to the Apache Software Foundation (ASF) under one
#or more contributor license agreements.  See the NOTICE file
#distributed with this work for additional information
#regarding copyright ownership.  The ASF licenses this file
#to you under the Apache License, Version 2.0 (the
#"License"); you may not use this file except in compliance
#with the License.  You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
#Unless required by applicable law or agreed to in writing,
#software distributed under the License is distributed on an
#"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#KIND, either express or implied.  See the License for the
#specific language governing permissions and limitations
#under the License.
#
#* @@@ END COPYRIGHT @@@
# */
#

import subprocess

def runScript ():
    proc = subprocess.Popen(scriptcontext.getCommand(),shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    scriptcontext.setExitCode(proc.wait())
    for line in proc.stdout:
         scriptcontext.getStdOut().append(line.rstrip())
    for line in proc.stderr:
         scriptcontext.getStdErr().append(line.rstrip())
    
def start():
    runScript()

#######################################################################
#                main portion of script starts here
#######################################################################
if scriptcontext.isDebug() == True  :
    print 'Begin script[%s]' % scriptcontext.getScriptName()
    
start()

if scriptcontext.isDebug() == True  :
    print 'End script[%s]' % scriptcontext.getScriptName()

#/**
#* @@@ START COPYRIGHT @@@
#*
#* Licensed to the Apache Software Foundation (ASF) under one
#* or more contributor license agreements.  See the NOTICE file
#* distributed with this work for additional information
#* regarding copyright ownership.  The ASF licenses this file
#* to you under the Apache License, Version 2.0 (the
#* "License"); you may not use this file except in compliance
#* with the License.  You may obtain a copy of the License at
#*
#*   http://www.apache.org/licenses/LICENSE-2.0
#*
#* Unless required by applicable law or agreed to in writing,
#* software distributed under the License is distributed on an
#* "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#* KIND, either express or implied.  See the License for the
#* specific language governing permissions and limitations
#* under the License.
#*
#* @@@ END COPYRIGHT @@@
# */
#

import subprocess
import datetime

def runScript ():

    command = scriptcontext.getCommand()

    if isDebug() == True :
        print 'command[%s]' % command   
        
    proc = subprocess.Popen(command,shell=True, stderr=subprocess.PIPE, stdout=subprocess.PIPE)
    scriptcontext.setExitCode(proc.wait())
    for line in proc.stdout:
         if scriptcontext.getStripStdOut() == True :
            scriptcontext.getStdOut().append(line.rstrip())
         else :
            scriptcontext.getStdOut().append(line)
    for line in proc.stderr:
         if scriptcontext.getStripStdErr() == True :
            scriptcontext.getStdErr().append(line.rstrip())
         else :
           scriptcontext.getStdErr().append(line)         
         
    if isDebug() == True :
        print 'exitCode[%d]' % scriptcontext.getExitCode()
        print 'stdout[%s]' % scriptcontext.getStdOut()
        print 'stderr[%s]' % scriptcontext.getStdErr()   
    
def start():
    runScript()
    
def isDebug():
    return False
    
def log(msg):
    dts = datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    print dts + ' ' + msg
 
#######################################################################
#                main portion of script starts here
#######################################################################
if isDebug() == True :
   log ('Begin script[' + scriptcontext.getScriptName() + ']')
    
start()

if isDebug() == True :
   log ('End script[' + scriptcontext.getScriptName() + ']')

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
# @@@ END COPYRIGHT @@@
# */
from java.lang import System
from org.trafodion.wms.thrift.generated import Action 
from org.trafodion.wms.thrift.generated import Operation

def begin():
    response.getData().setAction(Action.ACTION_CONTINUE)
#    print 'Thresholds before [%s]' % thresholds.toString()
#    thresholds.setMaxMemUsage(5)
#    print 'Thresholds after [%s]' % thresholds.toString()
 

def update():
    return

#    jobType = request.getJobType()
#    if jobType != JobType.HIVE  :
#        return

#    elapsed = System.currentTimeMillis() - request.getData().getBeginTimestamp()
#    if elapsed > 60000L :
#        print 'Canceling...elapsed time [%s] milliseconds' % elapsed
#        response.getData().setAction(Action.ACTION_CANCEL)
        
def end():
    return

def start():
    operation = request.getData().getOperation().toString()

    if operation == "OPERATION_BEGIN" :
        begin()
    elif operation == "OPERATION_UPDATE" :
        update()
    elif operation == "OPERATION_END" :
        end()
    else :
        response.getData().setAction(Action.ACTION_CONTINUE)

#######################################################################
#                main portion of script starts here
#######################################################################
print 'Rcvd request[%s]' % request
start()
print 'Send response:[%s]' % response

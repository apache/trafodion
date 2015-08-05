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
import string
    
def execute1(command):
    if(len(sys.argv)>2):
        args = string.join(sys.argv[2:])
    else:
        args = ''
        
    cin, cout ,cerr = os.popen3(command+' %s' % (args))
    
        
    while 1:
        text = cout.read()
        if text:
            print text
        else:
            break
            
    while 1:
        text = cerr.read()
        if text:
            print text
        else:
            break
    cin.close()
    cout.close()

if __name__ == '__main__':
    
    jy_classpath = ''
    if os.path.isfile('settings.py'):
        import settings
        jy_classpath=settings.getJythonClasspath()
    else:
        jy_classpath=os.getenv('TRAFCI_PYTHON_JSERVER')
        if jy_classpath == None :
            print ' '
            print " Environment variable TRAFCI_PYTHON_JSERVER is not set"
            raise SystemExit
    
    if len(sys.argv) <= 1 :
        print ' '
        print "Unknown number of arguments passed."
        raise SystemExit
         
    classpath='##TRAFCI_PERL_CLASSPATH##'+jy_classpath
    command = 'java -classpath '+ '"' + classpath + '"' + ' org.python.util.jython ' + sys.argv[1]
    execute1(command)

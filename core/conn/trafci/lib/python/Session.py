#!/usr/bin/jython
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
import sys
sys.path += ##TRAFCI_PYTHON_CLASSPATH##
from org.trafodion.ci import ScriptsInterface
from java.lang import System
from java.io import PrintStream
from java.io import BufferedOutputStream
from java.io import FileOutputStream
from java.io import FileNotFoundException
from java.io import IOException
from java.sql import SQLException
from java.lang import ClassNotFoundException
from java.lang import InstantiationException
from java.lang import IllegalAccessException

import exceptions



class Session:
    
    
    #System.setOut(sys.stdout);  

    def __init__(self):    
        
     self.pi=ScriptsInterface()


        

        
    def __connect__(self,user,password,server,port,dsn):
        
        try:
            
            self.pi.openConnection(user,password,server,port,dsn)
            
        except ClassNotFoundException ,cnfe:
            
            print "class not found: %s " % str(cnfe)
	    raise SystemExit
        
        except InstantiationException ,ie:
            
            print "InstantiationException: %s " % str(ie)
	    raise SystemExit
            
        except IllegalAccessException ,iae:
            
            print "IllegalAccessException: %s " %str(iae)
	    raise SystemExit
            
        except FileNotFoundException ,fnfe:
            
            print "FileNotFoundException: %s " %str(fnfe)
	    raise SystemExit
        
        except SQLException , sqle:
            print "SQL Exception: %s " %str(sqle)
	    raise SystemExit
        
        except IOException , ioe:
            
            print "IOException: %s " %str(ioe)
	    raise SystemExit
    
    def __execute__(self,query):
        try:
            
            return self.pi.executeQuery(query)
    
        except IOException , ioe:
            
            print "IOException: %s " %str(ioe)

    def __executeScript__(self,scriptFile,logFile):
    	try:
	   self.pi.executeScript(scriptFile,logFile)
	
        except IOException , ioe:
	   print "IO Exception: %s " %str(ioe)
    
        
    def __disconnect__(self):    
        self.pi.disconnect()
        del self.pi
        System.exit
	raise SystemExit

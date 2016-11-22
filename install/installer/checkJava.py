#!/usr/bin/python

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


import subprocess
import re
import os


pathToJava=os.environ["JAVA_HOME"]
pathToJava = pathToJava + "/bin/" + "java"
javaVersion  = [pathToJava, "-version"]

java7="1.7"
java8="1.8"
java67="67"

try: 
   sp = subprocess.Popen(javaVersion, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
except:
   print "***ERROR: Unable to find Java Version"
   os._exit(-1)

lst_Java=list(sp.communicate())


lst_Java=re.findall('version "[0-9.]+_[0-9]+"', lst_Java[1])

if not re.search(java7, str(lst_Java)):
   if re.search(java8, str(lst_Java)):
      os._exit(0)
   else:
      os._exit(-1)


lst_Java=re.findall('_[0-9]+', lst_Java[0])

lst_Java=re.findall('[0-9]+', lst_Java[0])



if int(lst_Java[0]) >= int(java67):
   os._exit(0)
else:
   os._exit(-1)


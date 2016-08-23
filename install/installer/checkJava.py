#!/usr/bin/python


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


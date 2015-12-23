.. # @@@ START COPYRIGHT @@@
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

##How to build and initialize default SPJs
###Build
```  
cd $MY_SQROOT/../conn/spj_init    
make clean && make all  
```
###Initialization
```  
cds  
./init_spj.sql  
```  
###SPJ Example
```
DEFAULT_SPJ.HELP(INOUT COMMANDNAME VARCHAR)  
E.g.   
trafci>set param ?p1 help;  
trafci>call DEFAULT_SPJ.HELP(?p1)  
PUT - Upload a JAR. SHOWDDL PRODURE DEFAULT_SPJ.PUT for more info.  
LS - List JARs. SHOWDDL PRODURE DEFAULT_SPJ.LS for more info.  
LSALL - List all JARs. SHOWDDL PRODURE DEFAULT_SPJ.LSALL for more info.  
RM - Remove a JAR. SHOWDDL PRODURE DEFAULT_SPJ.RM for more info.  
RMREX - Remove JARs by a perticular pattern. SHOWDDL PRODURE DEFAULT_SPJ.RMREX for more info.  
GETFILE - Upload a JAR. SHOWDDL PRODURE DEFAULT_SPJ.GETFILE for more info.  
```

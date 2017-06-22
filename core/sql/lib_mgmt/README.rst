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

=============================
Trafodion lib_mgmt README
=============================

How to build and initialize default SPJs 

  Build
cd $TRAF_HOME/../sql/lib_mgmt
make clean && make all

  Initialization

sqlci
initialize trafodion, create library management;

   if already initialized, do the following:

sqlci
initialize trafodion, upgrade library management;


   SPJ Example

"_LIBMGR_".HELP(INOUT COMMANDNAME VARCHAR)
 
E.g.
 
trafci>set param ?p1 help;
 
trafci>call "_LIBMGR_".HELP(?p1);

PUTFILE - Upload a library file. SHOWDDL PROCEDURE [SCHEMA NAME.]PUTFILE for more info.
LS - List library files. SHOWDDL PROCEDURE [SCHEMA NAME.]LS for more info.
LSALL - List all library files. SHOWDDL PROCEDURE [SCHEMA NAME.]LSALL for more info.
RM - Remove a library file. SHOWDDL PROCEDURE [SCHEMA NAME.]RM for more info.
RMREX - Remove library files by a perticular pattern. SHOWDDL PROCEDURE [SCHEMA NAME.]RMREX for more info.
GETFILE - Download a library file. SHOWDDL PROCEDURE [SCHEMA NAME.]GETFILE for more info.
ADDLIB - Create a library. SHOWDDL PROCEDURE [SCHEMA NAME.]ADDLIB for more info.
ALTERLIB - Update a library. SHOWDDL PROCEDURE [SCHEMA NAME.]ALTERLIB for more info.
DROPLIB - Drop a library. SHOWDDL PROCEDURE [SCHEMA NAME.]DROPLIB for more info.

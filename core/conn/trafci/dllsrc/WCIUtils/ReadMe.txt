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

========================================================================
    DYNAMIC LINK LIBRARY : WCIUtils Project Overview
========================================================================

WCIUtils.cpp
   This is the main DLL source file.

   When created, this DLL does not export any symbols. As a result, it  
   will not produce a .lib file when it is built. If you wish this project 
   to be a project dependency of some other project, you will either need to 
   add code to export some symbols from the DLL so that an export library 
   will be produced, or you can set the Ignore Input Library property to Yes 
   on the General propert page of the Linker folder in the project's Property 
   Pages dialog box.

/////////////////////////////////////////////////////////////////////////////
Other standard files:

StdAfx.h, StdAfx.cpp
    These files are used to build a precompiled header (PCH) file
    named WCIUtils.pch and a precompiled types file named StdAfx.obj.

/////////////////////////////////////////////////////////////////////////////
Other notes:

AppWizard uses "TODO:" comments to indicate parts of the source code you
should add to or customize.

Dev used VC++ version 8.0, which used the following compile/link command:
   cl /O2 /GL /EHsc /W3 /Wp64 /TP -LD CIUtils.cpp

Others used VC++ version 5.0, which used the following compile/link command:
   cl /O2 /GL /EHsc /W3 /TP -LD CIUtils.cpp

Using /Wp64 on version 5.0 yields:
   Command line error D2021 : invalid numeric argument '/Wp64' [Detects 64-bit portability problems]

Using /GL on version 5.0 yields:
   Command line warning D4002 : ignoring unknown option '/GL'

To build:

Start first to get the WCIUtils class file built.

Follow with a WCIUtils.make.bat to build the dll, ie.

> cd trafci/dllsrc/WCIUtils/WCIUtils>
> ./WCIUtils64.make.bat

Complete trafci build to pick the new dll to be included in the distribution
 

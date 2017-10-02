/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
#ifndef COM_MEMORY_DIAGS_H
#define COM_MEMORY_DIAGS_H
/* -*-C++-*-
****************************************************************************
*
* File:         ComMemoryDiags.h
* RCS:          $Id: ComMemoryDiags.h,v 1.1 2007/10/09 19:38:52  Exp $
* Description:
*
* Created:      7/15/1998
* Modified:     $ $Date: 2007/10/09 19:38:52 $ (GMT)
* Language:     C++
* Status:       $State: Exp $
*
*
**************************************************************************** */

// -----------------------------------------------------------------------
//  The following classes are defined in this file.
// -----------------------------------------------------------------------
class ComMemoryDiags;   // used to diagnose memory leaks

#include <iosfwd>
using namespace std;

//
// class ComMemoryDiags is simply a wrapper around the static data member
//
//      ostream * dumpMemoryInfo_
//
// This data member refers to a filestream where we dump memory information 
// at various stages of compilation.  The resulting file is used to debug
// memory leak / usage information (this is a vital component of the effort
// to reduce/remove memory leaks in arkcmp!).
//
// We're doing this for two reasons : we need to enforce the non-interdependence
// of cli.lib and tdm_arkcmp.exe; also, we'd like to minimize pollution of the global
// namespace.
// 
// This class (data member) is used in two places : 
//
//   /cli/Context.cpp
//   /sqlcomp/CmpMain.cpp
//
// The static data member is initialized in ComMemoryDiags.cpp.
//

class ComMemoryDiags {
public:
  static ostream *& DumpMemoryInfo() { return dumpMemoryInfo_ ; }
private:
  static ostream *  dumpMemoryInfo_ ; 

};



#endif /* COM_MEMORY_DIAGS_H */





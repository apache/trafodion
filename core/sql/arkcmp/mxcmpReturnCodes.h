/**********************************************************************
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 2003-2014 Hewlett-Packard Development Company, L.P.
//
//  Licensed under the Apache License, Version 2.0 (the "License");
//  you may not use this file except in compliance with the License.
//  You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
//  Unless required by applicable law or agreed to in writing, software
//  distributed under the License is distributed on an "AS IS" BASIS,
//  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
//  See the License for the specific language governing permissions and
//  limitations under the License.
//
// @@@ END COPYRIGHT @@@
**********************************************************************/
#ifndef __MXCMPRETURNCODES_H
#define __MXCMPRETURNCODES_H

/* -*-C++-*-
 *****************************************************************************
 * File:         mxcmpReturnCodes.h
 * Description:  This holds the mxcmp exit codes.
 * Created:      10/23/2003
 * Language:     C++
 *****************************************************************************
 */

// mxcmp & mxCompileUserModule need these constants so that
// they can both return the same exit codes to ETK.

enum mxcmpExitCode 
{ SUCCEED = 0, FAIL = 1, FAIL_RETRYABLE = -1, ERROR = 2, WARNING = 3 };

// The above constants used to be in arkcmp/cmpareas.h;
// We had to move them to this separate header file to
// work around an MS DevStudio c++ compiler bug that 
// declares this false error:
//
// @@@@ Building mxCompileUserModule executable 
// "Target platform: Windows NT (debug)"
// "================================================"
// "Compiling applicationfile.cpp to ../nskomake/mxcompileusermodule/nt/debug/applicationfile.obj"
// "================================================"
// cl /nologo /Zp4 /W3 /GX /Od -I../sqlci -I../arkcmp -I../comexe -I../sqlfe -I../eh -I../export -I../sqlmsg -I../sqlcomp -I../sqlcat -I../executor -I../parser -I../generator -I../exp -I../filesystem -I../optimizer -I../cli -I../nskcre -I../common -I../dml -I../arkfsindp2 -I../arkfsinopen -I../ddl -I../sort -I../catman -I../smdio -I../ustat -I../sqlshare -I../sqlmxevents -I../bin -I../langman -I../udrserv /I "../sqlco" /I "../psql/inc" /I "../psql/inc/fs2" /I "../rogue" /MDd /D "_DEBUG" /D "NA_WINNT" /D "WIN32" /D "_CONSOLE" /Z7 /Zm1000 /Fo"../nskomake/mxCompileUserModule/nt/debug/" /c ../arkcmp\applicationfile.cpp
// applicationfile.cpp
// ../arkcmp\applicationfile.cpp(67) : error C2556: 'mxcmpModule' : overloaded functions only differ by return type
// ../arkcmp\applicationfile.cpp(67) : error C2371: 'mxcmpModule' : redefinition; different basic types
//
// when we simply #include "cmpareas.h" into arkcmp/AppplicationFile.cpp.
// 
// A look at the MS DevStudio C++ output after the preprocessor phase
// reveals two inconsistent expansions (replacements) of "bool"
//
//   bool mxcmpModule(char *mdf, Cmdline_Args &args);
//   ...
//   short ApplicationFile::mxcmpModule(char *mdf, Cmdline_Args &args)
//
// For some reason, ETK c89 compiles arkcmp/ApplicationFile.cpp just fine!

#endif /* __MXCMPRETURNCODES_H */

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
/* -*-C++-*-
******************************************************************************
*
* File:         cmpargs.h
*               
* Created:      
* Language:     C++
*
*
*
*
******************************************************************************
*/


#ifndef __CMP_ARGSH
#define __CMP_ARGSH

#include "Ipc.h"
#include "NAString.h"

#include <stdlib.h>


struct ControlSetting {
  NAString attrName;
  NAString attrValue;
  ControlSetting(NAString n, NAString v) : attrName(n), attrValue(v) {}
  ControlSetting() : attrName(), attrValue() {}
};
typedef NAArray<ControlSetting> SettingsArray;

class Cmdline_Args  {
public:
   Cmdline_Args();
   void processArgs(Int32 argc, char ** argv);

   NAString getAppName() const;
   NAString getMDFname() const;
   NAString getModName() const;
   bool  isStatComp() const;
   NABoolean hasStmtName() const;
   void  printArgs();
   void  usage(Int32 argc, char ** argv);
   IpcServerAllocationMethod allocMethod() const;
   Int32 socketArg() const;
   Int32 portArg() const;
   bool  isVerbose() const;
   bool  ignoreErrors() const;
   bool  replaceModule() const;
   bool  noSeabaseDefTableRead() const
   {
     return noSeabaseDefTableRead_;
   }

  void overwriteSettings() const;

   enum LocalStatus { NOT_SET, MOD_GLOBAL, MOD_LOCAL };
   bool  moduleLocal() const;
   char* getModuleDir() const;


private:
  void addSetting(ControlSetting s);
 
   NAString  application_;  // application filename, default is NULL
   NAString  moddef_;       // module definition filename
   NAString  module_;       // ANSI module name (= compiled module filename)
   bool      isStat_;       // is static recompilation request
   bool      hasListing_;   // sqlcomp has to generate compiler listing
   bool      isVerbose_;    // verbose mode to print out extra information
   bool      ignoreErrors_; // convert static compile errors to warnings. 
                            // Statement will be recompiled at runtime. The
                            // error will be returned at runtime if
                            // the recompile fails.

   IpcServerAllocationMethod allocMethod_;
   Int32 socketArg_;
   Int32 portArg_;
   SettingsArray settings_; // control query default settings from cmd line
   LocalStatus modulePlacement_;// where user wants local module
   NAString  moduleDir_;    // directory for module file

   bool noSeabaseDefTableRead_;

  // process -g {moduleGlobal|moduleLocal[=OSSdirectory]}
  Int32 doModuleGlobalLocalDir(char *arg, Int32 argc, char **argv, Int32 &gCount, ComDiagsArea& diags);

  // process -g {moduleGlobal|moduleLocal}
  Int32 doModuleGlobalLocal(char *arg, Int32 argc, char **argv, ComDiagsArea& diags);
};
 
inline void Cmdline_Args::addSetting(ControlSetting s)
{ settings_.insertAt(settings_.entries(), s); }

inline NAString Cmdline_Args::getAppName() const
{
   return application_;
}

inline NAString Cmdline_Args::getMDFname() const
{
   return moddef_;
}

inline NAString Cmdline_Args::getModName() const
{
   return module_;
}
 
inline bool Cmdline_Args::isStatComp() const
{
  return isStat_;
  
} // end isStatComp

inline IpcServerAllocationMethod Cmdline_Args::allocMethod() const
{
  return allocMethod_;
}

inline Int32 Cmdline_Args::socketArg() const 
{
  return socketArg_; 
}

inline Int32 Cmdline_Args::portArg() const
{
  return portArg_;
}

inline bool Cmdline_Args::isVerbose() const
{
  return isVerbose_;
}

inline bool Cmdline_Args::ignoreErrors() const
{
  return ignoreErrors_;
}

#endif // __CMP_ARGSH

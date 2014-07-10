/* -*-C++-*-
 *****************************************************************************
 *
 * File:         CliDll.cpp
 * Description:  CLI DLL-related code for Windows NT - from Cli.cpp originally
 *               
 *               
 * Created:      7/19/97
 * Language:     C++
 *
 *
// @@@ START COPYRIGHT @@@
//
// (C) Copyright 1997-2014 Hewlett-Packard Development Company, L.P.
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
 *
 *
 *****************************************************************************
 */


// This file is home to InitApp() which is called when the DLL is loaded.
//
// This DLL is the *only* one which needs InitApp()!
//   In particular,
//	basic/BasicDll.cpp exp/ExpDll.cpp export/ExportDll.cpp
//   do *not* need it!


#include "Platform.h"


#include "cli_stdh.h"
#include "ex_transaction.h"
#include <stdarg.h>

//  Only if we're building a DLL do we need to include the stuff below;
// otherwise, just ignore them.
#if defined(CLI_DLL) || defined(CLI_LIB)
  #include "NABoolean.h"
//  #include "UtilInt.h"
  #include "catnames.h"
  #include "SqlciEnv.h"		    // only reason why we need sqlcilib in DLL.
  #include "StoredProcInterface.h"  // to export the interface.

    #include <setjmp.h>
    SQLEXPORT_LIB_FUNC jmp_buf ExportJmpBuf;
    extern "C"
    {
      SQLCLI_LIB_FUNC  void InitApp(Int32 *argc, char *argv[]) {}
    }

  //  stub functions used only in case CLI is built as a DLL.
  #if defined(CLI_DLL)

    // Stub for UNUSED virtual ../optimizer/SynthType.cpp function
    // (pulled in by ../sqlci/Formatter.cpp).
    #include "NAType.h"
    NABoolean NAType::isComparable(const NAType &, ItemExpr *, NABoolean) const
							    { return FALSE; }
  #endif

#endif


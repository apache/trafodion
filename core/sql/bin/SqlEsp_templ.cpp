/* -*-C++-*-
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
 *****************************************************************************
 *
 * File:         SqlEsp_templ.C
 * Description:  Source file to explicitly cause template instantiation
 *               and to avoid such instantiation at load time
 *
 * Created:      8/9/96
 * Language:     C++
 *
 *
 *
 *****************************************************************************
 */

#include "Platform.h"
#ifndef NO_TEMPLATE_INSTANTIATION_FILE

// -----------------------------------------------------------------------
// By setting the two defines below, force the compiler to read all
// implementation files for Tools.h++ and for the NA... collection
// type templates in Collections.h
// -----------------------------------------------------------------------
#ifndef RW_COMPILE_INSTANTIATE
#	define RW_COMPILE_INSTANTIATE
#endif
#ifndef NA_COMPILE_INSTANTIATE
#	define NA_COMPILE_INSTANTIATE
#endif

// -----------------------------------------------------------------------
// Include header files such that the included code covers a large part
// of all the different template references used in an ESP. Those
// templates that aren't referenced in a header file must be explicitly
// referenced in a dummy variable defined below.
// -----------------------------------------------------------------------
#include "NAIpc.h"
#include "ex_stdh.h"
#include "ex_tdb.h"
#include "ex_tcb.h"
#include "ex_send_bottom.h"
#include "ex_send_top.h"
#include "ex_split_bottom.h"
#include "ex_split_top.h"
#include "ex_frag_rt.h"
#include "ex_esp_frag_dir.h"
#include "ComDiags.h"


// -----------------------------------------------------------------------
// For those templates that are just used in .C files or that are used
// in header files not sourced into this file, make a dummy variable and
// force the instantiation system to instantiate it here.
// NOTE: we expect this file to be compiled with the -ptf -pta flags.
// NOTE: this file is designed for cfront-based compilers; it may not
// work in other environments, like c89.
// -----------------------------------------------------------------------
static void dummy_proc_()
{

  // LIST(CollIndex)               dummy21_;  // see ColStatDesc.C

}

#endif /* NO_TEMPLATE_INSTANTIATION_FILE */

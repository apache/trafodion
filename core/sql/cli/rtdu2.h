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
#ifndef RTDU2_H
#define RTDU2_H
	
/* -*-C++-*-
 *****************************************************************************
 *
 * File:         rtdu2.h
 * Description:  Contains description of the object module.
 *               
 *               
 * Created:      10/15/94
 * Language:     C++
 *
 *
 *
 *
 *****************************************************************************
 */
#include <fstream>
#include "ComDiags.h"
#include "NAMemory.h"
#include "rtdu.h"

// The following code used to be in two places: 
//   context.cpp's addModule() and
//   cmpmod.cpp's openInputModuleFile()
// So to avoid inconsistent code changes, we merged the two versions 
// of that code into the following helper functions.

// read and return module header structure
SQLCLI_LIB_FUNC module_header_struct*
readModuleHeader
(fstream              &mf,     // (IN): binary module file
 module_header_struct &hdr,    // (IN): target of read
 const char *         name,    // (IN): name of module (for error msg)
 ComDiagsArea         &diags); // (IN): deposit any error msg here
// requires: caller has invoked NAVersionedObject::setReallocator() to
//           set up the memory allocator for any version-migrated object(s)
//           mf is the binary module file (open for reading)
//           hdr has enough space to hold the module_header_struct
// modifies: hdr, diags, NAVersionedObject::reallocator Space
// effects : read module_header_struct from mf
//           driveUnpack, verify it, deposit any error msg in diags
//           return result of driveUnpack()

// read and return procedure location table area (header + entries)
SQLCLI_LIB_FUNC Int32
readPLTArea
(fstream              &mf,          // (IN) : binary module file
 module_header_struct &latestModHdr,// (IN) : its module header
 NAHeap               &heap,        // (IN) : allocate PLT area from here
 const char *         name,         // (IN) : module name (for error msg)
 ComDiagsArea         &diags,       // (IN) : deposit any error msg here
 plt_header_struct   *&pltArea);    // (OUT): plt header + entries
// requires: caller has invoked NAVersionedObject::setReallocator() to
//           set up the memory allocator for any version-migrated object(s)
//           mf is the binary module file (open for reading)
//           latestModHdr is the result of readModuleHeader()
// modifies: heap, diags, pltArea
// effects:  allocate space for procedure location table (PLT) area
//           read PLT area from mf
//           driveUnpack, verify it, deposit any error msg in diags
//           pltArea = result of driveUnpack()
//           return: < 0 if an error occurred,
//                   pltArea->num_procedures, otherwise.

// read and return desccriptor location table area (header + entries)
SQLCLI_LIB_FUNC Int32
readDLTArea
(fstream              &mf,          // (IN) : binary module file
 module_header_struct &latestModHdr,// (IN) : its module header
 NAHeap               &heap,        // (IN) : allocate DLT area from here
 const char *         name,         // (IN) : module name (for error msg)
 ComDiagsArea         &diags,       // (IN) : deposit any error msg here
 dlt_header_struct   *&dltArea);    // (OUT): dlt header + entries
// requires: caller has invoked NAVersionedObject::setReallocator() to
//           set up the memory allocator for any version-migrated object(s)
//           mf is the binary module file (open for reading)
//           latestModHdr is the result of readModuleHeader()
// modifies: heap, diags, dltArea
// effects:  allocate space for descriptor location table (DLT) area
//           read DLT area from mf
//           driveUnpack, verify it, deposit any error msg in diags
//           dltArea = result of driveUnpack()
//           return: < 0 if an error occurred,
//                   dltArea->num_descriptors, otherwise.

// read and return desccriptor area (headers + entries)
SQLCLI_LIB_FUNC Int32
readDescArea
(fstream              &mf,          // (IN) : binary module file
 module_header_struct &latestModHdr,// (IN) : its module header
 NAHeap               &heap,        // (IN) : allocate DESC area from here
 const char *         name,         // (IN) : module name (for error msg)
 ComDiagsArea         &diags,       // (IN) : deposit any error msg here
 char                *&descArea);   // (OUT): desc headers + entries
// requires: mf is the binary module file (open for reading)
//           latestModHdr is the result of readModuleHeader()
// modifies: heap, diags, descArea
// effects:  allocate space for descriptor (DESC) area
//           read DESC area from mf, deposit any error msg in diags
//           return: < 0 if an error occurred,
//                   >=0 otherwise

// read and return a binary module file's source area
SQLCLI_LIB_FUNC SourceBuf
readSourceArea
(fstream              &mf,          // (IN) : binary module file
 module_header_struct &latestModHdr,// (IN) : its module header
 NAHeap               &heap,        // (IN) : allocate source area from here
 const char *         name,        // (IN) : module name (for error msg)
 ComDiagsArea         &diags);      // (IN) : deposit any error msg here
// requires: mf is the binary module file (open for reading)
//           latestModHdr is the result of readModuleHeader()
// modifies: heap, diags
// effects:  allocate space for source area
//           read and return source area of mf if all OK
//           return NULL otherwise

#endif
